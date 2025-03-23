#include "stdafx.h"
#include "Netatmo.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../notifications/NotificationHelper.h"
#include <cinttypes>                    //PRIu64

#define NETATMO_OAUTH2_TOKEN_URI "https://api.netatmo.com/oauth2/token?"
#define NETATMO_API_URI "https://api.netatmo.com/"
#define NETATMO_PRESET_UNIT 10
// 03/03/2022 - PP Changing the Weather polling from 600 to 900s. This has reduce the number of server errors,
// 08/05/2024 - Give the poll interfval a defined name:
#define NETAMO_POLL_INTERVALL 900
#define NETAMO_LOGIN_INTERVALL 30
#define NETAMO_ERROR_INTERVALL 1800

#ifdef _DEBUG
//#define DEBUG_NetatmoWeatherStationR
#endif

//Logic ChildID
// 0	Sensor data
// 1	MAC-adres
// 2	Thermostat Schedule Switch
// 3	Battery-Level
// 4	Bridge MAC-adres
// 5	Kwh sensor
// 6	Doorsensor Alert
// 7	Setpoint
// 8	Temperature Thermostat
// 9	Boiler Status Switch
//10	Selector Switch
//11	Events Text
//12	Events Alert
//13    RF-Level
//14    Scenario Switch
//15    Blinds

// Some testfunctions for debugging
void SaveJson2Disk(Json::Value str, std::string filename)
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

#ifdef DEBUG_NetatmoWeatherStationR
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult;
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

CNetatmo::CNetatmo(const int ID, const std::string& username, const std::string& password)
	: m_username(CURLEncode::URLDecode(username))
	, m_password(CURLEncode::URLDecode(password))
{
	m_HwdID = ID;

	size_t pos = m_username.find(":");

	if (pos != std::string::npos)
	{
		m_clientId = m_username.substr(0, pos);
		m_clientSecret = m_username.substr(pos + 1);
		m_scopes = m_password;
	}
	else
	{
		Log(LOG_ERROR, "The username does not contain the client_id:client_secret!");
	}

	m_nextRefreshTs = mytime(nullptr);
	m_isLogged = false;
	m_ErrorFlag = false;

	Debug(DEBUG_HARDWARE, "Netatmo Actif Scopes %s ", m_scopes.c_str());

	m_bPollWeatherData = (m_scopes.find("station_R") != std::string::npos);      //read_station
	m_bPollHomecoachData = (m_scopes.find("homecoach_R") != std::string::npos);  //read_homecoach

	m_bPollHomeStatus = true;
	m_bPollHome = true;

	m_bPollThermostat = true;
	m_bFirstTimeHomeStatus = true;
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
	m_Room.clear();
	m_Room_mode.clear();
	m_Room_setpoint.clear();
	m_Room_Temp.clear();
	m_ModuleNames.clear();
	m_ModuleIDs.clear();
	m_Module_Bat_Level.clear();
	m_Module_RF_Level.clear();
	m_Types.clear();
	m_Device_types.clear();
	m_Module_category.clear();
	m_DeviceBridge.clear();
	m_thermostatModuleID.clear();
	m_ScheduleNames.clear();
	m_ScheduleIDs.clear();
	m_Scenarios.clear();
	m_selectedScenario.clear();
	m_ScheduleHome.clear();
	m_DeviceModuleID.clear();
	m_LightDeviceID.clear();
	m_PowerDeviceID.clear();
	m_homeid.clear();
	m_wifi_status.clear();
	m_DeviceHomeID.clear();
	m_PersonsNames.clear();

	m_ScheduleHomes.clear();
	m_selected_Schedule.clear();
        m_bPollThermostat = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_bForceSetpointUpdate = false;

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


std::string CNetatmo::ExtractHtmlStatusCode(const std::vector<std::string>& headers, const std::string& separator = ", ")
{
	std::string sReturn;

	std::string sHeaders;
	if (headers.size() > 0)
		for (const auto header : headers)
			if (header.find("HTTP") == 0)
			{
				if (!sHeaders.empty())
					sHeaders.append(separator);
				sHeaders.append( header);
			}
	if (sHeaders.empty())
		sHeaders =  "Not defined";

	return sHeaders;
}


void CNetatmo::Do_Work()
{
	int sec_counter = 600 - 5;
	bool bFirstTimeWS = true;
	bool bFirstTimeHS = true;
	bool bFirstTimeSS = true;
	bool bFirstTimeTH = true;
        std::string home_id;
	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % NETAMO_ERROR_INTERVALL == 0)
		{
			m_ErrorFlag = false;
		}
		if (!m_isLogged)
		{
			if (!m_ErrorFlag)
			{
				if (sec_counter % NETAMO_LOGIN_INTERVALL == 0)
				{
					Login();
					Log(LOG_STATUS,"Login %d",  m_isLogged);
				}
			}
		}
		if (m_isLogged)
		{
			if (!m_ErrorFlag)
			{
				if (RefreshToken())
				{
                                	// Thermostat is accessable through Homestatus / Homesdata in New API
                                	//Weather, HomeCoach, and Thermostat data is updated every  NETAMO_POLL_INTERVALL  seconds
					if ((sec_counter % NETAMO_POLL_INTERVALL == 0) || (bFirstTimeWS) || (bFirstTimeHS) || (bFirstTimeSS))
					{
						bFirstTimeWS = false;
                                        	bFirstTimeHS = false;
                                        	bFirstTimeSS = false;
						if (m_bPollWeatherData)
						{
							// ParseStationData
							GetWeatherDetails();
							Log(LOG_STATUS,"Weather %d",  m_isLogged);
						}
						if (m_bPollHomecoachData)
						{
							// ParseStationData
							GetHomecoachDetails();
							Log(LOG_STATUS,"HomeCoach %d",  m_isLogged);
						}
						if (m_bPollHomeStatus)
						{
							// GetHomesDataDetails
							GetHomeStatusDetails();
							Log(LOG_STATUS,"Status %d",  m_isLogged);
							m_bFirstTimeHomeStatus = false;
						}
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
								GetHomeStatusDetails();
						}
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
		Log(LOG_STATUS, "Use refresh token from database...");
		//Yes : we refresh our take
		if (RefreshToken(true))
		{
			m_isLogged = true;
			m_bPollThermostat = true;
			return true;
		}
	}

	if (m_refreshToken.empty())
	{
		Log (LOG_ERROR, "No refresh token available; please login to retreive a new one from Netatmo");
		StoreRequestTokenFlag(true);
		return false;
	}
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

	Log (LOG_STATUS, "Requesting refreshed tokens");
	m_ErrorFlag = false;

	// Time to refresh the token
	std::stringstream sstr;
	sstr << "grant_type=refresh_token&";
	sstr << "refresh_token=" << m_refreshToken << "&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret;

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::vector<std::string> returnHeaders;

//	ExtraHeaders.push_back("Host: api.netatmo.com");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

//        std::string httpUrl(NETATMO_API_URI + "oauth2/token?")
	std::string httpUrl(NETATMO_OAUTH2_TOKEN_URI);
	//Debug(DEBUG_HARDWARE, "Netatmo URL %s with Data %s", httpUrl.c_str(), httpData.c_str());

	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult, returnHeaders);

	//Check for returned data
	if (!ret)
	{
		Log(LOG_ERROR, "Error connecting to Server (refresh tokens): %s", ExtractHtmlStatusCode(returnHeaders).c_str());
		return false;
	}

	//Check for valid JSON
	Json::Value root;
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Debug(DEBUG_HARDWARE, "Netatmo Invalid ... %s", sResult.c_str());
		Log(LOG_ERROR, "Invalid/no data received (refresh tokens)... %s", ExtractHtmlStatusCode(returnHeaders).c_str());

		m_ErrorFlag = true;
		Log (LOG_STATUS, "Retry LOGIN within %d min. ", (NETAMO_ERROR_INTERVALL / 60));

		//Force login next time
		m_isLogged = false;

		//Access is Blocked so we clear AccessToken - Ready for renew
		m_accessToken = "";
		m_bForceLogin = false;
		m_bForceSetpointUpdate = false;

		m_tSetpointUpdateTime = time(nullptr);
		m_nextRefreshTs = mytime(nullptr);
		//Init();
		StoreRequestTokenFlag(true);
		return false;
	}

	//Check if token was refreshed and access granted
	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		if (!root["error"].empty())
			sResult = root["error"].asString();
		Log(LOG_ERROR, "No access granted, forcing login again (Refresh tokens): %s", sResult.c_str());
		//Force login next time
		StoreRefreshToken();
		m_isLogged = false;
		StoreRequestTokenFlag(true);
		return false;
	}

	//store the token
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	int expires = root["expires_in"].asInt();
	//Store the duration of validity of the token
	m_nextRefreshTs = mytime(nullptr) + expires * 2 / 3;

	StoreRequestTokenFlag(false);
	StoreRefreshToken();
	return true;
}


/// <summary>
/// Load the refresh token from the database
/// </summary>
/// <returns>true if token retrieved, store the token in member variables</returns>
bool CNetatmo::LoadRefreshToken()
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Extra FROM Hardware WHERE (ID==%d)", m_HwdID);
	if (result.empty())
	{
		Debug(DEBUG_HARDWARE, "Result Token Not found ... ");
		return false;
	}
	std::string refreshToken = result[0][0];
	if (refreshToken.empty())
	{
		Debug(DEBUG_HARDWARE, "No Refresh Token Found ... ");
		return false;
	}
	m_refreshToken = refreshToken;
	return true;
}


/// <summary>
/// Store an access token in the database for reuse after domoticz restart
/// (Note : we should also store token duration)
/// </summary>
void CNetatmo::StoreRequestTokenFlag(bool flag)
{
	Debug(DEBUG_HARDWARE, "Refresh Token Flag ... %d", flag?1:0);
	m_sql.safe_query("UPDATE Hardware SET Mode1='%d' WHERE (ID == %d)", flag?1:0, m_HwdID);
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
/// Function to identify the Wind Direction
///
/// </summary>
std::string CNetatmo::WindDirection(float dDirection)
{
	std::string strDirection;
	if (dDirection > 348.75 || dDirection < 11.26)
		strDirection = "N";
	else if (dDirection < 33.76)
		strDirection = "NNE";
	else if (dDirection < 56.26)
		strDirection = "NE";
	else if (dDirection < 78.76)
		strDirection = "ENE";
	else if (dDirection < 101.26)
		strDirection = "E";
	else if (dDirection < 123.76)
		strDirection = "ESE";
	else if (dDirection < 146.26)
		strDirection = "SE";
	else if (dDirection < 168.76)
		strDirection = "SSE";
	else if (dDirection < 191.26)
		strDirection = "S";
	else if (dDirection < 213.76)
		strDirection = "SSW";
	else if (dDirection < 236.26)
		strDirection = "SW";
	else if (dDirection < 258.76)
		strDirection = "WSW";
	else if (dDirection < 281.26)
		strDirection = "W";
	else if (dDirection < 303.76)
		strDirection = "WNW";
	else if (dDirection < 326.26)
		strDirection = "NW";
	else if (dDirection < 348.76)
		strDirection = "NNW";
	else
		strDirection = "---";
	return strDirection;
}


/// <summary>
/// Function to change bool to text
///
/// </summary>
std::string CNetatmo::bool_as_text(bool b)
{
                std::stringstream converter;
                converter << std::boolalpha << b;   // flag boolalpha calls converter.setf(std::ios_base::boolalpha)
                return converter.str();
}


/// <summary>
/// Function to change the MAC-adres to integer
///
/// </summary>
uint64_t CNetatmo::convert_mac(std::string mac)
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
uint64_t CNetatmo::UpdateValueInt(int HardwareID, const char* deviceID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
	const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
{
        uint64_t DeviceRowIdx = m_sql.UpdateValue(m_HwdID, HardwareID, deviceID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction, user.c_str());
        if (DeviceRowIdx == (uint64_t)-1)
                return -1;
        if (m_bOutputLog)
        {
                std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
                Log(LOG_NORM, szLogString);
        }
        m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
        m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, std::string(deviceID), devname, unit, devType, subType, nValue, sValue);
        m_mainworker.CheckSceneCode(DeviceRowIdx, devType, subType, nValue, sValue, "MQTT Auto");

        return DeviceRowIdx;
}


/// <summary>
/// Upon domoticz devices action (pressing a switch) take action
/// on the netatmo device through the API
/// </summary>
/// <param name="pdata">RAW data from domoticz device</param>
/// <param name=""></param>
/// <returns>success carrying the action (visible through domoticz)</returns>
bool CNetatmo::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	//Get a common structure to identify the actual action
	//the user has selected in domoticz (actionning a switch....)
	//Here a LIGHTING2 is used as we have selector switch for
	//our thermostat / valve devices
	// To Debug the Hardware on normal setup, uncomment lines "Log(LOG_STATUS" (on line 560 - 590 - 602 - 690)
	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	Debug(DEBUG_HARDWARE, "Netatmo Write to Hardware " );
	// Create/update Selector device for preset_modes
	int packetlength = pCmd->LIGHTING2.packetlength;
	unsigned char packettype2 = pCmd->LIGHTING2.packettype;

	if (packettype == pTypeLighting2)
	{
		Log(LOG_STATUS, "Packettype pTypeLighting2 ");
		return true;
	}
	//This is the selector switch for setting the thermostat schedule
	// unitcode == 0x02 ### means schedule switch
	if ((int)(pCmd->LIGHTING2.unitcode) == 2)
	{
		//Recast raw data to get switch specific data
		const _tGeneralSwitch* xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		Debug(DEBUG_HARDWARE, "Schedule id %d - %d", xcmd->id, xcmd->level);
		int uid = xcmd->id;       //switch ID
		//Log(LOG_STATUS, "Schedule uid %d - %d", uid, xcmd->level);
		int level = xcmd->level;  //Level selected on the switch
		int cmnd_SetLevel = xcmd->cmnd;
		int _rssi_ = xcmd->rssi;

		//Set the schedule on the thermostat
		SetSchedule(uid, level);
		return true;
	}
	if (packettype == pTypeGeneralSwitch)
	{
		Log(LOG_STATUS, "Packettype pTypeGeneralSwitch ");
		//
		int set_level = 0;
		// Recast raw data to get switch specific data
		const _tGeneralSwitch* xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int subtype = pCmd->LIGHTING2.subtype;
		int id1 = pCmd->LIGHTING2.id1;
		int id2 = pCmd->LIGHTING2.id2;
		int id3 = pCmd->LIGHTING2.id3;
		int id4 = pCmd->LIGHTING2.id4;
		std::string node_id = std::to_string(id4);
		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		int level = pCmd->LIGHTING2.level;
		int filler = pCmd->LIGHTING2.filler;
		int rssi = pCmd->LIGHTING2.rssi;
		std::string name = "";
		uint64_t ulId1 = id1; // PRIu64
		bool bIsNewDevice = false;

		//Log(LOG_STATUS, "Netatmo WriteToHardware subType %d id1 %d id2 %d id3 %d id4 %d bIsOn %d level %d filler %d rssi %d", subtype, id1, id2, id3, id4, bIsOn, level, filler, rssi);
		Debug(DEBUG_HARDWARE, "Netatmo WriteToHardware subType %d id1 %d id2 %d id3 %d id4 %d bIsOn %d level %d filler %d rssi %d", subtype, id1, id2, id3, id4, bIsOn, level, filler, rssi);
		int length = xcmd->len;
		int uid = xcmd->id;
		int unitcode = xcmd->unitcode;
		int xcmdType = xcmd->type;
		int SUB_Type = xcmd->subtype;
		int battery_level = xcmd->battery_level;
		int cmnd_SetLevel = xcmd->cmnd;
		int selectorLevel = xcmd->level;
		int _rssi_ = xcmd->rssi;
		int uid_hex = uid;
		//Log(LOG_STATUS, "Netatmo Write xcmd subType %" PRIu64 " length %d uid %d %08X unitcode %d xcmdType %d SUB_Type %d battery_level %d gswitch_sSetLevel %d selectorLevel %d rssi %d", ulId1, length, uid, uid, unitcode, xcmdType, SUB_Type, battery_level, cmnd_SetLevel, selectorLevel, _rssi_);
		Debug(DEBUG_HARDWARE, "Netatmo Write xcmd subType %" PRIu64 " length %d uid %d %08X unitcode %d xcmdType %d SUB_Type %d battery_level %d gswitch_sSetLevel %d selectorLevel %d rssi %d", ulId1, length, uid, uid_hex, unitcode, xcmdType, SUB_Type, battery_level, cmnd_SetLevel, selectorLevel, _rssi_);

		uint8_t unit = NETATMO_PRESET_UNIT; //preset mode
		int switchType = STYPE_Selector;
		int devType = packettype;  //unsigned char
		int subType = sSwitchTypeSelector;
		//Debug(DEBUG_HARDWARE, "Netatmo uid %08X", uid);
		//
		//Selector Switch for Thermostat Mode and Blinds
		if ((SUB_Type == 62) || (SUB_Type == 73))
		{
			set_level = selectorLevel;
		}
		//Dimmer Switch
		else if (SUB_Type == 11 && cmnd_SetLevel == 2)
		{
			set_level = selectorLevel;
			//return SetDimmerState(uid, set_level);
		}
		else
		{
			set_level = cmnd_SetLevel;
		}

		return SetProgramState(uid, set_level);
	}
	else
	{
		int node_id = pCmd->LIGHTING2.id4;
		const _tGeneralSwitch* xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);

		Log(LOG_STATUS, "Packettype unKnown %d", packettype);
	}
	return false;
}


