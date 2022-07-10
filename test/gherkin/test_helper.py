from pytest_bdd import scenario, given, when, then, parsers
import requests

@scenario('helper.feature', 'Test left trim function')
def test_ltrim():
    pass

@given(parsers.parse('I am testing the "{module}" module'))
def setup_test_module(test_domoticz, module):
    if module == "helper":
        test_domoticz.sTestModule = "helper"

@when(parsers.parse('I test the function "{function}"'))
def setup_test_function(test_domoticz,function):
    test_domoticz.sTestFunction = function

@when('I provide the following input\n{table}')
def setup_test_input(test_domoticz,table):
    pass