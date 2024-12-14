from pytest_bdd import scenario, given, when, then, parsers
import requests, subprocess

@scenario('jwt.feature', 'Test JWT decode function')
def test_jwt_decode():
    pass

@scenario('jwt.feature', 'Test JWT decode function 2')
def test_jwt_decode2():
    pass

@scenario('jwt.feature', 'Test JWT decode function 3')
def test_jwt_decode3():
    pass

@given(parsers.parse('I am testing the "{module}" module'))
def setup_test_module(test_domoticz, module):
    if module == "jwt":
        test_domoticz.sTestModule = "jwt"
    else:
        assert False

@when(parsers.parse('I test the function "{function}"'))
def setup_test_function(test_domoticz,function):
    test_domoticz.sTestFunction = function

@when(parsers.parse('I provide the following input "{input}"'))
def setup_test_input(test_domoticz,input):
    test_domoticz.sTestInput = input

@then(parsers.parse('I expect the function to {succeedorfail}'))
def execute_test(test_domoticz, succeedorfail):
    sOut = subprocess.run([ test_domoticz.sCommand, "-quiet", "-module", test_domoticz.sTestModule, "-function", test_domoticz.sTestFunction, "-input", test_domoticz.sTestInput ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if (succeedorfail == "succeed" and sOut.returncode != 0):
        assert False
    sResult = sOut.stdout.decode("utf-8").split("|")
    if (succeedorfail == "fail" and sOut.returncode != 0):
        if (len(sResult) > 1 and sResult[1].find("Failed! ") > 0):
            sResult = sResult[1].split("! (")
            sResult = sResult[1]
            test_domoticz.sTestOutput = sResult[0:sResult.rfind(")")]
        else:
            test_domoticz.sTestOutput = ""
    else:
        if not (len(sResult) > 1 and sResult[1].find("Result : ") > 0):
            assert False
        sResult = sResult[1].split(": .")
        sResult = sResult[1]
        test_domoticz.sTestOutput = sResult[0:sResult.rfind(".")]

@then(parsers.parse('have the following result "{output}"'))
def check_test_output(test_domoticz,output):
    assert test_domoticz.sTestOutput == output
