#include "stdafx.h"
#include "GoodweAPI.h"
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

//#define DEBUG_GoodweAPI 1

#define GOODWE_URL "http://www.goodwe-power.com/Mobile/GetMyPowerStationByUser?userName="

// parameter names for GetMyPowerStationByUser

#define BY_USER_STATION_ID "stationId"
#define BY_USER_STATION_NAME "stationName"
#define BY_USER_CURRENT_POWER_KW "currentPower"
#define BY_USER_DAY_TOTAL_KWH "value_eDayTotal"
#define BY_USER_TOTAL_KWH "value_eTotal"


#ifdef _DEBUG
//	#define DEBUG_GoodweAPIR
#endif

#define DEBUG_GoodweAPIW 1

#ifdef DEBUG_GoodweAPIW
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
#ifdef DEBUG_GoodweAPIR
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

GoodweAPI::GoodweAPI(const int ID, const std::string &userName):
	m_UserName(userName)
{
	m_HwdID=ID;
	m_stoprequested=false;
	Init();
}

GoodweAPI::~GoodweAPI(void)
{
}

void GoodweAPI::Init()
{
	m_LastUsage=0;
	m_LastCounter=0;
}

bool GoodweAPI::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&GoodweAPI::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool GoodweAPI::StopHardware()
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

void GoodweAPI::Do_Work()
{
	_log.Log(LOG_STATUS, "GoodweAPI Worker started...");
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
	_log.Log(LOG_STATUS,"GoodweAPI Worker stopped...");
}

bool GoodweAPI::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

int GoodweAPI::getSunRiseSunSetMinutes(const bool bGetSunRise)
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

void GoodweAPI::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_GoodweAPIR
	sResult = ReadFile("/tmp/Goodwe.json");
#else
	time_t atime = mytime(NULL);
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

	std::stringstream sURL;
	sURL << GOODWE_URL << m_UserName;
	bool bret;
	std::string szURL = sURL.str();

	bret = HTTPClient::GET(szURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Error getting http data!");
		return;
	}
#ifdef DEBUG_GoodweAPIW
	SaveString2Disk(sResult, "/tmp/Goodwe2.json");
#endif
#endif
	Json::Value root;
	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"GoodweAPI: Invalid data received!");
		return;
	}
	if (root.size() < 1)
	{
		_log.Log(LOG_ERROR,"GoodweAPI: Invalid data received, or invalid username");
		return;
	}
	for (int i = 0; i < root.size(); i++)
	{
		ParseData(root[i]);
	}
}

int GoodweAPI::hash(const char *str)
{
	/* 
	 * We need a way to generate the ChildID from the stationID
	 * This hash is definitely not perfect as we reduce the 128 bit
         * stationID to an int (normally 32 bits).
	 * But as almost all people will have only a very limited number of
	 * PV-converters, the risk for collisions should be low enough
	 * The djb2 hash is taken from http://www.cse.yorku.ca/~oz/hash.html
	 */

	long hash = 5381;
	int c;
	
	while(c = *str++)
	{
		hash = ((hash << 5) + hash) + c;
	}
	return (int)hash;
}

void GoodweAPI::ParseData(Json::Value object)
{
	if (object[BY_USER_STATION_ID].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: no or invalid data received - StationID is missing!");
		return;
	}
	if (object[BY_USER_STATION_NAME].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid data received - stationName is missing!");
		return;
	}
	if (object[BY_USER_CURRENT_POWER_KW].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid data received - currentPower is missing!");
		return;
	}

	if (object[BY_USER_DAY_TOTAL_KWH].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid data received - value_eDay_Total is missing!");
		 return;
	}

	if (object[BY_USER_TOTAL_KWH].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid data received - value_eTotal is missing!");
		return;
	}

	std::string sStationName = object[BY_USER_STATION_NAME].asString();
	std::string sCurrentPower = object[BY_USER_CURRENT_POWER_KW].asString();
	std::string sTotalKWh = object[BY_USER_TOTAL_KWH].asString();
	int childID = hash(object[BY_USER_STATION_ID].asCString());

	// convert currentPower and totalPower to floats

	float currentPowerKW;
	std::stringstream ssCurrentPower;

	ssCurrentPower << sCurrentPower.substr(0, sCurrentPower.find("kW", 0));
	ssCurrentPower >> currentPowerKW;

	float totalKWh;
	std::stringstream ssTotalKWh;
	ssTotalKWh << sTotalKWh.substr(0, sTotalKWh.find("kWh", 0));
	ssTotalKWh >> totalKWh;	

	// We use the stationname from the Goodwe website as defaultname
	SendKwhMeter(childID, 1, 255, currentPowerKW * 1000, totalKWh, sStationName);
}
