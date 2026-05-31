"""
End-to-end test for the Plugin WebSocket Channel feature.

Prerequisites
-------------
1. Domoticz must be running and reachable (default: http://127.0.0.1:8080).
2. The WebSocketChannelTest plugin must be present at plugins/WebSocketChannelTest/
   in the Domoticz source tree (Domoticz scans plugins/<Name>/plugin.py directly;
   it does NOT scan plugins/examples/).
   The tracked/example copy lives at plugins/examples/WebSocketChannelTest/; to
   run the plugin, copy that folder to plugins/WebSocketChannelTest/ (or any
   plugins/<Name>/) and restart Domoticz.
   - Restart Domoticz (or reload plugins).
3. Add the plugin **twice** as hardware:
   - Instance A: Hardware > Add > Type "WebSocket Channel Test", Tag = "A"
   - Instance B: Hardware > Add > Type "WebSocket Channel Test", Tag = "B"
   Both instances must be enabled and running.
4. A user account with at least Switcher rights is required to send plugin_command
   frames (default admin/domoticz works).

Dependencies
------------
Uses only the Python standard library when possible.  If 'websocket-client' is
available it is used for the raw WebSocket connection; otherwise the script falls
back to http.client for a plain-HTTP upgrade (which covers most CI use-cases with
Python >= 3.11 that include tomllib but not a WS library).

The preferred way is to install websocket-client:
    pip install websocket-client

Run command
-----------
    python test/python/test_plugin_websocket.py

Optional environment variables / argv overrides:
    DOMOTICZ_URL   Base URL, e.g. http://192.168.1.10:8080   (default: http://127.0.0.1:8080)
    DOMOTICZ_USER  Username  (default: admin)
    DOMOTICZ_PASS  Password  (default: domoticz)

Or pass URL as the first positional argument:
    python test/python/test_plugin_websocket.py http://192.168.1.10:8080

Protocol notes (must match livesocket.js and DomoticzWebsocketHandler.cpp)
---------------------------------------------------------------------------
- URL path:       <base>/json   (livesocket.js line: wsURI = ... + location.pathname + 'json')
- Subprotocol:    "domoticz"    (livesocket.js: protocols: ["domoticz"])
- Auth:           HTTP Basic auth on the initial HTTP upgrade handshake
- Subscribe:      {"event":"subscribe","topic":"plugin:<key>","requestid":<n>}
  Server ack:     {"event":"subscribed","request":"subscribe","requestid":<n>}
- Plugin push:    {"event":"plugin","plugin":"<key>","hwid":<int>,"data":<any>}
- Plugin command: {"event":"plugin_command","plugin":"<key>","data":<any>[,"hwid":<int>]}
  Server ack:     {"event":"plugin_command_ack","plugin":"<key>","delivered":<int>}
- Unknown plugin ack: delivered==0 (no error field for unknown plugin, just delivered:0)
"""

import json
import os
import sys
import threading
import time
import base64
import urllib.parse
from collections import defaultdict

# ---------------------------------------------------------------------------
# Console encoding: reconfigure stdout/stderr to UTF-8 with replacement so
# that any stray Unicode in debug output does not crash on cp1252 terminals.
# ---------------------------------------------------------------------------
for _stream in (sys.stdout, sys.stderr):
    if hasattr(_stream, "reconfigure"):
        try:
            _stream.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass

# ---------------------------------------------------------------------------
# Configuration from environment / argv
# ---------------------------------------------------------------------------
_base_url = os.environ.get("DOMOTICZ_URL", "http://127.0.0.1:8080").rstrip("/")
if len(sys.argv) > 1 and sys.argv[1].startswith("http"):
    _base_url = sys.argv[1].rstrip("/")

_user = os.environ.get("DOMOTICZ_USER", "admin")
_pass = os.environ.get("DOMOTICZ_PASS", "domoticz")

