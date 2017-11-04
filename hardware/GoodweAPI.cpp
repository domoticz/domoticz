#include "stdafx.h"
#include <stdio.h>
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
#if defined(_WIN32) || defined(_WIN64) 
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif


//#define DEBUG_GoodweAPI 1

#define GOODWE_BY_USER_URL "http://www.goodwe-power.com/Mobile/GetMyPowerStationByUser?userName="
#define GOODWE_BY_STATION_URL "http://www.goodwe-power.com/Mobile/GetMyPowerStationById?stationId="
#define GOODWE_DEVICE_LIST_URL "http://www.goodwe-power.com/Mobile/GetMyDeviceListById?stationId="

// parameter names for GetMyPowerStationByUser

#define BY_USER_STATION_ID "stationId"
#define BY_USER_STATION_NAME "stationName"
#define BY_USER_CURRENT_POWER_KW "currentPower"
#define BY_USER_DAY_TOTAL_KWH "value_eDayTotal"
#define BY_USER_TOTAL_KWH "value_eTotal"

// parameter names for GetMyPowerStationById

#define BY_STATION_CURRENT_POWER_KW "curpower"
#define BY_STATION_STATUS "status"
#define BY_STATION_DAY_TOTAL_KWH "eday"
#define BY_STATION_TOTAL_KWH "etotal"

// parameter names for GetMyDeviceListById

#define DEVICE_SERIAL "inventersn"
#define DEVICE_CURRENT_POWER_W "power"
#define DEVICE_STATUS "status"
#define DEVICE_DAY_TOTAL_KWH "eday"
#define DEVICE_TOTAL_KWH "etotal"
#define DEVICE_ERROR_MSG "errormsg"


#ifdef _DEBUG
	//#define DEBUG_GoodweAPIW 1
#endif


#ifdef DEBUG_GoodweAPIW
void SaveString2Disk(const std::string &str, const std::string &filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
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


uint32_t GoodweAPI::hash(const std::string &str)
{
	/* 
	 * We need a way to generate the NoddeId from the stationID 
	 * and the ChildID from device serial.
	 * This hash is definitely not perfect as we reduce the 128 bit
         * stationID to an int (normally 32 bits).
	 * But as almost all people will have only a very limited number of
	 * PV-converters, the risk for collisions should be low enough
	 * The djb2 hash is taken from http://www.cse.yorku.ca/~oz/hash.html
	 */

	long hash = 5381;
	size_t i = 0;
	
	for (i = 0; i < str.size(); i++)
	{
		int c = str[i++];
		hash = ((hash << 5) + hash) + c;
	}
	return (uint32_t)hash;
}

float GoodweAPI::getPowerWatt(const std::string &str)
{
	float result;
	std::string units;
	std::stringstream input;
	input << str;
	if (!(input >> result >> units)) {
		_log.Log(LOG_ERROR, "Error parsing %s for power!", str.c_str());
		result = 0;
	} else if (units == "W") {
		/* nothing to do here */
	} else if (units == "kW") {
		result = result * 1000;
	} else {
		_log.Log(LOG_ERROR, "Invalid power units in %s", str.c_str());
		result = 0;
	}	
	return result;
}

		
float GoodweAPI::getEnergyWh(const std::string &str)
{
	float result;
	std::string units;
	std::stringstream input;
	input << str;
	if (!(input >> result >> units)) {
		_log.Log(LOG_ERROR, "Error parsing %s for energy!", str.c_str());
		result = 0;
	} else if (units == "Wh") {
		/* nothing to do here */
	} else if (units == "kWh") {
		result = result * 1000;
	} else {
		_log.Log(LOG_ERROR, "Invalid energy units in %s", str.c_str());
		result = 0;
	}	
	return result;
}

void GoodweAPI::GetMeterDetails()
{
	std::string sResult;
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	// Only poll one hour before sunrise till two hours after sunset

	if (ActHourMin + 60 < sunRise)
		return;
	if (ActHourMin - 120 > sunSet)
		return;

	std::string sURL = GOODWE_BY_USER_URL + m_UserName;

	bool bret = HTTPClient::GET(sURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Error getting http user data!");
		return;
	}
 
#ifdef DEBUG_GoodweAPIW
	SaveString2Disk(sResult, "/tmp/Goodwe2.json");
#endif
	Json::Value root;
	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"GoodweAPI: Invalid user data received!");
		return;
	}
	if (root.size() < 1)
	{
		_log.Log(LOG_ERROR,"GoodweAPI: Invalid user data received, or invalid username");
		return;
	}
	for (Json::ArrayIndex i = 0; i < root.size(); i++)
	{
		// We use the received data only to retrieve station-id and station-name

		if (root[i][BY_USER_STATION_ID].empty() )
		{
			 _log.Log(LOG_ERROR,"GoodweAPI: no or invalid data received - StationID is missing!");
			return;
		}
		if (root[i][BY_USER_STATION_NAME].empty() )
		{
			 _log.Log(LOG_ERROR,"GoodweAPI: invalid data received - stationName is missing!");
			return;
		}
		std::string sStationId = root[i][BY_USER_STATION_ID].asString();
		std::string sStationName = root[i][BY_USER_STATION_NAME].asString();

		// ParseStation is not used for now as there may not be stations with multiple inverters
		// ParseStation(sStationId, sStationName); 

		ParseDeviceList(sStationId, sStationName);
	}
}

