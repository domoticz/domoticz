/**
 * SolarEdge API & Web Portal Hardware Driver v2.0
 *
 * Author: GizMoCuz
 *
 * Credits:
 *   - AndrewTapp / solaredgeoptimizers (Home Assistant integration)
 *     Reference implementation for SolarEdge web portal protocol
 *   - Claude (Anthropic) - AI-assisted development
 */

#pragma once

#include "DomoticzHardware.h"
#include "CounterHelper.h"
#include <map>
#include <string>

namespace Json
{
	class Value;
} // namespace Json

class SolarEdgeAPI : public CDomoticzHardwareBase
{
	struct _tInverterSettings
	{
		std::string name;
		std::string manufacturer;
		std::string model;
		std::string SN;
	};

	struct _tOptimizerInfo
	{
		int reporterId;
		std::string serialNumber;
		std::string displayName;
		std::string inverterName;
		int nodeId; // unique node ID starting at 300
	};

	struct _tWebNodeInfo
	{
		int reporterId;
		std::string displayName;
		int nodeId;
	};

public:
	SolarEdgeAPI(int ID, const std::string& APIKey, const std::string& Password, const std::string& Extra, int Mode1);
	~SolarEdgeAPI() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;

private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool GetSite();
	void GetBatteryFromInventory();
	void GetInverters();
	void GetMeterDetails();
	void GetInverterDetails(const _tInverterSettings* pInverterSettings, int iInverterNumber);
	int getSunRiseSunSetMinutes(bool bGetSunRise);

	void GetBatteryDetails();
	void GetOverview();
	void GetEnergyDetails();

	// Web portal methods
	bool GetSiteLayout();
	void GetOptimizerData();
	void GetEnergyFromLayout(const Json::Value& reportersData);

private:
	int m_SiteID;
	std::string m_APIKey;
	std::vector<_tInverterSettings> m_inverters;

	double m_totalActivePower;
	double m_totalEnergy;

	bool m_bPollBattery = true;

	std::string m_WebUsername;
	std::string m_WebPassword;
	std::string m_WebSiteID;
	bool m_bPollOptimizers = false;

	// Web portal state
	std::vector<_tOptimizerInfo> m_optimizers;
	std::vector<_tWebNodeInfo> m_webInverters;
	std::vector<_tWebNodeInfo> m_webStrings;
	std::map<int, CounterHelper> m_counterHelpers;

	std::shared_ptr<std::thread> m_thread;
};
