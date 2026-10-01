#!/usr/bin/env python3
"""A minimal, dependency-free MQTT 3.1.1 broker for local integration tests.

Purpose
-------
test_calibration.py needs a real MQTT round trip to exercise the MQTT Auto
Discovery hardware (hardware/MQTTAutoDiscover.cpp): Domoticz as one client,
a small publisher (paho-mqtt) as another, talking over a real TCP socket on
loopback. There is no broker available on this machine and none bundled in
this repository, so this module implements just enough of the wire protocol
for that one scenario.

Scope
-----
Supported: CONNECT/CONNACK, SUBSCRIBE/SUBACK, UNSUBSCRIBE/UNSUBACK, PUBLISH
in both directions, PUBACK, PINGREQ/PINGRESP, DISCONNECT, QoS 0 and 1,
retained messages (stored and replayed to subscribers that subscribe later,
which is what the Home Assistant discovery flow depends on), topic filters
with '+' and '#' wildcards.

Not supported, and not needed for this test: QoS 2, persistent sessions
(clean-session is assumed always on), Will messages, authentication checks
(username/password are accepted but never verified), TLS. Messages this
broker receives at QoS 1 are ack'd with PUBACK immediately and forwarded to
subscribers at QoS 0 -- "at most once" delivery on the broker side, which is
sufficient for a single publisher/single subscriber loopback test and keeps
this file free of retry/dup bookkeeping.

A malformed or unexpected packet is logged and the connection that sent it
is dropped; it never takes the whole broker thread down.

Usage
-----
As a library (see test_calibration.py's run_mqtt_case):

    broker = MiniMQTTBroker("127.0.0.1", 0)  # 0 = pick a free port
    broker.start()
    ... use broker.port ...
    broker.stop()

Standalone, for manual debugging:

    python mini_mqtt_broker.py [port]
"""
import logging
import socket
import struct
import sys
import threading
import time

log = logging.getLogger("mini_mqtt_broker")

# ---------------------------------------------------------------------------
# MQTT control packet types (top nibble of the fixed header's first byte)
# ---------------------------------------------------------------------------
CONNECT = 1
CONNACK = 2
PUBLISH = 3
PUBACK = 4
SUBSCRIBE = 8
SUBACK = 9
UNSUBSCRIBE = 10
UNSUBACK = 11
PINGREQ = 12
PINGRESP = 13
DISCONNECT = 14


class ConnectionClosed(Exception):
    """The peer closed the socket (or it died) while we were reading."""


def topic_matches(topic_filter, topic):
    """True if a concrete `topic` matches a subscription `topic_filter`,
    honouring the '+' (single level) and '#' (multi level, trailing only)
    wildcards as defined by the MQTT spec."""
    f = topic_filter.split("/")
    t = topic.split("/")
    i = 0
    while i < len(f):
        if f[i] == "#":
            return True
        if i >= len(t):
            return False
        if f[i] != "+" and f[i] != t[i]:
            return False
        i += 1
    return i == len(t)


def _encode_remaining_length(n):
    out = bytearray()
    while True:
        byte = n % 128
        n //= 128
        if n > 0:
            byte |= 0x80
        out.append(byte)
        if n <= 0:
            break
    return bytes(out)


def _encode_string(s):
    b = s.encode("utf-8") if isinstance(s, str) else s
    return struct.pack("!H", len(b)) + b


