#include "stdafx.h"
#include "mdns.hpp"

#include "../main/Logger.h"
#include "../main/Helper.h"

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

namespace domoticz_mdns
{

	mdns_string_t to_mdns_str_ref(const std::string &str_ref) { return {str_ref.c_str(), str_ref.length()}; }

	mDNS::~mDNS() { stopService(); }

	void mDNS::startService()
	{
		if (running_)
		{
			stopService();
		}

		if (hostname_.empty() || port_ == 0 || name_.empty())
		{
			// We need a hostname and port to start the service
			_log.Log(LOG_ERROR, "mDNS: Hostname or port not set!");
			return;
		}

		running_ = true;
		mdns_worker_thread_ = std::make_shared<std::thread>([this]() { this->mDnsMainLoop(); });
		SetThreadName(mdns_worker_thread_->native_handle(), "mDnsWorker");
		_log.Log(LOG_STATUS, "mDNS: Service started");
	}

	void mDNS::stopService()
	{
		if (running_ && mdns_worker_thread_->joinable())
		{
			running_ = false;
			RequestStop();
			mdns_worker_thread_->join();
			mdns_worker_thread_.reset();
			_log.Log(LOG_STATUS, "mDNS: Service stopped");
		}
	}

	bool mDNS::isServiceRunning() { return running_; }

	void mDNS::setServiceHostname(const std::string &hostname) { hostname_ = hostname; }

	void mDNS::setServicePort(std::uint16_t port) { port_ = port; }

	void mDNS::setServiceName(const std::string &name) { name_ = name; }

	void mDNS::setServiceTxtRecord(const std::string &txt_record_key, const std::string &txt_record_value)
	{
		if (txt_record_key.empty() || txt_record_value.empty())
			return;

		for (size_t i = 0; i < MDNS_TXT_RECORD_COUNT; ++i) {
			if (txt_key_pairs_[i].first.empty()) {
				txt_key_pairs_[i] = {txt_record_key, txt_record_value};
				break;
			}
		}
	}

