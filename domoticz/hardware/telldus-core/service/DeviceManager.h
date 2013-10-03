//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_DEVICEMANAGER_H_
#define TELLDUS_CORE_SERVICE_DEVICEMANAGER_H_

#include <set>
#include <string>
#include "service/Device.h"
#include "service/ControllerManager.h"
#include "service/ControllerMessage.h"
#include "service/EventUpdateManager.h"

class Sensor;

class DeviceManager {
public:
	DeviceManager(ControllerManager *controllerManager, TelldusCore::EventRef deviceUpdateEvent);
	~DeviceManager(void);
	int getNumberOfDevices(void);
	int addDevice();
	void connectTellStickController(int vid, int pid, const std::string &serial);
	void disconnectTellStickController(int vid, int pid, const std::string &serial);
	void executeActionEvent();
	int getDeviceId(int deviceIndex);
	int getDeviceLastSentCommand(int deviceId, int methodsSupported);
	int setDeviceLastSentCommand(int deviceId, int command, const std::wstring &value);
	int getDeviceMethods(int deviceId);
	int getDeviceMethods(int deviceId, int methodsSupported);
	std::wstring getDeviceModel(int deviceId);
	int setDeviceModel(int deviceId, const std::wstring &model);
	std::wstring getDeviceName(int deviceId);
	int setDeviceName(int deviceId, const std::wstring &name);
	std::wstring getDeviceParameter(int deviceId, const std::wstring &name, const std::wstring &defauleValue);
	int setDeviceParameter(int deviceId, const std::wstring &name, const std::wstring &value);
	std::wstring getDeviceProtocol(int deviceId);
	int setDeviceProtocol(int deviceId, const std::wstring &name);
	std::wstring getDeviceStateValue(int deviceId);
	int getDeviceType(int deviceId);
	int getPreferredControllerId(int deviceId);
	int doAction(int deviceId, int action, unsigned char data);
	int removeDevice(int deviceId);
	int sendRawCommand(const std::wstring &command, int reserved);

	void setExecuteActionEvent(TelldusCore::EventRef event);

	std::wstring getSensors() const;
	std::wstring getSensorValue(const std::wstring &protocol, const std::wstring &model, int id, int dataType) const;

	void handleControllerMessage(const ControllerEventData &event);

private:
	void handleSensorMessage(const ControllerMessage &msg);
	void setSensorValueAndSignal( const std::string &dataType, int dataTypeId, Sensor *sensor, const ControllerMessage &msg, time_t timestamp) const;
	int getDeviceMethods(int deviceId, std::set<int> *duplicateDeviceIds);
	int doGroupSceneAction(int deviceId, int action, unsigned char data);
	int executeScene(std::wstring singledevice, int groupDeviceId);
	bool triggerDeviceStateChange(int deviceId, int intDeviceState, const std::wstring &strDeviceStateValue );
	void fillDevices(void);

	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_DEVICEMANAGER_H_
