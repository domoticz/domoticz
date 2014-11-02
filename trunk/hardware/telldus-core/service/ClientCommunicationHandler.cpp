//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ClientCommunicationHandler.h"

#include <stdlib.h>
#include <string>

#include "common/Message.h"
#include "common/Strings.h"

class ClientCommunicationHandler::PrivateData {
public:
	TelldusCore::Socket *clientSocket;
	TelldusCore::EventRef event, deviceUpdateEvent;
	bool done;
	DeviceManager *deviceManager;
	ControllerManager *controllerManager;
};

ClientCommunicationHandler::ClientCommunicationHandler() {
}

ClientCommunicationHandler::ClientCommunicationHandler(TelldusCore::Socket *clientSocket, TelldusCore::EventRef event, DeviceManager *deviceManager, TelldusCore::EventRef deviceUpdateEvent, ControllerManager *controllerManager)
	:Thread() {
	d = new PrivateData;
	d->clientSocket = clientSocket;
	d->event = event;
	d->done = false;
	d->deviceManager = deviceManager;
	d->deviceUpdateEvent = deviceUpdateEvent;
	d->controllerManager = controllerManager;
}

ClientCommunicationHandler::~ClientCommunicationHandler(void) {
	wait();
	delete(d->clientSocket);
	delete d;
}

void ClientCommunicationHandler::run() {
	// run thread

	std::wstring clientMessage = d->clientSocket->read(2000);

	int intReturn;
	std::wstring strReturn;
	strReturn = L"";
	parseMessage(clientMessage, &intReturn, &strReturn);

	TelldusCore::Message msg;

	if(strReturn == L"") {
		msg.addArgument(intReturn);
	} else {
		msg.addArgument(strReturn);
	}
	msg.append(L"\n");
	d->clientSocket->write(msg);

	// We are done, signal for removal
	d->done = true;
	d->event->signal();
}

bool ClientCommunicationHandler::isDone() {
	return d->done;
}


