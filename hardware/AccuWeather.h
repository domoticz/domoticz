#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CAccuWeather : public CDomoticzHardwareBase
{
public:
	CAccuWeather(const int ID, const std::string &APIKey, const std::string &Location);
	~CAccuWeather(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	std::string GetLocationKey();
private:
	std::string m_APIKey;
	std::string m_Location;
	std::string m_LocationKey;
	std::string m_ForecastURL;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
};

