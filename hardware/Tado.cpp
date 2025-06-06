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
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/mainworker.h"
#include "../main/json_helper.h"
#include "../webserver/Base64.h"
#include "Tado.h"

#define TADO_CLIENT_ID "1bb50063-6b0c-4d11-bd99-387f4a91cc46"
#define TADO_API_DEVICE_AUTHORIZE "https://login.tado.com/oauth2/device_authorize"
#define TADO_API_ENVIRONMENT_URL "https://app.tado.com/env.js"


#define TADO_MY_API "https://my.tado.com/api/v2/"
#define TADO_HOPS_API "https://hops.tado.com/"
#define TADO_MOBILE "https://my.tado.com/mobile/1.9/"
#define TADO_EIQ "https://energy-insights.tado.com/api/"
#define TADO_TARIFF "https://tariff-experience.tado.com/api/"
#define TADO_GENIE "https://genie.tado.com/api/v2/"
#define TADO_MINDER "https://minder.tado.com/v1/"

#define TADO_API_GET_TOKEN "https://login.tado.com/oauth2/token"

#define TADO_POLL_LOGIN_INTERVAL 10
#define TADO_POLL_INTERVAL 30		// The plugin should collect information from the API every n seconds.
#define TADO_TOKEN_MAXLOOPS 12		// Default token validity is 600 seconds before it needs to be refreshed.
									// Each cycle takes 30-35 seconds, so let's stay a bit on the safe side.

CTado::CTado(const int ID)
{
	m_HwdID = ID;

	//Retrieve stored Refresh Token
	auto result = m_sql.safe_query("SELECT Extra FROM Hardware WHERE (ID==%d)", m_HwdID);
	if (!result.empty())
	{
		m_szRefreshToken = result[0][0];
	}
}

bool CTado::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
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

	return true;
}

bool CTado::WriteToHardware(const char* pdata, const unsigned char length)
{
	if (m_szAccessToken.empty())
		return false;

	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);


	int HomeIdx = pCmd->LIGHTING2.id2;
	int ZoneIdx = pCmd->LIGHTING2.id3;
	int ServiceIdx = pCmd->LIGHTING2.id4;

	int node_id = (HomeIdx * 1000) + (ZoneIdx * 100) + ServiceIdx;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, HomeIdx, ZoneIdx, ServiceIdx);

	Debug(DEBUG_HARDWARE, "Node %s = home %s zone %s device %d", szIdx, m_TadoHomes[HomeIdx].Name.c_str(), m_TadoHomes[HomeIdx].Zones[ZoneIdx].Name.c_str(), ServiceIdx);

	// ServiceIdx 1 = Away (Read only)
	// ServiceIdx 2 = Setpoint => should be handled in SetSetPoint
	// ServiceIdx 3 = TempHum (Read only)
	// ServiceIdx 4 = Setpoint Override
	// ServiceIdx 5 = Heating Enabled
	// ServiceIdx 6 = Heating On (Read only)
	// ServiceIdx 7 = Heating Power (Read only)
	// ServiceIdx 8 = Open Window Detected (Read only)

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
bool CTado::CreateOverlay(const int idx, const float temp, const bool heatingEnabled, const std::string& terminationType)
{
	Log(LOG_NORM, "CreateOverlay() called with idx=%d, temp=%f, termination type=%s", idx, temp, terminationType.c_str());

	int HomeIdx = idx / 1000;
	int ZoneIdx = (idx % 1000) / 100;
	int ServiceIdx = (idx % 1000) % 100;

	// Check if the zone actually exists.
	if (m_TadoHomes.empty() || m_TadoHomes[HomeIdx].Zones.empty())
	{
		Log(LOG_ERROR, "No such home/zone combo found: %d/%d", HomeIdx, ZoneIdx);
		return false;
	}

	Debug(DEBUG_HARDWARE, "Node %d = home %s zone %s device %d", idx, m_TadoHomes[HomeIdx].Name.c_str(), m_TadoHomes[HomeIdx].Zones[ZoneIdx].Name.c_str(), ServiceIdx);

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
		Log(LOG_ERROR, "Failed to set setpoint via Api: %s", what.c_str());
		return false;
	}

	Debug(DEBUG_HARDWARE, "Response: %s", _sResponse.c_str());

	// Trigger a zone refresh
	GetZoneState(HomeIdx, ZoneIdx, m_TadoHomes[HomeIdx], m_TadoHomes[HomeIdx].Zones[ZoneIdx]);

	return true;
}

