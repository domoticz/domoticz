//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolOregon.h"
#include <stdlib.h>
#include <iomanip>
#include <sstream>
#include <string>
#include "common/Strings.h"

std::string ProtocolOregon::decodeData(const ControllerMessage &dataMsg) {
	std::string data = dataMsg.getParameter("data");

	std::wstring model = dataMsg.model();
	if (model.compare(L"0xEA4C") == 0) {
		return decodeEA4C(data);
	} else if (model.compare(L"0x1A2D") == 0) {
		return decode1A2D(data);
	} else if (model.compare(L"0xF824") == 0) {
		return decodeF824(data);
	} else if (model.compare(L"0x1984") == 0 || model.compare(L"0x1994") == 0) {
		return decode1984(data, model);
	} else if (model.compare(L"0x2914") == 0) {
		return decode2914(data);
	}

	return "";
}

std::string ProtocolOregon::decodeEA4C(const std::string &data) {
	uint64_t value = TelldusCore::hexTo64l(data);

	uint8_t checksum = 0xE + 0xA + 0x4 + 0xC;
	checksum -= (value & 0xF) * 0x10;
	checksum -= 0xA;
	value >>= 8;

	uint8_t checksumw = (value >> 4) & 0xF;
	bool neg = value & (1 << 3);
	int hundred = value & 3;
	checksum += (value & 0xF);
	value >>= 8;

	uint8_t temp2 = value & 0xF;
	uint8_t temp1 = (value >> 4) & 0xF;
	checksum += temp2 + temp1;
	value >>= 8;

	uint8_t temp3 = (value >> 4) & 0xF;
	checksum += (value & 0xF) + temp3;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t address = value & 0xFF;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	// channel not used
	// uint8_t channel = (value >> 4) & 0x7;

	if (checksum != checksumw) {
		return "";
	}

	double temperature = ((hundred * 1000) + (temp1 * 100) + (temp2 * 10) + temp3)/10.0;
	if (neg) {
		temperature = -temperature;
	}

	std::stringstream retString;
	retString << "class:sensor;protocol:oregon;model:EA4C;id:" << static_cast<int>(address)
		<< ";temp:" << std::fixed << std::setprecision(1) << temperature << ";";

	return retString.str();
}

std::string ProtocolOregon::decode1984(const std::string &data, const std::wstring &model) {

	//wind
	uint64_t value = TelldusCore::hexTo64l(data);

	uint8_t crcCheck = value & 0xF; //PROBABLY crc
	value >>= 4;
	uint8_t messageChecksum1 = value & 0xF;
	value >>= 4;
	uint8_t messageChecksum2 = value & 0xF;

	value >>= 4;
	uint8_t avg1 = value & 0xF;
	value >>= 4;
	uint8_t avg2 = value & 0xF;
	value >>= 4;
	uint8_t avg3 = value & 0xF;
	value >>= 4;
	uint8_t gust1 = value & 0xF;
	value >>= 4;
	uint8_t gust2 = value & 0xF;
	value >>= 4;
	uint8_t gust3 = value & 0xF;
	value >>= 4;
	uint8_t unknown1 = value & 0xF;
	value >>= 4;
	uint8_t unknown2 = value & 0xF;
	value >>= 4;
	uint8_t direction = value & 0xF;

	value >>= 4;
	uint8_t battery = value & 0xF; //PROBABLY battery
	value >>= 4;
	uint8_t rollingcode = ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t checksum = ((value >> 4) & 0xF) + (value & 0xF);
	value >>= 8;
	uint8_t channel = value & 0xF;
	checksum += unknown1 + unknown2 + avg1 + avg2 + avg3 + gust1 + gust2 + gust3 + direction + battery + channel;

	if (model.compare(L"0x1984") == 0) {
		checksum += 0x1 + 0x9 + 0x8 + 0x4;
	}
	else {
		checksum += 0x1 + 0x9 + 0x9 + 0x4;
	}

	if (((checksum >> 4) & 0xF) != messageChecksum1 || (checksum & 0xF) != messageChecksum2){
		//checksum error
		return "";
	}


	double avg = ((avg1 * 100) + (avg2 * 10) + avg3)/10.0;
	double gust = ((gust1 * 100) + (gust2 * 10) + gust3)/10.0;
	float directiondegree = 22.5 * direction;

	std::stringstream retString;
	retString << "class:sensor;protocol:oregon;model:1984;id:" << static_cast<int>(rollingcode)
		<< ";winddirection:" << directiondegree
		<< ";windaverage:" << std::fixed << std::setprecision(1) << avg
		<< ";windgust:" << std::fixed << std::setprecision(1) << gust << ";";

	return retString.str();
}

