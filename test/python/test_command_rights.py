#!/usr/bin/env python3
"""Regression test for the minimum user rights of the JSON commands.

Every command is registered in main/WebServer.cpp with the minimum rights it
needs (RegisterCommandCode(name, fn, minRights, bypassAuthentication)) and
CWebServer::GetJSonPage refuses the call before the handler runs. Many
handlers still carry their own `session.rights` check as well.

Two things are verified:

  * Static: every registration matches the reviewed list in command_rights.json
    (level and whether it bypasses authentication), so a level cannot be lowered
    and a command cannot be added without a deliberate edit there. A handler
    that still refuses callers itself must not be registered below that level,
    and a command that bypasses authentication must not claim a level above
    URIGHTS_VIEWER.
  * Runtime: against a throwaway instance with an admin, a user and a viewer
    account, every command is refused where the registration says it must be:
    anonymous callers get 401 on every non-bypass command, the viewer and the
    user get 403 on every admin command, and the viewer gets 403 on every user
    command. Only requests that must be refused are sent, so no handler runs.
    A few spot checks cover the positive side and the MCP role checks.
    MCP also has to honor the per-user device sharing: a user with devices
    shared sees and controls only those devices, one with none shared sees all.

Usage:
    python test_command_rights.py <path-to-domoticz[.exe]>
"""
import base64
import glob
import hashlib
import json
import os
import re
import socket
import sys
import threading
import time
import urllib.error
import urllib.parse
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_api_sweep import (start_domoticz, repo_root, make_session,  # noqa: E402
                            raw_request)

CHECKS = 0
FAILURES = 0
FAILED = []

LEVELS = {"URIGHTS_VIEWER": 0, "URIGHTS_SWITCHER": 1, "URIGHTS_ADMIN": 2}


def check(cond, label):
    global CHECKS, FAILURES
    CHECKS += 1
    if cond:
        print("  PASS  " + label)
    else:
        print("  FAIL  " + label)
        FAILURES += 1
        FAILED.append(label)
    return bool(cond)


# --------------------------------------------------------------------------
# static: registration versus handler
# --------------------------------------------------------------------------

REG_RE = re.compile(r'RegisterCommandCode\(\s*"([^"]+)"\s*,\s*\[this\]\([^)]*\)\s*\{\s*'
                    r'([A-Za-z_0-9]+)\([^}]*\}\s*(?:,\s*(URIGHTS_\w+))?\s*(?:,\s*(true|false))?\s*\)')

# The expected level of every command. Most handlers no longer check the rights
# themselves, so the registration is all that protects them; comparing it with
# this reviewed list means a level cannot change, and a command cannot be added,
# without a deliberate edit here.
BASELINE_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "command_rights.json")
BASELINE_LEVELS = {"viewer": "URIGHTS_VIEWER", "user": "URIGHTS_SWITCHER", "admin": "URIGHTS_ADMIN"}


def load_baseline():
    """{command: (level, bypass)} from command_rights.json."""
    with open(BASELINE_FILE, encoding="utf-8") as fh:
        raw = json.load(fh)
    out = {}
    for cmd, val in raw.items():
        bypass = val.endswith(" (no login)")
        out[cmd] = (BASELINE_LEVELS[val.replace(" (no login)", "")], bypass)
    return out

ADMIN_COND = r'session\.rights\s*(?:!=|<)\s*(?:2|URIGHTS_ADMIN)\b'
SWITCHER_COND = r'session\.rights\s*==\s*URIGHTS_VIEWER\s*\|\|\s*session\.rights\s*==\s*URIGHTS_NONE'


def registrations(root):
    """[(command, handler, level, bypass)] straight from WebServer.cpp."""
    with open(os.path.join(root, "main", "WebServer.cpp"), encoding="utf-8", errors="replace") as fh:
        src = fh.read()
    total = len(re.findall(r'RegisterCommandCode\(\s*"', src))
    regs = [(c, f, lvl or "URIGHTS_VIEWER", byp == "true") for c, f, lvl, byp in REG_RE.findall(src)]
    return regs, total


