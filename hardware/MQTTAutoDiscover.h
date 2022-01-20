#pragma once

#include "MQTT.h"

class MQTTAutoDiscover : public MQTT
{
	struct _tMQTTASensor
	{
		std::string config;

		bool bEnabled_by_default = true;

		std::string component_type;
		std::string object_id;
		std::string unique_id;
		std::string device_identifiers;
		std::string name;
		std::string device_class;

		std::string availability_topic;
		std::string state_topic;
		std::string command_topic;
		std::string position_topic;
		std::string set_position_topic;
		std::string brightness_command_topic;
		std::string brightness_state_topic;
		std::string rgb_command_topic;
		std::string rgb_state_topic;

		std::string unit_of_measurement;

		std::string state_value_template;
		std::string value_template;
		std::string position_template;
		std::string set_position_template;
		std::string brightness_value_template;
		std::string rgb_value_template;
		std::string rgb_command_template;

		std::string icon;

		std::string payload_on = "ON";
		std::string payload_off = "OFF";
		std::string payload_open = "OPEN";
		std::string payload_close = "CLOSE";
		std::string payload_stop = "STOP";
		int position_open = 100;
		int position_closed = 0;

		std::string payload_available;
		std::string payload_not_available;
		std::string state_on;
		std::string state_off;

		bool bBrightness = false;
		float brightness_scale = 254.0F;

		bool bColor_mode = false;
		std::map<std::string, uint8_t> supported_color_modes;
		std::string color_temp_value_template = "color_temp";
		std::string hs_value_template;

		int min_mireds = 153;
		int max_mireds = 500;

		//Select
		std::vector<std::string> select_options;

		//Climate
		std::string mode_command_topic;
		std::string mode_state_topic;
		std::string mode_state_template;
		std::vector<std::string> climate_modes;
		std::string temperature_command_topic;
		std::string temperature_command_template;
		std::string temperature_state_topic;
		std::string temperature_state_template;
		std::string temperature_unit = "C";
		std::string current_temperature_topic;
		std::string current_temperature_template;

		//Lock
		std::string payload_lock = "LOCK";
		std::string payload_unlock = "UNLOCK";
		std::string state_locked = "LOCKED";
		std::string state_unlocked = "UNLOCKED";

		int qos = 0;

		std::map<std::string, std::string> keys;

		bool bOnline = false;
		time_t last_received = 0;
		std::string last_value;
		uint8_t devType = 0;
		uint8_t subType = 0;
		uint8_t devUnit = 1;
		int nValue = 0;
		std::string sValue;
		std::string szOptions;
		int SignalLevel = 12;
		int BatteryLevel = 255;
	};

	struct _tMQTTADevice
	{
		bool bSubscribed = false;
		std::string identifiers;
		std::string name;
		std::string sw_version;
		std::string model;
		std::string manufacturer;

		std::map<std::string, std::string> keys;

		std::map<std::string, bool> sensor_ids;
	};

public:
	MQTTAutoDiscover(int ID, const std::string &Name, const std::string &IPAddress, unsigned short usIPPort, const std::string &Username, const std::string &Password,
		      const std::string &CAfilenameExtra, int TLS_Version);
	~MQTTAutoDiscover() override = default;

	uint64_t UpdateValueInt(int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true);
	bool SendSwitchCommand(const std::string& DeviceID, const std::string& DeviceName, int Unit, std::string command, int level, _tColor color);
	bool SetSetpoint(const std::string& DeviceID, float Temp);

public:
	void on_message(const struct mosquitto_message *message) override;
	void on_connect(int rc) override;
	void on_disconnect(int rc) override;

private:
	void InsertUpdateSwitch(_tMQTTASensor* pSensor);
	void CleanValueTemplate(std::string& szValueTemplate);
	std::string GetValueTemplateKey(const std::string& szValueTemplate);
	std::string GetValueFromTemplate(Json::Value root, std::string szValueTemplate);
	bool SetValueWithTemplate(Json::Value& root, std::string szValueTemplate, std::string szValue);
	void GuessSensorTypeValue(const _tMQTTASensor* pSensor, uint8_t& devType, uint8_t& subType, std::string& szOptions, int& nValue, std::string& sValue);
	void ApplySignalLevelDevice(const _tMQTTASensor* pSensor);

	void on_auto_discovery_message(const struct mosquitto_message* message);
	void handle_auto_discovery_sensor_message(const struct mosquitto_message* message);

	void handle_auto_discovery_availability(_tMQTTASensor* pSensor, const std::string& payload, const struct mosquitto_message* message);
	void handle_auto_discovery_sensor(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_switch(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_light(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_binary_sensor(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_camera(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_cover(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_climate(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_select(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_scene(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_lock(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_battery(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	_tMQTTASensor* get_auto_discovery_sensor_unit(const _tMQTTASensor* pSensor, const std::string& szMeasurementUnit);
	_tMQTTASensor* get_auto_discovery_sensor_unit(const _tMQTTASensor* pSensor, const uint8_t devType, const int subType = -1, const int devUnit = -1);
private:
	std::string m_TopicDiscoveryPrefix;

	std::map<std::string, _tMQTTADevice> m_discovered_devices;
	std::map<std::string, _tMQTTASensor> m_discovered_sensors;
};
