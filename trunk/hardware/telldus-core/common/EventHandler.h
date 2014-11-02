//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_EVENTHANDLER_H_
#define TELLDUS_CORE_COMMON_EVENTHANDLER_H_

#include "common/Event.h"

namespace TelldusCore {

	class EventHandler {
	public:
		EventHandler();
		virtual ~EventHandler(void);

		EventRef addEvent();

		bool waitForAny();

	protected:
		void signal(Event *event);

	private:
		class PrivateData;
		PrivateData *d;
		bool listIsSignalled();

		friend class Event;
	};
}

#endif  // TELLDUS_CORE_COMMON_EVENTHANDLER_H_
