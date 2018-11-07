In order to run the tests from the command line you have to

* Install lua 5.2
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