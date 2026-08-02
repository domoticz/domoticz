#!/usr/bin/env python3
"""Sweep the whole Domoticz JSON API against the running webserver.

Purpose: libwebem sits under every one of these endpoints. A change to the
webserver library can break the API without breaking the build, and without
being visible in the small BDD suite (which covers a handful of URIs). This
walks the entire registered command surface and reports anything the HTTP
layer refuses to carry.

The distinction that matters here:

  * A TRANSPORT failure is libwebem's fault -- the connection was reset, the
    request timed out, or the server answered 400/413/414/431/501 to a short,
    well-formed GET. No API command should ever produce one.

  * An APPLICATION response is Domoticz's business and is NOT a failure.
    Calling "addhardware" with no arguments legitimately answers 200 with
    {"status":"ERR"}. This script deliberately does not judge those; doing so
    would drown a real regression in hundreds of expected errors.

The command list is extracted from main/WebServer.cpp at run time rather than
hard-coded, so it cannot drift out of date as endpoints are added or removed.

Beyond the sweep it probes the specific limits libwebem now enforces, since
those are what a security-hardening change is most likely to have set too
tight for real Domoticz traffic:

    request line length, header count, header size, request body size,
    requests per keep-alive connection, and the response headers themselves.

Usage:
    python test_api_sweep.py <path-to-domoticz.exe> [--keep-going]

It starts its own Domoticz on a free port with a throwaway database and user
data folder, so it never touches an existing installation or its data.
"""
import base64
import http.client
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

CHECKS = 0
FAILURES = 0
FAILED_LABELS = []

# Status codes that mean "libwebem refused to carry this request". For a short,
# well-formed GET to a registered endpoint, every one of these is a regression.
#
# 400 is deliberately absent. Domoticz itself answers 400 with a JSON body for
# endpoints that require a POST or specific parameters (logincheck,
# setupwizardcreateadmin, passkeylogin-complete all do), and that is a correct
# application response, not a transport failure. A 400 that came from the
# library instead carries libwebem's stock HTML error page, so the two are told
# apart by the body -- see looks_like_app_response().
TRANSPORT_REFUSALS = {413, 414, 431, 501}


def looks_like_app_response(body):
    """True if Domoticz answered, false if libwebem's stock error page did.

    libwebem's stock replies are HTML ("<html><head><title>..."); every
    Domoticz API answer is JSON. Distinguishing them is what keeps an
    application-level 400 from being misreported as a library regression.
    """
    if not body:
        return False
    head = body.lstrip()[:1]
    if head not in (b"{", b"["):
        return False
    try:
        json.loads(body.decode("utf-8", "replace"))
        return True
    except Exception:
        return False


def check(cond, label):
    global CHECKS, FAILURES
    CHECKS += 1
    if cond:
        print("  PASS  " + label)
    else:
        print("  FAIL  " + label)
        FAILURES += 1
        FAILED_LABELS.append(label)
    return bool(cond)


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


def extract_commands(root):
    """Pull every registered param= name out of WebServer.cpp.

    Returns (command_codes, action_codes). Extracting rather than hard-coding
    keeps this test honest as the API grows.
    """
    path = os.path.join(root, "main", "WebServer.cpp")
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        src = fh.read()
    cmds = sorted(set(re.findall(r'RegisterCommandCode\("([A-Za-z0-9_\-]+)"', src)))
    acts = sorted(set(re.findall(r'RegisterActionCode\("([A-Za-z0-9_\-]+)"', src)))
    return cmds, acts


class EarlyExit(RuntimeError):
    """Domoticz died during startup -- worth retrying on a fresh port/database."""


