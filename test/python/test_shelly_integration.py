"""End to end test of the native Shelly integration against mock devices.

It starts mock Shelly devices (mini_shelly_server.py) on 127.0.0.1, starts a scratch Domoticz on port 8097
with its own database and user data folder (never an existing installation), adds Shelly hardware (type 130)
through the JSON API and adds the mock devices by address with the shellyaddnode command. It then checks the
Domoticz devices that were created, that a command reaches the mock device, that a pushed status (a Gen2+
WebSocket NotifyStatus) and a polled status (Gen1) update a device, that authentication works and that a
rejected password is reported, and that the devices keep their identity across a restart.

    pytest test/python/test_shelly_integration.py

Environment:
    DOMOTICZ_EXE        the Domoticz executable (default: msbuild/x64/Debug/domoticz.exe, then the
                        Release build and a build in the checkout root)
    SHELLY_TEST_PORT    web port of the scratch instance (default: 8097 when free, else a free port)
    KEEP_TESTDATA=1     leave the scratch database and folder behind

The tests are skipped when no executable is found by the default search. They fail (they are not skipped) when
DOMOTICZ_EXE is set and does not exist or does not start, when SHELLY_TEST_PORT is set and the port is in use,
or when an instance that was found cannot be started. No real device is ever contacted: every address is on the
loopback interface.
"""
import base64
import json
import os
import shutil
import socket
import sqlite3
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

import pytest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mini_shelly_server import MiniShellyServer  # noqa: E402

HTYPE_SHELLY = 130
ADMIN_USER = "admin"
ADMIN_PASS = "shelly-test-pw"
GOOD_PASSWORD = "secret"
WRONG_PASSWORD = "not-the-password"
POLL_SECONDS = 5

# Domoticz device types used below
pTypeLighting2 = 17
pTypeGeneralSwitch = 244
pTypeColorSwitch = 241
pTypeTEMP = 80
pTypeTEMP_HUM = 82

# (node name, fixture, MAC, hardware key). The MACs are made up and unique in their last 24 bits.
NODES = [
    ("T2PM", "2pm_gen3_cover", "0A1B2C0A0001", "open"),
    ("TDUO", "duo_bulb_gen3", "0A1B2C0A0002", "open"),
    ("TPRESENCE", "presence_gen4", "0A1B2C0A0004", "open"),
    ("TWD", "wall_display_xl", "0A1B2C0A0005", "open"),
    ("TGEM", "shelly_em_gen1", None, "open"),
    ("TGHT", "shelly_ht_gen1", None, "open"),
    ("TG1PM", "shelly_1pm_gen1", None, "open"),
    ("TRGBW", "rgbw2", None, "open"),
    ("TPRO3EM", "pro_3em", "0A1B2C0A0003", "auth"),
    ("TG1PMAUTH", "shelly_1pm_gen1", "C45BBE0A0113", "auth"),
    ("TPRO3EMBAD", "pro_3em", "0A1B2C0A0006", "wrong"),
    ("TG1PMBAD", "shelly_1pm_gen1", "C45BBE0A0114", "wrong"),
]
# Devices that need a password on the mock; the hardware "auth" has the right one, "wrong" a wrong one
NEEDS_PASSWORD = {"auth", "wrong"}


# ---- locating and starting Domoticz ---------------------------------------------------------------------------

def repo_root():
    return os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))


def find_exe():
    env = os.environ.get("DOMOTICZ_EXE")
    if env:
        if not os.path.isfile(env):
            pytest.fail("DOMOTICZ_EXE is set but %r does not exist" % env)
        return env
    root = repo_root()
    for rel in ("msbuild/x64/Debug/domoticz.exe", "msbuild/x64/Release/domoticz.exe", "domoticz.exe", "domoticz", "build/domoticz"):
        path = os.path.join(root, *rel.split("/"))
        if os.path.isfile(path):
            return path
    return None


def port_in_use(port):
    s = socket.socket()
    try:
        s.settimeout(1)
        return s.connect_ex(("127.0.0.1", port)) == 0
    finally:
        s.close()


def free_port():
    s = socket.socket()
    try:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]
    finally:
        s.close()


