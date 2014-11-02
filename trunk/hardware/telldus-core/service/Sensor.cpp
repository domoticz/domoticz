//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/Sensor.h"
#include <map>
#include <string>
#include "common/common.h"
#include "client/telldus-core.h"

class Sensor::PrivateData {
public:
	std::wstring protocol, model;
	int id;
	std::map<int, std::string> values;
	time_t timestamp;
};

Sensor::Sensor(const std::wstring &protocol, const std::wstring &model, int id)
	:Mutex() {
	d = new PrivateData;
	d->protocol = protocol;
	d->model = model;
	d->id = id;
}

Sensor::~Sensor() {
	delete d;
}

std::wstring Sensor::protocol() const {
	return d->protocol;
}

std::wstring Sensor::model() const {
	return d->model;
}

int Sensor::id() const {
	return d->id;
}

time_t Sensor::timestamp() const {
	return d->timestamp;
}

int Sensor::dataTypes() const {
	int retval = 0;
	for (std::map<int, std::string>::iterator it = d->values.begin(); it != d->values.end(); ++it) {
		retval |= (*it).first;
	}
	return retval;
}

void Sensor::setValue(int type, const std::string &value, time_t timestamp) {
	if (value.substr(0, 2).compare("0x") == 0) {
		int intval = strtol(value.c_str(), NULL, 16);
		d->values[type] = TelldusCore::intToString(intval);
	} else {
		d->values[type] = value;
	}
	d->timestamp = timestamp;
}

std::string Sensor::value(int type) const {
	std::map<int, std::string>::const_iterator it = d->values.find(type);
	if (it == d->values.end()) {
		return "";
	}
	return (*it).second;
}
