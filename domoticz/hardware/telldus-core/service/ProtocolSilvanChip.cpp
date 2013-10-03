//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolSilvanChip.h"
#include <string>
#include "common/Strings.h"

int ProtocolSilvanChip::methods() const {
	if (TelldusCore::comparei(model(), L"kp100")) {
		return TELLSTICK_UP | TELLSTICK_DOWN | TELLSTICK_STOP | TELLSTICK_LEARN;
	} else if (TelldusCore::comparei(model(), L"ecosavers")) {
		return TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_LEARN;
	} else if (TelldusCore::comparei(model(), L"displaymatic")) {
		return TELLSTICK_UP | TELLSTICK_DOWN | TELLSTICK_STOP;
	}
	return 0;
}

std::string ProtocolSilvanChip::getStringForMethod(int method, unsigned char data, Controller *controller) {
	if (TelldusCore::comparei(model(), L"kp100")) {
		std::string preamble;
		preamble.append(1, 100);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 100);

		const std::string one = "\xFF\x1\x2E\x2E";
		const std::string zero = "\x2E\xFF\x1\x2E";
		int button = 0;
		if (method == TELLSTICK_UP) {
			button = 2;
		} else if (method == TELLSTICK_DOWN) {
			button = 8;
		} else if (method == TELLSTICK_STOP) {
			button = 4;
		} else if (method == TELLSTICK_LEARN) {
			button = 1;
		} else {
			return "";
		}
		return this->getString(preamble, one, zero, button);
	} else if (TelldusCore::comparei(model(), L"displaymatic")) {
		std::string preamble;
		preamble.append(1, 0x25);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 0x25);
		const std::string one = "\x69\25";
		const std::string zero = "\x25\x69";
		int button = 0;
		if (method == TELLSTICK_UP) {
			button = 1;
		} else if (method == TELLSTICK_DOWN) {
			button = 4;
		} else if (method == TELLSTICK_STOP) {
			button = 2;
		}
		return this->getString(preamble, one, zero, button);
	} else if (TelldusCore::comparei(model(), L"ecosavers")) {
		std::string preamble;
		preamble.append(1, 0x25);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 255);
		preamble.append(1, 1);
		preamble.append(1, 0x25);
		const std::string one = "\x69\25";
		const std::string zero = "\x25\x69";
		int intUnit = this->getIntParameter(L"unit", 1, 4);
		int button = 0;
		if (intUnit == 1) {
			button = 7;
		} else if (intUnit == 2) {
			button = 3;
		} else if (intUnit == 3) {
			button = 5;
		} else if (intUnit == 4) {
			button = 6;
		}

		if (method == TELLSTICK_TURNON || method == TELLSTICK_LEARN) {
			button |= 8;
		}
		return this->getString(preamble, one, zero, button);
	}
	return "";
}

std::string ProtocolSilvanChip::getString(const std::string &preamble, const std::string &one, const std::string &zero, int button) {
	int intHouse = this->getIntParameter(L"house", 1, 1048575);
	std::string strReturn = preamble;

	for( int i = 19; i >= 0; --i ) {
		if (intHouse & (1 << i)) {
			strReturn.append(one);
		} else {
			strReturn.append(zero);
		}
	}

	for( int i = 3; i >= 0; --i) {
		if (button & (1 << i)) {
			strReturn.append(one);
		} else {
			strReturn.append(zero);
		}
	}

	strReturn.append(zero);
	return strReturn;
}
