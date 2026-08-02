#!/usr/bin/env python3
"""
Verify that the WebSocket request path enforces authentication.

Background
----------
The WebSocket endpoint dispatches json.htm commands. Before the fix an
unauthenticated client could upgrade (HTTP 101) on a password-protected
instance and read viewer-level data (e.g. getdevices). After the fix the
upgrade is refused unless the client is authenticated.

This script tests two things against a *password-protected* instance
(i.e. at least one active user defined, and the client is NOT in
trusted-networks):

  1. NEGATIVE: a cookieless / unauthenticated WS upgrade must be REFUSED
     (no "101 Switching Protocols"; expect 401/403). This is the fix.

  2. POSITIVE (regression): an authenticated WS upgrade must still succeed
     (101) and a getdevices request must still return the device list.
     Authentication uses HTTP Basic (allowed over HTTPS, or over HTTP when
     "Allow plain http basic auth" is enabled in Settings).

Usage
-----
  # negative test only (no credentials needed)
  python test_ws_auth.py --host 127.0.0.1 --port 8080

  # both tests (provide a valid login for the regression check)
  python test_ws_auth.py --host 127.0.0.1 --port 8080 --user admin --password domoticz

  # TLS
  python test_ws_auth.py --host my.host --port 443 --tls --user admin --password secret

Exit code 0 = all selected checks passed, 1 = a check failed.
Pure standard library; no external dependencies.
"""
import argparse
import base64
import json
import os
import socket
import ssl
import struct
import sys
import time


def open_sock(host, port, use_tls, timeout):
    raw = socket.create_connection((host, port), timeout=timeout)
    if use_tls:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        return ctx.wrap_socket(raw, server_hostname=host)
    return raw


def ws_handshake(host, port, use_tls, auth_header=None, timeout=15):
    """Perform a WS upgrade. Returns (status_line, raw_leftover_bytes, sock)."""
    s = open_sock(host, port, use_tls, timeout)
    key = base64.b64encode(os.urandom(16)).decode()
    lines = [
        "GET / HTTP/1.1",
        f"Host: {host}",
        "Upgrade: websocket",
        "Connection: Upgrade",
        f"Sec-WebSocket-Key: {key}",
        "Sec-WebSocket-Version: 13",
        "Sec-WebSocket-Protocol: domoticz",
        f"Origin: http{'s' if use_tls else ''}://{host}",
    ]
    if auth_header:
        lines.append(f"Authorization: {auth_header}")
    req = ("\r\n".join(lines) + "\r\n\r\n").encode()
    s.sendall(req)

    buf = b""
    s.settimeout(timeout)
    while b"\r\n\r\n" not in buf:
        chunk = s.recv(4096)
        if not chunk:
            break
        buf += chunk
    head, _, rest = buf.partition(b"\r\n\r\n")
    status_line = head.split(b"\r\n")[0].decode(errors="replace") if head else ""
    return status_line, rest, s


def send_text(sock, payload):
    """Send a single masked text frame (client frames must be masked)."""
    d = payload.encode()
    m = os.urandom(4)
    n = len(d)
    h = bytearray([0x81])
    if n < 126:
        h.append(0x80 | n)
    elif n < 65536:
        h.append(0x80 | 126)
        h.extend(struct.pack(">H", n))
    else:
        h.append(0x80 | 127)
        h.extend(struct.pack(">Q", n))
    h += m
    sock.sendall(bytes(h) + bytes(b ^ m[i % 4] for i, b in enumerate(d)))


def drain(sock, seconds=2):
    data = b""
    sock.settimeout(seconds)
    end = time.time() + seconds
    try:
        while time.time() < end:
            chunk = sock.recv(65536)
            if not chunk:
                break
            data += chunk
    except Exception:
        pass
    return data


def test_unauthenticated_refused(args):
    print("[*] NEGATIVE: unauthenticated WS upgrade must be refused ...")
    try:
        status, _rest, s = ws_handshake(args.host, args.port, args.tls)
    except Exception as e:
        print(f"    connection error: {e}")
        return False
    try:
        s.close()
    except Exception:
        pass
    print(f"    handshake status: {status!r}")
    if "101" in status:
        print("    FAIL: upgrade succeeded without authentication (vulnerable).")
        return False
    if "401" in status or "403" in status:
        print("    PASS: upgrade refused.")
        return True
    print("    PASS (non-101 response; upgrade did not switch protocols).")
    return True


def test_authenticated_works(args):
    print("[*] POSITIVE: authenticated WS upgrade + getdevices must work ...")
    token = base64.b64encode(f"{args.user}:{args.password}".encode()).decode()
    auth = "Basic " + token
    try:
        status, rest, s = ws_handshake(args.host, args.port, args.tls, auth_header=auth)
    except Exception as e:
        print(f"    connection error: {e}")
        return False
    print(f"    handshake status: {status!r}")
    if "101" not in status:
        print("    FAIL: authenticated upgrade was refused (regression).")
        print("    Note: Basic auth over plain HTTP requires 'Allow plain http basic")
        print("          auth' enabled in Settings, or use --tls.")
        try:
            s.close()
        except Exception:
            pass
        return False
    try:
        send_text(s, json.dumps({
            "event": "request",
            "requestid": 1,
            "query": "type=command&param=getdevices&used=true",
        }))
        data = rest + drain(s, seconds=3)
    finally:
        try:
            s.close()
        except Exception:
            pass
    ok = (b'"status"' in data and b'OK' in data and b'result' in data)
    print(f"    received {len(data)} bytes; contains device payload: {ok}")
    if ok:
        print("    PASS: authenticated session still receives devices.")
        return True
    print("    FAIL: authenticated getdevices did not return a device payload.")
    return False


def main():
    ap = argparse.ArgumentParser(description="Test Domoticz WebSocket authentication.")
    ap.add_argument("--host", required=True)
    ap.add_argument("--port", type=int, required=True)
    ap.add_argument("--tls", action="store_true", help="use TLS (wss)")
    ap.add_argument("--user", help="username for the positive/regression test")
    ap.add_argument("--password", help="password for the positive/regression test")
    args = ap.parse_args()

    results = []
    results.append(("unauthenticated-refused", test_unauthenticated_refused(args)))

    if args.user and args.password:
        results.append(("authenticated-works", test_authenticated_works(args)))
    else:
        print("[*] Skipping POSITIVE test (no --user/--password given).")

    print("\n=== summary ===")
    all_ok = True
    for name, ok in results:
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
        all_ok = all_ok and ok
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
