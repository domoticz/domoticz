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
#include "../main/SQLHelper.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include <libwebem/Base64.h>
#include <openssl/rand.h>
#include <fstream>
#include <regex>

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

// Web portal node ID bases for inverters and strings
#define SE_WEB_INVERTER_BASE 250
#define SE_WEB_STRING_BASE 260

// Web portal power sub-ID (aggregated from child optimizers), for inverter/string nodes
#define SE_WEB_POWER 1

// SolarEdge web portal OAuth2 (PKCE) endpoints
#define SE_WEB_CLIENT_ID "ugfnsujd3384sshcjehaphlh3"
#define SE_WEB_REDIRECT_URI "https://monitoring.solaredge.com/mfe/auth/callback"
#define SE_WEB_AUTHORIZE_URL "https://login.solaredge.com/oauth2/authorize"
#define SE_WEB_TOKEN_URL "https://login.solaredge.com/oauth2/token"
#define SE_WEB_EXCHANGE_URL "https://monitoring.solaredge.com/services/auth/token?legacy=false"


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

namespace
{
	std::string ExtractNodeIdentity(const Json::Value& node)
	{
		if (!node["serial"].empty())
			return node["serial"].asString();
		const Json::Value& props = node["properties"];
		if (!props.empty() && !props["identifier"].empty())
			return props["identifier"].asString();
		if (!node["uuid"].empty())
			return node["uuid"].asString();
		return "";
	}

	bool IsInactiveNode(const Json::Value& node)
	{
		const Json::Value& props = node["properties"];
		if (props.empty())
			return false;
		return props.get("status", "").asString() == "INACTIVE";
	}

	int LastHttpStatusCode(const std::vector<std::string>& vHeaderData)
	{
		int iStatusCode = 0;
		for (const auto& line : vHeaderData)
		{
			if (line.compare(0, 5, "HTTP/") != 0)
				continue;
			size_t pos = line.find(' ');
			if (pos != std::string::npos)
				iStatusCode = atoi(line.c_str() + pos + 1);
		}
		return iStatusCode;
	}

	bool HeaderNameIs(const std::string& line, const std::string& headerName)
	{
		if (line.size() < headerName.size())
			return false;
		for (size_t i = 0; i < headerName.size(); i++)
		{
			if (tolower((unsigned char)line[i]) != tolower((unsigned char)headerName[i]))
				return false;
		}
		return true;
	}

	// Scans every captured redirect hop for a Location header carrying an OAuth "code" parameter
	std::string ExtractLocationCode(const std::vector<std::string>& vHeaderData)
	{
		for (const auto& line : vHeaderData)
		{
			if (!HeaderNameIs(line, "location:"))
				continue;
			size_t codePos = line.find("code=");
			if (codePos == std::string::npos)
				continue;
			size_t valueStart = codePos + 5;
			size_t valueEnd = line.find_first_of("&\r\n", valueStart);
			if (valueEnd == std::string::npos)
				return line.substr(valueStart);
			return line.substr(valueStart, valueEnd - valueStart);
		}
		return "";
	}
}

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

	LoadWebRefreshToken();
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
	// Layout: every 2 hours (also provides site/inverter/string/optimizer structure)
	// Optimizer playback: every 10 minutes (1 call for all optimizers, daylight only)
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

		bool bNowInDaylight = isDaylightWindow();
		if (m_bWasInDaylightWindow && !bNowInDaylight)
			ResetPowerValues();
		m_bWasInDaylightWindow = bNowInDaylight;

		// Web portal polling (requires web credentials)
		bool bWebCredentials = !m_WebUsername.empty() && !m_WebPassword.empty();

		// Web portal: refresh site layout every 2 hours (site/inverter/string/optimizer structure)
		// Need either a configured Web Site ID or an API-discovered m_SiteID
		if (bWebCredentials && !m_WebSiteID.empty() && layout_timer >= LAYOUT_INTERVAL)
		{
			layout_timer = 0;
			GetSiteLayout();
		}

		// Web portal: poll optimizer power data every 10 minutes
		// This makes 1 HTTP call for all optimizers, so gated by the Poll Optimizers setting
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
		return false;
	if (ActHourMin - 60 > sunSet)
		return false;
	return true;
}

