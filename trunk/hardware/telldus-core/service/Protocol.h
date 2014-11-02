//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_PROTOCOL_H_
#define TELLDUS_CORE_SERVICE_PROTOCOL_H_

#include <string>
#include <list>
#include <map>
#include "client/telldus-core.h"

typedef std::map<std::wstring, std::wstring> ParameterMap;

class Controller;

class Protocol {
public:
	Protocol();
	virtual ~Protocol(void);

	static Protocol *getProtocolInstance(const std::wstring &protocolname);
	static std::list<std::string> getParametersForProtocol(const std::wstring &protocolName);
	static std::list<std::string> decodeData(const std::string &fullData);

	virtual int methods() const = 0;
	std::wstring model() const;
	void setModel(const std::wstring &model);
	void setParameters(const ParameterMap &parameterList);

	virtual std::string getStringForMethod(int method, unsigned char data, Controller *controller) = 0;

protected:
	virtual std::wstring getStringParameter(const std::wstring &name, const std::wstring &defaultValue = L"") const;
	virtual int getIntParameter(const std::wstring &name, int min, int max) const;

	static bool checkBit(int data, int bit);

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_PROTOCOL_H_
