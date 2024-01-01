#include "stdafx.h"
#include "Netatmo.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"

#define round(a) ( int ) ( a + .5 )

#define NETATMO_OAUTH2_TOKEN_URI "https://api.netatmo.net/oauth2/token"
#define NETATMO_SCOPES "read_station read_thermostat write_thermostat read_homecoach read_smokedetector read_presence read_camera"
#define NETATMO_REDIRECT_URI "http://localhost/netatmo"

#ifdef _DEBUG
//#define DEBUG_NetatmoWeatherStationR
#endif

#ifdef DEBUG_NetatmoWeatherStationW
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

CNetatmo::CNetatmo(const int ID, const std::string& username, const std::string& password)
	: m_username(CURLEncode::URLDecode(username))
	, m_password(CURLEncode::URLDecode(password))
{
	m_scopes = NETATMO_SCOPES;
	m_redirectUri = NETATMO_REDIRECT_URI;
	m_authCode = m_password;

	size_t pos = m_username.find(":");
	if (pos != std::string::npos)
	{
		m_clientId = m_username.substr(0, pos);
		m_clientSecret = m_username.substr(pos + 1);
	}
	else
	{
		Log(LOG_ERROR, "The username does not contain the client_id:client_secret!");
		Debug(DEBUG_HARDWARE,"The username does not contain the client_id:client_secret! (%s)", m_username.c_str());
	}

	m_nextRefreshTs = mytime(nullptr);
	m_isLogged = false;
	m_energyType = NETYPE_WEATHER_STATION;
	m_weatherType = NETYPE_WEATHER_STATION;

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
	m_ScheduleNames.clear();
	m_ScheduleIDs.clear();
	m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_bForceSetpointUpdate = false;
	m_weatherType = NETYPE_WEATHER_STATION;
	m_energyType = NETYPE_WEATHER_STATION;

	m_bForceLogin = false;
}

bool CNetatmo::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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
	Log(LOG_STATUS, "Worker started...");
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
				//Weather / HomeCoach data is updated every 10 minutes
				// 03/03/2022 - PP Changing the Weather polling from 600 to 900s. This has reduce the number of server errors, 
				// but do not prevennt to have one time to time
				if ((sec_counter % 900 == 0) || (bFirstTimeWS))
				{
					bFirstTimeWS = false;
					if ((m_bPollWeatherData) || (sec_counter % 1200 == 0))
						GetMeterDetails();
				}

				//Thermostat data is updated every 10 minutes
				if ((sec_counter % 900 == 0) || (bFirstTimeTH))
				{
					bFirstTimeTH = false;
					if (m_bPollThermostat)
						GetThermostatDetails();
				}
				//Update Thermostat data when the
				//manual set point reach its end
				if (m_bForceSetpointUpdate)
				{
					time_t atime = time(nullptr);
					if (atime >= m_tSetpointUpdateTime)
					{
						m_bForceSetpointUpdate = false;
						if (m_bPollThermostat)
							GetThermostatDetails();
					}
				}
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

/// <summary>
/// Login to Netatmon API
/// </summary>
/// <returns>true if logged in, false otherwise</returns>
bool CNetatmo::Login()
{
	//Already logged noting
	if (m_isLogged)
		return true;

	//Check if a stored token is available
	if (LoadRefreshToken())
	{
		//Yes : we refresh our take
		if (RefreshToken(true))
		{
			m_isLogged = true;
			m_bPollThermostat = true;
			return true;
		}
	}

	//Loggin on the API
	std::stringstream sstr;
	sstr << "grant_type=authorization_code&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret << "&";
	sstr << "code=" << m_authCode << "&";
	sstr << "redirect_uri=" << m_redirectUri << "&";
	sstr << "scope=" << m_scopes;

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl(NETATMO_OAUTH2_TOKEN_URI);
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);

	//Check for returned data
	if (!ret)
	{
		Log(LOG_ERROR, "Error connecting to Server...");
		return false;
	}

	//Check the returned JSON
	Json::Value root;
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid/no data received...");
		return false;
	}
	//Check if access was granted
	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		Log(LOG_ERROR, "No access granted, check credentials...");
		Debug(DEBUG_HARDWARE, "No access granted, check credentials...(%s)(%s)", httpData.c_str(), root.toStyledString().c_str());
		return false;
	}

	//Initial Access Token
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();

	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(nullptr) + expires;

	//Store the token in database in case
	//of domoticz restart
	StoreRefreshToken();
	m_isLogged = true;
	return true;
}