/// <summary>
/// Set the thermostat / valve in "away mode"
/// </summary>
/// <param name="idx"> of the device to set in away mode</param>
/// <param name="bIsAway">wether to put in away or normal / schedule mode</param>
/// <returns>success status</returns>
bool CNetatmo::SetAway(const int id, const bool bIsAway)
{
	//Debug(DEBUG_HARDWARE, "NetatmoThermostat Away id = %d", id);
	int uid = id;
	return SetProgramState(uid, (bIsAway == true) ? 1 : 0);
}


/// <summary>
/// Set the thermostat / valve operationnal mode
/// </summary>
/// <param name="uid">id of the device to put in away mode</param>
/// <param name="newState">Mode of the device </param>
/// <returns>success status</returns>
bool CNetatmo::SetProgramState(const int uid, const int newState)
{
	//Check if logged, logging if needed
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}

	//Debug(DEBUG_HARDWARE, "SetProgramState");
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	Json::Value root;       // root JSON object
	std::string home_data;
	int _state;
	std::string _data;
	std::stringstream d;
	d << newState;
	d >> _state;
	bool bRet;              //Parsing status
	bool bHaveDevice;
	std::string module_id = m_DeviceModuleID[uid];            // mac-adres
	std::string type_module = m_Device_types[module_id];
	std::string module_MAC  = m_thermostatModuleID[uid];
	std::string name = m_ModuleNames[module_id];
	std::string Device_bridge = m_DeviceBridge[module_id];
	std::string roomNetatmoID = m_RoomIDs[module_id];
	std::string Home_id = m_DeviceHomeID[roomNetatmoID];      // Home_ID
	Debug(DEBUG_HARDWARE, "SetProgramState - Device MAC %s - Type %s State: %d", module_id.c_str(), type_module.c_str(), newState);

	//Log(LOG_STATUS, "SetProgramState - Device MAC %s - Type %s State: %d", module_id.c_str(), type_module.c_str(), newState);

	if (!m_thermostatModuleID[uid].empty())
	{
		//Debug(DEBUG_HARDWARE, "Set Thermostat State MAC = %s - %d", module_id.c_str(), newState);

		std::string thermState;
		switch (newState)
		{
		case 0:
			thermState = "off";      //The Thermostat is off (Not Supported by Thermostat)
			break;
		case 10:
			thermState = "schedule"; //The Thermostat is currently following its weekly schedule
			break;
		case 20:
			thermState = "away";     //The Thermostat is currently applying the away temperature
			break;
		case 30:
			thermState = "hg";       //he Thermostat is currently applying the frost-guard temperature
			break;
		default:
			Log(LOG_ERROR, "Netatmo: Invalid Thermostat state!");
			return false;
		}
		//https://api.netatmo.com/api/setthermmode?home_id=xxxxx&mode=schedule/away/hg
		home_data = "home_id=" + Home_id + "&mode=" + thermState ;
		Get_Respons_API(NETYPE_SETTHERMMODE, sResult, home_data, bRet, root, "");

		if (!bRet)
		{
			Log(LOG_ERROR, "NetatmoThermostat: Error setting Thermostat Mode !");
			return false;
		}
		bHaveDevice = true;
		// Energy        {NAPlug, OTH, BNS}
		// Home+control  {NLG,    OTH, BNS, NBG,              BNMH}
		// Home+security {NACamera, NOC, NDB, NSD, NCO, BNCX, BNMH}

		// NAPlug = Netatmo Thermostat
		// BNS = Smarther with Netatmo Thermostat
		// NLG = Legrand Gateway
		// OTH = Opentherm Thermostat Relay
		// NBG = 
		// BNMH = Bticino MyHome server 1
		//
		std::string Type_bridge = m_Device_types[Device_bridge];
		Debug(DEBUG_HARDWARE, "Type Device bridge = %s", Type_bridge.c_str());
		std::string Type;             // Only parse devices off current selected Device bridge
		if (type_module == "NRV")
			Type = "NAPlug";
		else
			Type = type_module;
		Debug(DEBUG_HARDWARE, "Device Type %s =  %s ??", Type.c_str(), type_module.c_str());
		// When a thermostat mode is changed all thermostat/valves in Home are changed by Netatmo
		// So we have to update the corresponding devices       Device_Types {NAPlug, OTH, BNS}
		// https://api.netatmo.com/api/homestatus?home_id=xxxxx&device_types=NAPlug
		//
		std::string _data = "home_id=" + Home_id + "&device_types=" + Type ;

		Get_Respons_API(NETYPE_STATUS, sResult, _data, bRet, root, "");
		//Parse API response
		bRet = ParseHomeStatus(sResult, root, Home_id);
		if (!bRet)
		{
			Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
			return false;
		}
	}

	if(!m_PowerDeviceID[uid].empty())
	{
		// Home+control  {NLG,    OTH, BNS, NBG,              BNMH, NLF, NLP, NLPO, NLM}
		// Home+security {NACamera, NOC, NDB, NSD, NCO, BNCX, BNMH}
		//Debug(DEBUG_HARDWARE, "Set Program State MAC = %s - %d", module_id.c_str(), newState);
		std::string _data;
		std::string State;

		if (type_module == "NOC")
		{
			std::string State;
			switch (newState)
			{
			case 0:
				State = "off";
				break;
			case 10:
				State = "on";
				break;
			case 20:
				State = "auto";
				break;
			default:
				Log(LOG_ERROR, "Netatmo: Invalid Floodlight state!");
				return false;
			}
			//
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["floodlight"] = State;
			json_data["home"]["modules"][0]["id"] = module_id;
			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"floodlight\":\"" + State + "\"}]}}" ;
		}
		else if (type_module == "NLF")
		{
			bool State;
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = module_id;
			json_data["home"]["modules"][0]["bridge"] = Device_bridge;

			switch (newState)
			{
			case 0:
				State = false;
				json_data["home"]["modules"][0]["on"] = State;
				break;
			case 1:
				State = true;
				json_data["home"]["modules"][0]["on"] = State;
				break;
			default:
				Log(LOG_ERROR, "Netatmo: Invalid NLF Device state!");
				return false;
			}
			//
			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"brightness\":\"" + _state + "\",\"bridge\":\"" + Device_bridge + "\"}]}}" ;
		}
		else if (type_module == "NLLF")
		{
			bool State;
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = module_id;
			json_data["home"]["modules"][0]["bridge"] = Device_bridge;

			switch (newState)
			{
			case 0:
				State = false;
				json_data["home"]["modules"][0]["fan_speed"] = 0;        //The Fan is off (Not Supported by API)
				break;
			case 10:
				State = true;
				json_data["home"]["modules"][0]["fan_speed"] = 1;        //The Fan is set to speedselection 1
				break;
			case 20:
				State = true;
				json_data["home"]["modules"][0]["fan_speed"] = 2;        //The Fan is set to speedselection 2
				break;
			default:
				Log(LOG_ERROR, "Netatmo: Invalid NLF Device state!");
				return false;
			}
			//
			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"brightness\":\"" + _state + "\",\"bridge\":\"" + Device_bridge + "\"}]}}" ;
		}
		else if (type_module == "NLP" || type_module == "NLPO" || type_module == "NLM" || type_module == "NLC" || type_module == "NLL" || type_module == "NLPM" || type_module == "NLPT" || type_module == "BNIL" || type_module == "BNCS")
		{
			bool State;
			switch (newState)
			{
			case 0:
				State = false;
				break;
			case 1:
				State = true;
				break;
			default:
				Log(LOG_ERROR, "Netatmo: Invalid Power Device state!");
				return false;
			}
			//module NLP NLPM NLC BNCS NLF NLM NLL NLPT BNIL
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = module_id;
			json_data["home"]["modules"][0]["on"] = State;
			json_data["home"]["modules"][0]["bridge"] = Device_bridge;
			_data = json_data.toStyledString();
			//_data = "{"home":{"id":"" + Home_id + "","modules":[{"id":"" + module_id + "","on":" + State + ","bridge":"" + Device_bridge + ""}]}}" ;
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"module\":[{\"id\":\"" + module_id + "\",\"on\":\"" + State + "\",\"bridge\":\"" + Device_bridge + "\"}]}}" ;
		}
		else if (type_module == "NLV" || type_module == "NLLV"  || type_module == "NLIV"  || type_module == "Z3V" || type_module == "BNAS")
		{
			Debug(DEBUG_HARDWARE, "SendBlindSensor (%s) %s command %d bridge %s", Home_id.c_str(), module_id.c_str(), _state, Device_bridge.c_str());
			//open shutter NLV BNAS NLLV NLIV Z3V
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = module_id;
			json_data["home"]["modules"][0]["target_position"] = _state;
			json_data["home"]["modules"][0]["bridge"] = Device_bridge;
			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"target_position\":\"" + _state + "\"}]}}" ;
		}
		else if (type_module == "NLG")
		{
			std::string SchName = m_ModuleNames["999"];
			Home_id = module_id;
			std::map<int, std::string> scenarios_names;
			scenarios_names = m_Scenarios[Home_id];
			std::string scenario_Name;
			int i = 0;

			for (std::map<int, std::string>::const_iterator itt = scenarios_names.begin(); itt != scenarios_names.end(); ++itt)
			{
				std::stringstream ss;
				//Debug(DEBUG_HARDWARE, "Gateway first  ; %d", itt->first);
				//Debug(DEBUG_HARDWARE, "Gateway second ; %s", itt->second.c_str());
				ss << itt->first;
				ss >> i;
				if (i == newState)
					scenario_Name = itt->second.c_str();
				i += 10;
			}

			//Scenario NLG
			Debug(DEBUG_HARDWARE, "Gateway set scenario %s", scenario_Name.c_str());
			m_selectedScenario[Home_id] = newState;
			Debug(DEBUG_HARDWARE, "Gateway Bridge %s Home_id %s newState %d ", m_DeviceBridge[module_id].c_str(), m_DeviceModuleID[uid].c_str(), newState);
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = Device_bridge;
			json_data["home"]["modules"][0]["scenario"] = scenario_Name;
			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"scenario\":\"" + State + "\"}]}}" ;
		//}
		//else if (type_module == "")
		//{
			//setpoint of the room to away or comfort NLC
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"therm_setpoint_mode\":\"" + "manual" + "\",\"therm_setpoint_fp\":\"" + "away" + "\",\"therm_setpoint_end_time\":\"" + 1505368800 + "\"}]}}" ;

			//setpoint of the room to Frost Guard NLC
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"therm_setpoint_mode\":\"" + "hg" + "\"}]}}" ;

			//temperatuur of the room BNS BNTH
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"therm_setpoint_end_time\":\"" + 1505368800 + "\",\"therm_setpoint_mode\":\"" + "manual" + "\",\"therm_setpoint_temperature\":\"" + 20 + "\"}]}}" ;

			//BOOST of the room (SMARTHER in heating)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"therm_setpoint_end_time\":\"" + 1505368800 + "\",\"therm_setpoint_mode\":\"" + "max" + "\"}]}}" ;

			//frostguard of the room (SMARTHER in heating)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"therm_setpoint_mode\":\"" + "hg" + "\"}]}}" ;

			//home schedule of the room (SMARTHER in heating)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"therm_setpoint_mode\":\"" + "home" + "\"}]}}" ;

			//Temperature setpoint of the room (SMARTHER in heating)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"cooling_setpoint_end_time\":\"" + 1505368800 + "\",\"cooling_setpoint_mode\":\"" + "manual" + "\", "cooling_setpoint_temperature\":\"" + 20 + "\"}]}}" ;

			//BOOST of the room (SMARTHER in heating)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"cooling_setpoint_end_time\":\"" + 1505368800 + "\",\"cooling_setpoint_mode\":\"" + "max" + "\"}]}}" ;

			//frostguard of the room (SMARTHER in cooling)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"cooling_setpoint_mode\":\"" + "off" + "\"}]}}" ;

			//Home schedule of the room (SMARTHER in cooling)
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"rooms\":[{\"id\":\"" + module_id + "\",\"cooling_setpoint_mode\":\"" + "home" + "\"}]}}" ;
		}
		else
		{
			Log(LOG_ERROR, "Netatmo: Invalid Device state!");
			_data = "";
		}
		home_data = "&";
		//Log(LOG_STATUS, "SetProgramState - JSON_data = %s", _data.c_str());

		Get_Respons_API(NETYPE_SETSTATE, sResult, home_data, bRet, root, _data);
		if (!bRet)
		{
			Log(LOG_ERROR, "Netatmo: Error setting Power Device !");
			return false;
		}
		bHaveDevice = true;
	}

	if(!m_LightDeviceID[uid].empty())
	{
		std::string _data;
		_state = newState;

		if (type_module == "NLF")
		{
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = module_id;
			json_data["home"]["modules"][0]["bridge"] = Device_bridge;
			json_data["home"]["modules"][0]["brightness"] = _state;

			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"brightness\":\"" + _state + "\",\"bridge\":\"" + Device_bridge + "\"}]}}" ;
		}

		home_data = "&";

		Get_Respons_API(NETYPE_SETSTATE, sResult, home_data, bRet, root, _data);
		if (!bRet)
		{
			Log(LOG_ERROR, "Netatmo: Error setting Light Device !");
			return false;
		}
		bHaveDevice = true;
	}

	//Return error if no device are found
	if (!bHaveDevice)
	{
		Log(LOG_ERROR, "Netatmo: Online device not available !");
		return false;
	}
	return true;
}


/// <summary>
/// Set the Dimmer State
/// </summary>
/// <param name="uid">id of the device to put in away mode</param>
/// <param name="newState">Mode of the device </param>
/// <returns>success status</returns>
bool CNetatmo::SetDimmerState(const int uid, const int newState)
{
	//Check if logged, logging if needed
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}

	//Debug(DEBUG_HARDWARE, "SetDimmerState");
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	Json::Value root;       // root JSON object
	std::string home_data;
	int _state;
	std::stringstream d;
	d << newState;
	d >> _state;
	bool bRet;              //Parsing status
	bool bHaveDevice;
	std::string module_id = m_DeviceModuleID[uid];            // mac-adres
	std::string type_module = m_Device_types[module_id];
	std::string module_MAC  = m_thermostatModuleID[uid];
	std::string name = m_ModuleNames[module_id];
	std::string Device_bridge = m_DeviceBridge[module_id];
	std::string roomNetatmoID = m_RoomIDs[module_id];
	std::string Home_id = m_DeviceHomeID[roomNetatmoID];      // Home_ID
	Log(LOG_STATUS, "SetDimmerState - Device MAC %s - Type %s State: %d", module_id.c_str(), type_module.c_str(), newState);

	if(!m_LightDeviceID[uid].empty())
	{
		// Home+control  {NLG,    OTH, BNS, NBG,              BNMH, NLF, NLP, NLPO, NLM}
		std::string _data;

		if (type_module == "NLF")
		{
			Json::Value json_data;
			//json_data {"body":{"home":{"id":
			json_data["home"]["id"] = Home_id;
			json_data["home"]["modules"][0]["id"] = module_id;
			json_data["home"]["modules"][0]["bridge"] = Device_bridge;
			json_data["home"]["modules"][0]["brightness"] = _state;

			_data = json_data.toStyledString();
			//_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"brightness\":\"" + _state + "\",\"bridge\":\"" + Device_bridge + "\"}]}}" ;
		}
		else
		{
			Log(LOG_ERROR, "Netatmo: Invalid Dimmer Type !");
			_data = "";
		}
		home_data = "&";
		Debug(DEBUG_HARDWARE, "SetProgramState - JSON_data = %s", _data.c_str());

		Get_Respons_API(NETYPE_SETSTATE, sResult, home_data, bRet, root, _data);
		if (!bRet)
		{
			Log(LOG_ERROR, "Netatmo: Error setting Dimmer Device !");
			return false;
		}
		bHaveDevice = true;
	}

	//Return error if no device are found
	if (!bHaveDevice)
	{
		Log(LOG_ERROR, "Netatmo: Online Dimmer not available !");
		return false;
	}
	return true;
}


