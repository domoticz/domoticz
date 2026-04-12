"""
<plugin key="WSFragmentTest" name="WebSocket Fragmentation Test" author="domoticz" version="1.0">
    <description>
        <h2>WebSocket Fragmentation Test</h2><br/>
        Tests that the Domoticz plugin system correctly reassembles fragmented WebSocket
        messages (RFC 6455 section 5.4).<br/><br/>
        Starts a local test server that intentionally sends messages split into small
        fragments, then connects to it using Protocol="WS" and verifies that each
        message is delivered to the plugin fully reassembled.<br/><br/>
        Three test sequences are sent:<br/>
        <ul>
            <li>An unfragmented message (baseline)</li>
            <li>A large message split into 10-byte fragments</li>
            <li>A fragmented message with a Ping control frame interleaved mid-sequence</li>
        </ul>
        Check the Domoticz log for PASS/FAIL results after the plugin starts.
    </description>
    <params>
        <param field="Mode1" label="Server port" width="60px" required="true" default="18765"/>
        <param field="Mode6" label="Debug" width="150px">
            <options>
                <option label="None" value="0" default="true"/>
                <option label="Python Only" value="2"/>
                <option label="All" value="-1"/>
            </options>
        </param>
    </params>
</plugin>
"""

import DomoticzEx as Domoticz
import base64
import hashlib
import queue
import secrets
import socket
import struct
import threading
import time


# ---------------------------------------------------------------------------
# Minimal WebSocket server helpers (server-side, runs in a background thread)
# ---------------------------------------------------------------------------

def _ws_frame(opcode, payload, fin=True):
    """Build a single WebSocket frame (unmasked - server to client)."""
    b0 = (0x80 if fin else 0x00) | opcode
    n = len(payload)
    if n < 126:
        header = struct.pack("BB", b0, n)
    elif n < 65536:
        header = struct.pack("!BBH", b0, 126, n)
    else:
        header = struct.pack("!BBQ", b0, 127, n)
    return header + (payload if isinstance(payload, bytes) else payload.encode("utf-8"))


