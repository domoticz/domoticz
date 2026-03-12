from pytest_bdd import scenario, given, when, then, parsers
import requests

@scenario('oauth2.feature', 'Deny access to secured method without a Token')
def test_denysecuredwithouttoken():
    pass

@scenario('oauth2.feature', 'Allow access to unsecured method without a Token')
def test_allowunsecuredwithouttoken():
    pass

@given('configured with security enabled')
def configure_security():
    pass