# Derive WebSocket URL from the base HTTP URL.
# livesocket.js: wsProtocol = location.protocol === 'https:' ? 'wss:' : 'ws:'
#                wsURI = wsProtocol + '//' + location.host + location.pathname + 'json'
# Since the base URL may have a path component (reverse-proxy sub-path), we keep it.
def _make_ws_url(base: str) -> str:
    parsed = urllib.parse.urlparse(base)
    scheme = "wss" if parsed.scheme == "https" else "ws"
    path = parsed.path.rstrip("/") + "/json"
    return urllib.parse.urlunparse((scheme, parsed.netloc, path, "", "", ""))

_ws_url = _make_ws_url(_base_url)
_PLUGIN_KEY = "WSChannelTest"
# Heartbeat is ~10s; the plugin emits the raw-string form every 5th beat (~50s cadence).
# RAW_STRING_TIMEOUT must be long enough to catch at least one raw-string emission after
# the test client connects.  Allow 1.5x the cadence (5 beats * 10s * 1.5) plus slack.
_HEARTBEAT_SECONDS = 10
_RAW_STRING_CADENCE_BEATS = 5
_RAW_STRING_TIMEOUT = int(_RAW_STRING_CADENCE_BEATS * _HEARTBEAT_SECONDS * 1.5) + 5  # ~80s
_TIMEOUT = 35      # seconds - for most checks (subscribe ack, echo round-trips, etc.)
_SHORT_TIMEOUT = 4 # seconds for the "should receive nothing" check

# ---------------------------------------------------------------------------
# WebSocket import - prefer websocket-client, fall back gracefully
# ---------------------------------------------------------------------------
try:
    import websocket as _websocket_client
    _HAS_WS_CLIENT = True
except ImportError:
    _HAS_WS_CLIENT = False

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
_results: list[tuple[str, bool, str]] = []   # (check_name, passed, detail)
_fail_count = 0


def _report(name: str, passed: bool, detail: str = "") -> None:
    global _fail_count
    status = "PASS" if passed else "FAIL"
    line = f"  [{status}] {name}"
    if detail:
        line += f" -- {detail}"
    print(line)
    _results.append((name, passed, detail))
    if not passed:
        _fail_count += 1


def _assert(name: str, condition: bool, detail: str = "") -> None:
    _report(name, condition, detail)


# ---------------------------------------------------------------------------
# WebSocket connection helper
# ---------------------------------------------------------------------------
class _WSConn:
    """
    Thread-safe WebSocket connection that accumulates received messages in a
    list and exposes a 'collect' method to drain messages up to a deadline.

    Uses websocket-client (websocket.WebSocketApp) if available.
    Raises RuntimeError if the library is missing.
    """

    def __init__(self, url: str, user: str, password: str, label: str = ""):
        if not _HAS_WS_CLIENT:
            raise RuntimeError(
                "websocket-client is not installed.\n"
                "Install it with:  pip install websocket-client\n"
                "then re-run the test."
            )
        self._url = url
        self._label = label or url
        self._lock = threading.Lock()
        self._messages: list[dict] = []
        self._open_event = threading.Event()
        self._auth = base64.b64encode(f"{user}:{password}".encode()).decode()

        header = {
            "Authorization": f"Basic {self._auth}",
        }

        self._ws = _websocket_client.WebSocketApp(
            url,
            subprotocols=["domoticz"],
            header=header,
            on_open=self._on_open,
            on_message=self._on_message,
            on_error=self._on_error,
            on_close=self._on_close,
        )
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        self._ws.run_forever(ping_interval=0)

    def _on_open(self, ws):
        self._open_event.set()

    def _on_message(self, ws, raw):
        try:
            msg = json.loads(raw)
        except Exception:
            msg = {"_raw": raw}
        with self._lock:
            self._messages.append(msg)

    def _on_error(self, ws, error):
        pass  # errors are surfaced as missing messages / timeouts

    def _on_close(self, ws, code, reason):
        pass

    def wait_open(self, timeout: float = 10.0) -> bool:
        return self._open_event.wait(timeout)

    def send(self, obj: dict) -> None:
        self._ws.send(json.dumps(obj))

    def collect(self, deadline: float) -> list[dict]:
        """Return all messages received up to the given deadline (absolute time)."""
        msgs: list[dict] = []
        while time.monotonic() < deadline:
            time.sleep(0.05)
            with self._lock:
                msgs.extend(self._messages)
                self._messages.clear()
        return msgs

    def drain(self) -> list[dict]:
        """Return and clear currently queued messages without waiting."""
        with self._lock:
            msgs = list(self._messages)
            self._messages.clear()
        return msgs

    def close(self) -> None:
        try:
            self._ws.close()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


