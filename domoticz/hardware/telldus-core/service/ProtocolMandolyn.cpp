//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolMandolyn.h"
#include <stdlib.h>
#include <iomanip>
#include <sstream>
#include <string>
#include "common/Strings.h"

std::string ProtocolMandolyn::decodeData(const ControllerMessage &dataMsg) {
	std::string data = dataMsg.getParameter("data");
	uint32_t value = (uint32_t)TelldusCore::hexTo64l(data);

	// parity not used
	// bool parity = value & 0x1;
	value >>= 1;

	double temp = static_cast<double>(value & 0x7FFF) - static_cast<double>(6400);
	temp = temp/128.0;
	value >>= 15;

	uint8_t humidity = (value & 0x7F);
	value >>= 7;

	// battOk not used
	// bool battOk = value & 0x1;
	value >>= 3;

	uint8_t channel = (value & 0x3)+1;
	value >>= 2;

	uint8_t house = value & 0xF;

	std::stringstream retString;
	retString << "class:sensor;protocol:mandolyn;id:"
		<< house*10+channel
		<< ";model:temperaturehumidity;"
		<< "temp:" << std::fixed << std::setprecision(1) << temp
		<< ";humidity:" << static_cast<int>(humidity) << ";";

	return retString.str();
}
