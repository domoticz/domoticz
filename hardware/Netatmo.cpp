#include "stdafx.h"
#include "Netatmo.h"
#include "hardwaretypes.h"
#include "../notifications/NotificationHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"


#define NETATMO_OAUTH2_TOKEN_URI "https://api.netatmo.com/oauth2/token"
#define NETATMO_API_URI "https://api.netatmo.com/"
#define NETATMO_SCOPES "read_station read_smarther write_smarther read_thermostat write_thermostat read_camera write_camera read_doorbell read_presence write_presence read_homecoach read_carbonmonoxidedetector read_smokedetector"
#define NETATMO_REDIRECT_URI "http://localhost/netatmo"
// https://api.netatmo.com/oauth2/authorize?client_id=<CLIENT_ID>&redirect_uri=http://localhost/netatmo&state=teststate&scope=read_station%20read_smarther%20write_smarther%20read_thermostat%20write_thermostat%20read_camera%20write_camera%20read_doorbell%20read_presence%20write_presence%20read_homecoach%20read_carbonmonoxidedetector%20read_smokedetector


#ifdef _DEBUG
//#define DEBUG_NetatmoWeatherStationR
#endif


#ifdef DEBUG_NetatmoWeatherStationW
void SaveString2Disk(Json::Value str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
                std::string buffer;
                buffer = JSonToFormatString(str);
                //buffer = JSonToRawString(str);
		fwrite(buffer.c_str(), 1, buffer.size(), fOut);
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


std::string bool_as_text(bool b)
{
                std::stringstream converter;
                converter << std::boolalpha << b;   // flag boolalpha calls converter.setf(std::ios_base::boolalpha)
                return converter.str();
}


uint64_t convert_mac(std::string mac)
{
        // Remove colons
        mac.erase(std::remove(mac.begin(), mac.end(), ':'), mac.end());
        // Convert to uint64_t
        return strtoul(mac.c_str(), NULL, 16);
}


/// <summary>
/// Send sensors to Main worker
///
/// </summary>
uint64_t CNetatmo::UpdateValueInt(int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
        const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
{
	uint64_t DeviceRowIdx = m_sql.UpdateValue(m_HwdID, HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction, user.c_str());
        if (DeviceRowIdx == (uint64_t)-1)
                return -1;
        if (m_bOutputLog)
        {
                std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
                Log(LOG_NORM, szLogString);
        }
        m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
        m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, ID, devname, unit, devType, subType, nValue, sValue);
        m_mainworker.CheckSceneCode(DeviceRowIdx, devType, subType, nValue, sValue, "MQTT Auto");
        return DeviceRowIdx;
}


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

//        m_weatherType = NETYPE_WEATHER_STATION;
//        m_homecoachType = NETYPE_AIRCARE;
//        m_energyType = NETYPE_ENERGY;

	m_HwdID = ID;
        m_Home_ID = "";

	m_ActHome = 0;

	m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bPollHomecoachData = true;
	m_bPollHomeStatus = true;
	m_bPollHome = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_tSetpointUpdateTime = time(nullptr);

	Init();
}


