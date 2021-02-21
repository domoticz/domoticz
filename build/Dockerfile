FROM debian:buster-slim
MAINTAINER Mark Ruys <mark@paracas.nl>

ARG CMAKE_VERSION
ARG BOOST_VERSION
ARG BOOST_VERSION_

RUN apt-get update -qq \
 && apt-get install -y -qq git make gcc g++ rsync libxml2-utils libxml-simple-perl \
 && apt-get install -y -qq libssl-dev libcurl4-gnutls-dev libusb-dev python3-dev zlib1g-dev libcereal-dev liblua5.3-dev uthash-dev \
 && rm -fr /var/lib/apt/lists/*

WORKDIR /root

ADD https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz .
RUN tar xzf cmake-${CMAKE_VERSION}.tar.gz \
 && cd cmake-${CMAKE_VERSION} \
 && ./bootstrap \
 && make \
 && make install \
 && cd .. \
 && rm -fr cmake-${CMAKE_VERSION} cmake-${CMAKE_VERSION}.tar.gz
  
ADD https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_}.tar.gz .
RUN tar xfz boost_${BOOST_VERSION_}.tar.gz \
 && cd boost_${BOOST_VERSION_} \
 && ./bootstrap.sh \
 && ./b2 stage threading=multi link=static --with-thread --with-system \
 && ./b2 install threading=multi link=static --with-thread --with-system \
 && cd .. \
 && rm -fr boost_${BOOST_VERSION_} boost_${BOOST_VERSION_}.tar.gz 

EXPOSE 8080

ENTRYPOINT [ "/root/build-helper.sh" ]
