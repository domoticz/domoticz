from pytest_bdd import scenario, given, when, then, parsers
import requests, subprocess

@scenario('helper.feature', 'Test trim function')
def test_trim():
    pass

@scenario('helper.feature', 'Test right trim function')
def test_rtrim():
    pass

@scenario('helper.feature', 'Test left trim function')
def test_ltrim():
    pass

@scenario('helper.feature', 'Test left trim function 2')
def test_ltrim2():
    pass

@scenario('helper.feature', 'Test left trim function 3')
def test_ltrim3():
    pass

@scenario('helper.feature', 'Test left trim function 4')
def test_ltrim4():
    pass

@scenario('helper.feature', 'Test MD5Hash generation function')
def test_md5hash():
    pass

@given(parsers.parse('I am testing the "{module}" module'))
def setup_test_module(test_domoticz, module):
    if module == "helper":
        test_domoticz.sTestModule = "helper"
    else:
        assert False

@when(parsers.parse('I test the function "{function}"'))
def setup_test_function(test_domoticz,function):
    test_domoticz.sTestFunction = function

@when(parsers.parse('I provide the following input "{input}"'))
def setup_test_input(test_domoticz,input):
    test_domoticz.sTestInput = input

@then('I expect the function to succeed')
def execute_test(test_domoticz):
    sOut = subprocess.run([ test_domoticz.sCommand, "-quiet", "-module", test_domoticz.sTestModule, "-function", test_domoticz.sTestFunction, "-input", test_domoticz.sTestInput ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if (sOut.returncode != 0):
        assert False
    sResult = sOut.stdout.decode("utf-8").split("|")
    if not (len(sResult) > 1 and sResult[1].find("Result : ") > 0):
        assert False
    sResult = sResult[1].split(": .")
    sResult = sResult[1].split(".")
    test_domoticz.sTestOutput = sResult[0]

@then(parsers.parse('have the following result "{output}"'))
def check_test_output(test_domoticz,output):
    assert test_domoticz.sTestOutput == output