def _ws_handshake(conn):
    """Read an HTTP upgrade request and respond with 101 Switching Protocols."""
    raw = b""
    while b"\r\n\r\n" not in raw:
        chunk = conn.recv(4096)
        if not chunk:
            return False
        raw += chunk
    key = None
    for line in raw.decode("latin-1").split("\r\n"):
        if line.lower().startswith("sec-websocket-key:"):
            key = line.split(":", 1)[1].strip()
            break
    if not key:
        return False
    accept = base64.b64encode(
        hashlib.sha1(
            (key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()
        ).digest()
    ).decode()
    conn.sendall((
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        f"Sec-WebSocket-Accept: {accept}\r\n\r\n"
    ).encode())
    return True


def _send_fragmented(conn, text, chunk_size):
    """Send a UTF-8 text message split into fragments of chunk_size bytes each."""
    data = text.encode("utf-8")
    chunks = [data[i:i + chunk_size] for i in range(0, len(data), chunk_size)]
    for idx, chunk in enumerate(chunks):
        first = (idx == 0)
        last  = (idx == len(chunks) - 1)
        opcode = 0x01 if first else 0x00   # text on first frame, continuation after
        conn.sendall(_ws_frame(opcode, chunk, fin=last))


def _send_fragmented_with_ping(conn, text, chunk_size):
    """
    Send a fragmented text message with a Ping interleaved after the first
    fragment (RFC 6455 section 5.4 allows control frames between data fragments).
    """
    data = text.encode("utf-8")
    chunks = [data[i:i + chunk_size] for i in range(0, len(data), chunk_size)]
    for idx, chunk in enumerate(chunks):
        first = (idx == 0)
        last  = (idx == len(chunks) - 1)
        opcode = 0x01 if first else 0x00
        conn.sendall(_ws_frame(opcode, chunk, fin=last))
        if first and len(chunks) > 1:
            # Inject a Ping between the first and second data fragment
            conn.sendall(_ws_frame(0x09, b"mid-frag-ping", fin=True))


# Expected payloads for validation
_UNFRAGMENTED_MSG = "Unfragmented baseline message."
_FRAGMENTED_MSG   = "Hello from the fragmented WebSocket world! " * 5
_PING_INTERLEAVED = "Interleaved-ping test: fragmentation must survive a mid-sequence Ping."
_CHUNK_SIZE       = 10


def _server_thread(port, log_q, stop_event):
    """
    Minimal WebSocket server.  Accepts one client, sends three test sequences,
    then drains incoming data until stopped.
    """
    try:
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("127.0.0.1", port))
        srv.listen(1)
        srv.settimeout(1.0)
    except OSError as e:
        log_q.put(("error", f"Test server failed to bind on port {port}: {e}"))
        return

    log_q.put(("log", f"Test server listening on 127.0.0.1:{port}"))

    conn = None
    while not stop_event.is_set():
        try:
            conn, _ = srv.accept()
        except socket.timeout:
            continue
        except OSError:
            break

        try:
            if not _ws_handshake(conn):
                log_q.put(("error", "Test server: WebSocket handshake failed"))
                conn.close()
                conn = None
                continue

            log_q.put(("log", "Test server: client connected - running test sequences"))

            # Test 1: unfragmented
            conn.sendall(_ws_frame(0x01, _UNFRAGMENTED_MSG))

            # Test 2: fragmented
            _send_fragmented(conn, _FRAGMENTED_MSG, _CHUNK_SIZE)

            # Test 3: fragmented + interleaved Ping
            _send_fragmented_with_ping(conn, _PING_INTERLEAVED, _CHUNK_SIZE)

            log_q.put(("log", "Test server: all test frames sent"))

            # Drain any incoming data (pong, close, etc.) until stopped
            conn.settimeout(1.0)
            while not stop_event.is_set():
                try:
                    data = conn.recv(256)
                    if not data:
                        break
                except socket.timeout:
                    continue
                except OSError:
                    break

        except OSError as e:
            log_q.put(("error", f"Test server socket error: {e}"))
        finally:
            conn.close()
            conn = None

    srv.close()
    log_q.put(("log", "Test server stopped"))


# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------

