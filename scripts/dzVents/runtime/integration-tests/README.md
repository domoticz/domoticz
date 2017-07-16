In order to run the integration tests from the command line you have to

* Install lua 5.2
* Install luarocks
* Install busted: `luarocks install busted`
* Install lodash: `luarocks install lodash`

Make sure you have Domoticz running with an empty database from the commandline so you can see the log files.

DO NOT RUN THIS SCRIPT ON YOUR PRODUCTION DATABASE AS IT WILL DELETE EVERYTHING IN YOUR DATABASE!!!!!

Then go to the test folder and run the tests:

```
cd runtime/integration-tests
busted *
```

When all tests pass you should see this in the logs:

```
dzVents: Info:  Results stage 2: SUCCEEDED
```
