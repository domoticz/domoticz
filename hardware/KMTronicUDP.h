#pragma once

#include "KMTronicBase.h"

class KMTronicUDP : public KMTronicBase
{
      public:
	KMTronicUDP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~KMTronicUDP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool WriteInt(const unsigned char *data, size_t len, bool bWaitForReturn) override;
	void Init();
	void GetMeterDetails();

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Username;
	std::string m_Password;
	std::shared_ptr<std::thread> m_thread;
};
