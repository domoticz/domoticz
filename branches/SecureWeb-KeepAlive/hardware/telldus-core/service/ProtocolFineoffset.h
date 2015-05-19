//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLFINEOFFSET_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLFINEOFFSET_H_

#include <string>
#include "service/Protocol.h"
#include "service/ControllerMessage.h"

class ProtocolFineoffset : public Protocol {
public:
	static std::string decodeData(const ControllerMessage &dataMsg);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLFINEOFFSET_H_
