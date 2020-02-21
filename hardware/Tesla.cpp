#include "stdafx.h"
#include "Tesla.h"
#include "TeslaApi.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include <sstream>
#include <iomanip>
#include <cmath>

#define TESLA_SWITCH_CHARGE 1
#define TESLA_SWITCH_CLIMATE 2
#define TESLA_SWITCH_DEFROST 3

#define TESLA_TEMP_CLIMATE 1
#define TESLA_ALERT_STATUS 1
#define TESLA_LEVEL_BATTERY 1

#define TESLA_MAXTRIES 5
#define TESLA_MAXIDLE 30

CTesla::CTesla(const int ID, const std::string& username, const std::string& password, const std::string& VIN) :
	m_username(username),
	m_password(password)
{
	m_loggedin = false;
	m_HwdID = ID;
	m_car.VIN = VIN;
	Init();
	m_commands.clear();
}

CTesla::~CTesla(void)
{
}

void CTesla::Init()
{
	m_car.charging = false;
	m_car.connected = false;
	m_car.is_home = false;
	m_car.climate_on = false;
	m_car.defrost = false;
	m_car.awake = false;
	m_trycounter = 0;
}

bool CTesla::Reset(std::string errortext)
{
	if(m_trycounter > TESLA_MAXTRIES)
	{
		Init();
		SendSwitch(TESLA_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
		SendSwitch(TESLA_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
		SendSwitch(TESLA_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
		m_commands.clear();
		Log(LOG_ERROR, "Multiple tries requesting %s. Assuming car offline.", errortext.c_str());
		SendAlertSensor(TESLA_ALERT_STATUS, 255, Offline, "Offline", m_Name + " State");
		return(false);
	}
	else
	{
		Log(LOG_ERROR, "Timeout requesting %s. Ignoring.", errortext.c_str());
		m_trycounter++;
	}

	return(true);
}

bool CTesla::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CTesla::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	if (!m_thread)
		return false;
	m_bIsStarted = true;
	sOnConnected(this);
	return true;
}

bool CTesla::StopHardware()
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

std::string CTesla::GetCommandString(const eCommandType command)
{
	switch (command)
	{
	case Send_Climate_Off:
		return("Switch Climate off");
	case Send_Climate_Comfort:
		return("Switch Climate on");
	case Send_Climate_Defrost:
		return("Switch Defrost mode on");
	case Send_Climate_Defrost_Off:
		return("Switch Defrost mode off");
	case Send_Charge_Start:
		return("Start charging");
	case Send_Charge_Stop:
		return("Stop charging");
	case Get_Climate:
		return("Get Climate status");
	case Get_Charge:
		return("Get Charge status");
	case Get_Drive:
		return("Get Drive status");
	default:
		return "";
	}
}

void CTesla::Do_Work()
{
	int sec_counter = 593;
	int sec_counter_after_active = 0;
	bool waking_up = false;
	int interval = 1000;
	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(interval))
	{
		interval = 1000;
		sec_counter++;
		time_t now = mytime(0);
		m_LastHeartbeat = now;

		// Only login if we should.
		if (!m_loggedin)
		{
			m_loggedin = m_api.Login(m_username, m_password, m_car.VIN);
			continue;
		}

		if (sec_counter % 68400 == 0)
		{
			m_loggedin = m_api.RefreshLogin();
			sec_counter = 593;
			continue;
		}

		// waking up the car if needed until it's awake
		if (waking_up)
		{
			if (WakeUp())
			{
				interval = 5000;
				if(m_car.awake)
				{
					sec_counter_after_active = 0;
					waking_up = false;
				}
				continue;
			}
			else
			{
				waking_up = false;
				interval = 1000;
				continue;
			}
		}

		// if commands scheduled, wake up car and execute commands
		if (!m_commands.empty())
		{
			if (m_car.awake)
			{
				// car is awake, execute command
				eCommandType item;
				m_commands.try_pop(item);
				if(DoCommand(item))
				{
					interval = 5000;
					sec_counter_after_active = 5;
					m_trycounter = 0;
				}
				else
				{
					interval = 1000;
					sec_counter_after_active = 1;
				}
			}
			else
			{
				// car should wake up first
				waking_up = true;
			}
		}
		else
		{
			// if no more commands scheduled, assume car goes back to sleep after 30 seconds
			sec_counter_after_active++;
			if (sec_counter_after_active > TESLA_MAXIDLE)
				m_car.awake = false;
		}

		// now schedule timed commands
		if (sec_counter % 600 == 0)
		{
			// check all statuses every 10 minutes
			m_commands.push(Get_Drive);
			m_commands.push(Get_Charge);
			m_commands.push(Get_Climate);
		}
		else if (sec_counter % 60 == 0)
		{
			if (m_car.charging || m_car.climate_on || m_car.defrost)
			{
				// check relevant statuses every minute
				m_commands.push(Get_Drive);
				if (m_car.charging)
					m_commands.push(Get_Charge);
				if (m_car.climate_on || m_car.defrost)
					m_commands.push(Get_Climate);
			}
		}
		else
		{
			;
		}
	}
	
	Log(LOG_STATUS, "Worker stopped...");
}

bool CTesla::WriteToHardware(const char* pdata, const unsigned char length)
{
	if (!m_loggedin)
		return false;

	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);

	m_commands.push(Get_Drive);
	switch (pCmd->LIGHTING2.id4)
	{
	case TESLA_SWITCH_CHARGE:
		m_commands.push(Get_Charge);
		if (bIsOn)
			m_commands.push(Send_Charge_Start);
		else
			m_commands.push(Send_Charge_Stop);
		break;
	case TESLA_SWITCH_CLIMATE:
		if (bIsOn)
			m_commands.push(Send_Climate_Comfort);
		else
			m_commands.push(Send_Climate_Off);
		break;
	case TESLA_SWITCH_DEFROST:
		if (bIsOn)
			m_commands.push(Send_Climate_Defrost);
		else
			m_commands.push(Send_Climate_Defrost_Off);
		break;
	default:
		Log(LOG_ERROR, "Unknown switch %d", pCmd->LIGHTING2.id4);
		return false;
	}

	Debug(DEBUG_NORM, "Write to hardware called: on: %d", bIsOn);
	return true;
}

