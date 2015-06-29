//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLOREGON_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLOREGON_H_

#include <string>
#include "service/Protocol.h"
#include "service/ControllerMessage.h"

class ProtocolOregon : public Protocol {
public:
	static std::string decodeData(const ControllerMessage &dataMsg);

protected:
	static std::string decodeEA4C(const std::string &data);
	static std::string decode1A2D(const std::string &data);
	static std::string decodeF824(const std::string &data);
	static std::string decode1984(const std::string &data, const std::wstring &model);
	static std::string decode2914(const std::string &data);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLOREGON_H_
