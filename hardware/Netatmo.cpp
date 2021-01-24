#include "stdafx.h"
#include "Netatmo.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
//#define DEBUG_NetatmoWeatherStationR
#endif

#ifdef DEBUG_NetatmoWeatherStationW
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
#ifdef DEBUG_NetatmoWeatherStationR
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

struct _tNetatmoDevice
{
	std::string ID;
	std::string ModuleName;
	std::string StationName;
	std::vector<std::string> ModulesIDs;
	//Json::Value Modules;
};

CNetatmo::CNetatmo(const int ID, const std::string &username, const std::string &password)
	: m_clientId("5588029e485a88af28f4a3c4")
	, m_clientSecret("6vIpQVjNsL2A74Bd8tINscklLw2LKv7NhE9uW2")
	, m_username(CURLEncode::URLEncode(username))
	, m_password(CURLEncode::URLEncode(password))
{
	m_nextRefreshTs = mytime(nullptr);
	m_isLogged = false;
	m_NetatmoType = NETYPE_WEATHER_STATION;

	m_HwdID = ID;

	m_ActHome = 0;

	m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_tSetpointUpdateTime = time(nullptr);
	Init();
}

void CNetatmo::Init()
{
	m_RainOffset.clear();
	m_OldRainCounter.clear();
	m_RoomNames.clear();
	m_RoomIDs.clear();
	m_ModuleNames.clear();
	m_ModuleIDs.clear();
	m_thermostatDeviceID.clear();
	m_thermostatModuleID.clear();
	m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_bForceSetpointUpdate = false;
	m_NetatmoType = NETYPE_WEATHER_STATION;

	m_bForceLogin = false;
}

bool CNetatmo::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CNetatmo::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CNetatmo::StopHardware()
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

void CNetatmo::Do_Work()
{
	int sec_counter = 600 - 5;
	bool bFirstTimeWS = true;
	bool bFirstTimeTH = true;
	_log.Log(LOG_STATUS, "Netatmo: Worker started...");
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
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
					if ((m_bPollWeatherData) || (sec_counter % 1200 == 0))
					{
						GetMeterDetails();
					}
				}
				if (m_bPollThermostat)
				{
					//Thermostat data is updated every 10 minutes
					if ((sec_counter % 600 == 0) || (bFirstTimeTH))
					{
						bFirstTimeTH = false;
						GetThermostatDetails();
					}
					if (m_bForceSetpointUpdate)
					{
						time_t atime = time(nullptr);
						if (atime >= m_tSetpointUpdateTime)
						{
							m_bForceSetpointUpdate = false;
							GetThermostatDetails();
						}
					}
				}

			}
		}
	}
	_log.Log(LOG_STATUS, "Netatmo: Worker stopped...");
}

bool CNetatmo::Login()
{
	if (m_isLogged)
		return true;

	if (LoadRefreshToken())
	{
		if (RefreshToken(true))
		{
			m_isLogged = true;
			m_bPollThermostat = true;
			return true;
		}
	}
	std::stringstream sstr;
	sstr << "grant_type=password&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret << "&";
	sstr << "username=" << m_username << "&";
	sstr << "password=" << m_password << "&";
	sstr << "scope=read_station read_thermostat write_thermostat read_homecoach";

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl("https://api.netatmo.net/oauth2/token");
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return false;
	}

	Json::Value root;
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Netatmo: Invalid/no data received...");
		return false;
	}

	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		_log.Log(LOG_ERROR, "Netatmo: No access granted, check username/password...");
		return false;
	}

	//Initial Access Token
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	//_log.Log(LOG_STATUS, "Access token: %s", m_accessToken.c_str());
	//_log.Log(LOG_STATUS, "RefreshToken: %s", m_refreshToken.c_str());
	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(nullptr) + expires;
	StoreRefreshToken();
	m_isLogged = true;
	return true;
}

bool CNetatmo::RefreshToken(const bool bForce)
{
	if (m_refreshToken.empty())
	{
		return false;
	}
	if (!bForce)
	{
		if (!m_isLogged)
			return false;
		if ((mytime(nullptr) - 15) < m_nextRefreshTs)
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

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl("https://api.netatmo.net/oauth2/token");
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return false;
	}

	Json::Value root;
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Netatmo: Invalid/no data received...");
		//Force login next time
		m_isLogged = false;
		return false;
	}

	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		//Force login next time
		_log.Log(LOG_ERROR, "Netatmo: No access granted, forcing login again...");
		m_isLogged = false;
		return false;
	}

	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(nullptr) + expires;
	//StoreRefreshToken();
	return true;
}

bool CNetatmo::LoadRefreshToken()
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Extra FROM Hardware WHERE (ID==%d)", m_HwdID);
	if (result.empty())
		return false;
	std::string refreshToken = result[0][0];
	if (refreshToken.empty())
		return false;
	m_refreshToken = refreshToken;
	return true;
}