bool CTesla::WakeUp()
{
	if (m_trycounter == 0)
		Log(LOG_STATUS, "Waking up car.");

	if (m_trycounter > 0 && m_car.is_home)
		SendAlertSensor(TESLA_ALERT_STATUS, 255, Offline, "At home, Waking up", m_Name + " State");	

	if (m_api.SendCommand(CTeslaApi::Wake_Up))
	{
		m_car.awake = true;
		if (m_trycounter > 0 && m_car.is_home)
			SendAlertSensor(TESLA_ALERT_STATUS, 255, Home, "At home", m_Name + " State");
		m_trycounter = 0;
		return true;
	}

	return Reset("wakeup");
}

bool CTesla::DoCommand(eCommandType command)
{
	Log(LOG_STATUS, "Executing command: %s", GetCommandString(command).c_str());

	switch (command)
	{
	case Send_Climate_Off:
	case Send_Climate_Comfort:
	case Send_Climate_Defrost:
	case Send_Climate_Defrost_Off:
	case Send_Charge_Start:
	case Send_Charge_Stop:
		return DoSetCommand(command);
		break;
	case Get_Climate:
		return GetClimateStatus();
		break;
	case Get_Charge:
		return GetChargeStatus();
		break;
	case Get_Drive:
		return GetDriveStatus();
		break;
	default:
		return false;
	}

	return false;
}

