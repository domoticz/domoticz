#pragma once

// implememtation for weather with free api key : http://openweathermap.org/
// by Fantom (szczukot@poczta.onet.pl)
// Update Juli 2020 by KidDigital to support new One Call API

#include "DomoticzHardware.h"

class COpenWeatherMap : public CDomoticzHardwareBase
{
public:
	COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location, const int adddayforecast, const int addhourforecast);
	~COpenWeatherMap(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	int GetForecastFromBarometricPressure(const float pressure, const float temp = -999.9f);
	std::string GetDayFromUTCtimestamp(const uint8_t daynr, std::string UTCtimestamp);
	std::string GetHourFromUTCtimestamp(const uint8_t hournr, std::string UTCtimestamp);
	bool ProcessForecast(Json::Value &forecast, const std::string period, const std::string periodname, const uint8_t count, const int startNodeID);

	bool ResolveLocation(const std::string& Location, double& latitude, double& longitude, const bool IsCityName = true);

	std::string m_APIKey;
	std::string m_Location;
	std::string m_ForecastURL;
	std::string m_Language;
	bool m_itIsRaining = false;
	bool m_add_dayforecast = false;
	bool m_add_hourforecast = false;
	double m_Lat = 0;
	double m_Lon = 0;
	std::shared_ptr<std::thread> m_thread;
};
