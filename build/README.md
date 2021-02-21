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

This can take some time as it builds Boost and CMake from source. 

## Usage

```shell
$ cd domoticz/build 
$ ./build
Usage:
  ./build init                              # build the Docker image
  ./build clean [-p openzwave | domoticz]   # clean source
  ./build compile [-p openzwave | domoticz] # compile source
  ./build shell                             # run bash inside the container

Only for Domoticz:
  ./build cmake                             # (re)creates Makefiles
  ./build run                               # run Domoticz for testing

Only for OpenZWave:
  ./build check                             # validates XML configuration files
  ./build updateIndexDefines
  ./build test
```

### Build

First generate the Domoticz Makefiles:

```shell
$ ./domoticz/build/build cmake
```

Next build OpenZWave and Domoticz:

```shell
$ ./domoticz/build/build compile
$ ls -l domoticz/domoticz
-rwxr-xr-x  1 markr  staff  17540712 21 feb 23:33 domoticz/domoticz
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

