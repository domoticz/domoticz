from pytest_bdd import scenario, given, when, then, parsers
import requests

class Domoticz:
    sBaseURI = ""
    iPort = ""
    sVersion = ""
    oResponse = ""
    oReqHeaders = ""

@scenario('webserver.feature', 'Get uncompressed source in compressed form')
def test_compressedout():
    pass

@scenario('webserver.feature', 'Get uncompressed source in uncompressed form')
def test_uncompressedout():
    pass

@scenario('webserver.feature', 'Get compressed source in uncompressed form')
def test_decompressedout():
    pass

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

@given(parsers.parse('my browser {gzipsupport} receiving compressed data'))
def setup_request(test_domoticz, gzipsupport):
    if gzipsupport == "supports":
        test_domoticz.oReqHeaders = {"Accept-Encoding": "gzip, deflate"}


@when(parsers.parse('I request the URI "{uri}"'))
def request_uri(test_domoticz,uri):
    oResult = requests.get(test_domoticz.sBaseURI + uri, headers=test_domoticz.oReqHeaders)
    test_domoticz.oResponse = oResult

@then(parsers.parse('the HTTP-return code should be "{returncode:d}"'))
def check_returncode(test_domoticz,returncode):
    assert test_domoticz.oResponse.status_code == returncode

@then(parsers.parse('I should receive the content saying "{content}"'))
def check_content(test_domoticz,content):
    print(test_domoticz.oResponse.text)
    assert test_domoticz.oResponse.text.rstrip() == content
    #pass

@then(parsers.parse('the HTTP-header "{headername}" should contain "{headervalue}"'))
def check_header(test_domoticz,headername, headervalue):
    bExists = headername in test_domoticz.oResponse.headers
    if bExists:
        assert test_domoticz.oResponse.headers[headername] == headervalue
    else:
        assert False

@then(parsers.parse('the HTTP-header "{headername}" should be absent'))
def check_noheader(test_domoticz,headername, headervalue):
    assert not headername in test_domoticz.oResponse.headers
