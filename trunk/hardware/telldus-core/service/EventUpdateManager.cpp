//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/EventUpdateManager.h"

#ifdef _LINUX
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif  // _LINUX

#include <list>
#include <map>
#include <memory>
#ifdef _LINUX
#include <string>
#include <vector>
#endif  // _LINUX

#include "common/EventHandler.h"
#include "common/Message.h"
#include "common/Socket.h"
#include "common/Strings.h"
#include "service/config.h"
#include "service/ConnectionListener.h"
#include "service/Log.h"

typedef std::list<TelldusCore::Socket *> SocketList;
typedef std::list<std::string> StringList;

class EventUpdateManager::PrivateData {
public:
	TelldusCore::EventHandler eventHandler;
	TelldusCore::EventRef stopEvent, updateEvent, clientConnectEvent;
	SocketList clients;
	ConnectionListener *eventUpdateClientListener;
#ifdef _LINUX
	std::map<std::string, StringList> fileList;
#endif  // _LINUX
};

EventUpdateManager::EventUpdateManager()
	:Thread() {
	d = new PrivateData;
	d->stopEvent = d->eventHandler.addEvent();
	d->updateEvent = d->eventHandler.addEvent();
	d->clientConnectEvent = d->eventHandler.addEvent();
	d->eventUpdateClientListener = new ConnectionListener(L"TelldusEvents", d->clientConnectEvent);
#ifdef _LINUX
	loadScripts("deviceevent");
	loadScripts("devicechangeevent");
	loadScripts("rawdeviceevent");
	loadScripts("sensorevent");
	loadScripts("controllerevent");
#endif  // _LINUX
}

EventUpdateManager::~EventUpdateManager(void) {
	d->stopEvent->signal();
	wait();
	delete d->eventUpdateClientListener;

	for (SocketList::iterator it = d->clients.begin(); it != d->clients.end(); ++it) {
		delete(*it);
	}

	delete d;
}

TelldusCore::EventRef EventUpdateManager::retrieveUpdateEvent() {
	return d->updateEvent;
}

void EventUpdateManager::run() {
	while(!d->stopEvent->isSignaled()) {
		if (!d->eventHandler.waitForAny()) {
			continue;
		}

		if(d->clientConnectEvent->isSignaled()) {
			// new client added
			TelldusCore::EventDataRef eventData = d->clientConnectEvent->takeSignal();
			ConnectionListenerEventData *data = dynamic_cast<ConnectionListenerEventData*>(eventData.get());
			if(data) {
				d->clients.push_back(data->socket);
			}
		} else if(d->updateEvent->isSignaled()) {
			// device event, signal all clients
			TelldusCore::EventDataRef eventData = d->updateEvent->takeSignal();
			EventUpdateData *data = reinterpret_cast<EventUpdateData*>(eventData.get());
			if(data) {
				sendMessageToClients(data);
				executeScripts(data);
			}
		}
	}
}

void EventUpdateManager::loadScripts(const std::string &folder) {
#ifdef _LINUX
	std::string path = TelldusCore::formatf("%s/%s", SCRIPT_PATH, folder.c_str());
	struct dirent **namelist;
	int count = scandir(path.c_str(), &namelist, NULL, alphasort);
	if (count < 0) {
		return;
	}

	for(int i = 0; i < count; ++i) {
		if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0) {
			d->fileList[folder].push_back(namelist[i]->d_name);
		}
		free(namelist[i]);
	}
	free(namelist);
#endif  // _LINUX
}

void EventUpdateManager::sendMessageToClients(EventUpdateData *data) {
	int connected = 0;
	for(SocketList::iterator it = d->clients.begin(); it != d->clients.end();) {
		if((*it)->isConnected()) {
			connected++;
			TelldusCore::Message msg;

			if(data->messageType == L"TDDeviceEvent") {
				msg.addArgument("TDDeviceEvent");
				msg.addArgument(data->deviceId);
				msg.addArgument(data->eventState);
				msg.addArgument(data->eventValue);  // string
			} else if(data->messageType == L"TDDeviceChangeEvent") {
				msg.addArgument("TDDeviceChangeEvent");
				msg.addArgument(data->deviceId);
				msg.addArgument(data->eventDeviceChanges);
				msg.addArgument(data->eventChangeType);
			} else if(data->messageType == L"TDRawDeviceEvent") {
				msg.addArgument("TDRawDeviceEvent");
				msg.addArgument(data->eventValue);  // string
				msg.addArgument(data->controllerId);
			} else if(data->messageType == L"TDSensorEvent") {
				msg.addArgument("TDSensorEvent");
				msg.addArgument(data->protocol);
				msg.addArgument(data->model);
				msg.addArgument(data->sensorId);
				msg.addArgument(data->dataType);
				msg.addArgument(data->value);
				msg.addArgument(data->timestamp);
			} else if(data->messageType == L"TDControllerEvent") {
				msg.addArgument("TDControllerEvent");
				msg.addArgument(data->controllerId);
				msg.addArgument(data->eventState);
				msg.addArgument(data->eventChangeType);
				msg.addArgument(data->eventValue);
			}

			(*it)->write(msg);

			it++;
		} else {
			// connection is dead, remove it
			delete *it;
			it = d->clients.erase(it);
		}
	}
}

