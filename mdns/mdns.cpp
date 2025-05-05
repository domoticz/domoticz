#include "stdafx.h"
#include "mdns.hpp"

#include <string.h>

#include <iostream>
#include <memory>
#include <thread>

#include "mdns.h"
#include "macros.hpp"

namespace mdns_cpp {

static mdns_record_txt_t txtbuffer[128];

class ServiceRecord {
 public:
  std::string service;
  std::string hostname;
  std::string service_instance;
  std::string hostname_qualified;
  struct sockaddr_in address_ipv4;
  struct sockaddr_in6 address_ipv6;
  uint16_t port;
  mdns_record_t record_ptr;
  mdns_record_t record_srv;
  mdns_record_t record_a;
  mdns_record_t record_aaaa;
  mdns_record_t txt_record[2];
};

mdns_string_t to_mdns_str_ref(const std::string &str_ref) { return {str_ref.c_str(), str_ref.length()}; }

int mDNS::openServiceSockets(int *sockets, int max_sockets) {
  // When receiving, each socket can receive data from all network interfaces
  // Thus we only need to open one socket for each address family
  int num_sockets = 0;

  // Call the client socket function to enumerate and get local addresses,
  // but not open the actual sockets
  openClientSockets(0, 0, 0);

  if (num_sockets < max_sockets) {
    sockaddr_in sock_addr{};
    sock_addr.sin_family = AF_INET;
#ifdef _WIN32
    sock_addr.sin_addr = in4addr_any;
#else
    sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif
    sock_addr.sin_port = htons(MDNS_PORT);
#ifdef __APPLE__
    sock_addr.sin_len = sizeof(struct sockaddr_in);
#endif
    const int sock = mdns_socket_open_ipv4(&sock_addr);
    if (sock >= 0) {
      sockets[num_sockets++] = sock;
    }
  }

  if (num_sockets < max_sockets) {
    sockaddr_in6 sock_addr{};
    sock_addr.sin6_family = AF_INET6;
    sock_addr.sin6_addr = in6addr_any;
    sock_addr.sin6_port = htons(MDNS_PORT);
#ifdef __APPLE__
    sock_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
    int sock = mdns_socket_open_ipv6(&sock_addr);
    if (sock >= 0) sockets[num_sockets++] = sock;
  }

  return num_sockets;
}

// Callback handling questions and answers dump
static int dump_callback(int sock, const struct sockaddr *from, size_t addrlen, mdns_entry_type_t entry,
                         uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void *data,
                         size_t size, size_t name_offset, size_t name_length, size_t record_offset,
                         size_t record_length, void *user_data) {
  char addrbuffer[64]{};
  char namebuffer[256]{};

  const auto fromaddrstr = ipAddressToString(addrbuffer, sizeof(addrbuffer), from, addrlen);
  size_t offset = name_offset;
  mdns_string_t name = mdns_string_extract(data, size, &offset, namebuffer, sizeof(namebuffer));
  const char *record_name = 0;
  if (rtype == MDNS_RECORDTYPE_PTR)
    record_name = "PTR";
  else if (rtype == MDNS_RECORDTYPE_SRV)
    record_name = "SRV";
  else if (rtype == MDNS_RECORDTYPE_A)
    record_name = "A";
  else if (rtype == MDNS_RECORDTYPE_AAAA)
    record_name = "AAAA";
  else if (rtype == MDNS_RECORDTYPE_TXT)
    record_name = "TXT";
  else if (rtype == MDNS_RECORDTYPE_ANY)
    record_name = "ANY";
  else
    record_name = "<UNKNOWN>";
  const char *entry_type = "Question";
  if (entry == MDNS_ENTRYTYPE_ANSWER)
    entry_type = "Answer";
  else if (entry == MDNS_ENTRYTYPE_AUTHORITY)
    entry_type = "Authority";
  else if (entry == MDNS_ENTRYTYPE_ADDITIONAL)
    entry_type = "Additional";
  printf("%.*s: %s %s %.*s rclass 0x%x ttl %u", (int)fromaddrstr.length(), fromaddrstr.c_str(), entry_type,
         record_name, MDNS_STRING_FORMAT(name), (unsigned int)rclass, ttl);
  return 0;
}

int mDNS::openClientSockets(int *sockets, int max_sockets, int port) {
  // When sending, each socket can only send to one network interface
  // Thus we need to open one socket for each interface and address family
  int num_sockets = 0;

#ifdef _WIN32

  IP_ADAPTER_ADDRESSES *adapter_address = nullptr;
  ULONG address_size = 8000;
  unsigned int ret{};
  unsigned int num_retries = 4;
  do {
    adapter_address = (IP_ADAPTER_ADDRESSES *)malloc(address_size);
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST, 0, adapter_address,
                               &address_size);
    if (ret == ERROR_BUFFER_OVERFLOW) {
      free(adapter_address);
      address_size *= 2;
    } else {
      break;
    }
  } while (num_retries-- > 0);

