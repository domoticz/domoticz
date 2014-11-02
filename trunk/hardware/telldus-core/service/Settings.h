//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_SETTINGS_H_
#define TELLDUS_CORE_SERVICE_SETTINGS_H_

#include <string>
#include "common/Mutex.h"

class Settings {
public:
	enum Node { Device, Controller };

	Settings(void);
	virtual ~Settings(void);

	std::wstring getSetting(const std::wstring &strName) const;
	int getNumberOfNodes(Node type) const;
	std::wstring getName(Node type, int intNodeId) const;
	int setName(Node type, int intDeviceId, const std::wstring &strNewName);
	std::wstring getProtocol(int intDeviceId) const;
	int setProtocol(int intDeviceId, const std::wstring &strVendor);
	std::wstring getModel(int intDeviceId) const;
	int setModel(int intDeviceId, const std::wstring &strModel);
	std::wstring getDeviceParameter(int intDeviceId, const std::wstring &strName) const;
	int setDeviceParameter(int intDeviceId, const std::wstring &strName, const std::wstring &strValue);
	bool setDeviceState( int intDeviceId, int intDeviceState, const std::wstring &strDeviceStateValue );
	int getDeviceState( int intDeviceId ) const;
	std::wstring getDeviceStateValue( int intDeviceId ) const;
	int getPreferredControllerId(int intDeviceId);
	int setPreferredControllerId(int intDeviceId, int value);

	int addNode(Node type);
	int getNodeId(Node type, int intDeviceIndex) const;
	int removeNode(Node type, int intNodeId);

	std::wstring getControllerSerial(int intControllerId) const;
	int setControllerSerial(int intControllerId, const std::wstring &serial);
	int getControllerType(int intControllerId) const;
	int setControllerType(int intControllerId, int type);

protected:
	std::wstring getStringSetting(Node type, int intNodeId, const std::wstring &name, bool parameter) const;
	int setStringSetting(Node type, int intDeviceId, const std::wstring &name, const std::wstring &value, bool parameter);
	int getIntSetting(Node type, int intDeviceId, const std::wstring &name, bool parameter) const;
	int setIntSetting(Node type, int intDeviceId, const std::wstring &name, int value, bool parameter);

private:
	int getNextNodeId(Node type) const;
	std::string getNodeString(Node type) const;

	class PrivateData;
	PrivateData *d;
	static TelldusCore::Mutex mutex;
};

#endif  // TELLDUS_CORE_SERVICE_SETTINGS_H_
