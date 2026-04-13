/**
 * SolarEdge API & Web Portal Hardware Driver v2.0
 *
 * Author: GizMoCuz
 *
 * Credits for Web API:
 *   - AndrewTapp / solaredgeoptimizers
 *   - Claude (Anthropic) - AI-assisted development
 */

#include "stdafx.h"
#include "SolarEdgeAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include <libwebem/Base64.h>

#define SE_VOLT_DC 20
#define SE_POWERLIMIT 21
#define SE_GROUND_RES 22
#define SE_INV_MODE 23
#define SE_AC_CURRENT 24
#define SE_DATE 25

#define SE_GRID 30
#define SE_LOAD 31
#define SE_PV 32
#define SE_STORAGE_STATUS 33
#define SE_STORAGE_POWER 34
#define SE_STORAGE_CHARGELEVEL 35
#define SE_STORAGE_CRITITAL 36

#define SE_OVERVIEW_CURRENT 40
#define SE_OVERVIEW_TODAY 41
#define SE_OVERVIEW_MONTH 42
#define SE_OVERVIEW_YEAR 43
#define SE_OVERVIEW_LIFETIME 44

#define SE_ENERGY_PRODUCTION 50
#define SE_ENERGY_CONSUMPTION 51
#define SE_ENERGY_SELFCONSUMPTION 52
#define SE_ENERGY_FEEDIN 53
#define SE_ENERGY_PURCHASED 54

// Optimizer web-portal sensor sub-IDs (per optimizer node, base node 300+)
#define SE_OPT_POWER 1
#define SE_OPT_VOLTAGE 2
#define SE_OPT_OPTIMIZER_VOLTAGE 3
#define SE_OPT_CURRENT 4
#define SE_OPT_LIFETIME_ENERGY 5

// Web portal node ID bases for inverters and strings
#define SE_WEB_INVERTER_BASE 250
#define SE_WEB_STRING_BASE 260

// Web portal energy sub-IDs (for inverter/string nodes)
#define SE_WEB_ENERGY_TODAY 1
#define SE_WEB_ENERGY_LIFETIME 2


#ifdef _DEBUG
//	#define DEBUG_SolarEdgeAPIR
//	#define DEBUG_SolarEdgeAPIW
#endif

#ifdef DEBUG_SolarEdgeAPIW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_SolarEdgeAPIR
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	std::string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif

SolarEdgeAPI::SolarEdgeAPI(const int ID, const std::string& APIKey, const std::string& Password, const std::string& Extra, const int Mode1) :
	m_APIKey(APIKey)
{
	m_SiteID = 0;
	m_HwdID = ID;
	m_totalActivePower = 0;
	m_totalEnergy = 0;

	// Parse Extra: "web_username|site_id"
	size_t pipePos = Extra.find('|');
	if (pipePos != std::string::npos)
	{
		m_WebUsername = Extra.substr(0, pipePos);
		m_WebSiteID = Extra.substr(pipePos + 1);
	}
	else
	{
		m_WebUsername = Extra; // No pipe, treat entire string as username
	}
	m_WebPassword = Password;
	m_bPollOptimizers = (Mode1 != 0);
}