def choose_port():
    """The port of the scratch instance: SHELLY_TEST_PORT when set (it must be free), else 8097 or any free port."""
    explicit = os.environ.get("SHELLY_TEST_PORT")
    if explicit:
        port = int(explicit)
        if port_in_use(port):
            pytest.fail("SHELLY_TEST_PORT=%d is in use" % port)
        return port
    return 8097 if not port_in_use(8097) else free_port()


class Domoticz:
    """A scratch Domoticz instance with its own port, database and user data folder."""

    def __init__(self, exe, port):
        self.exe = os.path.abspath(exe)
        self.port = port
        self.tmp = tempfile.mkdtemp(prefix="domo_shelly_")
        self.db = os.path.join(self.tmp, "domoticz.db")
        self.cookie = None
        self.proc = None
        self.log = None

    def start(self):
        self.log = open(os.path.join(self.tmp, "domoticz.out"), "a", encoding="utf-8", errors="replace")
        args = [self.exe, "-www", str(self.port), "-wwwbind", "127.0.0.1", "-sslwww", "0", "-dbase", self.db,
                "-userdata", self.tmp, "-wwwroot", os.path.join(repo_root(), "www"), "-nobrowser", "-noupdates",
                "-loglevel", "all"]
        self.proc = subprocess.Popen(args, stdout=self.log, stderr=subprocess.STDOUT, cwd=repo_root())
        deadline = time.time() + 90
        while time.time() < deadline:
            if self.proc.poll() is not None:
                raise RuntimeError("domoticz exited early (rc=%s), see %s" % (self.proc.returncode, self.tmp))
            try:
                code, _ = self.get("/json.htm?type=command&param=getversion", timeout=3)
                if code == 200:
                    return
            except Exception:
                pass
            time.sleep(0.5)
        raise RuntimeError("domoticz did not become ready, see %s" % self.tmp)

    def stop(self):
        if self.proc is not None:
            try:
                self.proc.terminate()
                self.proc.wait(timeout=30)
            except Exception:
                try:
                    self.proc.kill()
                    self.proc.wait(timeout=10)
                except Exception:
                    pass
            self.proc = None
        if self.log is not None:
            try:
                self.log.close()
            except Exception:
                pass
            self.log = None

    def cleanup(self):
        self.stop()
        if os.environ.get("KEEP_TESTDATA"):
            print("KEEP_TESTDATA set, leaving %s in place" % self.tmp)
        else:
            shutil.rmtree(self.tmp, ignore_errors=True)

    def url(self, path):
        return "http://127.0.0.1:%d%s" % (self.port, path)

    def get(self, path, timeout=15, headers=None):
        req = urllib.request.Request(self.url(path), headers=headers or {})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.status, r.read()
        except urllib.error.HTTPError as e:
            return e.code, e.read()

    def post(self, path, form):
        req = urllib.request.Request(self.url(path), data=urllib.parse.urlencode(form).encode(), method="POST",
                                     headers={"Content-Type": "application/x-www-form-urlencoded"})
        try:
            with urllib.request.urlopen(req, timeout=15) as r:
                return r.status, r.read(), r.headers.get_all("Set-Cookie") or []
        except urllib.error.HTTPError as e:
            return e.code, e.read(), e.headers.get_all("Set-Cookie") or []

    def login(self):
        """Create the first admin user (a no-op when it exists) and log in."""
        self.post("/json.htm?type=command&param=setupwizardcreateadmin", {"username": ADMIN_USER, "password": ADMIN_PASS})
        creds = {"username": base64.b64encode(ADMIN_USER.encode()).decode(), "password": ADMIN_PASS}
        _, _, cookies = self.post("/json.htm?type=command&param=logincheck", creds)
        for value in cookies:
            cookie = value.split(";")[0]
            if not cookie.endswith("=none"):
                self.cookie = cookie
                return
        raise RuntimeError("could not log in as the admin user")

    def cmd(self, query):
        """Run a json.htm command as the admin; returns the parsed JSON ({} when there is none)."""
        code, body = self.get("/json.htm?" + query, headers={"Cookie": self.cookie} if self.cookie else None)
        try:
            return json.loads(body.decode("utf-8", "replace"))
        except ValueError:
            return {}

    def restart(self):
        self.stop()
        self.start()
        self.login()

    # -- inspection --

    def devices(self, hardware_id):
        """All devices of a hardware, read from the database (the driver only writes through Domoticz)."""
        for _ in range(5):
            try:
                con = sqlite3.connect("file:%s?mode=ro" % self.db.replace("\\", "/"), uri=True, timeout=5)
                try:
                    cur = con.execute("SELECT ID, DeviceID, Unit, Type, SubType, SwitchType, Name, nValue, sValue FROM DeviceStatus WHERE HardwareID=? ORDER BY ID",
                                      (hardware_id,))
                    cols = ("idx", "DeviceID", "Unit", "Type", "SubType", "SwitchType", "Name", "nValue", "sValue")
                    return [dict(zip(cols, row)) for row in cur.fetchall()]
                finally:
                    con.close()
            except sqlite3.OperationalError:
                time.sleep(0.5)
        raise RuntimeError("the scratch database could not be read")

    def node_devices(self, hardware_id, name):
        """The devices named after a node: the node name alone or followed by a space (TGEM does not match TGEM2)."""
        return [d for d in self.devices(hardware_id) if d["Name"] == name or d["Name"].startswith(name + " ")]

    def log_text(self):
        lines = []
        for level in (1, 2, 4):
            lines += [m.get("message", "") for m in (self.cmd("type=command&param=getlog&loglevel=%d" % level).get("result") or [])]
        return "\n".join(lines)