void CNetatmo::StoreRefreshToken()
{
	if (m_refreshToken.empty())
		return;
	m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID == %d)", m_refreshToken.c_str(), m_HwdID);
}

int CNetatmo::GetBatteryLevel(const std::string &ModuleType, int battery_percent)
{
	int batValue = 255;

	// Others are plugged
	if ((ModuleType == "NAModule1") || (ModuleType == "NAModule2") || (ModuleType == "NAModule3") || (ModuleType == "NAModule4"))
		batValue = battery_percent;
	else if (ModuleType == "NRV")
	{
		if (battery_percent > 3200)
			battery_percent = 3200;
		else if (battery_percent < 2200)
			battery_percent = 2200;

		//range = 1000
		batValue = 3200 - battery_percent;
		batValue = 100 - int((100.0F / 1000.0F) * float(batValue));
	}
	else if (ModuleType == "NATherm1")
	{
		if (battery_percent > 4100)
			battery_percent = 4100;
		else if (battery_percent < 3000)
			battery_percent = 3000;

		//range = 1100
		batValue = 4100 - battery_percent;
		batValue = 100 - int((100.0F / 1100.0F) * float(batValue));
	}

	return batValue;
}

bool CNetatmo::ParseDashboard(const Json::Value &root, const int DevIdx, const int ID, const std::string &name, const std::string &ModuleType, const int battery_percent, const int rssiLevel)
{
	bool bHaveTemp = false;
	bool bHaveHum = false;
	bool bHaveBaro = false;
	bool bHaveCO2 = false;
	bool bHaveRain = false;
	bool bHaveSound = false;
	bool bHaveWind = false;
	bool bHaveSetpoint = false;

	float temp = 0, sp_temp = 0;
	int hum = 0;
	float baro = 0;
	int co2 = 0;
	float rain  = 0;
	int sound = 0;

	int wind_angle = 0;
	//int wind_gust_angle = 0;
	float wind_strength = 0;
	float wind_gust = 0;

	int batValue = GetBatteryLevel(ModuleType, battery_percent);

	// check for Netatmo cloud data timeout, except if we deal with a thermostat
	if (ModuleType != "NATherm1")
	{
		std::time_t tNetatmoLastUpdate = 0;
		std::time_t tNow = time(nullptr);

		// initialize the relevant device flag
		if ( m_bNetatmoRefreshed.find(ID) == m_bNetatmoRefreshed.end() )
		{
			m_bNetatmoRefreshed[ID] = true;
		}
		// Check when dashboard data was last updated
		if ( !root["time_utc"].empty() )
		{
			tNetatmoLastUpdate = root["time_utc"].asUInt();
		}
		_log.Debug(DEBUG_HARDWARE, "Netatmo: Module [%s] last update = %s", name.c_str(), ctime(&tNetatmoLastUpdate));
		// check if Netatmo data was updated in the past 10 mins (+1 min for sync time lags)... if not means sensors failed to send to cloud
		if (tNetatmoLastUpdate > (tNow - 660))
		{
			if (!m_bNetatmoRefreshed[ID])
			{
				_log.Log(LOG_STATUS, "Netatmo: cloud data for module [%s] is now updated again", name.c_str());
				m_bNetatmoRefreshed[ID] = true;
			}
		}
		else
		{
			if (m_bNetatmoRefreshed[ID])
				_log.Log(LOG_ERROR, "Netatmo: cloud data for module [%s] no longer updated (module possibly disconnected)", name.c_str());
			m_bNetatmoRefreshed[ID] = false;
			return false;
		}
	}

	if (!root["Temperature"].empty())
	{
		bHaveTemp = true;
		temp = root["Temperature"].asFloat();
	}
	else if (!root["temperature"].empty())
	{
		bHaveTemp = true;
		temp = root["temperature"].asFloat();
	}
	if (!root["Sp_Temperature"].empty())
	{
		bHaveSetpoint = true;
		sp_temp = root["Temperature"].asFloat();
	}
	else if (!root["setpoint_temp"].empty())
	{
		bHaveSetpoint = true;
		sp_temp = root["setpoint_temp"].asFloat();
	}
	if (!root["Humidity"].empty())
	{
		bHaveHum = true;
		hum = root["Humidity"].asInt();
	}
	if (!root["Pressure"].empty())
	{
		bHaveBaro = true;
		baro = root["Pressure"].asFloat();
	}
	/*
		if (!root["AbsolutePressure"].empty())
		{
			float apressure = root["AbsolutePressure"].asFloat();
		}
	*/
	if (!root["Noise"].empty())
	{
		bHaveSound = true;
		sound = root["Noise"].asInt();
	}
	if (!root["CO2"].empty())
	{
		bHaveCO2 = true;
		co2 = root["CO2"].asInt();
	}
	if (!root["sum_rain_24"].empty())
	{
		bHaveRain = true;
		rain = root["sum_rain_24"].asFloat();
	}
	if (!root["WindAngle"].empty())
	{
		if (
			(!root["WindAngle"].empty()) &&
			(!root["WindStrength"].empty()) &&
			(!root["GustAngle"].empty()) &&
			(!root["GustStrength"].empty())
			)
		{
			bHaveWind = true;
			wind_angle = root["WindAngle"].asInt();
			//wind_gust_angle = root["GustAngle"].asInt();
			wind_strength = root["WindStrength"].asFloat() / 3.6F;
			wind_gust = root["GustStrength"].asFloat() / 3.6F;
		}
	}

	if (bHaveTemp && bHaveHum && bHaveBaro)
	{
		int nforecast = m_forecast_calculators[ID].CalculateBaroForecast(temp, baro);
		SendTempHumBaroSensorFloat(ID, batValue, temp, hum, baro, (uint8_t)nforecast, name, rssiLevel);
	}
	else if (bHaveTemp && bHaveHum)
	{
		SendTempHumSensor(ID, batValue, temp, hum, name, rssiLevel);
	}
	else if (bHaveTemp)
	{
		SendTempSensor(ID, batValue, temp, name, rssiLevel);
	}

	if (bHaveSetpoint)
	{
		std::string sName = "SetPoint " + name;
		SendSetPointSensor((const uint8_t)DevIdx, 0, ID & 0xFF, sp_temp, sName);
	}

	if (bHaveRain)
	{
		bool bRefetchData = (m_RainOffset.find(ID) == m_RainOffset.end());
		if (!bRefetchData)
		{
			bRefetchData = ((m_RainOffset[ID] == 0) && (m_OldRainCounter[ID] == 0));
		}
		if (bRefetchData)
		{
			//get last rain counter from the database
			bool bExists = false;
			m_RainOffset[ID] = GetRainSensorValue(ID, bExists);
			m_RainOffset[ID] -= rain;
			if (m_RainOffset[ID] < 0)
				m_RainOffset[ID] = 0;
			if (m_OldRainCounter.find(ID) == m_OldRainCounter.end())
			{
				m_OldRainCounter[ID] = 0;
			}
		}
		if (rain < m_OldRainCounter[ID])
		{
			//daily counter went to zero
			m_RainOffset[ID] += m_OldRainCounter[ID];
		}
		m_OldRainCounter[ID] = rain;
		SendRainSensor(ID, batValue, m_RainOffset[ID] + m_OldRainCounter[ID], name, rssiLevel);
	}

	if (bHaveCO2)
	{
		SendAirQualitySensor((ID & 0xFF00) >> 8, ID & 0xFF, batValue, co2, name);
	}

	if (bHaveSound)
	{
		SendSoundSensor(ID, batValue, sound, name);
	}

	if (bHaveWind)
	{
		SendWind(ID, batValue, wind_angle, wind_strength, wind_gust, 0, 0, false, false, name, rssiLevel);
	}
	return true;
}

