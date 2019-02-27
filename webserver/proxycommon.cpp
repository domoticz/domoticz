#include "stdafx.h"
#ifndef NOCLOUD
#include "proxycommon.h"
#include "../main/Logger.h"

CValueLengthPart::CValueLengthPart(ProxyPdu *pdu)
{
	size_t len = pdu->length();
	_ptr = _data = malloc(len);
	if (_ptr == NULL) {
		_log.Log(LOG_ERROR, "CValueLengthPart (1): Could not allocate.");
		_len = 0;
		return;
	}
	memcpy(_data, pdu->content(), len);
	_len = len;
}

CValueLengthPart::CValueLengthPart(ProxyPdu &pdu)
{
	size_t len = pdu.length();
	_ptr = _data = malloc(len);
	if (_ptr == NULL) {
		_log.Log(LOG_ERROR, "CValueLengthPart (1): Could not allocate.");
		_len = 0;
		return;
	}
	memcpy(_data, pdu.content(), len);
	_len = len;
}

CValueLengthPart::CValueLengthPart()
{
	_data = _ptr = NULL;
	_len = 0;
}

CValueLengthPart::CValueLengthPart(void *data, size_t len)
{
	_ptr = _data = malloc(len);
	if (_ptr == NULL) {
		_log.Log(LOG_ERROR, "CValueLengthPart (2): Could not allocate.");
	}
	memcpy(_data, data, len);
	_len = len;
}

CValueLengthPart::~CValueLengthPart()
{
	if (_data) free(_data);
}

void CValueLengthPart::AddPart(void *data, size_t len)
{
	size_t newlen = _len + len + SIZE_SIZE_T;
	_data = realloc(_data, newlen);
	if (_data == NULL) {
		_log.Log(LOG_ERROR, "AddPart: Could not alloc.");
	}
	unsigned char *dataptr = (unsigned char*)_data + _len;
	size_t len2 = len;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		dataptr[i] = len2 % 256;
		len2 /= 256;
	}
	dataptr += SIZE_SIZE_T;
	memcpy((void *)dataptr, data, len);
	_ptr = _data;
	_len = newlen;
}

int CValueLengthPart::GetNextPart(std::string &nextpart, bool isstring)
{
	char *part;
	size_t length;

	int res = GetNextPart((void **)&part, &length);
	if (res) {
		if (isstring && length > 0 && part[length - 1] == 0) {
			length--;
		}
		nextpart = std::string(part, length);
		free(part);
	}
	return res;
}

void CValueLengthPart::AddPart(const std::string &nextpart, bool isstring)
{
	AddPart((void *)nextpart.c_str(), nextpart.length() + (isstring ? 1 : 0));
}

void CValueLengthPart::AddValue(void *data, size_t len)
{
	size_t newlen = _len + len + SIZE_SIZE_T;
	_data = realloc(_data, newlen);
	if (_data == NULL) {
		_log.Log(LOG_ERROR, "AddValue: Could not alloc.");
	}
	unsigned char *dataptr = (unsigned char*)_data + _len;
	size_t len2 = len;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		dataptr[i] = len2 % 256;
		len2 /= 256;
	}
	dataptr += SIZE_SIZE_T;
	memcpy((void *)dataptr, data, len);
	_ptr = _data;
	_len = newlen;
}

void CValueLengthPart::AddLong(long value)
{
	const size_t len = SIZE_SIZE_T;
	size_t newlen = _len + len + SIZE_SIZE_T;
	_data = realloc(_data, newlen);
	if (_data == NULL) {
		_log.Log(LOG_ERROR, "AddValue: Could not alloc.");
	}
	unsigned char *dataptr = (unsigned char*)_data + _len;
	size_t len2 = len;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		dataptr[i] = len2 % 256;
		len2 /= 256;
	}
	dataptr += SIZE_SIZE_T;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		dataptr[i] = value % 256;
		value /= 256;
	}
	_ptr = _data;
	_len = newlen;
}

void CValueLengthPart::GetPdu(void **data, size_t *length)
{
	*data = _data;
	*length = _len;
}

int CValueLengthPart::GetNextPart(void **data, size_t *length)
{
	if (_len < SIZE_SIZE_T) {
		_log.Log(LOG_ERROR, "GetNextPart: _len too small (%d).\n", (int)_len);
		return 0;
	}
	size_t len = 0;
	unsigned char *dataptr = (unsigned char*)_ptr;
	long mult = 1;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		len += dataptr[i] * mult;
		mult *= 256;
	}
	if (_len < SIZE_SIZE_T + len) {
		// TODO: This isnt right
		// something went wrong
		_log.Log(LOG_ERROR, "GetNextPart: _len too small (_len: %d, len: %d).", (int)_len, (int)len);
		return 0;
	}
	(*data) = malloc(len);
	if ((*data) == NULL) {
		_log.Log(LOG_ERROR, "GetNextPart: Could not alloc.");
		return 0;
	}
	memcpy(*data, dataptr + SIZE_SIZE_T, len);
	*length = len;
	_ptr = dataptr + SIZE_SIZE_T + len;
	return 1;
}

int CValueLengthPart::GetNextValue(void *data, size_t *length)
{
	if (_len < SIZE_SIZE_T) {
		return 0;
	}
	size_t len = 0;
	unsigned char *dataptr = (unsigned char*)_ptr;
	long mult = 1;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		len += dataptr[i] * mult;
		mult *= 256;
	}
	if (_len < SIZE_SIZE_T + len) {
		// TODO: This isnt right
		// something went wrong
		return 0;
	}
	memcpy(data, dataptr + SIZE_SIZE_T, len);
	*length = len;
	_ptr = dataptr + SIZE_SIZE_T + len;
	return 1;
}

