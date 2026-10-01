#!/usr/bin/env python3
"""Regression test for HTTP response splitting via the OAuth2 authorize endpoint.

Reported privately 2026-07-30 (CWE-113). `GET /oauth2/v1/authorize` writes the
`state` and `redirect_uri` query parameters straight into the `Location:`
header of its 302. Both are attacker-controlled and `redirect_uri` is
URL-decoded first, so a `%0d%0a` in either terminated the header and let an
unauthenticated client append arbitrary headers -- or, with `%0d%0a%0d%0a`, a
complete fabricated second response, which on a keep-alive socket is answered
to the *next* request on that connection as same-origin content.

The endpoint answers without credentials by design, so this needed no login.

Two independent defences have to hold, and this test drives both:

  * libwebem `reply::add_header()` refuses any name or value containing a
    control character, so a poisoned `Location` never reaches the wire.
  * Domoticz percent-encodes `state` before building the redirect, so a
    legitimate-but-awkward state does not silently lose its Location header.

Checks are written against raw sockets on purpose: an HTTP client library
normalises or rejects the split bytes and would hide the very thing under test.

Usage:
    python test_oauth2_response_splitting.py <path-to-domoticz[.exe]>
"""
import json
import os
import socket
import sys
import urllib.parse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_api_sweep import (start_domoticz, repo_root, make_session,  # noqa: E402
                            raw_request)

CHECKS = 0
FAILURES = 0
FAILED = []

# Exactly the payloads from the report.
POC_STATE = ("x%0d%0aSet-Cookie%3A%20DMZSID%3Dpg15injected%3B%20Path%3D%2F"
             "%0d%0aX-Injected-By%3A%20pg15")
POC_REDIRECT = "https%3A%2F%2Fa.tld%2F%0d%0aX-Via-RedirectUri%3A%20yes"
POC_DESYNC = ("x%0d%0aContent-Length%3A%200%0d%0a%0d%0aHTTP%2F1.1%20200%20OK"
              "%0d%0aContent-Type%3A%20text%2Fhtml%0d%0aContent-Length%3A%2044"
              "%0d%0a%0d%0a%3Cscript%3Ealert(document.domain)%3C%2Fscript%3E")


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


def raw_get(port, path, keep_alive=False):
    """Send one request, return the raw response bytes verbatim."""
    s = socket.create_connection(("127.0.0.1", port), timeout=10)
    s.settimeout(10)
    conn = "keep-alive" if keep_alive else "close"
    s.sendall(("GET %s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: %s\r\n\r\n"
               % (path, conn)).encode())
    buf = b""
    while True:
        try:
            c = s.recv(65536)
        except socket.timeout:
            break
        if not c:
            break
        buf += c
        if not keep_alive and len(buf) > 1 << 20:
            break
        if keep_alive and b"\r\n\r\n" in buf:
            break
    s.close()
    return buf


def header_block(raw):
    return raw.split(b"\r\n\r\n", 1)[0]