bool CNetatmo::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	if ((m_thermostatDeviceID.empty()) || (m_thermostatModuleID.empty()))
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
		return false;
	}

	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);

	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false; //later add RGB support, if someone can provide access

	if ((int)(pCmd->LIGHTING2.id1) >> 4){
		//id1 == 0x1### means boiler_status switch
		return true;
	}

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);

	if (m_NetatmoType != NETYPE_ENERGY)
	{
		int node_id = pCmd->LIGHTING2.id4;
		int therm_idx = pCmd->LIGHTING2.unitcode;
		if ((node_id == 3) && (therm_idx >= 0))
		{
			//Away
			return SetAway(therm_idx - 1, bIsOn);
		}
	}
	else
	{
		//Not that the idx is used at the moment. With the current API it is only possible to set the mode for the complete home_id
		unsigned long therm_idx = (pCmd->LIGHTING2.id1 << 24) + (pCmd->LIGHTING2.id2 << 16) + (pCmd->LIGHTING2.id3 << 8) + pCmd->LIGHTING2.id4;
		return SetAway(therm_idx, bIsOn);
	}

	return false;
}

bool CNetatmo::SetAway(const int idx, const bool bIsAway)
{
	return SetProgramState(idx, (bIsAway == true) ? 1 : 0);
}

