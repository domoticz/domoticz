#pragma once

#include "DomoticzHardware.h"

class CBuienRadar : public CDomoticzHardwareBase
{
public:
	CBuienRadar(const int ID, const int iForecast, const int iThreshold, const std::string &Location);
	~CBuienRadar(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	std::string GetForecastURL();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool GetStationDetails();
	void GetMeterDetails();
	void GetRainPrediction();
private:
	std::string m_szMyLatitude;
	std::string m_szMyLongitude;
	std::string m_sStationName;
	std::string m_sStationRegion;
	std::shared_ptr<std::thread> m_thread;
	int m_iForecast = 15;
	int m_iThreshold = 25;
	int m_iStationID = 0;
	bool m_stationidprovided = false;
	bool m_itIsRaining = false;
	int m_rainShowerLeadTime = 0;
};

