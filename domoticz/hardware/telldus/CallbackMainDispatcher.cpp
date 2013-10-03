/*
 *  CallbackMainDispatcher.cpp
 *  telldus-core
 *
 *  Created by Stefan Persson on 2012-02-23.
 *  Copyright 2012 Telldus Technologies AB. All rights reserved.
 *
 */

#include "client/CallbackMainDispatcher.h"

#include <list>

namespace TelldusCore {

typedef std::list<CallbackStruct *> CallbackList;

class CallbackMainDispatcher::PrivateData {
public:
	EventHandler eventHandler;
	EventRef stopEvent, generalCallbackEvent, janitor;

	Mutex mutex;
	std::list<std::tr1::shared_ptr<TelldusCore::TDEventDispatcher> > eventThreadList;

	CallbackList callbackList;

	int lastCallbackId;
};

CallbackMainDispatcher::CallbackMainDispatcher()
:Thread() {
	d = new PrivateData;
	d->stopEvent = d->eventHandler.addEvent();
	d->generalCallbackEvent = d->eventHandler.addEvent();
	d->janitor = d->eventHandler.addEvent();  // Used for cleanups

	d->lastCallbackId = 0;
}

CallbackMainDispatcher::~CallbackMainDispatcher(void) {
	d->stopEvent->signal();
	wait();
	{
		MutexLocker locker(&d->mutex);
	}
	delete d;
}

EventRef CallbackMainDispatcher::retrieveCallbackEvent() {
	return d->generalCallbackEvent;
}

int CallbackMainDispatcher::registerCallback(CallbackStruct::CallbackType type, void *eventFunction, void *context) {
	TelldusCore::MutexLocker locker(&d->mutex);
	int id = ++d->lastCallbackId;
	CallbackStruct *callback = new CallbackStruct;
	callback->type = type;
	callback->event = eventFunction;
	callback->id = id;
	callback->context = context;
	d->callbackList.push_back(callback);
	return id;
}

int CallbackMainDispatcher::unregisterCallback(int callbackId) {
	CallbackList newEventList;
	{
		TelldusCore::MutexLocker locker(&d->mutex);
		for(CallbackList::iterator callback_it = d->callbackList.begin(); callback_it != d->callbackList.end(); ++callback_it) {
			if ( (*callback_it)->id != callbackId ) {
				continue;
			}
			newEventList.splice(newEventList.begin(), d->callbackList, callback_it);
			break;
		}
	}
	if (newEventList.size()) {
		CallbackList::iterator it = newEventList.begin();
		{  // Lock and unlock to make sure no one else uses the object
			TelldusCore::MutexLocker locker( &(*it)->mutex );
		}
		delete (*it);
		newEventList.erase(it);
		return TELLSTICK_SUCCESS;
	}
	return TELLSTICK_ERROR_NOT_FOUND;
}

void CallbackMainDispatcher::run() {
	while(!d->stopEvent->isSignaled()) {
		if (!d->eventHandler.waitForAny()) {
			continue;
		}

		if(d->generalCallbackEvent->isSignaled()) {
			EventDataRef eventData = d->generalCallbackEvent->takeSignal();

			CallbackData *cbd = dynamic_cast<CallbackData *>(eventData.get());
			if (!cbd) {
				continue;
			}
			TelldusCore::MutexLocker locker(&d->mutex);
			for(CallbackList::iterator callback_it = d->callbackList.begin(); callback_it != d->callbackList.end(); ++callback_it) {
				if ( (*callback_it)->type == cbd->type ) {
					std::tr1::shared_ptr<TelldusCore::TDEventDispatcher> ptr(new TelldusCore::TDEventDispatcher(eventData, *callback_it, d->janitor));
					d->eventThreadList.push_back(ptr);
				}
			}
		}
		if (d->janitor->isSignaled()) {
			// Clear all of them if there is more than one
			while(d->janitor->isSignaled()) {
				d->janitor->popSignal();
			}
			this->cleanupCallbacks();
		}
	}
}

void CallbackMainDispatcher::cleanupCallbacks() {
	bool again = false;

	// Device Event
	do {
		again = false;
		MutexLocker locker(&d->mutex);
		std::list<std::tr1::shared_ptr<TDEventDispatcher> >::iterator it = d->eventThreadList.begin();
		for (; it != d->eventThreadList.end(); ++it) {
			if ((*it)->done()) {
				d->eventThreadList.erase(it);
				again = true;
				break;
			}
		}
	} while (again);
}

}  // namespace TelldusCore
