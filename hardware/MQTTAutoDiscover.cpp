#include "stdafx.h"
#include "MQTT.h"
#include "MQTTAutoDiscover.h"
#include "../main/json_helper.h"
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../notifications/NotificationHelper.h"
#include <iostream>

std::vector<std::string> allowed_components = {
		"binary_sensor",
		"button",
		"climate",
		"cover",
		//"device_automation",
		"light",
		"lock",
		"number",
		"select",
		"sensor",
		"switch",
		"fan"
};

#define CLIMATE_MODE_UNIT 1
#define CLIMATE_PRESET_UNIT 2
#define FAN_PRESET_UNIT 3

MQTTAutoDiscover::MQTTAutoDiscover(const int ID, const std::string& Name, const std::string& IPAddress, const unsigned short usIPPort, const std::string& Username, const std::string& Password,
	const std::string& CAfilenameExtra, const int TLS_Version)
	: MQTT(ID, IPAddress, usIPPort, Username, Password, CAfilenameExtra, TLS_Version, (int)MQTTAutoDiscover::PT_none, std::string("Domoticz-MQTT-AutoDiscover") + GenerateUUID() + std::to_string(ID), true)
{
	std::vector<std::string> strarray;
	StringSplit(CAfilenameExtra, ";", strarray);
	if (!strarray.empty())
	{
		m_CAFilename = strarray[0];

		if (strarray.size() > 3)
		{
			m_TopicDiscoveryPrefix = strarray[3];
		}
	}
	if (m_TopicDiscoveryPrefix.empty())
	{
		Log(LOG_ERROR, "Auto Discovery Topic empty!");
	}
}

void MQTTAutoDiscover::on_message(const struct mosquitto_message* message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	try
	{
		Debug(DEBUG_HARDWARE, "topic: %s, message: %s", topic.c_str(), qMessage.c_str());

		if (qMessage.empty())
			return;

		if (
			(topic.substr(0, topic.find('/')) == m_TopicDiscoveryPrefix)
			&& (topic.find("/config") != std::string::npos)
			)
		{
			on_auto_discovery_message(message);
			return;
		}

		std::string DiscoveryWildcard = m_TopicDiscoveryPrefix + "/#";

		for (auto& itt : m_subscribed_topics)
		{
			bool result = false;
			if (
				(itt.first != DiscoveryWildcard)
				&& (mosquitto_topic_matches_sub(itt.first.c_str(), topic.c_str(), &result) == MOSQ_ERR_SUCCESS)
				)
			{
				if (result == true)
				{
					handle_auto_discovery_sensor_message(message, itt.first);
					return;
				}
			}
		}

		return;
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Exception (on_message): %s! (topic: %s, message: %s)", e.what(), topic.c_str(), qMessage.c_str());
		return;
	}
}

