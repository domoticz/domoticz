//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#define _CRT_RAND_S
#include "service/Controller.h"
#include <stdlib.h>
#include <time.h>
#include <map>
#include <list>
#include <string>
#include "service/Protocol.h"
#include "service/EventUpdateManager.h"
#include "common/Strings.h"

inline int random( unsigned int* seed ) {
	#ifdef _WINDOWS
		unsigned int randomNumber;
		rand_s( &randomNumber );  // no seed needed
		return randomNumber;
	#else
		return rand_r( seed );
	#endif
}

class Controller::PrivateData {
public:
	TelldusCore::EventRef event, updateEvent;
	int id, firmwareVersion;
	unsigned int randSeed;
	std::map<std::string, time_t> duplicates;
};

Controller::Controller(int id, TelldusCore::EventRef event, TelldusCore::EventRef updateEvent) {
	d = new PrivateData;
	d->event = event;
	d->updateEvent = updateEvent;
	d->id = id;
	d->firmwareVersion = 0;
	d->randSeed = time(NULL);
}

Controller::~Controller() {
	delete d;
}

void Controller::publishData(const std::string &msg) const {
	ControllerEventData *data = new ControllerEventData;
	data->msg = msg;
	data->controllerId = d->id;
	d->event->signal(data);
}

void Controller::decodePublishData(const std::string &data) const {
	// Garbange collect?
	if (random(&d->randSeed) % 1000 == 1) {
		time_t t = time(NULL);
		// Standard associative-container erase idiom
		for (std::map<std::string, time_t>::iterator it = d->duplicates.begin(); it != d->duplicates.end(); /* no increment */) {
			if ((*it).second != t) {
				d->duplicates.erase(it++);
			} else {
				++it;
			}
		}
	}
	// Duplicate check
	if (d->duplicates.count(data) > 0) {
		time_t t = d->duplicates[data];
		if (t == time(NULL)) {
			// Duplicate message
			return;
		}
	}
	d->duplicates[data] = time(NULL);

	std::list<std::string> msgList = Protocol::decodeData(data);

	for (std::list<std::string>::iterator msgIt = msgList.begin(); msgIt != msgList.end(); ++msgIt) {
		this->publishData(*msgIt);
	}
}

int Controller::firmwareVersion() const {
	return d->firmwareVersion;
}

void Controller::setFirmwareVersion(int version) {
	d->firmwareVersion = version;
	EventUpdateData *eventData = new EventUpdateData();
	eventData->messageType = L"TDControllerEvent";
	eventData->controllerId = d->id;
	eventData->eventState = TELLSTICK_DEVICE_CHANGED;
	eventData->eventChangeType = TELLSTICK_CHANGE_FIRMWARE;
	eventData->eventValue = TelldusCore::intToWstring(version);
	d->updateEvent->signal(eventData);
}
