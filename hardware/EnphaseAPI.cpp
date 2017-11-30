#include "stdafx.h"
#include "EnphaseAPI.h"
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

//https://enphase.com/en-us/products-and-services/envoy

#define Enphase_request_INTERVAL 30

#ifdef _DEBUG
//#define DEBUG_EnphaseAPI
#endif

#ifdef DEBUG_EnphaseAPI
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

EnphaseAPI::EnphaseAPI(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_p1power.ID = 1;

	m_HwdID = ID;
	m_stoprequested = false;
}

EnphaseAPI::~EnphaseAPI(void)
{
}

bool EnphaseAPI::StartHardware()
{
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&EnphaseAPI::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

bool EnphaseAPI::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted = false;
	return true;
}

void EnphaseAPI::Do_Work()
{
	_log.Log(LOG_STATUS, "EnphaseAPI Worker started...");
	int sec_counter = Enphase_request_INTERVAL - 5;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % Enphase_request_INTERVAL == 0)
		{
			getProduction();
		}
	}
	_log.Log(LOG_STATUS, "EnphaseAPI Worker stopped...");
}

bool EnphaseAPI::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

int EnphaseAPI::getSunRiseSunSetMinutes(const bool bGetSunRise)
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

void EnphaseAPI::getProduction()
{
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	if (sunRise != 0 && sunSet != 0)
	{
		//We only poll one hour before sunrise till one hour after sunset
		if (ActHourMin + 60 < sunRise)
			return;
		if (ActHourMin - 60 > sunSet)
			return;
	}

	getProductionDetail();
}

void EnphaseAPI::getProductionDetail()
{
	std::string sResult;

#ifdef DEBUG_EnphaseAPI
	sResult = ReadFile("C:\\TEMP\\EnphaseAPI_get_production.txt");
#else
	std::stringstream sURL;
	sURL << "http://" << m_szIPAddress << "/production.json";

	bool bret;
	std::string szURL = sURL.str();
	bret = HTTPClient::GET(szURL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Error getting http data!");
		return;
	}

#endif

	Json::Value root;
	Json::Reader jReader;

	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Invalid data received!");
		return;
	}
	if (root["production"].empty() == true)
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Invalid data received");
		return;
	}

	Json::Value reading = root["production"][0];

	int musage = reading["wNow"].asInt();
	int mtotal = reading["whLifetime"].asInt();

	SendKwhMeter(m_HwdID, 1, 255, musage, mtotal / 1000.0, "Enphase kWh Production");

	m_p1power.powerusage1 = mtotal;
	m_p1power.powerusage2 = 0;
	m_p1power.usagecurrent = musage;
	sDecodeRXMessage(this, (const unsigned char *)&m_p1power, "Enphase Production kWh Total", 255);
}
