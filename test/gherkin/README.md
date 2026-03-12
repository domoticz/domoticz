# BDD and Gherkin style testing

Using the Python3 pytest and pytest-bdd packages, some testing of Domoticz has been automated.

## GOAL

Prevent _regressions_ and maybe even do __Behavior Driven Development__ where _first_ the _intended behavior_ gets described and _second_ the test is used to validate it the implemention actually works as intended.

To describe the _intended behavior_ the test are written in [_Gherkin syntax_](https://cucumber.io/docs/gherkin/) (Given, when, then)

## How to install and run

The following Python packages have to be installed:

```bash
sudo apt install python3-pytest python3-pytest-bdd
```

## Run the BDD tests

To run the available BDD tests from the available _feature files_ (from the Domoticz base directory):

```bash
pytest-3 -rA --tb=no test/gherkin/
```

The output will be something like the following:

```text
=================================================================================================== test session starts ===================================================================================================
platform linux -- Python 3.8.10, pytest-4.6.9, py-1.8.1, pluggy-0.13.0
rootdir: /home/vagrant/domoticz
plugins: bdd-3.2.1
collected 3 items                                                                                                                                                                                                                             

test/gherkin/test_webserver.py ...                                                                                                                                                                                  [100%]

================================================================================================= short test summary info =================================================================================================
PASSED test/gherkin/test_webserver.py::test_compressedout
PASSED test/gherkin/test_webserver.py::test_uncompressedout
PASSED test/gherkin/test_webserver.py::test_decompressedout
================================================================================================ 3 passed in 0.14 seconds =================================================================================================
```

You have to make sure that domoticz is running already (use `./domoticz -www 8080 -sslwww 0`)

### Prepare some testfiles

Some tests need specific testfiles to be present in the Domoticz environment. So run the following:

```bash
ln -s ../test/gherkin/resources/testwebcontent www/test
```

It will add a symbolic link to the Domoticz www directory called test pointing to some _test web content_ used by the tests for validation purposes.

NOTE: This symbolic link can be removed after testing ofcourse.

### Run everything in one go

You can run the full functional testsuite by executing the following (bash) script:

```bash
./test/runtests.sh
```