/// <summary>
/// Set temperture override on thermostat / valve for
/// one hour
/// </summary>
/// <param name="idx">ID of the device to override temp</param>
/// <param name="temp">Temperature to set</param>
void CNetatmo::SetSetpoint(unsigned long ID, const float temp)
{
        //Check if still connected to the API
	//connect to it if needed
	if (!m_isLogged == true)
	{
		if (!Login())
			return;
	}

	unsigned char uid = (unsigned char)ID;
	std::string sName = "";
	std::string id;
	id = std::to_string(ID);
	Json::Value root;
	std::string module_id = m_DeviceModuleID[uid];             //uid
	std::string name = m_ModuleNames[module_id];
	std::string roomNetatmoID = m_RoomIDs[module_id];
	std::string Home_id = m_DeviceHomeID[roomNetatmoID];      // Home_ID
	//Debug(DEBUG_HARDWARE, "Netatmo Thermostat MAC; %s in Room ID = %s in Home: %s", module_id.c_str(), roomNetatmoID.c_str(), Home_id.c_str());
	// mode of Room "manual" / "max" / "home"
	//std::string Mode = m_Room_mode[roomID]; //schedule
	std::string Mode = "manual";
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
	std::string home_data;
	bool ret = false;
	bool bRet;              //Parsing status

	if (m_energyType != NETYPE_ENERGY)
	{
		// Check if thermostat device is available
		if (m_DeviceModuleID[uid].empty())
		{
			Log(LOG_ERROR, "NetatmoThermostat : No thermostat found in online devices!");
			return;
		}

		home_data = "home_id=" + Home_id + "&room_id=" + roomNetatmoID.c_str() + "&mode=" + Mode  + "&temp=" + std::to_string(temp)  + "&endtime=" + std::to_string(end_time) + "&get_favorites=true&";
		// https://api.netatmo.com/api/setroomthermpoint?home_id=xxxxxx&room_id=xxxxxxx&mode=manual&temp=22&endtime=xxxxxxxxx
		Get_Respons_API(NETYPE_SETROOMTHERMPOINT, sResult, home_data, bRet, root, "");
	}
	else         // Not used ??
	{
		//find module id
		std::string module_MAC = m_thermostatModuleID[ID];

		if (module_MAC.empty())
		{
			Log(LOG_ERROR, "NetatmoThermostat: No thermostat or valve found in online devices!");
			return;
		}

		home_data = "home_id=" + Home_id + "&room_id=" + roomNetatmoID.c_str() + "&mode=" + Mode  + "&temp=" + std::to_string(temp)  + "&endtime=" + std::to_string(end_time) + "&get_favorites=true&";
		// https://api.netatmo.com/api/setroomthermpoint?home_id=xxxxxx&room_id=xxxxxxx&mode=manual&temp=22&endtime=xxxxxxxxx
		Get_Respons_API(NETYPE_SETROOMTHERMPOINT, sResult, home_data, bRet, root, "");
		Log(LOG_STATUS, "Netatmo module SetSetpoint else ? ");
	}

	if (!bRet)
	{
		Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint!");
		return;
	}

	//Set up for updating thermostat data when the set point reach its end
	m_tSetpointUpdateTime = time(nullptr) + 60;
	m_bForceSetpointUpdate = true;
}


/// <summary>
/// Change the schedule of the thermostat (new API only)
/// </summary>
/// <param name="scheduleId">ID of the schedule to use</param>
/// <param name="state"> Status of the schedule</param>
/// <returns>success status</returns>
bool CNetatmo::SetSchedule(int uId, int selected)
{
	//Checking if we are still connected to the API
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}

	//std::string scheduleName = m_ScheduleNames[selected];
	std::string schedule_Id = m_ScheduleIDs[selected];
	std::string homeid = m_DeviceHomeID[schedule_Id];            // Home_ID
	Debug(DEBUG_HARDWARE, "Schedule id selected = %s %d %d", schedule_Id.c_str(), uId, selected);

	std::stringstream uid;
	uid << uId;
	std::string schedule_Name;
	int i = 0;
	std::string Home_id = m_ScheduleHomes[uId];                          // home_id from crcId
	std::string module_id = m_thermostatModuleID[uId];                   // mac-adres
	std::map<int, std::string> Schedule_Names = m_ScheduleNames[Home_id];
	for (std::map<int, std::string>::const_iterator itt = Schedule_Names.begin(); itt != Schedule_Names.end(); ++itt)
	{
		std::stringstream ss;
		ss << itt->first;
		ss >> i;
		Debug(DEBUG_HARDWARE, "Data itt %d %s", i, itt->second.c_str());
		if (i == selected)
			schedule_Name = itt->second;
		i += 10;
	}
	//Json::Value Schedules = m_Schedule_Names[Home_id];
	std::string module_type = m_Device_types[module_id];
	std::string scheduleId;

	Debug(DEBUG_HARDWARE, "Schedule id = %s %s %d %s %s %d", module_type.c_str(), scheduleId.c_str(), uId, schedule_Name.c_str(), Home_id.c_str(), selected);
	std::stringstream bstr;
	std::string sResult;
	Json::Value root;       // root JSON object
	std::string scheduleState;
	bool bRet;              //Parsing status

	int state = 10;             //Forcing Thermostat to follow the selected weekly schedule

	switch (state)
	{
		case 0:
			scheduleState = "off";      //The Thermostat is off (Not Suported by  Thermostat)
			break;
		case 10:
			scheduleState = "schedule"; //The Thermostat is currently following its weekly schedule
			break;
		case 20:
			scheduleState = "away";     //The Thermostat is currently applying the away temperature
			break;
		case 30:
			scheduleState = "hg";       //he Thermostat is currently applying the frost-guard temperature
			break;
		default:
			Log(LOG_ERROR, "Netatmo: Invalid Schedule state!");
			return false;
	}

	//https://api.netatmo.com/api/switchhomeschedule?home_id=xxxxxxxx&schedule_id=xxxxxxxx
	std::string home_data = "home_id=" + Home_id + "&schedule_id=" + scheduleId;
	Get_Respons_API(NETYPE_SWITCHHOMESCHEDULE, sResult, home_data, bRet, root, "");

	if (!bRet)
	{
		Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
		return false;
	}
	//store the selected schedule in our local data to avoid
	//changing back the schedule when using away mode
	//m_selectedScheduleID = selected;
	m_selected_Schedule[Home_id] = selected;

	// When a thermostat mode is changed all thermostat/valves in Home are changed by Netatmo
	// So we have to update the corresponding devices
	// Valves (NRV) can also change the mode but the API oly accept Device_Types {NAPlug, OTH, BNS}
	// https://api.netatmo.com/api/homestatus?home_id=xxxxx&device_types=NAPlug
	std::string Type = "NAPlug";
	if ((module_type == "NAPlug") || (module_type == "OTH") || (module_type == "BNS"))
		Type = module_type;
	std::string _data = "home_id=" + Home_id + "&device_types=" + Type;

	Get_Respons_API(NETYPE_STATUS, sResult, _data, bRet, root, "");
	//Parse API response
	bRet = ParseHomeStatus(sResult, root, Home_id);
	if (!bRet)
	{
		Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
		return false;
	}
	return true;
}


/// <summary>
/// Utility to make the URI based on the type of device we
/// want to retrieve initial data
/// </summary>
/// <param name="NType">Netatmo device type</param>
/// <returns>API response</returns>
std::string CNetatmo::MakeRequestURL(const m_eNetatmoType NType, std::string data)
{
	std::stringstream sstr;

	switch (NType)
	{
	case NETYPE_MEASURE:
		sstr << NETATMO_API_URI;
		sstr << "api/getmeasure";
		//"https://api.netatmo.com/api/getmeasure?";
		break;
	case NETYPE_WEATHER_STATION:
		sstr << NETATMO_API_URI;
		sstr << "api/getstationsdata";
		//"https://api.netatmo.com/api/getstationsdata?";
		break;
	case NETYPE_AIRCARE:
		sstr << NETATMO_API_URI;
		sstr << "api/gethomecoachsdata";
		//"https://api.netatmo.com/api/gethomecoachsdata?";
		break;
	case NETYPE_THERMOSTAT:        // OLD API
		sstr << NETATMO_API_URI;
		sstr << "api/getthermostatsdata";
		//"https://api.netatmo.com/api/getthermostatsdata?";
		break;
	case NETYPE_HOME:              // OLD API
		sstr << NETATMO_API_URI;
		sstr << "api/homedata";
		//"https://api.netatmo.com/api/homedata?";
		break;
	case NETYPE_HOMESDATA:         // was NETYPE_ENERGY
		sstr << NETATMO_API_URI;
		sstr << "api/homesdata";
		//"https://api.netatmo.com/api/homesdata?";
		break;
	case NETYPE_STATUS:
		sstr << NETATMO_API_URI;
		sstr << "api/homestatus";
		//"https://api.netatmo.com/api/homestatus?home_id=";
		break;
	case NETYPE_CAMERAS:           // OLD API
		sstr << NETATMO_API_URI;
		sstr << "api/getcamerapicture";
		//"https://api.netatmo.com/api/getcamerapicture?";
		break;
	case NETYPE_EVENTS:
		sstr << NETATMO_API_URI;
		sstr << "api/getevents";
		//"https://api.netatmo.com/api/getevents?home_id=";
		break;
	case NETYPE_SETSTATE:
		sstr << NETATMO_API_URI;
		sstr << "api/setstate";
		//"https://api.netatmo.com/api/setstate?";
		break;
	case NETYPE_SCENARIOS:
		sstr << NETATMO_API_URI;
		sstr << "api/getscenarios";
		//"https://api.netatmo.com/api/getscenarios?";
		break;
	case NETYPE_SETROOMTHERMPOINT:
		sstr << NETATMO_API_URI;
		sstr << "api/setroomthermpoint";
		//"https://api.netatmo.com/api/setroomthermpoint?";
		break;
	case NETYPE_SETTHERMMODE:
		sstr << NETATMO_API_URI;
		sstr << "api/setthermmode";
		//"https://api.netatmo.com/api/setthermmode?";
		break;
	case NETYPE_SETPERSONSAWAY:
		sstr << NETATMO_API_URI;
		sstr << "api/setpersonsaway";
		//"https://api.netatmo.com/api/setpersonsaway?";
		break;
	case NETYPE_SETPERSONSHOME:
		sstr << NETATMO_API_URI;
		sstr << "api/setpersonshome";
		//"https://api.netatmo.com/api/setpersonshome?";
		break;
	case NETYPE_NEWHOMESCHEDULE:
		sstr << NETATMO_API_URI;
		sstr << "api/createnewhomeschedule";
		//"https://api.netatmo.com/api/createnewhomeschedule?";
		break;
	case NETYPE_SYNCHOMESCHEDULE:
		sstr << NETATMO_API_URI;
		sstr << "api/synchomeschedule";
		//"https://api.netatmo.com/api/synchomeschedule?";
		break;
	case NETYPE_SWITCHHOMESCHEDULE:
		sstr << NETATMO_API_URI;
		sstr << "api/switchhomeschedule";
		//"https://api.netatmo.com/api/switchhomeschedule?";
		break;
	case NETYPE_ADDWEBHOOK:
		sstr << NETATMO_API_URI;
		sstr << "api/addwebhook";
		//"https://api.netatmo.com/api/addwebhook?";
		break;
	case NETYPE_DROPWEBHOOK:
		sstr << NETATMO_API_URI;
		sstr << "api/dropwebhook";
		//"https://api.netatmo.com/api/dropwebhook?";
		break;
	case NETYPE_PUBLICDATA:
		sstr << NETATMO_API_URI;
		sstr << "api/getpublicdata";
		//"https://api.netatmo.com/api/getpublicdata?";
		break;

	default:
		return "";
	}

	sstr << "?";
	sstr << data;
	return sstr.str();
}


/// <summary>
/// Get API
/// </summary>
void CNetatmo::Get_Respons_API(const m_eNetatmoType& NType, std::string& sResult, std::string& home_data, bool& bRet, Json::Value& root, std::string extra_data)
{
	//Check if connected to the API
	if (!m_isLogged)
		return;
	//Locals
	std::string httpUrl;                             //URI
	//
	std::stringstream sstr;
	sstr << extra_data.c_str();
	//
	std::vector<std::string> ExtraHeaders;           // HTTP Headers
	ExtraHeaders.push_back("accept: application/json;charset=UTF-8");
	ExtraHeaders.push_back("Content-Type: application/json;charset=UTF-8");
	ExtraHeaders.push_back("Authorization: Bearer " + m_accessToken);
	//             //extra_data = "{\"home\":{\"id\":\"" + Home_id + "\",\"modules\":[{\"id\":\"" + module_id + "\",\"floodlight\":\"" + State + "\"}]}}" ;
	std::vector<std::string> returnHeaders;         // HTTP returned headers
	//https://api.netatmo.com/api/homestatus?home_id=
	//https://api.netatmo.com/api/getevents?home_id=
	//https://api.netatmo.com/api/getroommeasure?home_id=xxxxxxxx&room_id=xxxx&scale=30min&type=temperature&optimize=false&real_time=false"
	//https://api.netatmo.com/api/setthermmode?home_id=xxxxxxxxx&mode=schedule
	//https://api.netatmo.com/api/switchhomeschedule?home_id=xxxxxxxx&schedule_id=xxxxxxxx
	//https://api.netatmo.com/api/getmeasure?device_id=xx:xx&module_id=xx:xx&scale=30min&type=boileron&optimize=false&real_time=false
	//https://api.netatmo.com/api/setstate 
	//        //"Content-Type: application/json" -d "{\"home\":{\"id\":\"xxxxxxxx\",\"modules\":[{\"id\":\"00:xx:xx:xx:xx:xx\",\"floodlight\":\"auto\"}]}}"

	httpUrl = MakeRequestURL(NType, home_data);
	std::string sPostData = sstr.str();
	//Debug(DEBUG_HARDWARE, "Respons URL   %s", httpUrl.c_str()); // URI to be tested

	if (!HTTPClient::POST(httpUrl, sPostData, ExtraHeaders, sResult, returnHeaders))
	{
		Log(LOG_ERROR, "Error connecting to Server (Get_Respons_API): %s", ExtractHtmlStatusCode(returnHeaders).c_str());
		return ;
	}

	//Check for error
	std::string s_Sresult = sResult;
	size_t pos = s_Sresult.find(":");

	if (pos != std::string::npos)
	{
		std::string e_str = s_Sresult.substr(0, pos);

		std::size_t found = e_str.find("error");
		if (found!=std::string::npos)
		{
			Log(LOG_ERROR, "Error data ...  %s", sResult.c_str());
			return ;     // This prevents JSON Logic Error in case off Error respons.
		}
	}

	bRet = ParseJSon(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received (Get_Respons_API) ...");
		return ;
	}

	//Log(LOG_STATUS, "Get_Respons_API message returned from POST(%s): \n%s", httpUrl.c_str(), JSonToFormatString(root).c_str()); // prettifyJson(root);

	if (!root["error"].empty())
        {
		//We received an error
		Log(LOG_ERROR, "Get_Respons_API: Error = %s", root.asString().c_str());  // possible; 'error'  'errors'  'error [message]'
		m_isLogged = false;
		return ;
	}
}


