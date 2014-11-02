//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/Device.h"
#include <list>
#include <string>
#include "service/Settings.h"
#include "service/TellStick.h"

class Device::PrivateData {
public:
	std::wstring model;
	std::wstring name;
	ParameterMap parameterList;
	Protocol *protocol;
	std::wstring protocolName;
	int preferredControllerId;
	int state;
	std::wstring stateValue;
};

Device::Device(int id)
	:Mutex() {
	d = new PrivateData;
	d->protocol = 0;
	d->preferredControllerId = 0;
	d->state = 0;
}

Device::~Device(void) {
	delete d->protocol;
	delete d;
}

/**
* Get-/Set-methods
*/

int Device::getLastSentCommand(int methodsSupported) {
	int lastSentCommand = Device::maskUnsupportedMethods(d->state, methodsSupported);

	if (lastSentCommand == TELLSTICK_BELL) {
		// Bell is not a state
		lastSentCommand = TELLSTICK_TURNOFF;
	}
	if (lastSentCommand == 0) {
		lastSentCommand = TELLSTICK_TURNOFF;
	}
	return lastSentCommand;
}

int Device::getMethods() const {
	Protocol *p = this->retrieveProtocol();
	if (p) {
		return p->methods();
	}
	return 0;
}

void Device::setLastSentCommand(int command, std::wstring value) {
	d->state = command;
	d->stateValue = value;
}

std::wstring Device::getModel() {
	return d->model;
}

void Device::setModel(const std::wstring &model) {
	if(d->protocol) {
		delete(d->protocol);
		d->protocol = 0;
	}
	d->model = model;
}

std::wstring Device::getName() {
	return d->name;
}

void Device::setName(const std::wstring &name) {
	d->name = name;
}

std::wstring Device::getParameter(const std::wstring &key) {
	ParameterMap::iterator it = d->parameterList.find(key);
	if (it == d->parameterList.end()) {
		return L"";
	}
	return d->parameterList[key];
}

std::list<std::string> Device::getParametersForProtocol() const {
	return Protocol::getParametersForProtocol(getProtocolName());
}

void Device::setParameter(const std::wstring &key, const std::wstring &value) {
	d->parameterList[key] = value;
	if(d->protocol) {
		d->protocol->setParameters(d->parameterList);
	}
}

int Device::getPreferredControllerId() {
	return d->preferredControllerId;
}

void Device::setPreferredControllerId(int controllerId) {
	d->preferredControllerId = controllerId;
}

std::wstring Device::getProtocolName() const {
	return d->protocolName;
}

void Device::setProtocolName(const std::wstring &protocolName) {
	if(d->protocol) {
		delete(d->protocol);
		d->protocol = 0;
	}
	d->protocolName = protocolName;
}

std::wstring Device::getStateValue() {
	return d->stateValue;
}

int Device::getType() {
	if(d->protocolName == L"group") {
		return TELLSTICK_TYPE_GROUP;
	} else if(d->protocolName == L"scene") {
		return TELLSTICK_TYPE_SCENE;
	}
	return TELLSTICK_TYPE_DEVICE;
}

/**
* End Get-/Set
*/

int Device::doAction(int action, unsigned char data, Controller *controller) {
	Protocol *p = this->retrieveProtocol();
	if (!p) {
		// Syntax error in configuration, no such protocol
		return TELLSTICK_ERROR_CONFIG_SYNTAX;
	}
	// Try to determine if we need to call another method due to masking
	int method = this->isMethodSupported(action);
	if (method <= 0) {
		return TELLSTICK_ERROR_METHOD_NOT_SUPPORTED;
	}
	std::string code = p->getStringForMethod(method, data, controller);
	if (code == "") {
		return TELLSTICK_ERROR_METHOD_NOT_SUPPORTED;
	}
	if (code[0] != 'S' && code[0] != 'T' && code[0] != 'P' && code[0] != 'R') {
		// Try autodetect sendtype
		TellStick *tellstick = reinterpret_cast<TellStick *>(controller);
		if (!tellstick) {
			return TELLSTICK_ERROR_UNKNOWN;
		}
		unsigned int maxlength = 80;
		if (tellstick->pid() == 0x0c31) {
			maxlength = 512;
		}
		if (code.length() <= maxlength) {
			// S is enough
			code.insert(0, 1, 'S');
			code.append(1, '+');
		} else {
			code = TellStick::createTPacket(code);
		}
	}
	return controller->send(code);
}

int Device::isMethodSupported(int method) const {
	Protocol *p = this->retrieveProtocol();
	if (!p) {
		// Syntax error in configuration, no such protocol
		return TELLSTICK_ERROR_CONFIG_SYNTAX;
	}
	// Try to determine if we need to call another method due to masking
	int methods = p->methods();
	if ((method & methods) == 0) {
		// Loop all methods an see if any method masks to this one
		for(int i = 1; i <= methods; i <<= 1) {
			if ((i & methods) == 0) {
				continue;
			}
			if (this->maskUnsupportedMethods(i, method)) {
				method = i;
				break;
			}
		}
	}
	if ((method & methods) == 0) {
		return TELLSTICK_ERROR_METHOD_NOT_SUPPORTED;
	}
	return method;
}

Protocol* Device::retrieveProtocol() const {
	if (d->protocol) {
		return d->protocol;
	}

	d->protocol = Protocol::getProtocolInstance(d->protocolName);
	if(d->protocol) {
		d->protocol->setModel(d->model);
		d->protocol->setParameters(d->parameterList);
		return d->protocol;
	}

	return 0;
}

int Device::maskUnsupportedMethods(int methods, int supportedMethods) {
	// Bell -> On
	if ((methods & TELLSTICK_BELL) && !(supportedMethods & TELLSTICK_BELL)) {
		methods |= TELLSTICK_TURNON;
	}

	// Execute -> On
	if ((methods & TELLSTICK_EXECUTE) && !(supportedMethods & TELLSTICK_EXECUTE)) {
		methods |= TELLSTICK_TURNON;
	}

	// Up -> Off
	if ((methods & TELLSTICK_UP) && !(supportedMethods & TELLSTICK_UP)) {
		methods |= TELLSTICK_TURNOFF;
	}

	// Down -> On
	if ((methods & TELLSTICK_DOWN) && !(supportedMethods & TELLSTICK_DOWN)) {
		methods |= TELLSTICK_TURNON;
	}

	// Cut of the rest of the unsupported methods we don't have a fallback for
	return methods & supportedMethods;
}

int Device::methodId( const std::string &methodName ) {
	if (methodName.compare("turnon") == 0) {
		return TELLSTICK_TURNON;
	}
	if (methodName.compare("turnoff") == 0) {
		return TELLSTICK_TURNOFF;
	}
	if (methodName.compare("bell") == 0) {
		return TELLSTICK_BELL;
	}
	if (methodName.compare("dim") == 0) {
		return TELLSTICK_DIM;
	}
	if (methodName.compare("execute") == 0) {
		return TELLSTICK_EXECUTE;
	}
	if (methodName.compare("up") == 0) {
		return TELLSTICK_UP;
	}
	if (methodName.compare("down") == 0) {
		return TELLSTICK_DOWN;
	}
	if (methodName.compare("stop") == 0) {
		return TELLSTICK_STOP;
	}
	return 0;
}
