#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class SolarEdgeAPI : public CDomoticzHardwareBase
{
public:
	SolarEdgeAPI(const int ID, const int SiteID, const std::string &Serial, const std::string &APIKey);
	~SolarEdgeAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	int m_SiteID;
	std::string m_Serial;
	std::string m_APIKey;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	double m_LastUsage;
	double m_LastCounter;
	float m_LastTemp;
	float m_LastAC;
	float m_LastDC;
	float m_LastHz;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
	int getSunRiseSunSetMinutes(const bool bGetSunRise);
};