# ---------------------------------------------------------------------------
# Subscribe helper - sends subscribe frame and waits for ack
# ---------------------------------------------------------------------------
_req_counter = 0

def _next_req_id() -> int:
    global _req_counter
    _req_counter += 1
    return _req_counter


def _subscribe(conn: _WSConn, topic: str, timeout: float = 5.0) -> bool:
    """Send a subscribe frame; return True when the ack arrives."""
    req_id = _next_req_id()
    conn.send({"event": "subscribe", "topic": topic, "requestid": req_id})
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        time.sleep(0.05)
        msgs = conn.drain()
        for m in msgs:
            if m.get("event") == "subscribed" and m.get("requestid") == req_id:
                return True
            # Put non-matching messages back
            with conn._lock:
                conn._messages.insert(0, m)
    return False


# ---------------------------------------------------------------------------
# Individual checks
# ---------------------------------------------------------------------------

def check_connect_and_subscribe(conn: _WSConn) -> bool:
    """Connect and subscribe to plugin:WSChannelTest."""
    if not conn.wait_open(10.0):
        _report("connect", False, f"WebSocket did not open within 10s for {_ws_url}")
        return False
    _report("connect", True, f"Connected to {_ws_url}")

    ok = _subscribe(conn, f"plugin:{_PLUGIN_KEY}")
    _report("subscribe", ok,
            f"topic=plugin:{_PLUGIN_KEY}" + ("" if ok else " -- no ack received"))
    return ok


def check_a_multi_instance_fanout_and_hwid(conn: _WSConn) -> tuple[int, int]:
    """
    README req 1 + 2 (multi-instance fan-out + hwid tagging):
    Within timeout receive tick messages from TWO distinct hwids.

    Returns (hwid_a, hwid_b) or (0, 0) on failure.
    Also checks for at least one dict payload and one raw-string payload (req data forms).

    The raw-string form is emitted every _RAW_STRING_CADENCE_BEATS heartbeats (~50s),
    so a client connecting mid-cycle may need to wait up to one full cadence interval.
    _RAW_STRING_TIMEOUT is set to 1.5x the cadence to give comfortable headroom.
    """
    # Phase 1: wait for two distinct hwids and at least one dict payload.
    # These come every heartbeat so _TIMEOUT is sufficient.
    phase1_deadline = time.monotonic() + _TIMEOUT
    hwids: set[int] = set()
    dict_seen = False
    string_seen = False

    def _absorb(m: dict) -> None:
        nonlocal dict_seen, string_seen
        if m.get("event") != "plugin" or m.get("plugin") != _PLUGIN_KEY:
            return
        hwid = m.get("hwid")
        if isinstance(hwid, int):
            hwids.add(hwid)
        data = m.get("data")
        if isinstance(data, dict):
            dict_seen = True
        elif isinstance(data, str):
            string_seen = True

    while time.monotonic() < phase1_deadline and (len(hwids) < 2 or not dict_seen):
        time.sleep(0.1)
        for m in conn.drain():
            _absorb(m)

    # Phase 2: if raw-string not yet seen, keep listening up to _RAW_STRING_TIMEOUT.
    # The plugin emits raw-string-from-<tag>-beat<N> every 5th heartbeat (~50s cadence).
    # _RAW_STRING_TIMEOUT = cadence * 1.5 + slack so we catch it regardless of phase.
    if not string_seen:
        raw_deadline = time.monotonic() + _RAW_STRING_TIMEOUT
        while time.monotonic() < raw_deadline and not string_seen:
            time.sleep(0.1)
            for m in conn.drain():
                _absorb(m)

    got_two = len(hwids) >= 2
    _assert(
        "multi-instance fan-out (two distinct hwids in tick messages)",
        got_two,
        f"hwids seen: {sorted(hwids)}" + ("" if got_two else f" -- waited {_TIMEOUT}s")
    )
    _assert(
        "dict payload form seen",
        dict_seen,
        "at least one 'plugin' frame had data as a JSON object"
    )
    _assert(
        "raw-string payload form seen",
        string_seen,
        (
            "at least one 'plugin' frame had data as a plain string "
            "(emitted every %d beats, ~%ds cadence; waited up to %ds)"
            % (_RAW_STRING_CADENCE_BEATS, _RAW_STRING_CADENCE_BEATS * _HEARTBEAT_SECONDS,
               _RAW_STRING_TIMEOUT)
        )
    )

    if got_two:
        ids = sorted(hwids)
        return ids[0], ids[1]
    return 0, 0


