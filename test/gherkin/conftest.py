from pytest_bdd import scenario, given, when, then, parsers
import requests

class Domoticz:
    sBaseURI = ""
    iPort = ""
    sVersion = ""
    oResponse = ""
    oReqHeaders = ""

@given('Domoticz is running')
def test_domoticz():
    Domoticz.sBaseURI = "http://localhost"
    return Domoticz()

@given(parsers.parse('accessible on port {port:d}'))
def check_domoticz_port(test_domoticz,port):
    oResult = requests.get(test_domoticz.sBaseURI + ":" + str(port) + "/json.htm?type=command&param=getversion")
    if oResult.status_code == 200:
        test_domoticz.iPort = port
        test_domoticz.sBaseURI += ":" + str(port)
        oJSON = oResult.json()
        test_domoticz.sVersion = oJSON["version"]
    assert oResult.status_code == 200

@given('I am a normal Domoticz user')
def setup_user():
    pass

@when(parsers.parse('I request the URI "{uri}"'))
def request_uri(test_domoticz,uri):
    oResult = requests.get(test_domoticz.sBaseURI + uri, headers=test_domoticz.oReqHeaders)
    test_domoticz.oResponse = oResult

@when(parsers.parse('I request the "{method}"'))
def request_uri(test_domoticz,method):
    if method == "Configuration Settings":
        uri = '/json.htm?type=settings'
    elif method == "Version Information":
        uri = '/json.htm?type=command&param=getversion'
    else:
        uri = '/idonotexist'

    oResult = requests.get(test_domoticz.sBaseURI + uri, headers=test_domoticz.oReqHeaders)
    test_domoticz.oResponse = oResult

@then(parsers.parse('the HTTP-return code should be "{returncode:d}"'))
def check_returncode(test_domoticz,returncode):
    assert test_domoticz.oResponse.status_code == returncode

@then(parsers.parse('the HTTP-header "{headername}" should contain "{headervalue}"'))
def check_header(test_domoticz,headername, headervalue):
    bExists = headername in test_domoticz.oResponse.headers
    if bExists:
        assert test_domoticz.oResponse.headers[headername] == headervalue
    else:
        print(test_domoticz.oResponse.headers)
        assert False

@then(parsers.parse('the HTTP-header "{headername}" should be absent'))
def check_noheader(test_domoticz,headername, headervalue):
    assert not headername in test_domoticz.oResponse.headers