def wait_for(condition, timeout=30, interval=0.5):
    deadline = time.time() + timeout
    while time.time() < deadline:
        value = condition()
        if value:
            return value
        time.sleep(interval)
    return condition()


# ---- fixtures -------------------------------------------------------------------------------------------------

class Env:
    pass


@pytest.fixture(scope="module")
def env():
    exe = find_exe()
    if not exe:
        pytest.skip("no Domoticz executable (set DOMOTICZ_EXE or build msbuild/x64/Debug)")
    port = choose_port()

    e = Env()
    e.mocks = {}
    domo = Domoticz(exe, port)
    e.domo = domo
    try:
        try:
            domo.start()
            domo.login()
        except RuntimeError as ex:
            pytest.fail("cannot start the scratch Domoticz: %s" % ex)

        # One hardware per credential: no password, the right one and a wrong one
        e.hardware = {}
        for key, password in (("open", ""), ("auth", GOOD_PASSWORD), ("wrong", WRONG_PASSWORD)):
            q = ("type=command&param=addhardware&htype=%d&name=%s&enabled=true&loglevel=7&port=0&address=&username=admin&password=%s"
                 "&extra=&datatimeout=0&Mode1=%d" % (HTYPE_SHELLY, "Shelly%20" + key, urllib.parse.quote(password), POLL_SECONDS))
            res = domo.cmd(q)
            assert res.get("status") == "OK", "addhardware failed: %r" % res
            e.hardware[key] = int(res["idx"])

        for name, fixture, mac, hw in NODES:
            mock = MiniShellyServer(fixture, mac=mac, password=GOOD_PASSWORD if hw in NEEDS_PASSWORD else None)
            if name == "T2PM":
                # the recorded cover is not calibrated (no position), so Domoticz would not create a blind for it
                mock.update_status({"cover:0": {"pos_control": True, "current_pos": 50}})
            mock.start()
            e.mocks[name] = mock
        e.added = {}
        for name, fixture, mac, hw in NODES:
            mock = e.mocks[name]
            query = "type=command&param=shellyaddnode&idx=%d&ip=127.0.0.1&port=%d&name=%s" % (e.hardware[hw], mock.port, name)
            # the hardware worker may still be starting: retry until the node is accepted
            res = {}

            def added():
                nonlocal res
                res = domo.cmd(query)
                return res.get("status") == "OK"
            wait_for(added, timeout=20)
            e.added[name] = res
        yield e
    finally:
        for mock in e.mocks.values():
            mock.stop()
        domo.cleanup()


def hw_of(env, name):
    return next(env.hardware[hw] for n, _f, _m, hw in NODES if n == name)


def snapshot(env, name):
    """The stable identity of the devices of a node: (DeviceID, Unit, Type, SubType, Name)."""
    return sorted((d["DeviceID"], d["Unit"], d["Type"], d["SubType"], d["Name"]) for d in env.domo.node_devices(hw_of(env, name), name))


def wait_devices(env, name, timeout=40):
    return wait_for(lambda: env.domo.node_devices(hw_of(env, name), name), timeout)


