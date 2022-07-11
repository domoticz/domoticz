Feature: Helper routines
    Domoticz has a set of Helper routines which can be found in main/helper.cpp
    these helper functions provide all kind of support to make it easier for developers
    but therefor these functions have to stay reliable and predictable

    Background:
        Given Command domoticztester is available
        And can be executed on the commandline

    Scenario: Test left trim function
        Given I am testing the "helper" module
        When I test the function "stdstring_ltrim"
        And I provide the following input " 1 left"
        Then I expect the function to succeed
        And have the following result "1 left"

    Scenario: Test left trim function 2
        Given I am testing the "helper" module
        When I test the function "stdstring_ltrim"
        And I provide the following input " 1 left and right "
        Then I expect the function to succeed
        And have the following result "1 left and right "

    Scenario: Test left trim function 3
        Given I am testing the "helper" module
        When I test the function "stdstring_ltrim"
        And I provide the following input "  2 left and special\n"
        Then I expect the function to succeed
        And have the following result "2 left and special\n"

    Scenario: Test left trim function 4
        Given I am testing the "helper" module
        When I test the function "stdstring_ltrim"
        And I provide the following input " \r 1 left + special "
        Then I expect the function to succeed
        And have the following result "\r 1 left + special "

    Scenario: Test right trim function
        Given I am testing the "helper" module
        When I test the function "stdstring_rtrim"
        And I provide the following input " 1 left and 1 right "
        Then I expect the function to succeed
        And have the following result " 1 left and 1 right"

    Scenario: Test trim function
        Given I am testing the "helper" module
        When I test the function "stdstring_trim"
        And I provide the following input " 1 left and 1 right "
        Then I expect the function to succeed
        And have the following result "1 left and 1 right"

    Scenario: Test MD5Hash generation function
        Given I am testing the "helper" module
        When I test the function "GenerateMD5Hash"
        And I provide the following input "MD5 \#Hash Input wïth Speca1 (h@ract3r$!"
        Then I expect the function to succeed
        And have the following result "6c97df5a126629b446babadb6c5a619c"