def domoticz_running():
    """True if any domoticz process is alive.

    Domoticz takes a global "Local\\Domoticz" mutex at startup
    (main/domoticz.cpp) and exits immediately if another instance already holds
    it -- without writing anything to its log, which is exactly what an
    unexplained rc=1 and an empty log file look like. Tests therefore cannot
    run concurrently with any other instance, including one that is still
    shutting down from a previous test.
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


def start_domoticz(exe, www_root, attempts=5):
    """Start Domoticz, waiting out the single-instance mutex and retrying."""
    last = None
    for i in range(attempts):
        if not wait_for_no_domoticz():
            print("  (another domoticz is still running; the single-instance "
                  "mutex will refuse this start)")
        try:
            return Domoticz(exe, www_root)
        except EarlyExit as e:
            last = e
            print("  (startup attempt %d/%d failed: %s -- retrying)"
                  % (i + 1, attempts, e))
            time.sleep(3)
    raise RuntimeError("domoticz would not start after %d attempts: %s"
                       % (attempts, last))


class Domoticz:
    """A throwaway Domoticz instance: own port, own database, own userdata."""

    def __init__(self, exe, www_root):
        self.exe = os.path.abspath(exe)
        self.port = free_port()
        self.tmp = tempfile.mkdtemp(prefix="domo_api_sweep_")
        self.db = os.path.join(self.tmp, "domoticz.db")
        self.log = open(os.path.join(self.tmp, "domoticz.log"), "w", encoding="utf-8",
                        errors="replace")
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
            "-loglevel", "error",
        ]
        self.proc = subprocess.Popen(args, stdout=self.log, stderr=subprocess.STDOUT,
                                     cwd=www_root)
        self._wait_ready()

    def _wait_ready(self):
        deadline = time.time() + 90
        last = None
        while time.time() < deadline:
            if self.proc.poll() is not None:
                # Domoticz startup is intermittently flaky on this platform --
                # it occasionally exits 1 before writing anything to its log,
                # with no traffic involved at all (reproduced with the server
                # left completely idle). Retrying is the pragmatic answer; the
                # caller re-runs us on a fresh port and database.
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

    def stop(self):
        # Must fully exit before the next instance can start: Domoticz holds a
        # global single-instance mutex (see domoticz_running()).
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
        shutil.rmtree(self.tmp, ignore_errors=True)

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.cleanup()


# --------------------------------------------------------------------------
# raw-socket helpers, for the probes that must send deliberately malformed or
# oversized requests urllib would refuse to build
# --------------------------------------------------------------------------

def raw_request(port, payload, read_timeout=10):
    """Send raw bytes, return (status_code_or_None, raw_response_bytes)."""
    s = socket.create_connection(("127.0.0.1", port), timeout=read_timeout)
    s.settimeout(read_timeout)
    try:
        s.sendall(payload)
        buf = b""
        while True:
            try:
                c = s.recv(65536)
            except socket.timeout:
                break
            if not c:
                break
            buf += c
            if len(buf) > 4 * 1024 * 1024:
                break
    finally:
        s.close()
    if not buf:
        return None, b""
    try:
        return int(buf.split(b"\r\n", 1)[0].split(b" ")[1]), buf
    except (IndexError, ValueError):
        return None, buf


def connection_reuse_count(port, extra_headers, limit):
    """How many requests one socket serves before the server closes it.

    Reads each response fully (headers plus Content-Length body) so the next
    response starts at a clean boundary -- without that the count is measuring
    the parser's pipelining behaviour rather than connection reuse.
    """
    req = ("GET /json.htm?type=command&param=getversion HTTP/1.1\r\n"
           "Host: 127.0.0.1\r\n" + extra_headers + "\r\n").encode()
    s = socket.create_connection(("127.0.0.1", port), timeout=15)
    s.settimeout(15)
    n = 0
    try:
        for _ in range(limit):
            s.sendall(req)
            buf = b""
            while b"\r\n\r\n" not in buf:
                c = s.recv(8192)
                if not c:
                    raise ConnectionError("closed")
                buf += c
            head, _, body = buf.partition(b"\r\n\r\n")
            clen = 0
            for line in head.decode("latin-1").split("\r\n"):
                if line.lower().startswith("content-length:"):
                    clen = int(line.split(":", 1)[1].strip())
            while len(body) < clen:
                c = s.recv(8192)
                if not c:
                    raise ConnectionError("closed mid-body")
                body += c
            n += 1
    except Exception:
        pass
    finally:
        s.close()
    return n


def make_session(port):
    """Create an admin and log in; returns a "Cookie: ..." header or "".

    Needed for the WebSocket check: Domoticz authenticates WS upgrades
    unconditionally (libwebem cWebem.cpp step 9 -- an upgrade needs auth unless
    the URI is whitelisted), even where the same path served over plain HTTP is
    open. Without a session the upgrade is answered 401 and nothing downstream
    is exercised.
    """
    form = urllib.parse.urlencode({"username": "admin", "password": "testpw"}).encode()
    raw_request(port, (b"POST /json.htm?type=command&param=setupwizardcreateadmin "
                       b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                       b"Content-Type: application/x-www-form-urlencoded\r\n"
                       b"Content-Length: " + str(len(form)).encode() +
                       b"\r\nConnection: close\r\n\r\n" + form))
    creds = urllib.parse.urlencode({
        "username": base64.b64encode(b"admin").decode(),
        "password": "testpw",
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
                return "Cookie: %s\r\n" % val
    return ""


def ws_probe(port, cookie="", origin="http://127.0.0.1"):
    """Drive Domoticz's WebSocket API end to end over a raw socket.

    libwebem's WebSocket path changed substantially (frame-size and message
    limits, the reassembly state machine, and a strand serialising writes so
    the 101 cannot be overtaken by frames queued from an application thread).
    None of that is exercised by the HTTP sweep, and Domoticz registers its
    endpoint at "/" with the "domoticz" subprotocol.

    Returns (handshake_ok, negotiated_protocol, reply_text_or_None).
    """
    key = base64.b64encode(b"0123456789abcdef").decode()
    s = socket.create_connection(("127.0.0.1", port), timeout=15)
    s.settimeout(15)
    s.sendall(("GET / HTTP/1.1\r\n"
               "Host: 127.0.0.1\r\n"
               "Upgrade: websocket\r\n"
               "Connection: Upgrade\r\n"
               "Sec-WebSocket-Key: %s\r\n"
               "Sec-WebSocket-Protocol: domoticz\r\n"
               "Origin: %s\r\n"
               "Sec-WebSocket-Version: 13\r\n"
               "%s\r\n" % (key, origin, cookie)).encode())
    buf = b""
    try:
        while b"\r\n\r\n" not in buf:
            c = s.recv(4096)
            if not c:
                break
            buf += c
    except socket.timeout:
        pass
    head = buf.split(b"\r\n\r\n", 1)[0].decode("latin-1")
    ok = head.startswith("HTTP/1.1 101")
    proto = None
    for line in head.split("\r\n"):
        if line.lower().startswith("sec-websocket-protocol:"):
            proto = line.split(":", 1)[1].strip()
    if not ok:
        s.close()
        return False, proto, None

    # masked client text frame carrying a Domoticz WS API request
    payload = json.dumps({"event": "request", "requestid": 1,
                          "query": "type=command&param=getversion"}).encode()
    mask = b"\xa1\xb2\xc3\xd4"
    masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
    if len(payload) < 126:
        hdr = bytes([0x81, 0x80 | len(payload)])
    else:
        hdr = bytes([0x81, 0x80 | 126]) + len(payload).to_bytes(2, "big")
    s.sendall(hdr + mask + masked)

    # read one unmasked server frame back
    reply = None
    try:
        fr = s.recv(65536)
        if fr and len(fr) >= 2:
            ln = fr[1] & 0x7F
            off = 2
            if ln == 126:
                ln = int.from_bytes(fr[2:4], "big")
                off = 4
            elif ln == 127:
                ln = int.from_bytes(fr[2:10], "big")
                off = 10
            reply = fr[off:off + ln].decode("utf-8", "replace")
    except socket.timeout:
        pass
    s.close()
    return ok, proto, reply


def sweep(domo, names, kind):
    """Call every endpoint and report only transport-level breakage."""
    print("\n=== %s sweep (%d endpoints) ===" % (kind, len(names)))
    transport_fail = []
    server_error = []
    ok = 0
    for name in names:
        path = "/json.htm?type=command&param=" + name
        try:
            code, body = domo.get(path, timeout=20)
        except Exception as e:
            transport_fail.append((name, "connection: %s" % e))
            continue
        if code in TRANSPORT_REFUSALS:
            transport_fail.append((name, "HTTP %d" % code))
        elif code == 400 and not looks_like_app_response(body):
            # A 400 with libwebem's stock HTML page means the library refused
            # the request; a 400 with a JSON body is Domoticz saying "wrong
            # method / missing parameters", which is expected here.
            transport_fail.append((name, "HTTP 400 (library stock reply)"))
        elif code >= 500:
            server_error.append((name, "HTTP %d" % code))
        else:
            ok += 1

    print("  %d/%d endpoints answered at the HTTP level" % (ok, len(names)))
    check(not transport_fail,
          "%s: no endpoint was refused by the HTTP layer%s"
          % (kind, "" if not transport_fail else " -- " + ", ".join(
              "%s (%s)" % t for t in transport_fail[:10])))
    if server_error:
        # 500s are Domoticz throwing, not libwebem refusing -- surface them but
        # do not fail the run, since a fresh database legitimately lacks the
        # hardware/devices many commands operate on.
        print("  NOTE  %d endpoint(s) returned 5xx (application-level, listed for "
              "information): %s" % (len(server_error),
                                    ", ".join(n for n, _ in server_error[:15])))
    return transport_fail


def probe_limits(domo):
    """Exercise each new libwebem limit at a size real Domoticz traffic uses."""
    port = domo.port
    print("\n=== request-line length (max_request_line_length = 8 KiB) ===")
    # Domoticz builds genuinely long URLs: multi-device graph requests, long
    # device names, base64 in query strings.
    for size, expect_ok in ((2000, True), (7000, True)):
        q = "/json.htm?type=command&param=getversion&pad=" + ("a" * size)
        code, _ = domo.get(q, timeout=10)
        check(code == 200, "URI of ~%d bytes is accepted (status=%s)" % (size, code))
    over = "/json.htm?type=command&param=getversion&pad=" + ("a" * 9000)
    code, _ = raw_request(port, ("GET %s HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                                 "Connection: close\r\n\r\n" % over).encode())
    # 414, specifically -- not 431. Nothing is wrong with this request's
    # headers, and answering 431 sends the client looking in the wrong place.
    check(code == 414, "URI over 8 KiB is rejected as 414 URI Too Long "
                       "(status=%s)" % code)

    print("\n=== header count (max_header_count = 100) ===")
    hdrs = "".join("X-Pad-%d: v\r\n" % i for i in range(60))
    code, _ = raw_request(port, ("GET /json.htm?type=command&param=getversion HTTP/1.1\r\n"
                                 "Host: 127.0.0.1\r\n%sConnection: close\r\n\r\n"
                                 % hdrs).encode())
    check(code == 200, "60 headers accepted (status=%s)" % code)
    hdrs = "".join("X-Pad-%d: v\r\n" % i for i in range(200))
    code, _ = raw_request(port, ("GET /json.htm?type=command&param=getversion HTTP/1.1\r\n"
                                 "Host: 127.0.0.1\r\n%sConnection: close\r\n\r\n"
                                 % hdrs).encode())
    check(code in (400, 431), "200 headers rejected cleanly (status=%s)" % code)

    print("\n=== single header size (max_header_length = 8 KiB) ===")
    # A session cookie or bearer token is the realistic large header.
    code, _ = domo.get("/json.htm?type=command&param=getversion", timeout=10,
                       headers={"Authorization": "Bearer " + "t" * 4000})
    check(code in (200, 401, 403), "4 KiB Authorization header is carried "
                                   "(status=%s)" % code)

    print("\n=== request body size (max_request_body_size = 100 MB) ===")
    # The governing case: restoredatabase POSTs a whole domoticz.db, which on
    # an established install reaches 75 MB. That must work on shipped defaults.
    for mib, label in ((1, "1 MiB"), (8, "8 MiB"), (75, "75 MB (database restore)")):
        body = b"x" * (mib * 1024 * 1024)
        payload = (b"POST /json.htm?type=command&param=getversion HTTP/1.1\r\n"
                   b"Host: 127.0.0.1\r\nContent-Type: application/octet-stream\r\n"
                   b"Content-Length: " + str(len(body)).encode() + b"\r\n"
                   b"Connection: close\r\n\r\n" + body)
        code, _ = raw_request(port, payload, read_timeout=40)
        check(code is not None and code not in TRANSPORT_REFUSALS,
              "%s POST body accepted (status=%s)" % (label, code))

    # Above the fixed kMaxContentLength ceiling (100 MB), which no setting can
    # raise -- so this is refused on the declared length alone.
    body_len = 150 * 1024 * 1024
    payload_head = (b"POST /json.htm?type=command&param=getversion HTTP/1.1\r\n"
                    b"Host: 127.0.0.1\r\nContent-Type: application/octet-stream\r\n"
                    b"Content-Length: " + str(body_len).encode() + b"\r\n"
                    b"Connection: close\r\n\r\n")
    s = socket.create_connection(("127.0.0.1", port), timeout=30)
    s.settimeout(30)
    code = None
    try:
        s.sendall(payload_head)
        # The server should reject on the declared length alone, before we send
        # 20 MiB. Read the answer first; only push data if it stays silent.
        try:
            resp = s.recv(4096)
            if resp:
                code = int(resp.split(b"\r\n", 1)[0].split(b" ")[1])
        except socket.timeout:
            code = None
    finally:
        s.close()
    # 413, specifically. A client told "431 Request Header Fields Too Large"
    # after offering an oversized upload has no way to act on that.
    check(code == 413,
          "150 MB declared body is rejected as 413 Payload Too Large on the "
          "declared Content-Length alone, before the bytes are sent (status=%s)"
          % code)

    print("\n=== keep-alive (HTTP/1.1 persistence) ===")
    # HTTP/1.1 is persistent by DEFAULT (RFC 9112 s9.3). The connection must be
    # reused by a client that sends no Connection header at all -- which is
    # what Python's http.client, curl and most API clients do. libwebem
    # previously required an explicit "Connection: Keep-Alive" and closed after
    # one request otherwise, so every API call paid a fresh TCP handshake.
    bare = connection_reuse_count(port, "", limit=130)
    check(bare > 1, "connection is reused with NO Connection header, per the "
                    "HTTP/1.1 default (served %d request(s) on one socket)" % bare)
    check(bare == 100, "the connection closes at exactly the advertised budget "
                       "of 100 requests (served=%d)" % bare)

    explicit = connection_reuse_count(port, "Connection: Keep-Alive\r\n", limit=130)
    check(explicit == 100, "an explicit Connection: Keep-Alive behaves the same "
                           "(served=%d)" % explicit)

    # "Connection: close" must still be honoured, and must be honoured when it
    # appears as one token among several rather than as the whole value.
    closed = connection_reuse_count(port, "Connection: close\r\n", limit=5)
    check(closed == 1, "Connection: close is honoured (served=%d)" % closed)
    closed_list = connection_reuse_count(port, "Connection: TE, close\r\n", limit=5)
    check(closed_list == 1, "close is honoured inside a token list 'TE, close' "
                            "(served=%d)" % closed_list)

    # HTTP/1.0 keeps the opposite default: closed unless keep-alive is asked for.
    s = socket.create_connection(("127.0.0.1", port), timeout=10)
    s.settimeout(10)
    s.sendall(b"GET /json.htm?type=command&param=getversion HTTP/1.0\r\n"
              b"Host: 127.0.0.1\r\n\r\n")
    buf = b""
    while True:
        try:
            c = s.recv(8192)
        except socket.timeout:
            break
        if not c:
            break
        buf += c
    s.close()
    check(buf.startswith(b"HTTP/1."), "HTTP/1.0 request is answered")

    # A response that closes must say so, rather than letting the client find
    # out when its next request fails.
    _, raw = raw_request(port, b"GET /json.htm?type=command&param=getversion "
                               b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                               b"Connection: close\r\n\r\n")
    head = raw.split(b"\r\n\r\n", 1)[0].lower()
    check(b"connection: close" in head,
          "a response that closes the connection announces Connection: close")

    _, raw = raw_request(port, b"GET /json.htm?type=command&param=getversion "
                               b"HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n")
    head = raw.split(b"\r\n\r\n", 1)[0].lower()
    check(b"connection: close" not in head,
          "a response that keeps the connection does NOT announce close")

    print("\n=== response headers ===")
    code, raw = raw_request(port, b"GET /json.htm?type=command&param=getversion "
                                  b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                                  b"Origin: http://evil.example\r\n"
                                  b"Connection: close\r\n\r\n")
    head = raw.split(b"\r\n\r\n", 1)[0].lower()
    check(b"access-control-allow-origin: *" not in head,
          "no wildcard Access-Control-Allow-Origin is emitted")
    check(code == 200, "request with a foreign Origin still answered (status=%s)" % code)

    # Deliberately last: this creates a user, which permanently changes the
    # server's authentication state for everything after it.
    print("\n=== websocket API: no users configured ===")
    # With no users, HTTP is already open to anyone, so demanding credentials
    # for the upgrade alone would protect nothing while breaking live updates.
    ok, proto, reply = ws_probe(port)
    check(ok, "an unauthenticated upgrade succeeds when no users are configured")
    check(proto == "domoticz",
          "the 'domoticz' subprotocol is negotiated (got %r)" % proto)
    check(reply is not None and "GetVersion" in (reply or ""),
          "a request sent over that WebSocket is answered (reply=%r)"
          % ((reply or "")[:120],))

    # ...but the origin check is NOT relaxed with it. WebSocket is not subject
    # to CORS: without this, any site the user visits could open a socket to an
    # unprotected Domoticz and read or drive it. A cross-origin HTTP response
    # the browser would refuse to hand over is no protection here.
    ok_evil, _, _ = ws_probe(port, origin="http://evil.example")
    check(not ok_evil,
          "a cookie-less upgrade from a foreign Origin is refused even with no "
          "users configured")

    print("\n=== websocket API: users configured ===")
    cookie = make_session(port)
    if not cookie:
        check(False, "could not obtain a session cookie to test the WebSocket")
        return
    ok_anon, _, _ = ws_probe(port)
    check(not ok_anon,
          "once a user exists, an unauthenticated upgrade is refused")
    ok, proto, reply = ws_probe(port, cookie)
    check(ok, "an authenticated upgrade succeeds (101)")
    check(proto == "domoticz",
          "the 'domoticz' subprotocol is still negotiated (got %r)" % proto)
    check(reply is not None and "GetVersion" in (reply or ""),
          "a request sent over the authenticated WebSocket is answered (reply=%r)"
          % ((reply or "")[:120],))


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

    cmds, acts = extract_commands(root)
    print("extracted %d command codes and %d action codes from main/WebServer.cpp"
          % (len(cmds), len(acts)))

    with start_domoticz(exe, root) as domo:
        print("domoticz running on port %d (temp data in %s)" % (domo.port, domo.tmp))
        sweep(domo, cmds, "command")
        probe_limits(domo)

    print("\n%d checks, %d failure(s)" % (CHECKS, FAILURES))
    for lbl in FAILED_LABELS:
        print("  - %s" % lbl)
    return 0 if FAILURES == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
