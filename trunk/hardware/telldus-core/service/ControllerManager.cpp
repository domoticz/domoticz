//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ControllerManager.h"

#include <stdio.h>
#include <list>
#include <map>
#include <string>

#include "service/Controller.h"
#include "common/Mutex.h"
#include "service/TellStick.h"
#include "service/Log.h"
#include "common/Message.h"
#include "common/Strings.h"
#include "service/Settings.h"
#include "service/EventUpdateManager.h"
#include "client/telldus-core.h"

class ControllerDescriptor {
public:
	std::wstring name, serial;
	int type;
	Controller *controller;
};

typedef std::map<int, ControllerDescriptor> ControllerMap;

class ControllerManager::PrivateData {
public:
	int lastControllerId;
	Settings settings;
	ControllerMap controllers;
	TelldusCore::EventRef event, updateEvent;
	TelldusCore::Mutex mutex;
};

ControllerManager::ControllerManager(TelldusCore::EventRef event, TelldusCore::EventRef updateEvent) {
	d = new PrivateData;
	d->lastControllerId = 0;
	d->event = event;
	d->updateEvent = updateEvent;
	this->loadStoredControllers();
	this->loadControllers();
}

ControllerManager::~ControllerManager() {
	for (ControllerMap::iterator it = d->controllers.begin(); it != d->controllers.end(); ++it) {
		if (it->second.controller) {
			delete( it->second.controller );
		}
	}
	delete d;
}

int ControllerManager::count() {
	unsigned int count = 0;
	{
		TelldusCore::MutexLocker locker(&d->mutex);
		// Find all available controllers
		for(ControllerMap::const_iterator it = d->controllers.begin(); it != d->controllers.end(); ++it) {
			if (it->second.controller) {
				++count;
			}
		}
	}
	if (count == 0) {
		this->loadControllers();
		// Try again
		TelldusCore::MutexLocker locker(&d->mutex);
		// Find all available controllers
		for(ControllerMap::const_iterator it = d->controllers.begin(); it != d->controllers.end(); ++it) {
			if (it->second.controller) {
				++count;
			}
		}
	}
	return count;
}

void ControllerManager::deviceInsertedOrRemoved(int vid, int pid, const std::string &serial, bool inserted) {
	if (vid == 0x0 && pid == 0x0) {  // All
		if (inserted) {
			loadControllers();
		} else {
			// Disconnect all
			TelldusCore::MutexLocker locker(&d->mutex);
			while(d->controllers.size()) {
				ControllerMap::iterator it = d->controllers.begin();
				delete it->second.controller;
				it->second.controller = 0;
				signalControllerEvent(it->first, TELLSTICK_DEVICE_STATE_CHANGED, TELLSTICK_CHANGE_AVAILABLE, L"0");
			}
		}
		return;
	}
	if (vid != 0x1781) {
		return;
	}
	if (pid != 0x0C30 && pid != 0x0C31) {
		return;
	}
	if (inserted) {
		loadControllers();
	} else {
		// Autodetect which has been disconnected
		TelldusCore::MutexLocker locker(&d->mutex);
		for(ControllerMap::iterator it = d->controllers.begin(); it != d->controllers.end(); ++it) {
			if (!it->second.controller) {
				continue;
			}
			TellStick *tellstick = reinterpret_cast<TellStick*>(it->second.controller);
			if (!tellstick) {
				continue;
			}
			if (serial.compare("") != 0) {
				TellStickDescriptor tsd;
				tsd.vid = vid;
				tsd.pid = pid;
				tsd.serial = serial;
				if (!tellstick->isSameAsDescriptor(tsd)) {
					continue;
				}
			} else if (tellstick->stillConnected()) {
				continue;
			}

			it->second.controller = 0;
			delete tellstick;
			signalControllerEvent(it->first, TELLSTICK_DEVICE_STATE_CHANGED, TELLSTICK_CHANGE_AVAILABLE, L"0");
		}
	}
}

