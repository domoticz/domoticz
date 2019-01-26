// Nest OAuth API
//
// This plugin uses the proper public Nest Developer API as
// opposed to the old plugin which used the mobile interface API.

#include "stdafx.h"
#include "NestOAuthAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"
#include "../webserver/Base64.h"


#define round(a) ( int ) ( a + .5 )

// Base URL of API including trailing slash
const std::string NEST_OAUTHAPI_BASE = "https://developer-api.nest.com/";
const std::string NEST_OAUTHAPI_OAUTH_ACCESSTOKENURL = "https://api.home.nest.com/oauth2/access_token";

CNestOAuthAPI::CNestOAuthAPI(const int ID, const std::string &apikey, const std::string &extradata) : m_OAuthApiAccessToken(apikey)
{
	m_HwdID = ID;

	// get the data from the extradata field
	std::vector<std::string> strextra;
	StringSplit(extradata, "|", strextra);
	if (strextra.size() == 3)
	{
		m_ProductId = base64_decode(strextra[0]);
		m_ProductSecret = base64_decode(strextra[1]);
		m_PinCode = base64_decode(strextra[2]);
	}

	Init();
}

CNestOAuthAPI::~CNestOAuthAPI(void)
{
}

void CNestOAuthAPI::Init()
{
	m_bDoLogin = true;
}

bool CNestOAuthAPI::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CNestOAuthAPI::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CNestOAuthAPI::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
    return true;
}

#define NEST_POLL_INTERVAL 60

void CNestOAuthAPI::Do_Work()
{
	_log.Log(LOG_STATUS,"NestOAuthAPI: Worker started...");
	int sec_counter = NEST_POLL_INTERVAL-5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % NEST_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	Logout();
	_log.Log(LOG_STATUS,"NestOAuthAPI: Worker stopped...");
}

void CNestOAuthAPI::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), 255);
}

// Creates and updates switch used to log Heating and/or Cooling.
void CNestOAuthAPI::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
		m_HwdID, pTypeLighting2, sTypeAC, szIdx);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
			return;
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = 1;
	int level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

bool CNestOAuthAPI::ValidateNestApiAccessToken(const std::string &accesstoken) {
	std::string sResult;

	// Let's get a list of structures to see if the supplied Access Token works
	try {
		std::string sURL = NEST_OAUTHAPI_BASE + "structures.json?auth=" + m_OAuthApiAccessToken;
		_log.Log(LOG_NORM, "NestOAuthAPI: Trying to access API on " + sURL);
		std::vector<std::string> ExtraHeaders;
		std::vector<std::string> vHeaderData;

		HTTPClient::GET(sURL, ExtraHeaders, sResult, vHeaderData, false);
		if (sResult.empty()) {
			std::string sErrorMsg = "Got empty response body while getting structures. ";
			if (!vHeaderData.empty()) {
				sErrorMsg += "Response code: " + vHeaderData[0];
			}
			throw std::runtime_error(sErrorMsg.c_str());
		}
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error while performing login: " + what);
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Failed to parse received JSON data.");
		return false;
	}
	if (root.empty())
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: JSON data parsed but resulted in no nodes.");
		return false;
	}

	std::vector<std::string> vRootMembers = root.getMemberNames();
	Json::Value oFirstElement = root.get(vRootMembers[0], "");

	if (oFirstElement["name"].empty())
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Did not get name for first structure!");
		return false;
	}
	// _log.Log(LOG_NORM, ("NestOAuthAPI: Got first structure name: "+oFirstElement["name"].asString()).c_str());

	_log.Log(LOG_NORM, "NestOAuthAPI: Access token appears to be valid.");
	return true;
}

