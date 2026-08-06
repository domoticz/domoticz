import base64
import json
import os

import pytest
import requests
from pytest_bdd import given, parsers, scenarios, then, when

BASE = os.environ.get("DOMOTICZ_TEST_BASE", "http://localhost:8080")
ADMIN_USER = "YWRtaW4="                                    # base64 "admin"
ADMIN_PASS = "59515f6b193071e263f14bfa94bef645"            # md5 "domoticz"
VIEWER_PASS_MD5 = "5519c3ceabbd87956bcdbf2ecf312cbb"       # md5 "bddviewerpass"
THEME_CAP = 32

scenarios("themesettings.feature")


def _login(username_b64, password_md5):
    s = requests.Session()
    r = s.post(BASE + "/json.htm?type=command&param=logincheck",
               data={"username": username_b64, "password": password_md5})
    assert r.status_code == 200 and r.json().get("status") == "OK"
    return s


def _b64(name):
    return base64.b64encode(name.encode()).decode()


def _add_user(admin, name, password_md5):
    # adduser wants the plaintext username; the server base64-encodes it
    # internally before storing (unlike logincheck, which wants base64).
    r = admin.get(BASE + "/json.htm", params={
        "type": "command", "param": "adduser", "enabled": "true",
        "username": name, "password": password_md5, "rights": "0",
        "RemoteSharing": "false", "TabsEnabled": "31"})
    body = r.json()
    assert body.get("status") == "OK" or "Duplicate" in body.get("message", "")


def _user_idx(admin, name):
    r = admin.get(BASE + "/json.htm?type=command&param=getusers")
    for row in r.json().get("result", []):
        if row["Username"] == name:
            return row["idx"]
    return None


def _delete_user(admin, name):
    idx = _user_idx(admin, name)
    if idx is not None:
        admin.get(BASE + "/json.htm", params={"type": "command", "param": "deleteuser", "idx": idx})


def _set(session, param, theme, value=None, token=None, reset=False):
    data = {"theme": theme}
    if reset:
        data["reset"] = "true"
    if value is not None:
        data["value"] = value
    if token is not None:
        data["lastupdate"] = token
    return session.post(BASE + "/json.htm?type=command&param=" + param, data=data)


@pytest.fixture
def ctx():
    c = {}
    yield c
    # The theme-cap scenario creates many rows against the per-user quota;
    # clean them up so the quota does not leak into other scenarios or
    # into a later re-run of the suite against the same database.
    if "cap_theme_names" in c and "viewer" in c:
        for name in c["cap_theme_names"]:
            _set(c["viewer"], "themesettings_set", name, reset=True)


@given("Domoticz is running with themesettings support")
def dz_running(ctx):
    r = requests.get(BASE + "/json.htm?type=command&param=getversion")
    assert r.status_code == 200


@given("an authenticated admin session")
def admin_session(ctx):
    ctx["admin"] = _login(ADMIN_USER, ADMIN_PASS)
    # clean slate for the test theme
    _set(ctx["admin"], "themesettings_setdefault", "bddtheme", reset=True)


@given(parsers.parse('a viewer user "{name}" with an authenticated session'))
def viewer_session(ctx, name):
    _add_user(ctx["admin"], name, VIEWER_PASS_MD5)
    ctx["viewer"] = _login(_b64(name), VIEWER_PASS_MD5)
    _set(ctx["viewer"], "themesettings_set", "bddtheme", reset=True)


@when(parsers.parse("the admin stores the instance default '{value}' for theme \"{theme}\""))
@given(parsers.parse("the admin stored the instance default '{value}' for theme \"{theme}\""))
def admin_setdefault(ctx, value, theme):
    ctx["last"] = _set(ctx["admin"], "themesettings_setdefault", theme, value=value)


@when(parsers.parse("the viewer stores the overlay '{value}' for theme \"{theme}\""))
@given(parsers.parse("the viewer stored the overlay '{value}' for theme \"{theme}\""))
def viewer_set(ctx, value, theme):
    ctx["last"] = _set(ctx["viewer"], "themesettings_set", theme, value=value)


@when(parsers.parse("the viewer stores the overlay '{value}' for theme \"{theme}\" with token \"{token}\""))
def viewer_set_token(ctx, value, theme, token):
    ctx["last"] = _set(ctx["viewer"], "themesettings_set", theme, value=value, token=token)


@when(parsers.parse('the viewer tries to store an instance default for theme "{theme}"'))
def viewer_setdefault(ctx, theme):
    ctx["last"] = _set(ctx["viewer"], "themesettings_setdefault", theme, value='{"x": 1}')


@when(parsers.parse('the viewer resets theme "{theme}"'))
def viewer_reset(ctx, theme):
    ctx["last"] = _set(ctx["viewer"], "themesettings_set", theme, reset=True)


