#include "stdafx.h"
#include "Honeywell.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/WebServerHelper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"
#include "../webserver/Base64.h"

#define round(a) ( int ) ( a + .5 )

const std::string HONEYWELL_DEFAULT_APIKEY = "atD3jtzXC5z4X8WPbzvo0CBqWi7S81Nh";
const std::string HONEYWELL_DEFAULT_APISECRET = "TXDzy2aHpAJw6YiO";
const std::string HONEYWELL_LOCATIONS_PATH = "https://api.honeywell.com/v2/locations?apikey=[apikey]";
const std::string HONEYWELL_UPDATE_THERMOSTAT = "https://api.honeywell.com/v2/devices/thermostats/[deviceid]?apikey=[apikey]&locationId=[locationid]";
const std::string HONEYWELL_TOKEN_PATH = "https://api.honeywell.com/oauth2/token";

const std::string kHeatSetPointDesc = "Target temperature ([devicename])";
const std::string kHeatingDesc = "Heating ([devicename])";
const std::string kOperationStatusDesc = "Heating state ([devicename])";
const std::string kOutdoorTempDesc = "Outdoor temperature ([devicename])";
const std::string kRoomTempDesc = "Room temperature ([devicename])";
const std::string kAwayDesc = "Away ([name])";

extern http::server::CWebServerHelper m_webservers;

CHoneywell::CHoneywell(const int ID, const std::string &Username, const std::string &Password, const std::string &Extra)
{
	m_HwdID = ID;
	mAccessToken = Username;
	mRefreshToken = Password;
	stdstring_trim(mAccessToken);
	stdstring_trim(mRefreshToken);

        // get the data from the extradata field
        std::vector<std::string> strextra;
        StringSplit(Extra, "|", strextra);
	if (strextra.size() == 2)
	{
		mApiKey = base64_decode(strextra[0]);
		mApiSecret = base64_decode(strextra[1]);
	}
	if (mApiKey.empty()) {
		_log.Log(LOG_STATUS, "Honeywell: No API key was set. Using default API key. This will result in many errors caused by quota limitations.");
		mApiKey = HONEYWELL_DEFAULT_APIKEY;
		mApiSecret = HONEYWELL_DEFAULT_APISECRET;
	}
	if (Username.empty() || Password.empty()) {
		_log.Log(LOG_ERROR, "Honeywell: Please update your access token/request token!...");
	}
	mLastMinute = -1;
	Init();
}

CHoneywell::~CHoneywell(void)
{
}

void CHoneywell::Init()
{
	mTokenExpires = mytime(NULL);
}

bool CHoneywell::StartHardware()
{
	RequestStart();

	Init();
	mLastMinute = -1;
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CHoneywell::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	mIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CHoneywell::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}

	mIsStarted = false;
	return true;
}

#define HONEYWELL_POLL_INTERVAL 300 // 5 minutes
#define HWAPITIMEOUT 30 // 30 seconds

//
// worker thread
//
void CHoneywell::Do_Work()
{
	_log.Log(LOG_STATUS, "Honeywell: Worker started...");
	int sec_counter = HONEYWELL_POLL_INTERVAL - 5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % HONEYWELL_POLL_INTERVAL == 0)
		{
			GetThermostatData();
		}
	}
	_log.Log(LOG_STATUS, "Honeywell: Worker stopped...");
}


//
// callback from Domoticz backend to update the Honeywell thermostat
//
bool CHoneywell::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		//Light command

		int nodeID = pCmd->LIGHTING2.id4;
		int devID = nodeID / 10;
		std::string deviceName = mDeviceList[devID]["name"].asString();


		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		if ((nodeID % 10) == 3) {
			// heating on or off
			SetPauseStatus(devID, bIsOn, nodeID);
			return true;
		}

	}
	else if (pCmd->ICMND.packettype == pTypeThermostat && pCmd->LIGHTING2.subtype == sTypeThermSetpoint)
	{
		int nodeID = pCmd->LIGHTING2.id4;
		int devID = nodeID / 10;
		const _tThermostat *therm = reinterpret_cast<const _tThermostat*>(pdata);
		SetSetpoint(devID, therm->temp, nodeID);
	}
	return false;
}