std::string ProtocolOregon::decode1A2D(const std::string &data) {
	uint64_t value = TelldusCore::hexTo64l(data);
	// checksum2 not used yet
	// uint8_t checksum2 = value & 0xFF;
	value >>= 8;
	uint8_t checksum1 = value & 0xFF;
	value >>= 8;

	uint8_t checksum = ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t hum1 = value & 0xF;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t neg = value & (1 << 3);
	uint8_t hum2 = (value >> 4) & 0xF;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t temp2 = value & 0xF;
	uint8_t temp1 = (value >> 4) & 0xF;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t temp3 = (value >> 4) & 0xF;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t address = value & 0xFF;
	value >>= 8;

	checksum += ((value >> 4) & 0xF) + (value & 0xF);
	// channel not used
	// uint8_t channel = (value >> 4) & 0x7;

	checksum += 0x1 + 0xA + 0x2 + 0xD - 0xA;

	// TODO(micke): Find out how checksum2 works
	if (checksum != checksum1) {
		return "";
	}

	double temperature = ((temp1 * 100) + (temp2 * 10) + temp3)/10.0;
	if (neg) {
		temperature = -temperature;
	}
	int humidity = (hum1 * 10.0) + hum2;

	std::stringstream retString;
	retString << "class:sensor;protocol:oregon;model:1A2D;id:" << static_cast<int>(address)
		<< ";temp:" << std::fixed << std::setprecision(1) << temperature
		<< ";humidity:" << humidity << ";";

	return retString.str();
}

std::string ProtocolOregon::decode2914(const std::string &data) {

	//rain
	uint64_t value = TelldusCore::hexTo64l(data);

	uint8_t messageChecksum1 = value & 0xF;
	value >>= 4;
	uint8_t messageChecksum2 = value & 0xF;

	value >>= 4;
	uint8_t totRain1 = value & 0xF;
	value >>= 4;
	uint8_t totRain2 = value & 0xF;
	value >>= 4;
	uint8_t totRain3 = value & 0xF;
	value >>= 4;
	uint8_t totRain4 = value & 0xF;
	value >>= 4;
	uint8_t totRain5 = value & 0xF;
	value >>= 4;
	uint8_t totRain6 = value & 0xF;
	value >>= 4;
	uint8_t rainRate1 = value & 0xF;
	value >>= 4;
	uint8_t rainRate2 = value & 0xF;
	value >>= 4;
	uint8_t rainRate3 = value & 0xF;
	value >>= 4;
	uint8_t rainRate4 = value & 0xF;

	value >>= 4;
	uint8_t battery = value & 0xF; //PROBABLY battery
	value >>= 4;
	uint8_t rollingcode = ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t checksum = ((value >> 4) & 0xF) + (value & 0xF);
	value >>= 8;
	uint8_t channel = value & 0xF;
	checksum += totRain1 + totRain2 + totRain3 + totRain4 + totRain5 + totRain6 + rainRate1 + rainRate2 + rainRate3 + rainRate4 + battery + channel + 0x2 + 0x9 + 0x1 + 0x4;

	if (((checksum >> 4) & 0xF) != messageChecksum1 || (checksum & 0xF) != messageChecksum2){
		//checksum error
		return "";
	}

	double totRain = ((totRain1 * 100000) + (totRain2 * 10000) + (totRain3 * 1000) + (totRain4 * 100) + (totRain5 * 10) + totRain6)/1000.0*25.4;
	double rainRate = ((rainRate1 * 1000) + (rainRate2 * 100) + (rainRate3 * 10) + rainRate4)/100.0*25.4;

	std::stringstream retString;
	retString << "class:sensor;protocol:oregon;model:2914;id:" << static_cast<int>(rollingcode)
		<< ";raintotal:" << std::fixed << std::setprecision(1) << totRain
		<< ";rainrate:" << std::fixed << std::setprecision(1) << rainRate << ";";
	return retString.str();
}

std::string ProtocolOregon::decodeF824(const std::string &data) {

	uint64_t value = TelldusCore::hexTo64l(data);

	uint8_t crcCheck = value & 0xF; //PROBABLY crc
	value >>= 4;
	uint8_t messageChecksum1 = value & 0xF;
	value >>= 4;
	uint8_t messageChecksum2 = value & 0xF;
	value >>= 4;
	uint8_t unknown = value & 0xF;
	value >>= 4;
	uint8_t hum1 = value & 0xF;
	value >>= 4;
	uint8_t hum2 = value & 0xF;
	value >>= 4;
	uint8_t neg = value & 0xF;
	value >>= 4;
	uint8_t temp1 = value & 0xF;
	value >>= 4;
	uint8_t temp2 = value & 0xF;
	value >>= 4;
	uint8_t temp3 = value & 0xF;
	value >>= 4;
	uint8_t battery = value & 0xF; //PROBABLY battery
	value >>= 4;
	uint8_t rollingcode = ((value >> 4) & 0xF) + (value & 0xF);
	uint8_t checksum = ((value >> 4) & 0xF) + (value & 0xF);
	value >>= 8;
	uint8_t channel = value & 0xF;
	checksum += unknown + hum1 + hum2 + neg + temp1 + temp2 + temp3 + battery + channel + 0xF + 0x8 + 0x2 + 0x4;

	if (((checksum >> 4) & 0xF) != messageChecksum1 || (checksum & 0xF) != messageChecksum2){
		//checksum error
		return "";
	}

	double temperature = ((temp1 * 100) + (temp2 * 10) + temp3)/10.0;
	if (neg) {
		temperature = -temperature;
	}
	int humidity = (hum1 * 10.0) + hum2;

	std::stringstream retString;
	retString << "class:sensor;protocol:oregon;model:F824;id:" << static_cast<int>(rollingcode)
		<< ";temp:" << std::fixed << std::setprecision(1) << temperature
		<< ";humidity:" << humidity << ";";

	return retString.str();
}