void EventUpdateManager::executeScripts(EventUpdateData *data) {
#ifdef _LINUX
	std::string dir;
	std::vector<std::string> env;

	// Create a copy of the environment
	unsigned int size = 0;
	for(; ; ++size) {
		if (environ[size] == 0) {
			break;
		}
	}
	env.reserve(size + 6);  // 6 is the most used extra environmental variables any event uses
	for(unsigned int i = 0; i < size; ++i) {
		env.push_back(environ[i]);
	}

	if(data->messageType == L"TDDeviceEvent") {
		dir = "deviceevent";
		env.push_back(TelldusCore::formatf("DEVICEID=%i", data->deviceId));
		env.push_back(TelldusCore::formatf("METHOD=%i", data->eventState));
		env.push_back(TelldusCore::formatf("METHODDATA=%s", TelldusCore::wideToString(data->eventValue).c_str()));
	} else if(data->messageType == L"TDDeviceChangeEvent") {
		dir = "devicechangeevent";
		env.push_back(TelldusCore::formatf("DEVICEID=%i", data->deviceId));
		env.push_back(TelldusCore::formatf("CHANGEEVENT=%i", data->eventDeviceChanges));
		env.push_back(TelldusCore::formatf("CHANGETYPE=%i", data->eventChangeType));
	} else if(data->messageType == L"TDRawDeviceEvent") {
		dir = "rawdeviceevent";
		env.push_back(TelldusCore::formatf("RAWDATA=%s", TelldusCore::wideToString(data->eventValue).c_str()));  // string
		env.push_back(TelldusCore::formatf("CONTROLLERID=%i", data->controllerId));
	} else if (data->messageType == L"TDSensorEvent") {
		dir = "sensorevent";
		env.push_back(TelldusCore::formatf("PROTOCOL=%s", TelldusCore::wideToString(data->protocol).c_str()));
		env.push_back(TelldusCore::formatf("MODEL=%s", TelldusCore::wideToString(data->model).c_str()));
		env.push_back(TelldusCore::formatf("SENSORID=%i", data->sensorId));
		env.push_back(TelldusCore::formatf("DATATYPE=%i", data->dataType));
		env.push_back(TelldusCore::formatf("VALUE=%s", TelldusCore::wideToString(data->value).c_str()));
		env.push_back(TelldusCore::formatf("TIMESTAMP=%i", data->timestamp));
	} else if(data->messageType == L"TDControllerEvent") {
		dir = "controllerevent";
		env.push_back(TelldusCore::formatf("CONTROLLERID=%i", data->controllerId));
		env.push_back(TelldusCore::formatf("CHANGEEVENT=%i", data->eventState));
		env.push_back(TelldusCore::formatf("CHANGETYPE=%i", data->eventChangeType));
		env.push_back(TelldusCore::formatf("VALUE=%s", TelldusCore::wideToString(data->eventValue).c_str()));
	} else {
		// Unknown event, should not happen
		return;
	}

	char *newEnv[env.size()+1];  // +1 for the last stop element
	for(int i = 0; i < env.size(); ++i) {
		newEnv[i] = new char[env.at(i).length()+1];
		snprintf(newEnv[i], env.at(i).length()+1, "%s", env.at(i).c_str());
	}
	newEnv[env.size()] = 0;  // Mark end of array

	for(StringList::iterator it = d->fileList[dir].begin(); it != d->fileList[dir].end(); ++it) {
		executeScript(TelldusCore::formatf("%s/%s/%s", SCRIPT_PATH, dir.c_str(), (*it).c_str()), (*it), newEnv);
	}
	// Cleanup
	for(int i = 0; newEnv[i] != 0; ++i) {
		delete[] newEnv[i];
	}
#endif  // _LINUX
}

void EventUpdateManager::executeScript(std::string script, const std::string &name, char ** env) {
#ifdef _LINUX
	pid_t pid = fork();
	if (pid == -1) {
		Log::error("Could not fork() to execute script %s", script.c_str());
		return;
	}

	if (pid == 0) {
		char *n = new char[name.length()+1];
		snprintf(n, name.length()+1, "%s", name.c_str());
		static char * argv[] = { n };
		execve(script.c_str(), argv, env);
		delete[] n;
		Log::error("Could not execute %s (%i): %s", script.c_str(), errno, strerror(errno));
		exit(1);
	}
#endif  // _LINUX
}