/// <summary>
/// Get details for home to be used in GetHomeStatusDetails / ParseHomeStatus
/// </summary>
void CNetatmo::GetHomesDataDetails()
{
	//Locals
	std::string sResult;  // text returned by API
	std::string home_data = "&get_favorites=true&";
	std::string homeID;
	bool bRet;            //Parsing status
	Json::Value root;     // root JSON object
	Log(LOG_STATUS, "Get HomesData Details ");

	Get_Respons_API(NETYPE_HOMESDATA, sResult, home_data, bRet, root, "");
	//Log(LOG_STATUS, "GetHomesDataDetails HOMESDATA received: \n%s", JSonToFormatString(root).c_str()); // prettifyJson(root)

	if (!root["body"]["homes"].empty())
	{
		for (auto home : root["body"]["homes"])
		{
			if (!home["id"].empty())
			{
				// Home ID from Homesdata
				homeID = home["id"].asString();
				m_homeid.push_back(homeID);
				//Debug(DEBUG_HARDWARE, "Get Home ID %s", homeID.c_str());
				//SaveJson2Disk(home, std::string("./HomesData_" + homeID + ".txt"));
				std::stringstream stream_homeid;
				for(size_t i = 0; i < m_homeid.size(); ++i)
				{
					if(i != 0)
						stream_homeid << ",";
					stream_homeid << m_homeid[i];
				}
				std::string st_homeid = stream_homeid.str();
				//Debug(DEBUG_HARDWARE, "Get Homes ID %s", st_homeid.c_str());

				m_Home_ID = home["id"].asString();
				std::string Home_Name = home["name"].asString();
				m_RoomNames[homeID] = Home_Name;
				// home["altitude"];
				// home["coordinates"]; //List [5.590753555297852, 51.12159997589948]
				// home["country"].asString();
				// home["timezone"].asString();
				Log(LOG_STATUS, "Home id %s updated.", Home_Name.c_str());
				std::string room_name;
				std::string roomTYPE;
				uint64_t moduleId;

				//Get the rooms
				if (!home["rooms"].empty())
				{
					for (auto room : home["rooms"])
					{
						std::string roomNetatmoID = room["id"].asString();
						std::stringstream stream_id;
						stream_id << roomNetatmoID;

						room_name = room["name"].asString();
						roomTYPE = room["type"].asString();
						m_RoomNames[roomNetatmoID] = room_name;
						m_Types[roomNetatmoID] = roomTYPE;
						m_Room[roomNetatmoID] = room["module_ids"];
						std::string module_id;
						homeID = home["id"].asString();
						m_DeviceHomeID[roomNetatmoID] = homeID;

						for (auto module_ids : room["module_ids"])
						{
							// List modules in Room
							module_id = module_ids.asString();
							moduleId = convert_mac(module_id);
							int Hardware_int = (int)moduleId;
							//Debug(DEBUG_HARDWARE, "List Modules in Room_id  %lu - mac =  %s in room %s in home %s",moduleId, module_id.c_str(), roomNetatmoID.c_str(), homeID.c_str());
							m_RoomIDs[module_id] = roomNetatmoID;
						}
						//category not in Rooms?
						if (!room["category"].empty())
						{
							std::string category = room["category"].asString();
							m_Module_category[module_id] = category;
							//Debug(DEBUG_HARDWARE, "roomID %d - Type %s - Name %s - categorie %s ", roomID, roomTYPE.c_str(), room_name.c_str(), category.c_str());
						}
					}
				}
				//Get the module names
				if (!home["modules"].empty())
				{
					for (auto module : home["modules"])
					{
						if (!module["id"].empty())
						{
							std::string type = module["type"].asString();
							std::string macID = module["id"].asString();
							std::string roomNetatmoID;
							m_Device_types[macID] = type;
							uint64_t moduleID = convert_mac(macID);
							int Hardware_int = (int)moduleID;
							//Debug(DEBUG_HARDWARE, "Homedata modules %lu -  %s in Home = %s" , moduleID, macID.c_str(), homeID.c_str());
							m_ModuleNames[macID] = module["name"].asString();
							roomNetatmoID = m_RoomIDs[macID];

							std::string roomName = m_RoomNames[roomNetatmoID];
							std::string roomType = m_Types[roomNetatmoID];
							std::string roomCategory = m_Module_category[macID];
							std::string Module_Name = m_ModuleNames[macID];
							int crcId = Crc32(0, (const unsigned char*)macID.c_str(), macID.length());
							m_ModuleIDs[moduleID] = crcId;

							//Store thermostate name for later naming switch / sensor
							if (type == "NAPlug")
								m_DeviceBridge[homeID] = macID;
							if (module["type"] == "NATherm1")
								m_ThermostatName[macID] = module["name"].asString();
							if (module["type"] == "NRV")
								m_ThermostatName[macID] = module["name"].asString();
							if (!module["category"].empty())
							{
								std::string category = module["category"].asString();
								m_Module_category[macID] = category;
								//Debug(DEBUG_HARDWARE, "category roomID %s device %d - Type %s - Name %s - categorie %s ", roomNetatmoID.c_str(), moduleID, roomType.c_str(), roomName.c_str(), category.c_str());
							}
						}
					}
				}
				if (!home["temperature_control_mode"].empty())
				{
					std::string control_mode = home["temperature_control_mode"].asString();
					//Debug(DEBUG_HARDWARE, "temperature_control_mode %s", control_mode.c_str());
				}
				if (!home["therm_mode"].empty())
				{
					std::string schedule_mode = home["therm_mode"].asString();
					//Debug(DEBUG_HARDWARE, "therm_mode %s", schedule_mode.c_str());
				}
				if (!home["therm_setpoint_default_duration"].empty())
				{
					std::string therm_setpoint_default_duration = home["therm_setpoint_default_duration"].asString();
				}
				//Get the persons
				if (!home["persons"].empty())
				{
					int P_index = 0;
					for (auto person : home["persons"])
					{
						std::string person_id = person["id"].asString();
						if (!person["pseudo"].empty())
						{
							std::string person_name = person["pseudo"].asString();
							m_PersonsNames[person_id] = person_name;
						}
						else
						{
							// int to string
							m_PersonsNames[person_id] = std::to_string(P_index);
						}
						std::string person_url = person["url"].asString();
						P_index ++;
					}
				}
				//Get the schedules
				if (!home["schedules"].empty())
				{
					//Debug(DEBUG_HARDWARE, "Get the schedules from %s", homeID.c_str());
					std::stringstream schedules;
					std::string allSchName = "Off";
					std::string allSchAction = "00";
					int index = 0;
					std::map<std::string, std::string> _data;
					std::map<int, std::string> Schedule_Names;

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

						std::string schedule_name = schedule["name"].asString();
						bool schedule_default = schedule["default"].asBool();                 // true / false
						bool schedule_away_temp = schedule["away_temp"].asBool();
						bool schedule_hg_temp = schedule["hg_temp"].asBool();
						std::string schedule_id = schedule["id"].asString();
						std::string schedule_type = schedule["type"].asString();
						bool schedule_selected = schedule["selected"].asBool();                // true / false
						index += 10;
						_data[schedule_name] = schedule_id;

						if (schedule_type == "therm")
						{
							std::stringstream ssv;
							ssv << index;
							//json_data[ssv.str()] = schedule_name;
							Schedule_Names[index] = schedule["name"].asString();
						}
						m_ScheduleIDs[index] = schedule_id; //Not possible with multiple Homes

						m_DeviceHomeID[schedule_id] = homeID;
						if (!schedule["selected"].empty() && schedule["selected"].asBool() && schedule_type == "therm")
						{
							//m_selectedScheduleID = index;
							m_selected_Schedule[homeID] = index;
						}
					}
					//m_Schedule_Names[homeID] = json_data;
					m_ScheduleNames[homeID] = Schedule_Names;
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
				Debug(DEBUG_HARDWARE, "Get HomeStatus Details");
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

	std::string sResult; // text returned by API
	Json::Value root;    // root JSON object
	bool bRet;           //Parsing status
	std::string home_data = "&get_favorites=true&";
	Debug(DEBUG_HARDWARE, "Poll Get Weather (%d)", m_bFirstTimeWeatherData);

	if (m_bFirstTimeWeatherData)
	{
		//
		Get_Respons_API(NETYPE_WEATHER_STATION, sResult, home_data, bRet, root, "");

		//Parse API response
		bRet = ParseStationData(sResult, false);
	}
}


/// <summary>
/// Get details for homecoach
/// </summary>
void CNetatmo::GetHomecoachDetails()
{
	//Check if connected to the API
	if (!m_isLogged)
		return;
	if (m_bFirstTimeWeatherData)
	{
		//Locals
		std::string sResult; // text returned by API
		std::string home_data = "&get_favorites=true&";
		bool bRet;           //Parsing status
		Json::Value root;    // root JSON object
		Debug(DEBUG_HARDWARE, "Poll Get Homecoach (%d)", m_bFirstTimeWeatherData);

		Get_Respons_API(NETYPE_AIRCARE, sResult, home_data, bRet, root, "");

		//Parse API response
		bRet = ParseStationData(sResult, false);
	}
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
	bool bRet;                             //Parsing status
	Json::Value root;                      // root JSON object
	std::string device_types;
	std::string event_id;
	std::string person_id;
	std::string bridge_id;
	std::string module_id;
	int offset = 0;
	int size = 0;
	std::string locale;
	std::string home_data;
	std::string home_id;
	m_homeid.clear();

	GetHomesDataDetails();                 //Homes Data

	//Debug(DEBUG_HARDWARE, "Home Status Details");   // Multiple Homes possible
	size = (int)m_homeid.size();
	Log(LOG_STATUS, "Home Status Details, size (number of homes) is %d", size);   // Multiple Homes possible
	for (int i = 0; i < size; i++)
	{
		Debug(DEBUG_HARDWARE, "index %d of homes", i);
		home_id = m_homeid[i];
		home_data = "home_id=" + home_id + "&get_favorites=true&";
		//Debug(DEBUG_HARDWARE, "Home_Data : %s ", home_data.c_str());
		//Log(LOG_STATUS, "Home_Data : %s ", home_data.c_str());

		Get_Respons_API(NETYPE_STATUS, sResult, home_data, bRet, root, "");

		//Debug(DEBUG_HARDWARE, "sResult : %s ", sResult.c_str());
		std::string Home_Name = m_RoomNames[home_id];

		//Parse API response
		bRet = ParseHomeStatus(sResult, root, home_id);

		Json::Value scenarios;
		Get_Scenarios(home_id, scenarios);

		if (m_bPollGetEvents)
		{
			Get_Events(home_data, device_types, event_id, person_id, bridge_id, module_id, offset, size, locale);
		}
		Debug(DEBUG_HARDWARE, "Parsed index %d Home Status of HomeID %s %s", i, home_id.c_str(), Home_Name.c_str());
	}
}


/// <summary>
/// Get Historical data from Netatmo Device
/// Devices from Aircare, Weather, Energy and Home + Control API
/// <param name="gateway">MAC-adres of the Netatmo Main Device - Bridge</param>
/// <param name="module_id">MAC-adres of the Netatmo Module</param>
/// <param name="scale">Timeframe between two measurements {30min, 1hour, 3hours, 1day, 1week, 1month}</param>
/// <param name="type">Type of data to be returned</param>
/// Date data are only available for scales from 1 day to 1 month and unit is Local Unix Time in seconds.
/// </summary>
void CNetatmo::Get_Measure(std::string gateway, std::string module_id, std::string scale, std::string type)
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
	//Energy
	//// sum_boiler_off // sum_boiler_on // boileroff // boileron
	//Aircare
	//// temperature // date_max_pressure // date_min_pressure // date_max_noise // date_min_noise // date_max_hum // date_min_hum // date_max_CO2 // date_min_CO2 // date_max_temp
	//// date_min_temp // max_pressure // min_pressure // max_noise // min_noise // max_hum // min_hum // max_CO2 // min_CO2 // max_temp // min_temp // pressure // noise // humidity // CO2
	//Wheather
	//// date_max_gust // date_min_gust // date_max_rain // date_min_rain // date_max_noise // date_min_noise // date_max_pressure // date_min_pressure // date_max_co2 // date_min_co2
	//// date_max_hum // date_min_hum // date_max_temp // date_min_temp // sum_rain // max_rain // min_rain // max_noise // min_noise // max_pressure // min_pressure // max_co2 // min_co2
	//// max_hum // min_hum // max_temp // min_temp // gustangle // guststrength // windangle // windstrength // rain // noise // pressure // co2 // date_max_gust // date_min_gust
	//// date_max_rain // date_min_rain // date_max_noise // date_min_noise // date_max_pressure // date_min_pressure // date_max_co2 // date_min_co2 // date_max_hum // date_min_hum
	//// date_max_temp // date_min_temp // sum_rain // max_rain // min_rain // max_noise // min_noise // max_pressure // min_pressure // max_co2 // min_co2 // max_hum // min_hum
	//// max_temp // min_temp // gustangle // guststrength // windangle // windstrength // rain // noise // pressure // co2 // humidity

	std::string home_data = "device_id=" + gateway + "&module_id=" + module_id + "&scale=" + scale + "&type=" + type + "boileron" + "&date_begin=" + date_begin + "&date_end=" + date_end + "&limit=" + limit  + "&optimize=" + "false" + "&real_time="  + "false" ;
	bool bRet;           //Parsing status

	Get_Respons_API(NETYPE_MEASURE, sResult, home_data, bRet, root, "");

	if (!root["body"].empty())
	{
		//https://api.netatmo.com/api/getmeasure?device_id=xxxxx&module_id=xxxxx&scale=30min&type=sum_boiler_off&type=sum_boiler_on&type=boileroff&type=boileron&optimize=false&real_time=false
		//	// Data was parsed with success
		Log(LOG_STATUS, "Measure Data parsed");
	}
}


/// <summary>
/// Get events
/// <param name="home_id">ID-number of the NetatmoHome</param>
/// <param name="device_types">Type of the module {NACamera, NOC, NSD, NCO, NDB, BNCX, BNMH} to retrieve last 30 events</param>
/// <param name="event_id">identification number of event</param>
/// <param name="person_id">identification number of detected person</param>
/// <param name="device_id">MAC-adres of the Netatmo Device</param>
/// <param name="module_id">Identification number of Netatmo</param>
/// <param name="offset">Number of additional events retrieved when using person_id or device_id and module_id parameters</param>
/// <param name="size">Number of events when using event_id parameter (default value is 30)</param>
/// <param name="locale">Localisation for language of the responding Message</param>
/// </summary>
void CNetatmo::Get_Events(std::string home_id, std::string device_types, std::string event_id, std::string person_id, std::string device_id, std::string module_id, int offset, int size, std::string locale)
{
	//Check if connected to the API
	if (!m_isLogged)
		return ;

	//Locals
	std::string sResult; // text returned by API
	Json::Value root;    // root JSON object
	std::string offset_str = std::to_string(offset);
	std::string size_str = std::to_string(size);
	std::string home_events_data;
	// https://api.netatmo.com/api/getevents?home_id=xxx&device_types=xxx&event_id=xxx&person_id=xxx&device_id=xxx&module_id=xxx&offset=15&size=15&locale=nl
	if (!device_id.empty())
		home_events_data = home_id + "&device_types=" + device_types + "&event_id=" + event_id + "&person_id=" + person_id + "&module_id=" + module_id + "&offset=" + offset_str + "&size=" + size_str + "&";
	else
		home_events_data = home_id + "&";

	bool bRet;           //Parsing status

	Get_Respons_API(NETYPE_EVENTS, sResult, home_events_data, bRet, root, "");

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
/// Get Scenarios
/// <param name="home_id">ID-number of the NetatmoHome</param>
/// </summary>
void CNetatmo::Get_Scenarios(std::string home_id, Json::Value& scenarios)
{
	//Locals
	std::string sResult; // text returned by API
	Json::Value root;    // root JSON object

	//Check if connected to the API
	if (!m_isLogged)
		return;

	// https://api.netatmo.com/api/getscenarios
	std::string home_data = "home_id=" + home_id;
	bool bRet;           //Parsing status

	Get_Respons_API(NETYPE_SCENARIOS, sResult, home_data, bRet, root, "");

	if (!root["body"].empty())
	{
		if (!root["body"]["home"].empty())
		{
			//SaveJson2Disk(root, std::string("./scenario-s " + m_Name + "_:_" + home_id + ".txt"));
			scenarios = root["body"]["home"];

			//Selected Scenario ?
			m_selectedScenario[home_id] = "0";

			// Data was recieved with success
			Log(LOG_STATUS, "Scenarios Data Recieved");
		}
	}
	//Parse API response
	bRet = ParseScenarios(sResult, scenarios, home_id);
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
		Log(LOG_STATUS, "Invalid data received (ParseStationData) ...");
		return false;
	}
	bool bHaveDevices = true;

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

	//Return error if no devices are found
	if (!bHaveDevices)
	{
		Log(LOG_STATUS, "No devices found...");
		return false;
	}

	//Get data for the devices
	std::vector<m_tNetatmoDevice> m_netatmo_devices;
	int iDevIndex = 0;
	for (auto device : root["body"]["devices"])
	{
		if (!device["_id"].empty())
		{
			//Main Weather Station
			std::string id = device["_id"].asString();
			std::string type = device["type"].asString();
			std::string name;
                        std::string home_id;
			int mrf_status = 0;
			int RF_status = 0;
			int mbattery_percent = 255;
			uint64_t module_id = convert_mac(id);
			int Hardware_int = (int)module_id;

			if (!device["module_name"].empty())
				name = device["module_name"].asString();
			else if (!device["station_name"].empty())
				name = device["station_name"].asString();
			else if (!device["name"].empty())
				name = device["name"].asString();
			else if (!device["modules"].empty())
				name = device["modules"]["module_name"].asString();
			else
				name = "UNKNOWN NAME";

			//SaveJson2Disk(device, std::string("./" + name + ".txt"));

			//get Home ID from Weatherstation
			if (type == "NAMain")
			{
				m_Home_ID = device["home_id"].asString();
				home_id = device["home_id"].asString();
				Debug(DEBUG_HARDWARE, "m_Home_ID = %s", m_Home_ID.c_str());  //m_Home_ID ........................ numbers and letters
			}

			// Station_name NOT USED upstream ?
			std::string station_name;
			if (!device["station_name"].empty())
				station_name = device["station_name"].asString();
			else if (!device["name"].empty())
				station_name = device["name"].asString();

			int crcId = Crc32(0, (const unsigned char*)id.c_str(), id.length());
			m_ModuleIDs[module_id] = crcId;
			bool bHaveFoundND = false;

			m_tNetatmoDevice nDevice;
			nDevice.ID = crcId;
			nDevice.ModuleName = name;
			nDevice.StationName = station_name;
			nDevice.home_id = home_id;

			// Find the corresponding m_tNetatmoDevice
			std::vector<m_tNetatmoDevice>::const_iterator ittND;
			for (ittND = m_netatmo_devices.begin(); ittND != m_netatmo_devices.end(); ++ittND)
			{
				std::vector<std::string>::const_iterator ittNM;
				for (ittNM = ittND->ModulesIDs.begin(); ittNM != ittND->ModulesIDs.end(); ++ittNM)
				{
					if (*ittNM == id)
					{
						nDevice = *ittND;
						iDevIndex = static_cast<int>(ittND - m_netatmo_devices.begin());
						bHaveFoundND = true;
						break;
					}
				}
				if (bHaveFoundND == true)
					break;
			}

			if (!device["battery_percent"].empty())
 			{
				mbattery_percent = device["battery_percent"].asInt();
				//Debug(DEBUG_HARDWARE, "batterij - bat %d", mbattery_percent);
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
			// Homecoach
			if (!device["dashboard_data"].empty())
			{
				//SaveJson2Disk(device["dashboard_data"], std::string("./" + name + ".txt"));
				ParseDashboard(device["dashboard_data"], iDevIndex, crcId, name, type, mbattery_percent, RF_status, id, home_id);
			}
			//Weather modules (Temp sensor, Wind Sensor, Rain Sensor)
			if (!device["modules"].empty())
			{
				if (device["modules"].isArray())
				{
					// Add modules for this device
					int iModulIndex = 0;
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
							//SaveJson2Disk(module, std::string("./" + mname + ".txt"));
							int crcId = Crc32(0, (const unsigned char*)mid.c_str(), mid.length());
							uint64_t moduleID = convert_mac(mid);
							int Hardware_int = (int)moduleID;
							m_Device_types[mid] = mtype;
							m_ModuleIDs[moduleID] = crcId;
							//Debug(DEBUG_HARDWARE, "%d %lu -  %s " , Hardware_int, moduleID, mid.c_str());

							if (mname.empty())
								mname = "unknown" + mid;
							if (!module["battery_percent"].empty())
							{
								// battery percent %
								mbattery_percent = module["battery_percent"].asInt();
							}
							if (!module["rf_status"].empty())
							{
								// 90=low, 60=highest
								mrf_status = (90 - module["rf_status"].asInt()) / 3;
								if (mrf_status > 10)
									mrf_status = 10;
							}

							if (!module["dashboard_data"].empty())
							{
								//SaveJson2Disk(module["dashboard_data"], std::string("./" + mname + ".txt"));
								ParseDashboard(module["dashboard_data"], iModulIndex, crcId, mname, mtype, mbattery_percent, mrf_status, mid, home_id);
								nDevice.SignalLevel = mrf_status;
								nDevice.BatteryLevel = mbattery_percent;
							}

						}
						else
							nDevice.ModulesIDs.push_back(module.asString());
					}
				}
			}
			Log(LOG_STATUS, "Station Data parsed %s", name.c_str());
			m_netatmo_devices.push_back(nDevice);
			m_known_thermotats.push_back(nDevice);
		}
		iDevIndex++;
	}
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
/// <param name="Hardware_ID">MAC-adres of the Netatmo Device</param>
/// <param name="home_id">ID-number of the NetatmoHome</param>
/// <returns>success retreiving and parsing data</returns>
bool CNetatmo::ParseDashboard(const Json::Value& root, const int DevIdx, const int ID, std::string& name, const std::string& ModuleType, const int battery_percent, const int rssiLevel, std::string& Hardware_ID, std::string& homeID)
{
	//Local variable for holding data retrieved
	bool bHaveTemp = false;
	bool bHaveHum = false;
	bool bHaveBaro = false;
	bool bHaveCO2 = false;
	bool bHaveRain = false;
	bool bHaveSound = false;
	bool bHaveWind = false;
	bool bHaveSetpoint = false;

	int temp = 0, sp_temp = 0;
	float Temp = 0, SP_temp = 0;
	int hum = 0;
	int baro = 0;
	int co2 = 0;
	int rain = 0;
	float rain_1 = 0;
	float rain_24 = 0;
	int sound = 0;

	int wind_angle = 0;
	float wind_strength = 0;
	float wind_gust = 0;
	float wind_Temp = 0;
	float wind_Chill = 0;
	std::string Temp_outdoor = "0";

	int batValue = battery_percent;
	bool action = 0;
	int nValue = 0;
	std::stringstream t_str;
	std::stringstream sp_str;
	std::string str_ID;
	std::string str_ID4;
	std::string str_ID2;
	std::stringstream bt;

        // Hardware_ID mac to int
        uint64_t Hardware_convert = convert_mac(Hardware_ID);
        int Hardware_int = (int)Hardware_convert;
	std::stringstream hardware;

	//convert (integer) ID to std::string
	hardware << std::uppercase << std::hex << ID;
        hardware >> str_ID;

	//Only the last 4 degits:
	str_ID4 = std::to_string(ID & 0xFFFF);	//str_ID.substr(str_ID.length() - 4, 4);
	//Only the last 2 degits:
	str_ID2 = std::to_string(ID & 0xFF);	//str_ID.substr(str_ID.length() - 2, 2);

        //converting ID to char const
        char const* pchar_ID = str_ID.c_str();

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

		//Debug(DEBUG_HARDWARE, "Module [%s] last update = %s (%d)", name.c_str(), ctime(&tNetatmoLastUpdate), tNetatmoLastUpdate);

		// check if Netatmo data was updated in the past NETAMO_POLL_INTERVALL (+1 min for sync time lags)... if not means sensors failed to send to cloud
		int Interval = NETAMO_POLL_INTERVALL + 60;
		//Debug(DEBUG_HARDWARE, "Module [%s] Interval = %d", name.c_str(), Interval);

		if (tNetatmoLastUpdate > (tNow - Interval))
		{
			if (!m_bNetatmoRefreshed[ID])
			{
				Log(LOG_STATUS, "cloud Dashboard for module [%s] is now updated again", name.c_str());
				m_bNetatmoRefreshed[ID] = true;
			}
		}
		else
		{
			if (m_bNetatmoRefreshed[ID])
				Log(LOG_ERROR, "cloud Dashboard for module [%s] no longer updated (module possibly disconnected) since %s", name.c_str(), ctime(&tNetatmoLastUpdate));
			m_bNetatmoRefreshed[ID] = false;
			return false;
		}
	}

	if (!root["Temperature"].empty())
	{
		bHaveTemp = true;
		Temp = root["Temperature"].asFloat();
		t_str << std::setprecision(2) << Temp;
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Type = %s", ModuleType.c_str());
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Temperature [%s]", t_str.str().c_str());
	}
	else if (!root["temperature"].empty())
	{
		bHaveTemp = true;
		Temp = root["temperature"].asFloat();
		t_str << std::setprecision(2) << Temp;
		temp = static_cast<int>(Temp);
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module int temperature [%s]", t_str.str().c_str());
	}
	if (!root["Sp_Temperature"].empty())
	{
		bHaveSetpoint = true;
		SP_temp = root["Sp_Temperature"].asFloat();
		sp_str << std::setprecision(2) << SP_temp;
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Sp [%s]", sp_str.str().c_str());
	}
	else if (!root["setpoint_temp"].empty())
	{
		bHaveSetpoint = true;
		SP_temp = root["setpoint_temp"].asFloat();
		sp_str << std::setprecision(2) << SP_temp;
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module setpoint [%s]", sp_str.str().c_str());
	}
	if (!root["Humidity"].empty())
	{
		bHaveHum = true;
		hum = root["Humidity"].asInt();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module hum [%d]", hum);
	}
	if (!root["Pressure"].empty())
	{
		bHaveBaro = true;
		baro = root["Pressure"].asInt();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Pressure [%d]", baro);
	}
	if (!root["Noise"].empty())
	{
		bHaveSound = true;
		sound = root["Noise"].asInt();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Noise [%d]", sound);
	}
	if (!root["CO2"].empty())
	{
		bHaveCO2 = true;
		co2 = root["CO2"].asInt();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module CO2 [%d]", co2);
	}
	if (!root["Rain"].empty())
	{
		bHaveRain = true;
		rain = root["Rain"].asInt();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Rain [%d]", rain);
	}
	if (!root["sum_rain_1"].empty())
	{
		bHaveRain = true;
		rain_1 = root["sum_rain_1"].asFloat();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Rain_1 [%f]", rain_1);
	}
	if (!root["sum_rain_24"].empty())
	{
		bHaveRain = true;
		rain_24 = root["sum_rain_24"].asFloat();
		//Debug(DEBUG_HARDWARE, "ParseDashBoard Module Rain_24 [%f]", rain_24);
	}
	if (!root["WindAngle"].empty())
	{
		//Debug(DEBUG_HARDWARE, "wind %d : strength %d", root["WindAngle"].asInt(), root["WindStrength"].asInt());
		if ((!root["WindAngle"].empty()) && (!root["WindStrength"].empty()) && (!root["GustAngle"].empty()) && (!root["GustStrength"].empty()))
		{
			bHaveWind = true;
			wind_angle = root["WindAngle"].asInt();
			float windstrength = root["WindStrength"].asFloat();
			wind_strength = windstrength / 3.6F;
			float windgust = root["GustStrength"].asFloat();
			wind_gust = windgust / 3.6F;
		}
	}

	///Debug(DEBUG_HARDWARE, "bHave Temp %d : Hum %d : Baro %d : CO2 %d : Rain %d : Sound %d :  wind %d : setpoint %d", bHaveTemp, bHaveHum, bHaveBaro, bHaveCO2, bHaveRain, bHaveSound, bHaveWind, bHaveSetpoint);
	//Data retrieved create / update appropriate domoticz devices
	std::string sValue;
	std::string roomNetatmoID = m_RoomIDs[Hardware_ID];
	//Debug(DEBUG_HARDWARE, "Hardware_int (%d) %s [%s] %s", Hardware_int, str_ID.c_str(), name.c_str(), Hardware_ID.c_str());

	std::stringstream RF_level;
	RF_level << rssiLevel;
	RF_level >> sValue;
	std::string module_name  = name + " RF. Lvl";

	if (batValue != 255)
		SendPercentageSensor(ID, 3, batValue, static_cast<float>(batValue), name + " - Bat. Level");

	//Temperature and humidity sensors
	if (bHaveTemp && bHaveHum && bHaveBaro)
	{
		int nforecast = m_forecast_calculators[ID].CalculateBaroForecast(Temp, baro); //float temp, double pressure
		//Debug(DEBUG_HARDWARE, "%s name Temp & Hum & Baro %d [%s] %d %d %d - %d / %d ", Hardware_int, Hardware_ID.c_str(), name.c_str(), temp, hum, baro, batValue, rssiLevel);

		// Humidity status: 0 - Normal, 1 - Comfort, 2 - Dry, 3 - Wet
		SendTempHumBaroSensorFloat(ID, batValue, Temp, hum, static_cast<float>(baro), (uint8_t)nforecast, name, rssiLevel);
	}
	else if (bHaveTemp && bHaveHum)
	{
		//Debug(DEBUG_HARDWARE, "name Temp & Hum [%s] %d %d - %d / %d ", name.c_str(), temp, hum, batValue, rssiLevel);
		SendTempHumSensor(ID, batValue, Temp, hum, name, rssiLevel);
	}
	else if (bHaveTemp)
	{
		//Debug(DEBUG_HARDWARE, "name Temp [%s] %d - %d / %d ", name.c_str(), temp, batValue, rssiLevel);
		SendTempSensor(ID, batValue, Temp, name, rssiLevel);
	}

	//Thermostat device
	if (bHaveSetpoint)
	{
		std::string sName = name + " - SetPoint ";
		const uint8_t Unit = 7;
		sp_temp = static_cast<int>(SP_temp);
		std::string sValue = sp_str.str();
		//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] parsedashboard  %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), rssiLevel, batValue);
		SendSetPointSensor(ID, (uint8_t)((ID & 0x00FF0000) >> 16), (ID & 0XFF00) >> 8, ID & 0XFF, Unit, batValue, SP_temp, sName);    // No RF-level
	}

	//Rain meter
	if (bHaveRain)
	{
		//SendRainSensor(ID, batValue, rain_24 + rain_1, name, rssiLevel);
		SendRainSensorWU(ID, batValue, rain_24, rain_1, name, rssiLevel);
	}

	if (bHaveCO2)
	{
		//Debug(DEBUG_HARDWARE, "(%d) DevIdx = %d (%d) co2 = %d %s bHaveCO2 = %d", ID, DevIdx, batValue, co2, name.c_str(), bHaveCO2);
		SendAirQualitySensor(ID, DevIdx, batValue, co2, name);  // No RF-level
		//Debug(DEBUG_HARDWARE, "AirQuality DeviceID = %04X %d %s %d", ID & 0xff, ID & 0xff, name.c_str(), DevIdx);

		if (m_bFirstTimeHomeStatus)
		{
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%d') AND (Unit==%d)", m_HwdID, ID & 0xff, DevIdx);

			if (!result.empty())
			{
				int uId = std::stoi(result[0][0]);
				int nValue = std::stoi(result[0][1]);
				std::string sValue = result[0][2];
				//Debug(DEBUG_HARDWARE, "AirQuality uId %d %s", uId, name.c_str());
				m_sql.UpdateDeviceValue("CustomImage", 27, std::to_string(uId));           //27
	                }
		}
	}

	if (bHaveSound)
	{
		SendSoundSensor(ID, batValue, sound, name);  // No RF-level
	}

	if (bHaveWind)
	{
		SendWind(ID, batValue, wind_angle, wind_strength, wind_gust, wind_Temp, wind_Chill, false, false, name, rssiLevel);
	}

	return true;
}


/// <summary>
/// Parse data for energy/security devices
/// get and create/update domoticz devices
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <param name="root">JSON object to read</param>
/// <param name="home_id">ID-number of the NetatmoHome</param>
/// <returns></returns>
bool CNetatmo::ParseHomeStatus(const std::string& sResult, Json::Value& root, std::string& home_id)
{
	//Log(LOG_STATUS, "ParseHomeStatus for device: \n%s", JSonToFormatString(root).c_str()); // prettifyJson(root);

	//Check if JSON is Ok to parse
	if (root["body"].empty())
		return false;

	if (root["body"]["home"].empty())
		return false;

	//Parse Rooms
	// First pars Rooms for Thermostat
	//Debug(DEBUG_HARDWARE, "Home ID = %s ", home_id.c_str());
	std::string setpoint_mode_str;
	std::string setpoint_mode_fan;
	int setpoint_mode_i;
	int iDevIndex = 0;
	bool setModeSwitch = false;
	std::vector<m_tNetatmoDevice> m_netatmo_devices;
	m_tNetatmoDevice nDevice;
	m_bPollGetEvents = false;

	if (!root["body"]["home"]["rooms"].empty())
	{
		if (!root["body"]["home"]["rooms"].isArray())
			return false;
		Json::Value mRoot = root["body"]["home"]["rooms"];
		//SaveJson2Disk(root, std::string("./HomeStatus_" + m_Name + "_:_" + home_id + ".txt"));

		for (auto room : mRoot)
		{
			if (!room["id"].empty())
			{
				int room_ID;
				std::stringstream room_id_str;
				std::string roomNetatmoID = room["id"].asString();
				room_id_str << roomNetatmoID;
				room_id_str >> room_ID;
				std::string roomName;
				std::string room_temp;
				std::string room_setpoint_str;

				roomName = roomNetatmoID;
				int roomIndex = iDevIndex + 1;
				iDevIndex ++;

				//Find the room name
				// from Homesdata
				roomName = m_RoomNames[roomNetatmoID];
				std::string roomType = m_Types[roomNetatmoID];
				//SaveJson2Disk(room, std::string("./room_") + roomName.c_str() + ".txt");

				if (!room["reachable"].empty())
				{
					// True or False
				}
				if (!room["anticipating"].empty())
				{

				}
				if (!room["heating_power_request"].empty())
				{

				}
				if (!room["open_window"].empty())
				{

				}
				if (!room["therm_measured_temperature"].empty())
				{
					float room_measured = room["therm_measured_temperature"].asFloat();
					std::stringstream room_str;
					room_str << std::fixed << std::setprecision(2) << room_measured;
					room_str >> room_temp;
					m_Room_Temp[roomNetatmoID] = room_temp;
					//Debug(DEBUG_HARDWARE, "room %s %stemp = %s °C - %f ", roomName.c_str(), roomNetatmoID.c_str(), room_temp.c_str(), room_measured);
				}
				if (!room["therm_setpoint_temperature"].empty())
				{
					std::string setpoint_Name = roomName + " - Setpoint";
					float SP_temp = room["therm_setpoint_temperature"].asFloat();
					int sp_temp = static_cast<int>(SP_temp);
					std::stringstream setpoint_str;
					setpoint_str << std::setprecision(2) << SP_temp;
					setpoint_str >> room_setpoint_str;
					m_Room_setpoint[roomNetatmoID] = room_setpoint_str;
					//Debug(DEBUG_HARDWARE, "room %s %stemp = %s °C - %f ", roomName.c_str(), roomNetatmoID.c_str(), room_setpoint_str.c_str(), SP_temp);
				}
				if (!room["therm_setpoint_start_time"].empty())
				{
					int setpoint_start_time = room["therm_setpoint_start_time"].asInt();
				}
				if (!room["therm_setpoint_mode"].empty())
				{
					// create / update the switch for setting away mode
					// on the thermostat (we could create one for each room also,
					// but as this is not something we can do on the app, we don't here)
					// Possible; schedule / away / hg
					std::string setpoint_mode = room["therm_setpoint_mode"].asString();
					m_Room_mode[roomNetatmoID] = setpoint_mode;

					if (setpoint_mode == "away")
					{
						setpoint_mode_str = "20";
						setpoint_mode_i = 20;
					}
					else if (setpoint_mode == "hg")
					{
						setpoint_mode_str = "30";
						setpoint_mode_i = 30;
					}
					else
					{
						setpoint_mode_str = "10";
						setpoint_mode_i = 10;
					}
					// thermostatID not defined
					setModeSwitch = true;
				}
			}
		}
		Debug(DEBUG_HARDWARE, "Rooms Parsed");
	}

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
				std::string bat_percentage;
				std::string batName;
				int batteryLevel;
				float mrf_percentage = 12;
				int mrf_status;
				float rf_strength = 0;
				float last_seen = 0;
				float last_activity = 0;
				int nValue = 0;
				float wifi_status = 0;
				int moduleIndex = iModuleIndex;
				int crcId = 0;
				bool bIsActive;

				bool bHaveTemp = false;
				bool bHaveHum = false;
				bool bHaveBaro = false;
				bool bHaveCO2 = false;
				bool bHaveRain = false;
				bool bHaveSound = false;
				bool bHaveWind = false;
				bool bHaveSetpoint = false;

				int temp = 0, sp_temp = 0;
				float Temp = 0, SP_temp = 0;
				int hum = 0;
				int baro = 0;
				float abs_baro = 0;
				int co2 = 0;
				int rain = 0;
				float rain_1 = 0;
				float rain_24 = 0;
				int sound = 0;
				std::stringstream t_str;

				int wind_angle = 0;
				float wind_strength = 0;
				float wind_gust = 0;
				float wind_gust_angle = 0;
				float wind_Temp = 0;
				float wind_Chill = 0;
				std::string Temp_outdoor = "0";

				bool ionflag = 0;
				int fan_speed = 0;
				double powerflag = 0;
				bool offload = 0;
				int swlevel = 0;

				int target_position;
				int current_position;
				int target_step;

				//uint64_t DeviceRowIdx;
				iModuleIndex ++;
				// Hardware_ID hex to int
				uint64_t Hardware_convert = convert_mac(module_id);
				int Hardware_int = (int)Hardware_convert;
				//converting ID to char const
				char const* pchar_ID = module_id.c_str();
				std::string moduleName = m_ModuleNames[module_id];
				crcId = Crc32(0, (const unsigned char*)module_id.c_str(), module_id.length());
				m_ModuleIDs[Hardware_int] = crcId;
				std::string type = module["type"].asString();
				m_Device_types[module_id] = type;
				//SaveJson2Disk(module, std::string("./") + moduleName.c_str() + ".txt");

				//Debug(DEBUG_HARDWARE, " %d -  %s in Home; %s" , Hardware_int, module_id.c_str(), home_id.c_str());
				nDevice.ID = crcId;
				nDevice.ModuleName = moduleName;
				nDevice.home_id = home_id;
				nDevice.MAC = module_id;

				std::string ID;
				std::string ID2;
				std::string ID4;
				std::stringstream moduleid;

				moduleid << std::uppercase << std::hex << crcId;
				moduleid >> ID;
				ID2 = std::to_string(crcId & 0xFF);	//ID.substr(ID.length()-2, 2);
				ID4 = std::to_string(crcId & 0xFFFF);	// ID.substr(ID.length()-4, 4);
				std::string sValue;
				sValue = module_id;
				bool reachable;
				bool connected = true;
				std::time_t tNetatmoLastUpdate = 0;
				std::time_t tNow = time(nullptr);
				int module_fv;

				if (!module["firmware_revision"].empty())
				{
					//module firmware_revision
					module_fv = module["firmware_revision"].asInt();
				}
				//battery_state
				if (!module["battery_state"].empty())
				{
					std::string battery_state = module["battery_state"].asString();
				}
				//Device to get battery level / network strength
				if (!module["battery_level"].empty())
				{
					batName = " " + moduleName + " Bat. Lvl";
					float battery_Level = module["battery_level"].asFloat() / 100;
					batteryLevel = static_cast<int>(battery_Level);       // Float to int

					std::stringstream bat_str;
					bat_str << std::setprecision(2) << battery_Level;
					bat_str >> bat_percentage;
				}
				else
					batteryLevel = 255;

				if (!module["rf_state"].empty())
				{
					std::string rf_state = module["rf_state"].asString();
				}
				if (!module["last_seen"].empty())
				{
					tNetatmoLastUpdate = static_cast<size_t>(module["last_seen"].asFloat());
					// Check when module last updated values unless for Gateway and Wireless Switch
					if (type == "NLG")
					{
						tNetatmoLastUpdate = 0;
						m_DeviceBridge[home_id] = module_id;
					}
					else if (type == "NLT")
					{
						tNetatmoLastUpdate = 0;
					}
				}
				if (!module["reachable"].empty())
				{
					// True / False    Module disconnected or no battery
					reachable = module["reachable"].asBool();
				}
				if (!module["last_activity"].empty())
				{
					last_activity = module["last_activity"].asFloat();
					//
				}

				//Debug(DEBUG_HARDWARE, "Module [%s] last update = %s", moduleName.c_str(), ctime(&tNetatmoLastUpdate));
				// check if Netatmo data was updated in the past NETAMO_POLL_INTERVALL (+1 min for sync time lags)... if not means sensors failed to send to cloud
				int Interval = NETAMO_POLL_INTERVALL + 60;
				//Debug(DEBUG_HARDWARE, "Module [%s] Interval = %d %lu", moduleName.c_str(), Interval, tNetatmoLastUpdate);

				//Not All devices have a "last seen" so no check for Cloud data possible
				if (tNetatmoLastUpdate != 0)
				{
					if (tNetatmoLastUpdate > (tNow - Interval))
					{
						//Log(LOG_STATUS, "cloud data for module [%s] is now updated again", moduleName.c_str());
						Debug(DEBUG_HARDWARE, "cloud data for module [%s] is now updated again", moduleName.c_str());
					}
					else
					{
						Log(LOG_ERROR, "cloud data for module [%s] no longer updated (module possibly disconnected) since %s", moduleName.c_str(), ctime(&tNetatmoLastUpdate));
						connected = false;  //update, create sensor Blocked if cloud data is older then Interval.
					}
				}

				if (connected)
				{
					m_DeviceModuleID[crcId] = module_id;
					int Type = pTypeGeneralSwitch; // 244;   // Light/Switch
					int SubType = sSwitchTypeAC;   // 11;    // AC
					//62; // Selector Switch

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
							std::string pName = " " + moduleName + " - Signal Level";
							m_Module_Bat_Level[module_id] = batteryLevel;
							int RF_module_int = static_cast<int>(mrf_percentage);       // Float to int
							m_Module_RF_Level[module_id] = RF_module_int;
							std::string sigValue;
							std::stringstream p;
							p << mrf_percentage;
							p >> sigValue;

							// 90=low, 60=highest
							mrf_status = RF_module_int / 10;
							if (RF_module_int < 10)
								mrf_status = 1;
							if (mrf_status > 10)
								mrf_status = 10;

							//Debug(DEBUG_HARDWARE, "mrf_status =  %s -  %d", pName.c_str(), mrf_status);
							nDevice.SignalLevel = mrf_status;
							nDevice.BatteryLevel = batteryLevel;
						}
					}
					if (!module["wifi_state"].empty())
					{
						std::string wifi_state = module["wifi_state"].asString();
					}
					if (!module["wifi_strength"].empty())
					{
						// 86=bad, 56=good
						wifi_status = (86 - module["wifi_strength"].asFloat()) / 3;
						if (wifi_status > 10)
							wifi_status = 10;
						m_wifi_status[module_id] = (int)wifi_status;
						mrf_status = static_cast<int>(wifi_status); // Device has Wifi- or RF-strength not both
					}


					if (!module["battery_level"].empty())
					{
						std::string bat_Name = " " + moduleName + " - Bat. Lvl";
						SendPercentageSensor(crcId, 3, batteryLevel, static_cast<float>(batteryLevel), bat_Name);
					}
					if (!module["ts"].empty())
					{
						//timestamp
						int timestamp = module["ts"].asInt();
					}
					if (!module["sd_status"].empty())
					{
						//status off SD-card
						int sd_status = module["sd_status"].asInt();
						//
					}
					if (!module["alim_status"].empty())
					{
						// Checks the adaptor state
					}
					if (!module["vpn_url"].empty())
					{
						// VPN url from camera
					}
					if (!module["is_local"].empty())
					{
						// Camera is locally connected - True / False
					}
					if (!module["monitoring"].empty())
					{
						// Camera On / Off


					}
					if (!module["bridge"].empty())
					{
						std::string bridge_ = module["bridge"].asString();
						std::string Bridge_Name = m_ModuleNames[bridge_];
						m_DeviceBridge[module_id] = bridge_;
						std::string Module_Name = moduleName + " - Bridge";
						std::string bridgeValue = Bridge_Name + " " + bridge_;
						nDevice.StationName = Bridge_Name;
					}
					if (!module["boiler_valve_comfort_boost"].empty())
					{
						std::string boiler_boost = module["boiler_valve_comfort_boost"].asString();
						std::string bName = moduleName + " - Boost";
						//Debug(DEBUG_HARDWARE, "Boiler Boost %s - %s", bName.c_str(), boiler_boost.c_str() );
						bool bIsActive = module["boiler_valve_comfort_boost"].asBool();

					}
					if (!module["boiler_status"].empty())
					{
						//Thermostat status (boiler heating or not : informationnal switch)
						std::string aName = moduleName + " - Boiler Status";
						bIsActive = module["boiler_status"].asBool();
						std::string boiler_status = module["boiler_status"].asString();
						//Debug(DEBUG_HARDWARE, "Boiler Heating Status %s - %d %d %s - m_Name %s", aName.c_str(), crcId, bIsActive, module["boiler_status"].asString().c_str(), m_Name.c_str());
					}
					if (!module["status"].empty())
					{
						// Door sensor & Sirene
						std::string a_Name = moduleName + " - Status"; //m_[id];
						std::string sValue = module["status"].asString();
						//UpdateValueInt(0, ID.c_str(), 6, pTypeGeneral, sTypeAlert, mrf_status, batteryLevel, '0', sValue.c_str(), a_Name, 0, m_Name);
						int bIsActive;
						if (module["status"].asString() == "closed")
							bIsActive = 0;
						else if (module["status"].asString() == "open")
							bIsActive = 1;
						else if (module["status"].asString() == "weak_signal")
							bIsActive = 2;
						else if (module["status"].asString() == "no_news")
							bIsActive = 3;
						else if (module["status"].asString() == "no_sound") //Status Sirene (NIS)
							bIsActive = 0;
						else
							bIsActive = 4;

						SendAlertSensor(crcId, batteryLevel, bIsActive, sValue.c_str(), a_Name.c_str()); // no RF-status
					}
					if (!module["floodlight"].empty())
					{
						//Light Outdoor Camera Presence - NOC
						//      AUTO / ON / OFF
						std::string lName = moduleName + " - Light";
						bool bIsActive = module["is_local"].asBool();
						int Image = 0;
						bool bDropdown = true;
						bool bHideOff = false;
						std::string status = module["floodlight"].asString();            // "off|on|auto"
						std::string Selector;

						if (status == "off")
						{
							nValue = 0;
							Selector = "0";
						}
						else if (status == "on")
						{
							nValue = 10;
							Selector = "10";
						}
						else if (status == "auto")
						{
							nValue = 20;
							Selector = "20";
						}

						std::string setpoint_mode = module["monitoring"].asString();

						std::vector<std::vector<std::string> > result;
						result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit==%d)", m_HwdID, crcId, NETATMO_PRESET_UNIT);

						if (!result.empty())
                                                {
							int uId = std::stoi(result[0][0]);
							//Debug(DEBUG_HARDWARE, "Floodlight uId %d", uId);
						}
						m_PowerDeviceID[crcId] = lName;
						SendSelectorSwitch(crcId, NETATMO_PRESET_UNIT, Selector, lName, Image, bDropdown, "off|on|auto", "", bHideOff, m_Name);   // No RF-level - Battery level
					}
					// Sensors from new API
					if (!module["temperature"].empty())
					{
						bHaveTemp = true;
						std::stringstream bt;
						Temp = module["temperature"].asFloat();
						t_str << std::setprecision(2) << Temp;
						temp = static_cast<int>(Temp);
						if(type == "NAModule1")
						{
							bt << std::setprecision(2) << Temp;
							Temp_outdoor = bt.str();
						}
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Temperature [%s]", t_str.str().c_str());
					}
					if (!module["co2"].empty())
					{
						bHaveCO2 = true;
						co2 = module["co2"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module CO2 [%d]", co2);
					}
					if (!module["humidity"].empty())
					{
						bHaveHum = true;
						hum = module["humidity"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module hum [%d]", hum);
					}
					if (!module["noise"].empty())
					{
						bHaveSound = true;
						sound = module["noise"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Noise [%d]", sound);
					}
					if (!module["pressure"].empty())
					{
						bHaveBaro = true;
						baro = module["pressure"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Pressure [%d]", baro);
					}
					if (!module["absolute_pressure"].empty())
					{
						bHaveBaro = true;
						abs_baro = module["absolute_pressure"].asFloat();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Absolute Pressure [%f]", baro);
					}
					if (!module["rain"].empty())
					{
						bHaveRain = true;
						rain = module["rain"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Rain [%d]", rain);
					}
					if (!module["sum_rain_1"].empty())
					{
						bHaveRain = true;
						rain_1 = module["sum_rain_1"].asFloat();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Rain [%f]", rain_1);
					}
					if (!module["sum_rain_24"].empty())
					{
						bHaveRain = true;
						rain_24 = module["sum_rain_24"].asFloat();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Rain [%f]", rain_24);
					}
					if (!module["wind_strength"].empty())
					{
						bHaveWind = true;
						float windstrength = module["wind_strength"].asFloat();
						wind_strength = windstrength / 3.6F;
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Wind strength [%f] km/h", windstrength);
					}
					if (!module["wind_angle"].empty())
					{
						bHaveWind = true;
						wind_angle = module["wind_angle"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Wind Agle [%d]", wind_angle);
					}
					if (!module["wind_gust"].empty())
					{
						bHaveWind = true;
						float windgust = module["wind_gust"].asFloat();
						wind_gust = windgust / 3.6F;
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Wind gust [%f] km/h", windgust);
					}
					if (!module["wind_gust_angle"].empty())
					{
						bHaveWind = true;
						wind_gust_angle = module["wind_gust_angle"].asFloat(); // / 3.6F;
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Wind gust angle [%f]", wind_gust_angle);
					}
					// Power Devices
					if (!module["fan_speed"].empty())
					{
						fan_speed = module["fan_speed"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module fan_speed [%d]", fan_speed);
					}
					if (!module["on"].empty())
					{
						ionflag = module["on"].asBool();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module On [%d]", ionflag);
					}
					if (!module["power"].empty())
					{
						powerflag = module["power"].asDouble();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module Power [%d]", powerflag);
					}
					if (!module["brightness"].empty())
					{
						swlevel = module["brightness"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module brightness [%d]", swlevel);
					}
					if (!module["offload"].empty())
					{
						offload = module["offload"].asBool();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module On [%d]", offload);
					}
					if (!module["current_position"].empty())
					{
						current_position = module["current_position"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module current_position [%d]", current_position);
					}
					if (!module["target_position"].empty())
					{
						target_position = module["target_position"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module target_position [%d]", target_position);
					}
					if (!module["target_position:step"].empty())
					{
						target_step = module["target_position:step"].asInt();
						//Debug(DEBUG_HARDWARE, "HomeStatus Module target_step [%d]", target_step);
					}

					//Data retrieved create / update appropriate domoticz devices
					if (bHaveTemp && bHaveHum && bHaveBaro)
					{
						int nforecast = m_forecast_calculators[crcId].CalculateBaroForecast(Temp, baro); //float temp, double pressure
						//Debug(DEBUG_HARDWARE, "%d name Temp & Hum & Baro %s [%s] %f %d %d - %d / %d ", Hardware_int, Hardware_ID.c_str(), name.c_str(), Temp, hum, baro, mrf_status, batteryLevel);
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

						// sValue is string with values separated by semicolon: Temperature;Humidity;Humidity Status;Barometer;Forecast
						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] Temp & Humidity & Baro %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), r.str().c_str(), m_Name.c_str(), mrf_status, batteryLevel);
						//UpdateValueInt(0, ID.c_str(), 0, pTypeTEMP_HUM_BARO, sTypeTHBFloat, mrf_status, batteryLevel, '0', r.str().c_str(), moduleName,  0, m_Name);
						SendTempHumBaroSensorFloat(crcId, batteryLevel, Temp, hum, static_cast<float>(baro), (uint8_t)nforecast, moduleName, mrf_status);
					}
					else if (bHaveTemp && bHaveHum)
					{
						//Debug(DEBUG_HARDWARE, "name Temp & Hum [%s] %d %d - %d / %d ", name.c_str(), temp, hum, batValue, rssiLevel);
						const char status = '0';
						std::stringstream s;
						s << Temp;
						s << ";";
						s << hum;
						s << ";";
						s << status;

						// sValue is string with values separated by semicolon: Temperature;Humidity
						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] Temp & Humidity %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), s.str().c_str(), m_Name.c_str(), mrf_status, batteryLevel);
						//UpdateValueInt(0, ID.c_str(), 0, pTypeTEMP_HUM, sTypeTH_LC_TC, mrf_status, batteryLevel, '0', s.str().c_str(), moduleName, 0, m_Name);
						SendTempHumSensor(crcId, batteryLevel, Temp, hum, moduleName, mrf_status);
					}
					else if (bHaveTemp)
					{
						//Debug(DEBUG_HARDWARE, "name Temp [%s] %d - %d / %d ", name.c_str(), temp, batValue, rssiLevel);
						std::stringstream t;
						t << Temp;
						std::string sValue = "";
						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] temp %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), sValue.c_str(), m_Name.c_str(), mrf_status, batteryLevel);
						//UpdateValueInt(0, ID.c_str(), 0, pTypeGeneral, sTypeTemperature, mrf_status, batteryLevel, temp, sValue.c_str(), moduleName, 0, m_Name);
						SendTempSensor(crcId, batteryLevel, Temp, moduleName, mrf_status);
					}
					//Rain meter
					if (bHaveRain)
					{
						bool bRefetchData = (m_RainOffset.find(crcId) == m_RainOffset.end());
						if (!bRefetchData)
							bRefetchData = ((m_RainOffset[crcId] == 0) && (m_OldRainCounter[crcId] == 0));
						if (bRefetchData)
						{
							// get last rain counter from the database
							bool bExists = false;
							m_RainOffset[crcId] = GetRainSensorValue(crcId, bExists);
							m_RainOffset[crcId] -= rain_1;
							if (m_RainOffset[crcId] < 0)
								m_RainOffset[crcId] = 0;
							if (m_OldRainCounter.find(crcId) == m_OldRainCounter.end())
								m_OldRainCounter[crcId] = 0;
						}
						// daily counter went to zero ?
						if (rain < m_OldRainCounter[crcId])
							m_RainOffset[crcId] += m_OldRainCounter[crcId];

						m_OldRainCounter[crcId] = rain_1;
						std::stringstream v;
						v << rain_1;
						v << ";";
						v << rain_24;

						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] rain %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), v.str().c_str(), m_Name.c_str(), mrf_status, batteryLevel);
						//UpdateValueInt(0, ID.c_str(), 0, pTypeRAIN, sTypeRAINByRate, mrf_status, batteryLevel, '0', v.str().c_str(), moduleName, 0, m_Name);
						//SendRainSensor(crcId, batteryLevel, m_RainOffset[crcId] + m_OldRainCounter[crcId], moduleName, mrf_status);
						SendRainSensorWU(crcId, batteryLevel, rain_24, rain_1, moduleName + "-WU", mrf_status);
					}
					if (bHaveCO2)
					{
						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] co2 rssiLevel %d batValue %d nValue %d sValue %s %s ", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), mrf_status, batteryLevel, co2, std::to_string(co2).c_str(), m_Name.c_str());
						//UpdateValueInt(0, ID.c_str(), 0, pTypeAirQuality, sTypeVoc, mrf_status, batteryLevel, co2, "", moduleName, 0, m_Name);
						int unit;
						if ((type =="NAMain") || (type == "NAModule4"))
							unit = 0;
						else
							unit = iDevIndex;

						//Debug(DEBUG_HARDWARE, "(%d) %d (%s) [%s] co2 rssiLevel %d batValue %d nValue %d sValue %s %s ", Hardware_int, crcId, pchar_ID, moduleName.c_str(), mrf_status, batteryLevel, co2, std::to_string(co2).c_str(), m_Name.c_str());
						SendAirQualitySensor(crcId, unit, batteryLevel, co2, moduleName);
						//Debug(DEBUG_HARDWARE, "AirQualitySensor DeviceID = %04X %d unit = %d", crcId & 0xff, crcId & 0xff, unit);

						std::vector<std::vector<std::string> > result;
						result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d)  AND (DeviceID=='%d')", m_HwdID, crcId & 0xff);

						if (m_bFirstTimeHomeStatus)
						{
							if (!result.empty())
							{
								int uId = std::stoi(result[0][0]);
								int nValue = std::stoi(result[0][1]);
								std::string sValue = result[0][2];
								//Debug(DEBUG_HARDWARE, "AirQualitySensor uId %d %s", uId, moduleName.c_str());
								m_sql.UpdateDeviceValue("CustomImage", 27, std::to_string(uId));           //27
	        				        }
						}
					}

					if (bHaveSound)
					{
						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] sound %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), std::to_string(sound).c_str(), m_Name.c_str(), mrf_status, batteryLevel);
						//UpdateValueInt(0, ID.c_str(), 0, pTypeGeneral, sTypeSoundLevel, mrf_status, batteryLevel, '0', std::to_string(sound).c_str(), moduleName, 0, m_Name);
						SendSoundSensor(crcId, batteryLevel, sound, moduleName);    // No RF-level
					}
					if (bHaveWind)
					{
						std::string wind_direction = WindDirection(static_cast<float>(wind_angle));
						const char status = '0';
						std::stringstream y;
						y << wind_angle;
						y << ";";
						y << wind_direction;
						y << ";";
						y << wind_strength *10;
						y << ";";
						y << wind_gust *10;
						y << ";";
						y << Temp_outdoor;
						y << ";";
						y << wind_Chill;

						// sValue: "<WindDirDegrees>;<WindDirText>;<WindAveMeterPerSecond*10>;<WindGustMeterPerSecond*10>;<Temp_c>;<WindChill_c>"
						//Debug(DEBUG_HARDWARE, "(%d) %s (%s) [%s] wind %s %s %d %d", Hardware_int, str_ID.c_str(), pchar_ID, name.c_str(), y.str().c_str(), m_Name.c_str(), mrf_status, batteryLevel);
						//UpdateValueInt(0, ID.c_str(), 0, pTypeWIND, sTypeWINDNoTempNoChill, mrf_status, batteryLevel, '0', y.str().c_str(), moduleName, 0, m_Name);
						SendWind(crcId, batteryLevel, wind_angle, wind_strength, wind_gust, wind_Temp, wind_Chill, false, false, moduleName, mrf_status);
					}

					if ((type == "NATherm1") || (type == "NRV"))
					{
						int ChildID = 0;
						int roomIndex = 0;
						//Find the room info
						std::string roomNetatmoID = m_RoomIDs[module_id];
						std::string aName = moduleName + " - Heating Status";
						std::string roomName = m_RoomNames[roomNetatmoID];
						std::string roomType = m_Types[roomNetatmoID];
						std::string module_Category = m_Module_category[module_id];
						std::string room_setpoint = m_Room_setpoint[roomNetatmoID];
						std::string room_mode = m_Room_mode[roomNetatmoID];
						std::string room_temp = m_Room_Temp[roomNetatmoID];
						const uint8_t Unit = 7;
						nDevice.roomNetatmoID = roomNetatmoID;
						int sp_temp = stoi(room_setpoint);           // string to int
						float SP_temp = std::stof(room_setpoint);

						SendSetPointSensor(crcId, (uint8_t)((crcId & 0x00FF0000) >> 16), (crcId & 0XFF00) >> 8, crcId & 0XFF, Unit, batteryLevel, SP_temp, moduleName);   // No RF-level
						// thermostatModuleID
						uint64_t mid = convert_mac(module_id);
						int Hardware_int = (int)mid;
						//Debug(DEBUG_HARDWARE, "roomNetatmoID %d  %d -  %s %s in Home; %s ", crcId , Hardware_int, module_id.c_str(), roomNetatmoID.c_str(), home_id.c_str());
						m_thermostatModuleID[crcId] = module_id;              // mac-adres
						//m_RoomIDs[module_id] = roomNetatmoID;                 // Room Netatmo ID
						//m_DeviceHomeID[roomNetatmoID] = home_id;            // Home_ID
						m_ModuleIDs[mid] = crcId;
						//float room_measured = std::stof(room_temp);
						m_thermostatModuleID[crcId] = module_id;     // mac-adres
						//UpdateValueInt(0, ID.c_str(), 8, pTypeTEMP, sTypeTEMP5, mrf_status, batteryLevel, '0', room_temp.c_str(), moduleName, 0, m_Name);
						std::stringstream t_R;
						t_R << room_temp;
						t_R >> Temp;
						SendTempSensor(crcId, batteryLevel, Temp, moduleName, mrf_status);

						if (type == "NATherm1")
						{
							//information switch for active Boiler only when we have a Thermostat
							// Valves don't have Boiler status
							int ChildID = 9;
							std::string bName = moduleName + " - Boiler Status";
							SendGeneralSwitch(crcId, ChildID, batteryLevel, bIsActive, bIsActive, bName, m_Name, mrf_status);

							// Set option SwitchType to STYPE_Contact
							std::vector<std::vector<std::string> > result;
							result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit==%d)", m_HwdID, crcId, ChildID);

							if (!result.empty())
                                                        {
								//
								int uId = std::stoi(result[0][0]);
								int nValue = std::stoi(result[0][1]);
								std::string sValue = result[0][2];
								//Debug(DEBUG_HARDWARE, "NATherm1 uId %d", uId);
								if (m_bFirstTimeHomeStatus)
								{
                                	                                //m_sql.UpdateDeviceValue("SwitchType", STYPE_Dusk, std::to_string(uId));  //12
									m_sql.UpdateDeviceValue("SwitchType", STYPE_Contact, std::to_string(uId)); // 2
									//m_sql.UpdateDeviceValue("CustomImage", 15, std::to_string(uId));         //15 Thermometer
									m_sql.UpdateDeviceValue("CustomImage", 19, std::to_string(uId));           //19 Sun
								}
							}
						}

						//Thermostat schedule switch (actively changing thermostat schedule)
						std::string allSchName = "Off";
						std::string allSchAction = "";
						//Store Home_ID from crcID
						std::stringstream uid;
                                                uid << crcId;
						m_ScheduleHomes[crcId] = home_id;
						std::stringstream Hardware_str;
                                                Hardware_str << Hardware_int;
						m_ScheduleHomes[Hardware_int] = home_id;

						std::map<int, std::string> Schedule_Names = m_ScheduleNames[home_id];

						for (std::map<int, std::string>::const_iterator itt = Schedule_Names.begin(); itt != Schedule_Names.end(); ++itt)
						{
							allSchName = allSchName + "|" + itt->second;
							std::stringstream ss;
							ss << itt->first;
							//
						}

						//Json::Value json_data = m_Schedule_Names[home_id];
						int index = 10;
						//Debug(DEBUG_HARDWARE, "allSchName Data %s", allSchName.c_str());

						//Selected Index for the dropdown list
						std::stringstream ssv;
						ssv << m_selected_Schedule[home_id];

						//create update / domoticz device
						SendSelectorSwitch(Hardware_int, 2, ssv.str(), moduleName + " - Schedule", 15, true, allSchName, allSchAction, true, m_Name);   // No RF-level - Battery level visible

						std::string sName = moduleName + " - mode";
						SendSelectorSwitch(crcId, NETATMO_PRESET_UNIT, setpoint_mode_str, sName, 15, true, "Off|On|Away|Frost Guard", "", true, m_Name);   // No RF-level - Battery level visible

						m_thermostatModuleID[crcId] = module_id;                // mac-adres
						m_DeviceHomeID[roomNetatmoID] = home_id;              // Home_ID
					}
					if (type == "NRV")
					{
						//Debug(DEBUG_HARDWARE, "NRV");
						int ChildID = NETATMO_PRESET_UNIT;
						std::vector<std::vector<std::string> > result;
						result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit==%d)", m_HwdID, crcId, ChildID);

						if (!result.empty())
						{
							int uId = std::stoi(result[0][0]);
							int nValue = std::stoi(result[0][1]);
							std::string sValue = result[0][2];
							//Debug(DEBUG_HARDWARE, "NRV uId %d", uId);
							m_sql.UpdateDeviceValue("CustomImage", 36, std::to_string(uId));           //36
                                                }

					}

					if (type == "NLP" || type == "NLC" || type == "NLPD" || type == "NLPO" || type == "NLPM" || type == "NLPC" || type == "NLPT" || type == "NLPS" || type == "BNCS" || type == "BNXM")
					{
						std::string bName = moduleName + " - Power";
						SendGeneralSwitch(crcId, 0, batteryLevel, ionflag, swlevel, bName, m_Name, mrf_status);

						std::string cName = moduleName + " - Kwh";
						//calculation mTotal kWh - TODO
						double mTotal = 0;
						SendKwhMeter(crcId, 5, batteryLevel, powerflag, mTotal, cName, mrf_status);
						m_PowerDeviceID[crcId] = bName;
					}
					if (type == "NLF" || type == "NLM" || type == "NLFN" || type == "NLIS" || type == "NLL" || type == "NLFE" || type == "NLD" || type == "Z3L" || type == "BNLD" || type == "BNIL" || type == "BN3L")
					{
						std::string bName = moduleName + " - Switch";
						int ChildID = 0;
						SendGeneralSwitch(crcId, ChildID, batteryLevel, ionflag, swlevel, bName, m_Name, mrf_status);

						// Set option SwitchType to STYPE_Dimmer only if we have "brightness"
						if (!module["brightness"].empty())
						{
							m_LightDeviceID[crcId] = bName;
							std::vector<std::vector<std::string> > result;
							result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit==%d)", m_HwdID, crcId, ChildID);

							if (!result.empty())
                                                        {
								int uId = std::stoi(result[0][0]);
								int nValue = std::stoi(result[0][1]);
								std::string sValue = result[0][2];

								if (m_bFirstTimeHomeStatus)
								{
                                                                	m_sql.UpdateDeviceValue("SwitchType", STYPE_Dimmer, std::to_string(uId)); // 7
									//m_sql.UpdateDeviceValue("CustomImage", 7, std::to_string(uId));         // 7
								}
                                                        }

						}
						else
						{
							m_PowerDeviceID[crcId] = bName;
						}
					}
					if (type == "NLLF")
					{
						std::string bName = moduleName + " - Fan";
						int ChildID = 0;
						int Image = 7;
						bool bDropdown = true;
						bool bHideOff = true;
						std::string Selector;
						//SendGeneralSwitch(crcId, ChildID, batteryLevel, 1, fan_speed, bName, m_Name, mrf_status);
						//Fan is preset with 2 speeds
						if (fan_speed == 0)
						{
							nValue = 0;
							Selector = "0";
						}
						else if (fan_speed == 1)
						{
							nValue = 10;
							Selector = "10";
						}
						else if (fan_speed == 2)
						{
							nValue = 20;
							Selector = "20";
						}
						SendSelectorSwitch(crcId, NETATMO_PRESET_UNIT, Selector, bName, Image, bDropdown, "Off|Low Speed|High Speed", "", bHideOff, m_Name);   // No RF-level - Battery level visible

						SendFanSensor(crcId, batteryLevel, fan_speed, bName);

						std::string cName = moduleName + " - Kwh";
						// Calculation mTotal kWh - TODO
						double mTotal = 0;
						SendKwhMeter(crcId, 5, batteryLevel, powerflag, mTotal, cName, mrf_status);

						m_PowerDeviceID[crcId] = bName;
						std::vector<std::vector<std::string> > result;
						result = m_sql.safe_query("SELECT ID, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit==%d)", m_HwdID, crcId, NETATMO_PRESET_UNIT);

						if (!result.empty())
                                                {
							int uId = std::stoi(result[0][0]);
							Log(LOG_STATUS, "Fan uId %d", uId);
							if (m_bFirstTimeHomeStatus)
							{
								//m_sql.UpdateDeviceValue("SwitchType", STYPE_Contact, std::to_string(uId)); // 2
								m_sql.UpdateDeviceValue("CustomImage", 7, std::to_string(uId));              // 7
							}
						}

					}
					if ((type == "NLV") || (type == "BNAS") || (type == "NLLV") || (type == "NLIV") || (type == "Z3V"))
					{
						int ChildID = 15;
						int level = current_position;
						int Command = target_position;
						//target_step;
						Debug(DEBUG_HARDWARE, "SendBlindSensor (%d) %d %d command %d %d %s %s %d", crcId, ChildID, batteryLevel, Command, level, moduleName.c_str(), m_Name.c_str(), mrf_status);
						bool bDeviceUsed = true;
						bool bReversePosition = false;
						bool bReverseState = false;
						CreateBlindSwitch(crcId, ChildID, STYPE_BlindsPercentage, bDeviceUsed, bReversePosition, bReverseState, Command, level, moduleName.c_str(), m_Name.c_str(), batteryLevel, mrf_status);
						m_PowerDeviceID[crcId] = moduleName;
					}
					if (type == "NLE")
					{
						continue;
					}
					if ((type == "NACamera") || (type == "NCO") || (type == "NDB") || (type == "NOC") || (type == "NSD") || (type == "NIS") || (type == "NDL"))
					{
						//Only use Get Events when correct device is presend
						m_bPollGetEvents = true;
					}
					Debug(DEBUG_HARDWARE, "Poll Get Events (%d)", m_bPollGetEvents);
					//m_bPollGetEvents = false;  // Possible to Block GetEvents
				}
			m_netatmo_devices.push_back(nDevice);
			}
		}
	}

	//Parse Persons
	Log(LOG_STATUS, "Parse Persons");
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
				//Find the Person name
				PersonName = m_PersonsNames[PersonNetatmoID];

				//SaveJson2Disk(person, std::string("./person_") + PersonName.c_str() + ".txt");

				std::string PersonLastSeen = person["last_seen"].asString();
				std::string PersonAway = person["out_of_sight"].asString();
			}
		}
	}
	Log(LOG_STATUS, "HomeStatus parsed");
	return true;
}


