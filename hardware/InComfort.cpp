#include "stdafx.h"
#include "InComfort.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
//#define DEBUG_InComfort
#endif

CInComfort::CInComfort(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_stoprequested = false;

	m_LastUpdateFrequentChangingValues = 0;
	m_LastUpdateSlowChangingValues = 0;
	m_LastRoom1Temperature = 0.0;
	m_LastRoom2Temperature = 0.0;
	m_LastRoom1SetTemperature = 0.0;
	m_LastRoom1OverrideTemperature = 0.0;
	m_LastRoom2SetTemperature = 0.0;
	m_LastRoom2OverrideTemperature = 0.0;
	m_LastCentralHeatingTemperature = 0.0;
	m_LastCentralHeatingPressure = 0.0;
	m_LastTapWaterTemperature = 0.0;
	m_LastIO = 0;

	Init();
}

CInComfort::~CInComfort(void)
{
}

void CInComfort::Init()
{
	m_stoprequested = false;
}

bool CInComfort::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CInComfort::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

bool CInComfort::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
m_stoprequested = true;
m_thread->join();
	}
	m_bIsStarted = false;
	return true;
}

#define INCOMFORT_POLL_INTERVAL 30

void CInComfort::Do_Work()
{
	int sec_counter = 0;
	_log.Log(LOG_STATUS, "InComfort: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
		if (sec_counter%INCOMFORT_POLL_INTERVAL == 0)
		{
			GetHeaterDetails();
		}
	}
	_log.Log(LOG_STATUS, "InComfort: Worker stopped...");
}

bool CInComfort::WriteToHardware(const char *pdata, const unsigned char length)
{
	return true;
}


void CInComfort::SetSetpoint(const int idx, const float temp)
{
	_log.Log(LOG_NORM, "InComfort: Setpoint of sensor with idx idx changed to temp");
	std::string jsonData = SetRoom1SetTemperature(temp);
	if (jsonData.length() > 0)
		ParseAndUpdateDevices(jsonData);
}

std::string CInComfort::GetHTTPData(std::string sURL)
{
	// Get Data
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "InComfort: Error getting current state!");
	}
	return sResult;
}

std::string CInComfort::SetRoom1SetTemperature(float tempSetpoint)
{
	float setpointToSet = (tempSetpoint - 5.0f) * 10.0f;

	std::stringstream sstr;
	sstr << "http://" << m_szIPAddress << ":" << m_usIPPort << "/data.json?heater=0&setpoint=" << setpointToSet << "&thermostat=0";

	return GetHTTPData(sstr.str());
}

void CInComfort::GetHeaterDetails()
{
	if (m_szIPAddress.empty())
		return;

	std::stringstream sstr;
	sstr << "http://" << m_szIPAddress << ":" << m_usIPPort << "/data.json";

	// Get Data
	std::string sResult = GetHTTPData(sstr.str());
	if (sResult.empty())
	{
		_log.Log(LOG_ERROR, "InComfort: Error getting current state!");
		return;
	}
	ParseAndUpdateDevices(sResult);
}

void CInComfort::SetProgramState(const int newState)
{
}