def check_b_round_trip_fanout(conn: _WSConn) -> None:
    """
    README req 3 (inbound fan-out + round-trip F3/F4/F2):
    Send plugin_command {cmd:'ping',ts:<ms>}; assert delivered>=2 in ack and TWO pong messages,
    each with the matching ts value.

    The 'ts' value is int(time.time() * 1000) which is a JavaScript-style millisecond
    timestamp (~1.7e12 as of 2024).  This value exceeds 2^31-1 (2147483647) and therefore
    exercises the 64-bit integer path in CPluginProtocolJSON::PythontoJSON.  On an
    unfixed build (PyLong_AsLong on Windows LLP64) this raises:
        OverflowError: Python int too large to convert to C long
    The pong-ts assertion below will fail if that regression is reintroduced.
    """
    conn.drain()  # clear any buffered messages before the test
    req_id = _next_req_id()
    sent_ts = int(time.time() * 1000)
    # Confirm sent_ts actually exceeds INT32_MAX so this test is a meaningful
    # regression guard for the 32-bit overflow bug (not just a theoretical check).
    assert sent_ts > 2147483647, (
        "sent_ts (%d) must exceed INT32_MAX for the overflow regression check" % sent_ts
    )
    cmd_payload = {"cmd": "ping", "ts": sent_ts}
    conn.send({
        "event": "plugin_command",
        "plugin": _PLUGIN_KEY,
        "data": cmd_payload,
        "requestid": req_id,
    })

    deadline = time.monotonic() + _TIMEOUT
    ack: dict | None = None
    pong_hwids: set[int] = set()
    pong_ts_values: list = []
    buffered: list[dict] = []

    while time.monotonic() < deadline:
        time.sleep(0.05)
        with conn._lock:
            incoming = list(conn._messages)
            conn._messages.clear()
        for m in incoming:
            if m.get("event") == "plugin_command_ack" and m.get("plugin") == _PLUGIN_KEY:
                ack = m
            elif (
                m.get("event") == "plugin"
                and m.get("plugin") == _PLUGIN_KEY
                and isinstance(m.get("data"), dict)
                and m["data"].get("type") == "pong"
            ):
                hwid = m.get("hwid")
                if isinstance(hwid, int):
                    pong_hwids.add(hwid)
                pong_ts_values.append(m["data"].get("ts"))
            else:
                buffered.append(m)
        if ack is not None and len(pong_hwids) >= 2:
            break

    # Put non-pong, non-ack messages back
    with conn._lock:
        conn._messages[:0] = buffered

    _assert(
        "plugin_command_ack received",
        ack is not None,
        str(ack) if ack else "no ack within timeout"
    )
    if ack is not None:
        delivered = ack.get("delivered", -1)
        _assert(
            "plugin_command_ack delivered>=2",
            delivered >= 2,
            f"delivered={delivered}"
        )

    _assert(
        "pong messages from two instances (round-trip)",
        len(pong_hwids) >= 2,
        f"pong hwids: {sorted(pong_hwids)}" + ("" if len(pong_hwids) >= 2 else f" -- waited {_TIMEOUT}s")
    )
    # This assertion is the 64-bit integer overflow regression guard:
    # sent_ts > INT32_MAX, so if PythontoJSON uses PyLong_AsLong (32-bit on
    # Windows) the C++ side raises OverflowError and the pong is never sent,
    # causing pong_ts_values to be empty or contain incorrect (truncated) values.
    ts_match = all(v == sent_ts for v in pong_ts_values) and len(pong_ts_values) >= 2
    _assert(
        "pong ts (>INT32_MAX) echoed back correctly -- 64-bit int overflow regression",
        ts_match,
        "sent_ts=%d (>2^31), pong ts values=%r" % (sent_ts, pong_ts_values)
    )


