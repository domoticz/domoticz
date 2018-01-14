# MQTT client/server for C++14 based on Boost.Asio

Version 1.0.9 [![Build Status](https://travis-ci.org/redboltz/mqtt_cpp.svg?branch=master)](https://travis-ci.org/redboltz/mqtt_cpp) [![Build status](https://ci.appveyor.com/api/projects/status/21a267hd34s0kqu5/branch/master?svg=true)](https://ci.appveyor.com/project/redboltz/mqtt-client-cpp/branch/master)

## Overview

mqtt_cpp is a header only library. It requires C++14 and the Boost Libraries 1.57.0 or later (See [#33](https://github.com/redboltz/mqtt_cpp/issues/33)). The only feature that mqtt_cpp uses C++14 is binary literals. e.g.) 0b00001111

You may be able to use mqtt_cpp on C++11 with binary literal extension environment such as g++ and clang++.

Add mqtt_cpp/include to your include path. Then, include `mqtt_cpp.hpp` and/or `mqtt_server_cpp.hpp` as follows:

For clients:
```c++
#include <mqtt_client_cpp.hpp>
```
For servers:
```c++
#include <mqtt_server_cpp.hpp>
```

You can compile your program as follows:

```
g++ -std=c++14 -Ipath_to_mqtt_cpp/include -DMQTT_NO_TLS no_tls.cpp -lboost_system -lpthread
```

```
g++ -std=c++14 -Ipath_to_mqtt_cpp/include tls.cpp -lboost_system -lssl -lcrypto -lpthread
```

## WebSocket support

If you want to use MQTT on WebSocket, you need to define `MQTT_USE_WS` macro. mqtt_cpp uses https://github.com/boostorg/beast for WebSocket communication and it requires `boost::string_view`, so the boost library need to support `boost::string_view`.

## Example

* NO TLS
  * Client
    * TCP
      * [example/no_tls_client.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/no_tls_client.cpp)
    * WebSocket
      * [example/no_tls_ws_client.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/no_tls_ws_client.cpp)
  * Server
    * TCP
      * [example/no_tls_server.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/no_tls_server.cpp)
    * WebSocket
      * [example/no_tls_ws_server.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/no_tls_ws_server.cpp)
  * Client and Server
    * TCP
      * [example/no_tls_both.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/no_tls_both.cpp)
    * WebSocket
      * [example/no_tls_ws_both.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/no_tls_ws_both.cpp)
* TLS
  * Client
    * TCP
      * [example/tls_client.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/tls_client.cpp)
    * WebSocket
      * [example/tls_ws_client.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/tls_ws_client.cpp)
  * Server
    * TCP
      * [example/tls_server.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/tls_server.cpp)
    * WebSocket
      * [example/tls_ws_server.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/tls_ws_server.cpp)
  * Client and Server
    * TCP
      * [example/tls_both.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/tls_both.cpp)
    * WebSocket
      * [example/tls_ws_both.cpp](https://github.com/redboltz/mqtt_cpp/blob/master/example/tls_ws_both.cpp)

## Test

You can build tests and examples as follows:


At mqtt_cpp directory

```
mkdir build
cd build
cmake ..
make
make test
```

In order to build tests, you need to prepare the Boost Libraries 1.59.0.

## Documents
http://redboltz.github.io/contents/mqtt/index.html

You can create html documents using doxygen.

```
make doxygen
```

## License

mqtt_cpp is licensed under the Boost Software License, Version 1.0. See
the [`LICENSE_1_0.txt`](./LICENSE_1_0.txt) file for details.
