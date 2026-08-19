#!/usr/bin/env python3
"""Verify that a Python plugin can store dzBar range definitions in DeviceStatus.Color.

Purpose
-------
DeviceStatus.Color is dual purpose. It holds RGB/WW color state for color
switches, and it holds the bar range definitions rendered by the utility,
temperature and weather cards (www/app/widgets/dzBar.js). The utility card
reads a bare array, the temperature and weather cards read a keyed object.

The plugin framework predates the second use and pushed every Color write
through _tColor (hardware/ColorSwitch.cpp). _tColor::fromJSON leaves the mode
at ColorModeNone unless the payload is a JSON object carrying a valid "m"
member, and toJSONString() returns the empty string for ColorModeNone, so a
range payload was blanked before it ever reached the database. A plugin could
create a device whose card fully supports bars, but could never fill the bands
in; the user had to type every one by hand.

Plugin Color writes now go through Plugins::NormalizeDeviceColor
(hardware/plugins/PythonPluginUtils.h), which normalizes genuine color state
as before, stores any other well-formed JSON verbatim, and still rejects
anything that is not JSON.

This script drives that through a small Python plugin
(plugins/examples/ColorRangeTest/, copied into the test instance's own
throwaway userdata/plugins/ folder for the run) which updates its devices the
way any real plugin does: Domoticz.Device.Update(Color=...) ->
CSQLHelper::UpdateDeviceValue. It checks five payloads:

  1. A bare range array (utility card shape) is stored verbatim.
  2. A keyed range object (temperature/weather card shape) is stored verbatim.
  3. A range payload whose color string contains a single quote and SQL
     fragments is stored verbatim AND leaves every other device untouched.
     CSQLHelper::UpdateDeviceValue builds its UPDATE with sqlite3_mprintf, and
     its value used to be interpolated with %s, which does not escape quotes;
     a payload could close the string literal and take over the trailing WHERE
     clause, writing rows belonging to other hardware. Normalization used to be
     the only thing keeping raw strings out of that statement, so relaxing it
     without fixing the escaping would have opened the hole. JSON validity is
     not a defence here: a single quote is perfectly legal inside a JSON string.
  4. Genuine RGB color state is still normalized through _tColor rather than
     passed through, so color switches keep behaving exactly as before.
  5. A payload that is not JSON at all is still rejected to the empty string.

Usage
-----
    python test_plugin_color_ranges.py <path-to-domoticz[.exe]>

It starts its own Domoticz on a free port with a throwaway database and
userdata folder (own throwaway plugins/ subfolder too), so it never touches
an existing installation or its data.
"""
import base64
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

HTYPE_DUMMY = 15
HTYPE_PYTHON_PLUGIN = 94

TYPE_TEMP = 80            # pTypeTEMP
SUB_TEMP5 = 5             # sTypeTEMP5

PLUGIN_KEY = "ColorRangeTest"

# Plugin device units -- fixed contract with plugins/examples/ColorRangeTest/plugin.py
UNIT_TRIGGER = 1
UNIT_UTILITY = 2
UNIT_TEMPERATURE = 3
UNIT_QUOTED = 4
UNIT_COLOUR_STATE = 5
UNIT_GARBAGE = 6
PLUGIN_UNIT_COUNT = 6

# Payloads the plugin assigns -- fixed contract with plugin.py, keep both in sync.
PAYLOAD_UTILITY = '[{"from":0,"to":80,"color":"#66bb6a"},{"from":80,"to":100,"color":"#ef5350"}]'
PAYLOAD_TEMPERATURE = '{"temp":[{"from":-10,"to":18,"color":"#42a5f5"},{"from":18,"to":40,"color":"#ef5350"}]}'
PAYLOAD_QUOTED = '[{"from":0,"to":10,"color":"#fff\' , Name=\'INJECTED\' WHERE 1=1 -- "}]'
PAYLOAD_COLOUR_STATE = '{"b":255,"cw":0,"g":128,"m":3,"r":10,"t":0,"ww":0}'
PAYLOAD_GARBAGE = 'not json at all'

CHECKS = 0
FAILURES = 0
SKIPPED = 0


