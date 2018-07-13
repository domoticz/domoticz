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

const std::string HONEYWELL_APIKEY = "atD3jtzXC5z4X8WPbzvo0CBqWi7S81Nh";
const std::string HONEYWELL_APISECRET = "TXDzy2aHpAJw6YiO";
const std::string HONEYWELL_LOCATIONS_PATH = "https://api.honeywell.com/v2/locations?apikey=[apikey]";
const std::string HONEYWELL_UPDATE_THERMOSTAT = "https://api.honeywell.com/v2/devices/thermostats/[deviceid]?apikey=[apikey]&locationId=[locationid]";
const std::string HONEYWELL_TOKEN_PATH = "https://api.honeywell.com/oauth2/token";

const std::string kHeatSetPointDesc = "Target temperature ([devicename])";
const std::string kHeatingDesc = "Heating ([devicename])";
const std::string kOutdoorTempDesc = "Outdoor temperature ([devicename])";
const std::string kRoomTempDesc = "Room temperature ([devicename])";

extern http::server::CWebServerHelper m_webservers;

CHoneywell::CHoneywell(const int ID, const std::string &Username, const std::string &Password)
{
	m_HwdID = ID;
	mAccessToken = Username;
	mRefreshToken = Password;
	stdstring_trim(mAccessToken);
	stdstring_trim(mRefreshToken);
	if (Username.empty() || Password.empty()) {
		_log.Log(LOG_ERROR, "Honeywell: Please update your access token/request token!...");
	}
	Init();
}

CHoneywell::~CHoneywell(void) {
}

void CHoneywell::Init() {
	mStopRequested = false;
	mNeedsTokenRefresh = true;
}

bool CHoneywell::StartHardware() {
	Init();
	mLastMinute = -1;
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CHoneywell::Do_Work, this);
	mIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CHoneywell::StopHardware() {
	if (m_thread)
	{
		mStopRequested = true;
		m_thread->join();
		m_thread.reset();
	}

	mIsStarted = false;
	return true;
}

#define HONEYWELL_POLL_INTERVAL 300 // 5 minutes

//
// worker thread
//
void CHoneywell::Do_Work() {
	_log.Log(LOG_STATUS, "Honeywell: Worker started...");
	int sec_counter = HONEYWELL_POLL_INTERVAL - 5;
	while (!mStopRequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % HONEYWELL_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
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
bool CHoneywell::refreshToken() {
	if (mRefreshToken.empty())
		return false;

	std::string sResult;

	std::string postData = "grant_type=refresh_token&refresh_token=[refreshToken]";
	stdreplace(postData, "[refreshToken]", mRefreshToken);

	std::string auth = HONEYWELL_APIKEY;
	auth += ":";
	auth += HONEYWELL_APISECRET;
	std::string encodedAuth = base64_encode(auth);


	std::vector<std::string> headers;
	std::string authHeader = "Authorization: [auth]";
	stdreplace(authHeader, "[auth]", encodedAuth);
	headers.push_back(authHeader);
	headers.push_back("Content-Type: application/x-www-form-urlencoded");

	if (!HTTPClient::POST(HONEYWELL_TOKEN_PATH, postData, headers, sResult)) {
		_log.Log(LOG_ERROR, "Honeywell: Error refreshing token");
		mNeedsTokenRefresh = true;
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret) {
		_log.Log(LOG_ERROR, "Honeywell: Invalid/no data received...");
		mNeedsTokenRefresh = true;
		return false;
	}

	std::string at = root["access_token"].asString();
	std::string rt = root["refresh_token"].asString();
	if (at.length() && rt.length()) {
		mAccessToken = at;
		mRefreshToken = rt;
		_log.Log(LOG_NORM, "Honeywell: Storing received access & refresh token");
		m_sql.safe_query("UPDATE Hardware SET Username='%q', Password='%q' WHERE (ID==%d)", mAccessToken.c_str(), mRefreshToken.c_str(), m_HwdID);
		mNeedsTokenRefresh = false;
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
void CHoneywell::GetMeterDetails() {

	if (mNeedsTokenRefresh) {
		if (!refreshToken())
			return;
	}

	std::string sResult;
	std::string sURL = HONEYWELL_LOCATIONS_PATH;
	stdreplace(sURL, "[apikey]", HONEYWELL_APIKEY);
	if (!HTTPClient::GET(sURL, mSessionHeaders, sResult)) {
		_log.Log(LOG_ERROR, "Honeywell: Error getting thermostat data!");
		mNeedsTokenRefresh = true;
		return;
	}

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret) {
		_log.Log(LOG_ERROR, "Honeywell: Invalid/no data received...");
		mNeedsTokenRefresh = true;
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
	std::string url = HONEYWELL_UPDATE_THERMOSTAT;
	std::string deviceID = mDeviceList[idx]["deviceID"].asString();

	stdreplace(url, "[deviceid]", deviceID);
	stdreplace(url, "[apikey]", HONEYWELL_APIKEY);
	stdreplace(url, "[locationid]", mLocationList[idx]);

	Json::Value reqRoot;
	reqRoot["mode"] = bHeating ? "Heat" : "Off";
	reqRoot["heatSetpoint"] = mDeviceList[idx]["changeableValues"]["coolHeatpoint"].asInt();
	reqRoot["coolSetpoint"] = mDeviceList[idx]["changeableValues"]["coolSetpoint"].asInt();
	reqRoot["thermostatSetpointStatus"] = "TemporaryHold";
	Json::FastWriter writer;

	std::string sResult;
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
	std::string url = HONEYWELL_UPDATE_THERMOSTAT;
	std::string deviceID = mDeviceList[idx]["deviceID"].asString();

	stdreplace(url, "[deviceid]", deviceID);
	stdreplace(url, "[apikey]", HONEYWELL_APIKEY);
	stdreplace(url, "[locationid]", mLocationList[idx]);

	Json::Value reqRoot;
	reqRoot["mode"] = "Heat";
	reqRoot["heatSetpoint"] = temp;
	reqRoot["coolSetpoint"] = mDeviceList[idx]["changeableValues"]["coolSetpoint"].asInt();
	reqRoot["thermostatSetpointStatus"] = "TemporaryHold";
	Json::FastWriter writer;

	std::string sResult;
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