/// <summary>
/// Refresh a token previously granted by loggin to the API
/// (it avoid the need to submit username / password again)
/// </summary>
/// <param name="bForce">set to true to force refresh</param>
/// <returns>true if token refreshed, false otherwise</returns>
bool CNetatmo::RefreshToken(const bool bForce)
{
	//To refresh a token, we must have
	//one to refresh...
	if (m_refreshToken.empty())
		return false;

	//Check if we need to refresh the
	//token (token is valid for a fixed duration)
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

	std::string httpUrl(NETATMO_OAUTH2_TOKEN_URI);
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);

	//Check for returned data
	if (!ret)
	{
		Log(LOG_ERROR, "Error connecting to Server...");
		return false;
	}

	//Check for valid JSON
	Json::Value root;
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid/no data received...");
		//Force login next time
		m_isLogged = false;
		return false;
	}

	//Check if token was refreshed and access granted
	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		//Force login next time
		Log(LOG_ERROR, "No access granted, forcing login again...");
		m_isLogged = false;
		return false;
	}

	//store the token
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	int expires = root["expires_in"].asInt();
	//Store the duration of validity of the token
	m_nextRefreshTs = mytime(nullptr) + expires;

	return true;
}

/// <summary>
/// Load an access token from the database
/// </summary>
/// <returns>true if token retreived, store the token in member variables</returns>
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

/// <summary>
/// Store an access token in the database for reuse after domoticz restart
/// (Note : we should also store token duration)
/// </summary>
void CNetatmo::StoreRefreshToken()
{
	if (m_refreshToken.empty())
		return;
	m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID == %d)", m_refreshToken.c_str(), m_HwdID);
}

/// <summary>
/// Upon domoticz devices action (pressing a switch) take action
/// on the netatmo thermostat valve through the API
/// </summary>
/// <param name="pdata">RAW data from domoticz device</param>
/// <param name=""></param>
/// <returns>success carrying the action (visible through domoticz)</returns>
bool CNetatmo::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	//Check if a thermostat / valve device is available at all
	if ((m_thermostatDeviceID.empty()) || (m_thermostatModuleID.empty()))
	{
		Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
		return false;
	}

	//Get a common structure to identify the actual action
	//the user has selected in domoticz (actionning a switch....)
	//Here a LIGHTING2 is used as we have selector switch for
	//our thermostat / valve devices
	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2 &&
		(pCmd->LIGHTING2.packettype != pTypeGeneralSwitch && pCmd->LIGHTING2.subtype != sSwitchTypeSelector))
		return false;

	//This is the boiler status switch : do nothing
	// unitcode == 0x00 ### means boiler_status switch
	if ((int)(pCmd->LIGHTING2.unitcode) == 0)
		return true;

	//This is the selector switch for setting the thermostat schedule
	// unitcode == 0x02 ### means schedule switch
	if ((int)(pCmd->LIGHTING2.unitcode) == 2)
	{
		//Recast raw data to get switch specific data
		const _tGeneralSwitch* xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int ID = xcmd->id; //switch ID
		int level = xcmd->level; //Level selected on the switch

		//Set the schedule on the thermostat
		SetSchedule(level);

		return true;
	}

	//Set Away mode on thermostat
	// unitcode == 0x03 ### means mode switch
	if ((int)(pCmd->LIGHTING2.unitcode) == 3)
	{
		//Switch active = Turn on Away Mode
		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		//Recast raw data to get switch specific data
		const _tGeneralSwitch* xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int ID = xcmd->id;// switch ID
		int level = (xcmd->level);//Level selected on the switch

		// Get the mode ID and thermostat ID
		int mode = (level / 10) - 1;
		unsigned long therm_idx = (pCmd->LIGHTING2.id1 << 24) + (pCmd->LIGHTING2.id2 << 16) + (pCmd->LIGHTING2.id3 << 8) + pCmd->LIGHTING2.id4;

		//Set mode on the thermostat
		return SetProgramState(therm_idx, mode);
	}

	return false;
}

/// <summary>
/// Set the thermostat / valve in "away mode"
/// </summary>
/// <param name="idx">ID of the device to set in away mode</param>
/// <param name="bIsAway">wether to put in away or normal / schedule mode</param>
/// <returns>success status</returns>
bool CNetatmo::SetAway(const int idx, const bool bIsAway)
{
	return SetProgramState(idx, (bIsAway == true) ? 1 : 0);
}