def check(cond, label, detail=""):
    global CHECKS, FAILURES
    CHECKS += 1
    if cond:
        print("  PASS  %s" % label)
    else:
        FAILURES += 1
        print("  FAIL  %s%s" % (label, ("\n          %s" % detail) if detail else ""))
    return bool(cond)


def skip(label, note):
    global SKIPPED
    SKIPPED += 1
    print("  SKIP  %s -- %s" % (label, note))


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def repo_root(exe):
    """Walk up from the executable looking for the checkout root."""
    path = os.path.dirname(os.path.abspath(exe))
    for _ in range(6):
        if os.path.isdir(os.path.join(path, "www")) and os.path.isdir(os.path.join(path, "main")):
            return path
        path = os.path.dirname(path)
    return None


class EarlyExit(RuntimeError):
    pass


def domoticz_running():
    """True if some other domoticz already holds the single-instance mutex."""
    try:
        out = subprocess.run(["pgrep", "-x", "domoticz"], capture_output=True, timeout=10)
        return out.returncode == 0
    except Exception:
        return False


def wait_for_no_domoticz(timeout=60):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if not domoticz_running():
            return True
        time.sleep(1)
    return not domoticz_running()


def start_domoticz(exe, www_root, plugin_source_dir, attempts=5):
    """Start Domoticz, waiting out the single-instance mutex and retrying."""
    last = None
    for i in range(attempts):
        if not wait_for_no_domoticz():
            print("  (another domoticz is still running; the single-instance "
                  "mutex will refuse this start)")
        try:
            return Domoticz(exe, www_root, plugin_source_dir)
        except EarlyExit as e:
            last = e
            print("  (startup attempt %d/%d failed: %s -- retrying)" % (i + 1, attempts, e))
            time.sleep(3)
    raise RuntimeError("domoticz would not start after %d attempts: %s" % (attempts, last))


class Domoticz:
    """A throwaway Domoticz instance: own port, own database, own userdata.

    If plugin_source_dir is given, its contents are copied into this instance's
    own userdata/plugins/ColorRangeTest/ folder before startup, since Domoticz
    scans <userdata>/plugins/<Name>/plugin.py once at boot
    (CPluginSystem::BuildManifest()) -- it never touches the checkout's own
    plugins/ directory.
    """

    def __init__(self, exe, www_root, plugin_source_dir=None):
        self.exe = os.path.abspath(exe)
        self.port = free_port()
        self.tmp = tempfile.mkdtemp(prefix="domo_colorrange_")
        self.db = os.path.join(self.tmp, "domoticz.db")
        self.log = open(os.path.join(self.tmp, "domoticz.log"), "w", encoding="utf-8",
                        errors="replace")
        self.cookie_header: "str | None" = None  # set by login() once an admin session exists

        if plugin_source_dir and os.path.isdir(plugin_source_dir):
            dest = os.path.join(self.tmp, "plugins", PLUGIN_KEY)
            os.makedirs(dest, exist_ok=True)
            for fn in os.listdir(plugin_source_dir):
                src_f = os.path.join(plugin_source_dir, fn)
                if os.path.isfile(src_f):
                    shutil.copy2(src_f, os.path.join(dest, fn))

        args = [
            self.exe,
            "-www", str(self.port),
            "-wwwbind", "127.0.0.1",
            "-sslwww", "0",
            "-dbase", self.db,
            "-userdata", self.tmp,
            "-wwwroot", os.path.join(www_root, "www"),
            "-nobrowser",
            "-noupdates",
            "-loglevel", "debug",
        ]
        self.proc = subprocess.Popen(args, stdout=self.log, stderr=subprocess.STDOUT,
                                     cwd=www_root)
        self._wait_ready()

    def _wait_ready(self):
        deadline = time.time() + 90
        last = None
        while time.time() < deadline:
            if self.proc.poll() is not None:
                raise EarlyExit("domoticz exited early (rc=%s); log in %s"
                                % (self.proc.returncode, self.tmp))
            try:
                code, body = self.get("/json.htm?type=command&param=getversion", timeout=3)
                if code == 200:
                    return
                last = "status %s" % code
            except Exception as e:  # not up yet
                last = str(e)
            time.sleep(0.5)
        raise RuntimeError("domoticz did not become ready (last: %s); log in %s"
                           % (last, self.tmp))

    def url(self, path):
        return "http://127.0.0.1:%d%s" % (self.port, path)

    def get(self, path, timeout=15, headers=None):
        req = urllib.request.Request(self.url(path), headers=headers or {})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.status, r.read()
        except urllib.error.HTTPError as e:
            return e.code, e.read()

    def post(self, path, form, timeout=15):
        """POST url-encoded form data; return (status, body, set_cookie_values)."""
        data = urllib.parse.urlencode(form).encode()
        req = urllib.request.Request(
            self.url(path), data=data, method="POST",
            headers={"Content-Type": "application/x-www-form-urlencoded"})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.status, r.read(), r.headers.get_all("Set-Cookie") or []
        except urllib.error.HTTPError as e:
            return e.code, e.read(), e.headers.get_all("Set-Cookie") or []

    def stop(self):
        try:
            self.proc.terminate()
            self.proc.wait(timeout=30)
        except Exception:
            try:
                self.proc.kill()
                self.proc.wait(timeout=10)
            except Exception:
                pass
        try:
            self.log.close()
        except Exception:
            pass

    def cleanup(self):
        self.stop()
        if os.environ.get("KEEP_TESTDATA"):
            print("KEEP_TESTDATA set, leaving %s in place" % self.tmp)
            return
        shutil.rmtree(self.tmp, ignore_errors=True)

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.cleanup()


