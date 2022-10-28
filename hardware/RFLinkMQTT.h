#pragma once

#include "RFLinkBase.h"
#include "../main/mosquitto_helper.h"

// #ifdef BUILTIN_MQTT
// #include "../MQTT/mosquittopp.h"
// #else
// #ifdef WIN32
// #include "../MQTT/mosquittopp.h"
// #else
// #include <mosquittopp.h>
// #endif
// #endif

class CRFLinkMQTT: public CRFLinkBase, mosqdz::mosquittodz
{
public:
	CRFLinkMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort , const std::string &Username, const std::string &Password , const std::string &CAfilenameExtra, const int TLS_Version, const int PublishScheme, const bool Multidomonodesync);
	~CRFLinkMQTT(void);
	bool isConnected(){ return m_IsConnected; };
	void on_disconnect(int rc) override;
	virtual void on_connect(int rc) override;
	virtual void on_message(const struct mosquitto_message *message) override;
	void on_subscribe(int mid, int qos_count, const int *granted_qos) override;
	void SendMessage(const std::string &Topic, const std::string &Message);
	bool m_bDoReconnect;
	bool m_IsConnected;
private:
	bool ConnectInt();
	bool ConnectIntEx();
	bool StartHardware() override;
	bool StopHardware() override;
protected:
	std::string m_szIPAddressList;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	unsigned long m_syncid;
	bool m_bDoRestart;
	std::string m_UserName;
	std::string m_Password;
	std::string m_TopicIn;
	std::string m_TopicOut;
	std::string m_CAFilename;
	int m_TLS_Version;
	boost::signals2::connection m_sDeviceReceivedConnection;
	boost::signals2::connection m_sSwitchSceneConnection;
	void selectNextIPAdress( void );
	virtual bool WriteInt(const std::string &sendString); // override;
	void Do_Work();
	virtual void SendHeartbeat();
	void StopMQTT();
	std::shared_ptr<std::thread> m_thread;
	bool m_bPreventLoop = true;
	unsigned int m_lastmsgCRC = 0;
	time_t m_lastmsgTime = 0;
	bool m_bMultidomonodesync = false;

};