void ClientCommunicationHandler::parseMessage(const std::wstring &clientMessage, int *intReturn, std::wstring *wstringReturn) {
	(*intReturn) = 0;
	(*wstringReturn) = L"";
	std::wstring msg(clientMessage);  // Copy
	std::wstring function(TelldusCore::Message::takeString(&msg));

	if (function == L"tdTurnOn") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_TURNON, 0);

	} else if (function == L"tdTurnOff") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_TURNOFF, 0);

	} else if (function == L"tdBell") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_BELL, 0);

	} else if (function == L"tdDim") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		int level = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_DIM, level);

	} else if (function == L"tdExecute") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_EXECUTE, 0);

	} else if (function == L"tdUp") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_UP, 0);

	} else if (function == L"tdDown") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_DOWN, 0);

	} else if (function == L"tdStop") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_STOP, 0);

	} else if (function == L"tdLearn") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->doAction(deviceId, TELLSTICK_LEARN, 0);

	} else if (function == L"tdLastSentCommand") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		int methodsSupported = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->getDeviceLastSentCommand(deviceId, methodsSupported);

	} else if (function == L"tdLastSentValue") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*wstringReturn) = d->deviceManager->getDeviceStateValue(deviceId);

	} else if(function == L"tdGetNumberOfDevices") {
		(*intReturn) = d->deviceManager->getNumberOfDevices();

	} else if (function == L"tdGetDeviceId") {
		int deviceIndex = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->getDeviceId(deviceIndex);

	} else if (function == L"tdGetDeviceType") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->getDeviceType(deviceId);

	} else if (function == L"tdGetName") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*wstringReturn) = d->deviceManager->getDeviceName(deviceId);

	} else if (function == L"tdSetName") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		std::wstring name = TelldusCore::Message::takeString(&msg);
		(*intReturn) = d->deviceManager->setDeviceName(deviceId, name);
		sendDeviceSignal(deviceId, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_NAME);

	} else if (function == L"tdGetProtocol") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*wstringReturn) = d->deviceManager->getDeviceProtocol(deviceId);

	} else if (function == L"tdSetProtocol") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		std::wstring protocol = TelldusCore::Message::takeString(&msg);
		int oldMethods = d->deviceManager->getDeviceMethods(deviceId);
		(*intReturn) = d->deviceManager->setDeviceProtocol(deviceId, protocol);
		sendDeviceSignal(deviceId, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_PROTOCOL);
		if(oldMethods != d->deviceManager->getDeviceMethods(deviceId)) {
			sendDeviceSignal(deviceId, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_METHOD);
		}

	} else if (function == L"tdGetModel") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*wstringReturn) = d->deviceManager->getDeviceModel(deviceId);

	} else if (function == L"tdSetModel") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		std::wstring model = TelldusCore::Message::takeString(&msg);
		int oldMethods = d->deviceManager->getDeviceMethods(deviceId);
		(*intReturn) = d->deviceManager->setDeviceModel(deviceId, model);
		sendDeviceSignal(deviceId, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_MODEL);
		if(oldMethods != d->deviceManager->getDeviceMethods(deviceId)) {
			sendDeviceSignal(deviceId, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_METHOD);
		}

	} else if (function == L"tdGetDeviceParameter") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		std::wstring name = TelldusCore::Message::takeString(&msg);
		std::wstring defaultValue = TelldusCore::Message::takeString(&msg);
		(*wstringReturn) = d->deviceManager->getDeviceParameter(deviceId, name, defaultValue);

	} else if (function == L"tdSetDeviceParameter") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		std::wstring name = TelldusCore::Message::takeString(&msg);
		std::wstring value = TelldusCore::Message::takeString(&msg);
		int oldMethods = d->deviceManager->getDeviceMethods(deviceId);
		(*intReturn) = d->deviceManager->setDeviceParameter(deviceId, name, value);
		if(oldMethods != d->deviceManager->getDeviceMethods(deviceId)) {
			sendDeviceSignal(deviceId, TELLSTICK_DEVICE_CHANGED, TELLSTICK_CHANGE_METHOD);
		}

	} else if (function == L"tdAddDevice") {
		(*intReturn) = d->deviceManager->addDevice();
		if((*intReturn) >= 0) {
			sendDeviceSignal((*intReturn), TELLSTICK_DEVICE_ADDED, 0);
		}

	} else if (function == L"tdRemoveDevice") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->removeDevice(deviceId);
		if((*intReturn) == TELLSTICK_SUCCESS) {
			sendDeviceSignal(deviceId, TELLSTICK_DEVICE_REMOVED, 0);
		}

	} else if (function == L"tdMethods") {
		int deviceId = TelldusCore::Message::takeInt(&msg);
		int intMethodsSupported = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->getDeviceMethods(deviceId, intMethodsSupported);

	} else if (function == L"tdSendRawCommand") {
		std::wstring command = TelldusCore::Message::takeString(&msg);
		int reserved = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->deviceManager->sendRawCommand(command, reserved);

	} else if (function == L"tdConnectTellStickController") {
		int vid = TelldusCore::Message::takeInt(&msg);
		int pid = TelldusCore::Message::takeInt(&msg);
		std::string serial = TelldusCore::wideToString(TelldusCore::Message::takeString(&msg));
		d->deviceManager->connectTellStickController(vid, pid, serial);

	} else if (function == L"tdDisconnectTellStickController") {
		int vid = TelldusCore::Message::takeInt(&msg);
		int pid = TelldusCore::Message::takeInt(&msg);
		std::string serial = TelldusCore::wideToString(TelldusCore::Message::takeString(&msg));
		d->deviceManager->disconnectTellStickController(vid, pid, serial);

	} else if (function == L"tdSensor") {
		(*wstringReturn) = d->deviceManager->getSensors();

	} else if (function == L"tdSensorValue") {
		std::wstring protocol = TelldusCore::Message::takeString(&msg);
		std::wstring model = TelldusCore::Message::takeString(&msg);
		int id = TelldusCore::Message::takeInt(&msg);
		int dataType = TelldusCore::Message::takeInt(&msg);
		(*wstringReturn) = d->deviceManager->getSensorValue(protocol, model, id, dataType);

	} else if (function == L"tdController") {
		(*wstringReturn) = d->controllerManager->getControllers();

	} else if (function == L"tdControllerValue") {
		int id = TelldusCore::Message::takeInt(&msg);
		std::wstring name = TelldusCore::Message::takeString(&msg);
		(*wstringReturn) = d->controllerManager->getControllerValue(id, name);

	} else if (function == L"tdSetControllerValue") {
		int id = TelldusCore::Message::takeInt(&msg);
		std::wstring name = TelldusCore::Message::takeString(&msg);
		std::wstring value = TelldusCore::Message::takeString(&msg);
		(*intReturn) = d->controllerManager->setControllerValue(id, name, value);

	} else if (function == L"tdRemoveController") {
		int controllerId = TelldusCore::Message::takeInt(&msg);
		(*intReturn) = d->controllerManager->removeController(controllerId);

	} else {
		(*intReturn) = TELLSTICK_ERROR_UNKNOWN;
	}
}

void ClientCommunicationHandler::sendDeviceSignal(int deviceId, int eventDeviceChanges, int eventChangeType) {
	EventUpdateData *eventData = new EventUpdateData();
	eventData->messageType = L"TDDeviceChangeEvent";
	eventData->deviceId = deviceId;
	eventData->eventDeviceChanges = eventDeviceChanges;
	eventData->eventChangeType = eventChangeType;
	d->deviceUpdateEvent->signal(eventData);
}
