#pragma once

#include "MySensorsBase.h"
#include "../main/mosquitto_helper.h"
#include "../main/BaroForecastCalculator.h"

namespace Json
{
	class Value;
} // namespace Json

class COctoPrintMQTT : public MySensorsBase, mosqdz::mosquittodz
{
      public:
	COctoPrintMQTT(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAFile);
	~COctoPrintMQTT() override;
	bool isConnected()
	{
		return m_IsConnected;
	};

	void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	void on_message(const struct mosquitto_message *message) override;
	void on_subscribe(int mid, int qos_count, const int *granted_qos) override;

	void SendMessage(const std::string &Topic, const std::string &Message);

      public:
	void UpdateUserVariable(const std::string &varName, const std::string &varValue);

	bool m_bDoReconnect;
	bool m_IsConnected;
	boost::signals2::signal<void()> sDisconnected;

      protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	std::string m_TopicIn;
	bool StartHardware() override;
	bool StopHardware() override;
	void StopMQTT();
	void Do_Work();
	virtual void SendHeartbeat();
	void WriteInt(const std::string &sendStr) override;
	std::shared_ptr<std::thread> m_thread;

      private:
	bool ConnectInt();
	bool ConnectIntEx();
	std::string m_szLastEventName;
	std::map<std::string, time_t> m_LastSendTemp;
};