void MQTTAutoDiscover::on_connect(int rc)
{
	/* rc=
	** 0 - success
	** 1 - connection refused(unacceptable protocol version)
	** 2 - connection refused(identifier rejected)
	** 3 - connection refused(broker unavailable)
	*/

	if (rc == 0)
	{
		if (m_IsConnected)
		{
			Log(LOG_STATUS, "re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		}
		else
		{
			Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
		}

		if (!m_TopicDiscoveryPrefix.empty())
		{
			SubscribeTopic((m_TopicDiscoveryPrefix + std::string("/#")).c_str());
			//Send online status
			SendMessage(m_TopicDiscoveryPrefix + std::string("/status"), "online");
		}
	}
	else
	{
		Log(LOG_ERROR, "Connection failed!, restarting (rc=%d)", rc);
		m_bDoReconnect = true;
	}
}

void MQTTAutoDiscover::on_going_down()
{
	SendMessageEx(m_TopicDiscoveryPrefix + std::string("/status"), "offline", 0, m_bRetain);
}

void MQTTAutoDiscover::on_disconnect(int rc)
{
	m_discovered_devices.clear();
	m_discovered_sensors.clear();
	MQTT::on_disconnect(rc);
}

void MQTTAutoDiscover::CleanValueTemplate(std::string& szValueTemplate)
{
	if (szValueTemplate.empty())
		return;
	stdreplace(szValueTemplate, "{", "");
	stdreplace(szValueTemplate, "}", "");
	stdreplace(szValueTemplate, "'", "");
	stdreplace(szValueTemplate, "\"", "");

	szValueTemplate = szValueTemplate.substr(0, szValueTemplate.find("|"));
	szValueTemplate = szValueTemplate.substr(0, szValueTemplate.find(".split("));

	stdstring_trim(szValueTemplate);

	//Strip if/endif mombojumbo (until we are going to support it)
	if (szValueTemplate.find("endif") != std::string::npos)
	{
		size_t pos = szValueTemplate.find("value_json.");
		if (pos == std::string::npos)
		{
			pos = szValueTemplate.find("value_json[");
		}
		if (pos != std::string::npos)
		{
			szValueTemplate = szValueTemplate.substr(pos);
			pos = szValueTemplate.find(' ');
			if (pos == std::string::npos)
				pos = szValueTemplate.find("==");
			if (pos != std::string::npos)
			{
				szValueTemplate = szValueTemplate.substr(0, pos);
				stdstring_trim(szValueTemplate);
			}
		}
	}
	else if (szValueTemplate.find("(") == 0)
	{
		//strip calculations (until we are going to support it)
		szValueTemplate = szValueTemplate.substr(1);
		szValueTemplate = szValueTemplate.substr(0, szValueTemplate.find("/"));
		szValueTemplate = szValueTemplate.substr(0, szValueTemplate.find("+"));
		szValueTemplate = szValueTemplate.substr(0, szValueTemplate.find("-"));
		stdstring_trim(szValueTemplate);
	}
}

std::string MQTTAutoDiscover::GetValueTemplateKey(const std::string& szValueTemplate)
{
	std::string szKey = szValueTemplate;
	std::vector<std::string> strarray;

	if (szValueTemplate.find("value_json.") != std::string::npos)
	{
		StringSplit(szValueTemplate, ".", strarray);
		if (strarray.size() == 2)
		{
			szKey = strarray[1];
		}
	}
	else
	{
		StringSplit(szValueTemplate, ":", strarray);
		if (strarray.size() == 2)
		{
			szKey = strarray[0];
			stdreplace(szKey, "\"", "");
		}
	}
	stdstring_trim(szKey);
	return szKey;
}

//returns empty if value is not found
std::string MQTTAutoDiscover::GetValueFromTemplate(Json::Value root, std::string szValueTemplate)
{
	std::string szKey;
	std::vector<std::string> strarray;

	try
	{
		size_t pos;
		std::map<std::string, std::string> value_options_;
		pos = szValueTemplate.find("[value_json");
		if (pos != std::string::npos)
		{
			std::string szOptions = szValueTemplate.substr(0, pos);
			szValueTemplate = szValueTemplate.substr(pos + 1);
			stdreplace(szValueTemplate, "]", "");
			StringSplit(szOptions, ",", strarray);
			for (const auto itt : strarray)
			{
				std::vector<std::string> strarray2;
				StringSplit(itt, ":", strarray2);
				if (strarray2.size() == 2)
				{
					stdstring_trim(strarray2[0]);
					stdstring_trim(strarray2[1]);
					value_options_[strarray2[0]] = strarray2[1];
				}
			}
		}
		pos = szValueTemplate.find("value_json.");
		if (pos != std::string::npos)
		{
			std::string tstring = szValueTemplate.substr(pos + std::string("value_json.").size());
			StringSplit(tstring, ".", strarray);
			for (const auto itt : strarray)
			{
				szKey = itt;

				if (szKey.find('[') == std::string::npos)
				{
					if (root[szKey].empty())
						return ""; //key not found!
					root = root[szKey];
				}
				else
				{
					//we have an array, so we need to get the index
					if (szKey.find(']') == std::string::npos)
						return ""; //no index?

					std::string szIndex = szKey.substr(szKey.find('[') + 1);
					szIndex = szIndex.substr(0, szIndex.find(']'));
					if (szIndex.size() == 0)
						return ""; //no index?

					szKey= szKey.substr(0, szKey.find('['));
					int iIndex = std::stoi(szIndex);
					if (root[szKey].empty())
						return ""; //key not found!
					if (static_cast<int>(root[szKey].size()) <= iIndex)
						return ""; //index out of range!
					root = root[szKey][iIndex];
				}
			}
			if (root.isObject())
				return "";
			std::string retVal;
			if (root.isDouble())
			{
				//until we have c++20 where we can use std::format
				retVal = std_format("%g", root.asDouble());
			}
			else
				retVal = root.asString();
			if (value_options_.find(retVal) != value_options_.end())
			{
				retVal = value_options_[retVal];
			}
			return retVal;
		}
		else if (szValueTemplate.find("value_json[") != std::string::npos)
		{
			//could be one or multiple object and have a possible key at the end
			//value_json["key1"]["key2"]{.value}
			std::string tstring = szValueTemplate.substr(std::string("value_json").size());
			std::string suffix;
			StringSplit(tstring, ".", strarray);
			if (strarray.size() == 2)
			{
				tstring = strarray[0];
				suffix = strarray[1];
			}
			StringSplit(tstring, "]", strarray);
			for (const auto itt : strarray)
			{
				szKey = itt;
				stdreplace(szKey, "[", "");
				stdreplace(szKey, "]", "");
				if (
					(is_number(szKey)
						&& (root.isArray()))
					)
				{
					int iNumber = std::stoi(szKey);
					size_t object_size = root.size();
					if (iNumber < (int)object_size)
					{
						root = root[iNumber];
					}
					else
					{
						Log(LOG_ERROR, "Exception (GetValueFromTemplate): Array out of bound! (Template: %s)", szValueTemplate.c_str());
					}
				}
				else
				{
					if (root[szKey].empty())
						return ""; //key not found!
					root = root[szKey];
				}
			}
			if (suffix.empty())
				return root.asString();
			else
			{
				if (root[suffix].empty())
					return ""; //not found
				return root[suffix].asString();
			}
			return "";
		}
		else
		{
			StringSplit(szValueTemplate, ":", strarray);
			if (strarray.size() == 2)
			{
				szKey = strarray[0];
				stdreplace(szKey, "\"", "");
			}
			else
				szKey = szValueTemplate;
		}
		stdstring_trim(szKey);
		if (!root[szKey].empty())
			return root[szKey].asString();
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Exception (GetValueFromTemplate): %s! (Template: %s)", e.what(), szValueTemplate.c_str());
	}
	return "";
}

std::string MQTTAutoDiscover::GetValueFromTemplate(const std::string& szValue, std::string szValueTemplate)
{
	try
	{
		std::map<std::string, std::string> value_keys;

		stdreplace(szValueTemplate, " ", "");
		stdreplace(szValueTemplate, "%", "");

		//we only support if/else/endif
		if (szValueTemplate.find("ifvalue==") != 0)
			return "";
		szValueTemplate = szValueTemplate.substr(strlen("ifvalue=="));
		size_t pos;

		pos = szValueTemplate.find("elseif");
		if (pos != std::string::npos)
		{
			Log(LOG_ERROR, "This template is not (yet) support, please report to us (Template: %s)", szValueTemplate.c_str());
			return ""; //not supported
		}

		pos = szValueTemplate.find("else");
		if (pos == std::string::npos)
			return "";

		std::string szKey = szValueTemplate.substr(0, pos);

		szValueTemplate = szValueTemplate.substr(pos + strlen("else"));
		pos = szValueTemplate.find("endif");
		if (pos == std::string::npos)
			return "";

		szValueTemplate = szValueTemplate.substr(0, pos);

		std::string szValElse = szValueTemplate;

		if (szKey.find(szValue) == 0)
		{
			return szKey.substr(szValue.size());
		}

		return szValElse;
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Exception (GetValueFromTemplate): %s! (Template: %s)", e.what(), szValueTemplate.c_str());
	}
	return "";
}

//Returns true if value is set in JSon object
bool MQTTAutoDiscover::SetValueWithTemplate(Json::Value& root, std::string szValueTemplate, std::string szValue)
{
	try
	{
		size_t pos;
		std::vector<std::string> strarray;

		std::map<std::string, std::string> value_options_;
		pos = szValueTemplate.find("[value_json");
		if (pos != std::string::npos)
		{
			std::string szOptions = szValueTemplate.substr(0, pos);
			szValueTemplate = szValueTemplate.substr(pos + 1);
			stdreplace(szValueTemplate, "]", "");
			StringSplit(szOptions, ",", strarray);
			for (const auto itt : strarray)
			{
				std::vector<std::string> strarray2;
				StringSplit(itt, ":", strarray2);
				if (strarray2.size() == 2)
				{
					stdstring_trim(strarray2[0]);
					stdstring_trim(strarray2[1]);
					value_options_[strarray2[1]] = strarray2[0];
				}
			}
		}

		pos = szValueTemplate.find("value_json.");
		if (pos != std::string::npos)
		{
			std::string tstring = szValueTemplate.substr(pos + std::string("value_json.").size());
			StringSplit(tstring, ".", strarray);
			if (strarray.size() != 1)
				return false; //only support 1 depth for now
			std::string szKey = strarray[0];
			if (value_options_.find(szValue) != value_options_.end())
			{
				szValue = value_options_[szValue];
			}
			if (is_number(szValue))
				root[szKey] = atoi(szValue.c_str());
			else
				root[szKey] = szValue;
			return true;
		}
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Exception (SetValueWithTemplate): %s! (Template: %s)", e.what(), szValueTemplate.c_str());
	}
	return false;
}

/*
Some nodes seem to announce the mode/temperature command topics wrong
They include the value template name field

For example:

  "mode_command_topic": "zigbee2mqtt/My-ThermControl1/set/system_mode",
  "mode_state_template": "{{ value_json.system_mode }}",

or:

  "temperature_command_topic": "zigbee2mqtt/My-ThermControl1/set/current_heating_setpoint",
  "temperature_command_template": "{{ value_json.current_heating_setpoint }}",

This function checks this the command_topic ends with /{template name field}
and if found removes this from the command topic

The result will be:
  "mode_command_topic": "zigbee2mqtt/My-ThermControl1/set",
  "temperature_command_topic": "zigbee2mqtt/My-ThermControl1/set",
*/
void MQTTAutoDiscover::FixCommandTopicStateTemplate(std::string& command_topic, std::string& state_template)
{
	size_t pos = state_template.find("value_json.");
	if (pos == std::string::npos)
		return; //no fixing needed
	std::string value_json = state_template.substr(pos + std::string("value_json.").size());
	stdreplace(value_json, "]", "");

	std::string svalue = "/" + value_json;

	pos = command_topic.rfind(svalue);
	if (pos != (command_topic.size() - svalue.size()))
		return; //no fixing needed

	command_topic = command_topic.substr(0, pos);
}

void MQTTAutoDiscover::on_auto_discovery_message(const struct mosquitto_message* message)
{
	std::string topic = message->topic;
	std::string org_topic(topic);
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	Json::Value root;
	std::string node_id;
	std::string object_id;
	std::string action;
	std::string component;
	bool ret;

	if (qMessage.empty())
		return;

	if (topic == m_TopicDiscoveryPrefix)
		return; //direct payload in discovery topic not allowed

	topic = topic.substr(m_TopicDiscoveryPrefix.size() + 1);

	//<discovery_prefix>/<component>/[<node_id>/]<object_id>/config

	std::vector<std::string> strarray;
	StringSplit(topic, "/", strarray);
	if (strarray.size() < 3)
	{
		if (topic == "status")
			return;
		//not for us
		goto disovery_invaliddata;
	}

	component = strarray[0];

	if (std::find(allowed_components.begin(), allowed_components.end(), component) == allowed_components.end())
	{
		//not for us
		return;
	}

	//topic format: <discovery_prefix>/<component>/[<node_id>/]<object_id>/<action>
	if (!((strarray.size() == 3) || (strarray.size() == 4) || (strarray.size() == 5) || (strarray.size() == 6)))
		goto disovery_invaliddata;

	ret = ParseJSon(qMessage, root);
	if ((!ret) || (!root.isObject()))
	{
		if (topic == "status")
			return;
		goto disovery_invaliddata;
	}

	if (strarray.size() == 3)
	{
		object_id = strarray[1];
		action = strarray[2];
	}
	else if (strarray.size() == 4)
	{
		node_id = strarray[1];
		object_id = strarray[2];
		action = strarray[3];
	}
	else if (strarray.size() == 5)
	{
		node_id = strarray[1] + "_" + strarray[2];
		object_id = strarray[3];
		action = strarray[4];
	}
	else if (strarray.size() == 6)
	{
		node_id = strarray[1] + "_" + strarray[2];
		object_id = strarray[3];
		action = strarray[5];
	}

	//skip some non-needed types, they will be transmitted in the state topic anyways (for 90%) or not necessary
	if (
		(object_id == "update_available")
		|| (object_id == "power_on_behavior")
		|| (object_id == "power_outage_memory")
		|| (object_id == "update_state")
		|| (object_id == "over-load_status")
		|| (object_id == "linkquality")
		|| (object_id == "last_seen")
		|| (object_id == "sensitivity")
		|| (object_id == "color_temp_startup")
		|| (object_id == "requested_brightness_level")
		|| (object_id == "requested_brightness_percent")
		|| (object_id == "device_automation")
		|| (object_id == "over-load_status")
		|| (object_id == "hardware_status")
		|| (object_id.find("_address") != std::string::npos)
		|| (object_id.find("_ssid") != std::string::npos)
		|| (object_id.find("_signal_sensor") != std::string::npos)
		)
	{
		return;
	}

	if (component == "sensor")
	{
		if (
			(std_ends_with(object_id, "_min"))
			|| (std_ends_with(object_id, "_max"))
			|| (object_id.find("min_") == 0)
			|| (object_id.find("max_") == 0)
			|| (object_id.find("_sensitivity") != std::string::npos)
			|| (object_id.find("_alarm") != std::string::npos)
			)
		{
			return;
		}
	}

	if (action != "config")
	{
		if (action == "state")
		{
			//0/1 (or online/offline)
			return;
		}
		Log(LOG_ERROR, "MQTT_Discovery: Something other then 'config' received on discovery topic! (%s/%s)!", topic.c_str(), qMessage.c_str());

		if (action == "switch")
		{
			// seen with ESP-EASY
			// ON/OFF
			return;
		}

		goto disovery_invaliddata; // could be 'state' with a value 0/1 or switch, but it's not following protocol!
	}

	try
	{
		std::string sensor_unique_id;
		if (!root["unique_id"].empty())
			sensor_unique_id = root["unique_id"].asString();
		else if (!root["uniq_id"].empty())
			sensor_unique_id = root["uniq_id"].asString();
		else
		{
			//It's optional, but good to have one
			sensor_unique_id = GenerateUUID();
		}

		if (root["name"].empty())
		{
			root["name"] = sensor_unique_id;
		}

		std::string device_identifiers;

		if (!root["device"].empty())
		{
			if (!root["device"]["identifiers"].empty())
			{
				if (root["device"]["identifiers"].isArray())
					device_identifiers = root["device"]["identifiers"][0].asString();
				else
					device_identifiers = root["device"]["identifiers"].asString();
			}
		}
		else if (!root["dev"].empty())
		{
			if (!root["dev"]["ids"].empty())
			{
				if (root["dev"]["ids"].isArray())
					device_identifiers = root["dev"]["ids"][0].asString();
				else
					device_identifiers = root["dev"]["ids"].asString();
			}
		}
		else
		{
			//It's optional, but good to supply one
		}

		if (device_identifiers.empty())
		{
			// we need a deviceID, let's use the sensors unique id
			device_identifiers = sensor_unique_id;
		}

		//device
		if (m_discovered_devices.find(device_identifiers) == m_discovered_devices.end())
		{
			//new device
			_tMQTTADevice tmpDevice;
			tmpDevice.identifiers = device_identifiers;
			m_discovered_devices[device_identifiers] = tmpDevice;
		}
		_tMQTTADevice* pDevice = &m_discovered_devices[device_identifiers];

		pDevice->identifiers = device_identifiers;
		if (!root["device"]["name"].empty())
			pDevice->name = root["device"]["name"].asString();
		else if (!root["dev"]["name"].empty())
			pDevice->name = root["dev"]["name"].asString();
		if (pDevice->name.empty())
			pDevice->name = pDevice->identifiers;

		if (!root["device"]["sw_version"].empty())
			pDevice->sw_version = root["device"]["sw_version"].asString();
		else if (!root["dev"]["sw"].empty())
			pDevice->sw_version = root["dev"]["sw"].asString();
		if (!root["device"]["model"].empty())
			pDevice->model = root["device"]["model"].asString();
		else if (!root["dev"]["mdl"].empty())
			pDevice->model = root["dev"]["mdl"].asString();
		if (!root["device"]["manufacturer"].empty())
			pDevice->manufacturer = root["device"]["manufacturer"].asString();
		else if (!root["dev"]["mf"].empty())
			pDevice->manufacturer = root["dev"]["mf"].asString();

		for (const auto ittMember : root["device"].getMemberNames())
		{
			if (!root["device"][ittMember].empty())
			{
				if (root["device"][ittMember].isString())
					pDevice->keys[ittMember] = root["device"][ittMember].asString();
			}
		}
		for (const auto ittMember : root["dev"].getMemberNames())
		{
			if (!root["dev"][ittMember].empty())
			{
				if (root["dev"][ittMember].isString())
					pDevice->keys[ittMember] = root["dev"][ittMember].asString();
			}
		}


		//sensor
		/* skip this test to allow for device updates while running
		if (m_discovered_sensors.find(sensor_unique_id) != m_discovered_sensors.end())
		{
			//Already received
			return;
		}
		*/

		//Check if we have a base_topic and if so, adjust all payloads
		if (!root["~"].empty())
		{
			std::string base_topic = root["~"].asString();

			for (auto ittPayload : root)
			{
				for (const auto ittMember : root.getMemberNames())
				{
					std::string szKey = ittMember;
					if (szKey == "~")
						continue;
					if (!root[ittMember].empty())
					{
						if (root[ittMember].isString())
						{
							std::string szValue = root[ittMember].asString();
							if (szValue.empty())
								continue;
							if (
								(szValue.find('~') == 0)
								|| (szValue.find('~') == szValue.size() - 1)
								)
							{
								stdreplace(szValue, "~", base_topic);
								root[ittMember] = szValue;
							}
						}
					}
				}
			}
		}

		_tMQTTASensor tmpSensor;
		m_discovered_sensors[sensor_unique_id] = tmpSensor;
		_tMQTTASensor* pSensor = &m_discovered_sensors[sensor_unique_id];
		pSensor->unique_id = sensor_unique_id;
		pSensor->object_id = object_id;
		pSensor->config = qMessage;
		pSensor->component_type = component;
		pSensor->device_identifiers = device_identifiers;
		pSensor->name = root["name"].asString();

		if (!root["enabled_by_default"].empty())
			pSensor->bEnabled_by_default = root["enabled_by_default"].asBool();

		if (!root["availability_topic"].empty())
			pSensor->availability_topic = root["availability_topic"].asString();
		else if (!root["avty_t"].empty())
			pSensor->availability_topic = root["avty_t"].asString();
		else
		{
			Json::Value rootAvailability;
			if (!root["availability"].empty())
				rootAvailability = root["availability"];
			else if (!root["avty"].empty())
				rootAvailability = root["avty"];

			if (rootAvailability.isArray())
			{
				for (auto& ittAvail : rootAvailability)
				{
					if (!ittAvail["topic"].empty())
						pSensor->availability_topic = ittAvail["topic"].asString();
					if (!ittAvail["payload_available"].empty())
						pSensor->payload_available = ittAvail["payload_available"].asString();
					if (!ittAvail["payload_not_available"].empty())
						pSensor->payload_not_available = ittAvail["payload_not_available"].asString();
				}
			}
			else
			{
				if (!rootAvailability["topic"].empty())
					pSensor->availability_topic = rootAvailability["topic"].asString();
				if (!rootAvailability["payload_available"].empty())
					pSensor->payload_available = rootAvailability["payload_available"].asString();
				if (!rootAvailability["payload_not_available"].empty())
					pSensor->payload_not_available = rootAvailability["payload_not_available"].asString();
			}
		}
		if (!root["state_topic"].empty())
			pSensor->state_topic = root["state_topic"].asString();
		else if (!root["stat_t"].empty())
			pSensor->state_topic = root["stat_t"].asString();
		else if (!root["json_attributes_topic"].empty())
			pSensor->state_topic = root["json_attributes_topic"].asString();
		else if (!root["topic"].empty())
			pSensor->state_topic = root["topic"].asString();
		if (!root["state_on"].empty())
			pSensor->state_on = root["state_on"].asString();
		else if (!root["stat_on"].empty())
			pSensor->state_on = root["stat_on"].asString();
		if (!root["state_off"].empty())
			pSensor->state_off = root["state_off"].asString();
		else if (!root["stat_off"].empty())
			pSensor->state_off = root["stat_off"].asString();

		if (!root["command_topic"].empty())
			pSensor->command_topic = root["command_topic"].asString();
		else if (!root["cmd_t"].empty())
			pSensor->command_topic = root["cmd_t"].asString();

		if (!root["position_topic"].empty())
			pSensor->position_topic = root["position_topic"].asString();
		if (!root["pos_t"].empty())
			pSensor->position_topic = root["pos_t"].asString();
		if (!root["position_template"].empty())
			pSensor->position_template = root["position_template"].asString();
		if (!root["pos_tpl"].empty())
			pSensor->position_template = root["pos_tpl"].asString();
		CleanValueTemplate(pSensor->position_template);

		if (!root["set_position_topic"].empty())
			pSensor->set_position_topic = root["set_position_topic"].asString();
		else if (!root["set_pos_t"].empty())
			pSensor->set_position_topic = root["set_pos_t"].asString();
		if (!root["set_position_template"].empty())
			pSensor->set_position_template = root["set_position_template"].asString();
		if (!root["set_pos_tpl"].empty())
			pSensor->set_position_template = root["set_pos_tpl"].asString();
		CleanValueTemplate(pSensor->set_position_template);

		if (!root["brightness_command_topic"].empty())
			pSensor->brightness_command_topic = root["brightness_command_topic"].asString();
		else if (!root["bri_cmd_t"].empty())
			pSensor->brightness_command_topic = root["bri_cmd_t"].asString();
		if (!root["brightness_state_topic"].empty())
			pSensor->brightness_state_topic = root["brightness_state_topic"].asString();
		else if (!root["bri_stat_t"].empty())
			pSensor->brightness_state_topic = root["bri_stat_t"].asString();
		if (!root["brightness_scale"].empty())
		{
			pSensor->brightness_scale = root["brightness_scale"].asFloat();
			pSensor->bHave_brightness_scale = true;
		}
		else if (!root["bri_scl"].empty())
		{
			pSensor->brightness_scale = root["bri_scl"].asFloat();
			pSensor->bHave_brightness_scale = true;
		}
		if (!root["brightness_value_template"].empty())
			pSensor->brightness_value_template = root["brightness_value_template"].asString();
		else if (!root["bri_val_tpl"].empty())
			pSensor->brightness_value_template = root["bri_val_tpl"].asString();
		CleanValueTemplate(pSensor->brightness_value_template);

		if (!root["unit_of_measurement"].empty())
			pSensor->unit_of_measurement = root["unit_of_measurement"].asString();
		else if (!root["unit_of_meas"].empty())
			pSensor->unit_of_measurement = root["unit_of_meas"].asString();
		stdstring_trim(pSensor->unit_of_measurement);

		if (!root["value_template"].empty())
			pSensor->value_template = root["value_template"].asString();
		else if (!root["val_tpl"].empty())
			pSensor->value_template = root["val_tpl"].asString();
		CleanValueTemplate(pSensor->value_template);

		if (!root["state_value_template"].empty())
			pSensor->state_value_template = root["state_value_template"].asString();
		else if (!root["stat_val_tpl"].empty())
			pSensor->state_value_template = root["stat_val_tpl"].asString();
		CleanValueTemplate(pSensor->state_value_template);

		if (!root["icon"].empty())
			pSensor->icon = root["icon"].asString();
		else if (!root["ic"].empty())
			pSensor->icon = root["ic"].asString();

		if (!root["state_class"].empty())
			pSensor->state_class = root["state_class"].asString();
		else if (!root["stat_cla"].empty())
			pSensor->state_class = root["stat_cla"].asString();

		if (!root["device_class"].empty())
			pSensor->device_class = root["device_class"].asString();
		else if (!root["dev_cla"].empty())
			pSensor->device_class = root["dev_cla"].asString();

		if (!root["payload_on"].empty())
			pSensor->payload_on = root["payload_on"].asString();
		else if (!root["pl_on"].empty())
			pSensor->payload_on = root["pl_on"].asString();
		if (!root["payload"].empty())
			pSensor->payload_on = root["payload"].asString();
		if (!root["pl"].empty())
			pSensor->payload_on = root["pl"].asString();
		if (!root["payload_off"].empty())
			pSensor->payload_off = root["payload_off"].asString();
		else if (!root["pl_off"].empty())
			pSensor->payload_off = root["pl_off"].asString();

		if (!root["payload_open"].empty())
			pSensor->payload_open = root["payload_open"].asString();
		else if (!root["pl_open"].empty())
			pSensor->payload_open = root["pl_open"].asString();
		if (!root["payload_close"].empty())
			pSensor->payload_close = root["payload_close"].asString();
		else if (!root["pl_cls"].empty())
			pSensor->payload_close = root["pl_cls"].asString();
		if (!root["payload_stop"].empty())
			pSensor->payload_stop = root["payload_stop"].asString();
		else if (!root["pl_stop"].empty())
			pSensor->payload_stop = root["pl_stop"].asString();
		if (!root["position_open"].empty())
			pSensor->position_open = root["position_open"].asInt();
		else if (!root["pos_open"].empty())
			pSensor->position_open = root["pos_open"].asInt();
		if (!root["position_closed"].empty())
			pSensor->position_closed = root["position_closed"].asInt();
		else if (!root["pos_clsd"].empty())
			pSensor->position_closed = root["pos_clsd"].asInt();

		if (!root["payload_available"].empty())
			pSensor->payload_available = root["payload_available"].asString();
		else if (!root["pl_avail"].empty())
			pSensor->payload_available = root["pl_avail"].asString();
		if (!root["payload_not_available"].empty())
			pSensor->payload_not_available = root["payload_not_available"].asString();
		else if (!root["pl_not_avail"].empty())
			pSensor->payload_not_available = root["pl_not_avail"].asString();

		if (!root["payload_lock"].empty())
			pSensor->payload_lock = root["payload_lock"].asString();
		else if (!root["pl_lock"].empty())
			pSensor->payload_lock = root["pl_lock"].asString();
		if (!root["payload_unlock"].empty())
			pSensor->payload_unlock = root["payload_unlock"].asString();
		else if (!root["pl_unlk"].empty())
			pSensor->payload_unlock = root["pl_unlk"].asString();
		if (!root["state_locked"].empty())
			pSensor->state_locked = root["state_locked"].asString();
		else if (!root["stat_locked"].empty())
			pSensor->state_locked = root["stat_locked"].asString();
		if (!root["state_unlocked"].empty())
			pSensor->state_unlocked = root["state_unlocked"].asString();
		else if (!root["stat_unlocked"].empty())
			pSensor->state_unlocked = root["stat_unlocked"].asString();
		if (!root["payload_press"].empty())
			pSensor->payload_press = root["payload_press"].asString();
		else if (!root["pl_prs"].empty())
			pSensor->payload_press = root["pl_prs"].asString();

		if (!root["percentage_command_topic"].empty())
			pSensor->percentage_command_topic = root["percentage_command_topic"].asString();
		if (!root["pct_cmd_t"].empty())
			pSensor->percentage_command_topic = root["pct_cmd_t"].asString();
		if (!root["percentage_command_template"].empty())
			pSensor->percentage_command_template = root["percentage_command_template"].asString();
		if (!root["pct_cmd_tpl"].empty())
			pSensor->percentage_command_template = root["pct_cmd_tpl"].asString();
		CleanValueTemplate(pSensor->percentage_command_template);

		if (!root["percentage_state_topic"].empty())
			pSensor->percentage_state_topic = root["percentage_state_topic"].asString();
		if (!root["pct_stat_t"].empty())
			pSensor->percentage_state_topic = root["pct_stat_t"].asString();
		if (!root["percentage_value_template"].empty())
			pSensor->percentage_value_template = root["percentage_value_template"].asString();
		if (!root["pct_val_tpl"].empty())
			pSensor->percentage_value_template = root["pct_val_tpl"].asString();
		CleanValueTemplate(pSensor->percentage_value_template);

		if (!root["brightness"].empty())
		{
			pSensor->bBrightness = (root["brightness"].asString() == "true");
			if (pSensor->bBrightness)
				pSensor->bHave_brightness_scale = true;
		}

		if (!root["rgb_value_template"].empty())
			pSensor->rgb_value_template = root["rgb_value_template"].asString();
		if (!root["rgb_val_tpl"].empty())
			pSensor->rgb_value_template = root["rgb_val_tpl"].asString();
		CleanValueTemplate(pSensor->rgb_value_template);

		if (!root["rgb_command_template"].empty())
			pSensor->rgb_command_template = root["rgb_command_template"].asString();
		if (!root["rgb_cmd_tpl"].empty())
			pSensor->rgb_command_template = root["rgb_cmd_tpl"].asString();
		CleanValueTemplate(pSensor->rgb_command_template);

		if (!root["rgb_command_topic"].empty())
			pSensor->rgb_command_topic = root["rgb_command_topic"].asString();
		if (!root["rgb_cmd_t"].empty())
			pSensor->rgb_command_topic = root["rgb_cmd_t"].asString();
		if (!root["rgb_state_topic"].empty())
			pSensor->rgb_state_topic = root["rgb_state_topic"].asString();
		if (!root["rgb_stat_t"].empty())
			pSensor->rgb_state_topic = root["rgb_stat_t"].asString();

		if (!root["color_mode"].empty()) // documentation is a bit unclear, color_mode = true, hs, rgb
			pSensor->bColor_mode = (root["color_mode"].asString() != "false");
		if (!root["supported_color_modes"].empty())
		{
			// onoff, brightness, color_temp, hs, xy, rgb, rgbw, rgbww.
			for (const auto& itt : root["supported_color_modes"])
			{
				pSensor->supported_color_modes[itt.asString()] = 1;
			}
			if (pSensor->supported_color_modes.find("brightness") != pSensor->supported_color_modes.end())
			{
				pSensor->bBrightness = true;

				if (pSensor->supported_color_modes.size() == 1)
				{
					//we only support brightness, so it is a normal dimmer and does not support setting a color
					pSensor->bColor_mode = false;
					pSensor->supported_color_modes.clear();
				}
			}
		}
		else
		{
			if (!root["rgb"].empty()) {
				if ((root["rgb"].asString() != "false"))
				{
					pSensor->bColor_mode = true;
					pSensor->supported_color_modes["rgbcct"] = 1;
				}
			}
			if (!root["color_temp"].empty())
			{
				if (root["color_temp"].asString() != "false")
				{
					pSensor->supported_color_modes["color_temp"] = 1;
				}
			}

			// If there is an rgb_command_topic, add RGB colormode
			if (!root["rgb_command_topic"].empty() || !root["rgb_cmd_t"].empty())
			{
				// Note: there is currently no distinction between RGB and RGBW. All will be treated as RGBW devices!
				pSensor->supported_color_modes["rgbw"] = 1;
				pSensor->bColor_mode = true;
			}

			// If there is an hs_command_topic, add HS colormode
			if (!root["hs_command_topic"].empty() || !root["hs_cmd_t"].empty())
			{
				pSensor->supported_color_modes["hs"] = 1;
				pSensor->bColor_mode = true;

				if (!root["hs_value_template"].empty())
					pSensor->hs_value_template = root["hs_value_template"].asString();
				else if (!root["hs_val_tpl"].empty())
					pSensor->hs_value_template = root["hs_val_tpl"].asString();
				CleanValueTemplate(pSensor->hs_value_template);

				if (!root["hs_command_topic"].empty())
					pSensor->rgb_command_topic = root["hs_command_topic"].asString();
				else if (!root["hs_cmd_t"].empty())
					pSensor->rgb_command_topic = root["hs_cmd_t"].asString();

			}

			// If there is a color_temp_command_topic, add color temperature colormode
			if (!root["color_temp_command_topic"].empty() || !root["clr_temp_cmd_t"].empty())
			{
				pSensor->supported_color_modes["color temp"] = 1;
				pSensor->bColor_mode = true;
			}
		}

		if (!root["min_mireds"].empty())
			pSensor->min_mireds = root["min_mireds"].asInt();
		if (!root["min_mirs"].empty())
			pSensor->min_mireds = root["min_mirs"].asInt();
		if (!root["max_mireds"].empty())
			pSensor->max_mireds = root["max_mireds"].asInt();
		if (!root["max_mirs"].empty())
			pSensor->max_mireds = root["max_mirs"].asInt();

		if (!root["color_temp_value_template"].empty())
			pSensor->color_temp_value_template = root["color_temp_value_template"].asString();
		else if (!root["clr_temp_val_tpl"].empty())
			pSensor->color_temp_value_template = root["clr_temp_val_tpl"].asString();
		CleanValueTemplate(pSensor->color_temp_value_template);


		//Select
		if (!root["options"].empty())
		{
			for (const auto& ittMode : root["options"])
			{
				pSensor->select_options.push_back(ittMode.asString());
			}
		}
		if (!root["ops"].empty())
		{
			for (const auto& ittMode : root["ops"])
			{
				pSensor->select_options.push_back(ittMode.asString());
			}
		}
		if ((pSensor->component_type == "select") && (pSensor->select_options.empty()))
		{
			Log(LOG_ERROR, "MQTT_Discovery: component_type 'select' received without options! (%s/%s)!", topic.c_str(), qMessage.c_str());
			return;
		}

		//Climate
		if (!root["mode_command_topic"].empty())
			pSensor->mode_command_topic = root["mode_command_topic"].asString();
		if (!root["mode_cmd_t"].empty())
			pSensor->mode_command_topic = root["mode_cmd_t"].asString();
		if (!root["mode_command_template"].empty())
			pSensor->mode_command_template = root["mode_command_template"].asString();
		if (!root["mode_cmd_tpl"].empty())
			pSensor->mode_command_template = root["mode_cmd_tpl"].asString();
		if (!root["mode_state_topic"].empty())
			pSensor->mode_state_topic = root["mode_state_topic"].asString();
		if (!root["mode_stat_t"].empty())
			pSensor->mode_state_topic = root["mode_stat_t"].asString();
		if (!root["mode_state_template"].empty())
			pSensor->mode_state_template = root["mode_state_template"].asString();
		if (!root["mode_stat_tpl"].empty())
			pSensor->mode_state_template = root["mode_stat_tpl"].asString();
		if (!root["modes"].empty())
		{
			for (const auto& ittMode : root["modes"])
			{
				pSensor->climate_modes.push_back(ittMode.asString());
			}
		}
		if (!root["temperature_command_topic"].empty())
			pSensor->temperature_command_topic = root["temperature_command_topic"].asString();
		if (!root["temperature_command_template"].empty())
			pSensor->temperature_command_template = root["temperature_command_template"].asString();
		if (!root["temp_cmd_t"].empty())
			pSensor->temperature_command_topic = root["temp_cmd_t"].asString();
		if (!root["temperature_state_topic"].empty())
			pSensor->temperature_state_topic = root["temperature_state_topic"].asString();
		if (!root["temp_stat_t"].empty())
			pSensor->temperature_state_topic = root["temp_stat_t"].asString();
		if (!root["temperature_state_template"].empty())
			pSensor->temperature_state_template = root["temperature_state_template"].asString();
		if (!root["temp_stat_tpl"].empty())
			pSensor->temperature_state_template = root["temp_stat_tpl"].asString();
		if (!root["temperature_unit"].empty())
			pSensor->temperature_unit = root["temperature_unit"].asString();
		if (!root["temp_unit"].empty())
			pSensor->temperature_unit = root["temp_unit"].asString();
		if (!root["current_temperature_topic"].empty())
			pSensor->current_temperature_topic = root["current_temperature_topic"].asString();
		if (!root["curr_temp_t"].empty())
			pSensor->current_temperature_topic = root["curr_temp_t"].asString();
		if (!root["current_temperature_template"].empty())
			pSensor->current_temperature_template = root["current_temperature_template"].asString();
		if (!root["curr_temp_tpl"].empty())
			pSensor->current_temperature_template = root["curr_temp_tpl"].asString();
		if (!root["preset_modes"].empty())
		{
			for (const auto& ittMode : root["preset_modes"])
			{
				pSensor->preset_modes.push_back(ittMode.asString());
			}
		}
		if (!root["pr_modes"].empty())
		{
			for (const auto& ittMode : root["pr_modes"])
			{
				pSensor->preset_modes.push_back(ittMode.asString());
			}
		}
		if (!root["preset_mode_command_topic"].empty())
			pSensor->preset_mode_command_topic = root["preset_mode_command_topic"].asString();
		if (!root["preset_mode_command_template"].empty())
			pSensor->preset_mode_command_template = root["preset_mode_command_template"].asString();
		if (!root["preset_mode_state_topic"].empty())
			pSensor->preset_mode_state_topic = root["preset_mode_state_topic"].asString();
		if (!root["preset_mode_value_template"].empty())
			pSensor->preset_mode_value_template = root["preset_mode_value_template"].asString();
		if (!root["pr_mode_cmd_t"].empty())
			pSensor->preset_mode_command_topic = root["pr_mode_cmd_t"].asString();
		if (!root["pr_mode_cmd_tpl"].empty())
			pSensor->preset_mode_command_template = root["pr_mode_cmd_tpl"].asString();
		if (!root["pr_mode_stat_t"].empty())
			pSensor->preset_mode_state_topic = root["pr_mode_stat_t"].asString();
		if (!root["pr_mode_val_tpl"].empty())
			pSensor->preset_mode_value_template = root["pr_mode_val_tpl"].asString();

		CleanValueTemplate(pSensor->mode_state_template);
		CleanValueTemplate(pSensor->mode_command_template);
		CleanValueTemplate(pSensor->temperature_state_template);
		CleanValueTemplate(pSensor->current_temperature_template);
		CleanValueTemplate(pSensor->preset_mode_value_template);
		CleanValueTemplate(pSensor->preset_mode_command_template);

		FixCommandTopicStateTemplate(pSensor->mode_command_topic, pSensor->mode_state_template);
		FixCommandTopicStateTemplate(pSensor->temperature_command_topic, pSensor->temperature_command_template);
		FixCommandTopicStateTemplate(pSensor->preset_mode_command_topic, pSensor->preset_mode_value_template);

		//number (some configs use strings instead of numbers)
		if (!root["min"].empty())
			pSensor->number_min = root["min"].isDouble() ? root["min"].asDouble() : atof(root["min"].asString().c_str());
		if (!root["max"].empty())
			pSensor->number_max = root["max"].isDouble() ? root["max"].asDouble() : atof(root["max"].asString().c_str());
		if (!root["step"].empty())
			pSensor->number_step = root["step"].isDouble() ? root["step"].asDouble() : atof(root["step"].asString().c_str());

		if (!root["qos"].empty())
			pSensor->qos = atoi(root["qos"].asString().c_str());

		for (const auto ittMember : root.getMemberNames())
		{
			if (root[ittMember].isString())
				pSensor->keys[ittMember] = root[ittMember].asString();
		}

		pDevice->sensor_ids[pSensor->unique_id] = true;

		if (pSensor->supported_color_modes.empty())
			Log(LOG_STATUS, "discovered: %s/%s (unique_id: %s)", pDevice->name.c_str(), pSensor->name.c_str(), pSensor->unique_id.c_str());
		else
			Log(LOG_STATUS, "discovered: %s/%s (unique_id: %s)  supported_color_modes: %s", pDevice->name.c_str(), pSensor->name.c_str(), pSensor->unique_id.c_str(), std_map_to_string(pSensor->supported_color_modes).c_str());

		//Sanity checks
		if (pSensor->component_type == "sensor")
		{
			if (pSensor->state_topic.empty())
			{
				Log(LOG_ERROR, "A sensor should have a state_topic!");
				return;
			}
		}
		else if (
			(pSensor->component_type == "switch")
			|| (pSensor->component_type == "light")
			|| (pSensor->component_type == "lock")
			)
		{
			if (pSensor->command_topic.empty())
			{
				Log(LOG_ERROR, "Missing command_topic!");
				return;
			}
			InsertUpdateSwitch(pSensor);
		}
		else if (pSensor->component_type == "cover")
		{
			UpdateBlindPosition(pSensor);
		}
		else if (pSensor->component_type == "select")
		{
			if (pSensor->command_topic.empty())
			{
				Log(LOG_ERROR, "Missing command_topic!");
				return;
			}
			handle_auto_discovery_select(pSensor, message);
		}
		else if (pSensor->component_type == "climate")
		{
			if (pSensor->temperature_command_topic.empty())
			{
				Log(LOG_ERROR, "Missing temperature_command_topic!");
				return;
			}
			handle_auto_discovery_climate(pSensor, message);
		}
		else if (pSensor->component_type == "binary_sensor")
		{
			handle_auto_discovery_binary_sensor(pSensor, message);
		}
		else if (pSensor->component_type == "button")
		{
			handle_auto_discovery_button(pSensor, message);
		}
		else if (pSensor->component_type == "number")
		{
			if (pSensor->state_topic.empty())
			{
				Log(LOG_ERROR, "A number should have a state_topic!");
				return;
			}
		}
		else if (pSensor->component_type == "device_automation")
		{
			if (pSensor->state_topic.empty())
			{
				Log(LOG_ERROR, "device_automation should have a topic!");
				return;
			}
		}
		else if (pSensor->component_type == "fan")
		{
			if (pSensor->percentage_command_topic.empty())
			{
				Log(LOG_ERROR, "A fan should have a percentage_command_topic!");
				return;
			}
			handle_auto_discovery_fan(pSensor, message, "");

			//Should below not be done for climate as well?
			SubscribeTopic(pSensor->preset_mode_state_topic, pSensor->qos);
		}

		//Check if we want to subscribe to this sensor
		bool bDoSubscribe = false;

		//Only subscribe to the below compontent_types for now
		bDoSubscribe =
			(
				(pSensor->component_type == "sensor")
				|| (pSensor->component_type == "binary_sensor")
				|| (pSensor->component_type == "switch")
				|| (pSensor->component_type == "light")
				|| (pSensor->component_type == "lock")
				|| (pSensor->component_type == "select")
				|| (pSensor->component_type == "cover")
				|| (pSensor->component_type == "climate")
				|| (pSensor->component_type == "button")
				|| (pSensor->component_type == "number")
				|| (pSensor->component_type == "device_automation")
				|| (pSensor->component_type == "fan")
				);

		if (bDoSubscribe)
		{
			SubscribeTopic(pSensor->availability_topic, pSensor->qos);
			SubscribeTopic(pSensor->state_topic, pSensor->qos);
			SubscribeTopic(pSensor->position_topic, pSensor->qos);
			SubscribeTopic(pSensor->brightness_state_topic, pSensor->qos);
			SubscribeTopic(pSensor->mode_state_topic, pSensor->qos);
			SubscribeTopic(pSensor->current_temperature_topic, pSensor->qos);
			SubscribeTopic(pSensor->temperature_state_topic, pSensor->qos);
			SubscribeTopic(pSensor->rgb_state_topic, pSensor->qos);
			SubscribeTopic(pSensor->percentage_state_topic, pSensor->qos);
		}
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "MQTT_Discovery (on_auto_discovery_message): Error: %s! (topic: %s, message: %s", e.what(), org_topic.c_str(), qMessage.c_str());
	}
	return;
disovery_invaliddata:
	Log(LOG_ERROR, "MQTT_Discovery: Invalid/Unhandled data received! (topic: %s, message: %s)", org_topic.c_str(), qMessage.c_str());
}

void MQTTAutoDiscover::ApplySignalLevelDevice(const _tMQTTASensor* pSensor)
{
	for (auto& itt : m_discovered_sensors)
	{
		_tMQTTASensor* pDevSensor = &itt.second;
		if (pDevSensor->device_identifiers == pSensor->device_identifiers)
		{
			pDevSensor->SignalLevel = pSensor->SignalLevel;
		}
	}
}

void MQTTAutoDiscover::handle_auto_discovery_sensor_message(const struct mosquitto_message* message, const std::string& subscribed_topic)
{
	std::string topic = subscribed_topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	if (qMessage.empty())
		return;

	bool bIsJSON = false;
	Json::Value root;
	bool ret = ParseJSon(qMessage, root);
	if (ret)
	{
		bIsJSON = root.isObject();
	}

	for (auto& itt : m_discovered_sensors)
	{
		_tMQTTASensor* pSensor = &itt.second;

		if (
			(pSensor->state_topic == topic)
			|| (pSensor->position_topic == topic)
			|| (pSensor->brightness_state_topic == topic)
			|| (pSensor->rgb_state_topic == topic)
			|| (pSensor->mode_state_topic == topic)
			|| (pSensor->temperature_state_topic == topic)
			|| (pSensor->current_temperature_topic == topic)
			|| (pSensor->percentage_state_topic == topic)
			|| (pSensor->preset_mode_state_topic == topic)
			)
		{
			std::string szValue;
			if (bIsJSON)
			{
				if (!root["linkquality"].empty())
				{
					pSensor->SignalLevel = (int)round((10.0F / 255.0F) * root["linkquality"].asFloat());
				}
				if (!root["battery"].empty())
				{
					if (!root["battery"].isObject())
						pSensor->BatteryLevel = root["battery"].asInt();
					else
					{
						if (
							(!pSensor->value_template.empty())
							&& (pSensor->value_template.find("battery") != std::string::npos)
							)
						{
							szValue = GetValueFromTemplate(root, pSensor->value_template);
							if (!szValue.empty())
							{
								pSensor->BatteryLevel = std::stoi(szValue);
							}
						}
					}
				}

				if (!pSensor->position_template.empty())
				{
					szValue = GetValueFromTemplate(root, pSensor->position_template);
				}
				else if (!pSensor->value_template.empty())
				{
					szValue = GetValueFromTemplate(root, pSensor->value_template);
					if (szValue.empty())
					{
						// key not found or value 'null'!
						continue;
					}
					if (pSensor->value_template.find("RSSI") != std::string::npos)
					{
						pSensor->SignalLevel = (int)round((10.0F / 255.0F) * atof(szValue.c_str()));
						ApplySignalLevelDevice(pSensor);
					}
				}
				else
					szValue = qMessage;
			}
			else
			{
				szValue = qMessage;
			}
			pSensor->last_value = szValue;
			pSensor->last_received = mytime(nullptr);
			pSensor->last_topic = topic;
			pSensor->bIsJSON = bIsJSON;
			if (bIsJSON)
			{
				pSensor->last_json_value = qMessage;
			}
#ifdef _DEBUG
			std::string szLogMessage = std_format("%s (value: %s", pSensor->name.c_str(), pSensor->last_value.c_str());
			if (!pSensor->unit_of_measurement.empty())
			{
				szLogMessage += " " + utf8_to_string(pSensor->unit_of_measurement);
			}
			szLogMessage += ")";
			Log(LOG_NORM, "MQTT received: %s", szLogMessage.c_str());
#endif
			if (pSensor->component_type == "sensor")
				handle_auto_discovery_sensor(pSensor, message);
			else if (pSensor->component_type == "switch")
				handle_auto_discovery_switch(pSensor, message);
			else if (pSensor->component_type == "binary_sensor")
				handle_auto_discovery_binary_sensor(pSensor, message);
			else if (pSensor->component_type == "light")
				handle_auto_discovery_light(pSensor, message);
			else if (pSensor->component_type == "cover")
				handle_auto_discovery_cover(pSensor, message);
			else if (pSensor->component_type == "select")
				handle_auto_discovery_select(pSensor, message);
			else if (pSensor->component_type == "climate")
				handle_auto_discovery_climate(pSensor, message);
			else if (pSensor->component_type == "lock")
				handle_auto_discovery_lock(pSensor, message);
			else if (pSensor->component_type == "button")
				handle_auto_discovery_button(pSensor, message);
			else if (pSensor->component_type == "number")
				handle_auto_discovery_number(pSensor, message);
			else if (pSensor->component_type == "fan")
				handle_auto_discovery_fan(pSensor, message, subscribed_topic);
		}
		else if (pSensor->availability_topic == topic)
		{
			handle_auto_discovery_availability(pSensor, qMessage, message);
		}
	}
}

uint64_t MQTTAutoDiscover::UpdateValueInt(int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
	const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
{
	uint64_t DeviceRowIdx = m_sql.UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction, (!user.empty()) ? user.c_str() : m_Name.c_str());
	if (DeviceRowIdx == (uint64_t)-1)
		return -1;
	if (m_bOutputLog)
	{
		std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
		Log(LOG_NORM, szLogString);
	}
	m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
	m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, ID, devname, unit, devType, subType, nValue, sValue);
	m_mainworker.CheckSceneCode(DeviceRowIdx, devType, subType, nValue, sValue, "MQTT Auto");
	return DeviceRowIdx;
}

