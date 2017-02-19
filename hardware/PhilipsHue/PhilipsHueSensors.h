#pragma once

#include "../DomoticzHardware.h"
#include "../MySensorsBase.h"

using namespace std;

namespace Json
{
	class Value;
};

class CPHSensorType
{
public:
	const string Daylight = "Daylight";
	const string ZGPSwitch = "ZGPSwitch";
	const string ZLLSwitch = "ZLLSwitch";
	const string ZLLPresence = "ZLLPresence";
	const string ZLLTemperature = "ZLLTemperature";
	const string ZLLLightLevel = "ZLLLightLevel";
};

class CPHSensorState : CPHSensorType
{
public:
	string m_lastupdated;	// All devices
	int m_buttonevent;		// Switch devices
	bool m_presence;		// ZLL presence
	int m_temperature;		// ZLL temperature
	int m_lightlevel;		// ZLL Light level
	bool m_dark;			// ZLL Light level
	bool m_daylight;		// ZLL Light level
	CPHSensorState();
	CPHSensorState(Json::Value state);
	~CPHSensorState();
};

class CPHSensorConfig : CPHSensorType
{
public:
	bool m_on; // All devices
	bool m_reachable; // ZLL devices
	int m_battery;	  // ZLL devices
	int m_tholddark;  // ZLL light level devices
	int m_tholdoffset; // ZLL light level devices
	int m_sunriseoffset; // Daylight 
	int m_sunsetoffset; // Daylight
	bool m_configured; // Daylight
	CPHSensorConfig();
	CPHSensorConfig(Json::Value config);
	~CPHSensorConfig();
};

class CPHSensor
{
public:
	string m_name;
	string m_type;
	string m_model_id;
	string m_manufacturer_name;
	string m_sw_version;
	string m_uniqueid;
	CPHSensorState m_state;
	CPHSensorConfig m_config;
	CPHSensor();
	CPHSensor(Json::Value sensor);
	~CPHSensor();
};

//class CPHSZllLightLevel : public CPHSensor
//{
//	CPHSensorState state;
//	CPHSensorConfig config;
//};