#include "stdafx.h"
#include <stdio.h>
#include "GoodweAPI.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#define GOODWE_HOST_GLOBAL "https://hk.goodwe-power.com/"
#define GOODWE_HOST_EU "https://eu.goodwe-power.com/"
#define GOODWE_HOST_AU "https://au.goodwe-power.com/"

#define GOODWE_BY_USER_PATH "Mobile/GetMyPowerStationByUser?userName="
#define GOODWE_BY_STATION_PATH "Mobile/GetMyPowerStationById?stationId="
#define GOODWE_DEVICE_LIST_PATH "Mobile/GetMyDeviceListById?stationId="
#define GOODWE_DEVICE_LISTV3_PATH "Mobile/GetMyDeviceListById_V3?stationId="

// parameter names for GetMyPowerStationByUser

#define BY_USER_STATION_ID "stationId"
#define BY_USER_STATION_NAME "stationName"
#define BY_USER_CURRENT_POWER_KW "currentPower"
#define BY_USER_DAY_TOTAL_KWH "value_eDayTotal"
#define BY_USER_TOTAL_KWH "value_eTotal"

// parameter names for GetMyPowerStationById

#define BY_STATION_CURRENT_POWER_KW "curpower"
#define BY_STATION_STATUS "status"
#define BY_STATION_DAY_TOTAL_KWH "eday"
#define BY_STATION_TOTAL_KWH "etotal"

// parameter names for GetMyDeviceListById

#define DEVICE_SERIAL "inventersn"
#define DEVICE_CURRENT_POWER_W "power"
#define DEVICE_STATUS "status"
#define DEVICE_DAY_TOTAL_KWH "eday"
#define DEVICE_TOTAL_KWH "etotal"
#define DEVICE_ERROR_MSG "errormsg"

// Paramers added for GetMyDeviceListById_v3
#define DEVICE_RESULT "result"
#define DEVICE_VOLTAGE_STRING1 "vpv1"
#define DEVICE_VOLTAGE_STRING2 "vpv2"
#define DEVICE_CURRENT_STRING1 "ipv1"
#define DEVICE_CURRENT_STRING2 "ipv2"
#define DEVICE_VOLTAGE_PHASE1 "vac1"
#define DEVICE_VOLTAGE_PHASE2 "vac2"
#define DEVICE_VOLTAGE_PHASE3 "vac3"
#define DEVICE_CURRENT_PHASE1 "iac1"
#define DEVICE_CURRENT_PHASE2 "iac2"
#define DEVICE_CURRENT_PHASE3 "iac3"

// Status values
#define STATUS_WAITING 0
#define STATUS_NORMAL 1
// 2 is unknown 
#define STATUS_OFFLINE 3

// Child Indexes 
#define IDX_KWH 0
#define IDX_STATUS 1
#define IDX_ERRORMSG 2
#define IDX_VOLT_L1 3
#define IDX_VOLT_L2 4
#define IDX_VOLT_L3 5
#define IDX_VOLT_S1 6
#define IDX_VOLT_S2 7
#define IDX_CUR_L1 8
#define IDX_CUR_L2 9
#define IDX_CUR_L3 10
#define IDX_CUR_S1 11
#define IDX_CUR_S2 12


enum _eGoodweLocation {
	GOODWE_LOCATION_GLOBAL= 0,      // Global server
	GOODWE_LOCATION_OCEANIA = 1,    // Australian server
	GOODWE_LOCATION_EUROPE = 2      // European server
};

#ifdef _DEBUG
	//#define DEBUG_GoodweAPIW 1
#endif


#ifdef DEBUG_GoodweAPIW
void SaveString2Disk(const std::string &str, const std::string &filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif

GoodweAPI::GoodweAPI(const int ID, const std::string &userName, const int ServerLocation):
	m_UserName(userName)
{
	m_HwdID=ID;
	switch ((_eGoodweLocation)ServerLocation)
	{
		case GOODWE_LOCATION_EUROPE:
			m_Host = GOODWE_HOST_EU;
			break;
		case GOODWE_LOCATION_OCEANIA:
			m_Host = GOODWE_HOST_AU;
			break;
		default:
			m_Host = GOODWE_HOST_GLOBAL;
			break;
	}
	Init();
}

void GoodweAPI::Init()
{
}

bool GoodweAPI::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&GoodweAPI::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool GoodweAPI::StopHardware()
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

void GoodweAPI::Do_Work()
{
	_log.Log(LOG_STATUS, "GoodweAPI Worker started, using server URL %s...", m_Host.c_str());
	int sec_counter = 295;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % 300 == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"GoodweAPI Worker stopped...");
}

