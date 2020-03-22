In order to run the integration tests from the command line you have to


## Install Lua5.3 , luarocks-3.2.1, nodejs and npm on da Pi 

# sudo apt remove -y lua5.1 liblua5.1-dev lua5.2 liblua5.2-dev lua5.3 liblua5.3-dev 
# sudo apt install lua5.3 liblua5.3-dev 

# sudo wget https://luarocks.org/releases/luarocks-3.2.1.tar.gz
# sudo tar zxpfv luarocks-3.2.1.tar.gz
# cd luarocks-3.2.1
# ./configure
# make 
# sudo make install

# luarocks install lodash
# luarocks install luasocket
# luarocks install busted
# luarocks install luacov

# sudo apt install nodejs npm

# cd <domoticz testDir>/dzVents/runtime/integration-tests
# npm install
#

#Warning!!
DO NOT RUN THIS SCRIPT ON YOUR PRODUCTION DATABASE AS IT WILL DELETE EVERYTHING IN YOUR DATABASE!!!!!

Then:
 * Make sure you have Domoticz running with an empty database from the commandline so you can see the log files.
 * Start the node http server: in `<domoticz testDir>/dzVents/runtime/integration-tests` run `npm start`.

 So at this point you have Domoticz running and a test nodejs http server.


Then go to the test folder and run the tests:

```
cd runtime/integration-tests
busted test*
```

When all tests pass you should see this in the logs:
```
dzVents: Info:  Results stage 2: SUCCEEDED
```
Also check the domoticz log. Some errors are to be expected

for Raspberry Debian buster like systems a bash script is available: '/runtime/misc/testdzVents.sh' this script will execute all tests and integration tests. and
present the outcome to your screen.

========================= V4.11426 ================+========================== Tests ============================+
| time |           test-script                     | expected | tests | result | successful | failed |  seconds  |
===================================================+=============================================================+
00:00:02  Device                                        1        113      OK            113         0      1.0094
00:00:03  Domoticz                                      1         70      OK             70         0      0.9387
00:00:04  EventHelpers                                  1         32      OK             32         0      0.7769
00:00:05  EventHelpersStorage                           1         50      OK             50         0      0.7952
00:00:06  HTTPResponse                                  1          6      OK              6         0      0.0613
00:00:06  ScriptdzVentsDispatching                      1          2      OK              2         0      0.1862
00:00:07  TimedCommand                                  1         42      OK             42         0      0.3143
00:00:07  Time                                          3        326      OK            326         0      2.2273
00:00:10  Utils                                         1         17      OK             17         0      0.0970
00:00:10  Variable                                      1         15      OK             15         0      0.0923
00:00:10  ContactDoorLockInvertedSwitch                 3          2      OK              2         0      3.2331
00:00:14  DelayedVariableScene                          8          2      OK              2         0      7.2075
00:00:21  EventState                                   19          2      OK              2         0     18.2396
00:00:40  Integration                                  33        217      OK            217         0     32.6037
00:01:12  SelectorSwitch                               10          2      OK              2         0      9.6688


Total tests: 898
Finished without erors after 00:01:22
Terminated



