Feature: Helper routines
    Domoticz has a set of Helper routines which can be found in main/helper.cpp
    these helper functions provide all kind of support to make it easier for developers
    but therefor these functions have to stay reliable and predictable

    Background:
        Given Command domoticztester is available
        And can be executed on the commandline

    Scenario: Test left trim function
        Given I am testing the "helper" module
        When I test the function "std_ltrim"
        And I provide the following input
            | " 1 left"             |
            | "  2 left"            |
            | " 1 left and right "  |
            | " \t tab should stay" |
        Then I expect the function to succeed
        And have the following result
            | "1 left"              |
            | "2 left"              |
            | "1 left and right "   |
            | "\t tab should stay"  |