void CTado::SetSetpoint(const int id2, const int id3, const int id4, const float temp)
{

	int HomeIdx = id2;
	int ZoneIdx = id3;
	int ServiceIdx = id4;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, HomeIdx, ZoneIdx, ServiceIdx);
	Log(LOG_NORM, "SetSetpoint() called with idx=%s, temp=%f", szIdx, temp);

	int _idx = (HomeIdx * 1000) + (ZoneIdx * 100) + ServiceIdx;
	CreateOverlay(_idx, temp, true, "TADO_MODE");
}

// Requests a new Access token
bool CTado::GetAccessToken()
{
	if (m_szRefreshToken.empty())
	{
		Log(LOG_ERROR, "No Refresh Token?");
		return false;
	}

	m_szAccessToken.clear();

	std::ostringstream s;
	s << "client_id=" << TADO_CLIENT_ID
		<< "&grant_type=refresh_token"
		<< "&refresh_token=" << m_szRefreshToken;

	std::string sPostData = s.str();

	std::string _sResponse;
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
	std::vector<std::string> _vResponseHeaders;

	std::string sResponse;
	if (!HTTPClient::POST(TADO_API_GET_TOKEN, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders))
	{
		Log(LOG_ERROR, "Failed to get a Refresh Token");
		return false;
	}

	Json::Value root;
	if (!ParseJSon(sResponse, root)) {
		Log(LOG_ERROR, "GetAccessToken: Failed to decode Json response from Api.(%s)", sResponse.c_str());
		return false;
	}

	if (
		(root["access_token"].empty() == true)
		|| (root["refresh_token"].empty() == true)
		)
	{
		Log(LOG_ERROR, "GetAccessToken: No Access/Refresh token in respone!");
		if (root["error_description"].empty() == false)
		{
			std::string szError = root["error_description"].asString();
			Log(LOG_ERROR, "GetAccessToken: %s", szError.c_str());
		}

		//We are unable to get a new Access token with the current refresh token
		//we need to login again
		m_szAccessToken.clear();
		m_szRefreshToken.clear();

		Log(LOG_ERROR, "Going to start Login procedure in %d seconds", TADO_POLL_INTERVAL);

		return false;
	}

	m_szAccessToken = root["access_token"].asString();
	m_szRefreshToken = root["refresh_token"].asString();

	//Store refresh_token
	m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID==%d)", m_szRefreshToken.c_str(), m_HwdID);

	return true;
}

