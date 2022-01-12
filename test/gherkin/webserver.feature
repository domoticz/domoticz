Feature: Webserver handling
    The Domoticz webserver should be able to handle different content types both compressed and uncompressed
    and should respect several headers as provided by the users Browser

    Background:
        Given Domoticz is running
        And accessible on port 8080

    Scenario: Get uncompressed source in compressed form
        Given I am a normal Domoticz user
        And my browser supports receiving compressed data
        When I request the URI "/"
        Then the HTTP-return code should be "200"
        And the HTTP-header "Content-Type" should contain "text/html;charset=UTF-8"
        And the HTTP-header "Content-Encoding" should contain "gzip"

    Scenario: Get uncompressed source in uncompressed form
        Given I am a normal Domoticz user
        And my browser does not supports receiving compressed data
        When I request the URI "/index.html"
        Then the HTTP-return code should be "200"
        And the HTTP-header "Content-Type" should contain "text/html;charset=UTF-8"
        And the HTTP-header "Content-Encoding" should be absent

    Scenario: Get compressed source in uncompressed form
        Given I am a normal Domoticz user
        And my browser does not supports receiving compressed data
        When I request the URI "/test/test1.html"
        Then the HTTP-return code should be "200"
        And I should receive the content saying "It Works!"
        And the HTTP-header "Content-Type" should contain "text/html;charset=UTF-8"
        And the HTTP-header "Content-Encoding" should be absent

    Scenario: Get compressed source in compressed form
        Given I am a normal Domoticz user
        And my browser supports receiving compressed data
        When I request the URI "/test/test1.html"
        Then the HTTP-return code should be "200"
        And I should receive the content saying "It Works!"
        And the HTTP-header "Content-Type" should contain "text/html;charset=UTF-8"
        And the HTTP-header "Content-Encoding" should contain "gzip"

    Scenario: Get regular file without extension
        Given I am a normal Domoticz user
        When I request the URI "/test/test2"
        Then the HTTP-return code should be "200"
        And I should receive the content saying "A regular file without extension!"
        And the HTTP-header "Content-Length" should contain "34"
        And the HTTP-header "Content-Type" should be absent

    Scenario: Validate standard Not Found reply
        Given I am a normal Domoticz user
        When I request the URI "/idonotexist.png"
        Then the HTTP-return code should be "404"
        And the HTTP-header "Content-Type" should contain "text/html;charset=UTF-8"
