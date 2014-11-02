//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolBrateck.h"
#include <string>

int ProtocolBrateck::methods() const {
	return TELLSTICK_UP | TELLSTICK_DOWN | TELLSTICK_STOP;
}

std::string ProtocolBrateck::getStringForMethod(int method, unsigned char, Controller *) {
	const char S = '!';
	const char L = 'V';
	const char B1[] = {L, S, L, S, 0};
	const char BX[] = {S, L, L, S, 0};
	const char B0[] = {S, L, S, L, 0};
	const char BUP[]   = {L, S, L, S, S, L, S, L, S, L, S, L, S, L, S, L, S, 0};
	const char BSTOP[] = {S, L, S, L, L, S, L, S, S, L, S, L, S, L, S, L, S, 0};
	const char BDOWN[] = {S, L, S, L, S, L, S, L, S, L, S, L, L, S, L, S, S, 0};

	std::string strReturn;
	std::wstring strHouse = this->getStringParameter(L"house", L"");
	if (strHouse == L"") {
		return "";
	}

	for( size_t i = 0; i < strHouse.length(); ++i ) {
		if (strHouse[i] == '1') {
			strReturn.insert(0, B1);
		} else if (strHouse[i] == '-') {
			strReturn.insert(0, BX);
		} else if (strHouse[i] == '0') {
			strReturn.insert(0, B0);
		}
	}

	strReturn.insert(0, "S");
	if (method == TELLSTICK_UP) {
		strReturn.append(BUP);
	} else if (method == TELLSTICK_DOWN) {
		strReturn.append(BDOWN);
	} else if (method == TELLSTICK_STOP) {
		strReturn.append(BSTOP);
	} else {
		return "";
	}
	strReturn.append("+");

	return strReturn;
}
