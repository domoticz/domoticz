// Tado plugin for Domoticz
//
// This plugin uses the same API as the my.tado.com web interface. Unfortunately this
// API is unofficial and undocumented, but until Tado releases an official and
// documented API, it's the best we've got.
//
// Main documentation for parts of the API can be found at
// http://blog.scphillips.com/posts/2017/01/the-tado-api-v2/ but unfortunately
// this information is slightly outdated, the authentication part in particular.


#include "stdafx.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/mainworker.h"
#include "../json/json.h"
#include "../webserver/Base64.h"
#include "Tado.h"

#define TADO_POLL_INTERVAL 30		// The plugin should collect information from the API every n seconds.
#define TADO_API_ENVIRONMENT_URL "https://my.tado.com/webapp/env.js"
#define TADO_TOKEN_MAXLOOPS 12		// Default token validity is 600 seconds before it needs to be refreshed.
									// Each cycle takes 30-35 seconds, so let's stay a bit on the safe side.

CTado::~CTado(void)
{
}

CTado::CTado(const int ID, const std::string &username, const std::string &password):
m_TadoUsername(username),
m_TadoPassword(password)
{
	m_HwdID = ID;

	Init();
}

bool CTado::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CTado::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

void CTado::Init()
{
	m_bDoLogin = true;
	m_bDoGetHomes = true;
	m_bDoGetZones = false;
	m_bDoGetEnvironment = true;

	stdstring_trim(m_TadoUsername);
	stdstring_trim(m_TadoPassword);
}

bool CTado::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;

	//if (!m_bDoLogin)
	//	Logout();

	return true;
}

bool CTado::WriteToHardware(const char * pdata, const unsigned char length)
{
	if (m_TadoAuthToken.size() == 0)
		return false;

	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false;

	int node_id = pCmd->LIGHTING2.id4;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);


	int HomeIdx = node_id / 1000;
	int ZoneIdx = (node_id % 1000) / 100;
	int ServiceIdx = (node_id % 1000) % 100;

	_log.Debug(DEBUG_HARDWARE, "Tado: Node %d = home %s zone %s device %d", node_id, m_TadoHomes[HomeIdx].Name.c_str(), m_TadoHomes[HomeIdx].Zones[ZoneIdx].Name.c_str(), ServiceIdx);

	// ServiceIdx 1 = Away (Read only)
	// ServiceIdx 2 = Setpoint => should be handled in SetSetPoint
	// ServiceIdx 3 = TempHum (Read only)
	// ServiceIdx 4 = Setpoint Override
	// ServiceIdx 5 = Heating Enabled
	// ServiceIdx 6 = Heating On (Read only)
	// ServiceIdx 7 = Heating Power (Read only)

	// Cancel setpoint override.
	if (ServiceIdx == 4 && !bIsOn) return CancelOverlay(node_id);

	// Enable heating (= cancel overlay that turns off heating)
	if (ServiceIdx == 5 && bIsOn) return CancelOverlay(node_id);

	// Disable heating (= create overlay that turns off heating for an indeterminate amount of time)
	if (ServiceIdx == 5 && !bIsOn) return CreateOverlay(node_id, -1, false, "MANUAL");

	// If the writetohardware command is not handled by now, fail.
	return false;
}

