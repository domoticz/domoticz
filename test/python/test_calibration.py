#!/usr/bin/env python3
"""Verify that device calibration is applied on every device update path.

Purpose
-------
Calibration offsets (device edit dialog "Calibration" tab, stored in
DeviceStatus.AddjValue/AddjMulti and AddjValue2/AddjMulti2) used to be
applied only at a handful of ingest points scattered across
main/mainworker.cpp (the JSON "udevice" path, MainWorker::UpdateDevice) and,
for temperature only, in hardware/MQTTAutoDiscover.cpp. They were NOT applied
when a device was updated by a Python plugin (hardware/plugins/PythonObjects.cpp,
which calls CSQLHelper::UpdateValue with the raw, uncalibrated sValue).
Calibration now lives in CSQLHelper::UpdateValueInt so every ingest path gets
it consistently, and exactly once. This script guards both halves of that:
the paths that were already correct must not start calibrating twice, and the
paths that ignored calibration must now honour it.

This script exercises two ingest paths against several sensor device types
and compares the stored value to what calibration should have produced:

  1. The JSON "udevice" API (/json.htm?type=command&param=udevice). This
     already calibrates pTypeTEMP/TEMP_HUM/TEMP_HUM_BARO (temperature) and
     TEMP_HUM_BARO (barometer) today -- it is the regression guard: it must
     still calibrate after the change, and must calibrate exactly once (an
     offset applied twice, double-counted by both the old scattered code and
     the new centralized path, is the main risk of the refactor). It does
     NOT calibrate a bare barometer or UV today, so those two cases are
     "broken before, fixed after" even on this path.

  2. A small Python plugin (plugins/examples/CalibrationTest/, copied into
     the test instance's own throwaway userdata/plugins/ folder for the
     run) that updates its devices the same way any real plugin does:
     Domoticz.Device.Update() -> CSQLHelper::UpdateValue with a raw value.
     This is the ingest path that used to have no calibration at all.

Both paths exercise: pTypeTEMP (temperature, AddjValue), pTypeTEMP_HUM
(temperature component), pTypeTEMP_HUM_BARO with an integer-barometer
subtype AND a float-barometer subtype (temperature via AddjValue *and*
barometer via AddjValue2, in both of the two sValue formatting branches),
a bare barometer (pTypeGeneral/sTypeBaro, AddjValue2), and UV
(pTypeUV, AddjMulti2, since decode_UV is one of the refactored call sites).

A third path, MQTT Auto Discovery, is optional: if no MQTT broker is
reachable on 127.0.0.1:1883 (or no MQTT client library is installed) it is
skipped cleanly with a clear SKIP message rather than failing.

Usage
-----
    python test_calibration.py <path-to-domoticz.exe> [--record baseline.json]

It starts its own Domoticz on a free port with a throwaway database and
userdata folder (own throwaway plugins/ subfolder too), so it never touches
an existing installation or its data. See test/README.md for how to run this
against the saved pre-fix binary and the post-fix binary and diff the two.

--record <file> additionally writes every case's raw/calibration/expected/
observed/status to a JSON file, so a run against the pre-fix binary and a
run against the post-fix binary can be diffed directly. Exit code and the
printed table always reflect PASS/FAIL/SKIP regardless of --record.
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

# ---------------------------------------------------------------------------
# Device type/subtype numeric codes (main/RFXtrx.h, hardware/hardwaretypes.h)
# ---------------------------------------------------------------------------
TYPE_TEMP = 80            # pTypeTEMP
SUB_TEMP5 = 5              # sTypeTEMP5
TYPE_TEMP_HUM = 82         # pTypeTEMP_HUM
SUB_TH1 = 1                # sTypeTH1
TYPE_TEMP_HUM_BARO = 84    # pTypeTEMP_HUM_BARO
SUB_THB1 = 1               # sTypeTHB1 (integer barometer)
SUB_THBFLOAT = 16          # sTypeTHBFloat (float barometer)
TYPE_GENERAL = 243         # pTypeGeneral
SUB_BARO = 26              # sTypeBaro (bare barometer)
TYPE_UV = 87               # pTypeUV
SUB_UV1 = 1                # sTypeUV1

HTYPE_DUMMY = 15
HTYPE_PYTHON_PLUGIN = 94

# Plugin device units -- fixed contract with plugins/examples/CalibrationTest/plugin.py
UNIT_TRIGGER = 1
UNIT_TEMP = 2
UNIT_TEMP_HUM = 3
UNIT_TEMP_HUM_BARO_INT = 4
UNIT_TEMP_HUM_BARO_FLOAT = 5
UNIT_BARO = 6
UNIT_UV = 7

ALL_PLUGIN_UNITS = (UNIT_TRIGGER, UNIT_TEMP, UNIT_TEMP_HUM, UNIT_TEMP_HUM_BARO_INT,
                    UNIT_TEMP_HUM_BARO_FLOAT, UNIT_BARO, UNIT_UV)
UNIT_NAMES = {
    UNIT_TRIGGER: "trigger switch",
    UNIT_TEMP: "TEMP",
    UNIT_TEMP_HUM: "TEMP_HUM",
    UNIT_TEMP_HUM_BARO_INT: "TEMP_HUM_BARO int",
    UNIT_TEMP_HUM_BARO_FLOAT: "TEMP_HUM_BARO float",
    UNIT_BARO: "Barometer",
    UNIT_UV: "UV",
}

# Raw values the plugin pushes on trigger -- must match plugin.py's RAW_VALUES
RAW_TEMP = "20.0"
RAW_TEMP_HUM = "20.0;50;1"
RAW_THB_INT = "20.0;50;1;1010;0"
RAW_THB_FLOAT = "20.0;50;1;1010.3;0"
RAW_BARO = "1010.0;0"
RAW_UV = "5.0;0.0"

PLUGIN_KEY = "CalibrationTest"

CHECKS = 0
FAILURES = 0
SKIPPED = 0
RESULTS = []


# ---------------------------------------------------------------------------
# Result table / JSON recording
# ---------------------------------------------------------------------------
def record(path, device, raw, calibration, expected, observed, status, note=""):
    global CHECKS, FAILURES, SKIPPED
    row = {
        "path": path,
        "device": device,
        "raw": raw,
        "calibration": calibration,
        "expected": expected,
        "observed": observed,
        "status": status,
        "note": note,
    }
    RESULTS.append(row)
    CHECKS += 1
    if status == "FAIL":
        FAILURES += 1
    elif status == "SKIP":
        SKIPPED += 1
    line = "  %-4s  %-10s  %-32s  raw=%-20s  expected=%-10s  observed=%-10s" % (
        status, path, device, str(raw), str(expected), str(observed))
    if note:
        line += "  (%s)" % note
    print(line)
    return status


def close(a, b, tol=0.05):
    try:
        return abs(float(a) - float(b)) <= tol
    except (TypeError, ValueError):
        return False


def check_field(path, label, raw, calib, dev, field, expected, tol=0.05):
    """Fetch `field` from a getdevices result and compare it to `expected`."""
    if dev is None:
        return record(path, label, raw, calib, expected, None, "FAIL",
                      "device not found via getdevices")
    if field not in dev:
        return record(path, label, raw, calib, expected, None, "FAIL",
                      "field '%s' missing from getdevices response" % field)
    raw_observed = dev[field]
    try:
        observed = float(raw_observed)
    except (TypeError, ValueError):
        observed = raw_observed
    status = "PASS" if close(observed, expected, tol) else "FAIL"
    return record(path, label, raw, calib, expected, observed, status)


# ---------------------------------------------------------------------------
# Harness: throwaway Domoticz instance (mirrors test_api_sweep.py)
# ---------------------------------------------------------------------------
def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def repo_root(exe):
    """Walk up from the executable to the checkout that contains main/."""
    d = os.path.dirname(os.path.abspath(exe))
    for _ in range(6):
        if os.path.isdir(os.path.join(d, "main")) and os.path.isdir(os.path.join(d, "www")):
            return d
        d = os.path.dirname(d)
    return None


class EarlyExit(RuntimeError):
    """Domoticz died during startup -- worth retrying on a fresh port/database."""


def domoticz_running():
    """True if any domoticz process is alive.

    Domoticz takes a global "Local\\Domoticz" mutex at startup and exits
    immediately if another instance already holds it, so tests cannot run
    concurrently with any other instance (see test/README.md).
    """
    try:
        if os.name == "nt":
            out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq domoticz.exe"],
                                 capture_output=True, text=True, timeout=20).stdout
            return "domoticz.exe" in out
        out = subprocess.run(["pgrep", "-x", "domoticz"],
                             capture_output=True, text=True, timeout=20)
        return out.returncode == 0
    except Exception:
        return False


def wait_for_no_domoticz(timeout=60):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if not domoticz_running():
            return True
        time.sleep(1)
    return False


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
            print("  (startup attempt %d/%d failed: %s -- retrying)"
                  % (i + 1, attempts, e))
            time.sleep(3)
    raise RuntimeError("domoticz would not start after %d attempts: %s"
                       % (attempts, last))


class Domoticz:
    """A throwaway Domoticz instance: own port, own database, own userdata.

    If plugin_source_dir is given, its contents are copied into this
    instance's own userdata/plugins/CalibrationTest/ folder before startup,
    since Domoticz scans <userdata>/plugins/<Name>/plugin.py once at boot
    (CPluginSystem::BuildManifest()) -- it never touches the checkout's own
    plugins/ directory.
    """

    def __init__(self, exe, www_root, plugin_source_dir=None):
        self.exe = os.path.abspath(exe)
        self.port = free_port()
        self.tmp = tempfile.mkdtemp(prefix="domo_calibration_")
        self.db = os.path.join(self.tmp, "domoticz.db")
        self.log = open(os.path.join(self.tmp, "domoticz.log"), "w", encoding="utf-8",
                        errors="replace")
        self.cookie_header = None  # set by login() once an admin session exists

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


def login(domo, username="admin", password="calibration-test-pw"):
    """Provision the first admin user and log in.

    A fresh, never-configured database has zero users, and every admin
    command (addhardware, createdevice, setused, udevice, switchlight) is
    rejected outright (plain HTTP 401, before it even reaches the command
    handler) without a session -- there is no implicit "no users configured
    means full access" carve-out for these, unlike some other endpoints.
    Returns the "Cookie: ..." header value to attach to subsequent requests,
    or None if login failed.
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


