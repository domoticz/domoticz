#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CAccuWeather : public CDomoticzHardwareBase
{
public:
	CAccuWeather(const int ID, const std::string &APIKey, const std::string &Location);
	~CAccuWeather(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	std::string GetForecastURL();
private:
	std::string m_APIKey;
	std::string m_Location;
	std::string m_LocationKey;
	std::string m_ForecastURL;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
	std::string GetLocationKey();
};

