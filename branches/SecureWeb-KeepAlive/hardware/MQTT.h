#pragma once

#include "MySensorsBase.h"

class MQTT : public MySensorsBase
{
public:
	MQTT(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~MQTT(void);
	bool isConnected(){ return m_IsConnected; };
	void OnConnect();
	void OnMQTTMessage(char *topicName, int topicLen,  void *pMessage);
	void SendMessage(const std::string &Topic, const std::string &Message);
	bool m_bDoRestart;
	bool m_disc_finished;
	bool m_IsConnected;
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware();
	bool StopHardware();
	bool ConnectInt(const std::string &IPAddress, const unsigned short usIPPort);
	void WriteInt(const std::string &sendStr);
	void ProcessMySensorsMessage(const std::string &MySensorsMessage);
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void *m_mqtt_client;

	void StopMQTT();

	void Do_Work();
	void OnData(const unsigned char *pData, size_t length);

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