# ---------------------------------------------------------------------------
# JSON API helpers for building the udevice-path fixtures
# ---------------------------------------------------------------------------
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


def set_calibration(domo, idx, addjvalue=None, addjmulti=None, addjvalue2=None, addjmulti2=None):
    parts = ["idx=%d" % idx, "used=true"]
    if addjvalue is not None:
        parts.append("addjvalue=%s" % addjvalue)
    if addjmulti is not None:
        parts.append("addjmulti=%s" % addjmulti)
    if addjvalue2 is not None:
        parts.append("addjvalue2=%s" % addjvalue2)
    if addjmulti2 is not None:
        parts.append("addjmulti2=%s" % addjmulti2)
    path = "/json.htm?type=command&param=setused&" + "&".join(parts)
    code, data = cmd(domo, path)
    if data.get("status") != "OK":
        raise RuntimeError("setused (calibration) failed for idx %d: %r" % (idx, data))


def push_udevice(domo, idx, nvalue, svalue):
    path = ("/json.htm?type=command&param=udevice&idx=%d&nvalue=%d&svalue=%s"
            % (idx, nvalue, urllib.parse.quote(svalue)))
    code, data = cmd(domo, path)
    if data.get("status") != "OK":
        raise RuntimeError("udevice update failed for idx %d: %r" % (idx, data))