void SolarEdgeAPI::ResetPowerValues()
{
	Log(LOG_STATUS, "Daylight window ended, resetting power/current values to zero");
	char szTmp[100];

	SendKwhMeter(1, 1, 255, 0, m_totalEnergy / 1000.0, "kWh Meter Total");

	for (int i = 0; i < (int)m_inverters.size(); i++)
	{
		sprintf(szTmp, "kWh Meter %s", m_inverters[i].name.c_str());
		double energy = (i < (int)m_lastInverterEnergy.size()) ? m_lastInverterEnergy[i] : 0.0;
		SendKwhMeter(0, 1 + i, 255, 0, energy / 1000.0, szTmp);

		// No need to reset the next values, API returns 0 before daylight window ends
		//for (int ii = 0; ii < 3; ii++)
		//{
		//	int iPhase = ii + 1;
		//	sprintf(szTmp, "acCurrent L%d %s", iPhase, m_inverters[i].name.c_str());
		//	SendCustomSensor(i, SE_AC_CURRENT + ii, 255, 0, szTmp, "A");
		//	sprintf(szTmp, "Power L%d %s", iPhase, m_inverters[i].name.c_str());
		//	SendWattMeter(1 + i, iPhase, 255, 0, szTmp);
		//}
	}

	SendWattMeter(200, SE_OVERVIEW_CURRENT, 255, 0, "Site Current Power");
	SendWattMeter(200, SE_GRID, 255, 0, "Grid Power");
	SendWattMeter(200, SE_LOAD, 255, 0, "Load Power");
	SendWattMeter(200, SE_PV, 255, 0, "PV Power");
	if (m_bPollBattery)
		SendWattMeter(200, SE_STORAGE_POWER, 255, 0, "Battery Power");

	if (m_bPollOptimizers) // only reset optimizer values if we are polling them, otherwise they may create unnecessary devices.
	{
		for (const auto& opt : m_optimizers)
		{
			sprintf(szTmp, "%s Power", opt.displayName.c_str());
			SendWattMeter(opt.nodeId, SE_OPT_POWER, 255, 0, szTmp);
		}
		for (const auto& str : m_webStrings)
		{
			sprintf(szTmp, "%s Power", str.displayName.c_str());
			SendWattMeter(str.nodeId, SE_WEB_POWER, 255, 0, szTmp);
		}
		for (const auto& inv : m_webInverters)
		{
			sprintf(szTmp, "%s Power", inv.displayName.c_str());
			SendWattMeter(inv.nodeId, SE_WEB_POWER, 255, 0, szTmp);
		}
	}
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
	m_lastInverterEnergy.assign(m_inverters.size(), 0.0);
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
		if (iInverterNumber < (int)m_lastInverterEnergy.size())
			m_lastInverterEnergy[iInverterNumber] = curEnergy;
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

bool SolarEdgeAPI::LoadWebRefreshToken()
{
	auto result = m_sql.safe_query("SELECT Address, SerialPort FROM Hardware WHERE (ID==%d)", m_HwdID);
	if (result.empty())
		return false;
	m_WebRefreshToken = result[0][0];
	if (!result[0][1].empty())
		m_WebNextRefreshTs = std::stol(result[0][1]);
	return !m_WebRefreshToken.empty();
}

void SolarEdgeAPI::StoreWebRefreshToken()
{
	if (m_WebRefreshToken.empty())
		return;
	m_sql.safe_query("UPDATE Hardware SET Address='%q', SerialPort='%q' WHERE (ID == %d)", m_WebRefreshToken.c_str(), std::to_string(m_WebNextRefreshTs).c_str(), m_HwdID);
}

std::string SolarEdgeAPI::GetCookieValue(const std::string& name) const
{
	extern std::string szUserDataFolder;
	std::string sPath = szUserDataFolder + "domocookie.txt";
	std::ifstream file(sPath);
	if (!file.is_open())
		return "";
	std::string sLine;
	while (std::getline(file, sLine))
	{
		if (sLine.empty() || sLine[0] == '#')
			continue;
		std::vector<std::string> fields;
		StringSplit(sLine, "\t", fields);
		if (fields.size() < 7)
			continue;
		if (fields[5] == name)
			return fields[6];
	}
	return "";
}

bool SolarEdgeAPI::ParseLoginForm(const std::string& html, std::string& formAction, std::map<std::string, std::string>& fields) const
{
	static const std::regex formRegex("<form[^>]*action=\"([^\"]*)\"", std::regex::icase);
	std::smatch formMatch;
	if (!std::regex_search(html, formMatch, formRegex))
		return false;
	formAction = formMatch[1].str();

	// Unescape the only HTML entity that realistically shows up in a form action URL
	size_t pos = 0;
	while ((pos = formAction.find("&amp;", pos)) != std::string::npos)
	{
		formAction.replace(pos, 5, "&");
		pos += 1;
	}

	static const std::regex inputRegex("<input[^>]*>", std::regex::icase);
	static const std::regex nameRegex("name=\"([^\"]*)\"", std::regex::icase);
	static const std::regex valueRegex("value=\"([^\"]*)\"", std::regex::icase);

	auto inputsBegin = std::sregex_iterator(html.begin(), html.end(), inputRegex);
	auto inputsEnd = std::sregex_iterator();
	for (auto it = inputsBegin; it != inputsEnd; ++it)
	{
		std::string tag = it->str();
		std::smatch nameMatch;
		if (!std::regex_search(tag, nameMatch, nameRegex))
			continue;
		std::string value;
		std::smatch valueMatch;
		if (std::regex_search(tag, valueMatch, valueRegex))
			value = valueMatch[1].str();
		fields[nameMatch[1].str()] = value;
	}

	return !formAction.empty();
}

bool SolarEdgeAPI::WebExchangeSession(const std::string& tokenJsonBody)
{
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: Bearer " + m_WebAccessToken);
	ExtraHeaders.push_back("Content-Type: application/json");

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_WEB_EXCHANGE_URL, tokenJsonBody, ExtraHeaders, sResult, vHeaderData, true, true);

	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Session exchange status %d", iStatusCode);
	return (iStatusCode >= 200 && iStatusCode < 300);
}

