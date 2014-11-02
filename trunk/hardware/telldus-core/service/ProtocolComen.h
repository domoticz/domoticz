//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOLCOMEN_H_
#define TELLDUS_CORE_SERVICE_PROTOCOLCOMEN_H_

#include <string>
#include "service/ProtocolNexa.h"

class ProtocolComen : public ProtocolNexa {
public:
	virtual int methods() const;

protected:
	virtual int getIntParameter(const std::wstring &name, int min, int max) const;
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOLCOMEN_H_
