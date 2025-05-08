#pragma once

#include <string>
#include <thread>

#include "include/mdns.h"

#define DOMOTICZ_MDNS_SERVICE_TYPE "_http._tcp.local."
#define MDNS_TXT_RECORD_COUNT 2

namespace domoticz_mdns
{

	static mdns_record_txt_t txtbuffer[128];

	class ServiceRecord
	{
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
		mdns_record_t txt_record[MDNS_TXT_RECORD_COUNT];
	};

	class mDNS : public StoppableTask
	{
	public:
		~mDNS();

		void startService();
		void stopService();
		bool isServiceRunning();

		void setServiceHostname(const std::string &hostname);
		void setServicePort(std::uint16_t port);
		void setServiceName(const std::string &name);
		void setServiceTxtRecord(const std::string &text_record_key, const std::string &text_record_value);

		using ServiceQueries = std::vector<std::pair<std::string, int>>;
		void executeQuery(ServiceQueries service);
		void executeDiscovery();

	private:
		std::string name_{DOMOTICZ_MDNS_SERVICE_TYPE};
		std::string hostname_;
		std::uint16_t port_;
		std::array<std::pair<std::string, std::string>, MDNS_TXT_RECORD_COUNT> txt_key_pairs_;

		bool running_{false};
		struct sockaddr_in service_address_ipv4_;
		struct sockaddr_in6 service_address_ipv6_;
		std::shared_ptr<std::thread> mdns_worker_thread_;

		void mDnsMainLoop();
		int openClientSockets(int *sockets, int max_sockets, int port);
		int openServiceSockets(int *sockets, int max_sockets);

		static int service_callback(int sock, const struct sockaddr *from, size_t addrlen, mdns_entry_type entry, uint16_t query_id,
									uint16_t rtype, uint16_t rclass, uint32_t ttl, const void *data, size_t size, size_t name_offset,
									size_t name_length, size_t record_offset, size_t record_length, void *user_data);

		static std::string sockaddrToString(const sockaddr* addr);
	};
}