bool SolarEdgeAPI::WebRefreshToken()
{
	if (m_WebRefreshToken.empty())
		return false;

	std::string httpData = "grant_type=refresh_token";
	httpData += "&client_id=" SE_WEB_CLIENT_ID;
	httpData += "&refresh_token=" + CURLEncode::URLEncode(m_WebRefreshToken);

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	std::string sResult;
	std::vector<std::string> vHeaderData;
	HTTPClient::POST(SE_WEB_TOKEN_URL, httpData, ExtraHeaders, sResult, vHeaderData);

	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Token refresh status %d", iStatusCode);

	Json::Value root;
	// The refresh response only carries a new access_token, the existing refresh token stays valid
	if (iStatusCode != 200 || !ParseJSon(sResult, root) || !root.isObject() || root["access_token"].empty() || root["expires_in"].empty())
	{
		Log(LOG_ERROR, "Web portal: Failed to refresh access token, a new login will be attempted");
		m_WebAccessToken.clear();
		return false;
	}

	m_WebAccessToken = root["access_token"].asString();
	if (!root["refresh_token"].empty())
		m_WebRefreshToken = root["refresh_token"].asString();
	int expiresIn = root["expires_in"].asInt();
	int refreshIn = (expiresIn * 2) / 3;
	if (refreshIn < 30)
		refreshIn = 30; // never schedule the next refresh in the past
	m_WebNextRefreshTs = mytime(nullptr) + refreshIn;
	StoreWebRefreshToken();

	// The session exchange rejects a body without a refresh token, and the refresh response omits it
	root["refresh_token"] = m_WebRefreshToken;

	if (!WebExchangeSession(JSonToRawString(root)))
	{
		Log(LOG_ERROR, "Web portal: Session exchange failed after token refresh!");
		return false;
	}

	Debug(DEBUG_HARDWARE, "Web portal: Token refresh succeeded");
	return true;
}