def handler_sources(root):
    srcs = []
    for d in ("main", "hardware", "mcpserver", "webserver", "push", "notifications"):
        for p in glob.glob(os.path.join(root, d, "**", "*.cpp"), recursive=True):
            with open(p, encoding="utf-8", errors="replace") as fh:
                srcs.append(fh.read())
    return srcs


def handler_body(srcs, fn):
    pat = re.compile(r'void\s+CWebServer::' + re.escape(fn) +
                     r'\s*\(\s*WebEmSession\s*&[^)]*\)\s*\{')
    for s in srcs:
        m = pat.search(s)
        if m:
            i, depth = m.end(), 1
            while depth and i < len(s):
                if s[i] == "{":
                    depth += 1
                elif s[i] == "}":
                    depth -= 1
                i += 1
            body = s[m.end():i - 1]
            body = re.sub(r'//[^\n]*', '', body)
            return re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    return None


def top_level_guard(body, cond):
    """True when `if (<cond>)` refusing with forbidden sits at the top level of the body."""
    for m in re.finditer(r'if\s*\(\s*\(?' + cond + r'\)?\s*\)', body):
        depth = body[:m.start()].count("{") - body[:m.start()].count("}")
        if depth == 0 and "forbidden" in body[m.end():m.end() + 200]:
            return True
    return False


def static_checks(root):
    print("=== registration and handler agree on the minimum rights ===")
    regs, total = registrations(root)
    check(len(regs) == total,
          "every RegisterCommandCode line is understood by this test (%d of %d)" % (len(regs), total))
    srcs = handler_sources(root)
    mismatches, bypass_levels, missing = [], [], []
    for cmd, fn, level, bypass in regs:
        if bypass and level != "URIGHTS_VIEWER":
            bypass_levels.append(cmd)
        body = handler_body(srcs, fn)
        if body is None:
            missing.append(fn)
            continue
        if top_level_guard(body, ADMIN_COND) and level != "URIGHTS_ADMIN":
            mismatches.append("%s (handler is admin only, registered %s)" % (cmd, level))
        elif top_level_guard(body, SWITCHER_COND) and LEVELS[level] < LEVELS["URIGHTS_SWITCHER"]:
            mismatches.append("%s (handler refuses viewers, registered %s)" % (cmd, level))
    check(not missing, "every registered handler is found in the sources %s" % (missing or ""))
    check(not bypass_levels,
          "no command bypasses authentication while requiring more than viewer rights %s"
          % (bypass_levels or ""))
    check(not mismatches, "no handler checks for more rights than its registration %s"
          % (mismatches or ""))
    baseline = load_baseline()
    registered = {cmd: (level, bypass) for cmd, _, level, bypass in regs}
    changed = ["%s: registered %s%s, expected %s%s" % (
                   cmd, registered[cmd][0], " (no login)" if registered[cmd][1] else "",
                   baseline[cmd][0], " (no login)" if baseline[cmd][1] else "")
               for cmd in registered if cmd in baseline and registered[cmd] != baseline[cmd]]
    added = sorted(set(registered) - set(baseline))
    removed = sorted(set(baseline) - set(registered))
    check(not changed, "every command has the level listed in command_rights.json %s" % (changed or ""))
    check(not added, "every registered command is listed in command_rights.json %s" % (added or ""))
    check(not removed, "command_rights.json lists no command that is no longer registered %s"
          % (removed or ""))
    return regs


# --------------------------------------------------------------------------
# runtime
# --------------------------------------------------------------------------

def md5(s):
    return hashlib.md5(s.encode()).hexdigest()


def cookie_value(cookie_header):
    return cookie_header.split(":", 1)[1].strip() if cookie_header else ""