void GoodweAPI::ParseStation(const std::string &sStationId, const std::string &sStationName)
{
	// fetch power station details

	std::string sResult;
	std::string sURL = GOODWE_BY_STATION_URL + sStationId;
	bool bret;

	bret = HTTPClient::GET(sURL, sResult);

	if (!bret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Error getting http data for station %s!", sStationName.c_str());
		return;
	}

#ifdef DEBUG_GoodweAPIW
	SaveString2Disk(sResult, "/tmp/Goodwe3.json");
#endif

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Invalid station data received!");
		return;
	}

	// Check completeness of received data

	if (root[BY_STATION_CURRENT_POWER_KW].empty())
	{
		_log.Log(LOG_ERROR, "GoodweAPI: invalid ID data received - curpower is missing!");
		return;
	}

	if (root[BY_STATION_TOTAL_KWH].empty())
	{
		_log.Log(LOG_ERROR, "GoodweAPI: invalid ID data received - etotal is missing!");
		return;
	}

	if (root[BY_STATION_STATUS].empty())
	{
		_log.Log(LOG_ERROR, "GoodweAPI: invalid ID data received - status is missing!");
		return;
	}

	// retrieve strings

	std::string sStatus = root[BY_STATION_STATUS].asString();
	std::string sCurrentPower = root[BY_STATION_CURRENT_POWER_KW].asString();
	std::string sTotalEnergy = root[BY_STATION_TOTAL_KWH].asString();

	// Retrieve the values from the strings

	float currentPowerW = getPowerWatt(sCurrentPower);
	float totalEnergyKWh = getEnergyWh(sTotalEnergy) / 1000;

	// Calcullate NodeID from stationId

	uint32_t NodeID = hash(sStationId);

	// Use the station name from the Goodwe website as defaultname

	SendKwhMeter(NodeID, 1, 255, currentPowerW, totalEnergyKWh, "Station " + sStationName + " Return");
	SendTextSensor(NodeID, 2, 255, sStatus, "Station " + sStationName + " status");
}

void GoodweAPI::ParseDeviceList(const std::string &sStationId, const std::string &sStationName)
{
	// fetch interverter list

	std::string sURL = GOODWE_DEVICE_LIST_URL + sStationId;
	bool bret;
	std::string sResult;

	bret = HTTPClient::GET(sURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Error getting http data for device list !");
		return;
	}
#ifdef DEBUG_GoodweAPIW
	SaveString2Disk(sResult, "/tmp/Goodwe4.json");
#endif

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Invalid device list!");
		return;
	}

	if (root.size() < 1)
	{
		_log.Log(LOG_STATUS, "GoodweAPI: List if devices / devices is empty!");
		return;
	}
	for (Json::ArrayIndex i = 0; i < root.size(); i++)
	{
		ParseDevice(root[i], sStationId, sStationName);
	}
}

void GoodweAPI::ParseDevice(Json::Value device, const std::string &sStationId, const std::string &sStationName)
{
	if (device[DEVICE_SERIAL].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: no or invalid device data received - inverter serial nr is missing!");
		return;
	}
	if (device[DEVICE_CURRENT_POWER_W].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid device data received - power is missing!");
		return;
	}

	if (device[DEVICE_STATUS].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid device data received - status is missing!");
		return;
	}

	if (device[DEVICE_TOTAL_KWH].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid device data received - eTotal is missing!");
		return;
	}

	if (device[DEVICE_ERROR_MSG].empty() )
	{
		 _log.Log(LOG_ERROR,"GoodweAPI: invalid device data received - error message is missing!");
		return;
	}

	// fetch strings

	std::string sDeviceSerial = device[DEVICE_SERIAL].asString();
	std::string sCurrentPower = device[DEVICE_CURRENT_POWER_W].asString();
	std::string sTotalEnergyKWh = device[DEVICE_TOTAL_KWH].asString();
	std::string sErrorMsg = device[DEVICE_ERROR_MSG].asString();
	std::string sStatus = device[DEVICE_STATUS].asString();

	// convert currentPower and totalPower to floats

	float currentPowerW = getPowerWatt(sCurrentPower);
	float totalEnergyKWh = getEnergyWh(sTotalEnergyKWh)/1000;

	// Create NodeID and ChildID from station id and device serial 

	uint32_t NodeID = hash(sStationId);
	uint32_t ChildID = hash(sDeviceSerial);

	// reserve childIDs  0 - 10 for the station
	if (ChildID <= 10) {
		ChildID = ChildID + 10;
	}

	SendKwhMeter(NodeID, ChildID, 255, currentPowerW, totalEnergyKWh, sStationName + " " + sDeviceSerial + " Return");
	SendTextSensor(NodeID, ChildID + 1 , 255, sStatus, sStationName + " " + sDeviceSerial + " status");
	SendTextSensor(NodeID, ChildID + 2 , 255, sErrorMsg, sStationName + " " + sDeviceSerial + " error");
}