bool SolarEdgeAPI::WebLogin()
{
	if (m_WebUsername.empty() || m_WebPassword.empty())
	{
		Log(LOG_ERROR, "Web portal: No web username/password configured!");
		return false;
	}

	unsigned char verifierBytes[32];
	if (RAND_bytes(verifierBytes, sizeof(verifierBytes)) != 1)
	{
		Log(LOG_ERROR, "Web portal: Failed to generate PKCE code verifier!");
		return false;
	}
	std::string codeVerifier = base64url_encode_buf(verifierBytes, sizeof(verifierBytes));
	std::string codeChallenge = base64url_encode(sha256raw(codeVerifier));

	std::stringstream sAuthorizeURL;
	sAuthorizeURL << SE_WEB_AUTHORIZE_URL
		<< "?client_id=" SE_WEB_CLIENT_ID
		<< "&response_type=code"
		<< "&redirect_uri=" << CURLEncode::URLEncode(SE_WEB_REDIRECT_URI)
		<< "&code_challenge=" << CURLEncode::URLEncode(codeChallenge)
		<< "&code_challenge_method=S256";

	std::vector<std::string> authorizeHeaders;
	authorizeHeaders.push_back("Accept: text/html");

	std::string sFormResult;
	std::vector<std::string> vAuthorizeHeaderData;
	// Rely on the process-wide cookie jar so the login session carries through to the form POST below
	HTTPClient::GET(sAuthorizeURL.str(), authorizeHeaders, sFormResult, vAuthorizeHeaderData);

	int iAuthorizeStatus = LastHttpStatusCode(vAuthorizeHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Authorize status %d", iAuthorizeStatus);
	if (iAuthorizeStatus != 200 || sFormResult.empty())
	{
		Log(LOG_ERROR, "Web portal: Error requesting login form!");
		return false;
	}

	// With a valid SSO session the authorize call redirects straight to the callback and there
	// is no login form to submit, so take the code from the redirect chain when it is already there
	std::string authCode = ExtractLocationCode(vAuthorizeHeaderData);
	if (!authCode.empty())
		Debug(DEBUG_HARDWARE, "Web portal: Reused existing session, authorization code received");

	if (authCode.empty())
	{
		std::string formAction;
		std::map<std::string, std::string> formFields;
		if (!ParseLoginForm(sFormResult, formAction, formFields))
		{
			Log(LOG_ERROR, "Web portal: Could not find login form in authorize response!");
			return false;
		}

		if (formAction.compare(0, 4, "http") != 0)
		{
			if (formAction.empty() || formAction[0] != '/')
				formAction = "/" + formAction;
			formAction = "https://login.solaredge.com" + formAction;
		}

		static const std::regex hostRegex("^https?://([^/:]+)", std::regex::icase);
		std::smatch hostMatch;
		std::string host;
		if (std::regex_search(formAction, hostMatch, hostRegex))
			host = hostMatch[1].str();
		std::transform(host.begin(), host.end(), host.begin(), ::tolower);
		bool bHostOk = (host == "solaredge.com") || (host.size() > 14 && host.compare(host.size() - 14, 14, ".solaredge.com") == 0);
		if (!bHostOk)
		{
			Log(LOG_ERROR, "Web portal: Login form action does not point to a solaredge.com host, aborting for safety!");
			return false;
		}

		formFields["username"] = m_WebUsername;
		formFields["password"] = m_WebPassword;

		std::string postData;
		for (const auto& field : formFields)
		{
			if (!postData.empty())
				postData += "&";
			postData += CURLEncode::URLEncode(field.first) + "=" + CURLEncode::URLEncode(field.second);
		}

		std::vector<std::string> loginHeaders;
		loginHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

		std::string sLoginResult;
		std::vector<std::string> vLoginHeaderData;
		HTTPClient::POST(formAction, postData, loginHeaders, sLoginResult, vLoginHeaderData, true, true);

		authCode = ExtractLocationCode(vLoginHeaderData);
		Debug(DEBUG_HARDWARE, "Web portal: Login form submitted, authorization code %s", authCode.empty() ? "not found" : "received");
		if (authCode.empty())
		{
			Log(LOG_ERROR, "Web portal: Could not extract authorization code, check web username/password!");
			return false;
		}
	}

	std::string tokenData = "grant_type=authorization_code";
	tokenData += "&client_id=" SE_WEB_CLIENT_ID;
	tokenData += "&redirect_uri=" + CURLEncode::URLEncode(SE_WEB_REDIRECT_URI);
	tokenData += "&code=" + CURLEncode::URLEncode(authCode);
	tokenData += "&code_verifier=" + CURLEncode::URLEncode(codeVerifier);

	std::vector<std::string> tokenHeaders;
	tokenHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	std::string sTokenResult;
	std::vector<std::string> vTokenHeaderData;
	HTTPClient::POST(SE_WEB_TOKEN_URL, tokenData, tokenHeaders, sTokenResult, vTokenHeaderData);

	int iTokenStatus = LastHttpStatusCode(vTokenHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Token exchange status %d", iTokenStatus);

	Json::Value tokenRoot;
	if (iTokenStatus != 200 || !ParseJSon(sTokenResult, tokenRoot) || !tokenRoot.isObject()
		|| tokenRoot["access_token"].empty() || tokenRoot["refresh_token"].empty() || tokenRoot["expires_in"].empty())
	{
		Log(LOG_ERROR, "Web portal: Failed to obtain an access token!");
		return false;
	}

	m_WebAccessToken = tokenRoot["access_token"].asString();
	m_WebRefreshToken = tokenRoot["refresh_token"].asString();
	int expiresIn = tokenRoot["expires_in"].asInt();
	int refreshIn = (expiresIn * 2) / 3;
	if (refreshIn < 30)
		refreshIn = 30; // never schedule the next refresh in the past
	m_WebNextRefreshTs = mytime(nullptr) + refreshIn;
	StoreWebRefreshToken();

	if (!WebExchangeSession(sTokenResult))
	{
		Log(LOG_ERROR, "Web portal: Session exchange failed after login!");
		return false;
	}

	Log(LOG_STATUS, "Web portal: Login succeeded");
	return true;
}

bool SolarEdgeAPI::WebEnsureLoggedIn()
{
	if (!m_WebAccessToken.empty() && (mytime(nullptr) - 15) < m_WebNextRefreshTs)
		return true;

	if (!m_WebRefreshToken.empty())
	{
		if (WebRefreshToken())
			return true;
		Log(LOG_ERROR, "Web portal: Refresh token failed, a full login will be attempted");
	}

	return WebLogin();
}

bool SolarEdgeAPI::GetLayoutFromAPI(Json::Value& json_output)
{
	std::string sResult;

#ifdef DEBUG_SolarEdgeAPIR
	sResult = ReadFile("E:\\SolarEdge_web_layout.json");
#else
	if (m_WebSiteID.empty())
	{
		Log(LOG_ERROR, "Web portal: No Site ID available! Configure Site ID or enable API polling.");
		return false;
	}

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: Bearer " + m_WebAccessToken);
	ExtraHeaders.push_back("Accept: application/json");

	std::stringstream sURL;
	sURL << "https://monitoring.solaredge.com/services/layout/logical/generic/v2/site/" << m_WebSiteID << "?include-optimizers=true";

	std::vector<std::string> vHeaderData;
	bool bOK = HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData);
	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Layout URL %s status %d", sURL.str().c_str(), iStatusCode);
	if (!bOK)
	{
		std::string sStatusLine = !vHeaderData.empty() ? vHeaderData[0] : "no response";
		Log(LOG_ERROR, "Web portal: Error getting site layout! (%s)", sStatusLine.c_str());
		return false;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_web_layout.json");
#endif
#endif

	if (!ParseJSon(sResult, json_output) || !json_output.isObject())
	{
		Log(LOG_ERROR, "Web portal: Invalid JSON in site layout response!");
		return false;
	}
	if (json_output["siteStructure"].empty())
	{
		Log(LOG_ERROR, "Web portal: No siteStructure in site layout response!");
		return false;
	}

	return true;
}

void SolarEdgeAPI::WalkLayoutNode(const Json::Value& node, const std::string& inverterName, int inverterNodeId, int stringNodeId, int& inverterIndex, int& stringIndex, int& optimizerNodeBase)
{
	if (!node.isObject())
		return;
	if (IsInactiveNode(node))
		return;

	std::string type = node.get("type", "").asString();
	std::string name = node.get("name", "").asString();
	std::string identity = ExtractNodeIdentity(node);

	std::string curInverterName = inverterName;
	int curInverterNodeId = inverterNodeId;
	int curStringNodeId = stringNodeId;

	if (type == "INVERTER")
	{
		_tWebNodeInfo info;
		info.reporterId = identity;
		info.displayName = name;
		info.nodeId = SE_WEB_INVERTER_BASE + inverterIndex++;
		m_webInverters.push_back(info);
		curInverterName = name;
		curInverterNodeId = info.nodeId;
	}
	else if (type == "STRING")
	{
		_tWebNodeInfo info;
		info.reporterId = identity;
		// The v2 layout already names strings "String x.y", so only add the prefix when it is missing
		info.displayName = (name.find("String") == 0) ? name : "String " + name;
		info.nodeId = SE_WEB_STRING_BASE + stringIndex++;
		m_webStrings.push_back(info);
		curStringNodeId = info.nodeId;
	}
	else if (type == "OPTIMIZER")
	{
		_tOptimizerInfo info;
		info.reporterId = identity;
		info.serialNumber = node.get("serial", "").asString();
		info.displayName = name;
		info.inverterName = curInverterName;
		info.stringNodeId = curStringNodeId;
		info.inverterNodeId = curInverterNodeId;
		info.nodeId = optimizerNodeBase++;
		m_optimizers.push_back(info);
		return; // optimizers are leaves
	}

	const Json::Value& children = node["children"];
	if (!children.isArray())
		return;
	for (const auto& child : children)
		WalkLayoutNode(child, curInverterName, curInverterNodeId, curStringNodeId, inverterIndex, stringIndex, optimizerNodeBase);
}

bool SolarEdgeAPI::GetSiteLayout()
{
	if (!WebEnsureLoggedIn())
		return false;

	Json::Value root;
	if (!GetLayoutFromAPI(root))
		return false;

	m_optimizers.clear();
	m_webInverters.clear();
	m_webStrings.clear();

	int inverterIndex = 0;
	int stringIndex = 0;
	int optimizerNodeBase = 300;

	const Json::Value& siteNode = root["siteStructure"];
	WalkLayoutNode(siteNode, "", -1, -1, inverterIndex, stringIndex, optimizerNodeBase);

	Log(LOG_STATUS, "Web portal: Discovered %d inverters, %d strings, %d optimizers",
		(int)m_webInverters.size(), (int)m_webStrings.size(), (int)m_optimizers.size());

	return true;
}

void SolarEdgeAPI::GetOptimizerData()
{
	if (m_optimizers.empty())
		return;

	// Check daylight window
	if (!isDaylightWindow())
		return;

	if (!WebEnsureLoggedIn())
		return;

	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	char szStart[40];
	char szEnd[40];
	snprintf(szStart, sizeof(szStart), "%04d-%02d-%02dT00:00:00Z", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
	snprintf(szEnd, sizeof(szEnd), "%04d-%02d-%02dT%02d:%02d:%02dZ", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	std::string startDate = CURLEncode::URLEncode(szStart);
	std::string endDate = CURLEncode::URLEncode(szEnd);

	std::stringstream sURL;
	sURL << "https://monitoring.solaredge.com/services/layout/playback/site/" << m_WebSiteID << "/optimizers-compact"
		<< "?resolution=hours&start-date=" << startDate << "&end-date=" << endDate;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: Bearer " + m_WebAccessToken);
	ExtraHeaders.push_back("Accept: application/json");
	std::string csrfToken = GetCookieValue("CSRF-TOKEN");
	if (!csrfToken.empty())
		ExtraHeaders.push_back("X-CSRF-TOKEN: " + csrfToken);

	std::string sResult;
	std::vector<std::string> vHeaderData;
	bool bOK = HTTPClient::GET(sURL.str(), ExtraHeaders, sResult, vHeaderData);
	int iStatusCode = LastHttpStatusCode(vHeaderData);
	Debug(DEBUG_HARDWARE, "Web portal: Playback URL %s status %d", sURL.str().c_str(), iStatusCode);
	if (!bOK)
	{
		Log(LOG_ERROR, "Web portal: Error getting optimizer playback data! (status %d)", iStatusCode);
		return;
	}

	Json::Value root;
	if (!ParseJSon(sResult, root) || !root.isObject())
	{
		Log(LOG_ERROR, "Web portal: Invalid JSON in optimizer playback response!");
		return;
	}

	const Json::Value& serials = root["optimizerSerials"];
	const Json::Value& compressPowerData = root["compressPowerData"];
	if (!serials.isArray() || !compressPowerData.isArray())
	{
		Log(LOG_ERROR, "Web portal: Missing optimizerSerials/compressPowerData in playback response!");
		return;
	}

	int timeSlotsCount = root["timeSlotsCount"].asInt();
	size_t serialCount = serials.size();
	size_t headerLen = 2 + (2 * serialCount);
	if (compressPowerData.size() <= headerLen || timeSlotsCount <= 0)
	{
		Debug(DEBUG_HARDWARE, "Web portal: Playback response carries no measurements (serials %d, slots %d)", (int)serialCount, timeSlotsCount);
		return;
	}

	int dataStartIdx = compressPowerData[1].asInt();
	if (dataStartIdx < 0 || (size_t)dataStartIdx >= compressPowerData.size())
	{
		Log(LOG_ERROR, "Web portal: Invalid playback data start index %d!", dataStartIdx);
		return;
	}

	std::map<std::string, float> powerByShortSerial;
	for (size_t i = 0; i < serialCount; i++)
	{
		std::string shortSerial = serials[(Json::ArrayIndex)i].asString();
		int offset = compressPowerData[(Json::ArrayIndex)(3 + (i * 2))].asInt();
		if (offset < 0)
			continue;

		float value = 0;
		for (int s = timeSlotsCount - 1; s >= 0; s--)
		{
			size_t idx = (size_t)dataStartIdx + (size_t)offset + (size_t)s;
			if (idx >= compressPowerData.size())
				continue;
			value = compressPowerData[(Json::ArrayIndex)idx].asFloat();
			if (value != 0)
				break;
		}
		powerByShortSerial[shortSerial] = value;
	}

	Debug(DEBUG_HARDWARE, "Web portal: Playback decoded %d optimizer serials, %d slots", (int)serialCount, timeSlotsCount);

	std::map<int, double> stringPower;
	std::map<int, double> inverterPower;
	char szTmp[200];

	for (const auto& opt : m_optimizers)
	{
		std::string shortSerial = opt.serialNumber.substr(0, opt.serialNumber.find('-'));
		auto it = powerByShortSerial.find(shortSerial);
		if (it == powerByShortSerial.end())
			continue;

		float power = it->second;
		snprintf(szTmp, sizeof(szTmp), "%s Power", opt.displayName.c_str());
		SendWattMeter(opt.nodeId, SE_OPT_POWER, 255, power, szTmp);

		if (opt.stringNodeId >= 0)
			stringPower[opt.stringNodeId] += power;
		if (opt.inverterNodeId >= 0)
			inverterPower[opt.inverterNodeId] += power;
	}

	for (const auto& str : m_webStrings)
	{
		auto it = stringPower.find(str.nodeId);
		if (it == stringPower.end())
			continue;
		snprintf(szTmp, sizeof(szTmp), "%s Power", str.displayName.c_str());
		SendWattMeter(str.nodeId, SE_WEB_POWER, 255, (float)it->second, szTmp);
	}

	for (const auto& inv : m_webInverters)
	{
		auto it = inverterPower.find(inv.nodeId);
		if (it == inverterPower.end())
			continue;
		snprintf(szTmp, sizeof(szTmp), "%s Power", inv.displayName.c_str());
		SendWattMeter(inv.nodeId, SE_WEB_POWER, 255, (float)it->second, szTmp);
	}
}