/// <summary>
/// Set the thermostat / valve operationnal mode
/// </summary>
/// <param name="idx">ID of the device to put in away mode</param>
/// <param name="newState">Mode of the device (0 = schedule / normal; 1 = away mode; 2 = frost guard; 3 = off (not supported in new API)</param>
/// <returns>success status</returns>
bool CNetatmo::SetProgramState(const int idx, const int newState)
{
	//Check if logged, logging if needed
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}

	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	if (m_energyType != NETYPE_ENERGY)
	{
		// Check if thermostat device is available, reversing byte order to get our ID
		int reverseIdx = ((idx >> 24) & 0x000000FF) | ((idx >> 8) & 0x0000FF00) | ((idx << 8) & 0x00FF0000) | ((idx << 24) & 0xFF000000);
		if ((m_thermostatDeviceID[reverseIdx].empty()) || (m_thermostatModuleID[reverseIdx].empty()))
		{
			Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
			return false;
		}

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
			Log(LOG_ERROR, "NetatmoThermostat: Invalid thermostat state!");
			return false;
		}
		ExtraHeaders.push_back("Host: api.netatmo.net");
		ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

		std::stringstream sstr;
		sstr << "access_token=" << m_accessToken;
		sstr << "&device_id=" << m_thermostatDeviceID[reverseIdx];
		sstr << "&module_id=" << m_thermostatModuleID[reverseIdx];
		sstr << "&setpoint_mode=" << thermState;

		std::string httpData = sstr.str();

		std::string httpUrl("https://api.netatmo.net/api/setthermpoint");

		if (!HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
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
			Log(LOG_ERROR, "NetatmoThermostat: Invalid thermostat state!");
			return false;
		}
		std::string sPostData = "access_token=" + m_accessToken + "&home_id=" + m_Home_ID + "&mode=" + thermState;

		if (!HTTPClient::POST("https://api.netatmo.com/api/setthermmode", sPostData, ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
			return false;
		}
	}

	GetThermostatDetails();
	return true;
}

/// <summary>
/// Set temperture override on thermostat / valve for
/// one hour
/// </summary>
/// <param name="idx">ID of the device to override temp</param>
/// <param name="temp">Temperature to set</param>
void CNetatmo::SetSetpoint(int idx, const float temp)
{
	//Check if still connected to the API
	//connect to it if needed
	if (!m_isLogged == true)
	{
		if (!Login())
			return;
	}

	//Temp to set
	float tempDest = temp;
	unsigned char tSign = m_sql.m_tempsign[0];
	// convert back to Celsius
	if (tSign == 'F')
		tempDest = static_cast<float>(ConvertToCelsius(tempDest));

	//We change the setpoint for one hour
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
		if (!goodtime)
			localtime_r(&now, &etime);
	}

	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sstr;

	bool ret = false;
	if (m_energyType != NETYPE_ENERGY)
	{
		// Check if thermostat device is available
		if ((m_thermostatDeviceID[idx].empty()) || (m_thermostatModuleID[idx].empty()))
		{
			Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
			return;
		}

		ExtraHeaders.push_back("Host: api.netatmo.net");
		ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

		sstr << "access_token=" << m_accessToken;
		sstr << "&device_id=" << m_thermostatDeviceID[idx];
		sstr << "&module_id=" << m_thermostatModuleID[idx];
		sstr << "&setpoint_mode=manual&setpoint_temp=" << tempDest;
		sstr << "&setpoint_endtime=" << end_time;

		std::string httpData = sstr.str();
		ret = HTTPClient::POST("https://api.netatmo.net/api/setthermpoint", httpData, ExtraHeaders, sResult);
	}
	else
	{
		//find roomid
		std::string roomID = m_thermostatDeviceID[idx];
		if (roomID.empty())
		{
			Log(LOG_ERROR, "NetatmoThermostat: No thermostat or valve found in online devices!");
			return;
		}

		sstr << "access_token=" << m_accessToken;
		sstr << "&home_id=" << m_Home_ID;
		sstr << "&room_id=" << roomID;
		sstr << "&mode=manual&temp=" << tempDest;
		sstr << "&endtime=" << end_time;

		std::string sPostData = sstr.str();
		ret = HTTPClient::POST("https://api.netatmo.com/api/setroomthermpoint", sPostData, ExtraHeaders, sResult);
	}

	if (!ret)
	{
		Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint!");
		return;
	}

	//Retrieve new thermostat data
	GetThermostatDetails();
	//Set up for updating thermostat data when the set point reach its end
	m_tSetpointUpdateTime = time(nullptr) + 60;
	m_bForceSetpointUpdate = true;
}