def login(domo, username="admin", password="color-range-test-pw"):
    """Provision the first admin user and log in.

    A fresh, never-configured database has zero users, and every admin command
    (addhardware, createdevice, switchlight) is rejected outright without a
    session. Returns the "Cookie: ..." header value, or None if login failed.
    """
    domo.post("/json.htm?type=command&param=setupwizardcreateadmin",
              {"username": username, "password": password})

    creds = {
        "username": base64.b64encode(username.encode()).decode(),
        "password": password,
    }
    code, body, set_cookies = domo.post("/json.htm?type=command&param=logincheck", creds)
    for value in set_cookies:
        cookie = value.split(";")[0]
        if not cookie.endswith("=none"):
            return cookie
    return None


def cmd(domo, path, timeout=15):
    """GET a json.htm command (with the admin session cookie, if any) and
    return (http_status, parsed_json_or_empty_dict)."""
    headers = {"Cookie": domo.cookie_header} if getattr(domo, "cookie_header", None) else None
    code, body = domo.get(path, timeout=timeout, headers=headers)
    try:
        data = json.loads(body.decode("utf-8", "replace"))
    except Exception:
        data = {}
    return code, data


def get_device(domo, idx):
    code, data = cmd(domo, "/json.htm?type=command&param=getdevices&rid=%s" % idx)
    result = data.get("result") or []
    return result[0] if result else None


def device_color(domo, idx):
    """Stored Color for a device.

    getdevices (main/WebServer.cpp) serializes the raw column for everything
    except actual color-switch types, which are the ones it re-renders through
    _tColor. None of the devices here are color switches, so this is the value
    as stored.
    """
    dev = get_device(domo, idx)
    if dev is None:
        return None
    return dev.get("Color")


def add_dummy_hardware(domo, name):
    path = ("/json.htm?type=command&param=addhardware&htype=%d&name=%s"
            "&enabled=true&port=0&address=&username=&password=&extra=&datatimeout=0"
            % (HTYPE_DUMMY, urllib.parse.quote(name)))
    code, data = cmd(domo, path)
    if data.get("status") != "OK" or "idx" not in data:
        raise RuntimeError("addhardware (Dummy) failed: %r" % data)
    return int(data["idx"])


def create_device(domo, hw_idx, name, devtype, subtype):
    path = ("/json.htm?type=command&param=createdevice&idx=%d&sensorname=%s"
            "&devicetype=%d&devicesubtype=%d"
            % (hw_idx, urllib.parse.quote(name), devtype, subtype))
    code, data = cmd(domo, path)
    if data.get("status") != "OK" or "idx" not in data:
        raise RuntimeError("createdevice failed for %s: %r" % (name, data))
    return int(data["idx"])


