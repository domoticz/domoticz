//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolHasta.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include "common/Strings.h"

int ProtocolHasta::methods() const {
	return TELLSTICK_UP | TELLSTICK_DOWN | TELLSTICK_STOP | TELLSTICK_LEARN;
}

std::string ProtocolHasta::getStringForMethod(int method, unsigned char, Controller *) {
	if (TelldusCore::comparei(model(), L"selflearningv2")) {
		return getStringForMethodv2(method);
	}
	return getStringForMethodv1(method);
}

std::string ProtocolHasta::getStringForMethodv1(int method) {
	int house = this->getIntParameter(L"house", 1, 65536);
	int unit = this->getIntParameter(L"unit", 1, 15);
	std::string strReturn;

	strReturn.append(1, 164);
	strReturn.append(1, 1);
	strReturn.append(1, 164);
	strReturn.append(1, 1);
	strReturn.append(1, 164);
	strReturn.append(1, 164);

	strReturn.append(convertByte( (house & 0xFF) ));
	strReturn.append(convertByte( (house>>8) & 0xFF ));

	int byte = unit&0x0F;

	if (method == TELLSTICK_UP) {
		byte |= 0x00;

	} else if (method == TELLSTICK_DOWN) {
		byte |= 0x10;

	} else if (method == TELLSTICK_STOP) {
		byte |= 0x50;

	} else if (method == TELLSTICK_LEARN) {
		byte |= 0x40;

	} else {
		return "";
	}
	strReturn.append(convertByte(byte));

	strReturn.append(convertByte(0x0));
	strReturn.append(convertByte(0x0));

	// Remove the last pulse
	strReturn.erase(strReturn.end()-1, strReturn.end());

	return strReturn;
}

std::string ProtocolHasta::convertByte(unsigned char byte) {
	std::string retval;
	for(int i = 0; i < 8; ++i) {
		if (byte & 1) {
			retval.append(1, 33);
			retval.append(1, 17);
		} else {
			retval.append(1, 17);
			retval.append(1, 33);
		}
		byte >>= 1;
	}
	return retval;
}

std::string ProtocolHasta::getStringForMethodv2(int method) {
	int house = this->getIntParameter(L"house", 1, 65536);
	int unit = this->getIntParameter(L"unit", 1, 15);
	int sum = 0;
	std::string strReturn;
	strReturn.append(1, 245);
	strReturn.append(1, 1);
	strReturn.append(1, 245);
	strReturn.append(1, 245);
	strReturn.append(1, 63);
	strReturn.append(1, 1);
	strReturn.append(1, 63);
	strReturn.append(1, 1);
	strReturn.append(1, 35);
	strReturn.append(1, 35);

	strReturn.append(convertBytev2( (house>>8) & 0xFF ));
	sum = ((house>>8)&0xFF);
	strReturn.append(convertBytev2( (house & 0xFF) ));
	sum += (house & 0xFF);

	int byte = unit&0x0F;

	if (method == TELLSTICK_UP) {
		byte |= 0xC0;

	} else if (method == TELLSTICK_DOWN) {
		byte |= 0x10;

	} else if (method == TELLSTICK_STOP) {
		byte |= 0x50;

	} else if (method == TELLSTICK_LEARN) {
		byte |= 0x40;

	} else {
		return "";
	}
	strReturn.append(convertBytev2(byte));
	sum += byte;

	strReturn.append(convertBytev2(0x01));
	sum += 0x01;

	int checksum = ((static_cast<int>(sum/256)+1)*256+1) - sum;
	strReturn.append(convertBytev2(checksum));
	strReturn.append(1, 63);
	strReturn.append(1, 35);

	return strReturn;
}

std::string ProtocolHasta::convertBytev2(unsigned char byte) {
	std::string retval;
	for(int i = 0; i < 8; ++i) {
		if (byte & 1) {
			retval.append(1, 63);
			retval.append(1, 35);
		} else {
			retval.append(1, 35);
			retval.append(1, 63);
		}
		byte >>= 1;
	}
	return retval;
}

std::string ProtocolHasta::decodeData(const ControllerMessage& dataMsg) {
	uint64_t allData = dataMsg.getInt64Parameter("data");

	unsigned int house = 0;
	unsigned int unit = 0;
	unsigned int method = 0;
	std::string model;
	std::string methodstring;

	allData >>= 8;
	unit = allData & 0xF;
	allData >>= 4;
	method = allData & 0xF;
	allData >>= 4;
	if(TelldusCore::comparei(dataMsg.model(), L"selflearning")) {
		// version1
		house = allData & 0xFFFF;
		house = ((house << 8) | (house >> 8)) & 0xFFFF;
		model = "selflearning";
		if(method == 0) {
			methodstring = "up";
		} else if(method == 1) {
			methodstring = "down";
		} else if(method == 5) {
			methodstring = "stop";
		} else {
			return "";
		}
	} else {
		// version2
		house = allData & 0xFFFF;

		model = "selflearningv2";
		if(method == 12) {
			methodstring = "up";
		} else if(method == 1 || method == 8) {  // is method 8 correct?
			methodstring = "down";
		} else if(method == 5) {
			methodstring = "stop";
		} else {
			return "";
		}
	}

	if(house < 1 || house > 65535 || unit < 1 || unit > 16) {
		// not hasta
		return "";
	}

	std::stringstream retString;
	retString << "class:command;protocol:hasta;model:" << model << ";house:" << house << ";unit:" << unit << ";method:" << methodstring << ";";
	return retString.str();
}