void CInComfort::ParseAndUpdateDevices(std::string jsonData)
{
	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(jsonData, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "InComfort: Invalid data received. Data is not json formatted.");
		return;
	}
	if (root["nodenr"].empty() == true)
	{
		_log.Log(LOG_ERROR, "InComfort: Invalid data received. Nodenr not found.");
		return;
	}

	float room1Temperature = (root["room_temp_1_lsb"].asInt() + root["room_temp_1_msb"].asInt() * 256) / 100.0f;
	float room1SetTemperature = (root["room_temp_set_1_lsb"].asInt() + root["room_temp_set_1_msb"].asInt() * 256) / 100.0f;
	float room1OverrideTemperature = (root["room_set_ovr_1_lsb"].asInt() + root["room_set_ovr_1_msb"].asInt() * 256) / 100.0f;
	float room2Temperature = (root["room_temp_2_lsb"].asInt() + root["room_temp_2_msb"].asInt() * 256) / 100.0f;
	float room2SetTemperature = (root["room_temp_set_2_lsb"].asInt() + root["room_temp_set_2_msb"].asInt() * 256) / 100.0f;
	float room2OverrideTemperature = (root["room_set_ovr_2_lsb"].asInt() + root["room_set_ovr_2_msb"].asInt() * 256) / 100.0f;

	int statusDisplayCode = root["displ_code"].asInt();
	int rssi = root["rf_message_rssi"].asInt();
	int rfStatusCounter = root["rfstatus_cntr"].asInt();

	float centralHeatingTemperature = (root["ch_temp_lsb"].asInt() + root["ch_temp_msb"].asInt() * 256) / 100.0f;
	float centralHeatingPressure = (root["ch_pressure_lsb"].asInt() + root["ch_pressure_msb"].asInt() * 256) / 100.0f;
	float tapWaterTemperature = (root["tap_temp_lsb"].asInt() + root["tap_temp_msb"].asInt() * 256) / 100.0f;

	int io = root["IO"].asInt();
	bool lockout = (io & 0x01) > 0;

	std::string statusText;
	if (lockout)
	{
		std::stringstream ss;
		ss << "Error: " << statusDisplayCode;
		statusText = ss.str();
	}
	else
		switch (statusDisplayCode)
		{
		case 85: statusText = "Sensortest"; break;
		case 170: statusText = "Service"; break;
		case 204: statusText = "Tapwater"; break;
		case 51: statusText = "Tapwater Int."; break;
		case 240: statusText = "Boiler Int."; break;
		case 15: statusText = "Boiler Ext."; break;
		case 153: statusText = "Postrun Boiler"; break;
		case 102: statusText = "Central Heating"; break;
		case 0: statusText = "OpenTherm"; break;
		case 255: statusText = "Buffer"; break;
		case 24: statusText = "Frost"; break;
		case 231: statusText = "Postrun Central Heating"; break;
		case 126: statusText = "Standby"; break;
		case 37: statusText = "Central Heating RF"; break;
		default: statusText = "Unknown"; break;
		}


	// Compare the time of the last update to the current time. 
	// For items changing frequently, update the value every 5 minutes, for all others update every 15 minutes
	time_t currentTime = mytime(NULL);
	bool updateFrequentChangingValues = (currentTime - m_LastUpdateFrequentChangingValues) >= 300;
	if (updateFrequentChangingValues)
		m_LastUpdateFrequentChangingValues = currentTime;
	bool updateSlowChangingValues = (currentTime - m_LastUpdateSlowChangingValues) >= 900;
	if (updateSlowChangingValues)
		m_LastUpdateSlowChangingValues = currentTime;

	// Update all sensors, if they changed or if the need an update based on Time
	if ((m_LastRoom1Temperature != room1Temperature) || updateFrequentChangingValues)
	{
		m_LastRoom1Temperature = room1Temperature;
		SendTempSensor(0, 255, m_LastRoom1Temperature, "Room Temperature");
	}
	if ((m_LastRoom1SetTemperature != room1SetTemperature) || updateSlowChangingValues)
	{
		m_LastRoom1SetTemperature = room1SetTemperature;
		SendTempSensor(2, 255, m_LastRoom1SetTemperature, "Room Thermostat Setpoint");
	}
	if ((m_LastRoom1OverrideTemperature != room1OverrideTemperature) || updateSlowChangingValues)
	{
		m_LastRoom1OverrideTemperature = room1OverrideTemperature;
		SendSetPointSensor(3, 1, 0, m_LastRoom1OverrideTemperature, "Room Override Setpoint");
	}

	// room2temperature is 300+ the room 2 is not configured/in use in the LAN2RF gateway.
	if (room2Temperature < 100.0)
	{
		if ((m_LastRoom2Temperature != room2Temperature) || updateFrequentChangingValues)
		{
			m_LastRoom2Temperature = room2Temperature;
			SendTempSensor(1, 255, m_LastRoom2Temperature, "Room-2 Temperature");
		}
		if ((m_LastRoom2SetTemperature != room2SetTemperature) || updateSlowChangingValues)
		{
			m_LastRoom2SetTemperature = room2SetTemperature;
			SendTempSensor(4, 255, m_LastRoom2SetTemperature, "Room-2 Thermostat Setpoint");
		}
		if ((m_LastRoom2OverrideTemperature != room2OverrideTemperature) || updateSlowChangingValues)
		{
			m_LastRoom2OverrideTemperature = room2OverrideTemperature;
			SendSetPointSensor(5, 0, 3, m_LastRoom2OverrideTemperature, "Room-2 Override Setpoint");
		}
	}

	if ((m_LastCentralHeatingTemperature != centralHeatingTemperature) || updateFrequentChangingValues)
	{
		m_LastCentralHeatingTemperature = centralHeatingTemperature;
		SendTempSensor(4, 255, m_LastCentralHeatingTemperature, "Central Heating Water Temperature");
	}
	if ((m_LastCentralHeatingPressure != centralHeatingPressure) || updateFrequentChangingValues)
	{
		m_LastCentralHeatingPressure = centralHeatingPressure;
		SendPressureSensor(5, 0, 255, m_LastCentralHeatingPressure, "Central Heating Water Pressure");
	}
	if ((m_LastTapWaterTemperature != tapWaterTemperature) || updateFrequentChangingValues)
	{
		m_LastTapWaterTemperature = tapWaterTemperature;
		SendTempSensor(6, 255, m_LastTapWaterTemperature, "Tap Water Temperature");
	}
	if ((m_LastIO != io) || (m_LastStatusText != statusText) || updateFrequentChangingValues)
	{
		m_LastIO = io;
		SendTextSensor(8, 0, 255, statusText, "Heater Status");
		bool pumpActive = (io & 0x02) > 0;
		bool tapFunctionActive = (io & 0x04) > 0;
		bool burnerActive = (io & 0x08) > 0;
		SendSwitch(8, 1, 255, pumpActive, 0, "Pump Active");
		SendSwitch(8, 2, 255, tapFunctionActive, 0, "Tap Function Active");
		SendSwitch(8, 3, 255, burnerActive, 0, "Burner Active");
	}
}
