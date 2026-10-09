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

Usage:
    python test_command_rights.py <path-to-domoticz[.exe]>
"""
import base64
import glob
import hashlib
import json
import os
import re
import sys
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
               "Accept": "application/json, text/event-stream", "Cookie": cookie}
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
