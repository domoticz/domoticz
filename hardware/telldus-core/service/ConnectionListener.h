//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_CONNECTIONLISTENER_H_
#define TELLDUS_CORE_SERVICE_CONNECTIONLISTENER_H_

#include <string>
#include "common/Thread.h"
#include "common/Event.h"

class Event;
namespace TelldusCore {
	class Socket;
};

class ConnectionListenerEventData : public TelldusCore::EventDataBase {
public:
	TelldusCore::Socket *socket;
};

class ConnectionListener : public TelldusCore::Thread {
public:
	ConnectionListener(const std::wstring &name, TelldusCore::EventRef waitEvent);
	virtual ~ConnectionListener(void);

protected:
	void run();

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_CONNECTIONLISTENER_H_