@when(parsers.parse('the viewer sends the overlay for theme "{theme}" as a GET request'))
def viewer_set_get(ctx, theme):
    ctx["last"] = ctx["viewer"].get(BASE + "/json.htm", params={
        "type": "command", "param": "themesettings_set", "theme": theme, "value": '{"a": 1}'})


@when(parsers.parse('the admin deletes user "{name}" and recreates it'))
def delete_and_recreate(ctx, name):
    idx_before = _user_idx(ctx["admin"], name)
    _delete_user(ctx["admin"], name)
    _add_user(ctx["admin"], name, VIEWER_PASS_MD5)
    idx_after = _user_idx(ctx["admin"], name)
    assert idx_after == idx_before, "test assumption broken: recreated user did not reuse the deleted rowid"
    ctx["recreated"] = _login(_b64(name), VIEWER_PASS_MD5)


@given(parsers.parse("a temporary user \"{name}\" stored the overlay '{value}' for theme \"{theme}\""))
def temp_user_overlay(ctx, name, value, theme):
    _delete_user(ctx["admin"], name)
    _add_user(ctx["admin"], name, VIEWER_PASS_MD5)
    s = _login(_b64(name), VIEWER_PASS_MD5)
    assert _set(s, "themesettings_set", theme, value=value).json().get("status") == "OK"


@when(parsers.parse("the viewer stores {count:d} distinct overlay themes"))
def viewer_stores_many_themes(ctx, count):
    theme_names = [f"bddcaptheme{i}" for i in range(count)]
    ctx["cap_theme_names"] = theme_names
    responses = [_set(ctx["viewer"], "themesettings_set", name, value='{"a": 1}') for name in theme_names]
    ctx["cap_responses"] = responses
    ctx["last"] = responses[-1]


@then("the write is accepted")
def write_ok(ctx):
    body = ctx["last"].json()
    assert body.get("status") == "OK" and "lastupdate" in body


@then(parsers.parse('the write is rejected with error "{code}"'))
def write_rejected(ctx, code):
    assert ctx["last"].status_code != 200
    assert ctx["last"].json().get("error") == code


@then(parsers.parse('the {ordinal} write is rejected with error "{code}"'))
def nth_write_rejected(ctx, ordinal, code):
    # only used for the cap scenario ("the 33rd write ..."); the earlier
    # THEME_CAP writes in ctx["cap_responses"] must all have succeeded.
    for r in ctx["cap_responses"][:THEME_CAP]:
        assert r.status_code == 200 and r.json().get("status") == "OK"
    last = ctx["cap_responses"][-1]
    assert last.status_code != 200
    assert last.json().get("error") == code


@then(parsers.parse("the request is rejected with HTTP status {status:d}"))
def request_forbidden(ctx, status):
    assert ctx["last"].status_code == status


@then(parsers.parse('themesettings_get for "{theme}" as viewer shows the instance layer present'))
def get_instance_present(ctx, theme):
    body = ctx["viewer"].get(BASE + "/json.htm", params={
        "type": "command", "param": "themesettings_get", "theme": theme}).json()
    assert body["status"] == "OK" and body["instance"]["present"] is True


@then(parsers.parse('themesettings_get for "{theme}" as viewer shows the user layer absent'))
def get_user_absent(ctx, theme):
    body = ctx["viewer"].get(BASE + "/json.htm", params={
        "type": "command", "param": "themesettings_get", "theme": theme}).json()
    assert body["status"] == "OK" and body["user"]["present"] is False


@then(parsers.parse('themesettings_get for "{theme}" as the recreated user shows the user layer absent'))
def recreated_user_absent(ctx, theme):
    body = ctx["recreated"].get(BASE + "/json.htm", params={
        "type": "command", "param": "themesettings_get", "theme": theme}).json()
    assert body["status"] == "OK" and body["user"]["present"] is False


@then(parsers.parse("getsettings as viewer returns ThemeSettings \"{theme}\" equal to '{value}'"))
def getsettings_viewer(ctx, theme, value):
    body = ctx["viewer"].get(BASE + "/json.htm?type=command&param=getsettings").json()
    assert body["ThemeSettings"][theme] == json.loads(value)


@then(parsers.parse("getsettings as admin returns ThemeSettings \"{theme}\" equal to '{value}'"))
def getsettings_admin(ctx, theme, value):
    body = ctx["admin"].get(BASE + "/json.htm?type=command&param=getsettings").json()
    assert body["ThemeSettings"][theme] == json.loads(value)


@then(parsers.parse("getversion as viewer contains ThemeSettingsAPI {flag:d}"))
def getversion_flag(ctx, flag):
    body = ctx["viewer"].get(BASE + "/json.htm?type=command&param=getversion").json()
    assert body.get("ThemeSettingsAPI") == flag
