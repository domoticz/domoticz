from pytest_bdd import scenario, given, when, then, parsers
import requests

@scenario('webserver_session.feature', 'Receive a session cookie when requesting the index page')
def test_cookiefromindex():
    pass

@scenario('webserver_session.feature', 'Do not receive a session cookie when calling the API')
def test_nocookiefromapi():
    pass
