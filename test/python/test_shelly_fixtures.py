"""Sanity checks of the Shelly fixtures and of the mock Shelly server. Needs no Domoticz.

    pytest test/python/test_shelly_fixtures.py
"""
import base64
import hashlib
import json
import os
import re
import socket
import struct
import sys
import urllib.error
import urllib.request

import pytest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mini_shelly_server import FIXTURE_DIR, MiniShellyServer, fixture_names, load_fixture  # noqa: E402

REQUIRED_GEN2 = ["2pm_gen3_cover", "pro_3em", "duo_bulb_gen3", "wall_display_xl", "presence_gen4"]
REQUIRED_GEN1 = ["shelly_em_gen1", "shelly_ht_gen1", "shelly_1pm_gen1", "rgbw2"]
MAC_RE = re.compile(r"^[0-9A-F]{12}$")


def is_gen1(doc):
    return "gen" not in doc["shelly"]


def test_required_fixtures_present():
    names = fixture_names()
    for name in REQUIRED_GEN2 + REQUIRED_GEN1:
        assert name in names, "missing fixture %s" % name


@pytest.mark.parametrize("name", fixture_names())
def test_fixture_has_required_keys(name):
    doc = load_fixture(name)
    shelly = doc["shelly"]
    assert isinstance(doc["status"], dict) and doc["status"], "status must be a non-empty object"
    assert MAC_RE.match(shelly["mac"]), "mac must be 12 upper case hex digits"
    if is_gen1(doc):
        assert set(doc) >= {"shelly", "settings", "status"}
        assert shelly["type"] and (shelly.get("fw") or "num_outputs" in shelly)
        assert isinstance(doc["settings"], dict)
        assert doc["status"].get("mac", shelly["mac"]) == shelly["mac"]
    else:
        assert set(doc) >= {"config", "shelly", "status"}
        assert isinstance(shelly["gen"], int) and shelly["gen"] >= 2
        assert shelly["model"] and shelly["id"]
        assert isinstance(doc["config"], dict) and doc["config"]
        # every component in the status is also configured (a battery has no config of its own)
        for key in doc["status"]:
            if ":" in key and not key.startswith("devicepower:"):
                assert key in doc["config"], "%s is in the status but not in the config" % key


@pytest.mark.parametrize("name", fixture_names())
def test_fixture_file_is_plain_json(name):
    with open(os.path.join(FIXTURE_DIR, name + ".json"), "rb") as f:
        raw = f.read()
    assert not raw.startswith(b"\xef\xbb\xbf"), "no BOM"
    json.loads(raw.decode("utf-8"))


# ---- the mock server itself ----------------------------------------------------------------------------

def http(mock, path, data=None, headers=None):
    req = urllib.request.Request("http://%s:%d%s" % (mock.host, mock.port, path), data=data, headers=headers or {})
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.status, json.loads(r.read().decode("utf-8")), r.headers
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8")
        try:
            body = json.loads(body)
        except ValueError:
            pass
        return e.code, body, e.headers


def rpc(mock, method, params=None, headers=None):
    body = json.dumps({"id": 1, "method": method, "params": params or {}}).encode()
    return http(mock, "/rpc", body, dict({"Content-Type": "application/json"}, **(headers or {})))


def digest_header(www, password, uri="/rpc", method="POST"):
    fields = dict(re.findall(r'(\w+)="?([^",]+)"?', www))
    ha1 = hashlib.sha256(("admin:%s:%s" % (fields["realm"], password)).encode()).hexdigest()
    ha2 = hashlib.sha256(("%s:%s" % (method, uri)).encode()).hexdigest()
    nc, cnonce = "00000001", "abcdef"
    resp = hashlib.sha256(("%s:%s:%s:%s:auth:%s" % (ha1, fields["nonce"], nc, cnonce, ha2)).encode()).hexdigest()
    return ('Digest username="admin", realm="%s", nonce="%s", uri="%s", algorithm=SHA-256, qop=auth, nc=%s, cnonce="%s", response="%s"'
            % (fields["realm"], fields["nonce"], uri, nc, cnonce, resp))


def test_gen2_identity_status_and_switch():
    with MiniShellyServer("2pm_gen3_cover", mac="112233445566") as mock:
        code, doc, _ = http(mock, "/shelly")
        assert code == 200 and doc["mac"] == "112233445566" and doc["gen"] == 3
        assert doc["id"].endswith("112233445566".lower())
        code, doc, _ = rpc(mock, "Shelly.GetStatus")
        assert "cover:0" in doc["result"]
        code, doc, _ = rpc(mock, "Cover.GoToPosition", {"id": 0, "pos": 40})
        assert code == 200 and "result" in doc
        assert mock.status["cover:0"]["current_pos"] == 40
        assert mock.find_requests("Cover.GoToPosition")[0]["params"] == {"id": 0, "pos": 40}
        code, doc, _ = rpc(mock, "No.Such")
        assert doc["error"]["code"] == 404


