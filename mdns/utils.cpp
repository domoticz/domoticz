#include "stdafx.h"
#include "utils.hpp"

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <errno.h>
#include <stdio.h>

#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock.h>
#else
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "macros.hpp"

namespace mdns_cpp {

std::string getHostName() {
  const char *hostname = "dummy-host";

#ifdef _WIN32
  WORD versionWanted = MAKEWORD(1, 1);
  WSADATA wsaData;
  if (WSAStartup(versionWanted, &wsaData)) {
    const auto msg = "Error: Failed to initialize WinSock";
    MDNS_LOG << msg << "\n";
    throw std::runtime_error(msg);
  }

  char hostname_buffer[256];
  DWORD hostname_size = (DWORD)sizeof(hostname_buffer);
  if (GetComputerNameA(hostname_buffer, &hostname_size)) {
    hostname = hostname_buffer;
  }

#else

  char hostname_buffer[256];
  const size_t hostname_size = sizeof(hostname_buffer);
  if (gethostname(hostname_buffer, hostname_size) == 0) {
    hostname = hostname_buffer;
  }

#endif

  return hostname;
}

std::string ipv4AddressToString(char *buffer, size_t capacity, const sockaddr_in *addr, size_t addrlen) {
  char host[NI_MAXHOST] = {0};
  char service[NI_MAXSERV] = {0};
  const int ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)addrlen, host, NI_MAXHOST, service, NI_MAXSERV,
                              NI_NUMERICSERV | NI_NUMERICHOST);
  int len = 0;
  if (ret == 0) {
    if (addr->sin_port != 0) {
      len = snprintf(buffer, capacity, "%s:%s", host, service);
    } else {
      len = snprintf(buffer, capacity, "%s", host);
    }
  }
  if (len >= (int)capacity) {
    len = (int)capacity - 1;
  }

  return std::string(buffer, len);
}

std::string ipv6AddressToString(char *buffer, size_t capacity, const sockaddr_in6 *addr, size_t addrlen) {
  char host[NI_MAXHOST] = {0};
  char service[NI_MAXSERV] = {0};
  const int ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)addrlen, host, NI_MAXHOST, service, NI_MAXSERV,
                              NI_NUMERICSERV | NI_NUMERICHOST);
  int len = 0;
  if (ret == 0) {
    if (addr->sin6_port != 0) {
      {
        len = snprintf(buffer, capacity, "[%s]:%s", host, service);
      }
    } else {
      len = snprintf(buffer, capacity, "%s", host);
    }
  }
  if (len >= (int)capacity) {
    len = (int)capacity - 1;
  }

  return std::string(buffer, len);
}

std::string ipAddressToString(char *buffer, size_t capacity, const sockaddr *addr, size_t addrlen) {
  if (addr->sa_family == AF_INET6) {
    return ipv6AddressToString(buffer, capacity, (const struct sockaddr_in6 *)addr, addrlen);
  }
  return ipv4AddressToString(buffer, capacity, (const struct sockaddr_in *)addr, addrlen);
}

}  // namespace mdns_cpp