void MQTTAutoDiscover::handle_auto_discovery_availability(_tMQTTASensor* pSensor, const std::string& payload, const struct mosquitto_message* message)
{
	if (message->retain)
		return;
	if (pSensor->devType == 0)
		return; //not in system yet
	pSensor->bOnline = (payload == "online");
	if (!pSensor->payload_available.empty())
		pSensor->bOnline = (payload == pSensor->payload_available);
	else if (!pSensor->payload_not_available.empty())
		pSensor->bOnline = !(payload == pSensor->payload_not_available);
	if (pSensor->bOnline)
	{
		// update last seen
		m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", TimeToString(nullptr, TF_DateTime).c_str(), m_HwdID,
			pSensor->unique_id.c_str(), pSensor->devUnit);
	}
}

bool MQTTAutoDiscover::GuessSensorTypeValue(const _tMQTTASensor* pSensor, uint8_t& devType, uint8_t& subType, std::string& szOptions, int& nValue, std::string& sValue)
{
	devType = pTypeGeneral;
	subType = sTypeCustom;
	szOptions = "";
	nValue = 0;
	sValue = "";

	std::string szUnit = utf8_to_string(pSensor->unit_of_measurement);
	stdlower(szUnit);

	if (szUnit.empty())
	{
		if (!is_number(pSensor->last_value))
		{
			// conversion failed because the input wasn't a number
			// make it a text sensor
			szUnit = "text";
		}
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;

	if (
		(szUnit == "hpa")
		|| (szUnit == "kpa")
		|| (pSensor->value_template.find("pressure") != std::string::npos)
		)
	{
		devType = pTypeGeneral;
		subType = sTypeBaro;
		float pressure = static_cast<float>(atof(pSensor->last_value.c_str()));
		if (szUnit == "kpa")
			pressure *= 10.0F;
		int nforecast = bmpbaroforecast_cloudy;
		if (pressure <= 980)
			nforecast = bmpbaroforecast_thunderstorm;
		else if (pressure <= 995)
			nforecast = bmpbaroforecast_rain;
		else if (pressure >= 1029)
			nforecast = bmpbaroforecast_sunny;
		sValue = std_format("%.02f;%d", pressure, nforecast);
	}
	else if (szUnit == "ppm")
	{
		devType = pTypeAirQuality;
		subType = sTypeVoltcraft;
		nValue = atoi(pSensor->last_value.c_str());
	}
	else if (szUnit == "v")
	{
		devType = pTypeGeneral;
		subType = sTypeVoltage;
		sValue = std_format("%.3f", static_cast<float>(atof(pSensor->last_value.c_str())));
	}
	else if (szUnit == "mv")
	{
		devType = pTypeGeneral;
		subType = sTypeVoltage;
		sValue = std_format("%.3f", static_cast<float>(atof(pSensor->last_value.c_str())) / 1000.0F);
	}
	else if (szUnit == "a")
	{
		devType = pTypeGeneral;
		subType = sTypeCurrent;
		sValue = std_format("%.3f", static_cast<float>(atof(pSensor->last_value.c_str())));
	}
	else if (szUnit == "w")
	{
		devType = pTypeUsage;
		subType = sTypeElectric;

		float fUsage = static_cast<float>(atof(pSensor->last_value.c_str()));

		if (fUsage < -1000000)
		{
			//Way to negative, probably a bug in the sensor
			return false;
		}

		float fkWh = 0.0F;
		_tMQTTASensor* pkWhSensor = get_auto_discovery_sensor_unit(pSensor, "kwh");
		if (pkWhSensor)
			fkWh = static_cast<float>(atof(pkWhSensor->last_value.c_str())) * 1000.0F;
		else
		{
			pkWhSensor = get_auto_discovery_sensor_unit(pSensor, "wh");
			if (pkWhSensor)
				fkWh = static_cast<float>(atof(pkWhSensor->last_value.c_str()));
			else
			{
				pkWhSensor = get_auto_discovery_sensor_unit(pSensor, "wm");
				if (pkWhSensor)
					fkWh = static_cast<float>(atof(pkWhSensor->last_value.c_str())) / 60.0F;
			}
		}
		if (pkWhSensor)
		{
			if (pkWhSensor->last_received != 0)
			{
				pkWhSensor->sValue = std_format("%.3f;%.3f", fUsage, fkWh);
				mosquitto_message xmessage;
				xmessage.retain = false;
				// Trigger extra update for the kWh sensor with the new W value
				handle_auto_discovery_sensor(pkWhSensor, &xmessage);
			}
		}
		sValue = std_format("%.3f", fUsage);
	}
	else if (
		(szUnit == "kwh")
		|| (szUnit == "wh")
		|| (szUnit == "wm")
		)
	{
		devType = pTypeGeneral;
		subType = sTypeKwh;

		float fUsage = 0;
		float multiply = 1000.0F;

		if (szUnit == "wh")
			multiply = 1.0F;
		else if (szUnit == "wm")
			multiply = 1.0F / 60.0F;

		float fkWh = static_cast<float>(atof(pSensor->last_value.c_str())) * multiply;

		if (fkWh < -1000000)
		{
			//Way to negative, probably a bug in the sensor
			return false;
		}

		if (fkWh == 0)
		{
			//could be the first every value received.
			//could also be that this the middleware sends 0 when it has not received it before
			auto result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
				m_HwdID, pSensor->unique_id.c_str(), devType, subType);
			if (!result.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(result[0][0], ";", strarray);
				if (strarray.size() == 2)
				{
					fkWh = static_cast<float>(atof(strarray[1].c_str()));
				}
			}
		}

		_tMQTTASensor* pWattSensor = get_auto_discovery_sensor_WATT_unit(pSensor);
		if (pWattSensor)
		{
			fUsage = static_cast<float>(atof(pWattSensor->last_value.c_str()));
		}
		sValue = std_format("%.3f;%.3f", fUsage, fkWh);
	}
	else if (
		(szUnit == "lx")
		|| (szUnit == "lux")
		|| (pSensor->state_topic.find("illuminance_lux") != std::string::npos)
		)
	{
		devType = pTypeLux;
		subType = sTypeLux;
		sValue = std_format("%.0f", static_cast<float>(atof(pSensor->last_value.c_str())));
	}
	/*
	*	//our distance sensor is in meters or inches
		else if (szUnit == "m")
		{
			devType = pTypeGeneral;
			subType = sTypeDistance;
			sValue = std_format("%.1f", static_cast<float>(atof(pSensor->last_value.c_str())) * 100.0F);
		}
	*/
	else if (szUnit == "cm")
	{
		devType = pTypeGeneral;
		subType = sTypeDistance;
		sValue = std_format("%.1f", static_cast<float>(atof(pSensor->last_value.c_str())));
	}

	else if (
		(szUnit == "l")
		|| (szUnit == "liter")
		)
	{
		if (
			(pSensor->icon.find("counter-inc") != std::string::npos)
			|| (pSensor->icon.find("pulse") != std::string::npos)
			)
		{
			devType = pTypeGeneral;
			subType = sTypeCounterIncremental;
			sValue = pSensor->last_value;
		}
		else
		{
			devType = pTypeRFXMeter;
			subType = sTypeRFXMeterCount;
			unsigned long counter = (unsigned long)(atof(pSensor->last_value.c_str()) * 1000.0F);
			sValue = std_format("%lu", counter);
		}
	}
	else if (szUnit == "l/hr")
	{
		//our sensor is in Liters / minute
		devType = pTypeGeneral;
		subType = sTypeWaterflow;
		sValue = std_format("%.2f", static_cast<float>(atof(pSensor->last_value.c_str())) / 60.0F);
	}
	else if (
		(szUnit == "db")
		|| (szUnit == "dba")
		)
	{
		devType = pTypeGeneral;
		subType = sTypeSoundLevel;
		sValue = std_format("%d", atoi(pSensor->last_value.c_str()));
	}
	else if (szUnit == "text")
	{
		devType = pTypeGeneral;
		subType = sTypeTextStatus;
		sValue = pSensor->last_value;
	}
	else if (
		(szUnit == "m³")
		|| (szUnit == "m\xB3")
		|| (szUnit == "cubic meters")
		)
	{
		if (
			(pSensor->icon.find("counter-inc") != std::string::npos)
			|| (pSensor->icon.find("pulse") != std::string::npos)
			)
		{
			devType = pTypeGeneral;
			subType = sTypeCounterIncremental;
			sValue = pSensor->last_value;
		}
		else if (pSensor->icon.find("water") != std::string::npos)
		{
			devType = pTypeRAIN;
			subType = sTypeRAIN3;
			float TotalRain = static_cast<float>(atof(pSensor->last_value.c_str()));
			int Rainrate = 0;

			_tMQTTASensor* pRainRateSensor = get_auto_discovery_sensor_unit(pSensor, "mm/h");
			if (pRainRateSensor)
			{
				if (pRainRateSensor->last_received != 0)
				{
					Rainrate = atoi(pRainRateSensor->last_value.c_str());
				}
			}

			sValue = std_format("%d;%.1f", Rainrate, TotalRain * 1000.0F);
		}
		else if (pSensor->icon.find("gas") != std::string::npos)
		{
			devType = pTypeP1Gas;
			subType = sTypeP1Gas;
			float TotalGas = static_cast<float>(atof(pSensor->last_value.c_str()));
			sValue = std_format("%.0f", TotalGas * 1000.0F);
		}
		else if (
			(szUnit == "pulses")
			|| (pSensor->icon.find("pulse") != std::string::npos)
			)
		{
			devType = pTypeGeneral;
			subType = sTypeCounterIncremental;
			sValue = pSensor->last_value;
		}
		else
		{
			devType = pTypeRFXMeter;
			subType = sTypeRFXMeterCount;
			sValue = pSensor->last_value;
		}
	}
	else if (
		(szUnit == "mm/h")
		|| ((szUnit == "mm") && (pSensor->icon == "mdi:weather-rainy"))
		)
	{
		_tMQTTASensor* pRainSensor = get_auto_discovery_sensor_unit(pSensor, "cubic meters");
		if (!pRainSensor)
			pRainSensor = get_auto_discovery_sensor_unit(pSensor, "m³");
		if (pRainSensor)
		{
			if (pRainSensor->last_received != 0)
			{
				float TotalRain = static_cast<float>(atof(pRainSensor->last_value.c_str()));
				int Rainrate = atoi(pSensor->last_value.c_str());

				pRainSensor->sValue = std_format("%d;%.1f", Rainrate, TotalRain * 1000.0F);
				mosquitto_message xmessage;
				xmessage.retain = false;
				handle_auto_discovery_sensor(pRainSensor, &xmessage);
			}
		}
		devType = pTypeGeneral;
		subType = sTypeCustom;
		szOptions = pSensor->unit_of_measurement;
		sValue = pSensor->last_value;
	}
	else if (pSensor->device_class == "illuminance")
	{
		devType = pTypeLux;
		subType = sTypeLux;
		sValue = pSensor->last_value;
	}
	else if (
		(szUnit == "°c")
		|| (szUnit == "\xB0" "c")
		|| (szUnit == "c")
		|| (szUnit == "?c")
		|| (pSensor->object_id == "temperature")
		|| (pSensor->value_template.find("temperature") != std::string::npos)
		|| (pSensor->state_topic.find("temperature") != std::string::npos)
		)
	{
		devType = pTypeTEMP;
		subType = sTypeTEMP1;

		double temp = static_cast<float>(atof(pSensor->last_value.c_str()));

		if (
			(szUnit == "°f")
			|| (szUnit == "\xB0" "f")
			|| (szUnit == "f")
			|| (szUnit == "?f")
			)
		{
			temp = ConvertToCelsius(temp);
		}

		m_sql.GetAddjustment(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;
		sValue = std_format("%.2f", temp);
	}
	else if (szUnit == "%")
	{
		if (
			(pSensor->object_id.find("humidity") != std::string::npos)
			|| (pSensor->device_class.find("humidity") != std::string::npos)
			|| (pSensor->state_topic.find("humidity") != std::string::npos)
			|| (pSensor->unique_id.find("humidity") != std::string::npos)
			|| (pSensor->value_template.find("humidity") != std::string::npos)
			)
		{
			devType = pTypeHUM;
			subType = sTypeHUM2;
			nValue = atoi(pSensor->last_value.c_str());
			sValue = std_format("%d", Get_Humidity_Level(pSensor->nValue));
		}
		else
		{
			devType = pTypeGeneral;
			subType = sTypePercentage;
			szOptions = pSensor->unit_of_measurement;
			sValue = pSensor->last_value;
		}
	}

	/*
		else if (pSensor->state_class == "total_increasing")
		{
			devType = pTypeRFXMeter;
			subType = sTypeRFXMeterCount;
			sValue = pSensor->last_value;
		}
	*/
	else
	{
		devType = pTypeGeneral;
		subType = sTypeCustom;
		szOptions = pSensor->unit_of_measurement;
		sValue = pSensor->last_value;
	}

	if ((devType == pTypeGeneral) && (subType == sTypeCustom) && pSensor->szOptions.empty())
		szOptions = "??";
	return true;
}

MQTTAutoDiscover::_tMQTTASensor* MQTTAutoDiscover::get_auto_discovery_sensor_unit(const _tMQTTASensor* pSensor, const std::string& szMeasurementUnit)
{
	//Retrieved sensor from same device with specified measurement unit
	_tMQTTADevice* pDevice = &m_discovered_devices[pSensor->device_identifiers];
	if (pDevice == nullptr)
		return nullptr; //device not found!?

	int iUIDMatch = -1;
	_tMQTTASensor* pDeviceSensor = nullptr;

	// Check for the correct sensor with the largest match in the UID
	for (const auto ittSensorID : pDevice->sensor_ids)
	{
		if (m_discovered_sensors.find(ittSensorID.first) != m_discovered_sensors.end())
		{
			_tMQTTASensor* pTmpDeviceSensor = &m_discovered_sensors[ittSensorID.first];

			if (
				(pTmpDeviceSensor->component_type != "sensor")
				|| (std_ends_with(pTmpDeviceSensor->object_id, "_min"))
				|| (std_ends_with(pTmpDeviceSensor->object_id, "_max"))
				|| (pTmpDeviceSensor->object_id.find("min_") == 0)
				|| (pTmpDeviceSensor->object_id.find("max_") == 0)
				|| (pTmpDeviceSensor->object_id.find("sensitivity") != std::string::npos)
				|| (pTmpDeviceSensor->object_id.find("alarm") != std::string::npos)
				)
				continue; //ignore sensor

			std::string szUnit = utf8_to_string(pTmpDeviceSensor->unit_of_measurement);
			stdlower(szUnit);

			if (szUnit == szMeasurementUnit)
			{
				if (pSensor->unique_id == pTmpDeviceSensor->unique_id)
					return pTmpDeviceSensor;

				// Check the "match length" of the UID of the DEVICE with the UID of the SENSOR to get the correct subdevice in case the are multiple
				auto ittUnID1 = pSensor->unique_id.begin();
				auto ittUnID2 = pTmpDeviceSensor->unique_id.begin();
				size_t iTlen = (pSensor->unique_id.size() < pTmpDeviceSensor->unique_id.size()) ? pSensor->unique_id.size() : pTmpDeviceSensor->unique_id.size();
				for (int i = 1; i < (int)iTlen; ++i)
				{
					if (*++ittUnID1 != *++ittUnID2)
					{
						// In case of a longer matching string we assume this is a better sensor to use.
						if (i > iUIDMatch)
						{
							iUIDMatch = i;
							pDeviceSensor = pTmpDeviceSensor;
						}
						break;
					}
				}
			}
		}
	}
	return pDeviceSensor;
}

MQTTAutoDiscover::_tMQTTASensor* MQTTAutoDiscover::get_auto_discovery_sensor_unit(const _tMQTTASensor* pSensor, const uint8_t devType, const int subType, const int devUnit)
{
	//Retrieved sensor from same device with specified device type/subtype/unit
	_tMQTTADevice* pDevice = &m_discovered_devices[pSensor->device_identifiers];
	if (pDevice == nullptr)
		return nullptr; //device not found!?

	for (const auto ittSensorID : pDevice->sensor_ids)
	{
		if (m_discovered_sensors.find(ittSensorID.first) != m_discovered_sensors.end())
		{
			_tMQTTASensor* pDeviceSensor = &m_discovered_sensors[ittSensorID.first];

			if (
				(pDeviceSensor->component_type != "sensor")
				|| (std_ends_with(pDeviceSensor->object_id, "_min"))
				|| (std_ends_with(pDeviceSensor->object_id, "_max"))
				|| (pDeviceSensor->object_id.find("min_") == 0)
				|| (pDeviceSensor->object_id.find("max_") == 0)
				|| (pDeviceSensor->object_id.find("sensitivity") != std::string::npos)
				|| (pDeviceSensor->object_id.find("alarm") != std::string::npos)
				)
				continue; //ignore sensor

			uint8_t devsensor_devType, devsensor_subType;
			std::string szOptions;
			int nValue = 0;
			std::string sValue;

			if (!GuessSensorTypeValue(pDeviceSensor, devsensor_devType, devsensor_subType, szOptions, nValue, sValue))
				return nullptr;

			if (devsensor_devType == devType)
			{
				if ((devsensor_subType == subType) || (subType == -1))
				{
					if ((pDeviceSensor->devUnit == devUnit) || (devUnit == -1))
					{
						return pDeviceSensor;
					}
				}
			}
		}
	}
	return nullptr;
}

//This is a special routine that first tries to return the value of COMMAND_CLASS_SENSOR_MULTILEVEL else from COMMAND_CLASS_METER for ZWave nodes
MQTTAutoDiscover::_tMQTTASensor* MQTTAutoDiscover::get_auto_discovery_sensor_WATT_unit(const _tMQTTASensor* pSensor)
{
	//Retrieved sensor from same device with specified measurement unit
	_tMQTTADevice* pDevice = &m_discovered_devices[pSensor->device_identifiers];
	if (pDevice == nullptr)
		return nullptr; //device not found!?

	if (pSensor->unique_id.find("zwave") != 0)
		return get_auto_discovery_sensor_unit(pSensor, "w"); //not ZWave

	std::vector<std::string> strarraySensor;
	StringSplit(pSensor->unique_id, "-", strarraySensor);

	_tMQTTASensor* pSensor2Return = nullptr;

	// Check for the correct sensor with the largest match in the UID
	for (const auto ittSensorID : pDevice->sensor_ids)
	{
		//if (m_discovered_sensors.find(ittSensorID.first) != m_discovered_sensors.end())
		{
			_tMQTTASensor* pTmpDeviceSensor = &m_discovered_sensors[ittSensorID.first];

			std::string szUnit = utf8_to_string(pTmpDeviceSensor->unit_of_measurement);
			stdlower(szUnit);

			if (szUnit == "w")
			{
				if (pSensor->unique_id == pTmpDeviceSensor->unique_id)
					return pTmpDeviceSensor; //non-zwave?

				std::vector<std::string> strarrayTmp;
				StringSplit(pTmpDeviceSensor->unique_id, "-", strarrayTmp);
				if (strarrayTmp.size() < 4)
					continue; //not interested

				if (
					(strarrayTmp[0] == strarraySensor[0])
					&& (strarrayTmp[2] == strarraySensor[2])
					)
				{
					if (
						(pSensor2Return == nullptr)
						|| (strarrayTmp[1] == "49") //zwavejs2mqtt
						|| (strarrayTmp[3] == "49") //zwave-js-ui
						)
						pSensor2Return = pTmpDeviceSensor;
				}
			}
		}
	}
	return pSensor2Return;
}


void MQTTAutoDiscover::handle_auto_discovery_battery(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	if (pSensor->last_value.empty())
		return;
	if (!is_number(pSensor->last_value))
		return;

	int iLevel = atoi(pSensor->last_value.c_str());

	for (auto& itt : m_discovered_sensors)
	{
		_tMQTTASensor* pDevSensor = &itt.second;
		if (pDevSensor->device_identifiers == pSensor->device_identifiers)
		{
			pDevSensor->BatteryLevel = iLevel;
		}
	}
}

void MQTTAutoDiscover::handle_auto_discovery_number(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	if (pSensor->last_value.empty())
		return;
	if (!is_number(pSensor->last_value))
		return;

	return;
	int iValue = atoi(pSensor->last_value.c_str());

	for (auto& itt : m_discovered_sensors)
	{
		_tMQTTASensor* pDevSensor = &itt.second;
		if (pDevSensor->device_identifiers == pSensor->device_identifiers)
		{
			//nothing to be done here
			//pDevSensor->number_current = iValue;
		}
	}
}

void MQTTAutoDiscover::handle_auto_discovery_sensor(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	if (
		(pSensor->value_template == "action")
		|| (pSensor->value_template == "click")
		|| (pSensor->object_id.find("scene_state_scene") != std::string::npos)
		|| (pSensor->object_id == "action")
		)
	{
		//Scene
		if (pSensor->last_value == "wakeup")
			return;

		InsertUpdateSwitch(pSensor);
		return;
	}

	if (
		(pSensor->object_id.find("battery") != std::string::npos)
		&& is_number(pSensor->last_value)
		)
	{
		handle_auto_discovery_battery(pSensor, message);
		return;
	}

	if (
		(std_ends_with(pSensor->object_id, "_min"))
		|| (std_ends_with(pSensor->object_id, "_max"))
		|| (pSensor->object_id.find("min_") == 0)
		|| (pSensor->object_id.find("max_") == 0)
		|| (pSensor->object_id.find("sensitivity") != std::string::npos)
		|| (pSensor->object_id.find("alarm") != std::string::npos)
		)
	{
		//this is probably a configuration value that is specified as a sensor (it should not be here)
		return;
	}

	std::string szUnit = utf8_to_string(pSensor->unit_of_measurement);
	if (szUnit == "dBm")
	{
		// probably WIFI strength, should be handled differently and used as signal strength
		return;
	}

	pSensor->devUnit = 1;
	bool bIsTemp = false;
	bool bIsHum = false;
	bool bIsBaro = false;

	if (!GuessSensorTypeValue(pSensor, pSensor->devType, pSensor->subType, pSensor->szOptions, pSensor->nValue, pSensor->sValue))
		return;

	if (
		(pSensor->devType == pTypeTEMP)
		&& (pSensor->subType == sTypeTEMP1)
		)
	{
		bIsTemp = true;
	}
	else if (
		(pSensor->devType == pTypeHUM)
		&& (pSensor->subType == sTypeHUM2)
		)
	{
		bIsHum = true;
	}
	else if (
		(pSensor->devType == pTypeGeneral)
		&& (pSensor->subType == sTypeBaro)
		)
	{
		bIsBaro = true;
	}

	if (bIsTemp || bIsHum || bIsBaro)
	{
		if (m_discovered_devices.find(pSensor->device_identifiers) == m_discovered_devices.end())
			return; //not discovered yet!?

		std::string szDeviceID = pSensor->device_identifiers;
		std::string sDeviceName = m_discovered_devices[pSensor->device_identifiers].name;

		int nValue = 0;
		std::string sValue;
		uint8_t devType = pSensor->devType;
		uint8_t subType = pSensor->subType;

		float temp = 0;
		int humidity = 0;
		float pressure = 0;

		_tMQTTASensor* pTempSensor = (bIsTemp) ? pSensor : nullptr;
		_tMQTTASensor* pHumSensor = (bIsHum) ? pSensor : nullptr;
		_tMQTTASensor* pBaroSensor = (bIsBaro) ? pSensor : nullptr;

		bool bTreatStandAlone = false;

		if (
			(pSensor->object_id.find("dew_point") != std::string::npos)
			|| (pSensor->state_topic.find("Dew_point") != std::string::npos)
			)
		{
			//it's a standalone sensor
			bTreatStandAlone = true;
		}

		if (!bTreatStandAlone)
		{
			if (!pTempSensor)
				pTempSensor = get_auto_discovery_sensor_unit(pSensor, pTypeTEMP, sTypeTEMP1);
			if (!pHumSensor)
				pHumSensor = get_auto_discovery_sensor_unit(pSensor, pTypeHUM, sTypeHUM2);
			if (!pBaroSensor)
				pBaroSensor = get_auto_discovery_sensor_unit(pSensor, pTypeGeneral, sTypeBaro);
		}

		if (pTempSensor)
			temp = static_cast<float>(atof(pTempSensor->last_value.c_str()));
		if (pHumSensor)
			humidity = pHumSensor->nValue;
		if (pBaroSensor)
		{
			pressure = static_cast<float>(atof(pBaroSensor->last_value.c_str()));
			if (pBaroSensor->unit_of_measurement == "kPa")
				pressure *= 10.0F;
		}

		if (pTempSensor && pHumSensor && pBaroSensor)
		{
			//Temp + Hum + Baro
			if (
				(pTempSensor->last_received == 0)
				|| (pHumSensor->last_received == 0)
				|| (pBaroSensor->last_received == 0)
				)
				return; //not all 3 received yet
			devType = pTypeTEMP_HUM_BARO;
			subType = sTypeTHBFloat;

			uint8_t nforecast = wsbaroforecast_some_clouds;
			if (pressure <= 980)
				nforecast = wsbaroforecast_heavy_rain;
			else if (pressure <= 995)
			{
				if (temp > 1)
					nforecast = wsbaroforecast_rain;
				else
					nforecast = wsbaroforecast_snow;
			}
			else if (pressure >= 1029)
				nforecast = wsbaroforecast_sunny;
			sValue = std_format("%.2f;%d;%d;%.1f;%d", temp, humidity, Get_Humidity_Level(humidity), pressure, nforecast);
		}
		else if (pTempSensor && pHumSensor)
		{
			//Temp + Hum
			if (
				(pTempSensor->last_received == 0)
				|| (pHumSensor->last_received == 0)
				)
				return; // not all 2 received yet
			devType = pTypeTEMP_HUM;
			subType = sTypeTH1;

			sValue = std_format("%.2f;%d;%d", temp, humidity, Get_Humidity_Level(humidity));
		}
		else
		{
			//No combination found, use original device parameters
			devType = pSensor->devType;
			subType = pSensor->subType;
			szDeviceID = pSensor->unique_id;
			sDeviceName = pSensor->name;

			nValue = pSensor->nValue;
			sValue = pSensor->sValue;
		}
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
			m_HwdID, szDeviceID.c_str(), 1, devType, subType);
		if (result.empty())
		{
			// Insert
			int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
				"VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
				m_HwdID, szDeviceID.c_str(), devType, subType, pSensor->SignalLevel, pSensor->BatteryLevel, sDeviceName.c_str(), iUsed, nValue, sValue.c_str());
		}
		else
		{
			// Update
			if (message->retain)
				return; // only update when a new value is received
			UpdateValueInt(m_HwdID, szDeviceID.c_str(), 1, devType, subType, pSensor->SignalLevel, pSensor->BatteryLevel, nValue, sValue.c_str(), result[0][0]);
		}
	}
	else
	{
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
			pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType);
		if (result.empty())
		{
			//Insert
			int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
			uint8_t SwitchType = 0;

			if (
				(pSensor->devType == pTypeGeneral)
				&& (pSensor->subType == sTypeCounterIncremental)
				)
			{
				if (
					(szUnit == "l")
					|| (szUnit == "liter")
					)
					SwitchType = MTYPE_WATER;
			}

			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, Used, Options, nValue, sValue) "
				"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, '1;%q', %d, '%q')",
				m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, SwitchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed,
				pSensor->szOptions.c_str(), pSensor->nValue, pSensor->sValue.c_str());
		}
		else
		{
			//Update
			if (message->retain)
				return; //only update when a new value is received
			UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
				pSensor->sValue.c_str(), result[0][0]);
		}
	}
}

