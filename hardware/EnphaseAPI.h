#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class EnphaseAPI : public CDomoticzHardwareBase
{
public:
	EnphaseAPI(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~EnphaseAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	void getProduction();
	void getProductionDetail();

	void getConsumption();
	void getConsumptionDetail();

	void getNetConsumption();
	void getNetConsumptionDetail();

	int getSunRiseSunSetMinutes(const bool bGetSunRise);
private:
	std::string m_szIPAddress;
	P1Power m_p1power;
	P1Power m_c1power;
	P1Power m_c2power;
	std::shared_ptr<std::thread> m_thread;
};

