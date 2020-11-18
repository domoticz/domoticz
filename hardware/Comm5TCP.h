#pragma once

#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class Comm5TCP : public CDomoticzHardwareBase, ASyncTCP
{
      public:
	Comm5TCP(int ID, const std::string &IPAddress, unsigned short usIPPort);

	bool WriteToHardware(const char *pdata, unsigned char length) override;

      public: // signals
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;

      protected:
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

	void Do_Work();
	void ParseData(const unsigned char *data, size_t len);

	void queryRelayState();
	void querySensorState();
	void enableNotifications();

	void processSensorData(const std::string &line);

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	std::string buffer;
	bool initSensorData;
	unsigned int lastKnownSensorState;

	bool notificationEnabled;

	bool m_bReceiverStarted;

	std::shared_ptr<std::thread> m_thread;
};