bool CNestOAuthAPI::Login()
{
	// If we don't have an access token available
	if (m_OAuthApiAccessToken.empty())
	{
		_log.Log(LOG_NORM, "NestOAuthAPI: No API key available.");
		// Check if we do have a productid, secret and pin code
		if (!m_ProductId.empty() && !m_ProductSecret.empty() && !m_PinCode.empty()) {
			_log.Log(LOG_NORM, "NestOAuthAPI: Access token missing. Will request an API key based on Product Id, Product Secret and PIN code.");

			std::string sTmpToken = "";
			try
			{
				// Request the token using the information that we already have.
				sTmpToken = FetchNestApiAccessToken(m_ProductId, m_ProductSecret, m_PinCode);

				if (!sTmpToken.empty()) {
					_log.Log(LOG_NORM, "NestOAuthAPI: Received an access token to use for future requests: " + sTmpToken);

					// Store the access token in the database and set the application to use it.
					SetOAuthAccessToken(m_HwdID, sTmpToken);
				}
				else {
					throw std::runtime_error("Received access token does not appear to be valid.");
				}
			}
			catch (std::exception& e)
			{
				// Apparently something went wrong fetching the access token.
				std::string what = e.what();
				_log.Log(LOG_ERROR, "NestOAuthAPI: Error retrieving access token: " + what);
				return false;
			}
		}
		else _log.Log(LOG_NORM, "NestOAuthAPI: Access token missing. Will not attempt to request one using Product Id, Product Secret and PIN code since at least one of these is empty.");
	}

	// Check if we still don't have an access token available
	if (m_OAuthApiAccessToken.empty())
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Cannot login: access token was not supplied and failed to fetch one.");
		Logout();

		// Clear the retrieved tokens and secrets so we won't be
		// hammering the Nest API with logins which don't work anyway.
		m_ProductId = "";
		m_ProductSecret = "";
		m_PinCode = "";

		return false;
	}

	if (ValidateNestApiAccessToken(m_OAuthApiAccessToken))
	{
		_log.Log(LOG_NORM, "NestOAuthAPI: Login success. Token successfully validated.");
		m_bDoLogin = false;
		return true;
	}
	else {
		_log.Log(LOG_ERROR, "NestOAuthAPI: Login failed: token did not validate.");
		return false;
	}
}

void CNestOAuthAPI::Logout()
{
	if (m_bDoLogin)
		return; //we are not logged in
	m_bDoLogin = true;
}

bool CNestOAuthAPI::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (m_OAuthApiAccessToken.empty())
		return false;

	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false; //later add RGB support, if someone can provide access

	int node_id = pCmd->LIGHTING2.id4;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);

	if ((node_id - 3) % 3 == 0)
	{
		//Away
		return SetAway(node_id, bIsOn);
	}

	if ((node_id - 4) % 3 == 0)
	{
		// Manual Eco Mode
		return SetManualEcoMode(node_id, bIsOn);
	}

	return false;
}

void CNestOAuthAPI::UpdateSmokeSensor(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	bool bDeviceExists = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, Idx, 0);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szIdx);
	if (result.empty())
	{
		bDeviceExists = false;
	}
	else
	{
		//check if we have a change, if not only update the LastUpdate field
		bool bNoChange = false;
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			bNoChange = true;
		else if ((bOn && (nvalue != 0)))
			bNoChange = true;
		if (bNoChange)
		{
			time_t now = time(0);
			struct tm ltime;
			localtime_r(&now, &ltime);

			char szLastUpdate[40];
			sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

			m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%q' WHERE(HardwareID == %d) AND (DeviceID == '%q')",
				szLastUpdate, m_HwdID, szIdx);
			return;
		}
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = Idx;
	lcmd.LIGHTING2.id4 = 0;
	lcmd.LIGHTING2.unitcode = 1;
	int level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;

	if (!bDeviceExists)
	{
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
		//Assign default name for device
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')", defaultname.c_str(), m_HwdID, szIdx);
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szIdx);
		if (!result.empty())
		{
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID=='%q')", STYPE_SMOKEDETECTOR, result[0][0].c_str());
		}
	}
	else
		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

