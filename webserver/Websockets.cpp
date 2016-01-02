#include "stdafx.h"
#include "Websockets.hpp"
#include "connection.hpp"
#include <string>

namespace http {
	namespace server {

std::string CreateFrame(opcodes opcode, const std::string &payload)
{
	// note: we dont do masking nor fragmentation
	const unsigned char FIN_MASK = 0x80;
	const unsigned char OPCODE_MASK = 0x0f;
	size_t payloadlen = payload.size();
	std::string res = "";
	res += ((char)(FIN_MASK + (((char)opcode) & OPCODE_MASK)));
	if (payloadlen <= 127) {
		res += (char)payloadlen;
	}
	else {
		if (payloadlen <= 65535) {
			res += (char)126;
			int bits = 16;
			while (bits) {
				bits -= 8;
				res += (char)((payloadlen >> bits) & 0xff);
			}
		}
		else {
			res += (char)127;
			int bits = 64;
			while (bits) {
				bits -= 8;
				res += (char)((payloadlen >> bits) & 0xff);
			}
		}
	}
	res += payload;
	return res;
}

class CWebsocketFrame {
public:
	CWebsocketFrame() { };
	~CWebsocketFrame() {};

	std::string unmask(const unsigned char *mask, const unsigned char *bytes, size_t payloadlen) {
		std::string result = "";
		for (size_t i = 0; i < payloadlen; i++) {
			result += (char)(bytes[i] ^ mask[i % 4]);
		}
		return result;
	}

	bool Parse (const unsigned char *bytes, size_t size) {
		size_t remaining = size;
		bytes_consumed = 0;
		if (remaining < 2) {
			return false;
		}
		fin = (bytes[0] & FIN_MASK) > 0;
		rsvi1 = (bytes[0] & RSVI1_MASK) > 0;
		rsvi2 = (bytes[0] & RSVI2_MASK) > 0;
		rsvi3 = (bytes[0] & RSVI3_MASK) > 0;
		opcode = (opcodes)(bytes[0] & OPCODE_MASK);
		masking = (bytes[1] & MASKING_MASK) > 0;
		payloadlen = (bytes[1] & PAYLOADLEN_MASK);
		remaining -= 2;
		int ptr = 2;
		if (payloadlen == 126) {
			if (remaining < 2) {
				return false;
			}
			payloadlen = 0;
			int mult = 1;
			for (unsigned int i = 0; i < 2; i++) {
				payloadlen = (payloadlen * mult) + bytes[ptr++];
				mult *= 256;
				remaining--;
			}
		}
		else if (payloadlen == 127) {
			if (remaining < 8) {
				return false;
			}
			payloadlen = 0;
			int mult = 1;
			for (unsigned int i = 0; i < 8; i++) {
				payloadlen = (payloadlen * mult) + bytes[ptr++];
				mult *= 256;
				remaining--;
			}
		}
		if (masking) {
			if (remaining < 4) {
				return false;
			}
			for (unsigned int i = 0; i < 4; i++) {
				masking_key[i] = bytes[ptr++];
				remaining--;
			}
		}
		if (remaining < payloadlen) {
			return false;
		}
		if (masking) {
			payload = unmask(masking_key, &bytes[ptr], payloadlen);
		}
		else {
			payload.assign((char *)&bytes[ptr], payloadlen);
		}
		remaining -= payloadlen;
		ptr += payloadlen;
		bytes_consumed = ptr;
		return true;
	};

	std::string Payload() {
		return payload;
	};

	bool isFinal() {
		return fin;
	};

	size_t Consumed() {
		return bytes_consumed;
	};

	opcodes Opcode() {
		return opcode;
	};

private:
	const unsigned char FIN_MASK = 0x80;
	const unsigned char RSVI1_MASK = 0x40;
	const unsigned char RSVI2_MASK = 0x20;
	const unsigned char RSVI3_MASK = 0x10;
	const unsigned char OPCODE_MASK = 0x0f;
	const unsigned char MASKING_MASK = 0x80;
	const unsigned char PAYLOADLEN_MASK = 0x7f;
	bool fin;
	bool rsvi1;
	bool rsvi2;
	bool rsvi3;
	opcodes opcode;
	bool masking;
	size_t payloadlen, bytes_consumed;
	unsigned char masking_key[4];
	std::string payload;
};


CWebsocket::CWebsocket(connection *pConn)
{
	start_new_packet = true;
	conn = pConn;
}

CWebsocket::~CWebsocket()
{
}

boost::tribool CWebsocket::parse(const unsigned char *begin, size_t size, size_t &bytes_consumed, bool &keep_alive)
{
	CWebsocketFrame frame;
	if (!frame.Parse(begin, size)) {
		bytes_consumed = 0;
		return boost::indeterminate;
	}
	bytes_consumed = frame.Consumed();
	if (start_new_packet) {
		packet_data.clear();
		last_opcode = frame.Opcode();
	}
	packet_data += frame.Payload();
	if (frame.isFinal()) {
		// packet is ready for packet handler
		start_new_packet = true;
		switch (last_opcode) {
		case opcode_continuation:
			// shouldnt occur here
			return false;
			break;
		case opcode_text:
			OnReceiveText(packet_data);
			return true;
			break;
		case opcode_binary:
			OnReceiveBinary(packet_data);
			return true;
			break;
		case opcode_close:
			SendClose("");
			keep_alive = false;
			return false;
			break;
		case opcode_ping:
			SendPong(packet_data);
			return false;
			break;
		case opcode_pong:
			// ignore
			return false;
			break;
		}
	}
	// packet waits for more fragments
	start_new_packet = false;
	return false;
}

void CWebsocket::OnReceiveText(const std::string &packet_data)
{
	// todo: packet_data is get-request-data???
	// for test, we echo the data back
	std::string frame = CreateFrame(opcode_text, packet_data);
	conn->MyWrite(frame);
}

void CWebsocket::OnReceiveBinary(const std::string &packet_data)
{
	// the binary protocol is as follows:
	// 1st byte gzip indicator (0 or 1).
	// if 0, next bytes are text
	// if 1, next bytes are gzipped text
	bool isGzipped = ((char)packet_data.at(0) != 0);
	if (isGzipped) {
		// todo: unzip the data
	}
	else {
		OnReceiveText(packet_data.substr(1));
	}
}

void CWebsocket::SendPong(const std::string &packet_data)
{
	std::string frame = CreateFrame(opcode_pong, packet_data);
	conn->MyWrite(frame);
}

void CWebsocket::SendClose(const std::string &packet_data)
{
	std::string frame = CreateFrame(opcode_close, packet_data);
	conn->MyWrite(frame);
}

}
}