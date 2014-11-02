//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/TellStick.h"

#include <stdio.h>
#include <map>
#include <string>

std::string TellStick::createTPacket( const std::string &msg ) {
	std::map<unsigned char, char> times;
	std::string data;
	int index = 0;
	for(size_t i = 0; i < msg.length(); ++i) {
		// Search to se if it already exists and get the index
		if (times.find(msg.at(i)) == times.end()) {
			times[msg.at(i)] = index++;
			if (times.size() > 4) {
				return "";
			}
		}
		data.append(1, times[msg.at(i)]);
	}
	// Reorder the times
	unsigned char t0 = 1, t1 = 1, t2 = 1, t3 = 1;
	for(std::map<unsigned char, char>::const_iterator it = times.begin(); it != times.end(); ++it) {
		if ((*it).second == 0) {
			t0 = (*it).first;
		} else if ((*it).second == 1) {
			t1 = (*it).first;
		} else if ((*it).second == 2) {
			t2 = (*it).first;
		} else if ((*it).second == 3) {
			t3 = (*it).first;
		}
	}

	return TellStick::convertSToT(t0, t1, t2, t3, data);
}

std::string TellStick::convertSToT( unsigned char t0, unsigned char t1, unsigned char t2, unsigned char t3, const std::string &data ) {
	unsigned char dataByte = 0;
	std::string retString = "T";
	retString.append(1, t0);
	retString.append(1, t1);
	retString.append(1, t2);
	retString.append(1, t3);

	if (data.length() > 255) {
		return "";
	}
	unsigned char length = (unsigned char)data.length();
	retString.append(1, length);

	for (size_t i = 0; i < data.length(); ++i) {
		dataByte <<= 2;
		if (data.at(i) == 1) {
			dataByte |= 1;
		} else if (data.at(i) == 2) {
			dataByte |= 2;
		} else if (data.at(i) == 3) {
			dataByte |= 3;
		}
		if ( (i+1) % 4 == 0) {
			retString.append(1, dataByte);
			dataByte = 0;
		}
	}
	if (data.length() % 4 != 0) {
		dataByte <<= (data.length() % 4)*2;
		retString.append(1, dataByte);
	}

	retString.append("+");
	return retString;
}
