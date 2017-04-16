#include "stdafx.h"
#include "Fitbit.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
//	#define DEBUG_FitbitWeatherStationW
#endif

#ifdef DEBUG_FitbitWeatherStationW
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
#ifdef DEBUG_FitbitWeatherStationR
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

CFitbit::CFitbit(const int ID, const std::string& username, const std::string& password) :
m_username(CURLEncode::URLEncode(username)),
m_password(CURLEncode::URLEncode(password)),
m_clientId("12345"),
m_clientSecret("123456789abcdef")
{
	m_nextRefreshTs = mytime(NULL);
	m_isLogged = false;

	m_HwdID=ID;

	m_stoprequested=false;
	Init();
}

CFitbit::~CFitbit(void)
{
}

void CFitbit::Init()
{
}

bool CFitbit::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CFitbit::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CFitbit::StopHardware()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CFitbit::Do_Work()
{
	int sec_counter = 600 - 5;
	bool bFirstTimeWS = true;
	_log.Log(LOG_STATUS, "Fitbit: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (!m_isLogged)
		{
			if (sec_counter % 30 == 0)
			{
				Login();
			}
		}
		if (m_isLogged)
		{
			if (RefreshToken())
			{
				if ((sec_counter % 600 == 0) || (bFirstTimeWS))
				{
					//Weather station data is updated every 10 minutes
					bFirstTimeWS = false;
					if (sec_counter % 1200 == 0)
					{
						GetMeterDetails();
					}
				}
			}
		}
	}
	_log.Log(LOG_STATUS,"Fitbit: Worker stopped...");
}

bool CFitbit::Login()
{
	if (m_isLogged)
		return true;

	if (LoadRefreshToken())
	{
		if (RefreshToken(true))
		{
			m_isLogged = true;
			return true;
		}
	}
	std::stringstream sstr;
	sstr << "grant_type=password&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret << "&";
	sstr << "username=" << m_username << "&";
	sstr << "password=" << m_password << "&";
	sstr << "scope=activity heartrate";

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.fitbit.com");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl("https://api.fitbit.com/oauth2/token");
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Fitbit: Error connecting to Server...");
		return false;
	}

#ifdef DEBUG_FitbitWeatherStationW
	SaveString2Disk(sResult, "E:\\fitbit_login.json");
#endif
	Json::Value root;
	Json::Reader jReader;
	ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Fitbit: Invalid/no data received...");
		return false;
	}

	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		_log.Log(LOG_ERROR, "Fitbit: No access granted, check username/password...");
		return false;
	}
/*
	//Initial Access Token
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	//_log.Log(LOG_STATUS, "Access token: %s", m_accessToken.c_str());
	//_log.Log(LOG_STATUS, "RefreshToken: %s", m_refreshToken.c_str());
	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(NULL) + expires;
	StoreRefreshToken();
*/
	return true;
}

bool CFitbit::RefreshToken(const bool bForce)
{
	if (m_refreshToken.empty())
	{
		return false;
	}
	if (!bForce)
	{
		if (!m_isLogged)
			return false;
		if ((mytime(NULL) - 15) < m_nextRefreshTs)
			return true; //no need to refresh the token yet
	}

	// Time to refresh the token
	std::stringstream sstr;
	sstr << "grant_type=refresh_token&";
	sstr << "refresh_token=" << m_refreshToken << "&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret;

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.Fitbit.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl("https://api.fitbit.net/oauth2/token");
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Fitbit: Error connecting to Server...");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Fitbit: Invalid/no data received...");
		//Force login next time
		m_isLogged = false;
		return false;
	}

	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		//Force login next time
		_log.Log(LOG_ERROR, "Fitbit: No access granted, forcing login again...");
		m_isLogged = false;
		return false;
	}
/*
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(NULL) + expires;
	//StoreRefreshToken();
*/
	return true;
}

bool CFitbit::LoadRefreshToken()
{
	std::vector<std::vector<std::string> > result;
	result=m_sql.safe_query("SELECT Extra FROM Hardware WHERE (ID==%d)",m_HwdID);
	if (result.empty())
		return false;
	std::string refreshToken = result[0][0];
	if (refreshToken.empty())
		return false;
	m_refreshToken = refreshToken;
	return true;
}

void CFitbit::StoreRefreshToken()
{
	if (m_refreshToken.empty())
		return;
	m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID == %d)", m_refreshToken.c_str(), m_HwdID);
}


bool CFitbit::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

bool CFitbit::ParseFitbitGetResponse(const std::string &sResult)
{
	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_STATUS, "Fitbit: Invalid data received...");
		return false;
	}
	return false;
}

void CFitbit::GetMeterDetails()
{
	if (!m_isLogged)
		return;
	return;
	std::stringstream sstr2;
//	sstr2 << "https://api.Fitbit.net/api/devicelist";
	sstr2 << "https://api.Fitbit.net/api/getstationsdata";
	sstr2 << "?";
	sstr2 << "access_token=" << m_accessToken;
	sstr2 << "&" << "get_favorites=" << "true";
	std::string httpUrl = sstr2.str();

	std::vector<std::string> ExtraHeaders;
	std::string sResult;

#ifdef DEBUG_FitbitWeatherStationR
	//sResult = ReadFile("E:\\Fitbit_mdetails.json");
	sResult = ReadFile("E:\\Fitbit_getstationdata.json");
	bool ret = true;
#else
	bool ret=HTTPClient::GET(httpUrl, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_STATUS, "Fitbit: Error connecting to Server...");
		return;
	}
#endif
#ifdef DEBUG_FitbitWeatherStationW
	SaveString2Disk(sResult, "E:\\Fitbit_getstationdata.json");
#endif
	ParseFitbitGetResponse(sResult);
}