void CNetatmo::Init()
{
	m_RainOffset.clear();
	m_OldRainCounter.clear();
	m_Room.clear();
        m_RoomNames.clear();
	m_RoomIDs.clear();
	m_Room_status.clear();
	m_ModuleNames.clear();
	m_ModuleIDs.clear();
	m_Types.clear();
        m_Module_category.clear();
        m_thermostatDeviceID.clear();
	m_thermostatModuleID.clear();
        m_Camera_Name.clear();
        m_Camera_ID.clear();
        m_Smoke_Name.clear();
        m_Smoke_ID.clear();;
	m_Persons.clear();
        m_PersonsNames.clear();
        m_PersonsIDs.clear();
        m_ScheduleNames.clear();
	m_ScheduleIDs.clear();
        m_ZoneNames.clear();
        m_ZoneIDs.clear();
	m_Events_Device_ID.clear();

        m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bPollHomecoachData = true;
	m_bPollHomeStatus = true;
	m_bPollHome = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_bForceSetpointUpdate = false;

//        m_energyType = NETYPE_ENERGY;
//        m_weatherType = NETYPE_WEATHER_STATION;
//        m_homecoachType = NETYPE_AIRCARE;

	m_bForceLogin = false;
        //
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
	bool bFirstTimeHS = true;
	bool bFirstTimeSS = true;
	bool bFirstTimeCS = true;
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
                                // Thermostat is accessable through Homestatus / Homesdata in New API
                                //Thermostat data is updated every 10 minutes
                                // https://api.netatmo.com/api/getthermostatsdata
//                                if ((sec_counter % 900 == 0) || (bFirstTimeTH))
//                                {
//                                        bFirstTimeTH = false;
//                                        if ((m_bPollThermostat) || (sec_counter % 1200 == 0))
//                                                //
//                                                GetThermostatDetails();
//                                                Log(LOG_STATUS, "Thermostat. %d",  m_isLogged);
//                                                //
//                                }
//
                                // Deprecated HomeData
                                //"https://api.netatmo.com/api/homedata";
                                //
//                                if ((sec_counter % 900 == 0) || (bFirstTimeCS))
//                                {
//                                        bFirstTimeCS = false;
//                                        if ((m_bPollHome) || (sec_counter % 1200 == 0))
//                                                GetHomeDetails();
//                                                Log(LOG_STATUS, "HomeDetails. %d",  m_isLogged);
//                                }
				//
				//
				//Weather / HomeCoach data is updated every 10 minutes
				// 03/03/2022 - PP Changing the Weather polling from 600 to 900s. This has reduce the number of server errors,
				// but do not prevennt to have one time to time
				if ((sec_counter % 900 == 0) || (bFirstTimeWS))
				{
					bFirstTimeWS = false;
					if ((m_bPollWeatherData) || (sec_counter % 1200 == 0))
                                                // ParseStationData
						GetWeatherDetails();
                                                Debug(DEBUG_HARDWARE,"Weather %d",  m_isLogged);
                                                Debug(DEBUG_HARDWARE, "Weather %s", m_Home_ID.c_str());
				}

				if ((sec_counter % 900 == 0) || (bFirstTimeHS))
				{
					bFirstTimeHS = false;
					if ((m_bPollHomecoachData) || (sec_counter % 1200 == 0))
                                                // ParseStationData
						GetHomecoachDetails();
                                                Debug(DEBUG_HARDWARE,"HomeCoach %d",  m_isLogged);
				}

				if ((sec_counter % 900 == 0) || (bFirstTimeSS))
				{
					bFirstTimeSS = false;
					if ((m_bPollHomeStatus) || (sec_counter % 1200 == 0))
                                                // GetHomesDataDetails
						GetHomeStatusDetails();
                                                Debug(DEBUG_HARDWARE,"Status %d",  m_isLogged);
				}
				//
				//Update Thermostat data when the
				//manual set point reach its end
				if (m_bForceSetpointUpdate)
				{
					time_t atime = time(nullptr);
					if (atime >= m_tSetpointUpdateTime)
					{
						m_bForceSetpointUpdate = false;
						if (m_bPollThermostat)
                                                        //Needs function  -  TO DO
                                                        Debug(DEBUG_HARDWARE, "Home HS %s", m_Home_ID.c_str());
//							GetThermostatDetails();
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
//        std::string httpUrl(NETATMO_API_URI + "oauth2/token")
	std::string httpUrl(NETATMO_OAUTH2_TOKEN_URI);
	std::string sResult;
        Debug(DEBUG_HARDWARE, "Token %s", httpUrl.c_str());

	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);

	//Check for returned data
	if (!ret)
	{
		Log(LOG_ERROR, "Error connecting to Server... Token");
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
//        std::string httpUrl(NETATMO_API_URI + "oauth2/token")
	std::string httpUrl(NETATMO_OAUTH2_TOKEN_URI);
	std::string sResult;
        Debug(DEBUG_HARDWARE, "Refresh %s", httpUrl.c_str());

	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);

	//Check for returned data
	if (!ret)
	{
		Log(LOG_ERROR, "Error connecting to Server...Refresh");
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
/// Upon domoticz devices action (pressing a switch) take action          - TO DO
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
/// Set the thermostat / valve in "away mode"                             - TO DO
/// </summary>
/// <param name="idx">ID of the device to set in away mode</param>
/// <param name="bIsAway">wether to put in away or normal / schedule mode</param>
/// <returns>success status</returns>
bool CNetatmo::SetAway(const int idx, const bool bIsAway)
{
	return SetProgramState(idx, (bIsAway == true) ? 1 : 0);
}


/// <summary>
/// Set the thermostat / valve operationnal mode                          - TO DO
/// </summary>
/// <param name="idx">ID of the device to put in away mode</param>
/// <param name="newState">Mode of the device (0 = schedule / normal; 1 = away mode; 2 = frost guard; 3 = off (not supported in new API)</param>
/// <returns>success status</returns>
bool CNetatmo::SetProgramState(const int idx, const int newState)
{
	//Check if logged, logging if needed
	if (!m_isLogged == true)               //Double ifnot m_isLogged is true
	{
		if (!Login())
			return false;
	}

	std::vector<std::string> ExtraHeaders;
	std::string sResult;
        Json::Value root;       // root JSON object
        std::string home_data = "";
        bool bRet;              //Parsing status

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

                Get_Respons_API(NETYPE_SETTHERMPOINT, sResult, home_data, bRet, root);
                if (!bRet)
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

                Get_Respons_API(NETYPE_SETTHERMPOINT, sResult, home_data, bRet, root);
		if (!bRet)
                {
			Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint Mode !");
			return false;
		}
	}

//	GetThermostatDetails();
	return true;
}


/// <summary>
/// Set temperture override on thermostat / valve for                     - TO DO
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
	while (!goodtime)
        {
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
	std::stringstream bstr;
        Json::Value root;       // root JSON object
        std::string home_data = "";
	bool ret = false;
        bool bRet;              //Parsing status
	if (m_energyType != NETYPE_ENERGY)
	{
		// Check if thermostat device is available
		if ((m_thermostatDeviceID[idx].empty()) || (m_thermostatModuleID[idx].empty()))
		{
			Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
			return;
		}

                Get_Respons_API(NETYPE_SETTHERMPOINT, sResult, home_data, bRet, root);

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
                Get_Respons_API(NETYPE_SETROOMTHERMPOINT, sResult, home_data, bRet, root);

	}

	if (!bRet)
	{
		Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint!");
		return;
	}

	//Retrieve new thermostat data
//	GetThermostatDetails();
	//Set up for updating thermostat data when the set point reach its end
	m_tSetpointUpdateTime = time(nullptr) + 60;
	m_bForceSetpointUpdate = true;
}


/// <summary>
/// Change the schedule of the thermostat (new API only)                  - TO DO
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
	std::stringstream bstr;
        std::string sResult;
        Json::Value root;       // root JSON object
        std::string home_data = "";
        bool bRet;              //Parsing status
	if (m_energyType == NETYPE_ENERGY)
	{

                Get_Respons_API(NETYPE_SETTHERMMODE, sResult, home_data, bRet, root);

                if (!bRet)
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
std::string CNetatmo::MakeRequestURL(const _eNetatmoType NType, std::string data)
{
	std::stringstream sstr;

	switch (NType)
	{
	case NETYPE_MEASURE:
		sstr << NETATMO_API_URI;
                sstr << "api/getmeasure";
                //"https://api.netatmo.com/api/getmeasure";
		break;
	case NETYPE_WEATHER_STATION:
		sstr << NETATMO_API_URI;
                sstr << "api/getstationsdata";
                //"https://api.netatmo.com/api/getstationsdata";
		break;
	case NETYPE_AIRCARE:
		sstr << NETATMO_API_URI;
                sstr << "api/gethomecoachsdata";
                //"https://api.netatmo.com/api/gethomecoachsdata";
		break;
	case NETYPE_THERMOSTAT:        // OLD API
		sstr << NETATMO_API_URI;
                sstr << "api/getthermostatsdata";
                //"https://api.netatmo.com/api/getthermostatsdata";
		break;
	case NETYPE_HOME:              // OLD API
		sstr << NETATMO_API_URI;
                sstr << "api/homedata";
                //"https://api.netatmo.com/api/homedata";
		break;
	case NETYPE_HOMESDATA:         // was NETYPE_ENERGY
		sstr << NETATMO_API_URI;
                sstr << "api/homesdata";
                //"https://api.netatmo.com/api/homesdata";
		break;
	case NETYPE_STATUS:
		sstr << NETATMO_API_URI;
                sstr << "api/homestatus";
                //"https://api.netatmo.com/api/homestatus";
		break;
	case NETYPE_CAMERAS:           // OLD API
		sstr << NETATMO_API_URI;
                sstr << "api/getcamerapicture";
                //"https://api.netatmo.com/api/getcamerapicture";
		break;
	case NETYPE_EVENTS:
		sstr << NETATMO_API_URI;
                sstr << "api/getevents";
                //"https://api.netatmo.com/api/getevents";
		break;
        case NETYPE_SETSTATE:
                sstr << NETATMO_API_URI;
                sstr << "api/setstate";
                //"https://api.netatmo.com/api/setstate"
                break;
        case NETYPE_SETROOMTHERMPOINT:
                sstr << NETATMO_API_URI;
                sstr << "api/setroomthermpoint";
                //"https://api.netatmo.com/api/setroomthermpoint"
                break;
        case NETYPE_SETTHERMMODE:
                sstr << NETATMO_API_URI;
                sstr << "api/setthermmode";
                //"https://api.netatmo.com/api/setthermmode";
                break;
        case NETYPE_SETPERSONSAWAY:
                sstr << NETATMO_API_URI;
                sstr << "api/setpersonsaway";
                //"https://api.netatmo.com/api/setpersonsaway";
                break;
        case NETYPE_SETPERSONSHOME:
                sstr << NETATMO_API_URI;
                sstr << "api/setpersonshome";
                //"https://api.netatmo.com/api/setpersonshome"
                break;
        case NETYPE_NEWHOMESCHEDULE:
                sstr << NETATMO_API_URI;
                sstr << "api/createnewhomeschedule";
                //"https://api.netatmo.com/api/createnewhomeschedule"
                break;
        case NETYPE_SYNCHOMESCHEDULE:
                sstr << NETATMO_API_URI;
                sstr << "api/synchomeschedule";
                //"https://api.netatmo.com/api/synchomeschedule"
                break;
        case NETYPE_SWITCHHOMESCHEDULE:
                sstr << NETATMO_API_URI;
                sstr << "api/switchhomeschedule";
                //"https://api.netatmo.com/api/switchhomeschedule"
                break;
        case NETYPE_ADDWEBHOOK:
                sstr << NETATMO_API_URI;
                sstr << "api/addwebhook";
                //"https://api.netatmo.com/api/addwebhook"
                break;
        case NETYPE_DROPWEBHOOK:
                sstr << NETATMO_API_URI;
                sstr << "api/dropwebhook";
                //"https://api.netatmo.com/api/dropwebhook"
                break;
        case NETYPE_PUBLICDATA:
                sstr << NETATMO_API_URI;
                sstr << "api/getpublicdata";
                //"https://api.netatmo.com/api/getpublicdata";
                break;

	default:
		return "";
	}

        sstr << "?";
        sstr << data;
        sstr << "&";
        sstr << "access_token=" << m_accessToken;
        sstr << "&";
	sstr << "get_favorites=" << "true";
	return sstr.str();
}


/// <summary>
/// Get API
/// </summary>
void CNetatmo::Get_Respons_API(const _eNetatmoType& NType, std::string& sResult, std::string& home_data , bool& bRet, Json::Value& root )
{
        //
        //Check if connected to the API
        if (!m_isLogged)
                return;
        //Locals
        std::string httpUrl;                             //URI to be tested
        std::string data;

        std::stringstream sstr;
        sstr << "";
        std::vector<std::string> ExtraHeaders;           // HTTP Headers
        //
        httpUrl = MakeRequestURL(NType, home_data);

        std::string sPostData = "";
        Debug(DEBUG_HARDWARE, "Respons URL   %s", httpUrl.c_str());
        //
        if (!HTTPClient::POST(httpUrl, sPostData,  ExtraHeaders, sResult))
        {
                Log(LOG_ERROR, "Error connecting to Server...");
                return ;
        }

        //Check for error
        bRet = ParseJSon(sResult, root);
        if ((!bRet) || (!root.isObject()))
        {
                Log(LOG_ERROR, "Invalid data received...J");
                return ;
        }
        if (!root["error"].empty())
        {
                //We received an error
                Log(LOG_ERROR, "Error %s", root["error"]["message"].asString().c_str());
                m_isLogged = false;
                return ;
        }
        //
}


/// <summary>
/// Get details for home to be used in GetHomeStatusDetails / ParseHomeStatus
/// </summary>
void CNetatmo::GetHomesDataDetails()
{
	//Locals
	std::string sResult;  // text returned by API
        std::string home_data = "";
        bool bRet;            //Parsing status
        Json::Value root;     // root JSON object

        Get_Respons_API(NETYPE_HOMESDATA, sResult, home_data, bRet, root);
        //
        if (!root["body"]["homes"].empty())
	{
                //
                for (auto home : root["body"]["homes"])
                {
                        if (!home["id"].empty())
                        {
                                // Home ID from Homesdata
                                m_Home_ID = "home_id=" + home["id"].asString();
                                std::string Home_Name = home["name"].asString();
                                // home["altitude"];
                                // home["coordinates"]; //List [5.590753555297852, 51.12159997589948]
                                // home["country"].asString();
                                // home["timezone"].asString();
                                Log(LOG_STATUS, "Home id %s updated.", Home_Name.c_str());

                                //Get the rooms
                                if (!home["rooms"].empty())
                                {
                                        for (auto room : home["rooms"])
                                        {
                                                 //Json::Value room = home["rooms"];
                                                 std::string roomID = room["id"].asString();
                                                 m_RoomNames[roomID] = room["name"].asString();
                                                 std::string roomTYPE = room["type"].asString();
                                                 m_Types[roomID] = roomTYPE;

                                                 int crcId = 0;
                                                 for (auto module_ids : room["module_ids"])
                                                 {
                                                         // List
                                                         m_Room[roomID] = module_ids.toStyledString();

                                                         crcId = Crc32(0, (const unsigned char*)roomID.c_str(), roomID.length());
                                                         m_ModuleIDs[roomID] = crcId;    //For temperature devices in Rooms
                                                         m_RoomIDs[roomID] = crcId;
                                                 }
                                                 if (!room["category"].empty())
                                                        m_Module_category[roomID] = room["category"].asString();
                                                 Debug(DEBUG_HARDWARE, "mID %s - Type %s - Name %s - categorie %s - crcID %d", roomID.c_str(), roomTYPE.c_str(), m_RoomNames[roomID].c_str(), m_Module_category[roomID].c_str(), crcId );
                                        }
                                }
                                //Get the module names
                                if (!home["modules"].empty())
                                {
                                        for (auto module : home["modules"])
                                        {
                                                //
                                                if (!module["id"].empty())
                                                {
                                                        std::string type = module["type"].asString();
                                                        std::string mID = module["id"].asString();
                                                        //Debug(DEBUG_HARDWARE, "Type %s", type.c_str());
                                                        m_ModuleNames[mID] = module["name"].asString();
                                                        int crcId = Crc32(0, (const unsigned char*)mID.c_str(), mID.length());
                                                        m_ModuleIDs[mID] = crcId;
                                                        Debug(DEBUG_HARDWARE, "mID %s - Type %s - Name %s - crcID %d", mID.c_str(), type.c_str(), m_ModuleNames[mID].c_str(), crcId );
                                                        //Debug(DEBUG_HARDWARE, "Name %s", module["name"].asString().c_str());
                                                        //Store thermostate name for later naming switch / sensor
                                                        if (module["type"] == "NATherm1")
                                                                m_ThermostatName[mID] = module["name"].asString();
                                                        if (module["type"] == "NRV")
                                                                m_ThermostatName[mID] = module["name"].asString();
                                                }
                                        }
                                }
                                if (!home["temperature_control_mode"].empty())
                                {
                                        std::string control_mode = home["temperature_control_mode"].asString();
                                }
                                if (!home["therm_mode"].empty())
                                {
                                        std::string schedule_mode = home["therm_mode"].asString();
                                }
                                if (!home["therm_setpoint_default_duration"].empty())
                                {
                                        std::string therm_setpoint_default_duration = home["therm_setpoint_default_duration"].asString();
                                }
                                //Get the persons
                                if (!home["persons"].empty())
                                {
                                        for (auto person : home["persons"])
                                        {
                                                std::string person_id = person["id"].asString();
                                                if (!person["pseudo"].empty())
                                                        std::string person_name = person["pseudo"].asString();
                                                std::string person_url = person["url"].asString();
                                        }
                                }
                                //Get the schedules
                                if (!home["schedules"].empty())
                                {
                                        for (auto schedule : home["schedules"])
                                        {
                                                for (auto timetable : schedule["timetable"])
                                                {
                                                        bool zone_id = timetable["zone_id"].asBool();
                                                        bool zone_offset = timetable["m_offset"].asBool();
                                                        // mutiple zone_id '0' in list ??
                                                }
                                                for (auto zone : schedule["zones"])
                                                {
                                                        std::string zone_name = zone["name"].asString();
                                                        bool zone_id = zone["id"].asBool();
                                                        bool zone_type = zone["type"].asBool();
                                                        for (auto zone_room : zone["rooms_temp"])
                                                        {
                                                                std::string zone_room_id = zone_room["room_id"].asString();
                                                                bool zone_room_temp = zone_room["temp"].asBool();
                                                        }
                                                        for (auto room : zone["rooms"])
                                                        {
                                                                std::string room_id = room["id"].asString();
                                                                bool room_temp = room["therm_setpoint_temperature"].asBool();
                                                        }
                                                }
                                                //
                                                std::string schedule_name = schedule["name"].asString();
                                                bool schedule_default = schedule["default"].asBool();                 // true / false
                                                bool schedule_away_temp = schedule["away_temp"].asBool();
                                                bool schedule_hg_temp = schedule["hg_temp"].asBool();
                                                std::string schedule_id = schedule["id"].asString();
                                                std::string schedule_type = schedule["type"].asString();
                                                bool schedule_selected = schedule["selected"].asBool();                // true / false
                                        }
                                }
                                //Get the user info
                                if (!home["user"].empty())
                                {
                                        for (auto user : home["user"])
                                        {
                                                std::string user_mail = user["mail"].asString();
                                                std::string user_language = user["language"].asString();
                                                std::string user_locale = user["locale"].asString();
                                                bool user_feel = user["feel_like_algorithm"].asBool();
                                                bool user_pressure = user["unit_pressure"].asBool();
                                                bool user_system = user["unit_system"].asBool();
                                                bool user_wind = user["unit_wind"].asBool();
                                                std::string user_id = user["id"].asString();
                                        }
                                }
                        }
                }
        }
}


/// <summary>
/// Get details for weather station
/// </summary>
void CNetatmo::GetWeatherDetails()
{
	//Check if connected to the API
	if (!m_isLogged)
		return;
	//
        if (m_bFirstTimeWeatherData)
        {
        	std::string sResult; // text returned by API
                std::string home_data = "";
                bool bRet;           //Parsing
	        Json::Value root;    // root JSON object
                //
                Get_Respons_API(NETYPE_WEATHER_STATION, sResult, home_data, bRet, root);
                //
		//Parse API response
	        bRet = ParseStationData(sResult, false);
//                if (bRet)
//		{
//        		// Data was parsed with success so we have our device
//			m_weatherType = NETYPE_WEATHER_STATION;
//		}
		m_bPollWeatherData = false;
	}
//	m_bFirstTimeWeatherData = false;
}


/// <summary>
/// Get details for homecoach
/// </summary>
void CNetatmo::GetHomecoachDetails()
{
	//Check if connected to the API
	if (!m_isLogged)
		return;
	//Locals
	std::string sResult; // text returned by API
	std::string home_data = "";
	bool bRet;           //Parsing status
        Json::Value root;    // root JSON object
        //
        Get_Respons_API(NETYPE_AIRCARE, sResult, home_data, bRet, root);
	//
	//Parse API response
	bRet = ParseStationData(sResult, false);
//	if (bRet)
//	{
//			// Data was parsed with success so we have our device
//			m_homecoachType = NETYPE_AIRCARE;
//	}
	m_bPollHomecoachData = false;
}


/// <summary>
/// Get details for homeStatus
/// </summary>
void CNetatmo::GetHomeStatusDetails()
{
	//Check if connected to the API
	if (!m_isLogged)
		return;

	//Locals
	std::string sResult;                   // text returned by API
	std::string home_data = m_Home_ID;
        bool bRet;                             //Parsing status
	Json::Value root;                      // root JSON object
	std::string device_types = "";
        std::string event_id = "";
        std::string person_id = "";
        std::string bridge_id = "";
        std::string module_id = "";
        int offset = ' ';
        int size = ' ';
        std::string locale = "";
	//
        GetHomesDataDetails();                 //Homes Data
	//
        Get_Respons_API(NETYPE_STATUS, sResult, home_data, bRet, root);
	//
	//Parse API response
	bRet = ParseHomeStatus(sResult, root);
	//
	Get_Events(home_data, device_types, event_id, person_id, bridge_id, module_id, offset, size, locale);
//	if (bRet)
//	{
//			// Data was parsed with success so we have our device
//			m_energyType = NETYPE_STATUS;
//	}
	m_bPollHomeStatus = false;
}


/// <summary>
/// Get Historical data from Netatmo Device
/// Devices from Aircare, Weather, Energy and Home + Control API
/// </summary>
void CNetatmo::Get_Measure(std::string gateway, std::string module_id, std::string scale)
{
        //Check if connected to the API
        if (!m_isLogged)
                return ;

        //Locals
        std::string sResult; // text returned by API
        Json::Value root;    // root JSON object
        // scale = "30min";  //{30min, 1hour, 3hours, 1day, 1week, 1month}
        std::string date_begin = "";
        std::string date_end = "";
        std::string limit = "30";  //
        // https://api.netatmo.com/api/getmeasure?device_id=xxx&module_id=xxx&scale=30min&type=sum_boiler_on&type=boileron&type=boileroff&type=sum_boiler_off&date_begin=xxxx&date_end=xxxx&limit=10&optimize=false&real_time=false
        //type=  sum_boiler_off // sum_boiler_on // boileroff // boileron
        std::string home_data = "device_id=" + gateway + "&module_id=" + module_id + "&scale=" + scale + "&type=" + "boileron" + "&date_begin=" + date_begin + "&date_end=" + date_end + "&limit=" + limit  + "&optimize=" + "false" + "&real_time="  + "false";
        bool bRet;           //Parsing status
        //
        Get_Respons_API(NETYPE_MEASURE, sResult, home_data, bRet, root);
        //
        if (!root["body"].empty())
        {
                //https://api.netatmo.com/api/getmeasure?device_id=< >&module_id=< >&scale=30min&type=sum_boiler_off&type=sum_boiler_on&type=boileroff&type=boileron&optimize=false&real_time=false
                //bRet = ParseData(sResult);
                //if (bRet)
                //{
                        // Data was parsed with success
                Log(LOG_STATUS, "Measure Data parsed");
                //}
         }
}


/// <summary>
/// Get events
/// </summary>
void CNetatmo::Get_Events(std::string home_id, std::string device_types, std::string event_id, std::string person_id, std::string device_id, std::string module_id, bool offset, bool size, std::string locale)
{
        //Check if connected to the API
        if (!m_isLogged)
                return ;

        //Locals
        std::string sResult; // text returned by API
        Json::Value root;    // root JSON object
        std::string offset_str = bool_as_text(offset);
        std::string size_str = bool_as_text(size);
	std::string home_events_data;
        // https://api.netatmo.com/api/getevents?home_id=xxx&device_types=xxx&event_id=xxx&person_id=xxx&device_id=xxx&module_id=xxx&offset=15&size=15&locale=nl
        if (!device_id.empty())
                home_events_data = home_id + "&device_types=" + device_types + "&event_id=" + event_id + "&person_id=" + person_id + "&module_id=" + module_id + "&offset=" + offset_str + "&size=" + size_str;
        else
                home_events_data = home_id ;

	bool bRet;           //Parsing status
        //
        Get_Respons_API(NETYPE_EVENTS, sResult, home_events_data, bRet, root);
        //
        if (!root["body"]["home"].empty())
        {
		bRet = ParseEvents(sResult, root);
                if (bRet)
                {
                        // Data was parsed with success
                        Log(LOG_STATUS, "Events Data parsed");
                }
         }
}


/// <summary>
/// Parse data for weather station and Homecoach
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
        //
	if (root["body"].empty())
        {
                Debug(DEBUG_HARDWARE, "Body empty");
        	bool bHaveDevices = false;
        }
        else if (root["body"]["devices"].empty())
        {
                Debug(DEBUG_HARDWARE, "Devices empty");
		bool bHaveDevices = false;
        }
	else if (!root["body"]["devices"].isArray())
        {
                Debug(DEBUG_HARDWARE, "Devices no Array");
	        bool bHaveDevices = false;
        }
        // Homecoach        body - devices
        // WeatherStation   body - devices
        // HomesData     is body - homes
        // HomeStatus    is body - home
        //
	//Return error if no devices are found
	if (!bHaveDevices)
	{
                Log(LOG_STATUS, "No devices found...");
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
			std::string name;
                        int mrf_status = 0;
                        int RF_status = 0;
                        int mbattery_percent = 255;
                        if (!device["module_name"].empty())
                                name = device["module_name"].asString();
                        else if (!device["station_name"].empty())
                                name = device["station_name"].asString();
                        else if (!device["name"].empty())
                                name = device["name"].asString();
                        else if (!device["modules"].empty())
                                name = device["modules"]["module_name"].asString();
                        else
                                name = "UNKNOWN";

                        //get Home ID from Weatherstation
                        if (type == "NAMain")
                                m_Home_ID = "home_id=" + device["home_id"].asString();
                                Debug(DEBUG_HARDWARE, "m_Home_ID = %s", m_Home_ID.c_str());  //m_Home_ID ........................ numbers and letters

                        // Station_name NOT USED upstream ?
			std::string station_name;
			if (!device["station_name"].empty())
				station_name = device["station_name"].asString();
			else if (!device["name"].empty())
				station_name = device["name"].asString();

			//stdreplace(name, "'", "");
			//stdreplace(station_name, "'", "");
			_tNetatmoDevice nDevice;
			nDevice.ID = id;
			nDevice.ModuleName = name;
			nDevice.StationName = station_name;
                        int crcId = Crc32(0, (const unsigned char*)id.c_str(), id.length());
                        m_ModuleIDs[id] = crcId;
                        //Debug(DEBUG_HARDWARE, "ID %s - Name %s - crcId %d", id.c_str(), name.c_str(), crcId);   //
                        // Find the corresponding _tNetatmoDevice
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

                        if (!device["battery_percent"].empty())
                        {
                                mbattery_percent = device["battery_percent"].asInt(); //Batterij
                                Debug(DEBUG_HARDWARE, "batterij - bat %d", mbattery_percent);
                        }
                        if (!device["wifi_status"].empty())
                        {
                                // 86=bad, 56=good
                                RF_status = (86 - device["wifi_status"].asInt()) / 3;
                                if (RF_status > 10)
                                        RF_status = 10;
                        }
                        if (!device["rf_status"].empty())
                        {
                                RF_status = (86 - device["rf_status"].asInt()) / 3;
                                if (RF_status > 10)
                                        RF_status = 10;
                        }
                        if (!device["dashboard_data"].empty())
                        {
                                //SaveString2Disk(device["dashboard_data"], std::string("./") + name.c_str() + ".txt");
                                ParseDashboard(device["dashboard_data"], iDevIndex, crcId, name, type, mbattery_percent, RF_status, id);
                                //
                        }
			//Weather modules (Temp sensor, Wind Sensor, Rain Sensor)
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
                                                        int crcId = Crc32(0, (const unsigned char*)mid.c_str(), mid.length());
                                                        m_ModuleIDs[mid] = crcId;
							if (mname.empty())
								mname = "unknown" + mid;
							if (!module["battery_percent"].empty())
                                                                // battery percent %
								mbattery_percent = module["battery_percent"].asInt();
							if (!module["rf_status"].empty())
							{
								// 90=low, 60=highest
								mrf_status = (90 - module["rf_status"].asInt()) / 3; // ?
								if (mrf_status > 10)
									mrf_status = 10;
							}
							//
							if (!module["dashboard_data"].empty())
			                                {
                                                                //SaveString2Disk(module["dashboard_data"], std::string("./") + mname.c_str() + ".txt");
                        					ParseDashboard(module["dashboard_data"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status, mid);
							}
						}
						else
							nDevice.ModulesIDs.push_back(module.asString());
					}
				}
			}

			_netatmo_devices.push_back(nDevice);

		}
		iDevIndex++;
	}
        //return (!_netatmo_devices.empty());
        return true;
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
bool CNetatmo::ParseDashboard(const Json::Value& root, const int DevIdx, const int ID, std::string& name, const std::string& ModuleType, const int battery_percent, const int rssiLevel, std::string& Hardware_ID)
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

	int temp = 0;
        float Temp = 0;
        int sp_temp = 0;
	float SP_temp = 0;
	int hum = 0;
	int baro = 0;
	int co2 = 0;
	int rain = 0;
        int rain_1 = 0;
        int rain_24 = 0;
	int sound = 0;