//
// refresh the OAuth2 token through Honeywell API
//
bool CHoneywell::refreshToken()
{
	if (mRefreshToken.empty())
		return false;

	if (mTokenExpires > mytime(NULL))
		return true;

	std::string sResult;

	std::string postData = "grant_type=refresh_token&refresh_token=[refreshToken]";
	stdreplace(postData, "[refreshToken]", mRefreshToken);

	std::string auth = mApiKey;
	auth += ":";
	auth += mApiSecret;
	std::string encodedAuth = base64_encode(auth);


	std::vector<std::string> headers;
	std::string authHeader = "Authorization: [auth]";
	stdreplace(authHeader, "[auth]", encodedAuth);
	headers.push_back(authHeader);
	headers.push_back("Content-Type: application/x-www-form-urlencoded");

	HTTPClient::SetConnectionTimeout(HWAPITIMEOUT);
	HTTPClient::SetTimeout(HWAPITIMEOUT);
	if (!HTTPClient::POST(HONEYWELL_TOKEN_PATH, postData, headers, sResult)) {
		_log.Log(LOG_ERROR, "Honeywell: Error refreshing token");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret) {
		_log.Log(LOG_ERROR, "Honeywell: Invalid/no data received...");
		return false;
	}

	std::string at = root["access_token"].asString();
	std::string rt = root["refresh_token"].asString();
	std::string ei = root["expires_in"].asString();
	if (at.length() && rt.length() && ei.length()) {
		int expires_in = std::stoi(ei);
		mTokenExpires = mytime(NULL) + (expires_in > 0 ? expires_in : 600) - HWAPITIMEOUT;
		mAccessToken = at;
		mRefreshToken = rt;
		_log.Log(LOG_NORM, "Honeywell: Storing received access & refresh token. Token expires after %d seconds.",expires_in);
		m_sql.safe_query("UPDATE Hardware SET Username='%q', Password='%q' WHERE (ID==%d)", mAccessToken.c_str(), mRefreshToken.c_str(), m_HwdID);
		mSessionHeaders.clear();
		mSessionHeaders.push_back("Authorization:Bearer " + mAccessToken);
		mSessionHeaders.push_back("Content-Type: application/json");
	}
	else
		return false;

	return true;
}

//
// Get honeywell data through Honeywell API
//
void CHoneywell::GetThermostatData()
{
	if (!refreshToken())
		return;

	std::string sResult;
	std::string sURL = HONEYWELL_LOCATIONS_PATH;
	stdreplace(sURL, "[apikey]", mApiKey);
	
	HTTPClient::SetConnectionTimeout(HWAPITIMEOUT);
	HTTPClient::SetTimeout(HWAPITIMEOUT);
	if (!HTTPClient::GET(sURL, mSessionHeaders, sResult)) {
		_log.Log(LOG_ERROR, "Honeywell: Error getting thermostat data!");
		return;
	}

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret) {
		_log.Log(LOG_ERROR, "Honeywell: Invalid/no data received...");
		return;
	}

	int devNr = 0;
	mDeviceList.clear();
	mLocationList.clear();
	for (int locCnt = 0; locCnt < (int)root.size(); locCnt++)
	{
		Json::Value location = root[locCnt];
		Json::Value devices = location["devices"];
		for (int devCnt = 0; devCnt < (int)devices.size(); devCnt++)
		{
			Json::Value device = devices[devCnt];
			std::string deviceName = device["name"].asString();
			mDeviceList[devNr] = device;
			mLocationList[devNr] = location["locationID"].asString();

			float temperature;
			temperature = (float)device["indoorTemperature"].asFloat();
			std::string desc = kRoomTempDesc;
			stdreplace(desc, "[devicename]", deviceName);
			SendTempSensor(10 * devNr + 1, 255, temperature, desc);

			temperature = (float)device["outdoorTemperature"].asFloat();
			desc = kOutdoorTempDesc;
			stdreplace(desc, "[devicename]", deviceName);
			SendTempSensor(10 * devNr + 2, 255, temperature, desc);

			std::string mode = device["changeableValues"]["mode"].asString();
			bool bHeating = (mode == "Heat");
			desc = kHeatingDesc;
			stdreplace(desc, "[devicename]", deviceName);
			SendSwitch(10 * devNr + 3, 1, 255, bHeating, 0, desc);

			temperature = (float)device["changeableValues"]["heatSetpoint"].asFloat();
			desc = kHeatSetPointDesc;
			stdreplace(desc, "[devicename]", deviceName);
			SendSetPointSensor((uint8_t)(10 * devNr + 4), temperature, desc);
			devNr++;
			
			std::string operationstatus = device["operationStatus"]["mode"].asString();
			bool bStatus = (operationstatus != "EquipmentOff");
			desc = kOperationStatusDesc;
			stdreplace(desc, "[devicename]", deviceName);
			SendSwitch(10 * devNr + 5, 1, 255, bStatus, 0, desc);
		}
		
		bool geoFenceEnabled = location["geoFenceEnabled"].asBool();
		if (geoFenceEnabled) {
			
			Json::Value geofences = location["geoFences"];
			bool bAway = true;
			for (int geofCnt = 0; geofCnt < (int)geofences.size(); geofCnt++)
			{
				int withinFence = geofences[geofCnt]["geoOccupancy"]["withinFence"].asInt();
				if (withinFence > 0) {
					bAway = false;
					break;
				}
			}
			std::string desc = kAwayDesc;
			stdreplace(desc, "[name]", location["name"].asString());
			SendSwitch(10 * devNr + 6, 1, 255, bAway, 0, desc);
		}
	}
}