def find(devices, **match):
    return [d for d in devices if all(d[k] == v for k, v in match.items())]


# ---- discovery ------------------------------------------------------------------------------------------------

UNMAPPED = {}


@pytest.mark.parametrize("name", [n for n, _f, _m, hw in NODES if hw != "wrong"])
def test_node_is_added_and_devices_are_created(env, name):
    assert env.added[name].get("status") == "OK", env.added[name]
    assert wait_devices(env, name, timeout=20), "no devices were created for %s" % name


def test_node_identity_is_reported(env):
    nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % env.hardware["open"]).get("result") or []
    by_name = {n["name"]: n for n in nodes}
    assert by_name["T2PM"]["gen"] == 3 and by_name["T2PM"]["id"] == "0A1B2C0A0001"
    assert by_name["T2PM"]["model"] == "S3SW-002P16EU"
    assert by_name["TPRESENCE"]["gen"] == 4
    assert by_name["TWD"]["gen"] == 2
    assert by_name["TGEM"]["gen"] == 1 and by_name["TGEM"]["model"] == "SHEM"
    assert by_name["TGHT"]["sleepy"] is True


def test_gen2_cover_is_a_percentage_blind(env):
    devices = wait_devices(env, "T2PM")
    covers = find(devices, Name="T2PM Cover 1")
    assert len(covers) == 1
    assert covers[0]["Type"] == pTypeGeneralSwitch and covers[0]["DeviceID"] == "000A0001"


def test_gen2_switch_is_lighting2(env):
    devices = wait_devices(env, "TWD")
    assert find(devices, Type=pTypeLighting2), devices
    assert find(devices, Type=pTypeLighting2)[0]["DeviceID"] == "00A0005"


def test_gen2_cct_is_a_dimmer(env):
    devices = wait_devices(env, "TDUO")
    lights = find(devices, Type=pTypeLighting2)
    assert len(lights) == 1, devices


def test_gen2_cct_command_reaches_the_device(env):
    dev = find(wait_devices(env, "TDUO"), Type=pTypeLighting2)[0]
    mock = env.mocks["TDUO"]
    res = env.domo.cmd("type=command&param=switchlight&idx=%d&switchcmd=Set%%20Level&level=8" % dev["idx"])
    assert res.get("status") == "OK", res
    calls = wait_for(lambda: mock.find_requests("CCT.Set"))
    assert calls, "no CCT.Set reached the mock"
    assert calls[-1]["params"]["id"] == 0 and calls[-1]["params"]["on"] is True
    assert 1 <= calls[-1]["params"]["brightness"] <= 100, calls[-1]["params"]


def test_gen4_presence_zones_are_motion_sensors(env):
    devices = wait_devices(env, "TPRESENCE")
    zones = find(devices, Type=pTypeGeneralSwitch)
    assert len(zones) == 3, devices
    states = sorted(z["nValue"] for z in zones)
    assert states == [0, 1, 1], zones
    # illuminance without a lux value makes no device
    assert len(devices) == 3, devices


def test_gen2_three_phase_meter_has_a_device_per_phase(env):
    devices = wait_for(lambda: len(env.domo.node_devices(hw_of(env, "TPRO3EM"), "TPRO3EM")) >= 3 and env.domo.node_devices(hw_of(env, "TPRO3EM"), "TPRO3EM"))
    assert devices, "the three phases did not appear"


def test_gen1_em_has_relay_and_two_meters(env):
    devices = wait_devices(env, "TGEM")
    assert find(devices, Type=pTypeLighting2), "the relay is missing"
    assert len(devices) >= 3, devices


def test_gen1_sleepy_ht_is_a_temperature_humidity_sensor(env):
    devices = wait_devices(env, "TGHT")
    assert find(devices, Type=pTypeTEMP_HUM), devices
    row = find(devices, Type=pTypeTEMP_HUM)[0]
    assert row["sValue"].startswith("22.5;45")


def test_gen1_rgbw2_is_a_color_light(env):
    devices = wait_devices(env, "TRGBW")
    assert find(devices, Type=pTypeColorSwitch), devices


# ---- commands -------------------------------------------------------------------------------------------------

