In order to run the tests from the command line you have to

* Install lua 5.3
* Install luarocks (see below)
* Install busted: `luarocks install busted`
* Install luacov: `luarocks install luacov`
* Install lodash: `luarocks install lodash`

Then go to the test folder and run the tests:

```
cd runtime/tests
busted *
```

or if you want coverage info

```
cd runtime/tests
busted --coverage *
luacov
```

Then open `luacov.report.out`



## luarocks 5.3 on da Pi (not completely described yet !! )

* sudo apt-get install liblua5.3-dev lua5.3 luarocks

* sudo luarocks install busted
* sudo luarocks install lodash
* sudo luarocks install luasocket