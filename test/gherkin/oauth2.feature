Feature: OAuth2
    The Domoticz webserver should handle Authentication and Authorization as configured.
    Domoticz tries to adhere to the OAuth2/OpenID Connect (OIDC) standards as much as possible

    Background:
        Given Domoticz is running
        And accessible on port 8080
        And configured with security enabled

    Scenario: Deny access to secured method without a Token
        Given I am a normal Domoticz user
        When I request the "Configuration Settings"
        Then the HTTP-return code should be "401"
        And the HTTP-header "Content-Type" should contain "text/html;charset=UTF-8"

    Scenario: Allow access to unsecured method without a Token
        Given I am a normal Domoticz user
        When I request the "Version Information"
        Then the HTTP-return code should be "200"
        And the HTTP-header "Content-Type" should contain "application/json;charset=UTF-8"
        And the HTTP-header "WWW-Authenticate" should be absent