void MQTTAutoDiscover::handle_auto_discovery_switch(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_binary_sensor(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	//	if (pSensor->object_id.find("battery") != std::string::npos)
		//	return;
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_button(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_fan(_tMQTTASensor* pSensor, const struct mosquitto_message* message, const std::string& topic)
{
	//handle it as a switch/drimmer
	if (pSensor->preset_mode_state_topic != topic)
	{
		InsertUpdateSwitch(pSensor);
	}

	// Create/update Selector device for preset_modes
	bool bValid = true;
	if (!pSensor->preset_modes.empty())
	{
		pSensor->devType = pTypeGeneralSwitch;
		pSensor->subType = sSwitchGeneralSwitch;
		int switchType = STYPE_Selector;

		bool bIsNewDevice = false;

		uint8_t unit = FAN_PRESET_UNIT; //preset mode

		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, unit);
		if (result.empty())
		{
			// New switch, add it to the system
			bIsNewDevice = true;
			int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue, Options) "
				"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, %d, '0', null)",
				m_HwdID, pSensor->unique_id.c_str(), unit, pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed, 0);
			result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, unit);
			if (result.empty())
				return; // should not happen!
		}

		if (
			(pSensor->preset_mode_state_topic == topic)
			|| (bIsNewDevice)
			)
		{
			bool bIsJSON = false;
			Json::Value root;
			bool ret = ParseJSon(pSensor->last_value, root);
			if (ret)
			{
				bIsJSON = root.isObject();
			}

			std::string current_mode;
			if (
				(!pSensor->preset_mode_value_template.empty())
				&& (bIsJSON)
				)
			{
				current_mode = GetValueFromTemplate(root, pSensor->preset_mode_value_template);
				if ((pSensor->preset_mode_state_topic == topic) && current_mode.empty())
				{
					Log(LOG_ERROR, "Climate device no idea how to interpretate preset_mode_state value (%s)", pSensor->unique_id.c_str());
					bValid = false;
				}
			}
			else
				current_mode = pSensor->last_value;

			if (bValid)
			{
				std::string szIdx = result[0][0];
				uint64_t DevRowIdx = std::stoull(szIdx);
				std::string szDeviceName = result[0][1];
				int nValue = atoi(result[0][2].c_str());
				std::string sValue = result[0][3];
				std::string sOptions = result[0][4];

				int iActualIndex = current_mode.empty() ? 0 : -1;

				// Build switch options
				int iValueIndex = 0;
				std::string tmpOptionString;
				for (const auto& ittOptions : pSensor->preset_modes)
				{
					if (ittOptions == current_mode)
						iActualIndex = iValueIndex;
					if (!tmpOptionString.empty())
						tmpOptionString += "|";
					tmpOptionString += ittOptions;
					iValueIndex += 10;
				}

				if (iActualIndex == -1)
				{
					Log(LOG_ERROR, "Climate device invalid/unknown preset_mode received! (%s: %s)", pSensor->unique_id.c_str(), current_mode.c_str());
					bValid = false;
				}

				if (bValid)
				{
					std::map<std::string, std::string> optionsMap;
					optionsMap["SelectorStyle"] = "0";
					optionsMap["LevelOffHidden"] = "false";
					optionsMap["LevelNames"] = tmpOptionString;

					std::string newOptions = m_sql.FormatDeviceOptions(optionsMap);
					if (newOptions != sOptions)
						m_sql.SetDeviceOptions(DevRowIdx, optionsMap);

					pSensor->nValue = (iActualIndex == 0) ? 0 : 2;
					pSensor->sValue = std_format("%d", iActualIndex);

					if ((pSensor->nValue != nValue) || (pSensor->sValue != sValue))
					{
						UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), unit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
							pSensor->sValue.c_str(), szDeviceName);
					}
				}
			}
		}
	}
}

