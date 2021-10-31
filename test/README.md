# Domoticz (automated) testing

This folder is used for everything around (automated) testing of Domoticz.

## Functional testing

Some functional testing is done by using BDD/Gherkin style tests. See the [README.md](gherkin/README.md) in the _gherkin_ folder for more information.

## Unit testing

A start is made with the 'www' part of Domoticz. The 'www-test' folder contains tests for components in the _www_-folder. As the components in this folder are written in JavaScript, so are the tests. See the [README.md](www-test/README.md) in the _www-test_ folder for details.

## Test automation

For both Unit testing as Functional testing, there is some test automation using `mocha` and `pytest-3` (using BDD plugin).