def test_gen2_switch_command_reaches_the_device(env):
    devices = wait_devices(env, "TWD")
    dev = find(devices, Type=pTypeLighting2)[0]
    mock = env.mocks["TWD"]
    want_on = not mock.status["switch:0"]["output"]
    res = env.domo.cmd("type=command&param=switchlight&idx=%d&switchcmd=%s" % (dev["idx"], "On" if want_on else "Off"))
    assert res.get("status") == "OK", res
    calls = wait_for(lambda: mock.find_requests("Switch.Set"))
    assert calls, "the command did not reach the mock"
    assert calls[-1]["params"] == {"id": 0, "on": want_on}
    # the mock pushes the new state back, so the device follows
    assert wait_for(lambda: env.domo.node_devices(hw_of(env, "TWD"), "TWD") and find(env.domo.node_devices(hw_of(env, "TWD"), "TWD"), idx=dev["idx"])[0]["nValue"] == (1 if want_on else 0))


def test_gen2_cover_position_command_reaches_the_device(env):
    dev = find(wait_devices(env, "T2PM"), Name="T2PM Cover 1")[0]
    mock = env.mocks["T2PM"]
    res = env.domo.cmd("type=command&param=switchlight&idx=%d&switchcmd=Set%%20Level&level=40" % dev["idx"])
    assert res.get("status") == "OK", res
    calls = wait_for(lambda: mock.find_requests("Cover.GoToPosition"))
    assert calls, "no Cover.GoToPosition reached the mock"
    assert calls[-1]["params"]["id"] == 0
    assert calls[-1]["params"]["pos"] in (40, 60), calls[-1]["params"]


def test_gen1_relay_command_reaches_the_device(env):
    dev = find(wait_devices(env, "TG1PM"), Type=pTypeLighting2)[0]
    mock = env.mocks["TG1PM"]
    res = env.domo.cmd("type=command&param=switchlight&idx=%d&switchcmd=Off" % dev["idx"])
    assert res.get("status") == "OK", res
    calls = wait_for(lambda: mock.find_requests(target_prefix="/relay/0?turn=off"))
    assert calls, [r["target"] for r in mock.requests]
    # a Gen1 device is polled, so the state follows at the next poll
    assert wait_for(lambda: find(env.domo.node_devices(hw_of(env, "TG1PM"), "TG1PM"), idx=dev["idx"])[0]["nValue"] == 0, timeout=POLL_SECONDS * 4)


# ---- status updates -------------------------------------------------------------------------------------------

def test_gen2_websocket_push_updates_a_device(env):
    mock = env.mocks["TWD"]
    assert wait_for(lambda: mock.ws_client_count() >= 1), "the driver did not open a WebSocket"
    dev = find(wait_devices(env, "TWD"), Type=pTypeLighting2)[0]
    new_state = not mock.status["switch:0"]["output"]
    polls_before = len(mock.find_requests("Shelly.GetStatus"))
    assert mock.push_status({"switch:0": {"output": new_state}}) >= 1
    # well below the poll interval: only the push can have done this
    assert wait_for(lambda: find(env.domo.node_devices(hw_of(env, "TWD"), "TWD"), idx=dev["idx"])[0]["nValue"] == (1 if new_state else 0), timeout=POLL_SECONDS)
    assert len(mock.find_requests("Shelly.GetStatus")) <= polls_before + 1


def test_gen2_power_push_updates_a_meter(env):
    mock = env.mocks["TPRO3EM"]
    assert wait_for(lambda: mock.ws_client_count() >= 1), "the driver did not open an authenticated WebSocket"
    before = {d["idx"]: d["sValue"] for d in env.domo.node_devices(hw_of(env, "TPRO3EM"), "TPRO3EM")}
    mock.push_status({"em:0": {"a_act_power": 4321.0, "a_voltage": 229.9, "a_current": 18.8, "a_aprt_power": 4321.0, "a_pf": 1.0}})

    def changed():
        now = {d["idx"]: d["sValue"] for d in env.domo.node_devices(hw_of(env, "TPRO3EM"), "TPRO3EM")}
        return any(v != before.get(k) and "4321" in v for k, v in now.items())
    assert wait_for(changed, timeout=POLL_SECONDS * 2)