//        Debug(DEBUG_HARDWARE, "bHave %d : %d : %d : %d : %d : %d : %d : %d", bHaveTemp, bHaveHum, bHaveBaro, bHaveCO2, bHaveRain, bHaveSound, bHaveWind, bHaveSetpoint);
	int wind_angle = 0;
	float wind_strength = 0;
	float wind_gust = 0;
        int batValue = battery_percent ; // / 100;
        bool action = 0;
        //
        int nValue = 0;
	std::stringstream t_str;
        std::stringstream sp_str;
        //std::string sValue = "";      //const char*
        //
        //Debug(DEBUG_HARDWARE, "value %d : percent %d", batValue, battery_percent);
//	batValue = GetBatteryLevel(ModuleType, battery_percent);

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
		Temp = root["Temperature"].asFloat();
		t_str << std::setprecision(2) << Temp;
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module Temperature [%s]", t_str.str().c_str());
	}
	else if (!root["temperature"].empty())
	{
		bHaveTemp = true;
		Temp = root["temperature"].asFloat();
		t_str << std::setprecision(2) << Temp;
                temp = static_cast<int>(Temp);
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module int temperature [%s]", t_str.str().c_str());
	}
	if (!root["Sp_Temperature"].empty())
	{
		bHaveSetpoint = true;
                SP_temp = root["Sp_Temperature"].asFloat();
		sp_str << std::setprecision(2) << SP_temp;
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module Sp [%s]", sp_str.str().c_str());
	}
	else if (!root["setpoint_temp"].empty())
	{
		bHaveSetpoint = true;
		SP_temp = root["setpoint_temp"].asFloat();
		sp_str << std::setprecision(2) << SP_temp;
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module setpoint [%s]", sp_str.str().c_str());
	}
	if (!root["Humidity"].empty())
	{
		bHaveHum = true;
		hum = root["Humidity"].asInt();
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module hum [%d]", hum);
	}
	if (!root["Pressure"].empty())
	{
		bHaveBaro = true;
		baro = root["Pressure"].asInt();
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module Pressure [%d]", baro);
	}
	if (!root["Noise"].empty())
	{
		bHaveSound = true;
		sound = root["Noise"].asInt();
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module Noise [%d]", sound);
	}
	if (!root["CO2"].empty())
	{
		bHaveCO2 = true;
		co2 = root["CO2"].asInt();
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module CO2 [%d]", co2);
	}
	if (!root["sum_rain_24"].empty())
	{
		bHaveRain = true;
                rain = root["Rain"].asInt();
		rain_24 = root["sum_rain_24"].asInt();
                rain_1 = root["sum_rain_1"].asInt();
                Debug(DEBUG_HARDWARE, "ParseDashBoard Module Rain [%d]", rain);
	}
	if (!root["WindAngle"].empty())
	{
                Debug(DEBUG_HARDWARE, "wind %d : strength %d", root["WindAngle"].asInt(), root["WindStrength"].asInt());
		if ((!root["WindAngle"].empty()) && (!root["WindStrength"].empty()) && (!root["GustAngle"].empty()) && (!root["GustStrength"].empty()))
		{
			bHaveWind = true;
			wind_angle = root["WindAngle"].asInt();
			wind_strength = root["WindStrength"].asFloat() / 3.6F;
			wind_gust = root["GustStrength"].asFloat() / 3.6F;
		}
	}

        Debug(DEBUG_HARDWARE, "bHave Temp %d : Hum %d : Baro %d : CO2 %d : Rain %d : Sound %d :  wind %d : setpoint %d", bHaveTemp, bHaveHum, bHaveBaro, bHaveCO2, bHaveRain, bHaveSound, bHaveWind, bHaveSetpoint);
        //converting ID to char const
        std::string str = std::to_string(ID);
        char const* pchar_ID = str.c_str();
        // Hardware_ID hex to int
        int Hardware_int = convert_mac(Hardware_ID);
        //Debug(DEBUG_HARDWARE, "(%d) %s [%s] %d", Hardware_int, Hardware_ID.c_str(), name.c_str(), ID);
        //
	//Data retreived create / update appropriate domoticz devices
	//Temperature and humidity sensors
	if (bHaveTemp && bHaveHum && bHaveBaro)
	{
		int nforecast = m_forecast_calculators[ID].CalculateBaroForecast(temp, baro);
                //Debug(DEBUG_HARDWARE, "%s name Temp & Hum & Baro [%s] %d %d %d - %d / %d ", Hardware_ID.c_str(), name.c_str(), temp, hum, baro, batValue, rssiLevel);
        //	SendTempHumBaroSensorFloat(ID, batValue, temp, hum, baro, (uint8_t)nforecast, name, rssiLevel);
                // Humidity status: 0 - Normal, 1 - Comfort, 2 - Dry, 3 - Wet
                const char status = '0';
                std::stringstream r;
                r << Temp;
                r << ";";
                r << hum;
                r << ";";
                r << status;
                r << ";";
                r << baro;
                r << ";";
                r << nforecast;
                std::string sValue = r.str().c_str();
                //
                // sValue is string with values separated by semicolon: Temperature;Humidity;Humidity Status;Barometer;Forecast
                //
                //std::string baro_;
                //std::stringstream convert;
                //convert << baro;
                //baro_ = convert.str().c_str();
                //
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] Temp & Humidity & Baro %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                //pTypeTEMP_HUM_BARO, sTypeTHBFloat
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeTEMP_HUM_BARO, sTypeTHBFloat, rssiLevel, batValue, '0', sValue.c_str(), name,  '0', m_Name);
                //
	}
	else if (bHaveTemp && bHaveHum)
        {
                //Debug(DEBUG_HARDWARE, "name Temp & Hum [%s] %d %d - %d / %d ", name.c_str(), temp, hum, batValue, rssiLevel);
        //	SendTempHumSensor(ID, batValue, temp, hum, name, rssiLevel);
                //
                const char status = '0';
                std::stringstream s;
                s << Temp;
                s << ";";
                s << hum;
                s << ";";
                s << status;
                std::string sValue = s.str().c_str();
                //
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] Temp & Humidity %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                // sValue is string with values separated by semicolon: Temperature;Humidity
                //pTypeGeneral, sTypeTH_LC_TC
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeTEMP_HUM, sTypeTH_LC_TC, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //
        }
	else if (bHaveTemp)
        {
                //Debug(DEBUG_HARDWARE, "name Temp [%s] %d - %d / %d ", name.c_str(), temp, batValue, rssiLevel);
	//	SendTempSensor(ID, batValue, temp, name, rssiLevel);
                //
                std::stringstream t;
                t << Temp;
//                t << " ;";
//                std::string sValue = t.str().c_str();
                std::string sValue = "";
		Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] temp %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                //pTypeGeneral, sTypeTemperature
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeGeneral, sTypeTemperature, rssiLevel, batValue, temp, sValue.c_str(), name, 0, m_Name);
		//pTypeGeneral, sTypeThermTemperature