class BasePlugin:

    def __init__(self):
        self._conn       = None
        self._stop       = threading.Event()
        self._log_q      = queue.Queue()
        self._srv_thread = None
        self._results    = []        # reassembled message texts, in order
        self._expected   = [_UNFRAGMENTED_MSG, _FRAGMENTED_MSG, _PING_INTERLEAVED]

    # -- lifecycle -----------------------------------------------------------

    def onStart(self):
        if Parameters["Mode6"] != "0":
            Domoticz.Debugging(int(Parameters["Mode6"]))

        port = int(Parameters["Mode1"])
        Domoticz.Log("WebSocket Fragmentation Test starting")
        Domoticz.Log(f"  Unfragmented msg : {repr(_UNFRAGMENTED_MSG)}")
        Domoticz.Log(f"  Fragmented msg   : {len(_FRAGMENTED_MSG)} bytes, {_CHUNK_SIZE}-byte chunks")
        Domoticz.Log(f"  Interleaved-ping : {len(_PING_INTERLEAVED)} bytes, {_CHUNK_SIZE}-byte chunks + mid Ping")

        self._stop.clear()
        self._srv_thread = threading.Thread(
            name="WSFragTestServer",
            target=_server_thread,
            args=(port, self._log_q, self._stop),
            daemon=True,
        )
        self._srv_thread.start()
        time.sleep(0.3)   # let the server socket bind before connecting
        self._connect(port)

    def onStop(self):
        self._stop.set()
        if self._srv_thread:
            self._srv_thread.join(timeout=5)
        self._flush_log_queue()
        Domoticz.Log("WebSocket Fragmentation Test stopped")

    # -- connection ----------------------------------------------------------

    def _connect(self, port=None):
        if port is None:
            port = int(Parameters["Mode1"])
        self._conn = Domoticz.Connection(
            Name="WSFragTest",
            Transport="TCP/IP",
            Protocol="WS",
            Address="127.0.0.1",
            Port=str(port),
        )
        self._conn.Connect()

    def onConnect(self, Connection, Status, Description):
        self._flush_log_queue()
        if Status == 0:
            Domoticz.Log("Connected to test server - sending WebSocket upgrade")
            Connection.Send({
                "URL": "/",
                "Headers": {
                    "Host": "127.0.0.1",
                    "Origin": "http://127.0.0.1",
                    "Sec-WebSocket-Key": base64.b64encode(secrets.token_bytes(16)).decode(),
                },
            })
        else:
            Domoticz.Error(f"Connection failed ({Status}): {Description}")

    def onDisconnect(self, Connection):
        self._flush_log_queue()
        Domoticz.Log(f"Disconnected - received {len(self._results)} of {len(self._expected)} expected messages")
        self._report()

    # -- message handling ----------------------------------------------------

    def onMessage(self, Connection, Data):
        self._flush_log_queue()

        # HTTP 101 - upgrade complete
        if "Status" in Data:
            if Data["Status"] == "101":
                Domoticz.Log("WebSocket upgrade OK - waiting for test messages")
            else:
                Domoticz.Error(f"Unexpected HTTP status: {Data.get('Status')}")
            return

        # Control frames
        if "Operation" in Data:
            op = Data["Operation"]
            if op == "Ping":
                Domoticz.Log("Received mid-fragment Ping - sending Pong (RFC 6455 section 5.4)")
                Connection.Send({
                    "Operation": "Pong",
                    "Payload": Data.get("Payload", b""),
                    "Mask": secrets.randbits(32),
                })
            elif op == "Pong":
                Domoticz.Log("Received Pong")
            elif op == "Close":
                Domoticz.Log("Received Close")
            return

        # Data frame (fully reassembled by the plugin system)
        payload = Data.get("Payload", "")
        if isinstance(payload, (bytes, bytearray)):
            text = payload.decode("utf-8", errors="replace")
        else:
            text = str(payload)

        idx = len(self._results)
        self._results.append(text)

        if idx < len(self._expected):
            expected = self._expected[idx]
            if text == expected:
                Domoticz.Log(f"PASS  msg[{idx + 1}]: correctly reassembled {len(text)} bytes")
            else:
                Domoticz.Error(
                    f"FAIL  msg[{idx + 1}]: expected {len(expected)} bytes, "
                    f"got {len(text)} bytes"
                )
                Domoticz.Error(f"  expected: {repr(expected[:80])}")
                Domoticz.Error(f"  got     : {repr(text[:80])}")
        else:
            Domoticz.Log(f"INFO  extra msg[{idx + 1}] ({len(text)} bytes): {repr(text[:60])}")

    # -- heartbeat -----------------------------------------------------------

    def onHeartbeat(self):
        self._flush_log_queue()

    # -- helpers -------------------------------------------------------------

    def _flush_log_queue(self):
        """Drain server-thread log messages into Domoticz log."""
        while not self._log_q.empty():
            try:
                level, msg = self._log_q.get_nowait()
            except queue.Empty:
                break
            if level == "error":
                Domoticz.Error(msg)
            else:
                Domoticz.Log(msg)

    def _report(self):
        """Print a final pass/fail summary."""
        total   = len(self._expected)
        passed  = sum(
            1 for i, r in enumerate(self._results[:total])
            if r == self._expected[i]
        )
        Domoticz.Log("=" * 60)
        Domoticz.Log(f"WebSocket Fragmentation Test result: {passed}/{total} PASSED")
        for i, exp in enumerate(self._expected):
            got = self._results[i] if i < len(self._results) else None
            status = "PASS" if got == exp else "FAIL"
            Domoticz.Log(f"  [{status}] message {i + 1}: {len(exp)} bytes expected")
        Domoticz.Log("=" * 60)


# ---------------------------------------------------------------------------
# Domoticz plugin entry points
# ---------------------------------------------------------------------------

global _plugin
_plugin = BasePlugin()


def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()