class _ClientConn:
    """One accepted TCP connection, handled on its own thread."""

    def __init__(self, broker, sock, addr):
        self.broker = broker
        self.sock = sock
        self.addr = addr
        self.client_id = "?"
        self.filters = set()
        self._send_lock = threading.Lock()
        self._recv_buf = b""
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._closed = False

    def start(self):
        self._thread.start()

    def close(self):
        if self._closed:
            return
        self._closed = True
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass

    def matches(self, topic):
        return any(topic_matches(f, topic) for f in self.filters)

    # -- low level framing -------------------------------------------------
    def _read_exact(self, n):
        while len(self._recv_buf) < n:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionClosed()
            self._recv_buf += chunk
        data, self._recv_buf = self._recv_buf[:n], self._recv_buf[n:]
        return data

    def _read_packet(self):
        header = self._read_exact(1)[0]
        ptype = header >> 4
        flags = header & 0x0F
        multiplier = 1
        value = 0
        while True:
            b = self._read_exact(1)[0]
            value += (b & 0x7F) * multiplier
            if (b & 0x80) == 0:
                break
            multiplier *= 128
            if multiplier > 128 ** 3:
                raise ValueError("malformed remaining length")
        body = self._read_exact(value)
        return ptype, flags, body

    def _send(self, packet_bytes):
        with self._send_lock:
            self.sock.sendall(packet_bytes)

    # -- outgoing packets ----------------------------------------------------
    def send_connack(self):
        self._send(bytes([0x20, 0x02, 0x00, 0x00]))

    def send_suback(self, packet_id, granted):
        body = struct.pack("!H", packet_id) + bytes(granted)
        self._send(bytes([0x90]) + _encode_remaining_length(len(body)) + body)

    def send_unsuback(self, packet_id):
        self._send(bytes([0xB0, 0x02]) + struct.pack("!H", packet_id))

    def send_puback(self, packet_id):
        self._send(bytes([0x40, 0x02]) + struct.pack("!H", packet_id))

    def send_pingresp(self):
        self._send(bytes([0xD0, 0x00]))

    def deliver(self, topic, payload, retain):
        """Forward an application message to this client at QoS 0."""
        header = 0x30 | (0x01 if retain else 0x00)
        body = _encode_string(topic) + payload
        try:
            self._send(bytes([header]) + _encode_remaining_length(len(body)) + body)
        except OSError:
            pass

    # -- inbound packet handling ---------------------------------------------
    def _run(self):
        try:
            ptype, flags, body = self._read_packet()
            if ptype != CONNECT:
                log.warning("first packet from %s was not CONNECT (type=%d), dropping", self.addr, ptype)
                return
            self._handle_connect(body)
            self.send_connack()

            while True:
                ptype, flags, body = self._read_packet()
                try:
                    self._dispatch(ptype, flags, body)
                except Exception as e:  # noqa: BLE001 - never let a bad packet kill the thread
                    log.warning("error handling packet type=%d from %s: %s", ptype, self.addr, e)
                if ptype == DISCONNECT:
                    break
        except ConnectionClosed:
            pass
        except OSError:
            pass
        except Exception as e:  # noqa: BLE001
            log.warning("connection %s terminated: %s", self.addr, e)
        finally:
            self.broker.remove_client(self)
            self.close()

    def _handle_connect(self, body):
        try:
            name_len = struct.unpack("!H", body[0:2])[0]
            pos = 2 + name_len
            pos += 1  # protocol level
            pos += 1  # connect flags
            pos += 2  # keep alive
            cid_len = struct.unpack("!H", body[pos:pos + 2])[0]
            pos += 2
            self.client_id = body[pos:pos + cid_len].decode("utf-8", "replace") or "?"
        except Exception:
            self.client_id = "?"

    def _dispatch(self, ptype, flags, body):
        if ptype == PUBLISH:
            self._handle_publish(flags, body)
        elif ptype == SUBSCRIBE:
            self._handle_subscribe(body)
        elif ptype == UNSUBSCRIBE:
            self._handle_unsubscribe(body)
        elif ptype == PUBACK:
            pass  # nothing to reconcile, we don't track our own outgoing QoS 1 sends
        elif ptype == PINGREQ:
            self.send_pingresp()
        elif ptype == DISCONNECT:
            pass
        else:
            log.warning("unexpected/unsupported packet type=%d, skipping", ptype)

    def _handle_publish(self, flags, body):
        qos = (flags >> 1) & 0x03
        retain = bool(flags & 0x01)
        topic_len = struct.unpack("!H", body[0:2])[0]
        topic = body[2:2 + topic_len].decode("utf-8", "replace")
        pos = 2 + topic_len
        packet_id = None
        if qos > 0:
            packet_id = struct.unpack("!H", body[pos:pos + 2])[0]
            pos += 2
        payload = body[pos:]
        if qos == 1 and packet_id is not None:
            self.send_puback(packet_id)
        self.broker.publish(topic, payload, qos, retain, self)

    def _handle_subscribe(self, body):
        packet_id = struct.unpack("!H", body[0:2])[0]
        pos = 2
        new_filters = []
        while pos < len(body):
            flen = struct.unpack("!H", body[pos:pos + 2])[0]
            pos += 2
            topic_filter = body[pos:pos + flen].decode("utf-8", "replace")
            pos += flen
            requested_qos = body[pos]
            pos += 1
            new_filters.append((topic_filter, requested_qos))
        granted = []
        for topic_filter, requested_qos in new_filters:
            self.filters.add(topic_filter)
            granted.append(min(requested_qos, 1))
        self.send_suback(packet_id, granted)
        self.broker.handle_subscribe(self, [f for f, _ in new_filters])

    def _handle_unsubscribe(self, body):
        packet_id = struct.unpack("!H", body[0:2])[0]
        pos = 2
        while pos < len(body):
            flen = struct.unpack("!H", body[pos:pos + 2])[0]
            pos += 2
            topic_filter = body[pos:pos + flen].decode("utf-8", "replace")
            pos += flen
            self.filters.discard(topic_filter)
        self.send_unsuback(packet_id)


