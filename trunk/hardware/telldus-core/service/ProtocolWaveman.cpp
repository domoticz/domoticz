//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolWaveman.h"
#ifdef _MSC_VER
#else
#include <stdint.h>
#endif
#include <stdio.h>
#include <sstream>
#include <string>

int ProtocolWaveman::lastArctecCodeSwitchWasTurnOff = 0;

int ProtocolWaveman::methods() const {
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF;
}

std::string ProtocolWaveman::getStringForMethod(int method, unsigned char, Controller *) {
	return getStringCodeSwitch(method);
}

std::string ProtocolWaveman::getOffCode() const {
	return "$k$k$k$k$k$k$k$k$k+";
}

std::string ProtocolWaveman::decodeData(const ControllerMessage& dataMsg) {
	uint64_t allData = 0;
	unsigned int house = 0;
	unsigned int unit = 0;
	unsigned int method = 0;

	allData = dataMsg.getInt64Parameter("data");

	method = allData & 0xF00;
	method >>= 8;

	unit = allData & 0xF0;
	unit >>= 4;
	unit++;

	house = allData & 0xF;

	if(house > 16 || unit < 1 || unit > 16) {
		// not waveman
		return "";
	}

	house = house + 'A';  // house from A to P

	if(method != 6 && lastArctecCodeSwitchWasTurnOff == 1) {
		lastArctecCodeSwitchWasTurnOff = 0;
		return "";  // probably a stray turnon or bell	(perhaps: only certain time interval since last, check that it's the same house/unit... Will lose
						// one turnon/bell, but it's better than the alternative...
	}

	if(method == 6) {
		lastArctecCodeSwitchWasTurnOff = 1;
	}

	std::stringstream retString;
	retString << "class:command;protocol:waveman;model:codeswitch;house:" << static_cast<char>(house);

	if(method == 0) {
		retString << ";unit:" << unit << ";method:turnoff;";
	} else if(method == 14) {
		retString << ";unit:" << unit << ";method:turnon;";
	}	else {
		// not waveman
		return "";
	}

	return retString.str();
}
