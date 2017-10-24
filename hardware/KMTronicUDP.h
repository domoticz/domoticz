#pragma once

#include <iosfwd>
#include "KMTronicBase.h"

class KMTronicUDP : public KMTronicBase
{
public:
	KMTronicUDP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~KMTronicUDP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware();
	bool StopHardware();
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Username;
	std::string m_Password;

	bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn);
	void Init();
	void GetMeterDetails();
	void Do_Work();

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

