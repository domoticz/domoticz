//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLSARTANO_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLSARTANO_H_

#include <string>
#include "service/Protocol.h"
#include "service/ControllerMessage.h"

class ProtocolSartano : public Protocol {
public:
	int methods() const;
	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller);
	static std::string decodeData(const ControllerMessage &dataMsg);

protected:
	std::string getStringForCode(const std::wstring &code, int method);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLSARTANO_H_
