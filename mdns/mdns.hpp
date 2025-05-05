#pragma once

#include <functional>
#include <string>
#include <thread>

#include "utils.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

namespace mdns_cpp {

class mDNS {
 public:
  ~mDNS();

  void startService(bool dumpMode = false);
  void stopService();
  bool isServiceRunning();

  void setServiceHostname(const std::string &hostname);
  void setServicePort(std::uint16_t port);
  void setServiceName(const std::string &name);
  void setServiceTxtRecord(const std::string &text_record);

  using ServiceQueries = std::vector<std::pair<std::string, int>>;
  void executeQuery(ServiceQueries service);
  void executeDiscovery();

 private:
  void runMainLoop();
  void runDumpMode(int *sockets, int num_sockets);
  int openClientSockets(int *sockets, int max_sockets, int port);
  int openServiceSockets(int *sockets, int max_sockets);

  std::string hostname_{"dummy-host"};
  std::string name_{"_http._tcp.local."};
  std::uint16_t port_{42424};
  std::string txt_record_{};

  bool running_{false};
  bool dumpMode_{false};

  struct sockaddr_in service_address_ipv4_;
  struct sockaddr_in6 service_address_ipv6_;

  std::thread worker_thread_;
};

}  // namespace mdns_cpp