void MQTTAutoDiscover::handle_auto_discovery_light(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_lock(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_cover(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_scene(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTTAutoDiscover::handle_auto_discovery_select(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	if (qMessage.empty())
		return;

	bool bIsJSON = false;
	Json::Value root;
	bool ret = ParseJSon(qMessage, root);
	if (ret)
	{
		bIsJSON = root.isObject();
	}

	if (pSensor->select_options.empty())
		return;

	// Create/update Selector device for config and update payloads 
	std::string current_mode;
	if (!bIsJSON)
	{
		// Assume the payload is the new current value
		current_mode = qMessage;
	}
	else
	{
		if (!pSensor->value_template.empty())
		{
			current_mode = GetValueFromTemplate(root, pSensor->value_template);
			if ((pSensor->state_topic == topic) && current_mode.empty())
			{
				Log(LOG_ERROR, "Select device no idea how to interpretate state values (%s)", pSensor->unique_id.c_str());
				return;
			}
		}
	}

	pSensor->devType = pTypeGeneralSwitch;
	pSensor->subType = sSwitchGeneralSwitch;
	int switchType = STYPE_Selector;

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType);
	if (result.empty())
	{
		// New switch, add it to the system
		int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue, Options) "
			"VALUES (%d, '%q', 1, %d, %d, %d, %d, %d, '%q', %d, %d, '0', null)",
			m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed, 0);
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType,
			pSensor->subType);
		if (result.empty())
			return; // should not happen!
	}
	else {
		if (pSensor->last_value.empty())
			return;
	}

	std::string szIdx = result[0][0];
	uint64_t DevRowIdx = std::stoull(szIdx);
	std::string szDeviceName = result[0][1];
	int nValue = atoi(result[0][2].c_str());
	std::string sValue = result[0][3];

	std::string sOldOptions = result[0][4];
	std::map<std::string, std::string> oldOptionsMap = m_sql.BuildDeviceOptions(sOldOptions);

	int iActualIndex = ((current_mode != "") ? -1 : 0);

	// Build switch options
	int iValueIndex = 0;
	std::string tmpOptionString;
	for (const auto& ittOptions : pSensor->select_options)
	{
		if (ittOptions == current_mode)
			iActualIndex = iValueIndex;
		if (!tmpOptionString.empty())
			tmpOptionString += "|";
		tmpOptionString += ittOptions;
		iValueIndex += 10;
	}

	if (iActualIndex == -1) {
		Log(LOG_ERROR, "Select device doesn't have the option for received STATE \"%s\")", current_mode.c_str());
		iActualIndex = atoi(sValue.c_str());
	}

	std::map<std::string, std::string> optionsMap;
	if (oldOptionsMap.find("SelectorStyle") == oldOptionsMap.end())
	{
		optionsMap["SelectorStyle"] = (pSensor->select_options.size() > 5) ? "1" : "0";
		optionsMap["LevelOffHidden"] = "false";
	}
	else
	{
		optionsMap["SelectorStyle"] = oldOptionsMap["SelectorStyle"];
		optionsMap["LevelOffHidden"] = oldOptionsMap["LevelOffHidden"];
	}

	std::vector<std::string> strarray;
	StringSplit(tmpOptionString, "|", strarray);

	size_t totalOptions = strarray.size();
	size_t totalOldOptions = 0;

	if (oldOptionsMap.find("LevelNames") != oldOptionsMap.end())
	{
		StringSplit(tmpOptionString, "|", strarray);
		totalOldOptions = strarray.size();
	}

	if (totalOptions != totalOldOptions)
	{
		//Avoid renamed level names by user in Domoticz
		optionsMap["LevelNames"] = tmpOptionString;
	}
	else
		optionsMap["LevelNames"] = oldOptionsMap["LevelNames"];

	std::string newOptions = m_sql.FormatDeviceOptions(optionsMap);
	if (newOptions != sOldOptions)
		m_sql.SetDeviceOptions(DevRowIdx, optionsMap);

	pSensor->nValue = (iActualIndex == 0) ? 0 : 2;
	pSensor->sValue = std_format("%d", iActualIndex);

	if ((pSensor->nValue != nValue) || (pSensor->sValue != sValue))
	{
		UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
			pSensor->sValue.c_str(), szDeviceName);
	}
}

