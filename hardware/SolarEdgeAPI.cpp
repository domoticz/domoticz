#include "stdafx.h"
#include "SolarEdgeAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include <iostream>

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_SolarEdgeAPI

#define SE_VOLT_AC 1
#define SE_VOLT_DC 2
#define SE_FREQ 1

#ifdef _DEBUG
	//#define DEBUG_SolarEdgeAPIR_SITE
	//#define DEBUG_SolarEdgeAPIR_INVERTERS
	//#define DEBUG_SolarEdgeAPIR_DETAILS
	//#define DEBUG_SolarEdgeAPIW
#endif

#ifdef DEBUG_SolarEdgeAPIW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#if defined(DEBUG_SolarEdgeAPIR_DETAILS) || defined(DEBUG_SolarEdgeAPIR_SITE) || defined(DEBUG_SolarEdgeAPIR_INVERTERS)
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

SolarEdgeAPI::SolarEdgeAPI(const int ID, const std::string &APIKey):
	m_APIKey(APIKey)
{
	m_SiteID = 0;
	m_HwdID = ID;
	m_stoprequested=false;
	m_totalActivePower = 0;
	m_totalEnergy = 0;
}

SolarEdgeAPI::~SolarEdgeAPI(void)
{
}

bool SolarEdgeAPI::StartHardware()
{
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&SolarEdgeAPI::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool SolarEdgeAPI::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void SolarEdgeAPI::Do_Work()
{
	_log.Log(LOG_STATUS, "SolarEdgeAPI Worker started...");
	int sec_counter = 295;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 300 == 0)
		{
			if (m_SiteID == 0)
				GetSite();
			if (m_SiteID != 0)
			{
				if (m_inverters.empty())
				{
					GetInverters();
				}
			}
			if (!m_inverters.empty())
				GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"SolarEdgeAPI Worker stopped...");
}

bool SolarEdgeAPI::WriteToHardware(const char *pdata, const unsigned char length)
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
		else {
			return sunSetInMinutes;
		}
	}
	return 0;
}

void SolarEdgeAPI::GetSite()
{
	m_SiteID = 0;
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR_SITE
	sResult = ReadFile("E:\\SolarEdge_sites.json");
#else
	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/sites/list?size=1&api_key=" << m_APIKey << "&format=application/json";
	bool bret;
	std::string szURL = sURL.str();
	bret = HTTPClient::GET(szURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Error getting http data!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_sites.json");
#endif
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received!");
		return;
	}
	if (root["sites"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}
	if (root["sites"]["count"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}
	int tot_results = root["sites"]["count"].asInt();
	if (tot_results < 1)
		return;
	Json::Value reading = root["sites"]["site"][0];

	if (reading["id"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}
	m_SiteID = reading["id"].asInt();
}

void SolarEdgeAPI::GetInverters()
{
	m_inverters.clear();
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR_INVERTERS
	sResult = ReadFile("E:\\SolarEdge_inverters.json");
#else
	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/equipment/" << m_SiteID << "/list?api_key=" << m_APIKey << "&format=application/json";
	bool bret;
	std::string szURL = sURL.str();
	bret = HTTPClient::GET(szURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Error getting http data!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge_inverters.json");
#endif
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received!");
		return;
	}
	if (root["reporters"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}
	if (root["reporters"]["count"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
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

void SolarEdgeAPI::GetInverterDetails(const _tInverterSettings *pInverterSettings, const int iInverterNumber)
{
	std::string sResult;
	char szTmp[200];
#ifdef DEBUG_SolarEdgeAPIR_DETAILS
	sResult = ReadFile("E:\\SolarEdge.json");
#else
	time_t atime = mytime(NULL);
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
	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/equipment/" << m_SiteID << "/" << pInverterSettings->SN << "/data.json?startTime=" << startDate << "&endTime=" << endDate << "&api_key=" << m_APIKey << "&format=application/json";
	bool bret;
	std::string szURL = sURL.str();
	bret = HTTPClient::GET(szURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Error getting http data!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge.json");
#endif
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received!");
		return;
	}
	if (root["data"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}
	if (root["data"]["count"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}
	int tot_results = root["data"]["count"].asInt();
	if (tot_results < 1)
		return;
	if (root["data"]["telemetries"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid APIKey");
		return;
	}

	//We could have multiple sites here
	Json::Value reading = root["data"]["telemetries"][0];
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
	if (!reading["temperature"].empty())
	{
		float temp = reading["temperature"].asFloat();
		sprintf(szTmp, "SolarMain %s", pInverterSettings->name.c_str());
		SendTempSensor(1 + iInverterNumber, 255, temp, szTmp);
	}
	if (!reading["L1Data"].empty())
	{
		if (!reading["L1Data"]["acVoltage"].empty())
		{
			float acVoltage = reading["L1Data"]["acVoltage"].asFloat();
			sprintf(szTmp, "AC %s", pInverterSettings->name.c_str());
			SendVoltageSensor(iInverterNumber, SE_VOLT_AC, 255, acVoltage, szTmp);
		}
		if (!reading["L1Data"]["acFrequency"].empty())
		{
			float acFrequency = reading["L1Data"]["acFrequency"].asFloat();
			sprintf(szTmp, "Hz %s", pInverterSettings->name.c_str());
			SendPercentageSensor(1 + iInverterNumber, SE_FREQ, 255, acFrequency, szTmp);
		}
	}
}