bool CNetatmo::SetProgramState(const int idx, const int newState)
{
	if (m_NetatmoType != NETYPE_ENERGY)
	{
		if (idx >= (int)m_thermostatDeviceID.size())
			return false;
		if ((m_thermostatDeviceID[idx].empty()) || (m_thermostatModuleID[idx].empty()))
		{
			_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
			return false;
		}
	}
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}

	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	if (m_NetatmoType != NETYPE_ENERGY)
	{
		std::string thermState;
		switch (newState)
		{
		case 0:
			thermState = "program"; //The Thermostat is currently following its weekly schedule
			break;
		case 1:
			thermState = "away"; //The Thermostat is currently applying the away temperature
			break;
		case 2:
			thermState = "hg"; //he Thermostat is currently applying the frost-guard temperature
			break;
		case 3:
			thermState = "off"; //The Thermostat is off
			break;
		default:
			_log.Log(LOG_ERROR, "NetatmoThermostat: Invalid thermostat state!");
			return false;
		}
		ExtraHeaders.push_back("Host: api.netatmo.net");
		ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

		std::stringstream sstr;
		sstr << "access_token=" << m_accessToken;
		sstr << "&device_id=" << m_thermostatDeviceID[idx];
		sstr << "&module_id=" << m_thermostatModuleID[idx];
		sstr << "&setpoint_mode=" << thermState;

		std::string httpData = sstr.str();

		std::string httpUrl("https://api.netatmo.net/api/setthermpoint");

		if (!HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
			return false;
		}
	}
	else
	{
		std::string thermState;
		switch (newState)
		{
		case 0:
			thermState = "schedule"; //The Thermostat is currently following its weekly schedule
			break;
		case 1:
			thermState = "away"; //The Thermostat is currently applying the away temperature
			break;
		case 2:
			thermState = "hg";
			break;
		default:
			_log.Log(LOG_ERROR, "NetatmoThermostat: Invalid thermostat state!");
			return false;
		}
		std::string sPostData = "access_token=" + m_accessToken + "&home_id=" + m_Home_ID + "&mode=" + thermState;

		if (!HTTPClient::POST("https://api.netatmo.com/api/setthermmode", sPostData, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
			return false;
		}
	}

	GetThermostatDetails();
	return true;
}

void CNetatmo::SetSetpoint(int idx, const float temp)
{
	if (m_NetatmoType != NETYPE_ENERGY)
	{
		idx = (idx & 0x00FF0000) >> 16;
		if (idx >= (int)m_thermostatDeviceID.size())
		{
			return;
		}
		if ((m_thermostatDeviceID[idx].empty()) || (m_thermostatModuleID[idx].empty()))
		{
			_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
			return;
		}
	}
	if (!m_isLogged == true)
	{
		if (!Login())
			return;
	}

	float tempDest = temp;
	unsigned char tSign = m_sql.m_tempsign[0];

	if (tSign == 'F')
	{
		//convert back to Celsius
		tempDest = static_cast<float>(ConvertToCelsius(tempDest));
	}

	time_t now = mytime(nullptr);
	struct tm etime;
	localtime_r(&now, &etime);
	time_t end_time;
	int isdst = etime.tm_isdst;
	bool goodtime = false;
	while (!goodtime) {
		etime.tm_isdst = isdst;
		etime.tm_hour += 1;
		end_time = mktime(&etime);
		goodtime = (etime.tm_isdst == isdst);
		isdst = etime.tm_isdst;
		if (!goodtime) {
			localtime_r(&now, &etime);
		}
	}

	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sstr;

	bool ret = false;
	if (m_NetatmoType != NETYPE_ENERGY)
	{

		ExtraHeaders.push_back("Host: api.netatmo.net");
		ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

		sstr << "access_token=" << m_accessToken;
		sstr << "&device_id=" << m_thermostatDeviceID[idx];
		sstr << "&module_id=" << m_thermostatModuleID[idx];
		sstr << "&setpoint_mode=manual";
		sstr << "&setpoint_temp=" << tempDest;
		sstr << "&setpoint_endtime=" << end_time;

		std::string httpData = sstr.str();

		ret = HTTPClient::POST("https://api.netatmo.net/api/setthermpoint", httpData, ExtraHeaders, sResult);
	}
	else
	{
		//find roomid
		std::string roomID;
		for (const auto &therm : m_thermostatDeviceID)
		{
			if (therm.first == idx)
			{
				roomID = therm.second;
				break;
			}
		}
		if (roomID.empty())
		{
			_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
			return;
		}

		sstr << "access_token=" << m_accessToken << "&home_id=" << m_Home_ID << "&room_id=" << roomID << "&mode=manual&temp=" << tempDest << "&endtime=" << end_time;
		std::string sPostData = sstr.str();

		ret = HTTPClient::POST("https://api.netatmo.com/api/setroomthermpoint", sPostData, ExtraHeaders, sResult);
	}

	if (!ret)
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint!");
		return;
	}

	GetThermostatDetails();
	m_tSetpointUpdateTime = time(nullptr) + 60;
	m_bForceSetpointUpdate = true;
}