  if (!adapter_address || (ret != NO_ERROR)) {
    free(adapter_address);
    LogMessage() << "Failed to get network adapter addresses";
    return num_sockets;
  }

  int first_ipv4 = 1;
  int first_ipv6 = 1;
  for (PIP_ADAPTER_ADDRESSES adapter = adapter_address; adapter; adapter = adapter->Next) {
    if (adapter->TunnelType == TUNNEL_TYPE_TEREDO) {
      continue;
    }
    if (adapter->OperStatus != IfOperStatusUp) {
      continue;
    }

    for (IP_ADAPTER_UNICAST_ADDRESS *unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
      if (unicast->Address.lpSockaddr->sa_family == AF_INET) {
        struct sockaddr_in *saddr = (struct sockaddr_in *)unicast->Address.lpSockaddr;
        if ((saddr->sin_addr.S_un.S_un_b.s_b1 != 127) || (saddr->sin_addr.S_un.S_un_b.s_b2 != 0) ||
            (saddr->sin_addr.S_un.S_un_b.s_b3 != 0) || (saddr->sin_addr.S_un.S_un_b.s_b4 != 1)) {
          int log_addr = 0;
          if (first_ipv4) {
            service_address_ipv4_ = *saddr;
            first_ipv4 = 0;
            log_addr = 1;
          }

          if (num_sockets < max_sockets) {
            saddr->sin_port = htons((unsigned short)port);
            int sock = mdns_socket_open_ipv4(saddr);
            if (sock >= 0) {
              sockets[num_sockets++] = sock;
              log_addr = 1;
            } else {
              log_addr = 0;
            }
          }
          if (log_addr) {
            char buffer[128];
            const auto addr = ipv4AddressToString(buffer, sizeof(buffer), saddr, sizeof(struct sockaddr_in));
            MDNS_LOG << "Local IPv4 address: " << addr << "";
          }
        }
      } else if (unicast->Address.lpSockaddr->sa_family == AF_INET6) {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)unicast->Address.lpSockaddr;
        // Ignore link-local addresses
        if (saddr->sin6_scope_id) continue;
        static constexpr unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        static constexpr unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
        if ((unicast->DadState == NldsPreferred) && memcmp(saddr->sin6_addr.s6_addr, localhost, 16) &&
            memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16)) {
          int log_addr = 0;
          if (first_ipv6) {
            memcpy(&service_address_ipv6_, &saddr->sin6_addr, sizeof(saddr->sin6_addr));
            first_ipv6 = 0;
            log_addr = 1;
          }

          if (num_sockets < max_sockets) {
            saddr->sin6_port = htons((unsigned short)port);
            int sock = mdns_socket_open_ipv6(saddr);
            if (sock >= 0) {
              sockets[num_sockets++] = sock;
              log_addr = 1;
            } else {
              log_addr = 0;
            }
          }
          if (log_addr) {
            char buffer[128];
            const auto addr = ipv6AddressToString(buffer, sizeof(buffer), saddr, sizeof(struct sockaddr_in6));
            MDNS_LOG << "Local IPv6 address: " << addr << "";
          }
        }
      }
    }
  }

  free(adapter_address);

#else

  struct ifaddrs *ifaddr = nullptr;
  struct ifaddrs *ifa = nullptr;

  if (getifaddrs(&ifaddr) < 0) {
    MDNS_LOG << "Unable to get interface addresses";
  }

  int first_ipv4 = 1;
  int first_ipv6 = 1;
  for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) {
      continue;
    }
    if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_MULTICAST)) continue;
    if ((ifa->ifa_flags & IFF_LOOPBACK) || (ifa->ifa_flags & IFF_POINTOPOINT)) continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in *saddr = (struct sockaddr_in *)ifa->ifa_addr;
      if (saddr->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        int log_addr = 0;
        if (first_ipv4) {
          service_address_ipv4_ = *saddr;
          first_ipv4 = 0;
          log_addr = 1;
        }

        if (num_sockets < max_sockets) {
          saddr->sin_port = htons(port);
          int sock = mdns_socket_open_ipv4(saddr);
          if (sock >= 0) {
            sockets[num_sockets++] = sock;
            log_addr = 1;
          } else {
            log_addr = 0;
          }
        }
        if (log_addr) {
          char buffer[128];
          const auto addr = ipv4AddressToString(buffer, sizeof(buffer), saddr, sizeof(struct sockaddr_in));
          MDNS_LOG << "Local IPv4 address: " << addr << "";
        }
      }
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)ifa->ifa_addr;
      // Ignore link-local addresses
      if (saddr->sin6_scope_id) continue;
      static constexpr unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
      static constexpr unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
      if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) && memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16)) {
        int log_addr = 0;
        if (first_ipv6) {
          service_address_ipv6_ = *saddr;
          first_ipv6 = 0;
          log_addr = 1;
        }

        if (num_sockets < max_sockets) {
          saddr->sin6_port = htons(port);
          int sock = mdns_socket_open_ipv6(saddr);
          if (sock >= 0) {
            sockets[num_sockets++] = sock;
            log_addr = 1;
          } else {
            log_addr = 0;
          }
        }
        if (log_addr) {
          char buffer[128] = {};
          const auto addr = ipv6AddressToString(buffer, sizeof(buffer), saddr, sizeof(struct sockaddr_in6));
          MDNS_LOG << "Local IPv6 address: " << addr << "";
        }
      }
    }
  }

  freeifaddrs(ifaddr);

