//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolNexa.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include "service/TellStick.h"
#include "common/Strings.h"

int ProtocolNexa::lastArctecCodeSwitchWasTurnOff = 0;  // TODO(stefan): always removing first turnon now, make more flexible (waveman too)

int ProtocolNexa::methods() const {
	if (TelldusCore::comparei(model(), L"codeswitch")) {
		return (TELLSTICK_TURNON | TELLSTICK_TURNOFF);

	} else if (TelldusCore::comparei(model(), L"selflearning-switch")) {
		return (TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_LEARN);

	} else if (TelldusCore::comparei(model(), L"selflearning-dimmer")) {
		return (TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_DIM | TELLSTICK_LEARN);

	} else if (TelldusCore::comparei(model(), L"bell")) {
		return TELLSTICK_BELL;
	}
	return 0;
}

std::string ProtocolNexa::getStringForMethod(int method, unsigned char data, Controller *controller) {
	if (TelldusCore::comparei(model(), L"codeswitch")) {
		return getStringCodeSwitch(method);
	} else if (TelldusCore::comparei(model(), L"bell")) {
		return getStringBell();
	}
	if ((method == TELLSTICK_TURNON) && TelldusCore::comparei(model(), L"selflearning-dimmer")) {
		// Workaround for not letting a dimmer do into "dimming mode"
		return getStringSelflearning(TELLSTICK_DIM, 255);
	}
	if (method == TELLSTICK_LEARN) {
		std::string str = getStringSelflearning(TELLSTICK_TURNON, data);

		// Check to see if we are an old TellStick (fw <= 2, batch <= 8)
		TellStick *ts = reinterpret_cast<TellStick *>(controller);
		if (!ts) {
			return str;
		}
		if (ts->pid() == 0x0c30 && ts->firmwareVersion() <= 2) {
			// Workaround for the bug in early firmwares
			// The TellStick have a fixed pause (max) between two packets.
			// It is only correct between the first and second packet.
			// It seems faster to send two packes at a time and some
			// receivers seems picky about this when learning.
			// We also return the last packet so Device::doAction() doesn't
			// report TELLSTICK_ERROR_METHOD_NOT_SUPPORTED

			str.insert(0, 1, 2);  // Repeat two times
			str.insert(0, 1, 'R');
			for (int i = 0; i < 5; ++i) {
				controller->send(str);
			}
		}
		return str;
	}
	return getStringSelflearning(method, data);
}

std::string ProtocolNexa::getStringCodeSwitch(int method) {
	std::string strReturn = "S";

	std::wstring house = getStringParameter(L"house", L"A");
	int intHouse = house[0] - L'A';
	strReturn.append(getCodeSwitchTuple(intHouse));
	strReturn.append(getCodeSwitchTuple(getIntParameter(L"unit", 1, 16)-1));

	if (method == TELLSTICK_TURNON) {
		strReturn.append("$k$k$kk$$kk$$kk$$k+");
	} else if (method == TELLSTICK_TURNOFF) {
		strReturn.append(this->getOffCode());
	} else {
		return "";
	}
	return strReturn;
}

std::string ProtocolNexa::getStringBell() {
	std::string strReturn = "S";

	std::wstring house = getStringParameter(L"house", L"A");
	int intHouse = house[0] - L'A';
	strReturn.append(getCodeSwitchTuple(intHouse));
	strReturn.append("$kk$$kk$$kk$$k$k");  // Unit 7
	strReturn.append("$kk$$kk$$kk$$kk$$k+");  // Bell
	return strReturn;
}

std::string ProtocolNexa::getStringSelflearning(int method, unsigned char level) {
	int intHouse = getIntParameter(L"house", 1, 67108863);
	int intCode = getIntParameter(L"unit", 1, 16)-1;
	return getStringSelflearningForCode(intHouse, intCode, method, level);
}

