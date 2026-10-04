# AsyncSerial startup regression (Linux)

This standalone regression compiles the production serial implementation and uses
a real PTY. It injects one pthread creation failure through the executable's symbol
interposition, checks rollback/error state for both open methods, then retries and
verifies actual input. Application logging and the sleep helper are stubbed.

Requires a C++17 compiler, Boost Thread/System and JsonCpp headers. Run from the
repository root, placing outputs outside the checkout:

```sh
g++ -std=c++17 -ffunction-sections -fdata-sections -I main -I /usr/include/jsoncpp \
  -c hardware/ASyncSerial.cpp -o /tmp/asyncserial-startup.o
g++ -std=c++17 -I main -I /usr/include/jsoncpp -rdynamic -Wl,--gc-sections \
  test/cpp/asyncserial_startup_test.cpp /tmp/asyncserial-startup.o \
  -lboost_thread -lboost_system -lpthread -lutil -ldl -o /tmp/asyncserial-startup-test
timeout 10 /tmp/asyncserial-startup-test
```

This is not physical-device or full Domoticz integration validation.