def push_udevice(domo, idx, nvalue, svalue):
    path = ("/json.htm?type=command&param=udevice&idx=%d&nvalue=%d&svalue=%s"
            % (idx, nvalue, urllib.parse.quote(svalue)))
    code, data = cmd(domo, path)
    if data.get("status") != "OK":
        raise RuntimeError("udevice update failed for idx %d: %r" % (idx, data))


def wait_for_plugin_devices(domo, hw_idx, expected_count, timeout=60):
    """Map plugin Unit -> device idx, waiting for the plugin to create them."""
    deadline = time.time() + timeout
    found = {}
    while time.time() < deadline:
        code, data = cmd(domo, "/json.htm?type=command&param=getdevices&filter=all&used=all")
        found = {}
        for dev in data.get("result") or []:
            if str(dev.get("HardwareID")) == str(hw_idx):
                found[int(dev.get("Unit", -1))] = dev.get("idx")
        if len(found) >= expected_count:
            return found
        time.sleep(0.5)
    return found


def wait_for_trigger_state(domo, trigger_idx, state, timeout=30):
    deadline = time.time() + timeout
    while time.time() < deadline:
        dev = get_device(domo, trigger_idx)
        if dev and (dev.get("Status") == state or dev.get("Data") == state):
            return True
        time.sleep(0.5)
    return False


def apply_payloads(domo, trigger_idx):
    """Flip the plugin's trigger switch so it assigns its fixed Color payloads."""
    code, data = cmd(domo, "/json.htm?type=command&param=switchlight&idx=%s&switchcmd=On"
                           % trigger_idx)
    if data.get("status") != "OK":
        return "could not flip the ColorRangeTest trigger switch: %r" % data
    if not wait_for_trigger_state(domo, trigger_idx, "On"):
        return ("trigger switch never reported On -- the plugin worker thread may not "
                "have processed the switchlight command")
    return None


def wait_for_color(domo, idx, timeout=30):
    """Wait until a device reports a non-empty Color, then return it.

    The plugin applies its payloads on its own worker thread, so the write can
    land slightly after the trigger switch reports On.
    """
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        last = device_color(domo, idx)
        if last:
            return last
        time.sleep(0.5)
    return last


