#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class EnphaseAPI : public CDomoticzHardwareBase
{
public:
	EnphaseAPI(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~EnphaseAPI(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	std::string m_szIPAddress;
	P1Power m_p1power;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	void getProduction();
	void getProductionDetail();

	int getSunRiseSunSetMinutes(const bool bGetSunRise);
};