//                UpdateValueInt(Hardware_int, pchar_ID, 0, pTypeGeneral, sTypeThermTemperature, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //
        }
	//Thermostat device
	if (bHaveSetpoint)
	{
		std::string sName = name + " - SetPoint ";
                //Debug(DEBUG_HARDWARE, "sName [%s] ", sName.c_str());
        //	SendSetPointSensor((uint8_t)((ID & 0x00FF0000) >> 16), (ID & 0XFF00) >> 8, ID & 0XFF, sp_temp, sName);
                //
                sp_temp = static_cast<int>(SP_temp);
                std::string sValue = "";
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] setpoint %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                //pTypeSetpoint, sTypeSetpoint
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeSetpoint, sTypeSetpoint, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //UpdateValueInt(Hardware_int, pchar_ID, 0, pTypeGeneral, sTypeSetpoint, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //UpdateValueInt(Hardware_int, pchar_ID, 0, 242, 1, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
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
                // integer to const char* sValue
                //sValue = (const char*)rain;
                std::stringstream v;
                v << rain_1;
                v << ";";
                v << rain;
                std::string sValue = v.str().c_str();
                //Debug(DEBUG_HARDWARE, "name Rain [%s]  %d / %d ", name.c_str(), batValue, rssiLevel);
	//	SendRainSensor(ID, batValue, m_RainOffset[ID] + m_OldRainCounter[ID], name, rssiLevel);
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] rain %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
//                UpdateValueInt(int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue, const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
                //pTypeGeneral, sTypeRAINByRate
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeRAIN, sTypeRAINByRate, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //
	}

	if (bHaveCO2)
        {
                //Debug(DEBUG_HARDWARE, "name CO2 [%s]  %d /  ", name.c_str(), batValue);
	//	SendAirQualitySensor(ID, DevIdx, batValue, co2, name);
                //
                std::stringstream w;
                w << co2;
//                w << " ;";
                //std::string sValue = w.str().c_str();
                std::string sValue = "";
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] co2 %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                //pTypeAirQuality, sTypeVoc
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeAirQuality, sTypeVoc, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //
        }
	if (bHaveSound)
        {
                //Debug(DEBUG_HARDWARE, "name Temp [%s]  %d / ", name.c_str(), batValue);
	//	SendSoundSensor(ID, batValue, sound, name);
                //sValue = sound.str().c_str();
                std::stringstream x;
                x << sound;
                std::string sValue = x.str().c_str();
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] sound %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                //pTypeGeneral, sTypeSoundLevel
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeGeneral, sTypeSoundLevel, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //
        }
	if (bHaveWind)
        {
                //Debug(DEBUG_HARDWARE, "name Wind [%s]  %d / %d ", name.c_str(), batValue, rssiLevel);
	//	SendWind(ID, batValue, wind_angle, wind_strength, wind_gust, 0, 0, false, false, name, rssiLevel);
                //
                // sValue: "<WindDirDegrees>;<WindDirText>;<WindAveMeterPerSecond*10>;<WindGustMeterPerSecond*10>;<Temp_c>;<WindChill_c>"
                const char status = '0';
                std::stringstream y;
                y << wind_angle;
                y << ";";
                y << '0';
                y << ";";
                y << wind_strength;
                y << ";";
                y << wind_gust;
                y << ";";
                y << '0';
                y << ";";
                y << '0';
                std::string sValue = y.str().c_str();
                Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] wind %s %s %d %d", Hardware_int, Hardware_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
                //pTypeGeneral, sTypeWINDNoTemp
                UpdateValueInt(Hardware_int, Hardware_ID.c_str(), 0, pTypeWIND, sTypeWINDNoTemp, rssiLevel, batValue, '0', sValue.c_str(), name, 0, m_Name);
                //
        }
        //
	return true;
}


