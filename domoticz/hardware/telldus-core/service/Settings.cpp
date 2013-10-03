//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/Settings.h"
#include <string>

TelldusCore::Mutex Settings::mutex;

/*
* Get the name of the device
*/
std::wstring Settings::getName(Node type, int intNodeId) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getStringSetting(type, intNodeId, L"name", false);
}

/*
* Set the name of the device
*/
int Settings::setName(Node type, int intDeviceId, const std::wstring &strNewName) {
	TelldusCore::MutexLocker locker(&mutex);
	return setStringSetting(type, intDeviceId, L"name", strNewName, false);
}

/*
* Get the device vendor
*/
std::wstring Settings::getProtocol(int intDeviceId) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getStringSetting(Device, intDeviceId, L"protocol", false);
}

/*
* Set the device vendor
*/
int Settings::setProtocol(int intDeviceId, const std::wstring &strVendor) {
	TelldusCore::MutexLocker locker(&mutex);
	return setStringSetting(Device, intDeviceId, L"protocol", strVendor, false);
}

/*
* Get the device model
*/
std::wstring Settings::getModel(int intDeviceId) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getStringSetting(Device, intDeviceId, L"model", false);
}

/*
* Set the device model
*/
int Settings::setModel(int intDeviceId, const std::wstring &strModel) {
	TelldusCore::MutexLocker locker(&mutex);
	return setStringSetting(Device, intDeviceId, L"model", strModel, false);
}

/*
* Set device argument
*/
int Settings::setDeviceParameter(int intDeviceId, const std::wstring &strName, const std::wstring &strValue) {
	TelldusCore::MutexLocker locker(&mutex);
	return setStringSetting(Device, intDeviceId, strName, strValue, true);
}

/*
* Get device argument
*/
std::wstring Settings::getDeviceParameter(int intDeviceId, const std::wstring &strName) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getStringSetting(Device, intDeviceId, strName, true);
}

/*
* Set preferred controller id
*/
int Settings::setPreferredControllerId(int intDeviceId, int value) {
	TelldusCore::MutexLocker locker(&mutex);
	return setIntSetting(Device, intDeviceId,  L"controller", value, false);
}

/*
* Get preferred controller id
*/
int Settings::getPreferredControllerId(int intDeviceId) {
	TelldusCore::MutexLocker locker(&mutex);
	return getIntSetting(Device, intDeviceId, L"controller", false);
}

std::wstring Settings::getControllerSerial(int intControllerId) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getStringSetting(Controller, intControllerId, L"serial", false);
}

int Settings::setControllerSerial(int intControllerId, const std::wstring &serial) {
	TelldusCore::MutexLocker locker(&mutex);
	return setStringSetting(Controller, intControllerId,  L"serial", serial, false);
}

int Settings::getControllerType(int intControllerId) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getIntSetting(Controller, intControllerId, L"type", false);
}

int Settings::setControllerType(int intControllerId, int type) {
	TelldusCore::MutexLocker locker(&mutex);
	return setIntSetting(Controller, intControllerId,  L"type", type, false);
}

std::string Settings::getNodeString(Settings::Node type) const {
	if (type == Device) {
		return "device";
	} else if (type == Controller) {
		return "controller";
	}
	return "";
}

#ifndef _CONFUSE

bool Settings::setDeviceState( int intDeviceId, int intDeviceState, const std::wstring &strDeviceStateValue ) {
	TelldusCore::MutexLocker locker(&mutex);
	bool retval = setIntSetting( Settings::Device, intDeviceId, L"state", intDeviceState, true );
	setStringSetting( Settings::Device, intDeviceId, L"stateValue", strDeviceStateValue, true );
	return retval;
}

int Settings::getDeviceState( int intDeviceId ) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getIntSetting( Settings::Device, intDeviceId, L"state", true );
}

std::wstring Settings::getDeviceStateValue( int intDeviceId ) const {
	TelldusCore::MutexLocker locker(&mutex);
	return getStringSetting( Settings::Device, intDeviceId, L"stateValue", true );
}

#endif
