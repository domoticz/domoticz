#include "stdafx.h"
#include "EnphaseAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include <iostream>

//https://enphase.com/en-us/products-and-services/envoy

#define Enphase_request_INTERVAL 30

#ifdef _DEBUG
//#define DEBUG_EnphaseAPI_R
//#define DEBUG_EnphaseAPI_W
#endif

/*
Example
{"production":[{"type":"inverters","activeCount":9,"readingTime":1568991780,"wNow":712,"whLifetime":1448651},{"type":"eim","activeCount":1,"measurementType":"production","readingTime":1568991966,"wNow":624.315,"whLifetime":1455843.527,"varhLeadLifetime":0.001,"varhLagLifetime":311039.158,"vahLifetime":1619431.681,"rmsCurrent":2.803,"rmsVoltage":233.289,"reactPwr":137.092,"apprntPwr":654.245,"pwrFactor":0.95,"whToday":4295.527,"whLastSevenDays":74561.527,"vahToday":5854.681,"varhLeadToday":0.001,"varhLagToday":2350.158}],"consumption":[{"type":"eim","activeCount":1,"measurementType":"total-consumption","readingTime":1568991966,"wNow":1260.785,"whLifetime":2743860.336,"varhLeadLifetime":132372.858,"varhLagLifetime":273043.125,"vahLifetime":3033001.948,"rmsCurrent":5.995,"rmsVoltage":233.464,"reactPwr":437.269,"apprntPwr":1399.886,"pwrFactor":0.9,"whToday":11109.336,"whLastSevenDays":129007.336,"vahToday":13323.948,"varhLeadToday":895.858,"varhLagToday":3700.125},{"type":"eim","activeCount":1,"measurementType":"net-consumption","readingTime":1568991966,"wNow":636.47,"whLifetime":0.0,"varhLeadLifetime":132372.857,"varhLagLifetime":-37996.033,"vahLifetime":3033001.948,"rmsCurrent":3.191,"rmsVoltage":233.376,"reactPwr":574.361,"apprntPwr":744.807,"pwrFactor":0.85,"whToday":0,"whLastSevenDays":0,"vahToday":0,"varhLeadToday":0,"varhLagToday":0}],"storage":[{"type":"acb","activeCount":0,"readingTime":0,"wNow":0,"whNow":0,"state":"idle"}]}
*/

#ifdef DEBUG_EnphaseAPI_W
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

#ifdef DEBUG_EnphaseAPI_R
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

EnphaseAPI::EnphaseAPI(const int ID, const std::string &IPAddress, const unsigned short /*usIPPort*/) :
	m_szIPAddress(IPAddress)
{
	m_p1power.ID = 1;
	m_c1power.ID = 2;
	m_c2power.ID = 3;

	m_HwdID = ID;
}

bool EnphaseAPI::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool EnphaseAPI::StopHardware()
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

void EnphaseAPI::Do_Work()
{
	_log.Log(LOG_STATUS, "EnphaseAPI Worker started...");
	int sec_counter = Enphase_request_INTERVAL - 5;

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % Enphase_request_INTERVAL == 0)
		{
			Json::Value result;
			if (getProductionDetails(result))
			{
				parseProduction(result);
				parseConsumption(result);
				parseNetConsumption(result);
			}
		}
	}
	_log.Log(LOG_STATUS, "EnphaseAPI Worker stopped...");
}

bool EnphaseAPI::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
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
		if (strarray.size() < 2)
			return false;
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

bool EnphaseAPI::getProductionDetails(Json::Value& result)
{
	std::string sResult;

#ifdef DEBUG_EnphaseAPI_R
	sResult = ReadFile("E:\\EnphaseAPI_production.json");
#else
	std::stringstream sURL;
	sURL << "http://" << m_szIPAddress << "/production.json";

	if (!HTTPClient::GET(sURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Error getting http data!");
		return false;
	}
#ifdef DEBUG_EnphaseAPI_W
	SaveString2Disk(sResult, "E:\\EnphaseAPI_production.json")
#endif
#endif

	bool ret = ParseJSon(sResult, result);
	if ((!ret) || (!result.isObject()))
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Invalid data received!");
		return false;
	}
	if (
		(result["consumption"].empty())
		&& (result["production"].empty())
		)
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Invalid (no) data received");
		return false;
	}
	return true;
}

void EnphaseAPI::parseProduction(const Json::Value& root)
{
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	if (sunRise != 0 && sunSet != 0)
	{
		//We only process one hour before sunrise till one hour after sunset

		//GizMoCuz: why is this as the production.json is retreived anyway ?
			//Tuurtje: production.json also contains data from the CT's. Consumption and NetConsumption produce data all day.

		if (ActHourMin + 60 < sunRise)
			return;
		if (ActHourMin - 60 > sunSet)
			return;
	}

	if (root["production"].empty() == true)
	{
		//No production details available
		return;
	}

	Json::Value reading = root["production"][0];

	int musage = reading["wNow"].asInt();
	int mtotal = reading["whLifetime"].asInt();

	SendKwhMeter(m_HwdID, 1, 255, musage, mtotal / 1000.0, "Enphase kWh Production");

	m_p1power.powerusage1 = mtotal;
	m_p1power.powerusage2 = 0;
	m_p1power.usagecurrent = std::max(musage,0);
	sDecodeRXMessage(this, (const unsigned char *)&m_p1power, "Enphase Production kWh Total", 255, nullptr);
}

void EnphaseAPI::parseConsumption(const Json::Value& root)
{
	if (root["consumption"].empty())
	{
		return;
	}
	if (root["consumption"][0].empty())
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Invalid data received");
		return;
	}

	Json::Value reading = root["consumption"][0];

	int musage = reading["wNow"].asInt();
	int mtotal = reading["whLifetime"].asInt();

	SendKwhMeter(m_HwdID, 2, 255, musage, mtotal / 1000.0, "Enphase kWh Consumption");

	m_c1power.powerusage1 = mtotal;
	m_c1power.powerusage2 = 0;
	m_c1power.usagecurrent = std::max(musage,0);
	sDecodeRXMessage(this, (const unsigned char *)&m_c1power, "Enphase Consumption kWh Total", 255, nullptr);
}

void EnphaseAPI::parseNetConsumption(const Json::Value& root)
{
	if (root["consumption"].empty() == true)
	{
		return;
	}
	if (root["consumption"][1].empty() == true)
	{
		_log.Log(LOG_ERROR, "EnphaseAPI: Invalid data received");
		return;
	}

	Json::Value reading = root["consumption"][1];

	int musage = reading["wNow"].asInt();
	int mtotal = reading["whLifetime"].asInt();

	SendKwhMeter(m_HwdID, 3, 255, musage, mtotal / 1000.0, "Enphase kWh Net Consumption");

	m_c2power.powerusage1 = mtotal;
	m_c2power.powerusage2 = 0;
	m_c2power.usagecurrent = std::max(musage,0);
	sDecodeRXMessage(this, (const unsigned char *)&m_c2power, "Enphase Net Consumption kWh Total", 255, nullptr);
}