/// <summary>                                                          // Deprecated
/// Parse data for energy station (thermostat and valves) and get      // For Reference Only
/// module / room and schedule
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <returns>success parsing the data</returns>
//bool CNetatmo::ParseHomeData(const std::string& sResult, Json::Value& root )
// for (auto home : root["body"]["homes"])
// dict_keys(['id', 'name', 'altitude', 'coordinates', 'country', 'timezone', 'rooms', 'modules', 'temperature_control_mode', 'therm_mode', 'therm_setpoint_default_duration', 'persons', 'schedules'])



/// <summary>
/// Parse data for energy/security devices
/// get and create/update domoticz devices
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <returns></returns>
bool CNetatmo::ParseHomeStatus(const std::string& sResult, Json::Value& root )
{
	//
	if (root["body"].empty())
		return false;

	if (root["body"]["home"].empty())
		return false;

	//int thermostatID;
	int nValue = 0;
        int wifi_status = 0;
        float mrf_percentage = 0;
        float rf_strength = 0;
        int batteryLevel = 255;
        int crcId = 0; //Crc32(0, (const unsigned char*)roomNetatmoID.c_str(), roomNetatmoID.length());
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
				std::string module_id = module["id"].asString();
//                                Debug(DEBUG_HARDWARE, "ParseHome Index %d", iModuleIndex);
                                int moduleIndex = iModuleIndex;

                                iModuleIndex ++;

                                //converting ID to char const
                                char const* pchar_ID = module_id.c_str();
                                std::string moduleName = m_ModuleNames[module_id];
                                // Hardware_ID hex to int
                                int Hardware_int = convert_mac(module_id);
                                Debug(DEBUG_HARDWARE, "(%d) %s [%s]", Hardware_int, module_id.c_str(), moduleName.c_str());
                                std::string type = module["type"].asString();
				// Find the module (name/id)
                                crcId = Crc32(0, (const unsigned char*)module_id.c_str(), module_id.length());
                                m_ModuleIDs[module_id] = crcId;
                                //
				//Device to get battery level / network strength
				if (!module["battery_level"].empty())
				{
					batteryLevel = module["battery_level"].asInt() / 100;
                                        //Debug(DEBUG_HARDWARE, "ParseBat %d", batteryLevel); //Batterij
					//batteryLevel = GetBatteryLevel(module["type"].asString(), batteryLevel);
				};
				if (!module["rf_strength"].empty())
				{
					rf_strength = module["rf_strength"].asFloat();
					// 90=low, 60=highest
					if (rf_strength > 90.0F)
						rf_strength = 90.0F;
					if (rf_strength < 60.0F)
						rf_strength = 60.0F;

					//range is 30
					mrf_percentage = (100.0F / 30.0F) * float((90 - rf_strength));
					if (mrf_percentage != 0)
					{
						std::string pName = " " + moduleName + " - Sig. + Bat. Lvl";
//                                                Debug(DEBUG_HARDWARE, "Parse Name %s - %d", pName.c_str(), moduleID );
                                                //SendPercentageSensor(crcId, 1, batteryLevel, mrf_percentage, pName );
                                                //
					}
				};
                                if (!module["rf_state"].empty())
                                {
                                        //
                                        std::string rf_state = module["rf_state"].asString();
                                        //
                                };

                                if (!module["wifi_state"].empty())
                                {
                                        //
                                        std::string wifi_state = module["wifi_state"].asString();
                                        //
                                };
                                if (!module["wifi_strength"].empty())
                                {
                                        // 86=bad, 56=good
                                        wifi_status = (86 - module["wifi_strength"].asInt()) / 3;
                                        if (wifi_status > 10)
                                                wifi_status = 10;
                                        //
                                };
                                if (!module["ts"].empty())
                                {
                                        //timestamp
                                        int timestamp = module["ts"].asInt();
                                        //
                                };
                                if (!module["last_seen"].empty())
                                {
                                        //
                                        int last_seen = module["last_seen"].asInt();
                                        //
                                };
                                if (!module["last_activity"].empty())
                                {
                                        //
                                        int last_activity = module["last_activity"].asInt();
                                        //
                                };
                                if (!module["sd_status"].empty())
                                {
                                        //status off SD-card
                                        int sd_status = module["sd_status"].asInt();
                                        //
                                };
                                if (!module["reachable"].empty())
                                {
                                        // True / False
                                };
                                if (!module["alim_status"].empty())
                                {
                                        // Checks the adaptor state
                                };
                                if (!module["vpn_url"].empty())
                                {
                                        // VPN url from camera

                                        //
                                };
                                if (!module["is_local"].empty())
                                {
                                        // Camera is locally connected - True / False

                                        //
                                };
                                if (!module["monitoring"].empty())
                                {
                                        // Camera On / Off

                                        //
                                };
                                if (!module["bridge"].empty())
                                {
                                        std::string bridge_ = module["bridge"].asString();
                                        std::string Bridge_Name = m_ModuleNames[bridge_];
                                        std::string Module_Name = moduleName + " - Bridge";
                                        std::string sValue = bridge_ + "  " + Bridge_Name;
                                        //SendTextSensor(crcId, 1, 255, Bridge_Text, Module_Name);
                                        UpdateValueInt(Hardware_int, module_id.c_str(), 1, pTypeGeneral, sTypeTextStatus, '0', 255, '0', sValue.c_str(), Module_Name, 0, m_Name);
                                };
                                if (!module["boiler_valve_comfort_boost"].empty())
                                {
                                        std::string boiler_boost = module["boiler_valve_comfort_boost"].asString();
                                        std::string bName = moduleName + " - Boost";
                                        //Debug(DEBUG_HARDWARE, "Boiler Boost %s - %s", bName.c_str(), boiler_boost.c_str() );
                                        bool bIsActive = module["boiler_valve_comfort_boost"].asBool();
                                        std::stringstream b;
                                        b << boiler_boost;
                                        b << ";";
                                        b << boiler_boost;
                                        std::string sValue = b.str().c_str();
                                        //SendSwitch(crcId, 0, 255, bIsActive, 0, aName, m_Name);
                                        UpdateValueInt(Hardware_int, module_id.c_str(), 0, pTypeGeneralSwitch, sSwitchGeneralSwitch, '0', 255, '0', sValue.c_str(), bName,  bIsActive, m_Name);
                                };
                                if (!module["boiler_status"].empty())
                                {
                                        //Thermostat status (boiler heating or not : informationnal switch)
                                        std::string aName = moduleName + " - Heating Status";
                                        bool bIsActive = module["boiler_status"].asBool();
					std::string boiler_status = module["boiler_status"].asString();
                                        Debug(DEBUG_HARDWARE, "Boiler Heating Status %s - %d %d %s - m_Name %s", aName.c_str(), crcId, bIsActive, module["boiler_status"].asString().c_str(), m_Name.c_str());
                                        //SendSwitch(crcId, 1, 255, bIsActive, 0, aName, m_Name);
                                        m_Room_status[module_id] = bIsActive;

                                        //SendSwitch(crcId, 1, 255, bIsActive, 0, aName, m_Name);
                                        std::stringstream h;
                                        h << boiler_status;
                                        h << ";";
                                        h << boiler_status;
                                        std::string sValue = h.str().c_str();
                                        UpdateValueInt(Hardware_int, module_id.c_str(), 0, pTypeGeneralSwitch, sSwitchGeneralSwitch, '0', 255, '0', sValue.c_str(), aName,  bIsActive, m_Name);

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
                                        std::string sName = moduleName + " - Schedule";
                                        //SendSelectorSwitch(crcId, 2, ssv.str(), sName, 15, true, allSchName, allSchAction, true, m_Name);
                                        //
                                };
                                if (!module["status"].empty())
                                {
                                        // Door sensor
                                        std::string a_Name = moduleName + " - Status"; //m_[id];
                                        //
                                        // mrf_percentage & batteryLevel
                                        // rf_strength
					std::string sValue = module["status"].asString();
					//SendTextSensor(crcId, 2, batteryLevel, module["status"].asString(), a_Name);
                                        UpdateValueInt(Hardware_int, module_id.c_str(), 2, pTypeGeneral, sTypeTextStatus, '0', 255, '0', sValue.c_str(), a_Name, 0, m_Name);
					bool bIsActive = (module["status"].asString() == "open");
                                        //SendSwitch(crcId, 0, batteryLevel, bIsActive, 0, aName, m_Name, mrf_percentage);
					UpdateValueInt(Hardware_int, module_id.c_str(), 3, pTypeGeneralSwitch, sSwitchGeneralSwitch, '0', 255, '0', sValue.c_str(), a_Name, bIsActive, m_Name);
                                };
                                if (!module["floodlight"].empty())
                                {
                                        //Light Outdoor Camera
                                        //      AUTO / ON / OFF
                                        std::string lName = moduleName + " - Light";
                                        Debug(DEBUG_HARDWARE, "Floodlight name %s - %d  - m_Name %s", lName.c_str(), crcId,  m_Name.c_str());
                                        bool bIsActive = (module["floodlight"].asString() == "true");
                                        std::string sValue = (module["floodlight"].asString());            // "Off|On|Auto"
                                        std::string setpoint_mode = (module["floodlight"].asString());
                                        //SendSelectorSwitch(crcId, 0, setpoint_mode, lName, 15, true, "Off|On|Auto", "", true, m_Name);
                                        //SendSwitch(crcId, 0, 255, bIsActive, 0, lName, m_Name);
                                        
                                        UpdateValueInt(Hardware_int, module_id.c_str(), 0, pTypeGeneral, sSwitchTypeSelector, '0', 255, '0', sValue.c_str(), lName,  bIsActive, m_Name);
                                };
                                if (!module["t"].empty())
                                {
                                        //
                                        //std::string aName = m_[id];
                                        //;
                                        //
                                        //SendSwitch(crcId, 0, 255, bIsActive, 0, aName, m_Name);
                                };
			}
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
				float room_measured = 0;
				int rssiLevel = 12;
                                int batValue = 255;
                                std::stringstream m;
                                //std::string sValue = "";
                                //Debug(DEBUG_HARDWARE, "Parse RoomName %s", roomName.c_str());
				int roomID = iDevIndex + 1;
                                iDevIndex ++;
                                crcId = Crc32(0, (const unsigned char*)roomNetatmoID.c_str(), roomNetatmoID.length());
