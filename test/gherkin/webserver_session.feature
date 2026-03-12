Feature: Webserver session handling
    The Domoticz webserver should set a session cookie when certain resources are requested.
    But not when the API is used.

    Background:
        Given Domoticz is running
        And accessible on port 8080

    Scenario: Do not receive a session cookie when requesting the index page
        Given I am a normal Domoticz user
        When I request the URI "/"
        Then the HTTP-return code should be "200"
        And the HTTP-header "Set-Cookie" should be absent

    Scenario: Do not receive a session cookie when calling the API
        Given I am a normal Domoticz user
        When I request the URI "/json.htm?type=command&param=getversion"
        Then the HTTP-return code should be "200"
        And the HTTP-header "Set-Cookie" should be absent

    Scenario: Do not receive a session cookie when POSTing no/wrong credentials to the login page
        Given I am a normal Domoticz user
        When I submit wrong credentials to the loginpage
        Then the HTTP-return code should be "200"
        And the HTTP-header "Set-Cookie" should be absent

    Scenario: Receive a session cookie when POSTing to the login page
        Given I am a normal Domoticz user
        When I submit correct credentials to the loginpage
        Then the HTTP-return code should be "200"
        And the HTTP-header "Set-Cookie" should comply to pattern "^DMZSID=([0-9a-f]{32})_([0-9a-zA-Z]{48,52})\.([0-9]{10}); HttpOnly; SameSite=strict; path=\/; Expires=([0-9a-zA-Z ,:]{25}) GMT$"