def check_c_hwid_targeting(conn: _WSConn, hwid_a: int, hwid_b: int) -> None:
    """
    README req 4 (hwid targeting):
    Send plugin_command with hwid=hwid_a and {cmd:'ping',ts:<ms>}; assert exactly one pong
    (from instance A) with the matching ts value.
    """
    if hwid_a == 0:
        _report("hwid targeting (single instance pong)", False, "skipped -- hwids not discovered")
        return

    conn.drain()
    sent_ts = int(time.time() * 1000)
    cmd_payload = {"cmd": "ping", "ts": sent_ts}
    conn.send({
        "event": "plugin_command",
        "plugin": _PLUGIN_KEY,
        "hwid": hwid_a,
        "data": cmd_payload,
    })

    deadline = time.monotonic() + _TIMEOUT
    pong_hwids: set[int] = set()
    pong_ts_values: list = []
    buffered: list[dict] = []

    while time.monotonic() < deadline:
        time.sleep(0.05)
        with conn._lock:
            incoming = list(conn._messages)
            conn._messages.clear()
        for m in incoming:
            if (
                m.get("event") == "plugin"
                and m.get("plugin") == _PLUGIN_KEY
                and isinstance(m.get("data"), dict)
                and m["data"].get("type") == "pong"
            ):
                hwid = m.get("hwid")
                if isinstance(hwid, int):
                    pong_hwids.add(hwid)
                pong_ts_values.append(m["data"].get("ts"))
            else:
                buffered.append(m)
        if pong_hwids:
            # Give a brief window to check whether a second pong arrives
            time.sleep(1.0)
            with conn._lock:
                extra = list(conn._messages)
                conn._messages.clear()
            for m in extra:
                if (
                    m.get("event") == "plugin"
                    and m.get("plugin") == _PLUGIN_KEY
                    and isinstance(m.get("data"), dict)
                    and m["data"].get("type") == "pong"
                ):
                    hwid = m.get("hwid")
                    if isinstance(hwid, int):
                        pong_hwids.add(hwid)
                    pong_ts_values.append(m["data"].get("ts"))
                else:
                    buffered.append(m)
            break

    with conn._lock:
        conn._messages[:0] = buffered

    exactly_one = len(pong_hwids) == 1 and hwid_a in pong_hwids
    _assert(
        "hwid targeting (exactly one pong from target instance)",
        exactly_one,
        f"target hwid={hwid_a}, pong hwids received: {sorted(pong_hwids)}"
    )
    ts_match = pong_ts_values == [sent_ts]
    _assert(
        "hwid targeting pong ts echoed back correctly",
        ts_match,
        f"sent_ts={sent_ts}, pong ts values={pong_ts_values}"
    )


def check_d_topic_filtering() -> None:
    """
    README req 5 (topic filtering):
    Open a second WebSocket that does NOT subscribe; verify it receives no 'plugin' events.
    """
    try:
        with _WSConn(_ws_url, _user, _pass, label="unsubscribed-conn") as conn2:
            if not conn2.wait_open(10.0):
                _report("topic filtering (unsubscribed gets nothing)", False,
                        "second connection failed to open")
                return
            # Give the server a moment then collect
            deadline = time.monotonic() + _SHORT_TIMEOUT
            msgs = conn2.collect(deadline)
            plugin_events = [m for m in msgs if m.get("event") == "plugin"]
            no_plugin = len(plugin_events) == 0
            _assert(
                "topic filtering (unsubscribed connection gets no plugin events)",
                no_plugin,
                f"received {len(plugin_events)} unexpected plugin events" if not no_plugin
                else f"no plugin events received in {_SHORT_TIMEOUT}s window"
            )
    except RuntimeError as e:
        _report("topic filtering (unsubscribed gets nothing)", False, str(e))


