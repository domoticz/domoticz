#!/usr/bin/env python3
"""A small mock Shelly device for testing the native Shelly integration.

Purpose
-------
It replays a recorded device (a fixture, see shelly_fixtures/) on a loopback
port, so the Shelly driver (hardware/Shelly*.cpp) can be tested end to end
without owning every device. It is the sibling of mini_mqtt_broker.py and, like
it, uses the Python standard library only.

Gen2+ (RPC) fixtures are JSON documents shaped {"config", "shelly", "status"},
the three documents the driver reads (Shelly.GetConfig, GET /shelly,
Shelly.GetStatus). The fixtures taken from Home Assistant are in exactly this
shape. The mock serves:

  GET  /shelly                       the identity document (never needs auth)
  POST /rpc                          JSON-RPC: Shelly.GetDeviceInfo, Shelly.GetStatus,
                                     Shelly.GetConfig, Sys.GetStatus, Switch.Set/Toggle,
                                     Light.Set/Toggle, RGB.Set, RGBW.Set, CCT.Set,
                                     Cover.Open/Close/Stop/GoToPosition, Thermostat.SetConfig
  GET  /rpc/<Method>?<params>        the same, with the params in the query
  GET  /rpc  (Upgrade: websocket)    a WebSocket that answers RPC frames and carries the
                                     NotifyStatus / NotifyEvent frames pushed with
                                     push_status() and push_event()

Gen1 fixtures are {"shelly", "settings", "status"} (the documents of GET /shelly,
GET /settings and GET /status). The mock serves those three plus /relay/N?turn=,
/light/N, /color/N, /white/N, /roller/N, /settings/thermostats/N and /thermostats/N.

Authentication is optional (password=...):

  Gen2+  HTTP Digest, algorithm SHA-256, user "admin", the way a Shelly does it;
         the WebSocket uses the RPC "auth" object (a 401 error frame carries the
         challenge) with SHA-256 and the "dummy_method:dummy_uri" HA2.
  Gen1   HTTP Basic (username= defaults to "admin"); /shelly stays open.

Every request is recorded (see requests and find_requests()), a command changes
the replayed status, and a Gen2+ command also pushes the new state to the
WebSocket clients, as a real device does. The identity can be changed with
mac=..., so several copies of one fixture (they all share one MAC) can run side by
side.

Usage
-----
    python mini_shelly_server.py 2pm_gen3_cover [--port N] [--mac AABBCCDDEEFF]
                                 [--password SECRET] [--username NAME] [-v]
    python mini_shelly_server.py --list

The fixture is a name from shelly_fixtures/ or the path of a JSON file. It runs
until interrupted. From a test:

    with MiniShellyServer("2pm_gen3_cover") as mock:
        ...mock.host, mock.port...
        mock.push_status({"switch:0": {"output": True}})
"""
import argparse
import base64
import copy
import hashlib
import json
import os
import socket
import struct
import sys
import threading
import time
import urllib.parse
from collections import OrderedDict
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

FIXTURE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "shelly_fixtures")
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
RPC_USER = "admin"
MAX_NONCES = 1000    # issued digest nonces that are remembered
MAX_WS_CLIENTS = 64  # simultaneous WebSocket clients


def fixture_names():
    """Names (without .json) of the fixtures that ship with the tests."""
    if not os.path.isdir(FIXTURE_DIR):
        return []
    return sorted(f[:-5] for f in os.listdir(FIXTURE_DIR) if f.endswith(".json"))


def load_fixture(name_or_path):
    """Load a fixture by name (from shelly_fixtures/) or by file path."""
    path = name_or_path
    if not os.path.isfile(path):
        path = os.path.join(FIXTURE_DIR, name_or_path + ("" if name_or_path.endswith(".json") else ".json"))
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def deep_merge(target, delta):
    """Merge delta into target the way a NotifyStatus delta is merged."""
    for key, value in delta.items():
        if isinstance(value, dict) and isinstance(target.get(key), dict):
            deep_merge(target[key], value)
        else:
            target[key] = copy.deepcopy(value)
    return target


