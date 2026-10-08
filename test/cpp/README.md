# AsyncSerial startup regression (Linux)

This standalone regression compiles the production serial implementation and uses
a real PTY. It delays return from successful pthread creation while the actual worker
runs, exposing the valid ordering where doRead executes before open returns. It
checks received input for both open methods. It also injects one pthread creation
failure, checks rollback/error state, then retries and verifies input. Application
logging and the sleep helper are stubbed; production serial I/O and scheduling run.
Pass `baud` to test only openOnlyBaud or `full` to test only open.

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
