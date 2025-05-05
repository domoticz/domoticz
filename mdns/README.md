# mDNS-Cpp

This library provides a cross-platform mDNS and DNS-DS library in C++.

__NOTES__:

- This is a fork from [github.com/talaviram/mdns_cpp](https://github.com/talaviram/mdns_cpp) with small changes.
- The fork from [github.com/talaviram/mdns_cpp](https://github.com/talaviram/mdns_cpp) is in itself already a fork from [github.com/gocarlos/mdns_cpp](https://github.com/gocarlos/mdns_cpp) containing several improvements and fixes to make it compatible again with the updates of the original C library from [github.com/mjansson/mdns](https://github.com/mjansson/mdns).

Currently this library is only a C++ wrapper around Mattias Jansson ([@maniccoder](https://twitter.com/maniccoder)) [mdns.h](https://github.com/mjansson/mdns) file -> all feature credits go to him. His work is licensed with the [The Unlicense](https://github.com/mjansson/mdns/blob/master/LICENSE) license.

why does this exist:

* better integration in C++ projects
* better usage of modern C++ features
* higher level API
* pass logger function to library to use users logger

## Features

The library does DNS-SD discovery and service as well as one-shot single record mDNS query and response.

## Build

```bash
mkdir build
cd build
cmake ..
make -j
make install
```

## Usage

you can either install the library, include the library as subdirectory or use conan: `mdns_cpp/0.1.0@gocarlos/testing`

```cmake

# add subdirectory
add_subdirectory(vendor/mdns_cpp)
# or install / use conan
find_package(mdns_cpp)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE mdns_cpp::mdns_cpp)
```

See the example folder for the different examples:

### Service

```c++
#include <thread>
#include "mdns_cpp/mdns.hpp"

int main() {
  mdns_cpp::mDNS mdns;
  mdns.setServiceHostname("AirForce1");
  mdns.startService();
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
```

To listen for incoming DNS-SD requests and mDNS queries the socket can be opened/setup on the default interface by passing 0 as socket address in the call to the socket open/setup functions (the socket will receive data from all network interfaces). Then call `mdns_socket_listen` either on notification of incoming data, or by setting blocking mode and calling `mdns_socket_listen` to block until data is available and parsed.

The entry type passed to the callback will be `MDNS_ENTRYTYPE_QUESTION` and record type `MDNS_RECORDTYPE_PTR`. Use the `mdns_record_parse_ptr` function to get the name string of the service record that was asked for.

If service record name is `_services._dns-sd._udp.local.` you should use `mdns_discovery_answer` to send the records of the services you provide (DNS-SD).

If the service record name is a service you provide, use `mdns_query_answer` to send the service details back in response to the query.

See the test executable implementation for more details on how to handle the parameters to the given functions.

### Discovery

```c++
#include <thread>
#include "mdns_cpp/mdns.hpp"

int main() {
  mdns_cpp::mDNS mdns;
  mdns.executeDiscovery();
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
```

To send a DNS-SD service discovery request use `mdns_discovery_send`. This will send a single multicast packet (single PTR question record for `_services._dns-sd._udp.local.`) requesting a unicast response.

To read discovery responses use `mdns_discovery_recv`. All records received since last call will be piped to the callback supplied in the function call. The entry type will be one of `MDNS_ENTRYTYPE_ANSWER`, `MDNS_ENTRYTYPE_AUTHORITY` and `MDNS_ENTRYTYPE_ADDITIONAL`.

### Query

```c++
#include <thread>
#include "mdns_cpp/mdns.hpp"

int main() {
  mdns_cpp::mDNS mdns;
  const std::string service = "_http._tcp.local.";
  mdns.executeQuery(service);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
```

To send a one-shot mDNS query for a single record use `mdns_query_send`. This will send a single multicast packet for the given record (single PTR question record, for example `_http._tcp.local.`). You can optionally pass in a query ID for the query for later filtering of responses (even though this is discouraged by the RFC), or pass 0 to be fully compliant. The function returns the query ID associated with this query, which if non-zero can be used to filter responses in `mdns_query_recv`. If the socket is bound to port 5353 a multicast response is requested, otherwise a unicast response.

To read query responses use `mdns_query_recv`. All records received since last call will be piped to the callback supplied in the function call. If `query_id` parameter is non-zero the function will filter out any response with a query ID that does not match the given query ID. The entry type will be one of `MDNS_ENTRYTYPE_ANSWER`, `MDNS_ENTRYTYPE_AUTHORITY` and `MDNS_ENTRYTYPE_ADDITIONAL`.
