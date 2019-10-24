#pragma once

#include "MySensorsBase.h"
#ifdef BUILTIN_MQTT
#include "../MQTT/mosquittopp.h"
#else
#ifdef WIN32
#include "../MQTT/mosquittopp.h"
#else
#include <mosquittopp.h>
#endif
#endif

class MQTT : public MySensorsBase, mosqpp::mosquittopp
{
public:
	MQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAFile, const int TLS_Version, const int Topics);
	~MQTT(void);
	bool isConnected(){ return m_IsConnected; };

	virtual void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	virtual void on_message(const struct mosquitto_message *message) override;
	void on_subscribe(int mid, int qos_count, const int *granted_qos) override;

	void on_log(int level, const char* str) override;
	void on_error() override;

	void SendMessage(const std::string &Topic, const std::string &Message);

	bool m_bDoReconnect;
	bool m_IsConnected;
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool ConnectInt();
	bool ConnectIntEx();
	void SendDeviceInfo(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void SendSceneInfo(const uint64_t SceneIdx, const std::string &SceneName);
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	int m_TLS_Version;
	std::string m_TopicIn;
	std::string m_TopicOut;
	virtual bool StartHardware() override;
	virtual bool StopHardware() override;
	void StopMQTT();
	void Do_Work();
	virtual void SendHeartbeat();
	void WriteInt(const std::string &sendStr) override;
	std::shared_ptr<std::thread> m_thread;
	boost::signals2::connection m_sDeviceReceivedConnection;
	boost::signals2::connection m_sSwitchSceneConnection;
	enum _ePublishTopics {
		PT_none 	  = 0x00,
		PT_out  	  = 0x01, 	// publish on domoticz/out
		PT_floor_room = 0x02 	// publish on domoticz/<floor>/<room>
	};
	_ePublishTopics m_publish_topics;
};

