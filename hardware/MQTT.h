#pragma once

#include "MySensorsBase.h"
#include "../MQTT/mosquittopp.h"

class MQTT : public MySensorsBase, mosqpp::mosquittopp
{
public:
	MQTT(const int ID, const std::string IPAddress, const unsigned short usIPPort, const std::string Username, const std::string Password);
	~MQTT(void);
	bool isConnected(){ return m_IsConnected; };

	void on_connect(int rc);
	void on_disconnect(int rc);
	void on_message(const struct mosquitto_message *message);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);

	void OnMQTTMessage(char *topicName, int topicLen,  void *pMessage);
	void SendMessage(const std::string &Topic, const std::string &Message);

	bool m_bDoReconnect;
	bool m_disc_finished;
	bool m_IsConnected;
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware();
	bool StopHardware();
	bool ConnectInt();
	bool ConnectIntEx();
	void WriteInt(const std::string &sendStr);
	void ProcessMySensorsMessage(const std::string &MySensorsMessage);
	void SendDeviceInfo(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	boost::mutex m_mqtt_mutex;
	void StopMQTT();
	void Do_Work();
	void OnData(const unsigned char *pData, size_t length);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::signals2::connection m_sConnection;
};

