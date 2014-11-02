//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolGroup.h"
#include <string>

int ProtocolGroup::methods() const {
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_DIM | TELLSTICK_BELL | TELLSTICK_LEARN | TELLSTICK_EXECUTE | TELLSTICK_TOGGLE | TELLSTICK_UP | TELLSTICK_DOWN | TELLSTICK_STOP;
}

std::string ProtocolGroup::getStringForMethod(int method, unsigned char data, Controller *) {
	return "";
}