Controller *ControllerManager::getBestControllerById(int id) {
	TelldusCore::MutexLocker locker(&d->mutex);
	if (!d->controllers.size()) {
		return 0;
	}
	ControllerMap::const_iterator it = d->controllers.find(id);
	if (it != d->controllers.end() && it->second.controller) {
		return it->second.controller;
	}
	// Find first available controller
	for(it = d->controllers.begin(); it != d->controllers.end(); ++it) {
		if (it->second.controller) {
			return it->second.controller;
		}
	}
	return 0;
}

void ControllerManager::loadControllers() {
	TelldusCore::MutexLocker locker(&d->mutex);

	std::list<TellStickDescriptor> list = TellStick::findAll();

	std::list<TellStickDescriptor>::iterator it = list.begin();
	for(; it != list.end(); ++it) {
		// Most backend only report non-opened devices.
		// If they don't make sure we don't open them twice
		bool found = false;
		ControllerMap::const_iterator cit = d->controllers.begin();
		for(; cit != d->controllers.end(); ++cit) {
			if (!cit->second.controller) {
				continue;
			}
			TellStick *tellstick = reinterpret_cast<TellStick*>(cit->second.controller);
			if (!tellstick) {
				continue;
			}
			if (tellstick->isSameAsDescriptor(*it)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		}

		int type = TELLSTICK_CONTROLLER_TELLSTICK;
		if ((*it).pid == 0x0c31) {
			type = TELLSTICK_CONTROLLER_TELLSTICK_DUO;
		}
		int controllerId = 0;
		// See if the controller matches one of the loaded, non available controllers
		std::wstring serial = TelldusCore::charToWstring((*it).serial.c_str());
		for(cit = d->controllers.begin(); cit != d->controllers.end(); ++cit) {
			if (cit->second.type == type && cit->second.serial.compare(serial) == 0) {
				controllerId = cit->first;
				break;
			}
		}
		bool isNew = false;
		if (!controllerId) {
			controllerId = d->settings.addNode(Settings::Controller);
			if(controllerId < 0) {
				// TODO(micke): How to handle this?
				continue;
			}
			isNew = true;
			d->controllers[controllerId].type = type;
			d->settings.setControllerType(controllerId, type);
			d->controllers[controllerId].serial = TelldusCore::charToWstring((*it).serial.c_str());
			d->settings.setControllerSerial(controllerId, d->controllers[controllerId].serial);
		}

		// int controllerId = d->lastControllerId+1;
		TellStick *controller = new TellStick(controllerId, d->event, d->updateEvent, *it);
		if (!controller->isOpen()) {
			delete controller;
			continue;
		}
		d->controllers[controllerId].controller = controller;
		if (isNew) {
			signalControllerEvent(controllerId, TELLSTICK_DEVICE_ADDED, type, L"");
		} else {
			signalControllerEvent(controllerId, TELLSTICK_DEVICE_STATE_CHANGED, TELLSTICK_CHANGE_AVAILABLE, L"1");
		}
	}
}

void ControllerManager::loadStoredControllers() {
	int numberOfControllers = d->settings.getNumberOfNodes(Settings::Controller);
	TelldusCore::MutexLocker locker(&d->mutex);

	for (int i = 0; i < numberOfControllers; ++i) {
		int id = d->settings.getNodeId(Settings::Controller, i);
		d->controllers[id].controller = NULL;
		d->controllers[id].name = d->settings.getName(Settings::Controller, id);
		const int type = d->settings.getControllerType(id);
		d->controllers[id].type = type;
		d->controllers[id].serial = d->settings.getControllerSerial(id);
		signalControllerEvent(id, TELLSTICK_DEVICE_ADDED, type, L"");
	}
}

void ControllerManager::queryControllerStatus() {
	std::list<TellStick *> tellStickControllers;

	{
		TelldusCore::MutexLocker locker(&d->mutex);
		for(ControllerMap::iterator it = d->controllers.begin(); it != d->controllers.end(); ++it) {
			if (!it->second.controller) {
				continue;
			}
			TellStick *tellstick = reinterpret_cast<TellStick*>(it->second.controller);
			if (tellstick) {
				tellStickControllers.push_back(tellstick);
			}
		}
	}

	bool reloadControllers = false;
	std::string noop = "N+";
	for(std::list<TellStick *>::iterator it = tellStickControllers.begin(); it != tellStickControllers.end(); ++it) {
		int success = (*it)->send(noop);
		if(success == TELLSTICK_ERROR_BROKEN_PIPE) {
			Log::warning("TellStick query: Error in communication with TellStick, resetting USB");
			resetController(*it);
		}
		if(success == TELLSTICK_ERROR_BROKEN_PIPE || success == TELLSTICK_ERROR_NOT_FOUND) {
			reloadControllers = true;
		}
	}

	if(!tellStickControllers.size() || reloadControllers) {
		// no tellstick at all found, or controller was reset
		Log::debug("TellStick query: Rescanning USB ports");  // only log as debug, since this will happen all the time if no TellStick is connected
		loadControllers();
	}
}

int ControllerManager::resetController(Controller *controller) {
	TellStick *tellstick = reinterpret_cast<TellStick*>(controller);
	if (!tellstick) {
		return true;  // not tellstick, nothing to reset at the moment, just return true
	}
	int success = tellstick->reset();
	deviceInsertedOrRemoved(tellstick->vid(), tellstick->pid(), tellstick->serial(), false);  // remove from list and delete
	return success;
}

std::wstring ControllerManager::getControllers() const {
	TelldusCore::MutexLocker locker(&d->mutex);

	TelldusCore::Message msg;

	msg.addArgument(static_cast<int>(d->controllers.size()));

	for(ControllerMap::iterator it = d->controllers.begin(); it != d->controllers.end(); ++it) {
		msg.addArgument(it->first);
		msg.addArgument(it->second.type);
		msg.addArgument(it->second.name.c_str());
		msg.addArgument(it->second.controller ? 1 : 0);
	}
	return msg;
}

std::wstring ControllerManager::getControllerValue(int id, const std::wstring &name) {
	TelldusCore::MutexLocker locker(&d->mutex);

	ControllerMap::iterator it = d->controllers.find(id);
	if (it == d->controllers.end()) {
		return L"";
	}
	if (name == L"serial") {
		return it->second.serial;
	} else if (name == L"name") {
		return it->second.name;
	} else if (name == L"available") {
		return it->second.controller ? L"1" : L"0";
	} else if (name == L"firmware") {
		if (!it->second.controller) {
			return L"-1";
		}
		return TelldusCore::intToWstring(it->second.controller->firmwareVersion());
	}
	return L"";
}

int ControllerManager::removeController(int id) {
	TelldusCore::MutexLocker locker(&d->mutex);

	ControllerMap::iterator it = d->controllers.find(id);
	if (it == d->controllers.end()) {
		return TELLSTICK_ERROR_NOT_FOUND;
	}
	if (it->second.controller) {
		// Still connected
		return TELLSTICK_ERROR_PERMISSION_DENIED;
	}

	int ret = d->settings.removeNode(Settings::Controller, id);
	if (ret != TELLSTICK_SUCCESS) {
		return ret;
	}

	d->controllers.erase(it);

	signalControllerEvent(id, TELLSTICK_DEVICE_REMOVED, 0, L"");
	return TELLSTICK_SUCCESS;
}

int ControllerManager::setControllerValue(int id, const std::wstring &name, const std::wstring &value) {
	TelldusCore::MutexLocker locker(&d->mutex);

	ControllerMap::iterator it = d->controllers.find(id);
	if (it == d->controllers.end()) {
		return TELLSTICK_ERROR_NOT_FOUND;
	}
	if (name == L"name") {
		it->second.name = value;
		d->settings.setName(Settings::Controller, id, value);
		signalControllerEvent(id, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_NAME, value);
	} else {
		return TELLSTICK_ERROR_SYNTAX;  // TODO(micke): Is this the best error?
	}
	return TELLSTICK_SUCCESS;
}

void ControllerManager::signalControllerEvent(int controllerId, int changeEvent, int changeType, const std::wstring &newValue) {
	EventUpdateData *eventData = new EventUpdateData();
	eventData->messageType = L"TDControllerEvent";
	eventData->controllerId = controllerId;
	eventData->eventState = changeEvent;
	eventData->eventChangeType = changeType;
	eventData->eventValue = newValue;
	d->updateEvent->signal(eventData);
}
