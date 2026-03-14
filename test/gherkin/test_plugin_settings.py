from pytest_bdd import scenario, given, when, then, parsers
import requests
import json
import urllib.parse

HTYPE_PYTHONPLUGIN = 94
BASE_URL = ""
last_hardware_idx = None


@scenario('plugin_settings.feature', 'Add hardware with Settings JSON')
def test_add_hardware_with_settings():
    pass


@scenario('plugin_settings.feature', 'Update hardware Settings')
def test_update_hardware_settings():
    pass


@scenario('plugin_settings.feature', 'Empty Settings string is accepted')
def test_empty_settings():
    pass


@scenario('plugin_settings.feature', 'Settings returned as JSON object')
def test_settings_json_object():
    pass


@scenario('plugin_settings.feature', 'Reject malformed Settings JSON')
def test_reject_malformed_json():
    pass


@scenario('plugin_settings.feature', 'Existing Mode1-6 plugins work unchanged')
def test_mode_plugins_unchanged():
    pass


# Helper to get the base URL from the test_domoticz fixture
def get_base_url(test_domoticz):
    return test_domoticz.sBaseURI


def add_plugin_hardware(base_url, name, settings="", mode1="", mode2="", mode3="", mode4="", mode5="", mode6=""):
    """Add a Python plugin hardware entry via the API."""
    url = (
        base_url + "/json.htm?type=command&param=addhardware"
        "&htype=" + str(HTYPE_PYTHONPLUGIN) +
        "&name=" + urllib.parse.quote(name) +
        "&enabled=true"
        "&datatimeout=0"
        "&loglevel=0"
        "&address="
        "&port=0"
        "&serialport="
        "&username="
        "&password="
        "&extra=" + urllib.parse.quote("test-plugin-key") +
        "&Mode1=" + urllib.parse.quote(mode1) +
        "&Mode2=" + urllib.parse.quote(mode2) +
        "&Mode3=" + urllib.parse.quote(mode3) +
        "&Mode4=" + urllib.parse.quote(mode4) +
        "&Mode5=" + urllib.parse.quote(mode5) +
        "&Mode6=" + urllib.parse.quote(mode6) +
        "&settings=" + urllib.parse.quote(settings)
    )
    return requests.get(url)


def get_hardware_list(base_url):
    """Get the full hardware list."""
    url = base_url + "/json.htm?type=command&param=gethardware"
    return requests.get(url)


def find_last_plugin_hardware(base_url):
    """Find the last Python plugin hardware entry."""
    result = get_hardware_list(base_url)
    data = result.json()
    if "result" not in data:
        return None
    for hw in reversed(data["result"]):
        if hw.get("Type") == HTYPE_PYTHONPLUGIN:
            return hw
    return None


def update_plugin_hardware(base_url, idx, name, settings=""):
    """Update a Python plugin hardware entry via the API."""
    url = (
        base_url + "/json.htm?type=command&param=updatehardware"
        "&htype=" + str(HTYPE_PYTHONPLUGIN) +
        "&idx=" + str(idx) +
        "&name=" + urllib.parse.quote(name) +
        "&enabled=true"
        "&datatimeout=0"
        "&loglevel=0"
        "&address="
        "&port=0"
        "&serialport="
        "&username="
        "&password="
        "&extra=" + urllib.parse.quote("test-plugin-key") +
        "&Mode1=&Mode2=&Mode3=&Mode4=&Mode5=&Mode6=" +
        "&settings=" + urllib.parse.quote(settings)
    )
    return requests.get(url)


# --- Step implementations ---

@when(parsers.parse("I add a Python plugin hardware with settings '{settings}'"))
def add_hardware_with_settings(test_domoticz, settings):
    global last_hardware_idx
    base_url = get_base_url(test_domoticz)
    result = add_plugin_hardware(base_url, "TestPlugin_Settings", settings=settings)
    test_domoticz.oResponse = result
    # Try to capture the idx
    try:
        data = result.json()
        if "idx" in data:
            last_hardware_idx = data["idx"]
    except Exception:
        pass


@when("I add a Python plugin hardware with mode values and no settings")
def add_hardware_with_modes(test_domoticz):
    global last_hardware_idx
    base_url = get_base_url(test_domoticz)
    result = add_plugin_hardware(base_url, "TestPlugin_Modes", mode1="testmode1", mode2="testmode2")
    test_domoticz.oResponse = result
    try:
        data = result.json()
        if "idx" in data:
            last_hardware_idx = data["idx"]
    except Exception:
        pass


@when(parsers.parse("I update the last hardware settings to '{settings}'"))
def update_hardware_settings(test_domoticz, settings):
    base_url = get_base_url(test_domoticz)
    hw = find_last_plugin_hardware(base_url)
    assert hw is not None, "No plugin hardware found to update"
    result = update_plugin_hardware(base_url, hw["idx"], hw["Name"], settings=settings)
    test_domoticz.oResponse = result


@when("I request the hardware list")
def request_hardware_list(test_domoticz):
    base_url = get_base_url(test_domoticz)
    result = get_hardware_list(base_url)
    test_domoticz.oResponse = result


@then(parsers.parse('the response status should be "{status}"'))
def check_response_status(test_domoticz, status):
    data = test_domoticz.oResponse.json()
    assert data.get("status") == status, f"Expected status '{status}', got '{data.get('status')}'"


@then(parsers.parse('the last hardware entry should have setting "{key}" with value "{value}"'))
def check_setting_value(test_domoticz, key, value):
    base_url = get_base_url(test_domoticz)
    hw = find_last_plugin_hardware(base_url)
    assert hw is not None, "No plugin hardware found"
    settings = hw.get("Settings", {})
    assert key in settings, f"Setting '{key}' not found in {settings}"
    assert settings[key] == value, f"Expected '{value}', got '{settings[key]}'"


@then("the last hardware entry should have empty settings")
def check_empty_settings(test_domoticz):
    base_url = get_base_url(test_domoticz)
    hw = find_last_plugin_hardware(base_url)
    assert hw is not None, "No plugin hardware found"
    settings = hw.get("Settings", {})
    assert settings == {} or settings == "", f"Expected empty settings, got {settings}"


@then("the last hardware entry Settings should be a JSON object")
def check_settings_is_object(test_domoticz):
    base_url = get_base_url(test_domoticz)
    hw = find_last_plugin_hardware(base_url)
    assert hw is not None, "No plugin hardware found"
    settings = hw.get("Settings")
    assert isinstance(settings, dict), f"Expected dict, got {type(settings)}"


@then("the add hardware should fail silently")
def check_add_failed(test_domoticz):
    # Malformed JSON should cause the server to reject silently (no "status": "OK")
    data = test_domoticz.oResponse.json()
    assert data.get("status") != "OK" or test_domoticz.oResponse.status_code != 200


@then(parsers.parse('the last hardware entry should have Mode1 value "{value}"'))
def check_mode1_value(test_domoticz, value):
    base_url = get_base_url(test_domoticz)
    hw = find_last_plugin_hardware(base_url)
    assert hw is not None, "No plugin hardware found"
    assert str(hw.get("Mode1", "")) == value, f"Expected Mode1='{value}', got '{hw.get('Mode1')}'"
