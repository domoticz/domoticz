#!/bin/bash

# go to domoticz home dir
cd ~/domoticz

# create symbolic link for testing
ln -s ../test/gherkin/resources/testwebcontent www/test

# start domoticz
sudo ./domoticz -sslwww 0 -wwwroot www -pidfile /var/run/domoticz.pid -daemon

# run functional tests
pytest-3 -rA --tb=no test/gherkin/

# stop domoticz
sudo kill -s TERM `sudo cat /var/run/domoticz.pid`

# remove symbolic link
rm www/test
