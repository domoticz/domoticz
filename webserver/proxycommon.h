#pragma once
#ifndef NOCLOUD
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#define SIZE_SIZE_T 4
#define SIZE_INT 4

typedef enum {
	PDU_NONE = 0,
	PDU_AUTHENTICATE,
	PDU_AUTHRESP,
	PDU_ASSIGNKEY,
	PDU_ENQUIRE,
	PDU_REQUEST,
	PDU_RESPONSE,
	PDU_SIGNOFF,
	PDU_SERV_CONNECT,
	PDU_SERV_CONNECTRESP,
	PDU_SERV_RECEIVE,
	PDU_SERV_SEND,
	PDU_SERV_DISCONNECT,
	PDU_SERV_ROSTERIND,
	PDU_WS_OPEN,
	PDU_WS_CLOSE,
	PDU_WS_SEND,
	PDU_WS_RECEIVE,
} pdu_type;

#define SUBSYSTEM_HTTP 0x01
#define SUBSYSTEM_SHAREDDOMOTICZ 0x02
#define SUBSYSTEM_APPS 0x04

class CValueLengthPart;

class ProxyPdu {
public:
	ProxyPdu(pdu_type type, const char *data, size_t theLength);
	ProxyPdu(const char *data, size_t theLength);
	void InitPdu(pdu_type type, const char *data, size_t theLength);
	ProxyPdu(pdu_type type, CValueLengthPart *part);
	~ProxyPdu();
	std::string Serialize();
	int ReadPdu(const char *buffer, size_t buflen);
	void ParsePdu();
	size_t length();
	void *content();
	int Disconnected();
	pdu_type _type;
	boost::asio::ip::tcp::socket *socket;
	unsigned char *pdudata;
	int verbose;
	int disconnected;
	char signature[5];
	size_t _length;
	void *_content;
};

class CValueLengthPart {
public:
	CValueLengthPart();
	explicit CValueLengthPart(ProxyPdu *pdu);
	explicit CValueLengthPart(ProxyPdu &pdu);
	CValueLengthPart(void *data, size_t len);
	~CValueLengthPart();
	void AddPart(void *data, size_t len);
	void AddValue(void *data, size_t len);
	void AddLong(long value);
	void GetPdu(void **data, size_t *len);
	int GetNextPart(void **data, size_t *len);
	int GetNextValue(void *data, size_t *len);
	int GetNextLong(long &value);
	int GetNextPart(std::string &nextpart, bool isstring = true);
	void AddPart(const std::string &nextpart, bool isstring = true);
	void *_data;
	void *_ptr;
	size_t _len;
};
#endif
