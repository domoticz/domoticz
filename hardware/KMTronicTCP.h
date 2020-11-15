#pragma once

#include "KMTronicBase.h"

class KMTronicTCP : public KMTronicBase
{
      public:
	KMTronicTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &username, const std::string &password);
	~KMTronicTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void ParseRelays(const std::string &sResult);
	void ParseTemps(const std::string &sResult);
	std::string GenerateURL(bool bIsTempDevice);
	bool WriteInt(const unsigned char *data, size_t len, bool bWaitForReturn) override;
	void Init();
	void GetMeterDetails();
	void Do_Work();

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Username;
	std::string m_Password;
	std::shared_ptr<std::thread> m_thread;
	bool m_bCheckedForTempDevice;
	bool m_bIsTempDevice;
};