	void mDNS::mDnsMainLoop()
	{
		constexpr size_t number_of_sockets = 32;
		int sockets[number_of_sockets];

		const int num_sockets = openServiceSockets(sockets, sizeof(sockets) / sizeof(sockets[0]));
		if (num_sockets <= 0)
		{
			_log.Log(LOG_ERROR, "mDNS: Failed to open any client sockets!");
			return;
		}

		if (name_[name_.length() - 1] != '.')
		name_ += ".";

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
		service_record.record_ptr.data.ptr = mdns_record_ptr_t{.name = to_mdns_str_ref(service_record.service_instance)};
		service_record.record_ptr.rclass = 0;
		service_record.record_ptr.ttl = 0;

		// SRV record mapping "<hostname>.<_service-name>._tcp.local." to
		// "<hostname>.local." with port. Set weight & priority to 0.
		service_record.record_srv.name = to_mdns_str_ref(service_record.service_instance);
		service_record.record_srv.type = MDNS_RECORDTYPE_SRV;
		service_record.record_srv.data.srv = mdns_record_srv_t{.priority = 0,
															.weight = 0,
															.port = service_record.port,
															.name = to_mdns_str_ref(service_record.hostname_qualified)};
		service_record.record_srv.rclass = 0;
		service_record.record_srv.ttl = 0;

		// A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
		service_record.record_a.name = to_mdns_str_ref(service_record.hostname_qualified);
		service_record.record_a.type = MDNS_RECORDTYPE_A;
		service_record.record_a.data.a = {mdns_record_a_t{.addr = service_record.address_ipv4}};
		service_record.record_a.rclass = 0;
		service_record.record_a.ttl = 0;

		service_record.record_aaaa.name = to_mdns_str_ref(service_record.hostname_qualified);
		service_record.record_aaaa.type = MDNS_RECORDTYPE_AAAA;
		service_record.record_aaaa.data.aaaa.addr = service_record.address_ipv6;
		service_record.record_aaaa.rclass = 0;
		service_record.record_aaaa.ttl = 0;

		// Add TXT records for our service instance name, will be coalesced into one record with both key-value pair strings by the library
		for (size_t i = 0; i < MDNS_TXT_RECORD_COUNT; ++i) {
			service_record.txt_record[i].name = to_mdns_str_ref(service_record.service_instance);
			service_record.txt_record[i].type = MDNS_RECORDTYPE_TXT;
			service_record.txt_record[i].data.txt.key = to_mdns_str_ref(txt_key_pairs_[i].first);
			service_record.txt_record[i].data.txt.value = to_mdns_str_ref(txt_key_pairs_[i].second);
			service_record.txt_record[i].rclass = 0;
			service_record.txt_record[i].ttl = 0;
		}

		_log.Log(LOG_NORM, "mDNS: Service: %s:%d for Hostname: %s (%d socket%s)", name_.c_str(), port_, hostname_.c_str(), num_sockets, (num_sockets > 1 ? "s" : ""));

		constexpr size_t capacity = 2048u;
		std::shared_ptr<void> buffer(malloc(capacity), free);

		// Send an announcement on startup of service
		{
			_log.Debug(DEBUG_NORM, "mDNS: Sending mDNS announce");
			mdns_record_t additional[5] = {{}};
			size_t additional_count = 0;
			additional[additional_count++] = service_record.record_srv;
			if (service_record.address_ipv4.sin_family == AF_INET)
				additional[additional_count++] = service_record.record_a;
			if (service_record.address_ipv6.sin6_family == AF_INET6)
				additional[additional_count++] = service_record.record_aaaa;
			additional[additional_count++] = service_record.txt_record[0];
			additional[additional_count++] = service_record.txt_record[1];
			for (int isock = 0; isock < num_sockets; ++isock)
				mdns_announce_multicast(sockets[isock], buffer.get(), capacity, service_record.record_ptr, 0, 0, additional,
										additional_count);
		}

		// This is a crude implementation that checks for incoming queries
		while (running_)
		{
			int nfds = 0;
			fd_set readfs{};
			FD_ZERO(&readfs);
			for (int isock = 0; isock < num_sockets; ++isock)
			{
				if (sockets[isock] >= nfds)
					nfds = sockets[isock] + 1;
				FD_SET(sockets[isock], &readfs);
			}

			struct timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000;
			if (select(nfds, &readfs, 0, 0, &timeout) >= 0)
			{
				for (int isock = 0; isock < num_sockets; ++isock)
				{
					if (FD_ISSET(sockets[isock], &readfs))
					{
						mdns_socket_listen(sockets[isock], buffer.get(), capacity, service_callback, &service_record);
					}
					FD_SET(sockets[isock], &readfs);
				}
			}
			else
			{
				break;
			}
		}

		// Send a goodbye on end of service
		{
			_log.Debug(DEBUG_NORM, "mDNS: Sending mDNS goodbye");
			mdns_record_t additional[5] = {{}};
			size_t additional_count = 0;
			additional[additional_count++] = service_record.record_srv;
			if (service_record.address_ipv4.sin_family == AF_INET)
				additional[additional_count++] = service_record.record_a;
			if (service_record.address_ipv6.sin6_family == AF_INET6)
				additional[additional_count++] = service_record.record_aaaa;
			additional[additional_count++] = service_record.txt_record[0];
			additional[additional_count++] = service_record.txt_record[1];

			for (int isock = 0; isock < num_sockets; ++isock)
				mdns_goodbye_multicast(sockets[isock], buffer.get(), capacity, service_record.record_ptr, 0, 0, additional,
									   additional_count);
		}

		for (int isock = 0; isock < num_sockets; ++isock)
		{
			mdns_socket_close(sockets[isock]);
		}
		_log.Log(LOG_NORM, "mDNS: Closed %d socket%s", num_sockets, (num_sockets > 1 ? "s" : ""));
	}