void CNestOAuthAPI::GetMeterDetails()
{
	std::string sResult;
	if (m_bDoLogin)
	{
		if (!Login())
		return;
	}

	Json::Value deviceRoot;
	Json::Value structureRoot;
	Json::Reader jReader;

	std::vector<std::string> ExtraHeaders;
	std::string sURL;
	bool bRet;

	// Get Data for structures
	sURL = NEST_OAUTHAPI_BASE + "structures.json?auth=" + m_OAuthApiAccessToken;

	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error getting structures!");
		m_bDoLogin = true;
		return;
	}

	bRet = jReader.parse(sResult, structureRoot);
	if ((!bRet) || (!structureRoot.isObject()))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Invalid structures data received!");
		m_bDoLogin = true;
		return;
	}


	//Get Data for devices
	sURL = NEST_OAUTHAPI_BASE + "devices.json?auth=" + m_OAuthApiAccessToken;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error getting devices!");
		m_bDoLogin = true;
		return;
	}

	bRet = jReader.parse(sResult, deviceRoot);
	if ((!bRet) || (!deviceRoot.isObject()))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Invalid devices data received!");
		m_bDoLogin = true;
		return;
	}
	bool bHaveThermostats = !deviceRoot["thermostats"].empty();
	bool bHaveSmokeDetects = !deviceRoot["smoke_co_alarms"].empty();

	if ((!bHaveThermostats) && (!bHaveSmokeDetects))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: request not successful 1 (no thermostat or protect was received), restarting..!");
		m_bDoLogin = true;
		return;
	}

	//Protect
	if (bHaveSmokeDetects)
	{
		if (deviceRoot["smoke_co_alarms"].empty())
		{
			_log.Log(LOG_ERROR, "NestOAuthAPI: smoke detectors request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
		Json::Value::Members members = deviceRoot["smoke_co_alarms"].getMemberNames();
		if (members.empty())
		{
			_log.Log(LOG_ERROR, "NestOAuthAPI: smoke detectors request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
		int SwitchIndex = 1;
		for (Json::Value::iterator itDevice = deviceRoot["smoke_co_alarms"].begin(); itDevice != deviceRoot["smoke_co_alarms"].end(); ++itDevice)
		{
			Json::Value device = *itDevice;
			//std::string devstring = itDevice.key().asString();
			std::string devWhereName = device["where_name"].asString();

			if (devWhereName.empty())
				continue;

			std::string devName = devWhereName;

			// Default value is true, let's assume the worst.
			bool bSmokeAlarm = true;
			bool bCOAlarm = true;

			std::string sSmokeAlarmState = device["smoke_alarm_state"].asString();
			std::string sCOAlarmState = device["co_alarm_state"].asString();

			// Rookmelder
			if (!sSmokeAlarmState.empty())
			{
				if (sSmokeAlarmState == "ok")
				{
					// Smoke alarm state is ok, lets scale it down.
					bSmokeAlarm = false;
				}
			}

			// CO2 melder
			if (!sCOAlarmState.empty())
			{
				if (sCOAlarmState == "ok")
				{
					bCOAlarm = false;
				}
			}

			UpdateSmokeSensor(SwitchIndex, bSmokeAlarm, devName + " Smoke Alarm");
			UpdateSmokeSensor(SwitchIndex+1, bCOAlarm, devName + " CO Alarm");

			SwitchIndex = SwitchIndex + 2;
		}
	}

	//Thermostat
	if (!bHaveThermostats)
		return;
	if (deviceRoot["thermostats"].empty())
	{
		// If we do have a protect then just return
		if (bHaveSmokeDetects)
			return;

		_log.Log(LOG_ERROR, "NestOAuthAPI: request not successful 2 (no thermostat or protect was received), restarting..!");
		m_bDoLogin = true;
		return;
	}

	size_t iThermostat = 0;
	size_t iStructure = 0;
	for (Json::Value::iterator ittStructure = structureRoot.begin(); ittStructure != structureRoot.end(); ++ittStructure)
	{
		// Get general structure information
		Json::Value nstructure = *ittStructure;
		if (!nstructure.isObject()) continue;

		// Store the structure information in a map.
		_tNestStructure nstruct;
		nstruct.Name = nstructure["name"].asString();
		nstruct.StructureId = nstructure["structure_id"].asString();
		m_structures[iStructure] = nstruct;

		// Away is determined for a structure, not for a thermostat.
		if (!nstructure["away"].empty())
		{
			bool bIsAway = (nstructure["away"].asString() == "away" || nstructure["away"].asString() == "auto-away");
			SendSwitch((iStructure * 3) + 3, 1, 255, bIsAway, 0, nstruct.Name + " Away");
		}

		// Find out which thermostats are available under this structure
		for (Json::Value::iterator ittDevice = nstructure["thermostats"].begin(); ittDevice != nstructure["thermostats"].end(); ++ittDevice)
		{
			std::string devID = (*ittDevice).asString();

			// _log.Log(LOG_NORM, ("Nest: Found Thermostat " + devID + " in structure " + StructureName).c_str());

			// Get some more information for this thermostat
			Json::Value ndevice = deviceRoot["thermostats"][devID];
			if (!ndevice.isObject())
			{
				_log.Log(LOG_ERROR, "NestOAuthAPI: Structure referenced thermostat %s but it was not found.", devID.c_str());
				continue;
			}

			std::string Name = "Thermostat";
			std::string WhereName = ndevice["where_name"].asString();
			if (!WhereName.empty())
			{
				Name = nstruct.Name + " " + WhereName;
			}

			_tNestThemostat ntherm;
			ntherm.StructureID = nstruct.StructureId;
			ntherm.Name = Name;
			ntherm.Serial = devID;
			ntherm.CanCool = ndevice["can_cool"].asBool();
			ntherm.CanHeat = ndevice["can_heat"].asBool();

			m_thermostats[iThermostat] = ntherm;

			// Find out if we're using C or F.
			std::string temperatureScale = ndevice["temperature_scale"].asString();
			boost::to_lower(temperatureScale);

			std::string sHvacMode = ndevice["hvac_mode"].asString();
			std::string sHvacState = ndevice["hvac_state"].asString();

			//Setpoint
			if (!ndevice["target_temperature_"+temperatureScale].empty())
			{
				float currentSetpoint = ndevice["target_temperature_"+temperatureScale].asFloat();
				SendSetPointSensor((const unsigned char)(iThermostat * 3) + 1, currentSetpoint, Name + " Setpoint");
			}
			//Room Temperature/Humidity
			if (!ndevice["ambient_temperature_"+temperatureScale].empty())
			{
				float currentTemp = ndevice["ambient_temperature_"+temperatureScale].asFloat();
				int Humidity = ndevice["humidity"].asInt();
				SendTempHumSensor((iThermostat * 3) + 2, 255, currentTemp, Humidity, Name + " TempHum");
			}

			// Check if thermostat is currently Heating
			if (ntherm.CanHeat && !sHvacState.empty())
			{
				bool bIsHeating = (sHvacState == "heating");
				UpdateSwitch((unsigned char)(113 + (iThermostat * 3)), bIsHeating, Name + " HeatingOn");
			}

			// Check if thermostat is currently Cooling
			if (ntherm.CanCool && !sHvacState.empty())
			{
				bool bIsCooling = (sHvacState == "cooling");
				UpdateSwitch((unsigned char)(114 + (iThermostat * 3)), bIsCooling, Name + " CoolingOn");
			}

			// Indicates HVAC system heating/cooling modes, like Heat/Cool for systems with heating and cooling capacity, or Eco Temperatures for energy savings.
			// "heat", "cool", "heat-cool", "eco", "off"

			bool bManualEcomodeEnabled = (!sHvacMode.empty() && sHvacMode == "off");

			// UpdateSwitch(115 + (iThermostat * 3), bManualEcomodeEnabled, Name + " Manual Eco Mode");
			SendSwitch((iThermostat * 3) + 4, 1, 255, bManualEcomodeEnabled, 0, Name + " Manual Eco Mode");

			iThermostat++;
		}
		iStructure++;
	}
}

void CNestOAuthAPI::SetSetpoint(const int idx, const float temp)
{
	if (m_OAuthApiAccessToken.empty())
		return;

	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}

	size_t iThermostat = (idx - 1) / 3;
	if (iThermostat > m_thermostats.size())
		return;

	if (m_thermostats[iThermostat].Serial.empty())
	{
		_log.Log(LOG_NORM, "NestOAuthAPI: Thermostat has not been initialized yet. Try again later.");
		return;
	}

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization:Bearer " + m_OAuthApiAccessToken);
	ExtraHeaders.push_back("Content-Type:application/json");
	float tempDest = temp;

	unsigned char tSign = m_sql.m_tempsign[0];

	// Find out if we're using C or F.
	std::string temperatureScale(1, m_sql.m_tempsign[0]);
	boost::to_lower(temperatureScale);

	Json::Value root;
	root["target_temperature_" + temperatureScale] = tempDest;

	std::string sResult;

	std::string sURL = NEST_OAUTHAPI_BASE + "devices/thermostats/" + m_thermostats[iThermostat].Serial;
	if (!HTTPClient::PUT(sURL, root.toStyledString(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error setting setpoint!");
		m_bDoLogin = true;
		return;
	}

	GetMeterDetails();
}

bool CNestOAuthAPI::SetManualEcoMode(const unsigned char node_id, const bool bIsOn)
{
	// Determine the index for the thermostat.
	size_t iThermostat = (node_id - 4) / 3;

	// Check if we even got that many thermostats.
	if (iThermostat > m_thermostats.size()) return false;

	// Grab a reference to that thermostat.
	_tNestThemostat thermostat = m_thermostats[iThermostat];

	try
	{
		if (thermostat.Serial.empty())
		{
			_log.Log(LOG_ERROR, "NestOAuthAPI: Thermostat " + std::to_string(iThermostat) + " has not been initialized yet. Try again later.");
			return false;
		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Failed to get thermostat serial (for now). Trying again later..(%s)",e.what());
		return false;
	}

	// Figure out if the thermostat can heat, cool or both
	// We can't just set heat-cool since that only works on devices that can do both.
	// If it can heat-cool, then do that, else set to whatever thing it can do.
	Json::Value root;
	std::string sNewHvacMode;
	if (thermostat.CanCool && thermostat.CanHeat) sNewHvacMode = "heat-cool";
	else if (thermostat.CanCool) sNewHvacMode = "cool";
	else if (thermostat.CanHeat) sNewHvacMode = "heat";
	root["hvac_mode"] = (bIsOn ? "off" : sNewHvacMode);

	std::string sResult;
	std::string sURL = NEST_OAUTHAPI_BASE + "devices/thermostats/" + m_thermostats[iThermostat].Serial;

	if (!PushToNestApi("PUT", sURL, root, sResult))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error setting manual eco mode!");
		// Failure, request a fresh login.
		m_bDoLogin = true;
		return false;
	}
	return true;
}

bool CNestOAuthAPI::PushToNestApi(const std::string &sMethod, const std::string &sUrl, const Json::Value &jPostData, std::string &sResult)
{
	if (m_OAuthApiAccessToken.empty())
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Failed to push information to Nest Api: No access token supplied.");
		return false;
	}

	if (m_bDoLogin == true)
	{
		if (!Login())
			return false;
	}

	// Prepare the authentication headers
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization:Bearer " + m_OAuthApiAccessToken);
	ExtraHeaders.push_back("Content-Type: application/json");

	std::string sPostData = jPostData.toStyledString();

	if (!HTTPClient::PUT(sUrl, sPostData, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error pushing information to Nest Api.");
		return false;
	}
	return true;
}

bool CNestOAuthAPI::SetAway(const unsigned char Idx, const bool bIsAway)
{
	size_t iStructure = (Idx - 3) / 3;
	if (iStructure > m_structures.size())
		return false;

	if (m_structures[iStructure].StructureId.empty())
	{
		_log.Log(LOG_NORM, "NestOAuthAPI: Structure "+std::to_string(iStructure)+" has not been initialized yet. Try again later.");
		return false;
	}

	Json::Value root;
	root["away"] = (bIsAway ? "away" : "home");

	std::string sResult;

	std::string sURL = NEST_OAUTHAPI_BASE + "structures" + "/" + m_structures[iStructure].StructureId;
	if (!PushToNestApi("PUT", sURL, root, sResult))
	{
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error setting away mode!");
		m_bDoLogin = true;
		return false;
	}
	return true;
}

void CNestOAuthAPI::SetProgramState(const int newState)
{
	if (m_OAuthApiAccessToken.empty())
		return;

	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}
}

std::string CNestOAuthAPI::FetchNestApiAccessToken(const std::string &productid, const std::string &secret, const std::string &pincode) {
	// Check if there isn't already an access token
	if (!m_OAuthApiAccessToken.empty()) {
		_log.Log(LOG_ERROR, "NestOAuthAPI: There is already an API access token configured for this hardware ID: no need to fetch a new token.");
		return "";
	}

	// Check if we have a productid, secret and pin code
	if (productid.empty() || secret.empty() || pincode.empty()) {
		_log.Log(LOG_ERROR, "NestOAuthAPI: No access token supplied and no ProductId, Secret or PIN to obtain one either.");
		return "";
	}

	_log.Log(LOG_NORM, "NestOAuthAPI: Preparing URL");

	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::ostringstream s;

	std::string sProductid = productid;
	std::string sSecret = secret;
	std::string sPincode = pincode;

	std::string sReceivedAccessToken;

	boost::trim(sProductid);
	boost::trim(sSecret);
	boost::trim(sPincode);

	s << "code=" << sPincode << "&client_id=" << sProductid << "&client_secret=" << sSecret << "&grant_type=authorization_code";
	std::string sPostData = s.str();

	_log.Debug(DEBUG_HARDWARE, "NestOAuthAPI: postdata= " + sPostData);
	_log.Log(LOG_NORM, "NestOAuthAPI: Doing POST request to URL: " + NEST_OAUTHAPI_OAUTH_ACCESSTOKENURL);

	try
	{
		_log.Log(LOG_NORM, "NestOAuthAPI: Will now attempt to fetch access token.");

		std::vector<std::string> vResponseHeaders;
		if (!HTTPClient::POST(NEST_OAUTHAPI_OAUTH_ACCESSTOKENURL, sPostData, ExtraHeaders, sResult, vResponseHeaders, true, false))
		{
			std::string sErrorMsg = "Failed to fetch token from API. ";
			if (!vResponseHeaders.empty()) {
				sErrorMsg += "Response code: " + vResponseHeaders[0];
			}
			throw std::runtime_error(sErrorMsg.c_str());
		}
		_log.Log(LOG_NORM, "NestOAuthAPI: POST request completed. Result: " + sResult);

		if (sResult.empty())
		{
			throw std::runtime_error("Received empty response from API.");
		}

		try
		{
			_log.Log(LOG_NORM, "NestOAuthAPI: Will now parse result to JSON");
			Json::Value root;
			Json::Reader jReader;
			bool bRet = jReader.parse(sResult, root);
			_log.Log(LOG_NORM, "NestOAuthAPI: JSON data parse call returned.");

			if ((!bRet) || (!root.isObject())) throw std::runtime_error("Failed to parse JSON data.");
			_log.Log(LOG_NORM, "NestOAuthAPI: Parsing of JSON data apparently successful");

			if (root.empty()) throw std::runtime_error("Parsed JSON data contains no elements.");
			_log.Log(LOG_NORM, "NestOAuthAPI: parsed JSON data contains more than 0 elements");

			_log.Log(LOG_NORM, "NestOAuthAPI: Fetching access_token from JSON object");
			sReceivedAccessToken = root["access_token"].asString();

			if (sReceivedAccessToken.empty()) throw std::runtime_error("Received an empty access token from parsed JSON data.");
			_log.Log(LOG_NORM, ("NestOAuthAPI: Returning fetched access token: " + sReceivedAccessToken));
			return sReceivedAccessToken;
		}
		catch (std::exception &e)
		{
			std::string what = e.what();
			throw "Error parsing received JSON data: "+what;
		}
	}
	catch (std::exception &e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "NestOAuthAPI: Error getting access token: "+ what);
		return "";
	}
}

bool CNestOAuthAPI::SetOAuthAccessToken(const unsigned int ID, std::string &newToken)
{
	// Set the application to use the new token.
	m_OAuthApiAccessToken = newToken;

	_log.Log(LOG_NORM, "NestOAuthAPI: Storing received access token " + newToken + " and clearing token request information.");

	// This can probably be done in a better way. For now let's just assume things will succeed.
	m_sql.safe_query("UPDATE Hardware SET Username='%q', Extra='' WHERE (ID==%d)", newToken.c_str(), ID);

	return true;
}