/// <summary>
/// Parse data for energy/security devices
/// get and create/update domoticz devices
/// </summary>
/// <param name="sResult">JSON raw data to parse</param>
/// <param name="root">JSON object to read</param>
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
		std::string str_id;
		std::string events_Message;
		std::string events_subevents_type;
		std::string e_Name;
		int Hardware_int;
		char const* pchar_ID = 0;
		int crcId = 0;
		int batteryLevel = 255;
		int RF_status = 12;
		int alertLevel = 4;
		int nValue = 0;

		for (auto events : mRoot)
		{
			if (!events["id"].empty())
			// Domoticz Device for Events ? / Camera's ?
			{
				events_ID = events["id"].asString();
				//SaveJson2Disk(events, std::string("./events_") + events_ID.c_str() + ".txt");
			}
			// Using Textstatus / Alert for now
			if (!events["id"].empty())
			{
				events_ID = events["id"].asString();
			}
			if (!events["type"].empty())
			{
				events_Type = events["type"].asString();
			}
			if (!events["time"].empty())
			{
				bool events_Time = events["time"].asBool();
				//
			}
			if (!events["module_id"].empty())
			{
				events_Module_ID = events["module_id"].asString();
				uint64_t Hardware_convert = convert_mac(events_Module_ID);
				Hardware_int = (int)Hardware_convert;
				//Debug(DEBUG_HARDWARE, " %d -  %s ", Hardware_int, events_Module_ID.c_str());
				e_Name = m_ModuleNames[events_Module_ID] + " - events";
				//converting ID to char const
				crcId = Crc32(0, (const unsigned char*)events_Module_ID.c_str(), events_Module_ID.length());
			}
			if (!events["message"].empty())
			{
				events_Message = events["message"].asString();
			}
			if (!events["video_id"].empty())
			{
				std::string events_Video_ID = events["video_id"].asString();
			}
			if (!events["video_status"].empty())
			{
				std::string events_Video_Status = events["video_status"].asString();
			}
			if (!events["snapshot"].empty())
			{
				//events_Snapshot = events["snapshot"];
//				std::string events_Snapshot_url = events["snapshot"]["url"];
//				std::string events_Snapshot_exp = events["snapshot"]["expires_at"];
			}
			if (!events["vignette"].empty())
			{
				//events_Vignette = events["vignette"];
//				std::string events_Vignette_url = events["vignette"]["url"];
//				std::string events_Vignette_exp = events["vignette"]["expires_at"];
			}
			if (!events["subevents"].empty())
			{
				for (auto sub_events : events["subevents"])
				{
					if (!sub_events["id"].empty())
					{
						std::string events_subevents_ID = sub_events["id"].asString();
					}
					if (!sub_events["type"].empty())
					{
						events_subevents_type = sub_events["type"].asString();
					}
					if (!sub_events["time"].empty())
					{
						bool events_subevents_time = sub_events["time"].asBool();
					}
					if (!sub_events["verified"].empty())
					{
						std::string events_subevents_verified = sub_events["verified"].asString(); // true or false
					}
					if (!sub_events["offset"].empty())
					{
						int events_subevents_offset = sub_events["offset"].asInt();
					}
					if (!sub_events["snapshot"].empty())
					{
						//events_subevents_ID = sub_events["snapshot"];
//						std::string events_subevents_Snapshot = sub_events["snapshot"]["url"].asString();
//						std::string events_subevents_Snapshot = sub_events["snapshot"]["expires_at"].asString();
					}
					if (!sub_events["vignette"].empty())
					{
						//events_subevents_Vignette = sub_events["vignette"];
//						std::string events_subevents_Vignette = sub_events["vignette"]["url"].asString();
//						std::string events_subevents_Vignette = sub_events["vignette"]["expires_at"].asString();
					}
					if (!sub_events["message"].empty())
					{
						std::string events_subevents_Message = sub_events["message"].asString();
						events_Message = events_subevents_Message;
					}
				}
			}
			if (!events["sub_type"].empty())
			{
				bool events_sub_type = events["sub_type"].asBool();
				events_Type = events["sub_type"].asString();
			}
			if (!events["persons"].empty())
			{
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
				}
			}
			if (!events["person_id"].empty())
			{
				std::string events_person_id = events["person_id"].asString();
			}
			if (!events["out_of_sight"].empty())
			{
				bool events_sub_type = events["out_of_sight"].asBool(); // true or false
			}
			if (!events_Message.empty())
			{
				event_Text = events_Message + " - " + events_Type;
				std::string sValue = event_Text.c_str();
				SendAlertSensor(crcId, batteryLevel, alertLevel, sValue.c_str(), e_Name.c_str());
			}
		}
	}
	return true;
}