bool GoodweAPI::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void GoodweAPI::SendCurrentSensor(const int NodeID, const uint8_t ChildID, const int /*BatteryLevel*/, const float Amp, const std::string &defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = ChildID;
	gDevice.intval1 = (NodeID << 8) | ChildID;
	gDevice.floatval1 = Amp;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice, defaultname.c_str(), 255, nullptr);
}

int GoodweAPI::getSunRiseSunSetMinutes(const bool bGetSunRise)
{
	std::vector<std::string> strarray;
	std::vector<std::string> sunRisearray;
	std::vector<std::string> sunSetarray;

	if (!m_mainworker.m_LastSunriseSet.empty())
	{
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
		StringSplit(strarray[0], ":", sunRisearray);
		StringSplit(strarray[1], ":", sunSetarray);

		int sunRiseInMinutes = (atoi(sunRisearray[0].c_str()) * 60) + atoi(sunRisearray[1].c_str());
		int sunSetInMinutes = (atoi(sunSetarray[0].c_str()) * 60) + atoi(sunSetarray[1].c_str());

		if (bGetSunRise) {
			return sunRiseInMinutes;
		}

		return sunSetInMinutes;
	}
	return 0;
}

bool GoodweAPI::GoodweServerClient(const std::string &sPath, std::string &sResult)
{
	return HTTPClient::GET(m_Host + sPath, sResult);
}

uint32_t GoodweAPI::hash(const std::string &str)
{
	/*
	 * We need a way to generate the NodeId from the stationID
	 * and the ChildID from device serial.
	 * This hash is definitely not perfect as we reduce the 128 bit
         * stationID to an int (normally 32 bits).
	 * But as almost all people will have only a very limited number of
	 * PV-converters, the risk for collisions should be low enough
	 * The djb2 hash is taken from http://www.cse.yorku.ca/~oz/hash.html
	 */

	long hash = 5381;
	size_t i = 0;

	for (i = 0; i < str.size(); i++)
	{
		int c = str[i++];
		hash = ((hash << 5) + hash) + c;
	}
	return (uint32_t)hash;
}

bool GoodweAPI::getValueFromJson(const Json::Value &inputValue, std::string &outputValue, const std::string &errorString)
{
	if (inputValue.empty()) {
 		_log.Log(LOG_ERROR,"GoodweAPI: invalid device data received; %s missing!", errorString.c_str());
		return false;
	}
	outputValue = inputValue.asString();
	return true;
}

bool GoodweAPI::getValueFromJson(const Json::Value &inputValue, float &outputValue, const std::string &errorString)
{
	if (inputValue.empty()) {
		_log.Log(LOG_ERROR,"GoodweAPI: invalid device data received; %s missing!", errorString.c_str());
		return false;
	}
	std::string tempStr = inputValue.asString();
	std::stringstream input;
	input << tempStr;
	input >> outputValue;
	return true;
}

bool GoodweAPI::getValueFromJson(const Json::Value &inputValue, int &outputValue, const std::string &errorString)
{
	if (inputValue.empty()) {
		_log.Log(LOG_ERROR,"GoodweAPI: invalid device data received; %s missing!", errorString.c_str());
		return false;
	}
	std::string tempStr = inputValue.asString();
	std::stringstream input;
	input << tempStr;
	input >> outputValue;
	return true;
}

std::string getStatusString(const int status)
{
	switch(status) {
	case STATUS_WAITING: return "Waiting for the Sun";
	case STATUS_NORMAL: return "Normal/ Working";
	case STATUS_OFFLINE: return "Offline";
	default:
		return "Unkown status value " + std::to_string(status);
	}
}