// Changing the setpoint or heating mode is an overlay on the schedule.
// An overlay can end automatically (TADO_MODE, TIMER) or manually (MANUAL).
bool CTado::CreateOverlay(const int idx, const float temp, const bool heatingEnabled, const std::string &terminationType)
{
	_log.Log(LOG_NORM, "Tado: CreateOverlay() called with idx=%d, temp=%f, termination type=%s", idx, temp, terminationType.c_str());

	int HomeIdx = idx / 1000;
	int ZoneIdx = (idx % 1000) / 100;
	int ServiceIdx = (idx % 1000) % 100;

	// Check if the zone actually exists.
	if (m_TadoHomes.size() == 0 || m_TadoHomes[HomeIdx].Zones.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: no such home/zone combo found: %d/%d", HomeIdx, ZoneIdx);
		return false;
	}

	_log.Debug(DEBUG_HARDWARE, "Tado: Node %d = home %s zone %s device %d", idx, m_TadoHomes[HomeIdx].Name.c_str(), m_TadoHomes[HomeIdx].Zones[ZoneIdx].Name.c_str(), ServiceIdx);

	std::string _sUrl = m_TadoEnvironment["tgaRestApiV2Endpoint"] + "/homes/" + m_TadoHomes[HomeIdx].Id + "/zones/" + m_TadoHomes[HomeIdx].Zones[ZoneIdx].Id + "/overlay";
	std::string _sResponse;
	Json::Value _jsPostData;
	Json::Value _jsPostDataSetting;

	Json::Value _jsPostDataTermination;
	_jsPostDataSetting["type"] = "HEATING";
	_jsPostDataSetting["power"] = (heatingEnabled ? "ON" : "OFF");

	if (temp > -1)
	{
		Json::Value _jsPostDataSettingTemperature;
		_jsPostDataSettingTemperature["celsius"] = temp;
		_jsPostDataSetting["temperature"] = _jsPostDataSettingTemperature;
	}

	_jsPostData["setting"] = _jsPostDataSetting;
	_jsPostDataTermination["type"] = terminationType;
	_jsPostData["termination"] = _jsPostDataTermination;

	Json::Value _jsRoot;

	try
	{
		SendToTadoApi(Put, _sUrl, _jsPostData.toStyledString(), _sResponse, *(new std::vector<std::string>()), _jsRoot);
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: Failed to set setpoint via Api: %s", what.c_str());
		return false;
	}

	_log.Debug(DEBUG_HARDWARE, "Tado: Response: %s", _sResponse.c_str());

	// Trigger a zone refresh
	GetZoneState(HomeIdx, ZoneIdx, m_TadoHomes[HomeIdx], m_TadoHomes[HomeIdx].Zones[ZoneIdx]);

	return true;
}

void CTado::SetSetpoint(const int idx, const float temp)
{
	_log.Log(LOG_NORM, "Tado: SetSetpoint() called with idx=%d, temp=%f", idx, temp);
	CreateOverlay(idx, temp, true, "TADO_MODE");
}

// Requests an authentication token from the Tado OAuth Api.
bool CTado::GetAuthToken(std::string &authtoken, std::string &refreshtoken, const bool refreshUsingToken = false)
{
	try
	{
		if (m_TadoUsername.size() == 0 && !refreshUsingToken) throw std::runtime_error("No username specified.");
		if (m_TadoPassword.size() == 0 && !refreshUsingToken) throw std::runtime_error("No password specified.");
		if (m_bDoGetEnvironment) throw std::runtime_error("Environment not (yet) set up.");

		std::string _sUrl = m_TadoEnvironment["apiEndpoint"] + "/token";
		std::ostringstream s;
		std::string _sGrantType = (refreshUsingToken ? "refresh_token" : "password");

		s << "client_id=" << m_TadoEnvironment["clientId"] << "&grant_type=";
		s << _sGrantType << "&scope=home.user&client_secret=";
		s << m_TadoEnvironment["clientSecret"];

		if (refreshUsingToken)
		{
			s << "&refresh_token=" << refreshtoken;
		}
		else
		{
			s << "&password=" << CURLEncode::URLEncode(m_TadoPassword);
			s << "&username=" << CURLEncode::URLEncode(m_TadoUsername);
		}

		std::string sPostData = s.str();

		std::string _sResponse;
		std::vector<std::string> _vExtraHeaders;
		_vExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
		std::vector<std::string> _vResponseHeaders;

		Json::Value _jsRoot;

		try
		{
			SendToTadoApi(Post, _sUrl, sPostData, _sResponse, _vExtraHeaders, _jsRoot, true, false, false);
		}
		catch (std::exception& e)
		{
			std::string what = e.what();
			throw std::runtime_error(("Failed to get token from Api: %s", what.c_str()));
		}

		authtoken = _jsRoot["access_token"].asString();
		if (authtoken.size() == 0) throw std::runtime_error("Received token is zero length.");

		refreshtoken = _jsRoot["refresh_token"].asString();
		if (refreshtoken.size() == 0) throw std::runtime_error("Received refresh token is zero length.");

		_log.Log(LOG_STATUS, "Tado: Received access token from API.");
		_log.Log(LOG_STATUS, "Tado: Received refresh token from API.");

		return true;
	}
	catch (std::exception& e) {
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: GetAuthToken: %s", what.c_str());
		return false;
	}
}