class MiniMQTTBroker:
    """A tiny MQTT 3.1.1 broker bound to a single loopback address/port,
    running its accept loop and per-connection I/O on background threads."""

    def __init__(self, host="127.0.0.1", port=0, on_publish=None):
        self.host = host
        self.on_publish = on_publish
        self._lock = threading.Lock()
        self._clients = []
        self._retained = {}  # topic -> payload bytes
        self._running = False
        self._server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._server_sock.bind((host, port))
        self.port = self._server_sock.getsockname()[1]
        self._server_sock.listen(16)
        self._accept_thread = None

    def start(self):
        self._running = True
        self._server_sock.settimeout(0.5)
        self._accept_thread = threading.Thread(target=self._accept_loop, daemon=True)
        self._accept_thread.start()

    def _accept_loop(self):
        while self._running:
            try:
                conn, addr = self._server_sock.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            conn.settimeout(None)
            client = _ClientConn(self, conn, addr)
            with self._lock:
                self._clients.append(client)
            client.start()

    def stop(self, timeout=5):
        self._running = False
        try:
            self._server_sock.close()
        except OSError:
            pass
        with self._lock:
            clients = list(self._clients)
        for c in clients:
            c.close()
        if self._accept_thread:
            self._accept_thread.join(timeout=timeout)

    # -- called from client connection threads --------------------------
    def publish(self, topic, payload, qos, retain, from_client):
        if self.on_publish:
            try:
                self.on_publish(topic, payload, qos, retain, from_client.client_id)
            except Exception:
                pass
        if retain:
            with self._lock:
                if payload == b"":
                    self._retained.pop(topic, None)
                else:
                    self._retained[topic] = payload
        with self._lock:
            targets = [c for c in self._clients if c.matches(topic)]
        for c in targets:
            c.deliver(topic, payload, retain)

    def handle_subscribe(self, client, new_filters):
        with self._lock:
            retained_items = list(self._retained.items())
        for topic, payload in retained_items:
            if any(topic_matches(f, topic) for f in new_filters):
                client.deliver(topic, payload, True)

    def remove_client(self, client):
        with self._lock:
            if client in self._clients:
                self._clients.remove(client)


def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 0

    def on_publish(topic, payload, qos, retain, client_id):
        print("PUBLISH from %s: topic=%s qos=%d retain=%s payload=%r"
              % (client_id, topic, qos, retain, payload[:200]))

    broker = MiniMQTTBroker("127.0.0.1", port, on_publish=on_publish)
    broker.start()
    print("Mini MQTT broker listening on 127.0.0.1:%d (Ctrl+C to stop)" % broker.port)
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        broker.stop()


if __name__ == "__main__":
    main()
