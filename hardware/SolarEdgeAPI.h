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
	SolarEdgeAPI(int ID, const std::string& APIKey);
	~SolarEdgeAPI() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;

private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool GetSite();
	void GetInverters();
	void GetMeterDetails();
	void GetInverterDetails(const _tInverterSettings* pInverterSettings, int iInverterNumber);
	int getSunRiseSunSetMinutes(bool bGetSunRise);

	void GetBatteryDetails();

private:
	int m_SiteID;
	std::string m_APIKey;
	std::vector<_tInverterSettings> m_inverters;

	double m_totalActivePower;
	double m_totalEnergy;

	bool m_bPollBattery = true;

	std::shared_ptr<std::thread> m_thread;
};
