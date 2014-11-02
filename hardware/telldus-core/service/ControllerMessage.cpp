//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ControllerMessage.h"
#include <map>
#include <string>
#include "service/Device.h"
#include "common/Strings.h"
#include "common/common.h"

class ControllerMessage::PrivateData {
public:
	std::map<std::string, std::string> parameters;
	std::string protocol, model, msgClass;
	int method;
};

ControllerMessage::ControllerMessage(const std::string &message) {
	d = new PrivateData;

	// Process our message into bits
	size_t prevPos = 0;
	size_t pos = message.find(";");
	while(pos != std::string::npos) {
		std::string param = message.substr(prevPos, pos-prevPos);
		prevPos = pos+1;
		size_t delim = param.find(":");
		if (delim == std::string::npos) {
			break;
		}
		if (param.substr(0, delim).compare("class") == 0) {
			d->msgClass = param.substr(delim+1, param.length()-delim);
		} else if (param.substr(0, delim).compare("protocol") == 0) {
			d->protocol = param.substr(delim+1, param.length()-delim);
		} else if (param.substr(0, delim).compare("model") == 0) {
			d->model = param.substr(delim+1, param.length()-delim);
		} else if (param.substr(0, delim).compare("method") == 0) {
			d->method = Device::methodId(param.substr(delim+1, param.length()-delim));
		} else {
			d->parameters[param.substr(0, delim)] = param.substr(delim+1, param.length()-delim);
		}
		pos = message.find(";", pos+1);
	}
}

ControllerMessage::~ControllerMessage() {
	delete d;
}

std::string ControllerMessage::msgClass() const {
	return d->msgClass;
}

int ControllerMessage::method() const {
	return d->method;
}

std::wstring ControllerMessage::protocol() const {
	return TelldusCore::charToWstring(d->protocol.c_str());
}

std::wstring ControllerMessage::model() const {
	return TelldusCore::charToWstring(d->model.c_str());
}

uint64_t ControllerMessage::getInt64Parameter(const std::string &key) const {
	std::string strValue = getParameter(key);
	if (strValue.compare("") == 0) {
		return -1;
	}
	if (strValue.substr(0, 2).compare("0x") == 0) {
		return TelldusCore::hexTo64l(strValue);
	}
	// TODO(micke): strtol() does not return uint64_t. Create a platform independent version similar to hexTo64l()
	return strtol(strValue.c_str(), NULL, 10);
}

std::string ControllerMessage::getParameter(const std::string &key) const {
	std::map<std::string, std::string>::iterator it = d->parameters.find(key);
	if (it == d->parameters.end()) {
		return "";
	}
	return d->parameters[key];
}

bool ControllerMessage::hasParameter(const std::string &key) const {
	std::map<std::string, std::string>::iterator it = d->parameters.find(key);
	if (it == d->parameters.end()) {
		return false;
	}
	return true;
}
