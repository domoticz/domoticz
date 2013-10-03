//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolSartano.h"
#ifdef _MSC_VER
typedef unsigned __int16 uint16_t;
#else
#include <stdint.h>
#endif
#include <stdio.h>
#include <sstream>
#include <string>

int ProtocolSartano::methods() const {
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF;
}

std::string ProtocolSartano::getStringForMethod(int method, unsigned char, Controller *) {
	std::wstring strCode = this->getStringParameter(L"code", L"");
	return getStringForCode(strCode, method);
}

std::string ProtocolSartano::getStringForCode(const std::wstring &strCode, int method) {
	std::string strReturn("S");

	for (size_t i = 0; i < strCode.length(); ++i) {
		if (strCode[i] == L'1') {
			strReturn.append("$k$k");
		} else {
			strReturn.append("$kk$");
		}
	}

	if (method == TELLSTICK_TURNON) {
		strReturn.append("$k$k$kk$$k+");
	} else if (method == TELLSTICK_TURNOFF) {
		strReturn.append("$kk$$k$k$k+");
	} else {
		return "";
	}

	return strReturn;
}

std::string ProtocolSartano::decodeData(const ControllerMessage &dataMsg) {
	uint64_t allDataIn;
	uint16_t allData = 0;
	unsigned int code = 0;
	unsigned int method1 = 0;
	unsigned int method2 = 0;
	unsigned int method = 0;

	allDataIn = dataMsg.getInt64Parameter("data");

	uint16_t mask = (1<<11);
	for(int i = 0; i < 12; ++i) {
		allData >>= 1;
		if((allDataIn & mask) == 0) {
			allData |= (1<<11);
		}
		mask >>= 1;
	}

	code = allData & 0xFFC;
	code >>= 2;

	method1 = allData & 0x2;
	method1 >>= 1;

	method2 = allData & 0x1;

	if(method1 == 0 && method2 == 1) {
		method = 0;  // off
	} else if(method1 == 1 && method2 == 0) {
		method = 1;  // on
	} else {
		return "";
	}

	if(code > 1023) {
		// not sartano
		return "";
	}

	std::stringstream retString;
	retString << "class:command;protocol:sartano;model:codeswitch;code:";
	mask = (1<<9);
	for(int i = 0; i < 10; i++) {
		if((code & mask) != 0) {
			retString << 1;
		} else {
			retString << 0;
		}
		mask >>= 1;
	}
	retString << ";method:";

	if(method == 0) {
		retString << "turnoff;";
	} else {
		retString << "turnon;";
	}

	return retString.str();
}
