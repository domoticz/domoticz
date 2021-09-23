#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "MySensorsBase.h"
#include "../main/mosquitto_helper.h"

class MQTT : public MySensorsBase, mosqdz::mosquittodz
{
	struct _tMQTTASensor
	{
		std::string config;

		std::string component_type;
		std::string object_id;
		std::string unique_id;
		std::string device_identifiers;
		std::string name;

		std::string availability_topic;
		std::string state_topic;
		std::string command_topic;
		std::string position_topic;
		std::string set_position_topic;
		std::string brightness_command_topic;
		std::string brightness_state_topic;

		std::string unit_of_measurement;

		std::string state_value_template;
		std::string value_template;
		std::string position_template;
		std::string set_position_template;
		std::string brightness_value_template;

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
		std::string supported_color_modes = "rgb";

		int min_mireds = 154;
		int max_mireds = 500;

		//Climate
		std::string mode_command_topic;
		std::string mode_state_topic;
		std::string mode_state_template;
		std::vector<std::string> climate_modes;
		std::string temperature_command_topic;
		std::string temperature_state_topic;
		std::string temperature_state_template;
		std::string temperature_unit = "C";
		std::string current_temperature_topic;
		std::string current_temperature_template;

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
	MQTT(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilenameExtra, int TLS_Version,
	     int PublishScheme, const std::string &MQTTClientID, bool PreventLoop);
	~MQTT() override;
	bool isConnected()
	{
		return m_IsConnected;
	};

	void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	void on_message(const struct mosquitto_message *message) override;
	void on_subscribe(int mid, int qos_count, const int *granted_qos) override;

	void on_log(int level, const char *str) override;
	void on_error() override;

	void on_auto_discovery_message(const struct mosquitto_message *message);
	void handle_auto_discovery_sensor_message(const struct mosquitto_message *message);

	void handle_auto_discovery_availability(_tMQTTASensor *pSensor, const std::string &payload, const struct mosquitto_message* message);
	void handle_auto_discovery_sensor(_tMQTTASensor *pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_switch(_tMQTTASensor *pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_light(_tMQTTASensor *pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_binary_sensor(_tMQTTASensor *pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_camera(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_cover(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_climate(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	void handle_auto_discovery_scene(_tMQTTASensor* pSensor, const struct mosquitto_message* message);
	_tMQTTASensor* get_auto_discovery_sensor_unit(const _tMQTTASensor* pSensor, const std::string& szMeasurementUnit);
	_tMQTTASensor* get_auto_discovery_sensor_unit(const _tMQTTASensor* pSensor, const uint8_t devType, const int subType = -1, const int devUnit = -1);

	uint64_t UpdateValueInt(int HardwareID, const char *ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
				const char *sValue, std::string &devname, bool bUseOnOffAction = true);
	bool SendSwitchCommand(const std::string &DeviceID, const std::string &DeviceName, int Unit, std::string command, int level, _tColor color);
	bool SetSetpoint(const std::string &DeviceID, const float Temp);

	void SendMessage(const std::string &Topic, const std::string &Message);

	bool m_bDoReconnect;
	bool m_IsConnected;

      public:
	// signals
	boost::signals2::signal<void()> sDisconnected;

      protected:
	bool StartHardware() override;
	bool StopHardware() override;
	enum _ePublishTopics
	{
		PT_none = 0x00,
		PT_out = 0x01,	      // publish on domoticz/out
		PT_floor_room = 0x02, // publish on domoticz/<floor>/<room>
		PT_floor_room_and_out = PT_out | PT_floor_room,
		PT_device_idx = 0x04,  // publish on domoticz/out/idx
		PT_device_name = 0x08, // publish on domoticz/out/name
	};
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	int m_TLS_Version;
	std::string m_TopicIn;
	std::string m_TopicOut;
	std::string m_TopicDiscoveryPrefix;

      private:
	bool ConnectInt();
	bool ConnectIntEx();
	void SendDeviceInfo(int HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void SendSceneInfo(uint64_t SceneIdx, const std::string &SceneName);
	void StopMQTT();
	void Do_Work();
	void SubscribeTopic(const std::string &szTopic, const int qos = 0);
	void InsertUpdateSwitch(_tMQTTASensor* pSensor);
	void CleanValueTemplate(std::string &szValueTemplate);
	std::string GetValueTemplateKey(const std::string &szValueTemplate);
	void GuessSensorTypeValue(const _tMQTTASensor* pSensor, uint8_t& devType, uint8_t& subType, std::string& szOptions, int& nValue, std::string& sValue);
	virtual void SendHeartbeat();
	void WriteInt(const std::string &sendStr) override;
	std::shared_ptr<std::thread> m_thread;
	boost::signals2::connection m_sDeviceReceivedConnection;
	boost::signals2::connection m_sSwitchSceneConnection;
	_ePublishTopics m_publish_scheme;
	bool m_bPreventLoop = false;
	bool m_bRetain = false;
	uint64_t m_LastUpdatedDeviceRowIdx = 0;
	uint64_t m_LastUpdatedSceneRowIdx = 0;

	// Auto Discovery
	std::map<std::string, _tMQTTADevice> m_discovered_devices;
	std::map<std::string, _tMQTTASensor> m_discovered_sensors;
	std::map<std::string, bool> m_subscribed_topics;
};
