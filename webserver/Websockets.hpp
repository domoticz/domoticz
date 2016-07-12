#pragma once
#include <boost/logic/tribool.hpp>
#include <string>
#include "WebsocketHandler.h"

#define size_t_t long long

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
		class cWebem;

		class CWebsocketFrame {
		public:
			CWebsocketFrame();
			~CWebsocketFrame();
			bool Parse(const unsigned char *bytes, size_t_t size);
			std::string Payload();
			bool isFinal();
			size_t_t Consumed();
			opcodes Opcode();
			static std::string Create(opcodes opcode, const std::string &payload, bool domasking = true);
		private:
			static std::string unmask(const unsigned char *mask, const unsigned char *bytes, size_t_t payloadlen);
			bool fin;
			bool rsvi1;
			bool rsvi2;
			bool rsvi3;
			opcodes opcode;
			bool masking;
			size_t_t payloadlen, bytes_consumed;
			std::string payload;
		};

		class CWebsocket {
		public:
			CWebsocket(boost::function<void(const std::string &packet_data)> _MyWrite, CWebsocketHandler *_handler);
			~CWebsocket();
			boost::tribool parse(const unsigned char *begin, size_t_t size, size_t_t &bytes_consumed, bool &keep_alive);
			void SendClose(const std::string &packet_data);
			void SendPing();
			void Start();
			void Stop();
		private:
			void OnReceiveText(const std::string &packet_data);
			void OnReceiveBinary(const std::string &packet_data);
			void OnPong(const std::string &packet_data);
			void SendPong(const std::string &packet_data);
			std::string packet_data;
			bool start_new_packet;
			opcodes last_opcode;
			const char *OUR_PING_ID;
			CWebsocketHandler *handler;
			boost::function<void(const std::string &packet_data)> MyWrite;
		};

	}
}
