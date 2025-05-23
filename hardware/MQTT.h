#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "MySensorsBase.h"
#include "../main/mosquitto_helper.h"

class MQTT : public MySensorsBase, mosqdz::mosquittodz
{
	friend class MQTTAutoDiscover;

public:
	MQTT();
	MQTT(int ID, const std::string& IPAddress, unsigned short usIPPort, const std::string& Username, const std::string& Password, const std::string& CAfilenameExtra, int TLS_Version,
		int PublishScheme, const std::string& MQTTClientID, bool PreventLoop);
	~MQTT() override;
	bool isConnected()
	{
		return m_IsConnected;
	};
	bool isStarted()
	{
		return m_thread != nullptr;
	};

	void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	void on_message(const struct mosquitto_message* message) override;
	void on_subscribe(int mid, int qos_count, const int* granted_qos) override;
	virtual void on_going_down();

	void on_log(int level, const char* str) override;
	void on_error() override;

	void SendMessage(const std::string& Topic, const std::string& Message);
	void SendMessageEx(const std::string& Topic, const std::string& Message, int qos = 0, bool retain = false);

	bool ReconnectNow();

	bool m_bDoReconnect = false;
	bool m_IsConnected = false;

	void ReloadSharedDevices();

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
	unsigned short m_usIPPort = 1883;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	int m_TLS_Version = 0;
	std::string m_TopicIn;
	std::string m_TopicOut;
	bool m_RetainedMode = false;
private:
	bool ConnectInt();
	bool ConnectIntEx();
	void SendDeviceInfo(int HwdID, uint64_t DeviceRowIdx, const std::string& DeviceName, const unsigned char* pRXCommand);
	void SendSceneInfo(uint64_t SceneIdx, const std::string& SceneName);
	void StopMQTT();
	void Do_Work();
	void SubscribeTopic(const std::string& szTopic, int qos = -1);
	virtual void SendHeartbeat();
	void WriteInt(const std::string& sendStr) override;
	std::shared_ptr<std::thread> m_thread;
	boost::signals2::connection m_sDeviceReceivedConnection;
	boost::signals2::connection m_sSwitchSceneConnection;
	_ePublishTopics m_publish_scheme;
	bool m_bPreventLoop = false;
	bool m_bRetain = false;
	uint64_t m_LastUpdatedDeviceRowIdx = 0;
	uint64_t m_LastUpdatedSceneRowIdx = 0;
	std::mutex m_mutex;
	std::map<uint64_t, bool> m_shared_devices;
	std::map<std::string, bool> m_subscribed_topics;
};
