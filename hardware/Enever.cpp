#include "stdafx.h"
#include "Enever.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/json_helper.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../tinyxpath/tinyxml.h"
#include "../webserver/Base64.h"
#include "hardwaretypes.h"
#include <iostream>

//See https://enever.nl/prijzenfeeds
//We are able to do 5 requests per day, so we need to cache the results!
//New Electricity prices are available at 15:00 (but to be safe it is better to get them later... at 16:00)
//New Gas prices are available at 06:00 (but to be safe it is better to get them later ... at 07:00)


#ifdef _DEBUG
//#define DEBUG_Enever_R
//#define DEBUG_Enever_W
#endif

#define ENEVER_FEED_ELEC_TODAY "https://enever.nl/api/stroomprijs_vandaag.php?token={token}"
#define ENEVER_FEED_ELEC_TOMORROW "https://enever.nl/api/stroomprijs_morgen.php?token={token}"
#define ENEVER_FEED_GAS_TODAY "https://enever.nl/api/gasprijs_vandaag.php?token={token}"

#ifdef DEBUG_Enever_W
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

#ifdef DEBUG_Enever_R
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

Enever::Enever(int ID, const std::string& szToken, const std::string& szProvider) :
	m_szToken(szToken),
	m_szProvider(szProvider)
{
	m_HwdID = ID;

	std::vector<std::vector<std::string> > result;

	//Retreive current prices backup
	std::string szName = "Enever_Electricity_" + std::to_string(m_HwdID);
	result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", szName.c_str());
	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO UserVariables (Name, ValueType, Value) VALUES ('%q',%d,'%q')", szName.c_str(), USERVARTYPE_STRING, "");
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", szName.c_str());
		if (result.empty())
			return;
	}
	if (!result.empty())
	{
		m_szCurrentElectricityPrices = result[0][1];
	}

	szName = "Enever_Gas_" + std::to_string(m_HwdID);
	result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", szName.c_str());
	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO UserVariables (Name, ValueType, Value) VALUES ('%q',%d,'%q')", szName.c_str(), USERVARTYPE_STRING, "");
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", szName.c_str());
		if (result.empty())
			return;
	}
	if (!result.empty())
	{
		m_szCurrentGasPrices = result[0][1];
	}
}

bool Enever::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool Enever::StopHardware()
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

