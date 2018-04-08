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
#include "../httpclient/UrlEncode.h"
#include "../main/mainworker.h"
#include "../json/json.h"
#include "../webserver/Base64.h"
#include "Tado.h"
#include <boost/regex.hpp>


#define round(a) ( int ) ( a + .5 )
#define TADO_POLL_INTERVAL 30


CTado::~CTado(void)
{
}

CTado::CTado(const int ID, const std::string &username, const std::string &password)
{
	m_HwdID = ID;
	m_TadoUsername = username;
	m_TadoPassword = password;

	_log.Log(LOG_NORM, "Tado: Started Tado plugin with ID=" + boost::to_string(m_HwdID) + ", username=" + m_TadoUsername);

	Init();
}

bool CTado::StartHardware()
{
	_log.Log(LOG_NORM, "Tado: StartHardware() called.");
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CTado::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

void CTado::Init()
{
	_log.Log(LOG_NORM, "Tado: Init() called.");
	m_stoprequested = false;
	m_bDoLogin = true;
	m_timesUntilTokenRefresh = 0;

	boost::trim(m_TadoUsername);
	boost::trim(m_TadoPassword);
}


bool CTado::StopHardware()
{
	_log.Log(LOG_NORM, "Tado: StopHardware() called.");
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
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

	_log.Log(LOG_NORM, "Tado: Node " + boost::to_string(node_id) + " = home " + m_TadoHomes[HomeIdx].Name + " zone " + m_TadoZones[ZoneIdx].Name + " device " + boost::to_string(ServiceIdx));

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

bool CTado::CreateOverlay(const int idx, const float temp, const bool heatingEnabled, const std::string terminationType = "TADO_MODE")
{
	_log.Log(LOG_NORM, "Tado: CreateOverlay() called with idx=" + boost::to_string(idx) + ", temp=" + boost::to_string(temp)+", termination type="+terminationType);

	int HomeIdx = idx / 1000;
	int ZoneIdx = (idx % 1000) / 100;
	int ServiceIdx = (idx % 1000) % 100;

	// _log.Log(LOG_NORM, "Tado: Node " + boost::to_string(idx) + " = home " + m_TadoHomes[HomeIdx].Name + " zone " + m_TadoZones[ZoneIdx].Name + " device " + boost::to_string(ServiceIdx));


	std::string _sUrl = m_TadoRestApiV2Endpoint + "/homes/" + m_TadoZones[ZoneIdx].HomeId + "/zones/" + m_TadoZones[ZoneIdx].Id + "/overlay";
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Content-Type: application/json;charset=UTF-8");
	_vExtraHeaders.push_back("Authorization: Bearer " + m_TadoAuthToken);
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

	if (!HTTPClient::PUT(_sUrl, _jsPostData.toStyledString(), _vExtraHeaders, _sResponse, false)) {
		_log.Log(LOG_ERROR, "Tado: Failed to set setpoint via Api.");
		return false;
	}

	_log.Log(LOG_NORM, "Tado: Response: " + _sResponse);

	Json::Reader _jsReader;
	Json::Value _jsRoot;
	if (!_jsReader.parse(_sResponse, _jsRoot)) {
		_log.Log(LOG_ERROR, "Tado: Failed to parse Json Api response after setting setpoint.");
		return false;
	}

	if (temp > -1)
	{
		if (_jsRoot["setting"]["temperature"]["celsius"].asFloat() == temp) {
			SendSetPointSensor(idx, temp, m_TadoHomes[HomeIdx].Name + " " + m_TadoZones[ZoneIdx].Name + " Setpoint");
		}
	}

	return true;
}


void CTado::SetSetpoint(const int idx, const float temp)
{
	_log.Log(LOG_NORM, "Tado: SetSetpoint() called with idx=" + boost::to_string(idx) + ", temp=" + boost::to_string(temp));
	CreateOverlay(idx, temp, true, "TADO_MODE");
}

bool CTado::GetAuthToken(std::string &authtoken, std::string &refreshtoken, const bool refreshUsingToken = false)
{
	if (m_TadoUsername.size() == 0) 
	{
		_log.Log(LOG_ERROR, "Tado: No username specified.");
		return false;
	}
	if (m_TadoPassword.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: No password specified.");
		return false;
	}

	std::string _sURL = m_TadoApiEndpoint + "/token";
	std::ostringstream s;
	std::string _sGrantType = (refreshUsingToken ? "refresh_token" : "password");

	s << "client_id=" << m_TadoApiClientId << "&grant_type=";
	s << _sGrantType << "&scope=home.user&client_secret=";
	s << m_TadoApiClientSecret;

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
	// _vExtraHeaders.push_back("Referer: https://my.tado.com/");
	std::vector<std::string> _vResponseHeaders;
	
	if (!HTTPClient::POST(_sURL, sPostData, _vExtraHeaders, _sResponse, _vResponseHeaders, true, false)) {
		_log.Log(LOG_ERROR, "Tado: failed to get token.");
		return false;
	}

	// _log.Log(LOG_NORM, "Tado: got response: " + _sResponse);

	Json::Reader _jsReader;
	Json::Value _jsRoot;
	if (!_jsReader.parse(_sResponse, _jsRoot)) 
	{
		_log.Log(LOG_ERROR, "Tado: failed to parse token json.");
		return false;
	}

	if (_jsRoot["error"].asString().size() != 0) {
		_log.Log(LOG_ERROR, "Tado: Api error: " + _jsRoot["error"].asString());
		return false;
	}

	authtoken = _jsRoot["access_token"].asString();
	if (authtoken.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: received token is zero length.");
		return false;
	}

	refreshtoken = _jsRoot["refresh_token"].asString();
	if (refreshtoken.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: received refresh token is zero length.");
		return false;
	}
	
	_log.Log(LOG_STATUS, "Tado: Received access token from API.");
	_log.Log(LOG_STATUS, "Tado: Received refresh token from API.");
	
	// Reset the counter
	m_timesUntilTokenRefresh = 5;

	return true;
}

bool CTado::GetZoneState(const int HomeIndex, const int ZoneIndex, const _tTadoHome home, _tTadoZone &zone)
{
	std::string _sUrl = m_TadoRestApiV2Endpoint + "/homes/" + zone.HomeId + "/zones/" + zone.Id + "/state";
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Authorization: Bearer " + m_TadoAuthToken);

	std::string _sResponse;

	if (!HTTPClient::GET(_sUrl, _vExtraHeaders, _sResponse, false)) {
		_log.Log(LOG_ERROR, "Tado: failed to get information on zone " + zone.Name);
		return false;
	}

	Json::Reader _jsReader;
	Json::Value _jsRoot;
	if (!_jsReader.parse(_sResponse, _jsRoot)) {
		_log.Log(LOG_ERROR, "Tado: Failed to parse JSON information on zone " + zone.Name);
		return false;
	}

	// Zone Home/away
	bool _bTadoAway = !(_jsRoot["tadoMode"].asString() == "HOME");
	UpdateSwitch((unsigned char)ZoneIndex * 100 + 1, _bTadoAway, home.Name + " " + zone.Name + " Away");

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


// Creates and updates switch used to log Heating and/or Cooling.
void CTado::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
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

bool CTado::CancelOverlay(const int Idx)
{
	_log.Log(LOG_NORM, "Tado: CancelSetpointOverlay() called with idx=" + boost::to_string(Idx));

	int HomeIdx = Idx / 1000;
	int ZoneIdx = (Idx % 1000) / 100;
	int ServiceIdx = (Idx % 1000) % 100;

	std::string _sUrl = m_TadoRestApiV2Endpoint + "/homes/" + m_TadoHomes[HomeIdx].Id + "/zones/" + m_TadoZones[ZoneIdx].Id + "/overlay";
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Authorization: Bearer " + m_TadoAuthToken);
	std::string _sResponse;

	if (!HTTPClient::Delete(_sUrl, "", _vExtraHeaders, _sResponse, true))
	{
		_log.Log(LOG_ERROR, "Tado: error cancelling the setpoint overlay.");
		return false;
	}
	_log.Log(LOG_STATUS, "Tado: Setpoint overlay cancelled.");
	return true;
}

void CTado::Do_Work()
{
	_log.Log(LOG_STATUS, "Tado: Do_Work() called, worker started...");
	int sec_counter = TADO_POLL_INTERVAL - 5;

	// Only login if we should.
	if (m_bDoLogin) 
	{
		// Lets try to get authentication set up. If not, stop the worker.
		if (!Login()) m_stoprequested = true;
	}

	GetHomes();
	for (int i = 0; i < (int)m_TadoHomes.size(); i++) 
	{
		GetZones(m_TadoHomes[i]);
	}

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % TADO_POLL_INTERVAL == 0)
		{
			// Check if we should refresh the token
			if (m_timesUntilTokenRefresh == 0)
			{
				_log.Log(LOG_NORM, "Tado: refreshing token.");
				if (!GetAuthToken(m_TadoAuthToken, m_TadoRefreshToken, true)) {
					_log.Log(LOG_ERROR, "Tado: failed to refresh token. Skipping this cycle.");
				}
			}
			// Else decrease the number of iterations that we can use the token by 1.
			else m_timesUntilTokenRefresh--;

			// Iterate through the discovered homes and zones. Get some state information.
			for (int HomeIndex = 0; HomeIndex < (int)m_TadoHomes.size(); HomeIndex++) {
				for (int ZoneIndex = 0; ZoneIndex < (int)m_TadoZones.size(); ZoneIndex++)
				{
					GetZoneState(HomeIndex, ZoneIndex, m_TadoHomes[HomeIndex], m_TadoZones[ZoneIndex]);
				}
			}


			for (int i = 0; i < (int)m_TadoZones.size(); i++) {
				
			}

			// GetMeterDetails();

		}

	}
	_log.Log(LOG_STATUS, "Tado: Worker stopped...");
}


bool CTado::GetTadoApiEnvironment(std::string url)
{
	// This is a bit of a special case. Since we pretend to be the web
	// application (my.tado.com) we have to play by its rules. It works
	// with some information like a client id and a client secret. We 
	// have to pick that environment information from the web page and 
	// then parse it so we can use it in our future calls.

	std::string _sResponse;
	boost::match_results<std::string::const_iterator> matches;

	// Download the API environment file
	if (!HTTPClient::GET(url, _sResponse, false)) {
		_log.Log(LOG_ERROR, "Tado: Failed to retrieve Api environment.");
		return false;
	}

	// Grab the "clientId" from the response. 
	boost::regex reClientid ("clientId: '(.*?)'");
	if (!boost::regex_search(_sResponse, matches, reClientid)) {
		_log.Log(LOG_ERROR, "Tado: Failed to retrieve clientId from the Api environment.");
		return false;
	}
	m_TadoApiClientId = matches[1];
	if (m_TadoApiClientId.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: Received clientId is zero length.");
		return false;
	}

	// Grab the "clientSecret" from the response.
	boost::regex reClientSecret ("clientSecret: '(.*?)'");
	if (!boost::regex_search(_sResponse, matches, reClientSecret)) {
		_log.Log(LOG_ERROR, "Tado: Failed to retrieve clientSecret from the Api environment.");
		return false;
	}
	m_TadoApiClientSecret = matches[1];
	if (m_TadoApiClientSecret.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: Received clientSecret is zero length.");
		return false;
	}

	// Grab the "apiEndpoint" from the response
	boost::regex reApiEndpoint ("apiEndpoint: '(.*?)'");
	if (!boost::regex_search(_sResponse, matches, reApiEndpoint)) {
		_log.Log(LOG_ERROR, "Tado: Failed to retrieve apiEndpoint from the Api environment.");
		return false;
	}
	m_TadoApiEndpoint = matches[1];
	if (m_TadoApiEndpoint.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: Received apiEndpoint is zero length.");
		return false;
	}

	// Grab the "tgaRestApiV2Endpoint" from the response
	boost::regex reRestApiV2Endpoint("tgaRestApiV2Endpoint: '(.*?)'");
	if (!boost::regex_search(_sResponse, matches, reRestApiV2Endpoint)) {
		_log.Log(LOG_ERROR, "Tado: Failed to retrieve tgaRestApiV2Endpoint from the Api environment.");
		return false;
	}
	m_TadoRestApiV2Endpoint = matches[1];
	if (m_TadoRestApiV2Endpoint.size() == 0)
	{
		_log.Log(LOG_ERROR, "Tado: Received tgaRestApiV2Endpoint is zero length.");
		return false;
	}


	_log.Log(LOG_STATUS, "Tado: Retrieved webapp environment from API.");

	return true;
}

bool CTado::Login()
{
	_log.Log(LOG_NORM, "Tado: Attempting login.");

	// First get information about the environment of the web application.
	if (!GetTadoApiEnvironment("https://my.tado.com/webapp/env.js")) 
	{
		return false;
	}

	// Now fetch the token.
	if (!GetAuthToken(m_TadoAuthToken, m_TadoRefreshToken, false)) 
	{
		return false;
	}

	_log.Log(LOG_NORM, "Tado: Login succesful.");

	// We have logged in, no need to do it again.
	m_bDoLogin = false;

	return true;
}


bool CTado::GetHomes() {

	std::string _sUrl = m_TadoRestApiV2Endpoint + "/me";
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Authorization: Bearer " + m_TadoAuthToken);

	std::string _sResponse;
	
	if (!HTTPClient::GET(_sUrl, _vExtraHeaders, _sResponse, false)) {
		_log.Log(LOG_ERROR, "Tado: failed to get homes.");
		return false;
	}

	Json::Reader _jsReader;
	Json::Value _jsRoot;
	if (!_jsReader.parse(_sResponse, _jsRoot)) {
		_log.Log(LOG_ERROR, "Tado: Failed to parse homes from Api.");
		return false;
	}

	m_TadoHomes.clear();

	Json::Value _jsAllHomes = _jsRoot["homes"];
	_log.Log(LOG_NORM, "Tado: Found homes: " + boost::to_string(_jsAllHomes.size()));

	for (int i = 0; i < (int)_jsAllHomes.size(); i++) {
		// Store the tado home information in a map.

		if (!_jsAllHomes[i].isObject()) continue;

		_tTadoHome _structTadoHome;
		_structTadoHome.Name = _jsAllHomes[i]["name"].asString();
		_structTadoHome.Id = _jsAllHomes[i]["id"].asString();
		m_TadoHomes[i] = _structTadoHome;

		_log.Log(LOG_STATUS, "Tado: Registered home " + _structTadoHome.Name + " with id " + _structTadoHome.Id);
	}

	return true;
}


bool CTado::GetZones(const _tTadoHome &TadoHome) {

	std::string _sUrl = m_TadoRestApiV2Endpoint + "/homes/" + TadoHome.Id + "/zones";
	std::string _sResponse;
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Authorization: Bearer " + m_TadoAuthToken);

	if (!HTTPClient::GET(_sUrl, _vExtraHeaders, _sResponse, false)) {
		_log.Log(LOG_ERROR, "Tado: No zones found for home " + TadoHome.Id);
		return false;
	}

	Json::Reader _jsReader;
	Json::Value _jsRoot;
	if (!_jsReader.parse(_sResponse, _jsRoot)) {
		_log.Log(LOG_ERROR, "Tado: Failed to parse zones for home "+ TadoHome.Name +" from Api.");
		return false;
	}

	// Loop through each of the zones
	for (int zoneIdx = 0; zoneIdx < (int)_jsRoot.size(); zoneIdx++) {
		_tTadoZone _TadoZone;

		_TadoZone.HomeId = TadoHome.Id;
		_TadoZone.Id = _jsRoot[zoneIdx]["id"].asString();
		_TadoZone.Name = _jsRoot[zoneIdx]["name"].asString();
		_TadoZone.Type = _jsRoot[zoneIdx]["type"].asString();

		_log.Log(LOG_NORM, "Tado: Registered zone " + _TadoZone.Id + " '" + _TadoZone.Name + "' of type " + _TadoZone.Type);

		m_TadoZones[zoneIdx] = _TadoZone;
	}

	return true;
}