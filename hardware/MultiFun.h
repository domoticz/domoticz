#pragma once

// implememtation for regulators : https://www.recalart.com/
// by Fantom (szczukot@poczta.onet.pl)

#include <map>
#include "DomoticzHardware.h"

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

	bool StartHardware();
	bool StopHardware();
	void Do_Work();
};
