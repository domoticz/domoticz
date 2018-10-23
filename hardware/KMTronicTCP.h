#pragma once

#include "KMTronicBase.h"

class KMTronicTCP : public KMTronicBase
{
public:
	KMTronicTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password);
	~KMTronicTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void ParseRelays(const std::string &sResult);
	void ParseTemps(const std::string &sResult);
	std::string GenerateURL(const bool bIsTempDevice);
	bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn) override;
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

