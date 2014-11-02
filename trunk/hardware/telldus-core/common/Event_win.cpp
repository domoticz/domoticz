//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "common/Event.h"
#include "common/Thread.h"

namespace TelldusCore {

class Event::PrivateData {
public:
	EVENT_T event;
};

Event::Event(EventHandler *handler)
	:EventBase(handler) {
	d = new PrivateData;
	d->event = CreateEvent(NULL, true, false, NULL);
}

Event::~Event(void) {
	CloseHandle(d->event);
	delete d;
}

EVENT_T Event::retrieveNative() {
	return d->event;
}

void Event::clearSignal() {
	ResetEvent(d->event);
}

void Event::sendSignal() {
	SetEvent(d->event);
}

}  // namespace TelldusCore