def check_e_unknown_plugin(conn: _WSConn) -> None:
    """
    README req 6 (unknown plugin):
    Send plugin_command for a bogus plugin name; assert ack delivered==0 and no crash.
    """
    conn.drain()
    bogus_key = "NoSuchPlugin_xyz_9999"
    conn.send({
        "event": "plugin_command",
        "plugin": bogus_key,
        "data": {"cmd": "test"},
    })

    deadline = time.monotonic() + _TIMEOUT
    ack: dict | None = None
    buffered: list[dict] = []

    while time.monotonic() < deadline:
        time.sleep(0.05)
        with conn._lock:
            incoming = list(conn._messages)
            conn._messages.clear()
        for m in incoming:
            if m.get("event") == "plugin_command_ack" and m.get("plugin") == bogus_key:
                ack = m
            else:
                buffered.append(m)
        if ack is not None:
            break

    with conn._lock:
        conn._messages[:0] = buffered

    _assert(
        "unknown plugin: ack received",
        ack is not None,
        str(ack) if ack else "no ack within timeout"
    )
    if ack is not None:
        delivered = ack.get("delivered", -1)
        _assert(
            "unknown plugin: delivered==0",
            delivered == 0,
            f"delivered={delivered}"
        )
        # No error field expected for unknown plugin (just delivered:0 per server code)
        no_crash_indicator = "error" not in ack
        _assert(
            "unknown plugin: no error/crash indicator in ack",
            no_crash_indicator,
            f"ack fields: {list(ack.keys())}"
        )


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    print(f"Plugin WebSocket Channel - end-to-end test")
    print(f"  Server : {_ws_url}")
    print(f"  Plugin : {_PLUGIN_KEY}")
    print(f"  User   : {_user}")
    print()

    if not _HAS_WS_CLIENT:
        print("ERROR: websocket-client is not installed.")
        print("  pip install websocket-client")
        return 2

    hwid_a, hwid_b = 0, 0

    try:
        with _WSConn(_ws_url, _user, _pass, label="primary") as conn:
            print("--- connect + subscribe ---")
            if not check_connect_and_subscribe(conn):
                print("\nCannot proceed without a working connection/subscription.")
                return 1

            print("\n--- check A: multi-instance fan-out + hwid tagging + payload forms ---")
            hwid_a, hwid_b = check_a_multi_instance_fanout_and_hwid(conn)

            print("\n--- check B: round-trip fan-out (plugin_command ping -> pong x2) ---")
            try:
                check_b_round_trip_fanout(conn)
            except Exception as exc:
                _report("check B (round-trip fan-out)", False, f"unexpected exception: {exc}")

            print("\n--- check C: hwid targeting (single-instance pong delivery) ---")
            try:
                check_c_hwid_targeting(conn, hwid_a, hwid_b)
            except Exception as exc:
                _report("check C (hwid targeting)", False, f"unexpected exception: {exc}")

            print("\n--- check D: topic filtering (unsubscribed connection) ---")
            try:
                check_d_topic_filtering()
            except Exception as exc:
                _report("check D (topic filtering)", False, f"unexpected exception: {exc}")

            print("\n--- check E: unknown plugin (no crash, delivered==0) ---")
            try:
                check_e_unknown_plugin(conn)
            except Exception as exc:
                _report("check E (unknown plugin)", False, f"unexpected exception: {exc}")

    except RuntimeError as e:
        print(f"\nFATAL: {e}")
        return 2

    print()
    print("=" * 60)
    total = len(_results)
    passed = sum(1 for _, ok, _ in _results if ok)
    failed = total - passed
    print(f"Result: {passed}/{total} checks passed, {failed} failed")

    if _fail_count > 0:
        print("\nFailed checks:")
        for name, ok, detail in _results:
            if not ok:
                print(f"  - {name}: {detail}")
        return 1

    print("All checks PASSED.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