bool CNetatmo::ParseNetatmoGetResponse(const std::string &sResult, const _eNetatmoType /*NetatmoType*/, const bool bIsThermostat)
{
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_STATUS, "Netatmo: Invalid data received...");
		return false;
	}
	bool bHaveDevices = true;
	if (root["body"].empty())
	{
		bHaveDevices = false;
	}
	else if (root["body"]["devices"].empty())
	{
		bHaveDevices = false;
	}
	else if (!root["body"]["devices"].isArray())
	{
		bHaveDevices = false;
	}
	if (!bHaveDevices)
	{
		if ((!bIsThermostat) && (!m_bFirstTimeWeatherData) && (m_bPollWeatherData))
		{
			//Do not warn if we check if we have a Thermostat device
			_log.Log(LOG_STATUS, "Netatmo: No Weather Station devices found...");
		}
		return false;
	}

	std::vector<_tNetatmoDevice> _netatmo_devices;

	int iDevIndex = 0;
	for (auto device : root["body"]["devices"])
	{
		if (!device["_id"].empty())
		{
			std::string id = device["_id"].asString();
			std::string type = device["type"].asString();
			std::string name = device["module_name"].asString();
			std::string station_name;
			if (!device["station_name"].empty())
				station_name = device["station_name"].asString();
			else if (!device["name"].empty())
				station_name = device["name"].asString();

			stdreplace(name, "'", "");
			stdreplace(station_name, "'", "");

			_tNetatmoDevice nDevice;
			nDevice.ID = id;
			nDevice.ModuleName = name;
			nDevice.StationName = station_name;

			if (!device["modules"].empty())
			{
				if (device["modules"].isArray())
				{
					//Add modules for this device
					for (auto module : device["modules"])
					{
						if (module.isObject())
						{
							//New Method (getstationsdata and getthermostatsdata)
							if (module["_id"].empty())
							{
								iDevIndex++;
								continue;
							}
							std::string mid = module["_id"].asString();
							std::string mtype = module["type"].asString();
							std::string mname = module["module_name"].asString();
							if (mname.empty())
								mname = nDevice.ModuleName;
							int mbattery_percent = 0;
							if (!module["battery_percent"].empty())
							{
								mbattery_percent = module["battery_percent"].asInt();
							}
							int mrf_status = 0;
							if (!module["rf_status"].empty())
							{
								// 90=low, 60=highest
								mrf_status = (90 - module["rf_status"].asInt()) / 3;
								if (mrf_status > 10) {
									mrf_status = 10;
								}
							}
							int crcId = Crc32(0, (const unsigned char *)mid.c_str(), mid.length());
							if (!module["dashboard_data"].empty())
							{
								ParseDashboard(module["dashboard_data"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
							}
							else if (!module["measured"].empty())
							{
								ParseDashboard(module["measured"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
								if (mtype == "NATherm1")
								{
									m_thermostatDeviceID[iDevIndex] = nDevice.ID;
									m_thermostatModuleID[iDevIndex] = mid;

									if (!module["setpoint"].empty())
									{
										if (!module["setpoint"]["setpoint_mode"].empty())
										{
											std::string setpoint_mode = module["setpoint"]["setpoint_mode"].asString();
											bool bIsAway = (setpoint_mode == "away");
											std::string aName = "Away " + mname;
											SendSwitch(3, (uint8_t)(1 + iDevIndex), 255, bIsAway, 0, aName, m_Name);
										}
										//Check if setpoint was just set, and if yes, overrule the previous setpoint
										if (!module["setpoint"]["setpoint_temp"].empty())
										{
											ParseDashboard(module["setpoint"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
										}
									}
								}
							}
						}
						else
							nDevice.ModulesIDs.push_back(module.asString());
					}
				}
			}
			_netatmo_devices.push_back(nDevice);

			int battery_percent = 0;
			if (!device["battery_percent"].empty())
			{
				battery_percent = device["battery_percent"].asInt();
			}
			int wifi_status = 0;
			if (!device["wifi_status"].empty())
			{
				// 86=bad, 56=good
				wifi_status = (86 - device["wifi_status"].asInt()) / 3;
				if (wifi_status > 10) {
					wifi_status = 10;
				}
			}
			int crcId = Crc32(0, (const unsigned char *)id.c_str(), id.length());
			if (!device["dashboard_data"].empty())
			{
				ParseDashboard(device["dashboard_data"], iDevIndex, crcId, name, type, battery_percent, wifi_status);
			}
		}
		iDevIndex++;
	}

	Json::Value mRoot;
	if (root["body"]["modules"].empty())
	{
		//No additional modules defined
		return (!_netatmo_devices.empty());
	}
	if (!root["body"]["modules"].isArray())
	{
		//No additional modules defined
		return (!_netatmo_devices.empty());
	}
	mRoot = root["body"]["modules"];

	for (auto module : mRoot)
	{
		if (module["_id"].empty())
			continue;
		std::string id = module["_id"].asString();
		std::string type = module["type"].asString();
		std::string name = "Unknown";

		//Find the corresponding _tNetatmoDevice
		_tNetatmoDevice nDevice;
		bool bHaveFoundND = false;
		iDevIndex = 0;
		std::vector<_tNetatmoDevice>::const_iterator ittND;
		for (ittND = _netatmo_devices.begin(); ittND != _netatmo_devices.end(); ++ittND)
		{
			std::vector<std::string>::const_iterator ittNM;
			for (ittNM = ittND->ModulesIDs.begin(); ittNM != ittND->ModulesIDs.end(); ++ittNM)
			{
				if (*ittNM == id)
				{
					nDevice = *ittND;
					iDevIndex = (ittND - _netatmo_devices.begin());
					bHaveFoundND = true;
					break;
				}
			}
			if (bHaveFoundND == true)
				break;
		}

		if (!module["module_name"].empty())
		{
			name = module["module_name"].asString();
		}
		else
		{
			if (bHaveFoundND)
			{
				if (!nDevice.ModuleName.empty())
					name = nDevice.ModuleName;
				else if (!nDevice.StationName.empty())
					name = nDevice.StationName;
			}
		}

		if (type == "NATherm1")
		{
			continue; //handled above
		}

		int battery_percent = 0;
		if (!module["battery_percent"].empty())
		{
			battery_percent = module["battery_percent"].asInt();
		}
		int rf_status = 0;
		if (!module["rf_status"].empty())
		{
			// 90=low, 60=highest
			rf_status = (90 - module["rf_status"].asInt()) / 3;
			if (rf_status > 10) {
				rf_status = 10;
			}
		}
		stdreplace(name, "'", " ");

		//std::set<std::string> dataTypes;
		//for (Json::Value::iterator itDataType = module["data_type"].begin(); itDataType != module["data_type"].end(); ++itDataType)
		//{
		//	dataTypes.insert((*itDataType).asCString());
		//}
		int crcId = Crc32(0, (const unsigned char *)id.c_str(), id.length());
		if (!module["dashboard_data"].empty())
		{
			ParseDashboard(module["dashboard_data"], iDevIndex, crcId, name, type, battery_percent, rf_status);
		}
	}
	return (!_netatmo_devices.empty());
}

std::string CNetatmo::MakeRequestURL(const _eNetatmoType NType)
{
	std::stringstream sstr;

	switch (NType)
	{
	case NETYPE_WEATHER_STATION:
		sstr << "https://api.netatmo.net/api/getstationsdata";
		break;
	case NETYPE_HOMECOACH:
		sstr << "https://api.netatmo.net/api/gethomecoachsdata";
		break;
	case NETYPE_ENERGY:
		sstr << "https://api.netatmo.net/api/homesdata";
		break;
	default:
		return "";
	}

	sstr << "?";
	sstr << "access_token=" << m_accessToken;
	sstr << "&" << "get_favorites=" << "true";
	return sstr.str();
}

void CNetatmo::GetMeterDetails()
{
	if (!m_isLogged)
		return;

	std::string httpUrl = MakeRequestURL(m_NetatmoType);

	std::string sResult;

	bool bRet;
#ifdef DEBUG_NetatmoWeatherStationR
	//sResult = ReadFile("E:\\netatmo_mdetails.json");
	sResult = ReadFile("E:\\netatmo_getstationdata.json");
	bRet = true;
#else
	std::vector<std::string> ExtraHeaders;
	if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return;
	}
#endif
#ifdef DEBUG_NetatmoWeatherStationW
	SaveString2Disk(sResult, "E:\\netatmo_getstationdata.json");
#endif

	//Check for error
	Json::Value root;
	bRet = ParseJSon(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Netatmo: Invalid data received...");
		return;
	}

	if (!root["error"].empty())
	{
		//We received an error
		_log.Log(LOG_ERROR, "Netatmo: %s", root["error"]["message"].asString().c_str());
		m_isLogged = false;
		return;
	}

	if (!ParseNetatmoGetResponse(sResult, m_NetatmoType, false))
	{
		if (m_bFirstTimeWeatherData)
		{
			m_bFirstTimeWeatherData = false;
#ifdef DEBUG_NetatmoWeatherStationR
			//sResult = ReadFile("E:\\netatmo_mdetails.json");
			sResult = ReadFile("E:\\gethomecoachsdata.json");
#else
			//Check if the user has an Home Coach device
			httpUrl = MakeRequestURL(NETYPE_HOMECOACH);
			if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult))
			{
				_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
				return;
			}
#endif
			bool bRet = ParseJSon(sResult, root);
			if ((!bRet) || (!root.isObject()))
			{
				_log.Log(LOG_ERROR, "Netatmo: Invalid data received...");
				return;
			}
			if (!root["error"].empty())
			{
				//We received an error
				_log.Log(LOG_ERROR, "Netatmo: %s", root["error"]["message"].asString().c_str());
				m_isLogged = false;
				return;
			}
#ifdef DEBUG_NetatmoWeatherStationW
			SaveString2Disk(sResult, "E:\\gethomecoachsdata.json");
#endif
			bRet = ParseNetatmoGetResponse(sResult, NETYPE_HOMECOACH, false);
			if (bRet)
			{
				m_NetatmoType = NETYPE_HOMECOACH;
			}
			else
			{
				//Check if the user has an Enery device (thermostat)
#ifdef DEBUG_NetatmoWeatherStationR
				//sResult = ReadFile("E:\\netatmo_mdetails.json");
				sResult = ReadFile("E:\\homesdata.json");
#else
				httpUrl = MakeRequestURL(NETYPE_ENERGY);
				if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult))
				{
					_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
					return;
				}
#endif
				bRet = ParseJSon(sResult, root);
				if ((!bRet) || (!root.isObject()))
				{
					_log.Log(LOG_ERROR, "Netatmo: Invalid data received...");
					return;
				}
				if (!root["error"].empty())
				{
					//We received an error
					_log.Log(LOG_ERROR, "Netatmo: %s", root["error"]["message"].asString().c_str());
					m_isLogged = false;
					return;
				}
#ifdef DEBUG_NetatmoWeatherStationW
				SaveString2Disk(sResult, "E:\\homesdata.json");
#endif
				bRet = ParseHomeData(sResult);
				if (bRet)
				{
					m_NetatmoType = NETYPE_ENERGY;
					m_bPollThermostat = true;
				}
				m_bPollWeatherData = false;
			}
		}
	}
	m_bFirstTimeWeatherData = false;
}

