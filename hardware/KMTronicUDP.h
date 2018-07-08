#pragma once

#include "KMTronicBase.h"

class KMTronicUDP : public KMTronicBase
{
public:
	KMTronicUDP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~KMTronicUDP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn) override;
	void Init();
	void GetMeterDetails();
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Username;
	std::string m_Password;
	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
};