#endif

  return num_sockets;
}

// Callback handling parsing answers to queries sent
static int query_callback(int sock, const struct sockaddr *from, size_t addrlen, mdns_entry_type_t entry,
                          uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void *data,
                          size_t size, size_t name_offset, size_t name_length, size_t record_offset,
                          size_t record_length, void *user_data) {
  (void)sizeof(sock);
  (void)sizeof(query_id);
  (void)sizeof(name_length);
  (void)sizeof(user_data);

  static char addrbuffer[64]{};
  static char namebuffer[256]{};
  static char entrybuffer[256]{};

  const auto fromaddrstr = ipAddressToString(addrbuffer, sizeof(addrbuffer), from, addrlen);
  const char *entrytype =
      (entry == MDNS_ENTRYTYPE_ANSWER) ? "answer" : ((entry == MDNS_ENTRYTYPE_AUTHORITY) ? "authority" : "additional");
  mdns_string_t entrystr = mdns_string_extract(data, size, &name_offset, entrybuffer, sizeof(entrybuffer));

  const int str_capacity = 1000;
  char str_buffer[str_capacity] = {};

  if (rtype == MDNS_RECORDTYPE_PTR) {
    mdns_string_t namestr =
        mdns_record_parse_ptr(data, size, record_offset, record_length, namebuffer, sizeof(namebuffer));

    snprintf(str_buffer, str_capacity, "%s : %s %.*s PTR %.*s rclass 0x%x ttl %u length %d", fromaddrstr.data(),
             entrytype, MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(namestr), rclass, ttl, (int)record_length);
  } else if (rtype == MDNS_RECORDTYPE_SRV) {
    mdns_record_srv_t srv =
        mdns_record_parse_srv(data, size, record_offset, record_length, namebuffer, sizeof(namebuffer));
    snprintf(str_buffer, str_capacity, "%s : %s %.*s SRV %.*s priority %d weight %d port %d", fromaddrstr.data(),
             entrytype, MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(srv.name), srv.priority, srv.weight, srv.port);
  } else if (rtype == MDNS_RECORDTYPE_A) {
    struct sockaddr_in addr;
    mdns_record_parse_a(data, size, record_offset, record_length, &addr);
    const auto addrstr = ipv4AddressToString(namebuffer, sizeof(namebuffer), &addr, sizeof(addr));
    snprintf(str_buffer, str_capacity, "%s : %s %.*s A %s", fromaddrstr.data(), entrytype,
             MDNS_STRING_FORMAT(entrystr), addrstr.data());
  } else if (rtype == MDNS_RECORDTYPE_AAAA) {
    struct sockaddr_in6 addr;
    mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr);
    const auto addrstr = ipv6AddressToString(namebuffer, sizeof(namebuffer), &addr, sizeof(addr));
    snprintf(str_buffer, str_capacity, "%s : %s %.*s AAAA %s", fromaddrstr.data(), entrytype,
             MDNS_STRING_FORMAT(entrystr), addrstr.data());
  } else if (rtype == MDNS_RECORDTYPE_TXT) {
    size_t parsed = mdns_record_parse_txt(data, size, record_offset, record_length, txtbuffer,
                                          sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
    for (size_t itxt = 0; itxt < parsed; ++itxt) {
      if (txtbuffer[itxt].value.length) {
        snprintf(str_buffer, str_capacity, "%s : %s %.*s TXT %.*s = %.*s", fromaddrstr.data(), entrytype,
                 MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(txtbuffer[itxt].key),
                 MDNS_STRING_FORMAT(txtbuffer[itxt].value));
      } else {
        snprintf(str_buffer, str_capacity, "%s : %s %.*s TXT %.*s", fromaddrstr.data(), entrytype,
                 MDNS_STRING_FORMAT(entrystr), MDNS_STRING_FORMAT(txtbuffer[itxt].key));
      }
    }
  } else {
    snprintf(str_buffer, str_capacity, "%s : %s %.*s type %u rclass 0x%x ttl %u length %d", fromaddrstr.data(),
             entrytype, MDNS_STRING_FORMAT(entrystr), rtype, rclass, ttl, (int)record_length);
  }
  MDNS_LOG << std::string(str_buffer);

  return 0;
}

