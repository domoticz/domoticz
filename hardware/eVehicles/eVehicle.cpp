#include "stdafx.h"
#include "eVehicle.h"
#include "TeslaApi.h"
#include "../../main/Helper.h"
#include "../../main/Logger.h"
#include "../hardwaretypes.h"
#include "../../main/localtime_r.h"
#include "../../main/RFXtrx.h"
#include "../../main/SQLHelper.h"
#include <sstream>
#include <iomanip>
#include <cmath>

#define VEHICLE_SWITCH_CHARGE 1
#define VEHICLE_SWITCH_CLIMATE 2
#define VEHICLE_SWITCH_DEFROST 3

#define VEHICLE_TEMP_INSIDE 1
#define VEHICLE_TEMP_OUTSIDE 2
#define VEHICLE_ALERT_STATUS 1
#define VEHICLE_LEVEL_BATTERY 1

#define VEHICLE_MAXTRIES 5

CeVehicle::CeVehicle(const int ID, eVehicleType vehicletype, const std::string& username, const std::string& password, int defaultinterval, int activeinterval, const std::string& carid)
{
	m_loggedin = false;
	m_HwdID = ID;
	Init();
	m_commands.clear();
	switch (vehicletype)
	{
	case Tesla:
		m_api = new CTeslaApi(username, password, carid);
		break;
	default:
		Log(LOG_ERROR, "Unsupported vehicle type.");
		m_api = nullptr;
		break;
	}
	if (defaultinterval > 0)
		m_defaultinterval = defaultinterval;
	else
		m_defaultinterval = 10;

	if (activeinterval > 0)
		m_activeinterval = activeinterval;
	else
		m_activeinterval = 1;
}

CeVehicle::~CeVehicle(void)
{
	m_commands.clear();
	delete m_api;
}

void CeVehicle::Init()
{
	m_car.charging = false;
	m_car.connected = false;
	m_car.is_home = false;
	m_car.climate_on = false;
	m_car.defrost = false;
	m_car.awake = false;
	m_trycounter = 0;
}

