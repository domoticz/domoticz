# Building Domoticz

This builder still a bit work on progress. It uses Docker to compile OpenZWave and Domoticz. To speed up the build process on macos and Windows it uses a persistent volume for the actual compilation. Currently it supports only builds to amd64 targets. Using `docker buildx` it should be possible to add architectures like Raspberry Pi.



## Installation

The scripts expect that you've cloned both OpenZWave as Domoticz besides one other:

```shell
$ git clone https://github.com/OpenZWave/open-zwave
$ git clone https://github.com/domoticz/domoticz.git
```

Then create the Docker image:

```shell
$ ./domoticz/build/build init
```

This can take some time as it builds boost and cmake from source. 

## Usage

```shell
$ ./domoticz/build/build
Usage: ./domoticz/build/build init ]
Usage: ./domoticz/build/build open-zwave [ clean | check | test | updateIndexDefines ]
Usage: ./domoticz/build/build domoticz [ clean | cmake | run ] [ arguments ]
Usage: ./domoticz/build/build shell ]
```

### Build

First build OpenZWave:

```shell
$ ./domoticz/build/build open-zwave
```

Next build Domoticz:

```shell
$ ./domoticz/build/build domoticz cmake
$ ./domoticz/build/build domoticz
```

### CMake

You can configure CMake using an `.env` file with contents like:

```ini
USE_BUILTIN_JSONCPP=YES
USE_BUILTIN_MINIZIP=YES
USE_BUILTIN_MQTT=YES
USE_BUILTIN_SQLITE=YES

USE_PYTHON=YES
INCLUDE_LINUX_I2C=YES
INCLUDE_SPI=YES
WITH_LIBUSB=YES

USE_LUA_STATIC=YES
USE_OPENSSL_STATIC=NO
USE_STATIC_OPENZWAVE=YES

USE_PRECOMPILED_HEADER=YES
GIT_SUBMODULE=ON
```

Note that `USE_STATIC_BOOST` is not supported, it's always enabled.