// Callback handling questions incoming on service sockets
int service_callback(int sock, const struct sockaddr *from, size_t addrlen, mdns_entry_type entry, uint16_t query_id,
                     uint16_t rtype, uint16_t rclass, uint32_t ttl, const void *data, size_t size, size_t name_offset,
                     size_t name_length, size_t record_offset, size_t record_length, void *user_data) {
  (void)sizeof(ttl);

  if (static_cast<int>(entry) != MDNS_ENTRYTYPE_QUESTION) {
    return 0;
  }

  const char dns_sd[] = "_services._dns-sd._udp.local.";
  const ServiceRecord *service_record = (const ServiceRecord *)user_data;

  char addrbuffer[64] = {0};
  char namebuffer[256] = {0};

  const auto fromaddrstr = ipAddressToString(addrbuffer, sizeof(addrbuffer), from, addrlen);
  const mdns_string_t service =
      mdns_record_parse_ptr(data, size, record_offset, record_length, namebuffer, sizeof(namebuffer));
  const size_t service_length = service_record->service.length();
  char sendbuffer[1024] = {0};

  size_t offset = name_offset;
  mdns_string_t name = mdns_string_extract(data, size, &offset, namebuffer, sizeof(namebuffer));

  const char *record_name = 0;
  if (rtype == MDNS_RECORDTYPE_PTR)
    record_name = "PTR";
  else if (rtype == MDNS_RECORDTYPE_SRV)
    record_name = "SRV";
  else if (rtype == MDNS_RECORDTYPE_A)
    record_name = "A";
  else if (rtype == MDNS_RECORDTYPE_AAAA)
    record_name = "AAAA";
  else if (rtype == MDNS_RECORDTYPE_TXT)
    record_name = "TXT";
  else if (rtype == MDNS_RECORDTYPE_ANY)
    record_name = "ANY";
  else
    return 0;
  MDNS_LOG << "Query " << record_name << ":" << name.str << "(" << fromaddrstr << ")";
  const int str_capacity = 1000;
  char str_buffer[str_capacity] = {};
  if ((name.length == (sizeof(dns_sd) - 1)) && (strncmp(name.str, dns_sd, sizeof(dns_sd) - 1) == 0)) {
    if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) {
      // The PTR query was for the DNS-SD domain, send answer with a PTR record for the
      // service name we advertise, typically on the "<_service-name>._tcp.local." format
      // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
      // "<hostname>.<_service-name>._tcp.local."
      mdns_record_t answer;

      answer.name = name;
      answer.type = MDNS_RECORDTYPE_PTR;
      answer.data = { mdns_record_ptr_t{name = to_mdns_str_ref(service_record->service)} };
      answer.rclass = MDNS_CLASS_IN;
      answer.ttl = 60;

      // Send the answer, unicast or multicast depending on flag in query
      uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
      snprintf(str_buffer, str_capacity, "  --> answer %.*s (%s)", MDNS_STRING_FORMAT(answer.data.ptr.name), (unicast ? "unicast" : "multicast"));
      if (unicast) {
        mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
                                  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0, 0, 0);
      } else {
        mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, 0, 0);
      }
    }
  } else if ((service.length == service_length) &&
             (strncmp(service.str, service_record->service.c_str(), service_length) == 0)) {
    if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY)) {
      // The PTR query was for our service (usually "<_service-name._tcp.local"), answer a PTR
      // record reverse mapping the queried service name to our service instance name
      // (typically on the "<hostname>.<_service-name>._tcp.local." format), and add
      // additional records containing the SRV record mapping the service instance name to our
      // qualified hostname (typically "<hostname>.local.") and port, as well as any IPv4/IPv6
      // address for the hostname as A/AAAA records, and two test TXT records
      // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
      // "<hostname>.<_service-name>._tcp.local."
      mdns_record_t answer = service_record->record_ptr;
      mdns_record_t additional[5] = {{}};
      size_t additional_count = 0;
      // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
      // "<hostname>.local." with port. Set weight & priority to 0.
      additional[additional_count++] = service_record->record_srv;
      // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
      if (service_record->address_ipv4.sin_family == AF_INET) additional[additional_count++] = service_record->record_a;
      if (service_record->address_ipv6.sin6_family == AF_INET6)
        additional[additional_count++] = service_record->record_aaaa;
      // Add two test TXT records for our service instance name, will be coalesced into
      // one record with both key-value pair strings by the library
      additional[additional_count++] = service_record->txt_record[0];
      additional[additional_count++] = service_record->txt_record[1];
      // Send the answer, unicast or multicast depending on flag in query
      uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
      snprintf(str_buffer, str_capacity, "  --> answer %.*s port %d (%s)", MDNS_STRING_FORMAT(service_record->record_srv.data.srv.name),
             service_record->port, (unicast ? "unicast" : "multicast"));
      if (unicast) {
        mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
                                  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
                                  additional, additional_count);
      } else {
        mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
      }
    }
  } else if ((name.length == service_record->service_instance.length()) &&
             (strncmp(name.str, service_record->service_instance.c_str(), name.length) == 0)) {
    if ((rtype == MDNS_RECORDTYPE_SRV) || (rtype == MDNS_RECORDTYPE_ANY)) {
      // The SRV query was for our service instance (usually
      // "<hostname>.<_service-name._tcp.local"), answer a SRV record mapping the service
      // instance name to our qualified hostname (typically "<hostname>.local.") and port, as
      // well as any IPv4/IPv6 address for the hostname as A/AAAA records, and two test TXT
      // records
      // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
      // "<hostname>.<_service-name>._tcp.local."
      mdns_record_t answer = service_record->record_srv;
      mdns_record_t additional[5] = {{}};
      size_t additional_count = 0;
      // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
      if (service_record->address_ipv4.sin_family == AF_INET) additional[additional_count++] = service_record->record_a;
      if (service_record->address_ipv6.sin6_family == AF_INET6)
        additional[additional_count++] = service_record->record_aaaa;
      // Add two test TXT records for our service instance name, will be coalesced into
      // one record with both key-value pair strings by the library
      additional[additional_count++] = service_record->txt_record[0];
      additional[additional_count++] = service_record->txt_record[1];
      // Send the answer, unicast or multicast depending on flag in query
      uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
      snprintf(str_buffer, str_capacity, "  --> answer %.*s port %d (%s)", MDNS_STRING_FORMAT(service_record->record_srv.data.srv.name),
             service_record->port, (unicast ? "unicast" : "multicast"));
      if (unicast) {
        mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
                                  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
                                  additional, additional_count);
      } else {
        mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
      }
    }
  } else if ((name.length == service_record->hostname_qualified.length()) &&
             (strncmp(name.str, service_record->hostname_qualified.c_str(), name.length) == 0)) {
    if (((rtype == MDNS_RECORDTYPE_A) || (rtype == MDNS_RECORDTYPE_ANY)) &&
        (service_record->address_ipv4.sin_family == AF_INET)) {
      // The A query was for our qualified hostname (typically "<hostname>.local.") and we
      // have an IPv4 address, answer with an A record mappiing the hostname to an IPv4
      // address, as well as any IPv6 address for the hostname, and two test TXT records
      // Answer A records mapping "<hostname>.local." to IPv4 address
      mdns_record_t answer = service_record->record_a;
      mdns_record_t additional[5] = {{}};
      size_t additional_count = 0;
      // AAAA record mapping "<hostname>.local." to IPv6 addresses
      if (service_record->address_ipv6.sin6_family == AF_INET6)
        additional[additional_count++] = service_record->record_aaaa;
      // Add two test TXT records for our service instance name, will be coalesced into
      // one record with both key-value pair strings by the library
      additional[additional_count++] = service_record->txt_record[0];
      additional[additional_count++] = service_record->txt_record[1];
      // Send the answer, unicast or multicast depending on flag in query
      uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
      const auto addrstr =
          ipAddressToString(addrbuffer, sizeof(addrbuffer), (struct sockaddr *)&service_record->record_a.data.a.addr,
                            sizeof(service_record->record_a.data.a.addr));
      snprintf(str_buffer, str_capacity, "  --> answer %.*s IPv4 %.*s (%s)", MDNS_STRING_FORMAT(service_record->record_a.name),
             (int)addrstr.length(), addrstr.c_str(), (unicast ? "unicast" : "multicast"));
      if (unicast) {
        mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
                                  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
                                  additional, additional_count);
      } else {
        mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
      }
    } else if (((rtype == MDNS_RECORDTYPE_AAAA) || (rtype == MDNS_RECORDTYPE_ANY)) &&
               (service_record->address_ipv6.sin6_family == AF_INET6)) {
      // The AAAA query was for our qualified hostname (typically "<hostname>.local.") and we
      // have an IPv6 address, answer with an AAAA record mappiing the hostname to an IPv6
      // address, as well as any IPv4 address for the hostname, and two test TXT records
      // Answer AAAA records mapping "<hostname>.local." to IPv6 address
      mdns_record_t answer = service_record->record_aaaa;
      mdns_record_t additional[5] = {{}};
      size_t additional_count = 0;
      // A record mapping "<hostname>.local." to IPv4 addresses
      if (service_record->address_ipv4.sin_family == AF_INET) additional[additional_count++] = service_record->record_a;
      // Add two test TXT records for our service instance name, will be coalesced into
      // one record with both key-value pair strings by the library
      additional[additional_count++] = service_record->txt_record[0];
      additional[additional_count++] = service_record->txt_record[1];
      // Send the answer, unicast or multicast depending on flag in query
      uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
      auto addrstr = ipAddressToString(addrbuffer, sizeof(addrbuffer),
                                       (struct sockaddr *)&service_record->record_aaaa.data.aaaa.addr,
                                       sizeof(service_record->record_aaaa.data.aaaa.addr));
      snprintf(str_buffer, str_capacity, "  --> answer %.*s IPv6 %.*s (%s)", MDNS_STRING_FORMAT(service_record->record_a.name),
             (int)addrstr.length(), addrstr.c_str(), (unicast ? "unicast" : "multicast"));
      if (unicast) {
        mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
                                  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
                                  additional, additional_count);
      } else {
        mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
      }
    }
    // #endif
  }
  MDNS_LOG << std::string(str_buffer);
  return 0;
}

