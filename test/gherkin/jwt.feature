Feature: JSON Web Tokens (JWT) routines
    Domoticz has a set of Helper routines which can be found in main/helper.cpp
    these helper functions provide all kind of support to make it easier for developers
    but therefor these functions have to stay reliable and predictable

    Background:
        Given Command domoticztester is available
        And can be executed on the commandline

    Scenario: Test JWT decode function
        Given I am testing the "jwt" module
        When I test the function "jwt_decode"
        And I provide the following input "ThisIsAnInvalidJWT"
        Then I expect the function to fail

    Scenario: Test JWT decode function 2
        Given I am testing the "jwt" module
        When I test the function "jwt_decode"
        And I provide the following input "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"
        Then I expect the function to fail

    Scenario: Test JWT decode function 3
        Given I am testing the "jwt" module
        When I test the function "jwt_decode"
        And I provide the following input "eyJhbGciOiJIUzI1NiIsImtpZCI6IjIwMDAzIiwidHlwIjoiSldUIn0.eyJhdWQiOiJhcGl0ZXN0YXBwIiwiZXhwIjoxNzM0MjExNTc2LCJpYXQiOjE3MzQyMDc5NzYsImlzcyI6Imh0dHBzOi8vZG9tb3RpY3oubG9jYWw6ODQ0My8iLCJqdGkiOiI4NGVmOTJlOS0zZjRmLTRmMzItODg5MS01NjkzN2IxNmRiYzYiLCJuYW1lIjoiYXBpdGVzdCIsIm5iZiI6MTczNDIwNzk3NiwicHJlZmVycmVkX3VzZXJuYW1lIjoiYXBpdGVzdCIsInJvbGVzIjpbIjEiXSwic3ViIjoiYXBpdGVzdCJ9.5PhCREBj5ctE8TgEhLSgn9U_d_R_rU8VRZSp52wvqjY"
        Then I expect the function to succeed
        And have the following result "apitestapp"
