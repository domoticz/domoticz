//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ProtocolScene.h"
#include <string>

int ProtocolScene::methods() const {
	return TELLSTICK_EXECUTE;
}

std::string ProtocolScene::getStringForMethod(int method, unsigned char data, Controller *) {
	return "";
}
