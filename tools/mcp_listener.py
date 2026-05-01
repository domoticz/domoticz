#!/usr/bin/env python3
"""
Minimal MCP protocol listener/sniffer.
Listens on a local port and logs every HTTP request it receives verbatim,
replying with enough to keep the client talking.

Usage:
    python mcp_listener.py [--port 6275]

Point MCP Inspector (SSE transport) at http://localhost:6275/mcp
"""

import argparse
import json
import socket
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer


PORT = 6275
SSE_SESSION_ID = "debug-session-001"


def log(tag, msg):
    ts = time.strftime("%H:%M:%S")
    print(f"[{ts}] [{tag}] {msg}")


class MCPHandler(BaseHTTPRequestHandler):
    # suppress default "127.0.0.1 - - [date] GET /mcp" access log line
    def log_message(self, fmt, *args):
        pass

    def _dump_request(self, body=None):
        log("REQ", f"{self.command} {self.path}")
        for k, v in self.headers.items():
            log("HDR", f"  {k}: {v}")
        if body:
            log("BOD", f"  {body}")

    def _send_cors(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, HEAD, POST, DELETE, OPTIONS")
        self.send_header(
            "Access-Control-Allow-Headers",
            "Content-Type, Accept, Authorization, Mcp-Session-Id, Mcp-Protocol-Version, Last-Event-ID",
        )
        self.send_header("Access-Control-Max-Age", "86400")

    # ------------------------------------------------------------------
    # OPTIONS
    # ------------------------------------------------------------------
    def do_OPTIONS(self):
        self._dump_request()
        self.send_response(200)
        self._send_cors()
        self.send_header("Content-Length", "0")
        self.end_headers()
        log("RSP", "200 OPTIONS (CORS preflight)")

    # ------------------------------------------------------------------
    # GET  — SSE stream
    # ------------------------------------------------------------------
    def do_GET(self):
        body_len = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(body_len).decode() if body_len else None
        self._dump_request(body)

        accept = self.headers.get("Accept", "")
        if "text/event-stream" not in accept:
            self.send_response(400)
            self.end_headers()
            log("RSP", "400 — no text/event-stream in Accept")
            return

        session_id = self.headers.get("Mcp-Session-Id", "")
        last_event_id = self.headers.get("Last-Event-ID", "")
        log("INF", f"SSE GET — session={session_id!r}  last-event-id={last_event_id!r}")

        # --- send SSE headers ---
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Transfer-Encoding", "chunked")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "keep-alive")
        self._send_cors()
        self.end_headers()
        log("RSP", "200 SSE stream open")

        def write_sse(event_type, data):
            if isinstance(data, dict):
                data = json.dumps(data)
            raw = ""
            if event_type:
                raw += f"event: {event_type}\n"
            raw += f"data: {data}\n\n"
            # chunked encoding
            chunk = raw.encode()
            header = f"{len(chunk):x}\r\n".encode()
            try:
                self.wfile.write(header + chunk + b"\r\n")
                self.wfile.flush()
                log("SSE", f"sent event={event_type!r}  data={data[:120]}")
            except Exception as e:
                log("ERR", f"SSE write failed: {e}")

        # If no session ID — legacy SSE transport: send 'endpoint' event
        if not session_id:
            host = self.headers.get("Host", f"localhost:{PORT}")
            endpoint_url = f"http://{host}/mcp?sessionId={SSE_SESSION_ID}"
            log("INF", f"No session ID — legacy SSE mode, sending endpoint: {endpoint_url}")
            write_sse("endpoint", endpoint_url)
        else:
            # Streamable HTTP SSE — send connected notification
            notif = {
                "jsonrpc": "2.0",
                "method": "notifications/message",
                "params": {
                    "level": "notice",
                    "logger": "mcp",
                    "data": {"message": f"SSE stream connected for session {session_id}"},
                },
            }
            write_sse(None, notif)

        # Keep the stream open, send keepalive comments every 15s
        try:
            while True:
                time.sleep(15)
                comment = f": keepalive {time.strftime('%H:%M:%S')}\n\n"
                chunk = comment.encode()
                header = f"{len(chunk):x}\r\n".encode()
                self.wfile.write(header + chunk + b"\r\n")
                self.wfile.flush()
                log("SSE", "sent keepalive comment")
        except Exception:
            log("INF", "SSE stream closed by client")

    # ------------------------------------------------------------------
    # POST  — JSON-RPC requests
    # ------------------------------------------------------------------
    def do_POST(self):
        body_len = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(body_len).decode() if body_len else ""
        self._dump_request(body)

        try:
            req = json.loads(body) if body else {}
        except json.JSONDecodeError:
            req = {}

        method = req.get("method", "")
        req_id = req.get("id")
        session_id = self.headers.get("Mcp-Session-Id", "")
        log("INF", f"POST method={method!r}  id={req_id!r}  session={session_id!r}")

        # notifications — 202, no body
        if method.startswith("notifications/"):
            self.send_response(202)
            self._send_cors()
            self.send_header("Content-Length", "0")
            self.end_headers()
            log("RSP", "202 notification accepted")
            return

        # initialize
        if method == "initialize":
            result = {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "protocolVersion": "2024-11-05",
                    "serverInfo": {"name": "mcp-listener-debug", "version": "0.0.1"},
                    "capabilities": {"tools": {}, "resources": {}},
                },
            }
            body_out = json.dumps(result).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body_out)))
            self.send_header("Mcp-Session-Id", SSE_SESSION_ID)
            self._send_cors()
            self.end_headers()
            self.wfile.write(body_out)
            log("RSP", f"200 initialize → session {SSE_SESSION_ID}")
            return

        # everything else — return empty result
        result = {"jsonrpc": "2.0", "id": req_id, "result": {}}
        body_out = json.dumps(result).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body_out)))
        if session_id:
            self.send_header("Mcp-Session-Id", session_id)
        self._send_cors()
        self.end_headers()
        self.wfile.write(body_out)
        log("RSP", f"200 {method} → empty result")

    # ------------------------------------------------------------------
    # DELETE
    # ------------------------------------------------------------------
    def do_DELETE(self):
        self._dump_request()
        self.send_response(200)
        self._send_cors()
        self.send_header("Content-Length", "0")
        self.end_headers()
        log("RSP", "200 DELETE — session removed")


def main():
    parser = argparse.ArgumentParser(description="MCP protocol listener for debugging")
    parser.add_argument("--port", type=int, default=PORT)
    args = parser.parse_args()

    server = HTTPServer(("0.0.0.0", args.port), MCPHandler)
    print(f"MCP listener on http://localhost:{args.port}/mcp")
    print(f"Point MCP Inspector (SSE transport) at: http://localhost:{args.port}/mcp")
    print("Press Ctrl+C to stop.\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
