#pragma once

// implememtation for weather with free api key : http://openweathermap.org/
// by Fantom (szczukot@poczta.onet.pl)
// Update Juli 2020 by KidDigital to support new One Call API

#include "DomoticzHardware.h"

class COpenWeatherMap : public CDomoticzHardwareBase
{
public:
	COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location);
	~COpenWeatherMap(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
private:
	std::string m_APIKey;
	std::string m_Location;
	std::string m_ForecastURL;
	std::string m_Language;
	bool m_bHaveGPSCoordinated;
	double m_Lat;
	double m_Lon;
	uint16_t m_Interval;
	std::shared_ptr<std::thread> m_thread;
};

