//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_CONTROLLERMESSAGE_H_
#define TELLDUS_CORE_SERVICE_CONTROLLERMESSAGE_H_

#ifdef _MSC_VER
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif
#include <string>

class ControllerMessage {
public:
	explicit ControllerMessage(const std::string &rawMessage);
	virtual ~ControllerMessage();

	std::string msgClass() const;
	uint64_t getInt64Parameter(const std::string &key) const;
	std::string getParameter(const std::string &key) const;
	int method() const;
	std::wstring protocol() const;
	std::wstring model() const;

	bool hasParameter(const std::string &key) const;

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_CONTROLLERMESSAGE_H_