// Gets the status information of a zone.
bool CTado::GetZoneState(const int HomeIndex, const int ZoneIndex, const _tTadoHome &home, _tTadoZone &zone)
{
	try
	{
		std::string _sUrl = m_TadoEnvironment["tgaRestApiV2Endpoint"] + "/homes/" + zone.HomeId + "/zones/" + zone.Id + "/state";
		Json::Value _jsRoot;
		std::string _sResponse;

		try
		{
			SendToTadoApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), _jsRoot);
		}
		catch (std::exception& e)
		{
			std::string what = e.what();
			throw std::runtime_error(("Failed to get information on zone '%s': %s", zone.Name.c_str(), what.c_str()));
		}

		// Zone Home/away
		//bool _bTadoAway = !(_jsRoot["tadoMode"].asString() == "HOME");
		//UpdateSwitch((unsigned char)ZoneIndex * 100 + 1, _bTadoAway, home.Name + " " + zone.Name + " Away");

		// Zone setpoint
		float _fSetpointC = 0;
		if (_jsRoot["setting"]["temperature"]["celsius"].isNumeric())
			_fSetpointC = _jsRoot["setting"]["temperature"]["celsius"].asFloat();
		if (_fSetpointC > 0) {
			SendSetPointSensor((unsigned char)ZoneIndex * 100 + 2, _fSetpointC, home.Name + " " + zone.Name + " Setpoint");
		}

		// Current zone inside temperature
		float _fCurrentTempC = 0;
		if (_jsRoot["sensorDataPoints"]["insideTemperature"]["celsius"].isNumeric())
			_fCurrentTempC = _jsRoot["sensorDataPoints"]["insideTemperature"]["celsius"].asFloat();

		// Current zone humidity
		float fCurrentHumPct = 0;
		if (_jsRoot["sensorDataPoints"]["humidity"]["percentage"].isNumeric())
			fCurrentHumPct = _jsRoot["sensorDataPoints"]["humidity"]["percentage"].asFloat();
		if (_fCurrentTempC > 0) {
			SendTempHumSensor(ZoneIndex * 100 + 3, 255, _fCurrentTempC, (int)fCurrentHumPct, home.Name + " " + zone.Name + " TempHum");
		}

		// Manual override of zone setpoint
		bool _bManualControl = false;
		if (!_jsRoot["overlay"].isNull() && _jsRoot["overlay"]["type"].asString() == "MANUAL")
		{
			_bManualControl = true;
		}
		UpdateSwitch((unsigned char)ZoneIndex * 100 + 4, _bManualControl, home.Name + " " + zone.Name + " Manual Setpoint Override");


		// Heating Enabled
		std::string _sType = _jsRoot["setting"]["type"].asString();
		std::string _sPower = _jsRoot["setting"]["power"].asString();
		bool _bHeatingEnabled = false;
		if (_sType == "HEATING" && _sPower == "ON")
			_bHeatingEnabled = true;
		UpdateSwitch((unsigned char)ZoneIndex * 100 + 5, _bHeatingEnabled, home.Name + " " + zone.Name + " Heating Enabled");

		// Heating Power percentage
		std::string _sHeatingPowerType = _jsRoot["activityDataPoints"]["heatingPower"]["type"].asString();
		int _sHeatingPowerPercentage = _jsRoot["activityDataPoints"]["heatingPower"]["percentage"].asInt();
		bool _bHeatingOn = false;
		if (_sHeatingPowerType == "PERCENTAGE" && _sHeatingPowerPercentage >= 0 && _sHeatingPowerPercentage <= 100)
		{
			_bHeatingOn = _sHeatingPowerPercentage > 0;
			UpdateSwitch((unsigned char)ZoneIndex * 100 + 6, _bHeatingOn, home.Name + " " + zone.Name + " Heating On");

			SendPercentageSensor(ZoneIndex * 100 + 7, 0, 255, (float)_sHeatingPowerPercentage, home.Name + " " + zone.Name + " Heating Power");
		}

		return true;
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: GetZoneState: %s", what.c_str());
		return false;
	}
}

