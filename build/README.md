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

First build OpenZWave:

```shell
$ ./domoticz/build/build open-zwave
```

Next build Domoticz:

```shell
$ ./domoticz/build/build domoticz
```