void GoodweAPI::GetMeterDetails()
{
	std::string sResult;
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	// Only poll one hour before sunrise till two hours after sunset

	if (ActHourMin + 60 < sunRise)
		return;
	if (ActHourMin - 120 > sunSet)
		return;

	std::string sPATH = GOODWE_BY_USER_PATH + m_UserName;

	bool bret = GoodweServerClient(sPATH, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Error getting http user data!");
		return;
	}

#ifdef DEBUG_GoodweAPIW
	SaveString2Disk(sResult, "/tmp/Goodwe2.json");
#endif
	Json::Value root;
	bool ret= ParseJSon(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"GoodweAPI: Invalid user data received!");
		return;
	}
	if (root.empty())
	{
		_log.Log(LOG_ERROR,"GoodweAPI: Invalid user data received, or invalid username");
		return;
	}
	for (auto &i : root)
	{
		// We use the received data only to retrieve station-id and station-name

		if (i[BY_USER_STATION_ID].empty())
		{
			_log.Log(LOG_ERROR, "GoodweAPI: no or invalid data received - StationID is missing!");
			return;
		}
		if (i[BY_USER_STATION_NAME].empty())
		{
			_log.Log(LOG_ERROR, "GoodweAPI: invalid data received - stationName is missing!");
			return;
		}
		std::string sStationId = i[BY_USER_STATION_ID].asString();
		std::string sStationName = i[BY_USER_STATION_NAME].asString();

		ParseDeviceList(sStationId, sStationName);
	}
}

void GoodweAPI::ParseDeviceList(const std::string &sStationId, const std::string &sStationName)
{
	// fetch inverter list

	std::string sPATH = GOODWE_DEVICE_LISTV3_PATH + sStationId;
	bool bret;
	std::string sResult;

	bret = GoodweServerClient(sPATH, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Error getting http data for device list !");
		return;
	}
#ifdef DEBUG_GoodweAPIW
	SaveString2Disk(sResult, "/tmp/Goodwe4.json");
#endif

	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "GoodweAPI: Invalid device list!");
		return;
	}

	Json::Value result;
	result = root[DEVICE_RESULT];
	if (result.empty())
	{
		_log.Log(LOG_STATUS, "GoodweAPI: devicelist result is empty!");
		return;
	}

	for (auto &i : result)
	{
		ParseDevice(i, sStationId, sStationName);
	}
}