//                                m_ModuleIDs[roomNetatmoID] = crcId;

				//Find the room name
//				//roomName = m_RoomNames[roomNetatmoID].asString();
				roomName = m_RoomNames[roomNetatmoID];
                                roomID = m_RoomIDs[roomNetatmoID];
                                std::string roomType = m_Types[roomNetatmoID];
                                std::string roomCategory = m_Module_category[roomNetatmoID];

                                std::string Module_id = m_Room[roomNetatmoID];

				m_thermostatDeviceID[roomID & 0xFFFFFF] = roomNetatmoID;
				m_thermostatModuleID[roomID & 0xFFFFFF] = roomNetatmoID;

				//Create / update domoticz devices : Temp sensor / Set point sensor for each room
                                if (!room["reachable"].empty())
                                {

                                };
                                if (!room["anticipating"].empty())
                                {

                                };
                                if (!room["heating_power_request"].empty())
                                {

                                };
                                if (!room["open_window"].empty())
                                {

                                };
				if (!room["therm_measured_temperature"].empty())
				{
                                        //
					room_measured = room["therm_measured_temperature"].asFloat();
					//SendTempSensor(crcId, 255, room["therm_measured_temperature"].asInt(), roomName);
		                	std::stringstream room_str;
                                        room_str << std::setprecision(2) << room_measured;
					Debug(DEBUG_HARDWARE, "RoomName %s = %s °C", roomName.c_str(), room_str.str().c_str());
					std::stringstream rm;
                                        rm << std::setprecision(2) << room_measured;
					m << std::setprecision(2) << room_measured;
                			m << " ; ";
                                        //std::string sValue = rm.str().c_str();
					std::string sValue = rm.str().c_str();
					UpdateValueInt(roomID, Module_id.c_str(), 0, pTypeTEMP, sTypeTEMP5, 12, 255, '0', sValue.c_str(), roomName, 0, m_Name);
//                                        SendTempSensor((roomID & 0x00FFFFFF) | 0x03000000, 255, room["therm_measured_temperature"].asFloat(), roomName);
				};
				if (!room["therm_setpoint_temperature"].empty())
				{
                                        //
					std::string setpoint_Name = roomName + "_Setpoint";
					//SendSetPointSensor((uint8_t)((crcId & 0x00FF0000) >> 16), (roomID & 0XFF00) >> 8, roomID & 0XFF, room["therm_setpoint_temperature"].asInt(), roomName);
		                	float SP_temp = room["therm_setpoint_temperature"].asFloat();
					int sp_temp = static_cast<int>(SP_temp);
                                        std::stringstream setpoint_str;
                                        setpoint_str << std::setprecision(2) << SP_temp;
                                        std::stringstream sp_m;
                                        sp_m << std::setprecision(2) << SP_temp;
                                        m << std::setprecision(2) << SP_temp;
                                        m << " ; ";
					std::string sValue = sp_m.str().c_str();
					// pTypeGeneral, sTypeTextStatus
                                        UpdateValueInt(roomID, Module_id.c_str(), 1, pTypeGeneral, sTypeTextStatus, 12, 255, sp_temp, sValue.c_str(), setpoint_Name, 0, m_Name);
                                        // pTypeGeneral, sTypeSetPoint
					UpdateValueInt(roomID, Module_id.c_str(), 3, pTypeGeneral, sTypeSetPoint, 12, 255, '0', sValue.c_str(), setpoint_Name, 0, m_Name);
//                                        SendSetPointSensor((uint8_t)(((roomID & 0x00FF0000) | 0x02000000) >> 16), (roomID & 0XFF00) >> 8, roomID & 0XFF, room["therm_setpoint_temperature"].asFloat(), roomName));
				};
				if (!room["therm_setpoint_mode"].empty())
				{
					// create / update the switch for setting away mode
					// on the thermostat (we could create one for each room also,
					// but as this is not something we can do on the app, we don't here)
					std::string setpoint_mode = room["therm_setpoint_mode"].asString();
					std::string setpoint_mode_i;
					if (setpoint_mode == "away")
						setpoint_mode_i = "20";
					else if (setpoint_mode == "hg")
						setpoint_mode_i = "30";
					else
						setpoint_mode_i = "10";
                                        //SendSelectorSwitch(thermostatID, 3, setpoint_mode_i, m_ThermostatName[id] + " - Mode", 15, true, "Off|On|Away|Frost Guard", "", true, m_Name);
                                        Debug(DEBUG_HARDWARE, "Room - Mode %s = %s", roomName.c_str(), setpoint_mode.c_str());
					//SendSelectorSwitch(crcId, 3, setpoint_mode, roomName + " - Mode", 15, true, "Off|On|Away|Frost Guard", "", true, m_Name);
		                	m << setpoint_mode;
                			m << " ; ";
					// retrieve information status switch from thermostat
                                        std::string Module_id = m_Room[roomNetatmoID].c_str();
                                        std::string bIsActive = m_Room_status[Module_id].c_str();
					std::string moduleName = m_ModuleNames[Module_id];
                                        //Debug(DEBUG_HARDWARE, "Room %s - Status %s = %s / %s - %d", roomNetatmoID.c_str(), Module_id.c_str(), bIsActive.c_str(), m.str().c_str(), room_measured);
                                        m << bIsActive;
					//
                                        std::stringstream stherm;
                                        stherm << setpoint_mode;
                                        //
                                        std::string sValue = stherm.str().c_str();
                                        // thermostatID not defined
					setModeSwitch = true;
					//
                                        stherm << " ; ";
                                        stherm << bIsActive;
                                        sValue = stherm.str().c_str();
                                        //
                                        std::string mode_Name = roomName + "_Mode";
                                        // pTypeGeneral, sTypeTextStatus
                                        UpdateValueInt(roomID, Module_id.c_str(), 2, pTypeGeneral, sTypeTextStatus, rssiLevel, batValue, nValue, sValue.c_str(), mode_Name, 0, m_Name);
				}
                                if (!room["therm_setpoint_start_time"].empty())
                                {

                                };
				std::string sValue = m.str().c_str();
				//const char* sValue = m.str().c_str();
				// pTypeGeneral, sTypeTextStatus
                                UpdateValueInt(roomID, Module_id.c_str(), 4, pTypeGeneral, sTypeTextStatus, rssiLevel, batValue, room_measured, sValue.c_str(), roomName, 0, m_Name);
				// pTypeThermostat1, sTypeDigimax ERROR
				//UpdateValueInt(roomID, roomNetatmoID.c_str(), 0, pTypeThermostat1, sTypeDigimax, rssiLevel, batValue, room_measured, sValue, roomName, 0, m_Name);
				//
			}
		}
	}
        //Parse Persons
        int iPersonIndex = 0;
        if (!root["body"]["home"]["persons"].empty())
        {
                if (!root["body"]["home"]["persons"].isArray())
                        return false;
                Json::Value mRoot = root["body"]["home"]["persons"];

                for (auto person : mRoot)
                {
                        if (!person["id"].empty())
                        {
                                std::string PersonNetatmoID = person["id"].asString();
                                std::string PersonName;
                                int PersonID = iPersonIndex + 1;
                                iPersonIndex ++;
//                                Debug(DEBUG_HARDWARE, "ParseHome Person %d", PersonID);
                                //Find the Person name
                                PersonName = m_PersonsNames[PersonNetatmoID];

                                std::string PersonLastSeen = person["last_seen"].asString();
                                std::string PersonAway = person["out_of_sight"].asString();
                        }
                }
        }
