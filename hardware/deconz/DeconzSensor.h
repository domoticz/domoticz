#pragma once
#include "DeconzNode.h"

namespace Json {
	class Value;
}

class SensorState
{
public:
	std::string lastupdated;	// All devices
	int buttonevent;		// Switch devices
	bool presence;		// ZLL presence
	int temperature;		// ZLL temperature
	int lightlevel;		// ZLL Light level
	bool dark;			// ZLL Light level
	bool daylight;		// ZLL Light level
	SensorState();
	SensorState(Json::Value state);
	~SensorState();
};

class SensorConfig
{
public:
	bool on; // All devices
	bool reachable; // ZLL devices
	int battery;	  // ZLL devices
	int tholddark;  // ZLL light level devices
	int tholdoffset; // ZLL light level devices
	int sunriseoffset; // Daylight 
	int sunsetoffset; // Daylight
	bool configured; // Daylight
	SensorConfig();
	SensorConfig(Json::Value config);
	~SensorConfig();
};

class DeconzSensor : public DeconzNode
{
public:
	DeconzSensor(Json::Value sensor);
	DeconzSensor();
	SensorState state;
	SensorConfig config;
};