bool SolarEdgeAPI::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool SolarEdgeAPI::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void SolarEdgeAPI::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");

	// Polling intervals (seconds)
	// Layout: every 2 hours (also provides site/inverter/string/optimizer energy data)
	// Optimizer systemData: every 10 minutes (20 calls per cycle, daylight only)
	//
	// With 20 optimizers at 10-min intervals during ~14h daylight:
	//   Layout: ~12 calls/day
	//   systemData: ~84 cycles × 20 = ~1680 calls/day (web portal, no strict daily limit)
	constexpr int LAYOUT_INTERVAL = 7200;         // 2 hours
	constexpr int OPTIMIZER_DATA_INTERVAL = 600;   // 10 minutes

	// Start counters so layout runs ~5s after startup, optimizer data ~15s after
	int sec_counter = 295;
	int layout_timer = LAYOUT_INTERVAL - 5;
	int optimizer_data_timer = OPTIMIZER_DATA_INTERVAL - 15;

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		layout_timer++;
		optimizer_data_timer++;

		if (sec_counter % 12 == 0)
			m_LastHeartbeat = mytime(nullptr);

		// API-key polling (site overview, inverter telemetry, energy details, battery)
		if (sec_counter % 300 == 0)
		{
			if (m_SiteID == 0)
			{
				if (!GetSite())
					continue;
				GetBatteryFromInventory();
				GetInverters();
			}

			if (!m_inverters.empty())
				GetMeterDetails();
			GetOverview();
			GetEnergyDetails();
			if (m_bPollBattery)
				GetBatteryDetails();
		}

		// Web portal polling (requires web credentials)
		bool bWebCredentials = !m_WebUsername.empty() && !m_WebPassword.empty();

		// Web portal: refresh site layout + energy every 2 hours (site/inverter/string/optimizer energy)
		// Need either a configured Web Site ID or an API-discovered m_SiteID
		if (bWebCredentials && !m_WebSiteID.empty() && (m_optimizers.empty() || layout_timer >= LAYOUT_INTERVAL))
		{
			layout_timer = 0;
			GetSiteLayout();
		}

		// Web portal: poll optimizer real-time data (power/voltage/current) every 10 minutes
		// This makes 1 HTTP call per optimizer, so gated by the Poll Optimizers setting
		if (bWebCredentials && m_bPollOptimizers && !m_optimizers.empty() && optimizer_data_timer >= OPTIMIZER_DATA_INTERVAL)
		{
			optimizer_data_timer = 0;
			GetOptimizerData();
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool SolarEdgeAPI::WriteToHardware(const char* pdata, const unsigned char length)
{
	return false;
}

int SolarEdgeAPI::getSunRiseSunSetMinutes(const bool bGetSunRise)
{
	std::vector<std::string> strarray;
	std::vector<std::string> sunRisearray;
	std::vector<std::string> sunSetarray;

	if (!m_mainworker.m_LastSunriseSet.empty())
	{
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
		StringSplit(strarray[0], ":", sunRisearray);
		StringSplit(strarray[1], ":", sunSetarray);

		int sunRiseInMinutes = (atoi(sunRisearray[0].c_str()) * 60) + atoi(sunRisearray[1].c_str());
		int sunSetInMinutes = (atoi(sunSetarray[0].c_str()) * 60) + atoi(sunSetarray[1].c_str());

		if (bGetSunRise) {
			return sunRiseInMinutes;
		}
		return sunSetInMinutes;
	}
	return 0;
}

bool SolarEdgeAPI::isDaylightWindow()
{
	// Check daylight window
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;
	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);
	if (ActHourMin + 60 < sunRise)
		false;
	if (ActHourMin - 60 > sunSet)
		false;
	return true;
}


bool SolarEdgeAPI::GetSite()
{
	m_SiteID = 0;
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_sites.json");
#else

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/sites/list.json?size=1&api_key=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (Sites)!");
		return false;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_sites.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return false;
	}
	if (root["sites"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return false;
	}
	if (root["sites"]["count"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return false;
	}
	int tot_results = root["sites"]["count"].asInt();
	if (tot_results < 1)
		return false;
	Json::Value reading = root["sites"]["site"][0];

	if (reading["id"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return false;
	}
	m_SiteID = reading["id"].asInt();
	if (m_WebSiteID.empty())
		m_WebSiteID = std::to_string(m_SiteID);
	return true;
}

void SolarEdgeAPI::GetBatteryFromInventory()
{
	std::string sResult;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/site/" << m_SiteID << "/inventory.json?api_key=" << m_APIKey;

	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (Inventory)!");
		return;
	}

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}
	if (root["Inventory"]["batteries"].empty() == true)
		m_bPollBattery = false;
	else
		m_bPollBattery = true;

	return;
}

