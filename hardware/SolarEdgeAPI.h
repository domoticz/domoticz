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
		std::string reporterId; // device identity (serial, falling back to properties.identifier or uuid)
		std::string serialNumber; // full serial as reported by the layout, used to match playback data
		std::string displayName;
		std::string inverterName;
		int stringNodeId = -1;
		int inverterNodeId = -1;
		int nodeId; // unique node ID starting at 300
	};

	struct _tWebNodeInfo
	{
		std::string reporterId; // device identity (serial, falling back to properties.identifier or uuid)
		std::string displayName;
		int nodeId;
	};

public:
	SolarEdgeAPI(int ID, const std::string& APIKey, const std::string& Password, const std::string& Extra, int Mode1);
	~SolarEdgeAPI() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	std::string m_szSoftwareVersion;

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
	bool isDaylightWindow();
	void ResetPowerValues();

	void GetBatteryDetails();
	void GetOverview();
	void GetEnergyDetails();

	// Web portal OAuth2 (PKCE) authentication
	bool WebEnsureLoggedIn();
	bool WebLogin();
	bool WebRefreshToken();
	bool WebExchangeSession(const std::string& tokenJsonBody);
	std::string GetWebTokenPrefKey() const;
	bool LoadWebRefreshToken();
	void StoreWebRefreshToken();
	bool ParseLoginForm(const std::string& html, std::string& formAction, std::map<std::string, std::string>& fields) const;
	std::string GetCookieValue(const std::string& name) const;

	// Web portal methods
	bool GetLayoutFromAPI(Json::Value& json_output);
	bool GetSiteLayout();
	void WalkLayoutNode(const Json::Value& node, const std::string& inverterName, int inverterNodeId, int stringNodeId, int& inverterIndex, int& stringIndex, int& optimizerNodeBase);
	void GetOptimizerData();

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

	std::string m_WebAccessToken;
	std::string m_WebRefreshToken;
	time_t m_WebNextRefreshTs = 0;

	// Web portal state
	std::vector<_tOptimizerInfo> m_optimizers;
	std::vector<_tWebNodeInfo> m_webInverters;
	std::vector<_tWebNodeInfo> m_webStrings;
	std::map<int, CounterHelper> m_counterHelpers;

	bool m_bWasInDaylightWindow = true;
	std::vector<double> m_lastInverterEnergy;

	std::shared_ptr<std::thread> m_thread;
};
