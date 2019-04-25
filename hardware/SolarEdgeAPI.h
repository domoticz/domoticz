#pragma once

#include "DomoticzHardware.h"
#include <string>

class SolarEdgeAPI : public CDomoticzHardwareBase
{
	struct _tInverterSettings
	{
		std::string name;
		std::string manufacturer;
		std::string model;
		std::string SN;
	};
public:
	SolarEdgeAPI(const int ID, const std::string &APIKey);
	~SolarEdgeAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool GetSite();
	void GetInverters();
	void GetMeterDetails();
	void GetInverterDetails(const _tInverterSettings *pInverterSettings, const int iInverterNumber);
	int getSunRiseSunSetMinutes(const bool bGetSunRise);
private:
	int m_SiteID;
	std::string m_APIKey;
	std::vector<_tInverterSettings> m_inverters;

	double m_totalActivePower;
	double m_totalEnergy;

	std::shared_ptr<std::thread> m_thread;
};

