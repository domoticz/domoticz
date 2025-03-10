from pytest_bdd import scenario, given, when, then, parsers
import requests

@scenario('webserver_session.feature', 'Do not receive a session cookie when requesting the index page')
def test_nocookiefromindex():
    pass

@scenario('webserver_session.feature', 'Do not receive a session cookie when calling the API')
def test_nocookiefromapi():
    pass

@scenario('webserver_session.feature', 'Do not receive a session cookie when POSTing no/wrong credentials to the login page')
def test_nocookiefromlogin():
    pass

@scenario('webserver_session.feature', 'Receive a session cookie when POSTing to the login page')
def test_cookiefromlogin():
    pass