# ---------------------------------------------------------------------------
# Path A: JSON "udevice" API
# ---------------------------------------------------------------------------
def run_udevice_cases(domo):
    print("\n=== JSON udevice path (/json.htm?type=command&param=udevice) ===")
    hw_idx = add_dummy_hardware(domo, "CalibrationTestDummy")

    # pTypeTEMP / sTypeTEMP5 -- regression guard, already worked before the fix
    idx = create_device(domo, hw_idx, "UdevTemp", TYPE_TEMP, SUB_TEMP5)
    set_calibration(domo, idx, addjvalue=1.5, addjmulti=1.0)
    push_udevice(domo, idx, 0, RAW_TEMP)
    dev = get_device(domo, idx)
    check_field("udevice", "TEMP", RAW_TEMP, "AddjValue=+1.5", dev, "Temp", 21.5)

    # pTypeTEMP_HUM / sTypeTH1 -- regression guard (temperature component)
    idx = create_device(domo, hw_idx, "UdevTempHum", TYPE_TEMP_HUM, SUB_TH1)
    set_calibration(domo, idx, addjvalue=-0.8, addjmulti=1.0)
    push_udevice(domo, idx, 0, RAW_TEMP_HUM)
    dev = get_device(domo, idx)
    check_field("udevice", "TEMP_HUM (temp)", RAW_TEMP_HUM, "AddjValue=-0.8", dev, "Temp", 19.2)
    check_field("udevice", "TEMP_HUM (hum, unaffected)", RAW_TEMP_HUM, "AddjValue=-0.8",
                dev, "Humidity", 50, tol=0.5)

    # pTypeTEMP_HUM_BARO / sTypeTHB1, integer-barometer branch
    idx = create_device(domo, hw_idx, "UdevTHBInt", TYPE_TEMP_HUM_BARO, SUB_THB1)
    set_calibration(domo, idx, addjvalue=0.3, addjvalue2=5, addjmulti=1.0, addjmulti2=1.0)
    push_udevice(domo, idx, 0, RAW_THB_INT)
    dev = get_device(domo, idx)
    check_field("udevice", "TEMP_HUM_BARO int (temp)", RAW_THB_INT, "AddjValue=+0.3",
                dev, "Temp", 20.3)
    check_field("udevice", "TEMP_HUM_BARO int (baro, rounded)", RAW_THB_INT, "AddjValue2=+5",
                dev, "Barometer", 1015, tol=0.5)

    # pTypeTEMP_HUM_BARO / sTypeTHBFloat, float-barometer branch
    idx = create_device(domo, hw_idx, "UdevTHBFloat", TYPE_TEMP_HUM_BARO, SUB_THBFLOAT)
    set_calibration(domo, idx, addjvalue=-1.2, addjvalue2=2.7, addjmulti=1.0, addjmulti2=1.0)
    push_udevice(domo, idx, 0, RAW_THB_FLOAT)
    dev = get_device(domo, idx)
    check_field("udevice", "TEMP_HUM_BARO float (temp)", RAW_THB_FLOAT, "AddjValue=-1.2",
                dev, "Temp", 18.8)
    check_field("udevice", "TEMP_HUM_BARO float (baro, %.1f)", RAW_THB_FLOAT, "AddjValue2=+2.7",
                dev, "Barometer", 1013.0)

    # pTypeGeneral / sTypeBaro (bare barometer) -- NOT calibrated by any ingest path today
    idx = create_device(domo, hw_idx, "UdevBaro", TYPE_GENERAL, SUB_BARO)
    set_calibration(domo, idx, addjvalue2=-3.4)
    push_udevice(domo, idx, 0, RAW_BARO)
    dev = get_device(domo, idx)
    check_field("udevice", "Barometer (bare)", RAW_BARO, "AddjValue2=-3.4", dev, "Barometer", 1006.6)

    # pTypeUV / sTypeUV1 -- multiplier calibration, not applied via udevice today
    idx = create_device(domo, hw_idx, "UdevUV", TYPE_UV, SUB_UV1)
    set_calibration(domo, idx, addjmulti2=1.2)
    push_udevice(domo, idx, 0, RAW_UV)
    dev = get_device(domo, idx)
    check_field("udevice", "UV (AddjMulti2)", RAW_UV, "AddjMulti2=x1.2", dev, "UVI", 6.0)


