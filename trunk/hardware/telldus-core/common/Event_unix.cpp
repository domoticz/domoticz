//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "common/Event.h"
#include "common/EventHandler.h"
#include "common/Thread.h"

namespace TelldusCore {

class Event::PrivateData {
public:
};

Event::Event(EventHandler *handler)
	:EventBase(handler) {
	d = new PrivateData;
}

Event::~Event(void) {
	delete d;
}

void Event::clearSignal() {
}

void Event::sendSignal() {
	EventHandler *handler = this->handler();
	if (handler) {
		handler->signal(this);
	}
}

}  // namespace TelldusCore
