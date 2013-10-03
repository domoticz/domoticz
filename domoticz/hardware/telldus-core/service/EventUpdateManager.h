//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_EVENTUPDATEMANAGER_H_
#define TELLDUS_CORE_SERVICE_EVENTUPDATEMANAGER_H_

#include <string>
#include "common/Thread.h"
#include "common/Event.h"

class EventUpdateData : public TelldusCore::EventDataBase {
public:
	std::wstring messageType;
	int controllerId;
	int deviceId;
	int eventChangeType;
	int eventDeviceChanges;
	int eventState;
	std::wstring eventValue;

	// Sensor event
	std::wstring protocol;
	std::wstring model;
	int sensorId;
	int dataType;
	std::wstring value;
	int timestamp;
};

class EventUpdateManager  : public TelldusCore::Thread {
public:
	EventUpdateManager(void);
	~EventUpdateManager(void);

	TelldusCore::EventRef retrieveUpdateEvent();
	TelldusCore::EventRef retrieveClientConnectEvent();

protected:
	void run();

private:
	class PrivateData;
	PrivateData *d;
	void loadScripts(const std::string &folder);
	void sendMessageToClients(EventUpdateData *data);
	void executeScripts(EventUpdateData *data);
	static void executeScript(std::string script, const std::string &name, char **env);
};

#endif  // TELLDUS_CORE_SERVICE_EVENTUPDATEMANAGER_H_
