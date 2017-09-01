In order to run the integration tests from the command line you have to

* Install lua 5.2
* Install luarocks (for 5.2 !! Google on how to make this work! or see below)
* Install busted: `luarocks install busted`
* Install lodash: `luarocks install lodash`
* Install luasocket: `luarocks install luasocket`

Make sure you have Domoticz running with an empty database from the commandline so you can see the log files.

DO NOT RUN THIS SCRIPT ON YOUR PRODUCTION DATABASE AS IT WILL DELETE EVERYTHING IN YOUR DATABASE!!!!!

Then go to the test folder and run the tests:

```
cd runtime/integration-tests
busted testIntegration.lua
```

When all tests pass you should see this in the logs:

```
dzVents: Info:  Results stage 2: SUCCEEDED
```


## luarocks 5.2 on da Pi

* sudo apt-get install liblua5.2-dev
* sudo apt-get install lua5.2
* git clone https://github.com/luarocks/luarocks
* cd luarocks
* git checkout tags/v2.4.2
* ./configure --lua-version=5.2 --versioned-rocks-dir
* make build
* sudo make install
* sudo luarocks-5.2 install busted
* sudo luarocks-5.2 install lodash
* sudo luarocks-5.2 install luasocket