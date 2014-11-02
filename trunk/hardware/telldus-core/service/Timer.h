//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_TIMER_H_
#define TELLDUS_CORE_SERVICE_TIMER_H_

#include "common/Event.h"
#include "common/Thread.h"

class Timer : public TelldusCore::Thread {
public:
	explicit Timer(TelldusCore::EventRef event);
	virtual ~Timer();

	void setInterval(int sec);
	void stop();

protected:
	void run();

private:
	class PrivateData;
	PrivateData *d;
};


#endif  // TELLDUS_CORE_SERVICE_TIMER_H_
