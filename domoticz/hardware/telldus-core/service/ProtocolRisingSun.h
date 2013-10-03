//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLRISINGSUN_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLRISINGSUN_H_

#include <string>
#include "service/Protocol.h"

class ProtocolRisingSun : public Protocol {
public:
	int methods() const;
	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller);

protected:
	std::string getStringSelflearning(int method);
	std::string getStringCodeSwitch(int method);
	static std::string getCodeSwitchTuple(int code);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLRISINGSUN_H_
