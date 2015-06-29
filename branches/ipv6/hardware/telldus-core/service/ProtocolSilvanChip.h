//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLSILVANCHIP_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLSILVANCHIP_H_

#include <string>
#include "service/Protocol.h"

class ProtocolSilvanChip : public Protocol {
public:
	int methods() const;
	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller);

protected:
	virtual std::string getString(const std::string &preamble, const std::string &one, const std::string &zero, int button);
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLSILVANCHIP_H_
