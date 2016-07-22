#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "../json/json.h"

class GoodweAPI : public CDomoticzHardwareBase
{
public:
	GoodweAPI(const int ID, const std::string &userName);
	~GoodweAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	int m_SiteID;
	std::string m_UserName;
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
	void ParseData(Json::Value object);
	int hash(const char *str);
	int getSunRiseSunSetMinutes(const bool bGetSunRise);
};

