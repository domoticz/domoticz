# AsyncSerial startup regression (Linux)

This standalone regression compiles the production serial implementation and uses
a real PTY. It holds return from successful pthread creation until the actual worker
has delivered PTY input, using a bounded condition-variable wait rather than a delay.
Both open methods must deliver the callback before open returns. It also injects one
pthread creation failure after a separate writer has queued data, checks rollback/error
state, then retries and verifies both queued output and input. Application
logging and the sleep helper are stubbed; production serial I/O and scheduling run.
Pass `baud` to test only openOnlyBaud or `full` to test only open.

Assertions must remain enabled (do not compile with NDEBUG).
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
