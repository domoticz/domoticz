#pragma once

// implememtation for regulators : https://www.recalart.com/
// by Fantom (szczukot@poczta.onet.pl)

#include <map>
#include "DomoticzHardware.h"

class csocket;

class MultiFun : public CDomoticzHardwareBase
{
public:
	MultiFun(const int ID, const std::string &IPAddress, const unsigned short IPPort);
	virtual ~MultiFun();

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:

	const unsigned short m_IPPort;
	const std::string m_IPAddress;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	csocket *m_socket;
	sockaddr_in m_addr;
	boost::mutex m_mutex;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	bool ConnectToDevice();
	void DestroySocket();

	int SendCommand(const char* cmd, const unsigned int cmdLength, unsigned char *answer);

	void GetTemperatures();
};
