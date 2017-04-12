In order to run the tests from the command line you have to

* Install lua 5.2
* Install luarocks 
* Install busted: `luarocks install busted`
* Install luacov: `luarocks install luacov`
* Install lodash: `luarocks install lodash`

Then go to the test folder and run the tests:

```
cd dzVents/tests
busted *
```

or if you want coverage info

```
cd dzVents/tests
busted --coverage *
luacov
```

Then open `luacov.report.out`