#include "stdafx.h"
#include "../../json/value.h"
#include "DeconzSensor.h"

DeconzSensor::DeconzSensor(Json::Value sensor) :  DeconzNode(sensor), state(sensor["state"]), config(sensor["config"])
{
};

DeconzSensor::DeconzSensor() :  DeconzNode()
{
	
};

SensorState::SensorState(): buttonevent(0), presence(false), temperature(0), lightlevel(0), dark(false), daylight(false)
{	
}

SensorState::SensorState(Json::Value state)
{	
	lastupdated = (state["lastupdated"] != Json::Value::null) ? state["lastupdated"].asString() : "none";
	buttonevent = (state["buttonevent"] != Json::Value::null) ? state["buttonevent"].asInt() : 0;
	presence = (state["presence"] != Json::Value::null) ? state["presence"].asBool() : false;
	temperature = (state["temperature"] != Json::Value::null) ? state["temperature"].asInt() : 0;
	lightlevel = (state["lightlevel"] != Json::Value::null) ? state["lightlevel"].asInt() : 0;
	dark = (state["dark"] != Json::Value::null) ? state["dark"].asBool() : false;
	daylight = (state["daylight"] != Json::Value::null) ? state["daylight"].asBool() : false;
}

SensorState::~SensorState()
{	
}

SensorConfig::SensorConfig(): on(false), reachable(false), battery(0), tholddark(0),
tholdoffset(0), sunriseoffset(0), sunsetoffset(0), configured(false)
{	
}

SensorConfig::SensorConfig(Json::Value config)
{
	on = config["on"].asBool();
	reachable = (config["reachable"] != Json::Value::null) ? config["reachable"].asBool() : true;
	battery = (config["battery"] != Json::Value::null) ? config["battery"].asInt() : 100;
	tholddark = (config["tholddark"] != Json::Value::null) ? config["tholddark"].asInt() : 0;
	tholdoffset = (config["tholdoffset"] != Json::Value::null) ? config["tholdoffset"].asInt() : 0;
	configured = (config["configured"] != Json::Value::null) ? config["configured"].asBool() : true;
	sunriseoffset = (config["sunriseoffset"] != Json::Value::null) ? config["sunriseoffset"].asInt() : 0;
	sunsetoffset = (config["sunsetoffset"] != Json::Value::null) ? config["sunsetoffset"].asInt() : 0;
}

SensorConfig::~SensorConfig()
{	
}