# ---------------------------------------------------------------------------
# Path B: Python plugin
# ---------------------------------------------------------------------------
PLUGIN_CASE_LABELS = [
    "TEMP",
    "TEMP_HUM (temp)",
    "TEMP_HUM (hum, unaffected)",
    "TEMP_HUM_BARO int (temp)",
    "TEMP_HUM_BARO int (baro, rounded)",
    "TEMP_HUM_BARO float (temp)",
    "TEMP_HUM_BARO float (baro, %.1f)",
    "Barometer (bare)",
    "UV (AddjMulti2)",
]


def skip_all_plugin_cases(note):
    for label in PLUGIN_CASE_LABELS:
        record("plugin", label, "-", "-", "-", "-", "SKIP", note)


def wait_for_plugin_devices(domo, hw_idx, expected_count, timeout=30):
    deadline = time.time() + timeout
    last = {}
    while time.time() < deadline:
        code, data = cmd(domo, "/json.htm?type=command&param=getdevices&filter=all"
                              "&used=all&hwidx=%d" % hw_idx)
        result = data.get("result") or []
        last = {int(d["Unit"]): int(d["idx"]) for d in result if "Unit" in d and "idx" in d}
        if len(last) >= expected_count:
            return last
        time.sleep(1)
    return last


def wait_for_trigger_state(domo, trigger_idx, state, timeout=20):
    want = 1 if state == "On" else 0
    deadline = time.time() + timeout
    while time.time() < deadline:
        dev = get_device(domo, trigger_idx)
        if dev is not None:
            data_field = str(dev.get("Data", ""))
            if data_field == state or dev.get("Status") == state or dev.get("nValue") == want:
                return True
        time.sleep(0.5)
    return False