bool CTado::GetHomeState(const int HomeIndex, _tTadoHome & home)
{
	try {
		std::stringstream _sstr;
		_sstr << m_TadoEnvironment["tgaRestApiV2Endpoint"] << "/homes/" << home.Id << "/state";
		std::string _sUrl = _sstr.str();
		Json::Value _jsRoot;
		std::string _sResponse;
		try
		{
			SendToTadoApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), _jsRoot);
		}
		catch (std::exception& e)
		{
			std::string what = e.what();
			throw std::runtime_error(("Failed to get state information on home '%s': %s", home.Name.c_str(), what.c_str()));
		}

		// Home/away
		bool _bTadoAway = !(_jsRoot["presence"].asString() == "HOME");
		UpdateSwitch((unsigned char)HomeIndex * 1000 + 0, _bTadoAway, home.Name + " Away");

		return true;
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: GetZoneState: %s", what.c_str());
		return false;
	}
}

void CTado::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string & defaultname)
{
	_tThermostat thermos;
	thermos.subtype = sTypeThermSetpoint;
	thermos.id1 = 0;
	thermos.id2 = 0;
	thermos.id3 = 0;
	thermos.id4 = Idx;
	thermos.dunit = 0;

	thermos.temp = Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), 255);
}

// Creates or updates on/off switches.
void CTado::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	//bool bDeviceExits = true;
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

// Removes any active overlay from a specific zone.
bool CTado::CancelOverlay(const int Idx)
{
	_log.Debug(DEBUG_HARDWARE, "Tado: CancelSetpointOverlay() called with idx=%d", Idx);

	int HomeIdx = Idx / 1000;
	int ZoneIdx = (Idx % 1000) / 100;
	//int ServiceIdx = (Idx % 1000) % 100;

	// Check if the home and zone actually exist.
	if (m_TadoHomes.size() == 0 || m_TadoHomes[HomeIdx].Zones.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: no such home/zone combo found: %d/%d", HomeIdx, ZoneIdx);
		return false;
	}

	std::stringstream _sstr;
	_sstr << m_TadoEnvironment["tgaRestApiV2Endpoint"] << "/homes/" << m_TadoHomes[HomeIdx].Id << "/zones/" << m_TadoHomes[HomeIdx].Zones[ZoneIdx].Id << "/overlay";
	std::string _sUrl = _sstr.str();
	std::string _sResponse;

	try
	{
		SendToTadoApi(Delete, _sUrl, "", _sResponse, *(new std::vector<std::string>()), *(new Json::Value), false, true, true);

	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: error cancelling the setpoint overlay: %s", what.c_str());
		return false;
	}

	// Trigger a zone refresh
	_log.Log(LOG_STATUS, "Tado: Setpoint overlay cancelled.");
	GetZoneState(HomeIdx, ZoneIdx, m_TadoHomes[HomeIdx], m_TadoHomes[HomeIdx].Zones[ZoneIdx]);

	return true;
}