//	return (iPersonIndex > 0);
	return true;
}


/// <summary>
/// Parse data for energy/security devices
/// get and create/update domoticz devices
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <returns></returns>
bool CNetatmo::ParseEvents(const std::string& sResult, Json::Value& root )
{
        //Parse Events
        int iEventsIndex = 0;
        if (!root["body"]["home"]["events"].empty())
        {
                if (!root["body"]["home"]["events"].isArray())
                        return false;
                Json::Value mRoot = root["body"]["home"]["events"];
                std::string event_Text = "check Parse Events";
                std::string events_ID;
                std::string events_Type;
                std::string events_Module_ID;
                std::string events_Message;
                std::string events_subevents_type;
                std::string e_Name = "";
                int Hardware_int = 0;
                char const* pchar_ID = 0;
                int RF_status = 0;
                int nValue = 0;
                //std::string sValue = "";

                for (auto events : mRoot)
                {
                        if (!events["id"].empty())
                        // Domoticz Device for Events ? / Camera's ?
			// Using Textstatus for now
                        if (!events["id"].empty())
                        {
                                events_ID = events["id"].asString();
                        };
                        if (!events["type"].empty())
                        {
                                events_Type = events["type"].asString();
                        };
                        if (!events["time"].empty())
                        {
                                bool events_Time = events["time"].asBool();
                        };
                        if (!events["module_id"].empty())
                        {
                                events_Module_ID = events["module_id"].asString();
                                e_Name = m_ModuleNames[events_Module_ID] + " - events";
                                //converting ID to char const
                                //std::string str_id = std::to_string(events_Module_ID);
                                //char const* pchar_ID = events_ID.c_str();
                                //
                                Hardware_int = convert_mac(events_Module_ID);
                                //pchar_ID = str_id.c_str();
                                pchar_ID = events_Module_ID.c_str();
                        };
                        if (!events["message"].empty())
                        {
                                events_Message = events["message"].asString();
                                std::stringstream y;
                                y << events_Message;
                                y << ";";
                                y << events_Type;
                                std::string sValue = y.str().c_str();
                                Debug(DEBUG_HARDWARE, "Message %s from %s %d", sValue.c_str(), e_Name.c_str(), Hardware_int);
                                //UpdateValueInt(Hardware_int, pchar_ID, 5, pTypeGeneral, sTypeTextStatus, RF_status, 255, nValue, sValue.c_str(), e_Name, '0', m_Name); Database ERROR
                        };
                        if (!events["video_id"].empty())
                        {
                                std::string events_Video_ID = events["video_id"].asString();
                        };
                        if (!events["video_status"].empty())
			{
                                std::string events_Video_Status = events["video_status"].asString();
                        };
                        if (!events["snapshot"].empty())
                        {
                                //events_Snapshot = events["snapshot"];
//                                std::string events_Snapshot_url = events["snapshot"]["url"];
//                                std::string events_Snapshot_exp = events["snapshot"]["expires_at"];
                        };
                        if (!events["vignette"].empty())
                        {
//                                //events_Vignette = events["vignette"];
//                                std::string events_Vignette_url = events["vignette"]["url"];
//                                std::string events_Vignette_exp = events["vignette"]["expires_at"];
                        };
                        if (!events["subevents"].empty())
                        {
                                for (auto sub_events : events["subevents"])
                                {
                                        if (!sub_events["id"].empty())
                                        {
                                                std::string events_subevents_ID = sub_events["id"].asString();
                                        };
                                        if (!sub_events["type"].empty())
                                        {
                                                events_subevents_type = sub_events["type"].asString();
                                        };
                                        if (!sub_events["time"].empty())
                                        {
                                                bool events_subevents_time = sub_events["time"].asBool();
                                        };
                                        if (!sub_events["verified"].empty())
                                        {
                                                std::string events_subevents_verified = sub_events["verified"].asString(); // true or false
                                        };
                                        if (!sub_events["offset"].empty())
                                        {
                                                int events_subevents_offset = sub_events["offset"].asInt();
                                        };
                                        if (!sub_events["snapshot"].empty())
                                        {
                                                //events_subevents_ID = sub_events["snapshot"];
//                                                std::string events_subevents_Snapshot = sub_events["snapshot"]["url"].asString();
//                                                std::string events_subevents_Snapshot = sub_events["snapshot"]["expires_at"].asString();
                                        };
                                        if (!sub_events["vignette"].empty())
                                        {
                                                //events_subevents_Vignette = sub_events["vignette"];
//                                                std::string events_subevents_Vignette = sub_events["vignette"]["url"].asString();
//                                                std::string events_subevents_Vignette = sub_events["vignette"]["expires_at"].asString();
                                        };
					if (!sub_events["message"].empty())
                                        {
                                                std::string events_subevents_Message = sub_events["message"].asString();
                                                std::stringstream z;
                                                z << events_subevents_type;
                                                z << ";";
                                                z << events_subevents_Message;
                                                events_Message = events_subevents_Message;
                                                std::string sValue = z.str().c_str();
                                                Debug(DEBUG_HARDWARE, "Sub Message %s from %s %d", sValue.c_str(), e_Name.c_str(), Hardware_int);
                                                //UpdateValueInt(Hardware_int, pchar_ID, 5, pTypeGeneral, sTypeTextStatus, RF_status, 255, nValue, sValue.c_str(), e_Name, '0', m_Name) Database ERROR;
                                        };
                                };
                        };
                        if (!events["sub_type"].empty())
			{
                                bool events_sub_type = events["sub_type"].asBool();
				events_Type = events["sub_type"].asString();
			};
			if (!events["persons"].empty())
                                for (auto person : events["persons"])
                                {
                                        if (!person["id"].empty())
                                                std::string events_person_id = person["id"].asString();
                                        if (!person["face_id"].empty())
                                                std::string events_person_face_id = person["face_id"].asString();
                                        if (!person["face_key"].empty())
                                                std::string events_person_face_key = person["face_key"].asString();
                                        if (!person["is_known"].empty())
                                                std::string events_person_is_known = person["is_known"].asString();
                                        if (!person["face_url"].empty())
                                                std::string events_person_face_url = person["face_url"].asString();
                                };
			if (!events["person_id"].empty())
                        {
                                std::string events_person_id = events["person_id"].asString();
                        };
                        if (!events["out_of_sight"].empty())
                        {
                                bool events_sub_type = events["out_of_sight"].asBool(); // true or false
                        };
                        if (!events_Message.empty())
                        {
                                event_Text = events_Message + " - " + events_Type;
                                int crcId;
                                crcId = Crc32(0, (const unsigned char*)events_Module_ID.c_str(), events_Module_ID.length());
				//SendTextSensor(crcId, 1, 255, event_Text, e_Name);
                                std::string sValue = event_Text.c_str();
				UpdateValueInt(Hardware_int, events_Module_ID.c_str(), 5, pTypeGeneral, sTypeTextStatus, RF_status, 255, nValue, sValue.c_str(), e_Name, '0', m_Name);
                        };
                        //
                };
        };
        //return (iEventsIndex > 0);
        return true;
}


/// <summary>
/// Normalize battery level for station module                    //Not Used anymore
/// </summary>
/// <param name="ModuleType">Module type</param>
/// <param name="battery_percent">battery percent</param>
/// <returns>normalized battery level</returns>
int CNetatmo::GetBatteryLevel(const std::string& ModuleType, int battery_percent)
{
	int batValue = 255;
        Log(LOG_STATUS, "batterij Level Updated");
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