/// <summary>
/// Change the schedule of the thermostat (new API only)
/// </summary>
/// <param name="scheduleId">ID of the schedule to use</param>
/// <returns>success status</returns>
bool CNetatmo::SetSchedule(int scheduleId)
{
	//Checking if we are still connected to the API
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}

	//Setting the schedule only if we have
	//the right thermostat type
	if (m_energyType == NETYPE_ENERGY)
	{
		std::string sResult;
		std::string thermState = "schedule";
		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back("Host: api.netatmo.net");
		ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

		std::string sPostData = "access_token=" + m_accessToken + "&home_id=" + m_Home_ID + "&mode=" + thermState + "&schedule_id=" + m_ScheduleIDs[scheduleId];

		if (!HTTPClient::POST("https://api.netatmo.com/api/setthermmode", sPostData, ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
			return false;
		}
		//store the selected schedule in our local data to avoid
		//changing back the schedule when using away mode
		m_selectedScheduleID = scheduleId;
	}

	return true;
}

/// <summary>
/// Utility to make the URI based on the type of device we
/// want to retrieve initial data
/// </summary>
/// <param name="NType">Netatmo device type</param>
/// <returns>API response</returns>
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

/// <summary>
/// Get details for wweather station then check and identify
/// thermostat device
/// </summary>
void CNetatmo::GetMeterDetails()
{
	//Check if connected to the API
	if (!m_isLogged)
		return;

	//Locals
	std::string httpUrl; //URI to be tested
	std::string sResult; // text returned by API
	Json::Value root; // root JSON object
	bool bRet; //Parsing status
	std::vector<std::string> ExtraHeaders; // HTTP Headers

	//check if user has an energy device (only once)
	if (m_bFirstTimeWeatherData)
	{
		//URI for energy device
		httpUrl = MakeRequestURL(NETYPE_ENERGY);
		if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "Error connecting to Server...");
			return;
		}

		// Check for error
		bRet = ParseJSon(sResult, root);
		if ((!bRet) || (!root.isObject()))
		{
			Log(LOG_ERROR, "Invalid data received...");
			return;
		}
		if (!root["error"].empty())
		{
			// We received an error
			Log(LOG_ERROR, "%s", root["error"]["message"].asString().c_str());
			m_isLogged = false;
			return;
		}

		//Parse API response
		bRet = ParseHomeData(sResult);
		if (bRet)
		{
			// Data was parsed with success so we have our device
			m_energyType = NETYPE_ENERGY;
		}
	}

	//Check if user has a weather or homecoach device
	httpUrl = MakeRequestURL(m_weatherType);
	if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error connecting to Server...");
		return;
	}

	//Check for error
	bRet = ParseJSon(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received...");
		return;
	}
	if (!root["error"].empty())
	{
		//We received an error
		Log(LOG_ERROR, "%s", root["error"]["message"].asString().c_str());
		m_isLogged = false;
		return;
	}

	//Parse API response
	if (!ParseStationData(sResult, false))
	{
		// User doesn't have a weather station, so we check if it has
		// a homecoach device (only once)
		if (m_bFirstTimeWeatherData)
		{
			// URI for homecoach device
			httpUrl = MakeRequestURL(NETYPE_HOMECOACH);
			if (!HTTPClient::GET(httpUrl, ExtraHeaders, sResult))
			{
				Log(LOG_ERROR, "Error connecting to Server...");
				return;
			}

			// Check for error
			bool bRet = ParseJSon(sResult, root);
			if ((!bRet) || (!root.isObject()))
			{
				Log(LOG_ERROR, "Invalid data received...");
				return;
			}
			if (!root["error"].empty())
			{
				// We received an error
				Log(LOG_ERROR, "%s", root["error"]["message"].asString().c_str());
				m_isLogged = false;
				return;
			}

			// Parse API Response
			bRet = ParseStationData(sResult, false);
			if (bRet)
				m_weatherType = NETYPE_HOMECOACH;
			else
				m_bPollWeatherData = false;
		}
	}

	m_bFirstTimeWeatherData = false;
}

