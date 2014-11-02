//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_SENSOR_H_
#define TELLDUS_CORE_SERVICE_SENSOR_H_

#include <string>
#include "common/Mutex.h"

class Sensor : public TelldusCore::Mutex {
public:
	Sensor(const std::wstring &protocol, const std::wstring &model, int id);
	~Sensor();

	std::wstring protocol() const;
	std::wstring model() const;
	int id() const;
	time_t timestamp() const;

	int dataTypes() const;

	void setValue(int type, const std::string &value, time_t timestamp);
	std::string value(int type) const;

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_SENSOR_H_
