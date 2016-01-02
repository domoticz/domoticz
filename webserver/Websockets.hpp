#pragma once
#include <boost/logic/tribool.hpp>

namespace http {
	namespace server {

		enum opcodes {
			opcode_continuation = 0x00,
			opcode_text = 0x01,
			opcode_binary = 0x02,
			opcode_close = 0x08,
			opcode_ping = 0x09,
			opcode_pong = 0x0a
		};

		class connection;

		class CWebsocket {
		public:
			CWebsocket(connection *pConn);
			~CWebsocket();
			boost::tribool parse(const char *begin, size_t size, std::string &websocket_data, size_t &bytes_consumed, bool &keep_alive);
		private:
			void OnReceiveText(const std::string &packet_data);
			void OnReceiveBinary(const std::string &packet_data);
			void SendPong(const std::string &packet_data);
			void SendClose(const std::string &packet_data);
			std::string packet_data;
			bool start_new_packet;
			opcodes last_opcode;
			connection *conn;
		};

	}
}