#pragma once

#include "DomoticzHardware.h"

class CMetOffice : public CDomoticzHardwareBase
{
      public:
	CMetOffice(int ID, const std::string &ForecastAPIKey, const std::string &ObservationAPIKey, const std::string &Location, bool bAddDailyForecast);
	~CMetOffice() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	struct ForecastStep
	{
		time_t time = 0;
		float temperature = 0;
		float dewpoint = 0;
		float feelslike = 0;
		float humidity = 0;
		float pressure = 0; // hPa
		float windspeed = 0; // m/s
		float windgust = 0; // m/s
		int winddirection = 0;
		float visibility = 0; // metres
		float uvindex = 0;
		float rainrate = 0; // mm/h
		float rainprobability = 0;
		int weathercode = -100;
	};

	struct DailyStep
	{
		std::string date; // YYYY-MM-DD
		float maxtemp = 0;
		float mintemp = 0;
		float rainprobability = 0;
		int weathercode = -100;
	};

	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool HTTPGet(const char *szWhat, const std::string &sURL, const std::string &sAPIKey, std::string &sResult);

	std::string CacheFile(const char *szKind) const;
	bool LoadCache(const char *szKind, time_t maxAge, std::string &sData, time_t &fetched);
	void SaveCache(const char *szKind, const std::string &sData, time_t fetched);

	bool ParseForecast(const std::string &sData);
	void FetchForecast();
	void UpdateForecastDevices();

	bool ParseDailyForecast(const std::string &sData);
	void FetchDailyForecast();
	time_t DailyForecastSlotStart(time_t atime) const;
	void UpdateDailyForecastDevices();

	bool ResolveObservationSite();
	void FetchObservation();

	std::string m_ForecastAPIKey;
	std::string m_ObservationAPIKey;
	std::string m_Location;
	double m_Lat = 0;
	double m_Lon = 0;
	bool m_bHaveLocation = false;

	std::vector<ForecastStep> m_Forecast;
	time_t m_ForecastFetched = 0;
	time_t m_NextForecastAttempt = 0;

	bool m_bAddDailyForecast = false;
	std::vector<DailyStep> m_DailyForecast;
	time_t m_DailyForecastFetched = 0;
	time_t m_NextDailyForecastAttempt = 0;
	int m_DailyForecastOffset = 0;

	std::string m_ObservationGeohash;
	time_t m_NextObservationAttempt = 0;

	std::shared_ptr<std::thread> m_thread;
};