def test_gen1_poll_updates_a_device(env):
    mock = env.mocks["TGHT"]
    # a sleepy node is polled at most every 5 minutes, so use the always on EM instead
    mock = env.mocks["TGEM"]
    mock.update_status({"emeters": [{"power": 777.0}]})
    devices = env.domo.node_devices(hw_of(env, "TGEM"), "TGEM")
    assert devices
    assert wait_for(lambda: any("777" in d["sValue"] for d in env.domo.node_devices(hw_of(env, "TGEM"), "TGEM")), timeout=POLL_SECONDS * 4)


# ---- authentication -------------------------------------------------------------------------------------------

def test_gen2_digest_auth_is_used(env):
    mock = env.mocks["TPRO3EM"]
    assert mock.find_requests(auth="ok", proto="http"), "no digest authenticated HTTP request"
    assert mock.find_requests(auth="ok", proto="ws"), "no authenticated WebSocket request"
    assert not mock.find_requests(auth="bad")


def test_gen1_basic_auth_is_used(env):
    mock = env.mocks["TG1PMAUTH"]
    assert wait_devices(env, "TG1PMAUTH")
    assert mock.find_requests(auth="ok")
    assert not mock.find_requests(auth="bad")
    dev = find(wait_devices(env, "TG1PMAUTH"), Type=pTypeLighting2)[0]
    res = env.domo.cmd("type=command&param=switchlight&idx=%d&switchcmd=Off" % dev["idx"])
    assert res.get("status") == "OK", res
    calls = wait_for(lambda: mock.find_requests(target_prefix="/relay/0?turn=off", auth="ok"))
    assert calls, "the command was not sent with credentials"


def test_gen2_wrong_password_is_reported_and_creates_no_devices(env):
    mock = env.mocks["TPRO3EMBAD"]
    assert env.added["TPRO3EMBAD"].get("status") == "OK", env.added["TPRO3EMBAD"]
    assert wait_for(lambda: mock.find_requests(auth="bad"), timeout=20), "the driver never tried the password"
    assert wait_for(lambda: "authentication failed" in env.domo.log_text(), timeout=30), "the rejected password is not reported in the log"
    assert not mock.find_requests(auth="ok")
    assert env.domo.node_devices(env.hardware["wrong"], "TPRO3EMBAD") == []


def test_gen1_wrong_password_creates_no_devices(env):
    mock = env.mocks["TG1PMBAD"]
    assert wait_for(lambda: mock.find_requests(auth="bad"), timeout=20), "the driver never tried the password"
    # a full poll interval later the node must still not have been accepted
    seen = len(mock.find_requests(auth="bad"))
    assert wait_for(lambda: len(mock.find_requests(auth="bad")) > seen, timeout=POLL_SECONDS * 4), "the driver stopped polling"
    assert not mock.find_requests(auth="ok")
    assert env.domo.node_devices(env.hardware["wrong"], "TG1PMBAD") == []


def test_gen1_wrong_password_is_reported_once(env):
    assert wait_for(lambda: env.mocks["TG1PMBAD"].find_requests(auth="bad"), timeout=20)
    assert wait_for(lambda: "TG1PMBAD: authentication failed, check the password" in env.domo.log_text(), timeout=30)
    # two more rejected polls must have happened before the single message is counted
    seen = len(env.mocks["TG1PMBAD"].find_requests(auth="bad"))
    assert wait_for(lambda: len(env.mocks["TG1PMBAD"].find_requests(auth="bad")) >= seen + 2, timeout=POLL_SECONDS * 6), "the driver stopped polling"
    assert env.domo.log_text().count("TG1PMBAD: authentication failed, check the password") == 1


# ---- restart --------------------------------------------------------------------------------------------------

def test_devices_keep_their_identity_across_a_restart(env):
    names = [n for n, _f, _m, hw in NODES if hw != "wrong" and n not in UNMAPPED]
    before = {n: snapshot(env, n) for n in names}
    assert all(before.values())
    # let a move that is in flight settle (the identities stay the same for a moment), then restart on the same database
    assert wait_for(lambda: all(snapshot(env, n) == before[n] for n in names), timeout=10)
    env.domo.restart()
    for n in names:
        assert wait_for(lambda: snapshot(env, n) == before[n], timeout=40), "%s changed across the restart:\n%r\n%r" % (n, before[n], snapshot(env, n))
    # no node was lost and no device was duplicated
    wanted = sorted(n for n, _f, _m, hw in NODES if hw == "open")

    def node_names():
        nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % env.hardware["open"]).get("result") or []
        return sorted(n["name"] for n in nodes)
    assert wait_for(lambda: node_names() == wanted, timeout=40), node_names()
    # and the push channel is back
    assert wait_for(lambda: env.mocks["TWD"].ws_client_count() >= 1, timeout=40)