/// <summary>
/// Parse Scenarios
/// <param name="home_id">ID-number of the NetatmoHome</param>
/// </summary>
bool CNetatmo::ParseScenarios(const std::string& sResult, Json::Value& scenarios, std::string home_id)
{
	//Locals
	std::string scenario_id;
	std::string scenario_category;
	std::string scenario_type;
	std::string id_mod;
	std::string scenario_index;
	std::string scenario_SchName;
	bool status_mod;
	bool target_position;
	std::map<int, std::string> _data;
	int index = 0;
	if (!scenarios.isObject())
		return false;

	//Json::Value root;
	//std::string File = ReadFile("./jsoncreated/scenario.txt");
	//bool ret = ParseJSon(File, scenarios);
	if (scenarios.isObject())
	{
		if (!scenarios["id"].empty())
		{
			scenario_id = scenarios["id"].asString();
			//Debug(DEBUG_HARDWARE, "Get the scenarios from Home %s", home_id.c_str());

		}
		if (!scenarios["modules"].empty())
		{
			Log(LOG_STATUS, "Get the scenarios modules from %s in home %s", scenario_id.c_str(), home_id.c_str());
			for (auto moduless : scenarios["modules"])
			{
				if (!moduless["id"].empty())
				{
					id_mod = moduless["id"].asString();
					//Debug(DEBUG_HARDWARE, "Scenarios Module id %s", id_mod.c_str());
				}
				if (!moduless["scenarios"].empty())
				{
					for (auto scenarios_mod : moduless["scenarios"])
					{
						if (!scenarios_mod["id"].empty())
						{
							scenario_index = scenarios_mod["id"].asString();
							//Debug(DEBUG_HARDWARE, "Scenarios index %s", scenario_index.c_str());
						}
						if (!scenarios_mod["on"].empty())
						{
							status_mod = scenarios_mod["on"].asBool();
						}
						if (!scenarios_mod["target_position"].empty())
						{
							target_position = scenarios_mod["target_position"].asBool();
						}
					}
				}

			}
		}
		if (!scenarios["scenarios"].empty())
		{
			for (auto scenarioss : scenarios["scenarios"])
			{
				std::string scenario_id;
				std::string scenario_name;
				bool scenario_custom;
				bool scenario_edit;
				bool scenario_del;

				if (scenarioss["category"].empty())
				{
					scenario_category = scenarioss["category"].asString();
					//Debug(DEBUG_HARDWARE, "Scenarios category %s", scenario_category.c_str());
				}
				if (!scenarioss["customizable"].empty())
				{
					scenario_custom = scenarioss["customizable"].asBool();
				}
				if (!scenarioss["deletable"].empty())
				{
					scenario_del = scenarioss["deletable"].asBool();
				}
				if (!scenarioss["editable"].empty())
				{
					scenario_edit = scenarioss["editable"].asBool();
				}
				if (!scenarioss["id"].empty())
				{
					scenario_id = scenarioss["id"].asString();
				}
				if (!scenarioss["name"].empty())
				{
					scenario_name = scenarioss["name"].asString();
				}
				if (!scenarioss["type"].empty())
				{
					if (!scenario_name.empty())
					{
						_data[index] = scenario_name;
						scenario_SchName = scenario_SchName + scenario_name + "|";
					}
					else
					{
						scenario_type = scenarioss["type"].asString();
						scenario_SchName = scenario_SchName + scenario_type + "|";
						_data[index] = scenario_type;
					}
					//Debug(DEBUG_HARDWARE, "Scenario %s : %s %s %s", scenario_id.c_str(), scenario_name.c_str(), scenario_type.c_str(), scenario_category.c_str());
				}
				index += 10;
			}
			std::stringstream ss;
			ss << "\n";
			for (const auto& pair : _data)
			{
				ss << pair.first << ": " << pair.second << "\n";
			}
			Debug(DEBUG_HARDWARE, "Scenario %s", ss.str().c_str());
			m_Scenarios[home_id] = _data;
			if (scenario_SchName.size() > 0)  scenario_SchName.resize(scenario_SchName.size() - 1); 
			m_ModuleNames["999"] = scenario_SchName;
		}

		if (!scenario_type.empty())
		{
			Log(LOG_STATUS, "Scenarios Selector Switch");
			std::string lName = "Scenario";
			bool bIsActive = 0;
			int ChildID = 14;
			int Image = 0;
			bool bDropdown = true;
			bool bHideOff = false;
			int crcId = Crc32(0, (const unsigned char*)home_id.c_str(), home_id.length());
			std::stringstream uid;
			uid << crcId;
			std::string Selector;
			Selector = m_selectedScenario[home_id]; //Active selecting
			m_ScheduleHomes[crcId] = home_id;
			m_PowerDeviceID[crcId] = lName;
			m_DeviceModuleID[crcId] = home_id;
			m_Device_types[home_id] = "NLG";
			SendSelectorSwitch(crcId, ChildID, Selector, lName, Image, bDropdown, scenario_SchName, "", bHideOff, m_Name);   // No RF-level - Battery level
		}
		else
			return false;
	}
	return true;
}
