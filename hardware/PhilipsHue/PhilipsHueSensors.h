#pragma once

namespace Json
{
	class Value;
};

class CPHSensorState
{
public:
	std::string m_lastupdated;	// All devices
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

class CPHSensorConfig
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
	std::string m_name;
	std::string m_type;
	std::string m_model_id;
	std::string m_manufacturer_name;
	std::string m_sw_version;
	std::string m_uniqueid;
	CPHSensorState m_state;
	CPHSensorConfig m_config;
	CPHSensor();
	CPHSensor(Json::Value sensor);
	~CPHSensor();
};
