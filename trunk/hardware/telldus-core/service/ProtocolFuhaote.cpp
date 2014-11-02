//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolFuhaote.h"
#include <string>

int ProtocolFuhaote::methods() const {
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF;
}

std::string ProtocolFuhaote::getStringForMethod(int method, unsigned char, Controller *) {
	const char S = 19;
	const char L = 58;
	const char B0[] = {S, L, L, S, 0};
	const char B1[] = {L, S, L, S, 0};
	const char OFF[] = {S, L, S, L, S, L, L, S, 0};
	const char ON[]  = {S, L, L, S, S, L, S, L, 0};

	std::string strReturn = "S";
	std::wstring strCode = this->getStringParameter(L"code", L"");
	if (strCode == L"") {
		return "";
	}

	// House code
	for(size_t i = 0; i < 5; ++i) {
		if (strCode[i] == '0') {
			strReturn.append(B0);
		} else if (strCode[i] == '1') {
			strReturn.append(B1);
		}
	}
	// Unit code
	for(size_t i = 5; i < 10; ++i) {
		if (strCode[i] == '0') {
			strReturn.append(B0);
		} else if (strCode[i] == '1') {
			strReturn.append(1, S);
			strReturn.append(1, L);
			strReturn.append(1, S);
			strReturn.append(1, L);
		}
	}

	if (method == TELLSTICK_TURNON) {
		strReturn.append(ON);
	} else if (method == TELLSTICK_TURNOFF) {
		strReturn.append(OFF);
	} else {
		return "";
	}

	strReturn.append(1, S);
	strReturn.append("+");
	return strReturn;
}