def test_gen2_digest_auth():
    with MiniShellyServer("pro_3em", password="secret") as mock:
        code, doc, _ = http(mock, "/shelly")
        assert code == 200 and doc["auth_en"] is True
        code, doc, headers = rpc(mock, "Shelly.GetStatus")
        assert code == 401
        www = headers["WWW-Authenticate"]
        assert "SHA-256" in www
        code, doc, _ = rpc(mock, "Shelly.GetStatus", headers={"Authorization": digest_header(www, "secret")})
        assert code == 200 and "em:0" in doc["result"]
        code, doc, _ = rpc(mock, "Shelly.GetStatus", headers={"Authorization": digest_header(www, "wrong")})
        assert code == 401
        assert len(mock.find_requests(auth="ok")) == 1


def test_gen1_documents_and_commands():
    with MiniShellyServer("shelly_1pm_gen1") as mock:
        code, doc, _ = http(mock, "/shelly")
        assert doc["type"] == "SHSW-PM"
        code, doc, _ = http(mock, "/relay/0?turn=off")
        assert code == 200 and doc["ison"] is False
        assert mock.status["relays"][0]["ison"] is False
        code, doc, _ = http(mock, "/status")
        assert doc["relays"][0]["ison"] is False
        code, _, _ = http(mock, "/relay/5?turn=on")
        assert code == 404
        assert mock.find_requests(target_prefix="/relay/0")[0]["method"] == "GET"


def test_gen1_basic_auth():
    with MiniShellyServer("shelly_em_gen1", password="pw") as mock:
        assert http(mock, "/shelly")[0] == 200
        code, _, headers = http(mock, "/status")
        assert code == 401 and headers["WWW-Authenticate"].startswith("Basic")
        good = "Basic " + base64.b64encode(b"admin:pw").decode()
        assert http(mock, "/status", headers={"Authorization": good})[0] == 200
        bad = "Basic " + base64.b64encode(b"admin:no").decode()
        assert http(mock, "/status", headers={"Authorization": bad})[0] == 401


def ws_connect(mock):
    sock = socket.create_connection((mock.host, mock.port), timeout=5)
    key = base64.b64encode(os.urandom(16)).decode()
    sock.sendall(("GET /rpc HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                  "Sec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n" % key).encode())
    head = b""
    while b"\r\n\r\n" not in head:
        head += sock.recv(1)
    assert b"101" in head.split(b"\r\n")[0]
    accept = base64.b64encode(hashlib.sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()).decode()
    assert accept.encode() in head
    return sock


def ws_send(sock, text):
    payload = text.encode()
    mask = os.urandom(4)
    masked = bytes(c ^ mask[i % 4] for i, c in enumerate(payload))
    if len(payload) < 126:
        header = bytes([0x81, 0x80 | len(payload)])
    else:
        header = bytes([0x81, 0x80 | 126]) + struct.pack(">H", len(payload))
    sock.sendall(header + mask + masked)


def ws_recv(sock):
    def exact(n):
        buf = b""
        while len(buf) < n:
            chunk = sock.recv(n - len(buf))
            assert chunk, "connection closed"
            buf += chunk
        return buf
    b1, b2 = exact(2)
    n = b2 & 0x7F
    if n == 126:
        n = struct.unpack(">H", exact(2))[0]
    return json.loads(exact(n).decode())


def test_gen2_websocket_rpc_and_notify():
    with MiniShellyServer("2pm_gen3_cover") as mock:
        sock = ws_connect(mock)
        try:
            ws_send(sock, json.dumps({"id": 7, "src": "t", "method": "Sys.GetStatus"}))
            reply = ws_recv(sock)
            assert reply["id"] == 7 and "result" in reply
            assert mock.push_status({"cover:0": {"current_pos": 55}}) == 1
            note = ws_recv(sock)
            assert note["method"] == "NotifyStatus" and note["params"]["cover:0"]["current_pos"] == 55
        finally:
            sock.close()


def test_gen2_websocket_auth():
    with MiniShellyServer("pro_3em", password="secret") as mock:
        sock = ws_connect(mock)
        try:
            ws_send(sock, json.dumps({"id": 1, "src": "t", "method": "Sys.GetStatus"}))
            reply = ws_recv(sock)
            assert reply["error"]["code"] == 401
            challenge = json.loads(reply["error"]["message"])
            assert challenge["algorithm"] == "SHA-256"
            # not authenticated yet: nothing is pushed to this client
            assert mock.push_status({"em:0": {"a_act_power": 1.0}}) == 0
            ha1 = hashlib.sha256(("admin:%s:secret" % challenge["realm"]).encode()).hexdigest()
            ha2 = hashlib.sha256(b"dummy_method:dummy_uri").hexdigest()
            resp = hashlib.sha256(("%s:%s:00000001:99:auth:%s" % (ha1, challenge["nonce"], ha2)).encode()).hexdigest()
            auth = {"realm": challenge["realm"], "username": "admin", "nonce": challenge["nonce"], "cnonce": 99,
                    "nc": "00000001", "response": resp, "algorithm": "SHA-256"}
            ws_send(sock, json.dumps({"id": 2, "src": "t", "method": "Sys.GetStatus", "auth": auth}))
            assert "result" in ws_recv(sock)
            assert mock.push_status({"em:0": {"a_act_power": 2.0}}) == 1
            assert ws_recv(sock)["method"] == "NotifyStatus"
        finally:
            sock.close()
