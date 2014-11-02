//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolRisingSun.h"
#include <string>
#include "common/Strings.h"

int ProtocolRisingSun::methods() const {
	if (TelldusCore::comparei(model(), L"selflearning")) {
		return (TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_LEARN);
	}
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF;
}

std::string ProtocolRisingSun::getStringForMethod(int method, unsigned char data, Controller *controller) {
	if (TelldusCore::comparei(model(), L"selflearning")) {
		return getStringSelflearning(method);
	}
	return getStringCodeSwitch(method);
}

std::string ProtocolRisingSun::getStringSelflearning(int method) {
	int intHouse = this->getIntParameter(L"house", 1, 33554432)-1;
	int intCode = this->getIntParameter(L"code", 1, 16)-1;

	const char code_on[][7] = {
		"110110", "001110", "100110", "010110",
		"111001", "000101", "101001", "011001",
		"110000", "001000", "100000", "010000",
		"111100", "000010", "101100", "011100"
	};
	const char code_off[][7] = {
		"111110", "000001", "101110", "011110",
		"110101", "001101", "100101", "010101",
		"111000", "000100", "101000", "011000",
		"110010", "001010", "100010", "010010"
	};
	const char l = 120;
	const char s = 51;

	std::string strCode = "10";
	int code = intCode;
	code = (code < 0 ? 0 : code);
	code = (code > 15 ? 15 : code);
	if (method == TELLSTICK_TURNON) {
		strCode.append(code_on[code]);
	} else if (method == TELLSTICK_TURNOFF) {
		strCode.append(code_off[code]);
	} else if (method == TELLSTICK_LEARN) {
		strCode.append(code_on[code]);
	} else {
		return "";
	}

	int house = intHouse;
	for(int i = 0; i < 25; ++i) {
		if (house & 1) {
			strCode.append(1, '1');
		} else {
			strCode.append(1, '0');
		}
		house >>= 1;
	}

	std::string strReturn;
	for(unsigned int i = 0; i < strCode.length(); ++i) {
		if (strCode[i] == '1') {
			strReturn.append(1, l);
			strReturn.append(1, s);
		} else {
			strReturn.append(1, s);
			strReturn.append(1, l);
		}
	}

	std::string prefix = "P";
	prefix.append(1, 5);
	if (method == TELLSTICK_LEARN) {
		prefix.append("R");
		prefix.append( 1, 50 );
	}
	prefix.append("S");
	strReturn.insert(0, prefix);
	strReturn.append(1, '+');
	return strReturn;
}

std::string ProtocolRisingSun::getStringCodeSwitch(int method) {
	std::string strReturn = "S.e";
	strReturn.append(getCodeSwitchTuple(this->getIntParameter(L"house", 1, 4)-1));
	strReturn.append(getCodeSwitchTuple(this->getIntParameter(L"unit", 1, 4)-1));
	if (method == TELLSTICK_TURNON) {
		strReturn.append("e..ee..ee..ee..e+");
	} else if (method == TELLSTICK_TURNOFF) {
		strReturn.append("e..ee..ee..e.e.e+");
	} else {
		return "";
	}
	return strReturn;
}

std::string ProtocolRisingSun::getCodeSwitchTuple(int intToConvert) {
	std::string strReturn = "";
	for(int i = 0; i < 4; ++i) {
		if (i == intToConvert) {
			strReturn.append( ".e.e" );
		} else {
			strReturn.append( "e..e" );
		}
	}
	return strReturn;
}
