//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLHASTA_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLHASTA_H_

#include <string>
#include "service/ControllerMessage.h"
#include "service/Protocol.h"

class ProtocolHasta : public Protocol {
public:
	int methods() const;
	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller);
	static std::string decodeData(const ControllerMessage &dataMsg);

protected:
	static std::string convertByte(unsigned char byte);
	static std::string convertBytev2(unsigned char byte);
	std::string getStringForMethodv1(int method);
	std::string getStringForMethodv2(int method);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLHASTA_H_
