/*
 *  CallbackDispatcher.cpp
 *  telldus-core
 *
 *  Created by Micke Prag on 2010-11-02.
 *  Copyright 2010 Telldus Technologies AB. All rights reserved.
 *
 */

#include "client/CallbackDispatcher.h"

namespace TelldusCore {

TDEventDispatcher::TDEventDispatcher(EventDataRef cbd, CallbackStruct *cb, EventRef cbDone)
	:Thread(), doneRunning(false), callbackData(cbd), callback(cb), callbackExecuted(cbDone) {
	this->startAndLock(&callback->mutex);
}

TDEventDispatcher::~TDEventDispatcher() {
	this->wait();
}

bool TDEventDispatcher::done() const {
	return doneRunning;
}

void TDEventDispatcher::run() {
	this->fireEvent();
	doneRunning = true;
	callbackExecuted->signal();
}

void TDEventDispatcher::fireEvent() {
	if (callback->type == CallbackStruct::DeviceEvent) {
		DeviceEventCallbackData *data = dynamic_cast<DeviceEventCallbackData *>(callbackData.get());
		if (!data) {
			return;
		}
		((TDDeviceEvent)callback->event)(data->deviceId, data->deviceState, data->deviceStateValue.c_str(), callback->id, callback->context);

	} else if (callback->type == CallbackStruct::DeviceChangeEvent) {
		DeviceChangeEventCallbackData *data = dynamic_cast<DeviceChangeEventCallbackData *>(callbackData.get());
		if (!data) {
			return;
		}
		((TDDeviceChangeEvent)callback->event)(data->deviceId, data->changeEvent, data->changeType, callback->id, callback->context);

	} else if (callback->type == CallbackStruct::RawDeviceEvent) {
		RawDeviceEventCallbackData *data = dynamic_cast<RawDeviceEventCallbackData *>(callbackData.get());
		if (!data) {
			return;
		}
		((TDRawDeviceEvent)callback->event)(data->data.c_str(), data->controllerId, callback->id, callback->context);

	} else if (callback->type == CallbackStruct::SensorEvent) {
		SensorEventCallbackData *data = dynamic_cast<SensorEventCallbackData *>(callbackData.get());
		if (!data) {
			return;
		}
		((TDSensorEvent)callback->event)(data->protocol.c_str(), data->model.c_str(), data->id, data->dataType, data->value.c_str(), data->timestamp, callback->id, callback->context);

	} else if (callback->type == CallbackStruct::ControllerEvent) {
		ControllerEventCallbackData *data = dynamic_cast<ControllerEventCallbackData *>(callbackData.get());
		if (!data) {
			return;
		}
		((TDControllerEvent)callback->event)(data->controllerId, data->changeEvent, data->changeType, data->newValue.c_str(), callback->id, callback->context);
	}
}

}  // namespace TelldusCore