/// <summary>
/// Get details for thermostat / valve
/// </summary>
void CNetatmo::GetThermostatDetails()
{
	//Check if connected to the API
	if (!m_isLogged)
		return;

	if (m_bFirstTimeThermostat)
		m_bFirstTimeThermostat = false;

	std::string sResult;
	std::stringstream sstr2;
	std::vector<std::string> ExtraHeaders;
	bool ret;

	if (m_energyType != NETYPE_ENERGY)
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
		Log(LOG_ERROR, "Error connecting to Server...");
		return;
	}
	if (m_energyType != NETYPE_ENERGY)
	{
		if (!ParseStationData(sResult, true))
			m_bPollThermostat = false;
	}
	else
	{
		if (!ParseHomeStatus(sResult))
			m_bPollThermostat = false;
	}
}

/// <summary>
/// Parse data for weather station and thermostat (with old API)
/// </summary>
/// <param name="sResult">JSON raw data to be parsed</param>
/// <param name="bIsThermostat">set to true if a thermostat is available</param>
/// <returns>success retreiving and parsing data status</returns>
bool CNetatmo::ParseStationData(const std::string& sResult, const bool bIsThermostat)
{
	//Check for well formed JSON data
	//and devices objects in the JSON reply
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_STATUS, "Invalid data received...");
		return false;
	}
	bool bHaveDevices = true;
	if (root["body"].empty())
		bHaveDevices = false;
	else if (root["body"]["devices"].empty())
		bHaveDevices = false;
	else if (!root["body"]["devices"].isArray())
		bHaveDevices = false;

	//Return error if no devices are found
	if (!bHaveDevices)
	{
		if ((!bIsThermostat) && (!m_bFirstTimeWeatherData) && (m_bPollWeatherData))
		{
			// Do not warn if we check if we have a Thermostat device
			Log(LOG_STATUS, "No Weather Station devices found...");
		}
		return false;
	}

	//Get data for the devices
	std::vector<_tNetatmoDevice> _netatmo_devices;
	int iDevIndex = 0;
	for (auto device : root["body"]["devices"])
	{
		if (!device["_id"].empty())
		{
			//Main Weather Station
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

			//Weather modules (Temp sensor, Wind Sensor, Rain Sensor)
			//Thermostat module
			if (!device["modules"].empty())
			{
				if (device["modules"].isArray())
				{
					// Add modules for this device
					for (auto module : device["modules"])
					{
						if (module.isObject())
						{
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
								mbattery_percent = module["battery_percent"].asInt();
							int mrf_status = 0;
							if (!module["rf_status"].empty())
							{
								// 90=low, 60=highest
								mrf_status = (90 - module["rf_status"].asInt()) / 3;
								if (mrf_status > 10)
									mrf_status = 10;
							}
							int crcId = Crc32(0, (const unsigned char*)mid.c_str(), mid.length());
							if (!module["dashboard_data"].empty())
								ParseDashboard(module["dashboard_data"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
							else if (m_energyType != NETYPE_ENERGY && !module["measured"].empty())
							{
								//We have a thermostat module : creating domoticz devices for the thermostat
								ParseDashboard(module["measured"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
								if (mtype == "NATherm1")
								{
									m_thermostatDeviceID[crcId & 0xFFFFFF] = nDevice.ID;
									m_thermostatModuleID[crcId & 0xFFFFFF] = mid;

									if (!module["setpoint"].empty())
									{
										if (!module["setpoint"]["setpoint_mode"].empty())
										{
											//Create Mode Switch (Away / Frost Guard)
											std::string setpoint_mode = module["setpoint"]["setpoint_mode"].asString();
											// create / update the switch for setting away mode
											// on the themostat
											if (setpoint_mode == "away")
												setpoint_mode = "20";
											else if (setpoint_mode == "hg")
												setpoint_mode = "30";
											else
												setpoint_mode = "10";
											SendSelectorSwitch(crcId & 0xFFFFFF, 3, setpoint_mode, mname + " - Mode", 15, true, "Off|On|Away|Frost Guard", "", true, m_Name);

										}
										// Check if setpoint was just set, and if yes, overrule the previous setpoint
										if (!module["setpoint"]["setpoint_temp"].empty())
											ParseDashboard(module["setpoint"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
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
				battery_percent = device["battery_percent"].asInt();
			int wifi_status = 0;
			if (!device["wifi_status"].empty())
			{
				// 86=bad, 56=good
				wifi_status = (86 - device["wifi_status"].asInt()) / 3;
				if (wifi_status > 10)
					wifi_status = 10;
			}
			int crcId = Crc32(0, (const unsigned char*)id.c_str(), id.length());
			if (!device["dashboard_data"].empty())
				ParseDashboard(device["dashboard_data"], iDevIndex, crcId, name, type, battery_percent, wifi_status);
		}
		iDevIndex++;
	}

	//Modules in old API
	Json::Value mRoot;
	if (root["body"]["modules"].empty())
	{
		// No additional modules defined
		return (!_netatmo_devices.empty());
	}
	if (!root["body"]["modules"].isArray())
	{
		// No additional modules defined
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

		// Find the corresponding _tNetatmoDevice
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
					iDevIndex = static_cast<int>(ittND - _netatmo_devices.begin());
					bHaveFoundND = true;
					break;
				}
			}
			if (bHaveFoundND == true)
				break;
		}

		if (!module["module_name"].empty())
			name = module["module_name"].asString();
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
			continue; // handled above

		int battery_percent = 0;
		if (!module["battery_percent"].empty())
			battery_percent = module["battery_percent"].asInt();
		int rf_status = 0;
		if (!module["rf_status"].empty())
		{
			// 90=low, 60=highest
			rf_status = (90 - module["rf_status"].asInt()) / 3;
			if (rf_status > 10)
				rf_status = 10;
		}
		stdreplace(name, "'", " ");

		int crcId = Crc32(0, (const unsigned char*)id.c_str(), id.length());
		if (!module["dashboard_data"].empty())
			ParseDashboard(module["dashboard_data"], iDevIndex, crcId, name, type, battery_percent, rf_status);
	}
	return (!_netatmo_devices.empty());
}

/// <summary>
/// Parse weather data for weather / homecoach station based on previously parsed JSON (with ParseStationData)
/// </summary>
/// <param name="root">JSON object to read</param>
/// <param name="DevIdx">Index of the device</param>
/// <param name="ID">ID of the module</param>
/// <param name="name">Name of the module (previously parsed)</param>
/// <param name="ModuleType">Type of module (previously parsed)</param>
/// <param name="battery_percent">battery percent (previously parsed)</param>
/// <param name="rssiLevel">radio network level (previously parsed)</param>
/// <returns>success retreiving and parsing data</returns>
bool CNetatmo::ParseDashboard(const Json::Value& root, const int DevIdx, const int ID, const std::string& name, const std::string& ModuleType, const int battery_percent, const int rssiLevel)
{
	//Local variable for holding data retreived
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
	float rain = 0;
	int sound = 0;

	int wind_angle = 0;
	float wind_strength = 0;
	float wind_gust = 0;

	int batValue = GetBatteryLevel(ModuleType, battery_percent);

	// check for Netatmo cloud data timeout, except if we deal with a thermostat
	if (ModuleType != "NATherm1")
	{
		std::time_t tNetatmoLastUpdate = 0;
		std::time_t tNow = time(nullptr);

		// initialize the relevant device flag
		if (m_bNetatmoRefreshed.find(ID) == m_bNetatmoRefreshed.end())
			m_bNetatmoRefreshed[ID] = true;
		// Check when dashboard data was last updated
		if (!root["time_utc"].empty())
			tNetatmoLastUpdate = root["time_utc"].asUInt();
		Debug(DEBUG_HARDWARE, "Module [%s] last update = %s", name.c_str(), ctime(&tNetatmoLastUpdate));
		// check if Netatmo data was updated in the past 10 mins (+1 min for sync time lags)... if not means sensors failed to send to cloud
		if (tNetatmoLastUpdate > (tNow - 660))
		{
			if (!m_bNetatmoRefreshed[ID])
			{
				Log(LOG_STATUS, "cloud data for module [%s] is now updated again", name.c_str());
				m_bNetatmoRefreshed[ID] = true;
			}
		}
		else
		{
			if (m_bNetatmoRefreshed[ID])
				Log(LOG_ERROR, "cloud data for module [%s] no longer updated (module possibly disconnected)", name.c_str());
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
		if ((!root["WindAngle"].empty()) && (!root["WindStrength"].empty()) && (!root["GustAngle"].empty()) && (!root["GustStrength"].empty()))
		{
			bHaveWind = true;
			wind_angle = root["WindAngle"].asInt();
			wind_strength = root["WindStrength"].asFloat() / 3.6F;
			wind_gust = root["GustStrength"].asFloat() / 3.6F;
		}
	}

	//Data retreived create / update appropriate domoticz devices
	//Temperature and humidity sensors
	if (bHaveTemp && bHaveHum && bHaveBaro)
	{
		int nforecast = m_forecast_calculators[ID].CalculateBaroForecast(temp, baro);
		SendTempHumBaroSensorFloat(ID, batValue, temp, hum, baro, (uint8_t)nforecast, name, rssiLevel);
	}
	else if (bHaveTemp && bHaveHum)
		SendTempHumSensor(ID, batValue, temp, hum, name, rssiLevel);
	else if (bHaveTemp)
		SendTempSensor(ID, batValue, temp, name, rssiLevel);

	//Thermostat device
	if (bHaveSetpoint)
	{
		std::string sName = name + " - SetPoint ";
		SendSetPointSensor((uint8_t)((ID & 0x00FF0000) >> 16), (ID & 0XFF00) >> 8, ID & 0XFF, sp_temp, sName);
	}

	//Rain meter
	if (bHaveRain)
	{
		bool bRefetchData = (m_RainOffset.find(ID) == m_RainOffset.end());
		if (!bRefetchData)
			bRefetchData = ((m_RainOffset[ID] == 0) && (m_OldRainCounter[ID] == 0));
		if (bRefetchData)
		{
			// get last rain counter from the database
			bool bExists = false;
			m_RainOffset[ID] = GetRainSensorValue(ID, bExists);
			m_RainOffset[ID] -= rain;
			if (m_RainOffset[ID] < 0)
				m_RainOffset[ID] = 0;
			if (m_OldRainCounter.find(ID) == m_OldRainCounter.end())
				m_OldRainCounter[ID] = 0;
		}
		// daily counter went to zero ?
		if (rain < m_OldRainCounter[ID])
			m_RainOffset[ID] += m_OldRainCounter[ID];

		m_OldRainCounter[ID] = rain;
		SendRainSensor(ID, batValue, m_RainOffset[ID] + m_OldRainCounter[ID], name, rssiLevel);
	}

	if (bHaveCO2)
		SendAirQualitySensor(ID, DevIdx, batValue, co2, name);

	if (bHaveSound)
		SendSoundSensor(ID, batValue, sound, name);

	if (bHaveWind)
		SendWind(ID, batValue, wind_angle, wind_strength, wind_gust, 0, 0, false, false, name, rssiLevel);

	return true;
}

/// <summary>
/// Parse data for energy station (thermostat and valves) and get
/// module / room and schedule
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <returns>success parsing the data</returns>
bool CNetatmo::ParseHomeData(const std::string& sResult)
{
	//Check if JSON is Ok to parse
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_STATUS, "Invalid data received...");
		return false;
	}
	//Check if we have a home id
	if (!root["body"]["homes"].empty())
	{
		//We support only one home for now
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
					int crcId = Crc32(0, (const unsigned char*)mID.c_str(), mID.length());
					m_ModuleIDs[mID] = crcId;
					//Store thermostate name for later naming switch / sensor
					if (module["type"] == "NATherm1")
						m_ThermostatName = module["name"].asString();
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
					int crcId = Crc32(0, (const unsigned char*)rID.c_str(), rID.length());
					m_RoomIDs[rID] = crcId;
				}
			}

			//Get the schedules
			if (!root["body"]["homes"][m_ActHome]["schedules"].empty())
			{
				mRoot = root["body"]["homes"][m_ActHome]["schedules"];

				std::string allSchName = "Off";
				std::string allSchAction = "00";
				int index = 0;
				for (auto schedule : mRoot)
				{
					if (!schedule["id"].empty())
					{
						std::string sID = schedule["id"].asString();
						index += 10;
						m_ScheduleNames[index] = schedule["name"].asString();
						m_ScheduleIDs[index] = sID;
						if (!schedule["selected"].empty() && schedule["selected"].asBool())
							m_selectedScheduleID = index;
					}
				}
			}

			return true;
		}
	}
	if ((!m_bFirstTimeWeatherData) && (m_bPollWeatherData))
	{
		//Do not warn if we check if we have a Thermostat device
		Log(LOG_STATUS, "No Weather Station devices found...");
	}
	return false;
}

/// <summary>
/// Parse data for energy station (thermostat and valves) get
/// temperatures and create/update domoticz devices
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <returns></returns>
bool CNetatmo::ParseHomeStatus(const std::string& sResult)
{
	//Check if JSON is Ok to parse
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_STATUS, "Invalid data received...");
		return false;
	}

	if (root["body"].empty())
		return false;

	if (root["body"]["home"].empty())
		return false;

	int thermostatID;
	//Parse module and create / update domoticz devices
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

				// Find the module (name/id)
				moduleName = m_ModuleNames[id];
				moduleID = m_ModuleIDs[id];

				int batteryLevel = 255;
				if (!module["boiler_status"].empty())
				{
					//Thermostat status (boiler heating or not : informationnal switch)
					std::string aName = m_ThermostatName + " - Heating Status";
					bool bIsActive = (module["boiler_status"].asString() == "true");
					thermostatID = moduleID;
					SendSwitch(moduleID, 0, 255, bIsActive, 0, aName, m_Name);

					//Thermostat schedule switch (actively changing thermostat schedule)
					std::string allSchName = "Off";
					std::string allSchAction = "";
					for (std::map<int, std::string>::const_iterator itt = m_ScheduleNames.begin(); itt != m_ScheduleNames.end(); ++itt)
					{
						allSchName = allSchName + "|" + itt->second;
						std::stringstream ss;
						ss << itt->first;
					}
					//Selected Index for the dropdown list
					std::stringstream ssv;
					ssv << m_selectedScheduleID;
					//create update / domoticz device
					SendSelectorSwitch(moduleID, 2, ssv.str(), m_ThermostatName + " - Schedule", 15, true, allSchName, allSchAction, true, m_Name);
				}
				//Device to get battery level / network strength for valves
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
						std::string pName = moduleName + " - Sig. + Bat. Lvl";
						SendPercentageSensor(moduleID, 1, batteryLevel, mrf_percentage, pName);
					}
				}
			}
			iModuleIndex++;
		}
	}
	//Parse Rooms
	int iDevIndex = 0;
	bool setModeSwitch = false;
	if (!root["body"]["home"]["rooms"].empty())
	{
		if (!root["body"]["home"]["rooms"].isArray())
			return false;
		Json::Value mRoot = root["body"]["home"]["rooms"];

		for (auto room : mRoot)
		{
			if (!room["id"].empty())
			{
				std::string roomNetatmoID = room["id"].asString();
				std::string roomName = roomNetatmoID;
				int roomID = iDevIndex + 1;

				//Find the room name
				roomName = m_RoomNames[roomNetatmoID];
				roomID = m_RoomIDs[roomNetatmoID];

				m_thermostatDeviceID[roomID & 0xFFFFFF] = roomNetatmoID;
				m_thermostatModuleID[roomID & 0xFFFFFF] = roomNetatmoID;

				//Create / update domoticz devices : Temp sensor / Set point sensor for each room
				if (!room["therm_measured_temperature"].empty())
					SendTempSensor((roomID & 0x00FFFFFF) | 0x03000000, 255, room["therm_measured_temperature"].asFloat(), roomName);
				if (!room["therm_setpoint_temperature"].empty())
					SendSetPointSensor((uint8_t)(((roomID & 0x00FF0000) | 0x02000000) >> 16), (roomID & 0XFF00) >> 8, roomID & 0XFF, room["therm_setpoint_temperature"].asFloat(), roomName);

				if (!setModeSwitch)
				{
					// create / update the switch for setting away mode
					// on the themostat (we could create one for each room also,
					// but as this is not something we can do on the app, we don't here)
					std::string setpoint_mode = room["therm_setpoint_mode"].asString();
					if (setpoint_mode == "away")
						setpoint_mode = "20";
					else if (setpoint_mode == "hg")
						setpoint_mode = "30";
					else
						setpoint_mode = "10";
					SendSelectorSwitch(thermostatID, 3, setpoint_mode, m_ThermostatName + " - Mode", 15, true, "Off|On|Away|Frost Guard", "", true, m_Name);

					setModeSwitch = true;
				}
			}
			iDevIndex++;
		}
	}

	return (iDevIndex > 0);
}

/// <summary>
/// Normalize battery level for station module
/// </summary>
/// <param name="ModuleType">Module type</param>
/// <param name="battery_percent">battery percent</param>
/// <returns>normalized battery level</returns>
int CNetatmo::GetBatteryLevel(const std::string& ModuleType, int battery_percent)
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

		// range = 1000
		batValue = 3200 - battery_percent;
		batValue = 100 - int((100.0F / 1000.0F) * float(batValue));
	}
	else if (ModuleType == "NATherm1")
	{
		if (battery_percent > 4100)
			battery_percent = 4100;
		else if (battery_percent < 3000)
			battery_percent = 3000;

		// range = 1100
		batValue = 4100 - battery_percent;
		batValue = 100 - int((100.0F / 1100.0F) * float(batValue));
	}

	return batValue;
}