void CTado::Do_Work()
{
	_log.Log(LOG_STATUS, "Tado: Worker started. Will poll every %d seconds.", TADO_POLL_INTERVAL);
	int iSecCounter = TADO_POLL_INTERVAL - 5;
	int iTokenCycleCount = 0;

	while (!IsStopRequested(1000))
	{
		iSecCounter++;
		if (iSecCounter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if (iSecCounter % TADO_POLL_INTERVAL == 0)
		{
			// Only login if we should.
			if (m_bDoLogin)
			{
				// Lets try to get authentication set up.
				// If not, try again next time.
				m_bDoLogin = false;
				if (!Login())
				{
					// Mark that we still need to log in.
					m_bDoLogin = true;
					// Not much of a point doing other actions.
					continue;
				}
			}

			// Check if we should get homes from tado account
			if (m_bDoGetHomes)
			{
				// If we fail to do so, abort.
				if (!GetHomes())
				{
					_log.Log(LOG_ERROR, "Tado: Failed to get homes from Tado account.");
					// Try to get homes next iteration. Other than that we can't do much now.
					continue;
				}
				// Else move on to getting zones for each of the homes.
				else {
					m_bDoGetZones = true;
					m_bDoGetHomes = false;
				}
			}

			// Check if we should be collecting zones for each of the homes.
			if (m_bDoGetZones)
			{
				// Mark that we don't need to collect zones any more.
				m_bDoGetZones = false;
				for (int i = 0; i < (int)m_TadoHomes.size(); i++)
				{
					if (!GetZones(m_TadoHomes[i])){
						// Something went wrong, indicate that we do need to collect zones next time.
						m_bDoGetZones = true;
						_log.Log(LOG_ERROR, "Tado: Failed to get zones for home '%s'", m_TadoHomes[i].Name.c_str());
					}
				}
			}

			// Check if the authentication token is still useable.
			if (iTokenCycleCount++ > TADO_TOKEN_MAXLOOPS)
			{
				_log.Log(LOG_NORM, "Tado: refreshing token.");
				if (!GetAuthToken(m_TadoAuthToken, m_TadoRefreshToken, true)) {
					_log.Log(LOG_ERROR, "Tado: failed to refresh authentication token. Skipping this cycle.");
				}
				// Reset the counter to its initial value.
				else iTokenCycleCount = 0;
			}
			// Increase the number of cycles that the token has been used by 1.
			else iTokenCycleCount++;

			// Iterate through the discovered homes and zones. Get some state information.
			for (int HomeIndex = 0; HomeIndex < (int)m_TadoHomes.size(); HomeIndex++) {

				if (!GetHomeState(HomeIndex, m_TadoHomes[HomeIndex]))
				{
					_log.Log(LOG_ERROR, "Tado: Failed to get state for home '%s'", m_TadoHomes[HomeIndex].Name.c_str());
					// Skip to the next home
					continue;
				}

				for (int ZoneIndex = 0; ZoneIndex < (int)m_TadoHomes[HomeIndex].Zones.size(); ZoneIndex++)
				{
					if (!GetZoneState(HomeIndex, ZoneIndex, m_TadoHomes[HomeIndex], m_TadoHomes[HomeIndex].Zones[ZoneIndex]))
					{
						_log.Log(LOG_ERROR, "Tado: Failed to get state for home '%s', zone '%s'", m_TadoHomes[HomeIndex].Name.c_str(), m_TadoHomes[HomeIndex].Zones[ZoneIndex].Name.c_str());
					}
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "Tado: Worker stopped.");
}

// Splits a string inputString by delimiter. If specified returns up to maxelements elements.
// This is an extension of the ::StringSplit function in the helper class.
std::vector<std::string> CTado::StringSplitEx(const std::string &inputString, const std::string &delimiter, const int maxelements)
{
	// Split using the Helper class' StringSplitEx function.
	std::vector<std::string> array;
	StringSplit(inputString, delimiter, array);

	// If we don't have a max number of elements specified we're done.
	if (maxelements == 0) return array;

	// Else we're building a new vector with all the overflowing elements merged into the last element.
	std::vector<std::string> cappedArray;
	for (int i = 0; (unsigned int)i < array.size(); i++)
	{
		if (i <= maxelements-1)
		{
			cappedArray.push_back(array[i]);
		}
		else {
			cappedArray[maxelements-1].append(array[i]);
		}
	}

	return cappedArray;
}


// Runs through the Tado web interface environment file and attempts to regex match the specified key.
bool CTado::MatchValueFromJSKey(const std::string &sKeyName, const std::string &sJavascriptData, std::string &sValue)
{
	// Rewritten to no longer use regex matching. Regex matching is the preferred, more robust way
	// but for various reasons we're not supposed to leverage it. Not using boost library either for
	// the same reasons, so various std::string features are unavailable and have to be implemented manually.

	std::vector<std::string> _sJavascriptDataLines;
	std::map<std::string, std::string> _mEnvironmentKeys;

	// Get the javascript response and split its lines
	StringSplit(sJavascriptData, "\n", _sJavascriptDataLines);

	_log.Debug(DEBUG_HARDWARE, "Tado: MatchValueFromJSKey: Got %lu lines from javascript data.", _sJavascriptDataLines.size());

	if (_sJavascriptDataLines.size() <= 0)
	{
		_log.Log(LOG_ERROR, "Tado: Failed to get any lines from javascript environment file.");
		return false;
	}

	// Process each line.
	for (int i = 0; i < (int)_sJavascriptDataLines.size(); i++)
	{
		std::string _sLine = _sJavascriptDataLines[i];

		_log.Debug(DEBUG_HARDWARE, "Tado: MatchValueFromJSKey: Processing line: '%s'", _sLine.c_str());

		std::string _sLineKey = "";
		std::string _sLineValue = "";

		// Let's split each line on a colon.
		std::vector<std::string> _sLineParts = StringSplitEx(_sLine, ": ", 2);
		if (_sLineParts.size() != 2)
		{
			continue;
		}

		for (int j = 0; j < (int)_sLineParts.size(); j++)
		{
			// Do some cleanup on the parts, so we only keep the text that we want.
			std::string _sLinePart = _sLineParts[j];
			stdreplace(_sLinePart, "\t", "");
			stdreplace(_sLinePart, "',", "");
			stdreplace(_sLinePart, "'","");
			_sLinePart = stdstring_trim(_sLinePart);

			// Check if a Key is already set for the key-value pair. If we don't have a key yet
			// assume that this first entry in the line is the key.
			if (_sLineKey == "")
			{
				_sLineKey = _sLinePart;
			}
			else
			{
				// Since we already have a key, assume that this second entry is the value.
				_sLineValue = _sLinePart;

				// Now that we've got both a key and a value put it in the map
				_mEnvironmentKeys[_sLineKey] = _sLineValue;

				_log.Debug(DEBUG_HARDWARE, "Tado: MatchValueFromJSKey: Line: '%s':'%s'", _sLineKey.c_str(), _sLineValue.c_str());
			}
		}
	}


	// Check the map to get the value we were looking for in the first place.
	if (_mEnvironmentKeys[sKeyName].empty())
	{
		_log.Log(LOG_ERROR, "Tado: Failed to grab %s from the javascript data.", sKeyName.c_str());
		return false;
	}

	sValue = _mEnvironmentKeys[sKeyName];
	if (sValue.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: Value for key %s is zero length.", sKeyName.c_str());
		return false;
	}
	return true;
}

// Grabs the web app environment file
bool CTado::GetTadoApiEnvironment(std::string sUrl)
{
	_log.Debug(DEBUG_HARDWARE, "Tado: GetTadoApiEnvironment called with sUrl=%s", sUrl.c_str());

	// This is a bit of a special case. Since we pretend to be the web
	// application (my.tado.com) we have to play by its rules. It works
	// with some information like a client id and a client secret. We
	// have to pluck that environment information from the web page and
	// then parse it so we can use it in our future calls.

	std::string _sResponse;

	// Download the API environment file
	if (!HTTPClient::GET(sUrl, _sResponse, false)) {
		_log.Log(LOG_ERROR, "Tado: Failed to retrieve API environment from %s", sUrl.c_str());
		return false;
	}

	// Determine which keys we want to grab from the environment
	std::vector<std::string> _vKeysToFetch;
	_vKeysToFetch.push_back("clientId");
	_vKeysToFetch.push_back("clientSecret");
	_vKeysToFetch.push_back("apiEndpoint");
	_vKeysToFetch.push_back("tgaRestApiV2Endpoint");

	// The key values will be stored in a map, lets clean it out first.
	m_TadoEnvironment.clear();

	for (int i = 0; i < (int)_vKeysToFetch.size(); i++)
	{
		// Feed the function the javascript response, and have it attempt to grab the provided key's value from it.
		// Value is stored in m_TadoEnvironment[keyName]
		std::string _sKeyName = _vKeysToFetch[i];
		if (!MatchValueFromJSKey(_sKeyName, _sResponse, m_TadoEnvironment[_sKeyName])) {
			_log.Log(LOG_ERROR, "Tado: Failed to retrieve/match key '%s' from the API environment.", _sKeyName.c_str());
			return false;
		}
	}

	_log.Log(LOG_STATUS, "Tado: Retrieved webapp environment from API.");

	// Mark this action as completed.
	m_bDoGetEnvironment = false;
	return true;
}

// Sets up the environment and grabs an auth token.
bool CTado::Login()
{
	_log.Log(LOG_NORM, "Tado: Attempting login.");

	if (m_bDoGetEnvironment) {
		// First get information about the environment of the web application.
		if (!GetTadoApiEnvironment(TADO_API_ENVIRONMENT_URL))
		{
			return false;
		}
	}

	// Now fetch the token.
	if (!GetAuthToken(m_TadoAuthToken, m_TadoRefreshToken, false))
	{
		return false;
	}

	_log.Log(LOG_NORM, "Tado: Login succesful.");

	return true;
}

// Gets all the homes in the account.
bool CTado::GetHomes() {

	_log.Debug(DEBUG_HARDWARE, "Tado: GetHomes() called.");

	std::stringstream _sstr;
	_sstr << m_TadoEnvironment["tgaRestApiV2Endpoint"] << "/me";
	std::string _sUrl = _sstr.str();

	Json::Value _jsRoot;
	std::string _sResponse;

	try
	{
		SendToTadoApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), _jsRoot);
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: failed to get homes: %s", what.c_str());
		return false;
	}

	// Make sure we start with an empty list.
	m_TadoHomes.clear();

	Json::Value _jsAllHomes = _jsRoot["homes"];

	_log.Debug(DEBUG_HARDWARE, "Tado: Found %d homes.", _jsAllHomes.size());

	for (int i = 0; i < (int)_jsAllHomes.size(); i++) {
		// Store the tado home information in a map.

		if (!_jsAllHomes[i].isObject()) continue;

		_tTadoHome _structTadoHome;
		_structTadoHome.Name = _jsAllHomes[i]["name"].asString();
		_structTadoHome.Id = _jsAllHomes[i]["id"].asString();
		m_TadoHomes.push_back(_structTadoHome);

		_log.Log(LOG_STATUS, "Tado: Registered Home '%s' with id %s", _structTadoHome.Name.c_str(), _structTadoHome.Id.c_str());
	}
	// Sort the homes so they end up in the same order every time.
	sort(m_TadoHomes.begin(), m_TadoHomes.end());

	return true;
}

// Gets all the zones for a particular home
bool CTado::GetZones(_tTadoHome &tTadoHome) {

	std::stringstream ss;
	ss << m_TadoEnvironment["tgaRestApiV2Endpoint"] << "/homes/" << tTadoHome.Id << "/zones";
	std::string _sUrl = ss.str();
	std::string _sResponse;
	Json::Value _jsRoot;

	tTadoHome.Zones.clear();

	try
	{
		SendToTadoApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), _jsRoot);
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "Tado: Failed to get zones from API for Home %s: %s", tTadoHome.Id.c_str(), what.c_str());
		return false;
	}

	// Loop through each of the zones
	for (int zoneIdx = 0; zoneIdx < (int)_jsRoot.size(); zoneIdx++) {
		_tTadoZone _TadoZone;

		_TadoZone.HomeId = tTadoHome.Id;
		_TadoZone.Id = _jsRoot[zoneIdx]["id"].asString();
		_TadoZone.Name = _jsRoot[zoneIdx]["name"].asString();
		_TadoZone.Type = _jsRoot[zoneIdx]["type"].asString();

		_log.Log(LOG_STATUS, "Tado: Registered Zone %s '%s' of type %s", _TadoZone.Id.c_str(), _TadoZone.Name.c_str(), _TadoZone.Type.c_str());

		tTadoHome.Zones.push_back(_TadoZone);
	}

	// Sort the zones based on Id (defined in structure) so we always get them in the same order.
	sort(tTadoHome.Zones.begin(), tTadoHome.Zones.end());

	return true;
}