void CNetatmo::GetThermostatDetails()
{
	if (!m_isLogged)
		return;

	if (m_bFirstTimeThermostat)
		m_bFirstTimeThermostat = false;

	std::string sResult;

#ifdef DEBUG_NetatmoWeatherStationR
	if (m_NetatmoType != NETYPE_ENERGY)
		sResult = ReadFile("E:\\netatmo_mdetails_thermostat_0002.json");
	else
		sResult = ReadFile("E:\\netatmo_homestatus_0001.json");
	bool ret = true;
#else
	std::stringstream sstr2;
	std::vector<std::string> ExtraHeaders;
	bool ret;

	if (m_NetatmoType != NETYPE_ENERGY)
	{
		sstr2 << "https://api.netatmo.net/api/getthermostatsdata";
		sstr2 << "?access_token=" << m_accessToken;
		ret = HTTPClient::GET(sstr2.str(), ExtraHeaders, sResult);
	}
	else
	{
		sstr2 << "https://api.netatmo.net/api/homestatus";
		std::string sPostData = "access_token=" + m_accessToken + "&home_id=" + m_Home_ID;
		ret = HTTPClient::POST(sstr2.str(), sPostData, ExtraHeaders, sResult);
	}

	if (!ret)
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return;
	}
#endif
#ifdef DEBUG_NetatmoWeatherStationW
	static int cntr = 1;
	char szFileName[255];
	if (m_NetatmoType != NETYPE_ENERGY)
		sprintf(szFileName, "E:\\netatmo_mdetails_thermostat_%04d.json", cntr++);
	else
		sprintf(szFileName, "E:\\netatmo_homestatus_%04d.json", cntr++);
	SaveString2Disk(sResult, szFileName);
