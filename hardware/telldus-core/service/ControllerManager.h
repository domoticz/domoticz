//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_CONTROLLERMANAGER_H_
#define TELLDUS_CORE_SERVICE_CONTROLLERMANAGER_H_

#include <string>
#include "common/Event.h"
class Controller;


class ControllerManager {
public:
	ControllerManager(TelldusCore::EventRef event, TelldusCore::EventRef updateEvent);
	~ControllerManager(void);

	void deviceInsertedOrRemoved(int vid, int pid, const std::string &serial, bool inserted);

	int count();
	Controller *getBestControllerById(int id);
	void loadControllers();
	void loadStoredControllers();
	void queryControllerStatus();
	int resetController(Controller *controller);

	std::wstring getControllers() const;
	std::wstring getControllerValue(int id, const std::wstring &name);
	int removeController(int id);
	int setControllerValue(int id, const std::wstring &name, const std::wstring &value);

private:
	void signalControllerEvent(int controllerId, int changeEvent, int changeType, const std::wstring &newValue);
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_CONTROLLERMANAGER_H_