//
// send the temperature from honeywell to domoticz backend
//
void CHoneywell::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
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

//
// transfer pause status to Honeywell api
//
void CHoneywell::SetPauseStatus(const int idx, bool bHeating, const int /*nodeid*/)
{
	if (!refreshToken()) {
		_log.Log(LOG_ERROR,"Honeywell: No token available. Failed setting thermostat data");
		return;
	}

	std::string url = HONEYWELL_UPDATE_THERMOSTAT;
	std::string deviceID = mDeviceList[idx]["deviceID"].asString();

	stdreplace(url, "[deviceid]", deviceID);
	stdreplace(url, "[apikey]", mApiKey);
	stdreplace(url, "[locationid]", mLocationList[idx]);

	Json::Value reqRoot;
	reqRoot["mode"] = bHeating ? "Heat" : "Off";
	reqRoot["heatSetpoint"] = mDeviceList[idx]["changeableValues"]["coolHeatpoint"].asInt();
	reqRoot["coolSetpoint"] = mDeviceList[idx]["changeableValues"]["coolSetpoint"].asInt();
	reqRoot["thermostatSetpointStatus"] = "TemporaryHold";
	Json::FastWriter writer;

	std::string sResult;
	HTTPClient::SetConnectionTimeout(HWAPITIMEOUT);
	HTTPClient::SetTimeout(HWAPITIMEOUT);
	if (!HTTPClient::POST(url, writer.write(reqRoot), mSessionHeaders, sResult, true, true)) {
		_log.Log(LOG_ERROR, "Honeywell: Error setting thermostat data!");
		return;
	}

	std::string desc = kHeatingDesc;
	stdreplace(desc, "[devicename]", mDeviceList[idx]["name"].asString());
	SendSwitch(10 * idx + 3, 1, 255, bHeating, 0, desc);
}

//
// transfer the updated temperature to Honeywell API
//
void CHoneywell::SetSetpoint(const int idx, const float temp, const int /*nodeid*/)
{
	if (!refreshToken()) {
		_log.Log(LOG_ERROR, "Honeywell: No token available. Error setting thermostat data!");
		return;
	}

	std::string url = HONEYWELL_UPDATE_THERMOSTAT;
	std::string deviceID = mDeviceList[idx]["deviceID"].asString();

	stdreplace(url, "[deviceid]", deviceID);
	stdreplace(url, "[apikey]", mApiKey);
	stdreplace(url, "[locationid]", mLocationList[idx]);

	Json::Value reqRoot;
	reqRoot["mode"] = "Heat";
	reqRoot["heatSetpoint"] = temp;
	reqRoot["coolSetpoint"] = mDeviceList[idx]["changeableValues"]["coolSetpoint"].asInt();
	reqRoot["thermostatSetpointStatus"] = "TemporaryHold";
	Json::FastWriter writer;

	std::string sResult;
	HTTPClient::SetConnectionTimeout(HWAPITIMEOUT);
	HTTPClient::SetTimeout(HWAPITIMEOUT);
	if (!HTTPClient::POST(url, writer.write(reqRoot), mSessionHeaders, sResult, true, true)) {
		_log.Log(LOG_ERROR, "Honeywell: Error setting thermostat data!");
		return;
	}

	std::string desc = kHeatSetPointDesc;
	stdreplace(desc, "[devicename]", mDeviceList[idx]["name"].asString());
	SendSetPointSensor((uint8_t)(10 * idx + 4), temp, desc);

	desc = kHeatingDesc;
	stdreplace(desc, "[devicename]", mDeviceList[idx]["name"].asString());
	SendSwitch(10 * idx + 3, 1, 255, true, 0, desc);
}