#endif
	if (m_NetatmoType != NETYPE_ENERGY)
	{
		if (!ParseNetatmoGetResponse(sResult, m_NetatmoType, true))
		{
			m_bPollThermostat = false;
		}
	}
	else
	{
		if (!ParseHomeStatus(sResult))
		{
			m_bPollThermostat = false;
		}
	}
}

bool CNetatmo::ParseHomeData(const std::string &sResult)
{
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_STATUS, "Netatmo: Invalid data received...");
		return false;
	}
	//Check if we have a home id
	if (!root["body"]["homes"].empty())
	{
		//Lets assume we have only 1 home for now
		if ((int)root["body"]["homes"].size() <= m_ActHome)
			return false;
		if (!root["body"]["homes"][m_ActHome]["id"].empty())
		{
			m_Home_ID = root["body"]["homes"][m_ActHome]["id"].asString();

			//Get the module names
			if (root["body"]["homes"][m_ActHome]["modules"].empty())
				return false;
			Json::Value mRoot = root["body"]["homes"][m_ActHome]["modules"];
			for (auto module : mRoot)
			{
				if (!module["id"].empty())
				{
					std::string mID = module["id"].asString();
					m_ModuleNames[mID] = module["name"].asString();
					int crcId = Crc32(0, (const unsigned char *)mID.c_str(), mID.length());
					m_ModuleIDs[mID] = crcId;
				}
			}

			//Get the room names
			if (root["body"]["homes"][m_ActHome]["rooms"].empty())
				return false;
			mRoot = root["body"]["homes"][m_ActHome]["rooms"];
			for (auto room : mRoot)
			{
				if (!room["id"].empty())
				{
					std::string rID = room["id"].asString();
					m_RoomNames[rID] = room["name"].asString();
					int crcId = Crc32(0, (const unsigned char *)rID.c_str(), rID.length());
					m_RoomIDs[rID] = crcId;
				}
			}
			return true;
		}
	}
	if ((!m_bFirstTimeWeatherData) && (m_bPollWeatherData))
	{
		//Do not warn if we check if we have a Thermostat device
		_log.Log(LOG_STATUS, "Netatmo: No Weather Station devices found...");
	}
	return false;
}

