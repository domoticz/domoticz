#pragma once
#include <boost/logic/tribool.hpp>

namespace http {
	namespace server {

		class CWebsocket {
		public:
			CWebsocket();
			~CWebsocket();
			boost::tribool parse(const char *begin, size_t size, std::string &websocket_data);
			void handle_packet(const std::string &websocket_data, bool &keepalive_);
		};

	}
}