	int mDNS::openServiceSockets(int *sockets, int max_sockets)
	{
		// When receiving, each socket can receive data from all network interfaces
		// Thus we only need to open one socket for each address family
		int num_sockets = 0;

		// Call the client socket function to enumerate and get local addresses,
		// but not open the actual sockets
		openClientSockets(0, 0, 0);

		if (num_sockets < max_sockets)
		{
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
			if (sock >= 0)
			{
				sockets[num_sockets++] = sock;
				const auto addr = sockaddrToString(reinterpret_cast<struct sockaddr *>(&sock_addr));
				_log.Debug(DEBUG_NORM, "mDNS: Local IPv4 address: %s", addr.c_str());
			}
		}

		if (num_sockets < max_sockets)
		{
			sockaddr_in6 sock_addr{};
			sock_addr.sin6_family = AF_INET6;
			sock_addr.sin6_addr = in6addr_any;
			sock_addr.sin6_port = htons(MDNS_PORT);
#ifdef __APPLE__
			sock_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
			int sock = mdns_socket_open_ipv6(&sock_addr);
			if (sock >= 0)
				sockets[num_sockets++] = sock;
				const auto addr = sockaddrToString(reinterpret_cast<struct sockaddr *>(&sock_addr));
				_log.Debug(DEBUG_NORM, "mDNS: Local IPv6 address: %s", addr.c_str());
		}

		return num_sockets;
	}

	int mDNS::openClientSockets(int *sockets, int max_sockets, int port)
	{
		// When sending, each socket can only send to one network interface
		// Thus we need to open one socket for each interface and address family
		int num_sockets = 0;

#ifdef _WIN32

		IP_ADAPTER_ADDRESSES *adapter_address = nullptr;
		ULONG address_size = 8000;
		unsigned int ret{};
		unsigned int num_retries = 4;
		do
		{
			adapter_address = (IP_ADAPTER_ADDRESSES *)malloc(address_size);
			ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST, 0, adapter_address,
									   &address_size);
			if (ret == ERROR_BUFFER_OVERFLOW)
			{
				free(adapter_address);
				address_size *= 2;
			}
			else
			{
				break;
			}
		} while (num_retries-- > 0);

		if (!adapter_address || (ret != NO_ERROR))
		{
			free(adapter_address);
			_log.Log(LOG_ERROR, "mDNS: Failed to get network adapter addresses!");
			return num_sockets;
		}

		int first_ipv4 = 1;
		int first_ipv6 = 1;
		for (PIP_ADAPTER_ADDRESSES adapter = adapter_address; adapter; adapter = adapter->Next)
		{
			if (adapter->TunnelType == TUNNEL_TYPE_TEREDO)
			{
				continue;
			}
			if (adapter->OperStatus != IfOperStatusUp)
			{
				continue;
			}

			for (IP_ADAPTER_UNICAST_ADDRESS *unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
			{
				if (unicast->Address.lpSockaddr->sa_family == AF_INET)
				{
					struct sockaddr_in *saddr = (struct sockaddr_in *)unicast->Address.lpSockaddr;
					if ((saddr->sin_addr.S_un.S_un_b.s_b1 != 127) || (saddr->sin_addr.S_un.S_un_b.s_b2 != 0) ||
						(saddr->sin_addr.S_un.S_un_b.s_b3 != 0) || (saddr->sin_addr.S_un.S_un_b.s_b4 != 1))
					{
						if (first_ipv4)
						{
							service_address_ipv4_ = *saddr;
							first_ipv4 = 0;
						}

						if (num_sockets < max_sockets)
						{
							saddr->sin_port = htons((unsigned short)port);
							int sock = mdns_socket_open_ipv4(saddr);
							if (sock >= 0)
							{
								sockets[num_sockets++] = sock;
							}
						}
					}
				}
				else if (unicast->Address.lpSockaddr->sa_family == AF_INET6)
				{
					struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)unicast->Address.lpSockaddr;
					// Ignore link-local addresses
					if (saddr->sin6_scope_id)
						continue;
					static constexpr unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
					static constexpr unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
					if ((unicast->DadState == NldsPreferred) && memcmp(saddr->sin6_addr.s6_addr, localhost, 16) &&
						memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16))
					{
						if (first_ipv6)
						{
							memcpy(&service_address_ipv6_, &saddr->sin6_addr, sizeof(saddr->sin6_addr));
							first_ipv6 = 0;
						}

						if (num_sockets < max_sockets)
						{
							saddr->sin6_port = htons((unsigned short)port);
							int sock = mdns_socket_open_ipv6(saddr);
							if (sock >= 0)
							{
								sockets[num_sockets++] = sock;
							}
						}
					}
				}
			}
		}

		free(adapter_address);