bool CTesla::DoSetCommand(eCommandType command)
{
	Json::Value reply;
	CTeslaApi::eCommandType api_command;

	if (!m_car.is_home)
	{
		Log(LOG_ERROR, "Car not home. No commands allowed.");
		SendSwitch(TESLA_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
		SendSwitch(TESLA_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
		SendSwitch(TESLA_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
		return true;
	}

	switch (command)
	{
	case Send_Charge_Start:
		if (!m_car.connected)
		{
			Log(LOG_ERROR, "Charge cable not connected. No charge commands possible.");
			SendSwitch(TESLA_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
			return false;
		}
		api_command = CTeslaApi::Charge_Start;
		break;
	case Send_Charge_Stop:
		if (!m_car.connected)
		{
			Log(LOG_ERROR, "Charge cable not connected. No charge commands possible.");
			SendSwitch(TESLA_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
			return false;
		}
		api_command = CTeslaApi::Charge_Stop;
		break;
	case Send_Climate_Off:
		api_command = CTeslaApi::Climate_Off;
		break;
	case Send_Climate_Comfort:
		api_command = CTeslaApi::Climate_Comfort;
		break;
	case Send_Climate_Defrost:
		api_command = CTeslaApi::Climate_Defrost;
		break;
	case Send_Climate_Defrost_Off:
		api_command = CTeslaApi::Climate_Defrost_Off;
		break;
	}

	if (m_api.SendCommand(api_command))
	{
		switch (command)
		{
		case Send_Charge_Start:
		case Send_Charge_Stop:
			m_commands.push(Get_Charge);
			m_car.charging = true; // if check after command fails, this will result in fast retry
			return true;
		case Send_Climate_Off:
		case Send_Climate_Comfort:
			m_commands.push(Get_Climate);
			m_car.climate_on = true; // if check after command fails, this will result in fast retry
			return true;
		case Send_Climate_Defrost:
		case Send_Climate_Defrost_Off:
			m_car.defrost = true; // if check after command fails, this will result in fast retry
			m_commands.push(Get_Climate);
			return true;
		}
	}

	return Reset("command");
}

bool CTesla::GetDriveStatus()
{
	Json::Value reply;

	if (m_api.GetData(CTeslaApi::Drive_State, reply))
	{
		std::string CarLatitude = reply["response"]["latitude"].asString();
		std::string CarLongitude = reply["response"]["longitude"].asString();
		int Speed = reply["response"]["speed"].asInt();

		int nValue;
		std::string sValue;
		std::vector<std::string> strarray;
		if (m_sql.GetPreferencesVar("Location", nValue, sValue))
			StringSplit(sValue, ";", strarray);

		if (strarray.size() != 2)
		{
			Log(LOG_ERROR, "No location set in Domoticz. Assuming car is not home.");
			m_car.is_home = false;
		}
		else
		{
			std::string Latitude = strarray[0];
			std::string Longitude = strarray[1];

			double LaDz = std::stod(Latitude);
			double LoDz = std::stod(Longitude);
			double LaCar = std::stod(CarLatitude);
			double LoCar = std::stod(CarLongitude);

			m_car.is_home = ((std::fabs(LaDz - LaCar) < 2E-4) && (std::fabs(LoDz - LoCar) < 2E-3) && (Speed == 0));

			Log(LOG_NORM, "Location: %s %s Speed: %d Home: %s", CarLatitude.c_str(), CarLongitude.c_str(), Speed, m_car.is_home ? "true" : "false");

		}

		if (!m_car.is_home)
			SendAlertSensor(TESLA_ALERT_STATUS, 255, NotHome, "Not home", m_Name + " State");
		else
			SendAlertSensor(TESLA_ALERT_STATUS, 255, Home, "At home", m_Name + " State");

		return true;
	}

	SendAlertSensor(TESLA_ALERT_STATUS, 255, NotHome, "Not home", m_Name + " State");
	return Reset("drive status");
}

bool CTesla::GetClimateStatus()
{
	Json::Value reply;

	if (m_api.GetData(CTeslaApi::Climate_State, reply))
	{
		float temp_inside = reply["response"]["inside_temp"].asFloat();
		m_car.climate_on = reply["response"]["is_climate_on"].asBool(); 
		m_car.defrost = (reply["response"]["defrost_mode"].asInt() != 0);
		SendTempSensor(TESLA_TEMP_CLIMATE, 255, static_cast<float>(temp_inside), m_Name + " Temperature");
		SendSwitch(TESLA_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
		SendSwitch(TESLA_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
		return true;
	}

	return Reset("climate status");
}

bool CTesla::GetChargeStatus()
{
	Json::Value reply;

	if (m_api.GetData(CTeslaApi::Charge_State, reply))
	{
		int battery_level = reply["response"]["battery_level"].asInt();
		SendPercentageSensor(TESLA_LEVEL_BATTERY, 1, battery_level, static_cast<float>(battery_level), m_Name + " Battery Level");
		std::string charge_state = reply["response"]["charging_state"].asString();
		m_car.connected = (charge_state != "Disconnected");
		m_car.charging = (charge_state == "Charging") || (charge_state == "Starting");
		if(m_car.is_home)
		{
			if (!m_car.connected)
				SendAlertSensor(TESLA_ALERT_STATUS, 255, Home, "At home, No cable connected", m_Name + " State");
			else if (m_car.charging)
				SendAlertSensor(TESLA_ALERT_STATUS, 255, Charging, "At home, " + charge_state, m_Name + " State");
			else
				SendAlertSensor(TESLA_ALERT_STATUS, 255, NotCharging, "At home, " + charge_state, m_Name + " State");
		}
		SendSwitch(TESLA_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
		return true;
	}

	return Reset("charge status");
}