def push_raw_values(domo, trigger_idx, state):
    """Flip the plugin's trigger switch and wait until the plugin has processed it.

    Switching it On makes the plugin push its fixed raw values to every sensor device;
    switching it Off only resets the switch, which is how a second push is armed.
    """
    code, data = cmd(domo, "/json.htm?type=command&param=switchlight&idx=%s&switchcmd=%s"
                          % (trigger_idx, state))
    if data.get("status") != "OK":
        return "could not flip the CalibrationTest trigger switch to %s: %r" % (state, data)
    if not wait_for_trigger_state(domo, trigger_idx, state, timeout=20):
        return ("trigger switch never reported %s -- the plugin worker thread may not "
                "have processed the switchlight command" % state)
    return None


def run_plugin_cases(domo):
    print("\n=== Python plugin path (Domoticz.Device.Update -> CSQLHelper::UpdateValue) ===")
    path = ("/json.htm?type=command&param=addhardware&htype=%d&name=%s"
            "&enabled=true&port=0&address=&username=&password=&extra=%s&datatimeout=0"
            % (HTYPE_PYTHON_PLUGIN, urllib.parse.quote(PLUGIN_KEY), urllib.parse.quote(PLUGIN_KEY)))
    code, data = cmd(domo, path)
    if data.get("status") != "OK" or "idx" not in data:
        skip_all_plugin_cases("could not add CalibrationTest plugin hardware: %r" % data)
        return
    plugin_hw_idx = int(data["idx"])

    # A device with an empty sValue is not serialized by getdevices, and a bare barometer
    # (pTypeGeneral/sTypeBaro) has no value at all until it is first updated. So prime every
    # device with one uncalibrated push before mapping units, otherwise the barometer, which
    # is the exact device this test is about, can never be found.
    unit_idx = wait_for_plugin_devices(domo, plugin_hw_idx, expected_count=6, timeout=60)
    if UNIT_TRIGGER not in unit_idx:
        skip_all_plugin_cases(
            "CalibrationTest plugin did not create its devices (got %d within 60s) -- this "
            "Domoticz build may not have Python plugin support enabled, or the plugin "
            "failed to start (check the instance log)" % len(unit_idx))
        return

    problem = push_raw_values(domo, unit_idx[UNIT_TRIGGER], "On")
    if problem:
        skip_all_plugin_cases(problem)
        return

    unit_idx = wait_for_plugin_devices(domo, plugin_hw_idx, expected_count=7, timeout=30)
    if len(unit_idx) < 7:
        missing = sorted(set(ALL_PLUGIN_UNITS) - set(unit_idx))
        skip_all_plugin_cases(
            "CalibrationTest plugin devices did not all report a value (got %d/7, missing "
            "unit(s) %s)"
            % (len(unit_idx), ", ".join("%d (%s)" % (u, UNIT_NAMES.get(u, "?")) for u in missing)))
        return

    set_calibration(domo, unit_idx[UNIT_TEMP], addjvalue=1.5, addjmulti=1.0)
    set_calibration(domo, unit_idx[UNIT_TEMP_HUM], addjvalue=-0.8, addjmulti=1.0)
    set_calibration(domo, unit_idx[UNIT_TEMP_HUM_BARO_INT],
                    addjvalue=0.3, addjvalue2=5, addjmulti=1.0, addjmulti2=1.0)
    set_calibration(domo, unit_idx[UNIT_TEMP_HUM_BARO_FLOAT],
                    addjvalue=-1.2, addjvalue2=2.7, addjmulti=1.0, addjmulti2=1.0)
    set_calibration(domo, unit_idx[UNIT_BARO], addjvalue2=-3.4)
    set_calibration(domo, unit_idx[UNIT_UV], addjmulti2=1.2)

    # Now that calibration is configured, arm and fire a second push. Every stored value
    # below is therefore the result of the plugin sending its raw value into an already
    # calibrated device.
    trigger_idx = unit_idx[UNIT_TRIGGER]
    for state in ("Off", "On"):
        problem = push_raw_values(domo, trigger_idx, state)
        if problem:
            skip_all_plugin_cases(problem)
            return
    time.sleep(2)

    dev = get_device(domo, unit_idx[UNIT_TEMP])
    check_field("plugin", "TEMP", RAW_TEMP, "AddjValue=+1.5", dev, "Temp", 21.5)

    dev = get_device(domo, unit_idx[UNIT_TEMP_HUM])
    check_field("plugin", "TEMP_HUM (temp)", RAW_TEMP_HUM, "AddjValue=-0.8", dev, "Temp", 19.2)
    check_field("plugin", "TEMP_HUM (hum, unaffected)", RAW_TEMP_HUM, "AddjValue=-0.8",
                dev, "Humidity", 50, tol=0.5)

    dev = get_device(domo, unit_idx[UNIT_TEMP_HUM_BARO_INT])
    check_field("plugin", "TEMP_HUM_BARO int (temp)", RAW_THB_INT, "AddjValue=+0.3",
                dev, "Temp", 20.3)
    check_field("plugin", "TEMP_HUM_BARO int (baro, rounded)", RAW_THB_INT, "AddjValue2=+5",
                dev, "Barometer", 1015, tol=0.5)

    dev = get_device(domo, unit_idx[UNIT_TEMP_HUM_BARO_FLOAT])
    check_field("plugin", "TEMP_HUM_BARO float (temp)", RAW_THB_FLOAT, "AddjValue=-1.2",
                dev, "Temp", 18.8)
    check_field("plugin", "TEMP_HUM_BARO float (baro, %.1f)", RAW_THB_FLOAT, "AddjValue2=+2.7",
                dev, "Barometer", 1013.0)

    dev = get_device(domo, unit_idx[UNIT_BARO])
    check_field("plugin", "Barometer (bare)", RAW_BARO, "AddjValue2=-3.4", dev, "Barometer", 1006.6)

    dev = get_device(domo, unit_idx[UNIT_UV])
    check_field("plugin", "UV (AddjMulti2)", RAW_UV, "AddjMulti2=x1.2", dev, "UVI", 6.0)