void MQTTAutoDiscover::handle_auto_discovery_climate(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	if (qMessage.empty())
		return;

	bool bIsJSON = false;
	Json::Value root;
	bool ret = ParseJSon(qMessage, root);
	if (ret)
	{
		bIsJSON = root.isObject();
	}

	// Create/update Selector device for config and update payloads 
	bool bValid = true;
	if (!pSensor->climate_modes.empty())
	{
		pSensor->devType = pTypeGeneralSwitch;
		pSensor->subType = sSwitchGeneralSwitch;
		int switchType = STYPE_Selector;

		bool bIsNewDevice = false;

		uint8_t unit = CLIMATE_MODE_UNIT;

		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, unit);
		if (result.empty())
		{
			// New switch, add it to the system
			bIsNewDevice = true;
			int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue, Options) "
				"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, %d, '0', null)",
				m_HwdID, pSensor->unique_id.c_str(), unit, pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed, 0);
			result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, unit);
			if (result.empty())
				return; // should not happen!
		}

		if (
			(pSensor->mode_state_topic == topic)
			|| (bIsNewDevice)
			)
		{
			std::string current_mode;

			if (bIsJSON)
			{
				if (!pSensor->mode_state_template.empty())
				{
					current_mode = GetValueFromTemplate(root, pSensor->mode_state_template);
					if ((pSensor->mode_state_topic == topic) && current_mode.empty())
					{
						Log(LOG_ERROR, "Climate device no idea how to interpretate state values (%s)", pSensor->unique_id.c_str());
						bValid = false;
					}
				}
				else
				{
					//should have a template for a json value!
					Log(LOG_ERROR, "Climate device no idea how to interpretate state values (no state template!)(%s)", pSensor->unique_id.c_str());
					bValid = false;
				}
			}
			else
			{
				if (!pSensor->mode_state_template.empty())
				{
					current_mode = GetValueFromTemplate(qMessage, pSensor->mode_state_template);
					if ((pSensor->mode_state_topic == topic) && current_mode.empty())
					{
						//silence error for now
						current_mode = qMessage;
						//Log(LOG_ERROR, "Climate device no idea how to interpretate state values (%s)", pSensor->unique_id.c_str());
						//bValid = false;
					}
				}
				else
					current_mode = qMessage;
			}

			if (bValid)
			{
				std::string szIdx = result[0][0];
				uint64_t DevRowIdx = std::stoull(szIdx);
				std::string szDeviceName = result[0][1];
				int nValue = atoi(result[0][2].c_str());
				std::string sValue = result[0][3];
				std::string sOptions = result[0][4];

				int iActualIndex = current_mode.empty() ? 0 : -1;

				// Build switch options
				int iValueIndex = 0;
				std::string tmpOptionString;
				for (const auto& ittOptions : pSensor->climate_modes)
				{
					if (ittOptions == current_mode)
						iActualIndex = iValueIndex;
					if (!tmpOptionString.empty())
						tmpOptionString += "|";
					tmpOptionString += ittOptions;
					iValueIndex += 10;
				}

				if (iActualIndex == -1)
				{
					Log(LOG_ERROR, "Climate device invalid/unknown mode received! (%s: %s)", pSensor->unique_id.c_str(), current_mode.c_str());
					bValid = false;
				}

				if (bValid)
				{
					std::map<std::string, std::string> optionsMap;
					optionsMap["SelectorStyle"] = "0";
					optionsMap["LevelOffHidden"] = "false";
					optionsMap["LevelNames"] = tmpOptionString;

					std::string newOptions = m_sql.FormatDeviceOptions(optionsMap);
					if (newOptions != sOptions)
						m_sql.SetDeviceOptions(DevRowIdx, optionsMap);

					pSensor->nValue = (iActualIndex == 0) ? 0 : 2;
					pSensor->sValue = std_format("%d", iActualIndex);

					if ((pSensor->nValue != nValue) || (pSensor->sValue != sValue))
					{
						UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), unit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
							pSensor->sValue.c_str(), szDeviceName);
					}
				}
			}
		}
	}

	// Create/update Selector device for preset_modes
	bValid = true;
	if (!pSensor->preset_modes.empty())
	{
		pSensor->devType = pTypeGeneralSwitch;
		pSensor->subType = sSwitchGeneralSwitch;
		int switchType = STYPE_Selector;

		bool bIsNewDevice = false;

		uint8_t unit = CLIMATE_PRESET_UNIT; //preset mode

		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, unit);
		if (result.empty())
		{
			// New switch, add it to the system
			bIsNewDevice = true;
			int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue, Options) "
				"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, %d, '0', null)",
				m_HwdID, pSensor->unique_id.c_str(), unit, pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed, 0);
			result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, unit);
			if (result.empty())
				return; // should not happen!
		}

		if (
			(pSensor->preset_mode_state_topic == topic)
			|| (bIsNewDevice)
			)
		{
			std::string current_mode;
			if (
				(!pSensor->preset_mode_value_template.empty())
				&& (bIsJSON)
				)
			{
				current_mode = GetValueFromTemplate(root, pSensor->preset_mode_value_template);
				if ((pSensor->preset_mode_state_topic == topic) && current_mode.empty())
				{
					Log(LOG_ERROR, "Climate device no idea how to interpretate preset_mode_state value (%s)", pSensor->unique_id.c_str());
					bValid = false;
				}
			}
			else
				current_mode = qMessage;

			if (bValid)
			{
				std::string szIdx = result[0][0];
				uint64_t DevRowIdx = std::stoull(szIdx);
				std::string szDeviceName = result[0][1];
				int nValue = atoi(result[0][2].c_str());
				std::string sValue = result[0][3];
				std::string sOptions = result[0][4];

				int iActualIndex = current_mode.empty() ? 0 : -1;

				// Build switch options
				int iValueIndex = 0;
				std::string tmpOptionString;
				for (const auto& ittOptions : pSensor->preset_modes)
				{
					if (ittOptions == current_mode)
						iActualIndex = iValueIndex;
					if (!tmpOptionString.empty())
						tmpOptionString += "|";
					tmpOptionString += ittOptions;
					iValueIndex += 10;
				}

				if (iActualIndex == -1)
				{
					Log(LOG_ERROR, "Climate device invalid/unknown preset_mode received! (%s: %s)", pSensor->unique_id.c_str(), current_mode.c_str());
					bValid = false;
				}

				if (bValid)
				{
					std::map<std::string, std::string> optionsMap;
					optionsMap["SelectorStyle"] = "0";
					optionsMap["LevelOffHidden"] = "false";
					optionsMap["LevelNames"] = tmpOptionString;

					std::string newOptions = m_sql.FormatDeviceOptions(optionsMap);
					if (newOptions != sOptions)
						m_sql.SetDeviceOptions(DevRowIdx, optionsMap);

					pSensor->nValue = (iActualIndex == 0) ? 0 : 2;
					pSensor->sValue = std_format("%d", iActualIndex);

					if ((pSensor->nValue != nValue) || (pSensor->sValue != sValue))
					{
						UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), unit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
							pSensor->sValue.c_str(), szDeviceName);
					}
				}
			}
		}
	}

	// Create/update SetPoint Thermostat for config and update payloads 
	bValid = true;
	if (!pSensor->temperature_command_topic.empty())
	{
		double temp_setpoint = 18;
		bool bHaveReceiveValue = false;
		if (pSensor->temperature_state_topic == topic)
		{
			if (bIsJSON)
			{
				//Current Setpoint
				if (!pSensor->temperature_state_template.empty())
				{
					std::string tstring = GetValueFromTemplate(root, pSensor->temperature_state_template);
					if (tstring.empty())
					{
						Log(LOG_ERROR, "Climate device unhandled temperature_state_template (%s)", pSensor->unique_id.c_str());
						bValid = false;
					}
					temp_setpoint = static_cast<double>(atof(tstring.c_str()));
				}
			}
			else
				temp_setpoint = static_cast<double>(atof(qMessage.c_str()));
			bHaveReceiveValue = true;
		}
		if (bValid)
		{
			if (pSensor->temperature_unit == "F")
			{
				// Convert back to Celsius
				temp_setpoint = ConvertToCelsius(temp_setpoint);
			}

			pSensor->nValue = 0;
			pSensor->sValue = std_format("%.2f", temp_setpoint);
			pSensor->devType = pTypeSetpoint;
			pSensor->subType = sTypeSetpoint;
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
				pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType);
			if (result.empty())
			{
				// Insert
				int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
				m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
					"VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
					m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed,
					pSensor->nValue, pSensor->sValue.c_str());
			}
			else
			{
				// Update
				if (bHaveReceiveValue)
				{
					UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
						pSensor->sValue.c_str(),
						result[0][0]);
				}
			}
		}
	}

	// Create/update Temp device for config and update payloads
	bValid = true;
	if (pSensor->current_temperature_topic == topic)
	{
		double temp_current = 0;
		if (bIsJSON)
		{
			//Current temperature
			if (!pSensor->current_temperature_template.empty())
			{
				std::string tstring = GetValueFromTemplate(root, pSensor->current_temperature_template);
				if (tstring.empty())
				{
					Log(LOG_ERROR, "Climate device unhandled current_temperature_template (%s)", pSensor->unique_id.c_str());
					bValid = false;
				}
				temp_current = static_cast<double>(atof(tstring.c_str()));
			}
		}
		else
			temp_current = static_cast<double>(atof(qMessage.c_str()));

		if (bValid)
		{
			if (pSensor->temperature_unit == "F")
			{
				// Convert back to Celsius
				temp_current = ConvertToCelsius(temp_current);
			}

			pSensor->devType = pTypeTEMP;
			pSensor->subType = sTypeTEMP1;

			pSensor->nValue = 0;

			float AddjValue = 0.0F;
			float AddjMulti = 1.0F;

			m_sql.GetAddjustment(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, AddjValue, AddjMulti);
			temp_current += AddjValue;
			pSensor->sValue = std_format("%.1f", temp_current);

			pSensor->subType = sTypeSetpoint;
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
				pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType);
			if (result.empty())
			{
				// Insert
				int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
				m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
					"VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
					m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), iUsed,
					pSensor->nValue, pSensor->sValue.c_str());
			}
			else
			{
				// Update
				UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
					pSensor->sValue.c_str(), result[0][0]);
			}
		}
	}
}

