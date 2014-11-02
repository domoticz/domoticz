//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolComen.h"
#include <string>

int ProtocolComen::methods() const {
	return (TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_LEARN);
}

int ProtocolComen::getIntParameter(const std::wstring &name, int min, int max) const {
	if (name.compare(L"house") == 0) {
		int intHouse = Protocol::getIntParameter(L"house", 1, 16777215);
		// The last two bits must be hardcoded
		intHouse <<= 2;
		intHouse += 2;
		return intHouse;
	}
	return Protocol::getIntParameter(name, min, max);
}
