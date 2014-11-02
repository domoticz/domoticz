//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "common/EventHandler.h"
#include <windows.h>
#include <list>

#include "common/Event.h"
#include "common/Mutex.h"

namespace TelldusCore {

class EventHandler::PrivateData {
public:
	HANDLE *eventArray;
	EventRef *eventObjectArray;
	TelldusCore::Mutex mutex;
	int eventCount;
};

EventHandler::EventHandler() {
	d = new PrivateData;
	d->eventCount = 0;
	d->eventArray = new HANDLE[0];
	d->eventObjectArray = new EventRef[0];
}

EventHandler::~EventHandler(void) {
	delete[] d->eventObjectArray;
	delete[] d->eventArray;
	delete d;
}

EventRef EventHandler::addEvent() {
	EventRef event(new Event(this));

	TelldusCore::MutexLocker locker(&d->mutex);

	HANDLE *newArray = new HANDLE[d->eventCount+1];
	EventRef *newObjectArray = new EventRef[d->eventCount+1];
	for (int i = 0; i < d->eventCount; ++i) {
		newArray[i] = d->eventArray[i];
		newObjectArray[i] = d->eventObjectArray[i];
	}
	delete[] d->eventArray;
	delete[] d->eventObjectArray;
	d->eventArray = newArray;
	d->eventObjectArray = newObjectArray;
	d->eventArray[d->eventCount] = event->retrieveNative();
	d->eventObjectArray[d->eventCount] = event;
	++d->eventCount;
	return event;
}

void EventHandler::signal(Event *event) {
}

bool EventHandler::waitForAny() {
	while(1) {
		int result = WaitForMultipleObjects(d->eventCount, d->eventArray, FALSE, 1000);
		if (result == WAIT_TIMEOUT) {
			continue;
		}
		TelldusCore::MutexLocker locker(&d->mutex);
		int eventIndex = result - WAIT_OBJECT_0;
		if (eventIndex >= d->eventCount) {
			return false;
		}
		return true;
	}
}

}  // namespace TelldusCore