bool CeVehicle::ConditionalReturn(bool commandOK, eApiCommandType command)
{
	if(commandOK)
	{
		m_trycounter = 0;
		return true;
	}
	else if(m_trycounter > VEHICLE_MAXTRIES)
	{
		Init();
		SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
		SendSwitch(VEHICLE_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
		SendSwitch(VEHICLE_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
		m_commands.clear();
		Log(LOG_ERROR, "Multiple tries requesting %s. Assuming car offline.", GetCommandString(command).c_str());
		SendAlertSensor(VEHICLE_ALERT_STATUS, 255, Offline, "Offline", m_Name + " State");
		return(false);
	}
	else
	{
		if(command == Wake_Up)
			Log(LOG_ERROR, "Car not yet awake. Will retry.");
		else
			Log(LOG_ERROR, "Timeout requesting %s. Will retry.", GetCommandString(command).c_str());
		m_trycounter++;
	}

	return(true);
}

bool CeVehicle::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CeVehicle::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	if (!m_thread)
		return false;
	m_bIsStarted = true;
	sOnConnected(this);
	return true;
}

bool CeVehicle::StopHardware()
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

std::string CeVehicle::GetCommandString(const eApiCommandType command)
{
	switch (command)
	{
	case Send_Climate_Off:
		return("Switch Climate off");
	case Send_Climate_On:
		return("Switch Climate on");
	case Send_Climate_Defrost:
		return("Switch Defrost mode on");
	case Send_Climate_Defrost_Off:
		return("Switch Defrost mode off");
	case Send_Charge_Start:
		return("Start charging");
	case Send_Charge_Stop:
		return("Stop charging");
	case Get_All_States:
		return("Get All states");
	case Get_Climate_State:
		return("Get Climate state");
	case Get_Charge_State:
		return("Get Charge state");
	case Get_Location_State:
		return("Get Location state");
	case Wake_Up:
		return("Wake Up");
	default:
		return "";
	}
}

void CeVehicle::Do_Work()
{
	int sec_counter = (60 * m_defaultinterval) - 7; // to receive states after startup
	bool waking_up = false;
	int interval = 1000;
	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(interval))
	{
		interval = 1000;
		sec_counter++;
		time_t now = mytime(0);
		m_LastHeartbeat = now;

		if (m_api == nullptr)
			break;

		// Only login if we should.
		if (!m_loggedin)
		{
			m_loggedin = m_api->Login();
			m_car.awake = m_api->IsAwake();
			continue;
		}

		if (sec_counter % 68400 == 0)
		{
			m_loggedin = m_api->RefreshLogin();
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
			m_car.awake = m_api->IsAwake();
			if (m_car.awake)
			{
				// car is awake, execute command
				eApiCommandType item;
				m_commands.try_pop(item);
				if(DoCommand(item))
				{
					// if failed try (e.g. timeout), reschedule command but wait a while
					if (m_trycounter > 0)
					{
						interval = 5000;
						m_commands.push(item);
					}
				}
				else
				{
					interval = 1000;
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
			// if no more commands scheduled, check if awake every minute
			if ((sec_counter % 60) == 0)
				m_car.awake = m_api->IsAwake();
		}

		// now schedule timed commands
		if (sec_counter % (60*m_defaultinterval) == 0)
		{
			// check all states every default interval
			m_commands.push(Get_All_States);
		}
		else if (sec_counter % (60*m_activeinterval) == 0)
		{
			if (m_car.is_home && (m_car.charging || m_car.climate_on || m_car.defrost))
			{
				// check relevant states every active interval
				if (m_car.charging)
					m_commands.push(Get_Charge_State);
				if (m_car.climate_on || m_car.defrost)
					m_commands.push(Get_Climate_State);
			}
		}
		else
		{
			;
		}
	}
	
	Log(LOG_STATUS, "Worker stopped...");
}

bool CeVehicle::WriteToHardware(const char* pdata, const unsigned char length)
{
	if (!m_loggedin)
		return false;

	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);

	m_commands.push(Get_Location_State);
	switch (pCmd->LIGHTING2.id4)
	{
	case VEHICLE_SWITCH_CHARGE:
		m_commands.push(Get_Charge_State);
		if (bIsOn)
			m_commands.push(Send_Charge_Start);
		else
			m_commands.push(Send_Charge_Stop);
		break;
	case VEHICLE_SWITCH_CLIMATE:
		if (bIsOn)
			m_commands.push(Send_Climate_On);
		else
			m_commands.push(Send_Climate_Off);
		break;
	case VEHICLE_SWITCH_DEFROST:
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

bool CeVehicle::WakeUp()
{
	if (m_trycounter == 0)
		Log(LOG_STATUS, "Waking up car.");

	if (m_trycounter > 0 && m_car.is_home)
		SendAlertSensor(VEHICLE_ALERT_STATUS, 255, Offline, "At home, Waking up", m_Name + " State");	

	if (m_api->SendCommand(CVehicleApi::Wake_Up))
	{
		m_car.awake = true;
		if (m_trycounter > 0 && m_car.is_home)
			SendAlertSensor(VEHICLE_ALERT_STATUS, 255, Home, "At home", m_Name + " State");
		return ConditionalReturn(true, Wake_Up);
	}

	return ConditionalReturn(false, Wake_Up);
}

bool CeVehicle::DoCommand(eApiCommandType command)
{
	Log(LOG_STATUS, "Executing command: %s", GetCommandString(command).c_str());

	switch (command)
	{
	case Send_Climate_Off:
	case Send_Climate_On:
	case Send_Climate_Defrost:
	case Send_Climate_Defrost_Off:
	case Send_Charge_Start:
	case Send_Charge_Stop:
		return DoSetCommand(command);
		break;
	case Get_All_States:
		return GetAllStates();
		break;
	case Get_Climate_State:
		return GetClimateState();
		break;
	case Get_Charge_State:
		return GetChargeState();
		break;
	case Get_Location_State:
		return GetLocationState();
		break;
	default:
		return false;
	}

	return false;
}

bool CeVehicle::DoSetCommand(eApiCommandType command)
{
	CVehicleApi::eCommandType api_command;

	if (!m_car.is_home)
	{
		Log(LOG_ERROR, "Car not home. No commands allowed.");
		SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
		SendSwitch(VEHICLE_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
		SendSwitch(VEHICLE_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
		return true;
	}

	switch (command)
	{
	case Send_Charge_Start:
		if (!m_car.connected)
		{
			Log(LOG_ERROR, "Charge cable not connected. No charge commands possible.");
			SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
			return false;
		}
		api_command = CVehicleApi::Charge_Start;
		break;
	case Send_Charge_Stop:
		if (!m_car.connected)
		{
			Log(LOG_ERROR, "Charge cable not connected. No charge commands possible.");
			SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
			return false;
		}
		api_command = CVehicleApi::Charge_Stop;
		break;
	case Send_Climate_Off:
		api_command = CVehicleApi::Climate_Off;
		break;
	case Send_Climate_On:
		api_command = CVehicleApi::Climate_On;
		break;
	case Send_Climate_Defrost:
		api_command = CVehicleApi::Climate_Defrost;
		break;
	case Send_Climate_Defrost_Off:
		api_command = CVehicleApi::Climate_Defrost_Off;
		break;
	}

	if (m_api->SendCommand(api_command))
	{
		switch (command)
		{
		case Send_Charge_Start:
		case Send_Charge_Stop:
			m_commands.push(Get_Charge_State);
			m_car.charging = true; // if check after command fails, this will result in fast retry
			return true;
		case Send_Climate_Off:
		case Send_Climate_On:
			m_commands.push(Get_Climate_State);
			m_car.climate_on = true; // if check after command fails, this will result in fast retry
			return true;
		case Send_Climate_Defrost:
		case Send_Climate_Defrost_Off:
			m_car.defrost = true; // if check after command fails, this will result in fast retry
			m_commands.push(Get_Climate_State);
			return ConditionalReturn(true, command);
		}
	}

	return ConditionalReturn(false, command);
}

bool CeVehicle::GetAllStates()
{
	CVehicleApi::tAllCarData reply;

	if (m_api->GetAllData(reply))
	{
		UpdateLocationData(reply.location);
		UpdateChargeData(reply.charge);
		UpdateClimateData(reply.climate);
		return ConditionalReturn(true, Get_All_States);
	}

	return ConditionalReturn(false, Get_All_States);
}

bool CeVehicle::GetLocationState()
{
	CVehicleApi::tLocationData reply;

	if (m_api->GetLocationData(reply))
	{
		UpdateLocationData(reply);
		return ConditionalReturn(true, Get_Location_State);
	}

	return ConditionalReturn(false, Get_Location_State);
}

void CeVehicle::UpdateLocationData(CVehicleApi::tLocationData& data)
{
	bool car_old_state = m_car.is_home;
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

		m_car.is_home = ((std::fabs(LaDz - data.latitude) < 2E-4) && (std::fabs(LoDz - data.longitude) < 2E-3) && !data.is_driving);

		Log(LOG_NORM, "Location: %f %f Speed: %d Home: %s", data.latitude, data.longitude, data.speed, m_car.is_home ? "true" : "false");

	}

	if (!m_car.is_home)
		SendAlertSensor(VEHICLE_ALERT_STATUS, 255, NotHome, "Not home", m_Name + " State");
	else
		if (car_old_state != m_car.is_home)
			SendAlertSensor(VEHICLE_ALERT_STATUS, 255, Home, "At home", m_Name + " State");
}

bool CeVehicle::GetClimateState()
{
	CVehicleApi::tClimateData reply;

	if (m_api->GetClimateData(reply))
	{
		UpdateClimateData(reply);
		return ConditionalReturn(true, Get_Climate_State);
	}

	return ConditionalReturn(false, Get_Climate_State);
}

void CeVehicle::UpdateClimateData(CVehicleApi::tClimateData& data)
{
	m_car.climate_on = data.is_climate_on;
	m_car.defrost = data.is_defrost_on;
	SendTempSensor(VEHICLE_TEMP_INSIDE, 255, data.inside_temp, m_Name + " Temperature");
	SendTempSensor(VEHICLE_TEMP_OUTSIDE, 255, data.outside_temp, m_Name + " Outside Temperature");
	SendSwitch(VEHICLE_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
	SendSwitch(VEHICLE_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
}

bool CeVehicle::GetChargeState()
{
	CVehicleApi::tChargeData reply;

	if (m_api->GetChargeData(reply))
	{
		UpdateChargeData(reply);
		return ConditionalReturn(true, Get_Charge_State);
	}

	return ConditionalReturn(false, Get_Charge_State);
}

void CeVehicle::UpdateChargeData(CVehicleApi::tChargeData& data)
{
	SendPercentageSensor(VEHICLE_LEVEL_BATTERY, 1, static_cast<int>(data.battery_level), data.battery_level, m_Name + " Battery Level");
	m_car.connected = data.is_connected;
	m_car.charging = data.is_charging;
	if (m_car.is_home)
	{
		if (!m_car.connected)
			SendAlertSensor(VEHICLE_ALERT_STATUS, 255, Home, "At home, No cable connected", m_Name + " State");
		else if (m_car.charging)
			SendAlertSensor(VEHICLE_ALERT_STATUS, 255, Charging, "At home, " + data.status_string, m_Name + " State");
		else
			SendAlertSensor(VEHICLE_ALERT_STATUS, 255, NotCharging, "At home, " + data.status_string, m_Name + " State");
	}
	SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
}
