//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_CONTROLLERLISTENER_H_
#define TELLDUS_CORE_SERVICE_CONTROLLERLISTENER_H_

#include "common/Thread.h"
#include "common/Event.h"

class ControllerChangeEventData : public TelldusCore::EventDataBase {
public:
	int vid, pid;
	bool inserted;
};

class ControllerListener : public TelldusCore::Thread {
public:
	explicit ControllerListener(TelldusCore::EventRef event);
	virtual ~ControllerListener();

protected:
	void run();

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_CONTROLLERLISTENER_H_