def status_lines(raw):
    """Every HTTP status line in the response -- more than one means a split."""
    return [ln for ln in raw.split(b"\r\n") if ln.startswith(b"HTTP/1.")]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    exe = sys.argv[1]
    if not os.path.exists(exe):
        print("domoticz executable not found: %s" % exe)
        return 2

    with start_domoticz(exe, repo_root(exe)) as d:
        p = d.port
        print("domoticz on port %d\n" % p)

        print("=== the reported payload: CRLF in `state` ===")
        raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                         "&redirect_uri=https%3A%2F%2Fattacker.tld%2F"
                         "&state=" + POC_STATE)
        head = header_block(raw).lower()
        check(b"set-cookie: dmzsid=pg15injected" not in head,
              "no attacker-injected Set-Cookie in the response headers")
        check(b"x-injected-by: pg15" not in head,
              "no attacker-injected X-Injected-By header")
        check(len(status_lines(raw)) == 1,
              "exactly one HTTP status line (got %d)" % len(status_lines(raw)))

        print("\n=== second sink: CRLF in `redirect_uri` ===")
        raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                         "&redirect_uri=" + POC_REDIRECT)
        head = header_block(raw).lower()
        check(b"x-via-redirecturi" not in head,
              "no injected header via redirect_uri")
        check(len(status_lines(raw)) == 1,
              "exactly one HTTP status line (got %d)" % len(status_lines(raw)))

        print("\n=== third sink: `state` with no redirect_uri at all ===")
        raw = raw_get(p, "/oauth2/v1/authorize?state=z%0d%0aX-NoRedirectUri%3A%20yes")
        head = header_block(raw).lower()
        check(b"x-noredirecturi" not in head,
              "no injected header on the missing-redirect_uri path")

        print("\n=== the impact claim: fabricated second response ===")
        raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                         "&redirect_uri=https%3A%2F%2Fa.tld%2F"
                         "&state=" + POC_DESYNC)
        check(len(status_lines(raw)) == 1,
              "a fabricated second response cannot be smuggled (status lines=%d)"
              % len(status_lines(raw)))
        check(b"<script>" not in raw.lower(),
              "no attacker-authored script body in the response")

        print("\n=== open redirect: no client_id means no redirect (RFC 6749 4.1.2.1) ===")
        # Nothing has established that this redirect_uri belongs to anyone, so
        # honouring it makes the endpoint an open redirect for any anonymous
        # caller. The error in the query string is no mitigation -- the browser
        # follows the 302 regardless.
        raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                         "&redirect_uri=https%3A%2F%2Fattacker.tld%2Fsteal")
        head = header_block(raw)
        check(b"attacker.tld" not in head,
              "an unverified redirect_uri is not echoed into a Location header")
        check(not head.split(b"\r\n", 1)[0].startswith(b"HTTP/1.1 302"),
              "no 302 is issued for an unknown client (got %r)"
              % head.split(b"\r\n", 1)[0])

        raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                         "&client_id=definitely-not-registered"
                         "&redirect_uri=https%3A%2F%2Fattacker.tld%2Fsteal")
        head = header_block(raw)
        check(b"attacker.tld" not in head,
              "an unregistered client_id does not get a redirect either")

        print("\n=== state is percent-encoded into the callback query ===")
        # Unencoded, "x&code=ATTACKER" injected a `code` parameter into the
        # client's callback -- the one parameter that callback exists to read.
        raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                         "&redirect_uri=https%3A%2F%2Fexample.com%2Fcb"
                         "&state=x%26code%3DATTACKER%26foo%3Dbar")
        head = header_block(raw)
        loc = b""
        for ln in head.split(b"\r\n"):
            if ln.lower().startswith(b"location:"):
                loc = ln
        # Either the request is refused outright (unknown client) or, if it is
        # redirected, the injected parameters must not appear as real ones.
        check(b"&code=ATTACKER" not in loc,
              "state cannot inject a `code` parameter into the callback (Location=%r)"
              % loc[:160])
        check(b"&foo=bar" not in loc,
              "state cannot inject arbitrary parameters into the callback")

        print("\n=== a REGISTERED client is still redirected normally ===")
        # The fixes above must not break the real flow. This needs a registered
        # application, so create one; without it the positive path is not
        # exercised at all and the tests above would pass vacuously against a
        # server that simply refuses every authorize request.
        cookie = make_session(p)
        client_id = "libwebem-oauth-test"
        if not cookie:
            check(False, "could not authenticate to register a test application")
        else:
            # A "public" client requires a PEM key file, so register a
            # confidential one with a secret instead -- either is fine here, the
            # point is only that FindClient() resolves the client_id.
            form = urllib.parse.urlencode({
                "enabled": "true", "public": "false",
                "applicationname": client_id, "secret": "test-secret",
            }).encode()
            _, addbody = raw_request(
                p, (b"POST /json.htm?type=command&param=addapplication "
                    b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                    b"Content-Type: application/x-www-form-urlencoded\r\n"
                    b"Content-Length: " + str(len(form)).encode() + b"\r\n"
                    + cookie.encode() + b"Connection: close\r\n\r\n" + form))
            check(b'"status" : "OK"' in addbody,
                  "test application registered (else the positive path below "
                  "would pass vacuously) -- got %r" % addbody[-160:])

            raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                             "&client_id=" + client_id +
                             "&redirect_uri=https%3A%2F%2Fexample.com%2Fcb"
                             "&state=abc123")
            head = header_block(raw)
            first = head.split(b"\r\n", 1)[0]
            redirected = first.startswith(b"HTTP/1.1 30") and b"Location:" in head
            # A login dialog (200) is also a legitimate outcome for a GET from an
            # unauthenticated user-agent; what must NOT happen is a refusal.
            check(redirected or first.startswith(b"HTTP/1.1 200"),
                  "a registered client is served normally, not refused (got %r)"
                  % first)
            if redirected:
                check(b"example.com/cb" in head,
                      "the registered client's redirect_uri is honoured")
                check(b"state=abc123" in head,
                      "the state is echoed back in the redirect")

            # A state that is merely awkward -- not an attack -- must survive
            # rather than have its Location silently dropped by the CRLF guard.
            raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                             "&client_id=" + client_id +
                             "&redirect_uri=https%3A%2F%2Fexample.com%2Fcb"
                             "&state=a%20b")
            head = header_block(raw)
            first = head.split(b"\r\n", 1)[0]
            check(not first.startswith(b"HTTP/1.1 400"),
                  "a state containing a space is not rejected outright (got %r)" % first)

            print("\n=== a failed login is not answered with a redirect ===")
            # The endpoint used to bounce the user-agent to redirect_uri after
            # three failed POSTs, which put the open redirect back within reach
            # of a caller who never authenticated but knows a client_id.
            for attempt in range(4):
                form = urllib.parse.urlencode({
                    "uname": "nosuchuser", "psw": "wrongpassword",
                }).encode()
                _, body = raw_request(
                    p, (b"POST /oauth2/v1/authorize?response_type=code"
                        b"&client_id=" + client_id.encode() +
                        b"&redirect_uri=https%3A%2F%2Fattacker.tld%2Fsteal "
                        b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                        b"Content-Type: application/x-www-form-urlencoded\r\n"
                        b"Content-Length: " + str(len(form)).encode() + b"\r\n"
                        b"Connection: close\r\n\r\n" + form))
                check(b"attacker.tld" not in body,
                      "failed login attempt %d does not leak the redirect target"
                      % (attempt + 1))

            print("\n=== a registered Redirect URI list is enforced ===")
            _, appsbody = raw_request(
                p, (b"GET /json.htm?type=command&param=getapplications "
                    b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                    + cookie.encode() + b"Connection: close\r\n\r\n"))
            app_idx = ""
            try:
                apps = json.loads(appsbody.split(b"\r\n\r\n", 1)[1].decode())
                for a in apps.get("result", []):
                    if a.get("Applicationname") == client_id:
                        app_idx = str(a.get("idx"))
            except (ValueError, IndexError):
                pass
            if check(app_idx != "",
                     "the test application can be looked up by name before updating it"):
                form = urllib.parse.urlencode({
                    "enabled": "true", "public": "false",
                    "applicationname": client_id, "secret": "test-secret",
                    "idx": app_idx,
                    "redirecturis": "https://example.com/cb\nhttp://127.0.0.1:8123/auth",
                }).encode()
                _, updbody = raw_request(
                    p, (b"POST /json.htm?type=command&param=updateapplication "
                        b"HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                        b"Content-Type: application/x-www-form-urlencoded\r\n"
                        b"Content-Length: " + str(len(form)).encode() + b"\r\n"
                        + cookie.encode() + b"Connection: close\r\n\r\n" + form))
                check(b'"status" : "OK"' in updbody,
                      "Redirect URIs stored on the test application -- got %r"
                      % updbody[-160:])

                raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                                 "&client_id=" + client_id +
                                 "&redirect_uri=https%3A%2F%2Fattacker.tld%2Fsteal")
                head = header_block(raw)
                first = head.split(b"\r\n", 1)[0]
                check(b"attacker.tld" not in head,
                      "a redirect_uri outside the registered list is not echoed back")
                # Without the allow-list this same request is answered with a
                # login dialog, so asserting only the absence of a Location
                # header would pass against an unpatched build too.
                check(first.startswith(b"HTTP/1.1 400"),
                      "a redirect_uri outside the registered list is refused outright"
                      " (got %r)" % first)

                # RFC 8252 section-7.3 loopback redirects arrive on whatever
                # ephemeral port the native app got this launch, so the port
                # cannot take part in the match.
                raw = raw_get(p, "/oauth2/v1/authorize?response_type=code"
                                 "&client_id=" + client_id +
                                 "&redirect_uri=http%3A%2F%2F127.0.0.1%3A54321%2Fauth")
                first = header_block(raw).split(b"\r\n", 1)[0]
                check(not first.startswith(b"HTTP/1.1 400"),
                      "a registered loopback URI matches on a different port (got %r)"
                      % first)

    print("\n%d checks, %d failure(s)" % (CHECKS, FAILURES))
    for f in FAILED:
        print("  - %s" % f)
    return 0 if FAILURES == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