void Enever::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");

	//poll every 5 hours

	int last_min = -1;
	int last_hour = -1;
	int last_day = -1;

	int sec_counter = 0;

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		time_t atime = mytime(nullptr);
		struct tm* ltime = localtime(&atime);

		if (ltime->tm_hour != last_hour)
		{
			Json::Value restult;
			if ((ltime->tm_hour == 16) || (last_hour==-1))
			{
				if (GetPriceElectricity(restult))
				{
					parseElectricity(restult);
				}
			}
			if ((ltime->tm_hour == 7) || (last_hour == -1))
			{
				restult.clear();
				if (GetPriceGas(restult))
				{
					parseGas(restult);
				}
			}
			last_hour = ltime->tm_hour;
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool Enever::WriteToHardware(const char* pdata, const unsigned char length)
{
	return false;
}

std::string Enever::MakeURL(const std::string& sURL)
{
	std::string szResult = sURL;
	stdreplace(szResult, "{token}", m_szToken);
	return szResult;
}

bool Enever::GetPriceElectricity(Json::Value& result)
{
	std::string sResult;

	time_t atime = mytime(nullptr);
	struct tm* ltime = localtime(&atime);

	bool bNeedUpdate = true;
	if (!m_szCurrentElectricityPrices.empty())
	{
		//check if we need to update (data is not from today)
		Json::Value jsonCurrent;
		Json::Reader reader;
		if (reader.parse(m_szCurrentElectricityPrices, jsonCurrent))
		{
			if (jsonCurrent.isMember("data"))
			{
				Json::Value firstRecord = jsonCurrent["data"][0];
				std::string szDate = firstRecord["datum"].asString();
				time_t rtime;
				struct tm lltime;
				if (ParseSQLdatetime(rtime, lltime, szDate))
				{
					if (
						(lltime.tm_year == ltime->tm_year)
						|| (lltime.tm_mon == ltime->tm_mon)
						|| (lltime.tm_mday == ltime->tm_mday)
						)
					{
						bNeedUpdate = false;
						sResult = m_szCurrentElectricityPrices;
					}
				}
			}
		}
	}
	if (bNeedUpdate)
	{
#ifdef DEBUG_Enever_R
		sResult = ReadFile("E:\\enever_stroom.json");
#else
		std::vector<std::string> ExtraHeaders;
		if (m_szToken.empty()) {
			return false;
		}

		if (!HTTPClient::GET(MakeURL(ENEVER_FEED_ELEC_TODAY), ExtraHeaders, sResult))
		{
			if (!m_szToken.empty())
			{
				return false;
			}
			else
			{
				Log(LOG_ERROR, "Error getting http data! (electricity)");
				return false;
			}
		}
#ifdef DEBUG_Enever_W
		SaveString2Disk(sResult, "E:\\enever_stroom.json");
#endif
#endif
		Debug(DEBUG_RECEIVED, "electricity_prices: %s", sResult.c_str());

		//Store token for later usage
		std::string szName = "Enever_Electricity_" + std::to_string(m_HwdID);
		m_sql.safe_query("UPDATE UserVariables SET Value='%q', LastUpdate='%s' WHERE (Name=='%q')", sResult.c_str(), TimeToString(nullptr, TF_DateTime).c_str(), szName.c_str());

		m_szCurrentElectricityPrices = sResult;
	}

	bool ret = ParseJSon(sResult, result);
	if ((!ret) || (!result.isObject()))
	{
		m_szToken.clear();
		Log(LOG_ERROR, "Invalid data received! (electricity/json)");
		return false;
	}
	if (result["data"].empty())
	{
		m_szToken.clear();
		Log(LOG_ERROR, "Invalid (no) data received (electricity prices, date object not found)");
		return false;
	}
	return true;
}

void Enever::parseElectricity(const Json::Value& root)
{
	if (root["data"].empty() == true)
	{
		//No date details available
		return;
	}

	time_t atime = mytime(nullptr);
	struct tm* ltime = localtime(&atime);

	int act_hour = ltime->tm_hour;

	for (const auto& itt : root["data"])
	{
		std::string szDate = itt["datum"].asString();
		std::string szPrice = itt["prijs"].asString();

		time_t rtime;
		struct tm lltime;
		if (!ParseSQLdatetime(rtime, lltime, szDate))
		{
			Log(LOG_ERROR, "Invalid date received (electricity prices)");
			return; //invalid date!
		}

		std::string szProviderPriceName = "prijs" + m_szProvider;
		if (itt[szProviderPriceName].empty())
			return; //no price for this provider

		std::string szProviderPrice = itt[szProviderPriceName].asString();
		float fProviderPrice = std::stof(szProviderPrice);

		if (lltime.tm_hour == act_hour)
		{
			SendCustomSensor(1, 1, 255, fProviderPrice, "Actual Electricity Price", "Euro / kWh");
		}
		while (1 == 0);
	}
}

bool Enever::GetPriceGas(Json::Value& result)
{
	std::string sResult;

	time_t atime = mytime(nullptr);
	struct tm* ltime = localtime(&atime);

	bool bNeedUpdate = true;
	if (!m_szCurrentGasPrices.empty())
	{
		//check if we need to update (data is not from today)
		Json::Value jsonCurrent;
		Json::Reader reader;
		if (reader.parse(m_szCurrentGasPrices, jsonCurrent))
		{
			if (jsonCurrent.isMember("data"))
			{
				Json::Value firstRecord = jsonCurrent["data"][0];
				std::string szDate = firstRecord["datum"].asString();
				time_t rtime;
				struct tm lltime;
				if (ParseSQLdatetime(rtime, lltime, szDate))
				{
					if (
						(lltime.tm_year == ltime->tm_year)
						|| (lltime.tm_mon == ltime->tm_mon)
						|| (lltime.tm_mday == ltime->tm_mday)
						)
					{
						bNeedUpdate = false;
						sResult = m_szCurrentGasPrices;
					}
				}
			}
		}
	}
	if (bNeedUpdate)
	{
#ifdef DEBUG_Enever_R
		sResult = ReadFile("E:\\enever_gas.json");
#else
		std::vector<std::string> ExtraHeaders;
		if (m_szToken.empty()) {
			return false;
		}

		if (!HTTPClient::GET(MakeURL(ENEVER_FEED_GAS_TODAY), ExtraHeaders, sResult))
		{
			if (!m_szToken.empty())
			{
				return false;
			}
			else
			{
				Log(LOG_ERROR, "Error getting http data! (gas)");
				return false;
			}
		}
#ifdef DEBUG_Enever_W
		SaveString2Disk(sResult, "E:\\enever_gas.json");
#endif
#endif
		Debug(DEBUG_RECEIVED, "gas_prices: %s", sResult.c_str());

		//Store token for later usage
		std::string szName = "Enever_Gas_" + std::to_string(m_HwdID);
		m_sql.safe_query("UPDATE UserVariables SET Value='%q', LastUpdate='%s' WHERE (Name=='%q')", sResult.c_str(), TimeToString(nullptr, TF_DateTime).c_str(), szName.c_str());

		m_szCurrentGasPrices = sResult;
	}

	bool ret = ParseJSon(sResult, result);
	if ((!ret) || (!result.isObject()))
	{
		m_szToken.clear();
		Log(LOG_ERROR, "Invalid data received! (gas/json)");
		return false;
	}
	if (result["data"].empty())
	{
		m_szToken.clear();
		Log(LOG_ERROR, "Invalid (no) data received (gas prices, date object not found)");
		return false;
	}
	return true;
}

//Gas prices are for the entire day
void Enever::parseGas(const Json::Value& root)
{
	if (root["data"].empty() == true)
	{
		//No date details available
		return;
	}

	time_t atime = mytime(nullptr);
	struct tm* ltime = localtime(&atime);

	std::string szDate = root["data"][0]["datum"].asString();
	std::string szPrice = root["data"][0]["prijs"].asString();

	time_t rtime;
	struct tm lltime;
	if (!ParseSQLdatetime(rtime, lltime, szDate))
	{
		Log(LOG_ERROR, "Invalid date received (gas prices)");
		return; //invalid date!
	}

	std::string szProviderPriceName = "prijs" + m_szProvider;
	if (root["data"][0][szProviderPriceName].empty())
		return; //no price for this provider

	std::string szProviderPrice = root["data"][0][szProviderPriceName].asString();
	float fProviderPrice = std::stof(szProviderPrice);

	SendCustomSensor(1, 2, 255, fProviderPrice, "Actual Gas Price", "Euro / m3");
}
