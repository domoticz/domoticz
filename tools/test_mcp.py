#!/usr/bin/env python3
"""
Domoticz MCP (Model Context Protocol) server test script.
Tests the HTTP+SSE transport directly without any MCP SDK.

Usage:
    python test_mcp.py [--host HOST] [--port PORT] [--user USER] [--password PASSWORD]
"""

import argparse
import json
import sys
import threading
import time
import requests


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def print_pass(label):
    print(f"  [PASS] {label}")

def print_fail(label, detail=""):
    msg = f"  [FAIL] {label}"
    if detail:
        msg += f": {detail}"
    print(msg)

def print_step(n, title):
    print(f"\nStep {n}: {title}")
    print("-" * 60)


def parse_sse_events(raw_text):
    """Parse a raw SSE response body (possibly partial) into a list of events.

    Each SSE event is a dict with keys 'event' (optional), 'data' (optional),
    and 'id' (optional).
    """
    events = []
    current = {}
    for line in raw_text.splitlines():
        if line.startswith("event:"):
            current["event"] = line[len("event:"):].strip()
        elif line.startswith("data:"):
            data_str = line[len("data:"):].strip()
            current.setdefault("data_lines", []).append(data_str)
        elif line.startswith("id:"):
            current["id"] = line[len("id:"):].strip()
        elif line == "":
            # blank line = end of event
            if current:
                if "data_lines" in current:
                    current["data"] = "\n".join(current.pop("data_lines"))
                events.append(current)
                current = {}
    # flush any trailing partial event
    if current:
        if "data_lines" in current:
            current["data"] = "\n".join(current.pop("data_lines"))
        events.append(current)
    return events


def make_request(method, params=None, req_id=1):
    """Build a JSON-RPC 2.0 request object."""
    obj = {"jsonrpc": "2.0", "method": method, "id": req_id}
    if params is not None:
        obj["params"] = params
    return obj


def make_notification(method, params=None):
    """Build a JSON-RPC 2.0 notification (no id)."""
    obj = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        obj["params"] = params
    return obj


# ---------------------------------------------------------------------------
# SSE stream reader (runs in a background thread)
# ---------------------------------------------------------------------------

