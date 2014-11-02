//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolUpm.h"
#include <string>

int ProtocolUpm::methods() const {
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_LEARN;
}

std::string ProtocolUpm::getStringForMethod(int method, unsigned char, Controller *) {
	const char S = ';';
	const char L = '~';
	const char START[] = {S, 0};
	const char B1[] = {L, S, 0};
	const char B0[] = {S, L, 0};
	// const char BON[] = {S,L,L,S,0};
	// const char BOFF[] = {S,L,S,L,0};

	int intUnit = this->getIntParameter(L"unit", 1, 4)-1;
	std::string strReturn;

	int code = this->getIntParameter(L"house", 0, 4095);
	for( size_t i = 0; i < 12; ++i ) {
		if (code & 1) {
			strReturn.insert(0, B1);
		} else {
			strReturn.insert(0, B0);
		}
		code >>= 1;
	}
	strReturn.insert(0, START);  // Startcode, first

	code = 0;
	if (method == TELLSTICK_TURNON || method == TELLSTICK_LEARN) {
		code += 2;
	} else if (method != TELLSTICK_TURNOFF) {
		return "";
	}
	code <<= 2;
	code += intUnit;

	int check1 = 0, check2 = 0;
	for( size_t i = 0; i < 6; ++i ) {
		if (code & 1) {
			if (i % 2 == 0) {
				check1++;
			} else {
				check2++;
			}
		}
		if (code & 1) {
			strReturn.append(B1);
		} else {
			strReturn.append(B0);
		}
		code >>= 1;
	}

	if (check1 % 2 == 0) {
		strReturn.append(B0);
	} else {
		strReturn.append(B1);
	}
	if (check2 % 2 == 0) {
		strReturn.append(B0);
	} else {
		strReturn.append(B1);
	}

	strReturn.insert(0, "S");
	strReturn.append("+");
	return strReturn;
}

