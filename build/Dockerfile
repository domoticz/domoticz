FROM debian:buster-slim
MAINTAINER Mark Ruys <mark@paracas.nl>

# Its recommended that you use the latest Boost libraries
ENV BOOST_VERSION=1.75.0
ENV BOOST_VERSION_=1_75_0

# We need CMake version 3.16.0 or higher, so build it from source
ENV CMAKE_VERSION=3.19.3

RUN apt-get update -qq
RUN apt-get install -y -qq git make gcc g++ rsync libxml2-utils libxml-simple-perl
RUN apt-get install -y -qq libssl-dev libcurl4-gnutls-dev libusb-dev python3-dev zlib1g-dev libcereal-dev liblua5.3-dev uthash-dev

WORKDIR /root

ADD https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz .
RUN tar xzf cmake-${CMAKE_VERSION}.tar.gz
RUN cd cmake-${CMAKE_VERSION} && ./bootstrap
RUN cd cmake-${CMAKE_VERSION} && make
RUN cd cmake-${CMAKE_VERSION} && make install
RUN rm -fr cmake-${CMAKE_VERSION} cmake-${CMAKE_VERSION}.tar.gz

ADD https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_}.tar.gz .
RUN tar xfz boost_${BOOST_VERSION_}.tar.gz
RUN cd boost_${BOOST_VERSION_} && ./bootstrap.sh
RUN cd boost_${BOOST_VERSION_} && ./b2 stage threading=multi link=static --with-thread --with-system
RUN cd boost_${BOOST_VERSION_} && ./b2 install threading=multi link=static --with-thread --with-system
RUN rm -fr boost_${BOOST_VERSION_} boost_${BOOST_VERSION_}.tar.gz 

RUN rm -fr /var/lib/apt/lists/*

CMD ["/bin/bash"]
