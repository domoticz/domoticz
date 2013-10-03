//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLX10_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLX10_H_

#include <string>
#include "service/Protocol.h"
#include "service/ControllerMessage.h"

class ProtocolX10 : public Protocol {
public:
	int methods() const;
	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller);

	static std::string decodeData(const ControllerMessage &dataMsg);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLX10_H_