def run_cases(domo, plugin_source_dir):
    print("\n=== Python plugin Color path (Device.Update(Color=) -> UpdateDeviceValue) ===")

    # An unrelated device belonging to different hardware. Nothing the plugin does
    # may touch this row -- it is the witness for the injection case.
    dummy_hw = add_dummy_hardware(domo, "ColorRangeWitness")
    witness_idx = create_device(domo, dummy_hw, "Witness", TYPE_TEMP, SUB_TEMP5)
    push_udevice(domo, witness_idx, 0, "19.5")
    witness_before = get_device(domo, witness_idx)
    if witness_before is None:
        print("  ERROR: witness device could not be read back; cannot run cases")
        return
    before_name = witness_before.get("Name")
    before_color = witness_before.get("Color")

    path = ("/json.htm?type=command&param=addhardware&htype=%d&name=%s"
            "&enabled=true&port=0&address=&username=&password=&extra=%s&datatimeout=0"
            % (HTYPE_PYTHON_PLUGIN, urllib.parse.quote(PLUGIN_KEY),
               urllib.parse.quote(PLUGIN_KEY)))
    code, data = cmd(domo, path)
    if data.get("status") != "OK" or "idx" not in data:
        skip("all cases", "could not add ColorRangeTest plugin hardware: %r" % data)
        return
    plugin_hw_idx = int(data["idx"])

    unit_idx = wait_for_plugin_devices(domo, plugin_hw_idx, PLUGIN_UNIT_COUNT)
    if UNIT_TRIGGER not in unit_idx:
        skip("all cases",
             "ColorRangeTest plugin did not create its devices (got %d within 60s) -- this "
             "Domoticz build may not have Python plugin support enabled, or the plugin "
             "failed to start (check the instance log)" % len(unit_idx))
        return

    problem = apply_payloads(domo, unit_idx[UNIT_TRIGGER])
    if problem:
        skip("all cases", problem)
        return

    unit_idx = wait_for_plugin_devices(domo, plugin_hw_idx, PLUGIN_UNIT_COUNT)
    missing = [u for u in (UNIT_UTILITY, UNIT_TEMPERATURE, UNIT_QUOTED,
                           UNIT_COLOUR_STATE, UNIT_GARBAGE) if u not in unit_idx]
    if missing:
        skip("all cases", "plugin devices missing for unit(s) %s"
             % ", ".join(str(u) for u in missing))
        return

    # 1. Utility card shape: a bare array, stored verbatim.
    got = wait_for_color(domo, unit_idx[UNIT_UTILITY])
    check(got == PAYLOAD_UTILITY,
          "utility range array reaches the database unchanged",
          "expected %r\n          observed %r" % (PAYLOAD_UTILITY, got))

    # 2. Temperature/weather card shape: a keyed object, stored verbatim.
    got = wait_for_color(domo, unit_idx[UNIT_TEMPERATURE])
    check(got == PAYLOAD_TEMPERATURE,
          "keyed temperature range object reaches the database unchanged",
          "expected %r\n          observed %r" % (PAYLOAD_TEMPERATURE, got))

    # 3. A quote-bearing payload is stored verbatim and changes nothing else.
    got = wait_for_color(domo, unit_idx[UNIT_QUOTED])
    check(got == PAYLOAD_QUOTED,
          "quote-bearing range payload is stored verbatim",
          "expected %r\n          observed %r" % (PAYLOAD_QUOTED, got))

    witness_after = get_device(domo, witness_idx)
    check(witness_after is not None and witness_after.get("Name") == before_name,
          "quote-bearing payload does not rename an unrelated device",
          "Name before %r, after %r -- the UPDATE statement's WHERE clause was taken over"
          % (before_name, witness_after.get("Name") if witness_after else None))
    check(witness_after is not None and witness_after.get("Color") == before_color,
          "quote-bearing payload does not overwrite an unrelated device's Color",
          "Color before %r, after %r" % (before_color,
                                         witness_after.get("Color") if witness_after else None))

    # 4. Genuine color state is still normalized through _tColor.
    got = wait_for_color(domo, unit_idx[UNIT_COLOUR_STATE])
    parsed = None
    try:
        parsed = json.loads(got) if got else None
    except Exception:
        parsed = None
    check(isinstance(parsed, dict)
          and parsed.get("m") == 3 and parsed.get("r") == 10
          and parsed.get("g") == 128 and parsed.get("b") == 255,
          "RGB color state is still normalized through _tColor",
          "expected a color object with m=3 r=10 g=128 b=255, observed %r" % (got,))

    # 5. A non-JSON payload is still rejected.
    got = device_color(domo, unit_idx[UNIT_GARBAGE])
    check(got == "",
          "a payload that is not JSON is still rejected",
          "expected '' , observed %r" % (got,))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        print(__doc__)
        return 2
    exe = args[0]
    if not os.path.exists(exe):
        print("domoticz executable not found: %s" % exe)
        return 2
    root = repo_root(exe)
    if not root:
        print("could not locate the Domoticz checkout from %s" % exe)
        return 2
    print("checkout: %s" % root)
    print("testing:  %s" % os.path.abspath(exe))

    plugin_source_dir = os.path.join(root, "plugins", "examples", PLUGIN_KEY)
    if not os.path.isdir(plugin_source_dir):
        print("ERROR: %s not found in checkout" % plugin_source_dir)
        return 2

    with start_domoticz(exe, root, plugin_source_dir) as domo:
        print("domoticz running on port %d (temp data in %s)" % (domo.port, domo.tmp))

        domo.cookie_header = login(domo)
        if not domo.cookie_header:
            print("ERROR: could not create/log in as an admin user; every case below "
                  "needs admin rights for addhardware/createdevice/switchlight")
            return 2

        run_cases(domo, plugin_source_dir)

    print("\n%d checks, %d failure(s), %d skipped" % (CHECKS, FAILURES, SKIPPED))
    return 0 if FAILURES == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