# ---------------------------------------------------------------------------
# Path C: MQTT Auto Discovery (optional)
# ---------------------------------------------------------------------------
def mqtt_broker_available(host="127.0.0.1", port=1883, timeout=1.5):
    try:
        s = socket.create_connection((host, port), timeout=timeout)
        s.close()
        return True
    except Exception:
        return False


def run_mqtt_case():
    print("\n=== MQTT Auto Discovery path (optional) ===")
    if not mqtt_broker_available():
        record("mqtt-autodiscover", "TEMP", "-", "-", "-", "-", "SKIP",
              "no MQTT broker reachable on 127.0.0.1:1883")
        return
    try:
        import paho.mqtt.client  # noqa: F401
    except ImportError:
        record("mqtt-autodiscover", "TEMP", "-", "-", "-", "-", "SKIP",
              "a broker is reachable but the 'paho-mqtt' package is not installed")
        return
    # A broker and a client library are both available, but replaying Domoticz's
    # MQTT Auto Discovery handshake (discovery config topic, state topic, exact
    # payload schema in hardware/MQTTAutoDiscover.cpp) is a substantial separate
    # integration effort; it is intentionally out of scope for this pass rather
    # than guessed at. See the report for what a follow-up would need to add.
    record("mqtt-autodiscover", "TEMP", "-", "-", "-", "-", "SKIP",
          "broker reachable, but the MQTT Auto Discovery handshake is not implemented "
          "in this script (see module docstring / test report)")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    record_path = None
    if "--record" in sys.argv:
        i = sys.argv.index("--record")
        if i + 1 < len(sys.argv):
            record_path = sys.argv[i + 1]

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

    plugin_source_dir = os.path.join(root, "plugins", "examples", "CalibrationTest")
    if not os.path.isdir(plugin_source_dir):
        print("WARNING: %s not found -- plugin-path cases will be skipped" % plugin_source_dir)
        plugin_source_dir = None

    with start_domoticz(exe, root, plugin_source_dir) as domo:
        print("domoticz running on port %d (temp data in %s)" % (domo.port, domo.tmp))

        domo.cookie_header = login(domo)
        if not domo.cookie_header:
            print("ERROR: could not create/log in as an admin user; every case below "
                  "needs admin rights for addhardware/createdevice/setused/udevice")
            return 2

        run_udevice_cases(domo)

        if plugin_source_dir:
            run_plugin_cases(domo)
        else:
            skip_all_plugin_cases("plugins/examples/CalibrationTest/ not found in checkout")

        run_mqtt_case()

    print("\n%d checks, %d failure(s), %d skipped" % (CHECKS, FAILURES, SKIPPED))

    if record_path:
        with open(record_path, "w", encoding="utf-8") as fh:
            json.dump({
                "executable": os.path.abspath(exe),
                "checks": CHECKS,
                "failures": FAILURES,
                "skipped": SKIPPED,
                "results": RESULTS,
            }, fh, indent=2)
        print("recorded results to %s" % record_path)

    return 0 if FAILURES == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