#else

		struct ifaddrs *ifaddr = nullptr;
		struct ifaddrs *ifa = nullptr;

		if (getifaddrs(&ifaddr) < 0)
		{
			_log.Log(LOG_ERROR, "mDNS: Unable to get interface addresses!");
		}

		int first_ipv4 = 1;
		int first_ipv6 = 1;
		for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
		{
			if (!ifa->ifa_addr)
			{
				continue;
			}
			if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_MULTICAST))
				continue;
			if ((ifa->ifa_flags & IFF_LOOPBACK) || (ifa->ifa_flags & IFF_POINTOPOINT))
				continue;

			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				struct sockaddr_in *saddr = (struct sockaddr_in *)ifa->ifa_addr;
				if (saddr->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
				{
					if (first_ipv4)
					{
						service_address_ipv4_ = *saddr;
						first_ipv4 = 0;
					}

					if (num_sockets < max_sockets)
					{
						saddr->sin_port = htons(port);
						int sock = mdns_socket_open_ipv4(saddr);
						if (sock >= 0)
						{
							sockets[num_sockets++] = sock;
						}
					}
				}
			}
			else if (ifa->ifa_addr->sa_family == AF_INET6)
			{
				struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)ifa->ifa_addr;
				// Ignore link-local addresses
				if (saddr->sin6_scope_id)
					continue;
				static constexpr unsigned char localhost[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
				static constexpr unsigned char localhost_mapped[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1};
				if (memcmp(saddr->sin6_addr.s6_addr, localhost, 16) && memcmp(saddr->sin6_addr.s6_addr, localhost_mapped, 16))
				{
					if (first_ipv6)
					{
						service_address_ipv6_ = *saddr;
						first_ipv6 = 0;
					}

					if (num_sockets < max_sockets)
					{
						saddr->sin6_port = htons(port);
						int sock = mdns_socket_open_ipv6(saddr);
						if (sock >= 0)
						{
							sockets[num_sockets++] = sock;
						}
					}
				}
			}
		}

		freeifaddrs(ifaddr);