// Gets the status information of a zone.
bool CTado::GetZoneState(const int HomeIndex, const int ZoneIndex, const _tTadoHome& home, _tTadoZone& zone)
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
			Log(LOG_ERROR, "Failed to get information on zone '%s': %s", zone.Name.c_str(), what.c_str());
			return false;
		}

		// Zone Home/away
		//bool _bTadoAway = !(_jsRoot["tadoMode"].asString() == "HOME");
		//UpdateSwitch((unsigned char)ZoneIndex * 100 + 1, _bTadoAway, home.Name + " " + zone.Name + " Away");

		// Zone setpoint
		float _fSetpointC = 0;
		if (_jsRoot["setting"]["temperature"]["celsius"].isNumeric())
			_fSetpointC = _jsRoot["setting"]["temperature"]["celsius"].asFloat();
		if (_fSetpointC > 0) {
			SendSetPointSensor(ZoneIndex * 100 + 2, _fSetpointC, home.Name + " " + zone.Name + " Setpoint");
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
		UpdateSwitch(ZoneIndex * 100 + 4, _bManualControl, home.Name + " " + zone.Name + " Manual Setpoint Override");


		// Heating Enabled
		std::string _sType = _jsRoot["setting"]["type"].asString();
		std::string _sPower = _jsRoot["setting"]["power"].asString();
		bool _bHeatingEnabled = false;
		if (_sType == "HEATING" && _sPower == "ON")
			_bHeatingEnabled = true;
		UpdateSwitch(ZoneIndex * 100 + 5, _bHeatingEnabled, home.Name + " " + zone.Name + " Heating Enabled");

		// Heating Power percentage
		std::string _sHeatingPowerType = _jsRoot["activityDataPoints"]["heatingPower"]["type"].asString();
		int _sHeatingPowerPercentage = _jsRoot["activityDataPoints"]["heatingPower"]["percentage"].asInt();
		bool _bHeatingOn = false;
		if (_sHeatingPowerType == "PERCENTAGE" && _sHeatingPowerPercentage >= 0 && _sHeatingPowerPercentage <= 100)
		{
			_bHeatingOn = _sHeatingPowerPercentage > 0;
			UpdateSwitch(ZoneIndex * 100 + 6, _bHeatingOn, home.Name + " " + zone.Name + " Heating On");

			SendPercentageSensor(ZoneIndex * 100 + 7, 0, 255, (float)_sHeatingPowerPercentage, home.Name + " " + zone.Name + " Heating Power");
		}

		// Open Window Detected
		if (zone.OpenWindowDetectionSupported)
		{
			bool _bOpenWindowDetected = false;
			if (_jsRoot["openWindowDetected"].isBool())
			{
				_bOpenWindowDetected = _jsRoot["openWindowDetected"].asBool();
			}

			UpdateSwitch(ZoneIndex * 100 + 8, _bOpenWindowDetected, home.Name + " " + zone.Name + " Open Window Detected");

		}

		return true;
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		Log(LOG_ERROR, "GetZoneState: %s", what.c_str());
		return false;
	}
}

bool CTado::GetHomeState(const int HomeIndex, _tTadoHome& home)
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
			Log(LOG_ERROR, "Failed to get state information on home '%s': %s", home.Name.c_str(), what.c_str());
			return false;
		}

		// Home/away
		bool _bTadoAway = !(_jsRoot["presence"].asString() == "HOME");
		UpdateSwitch(HomeIndex * 1000 + 0, _bTadoAway, home.Name + " Away");

		return true;
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		Log(LOG_ERROR, "GetZoneState: %s", what.c_str());
		return false;
	}
}

void CTado::SendSetPointSensor(const int Idx, const float Temp, const std::string& defaultname)
{
	int HomeIdx = Idx / 1000;
	int ZoneIdx = (Idx % 1000) / 100;
	int ServiceIdx = (Idx % 1000) % 100;

	_tSetpoint thermos;
	thermos.subtype = sTypeSetpoint;
	thermos.id1 = 0;
	thermos.id2 = HomeIdx;
	thermos.id3 = ZoneIdx;
	thermos.id4 = ServiceIdx;
	thermos.dunit = 0;
	thermos.value = Temp;
	sDecodeRXMessage(this, (const unsigned char*)&thermos, defaultname.c_str(), 255, nullptr);
}

