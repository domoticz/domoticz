FROM debian:buster-slim

LABEL maintainer="Mark Ruys"

ARG CMAKE_VERSION
ARG BOOST_VERSION

RUN apt-get update -qq \
 && apt-get install -y -qq \
      g++ \
      gcc \
      git \
      libcereal-dev \
      libcurl4-gnutls-dev \
      libjsoncpp-dev \
      liblua5.3-dev \
      libminizip-dev \
      libmosquitto-dev \
      libsqlite3-dev \
      libssl-dev \
      libusb-dev \
      libxml-simple-perl \
      libxml2-utils \
      make \
      python3-dev \
      rsync \
      uthash-dev \
      wget \
      zlib1g-dev \
 && rm -fr /var/lib/apt/lists/*

WORKDIR /opt

ADD https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz .
RUN tar xzf cmake-${CMAKE_VERSION}.tar.gz \
 && cd cmake-${CMAKE_VERSION} \
 && ./bootstrap \
 && make \
 && make install \
 && cd .. \
 && rm -fr cmake-${CMAKE_VERSION} cmake-${CMAKE_VERSION}.tar.gz

RUN VERSION_=$(echo ${BOOST_VERSION} | tr . _) \
 && wget -nv -O - https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${VERSION_}.tar.gz \
  | tar xfz - \
 && cd boost_${VERSION_} \
 && ./bootstrap.sh \
 && ./b2 stage threading=multi link=static --with-thread --with-system \
 && ./b2 install threading=multi link=static --with-thread --with-system \
 && cd .. \
 && rm -fr boost_${VERSION_}

COPY build-helper.sh .

EXPOSE 8080
EXPOSE 443

ENTRYPOINT [ "./build-helper.sh" ]
