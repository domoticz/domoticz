Feature: Extended Plugin Settings
    The Settings column stores custom plugin configuration as JSON.
    Settings are sent, stored, and returned via the hardware API.

    Background:
        Given Domoticz is running
        And accessible on port 8080

    Scenario: Add hardware with Settings JSON
        Given I am a normal Domoticz user
        When I add a Python plugin hardware with settings '{"Interval":"30","EnableDebug":"true"}'
        Then the HTTP-return code should be "200"
        And the response status should be "OK"
        When I request the hardware list
        Then the last hardware entry should have setting "Interval" with value "30"
        And the last hardware entry should have setting "EnableDebug" with value "true"

    Scenario: Update hardware Settings
        Given I am a normal Domoticz user
        When I add a Python plugin hardware with settings '{"Interval":"30"}'
        And I update the last hardware settings to '{"Interval":"60","NewField":"hello"}'
        And I request the hardware list
        Then the last hardware entry should have setting "Interval" with value "60"
        And the last hardware entry should have setting "NewField" with value "hello"

    Scenario: Empty Settings string is accepted
        Given I am a normal Domoticz user
        When I add a Python plugin hardware with settings ''
        Then the HTTP-return code should be "200"
        And the response status should be "OK"
        When I request the hardware list
        Then the last hardware entry should have empty settings

    Scenario: Settings returned as JSON object
        Given I am a normal Domoticz user
        When I add a Python plugin hardware with settings '{"Key1":"val1"}'
        And I request the hardware list
        Then the last hardware entry Settings should be a JSON object

    Scenario: Reject malformed Settings JSON
        Given I am a normal Domoticz user
        When I add a Python plugin hardware with settings 'not-valid-json'
        Then the add hardware should fail silently

    Scenario: Existing Mode1-6 plugins work unchanged
        Given I am a normal Domoticz user
        When I add a Python plugin hardware with mode values and no settings
        Then the HTTP-return code should be "200"
        And the response status should be "OK"
        When I request the hardware list
        Then the last hardware entry should have Mode1 value "testmode1"