// Creates or updates on/off switches.
void CTado::UpdateSwitch(const int Idx, const bool bOn, const std::string& defaultname)
{

	int HomeIdx = Idx / 1000;
	int ZoneIdx = (Idx % 1000) / 100;
	int ServiceIdx = (Idx % 1000) % 100;

	//bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, HomeIdx, ZoneIdx, ServiceIdx);
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
	lcmd.LIGHTING2.id2 = HomeIdx;
	lcmd.LIGHTING2.id3 = ZoneIdx;
	lcmd.LIGHTING2.id4 = ServiceIdx;
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
	sDecodeRXMessage(this, (const unsigned char*)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}

// Removes any active overlay from a specific zone.
bool CTado::CancelOverlay(const int Idx)
{
	Debug(DEBUG_HARDWARE, "CancelSetpointOverlay() called with idx=%d", Idx);

	int HomeIdx = Idx / 1000;
	int ZoneIdx = (Idx % 1000) / 100;
	//int ServiceIdx = (Idx % 1000) % 100;

	// Check if the home and zone actually exist.
	if (m_TadoHomes.empty() || m_TadoHomes[HomeIdx].Zones.empty())
	{
		Log(LOG_ERROR, "No such home/zone combo found: %d/%d", HomeIdx, ZoneIdx);
		return false;
	}

	std::stringstream _sstr;
	_sstr << m_TadoEnvironment["tgaRestApiV2Endpoint"] << "/homes/" << m_TadoHomes[HomeIdx].Id << "/zones/" << m_TadoHomes[HomeIdx].Zones[ZoneIdx].Id << "/overlay";
	std::string _sUrl = _sstr.str();
	std::string _sResponse;

	try
	{
		SendToTadoApi(Delete, _sUrl, "", _sResponse, *(new std::vector<std::string>()), *(new Json::Value), false, true);

	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		Log(LOG_ERROR, "Error cancelling the setpoint overlay: %s", what.c_str());
		return false;
	}

	// Trigger a zone refresh
	Log(LOG_STATUS, "Setpoint overlay cancelled.");
	GetZoneState(HomeIdx, ZoneIdx, m_TadoHomes[HomeIdx], m_TadoHomes[HomeIdx].Zones[ZoneIdx]);

	return true;
}

void CTado::Print_Login_URL(const std::string& url)
{
	Log(LOG_STATUS, "Copy and paste the below URL in your browser and follow the steps in your browser.");
	Log(LOG_STATUS, "Domoticz will poll every 10 seconds to see if you have complete all the steps and continue.");
	Log(LOG_STATUS, "------------------------------");
	Log(LOG_STATUS, "%s", url.c_str());
	Log(LOG_STATUS, "------------------------------");
}

bool CTado::Do_Login_Work()
{
	Log(LOG_STATUS, "We need to authenticate ourselfs");

	std::ostringstream s;
	s << "client_id=" << TADO_CLIENT_ID
		<< "&scope=offline_access";

	std::string sPostData = s.str();

	std::string _sResponse;
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
	std::vector<std::string> _vResponseHeaders;

	std::string sResponse;
	if (!HTTPClient::POST(TADO_API_DEVICE_AUTHORIZE, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders))
	{
		Log(LOG_ERROR, "Failed to Authorize device with TADO server!");
		return false;
	}

	Json::Value root;
	if (!ParseJSon(sResponse, root)) {
		return false;
	}

	if (
		(root["device_code"].empty() == true)
		|| (root["interval"].empty() == true)
		|| (root["verification_uri"].empty() == true)
		|| (root["verification_uri_complete"].empty() == true)
		)
	{
		Log(LOG_ERROR, "Authentication: Invalid API response!");
		return false;
	}

	std::string verification_uri_complete = root["verification_uri_complete"].asString();
	std::string device_code = root["device_code"].asString();
	int iPollInterval = root["interval"].asInt();

	//Let's find out if the user completed the authorization
	int iSecCounter = 0;

	while (!IsStopRequested(1000))
	{
		if (iSecCounter % 60 == 0)
		{
			Print_Login_URL(verification_uri_complete);
		}
		iSecCounter++;
		if (iSecCounter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (iSecCounter % TADO_POLL_LOGIN_INTERVAL == 0) //iPollInterval
		{
			s.str("");
			s << "client_id=" << TADO_CLIENT_ID
				<< "&device_code="<< device_code
				<< "&grant_type=urn:ietf:params:oauth:grant-type:device_code";
			sPostData = s.str();

			if (!HTTPClient::POST(TADO_API_GET_TOKEN, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders))
			{
				continue;
			}

			Json::Value root;
			if (!ParseJSon(sResponse, root)) {
				continue;
			}

			if (
				(root["access_token"].empty() == true)
				|| (root["refresh_token"].empty() == true)
				)
			{
				continue;
			}

			m_szAccessToken = root["access_token"].asString();
			m_szRefreshToken = root["refresh_token"].asString();

			//Store refresh_token
			m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID==%d)", m_szRefreshToken.c_str(), m_HwdID);
			return true;
		}
	}
	return false;
}

void CTado::Do_Work()
{
	Log(LOG_STATUS, "Worker started. Will poll every %d seconds.", TADO_POLL_INTERVAL);
	int iSecCounter = TADO_POLL_INTERVAL - 3;
	int iTokenCycleCount = 0;

	while (!IsStopRequested(1000))
	{
		iSecCounter++;
		if (iSecCounter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (!(iSecCounter % TADO_POLL_INTERVAL == 0))
			continue;

		if (m_bDoGetEnvironment)
		{
			if (GetTadoApiEnvironment(TADO_API_ENVIRONMENT_URL))
			{
				m_bDoGetEnvironment = false;
			}
		}
		if (m_bDoGetEnvironment)
			continue;

		// Only login if we should.
		if (m_szRefreshToken.empty())
		{
			if (!Do_Login_Work())
			{
				if (IsStopRequested(100))
				{
					break;
				}
				continue;
			}
		}
		if (
			(m_szAccessToken.empty())
			|| (iTokenCycleCount++ > TADO_TOKEN_MAXLOOPS)
			)
		{
			GetAccessToken();
			iTokenCycleCount = 0;
		}
		iTokenCycleCount++;

		if (m_szAccessToken.empty())
			continue; //no need to continue if we don't have an access token

		// Check if we should get homes from tado account
		if (m_bDoGetHomes)
		{
			// If we fail to do so, abort.
			if (!GetHomes())
			{
				Log(LOG_ERROR, "Failed to get homes from Tado account.");
				// Try to get homes next iteration. Other than that we can't do much now.
				continue;
			}
			// Else move on to getting zones for each of the homes.
			m_bDoGetZones = true;
			m_bDoGetHomes = false;
		}

		// Check if we should be collecting zones for each of the homes.
		if (m_bDoGetZones)
		{
			// Mark that we don't need to collect zones any more.
			m_bDoGetZones = false;
			for (auto& m_TadoHome : m_TadoHomes)
			{
				if (!GetZones(m_TadoHome))
				{
					// Something went wrong, indicate that we do need to collect zones next time.
					m_bDoGetZones = true;
					Log(LOG_ERROR, "Failed to get zones for home '%s'", m_TadoHome.Name.c_str());
				}
			}
		}

		// Iterate through the discovered homes and zones. Get some state information.
		for (int HomeIndex = 0; HomeIndex < (int)m_TadoHomes.size(); HomeIndex++) {

			if (!GetHomeState(HomeIndex, m_TadoHomes[HomeIndex]))
			{
				Log(LOG_ERROR, "Failed to get state for home '%s'", m_TadoHomes[HomeIndex].Name.c_str());
				// Skip to the next home
				continue;
			}

			for (int ZoneIndex = 0; ZoneIndex < (int)m_TadoHomes[HomeIndex].Zones.size(); ZoneIndex++)
			{
				if (!GetZoneState(HomeIndex, ZoneIndex, m_TadoHomes[HomeIndex], m_TadoHomes[HomeIndex].Zones[ZoneIndex]))
				{
					Log(LOG_ERROR, "Failed to get state for home '%s', zone '%s'", m_TadoHomes[HomeIndex].Name.c_str(), m_TadoHomes[HomeIndex].Zones[ZoneIndex].Name.c_str());
				}
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped.");
}

// Splits a string inputString by delimiter. If specified returns up to maxelements elements.
// This is an extension of the ::StringSplit function in the helper class.
std::vector<std::string> CTado::StringSplitEx(const std::string& inputString, const std::string& delimiter, const int maxelements)
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
		if (i <= maxelements - 1)
		{
			cappedArray.push_back(array[i]);
		}
		else {
			cappedArray[maxelements - 1].append(array[i]);
		}
	}

	return cappedArray;
}


// Runs through the Tado web interface environment file and attempts to regex match the specified key.
bool CTado::MatchValueFromJSKey(const std::string& sKeyName, const std::string& sJavascriptData, std::string& sValue)
{
	// Rewritten to no longer use regex matching. Regex matching is the preferred, more robust way
	// but for various reasons we're not supposed to leverage it. Not using boost library either for
	// the same reasons, so various std::string features are unavailable and have to be implemented manually.

	std::vector<std::string> _sJavascriptDataLines;
	std::map<std::string, std::string> _mEnvironmentKeys;

	// Get the javascript response and split its lines
	StringSplit(sJavascriptData, "\n", _sJavascriptDataLines);

	Debug(DEBUG_HARDWARE, "MatchValueFromJSKey: Got %zu lines from javascript data.", _sJavascriptDataLines.size());

	if (_sJavascriptDataLines.empty())
	{
		Log(LOG_ERROR, "Failed to get any lines from javascript environment file.");
		return false;
	}

	// Process each line.
	for (const auto& _sLine : _sJavascriptDataLines)
	{
		Debug(DEBUG_HARDWARE, "MatchValueFromJSKey: Processing line: '%s'", _sLine.c_str());

		std::string _sLineKey;
		std::string _sLineValue;

		// Let's split each line on a colon.
		std::vector<std::string> _sLineParts = StringSplitEx(_sLine, ": ", 2);
		if (_sLineParts.size() != 2)
		{
			continue;
		}

		for (auto _sLinePart : _sLineParts)
		{
			// Do some cleanup on the parts, so we only keep the text that we want.
			stdreplace(_sLinePart, "\t", "");
			stdreplace(_sLinePart, "',", "");
			stdreplace(_sLinePart, "'", "");
			_sLinePart = stdstring_trim(_sLinePart);

			// Check if a Key is already set for the key-value pair. If we don't have a key yet
			// assume that this first entry in the line is the key.
			if (_sLineKey.empty())
			{
				_sLineKey = _sLinePart;
			}
			else
			{
				// Since we already have a key, assume that this second entry is the value.
				_sLineValue = _sLinePart;

				// Now that we've got both a key and a value put it in the map
				_mEnvironmentKeys[_sLineKey] = _sLineValue;

				Debug(DEBUG_HARDWARE, "MatchValueFromJSKey: Line: '%s':'%s'", _sLineKey.c_str(), _sLineValue.c_str());
			}
		}
	}

	// Check the map to get the value we were looking for in the first place.
	if (_mEnvironmentKeys[sKeyName].empty())
	{
		Log(LOG_ERROR, "Failed to grab %s from the javascript data.", sKeyName.c_str());
		return false;
	}

	sValue = _mEnvironmentKeys[sKeyName];
	if (sValue.empty())
	{
		Log(LOG_ERROR, "Value for key %s is zero length.", sKeyName.c_str());
		return false;
	}
	return true;
}

// Grabs the web app environment file
bool CTado::GetTadoApiEnvironment(const std::string& sUrl)
{
	// The key values will be stored in a map, lets clean it out first.
	m_TadoEnvironment.clear();
	m_TadoEnvironment["tgaRestApiV2Endpoint"] = TADO_MY_API;
	return true;
}

// Gets all the homes in the account.
bool CTado::GetHomes() {

	Debug(DEBUG_HARDWARE, "GetHomes() called.");

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
		Log(LOG_ERROR, "Failed to get homes: %s", what.c_str());
		return false;
	}

	// Make sure we start with an empty list.
	m_TadoHomes.clear();

	Json::Value _jsAllHomes = _jsRoot["homes"];

	Debug(DEBUG_HARDWARE, "Found %d homes.", _jsAllHomes.size());

	for (auto& _jsAllHome : _jsAllHomes)
	{
		// Store the tado home information in a map.

		if (!_jsAllHome.isObject())
			continue;

		_tTadoHome _structTadoHome;
		_structTadoHome.Name = _jsAllHome["name"].asString();
		_structTadoHome.Id = _jsAllHome["id"].asString();
		m_TadoHomes.push_back(_structTadoHome);

		Log(LOG_STATUS, "Registered Home '%s' with id %s", _structTadoHome.Name.c_str(), _structTadoHome.Id.c_str());
	}
	// Sort the homes so they end up in the same order every time.
	sort(m_TadoHomes.begin(), m_TadoHomes.end());

	return true;
}

// Gets all the zones for a particular home
bool CTado::GetZones(_tTadoHome& tTadoHome) {

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
		Log(LOG_ERROR, "Failed to get zones from API for Home %s: %s", tTadoHome.Id.c_str(), what.c_str());
		return false;
	}

	// Loop through each of the zones
	for (auto& zoneIdx : _jsRoot)
	{
		_tTadoZone _TadoZone;

		_TadoZone.HomeId = tTadoHome.Id;
		_TadoZone.Id = zoneIdx["id"].asString();
		_TadoZone.Name = zoneIdx["name"].asString();
		_TadoZone.Type = zoneIdx["type"].asString();
		_TadoZone.OpenWindowDetectionSupported = zoneIdx["openWindowDetection"]["supported"].asBool();

		Log(LOG_STATUS, "Registered Zone %s '%s' of type %s", _TadoZone.Id.c_str(), _TadoZone.Name.c_str(), _TadoZone.Type.c_str());

		tTadoHome.Zones.push_back(_TadoZone);
	}

	// Sort the zones based on Id (defined in structure) so we always get them in the same order.
	sort(tTadoHome.Zones.begin(), tTadoHome.Zones.end());

	return true;
}

// Sends a request to the Tado API.
bool CTado::SendToTadoApi(const eTadoApiMethod eMethod, const std::string& sUrl, const std::string& sPostData,
	std::string& sResponse, const std::vector<std::string>& vExtraHeaders, Json::Value& jsDecodedResponse,
	const bool bDecodeJsonResponse, const bool bIgnoreEmptyResponse)
{
	if (m_szAccessToken.empty())
	{
		Log(LOG_ERROR, "No Access Token available.");
		return false;
	}

	try {
		// If there is no token stored then there is no point in doing a request. Unless we specifically
		// decide not to do authentication.

		// Prepare the headers. Copy supplied vector.
		std::vector<std::string> _vExtraHeaders = vExtraHeaders;

		// If the supplied postdata validates as json, add an appropriate content type header
		if (!sPostData.empty())
		{
			if (ParseJSon(sPostData, *(new Json::Value))) {
				_vExtraHeaders.push_back("Content-Type: application/json");
			}
		}

		// Prepare the authentication headers if requested.
		_vExtraHeaders.push_back("Authorization: Bearer " + m_szAccessToken);

		std::vector<std::string> _vResponseHeaders;
		std::stringstream _ssResponseHeaderString;

		switch (eMethod)
		{
		case Put:
			if (!HTTPClient::PUT(sUrl, sPostData, _vExtraHeaders, sResponse, bIgnoreEmptyResponse))
			{
				Log(LOG_ERROR, "Failed to perform PUT request to Tado Api: %s", sResponse.c_str());
				return false;
			}
			break;

		case Post:
			if (!HTTPClient::POST(sUrl, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders, true, bIgnoreEmptyResponse))
			{
				for (auto& _vResponseHeader : _vResponseHeaders)
					_ssResponseHeaderString << _vResponseHeader;
				Log(LOG_ERROR, "Failed to perform POST request to Tado Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str().c_str());
				return false;
			}
			break;

		case Get:
			if (!HTTPClient::GET(sUrl, _vExtraHeaders, sResponse, _vResponseHeaders, bIgnoreEmptyResponse))
			{
				for (auto& _vResponseHeader : _vResponseHeaders)
					_ssResponseHeaderString << _vResponseHeader;
				Log(LOG_ERROR, "Failed to perform GET request to Tado Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str().c_str());
				return false;
			}
			break;

		case Delete:
			if (!HTTPClient::Delete(sUrl, sPostData, _vExtraHeaders, sResponse, bIgnoreEmptyResponse)) {
				{
					Log(LOG_ERROR, "Failed to perform DELETE request to Tado Api: %s", sResponse.c_str());
					return false;
				}
			}
			break;

		default:
		{
			Log(LOG_ERROR, "Unknown method specified.");
			return false;
		}
		}

		if (sResponse.empty())
		{
			if (!bIgnoreEmptyResponse) {
				Log(LOG_ERROR, "Received an empty response from Api.");
				return false;
			}
		}

		if (bDecodeJsonResponse) {
			if (!ParseJSon(sResponse, jsDecodedResponse)) {
				Log(LOG_ERROR, "Failed to decode Json response from Api.");
				return false;
			}
		}
	}
	catch (std::exception& e)
	{
		std::string what = e.what();
		Log(LOG_ERROR, "Error sending information to Tado API: %s", what.c_str());
		return false;
	}
	return true;
}