def login(port, username, password):
    creds = urllib.parse.urlencode({
        "username": base64.b64encode(username.encode()).decode(),
        "password": password,
    }).encode()
    _, raw = raw_request(port, (b"POST /json.htm?type=command&param=logincheck "
                                b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                                b"Content-Type: application/x-www-form-urlencoded\r\n"
                                b"Content-Length: " + str(len(creds)).encode() +
                                b"\r\nConnection: close\r\n\r\n" + creds))
    for line in raw.split(b"\r\n\r\n", 1)[0].split(b"\r\n"):
        if line.lower().startswith(b"set-cookie:"):
            val = line.split(b":", 1)[1].strip().split(b";")[0].decode("latin-1")
            if not val.endswith("=none"):
                return val
    return ""


def api(d, cookie, query):
    return d.get("/json.htm?type=command&" + query, headers={"Cookie": cookie} if cookie else {})


def mcp_call(d, cookie, sid, payload):
    headers = {"Content-Type": "application/json",
               "Accept": "application/json, text/event-stream"}
    # An access token is passed as "Bearer <jwt>" in place of a session cookie
    if cookie.startswith("Bearer "):
        headers["Authorization"] = cookie
    else:
        headers["Cookie"] = cookie
    if sid:
        headers["Mcp-Session-Id"] = sid
    req = urllib.request.Request(d.url("/mcp"), data=json.dumps(payload).encode(), headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            return r.headers.get("Mcp-Session-Id", sid), r.read()
    except urllib.error.HTTPError as e:
        return sid, e.read()


def mcp_session(d, cookie):
    sid, _ = mcp_call(d, cookie, "", {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
        "protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}}})
    mcp_call(d, cookie, sid, {"jsonrpc": "2.0", "method": "notifications/initialized"})
    return sid


class SseStream:
    """Collects the raw bytes of an MCP SSE stream (GET /mcp) in a background thread."""

    def __init__(self, port, cookie, sid):
        self.buf = b""
        self.lock = threading.Lock()
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=15)
        self.sock.sendall(("GET /mcp HTTP/1.1\r\nHost: 127.0.0.1\r\nAccept: text/event-stream\r\n"
                           "Cookie: %s\r\nMcp-Session-Id: %s\r\n\r\n" % (cookie, sid)).encode())
        self.thread = threading.Thread(target=self._read, daemon=True)
        self.thread.start()

    def _read(self):
        while True:
            try:
                c = self.sock.recv(65536)
            except OSError:
                return
            if not c:
                return
            with self.lock:
                self.buf += c

    def contains(self, needle):
        with self.lock:
            return needle in self.buf

    def wait_for(self, needle, timeout=5.0):
        end = time.time() + timeout
        while time.time() < end:
            if self.contains(needle):
                return True
            time.sleep(0.1)
        return self.contains(needle)

    def wait_for_count(self, needle, count, timeout=5.0):
        end = time.time() + timeout
        while time.time() < end:
            with self.lock:
                if self.buf.count(needle) >= count:
                    return True
            time.sleep(0.1)
        with self.lock:
            return self.buf.count(needle) >= count

    def close(self):
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self.sock.close()


def runtime_checks(exe, regs):
    with start_domoticz(exe, repo_root(exe)) as d:
        p = d.port
        print("\ndomoticz on port %d" % p)

        admin = cookie_value(make_session(p))
        if not check(admin != "", "admin session created"):
            return
        for name, rights in (("user", 1), ("viewer", 0)):
            code, body = api(d, admin, urllib.parse.urlencode({
                "param": "adduser", "enabled": "true", "username": name,
                "password": md5(name + "pw"), "rights": rights,
                "RemoteSharing": "false", "TabsEnabled": 0}))
            check(code == 200 and b'"OK"' in body, "%s account created" % name)
        user = login(p, "user", md5("userpw"))
        viewer = login(p, "viewer", md5("viewerpw"))
        if not (check(user != "", "user logged in") and check(viewer != "", "viewer logged in")):
            return

        # A viewer that is treated as admin (e.g. a trusted local network) makes every
        # refusal below pass vacuously, so make sure the roles really differ first.
        code, _ = api(d, viewer, "param=getsettings")
        if not check(code == 403, "the viewer is not treated as admin (getsettings got %d)" % code):
            return

        print("\n=== every command is refused below its registered level ===")
        wrong = {"anonymous": [], "viewer": [], "user": []}
        for cmd, _, level, bypass in regs:
            q = "param=" + urllib.parse.quote(cmd)
            if not bypass:
                code, _ = api(d, "", q)
                if code != 401:
                    wrong["anonymous"].append("%s=%d" % (cmd, code))
            if LEVELS[level] >= LEVELS["URIGHTS_SWITCHER"]:
                code, _ = api(d, viewer, q)
                if code != 403:
                    wrong["viewer"].append("%s=%d" % (cmd, code))
            if level == "URIGHTS_ADMIN":
                code, _ = api(d, user, q)
                if code != 403:
                    wrong["user"].append("%s=%d" % (cmd, code))
        check(not wrong["anonymous"], "anonymous callers get 401 on every non-bypass command %s"
              % (wrong["anonymous"] or ""))
        check(not wrong["viewer"], "the viewer gets 403 on every user and admin command %s"
              % (wrong["viewer"] or ""))
        check(not wrong["user"], "the user gets 403 on every admin command %s" % (wrong["user"] or ""))

        print("\n=== what the lower roles may still do ===")
        for who, cookie in (("viewer", viewer), ("user", user)):
            code, _ = api(d, cookie, "param=getdevices")
            check(code == 200, "the %s can list devices (got %d)" % (who, code))
            # Themes boot from getconfig, since getsettings is admin only
            code, body = api(d, cookie, "param=getconfig")
            theme = json.loads(body).get("WebTheme") if code == 200 else None
            check(theme == "default", "the %s reads the active theme from getconfig (got %d, %r)" % (who, code, theme))
        code, body = api(d, user, "param=addlogmessage&message=command+rights+test")
        check(code == 200, "the user can add a log message (got %d)" % code)

        print("\n=== gethardware hides credentials from non-admins ===")
        code, body = api(d, admin, "param=addhardware&htype=15&name=RightsTest&enabled=true"
                                   "&address=10.1.2.3&port=1883&username=hwuser&password=hwsecret&datatimeout=0")
        check(code == 200, "test hardware added")
        for who, cookie, expect in (("admin", admin, True), ("viewer", viewer, False)):
            code, body = api(d, cookie, "param=gethardware")
            hw = [h for h in json.loads(body).get("result", []) if h.get("Name") == "RightsTest"]
            if check(code == 200 and len(hw) == 1, "the %s sees the test hardware" % who):
                shown = hw[0].get("Password") == "hwsecret" and hw[0].get("Username") == "hwuser"
                check(shown == expect, "the %s %s the hardware credentials"
                      % (who, "sees" if expect else "does not see"))

        print("\n=== param based commands ===")
        code, _ = api(d, viewer, "param=resetsecuritystatus&idx=1&switchcmd=Normal")
        check(code in (401, 403), "the viewer cannot reset the security status (got %d)" % code)

        print("\n=== MCP ===")
        api(d, admin, "param=adduservariable&vname=rightsvar&vtype=2&vvalue=topsecret")
        sid = mcp_session(d, viewer)
        if check(sid != "", "viewer MCP session initialised"):
            _, body = mcp_call(d, viewer, sid, {"jsonrpc": "2.0", "id": 2, "method": "resources/read",
                                                "params": {"uri": "domoticz://user-variables"}})
            check(b"topsecret" not in body and b"-32002" in body,
                  "the viewer cannot read the user variables resource")
            _, body = mcp_call(d, viewer, sid, {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
                                                "params": {"name": "get_users", "arguments": {}}})
            check(b"-32002" in body, "the viewer cannot call the get_users tool")
            _, body = mcp_call(d, viewer, sid, {"jsonrpc": "2.0", "id": 4, "method": "resources/read",
                                                "params": {"uri": "domoticz://devices"}})
            check(b'"result"' in body, "the viewer can still read the devices resource")
        sid = mcp_session(d, admin)
        if check(sid != "", "admin MCP session initialised"):
            _, body = mcp_call(d, admin, sid, {"jsonrpc": "2.0", "id": 2, "method": "resources/read",
                                               "params": {"uri": "domoticz://user-variables"}})
            check(b"topsecret" in body, "the admin can read the user variables resource")

        mcp_shared_device_checks(d, admin)


def mcp_tool(d, cookie, sid, rid, name, args):
    _, body = mcp_call(d, cookie, sid, {"jsonrpc": "2.0", "id": rid, "method": "tools/call",
                                        "params": {"name": name, "arguments": args}})
    return body


def tool_failed(body):
    try:
        res = json.loads(body)
    except ValueError:
        return False
    return "error" in res or res.get("result", {}).get("isError") is True


def mcp_shared_device_checks(d, admin):
    """MCP honors the per-user device sharing exactly like the JSON device list."""
    print("\n=== MCP device sharing ===")
    code, body = api(d, admin, "param=addhardware&htype=15&name=ShareDummy&enabled=true"
                               "&port=0&address=&username=&password=&extra=&datatimeout=0")
    hw = json.loads(body).get("idx") if code == 200 else None
    if not check(hw is not None, "dummy hardware added"):
        return
    idx = {}
    for name in ("ShareSwitchA", "ShareSecretB"):
        code, body = api(d, admin, "param=createdevice&idx=%s&sensorname=%s&devicetype=244&devicesubtype=73"
                         % (hw, name))
        idx[name] = json.loads(body).get("idx") if code == 200 else None
        if not check(idx[name] is not None, "switch %s created" % name):
            return
    shared, secret = idx["ShareSwitchA"], idx["ShareSecretB"]

    # sviewer and sswitch only get ShareSwitchA; nshare has nothing shared and so sees every device
    for name, rights in (("sviewer", 0), ("sswitch", 1), ("nshare", 1)):
        code, body = api(d, admin, urllib.parse.urlencode({
            "param": "adduser", "enabled": "true", "username": name,
            "password": md5(name + "pw"), "rights": rights,
            "RemoteSharing": "false", "TabsEnabled": 0}))
        check(code == 200 and b'"OK"' in body, "%s account created" % name)
    code, body = api(d, admin, urllib.parse.urlencode({
        "param": "adduser", "enabled": "true", "username": "at:7",
        "password": md5("at7pw"), "rights": 0, "RemoteSharing": "false", "TabsEnabled": 0}))
    check(b'"OK"' not in body and b"at:" in body, "a user name that looks like an access token is refused")
    code, body = api(d, admin, "param=getusers")
    users ={u.get("Username"): u.get("idx") for u in json.loads(body).get("result", [])} if code == 200 else {}
    for name in ("sviewer", "sswitch"):
        code, body = api(d, admin, "param=setshareduserdevices&idx=%s&devices=%s" % (users.get(name), shared))
        check(code == 200 and b'"OK"' in body, "ShareSwitchA shared with %s" % name)

    cookies = {name: login(d.port, name, md5(name + "pw")) for name in ("sviewer", "sswitch", "nshare")}
    cookies["admin"] = admin
    sids = {}
    for name, cookie in cookies.items():
        sids[name] = mcp_session(d, cookie) if cookie else ""
        if not check(sids[name] != "", "%s MCP session initialised" % name):
            return

    def resource(name, uri):
        _, body = mcp_call(d, cookies[name], sids[name], {"jsonrpc": "2.0", "id": 50, "method": "resources/read",
                                                          "params": {"uri": uri}})
        return body

    for who in ("sviewer", "sswitch"):
        body = resource(who, "domoticz://device/%s" % secret)
        check(b"ShareSecretB" not in body and b"-32001" in body,
              "%s gets not found reading the unshared device resource" % who)
        body = resource(who, "domoticz://device/%s" % shared)
        check(b"ShareSwitchA" in body, "%s reads the shared device resource" % who)
        body = resource(who, "domoticz://devices")
        check(b"ShareSwitchA" in body and b"ShareSecretB" not in body,
              "%s sees only the shared device in the devices resource" % who)
        body = mcp_tool(d, cookies[who], sids[who], 51, "get_device", {"idx": int(secret)})
        check(tool_failed(body) and b"ShareSecretB" not in body, "%s gets not found from get_device by idx" % who)
        body = mcp_tool(d, cookies[who], sids[who], 52, "get_device", {"name": "ShareSecretB"})
        check(tool_failed(body) and b"No device found" in body, "%s gets not found from get_device by name" % who)
        body = mcp_tool(d, cookies[who], sids[who], 53, "get_sensor_value", {"idx": int(secret)})
        check(tool_failed(body) and b"ShareSecretB" not in body, "%s gets not found from get_sensor_value" % who)
        body = mcp_tool(d, cookies[who], sids[who], 54, "search_devices", {"query": "Share"})
        check(b"ShareSwitchA" in body and b"ShareSecretB" not in body,
              "%s finds only the shared device with search_devices" % who)
        body = mcp_tool(d, cookies[who], sids[who], 55, "get_all_devices", {})
        check(b"ShareSecretB" not in body, "%s does not see the unshared device in get_all_devices" % who)
        body = mcp_tool(d, cookies[who], sids[who], 56, "get_sensor_history", {"idx": int(secret)})
        check(tool_failed(body) and b"ShareSecretB" not in body, "%s gets not found from get_sensor_history" % who)
        body = mcp_tool(d, cookies[who], sids[who], 57, "get_switch_state", {"idx": int(secret)})
        check(tool_failed(body) and b"ShareSecretB" not in body, "%s gets not found from get_switch_state" % who)
        _, body = mcp_call(d, cookies[who], sids[who], {"jsonrpc": "2.0", "id": 58, "method": "resources/subscribe",
                                                        "params": {"uri": "domoticz://devices/%s" % secret}})
        check(b"-32001" in body, "%s cannot subscribe to the unshared device" % who)

    body = mcp_tool(d, cookies["sswitch"], sids["sswitch"], 60, "set_switch_state", {"idx": int(secret), "state": "On"})
    check(tool_failed(body), "the restricted switcher cannot switch the unshared device")
    code, body = api(d, admin, "param=getdevices&rid=%s" % secret)
    dev = (json.loads(body).get("result") or [{}])[0] if code == 200 else {}
    check(dev.get("Status") == "Off", "the unshared device is still Off (got %r)" % dev.get("Status"))
    body = mcp_tool(d, cookies["sswitch"], sids["sswitch"], 61, "toggle_switch_state", {"idx": int(secret)})
    check(tool_failed(body), "the restricted switcher cannot toggle the unshared device")
    body = mcp_tool(d, cookies["sswitch"], sids["sswitch"], 62, "set_switch_state", {"idx": int(shared), "state": "On"})
    check(not tool_failed(body), "the restricted switcher can switch the shared device")

    for who in ("admin", "nshare"):
        body = resource(who, "domoticz://devices")
        check(b"ShareSwitchA" in body and b"ShareSecretB" in body, "%s sees every device in the devices resource" % who)
        body = mcp_tool(d, cookies[who], sids[who], 63, "get_device", {"idx": int(secret)})
        check(not tool_failed(body) and b"ShareSecretB" in body, "%s reads the device with get_device" % who)

    # An access token has no device shares, so like the JSON device list it sees every device
    code, body = api(d, admin, "param=createaccesstoken&name=ShareToken&rights=0&expiry=0")
    token = json.loads(body).get("token") if code == 200 else None
    if check(bool(token), "viewer access token created"):
        bearer = "Bearer " + token
        tsid = mcp_session(d, bearer)
        if check(tsid != "", "access token MCP session initialised"):
            _, body = mcp_call(d, bearer, tsid, {"jsonrpc": "2.0", "id": 64, "method": "resources/read",
                                                 "params": {"uri": "domoticz://devices"}})
            check(b"ShareSwitchA" in body and b"ShareSecretB" in body,
                  "the access token sees every device in the devices resource")
            body = mcp_tool(d, bearer, tsid, 65, "get_device", {"idx": int(secret)})
            check(not tool_failed(body) and b"ShareSecretB" in body, "the access token reads the device with get_device")

    mcp_notification_checks(d, admin, cookies, sids, shared, secret)


def mcp_notification_checks(d, admin, cookies, sids, shared, secret):
    """SSE notifications follow the device sharing, and log lines only reach admins."""
    print("\n=== MCP notifications ===")
    _, body = mcp_call(d, cookies["sviewer"], sids["sviewer"], {"jsonrpc": "2.0", "id": 70,
                       "method": "logging/setLevel", "params": {"level": "debug"}})
    check(b'"error"' in body and b"requires admin" in body, "a non-admin cannot set the MCP log level")

    streams = {who: SseStream(d.port, cookies[who], sids[who]) for who in ("admin", "sviewer", "nshare")}
    try:
        for who, s in streams.items():
            if not check(s.wait_for(b"notifications/message"), "%s SSE stream connected" % who):
                return
        api(d, admin, "param=switchlight&idx=%s&switchcmd=On" % secret)
        api(d, admin, "param=switchlight&idx=%s&switchcmd=Off" % shared)
        probe = b"McpLogProbe7315"
        api(d, admin, "param=addlogmessage&level=4&message=%s" % probe.decode())

        secret_uri = b'"domoticz://devices/%s"' % str(secret).encode()
        shared_uri = b'"domoticz://devices/%s"' % str(shared).encode()
        check(streams["admin"].wait_for(secret_uri), "admin is notified of the unshared device update")
        check(streams["admin"].wait_for(probe), "admin receives error log lines")
        check(streams["nshare"].wait_for(secret_uri),
              "a user with nothing shared is notified of every device update")
        check(streams["sviewer"].wait_for(shared_uri), "sviewer is notified of the shared device update")
        # A second update of the shared device is sent after the unshared update and the log line.
        # Once a stream has it, anything leaked before it would already have arrived as well.
        before = {}
        for who in ("sviewer", "nshare"):
            with streams[who].lock:
                before[who] = streams[who].buf.count(shared_uri)
        api(d, admin, "param=switchlight&idx=%s&switchcmd=On" % shared)
        for who in ("sviewer", "nshare"):
            check(streams[who].wait_for_count(shared_uri, before[who] + 1),
                  "%s receives the later shared device update" % who)
        check(not streams["sviewer"].contains(secret_uri), "sviewer is not notified of the unshared device update")
        for who in ("sviewer", "nshare"):
            check(not streams[who].contains(probe), "%s receives no log lines" % who)
    finally:
        for s in streams.values():
            s.close()


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    exe = sys.argv[1]
    if not os.path.exists(exe):
        print("domoticz executable not found: %s" % exe)
        return 2

    regs = static_checks(repo_root(exe))
    runtime_checks(exe, regs)

    print("\n%d checks, %d failure(s)" % (CHECKS, FAILURES))
    for f in FAILED:
        print("  - %s" % f)
    return 0 if FAILURES == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
