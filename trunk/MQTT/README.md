# Eclipse Paho MQTT C client


This repository contains the source code for the [Eclipse Paho](http://eclipse.org/paho) MQTT C client library. 

This code builds libraries which enable applications to connect to an [MQTT](http://mqtt.org) broker to publish messages, and to subscribe to topics and receive published messages.

Both synchronous and asynchronous modes of operation are supported.

## Build requirements / compilation

The build process requires GNU Make and gcc, and also requires OpenSSL libraries and includes to be available. There are make rules for a number of Linux "flavors" including ARM and s390, OS X, AIX and Solaris.

The documentation requires doxygen and optionally graphviz. 

Before compiling, set some environment variables (or pass these values to the make command directly) in order to configure library locations and other options.

Specify the location of OpenSSL using `SSL_DIR`

e.g. using [homebrew](http://mxcl.github.com/homebrew/) on OS X:

`export SSL_DIR=/usr/local/Cellar/openssl/1.0.1e`

Specify where the source files are in relation to where the make command is being executed.

``export MQTTCLIENT_DIR=../src``

To build the samples, enable the option (`BUILD_SAMPLES`), and specify the location of the code:

```
export BUILD_SAMPLES=YES
export SAMPLES_DIR=../src/samples
```

One more useful environment variable is ``TARGET_PLATFORM``. This provides for cross-compilation. Currently the only recognised value is "Arm" - for instance to cross-compile a Linux ARM version of the libraries:

``export TARGET_PLATFORM=Arm``

## Libraries

The Paho C client comprises four shared libraries:

 * libmqttv3a.so - asynchronous
 * libmqttv3as.so - asynchronous with SSL
 * libmqttv3c.so - "classic" / synchronous
 * libmqttv3cs.so - "classic" / synchronous with SSL

## Usage and API

Detailed API documentation is available by building the Doxygen docs in the  ``doc`` directory. A [snapshot is also available online](http://www.eclipse.org/paho/files/mqttdoc/Cclient/index.html).

Samples are available in the Doxygen docs and also in ``src/samples`` for reference.

Note that using the C headers from a C++ program requires the following declaration as part of the C++ code:

```
    extern "C" {
    #include "MQTTClient.h"
    #include "MQTTClientPersistence.h"
    }
```

## Runtime tracing

A number of environment variables control runtime tracing of the C library. 

Tracing is switched on using ``MQTT_C_CLIENT_TRACE`` (a value of ON traces to stdout, any other value should specify a file to trace to). 

The verbosity of the output is controlled using the  ``MQTT_C_CLIENT_TRACE_LEVEL`` environment variable - valid values are ERROR, PROTOCOL, MINIMUM, MEDIUM and MAXIMUM (from least to most verbose).

The variable ``MQTT_C_CLIENT_TRACE_MAX_LINES`` limits the number of lines of trace that are output.

```
export MQTT_C_CLIENT_TRACE=ON
export MQTT_C_CLIENT_TRACE_LEVEL=PROTOCOL
```

## Reporting bugs

Please report bugs under the "MQTT-C" Component in [Eclipse Bugzilla](http://bugs.eclipse.org/bugs/) for the Paho Technology project.

## More information

Discussion of the Paho clients takes place on the [Eclipse paho-dev mailing list](https://dev.eclipse.org/mailman/listinfo/paho-dev).

General questions about the MQTT protocol are discussed in the [MQTT Google Group](https://groups.google.com/forum/?hl=en-US&fromgroups#!forum/mqtt).

There is much more information available via the [MQTT community site](http://mqtt.org).
