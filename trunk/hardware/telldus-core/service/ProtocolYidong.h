//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLYIDONG_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLYIDONG_H_

#include <string>
#include "service/ProtocolSartano.h"

class ProtocolYidong : public ProtocolSartano {
public:
	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLYIDONG_H_
