#include "stdafx.h"
#include "PhilipsHueSensors.h"
#include "../../main/mainworker.h"
#include "../../json/json.h"

CPHSensorState::CPHSensorState(const Json::Value& state, const Json::Value& capabilities)
{
	m_lastupdated = (state["lastupdated"] != Json::Value::null) ? state["lastupdated"].asString() : "none";
	m_buttonevent = (state["buttonevent"] != Json::Value::null) ? state["buttonevent"].asInt() : 0;
	m_presence = (state["presence"] != Json::Value::null) ? state["presence"].asBool() : false;
	m_temperature = (state["temperature"] != Json::Value::null) ? state["temperature"].asInt() : 0;
	m_lightlevel = (state["lightlevel"] != Json::Value::null) ? state["lightlevel"].asInt() : 0;
	m_dark = (state["dark"] != Json::Value::null) ? state["dark"].asBool() : false;
	m_daylight = (state["daylight"] != Json::Value::null) ? state["daylight"].asBool() : false;

	m_button_nr = m_buttonevent;
	if (m_buttonevent > 0)
		ExtractButtonInfo(state, capabilities);
}

void CPHSensorState::ExtractButtonInfo(const Json::Value& state, const Json::Value& capabilities)
{
	Json::Value inputs = capabilities["inputs"];
	m_button_count = inputs.size();
	for (int buttonidx = 0; buttonidx < m_button_count; buttonidx++)
	{
		Json::Value events = inputs[buttonidx]["events"];
		unsigned int event_count = events.size();
		for (unsigned int eventidx = 0; eventidx < event_count; eventidx++)
		{
			std::string eventtypestring = events[eventidx]["eventtype"].asString();
			if (eventtypestring == EVENT_LONG_RELEASE)
				m_button_can_long_press = true;

			int eventid = events[eventidx]["buttonevent"].asInt();
			if (eventid == m_buttonevent)
			{
				m_button_nr = buttonidx;
				m_button_valid = true;

				if (event_count == 1)
					m_button_eventtype = SensorZLLSwitchEventSimple;
				else if (EVENT_INITIAL_PRESS == eventtypestring)
					m_button_eventtype = SensorZLLSwitchEventInitialPress;
				else if (EVENT_REPEAT == eventtypestring)
					m_button_eventtype = SensorZLLSwitchEventRepeat;
				else if (EVENT_SHORT_RELEASE == eventtypestring)
					m_button_eventtype = SensorZLLSwitchEventShortRelease;
				else if (EVENT_LONG_RELEASE == eventtypestring)
					m_button_eventtype = SensorZLLSwitchEventLongRelease;
				else
					m_button_valid = false;

			}
		}
	}
}

int32_t CPHSensorState::GetSelectorLevel(const CPHSensorState& previous_state)
{
	if (m_button_valid == false)
		return -1;

	if (SensorZLLSwitchEventInitialPress == m_button_eventtype)
		return -1;

	int32_t selectorLevel = m_button_nr * 10;
	if (m_button_can_long_press)
		selectorLevel *= 2;
	selectorLevel += 10;

	if (SensorZLLSwitchEventRepeat == m_button_eventtype || SensorZLLSwitchEventLongRelease == m_button_eventtype)
	{
		if (previous_state.m_button_nr == m_button_nr &&
			previous_state.m_button_eventtype == SensorZLLSwitchEventRepeat)
		{
			return -1; //It's a repeat of a long press.
		}
		selectorLevel += 10;
	}

	return selectorLevel;
}

std::map<std::string, std::string> CPHSensorState::GetButtonOptions()
{
	std::map<std::string, std::string> options;
	options["SelectorStyle"] = "1";
	options["LevelOffHidden"] = "true";
	options["LevelNames"] = "Off";
	for (int buttonidx = 0; buttonidx < m_button_count; buttonidx++)
	{
		if (m_button_can_long_press)
		{
			options["LevelNames"] += "|Button " + std::to_string(buttonidx + 1) + " short";
			options["LevelNames"] += "|Button " + std::to_string(buttonidx + 1) + " long";
		}
		else
		{
			options["LevelNames"] += "|Button " + std::to_string(buttonidx + 1);
		}
	}
	return options;
}

bool CPHSensorState::operator==(const CPHSensorState& other)
{
	return (m_lastupdated == other.m_lastupdated &&
		m_buttonevent == other.m_buttonevent &&
		m_presence == other.m_presence &&
		m_temperature == other.m_temperature &&
		m_lightlevel == other.m_lightlevel &&
		m_dark == other.m_dark &&
		m_daylight == other.m_daylight);
}

CPHSensorConfig::CPHSensorConfig(const Json::Value& config)
{
	m_on = config["on"].asBool();
	m_reachable =		(config["reachable"] != Json::Value::null)		? config["reachable"].asBool()		: true;
	m_battery =			(config["battery"] != Json::Value::null)		? config["battery"].asInt()			: 255;
	m_tholddark =		(config["tholddark"] != Json::Value::null)		? config["tholddark"].asInt()		: 0;
	m_tholdoffset =		(config["tholdoffset"] != Json::Value::null)	? config["tholdoffset"].asInt()		: 0;
	m_configured =		(config["configured"] != Json::Value::null)		? config["configured"].asBool()		: true;
	m_sunriseoffset =	(config["sunriseoffset"] != Json::Value::null)	? config["sunriseoffset"].asInt()	: 0;
	m_sunsetoffset =	(config["sunsetoffset"] != Json::Value::null)	? config["sunsetoffset"].asInt()	: 0;
}


CPHSensor::CPHSensor(const Json::Value& sensor) : m_state(sensor["state"], sensor["capabilities"]), m_config(sensor["config"])
{
	m_name = sensor["name"].asString();
	m_type = sensor["type"].asString();
	m_model_id = sensor["modelid"].asString();
	m_manufacturer_name = sensor["manufacturername"].asString();	
	m_sw_version = sensor["swversion"].asString();
	m_uniqueid = (sensor["uniqueid"] != Json::Value::null) ? sensor["uniqueid"].asString() : "";
}