bool CNetatmo::ParseHomeStatus(const std::string &sResult)
{
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_STATUS, "Netatmo: Invalid data received...");
		return false;
	}
	//bool bHaveDevices = true;
	if (root["body"].empty())
		return false;

	if (root["body"]["home"].empty())
		return false;

	//Parse modules (for RSSI/Battery level) ?
	if (!root["body"]["home"]["modules"].empty())
	{
		if (!root["body"]["home"]["modules"].isArray())
			return false;
		Json::Value mRoot = root["body"]["home"]["modules"];

		int iModuleIndex = 0;
		for (auto module : mRoot)
		{
			if (!module["id"].empty())
			{
				std::string id = module["id"].asString();

				std::string moduleName = id;
				int moduleID = iModuleIndex;

				//Find the module (name/id)
				for (std::map<std::string, std::string>::const_iterator itt = m_ModuleNames.begin(); itt != m_ModuleNames.end(); ++itt)
				{
					if (itt->first == id)
					{
						moduleName = itt->second;
						moduleID = m_ModuleIDs[id];
						break;
					}
				}

				int batteryLevel = 255;
				if (!module["boiler_status"].empty())
				{
					std::string aName = "Status";
					bool bIsActive = (module["boiler_status"].asString() == "true");
					SendSwitch((moduleID & 0x00FFFFFF) | 0x10000000, 1, 255, bIsActive, 0, aName, m_Name);
				}
				if (!module["battery_level"].empty())
				{
					batteryLevel = module["battery_level"].asInt();
					batteryLevel = GetBatteryLevel(module["type"].asString(), batteryLevel);
				}
				if (!module["rf_strength"].empty())
				{
					float rf_strength = module["rf_strength"].asFloat();
					// 90=low, 60=highest
					if (rf_strength > 90.0F)
						rf_strength = 90.0F;
					if (rf_strength < 60.0F)
						rf_strength = 60.0F;

					//range is 30
					float mrf_percentage = (100.0F / 30.0F) * float((90 - rf_strength));
					if (mrf_percentage != 0)
					{
						std::string pName = moduleName + " RF (+Batt)";
						SendPercentageSensor(moduleID, 0, batteryLevel, mrf_percentage, pName);
					}
				}
			}
			iModuleIndex++;
		}
	}
	//Parse Rooms
	int iDevIndex = 0;
	if (!root["body"]["home"]["rooms"].empty())
	{
		if (!root["body"]["home"]["rooms"].isArray())
			return false;
		Json::Value mRoot = root["body"]["home"]["rooms"];

		for (auto room : mRoot)
		{
			if (!room["id"].empty())
			{
				std::string id = room["id"].asString();

				std::string roomName = id;
				int roomID = iDevIndex + 1;

				//Find the room name
				for (std::map<std::string, std::string>::const_iterator itt = m_RoomNames.begin(); itt != m_RoomNames.end(); ++itt)
				{
					if (itt->first == id)
					{
						roomName = itt->second;
						roomID = m_RoomIDs[id];
						break;
					}
				}

				m_thermostatDeviceID[roomID & 0xFFFFFF] = id;
				m_thermostatModuleID[roomID & 0xFFFFFF] = id;

				if (!room["therm_measured_temperature"].empty())
				{
					SendTempSensor((roomID & 0x00FFFFFF) | 0x03000000, 255, room["therm_measured_temperature"].asFloat(), roomName);
				}
				if (!room["therm_setpoint_temperature"].empty())
				{
					SendSetPointSensor((uint8_t)(((roomID & 0x00FF0000) | 0x02000000) >> 16), (roomID & 0XFF00) >> 8, roomID & 0XFF, room["therm_setpoint_temperature"].asFloat(),
							   roomName);
				}
				if (!room["therm_setpoint_mode"].empty())
				{
					std::string setpoint_mode = room["therm_setpoint_mode"].asString();
					bool bIsAway = (setpoint_mode == "away");
					std::string aName = "Away " + roomName;
					SendSwitch((roomID & 0x00FFFFFF) | 0x01000000, 1, 255, bIsAway, 0, aName, m_Name);
				}
			}
			iDevIndex++;
		}
	}

	return (iDevIndex > 0);
}