// Sends a request to the Tado API.
bool CTado::SendToTadoApi(const eTadoApiMethod eMethod, const std::string &sUrl, const std::string &sPostData,
				std::string &sResponse, const std::vector<std::string> & vExtraHeaders, Json::Value &jsDecodedResponse,
				const bool bDecodeJsonResponse, const bool bIgnoreEmptyResponse, const bool bSendAuthHeaders)
{
	try {
		// If there is no token stored then there is no point in doing a request. Unless we specifically
		// decide not to do authentication.
		if (m_TadoAuthToken.size() == 0 && bSendAuthHeaders) throw std::runtime_error("No access token available.");

		// Prepare the headers. Copy supplied vector.
		std::vector<std::string> _vExtraHeaders = vExtraHeaders;

		// If the supplied postdata validates as json, add an appropriate content type header
		if (sPostData.size() > 0)
		{
			Json::Reader _jsReader;
			if (_jsReader.parse(sPostData, *(new Json::Value))) {
				_vExtraHeaders.push_back("Content-Type: application/json");
			}
		}

		// Prepare the authentication headers if requested.
		if (bSendAuthHeaders) _vExtraHeaders.push_back("Authorization: Bearer " + m_TadoAuthToken);

		std::vector<std::string> _vResponseHeaders;
		std::stringstream _ssResponseHeaderString;

		switch (eMethod)
		{
			case Put:
				if (!HTTPClient::PUT(sUrl, sPostData, _vExtraHeaders, sResponse, bIgnoreEmptyResponse))
				{
					throw std::runtime_error(("Failed to perform PUT request to Tado Api: %s", sResponse.c_str()));
				}
				break;

			case Post:
				if (!HTTPClient::POST(sUrl, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders, true, bIgnoreEmptyResponse))
				{
					for (unsigned int i = 0; i < _vResponseHeaders.size(); i++) _ssResponseHeaderString << _vResponseHeaders[i];
					throw std::runtime_error(("Failed to perform POST request to Tado Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str()));
				}
				break;

			case Get:
				if (!HTTPClient::GET(sUrl, _vExtraHeaders, sResponse, _vResponseHeaders, bIgnoreEmptyResponse))
				{
					for (unsigned int i = 0; i < _vResponseHeaders.size(); i++) _ssResponseHeaderString << _vResponseHeaders[i];
					throw std::runtime_error(("Failed to perform GET request to Tado Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str()));
				}
				break;

			case Delete:
				if (!HTTPClient::Delete(sUrl, sPostData, _vExtraHeaders, sResponse, bIgnoreEmptyResponse)) {
					{
						throw std::runtime_error(("Failed to perform DELETE request to Tado Api: %s", sResponse.c_str()));
					}
				}
				break;

			default:
				throw std::runtime_error("Unknown method specified.");
		}

		if (sResponse.size() == 0)
		{
			if (!bIgnoreEmptyResponse) throw std::runtime_error("Received an empty response from Api.");
		}

		if (bDecodeJsonResponse) {
			Json::Reader _jsReader;
			if (!_jsReader.parse(sResponse, jsDecodedResponse)) {
				throw std::runtime_error("Failed to decode Json response from Api.");
			}
		}

		return true;
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		throw std::runtime_error(("Error sending information to Tado API: %s", what.c_str()));
	}
}
