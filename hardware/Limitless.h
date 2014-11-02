#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CLimitLess : public CDomoticzHardwareBase
{
public:
	CLimitLess(const int ID, const int LedType, const std::string IPAddress, const unsigned short usIPPort);
	~CLimitLess(void);
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool AddSwitchIfNotExits(const unsigned char Unit, const std::string& devname);
	unsigned char m_LEDType;

	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	SOCKET	m_RemoteSocket;
	sockaddr_in m_stRemoteDestAddr;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
};