mDNS::~mDNS() { stopService(); }

void mDNS::startService(const bool dumpMode) {
  dumpMode_ = dumpMode;
  if (running_) {
    stopService();
  }

  running_ = true;
  worker_thread_ = std::thread([this]() { this->runMainLoop(); });
}

void mDNS::stopService() {
  running_ = false;
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}

bool mDNS::isServiceRunning() { return running_; }

void mDNS::setServiceHostname(const std::string &hostname) { hostname_ = hostname; }

void mDNS::setServicePort(std::uint16_t port) { port_ = port; }

void mDNS::setServiceName(const std::string &name) { name_ = name; }

void mDNS::setServiceTxtRecord(const std::string &txt_record) { txt_record_ = txt_record; }

void mDNS::runMainLoop() {
  constexpr size_t number_of_sockets = 32;
  int sockets[number_of_sockets];
  const int num_sockets = openServiceSockets(sockets, sizeof(sockets) / sizeof(sockets[0]));
  if (num_sockets <= 0) {
    const auto msg = "Error: Failed to open any client sockets";
    MDNS_LOG << msg << "";
    throw std::runtime_error(msg);
  }

  if (dumpMode_) {
    runDumpMode(sockets, num_sockets);
    dumpMode_ = false;
    return;
  }

  if (name_.length() == 0) {
    const auto msg = "Error: invalid service name";
    MDNS_LOG << msg << "";
    throw std::runtime_error(msg);
  }
  if (name_[name_.length() - 1] != '.') name_ += ".";

  MDNS_LOG << "Opened " << std::to_string(num_sockets) << " socket" << (num_sockets > 1 ? "s" : "")
           << " for mDNS service";
  MDNS_LOG << "Service mDNS: " << name_ << ":" << port_ << "";
  MDNS_LOG << "Hostname: " << hostname_.data() << "";

  constexpr size_t capacity = 2048u;
  std::shared_ptr<void> buffer(malloc(capacity), free);
  ServiceRecord service_record{};
  service_record.service = name_;
  service_record.hostname = hostname_;
  {
    // Build the service instance "<hostname>.<_service-name>._tcp.local." string
    std::ostringstream oss;
    oss << hostname_ << "." << name_;
    service_record.service_instance = oss.str();
  }
  {
    // Build the "<hostname>.local." string
    std::ostringstream oss;
    oss << hostname_ << ".local.";
    service_record.hostname_qualified = oss.str();
  }
  service_record.address_ipv4 = service_address_ipv4_;
  service_record.address_ipv6 = service_address_ipv6_;
  service_record.port = port_;

  // Setup our mDNS records

  // PTR record reverse mapping "<_service-name>._tcp.local." to
  // "<hostname>.<_service-name>._tcp.local."
  service_record.record_ptr.name = to_mdns_str_ref(service_record.service);
  service_record.record_ptr.type = MDNS_RECORDTYPE_PTR,
  service_record.record_ptr.data.ptr.name = to_mdns_str_ref(service_record.service_instance);
  service_record.record_ptr.rclass = 0;
  service_record.record_ptr.ttl = 0;

  // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
  // "<hostname>.local." with port. Set weight & priority to 0.
  service_record.record_srv.name = to_mdns_str_ref(service_record.service_instance);
  service_record.record_srv.type = MDNS_RECORDTYPE_SRV;
  service_record.record_srv.data.srv.priority = 0;
  service_record.record_srv.data.srv.weight = 0;
  service_record.record_srv.data.srv.port = service_record.port;
  service_record.record_srv.data.srv.name = to_mdns_str_ref(service_record.hostname_qualified);
  service_record.record_srv.rclass = 0;
  service_record.record_srv.ttl = 0;

  // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
  service_record.record_a.name = to_mdns_str_ref(service_record.hostname_qualified);
  service_record.record_a.type = MDNS_RECORDTYPE_A;
  service_record.record_a.data.a.addr = service_record.address_ipv4;
  service_record.record_a.rclass = 0;
  service_record.record_a.ttl = 0;

  service_record.record_aaaa.name = to_mdns_str_ref(service_record.hostname_qualified);
  service_record.record_aaaa.type = MDNS_RECORDTYPE_AAAA,
  service_record.record_aaaa.data.aaaa.addr = service_record.address_ipv6;
  service_record.record_aaaa.rclass = 0;
  service_record.record_aaaa.ttl = 0;

  // Add two test TXT records for our service instance name, will be coalesced into
  // one record with both key-value pair strings by the library
  service_record.txt_record[0].name = to_mdns_str_ref(service_record.service_instance);
  service_record.txt_record[0].type = MDNS_RECORDTYPE_TXT;
  service_record.txt_record[0].data.txt.key = to_mdns_str_ref("test");
  service_record.txt_record[0].data.txt.value = to_mdns_str_ref("1");
  service_record.txt_record[0].rclass = 0;
  service_record.txt_record[0].ttl = 0;

  service_record.txt_record[1].name = to_mdns_str_ref(service_record.service_instance);
  service_record.txt_record[1].type = MDNS_RECORDTYPE_TXT;
  service_record.txt_record[1].data.txt.key = to_mdns_str_ref("other");
  service_record.txt_record[1].data.txt.value = to_mdns_str_ref("value");
  service_record.txt_record[1].rclass = 0;
  service_record.txt_record[1].ttl = 0;

  // Send an announcement on startup of service
  {
    MDNS_LOG << "Sending announce";
    mdns_record_t additional[5] = {{}};
    size_t additional_count = 0;
    additional[additional_count++] = service_record.record_srv;
    if (service_record.address_ipv4.sin_family == AF_INET) additional[additional_count++] = service_record.record_a;
    if (service_record.address_ipv6.sin6_family == AF_INET6)
      additional[additional_count++] = service_record.record_aaaa;
    additional[additional_count++] = service_record.txt_record[0];
    additional[additional_count++] = service_record.txt_record[1];
    for (int isock = 0; isock < num_sockets; ++isock)
      mdns_announce_multicast(sockets[isock], buffer.get(), capacity, service_record.record_ptr, 0, 0, additional,
                              additional_count);
  }

  // This is a crude implementation that checks for incoming queries
  while (running_) {
    int nfds = 0;
    fd_set readfs{};
    FD_ZERO(&readfs);
    for (int isock = 0; isock < num_sockets; ++isock) {
      if (sockets[isock] >= nfds) nfds = sockets[isock] + 1;
      FD_SET(sockets[isock], &readfs);
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    if (select(nfds, &readfs, 0, 0, &timeout) >= 0) {
      for (int isock = 0; isock < num_sockets; ++isock) {
        if (FD_ISSET(sockets[isock], &readfs)) {
          mdns_socket_listen(sockets[isock], buffer.get(), capacity, service_callback, &service_record);
        }
        FD_SET(sockets[isock], &readfs);
      }
    } else {
      break;
    }
  }

  // Send a goodbye on end of service
  {
    MDNS_LOG << "Sending goodbye";
    mdns_record_t additional[5] = {{}};
    size_t additional_count = 0;
    additional[additional_count++] = service_record.record_srv;
    if (service_record.address_ipv4.sin_family == AF_INET) additional[additional_count++] = service_record.record_a;
    if (service_record.address_ipv6.sin6_family == AF_INET6)
      additional[additional_count++] = service_record.record_aaaa;
    additional[additional_count++] = service_record.txt_record[0];
    additional[additional_count++] = service_record.txt_record[1];

    for (int isock = 0; isock < num_sockets; ++isock)
      mdns_goodbye_multicast(sockets[isock], buffer.get(), capacity, service_record.record_ptr, 0, 0, additional,
                             additional_count);
  }

  for (int isock = 0; isock < num_sockets; ++isock) {
    mdns_socket_close(sockets[isock]);
  }
  MDNS_LOG << "Closed socket" << (num_sockets > 1 ? "s" : "") << "";
}

void mDNS::executeQuery(ServiceQueries serviceQueries) {
  int sockets[32];
  int query_id[32];
  int num_sockets = openClientSockets(sockets, sizeof(sockets) / sizeof(sockets[0]), 0);

  if (num_sockets <= 0) {
    const auto msg = "Failed to open any client sockets";
    MDNS_LOG << msg << "";
    throw std::runtime_error(msg);
  }
  MDNS_LOG << "Opened " << num_sockets << " socket" << (num_sockets > 1 ? "s" : "") << " for mDNS query";

  size_t capacity = 2048;
  void *buffer = malloc(capacity);
  void *user_data = 0;

  std::vector<mdns_query_t> queries;

  MDNS_LOG << "Sending mDNS query";
  for (auto &query : serviceQueries) {
    auto &[name, type] = query;
    const char *record_name = "PTR";
    if (type == MDNS_RECORDTYPE_SRV)
      record_name = "SRV";
    else if (type == MDNS_RECORDTYPE_A)
      record_name = "A";
    else if (type == MDNS_RECORDTYPE_AAAA)
      record_name = "AAAA";
    else
      type = MDNS_RECORDTYPE_PTR;
    queries.push_back(mdns_query_t{static_cast<mdns_record_type>(type), name.c_str(), name.length()});
    MDNS_LOG << " : " << name.c_str() << " " << record_name;
  }

  for (int isock = 0; isock < num_sockets; ++isock) {
    query_id[isock] = mdns_multiquery_send(sockets[isock], queries.data(), queries.size(), buffer, capacity, 0);
    if (query_id[isock] < 0) {
      MDNS_LOG << "Failed to send mDNS query: " << strerror(errno) << "";
    }
  }

  // This is a simple implementation that loops for 5 seconds or as long as we
  // get replies
  int res{};
  MDNS_LOG << "Reading mDNS query replies";
  int records = 0;
  do {
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    int nfds = 0;
    fd_set readfs;
    FD_ZERO(&readfs);
    for (int isock = 0; isock < num_sockets; ++isock) {
      if (sockets[isock] >= nfds) nfds = sockets[isock] + 1;
      FD_SET(sockets[isock], &readfs);
    }

    res = select(nfds, &readfs, 0, 0, &timeout);
    if (res > 0) {
      for (int isock = 0; isock < num_sockets; ++isock) {
        if (FD_ISSET(sockets[isock], &readfs)) {
          auto rec = mdns_query_recv(sockets[isock], buffer, capacity, query_callback, user_data, query_id[isock]);
          if (rec > 0) records += static_cast<int>(rec);
        }
        FD_SET(sockets[isock], &readfs);
      }
    }
  } while (res > 0);
  MDNS_LOG << "Read " << records << " records";

  free(buffer);

  for (int isock = 0; isock < num_sockets; ++isock) {
    mdns_socket_close(sockets[isock]);
  }
  MDNS_LOG << "Closed socket" << (num_sockets > 1 ? "s" : "") << "";
}

void mDNS::executeDiscovery() {
  int sockets[32];
  int num_sockets = openClientSockets(sockets, sizeof(sockets) / sizeof(sockets[0]), 0);
  if (num_sockets <= 0) {
    const auto msg = "Failed to open any client sockets";
    MDNS_LOG << msg << "";
    throw std::runtime_error(msg);
  }

  MDNS_LOG << "Opened " << num_sockets << " socket" << (num_sockets > 1 ? "s" : "") << " for DNS-SD";
  MDNS_LOG << "Sending DNS-SD discovery";
  for (int isock = 0; isock < num_sockets; ++isock) {
    if (mdns_discovery_send(sockets[isock])) {
      MDNS_LOG << "Failed to send DNS-DS discovery: " << strerror(errno) << " ";
    }
  }

  size_t capacity = 2048;
  void *buffer = malloc(capacity);
  void *user_data = 0;
  size_t records;

  // This is a simple implementation that loops for 5 seconds or as long as we
  // get replies
  int res;
  MDNS_LOG << "Reading DNS-SD replies";
  do {
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int nfds = 0;
    fd_set readfs;
    FD_ZERO(&readfs);
    for (int isock = 0; isock < num_sockets; ++isock) {
      if (sockets[isock] >= nfds) nfds = sockets[isock] + 1;
      FD_SET(sockets[isock], &readfs);
    }

    records = 0;
    res = select(nfds, &readfs, 0, 0, &timeout);
    if (res > 0) {
      for (int isock = 0; isock < num_sockets; ++isock) {
        if (FD_ISSET(sockets[isock], &readfs)) {
          records += mdns_discovery_recv(sockets[isock], buffer, capacity, query_callback, user_data);
        }
      }
    }
  } while (res > 0);

  free(buffer);

  for (int isock = 0; isock < num_sockets; ++isock) {
    mdns_socket_close(sockets[isock]);
  }
  MDNS_LOG << "Closed socket" << (num_sockets > 1 ? "s" : "") << "";
}

void mDNS::runDumpMode(int *sockets, const int num_sockets) {
  printf("Opened %d socket%s for mDNS dump", num_sockets, num_sockets > 1 ? "s" : "");
  size_t capacity = 2048;
  void *buffer = malloc(capacity);
  // This is a crude implementation that checks for incoming queries and answers
  while (running_) {
    int nfds = 0;
    fd_set readfs;
    FD_ZERO(&readfs);
    for (int isock = 0; isock < num_sockets; ++isock) {
      if (sockets[isock] >= nfds) nfds = sockets[isock] + 1;
      FD_SET(sockets[isock], &readfs);
    }
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    if (select(nfds, &readfs, 0, 0, &timeout) >= 0) {
      for (int isock = 0; isock < num_sockets; ++isock) {
        if (FD_ISSET(sockets[isock], &readfs)) {
          mdns_socket_listen(sockets[isock], buffer, capacity, dump_callback, 0);
        }
        FD_SET(sockets[isock], &readfs);
      }
    } else {
      break;
    }
  }
  free(buffer);
  for (int isock = 0; isock < num_sockets; ++isock) mdns_socket_close(sockets[isock]);
  printf("Closed socket%s", num_sockets > 1 ? "s" : "");
}

}  // namespace mdns_cpp
