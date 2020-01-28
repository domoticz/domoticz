# Integration tests and unit tests

## Features

- based on Google Test / Google Mock
- uses BleBox devices integration as an example
- uses in-memory SQLite3 database
- end-to-end tests hardware/devices directly via domoticz mainworker
- convenience helpers for checking database state
- separate from main build

## Good for ...

- testing C++ integrations
- testing communication between C++ integrations and Domoticz main worker
- testing database migrations
- mocking/stubbing HTTP calls
- various unit tests

## Simple way to build and run:

## Initial Setup

Checkout Google Test / Google Mock sources

$ git submodule init --recursive

### Recommended options (for faster and leaner builds mostly)

$ mkdir integration_tests
$ cd integration_tests && cmake -D CMAKE_BUILD_TYPE=Debug -D USE_GPIO=n -D INCLUDE_LINUX_I2C=n -D INCLUDE_SPI=n -D USE_OPENSSL_STATIC=n -D USE_PYTHON=n -D USE_STATIC_BOOST=n -D USE_STATIC_LIBSTDCXX=n -D WITH_LIBUSB=n ../tests && make testDomoticz && ./testDomoticz