void MQTTAutoDiscover::InsertUpdateSwitch(_tMQTTASensor* pSensor)
{
	pSensor->devUnit = 1;
	pSensor->devType = pTypeGeneralSwitch;
	pSensor->subType = sSwitchGeneralSwitch;

	if (pSensor->component_type == "cover")
	{
		UpdateBlindPosition(pSensor);
		return;
	}

	int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
	std::string szSwitchCmd = pSensor->last_value;
	int level = 0;
	int switchType = STYPE_OnOff;
	std::string szSensorName = pSensor->name;

	if ((pSensor->bColor_mode) || (!pSensor->supported_color_modes.empty()))
	{
		pSensor->devType = pTypeColorSwitch;
		pSensor->subType = sTypeColor_RGB;

		bool bHaveBrightness = (
			(pSensor->supported_color_modes.find("brightness") != pSensor->supported_color_modes.end())
			|| (pSensor->bBrightness)
			);
		bool bHaveColor = (
			(pSensor->bColor_mode)
			|| (pSensor->supported_color_modes.find("xy") != pSensor->supported_color_modes.end())
			|| (pSensor->supported_color_modes.find("rgb") != pSensor->supported_color_modes.end())
			|| (pSensor->supported_color_modes.find("rgbw") != pSensor->supported_color_modes.end())
			|| (pSensor->supported_color_modes.find("rgbww") != pSensor->supported_color_modes.end())
			|| (pSensor->supported_color_modes.find("rgbcct") != pSensor->supported_color_modes.end())
			);
		bool bHaveColorTemp = (pSensor->supported_color_modes.find("color_temp") != pSensor->supported_color_modes.end());

		if (bHaveColor)
		{
			if (
				(pSensor->supported_color_modes.find("rgbw") != pSensor->supported_color_modes.end())
				|| (
					(pSensor->supported_color_modes.find("rgb") != pSensor->supported_color_modes.end())
					&& (pSensor->supported_color_modes.find("white") != pSensor->supported_color_modes.end())
					)
				)
			{
				pSensor->subType = sTypeColor_RGB_W_Z;
			}
			else if (
				(pSensor->supported_color_modes.find("rgbww") != pSensor->supported_color_modes.end())
				|| (pSensor->supported_color_modes.find("rgb") != pSensor->supported_color_modes.end() && bHaveColorTemp)
				|| (pSensor->supported_color_modes.find("xy") != pSensor->supported_color_modes.end() && bHaveColorTemp)
				)
			{
				pSensor->subType = sTypeColor_RGB_CW_WW_Z;
			}
			else if (
				(pSensor->supported_color_modes.find("rgb") != pSensor->supported_color_modes.end() && !bHaveColorTemp)
				|| (pSensor->supported_color_modes.find("xy") != pSensor->supported_color_modes.end() && !bHaveColorTemp)
				)
			{
				pSensor->subType = sTypeColor_RGB;
			}
			else if (
				(pSensor->supported_color_modes.find("rgbcct") != pSensor->supported_color_modes.end())
				|| (pSensor->supported_color_modes.find("hs") != pSensor->supported_color_modes.end())
				)
			{
				pSensor->subType = sTypeColor_RGB_CW_WW;
			}
			else if (bHaveColorTemp)
			{
				pSensor->subType = sTypeColor_CW_WW;
			}
			else
			{
				//assume RGB
				pSensor->subType = sTypeColor_RGB;
				//Log(LOG_ERROR, "Unhandled color switch type '%s' (%s)", std_map_to_string(pSensor->supported_color_modes).c_str(), pSensor->name.c_str());
				//return;
			}
		}
		else if (bHaveColorTemp)
		{
			pSensor->subType = sTypeColor_CW_WW;
		}
		else if (bHaveBrightness)
		{
			//a normal dimmer
			pSensor->devType = pTypeGeneralSwitch;
			pSensor->subType = sSwitchGeneralSwitch;
		}
		else
		{
			Log(LOG_ERROR, "Unhandled color switch type '%s' (%s)", std_map_to_string(pSensor->supported_color_modes).c_str(), pSensor->name.c_str());
			return;
		}
		switchType = STYPE_Dimmer;
	}
	else if (
		(pSensor->bBrightness)
		|| (!pSensor->brightness_command_topic.empty())
		|| (!pSensor->brightness_state_topic.empty())
		)
	{
		switchType = STYPE_Dimmer;
	}
	else if (!pSensor->percentage_command_topic.empty())
	{
		switchType = STYPE_Dimmer;
	}
	else if (pSensor->component_type == "binary_sensor")
	{
		if (
			(pSensor->object_id.find("_status") != std::string::npos)
			|| (pSensor->object_id.find("_unknown") != std::string::npos)
			|| (pSensor->object_id.find("tamper") != std::string::npos)
			)
		{
			//Don't add these into our system used
			iUsed = 0;
		}
	}
	if (pSensor->object_id == "occupancy")
		switchType = STYPE_Motion;
	else if (pSensor->object_id == "motion")
		switchType = STYPE_Motion;
	else if (pSensor->object_id == "contact")
		switchType = STYPE_DoorContact;
	else if (pSensor->object_id == "smoke")
		switchType = STYPE_SMOKEDETECTOR;
	else if (pSensor->object_id == "door-window")
		switchType = STYPE_DoorContact;
	else if (pSensor->object_id == "door_state")
		switchType = STYPE_DoorContact;
	else if (pSensor->component_type == "binary_sensor" && !pSensor->device_class.empty() && pSensor->device_class == "door")
		switchType = STYPE_DoorContact;
	else if (pSensor->component_type == "binary_sensor" && !pSensor->device_class.empty() && pSensor->device_class == "motion")
		switchType = STYPE_Motion;
	else if (pSensor->component_type == "lock")
		switchType = STYPE_DoorLock;
	else if (
		(pSensor->value_template == "action")
		|| (pSensor->object_id == "action")
		|| (pSensor->value_template == "click")
		)
	{
		//Scene devices
		if (pSensor->last_value.empty())
			return;
		if (pSensor->last_value == "on")
		{
			pSensor->devUnit = 1;
			switchType = STYPE_PushOn;
			szSwitchCmd = "on";
		}
		else if (pSensor->last_value == "off")
		{
			pSensor->devUnit = 2;
			switchType = STYPE_PushOff;
			szSwitchCmd = "off";
		}
		else if (pSensor->last_value == "brightness_up")
		{
			pSensor->devUnit = 3;
			switchType = STYPE_PushOn;
			szSwitchCmd = "on";
		}
		else if (pSensor->last_value == "brightness_down")
		{
			pSensor->devUnit = 4;
			switchType = STYPE_PushOff;
			szSwitchCmd = "off";
		}
		else {
			//Assume action trigger
			pSensor->devUnit = Crc8_strMQ(0, (uint8_t*)pSensor->last_value.c_str(), pSensor->last_value.size());
			switchType = STYPE_PushOn;
			szSwitchCmd = "on";
		}
		szSensorName += "_" + pSensor->last_value;
	}
	else if (pSensor->object_id.find("scene_state_scene") != std::string::npos)
	{
		pSensor->devUnit = atoi(pSensor->last_value.c_str());
		switchType = STYPE_PushOn;
		szSensorName += "_" + pSensor->last_value;
		szSwitchCmd = "on";
	}
	else if (pSensor->object_id.find("battery") != std::string::npos)
	{
		//99,9% also received as percentage, so don't add this as 'used'
		iUsed = 0;
	}
	else if (pSensor->component_type == "button")
	{
		switchType = STYPE_PushOn;
		if (!pSensor->payload_press.empty())
			szSwitchCmd = pSensor->payload_press;
		else
			szSwitchCmd = "on";
	}

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID, Name, nValue, sValue, Color, SubType, SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit);
	if (result.empty())
	{
		// New switch, add it to the system
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
			"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, %d, '%q')",
			m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, szSensorName.c_str(), iUsed, 0, "0");
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Color,SubType, SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit);
	}
	if (result.empty())
		return; // should not happen!

	if (pSensor->last_value.empty())
		return;

	std::string szIdx = result[0][0];
	uint64_t DevRowIdx = std::stoull(szIdx);
	std::string szDeviceName = result[0][1];
	int nValue = atoi(result[0][2].c_str());
	std::string sValue = result[0][3];
	std::string sColor = result[0][4];
	int subType = atoi(result[0][5].c_str());
	_eSwitchType sSwitchType = (_eSwitchType)atoi(result[0][6].c_str());

	if (pSensor->subType != subType)
		m_sql.UpdateDeviceValue("SubType", subType, szIdx);
	pSensor->subType = subType;

	bool bOn = false;

	bool bIsJSON = false;
	Json::Value root;
	if (pSensor->bIsJSON)
	{
		bool ret = ParseJSon(pSensor->last_json_value, root);
		if (ret)
		{
			bIsJSON = root.isObject();
		}
	}

	_tColor color_old;
	_tColor color_new;
	color_old.mode = ColorModeCustom;
	color_new.mode = ColorModeCustom;
	bool bHaveColorChange = false;
	bool bDoNotUpdateLevel = false;

	if (bIsJSON)
	{
		if (root["value"].isObject() && (!root["value"]["red"].empty() && root["value"]["r"].empty()))
		{
			// Color values are defined in "value" object instead of "color" as expected by domoticz (e.g. Fibaro FGRGBW)
			root["color"] = root["value"];
			root.removeMember("value");
		}

		if (root["color"].isObject() && !root["color"]["red"].empty())
		{
			// The device uses "red", "green"... to specify the color components, default for domoticz would be "r", "g"... (e.g. Fibaro FGRGBW)
			JSonRenameKey(root["color"], "red", "r");
			JSonRenameKey(root["color"], "green", "g");
			JSonRenameKey(root["color"], "blue", "b");
			JSonRenameKey(root["color"], "warmWhite", "w");
			JSonRenameKey(root["color"], "coldWhite", "c");
		}

		if (root["state"].empty() && root["color"].isObject())
		{
			// The on/off state is omitted in the message, so guess it from the color components (e.g. Fibaro FGRGBW)
			int r = root["color"]["r"].asInt();
			int g = root["color"]["g"].asInt();
			int b = root["color"]["b"].asInt();
			int w = root["color"]["w"].asInt();
			int c = root["color"]["c"].asInt();
			if (r == 0 && g == 0 && b == 0 && w == 0 && c == 0)
			{
				root["state"] = "OFF";
			}
			else
				root["state"] = "ON";
		}

		if (!pSensor->state_value_template.empty())
		{
			if (pSensor->state_topic == pSensor->last_topic)
			{
				std::string szValue = GetValueFromTemplate(root, pSensor->state_value_template);
				if (szValue == pSensor->state_on)
					szSwitchCmd = pSensor->payload_on;
				else if (szValue == pSensor->state_off)
					szSwitchCmd = pSensor->payload_off;
				else
					szSwitchCmd = szValue;
			}
		}

		if (!root["state"].empty())
		{
			szSwitchCmd = root["state"].asString();
		}
		else if (!root["value"].empty())
		{
			szSwitchCmd = root["value"].asString();
			if (
				(szSwitchCmd != pSensor->payload_on)
				&& (szSwitchCmd != pSensor->payload_off)
				)
			{
				if (is_number(szSwitchCmd))
				{
					//must be a level
					level = atoi(szSwitchCmd.c_str());

					if (pSensor->bHave_brightness_scale)
						level = (int)round((100.0 / pSensor->brightness_scale) * level);

					if (level == 0)
						szSwitchCmd = "off";
					else if (level == 100)
						szSwitchCmd = "on";
					else
						szSwitchCmd = "Set Level";
				}
			}
		}

		if (!root["brightness"].empty())
		{
			float dLevel = (100.F / pSensor->brightness_scale) * root["brightness"].asInt();
			level = (int)round(dLevel);
			if (
				(szSwitchCmd != pSensor->payload_on)
				&& (szSwitchCmd != pSensor->payload_off))
			{
				if (level == 0)
					szSwitchCmd = pSensor->payload_off;
				else if (level == 100)
					szSwitchCmd = pSensor->payload_on;
				else
					szSwitchCmd = "Set Level";
			}
			else if ((szSwitchCmd == pSensor->payload_on) && (level > 0) && (level < 100))
			{
				szSwitchCmd = "Set Level";
			}
		}
		if (!root["color"].empty())
		{
			Json::Value root_color;
			bool ret = ParseJSon(sColor, root_color);
			if (ret)
			{
				if (root_color.isObject())
				{
					color_old.fromJSON(root_color);
				}
			}

			bool bReceivedColor = false;
			//for some nodes "color" = {}, in this case there could be a brightness only status
			if (!root["color"]["r"].empty())
			{
				bReceivedColor = true;
				color_new.r = root["color"]["r"].asInt();
			}
			if (!root["color"]["g"].empty())
			{
				bReceivedColor = true;
				color_new.g = root["color"]["g"].asInt();
			}
			if (!root["color"]["b"].empty())
			{
				bReceivedColor = true;
				color_new.b = root["color"]["b"].asInt();
			}
			if (!root["color"]["c"].empty())
			{
				bReceivedColor = true;
				color_new.cw = root["color"]["c"].asInt();
			}
			if (!root["color"]["w"].empty())
			{
				bReceivedColor = true;
				color_new.ww = root["color"]["w"].asInt();
			}

			if ((!root["color"]["x"].empty()) && (!root["color"]["y"].empty()))
			{
				bReceivedColor = true;
				// convert xy to rgb
				_tColor::RgbFromXY(root["color"]["x"].asDouble(), root["color"]["y"].asDouble(), color_new.r, color_new.g, color_new.b);
			}
			if ((!root["color"]["h"].empty()) && (!root["color"]["s"].empty()))
			{
				bReceivedColor = true;
				// convert hue/sat to rgb
				float iHue = float(root["color"]["h"].asDouble()) * 360.0F / 65535.0F;
				float iSat = float(root["color"]["s"].asDouble()) / 254.0F;
				int r = 0;
				int g = 0;
				int b = 0;
				hsb2rgb(iHue, iSat, 1.0F, r, g, b, 255);
				color_new.r = (uint8_t)r;
				color_new.g = (uint8_t)g;
				color_new.b = (uint8_t)b;
			}

			if (!root["color_temp"].empty())
			{
				bReceivedColor = true;
				float CT = root["color_temp"].asFloat();
				float iCt = ((float(CT) - pSensor->min_mireds) / (pSensor->max_mireds - pSensor->min_mireds)) * 255.0F;
				_tColor color_CT = _tColor((uint8_t)round(iCt), ColorModeTemp);
				color_new.t = color_CT.t;
				color_new.cw = color_CT.cw;
				color_new.ww = color_CT.ww;
			}

			std::string szColorOld = color_old.toJSONString();
			std::string szColorNew = color_new.toJSONString();
			bHaveColorChange = szColorOld != szColorNew;
			if (bReceivedColor)
				bDoNotUpdateLevel = true;
		}
		else if (!root["color_temp"].empty())
		{
			float CT = root["color_temp"].asFloat();
			float iCt = ((float(CT) - pSensor->min_mireds) / (pSensor->max_mireds - pSensor->min_mireds)) * 255.0F;
			color_new = _tColor((uint8_t)round(iCt), ColorModeTemp);
			std::string szColorOld = color_old.toJSONString();
			std::string szColorNew = color_new.toJSONString();
			bHaveColorChange = szColorOld != szColorNew;
			bDoNotUpdateLevel = true;
		}
	}
	else
	{
		//not json
		if (is_number(szSwitchCmd))
		{
			//must be a level
			level = atoi(szSwitchCmd.c_str());
			if (pSensor->component_type == "binary_sensor" && szSwitchCmd == pSensor->payload_off)
				szSwitchCmd = "off";
			else if (pSensor->component_type == "binary_sensor" && szSwitchCmd == pSensor->payload_on)
				szSwitchCmd = "on";
			else if (pSensor->component_type == "lock" && szSwitchCmd == pSensor->state_unlocked)
				szSwitchCmd = "off";
			else if (pSensor->component_type == "lock" && szSwitchCmd == pSensor->state_locked)
				szSwitchCmd = "on";
			else if (pSensor->component_type == "lock" && !pSensor->value_template.empty())
				// ignore states defined in the value template until value_template support is built in
				return;

			else
			{
				if (level == 0)
					szSwitchCmd = "off";
				else {
					szSwitchCmd = "Set Level";
					if (pSensor->bHave_brightness_scale)
						level = (int)round((100.0 / pSensor->brightness_scale) * level);
					else if (!pSensor->percentage_value_template.empty())
					{
						level = (int)round((100.0 / 255) * level);
					}
				}
			}
		}
		else
		{
			if (pSensor->component_type == "binary_sensor")
			{
				if (szSwitchCmd == pSensor->payload_off)
					szSwitchCmd = "off";
				else if (szSwitchCmd == pSensor->payload_on)
					szSwitchCmd = "on";
			}
			else if (pSensor->component_type == "lock")
			{
				if (szSwitchCmd == pSensor->state_unlocked)
					szSwitchCmd = "off";
				else if (szSwitchCmd == pSensor->state_locked)
					szSwitchCmd = "on";
			}
		}
	}

	if (level > 100)
		level = 100;

	int slevel = atoi(sValue.c_str());
	bool bHaveLevelChange = (slevel != level);


	if (szSwitchCmd == pSensor->payload_on)
		bOn = true;
	else if (szSwitchCmd == pSensor->payload_off)
		bOn = false;
	else
		bOn = (szSwitchCmd == "on") || (szSwitchCmd == "ON") || (szSwitchCmd == "true") || (szSwitchCmd == "Set Level");

	// check if we have a change, if not do not update it
	if (
		(switchType != STYPE_PushOn)
		&& (switchType != STYPE_PushOff)
		)
	{
		if ((!bOn) && (nValue == 0))
		{
			if (!bHaveLevelChange)
			{
				if (!bHaveColorChange)
					return;
			}
		}
		if ((bOn && (nValue != 0)))
		{
			// Check Level
			int slevel = atoi(sValue.c_str());
			if (slevel == level)
			{
				if (!bHaveColorChange)
					return;
			}
		}
		/*
				if (bOn && (level == 0) && (slevel != 0)) {
					level = slevel; //set level to last level (can't have a light that is on with a level of 0)
					szSwitchCmd = "Set Level";
				}
		*/
	}

	sValue = std_format("%d", level);

	if (pSensor->devType != pTypeColorSwitch)
	{
		if (szSwitchCmd == "Set Level")
		{
			nValue = gswitch_sSetLevel;
		}
		else
		{
			nValue = (bOn) ? gswitch_sOn : gswitch_sOff;
		}
	}
	else
	{
		if (
			(szSwitchCmd == "Set Level")
			|| (bOn && (level > 0) && (sValue != pSensor->payload_on))
			)
		{
			nValue = Color_SetBrightnessLevel;
		}
		else
			nValue = (bOn) ? Color_LedOn : Color_LedOff;
	}

	pSensor->nValue = nValue;
	pSensor->sValue = sValue;

	UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue, pSensor->sValue.c_str(),
		szDeviceName);
	if (bHaveColorChange)
		m_sql.UpdateDeviceValue("Color", color_new.toJSONString(), szIdx);
	if (bHaveLevelChange && !bDoNotUpdateLevel)
		m_sql.UpdateDeviceValue("LastLevel", level, szIdx);
}

