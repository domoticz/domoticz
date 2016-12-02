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

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_SolarEdgeAPI

#define SE_VOLT_AC 1
#define SE_VOLT_DC 2
#define SE_FREQ 1

#ifdef _DEBUG
//	#define DEBUG_SolarEdgeAPIR
//	#define DEBUG_SolarEdgeAPIW
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

SolarEdgeAPI::SolarEdgeAPI(const int ID, const int SiteID, const std::string &Serial, const std::string &APIKey):
	m_Serial(Serial),
	m_APIKey(APIKey)
{
	m_HwdID=ID;
	m_SiteID = SiteID;
	m_stoprequested=false;
	Init();
}

SolarEdgeAPI::~SolarEdgeAPI(void)
{
}

void SolarEdgeAPI::Init()
{
	m_LastUsage=0;
	m_LastCounter=0;
	m_LastTemp=0;
	m_LastAC=0;
	m_LastDC=0;
	m_LastHz=0;
}

bool SolarEdgeAPI::StartHardware()
{
	Init();
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

void SolarEdgeAPI::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_SolarEdgeAPIR
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
	constructTime(atime_min10,ltime_min10,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min-10,ltime.tm_sec,ltime.tm_isdst);

	char szTmp[100];
	sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d", ltime_min10.tm_year + 1900, ltime_min10.tm_mon + 1, ltime_min10.tm_mday, ltime_min10.tm_hour, ltime_min10.tm_min, ltime_min10.tm_sec);
	std::string startDate= CURLEncode::URLEncode(szTmp);

	sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	std::string endDate = CURLEncode::URLEncode(szTmp);
	std::stringstream sURL;
	sURL << "https://monitoringapi.solaredge.com/equipment/" << m_SiteID << "/" << m_Serial << "/data.json?startTime=" << startDate << "&endTime=" << endDate << "&api_key=" << m_APIKey;
	bool bret;
	std::string szURL = sURL.str();
	bret = HTTPClient::GET(szURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Error getting http data!");
		return;
	}
#ifdef DEBUG_SolarEdgeAPIW
	SaveString2Disk(sResult, "E:\\SolarEdge2.json");
#endif
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"SolarEdgeAPI: Invalid data received!");
		return;
	}
	if (root["data"].empty()==true)
	{
		_log.Log(LOG_ERROR,"SolarEdgeAPI: Invalid data received, or invalid SiteID/Serial/APIKey");
		return;
	}
	if (root["data"]["count"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid SiteID/Serial/APIKey");
		return;
	}
	int tot_results = root["data"]["count"].asInt();
	if (tot_results < 1)
		return;
	if (root["data"]["telemetries"].empty() == true)
	{
		_log.Log(LOG_ERROR, "SolarEdgeAPI: Invalid data received, or invalid SiteID/Serial/APIKey");
		return;
	}
	Json::Value reading = root["data"]["telemetries"][0];
	if ((!reading["totalActivePower"].empty()) && (!reading["totalEnergy"].empty()))
	{
		double totalActivePower = reading["totalActivePower"].asDouble();
		double totalEnergy = reading["totalEnergy"].asDouble();
		if (totalEnergy != 0)
		{
			SendKwhMeter(0, 1, 255, totalActivePower, totalEnergy / 1000.0, "kWh Meter");
		}
	}
	if (!reading["dcVoltage"].empty())
	{
		float dcVoltage = reading["dcVoltage"].asFloat();
		SendVoltageSensor(0, SE_VOLT_DC, 255, dcVoltage, "DC");
	}
	if (!reading["temperature"].empty())
	{
		float temp = reading["temperature"].asFloat();
		SendTempSensor(1, 255, temp, "SolarMain");
	}
	if (!reading["L1Data"].empty())
	{
		if (!reading["L1Data"]["acVoltage"].empty())
		{
			float acVoltage = reading["L1Data"]["acVoltage"].asFloat();
			SendVoltageSensor(0, SE_VOLT_AC, 255, acVoltage, "AC");
		}
		if (!reading["L1Data"]["acFrequency"].empty())
		{
			float acFrequency = reading["L1Data"]["acFrequency"].asFloat();
			SendPercentageSensor(1, SE_FREQ, 255, acFrequency, "Hz");
		}
	}
}

