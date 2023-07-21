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
        And I provide the following input " 	 1 left + special "
        Then I expect the function to succeed
        And have the following result "	 1 left + special "

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

    Scenario: Test trim function UTF-8
        Given I am testing the "helper" module
        When I test the function "stdstring_trim"
        And I provide the following input "  Â°C 2 left UTF-8 Δ 1 right "
        Then I expect the function to succeed
        And have the following result "Â°C 2 left UTF-8 Δ 1 right"

    Scenario: Test whitespace trim function
        Given I am testing the "helper" module
        When I test the function "stdstring_trimws"
        And I provide the following input " 	 3 left with whitespace and UTF-8 Δ and 2 right  "
        Then I expect the function to succeed
        And have the following result "3 left with whitespace and UTF-8 Δ and 2 right"

    Scenario: Test MD5Hash generation function
        Given I am testing the "helper" module
        When I test the function "GenerateMD5Hash"
        And I provide the following input "MD5 \#Hash Input wïth Specia1 (h@ract3r$!"
        Then I expect the function to succeed
        And have the following result "a175fd87223b1b548c72f5d37a9984c1"

    Scenario: Test CalculateDewPoint function
        Given I am testing the "helper" module
        When I test the function "CalculateDewPoint"
        And I provide the following input "25|#|70"
        Then I expect the function to succeed
        And have the following result "19.15"

    Scenario: Test base32_decode function
        Given I am testing the "helper" module
        When I test the function "base32_decode"
        And I provide the following input "JV4VG5LQMVZFGZLDOJSXIMJSGM2DKNRX"
        Then I expect the function to succeed
        And have the following result "MySuperSecret1234567"

    Scenario: Test base32_decode padding
        Given I am testing the "helper" module
        When I test the function "base32_decode"
        And I provide the following input "JV4VG5LQMVZFGZLDOJSXIMJSGM2A===="
        Then I expect the function to succeed
        And have the following result "MySuperSecret1234"

    Scenario: Test base32_decode shorter
        Given I am testing the "helper" module
        When I test the function "base32_decode"
        And I provide the following input "GBGUOIKJORLUA4TLOM======"
        Then I expect the function to succeed
        And have the following result "0MG!ItW@rks"

    Scenario: Test base32_decode failure
        Given I am testing the "helper" module
        When I test the function "base32_decode"
        And I provide the following input "THIS!sN0TB@se32Input"
        Then I expect the function to fail

    Scenario: Test base32_encode function
        Given I am testing the "helper" module
        When I test the function "base32_encode"
        And I provide the following input "MySuperSecret1234567"
        Then I expect the function to succeed
        And have the following result "JV4VG5LQMVZFGZLDOJSXIMJSGM2DKNRX"
