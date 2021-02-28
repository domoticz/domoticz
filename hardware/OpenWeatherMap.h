#pragma once

// implememtation for weather with free api key : http://openweathermap.org/
// by Fantom (szczukot@poczta.onet.pl)
// Update Juli 2020 by KidDigital to support new One Call API

#include "DomoticzHardware.h"

class COpenWeatherMap : public CDomoticzHardwareBase
{
      public:
	COpenWeatherMap(int ID, const std::string &APIKey, const std::string &Location, int adddayforecast, int addhourforecast, int adddescdev, int owmforecastscreen);
	~COpenWeatherMap() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	std::string GetForecastURL();
	Json::Value GetForecastData();

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	int GetForecastFromBarometricPressure(float pressure, float temp = -999.9F);
	std::string GetDayFromUTCtimestamp(uint8_t daynr, const std::string &UTCtimestamp);
	std::string GetHourFromUTCtimestamp(uint8_t hournr, const std::string &UTCtimestamp);
	bool ProcessForecast(Json::Value &forecast, const std::string &period, const std::string &periodname, uint8_t count, int startNodeID);

	bool ResolveLocation(const std::string &Location, double &latitude, double &longitude, uint32_t &cityid, bool IsCityName = true);

	std::string m_APIKey;
	std::string m_Location;
	std::string m_ForecastURL;
	std::string m_Language;
	bool m_itIsRaining = false;
	bool m_add_dayforecast = false;
	bool m_add_hourforecast = false;
	bool m_add_descriptiondevices = false;
	bool m_use_owminforecastscreen = false;
	double m_Lat = 0;
	double m_Lon = 0;
	uint32_t m_CityID = 0;
	std::shared_ptr<std::thread> m_thread;
};