#endif

		return num_sockets;
	}

	int mDNS::service_callback(int sock, const struct sockaddr *from, size_t addrlen, mdns_entry_type entry, uint16_t query_id,
							   uint16_t rtype, uint16_t rclass, uint32_t ttl, const void *data, size_t size, size_t name_offset,
							   size_t name_length, size_t record_offset, size_t record_length, void *user_data)
	{
		(void)sizeof(ttl);

		if (static_cast<int>(entry) != MDNS_ENTRYTYPE_QUESTION)
		{
			return 0;
		}

		// mDNS meta command to query for all servicetypes
		const char dns_sd[] = "_services._dns-sd._udp.local.";
		const ServiceRecord *service_record = (const ServiceRecord *)user_data;

		char namebuffer[256] = {0};
		char sendbuffer[1024] = {0};

		const mdns_string_t service = mdns_record_parse_ptr(data, size, record_offset, record_length, namebuffer, sizeof(namebuffer));
		const size_t service_length = service_record->service.length();
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

		//_log.Debug(DEBUG_RECEIVED, "mDNS: Query %s:%.*s", record_name, MDNS_STRING_FORMAT(name));
		const int str_capacity = 1000;
		if ((name.length == (sizeof(dns_sd) - 1)) && (strncmp(name.str, dns_sd, sizeof(dns_sd) - 1) == 0))
		{
			if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY))
			{
				// The PTR query was for the DNS-SD domain, send answer with a PTR record for the
				// service name we advertise, typically on the "<_service-name>._tcp.local." format
				// Answer PTR record reverse mapping "<_service-name>._tcp.local." to
				// "<hostname>.<_service-name>._tcp.local."
				mdns_record_t answer = {.name = name,
										.type = MDNS_RECORDTYPE_PTR,
										.data = {mdns_record_ptr_t{name = to_mdns_str_ref(service_record->service)}},
										.rclass = MDNS_CLASS_IN,
										.ttl = 60};
				// Send the answer, unicast or multicast depending on flag in query
				uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
				if (unicast)
				{
					mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
											  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0, 0, 0);
				}
				else
				{
					mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, 0, 0);
				}
				_log.Debug(DEBUG_RECEIVED, "mDNS: Query %s:%.*s --> answer %.*s (%s)", record_name, MDNS_STRING_FORMAT(name),
						MDNS_STRING_FORMAT(answer.data.ptr.name), (unicast ? "unicast" : "multicast"));
			}
		}
		else if ((service.length == service_record->service.length()) &&
				 (strncmp(service.str, service_record->service.c_str(), service_length) == 0))
		{
			if ((rtype == MDNS_RECORDTYPE_PTR) || (rtype == MDNS_RECORDTYPE_ANY))
			{
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
				if (service_record->address_ipv4.sin_family == AF_INET)
					additional[additional_count++] = service_record->record_a;
				if (service_record->address_ipv6.sin6_family == AF_INET6)
					additional[additional_count++] = service_record->record_aaaa;
				// Add two test TXT records for our service instance name, will be coalesced into
				// one record with both key-value pair strings by the library
				additional[additional_count++] = service_record->txt_record[0];
				additional[additional_count++] = service_record->txt_record[1];
				// Send the answer, unicast or multicast depending on flag in query
				uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
				if (unicast)
				{
					mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
											  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
											  additional, additional_count);
				}
				else
				{
					mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
				}
				_log.Debug(DEBUG_RECEIVED, "mDNS: Query %s:%.*s --> answer %.*s port %d (%s)", record_name, MDNS_STRING_FORMAT(name),
						MDNS_STRING_FORMAT(service_record->record_srv.data.srv.name), service_record->port, (unicast ? "unicast" : "multicast"));
			}
		}
		else if ((name.length == service_record->service_instance.length()) &&
				 (strncmp(name.str, service_record->service_instance.c_str(), name.length) == 0))
		{
			if ((rtype == MDNS_RECORDTYPE_SRV) || (rtype == MDNS_RECORDTYPE_ANY))
			{
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
				if (service_record->address_ipv4.sin_family == AF_INET)
					additional[additional_count++] = service_record->record_a;
				if (service_record->address_ipv6.sin6_family == AF_INET6)
					additional[additional_count++] = service_record->record_aaaa;
				// Add two test TXT records for our service instance name, will be coalesced into
				// one record with both key-value pair strings by the library
				additional[additional_count++] = service_record->txt_record[0];
				additional[additional_count++] = service_record->txt_record[1];
				// Send the answer, unicast or multicast depending on flag in query
				uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
				if (unicast)
				{
					mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
											  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
											  additional, additional_count);
				}
				else
				{
					mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
				}
				_log.Debug(DEBUG_RECEIVED, "mDNS: Query %s:%.*s --> answer %.*s port %d (%s)", record_name, MDNS_STRING_FORMAT(name),
						MDNS_STRING_FORMAT(service_record->record_srv.data.srv.name), service_record->port, (unicast ? "unicast" : "multicast"));
			}
		}
		else if ((name.length == service_record->hostname_qualified.length()) &&
				 (strncmp(name.str, service_record->hostname_qualified.c_str(), name.length) == 0))
		{
			if (((rtype == MDNS_RECORDTYPE_A) || (rtype == MDNS_RECORDTYPE_ANY)) &&
				(service_record->address_ipv4.sin_family == AF_INET))
			{
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
				const auto addrstr = sockaddrToString((struct sockaddr *)&service_record->record_a.data.a.addr);
				if (unicast)
				{
					mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
											  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
											  additional, additional_count);
				}
				else
				{
					mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
				}
				_log.Debug(DEBUG_RECEIVED, "mDNS: Query %s:%.*s --> answer %.*s IPv4 %.*s (%s)", record_name, MDNS_STRING_FORMAT(name),
						MDNS_STRING_FORMAT(service_record->record_a.name), (int)addrstr.length(), addrstr.c_str(), (unicast ? "unicast" : "multicast"));
			}
			else if (((rtype == MDNS_RECORDTYPE_AAAA) || (rtype == MDNS_RECORDTYPE_ANY)) &&
					 (service_record->address_ipv6.sin6_family == AF_INET6))
			{
				// The AAAA query was for our qualified hostname (typically "<hostname>.local.") and we
				// have an IPv6 address, answer with an AAAA record mappiing the hostname to an IPv6
				// address, as well as any IPv4 address for the hostname, and two test TXT records
				// Answer AAAA records mapping "<hostname>.local." to IPv6 address
				mdns_record_t answer = service_record->record_aaaa;
				mdns_record_t additional[5] = {{}};
				size_t additional_count = 0;
				// A record mapping "<hostname>.local." to IPv4 addresses
				if (service_record->address_ipv4.sin_family == AF_INET)
					additional[additional_count++] = service_record->record_a;
				// Add two test TXT records for our service instance name, will be coalesced into
				// one record with both key-value pair strings by the library
				additional[additional_count++] = service_record->txt_record[0];
				additional[additional_count++] = service_record->txt_record[1];
				// Send the answer, unicast or multicast depending on flag in query
				uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
				auto addrstr = sockaddrToString((struct sockaddr *)&service_record->record_aaaa.data.aaaa.addr);
				if (unicast)
				{
					mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer), query_id,
											  static_cast<mdns_record_type_t>(rtype), name.str, name.length, answer, 0, 0,
											  additional, additional_count);
				}
				else
				{
					mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, additional, additional_count);
				}
				_log.Debug(DEBUG_RECEIVED, "mDNS: Query %s:%.*s --> answer %.*s IPv6 %.*s (%s)", record_name, MDNS_STRING_FORMAT(name), 
						MDNS_STRING_FORMAT(service_record->record_aaaa.name), (int)addrstr.length(), addrstr.c_str(), (unicast ? "unicast" : "multicast"));
			}
			// #endif
		}
		return 0;
	}

	std::string mDNS::sockaddrToString(const sockaddr* addr)
	{
		char buffer[INET6_ADDRSTRLEN]; // Large enough for both IPv4 and IPv6 addresses

		if (addr->sa_family == AF_INET) {
			// IPv4
			const sockaddr_in* ipv4 = reinterpret_cast<const sockaddr_in*>(addr);
			if (inet_ntop(AF_INET, &(ipv4->sin_addr), buffer, sizeof(buffer)) == nullptr) {
				return "Invalid IPv4 address";
			}
			return std::string(buffer);
		} else if (addr->sa_family == AF_INET6) {
			// IPv6
			const sockaddr_in6* ipv6 = reinterpret_cast<const sockaddr_in6*>(addr);
			if (inet_ntop(AF_INET6, &(ipv6->sin6_addr), buffer, sizeof(buffer)) == nullptr) {
				return "Invalid IPv6 address";
			}
			return std::string(buffer);
		}
		return "Unknown address family";
	}
}
