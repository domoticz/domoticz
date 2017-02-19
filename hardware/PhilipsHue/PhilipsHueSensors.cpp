#include "stdafx.h"
#include "PhilipsHueSensors.h"
#include "../../main/mainworker.h"
#include "../../json/json.h"

CPHSensorState::CPHSensorState(): m_buttonevent(0), m_presence(false), m_temperature(0), m_lightlevel(0), m_dark(false), m_daylight(false) { }

CPHSensorState::CPHSensorState(Json::Value state)
{
	m_lastupdated =	(state["lastupdated"] != Json::Value::null) ? state["lastupdated"].asString() : "none";
	m_buttonevent =	(state["buttonevent"] != Json::Value::null) ? state["buttonevent"].asInt() : 0;
	m_presence  =	(state["presence"] != Json::Value::null) ? state["presence"].asBool() : false;
	m_temperature =	(state["temperature"] != Json::Value::null) ? state["temperature"].asInt() : 0;
	m_lightlevel =	(state["lightlevel"] != Json::Value::null) ? state["lightlevel"].asInt() : 0;
	m_dark =		(state["dark"] != Json::Value::null) ? state["dark"].asBool() : false;
	m_daylight =	(state["daylight"] != Json::Value::null) ? state["daylight"].asBool() : false;
}

CPHSensorState::~CPHSensorState() { }

CPHSensorConfig::CPHSensorConfig(): m_on(false), m_reachable(false), m_battery(0), m_tholddark(0),
									m_tholdoffset(0), m_sunriseoffset(0), m_sunsetoffset(0), m_configured(false) { }

CPHSensorConfig::CPHSensorConfig(Json::Value config)
{
	m_on = config["on"].asBool();
	m_reachable =		(config["reachable"] != Json::Value::null)		? config["reachable"].asBool()		: true;
	m_battery =			(config["battery"] != Json::Value::null)		? config["battery"].asInt()			: 100;
	m_tholddark =		(config["tholddark"] != Json::Value::null)		? config["tholddark"].asInt()		: 0;
	m_tholdoffset =		(config["tholdoffset"] != Json::Value::null)	? config["tholdoffset"].asInt()		: 0;
	m_configured =		(config["configured"] != Json::Value::null)		? config["configured"].asBool()		: true;
	m_sunriseoffset =	(config["sunriseoffset"] != Json::Value::null)	? config["sunriseoffset"].asInt()	: 0;
	m_sunsetoffset =	(config["sunsetoffset"] != Json::Value::null)	? config["sunsetoffset"].asInt()	: 0;
}

CPHSensorConfig::~CPHSensorConfig() { }

CPHSensor::CPHSensor() { }

CPHSensor::CPHSensor(Json::Value sensor) : m_state(sensor["state"]), m_config(sensor["config"])
{
	m_name = sensor["name"].asString();
	m_type = sensor["type"].asString();
	m_model_id = sensor["modelid"].asString();
	m_manufacturer_name = sensor["manufacturername"].asString();	
	m_sw_version = sensor["swversion"].asString();
	m_uniqueid = (sensor["uniqueid"] != Json::Value::null) ? sensor["uniqueid"].asString() : "";
}

CPHSensor::~CPHSensor() { }