void SolarEdgeAPI::GetInverters()
{
	m_inverters.clear();
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_inverters.json");
#else

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/equipment/" << m_SiteID << "/list.json?api_key=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (Equipment)!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_inverters.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}
	if (root["reporters"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	if (root["reporters"]["count"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	int tot_results = root["reporters"]["count"].asInt();
	if (tot_results < 1)
		return;

	for (int iInverter = 0; iInverter < tot_results; iInverter++)
	{
		Json::Value reading = root["reporters"]["list"][iInverter];

		if (reading["name"].empty() == true)
			return;
		_tInverterSettings iSettings;
		iSettings.name = reading["name"].asString();
		iSettings.manufacturer = reading["manufacturer"].asString();
		iSettings.model = reading["model"].asString();
		iSettings.SN = reading["serialNumber"].asString();
		m_inverters.push_back(iSettings);
	}
}

void SolarEdgeAPI::GetMeterDetails()
{
	m_totalActivePower = 0;
	m_totalEnergy = 0;

	for (int iInverter = 0; iInverter < (int)m_inverters.size(); iInverter++)
	{
		GetInverterDetails(&m_inverters[iInverter], iInverter);
	}

	if ((m_inverters.size() > 1) && (m_totalEnergy > 0))
	{
		//Send total kWh
		SendKwhMeter(1, 1, 255, m_totalActivePower, m_totalEnergy / 1000.0, "kWh Meter Total");
	}
}

void SolarEdgeAPI::GetInverterDetails(const _tInverterSettings* pInverterSettings, const int iInverterNumber)
{
	std::string sResult;
	char szTmp[200];
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge.json");
#else
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	//We only poll one hour before sunrise till one hour after sunset
	if (!isDaylightWindow())
		return;

	struct tm ltime_min10;
	time_t atime_min10;
	constructTime(atime_min10, ltime_min10, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min - 10, ltime.tm_sec, ltime.tm_isdst);

	sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d", ltime_min10.tm_year + 1900, ltime_min10.tm_mon + 1, ltime_min10.tm_mday, ltime_min10.tm_hour, ltime_min10.tm_min, ltime_min10.tm_sec);
	std::string startDate = CURLEncode::URLEncode(szTmp);

	sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	std::string endDate = CURLEncode::URLEncode(szTmp);

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/equipment/" << m_SiteID << "/" << pInverterSettings->SN << "/data.json?startTime=" << startDate << "&endTime=" << endDate << "&api_key=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (Equipment details)!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}
	if (root["data"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	if (root["data"]["count"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	int tot_results = root["data"]["count"].asInt();
	if (tot_results < 1)
		return;
	if (root["data"]["telemetries"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}

	int rsize = (int)root["data"]["telemetries"].size();
	if (rsize < 1)
	{
		return;
	}

	//We could have multiple sites here
	Json::Value reading = root["data"]["telemetries"][rsize - 1];
	if ((!reading["totalActivePower"].empty()) && (!reading["totalEnergy"].empty()))
	{
		double curActivePower = reading["totalActivePower"].asDouble();
		double curEnergy = reading["totalEnergy"].asDouble();
		if (curEnergy != 0)
		{
			sprintf(szTmp, "kWh Meter %s", pInverterSettings->name.c_str());
			SendKwhMeter(0, 1 + iInverterNumber, 255, curActivePower, curEnergy / 1000.0, szTmp);
		}
		m_totalActivePower += curActivePower;
		m_totalEnergy += curEnergy;
	}
	if (!reading["dcVoltage"].empty())
	{
		float dcVoltage = reading["dcVoltage"].asFloat();
		sprintf(szTmp, "DC %s", pInverterSettings->name.c_str());
		SendVoltageSensor(iInverterNumber, SE_VOLT_DC, 255, dcVoltage, szTmp);
	}
	if (!reading["powerLimit"].empty())
	{
		float powerLimit = reading["powerLimit"].asFloat();
		sprintf(szTmp, "powerLimit %s", pInverterSettings->name.c_str());
		SendPercentageSensor(iInverterNumber, SE_POWERLIMIT, 255, powerLimit, szTmp);
	}
	if (!reading["groundFaultResistance"].empty())
	{
		float groundFaultResistance = reading["groundFaultResistance"].asFloat();
		sprintf(szTmp, "groundFaultResistance %s", pInverterSettings->name.c_str());
		SendCustomSensor(iInverterNumber, SE_GROUND_RES, 255, groundFaultResistance, szTmp, "kOhm");
	}
	if (!reading["inverterMode"].empty())
	{
		sprintf(szTmp, "inverterMode %s", pInverterSettings->name.c_str());
		SendTextSensor(iInverterNumber, SE_INV_MODE, 255, reading["inverterMode"].asString(), szTmp);
	}
	if (!reading["date"].empty())
	{
		sprintf(szTmp, "date %s", pInverterSettings->name.c_str());
		SendTextSensor(iInverterNumber, SE_DATE, 255, reading["date"].asString(), szTmp);
	}
	if (!reading["temperature"].empty())
	{
		float temp = reading["temperature"].asFloat();
		sprintf(szTmp, "Temp %s", pInverterSettings->name.c_str());
		SendTempSensor(1 + iInverterNumber, 255, temp, szTmp);
	}

	char szPhase[30];
	for (int ii = 0; ii < 3; ii++)
	{
		int iPhase = ii + 1;
		sprintf(szPhase, "L%dData", iPhase);
		if (!reading[szPhase].empty())
		{
			if (!reading[szPhase]["acVoltage"].empty())
			{
				float acVoltage = reading[szPhase]["acVoltage"].asFloat();
				sprintf(szTmp, "AC L%d %s", iPhase, pInverterSettings->name.c_str());
				SendVoltageSensor(iInverterNumber, iPhase, 255, acVoltage, szTmp);
			}
			if (!reading[szPhase]["acFrequency"].empty())
			{
				float acFrequency = reading[szPhase]["acFrequency"].asFloat();
				sprintf(szTmp, "Hz L%d %s", iPhase, pInverterSettings->name.c_str());
				SendCustomSensor(1 + iInverterNumber, iPhase, 255, acFrequency, szTmp, "Hz");
			}
			if (!reading[szPhase]["acCurrent"].empty())
			{
				float acCurrent = reading[szPhase]["acCurrent"].asFloat();
				sprintf(szTmp, "acCurrent L%d %s", iPhase, pInverterSettings->name.c_str());
				SendCustomSensor(iInverterNumber, SE_AC_CURRENT + ii, 255, acCurrent, szTmp, "A");
			}

			if (!reading[szPhase]["activePower"].empty())
			{
				float ActivePower = reading[szPhase]["activePower"].asFloat();
				sprintf(szTmp, "Power L%d %s", iPhase, pInverterSettings->name.c_str());
				SendWattMeter(1 + iInverterNumber, iPhase, 255, ActivePower, szTmp);
			}
		}
	}
}

void SolarEdgeAPI::GetBatteryDetails()
{
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_currentPowerFlow.json");
#else

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/site/" << m_SiteID << "/currentPowerFlow?api_key=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (currentPowerFlow details)!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_currentPowerFlow.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}
	if (root["siteCurrentPowerFlow"].empty() == true)
	{
		return;
	}
	root = root["siteCurrentPowerFlow"];

	// Parse connections to determine power flow direction
	std::vector<std::string> power_from;
	std::vector<std::string> power_to;
	if (!root["connections"].empty())
	{
		for (const auto& conn : root["connections"])
		{
			if (!conn["from"].empty())
			{
				std::string from = conn["from"].asString();
				std::transform(from.begin(), from.end(), from.begin(), ::tolower);
				power_from.push_back(from);
			}
			if (!conn["to"].empty())
			{
				std::string to = conn["to"].asString();
				std::transform(to.begin(), to.end(), to.begin(), ::tolower);
				power_to.push_back(to);
			}
		}
	}

	std::string status;
	float power;

	if (!root["GRID"].empty())
	{
		status = root["GRID"]["status"].asString();
		if (status == "Active")
			power = root["GRID"]["currentPower"].asFloat();
		else
			power = 0;
		// If grid is in power_to, we are exporting — negate
		if (std::find(power_to.begin(), power_to.end(), "grid") != power_to.end())
			power = -power;
		SendWattMeter(200, SE_GRID, 255, power * 1000, "Grid Power");
	}
	if (!root["LOAD"].empty())
	{
		status = root["LOAD"]["status"].asString();
		if (status == "Active")
			power = root["LOAD"]["currentPower"].asFloat();
		else
			power = 0;
		SendWattMeter(200, SE_LOAD, 255, power * 1000, "Load Power");
	}
	if (!root["PV"].empty())
	{
		status = root["PV"]["status"].asString();
		if (status == "Active")
			power = root["PV"]["currentPower"].asFloat();
		else
			power = 0;
		SendWattMeter(200, SE_PV, 255, power * 1000, "PV Power");
	}
	if (!root["STORAGE"].empty())
	{
		status = root["STORAGE"]["status"].asString();
		SendTextSensor(200, SE_STORAGE_STATUS, 255, status, "Battery Status");

		power = root["STORAGE"]["currentPower"].asFloat();

		// If storage is in power_to, it is charging — negate
		if (std::find(power_to.begin(), power_to.end(), "storage") != power_to.end())
		{
			if (power > 0)
				power = -power;
		}

		SendWattMeter(200, SE_STORAGE_POWER, 255, power * 1000, "Battery Power");

		float chargeLevel = root["STORAGE"]["chargeLevel"].asFloat();
		SendPercentageSensor(200, SE_STORAGE_CHARGELEVEL, 255, chargeLevel, "Battery Charge Level");

		bool batteryCritical = root["STORAGE"]["critical"].asBool();
		SendSwitch(200, SE_STORAGE_CRITITAL, 255, batteryCritical, 0, "Battery Critical", "SolarEdge");
	}
}

void SolarEdgeAPI::GetOverview()
{
	// Check daylight window
	if (!isDaylightWindow())
		return;

	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_overview.json");
#else

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/site/" << m_SiteID << "/overview.json?api_key=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (Overview)!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_overview.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}
	if (root["overview"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	const Json::Value& overview = root["overview"];

	float power = 0;
	if (!overview["currentPower"].empty())
	{
		power = overview["currentPower"]["power"].asFloat();
		SendWattMeter(200, SE_OVERVIEW_CURRENT, 255, power, "Site Current Power");

		if (power > 0) // "last..." is only valid if there is power, otherwise API may return previous values
		{
			if (!overview["lastDayData"].empty())
			{
				float energy = overview["lastDayData"]["energy"].asFloat();
				SendCustomSensor(200, SE_OVERVIEW_TODAY, 255, energy / 1000, "Energy Today", "kWh");
			}

			if (!overview["lastMonthData"].empty())
			{
				float energy = overview["lastMonthData"]["energy"].asFloat();
				SendCustomSensor(200, SE_OVERVIEW_MONTH, 255, energy / 1000, "Energy This Month", "kWh");
			}
			if (!overview["lastYearData"].empty())
			{
				float energy = overview["lastYearData"]["energy"].asFloat();
				SendCustomSensor(200, SE_OVERVIEW_YEAR, 255, energy / 1000, "Energy This Year", "kWh");
			}
		}
	}
	if (!overview["lifeTimeData"].empty())
	{
		float energy = overview["lifeTimeData"]["energy"].asFloat();
		SendCustomSensor(200, SE_OVERVIEW_LIFETIME, 255, energy / 1000, "Lifetime Energy", "kWh");
	}
}

void SolarEdgeAPI::GetEnergyDetails()
{
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_energyDetails.json");
#else
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	
	// get yesterday from today's noon
	struct tm ltime_noon;
	getNoon(atime, ltime_noon);
	struct tm ltime_yesterday;
	time_t yesterday = atime - 86400;
	localtime_r(&yesterday, &ltime_yesterday);

	char szTmp[200];
	sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d", ltime_yesterday.tm_year + 1900, ltime_yesterday.tm_mon + 1, ltime_yesterday.tm_mday, 0, 0, 0);
	std::string startDate = CURLEncode::URLEncode(szTmp);

	sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, 0, 0, 0);
	std::string endDate = CURLEncode::URLEncode(szTmp);

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/site/" << m_SiteID << "/energyDetails.json?startTime=" << startDate << "&endTime=" << endDate << "&timeUnit=DAY&api_key=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error getting http data (EnergyDetails)!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_energyDetails.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}
	if (root["energyDetails"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	if (root["energyDetails"]["meters"].empty() == true)
		return;

	const Json::Value& meters = root["energyDetails"]["meters"];
	for (const auto& meter : meters)
	{
		if (meter["type"].empty() || meter["values"].empty())
			continue;
		const std::string meterType = meter["type"].asString();
		const Json::Value& values = meter["values"];
		if (values.empty())
			continue;
		const Json::Value& yesterday = values[0];
		if (yesterday["value"].empty()) // no previous value, skip
			continue;

		const Json::Value& today = values[1]; // this is todays value

		float energy = today["value"].asFloat();

		if (meterType == "Production")
		{
			SendCustomSensor(201, SE_ENERGY_PRODUCTION, 255, energy / 1000, "Energy Production", "kWh");
		}
		else if (meterType == "Consumption")
		{
			SendCustomSensor(201, SE_ENERGY_CONSUMPTION, 255, energy / 1000, "Energy Consumption", "kWh");
		}
		else if (meterType == "SelfConsumption")
		{
			SendCustomSensor(201, SE_ENERGY_SELFCONSUMPTION, 255, energy / 1000, "Energy Self Consumption", "kWh");
		}
		else if (meterType == "FeedIn")
		{
			SendCustomSensor(201, SE_ENERGY_FEEDIN, 255, energy / 1000, "Energy Feed In", "kWh");
		}
		else if (meterType == "Purchased")
		{
			SendCustomSensor(201, SE_ENERGY_PURCHASED, 255, energy / 1000, "Energy Purchased", "kWh");
		}
	}
}

bool SolarEdgeAPI::GetLayoutFromAPI(Json::Value& json_output, bool bGetLifeTimeData)
{
	std::string sResult;
	// Json::Value root;

#ifdef DEBUG_SolarEdgeAPIR
	if (bGetLifeTimeData)
		sResult = ReadFile("E:\\SolarEdge_web_layout_lifetime.json");
	else
		sResult = ReadFile("E:\\SolarEdge_web_layout.json");
#else
	// Determine site ID for URL
	if (m_WebSiteID.empty())
	{
		Log(LOG_ERROR, "Web portal: No Site ID available! Configure Site ID or enable API polling.");
		return false;
	}

	// Build Basic Auth header
	std::string credentials = m_WebUsername + ":" + m_WebPassword;
	std::string basicAuth = "Authorization: Basic " + base64_encode(credentials);

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back(basicAuth);
	ExtraHeaders.push_back("Accept: */*");
	ExtraHeaders.push_back("Content-Type: application/json");
	ExtraHeaders.push_back("X-Requested-With: XMLHttpRequest");

	std::stringstream sURL;
	sURL << "https://monitoring.solaredge.com/solaredge-apigw/api/sites/" << m_WebSiteID << "/layout/logical";
	if (bGetLifeTimeData)
	{
		sURL << "?timeUnit=ALL"; // timeUnit required for lifetime data
	}

	if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Web portal: Error getting site layout!");
		return false;
	}
#ifdef DEBUG_SolarEdgeAPIW
	if (bGetLifeTimeData)
		SaveString2Disk(sResult, "E:\\SolarEdge_web_layout_lifetime.json");
	else
		SaveString2Disk(sResult, "E:\\SolarEdge_web_layout.json");
#endif
#endif

	// Json::Value root;
	if (!ParseJSon(sResult, json_output) || !json_output.isObject())
	{
		Log(LOG_ERROR, "Web portal: Invalid JSON in site layout response!");
		return false;
	}
	if (json_output["logicalTree"].empty())
	{
		Log(LOG_ERROR, "Web portal: No logicalTree in site layout response!");
		return false;
	}

	return true;
}

bool SolarEdgeAPI::GetSiteLayout()
{
	Json::Value root;
	if (!GetLayoutFromAPI(root, false))  // get live data
	{
		return false;
	}

	m_optimizers.clear();
	m_webInverters.clear();
	m_webStrings.clear();
	int optimizerNodeBase = 300;
	int inverterIndex = 0;
	int stringIndex = 0;

	const Json::Value& tree = root["logicalTree"];
	const Json::Value& inverterChildren = tree["children"];
	if (inverterChildren.empty())
		return true;

	for (const auto& inverterNode : inverterChildren)
	{
		const Json::Value& invData = inverterNode["data"];
		if (invData.empty())
			continue;

		std::string inverterName = invData.get("displayName", invData.get("name", "").asString()).asString();

		// Collect inverter info
		int invId = invData.get("id", 0).asInt();
		if (invId != 0)
		{
			_tWebNodeInfo invInfo;
			invInfo.reporterId = invId;
			invInfo.displayName = inverterName;
			invInfo.nodeId = SE_WEB_INVERTER_BASE + inverterIndex++;
			m_webInverters.push_back(invInfo);
		}

		const Json::Value& strings = inverterNode["children"];
		if (strings.empty())
			continue;

		for (const auto& stringNode : strings)
		{
			const Json::Value& strData = stringNode["data"];

			// Collect string info
			if (!strData.empty())
			{
				int strId = strData.get("id", 0).asInt();
				if (strId != 0)
				{
					_tWebNodeInfo strInfo;
					strInfo.reporterId = strId;
					strInfo.displayName = "String " + strData.get("displayName", strData.get("name", "").asString()).asString();
					strInfo.nodeId = SE_WEB_STRING_BASE + stringIndex++;
					m_webStrings.push_back(strInfo);
				}
			}

			const Json::Value& panels = stringNode["children"];
			if (panels.empty())
				continue;

			for (const auto& panelNode : panels)
			{
				const Json::Value& data = panelNode["data"];
				if (data.empty())
					continue;

				// Only pick up POWER_BOX (optimizer) nodes
				std::string nodeType = data.get("type", "").asString();
				if (nodeType != "POWER_BOX")
					continue;

				int id = data.get("id", 0).asInt();
				if (id == 0)
					continue;

				_tOptimizerInfo info;
				info.reporterId = id;
				info.serialNumber = data.get("serialNumber", "").asString();
				info.displayName = data.get("displayName", data.get("name", "").asString()).asString();
				info.inverterName = inverterName;
				info.nodeId = optimizerNodeBase++;
				m_optimizers.push_back(info);
			}
		}
	}

	Log(LOG_STATUS, "Web portal: Discovered %d inverters, %d strings, %d optimizers",
		(int)m_webInverters.size(), (int)m_webStrings.size(), (int)m_optimizers.size());
	GetEnergyFromLayout(root["reportersData"], false);

	if (!GetLayoutFromAPI(root, true))  // get lifetime data
	{
		return false;
	}
	GetEnergyFromLayout(root["reportersData"], true);

	return true;
}

void SolarEdgeAPI::GetOptimizerData()
{
	if (m_optimizers.empty())
		return;

	// Check daylight window
	if (!isDaylightWindow())
		return;

#ifndef DEBUG_SolarEdgeAPIR
	std::string basicAuth = "Authorization: Basic " + base64_encode(m_WebUsername + ":" + m_WebPassword);
#endif

	for (const auto& opt : m_optimizers)
	{
		std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
		sResult = ReadFile("E:\\SolarEdge_web_optimizer_" + std::to_string(opt.reporterId) + ".html");
#else
		time_t now = mytime(nullptr);
		std::stringstream sURL;
		sURL << "https://monitoring.solaredge.com/solaredge-web/p/systemData?reporterId=" << opt.reporterId
			<< "&type=panel&activeTab=0&fieldId=" << m_WebSiteID
			<< "&isPublic=false&locale=en_US&v=" << (long long)now * 1000;

		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back(basicAuth);
		ExtraHeaders.push_back("Accept: */*");
		ExtraHeaders.push_back("X-Requested-With: XMLHttpRequest");

		// Use bStartNewSession=true to avoid stale cookie jar interfering with Basic Auth
		if (!HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, true, true))
		{
			Log(LOG_ERROR, "Web portal: Error getting data for optimizer %d (%s)!", opt.reporterId, opt.displayName.c_str());
			return;
		}
#ifdef DEBUG_SolarEdgeAPIW
		SaveString2Disk(sResult, "E:\\SolarEdge_web_optimizer_" + std::to_string(opt.reporterId) + ".html");
#endif
#endif

		// The response is HTML containing SE.systemData = { ... }
		// Extract the JSON block after "SE.systemData = "
		const std::string marker = "SE.systemData = ";
		size_t pos = sResult.find(marker);
		if (pos == std::string::npos)
		{
			Log(LOG_ERROR, "Web portal: systemData marker not found for optimizer %d!", opt.reporterId);
			continue;
		}
		pos += marker.size();
		// Find the opening brace
		size_t braceStart = sResult.find('{', pos);
		if (braceStart == std::string::npos)
			continue;

		// Find the matching closing brace
		int depth = 0;
		size_t braceEnd = braceStart;
		for (size_t i = braceStart; i < sResult.size(); i++)
		{
			if (sResult[i] == '{')
				depth++;
			else if (sResult[i] == '}')
			{
				depth--;
				if (depth == 0)
				{
					braceEnd = i;
					break;
				}
			}
		}
		if (depth != 0)
			continue;

		std::string jsonStr = sResult.substr(braceStart, braceEnd - braceStart + 1);
		Json::Value dataRoot;
		if (!ParseJSon(jsonStr, dataRoot) || !dataRoot.isObject())
		{
			Log(LOG_ERROR, "Web portal: Invalid systemData JSON for optimizer %d!", opt.reporterId);
			continue;
		}

		// Response is per-optimizer: measurements are directly on root object with string values
		const Json::Value& measurements = dataRoot["measurements"];
		if (measurements.empty())
			continue;

		char szTmp[200];

		if (!measurements["Power [W]"].empty())
		{
			float power = static_cast<float>(atof(measurements["Power [W]"].asString().c_str()));
			snprintf(szTmp, sizeof(szTmp), "Power %s", opt.displayName.c_str());
			SendWattMeter(opt.nodeId, SE_OPT_POWER, 255, power, szTmp);
		}
		if (!measurements["Voltage [V]"].empty())
		{
			float voltage = static_cast<float>(atof(measurements["Voltage [V]"].asString().c_str()));
			snprintf(szTmp, sizeof(szTmp), "Voltage %s", opt.displayName.c_str());
			SendVoltageSensor(opt.nodeId, SE_OPT_VOLTAGE, 255, voltage, szTmp);
		}
		if (!measurements["Optimizer Voltage [V]"].empty())
		{
			float optVoltage = static_cast<float>(atof(measurements["Optimizer Voltage [V]"].asString().c_str()));
			snprintf(szTmp, sizeof(szTmp), "Optimizer Voltage %s", opt.displayName.c_str());
			SendVoltageSensor(opt.nodeId, SE_OPT_OPTIMIZER_VOLTAGE, 255, optVoltage, szTmp);
		}
		if (!measurements["Current [A]"].empty())
		{
			float current = static_cast<float>(atof(measurements["Current [A]"].asString().c_str()));
			snprintf(szTmp, sizeof(szTmp), "Current %s", opt.displayName.c_str());
			SendCustomSensor(opt.nodeId, SE_OPT_CURRENT, 255, current, szTmp, "A");
		}
	}
}

void SolarEdgeAPI::GetEnergyFromLayout(const Json::Value& reportersData, bool bSetLifeTimeData)
{
	if (reportersData.empty())
		return;

	// We only process live data one hour before sunrise till one hour after sunset
	if (!bSetLifeTimeData && !isDaylightWindow())
		return;

	char szTmp[200];
	int childID = bSetLifeTimeData ? SE_WEB_ENERGY_LIFETIME : SE_WEB_ENERGY_TODAY;

	// Inverter-level energy
	for (const auto& inv : m_webInverters)
	{
		std::string key = std::to_string(inv.reporterId);
		if (reportersData[key].empty() || reportersData[key]["unscaledEnergy"].empty())
			continue;

		float energy = reportersData[key]["unscaledEnergy"].asFloat();
		if (energy < 0)
			continue;

		snprintf(szTmp, sizeof(szTmp), bSetLifeTimeData ? "Lifetime Energy Inverter %s" : "Energy Today Inverter %s", inv.displayName.c_str());
		SendCustomSensor(inv.nodeId, childID, 255, energy / 1000, szTmp, "kWh");
	}

	// String-level energy
	for (const auto& str : m_webStrings)
	{
		std::string key = std::to_string(str.reporterId);
		if (reportersData[key].empty() || reportersData[key]["unscaledEnergy"].empty())
			continue;

	    float energy = reportersData[key]["unscaledEnergy"].asFloat();
		if (energy < 0)
			continue;

		snprintf(szTmp, sizeof(szTmp), bSetLifeTimeData ? "Lifetime Energy %s" : "Energy Today %s", str.displayName.c_str());
		SendCustomSensor(str.nodeId, childID, 255, energy / 1000, szTmp, "kWh");
	}

	// Optimizer-level energy
	if (bSetLifeTimeData)
	{
		for (const auto& opt : m_optimizers)
		{
			std::string key = std::to_string(opt.reporterId);
			if (reportersData[key].empty() || reportersData[key]["unscaledEnergy"].empty())
				continue;

			float energy = reportersData[key]["unscaledEnergy"].asFloat();
			if (energy < 0)
				continue;

			snprintf(szTmp, sizeof(szTmp), "Lifetime Energy %s", opt.displayName.c_str());
			SendCustomSensor(opt.nodeId, SE_OPT_LIFETIME_ENERGY, 255, energy / 1000, szTmp, "kWh");
		}
	}
}