# ---- removal (keep last, it deletes a node) ---------------------------------------------------------------------

def test_renaming_a_node_renames_its_devices(env):
    hw = env.hardware["open"]
    nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % hw).get("result") or []
    node_id = {n["name"]: n["id"] for n in nodes}["TGEM"]
    before = env.domo.node_devices(hw, "TGEM")
    assert before
    res = env.domo.cmd("type=command&param=shellyrenamenode&idx=%d&nodeid=%s&name=TGEM2" % (hw, node_id))
    assert res.get("status") == "OK", res
    assert wait_for(lambda: len(env.domo.node_devices(hw, "TGEM2")) == len(before), timeout=20), [d["Name"] for d in env.domo.devices(hw)]
    nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % hw).get("result") or []
    assert {n["id"]: n["name"] for n in nodes}[node_id] == "TGEM2"


def test_removing_a_node_deletes_its_devices_and_only_those(env):
    hw = env.hardware["open"]
    nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % hw).get("result") or []
    node_id = {n["name"]: n["id"] for n in nodes}["TGEM2"]
    assert env.domo.node_devices(hw, "TGEM2"), "TGEM2 has no devices to remove"
    others = {n: len(env.domo.node_devices(hw, n)) for n in ("T2PM", "TWD")}
    assert all(others.values())

    res = env.domo.cmd("type=command&param=shellyremovenode&idx=%d&nodeid=%s" % (hw, node_id))
    assert res.get("status") == "OK", res
    assert wait_for(lambda: not env.domo.node_devices(hw, "TGEM2"), timeout=20), env.domo.node_devices(hw, "TGEM2")
    assert {n: len(env.domo.node_devices(hw, n)) for n in others} == others


def test_removing_a_node_deletes_its_temperature_sensor(env):
    hw = env.hardware["open"]
    nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % hw).get("result") or []
    node_id = {n["name"]: n["id"] for n in nodes}["TGHT"]
    before = env.domo.node_devices(hw, "TGHT")
    assert before, "TGHT has no devices to remove"
    temperature_ids = [d["idx"] for d in before if d["Type"] in (80, 81, 82)]
    assert temperature_ids, "TGHT has no temperature or humidity device: %r" % [(d["Name"], d["Type"]) for d in before]
    res = env.domo.cmd("type=command&param=shellyremovenode&idx=%d&nodeid=%s" % (hw, node_id))
    assert res.get("status") == "OK", res
    assert wait_for(lambda: not env.domo.node_devices(hw, "TGHT"), timeout=20), env.domo.node_devices(hw, "TGHT")
    left = {d["idx"] for d in env.domo.devices(hw)}
    assert not (set(temperature_ids) & left), "the temperature device of TGHT is still there"


# Keep this test last: it leaves "Accept new Hardware Devices" disabled in the scratch instance.
def test_adding_a_new_node_explains_that_new_hardware_is_disabled(env):
    hw = env.hardware["open"]
    env.domo.stop()
    con = sqlite3.connect(env.domo.db)
    try:
        assert con.execute("UPDATE Preferences SET nValue=0 WHERE Key='AcceptNewHardware'").rowcount == 1
        con.commit()
    finally:
        con.close()
    env.domo.start()
    env.domo.login()
    mock = MiniShellyServer("shelly_1pm_gen1", mac="0A1B2C0A0F0F")
    mock.start()
    try:
        res = {}

        def refused():
            nonlocal res
            res = env.domo.cmd("type=command&param=shellyaddnode&idx=%d&ip=127.0.0.1&port=%d&name=TNEW" % (hw, mock.port))
            return res.get("message", "").startswith("Accepting new hardware devices is disabled")
        assert wait_for(refused, timeout=30), res
        assert res.get("status") == "ERR"
        nodes = env.domo.cmd("type=command&param=shellygetnodes&idx=%d" % hw).get("result") or []
        assert "TNEW" not in [n["name"] for n in nodes]
    finally:
        mock.stop()