def _sha256(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def _parse_auth_header(value):
    """Parse 'Digest a="b", c=d' into a dict (lower case keys)."""
    out = {}
    if not value:
        return out
    scheme, _, rest = value.partition(" ")
    out["_scheme"] = scheme.lower()
    i, n = 0, len(rest)
    while i < n:
        while i < n and rest[i] in " ,":
            i += 1
        j = i
        while j < n and rest[j] != "=":
            j += 1
        key = rest[i:j].strip().lower()
        i = j + 1
        if i < n and rest[i] == '"':
            i += 1
            val = []
            while i < n and rest[i] != '"':
                if rest[i] == "\\" and i + 1 < n:
                    i += 1
                val.append(rest[i])
                i += 1
            i += 1
            value_s = "".join(val)
        else:
            j = i
            while j < n and rest[j] != ",":
                j += 1
            value_s = rest[i:j].strip()
            i = j
        if key:
            out[key] = value_s
    return out


class _WsClient:
    def __init__(self, sock):
        self.sock = sock
        self.lock = threading.Lock()
        self.authed = False  # a frame with valid auth was seen (always true without a password)
        self.alive = True

    def send_frame(self, opcode, payload):
        header = bytes([0x80 | opcode])
        n = len(payload)
        if n < 126:
            header += bytes([n])
        elif n < 65536:
            header += bytes([126]) + struct.pack(">H", n)
        else:
            header += bytes([127]) + struct.pack(">Q", n)
        with self.lock:
            try:
                self.sock.sendall(header + payload)
                return True
            except OSError:
                self.alive = False
                return False

    def send_text(self, text):
        return self.send_frame(0x1, text.encode("utf-8"))


class MiniShellyServer:
    """One mock Shelly device on host:port (port 0 picks a free one)."""

    def __init__(self, fixture, host="127.0.0.1", port=0, mac=None, password=None, username=None, verbose=False):
        doc = load_fixture(fixture) if isinstance(fixture, str) else copy.deepcopy(fixture)
        self.name = fixture if isinstance(fixture, str) else "inline"
        self.gen1 = "gen" not in doc.get("shelly", {})
        self.shelly = copy.deepcopy(doc["shelly"])
        self.status = copy.deepcopy(doc["status"])
        self.config = copy.deepcopy(doc.get("settings" if self.gen1 else "config", {}))
        self.password = password or None
        self.username = username or RPC_USER
        self.verbose = verbose
        self.host = host
        self.requests = []
        self._lock = threading.RLock()
        self._nonces = OrderedDict()  # issued nonces, the oldest are forgotten beyond MAX_NONCES
        self._nonce_counter = 0
        self._clients = []
        self._frame_nc = {}
        self._stopped = False

        if mac:
            self._set_mac(mac)
        if self.gen1:
            self.shelly["auth"] = bool(self.password)
            self.realm = "shelly-" + self.mac[-6:].lower()
        else:
            self.shelly["auth_en"] = bool(self.password)
            self.shelly["auth_domain"] = self.shelly.get("id") if self.password else None
            self.realm = self.shelly.get("id") or "shelly"

        self._httpd = ThreadingHTTPServer((host, port), _make_handler(self))
        self._httpd.daemon_threads = True
        self.port = self._httpd.server_address[1]
        self._thread = None

    # -- identity -----------------------------------------------------------------------------

    @property
    def mac(self):
        return self.shelly["mac"]

    def _set_mac(self, mac):
        old = self.shelly["mac"]
        new = mac.replace(":", "").upper()
        self.shelly["mac"] = new
        text = json.dumps({"shelly": self.shelly, "status": self.status, "config": self.config})
        # the device id (shellyplus1-aabbcc...) and the host name carry the MAC as well
        text = text.replace(old, new).replace(old.lower(), new.lower())
        doc = json.loads(text)
        self.shelly, self.status, self.config = doc["shelly"], doc["status"], doc["config"]

    # -- lifecycle ----------------------------------------------------------------------------

    def start(self):
        self._thread = threading.Thread(target=self._httpd.serve_forever, kwargs={"poll_interval": 0.1}, daemon=True)
        self._thread.start()
        return self

    def stop(self):
        self._stopped = True
        with self._lock:
            clients = list(self._clients)
        for c in clients:
            c.alive = False
            try:
                c.sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
        self._httpd.shutdown()
        self._httpd.server_close()
        if self._thread:
            self._thread.join(timeout=5)

    def __enter__(self):
        return self.start()

    def __exit__(self, *exc):
        self.stop()

    # -- recording and inspection -------------------------------------------------------------

    def record(self, proto, method, target, params=None, auth="none"):
        entry = {"time": time.time(), "proto": proto, "method": method, "target": target, "params": params, "auth": auth}
        with self._lock:
            self.requests.append(entry)
        if self.verbose:
            print("[%s] %s %s %s auth=%s" % (proto, method, target, json.dumps(params) if params else "", auth), flush=True)
        return entry

    def find_requests(self, method=None, target_prefix=None, proto=None, auth=None):
        """Recorded requests, filtered. For Gen2+ method is the RPC method; for Gen1 it is 'GET' and target is the path."""
        with self._lock:
            items = list(self.requests)
        return [r for r in items
                if (method is None or r["method"] == method)
                and (target_prefix is None or r["target"].startswith(target_prefix))
                and (proto is None or r["proto"] == proto)
                and (auth is None or r["auth"] == auth)]

    def ws_client_count(self):
        with self._lock:
            return sum(1 for c in self._clients if c.alive and c.authed)

    # -- state changes and push ---------------------------------------------------------------

    def update_status(self, delta):
        """Merge delta into the replayed status without notifying anybody."""
        with self._lock:
            deep_merge(self.status, delta)

    def push_status(self, delta):
        """Merge delta into the status and send it as a NotifyStatus to every WebSocket client (Gen2+).

        Returns the number of clients it was sent to.
        """
        with self._lock:
            deep_merge(self.status, delta)
            return self._notify("NotifyStatus", dict(delta))

    def push_event(self, events):
        """Send a NotifyEvent with the given list of events (Gen2+)."""
        return self._notify("NotifyEvent", {"ts": time.time(), "events": events})

    def _notify(self, method, params):
        params = dict(params)
        params.setdefault("ts", round(time.time(), 2))
        frame = json.dumps({"src": self.shelly.get("id", "mock"), "dst": "domoticz", "method": method, "params": params})
        with self._lock:
            targets = [c for c in self._clients if c.alive and c.authed]
        sent = 0
        for c in targets:
            if c.send_text(frame):
                sent += 1
        return sent

    # -- authentication -----------------------------------------------------------------------

    def new_nonce(self):
        with self._lock:
            self._nonce_counter += 1
            nonce = int(time.time() * 1000) % 2000000000 + self._nonce_counter
            self._nonces[str(nonce)] = True
            while len(self._nonces) > MAX_NONCES:
                self._nonces.popitem(last=False)
        return nonce

    def check_digest_http(self, header, method, path):
        """True when the Authorization header is a valid SHA-256 digest answer."""
        a = _parse_auth_header(header)
        if a.get("_scheme") != "digest" or a.get("username") != self.username_rpc():
            return False
        if a.get("algorithm", "").upper() != "SHA-256" or a.get("realm") != self.realm or a.get("nonce") not in self._nonces:
            return False
        if a.get("uri") != path:
            return False
        ha1 = _sha256("%s:%s:%s" % (a["username"], self.realm, self.password))
        ha2 = _sha256("%s:%s" % (method, a["uri"]))
        expected = _sha256("%s:%s:%s:%s:auth:%s" % (ha1, a["nonce"], a.get("nc", ""), a.get("cnonce", ""), ha2))
        return a.get("qop") == "auth" and a.get("response") == expected

    def username_rpc(self):
        return RPC_USER  # a Gen2+ digest user is always admin

    def check_ws_auth(self, auth):
        """True when a frame's "auth" object answers a challenge we issued."""
        if not isinstance(auth, dict):
            return False
        if auth.get("username") != RPC_USER or auth.get("realm") != self.realm:
            return False
        if str(auth.get("nonce")) not in self._nonces or str(auth.get("algorithm", "")).upper() != "SHA-256":
            return False
        ha1 = _sha256("%s:%s:%s" % (RPC_USER, self.realm, self.password))
        ha2 = _sha256("dummy_method:dummy_uri")
        expected = _sha256("%s:%s:%s:%s:auth:%s" % (ha1, auth["nonce"], auth.get("nc"), auth.get("cnonce"), ha2))
        return auth.get("response") == expected

    def check_basic(self, header):
        if not header or not header.lower().startswith("basic "):
            return False
        try:
            user, _, pw = base64.b64decode(header[6:].strip()).decode("utf-8").partition(":")
        except Exception:
            return False
        return user == self.username and pw == self.password

    # -- Gen2+ RPC ----------------------------------------------------------------------------

    def _component(self, prefix, cid):
        """The status key of a component, e.g. light:0 (also cct:0 / rgb:0 for a light method)."""
        wanted = [prefix.lower() + ":%d" % cid]
        if prefix.lower() == "light":
            wanted += ["cct:%d" % cid, "rgb:%d" % cid, "rgbw:%d" % cid]
        for key in wanted:
            if key in self.status:
                return key
        return wanted[0]

    def rpc(self, method, params):
        """Run one RPC method. Returns (result, error) where error is None or {"code", "message"}."""
        params = params or {}
        with self._lock:
            if method == "Shelly.GetDeviceInfo":
                return copy.deepcopy(self.shelly), None
            if method == "Shelly.GetStatus":
                return copy.deepcopy(self.status), None
            if method == "Shelly.GetConfig":
                return copy.deepcopy(self.config), None
            if method == "Sys.GetStatus":
                return copy.deepcopy(self.status.get("sys", {})), None
            if method == "Shelly.GetComponents":
                return {"components": [], "cfg_rev": 0, "offset": 0, "total": 0}, None

            comp, _, action = method.partition(".")
            cid = params.get("id", 0)
            if comp in ("Switch", "Light", "RGB", "RGBW", "CCT") and action in ("Set", "Toggle"):
                key = self._component(comp, cid)
                state = self.status.setdefault(key, {})
                was_on = state.get("output", False)
                delta = {}
                if action == "Toggle":
                    delta["output"] = not state.get("output", False)
                else:
                    for src, dst in (("on", "output"), ("brightness", "brightness"), ("rgb", "rgb"), ("white", "white"), ("ct", "ct")):
                        if src in params:
                            delta[dst] = params[src]
                state.update(delta)
                self._notify("NotifyStatus", {key: delta})
                return {"was_on": was_on}, None
            if comp == "Cover" and action in ("Open", "Close", "Stop", "GoToPosition"):
                key = "cover:%d" % cid
                state = self.status.setdefault(key, {})
                if action == "Open":
                    delta = {"state": "open", "current_pos": 100}
                elif action == "Close":
                    delta = {"state": "closed", "current_pos": 0}
                elif action == "Stop":
                    delta = {"state": "stopped"}
                else:
                    pos = int(params.get("pos", 0))
                    delta = {"state": "open" if pos >= 100 else "closed" if pos <= 0 else "stopped", "current_pos": pos}
                state.update(delta)
                self._notify("NotifyStatus", {key: delta})
                return {}, None
            if method == "Thermostat.SetConfig":
                key = "thermostat:%d" % cid
                cfg = params.get("config", {})
                self.config.setdefault(key, {}).update(cfg)
                delta = {k: v for k, v in cfg.items() if k in ("target_C",)}
                if delta:
                    self.status.setdefault(key, {}).update(delta)
                    self._notify("NotifyStatus", {key: delta})
                return {"restart_required": False}, None
        return None, {"code": 404, "message": "No handler for %s" % method}

    def rpc_reply(self, rid, method, params, proto, src="http"):
        result, error = self.rpc(method, params)
        self.record(proto, method, "/rpc", params, auth=src)
        reply = {"id": rid, "src": self.shelly.get("id", "mock")}
        if error:
            reply["error"] = error
        else:
            reply["result"] = result
        return reply

    # -- Gen1 ---------------------------------------------------------------------------------

    def gen1_get(self, path, query):
        """Handle a Gen1 GET. Returns (status, json_document or None)."""
        q = {k: v[0] for k, v in urllib.parse.parse_qs(query, keep_blank_values=True).items()}
        parts = [p for p in path.split("/") if p]
        with self._lock:
            if parts == ["shelly"]:
                return 200, self.shelly
            if parts == ["status"]:
                return 200, self.status
            if parts == ["settings"]:
                return 200, self.config
            if len(parts) == 2 and parts[0] == "relay" and parts[1].isdigit():
                idx = int(parts[1])
                relays = self.status.get("relays") or []
                if idx >= len(relays):
                    return 404, None
                turn = q.get("turn")
                if turn == "toggle":
                    relays[idx]["ison"] = not relays[idx].get("ison", False)
                elif turn in ("on", "off"):
                    relays[idx]["ison"] = (turn == "on")
                return 200, relays[idx]
            if len(parts) == 2 and parts[0] in ("light", "color", "white") and parts[1].isdigit():
                idx = int(parts[1])
                lights = self.status.get("lights") or []
                if idx >= len(lights):
                    return 404, None
                light = lights[idx]
                if q.get("turn") == "toggle":
                    light["ison"] = not light.get("ison", False)
                elif q.get("turn") in ("on", "off"):
                    light["ison"] = (q["turn"] == "on")
                for key in ("brightness", "gain", "red", "green", "blue", "white", "temp"):
                    if key in q and q[key].lstrip("-").isdigit():
                        light[key] = int(q[key])
                if "mode" in q:
                    light["mode"] = q["mode"]
                return 200, light
            if len(parts) == 2 and parts[0] == "roller" and parts[1].isdigit():
                idx = int(parts[1])
                rollers = self.status.get("rollers") or []
                if idx >= len(rollers):
                    return 404, None
                roller = rollers[idx]
                go = q.get("go")
                if go == "open":
                    roller.update(state="stop", current_pos=100, last_direction="open")
                elif go == "close":
                    roller.update(state="stop", current_pos=0, last_direction="close")
                elif go == "stop":
                    roller["state"] = "stop"
                elif go == "to_pos" and q.get("roller_pos", "").isdigit():
                    roller.update(state="stop", current_pos=int(q["roller_pos"]))
                return 200, roller
            if len(parts) == 3 and parts[:2] == ["settings", "thermostats"] and parts[2].isdigit():
                idx = int(parts[2])
                thermostats = self.config.get("thermostats") or []
                status = self.status.get("thermostats") or []
                if idx >= len(thermostats):
                    return 404, None
                if "target_t" in q:
                    try:
                        target = float(q["target_t"])
                    except ValueError:
                        return 400, None
                    thermostats[idx].setdefault("target_t", {}).update(value=target, enabled=(q.get("target_t_enabled") != "false"))
                    if idx < len(status):
                        status[idx].setdefault("target_t", {}).update(value=target, enabled=True)
                return 200, thermostats[idx]
            if len(parts) == 2 and parts[0] == "thermostats" and parts[1].isdigit():
                status = self.status.get("thermostats") or []
                idx = int(parts[1])
                return (200, status[idx]) if idx < len(status) else (404, None)
        return 404, None


def _make_handler(mock):
    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"
        server_version = "mini-shelly"

        def log_message(self, fmt, *args):
            if mock.verbose:
                sys.stderr.write("mini_shelly: " + (fmt % args) + "\n")

        # -- helpers --
        def _send(self, status, body=b"", headers=None, ctype="application/json"):
            if isinstance(body, (dict, list)):
                body = json.dumps(body).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(body)))
            for k, v in (headers or {}).items():
                self.send_header(k, v)
            self.end_headers()
            if self.command != "HEAD":
                self.wfile.write(body)

        def _challenge_gen2(self, rid=None):
            nonce = mock.new_nonce()
            challenge = {"auth_type": "digest", "nonce": nonce, "nc": 1, "realm": mock.realm, "algorithm": "SHA-256"}
            www = 'Digest qop="auth", realm="%s", nonce="%s", algorithm=SHA-256' % (mock.realm, nonce)
            body = {"id": rid, "src": mock.shelly.get("id", "mock"), "error": {"code": 401, "message": json.dumps(challenge)}}
            self._send(401, body, {"WWW-Authenticate": www})

        def _authorized_gen2(self, method, path):
            """True when no password is set or the digest answer is valid."""
            if not mock.password:
                return True
            return mock.check_digest_http(self.headers.get("Authorization"), method, path)

        # -- GET --
        def do_GET(self):
            parsed = urllib.parse.urlsplit(self.path)
            if mock.gen1:
                return self._gen1(parsed)
            if parsed.path == "/shelly":
                mock.record("http", "GET", "/shelly")
                return self._send(200, mock.shelly)
            if parsed.path == "/rpc" and self.headers.get("Upgrade", "").lower() == "websocket":
                return self._websocket()
            if parsed.path.startswith("/rpc/"):
                if not self._authorized_gen2("GET", parsed.path + ("?" + parsed.query if parsed.query else "")):
                    mock.record("http", "GET", parsed.path, auth="bad" if self.headers.get("Authorization") else "none")
                    return self._challenge_gen2()
                params = {}
                for k, v in urllib.parse.parse_qsl(parsed.query):
                    try:
                        params[k] = json.loads(v)
                    except ValueError:
                        params[k] = v
                reply = mock.rpc_reply(None, parsed.path[5:], params, "http", "ok" if mock.password else "none")
                return self._send(200 if "result" in reply else 404, reply.get("result", reply))
            self._send(404, b"Not Found", ctype="text/plain")

        # -- POST --
        def do_POST(self):
            parsed = urllib.parse.urlsplit(self.path)
            length = int(self.headers.get("Content-Length") or 0)
            body = self.rfile.read(length) if length else b""
            if mock.gen1 or parsed.path != "/rpc":
                return self._send(404, b"Not Found", ctype="text/plain")
            if not self._authorized_gen2("POST", parsed.path):
                mock.record("http", "POST", "/rpc", auth="bad" if self.headers.get("Authorization") else "none")
                return self._challenge_gen2()
            try:
                req = json.loads(body.decode("utf-8"))
                method = req["method"]
            except (ValueError, KeyError, TypeError):
                return self._send(400, {"error": {"code": -32700, "message": "Parse error"}})
            reply = mock.rpc_reply(req.get("id"), method, req.get("params"), "http", "ok" if mock.password else "none")
            self._send(200, reply)

        # -- Gen1 --
        def _gen1(self, parsed):
            path = parsed.path
            query = parsed.query
            target = path + ("?" + query if query else "")
            if path != "/shelly" and mock.password:
                if not mock.check_basic(self.headers.get("Authorization")):
                    mock.record("http", "GET", target, auth="bad" if self.headers.get("Authorization") else "none")
                    return self._send(401, b"Unauthorized", {"WWW-Authenticate": 'Basic realm="%s"' % mock.realm}, "text/plain")
            auth = "ok" if (mock.password and path != "/shelly") else "none"
            mock.record("http", "GET", target, auth=auth)
            status, doc = mock.gen1_get(path, query)
            if doc is None:
                return self._send(status, b"Not Found" if status == 404 else b"Bad Request", ctype="text/plain")
            self._send(status, doc)

        # -- WebSocket --
        def _websocket(self):
            key = self.headers.get("Sec-WebSocket-Key")
            if not key:
                return self._send(400, b"Bad Request", ctype="text/plain")
            accept = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
            self.send_response(101, "Switching Protocols")
            self.send_header("Upgrade", "websocket")
            self.send_header("Connection", "Upgrade")
            self.send_header("Sec-WebSocket-Accept", accept)
            self.end_headers()
            self.wfile.flush()
            self.close_connection = True
            sock = self.connection
            sock.settimeout(None)
            client = _WsClient(sock)
            client.authed = not mock.password
            with mock._lock:
                if len(mock._clients) >= MAX_WS_CLIENTS:
                    too_many = True
                else:
                    too_many = False
                    mock._clients.append(client)
            if too_many:
                client.alive = False
                return
            mock.record("ws", "CONNECT", "/rpc")
            try:
                self._ws_loop(client)
            finally:
                client.alive = False
                with mock._lock:
                    if client in mock._clients:
                        mock._clients.remove(client)

        def _read_exact(self, n):
            buf = b""
            while len(buf) < n:
                chunk = self.rfile.read(n - len(buf))
                if not chunk:
                    raise EOFError()
                buf += chunk
            return buf

        def _read_frame(self):
            b1, b2 = self._read_exact(2)
            opcode = b1 & 0x0F
            fin = bool(b1 & 0x80)
            masked = bool(b2 & 0x80)
            n = b2 & 0x7F
            if n == 126:
                n = struct.unpack(">H", self._read_exact(2))[0]
            elif n == 127:
                n = struct.unpack(">Q", self._read_exact(8))[0]
            mask = self._read_exact(4) if masked else b""
            data = self._read_exact(n)
            if masked:
                data = bytes(c ^ mask[i % 4] for i, c in enumerate(data))
            return fin, opcode, data

        def _ws_loop(self, client):
            message = b""
            while client.alive and not mock._stopped:
                try:
                    fin, opcode, data = self._read_frame()
                except (EOFError, OSError):
                    return
                if opcode == 0x8:
                    client.send_frame(0x8, data[:2])
                    return
                if opcode == 0x9:
                    client.send_frame(0xA, data)
                    continue
                if opcode == 0xA:
                    continue
                message += data
                if not fin:
                    continue
                text, message = message.decode("utf-8", "replace"), b""
                try:
                    req = json.loads(text)
                    method = req["method"]
                except (ValueError, KeyError, TypeError):
                    continue
                self._ws_rpc(client, req, method)

        def _ws_rpc(self, client, req, method):
            rid = req.get("id")
            src = mock.shelly.get("id", "mock")
            if mock.password:
                if not mock.check_ws_auth(req.get("auth")):
                    nonce = mock.new_nonce()
                    challenge = {"auth_type": "digest", "nonce": nonce, "nc": 1, "realm": mock.realm, "algorithm": "SHA-256"}
                    mock.record("ws", method, "/rpc", req.get("params"), auth="bad" if req.get("auth") else "none")
                    client.send_text(json.dumps({"id": rid, "src": src, "dst": req.get("src"),
                                                 "error": {"code": 401, "message": json.dumps(challenge)}}))
                    return
                client.authed = True
            reply = mock.rpc_reply(rid, method, req.get("params"), "ws", "ok" if mock.password else "none")
            reply["dst"] = req.get("src")
            client.send_text(json.dumps(reply))

    return Handler


def main(argv=None):
    ap = argparse.ArgumentParser(description="Mock Shelly device replaying a fixture")
    ap.add_argument("fixture", nargs="?", help="fixture name from shelly_fixtures/ or a JSON file path")
    ap.add_argument("--list", action="store_true", help="list the bundled fixtures and exit")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=0)
    ap.add_argument("--mac", help="override the MAC address of the fixture (12 hex digits)")
    ap.add_argument("--password", help="require authentication with this password")
    ap.add_argument("--username", help="Gen1 basic auth user (default admin)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args(argv)
    if args.list:
        print("\n".join(fixture_names()))
        return 0
    if not args.fixture:
        ap.error("a fixture is required (see --list)")
    mock = MiniShellyServer(args.fixture, host=args.host, port=args.port, mac=args.mac,
                            password=args.password, username=args.username, verbose=True)
    print("mock Shelly %s (%s, MAC %s) on http://%s:%d" % (args.fixture, "Gen1" if mock.gen1 else "Gen2+", mock.mac, mock.host, mock.port), flush=True)
    mock.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        mock.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
