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
#include <iostream>

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

SolarEdgeAPI::SolarEdgeAPI(const int ID, const std::string& APIKey) :
	m_APIKey(APIKey)
{
	m_SiteID = 0;
	m_HwdID = ID;
	m_totalActivePower = 0;
	m_totalEnergy = 0;
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
	int sec_counter = 295;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % 300 == 0)
		{
			if (m_SiteID == 0)
			{
				if (!GetSite())
					continue;
				GetInverters();
			}
			if (!m_inverters.empty())
				GetMeterDetails();
			if (m_bPollBattery)
				GetBatteryDetails();
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
	return true;
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
	//atime = (atime / 300) * 300;
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	//We only poll one hour before sunrise till one hour after sunset
	if (ActHourMin + 60 < sunRise)
		return;
	if (ActHourMin - 60 > sunSet)
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
		m_bPollBattery = false;
		//Log(LOG_ERROR, "Invalid data received, or invalid APIKey");
		return;
	}
	root = root["siteCurrentPowerFlow"];

	std::string status;
	float power;

	if (!root["GRID"].empty())
	{
		status = root["GRID"]["status"].asString();
		if (status == "Active")
		{
			power = root["GRID"]["currentPower"].asFloat();
			SendWattMeter(200, SE_GRID, 255, power * 1000, "Grid Power");
		}
	}
	if (!root["LOAD"].empty())
	{
		status = root["LOAD"]["status"].asString();
		if (status == "Active")
		{
			power = root["LOAD"]["currentPower"].asFloat();
			SendWattMeter(200, SE_LOAD, 255, power * 1000, "Load Power");
		}
	}
	if (!root["PV"].empty())
	{
		status = root["PV"]["status"].asString();
		if (status == "Active")
		{
			power = root["PV"]["currentPower"].asFloat();
			SendWattMeter(200, SE_PV, 255, power * 1000, "PV Power");
		}
	}
	if (!root["STORAGE"].empty())
	{
		status = root["STORAGE"]["status"].asString();
		SendTextSensor(200, SE_STORAGE_STATUS, 255, status, "Battery Status");

		power = root["STORAGE"]["currentPower"].asFloat();

		if (status == "Discharging")
		{
			if (power > 0)
				power = -power;
		}

		SendWattMeter(200, SE_STORAGE_POWER, 255, power * 1000, "Battery Power");

		float chargeLevel = root["STORAGE"]["chargeLevel"].asFloat();
		SendPercentageSensor(200, SE_STORAGE_CHARGELEVEL, 255, chargeLevel, "Battery Charge Level");

		bool battertCritical = root["STORAGE"]["critical"].asFloat();
		SendSwitch(200, SE_STORAGE_CRITITAL, 255, battertCritical, 0, "Battery Critical", "SolarEdge");
	}
}

