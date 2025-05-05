#pragma once

#include <string>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace mdns_cpp {

std::string getHostName();

std::string ipv4AddressToString(char *buffer, size_t capacity, const struct sockaddr_in *addr, size_t addrlen);

std::string ipv6AddressToString(char *buffer, size_t capacity, const struct sockaddr_in6 *addr, size_t addrlen);

std::string ipAddressToString(char *buffer, size_t capacity, const struct sockaddr *addr, size_t addrlen);

}  // namespace mdns_cpp