class SSEReader:
    """Opens a GET /mcp SSE stream and collects events in a list."""

    def __init__(self, url, session_id, auth, timeout=10):
        self.url = url
        self.session_id = session_id
        self.auth = auth
        self.timeout = timeout
        self.events = []
        self.sse_status = None
        self.sse_error = None
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._started = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self):
        self._thread.start()
        # give the thread a moment to connect
        self._started.wait(timeout=5)

    def stop(self):
        self._stop.set()

    def get_events(self):
        with self._lock:
            return list(self.events)

    def wait_for_event(self, event_type=None, timeout=5):
        """Block until an event of the given type arrives (or timeout)."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                for ev in self.events:
                    if event_type is None or ev.get("event") == event_type:
                        return ev
            time.sleep(0.1)
        return None

    def _run(self):
        headers = {
            "Accept": "text/event-stream",
            "Mcp-Session-Id": self.session_id,
        }
        try:
            resp = requests.get(
                self.url,
                headers=headers,
                auth=self.auth,
                stream=True,
                timeout=self.timeout,
            )
            self.sse_status = resp.status_code
            self._started.set()
            if resp.status_code != 200:
                self.sse_error = f"HTTP {resp.status_code}: {resp.text[:200]}"
                return

            buf = ""
            for chunk in resp.iter_content(chunk_size=None, decode_unicode=True):
                if self._stop.is_set():
                    break
                if chunk:
                    buf += chunk
                    # process complete events (terminated by blank line)
                    while "\n\n" in buf or "\r\n\r\n" in buf:
                        # split off the first complete event
                        for sep in ("\r\n\r\n", "\n\n"):
                            idx = buf.find(sep)
                            if idx != -1:
                                block = buf[:idx]
                                buf = buf[idx + len(sep):]
                                events = parse_sse_events(block)
                                with self._lock:
                                    self.events.extend(events)
                                break
        except Exception:
            pass
        finally:
            self._started.set()  # unblock wait in case of early error


# ---------------------------------------------------------------------------
# Main test routine
# ---------------------------------------------------------------------------

def run_tests(host, port, username, password):
    base_url = f"http://{host}:{port}"
    mcp_url = f"{base_url}/mcp"
    auth = (username, password) if (username and password) else None

    print(f"Target: {mcp_url}")
    if auth:
        print(f"Auth:   {username}:{'*' * len(password)}")
    print("=" * 60)

    session_id = None
    sse_reader = None
    all_passed = True

    # ------------------------------------------------------------------
    # Step 1 — CORS OPTIONS preflight
    # ------------------------------------------------------------------
    print_step(1, "CORS OPTIONS preflight")
    try:
        resp = requests.options(
            mcp_url,
            headers={
                "Origin": f"http://{host}",
                "Access-Control-Request-Method": "POST",
                "Access-Control-Request-Headers": "Content-Type, Mcp-Session-Id",
            },
            auth=auth,
            timeout=10,
        )
        print(f"  Status: {resp.status_code}")

        allow_headers = resp.headers.get("Access-Control-Allow-Headers", "")
        allow_methods = resp.headers.get("Access-Control-Allow-Methods", "")
        allow_origin = resp.headers.get("Access-Control-Allow-Origin", "")

        print(f"  Access-Control-Allow-Origin:  {allow_origin or '(missing)'}")
        print(f"  Access-Control-Allow-Methods: {allow_methods or '(missing)'}")
        print(f"  Access-Control-Allow-Headers: {allow_headers or '(missing)'}")

        if resp.status_code in (200, 204):
            print_pass("Preflight status OK")
        else:
            print_fail("Preflight status", f"got {resp.status_code}")
            all_passed = False

        if "mcp-session-id" in allow_headers.lower():
            print_pass("Mcp-Session-Id present in Access-Control-Allow-Headers")
        else:
            print_fail("Mcp-Session-Id missing from Access-Control-Allow-Headers")
            all_passed = False

    except Exception as exc:
        print_fail("OPTIONS request failed", str(exc))
        all_passed = False

    # ------------------------------------------------------------------
    # Step 2 — initialize
    # ------------------------------------------------------------------
    print_step(2, "Send 'initialize' request")
    try:
        init_payload = make_request(
            "initialize",
            params={
                "protocolVersion": "2024-11-05",
                "capabilities": {},
                "clientInfo": {"name": "domoticz-mcp-test", "version": "1.0"},
            },
            req_id=1,
        )
        resp = requests.post(
            mcp_url,
            json=init_payload,
            headers={"Content-Type": "application/json", "Accept": "application/json"},
            auth=auth,
            timeout=10,
        )
        print(f"  Status: {resp.status_code}")

        if resp.status_code == 200:
            print_pass("initialize returned 200")
        else:
            print_fail("initialize status", f"got {resp.status_code}")
            all_passed = False
            return all_passed  # can't continue without a session

        session_id = resp.headers.get("Mcp-Session-Id", "")
        if session_id:
            print_pass(f"Got Mcp-Session-Id: {session_id}")
        else:
            print_fail("Mcp-Session-Id header missing from initialize response")
            all_passed = False
            return all_passed

        body = resp.json()
        result = body.get("result", {})
        proto_ver = result.get("protocolVersion", "(missing)")
        server_info = result.get("serverInfo", {})
        capabilities = result.get("capabilities", {})
        print(f"  Protocol version : {proto_ver}")
        print(f"  Server name      : {server_info.get('name', '?')} {server_info.get('version', '')}")
        print(f"  Capabilities     : {json.dumps(capabilities, indent=4)}")

        if "result" in body:
            print_pass("initialize result is valid JSON-RPC response")
        else:
            print_fail("initialize response missing 'result'", json.dumps(body))
            all_passed = False

    except Exception as exc:
        print_fail("initialize request failed", str(exc))
        all_passed = False
        return all_passed

    # ------------------------------------------------------------------
    # Step 3 — notifications/initialized
    # ------------------------------------------------------------------
    print_step(3, "Send 'notifications/initialized' notification")
    try:
        notif_payload = make_notification("notifications/initialized")
        resp = requests.post(
            mcp_url,
            json=notif_payload,
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json",
                "Mcp-Session-Id": session_id,
            },
            auth=auth,
            timeout=10,
        )
        print(f"  Status: {resp.status_code}")
        if resp.status_code == 202:
            print_pass("notifications/initialized accepted (202)")
        else:
            print_fail("notifications/initialized status", f"expected 202, got {resp.status_code}")
            all_passed = False

    except Exception as exc:
        print_fail("notifications/initialized request failed", str(exc))
        all_passed = False

    # ------------------------------------------------------------------
    # Step 4 — Open SSE stream, wait for 'connected' event
    # ------------------------------------------------------------------
    print_step(4, "Open SSE stream and wait for 'connected' event (timeout 5s)")
    sse_reader = SSEReader(mcp_url, session_id, auth, timeout=30)
    sse_reader.start()
    print(f"  SSE GET status: {sse_reader.sse_status}")
    if sse_reader.sse_error:
        print(f"  SSE error: {sse_reader.sse_error}")

    connected_event = sse_reader.wait_for_event(event_type="connected", timeout=5)
    if connected_event is not None:
        print_pass("Received 'connected' SSE event")
        print(f"  Event data: {connected_event.get('data', '(empty)')}")
    else:
        # Some servers send an initial 'message' event instead of 'connected'
        # — accept any event as proof the stream is alive
        any_event = sse_reader.wait_for_event(timeout=2)
        if any_event is not None:
            print_pass(f"SSE stream is live (received event type={any_event.get('event', 'message')!r})")
            print(f"  Event data: {any_event.get('data', '(empty)')[:200]}")
        else:
            print_fail("No SSE event received within 7s")
            print(f"  Events collected so far: {sse_reader.get_events()}")
            all_passed = False

    # ------------------------------------------------------------------
    # Step 5 — tools/list
    # ------------------------------------------------------------------
    print_step(5, "Send 'tools/list' request")
    try:
        resp = requests.post(
            mcp_url,
            json=make_request("tools/list", req_id=2),
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json",
                "Mcp-Session-Id": session_id,
            },
            auth=auth,
            timeout=10,
        )
        print(f"  Status: {resp.status_code}")
        if resp.status_code == 200:
            print_pass("tools/list returned 200")
            body = resp.json()
            tools = body.get("result", {}).get("tools", [])
            print(f"  Tools returned: {len(tools)}")
            for t in tools:
                desc = t.get("description", "")
                if len(desc) > 60:
                    desc = desc[:57] + "..."
                print(f"    - {t.get('name', '?'):30s}  {desc}")
        else:
            print_fail("tools/list status", f"got {resp.status_code} — {resp.text[:200]}")
            all_passed = False

    except Exception as exc:
        print_fail("tools/list request failed", str(exc))
        all_passed = False

    # ------------------------------------------------------------------
    # Step 6 — resources/list
    # ------------------------------------------------------------------
    print_step(6, "Send 'resources/list' request")
    try:
        resp = requests.post(
            mcp_url,
            json=make_request("resources/list", req_id=3),
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json",
                "Mcp-Session-Id": session_id,
            },
            auth=auth,
            timeout=10,
        )
        print(f"  Status: {resp.status_code}")
        if resp.status_code == 200:
            print_pass("resources/list returned 200")
            body = resp.json()
            resources = body.get("result", {}).get("resources", [])
            print(f"  Resources returned: {len(resources)}")
            for r in resources[:20]:
                print(f"    - {r.get('uri', '?')}")
            if len(resources) > 20:
                print(f"    ... and {len(resources) - 20} more")
        else:
            print_fail("resources/list status", f"got {resp.status_code} — {resp.text[:200]}")
            all_passed = False

    except Exception as exc:
        print_fail("resources/list request failed", str(exc))
        all_passed = False

    # ------------------------------------------------------------------
    # Step 7 — cleanup (DELETE /mcp)
    # ------------------------------------------------------------------
    print_step(7, "Cleanup: DELETE /mcp")
    if sse_reader:
        sse_reader.stop()

    try:
        resp = requests.delete(
            mcp_url,
            headers={"Mcp-Session-Id": session_id},
            auth=auth,
            timeout=10,
        )
        print(f"  Status: {resp.status_code}")
        if resp.status_code in (200, 204):
            print_pass("Session deleted successfully")
        else:
            # Some servers return 404 if sessions aren't persisted — not fatal
            print(f"  Note: DELETE returned {resp.status_code} (may be normal if server doesn't track sessions)")

    except Exception as exc:
        print_fail("DELETE request failed", str(exc))
        all_passed = False

    return all_passed


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Test a Domoticz MCP HTTP+SSE server without any MCP SDK."
    )
    parser.add_argument("--host", default="192.168.0.70", help="Domoticz host (default: 192.168.0.70)")
    parser.add_argument("--port", default="8080", help="Domoticz port (default: 8080)")
    parser.add_argument("--user", default="", help="HTTP Basic Auth username")
    parser.add_argument("--password", default="", help="HTTP Basic Auth password")
    args = parser.parse_args()

    ok = run_tests(args.host, args.port, args.user, args.password)

    print("\n" + "=" * 60)
    if ok:
        print("All tests passed.")
        sys.exit(0)
    else:
        print("One or more tests FAILED.")
        sys.exit(1)


if __name__ == "__main__":
    main()