void GoodweAPI::ParseDevice(const Json::Value &device, const std::string &sStationId, const std::string &sStationName)
{
	std::string sDeviceSerial;
	std::string sCurrentPower;
	std::string sTotalEnergyKWh;
	std::string sErrorMsg;
	int iStatus;
	float fCurrentPowerW;
	float fTotalEnergyKWh;
	float fVoltageString1;
	float fVoltageString2;
	float fCurrentString1;
	float fCurrentString2;
	float fVoltagePhase1;
	float fVoltagePhase2;
	float fVoltagePhase3;
	float fCurrentPhase1;
	float fCurrentPhase2;
	float fCurrentPhase3;

	// Parse received JSON
	
	if (!getValueFromJson( device[DEVICE_SERIAL], sDeviceSerial, "Inverter Serial Number") |
	    !getValueFromJson( device[DEVICE_CURRENT_POWER_W], fCurrentPowerW, "Current Power") |
	    !getValueFromJson( device[DEVICE_STATUS], iStatus, "Device Status") |
	    !getValueFromJson( device[DEVICE_TOTAL_KWH], fTotalEnergyKWh, "Total Energy Produced") |
	    !getValueFromJson( device[DEVICE_ERROR_MSG], sErrorMsg, "Error Message")|
	    !getValueFromJson( device[DEVICE_VOLTAGE_STRING1], fVoltageString1, "Voltage String 1") |
	    !getValueFromJson( device[DEVICE_VOLTAGE_STRING2], fVoltageString2, "Voltage String 2") |
	    !getValueFromJson( device[DEVICE_CURRENT_STRING1], fCurrentString1, "Current String 1") |
	    !getValueFromJson( device[DEVICE_CURRENT_STRING2], fCurrentString2, "Current String 2") |
	    !getValueFromJson( device[DEVICE_VOLTAGE_PHASE1], fVoltagePhase1, "Voltage Phase 1") |
	    !getValueFromJson( device[DEVICE_VOLTAGE_PHASE2], fVoltagePhase2, "Voltage Phase 2") |
	    !getValueFromJson( device[DEVICE_VOLTAGE_PHASE3], fVoltagePhase3, "Voltage Phase 3") |
	    !getValueFromJson( device[DEVICE_CURRENT_PHASE1], fCurrentPhase1, "Current Phase 1") |
	    !getValueFromJson( device[DEVICE_CURRENT_PHASE2], fCurrentPhase2, "Current Phase 2") |
	    !getValueFromJson( device[DEVICE_CURRENT_PHASE3], fCurrentPhase3, "Current Phase 3") ) {
		
		// Error parsing message, return
		return;
	}

	// Create NodeID and ChildID from station id and device serial

	uint32_t NodeID = hash(sStationId);
	uint32_t ChildID = hash(sDeviceSerial);

	// reserve childIDs  0 - 10 for the station
	if (ChildID <= 10) {
		ChildID = ChildID + 10;
	}

	// do not send meter values when status is not normal (meter values are 0 when offline)
	// It is unknown if other cases exist where 0 values are returned, so we only send 
	// values when status is normal
	
	if (iStatus == STATUS_NORMAL)
        {
		SendKwhMeter(NodeID, ChildID + IDX_KWH, 255, fCurrentPowerW, fTotalEnergyKWh, 
			sStationName + " " + sDeviceSerial + " Return");
	}
	SendTextSensor(NodeID, ChildID + IDX_STATUS , 255, getStatusString(iStatus), 
		sStationName + " " + sDeviceSerial + " status");
	SendTextSensor(NodeID, ChildID + IDX_ERRORMSG , 255, sErrorMsg, 
		sStationName + " " + sDeviceSerial + " error");
	SendVoltageSensor(NodeID, ChildID + IDX_VOLT_L1, 255, fVoltagePhase1, 
		sStationName + " " + sDeviceSerial + " Mains L1");
	SendCurrentSensor(NodeID, (uint8_t)ChildID + IDX_CUR_L1, 255, fCurrentPhase1,
		sStationName + " " + sDeviceSerial + " Mains L1");

	// Send data for L2 and L3 only when we detect a voltage

	if (fVoltagePhase2 > 0.1F)
	{
		SendVoltageSensor(NodeID, ChildID + IDX_VOLT_L2, 255, fVoltagePhase2, 
			sStationName + " " + sDeviceSerial + " Mains L2");
		SendCurrentSensor(NodeID, (uint8_t)ChildID + IDX_CUR_L2, 255, fCurrentPhase2,
			sStationName + " " + sDeviceSerial + " Mains L2");
	}
	if (fVoltagePhase3 > 0.1F)
	{
		SendVoltageSensor(NodeID, ChildID + IDX_VOLT_L3, 255, fVoltagePhase3, 
			sStationName + " " + sDeviceSerial + " Mains L3");
		SendCurrentSensor(NodeID, (uint8_t)ChildID + IDX_CUR_L3, 255, fCurrentPhase3,
			sStationName + " " + sDeviceSerial + " Mains L3");
	}

	SendVoltageSensor(NodeID, ChildID + IDX_VOLT_S1, 255, fVoltageString1, 
		sStationName + " " + sDeviceSerial + "Input string 1");
	SendCurrentSensor(NodeID, (uint8_t)ChildID + IDX_CUR_S1, 255, fCurrentString1,
		sStationName + " " + sDeviceSerial + " Input String 1");

	// Send data for string 2 only when we detect a voltage

	if (fVoltageString2 > 0.1F)
	{
		SendVoltageSensor(NodeID, ChildID + IDX_VOLT_S2, 255, fVoltageString2, 
			sStationName + " " + sDeviceSerial + "Input string 2");
		SendCurrentSensor(NodeID, (uint8_t)ChildID + IDX_CUR_S2, 255, fCurrentString2,
			sStationName + " " + sDeviceSerial + " Input String 2");
	}
}