bool MQTTAutoDiscover::SendSwitchCommand(const std::string& DeviceID, const std::string& DeviceName, int Unit, std::string command, int level, _tColor color, const std::string& user)
{
	if (m_discovered_sensors.find(DeviceID) == m_discovered_sensors.end())
	{
		Log(LOG_ERROR, "Switch not found!? (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
		return false;
	}
	_tMQTTASensor* pSensor = &m_discovered_sensors[DeviceID];

	if (pSensor->component_type == "cover")
		return SendCoverCommand(pSensor, DeviceName, command, level, user);

	if (
		(pSensor->component_type != "switch")
		&& (pSensor->component_type != "light")
		&& (pSensor->component_type != "climate")
		&& (pSensor->component_type != "select")
		&& (pSensor->component_type != "lock")
		&& (pSensor->component_type != "button")
		&& (pSensor->component_type != "fan")
		)
	{
		Log(LOG_ERROR, "sending switch commands for switch type '%s' is not supported (yet...) (%s/%s)", pSensor->component_type.c_str(), DeviceID.c_str(), DeviceName.c_str());
		return false;
	}

	std::string szSendValue;
	std::string command_topic = pSensor->command_topic;

	if (
		(pSensor->component_type != "climate")
		&& (pSensor->component_type != "select")
		&& (pSensor->component_type != "lock")
		&& (pSensor->component_type != "fan")
		)
	{
		if (command == "On")
			szSendValue = pSensor->payload_on;
		else if (command == "Off")
			szSendValue = pSensor->payload_off;
		else if (command == "Stop")
			szSendValue = pSensor->payload_stop;
		else if (command == "Set Level")
		{
			if (level == 0)
			{
				command = "Off";
				szSendValue = pSensor->payload_off;
			}
			else if (level > 100)
				level = 100;
		}
		else if ((command == "Set Color") && (pSensor->component_type == "light"))
		{
			// That's valid
		}
		else
		{
			Log(LOG_ERROR, "Switch command not supported (%s - %s/%s)", command.c_str(), DeviceID.c_str(), DeviceName.c_str());
			return false;
		}
	}

	if (pSensor->component_type == "button")
	{
		szSendValue = pSensor->payload_press;
	}
	else if (pSensor->component_type == "lock")
	{
		if (command == "On")
			szSendValue = pSensor->payload_lock;
		else if (command == "Off")
			szSendValue = pSensor->payload_unlock;
	}
	else if (pSensor->component_type == "light")
	{
		Json::Value root;

		if ((command == "On") || (command == "Off"))
		{
			if (!pSensor->brightness_value_template.empty())
			{
				SendMessage(pSensor->command_topic, szSendValue);
				return true;
			}
			else
			{
				if (szSendValue == "true")
					root["state"] = true;
				else if (szSendValue == "false")
					root["state"] = false;
				else
				{
					root["state"] = szSendValue;
					if (is_number(szSendValue))
						root["value"] = szSendValue;	// Required for e.g. FGRGBW color dimmer
				}
			}
		}
		else if (command == "Set Level")
		{
			//root["state"] = pSensor->payload_on;
			int slevel = level;
			if (pSensor->bHave_brightness_scale)
				slevel = (int)round((pSensor->brightness_scale / 100.F) * level);

			if (!pSensor->brightness_value_template.empty())
			{
				std::string szKey = GetValueTemplateKey(pSensor->brightness_value_template);

				if (!szKey.empty() && szKey == "value")
					// just send the plain percentage as HA does for value in template
					root = slevel;
				else if (!szKey.empty())
				{
					// in case another key was requested
					root[szKey] = slevel;
					if (pSensor->command_topic == pSensor->brightness_command_topic)
					{
						std::string szOnPayload = pSensor->payload_on;
						CleanValueTemplate(szOnPayload);
						std::vector<std::string> strarray;
						StringSplit(szOnPayload, ":", strarray);
						if (strarray.size() == 2)
						{
							root[strarray[0]] = strarray[1];
						}
					}
				}
				else
				{
					Log(LOG_ERROR, "light device unhandled brightness_value_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
					return false;
				}
			}
			else
			{
				root["brightness"] = slevel;
				root["state"] = (slevel > 0) ? "ON" : "OFF";
			}

			szSendValue = JSonToRawString(root);
			std::string szTopic = pSensor->command_topic;
			if (!pSensor->brightness_command_topic.empty())
				szTopic = pSensor->brightness_command_topic;
			else if (!pSensor->set_position_topic.empty())
				szTopic = pSensor->set_position_topic;
			SendMessage(szTopic, szSendValue);
			return true;
		}
		else if (command == "Set Color")
		{
			root["state"] = pSensor->payload_on;

			bool bCouldUseBrightness = false;

			if (
				(color.mode == ColorModeRGB)
				|| (color.mode == ColorModeCustom)
				)
			{
				if (pSensor->supported_color_modes.find("xy") != pSensor->supported_color_modes.end())
				{
					double Y, x, y;
					_tColor::XYFromRGB(color.r, color.g, color.b, x, y, Y);
					root["color"]["x"] = x;
					root["color"]["y"] = y;
					bCouldUseBrightness = true;
				}
				else if (pSensor->supported_color_modes.find("hs") != pSensor->supported_color_modes.end())
				{
					float hsb[3];
					rgb2hsb(color.r, color.g, color.b, hsb);

					if (!pSensor->hs_value_template.empty())
					{
						std::string szKey = GetValueTemplateKey(pSensor->hs_value_template);
						if (!szKey.empty())
						{
							std::string hsbColor = std_format("%.0f,%.0f,%.0f", hsb[0] * 360.0F, hsb[1] * 100.0F, hsb[2] * pSensor->brightness_scale);
							root[szKey] = hsbColor;
						}
						else
						{
							Log(LOG_ERROR, "Color device unhandled hs_value_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
							return false;
						}
					}
					else
					{
						root["color"]["h"] = hsb[0] * 360.0F;
						root["color"]["s"] = hsb[1] * 100.0F;
						bCouldUseBrightness = true;
					}
				}
				else if (
					(pSensor->bColor_mode)
					|| (pSensor->supported_color_modes.find("rgb") != pSensor->supported_color_modes.end())
					|| (pSensor->supported_color_modes.find("rgbw") != pSensor->supported_color_modes.end())
					|| (pSensor->supported_color_modes.find("rgbww") != pSensor->supported_color_modes.end())
					|| (pSensor->supported_color_modes.find("rgbcct") != pSensor->supported_color_modes.end())
					)
				{
					root["color"]["r"] = color.r;
					root["color"]["g"] = color.g;
					root["color"]["b"] = color.b;
					if ((pSensor->bBrightness) && (level > 1))
						bCouldUseBrightness = true;
				}
				if (
					(pSensor->supported_color_modes.find("rgbw") != pSensor->supported_color_modes.end())
					|| (pSensor->supported_color_modes.find("rgbww") != pSensor->supported_color_modes.end())
					)
					root["color"]["c"] = color.cw;
				if (pSensor->supported_color_modes.find("rgbww") != pSensor->supported_color_modes.end())
					root["color"]["w"] = color.ww;

				// Check if the rgb_command_template suggests to use "red", "green"... instead of the default "r", "g"... (e.g. Fibaro FGRGBW)
				if (!pSensor->rgb_command_template.empty() && pSensor->rgb_command_template.find("red: red") != std::string::npos)
				{
					// For the Fibaro FGRGBW dimmer:
					//  "rgb_command_template": "{{ {'red': red, 'green': green, 'blue': blue}|to_json }}",  
					//	"rgb_value_template": "{{ value_json.value.red }},{{ value_json.value.green }},{{ value_json.value.blue }}",
					//  -> variables are red, green and blue, but white is missing entirely, so the template can't be used as is.
					Json::Value colorDef;
					root["value"] = colorDef;
					root["value"]["red"] = root["color"]["r"];
					root["value"]["green"] = root["color"]["g"];
					root["value"]["blue"] = root["color"]["b"];
					root["value"]["warmWhite"] = root["color"]["c"];		// In Domoticz cw is used for RGB_W dimmers, but Zwavejs requires warmWhite
					root.removeMember("color");
				}
			}
			else if (
				(color.mode == ColorModeTemp)
				|| (color.mode == ColorModeCustom)
				)
			{
				if (pSensor->supported_color_modes.find("color_temp") != pSensor->supported_color_modes.end())
				{
					//color.cw color.ww t
					float iCt = pSensor->min_mireds + ((static_cast<float>(pSensor->max_mireds - pSensor->min_mireds) / 255.0F) * color.t);
					int iCT = (int)round(iCt);
					if (!pSensor->color_temp_value_template.empty())
					{
						std::string szKey = GetValueTemplateKey(pSensor->color_temp_value_template);
						if (!szKey.empty())
							root[szKey] = iCT;
						else
						{
							Log(LOG_ERROR, "Color device unhandled color_temp_value_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
							return false;
						}
					}
					else
						root["color_temp"] = iCT;
				}
				bCouldUseBrightness = true;
			}
			if (!pSensor->rgb_command_topic.empty())
				command_topic = pSensor->rgb_command_topic;

			if ((bCouldUseBrightness) && (pSensor->bBrightness))
			{
				int slevel = (int)round((pSensor->brightness_scale / 100.0F) * level);

				if (!pSensor->brightness_value_template.empty())
				{
					std::string szKey = GetValueTemplateKey(pSensor->brightness_value_template);
					if (!szKey.empty())
					{
						root[szKey] = slevel;
					}
					else
					{
						Log(LOG_ERROR, "Color device unhandled brightness_value_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
						return false;
					}
				}
				else
					root["brightness"] = slevel;
			}
		}
		else
		{
			Log(LOG_ERROR, "Switch command not supported (%s - %s/%s)", command.c_str(), DeviceID.c_str(), DeviceName.c_str());
			return false;
		}

		szSendValue = JSonToRawString(root);
	}
	else if (
		(pSensor->component_type == "climate")
		|| (pSensor->component_type == "select")
		)
	{
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type=%d) AND (SubType=%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(),
			pTypeGeneralSwitch, sSwitchGeneralSwitch, Unit);
		if (result.empty())
			return false;

		std::string szIdx = result[0][0];

		std::map<std::string, std::string> deviceOptions = m_sql.GetDeviceOptions(szIdx);

		int iLevel = level / 10;

		std::string newState;
		std::string state_template;
		std::string szCommandTopic;

		if (pSensor->component_type == "climate")
		{
			if ((!pSensor->climate_modes.empty()) && (Unit == CLIMATE_MODE_UNIT))
			{
				if (iLevel < (int)pSensor->climate_modes.size())
					newState = pSensor->climate_modes.at(iLevel);
				szCommandTopic = pSensor->mode_command_topic;
				if (!pSensor->mode_command_template.empty())
					state_template = pSensor->mode_command_template;
				else if (!pSensor->mode_state_template.empty())
					state_template = pSensor->mode_state_template;
			}
			else if ((!pSensor->preset_modes.empty()) && (Unit == CLIMATE_PRESET_UNIT))
			{
				if (iLevel < (int)pSensor->preset_modes.size())
					newState = pSensor->preset_modes.at(iLevel);
				szCommandTopic = pSensor->preset_mode_command_topic;
				if (!pSensor->preset_mode_value_template.empty())
					state_template = pSensor->preset_mode_value_template;
			}
		}
		else if (
			(pSensor->component_type == "select")
			&& (iLevel < (int)pSensor->select_options.size())
			)
		{
			newState = pSensor->select_options.at(iLevel);
			szCommandTopic = pSensor->command_topic;
		}
		if (newState.empty())
		{
			Log(LOG_ERROR, "%s device invalid state requested (%s/%s)", pSensor->component_type.c_str(), DeviceID.c_str(), DeviceName.c_str());
			return false;
		}

		if (!szCommandTopic.empty())
		{
			if (!state_template.empty())
			{
				Json::Value root;
				if (SetValueWithTemplate(root, state_template, newState))
				{
					szSendValue = JSonToRawString(root);
				}
				else
				{
					Log(LOG_ERROR, "%s device unhandled mode_state_template (%s/%s)", pSensor->component_type.c_str(), DeviceID.c_str(), DeviceName.c_str());
					return false;
				}
			}
			else
				szSendValue = newState;
			SendMessage(szCommandTopic, szSendValue);
			return true;
		}
		else
		{
			Log(LOG_ERROR, "%s device does not have mode_command_topic (%s/%s)", pSensor->component_type.c_str(), DeviceID.c_str(), DeviceName.c_str());
			return false;
		}
	}
	else if (pSensor->component_type == "fan")
	{
		if (Unit == 1)
		{
			if (command == "On")
				szSendValue = "50";
			else if (command == "Off")
				szSendValue = "0";
			else if (command == "Set Level")
			{
				if (level > 100)
					level = 100;
				int slevel = (int)round((255 / 100.0F) * level);
				szSendValue = std::to_string(slevel);
				command_topic = pSensor->percentage_command_topic;
			}
		}
		else if ((!pSensor->preset_modes.empty()) && (Unit == FAN_PRESET_UNIT))
		{
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type=%d) AND (SubType=%d) AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(),
				pTypeGeneralSwitch, sSwitchGeneralSwitch, Unit);
			if (result.empty())
				return false;

			std::string szIdx = result[0][0];

			std::map<std::string, std::string> deviceOptions = m_sql.GetDeviceOptions(szIdx);

			int iLevel = level / 10;

			std::string newState;
			std::string state_template;

			if (iLevel < (int)pSensor->preset_modes.size())
				newState = pSensor->preset_modes.at(iLevel);
			command_topic = pSensor->preset_mode_command_topic;
			if (!pSensor->preset_mode_value_template.empty())
				state_template = pSensor->preset_mode_value_template;

			if (!state_template.empty())
			{
				Json::Value root;
				if (SetValueWithTemplate(root, state_template, newState))
				{
					szSendValue = JSonToRawString(root);
				}
				else
				{
					Log(LOG_ERROR, "%s device unhandled mode_state_template (%s/%s)", pSensor->component_type.c_str(), DeviceID.c_str(), DeviceName.c_str());
					return false;
				}
			}
			else
				szSendValue = newState;
		}
	}

	SendMessage(command_topic, szSendValue);
	return true;
}

void MQTTAutoDiscover::UpdateBlindPosition(_tMQTTASensor* pSensor)
{
	pSensor->devUnit = 1;
	pSensor->devType = pTypeGeneralSwitch;
	pSensor->subType = sSwitchGeneralSwitch;

	int iUsed = (pSensor->bEnabled_by_default) ? 1 : 0;
	std::string szSwitchCmd = pSensor->last_value;
	int level = 0;
	int switchType = STYPE_Blinds;
	std::string szSensorName = pSensor->name;

	if (
		(!pSensor->position_topic.empty())
		|| (!pSensor->set_position_topic.empty())
		)
	{
		if (pSensor->payload_stop.empty())
			switchType = STYPE_BlindsPercentage;
		else
			switchType = STYPE_BlindsPercentageWithStop;
	}

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID, Name, nValue, sValue, SubType, SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit);
	if (result.empty())
	{
		// New switch, add it to the system
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
			"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, %d, '%q')",
			m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, szSensorName.c_str(), iUsed, 0, "0");
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,SubType, SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit);
	}
	if (result.empty())
		return; // should not happen!

	if (pSensor->last_value.empty())
		return;

	std::string szIdx = result[0][0];
	uint64_t DevRowIdx = std::stoull(szIdx);
	std::string szDeviceName = result[0][1];
	int nValue = atoi(result[0][2].c_str());
	std::string sValue = result[0][3];
	int subType = atoi(result[0][4].c_str());
	_eSwitchType sSwitchType = (_eSwitchType)atoi(result[0][5].c_str());

	if (pSensor->subType != subType)
		m_sql.UpdateDeviceValue("SubType", subType, szIdx);
	pSensor->subType = subType;

	bool bIsJSON = false;
	Json::Value root;
	bool ret = ParseJSon(pSensor->last_value, root);
	if (ret)
	{
		bIsJSON = root.isObject();
	}

	bool bDoNotUpdateLevel = false;

	if (bIsJSON)
	{
		if (!root["position"].empty())
		{
			level = root["position"].asInt();
			szSwitchCmd = "Set Level";
		}
		else if (!root["value"].empty())
		{
			//check against a possible value template ?
			szSwitchCmd = root["value"].asString();
		}
		else if (!root["state"].empty())
		{
			szSwitchCmd = root["state"].asString();
		}
		else
		{
#ifdef _DEBUG
			_log.Debug(DEBUG_NORM, "ERROR: Cover, unknown how to interpretate position/state", pSensor->unique_id.c_str(), szDeviceName.c_str());
#endif
			return;
		}
		if (is_number(szSwitchCmd))
		{
			//must be a position
			level = atoi(szSwitchCmd.c_str());
			szSwitchCmd = "Set Level";
		}
	}
	else
	{
		//not json
		szSwitchCmd = pSensor->last_value;
		if (is_number(szSwitchCmd))
		{
			//must be a level
			level = atoi(szSwitchCmd.c_str());
			szSwitchCmd = "Set Level";
		}
	}

	if (level == pSensor->position_open)
	{
		szSwitchCmd = "Open";
	}
	else if (level == pSensor->position_closed)
	{
		szSwitchCmd = "Close";
	}

	// Always recalculate to make level relative to 100
	level = (int)round((100.0 / (pSensor->position_open - pSensor->position_closed)) * level);

	if (level > 100)
		level = 100;

	int lastlevel = atoi(sValue.c_str());
	bool bHaveLevelChange = (lastlevel != level);

	std::string new_sValue = std_format("%d", level);
	int new_nValue = 0;

	if (szSwitchCmd == "Set Level")
	{
		new_nValue = gswitch_sSetLevel;
	}
	else
	{
		new_nValue = (szSwitchCmd == "Open") ? gswitch_sOpen : gswitch_sClose;
	}

	if (
		(nValue != new_nValue)
		|| (sValue != new_sValue)
		|| (bHaveLevelChange == true)
		)
	{
		pSensor->nValue = new_nValue;
		pSensor->sValue = new_sValue;

		UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue, pSensor->sValue.c_str(),
			szDeviceName);

		if (bHaveLevelChange)
			m_sql.UpdateDeviceValue("LastLevel", level, szIdx);
	}
}

bool MQTTAutoDiscover::SendCoverCommand(_tMQTTASensor* pSensor, const std::string& DeviceName, std::string command, int level, const std::string& user)
{
	Json::Value root;
	std::string szValue;

	if (command == "Open")
	{
		szValue = pSensor->payload_open;
		level = 100;
		if (!pSensor->set_position_topic.empty())
			command = "Set Level";
	}
	else if (command == "Close")
	{
		szValue = pSensor->payload_close;
		level = 0;
		if (!pSensor->set_position_topic.empty())
			command = "Set Level";
	}
	else if (command == "Stop")
	{
		level = -1;
		szValue = pSensor->payload_stop;
	}

	if (command == "Set Level")
	{
		szValue = std::to_string(level);
		if (!pSensor->set_position_topic.empty())
		{
			std::string szSendValue;
			int iValue = (int)round(((pSensor->position_open - pSensor->position_closed) / 100.0F) * float(level));
			if (pSensor->set_position_template.empty())
			{
				szSendValue = std::to_string(iValue);
			}
			else
			{
				std::string szKey = GetValueTemplateKey(pSensor->set_position_template);
				if (szKey.empty())
				{
					Log(LOG_ERROR, "Cover device unhandled set_position_template (%s/%s)", pSensor->unique_id.c_str(), DeviceName.c_str());
					return false;
				}
				if (is_number(szValue))
				{
					root[szKey] = (int)iValue;
				}
				else
					root[szKey] = szValue;
				szSendValue = JSonToRawString(root);
			}
			SendMessage(pSensor->set_position_topic, szSendValue);
		}
		else {
			Log(LOG_ERROR, "Cover device does not have set_position_topic (%s/%s)", pSensor->unique_id.c_str(), DeviceName.c_str());
			return false;
		}
	}
	else {
		if (!pSensor->command_topic.empty())
		{
			SendMessage(pSensor->command_topic, szValue);
		}
		else {
			Log(LOG_ERROR, "Cover device does not have command_topic (%s/%s)", pSensor->unique_id.c_str(), DeviceName.c_str());
			return false;
		}
	}
	auto result = m_sql.safe_query("SELECT ID,Name,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, pSensor->unique_id.c_str());
	if (
		(!result.empty())
		&& (level != -1)
		)
	{
		int nValue = gswitch_sSetLevel;
		if (level == 0)
			nValue = gswitch_sClose;
		else if (level == 100)
			nValue = gswitch_sOpen;
		//nValue = gswitch_sSetLevel;
		m_sql.safe_query(
			"UPDATE DeviceStatus SET LastLevel=%d, LastUpdate='%s' WHERE (ID = %s)", level, TimeToString(nullptr, TF_DateTime).c_str(), result[0][0].c_str());
		UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel,
			nValue, std::to_string(level).c_str(), result[0][1], true, user);

	}
	return true;
}

bool MQTTAutoDiscover::SetSetpoint(const std::string& DeviceID, float Temp)
{
	if (m_discovered_sensors.find(DeviceID) == m_discovered_sensors.end())
	{
		return false;
	}
	_tMQTTASensor* pSensor = &m_discovered_sensors[DeviceID];
	if (pSensor->component_type != "climate")
	{
		return false;
	}

	Json::Value root;
	std::string szSendValue;
	if (!pSensor->temperature_command_template.empty())
	{
		std::string szKey = GetValueTemplateKey(pSensor->temperature_command_template);
		if (!szKey.empty())
			root[szKey] = Temp;
		else
		{
			Log(LOG_ERROR, "Climate device unhandled temperature_command_template (%s/%s)", DeviceID.c_str(), pSensor->name.c_str());
			return false;
		}
		szSendValue = JSonToRawString(root);
	}
	else
		szSendValue = std_format("%.1f", Temp);

	std::string szTopic = pSensor->state_topic;
	if (!pSensor->temperature_command_topic.empty())
		szTopic = pSensor->temperature_command_topic;
	SendMessage(szTopic, szSendValue);

	//Because thermostats could be battery operated and not listening 24/7
	//we force a value update internally in Domoticz so the user sees the just set SetPoint

	pSensor->sValue = std_format("%.2f", Temp);
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
		pSensor->unique_id.c_str(), 1, pTypeSetpoint, sTypeSetpoint);
	if (result.empty())
		return false; //?? That's impossible
	// Update
	UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), 1, pTypeSetpoint, sTypeSetpoint, pSensor->SignalLevel, pSensor->BatteryLevel,
		pSensor->nValue, pSensor->sValue.c_str(),
		result[0][0]);

	return true;
}

void MQTTAutoDiscover::GetConfig(Json::Value& root)
{
	int ii = 0;
	for (auto& itt : m_discovered_sensors)
	{
		if (itt.second.component_type == "number")
		{
			root["result"][ii]["idx"] = itt.first;

			_tMQTTADevice* pDevice = &m_discovered_devices[itt.second.device_identifiers];
			root["result"][ii]["dev_name"] = (pDevice != nullptr) ? pDevice->name : "?";
			root["result"][ii]["name"] = itt.second.name;
			root["result"][ii]["value"] = itt.second.last_value;
			root["result"][ii]["unit"] = itt.second.unit_of_measurement;
			root["result"][ii]["min"] = std_format("%g", itt.second.number_min);
			root["result"][ii]["max"] = std_format("%g", itt.second.number_max);
			root["result"][ii]["step"] = std_format("%g", itt.second.number_step);
			ii++;
		}
	}
}

bool MQTTAutoDiscover::UpdateNumber(const std::string& idx, const std::string& sValue)
{
	for (auto& itt : m_discovered_sensors)
	{
		if (itt.first == idx)
		{
			double dValue = atof(sValue.c_str());
			if (dValue < itt.second.number_min || dValue > itt.second.number_max)
				return false;
			SendMessage(itt.second.command_topic, sValue);
			/*
						std::string szSendValue = std::to_string(nValue);
						if (!itt.second.value_template.empty())
						{
							std::string szKey = GetValueTemplateKey(itt.second.value_template);
							if (!szKey.empty())
							{
								Json::Value root;
								root[szKey] = std::to_string(nValue);
								szSendValue = JSonToRawString(root);
							}
							else
							{
								Log(LOG_ERROR, "number device unhandled value_value_template (%s/%s)", itt.second.unique_id.c_str(), itt.second.name.c_str());
								return false;
							}
						}
						else
							szSendValue = std::to_string(nValue);
						SendMessage(itt.second.command_topic, szSendValue);
			*/
			return true;
		}
	}
	return false;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_MQTTAD_GetConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(std::stoi(hwid));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_MQTTAutoDiscovery)
				return;

			root["status"] = "OK";
			root["title"] = "GetMQTTConfig";

			MQTTAutoDiscover* pMQTT = reinterpret_cast<MQTTAutoDiscover*>(pHardware);
			pMQTT->GetConfig(root);
		}

		void CWebServer::Cmd_MQTTAD_UpdateNumber(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string devid = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "name")));
			std::string value = request::findValue(&req, "value");
			if (
				hwid.empty()
				|| devid.empty()
				|| value.empty()
				)
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(std::stoi(hwid));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_MQTTAutoDiscovery)
				return;

			MQTTAutoDiscover* pMQTT = reinterpret_cast<MQTTAutoDiscover*>(pHardware);
			try
			{
				if (pMQTT->UpdateNumber(devid, value))
				{
					root["title"] = "GetMQTTUpdateNumber";
					root["status"] = "OK";
				}
			}
			catch (const std::exception&)
			{

			}
		}
	} // namespace server
} // namespace http