int CValueLengthPart::GetNextLong(long &value)
{
	if (_len < SIZE_SIZE_T) {
		return 0;
	}
	size_t len = 0;
	unsigned char *dataptr = (unsigned char*)_ptr;
	long mult = 1;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		len += dataptr[i] * mult;
		mult *= 256;
	}
	if (_len < SIZE_SIZE_T + len || len != SIZE_SIZE_T) {
		// TODO: This isnt right
		// something went wrong
		return 0;
	}
	dataptr += SIZE_SIZE_T;
	value = 0;
	mult = 1;
	for (unsigned int i = 0; i < len; i++) {
		value += dataptr[i] * mult;
		mult *= 256;
	}
	_ptr = dataptr + len;
	return 1;
}

ProxyPdu::ProxyPdu(pdu_type type, CValueLengthPart *part)
{
	socket = NULL;
	char *data;
	size_t theLength;
	part->GetPdu((void **)&data, &theLength);
	InitPdu(type, data, theLength);
}

ProxyPdu::ProxyPdu(const char *data, size_t theLength)
{
	socket = NULL;
	verbose = 1;
	_content = pdudata = NULL;
	sprintf(signature, "RKDM");
	disconnected = ReadPdu(data, theLength);
}


ProxyPdu::ProxyPdu(pdu_type type, const char *data, size_t theLength)
{
	socket = NULL;
	InitPdu(type, data, theLength);
}

void ProxyPdu::InitPdu(pdu_type type, const char *data, size_t theLength)
{
	verbose = 1;
	_content = NULL;
	disconnected = 0;
	sprintf(signature, "RKDM");
	_length = theLength + strlen(signature) + SIZE_SIZE_T + 1;
	pdudata = (unsigned char *)malloc(_length);
	if (pdudata == NULL) {
		_log.Log(LOG_ERROR, "InitPdu: Could not alloc.");
	}
	unsigned char *ptr = pdudata;
	memcpy((void *)ptr, (void *)signature, strlen(signature));
	ptr += strlen(signature);
	*ptr++ = (unsigned char)type;
	size_t len2 = theLength;
	for (unsigned int i = 0; i < SIZE_SIZE_T; i++) {
		*ptr++ = len2 % 256;
		len2 /= 256;
	}
	memcpy((void *)ptr, data, theLength);
	_type = type;
}

ProxyPdu::~ProxyPdu()
{
	if (pdudata) free(pdudata);
}

size_t ProxyPdu::length()
{
	return _length;
}

void *ProxyPdu::content()
{
	if (pdudata) {
		return (void *)pdudata;
	}
	return _content;
}

int ProxyPdu::ReadPdu(const char *buffer, size_t buflen)
{
	int datacounter = 0;
	int mult = 1;
	unsigned int lencounter = 0;
	size_t readlen = 0;
	enum {
		STATE_SIGNATURE1,
		STATE_SIGNATURE2,
		STATE_SIGNATURE3,
		STATE_SIGNATURE4,
		STATE_OPCODE,
		STATE_PDULEN,
		STATE_PDUDATA,
		STATE_DONE
	} state;

	_type = PDU_NONE;
	state = STATE_SIGNATURE1;
	while (state != STATE_DONE) {
		if (readlen >= buflen) {
			return 1;
		}
		unsigned char buf = buffer[readlen++];
		switch (state) {
		case STATE_SIGNATURE1:
			if (buf == signature[0]) {
				state = STATE_SIGNATURE2;
			}
			else {
				state = STATE_SIGNATURE1;
			}
			break;
		case STATE_SIGNATURE2:
			if (buf == signature[1]) {
				state = STATE_SIGNATURE3;
			}
			else {
				state = STATE_SIGNATURE1;
			}
			break;
		case STATE_SIGNATURE3:
			if (buf == signature[2]) {
				state = STATE_SIGNATURE4;
			}
			else {
				state = STATE_SIGNATURE1;
			}
			break;
		case STATE_SIGNATURE4:
			if (buf == signature[3]) {
				state = STATE_OPCODE;
			}
			else {
				state = STATE_SIGNATURE1;
			}
			break;
		case STATE_OPCODE:
			_type = (pdu_type)buf;
			mult = 1;
			_length = 0;
			lencounter = 0;
			state = STATE_PDULEN;
			break;
		case STATE_PDULEN:
			_length += buf * mult;
			mult *= 256;
			lencounter++;
			if (lencounter == SIZE_SIZE_T) {
				if (_length == 0) {
					pdudata = NULL;
					state = STATE_DONE;
				}
				else {
					datacounter = 0;
					pdudata = (unsigned char *)malloc(_length);
					if (pdudata == NULL) {
						_log.Log(LOG_ERROR, "ReadPdu: Could not alloc, length=%d.", (int)_length);
					}
					state = STATE_PDUDATA;
				}
			}
			break;
		case STATE_PDUDATA:
			pdudata[datacounter++] = buf;
			if (datacounter == _length) {
				state = STATE_DONE;
			}
			break;
		case STATE_DONE:
			// satisfy compiler
			break;
		}
	}
	return 0;
}

// TODO!
void ProxyPdu::ParsePdu()
{
	if (_length < 1) {
		return;
	}
}

int ProxyPdu::Disconnected()
{
	return disconnected;
}

std::string ProxyPdu::Serialize()
{
	return std::string((char *)content(), length());
}
#endif
