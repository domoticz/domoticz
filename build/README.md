# Building Domoticz

This builder still a bit work on progress. It uses Docker to compile Open ZWave and Domoticz. To speed up the build process on macos and Windows it uses a persistent volume for the actual compilation. Currently it supports only builds to amd64 targets. Using `docker buildx` it should be possible to add architectures like Raspberry Pi.



## Installation

Create the Docker image:

```shell
~/domoticz/build$ docker build -t markr/domoticz-build .
```

This can take some time as it builds booster and cmake from source. Furthermore it expects that you cloned both Open ZWave as Domoticz besides one other:

```shell
$ git clone https://github.com/OpenZWave/open-zwave
$ git clone https://github.com/domoticz/domoticz.git
```



## Usage

First build Open ZWave:

```shell
$ ./domoticz/build/build open-zwave
```

Next build Domoticz:

```shell
$ ./domoticz/build/build domoticz
```

