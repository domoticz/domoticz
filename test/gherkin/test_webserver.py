from pytest_bdd import scenario, given, when, then, parsers
import requests

@scenario('webserver.feature', 'Get uncompressed source in compressed form')
def test_compressedout():
    pass

@scenario('webserver.feature', 'Get uncompressed source in uncompressed form')
def test_uncompressedout():
    pass

@scenario('webserver.feature', 'Get compressed source in uncompressed form')
def test_decompressedout():
    pass

@scenario('webserver.feature', 'Get compressed source in compressed form')
def test_notdecompressedout():
    pass

@scenario('webserver.feature', 'Get regular file without extension')
def test_regularfile():
    pass

@scenario('webserver.feature', 'Validate standard Not Found reply')
def test_notfoundfile():
    pass

@given(parsers.parse('my browser {gzipsupport} receiving compressed data'))
def setup_request(test_domoticz, gzipsupport):
    if gzipsupport == "supports":
        test_domoticz.oReqHeaders = {"Accept-Encoding": "gzip, deflate"}

@then(parsers.parse('I should receive the content saying "{content}"'))
def check_content(test_domoticz,content):
    print(test_domoticz.oResponse.text)
    assert test_domoticz.oResponse.text.rstrip() == content
