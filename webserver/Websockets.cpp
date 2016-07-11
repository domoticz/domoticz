#include "stdafx.h"
#include "Websockets.hpp"
#include <boost/lexical_cast.hpp>
// debug
#include "../main/Logger.h"

namespace http {
	namespace server {

		std::string CreateFrame(opcodes opcode, const std::string &payload)
		{
			// note: we dont do masking nor fragmentation
			const unsigned char FIN_MASK = 0x80;
			const unsigned char OPCODE_MASK = 0x0f;
			size_t_t payloadlen = payload.size();
			std::string res = "";
			res += ((unsigned char)(FIN_MASK + (((unsigned char)opcode) & OPCODE_MASK)));
			if (payloadlen < 126) {
				res += (unsigned char)payloadlen;
			}
			else {
				if (payloadlen <= 0xffff) {
					res += (unsigned char)126;
					int bits = 16;
					while (bits) {
						bits -= 8;
						res += (unsigned char)((payloadlen >> bits) & 0xff);
					}
				}
				else {
					res += (unsigned char)127;
					int bits = 64;
					while (bits) {
						bits -= 8;
						res += (unsigned char)((payloadlen >> bits) & 0xff);
					}
				}
			}
			res += payload;
			return res;
		}

CWebsocketFrame::CWebsocketFrame() { };
CWebsocketFrame::~CWebsocketFrame() {};

std::string CWebsocketFrame::unmask(const unsigned char *mask, const unsigned char *bytes, size_t_t payloadlen) {
	std::string result;
	result.resize(payloadlen);
	for (size_t_t i = 0; i < payloadlen; i++) {
		result[i] = (unsigned char)(bytes[i] ^ mask[i % 4]);
	}
	return result;
}

std::string CWebsocketFrame::Create(opcodes opcode, const std::string &payload)
{
	size_t_t payloadlen = payload.length();
	std::string res;
	// byte 0
	res += ((unsigned char)opcode | FIN_MASK);
	if (payloadlen < 126) {
		res += (unsigned char)payloadlen | MASKING_MASK;
	}
	else {
		if (payloadlen <= 0xffff) {
			res += (unsigned char)126 | MASKING_MASK;
			int bits = 16;
			while (bits) {
				bits -= 8;
				res += (unsigned char)((payloadlen >> bits) & 0xff);
			}
		}
		else {
			res += (unsigned char)127 | MASKING_MASK;
			int bits = 64;
			while (bits) {
				bits -= 8;
				unsigned char ch = (unsigned char)((size_t_t)(payloadlen >> bits) & 0xff);
				res += (unsigned char)((size_t_t)(payloadlen >> bits) & 0xff);
			}
		}
	}
	// masking key
	for (unsigned i = 0; i < 4; i++) {
		masking_key[i] = rand();
		res += masking_key[i];
	}
	res += unmask(masking_key, (const unsigned char *)payload.c_str(), payloadlen);
	return res;
}

bool CWebsocketFrame::Parse(const unsigned char *bytes, size_t_t size) {
	size_t_t remaining = size;
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
	size_t_t ptr = 2;
	if (payloadlen == 126) {
		if (remaining < 2) {
			return false;
		}
		payloadlen = 0;
		int bits = 16;
		for (unsigned int i = 0; i < 2; i++) {
			bits -= 8;
			payloadlen += (size_t_t)bytes[ptr++] << bits;
			remaining--;
		}
	}
	else if (payloadlen == 127) {
		if (remaining < 8) {
			return false;
		}
		payloadlen = 0;
		int bits = 64;
		for (unsigned int i = 0; i < 8; i++) {
			bits -= 8;
			payloadlen += (size_t_t)bytes[ptr++] << bits;
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

std::string CWebsocketFrame::Payload() {
	return payload;
};

bool CWebsocketFrame::isFinal() {
	return fin;
};

size_t_t CWebsocketFrame::Consumed() {
	return bytes_consumed;
};

opcodes CWebsocketFrame::Opcode() {
	return opcode;
};

CWebsocket::CWebsocket(boost::function<void(const std::string &packet_data)> _MyWrite, CWebsocketHandler *handler) : m_Push(this)
{
	start_new_packet = true;
	MyWrite = _MyWrite;
}

CWebsocket::~CWebsocket()
{
	m_Push.Stop();
}

boost::tribool CWebsocket::parse(const unsigned char *begin, size_t_t size, size_t_t &bytes_consumed, bool &keep_alive)
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
	m_Push.Start();
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
			OnPong(packet_data);
			return false;
			break;
		}
	}
	// packet waits for more fragments
	start_new_packet = false;
	return false;
}

// we receive a json request here. the final format is still to be decided
// todo: move the body of this function to the websocket handler, so it can be
// re-used from the proxy client
// note: We mimic a web request here. This is just for testing purposes to see
//       if everything works. We need a proper implementation here.
void CWebsocket::OnReceiveText(const std::string &packet_data)
{
	boost::tribool result = handler->Handle(packet_data);
}

void CWebsocket::OnReceiveBinary(const std::string &packet_data)
{
	// we assume we received a gzipped json request
	// todo: unzip the data
	std::string the_data = packet_data;
	OnReceiveText(the_data);
}

void CWebsocket::SendPing()
{
	// todo: set the ping timer
	std::string frame = CreateFrame(opcode_ping, OUR_PING_ID);
	MyWrite(frame);
}

void CWebsocket::OnPong(const std::string &packet_data)
{
	if (packet_data == OUR_PING_ID) {
		// todo: this was a response to one of our pings. reset the ping timer.
	}
}

void CWebsocket::SendPong(const std::string &packet_data)
{
	std::string frame = CreateFrame(opcode_pong, packet_data);
	MyWrite(frame);
}

void CWebsocket::SendClose(const std::string &packet_data)
{
	std::string frame = CreateFrame(opcode_close, packet_data);
	MyWrite(frame);
}

void CWebsocket::OnDeviceChanged(const unsigned long long DeviceRowIdx)
{
	std::string query = "type=devices&rid=" + boost::lexical_cast<std::string>(DeviceRowIdx);
	std::string packet = "\"-1/" + query + "\"";
	OnReceiveText(packet);
}

void CWebsocket::OnMessage(const std::string & Subject, const std::string & Text, const std::string & ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)
{
# if 0
	Json::Value json;

	json["event"] = "notification";
	json["Text"] = Text;
	// todo: other parameters
	std::string response = json.toStyledString();
	std::string frame = CreateFrame(opcode_text, response);
	MyWrite(frame);
#endif
}

}
}