std::string ProtocolNexa::getStringSelflearningForCode(int intHouse, int intCode, int method, unsigned char level) {
	const unsigned char START[] = {'T', 127, 255, 24, 1, 0};
	// const char START[] = {'T',130,255,26,24,0};

	std::string strMessage(reinterpret_cast<const char*>(START));
	strMessage.append(1, (method == TELLSTICK_DIM ? 147 : 132));  // Number of pulses

	std::string m;
	for (int i = 25; i >= 0; --i) {
		m.append( intHouse & 1 << i ? "10" : "01" );
	}
	m.append("01");  // Group

	// On/off
	if (method == TELLSTICK_DIM) {
		m.append("00");
	} else if (method == TELLSTICK_TURNOFF) {
		m.append("01");
	} else if (method == TELLSTICK_TURNON) {
		m.append("10");
	} else {
		return "";
	}

	for (int i = 3; i >= 0; --i) {
		m.append( intCode & 1 << i ? "10" : "01" );
	}

	if (method == TELLSTICK_DIM) {
		unsigned char newLevel = level/16;
		for (int i = 3; i >= 0; --i) {
			m.append(newLevel & 1 << i ? "10" : "01");
		}
	}

	// The number of data is odd.
	// Add this to make it even, otherwise the following loop will not work
	m.append("0");

	unsigned char code = 9;  // b1001, startcode
	for (unsigned int i = 0; i < m.length(); ++i) {
		code <<= 4;
		if (m[i] == '1') {
			code |= 8;  // b1000
		} else {
			code |= 10;  // b1010
			// code |= 11; //b1011
		}
		if (i % 2 == 0) {
			strMessage.append(1, code);
			code = 0;
		}
	}
	strMessage.append("+");

// 	for( int i = 0; i < strMessage.length(); ++i ) {
// 		printf("%i,", (unsigned char)strMessage[i]);
// 	}
// 	printf("\n");
	return strMessage;
}

std::string ProtocolNexa::decodeData(const ControllerMessage& dataMsg) {
	uint64_t allData = dataMsg.getInt64Parameter("data");

	if(TelldusCore::comparei(dataMsg.model(), L"selflearning")) {
		// selflearning
		return decodeDataSelfLearning(allData);
	} else {
		// codeswitch
		return decodeDataCodeSwitch(allData);
	}
}

std::string ProtocolNexa::decodeDataSelfLearning(uint64_t allData) {
	unsigned int house = 0;
	unsigned int unit = 0;
	unsigned int group = 0;
	unsigned int method = 0;

	house = allData & 0xFFFFFFC0;
	house >>= 6;

	group = allData & 0x20;
	group >>= 5;

	method = allData & 0x10;
	method >>= 4;

	unit = allData & 0xF;
	unit++;

	if(house < 1 || house > 67108863 || unit < 1 || unit > 16) {
		// not arctech selflearning
		return "";
	}

	std::stringstream retString;
	retString << "class:command;protocol:arctech;model:selflearning;house:" << house << ";unit:" << unit << ";group:" << group << ";method:";
	if(method == 1) {
		retString << "turnon;";
	} else if(method == 0) {
		retString << "turnoff;";
	} else {
		// not arctech selflearning
		return "";
	}

	return retString.str();
}

std::string ProtocolNexa::decodeDataCodeSwitch(uint64_t allData) {
	unsigned int house = 0;
	unsigned int unit = 0;
	unsigned int method = 0;

	method = allData & 0xF00;
	method >>= 8;

	unit = allData & 0xF0;
	unit >>= 4;
	unit++;

	house = allData & 0xF;

	if(house > 16 || unit < 1 || unit > 16) {
		// not arctech codeswitch
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
	retString << "class:command;protocol:arctech;model:codeswitch;house:" << static_cast<char>(house);

	if(method == 6) {
		retString << ";unit:" << unit << ";method:turnoff;";
	} else if(method == 14) {
		retString << ";unit:" << unit << ";method:turnon;";
	} else if(method == 15) {
		retString << ";method:bell;";
	} else {
		// not arctech codeswitch
		return "";
	}

	return retString.str();
}

std::string ProtocolNexa::getCodeSwitchTuple(int intCode) {
	std::string strReturn = "";
	for( int i = 0; i < 4; ++i ) {
		if (intCode & 1) {  // Convert 1
			strReturn.append("$kk$");
		} else {  // Convert 0
			strReturn.append("$k$k");
		}
		intCode >>= 1;
	}
	return strReturn;
}

std::string ProtocolNexa::getOffCode() const {
	return "$k$k$kk$$kk$$k$k$k+";
}
