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

CeVehicle::CeVehicle(const int ID, eVehicleType vehicletype, const std::string& username, const std::string& password, int defaultinterval, int activeinterval, bool allowwakeup, const std::string& carid)
{
	m_loggedin = false;
	m_HwdID = ID;
	Init();
	m_commands.clear();
	m_currentalert = Sleeping;
	m_currentalerttext = "";

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
	{
		m_defaultinterval = defaultinterval;
		if(m_defaultinterval < m_api->GetSleepInterval())
			Log(LOG_ERROR, "Warning: default interval of %d minutes will prevent the car to sleep.", m_defaultinterval);
	}
	else
		m_defaultinterval = m_api->GetSleepInterval();

	if (activeinterval > 0)
		m_activeinterval = activeinterval;
	else
		m_activeinterval = 1;

	m_allowwakeup = allowwakeup;
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
	m_car.wake_state = Asleep;
	m_car.charge_state = "";
	m_command_nr_tries = 0;
	m_setcommand_scheduled = false;
}

void CeVehicle::SendAlert()
{
	eAlertType alert;
	std::string title;

	if (m_car.is_home && (m_car.wake_state != Asleep))
	{
		if (m_car.charging)
		{
			alert = Charging;
			title = "Home";
		}
		else if (m_car.connected)
		{
			alert = NotCharging;
			title = "Home";
		}
		else
		{
			alert = Idling;
			if(m_car.wake_state == WakingUp)
				title = "Waking Up";
			else
				title = "Home";
		}
		if (!m_car.charge_state.empty())
			title = title + ", " + m_car.charge_state;
	}
	else if (m_car.wake_state == Asleep)
	{
		alert = Sleeping;
		if (m_command_nr_tries > VEHICLE_MAXTRIES)
			title = "Offline";
		else
			title = "Asleep";
	}
	else
	{
		alert = NotHome;
		if (m_car.wake_state == WakingUp)
			title = "Waking Up";
		else
			title = "Not Home";
		if (m_car.charging && !m_car.charge_state.empty())
			title = title + ", " + m_car.charge_state;
	}

	if((alert != m_currentalert) || (title != m_currentalerttext))
	{
		SendAlertSensor(VEHICLE_ALERT_STATUS, 255, alert, title, m_Name + " State");
		m_currentalert = alert;
		m_currentalerttext = title;
	}
}


bool CeVehicle::ConditionalReturn(bool commandOK, eApiCommandType command)
{
	if(commandOK)
	{
		m_command_nr_tries = 0;
		SendAlert();
		return true;
	}
	else if(m_command_nr_tries > VEHICLE_MAXTRIES)
	{
		Init();
		SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
		SendSwitch(VEHICLE_SWITCH_CLIMATE, 1, 255, m_car.climate_on, 0, m_Name + " Climate switch");
		SendSwitch(VEHICLE_SWITCH_DEFROST, 1, 255, m_car.defrost, 0, m_Name + " Defrost switch");
		m_commands.clear();
		Log(LOG_ERROR, "Multiple tries requesting %s. Assuming car offline.", GetCommandString(command).c_str());
		SendAlert();
		return(false);
	}
	else
	{
		if (command == Wake_Up)
		{
			Log(LOG_ERROR, "Car not yet awake. Will retry.");
			SendAlert();
		}
		else
			Log(LOG_ERROR, "Timeout requesting %s. Will retry.", GetCommandString(command).c_str());
		m_command_nr_tries++;
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
	case Get_Awake_State:
		return("Get Awake state");
	default:
		return "";
	}
}

void CeVehicle::Do_Work()
{
	int sec_counter = 0;
	int interval = 1000;
	bool initial_check = true;
	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(interval))
	{
		interval = 1000;
		sec_counter++;
		time_t now = mytime(0);
		m_LastHeartbeat = now;

		if (m_api == nullptr)
			break;

		// Only login if we should
		if (!m_loggedin || (sec_counter % 68400 == 0))
		{
			Login();
			sec_counter = 1;
			continue;
		}

		// if commands scheduled, wake up car if needed and execute commands
		if (!m_commands.empty())
		{
			if (IsAwake())
			{
				if(DoNextCommand())
				{
					// if failed try (e.g. timeout), wait a while
					if (m_command_nr_tries > 0)
					{
						interval = 5000;
					}
				}
			}
			else
			{
				// car should wake up first, if allowed
				if (m_allowwakeup || m_car.charging || m_setcommand_scheduled)
				{
					if (WakeUp())
						if(m_car.wake_state == Awake)
							interval = 5000;
				}
				else
				{
					eApiCommandType item;
					m_commands.try_pop(item);
					Log(LOG_STATUS, "Car asleep, not allowed to wake up, command %s ignored.", GetCommandString(item).c_str());
				}
			}
		}
		else
		{
			m_setcommand_scheduled = false;
		}

		// now do wake state checks
		if (initial_check)
		{
			if(IsAwake() || m_allowwakeup)
				m_commands.push(Get_All_States);
			initial_check = false;
		}
		else if ((sec_counter % 60) == 0)
		{
			// check awake state every minute
			if (IsAwake())
			{
				if (!m_allowwakeup && m_car.wake_state == SelfAwake)
				{
					Log(LOG_STATUS, "Spontaneous wake up detected.");
					m_commands.push(Get_All_States);
				}
			}
		}

		// now schedule timed commands
		if ((sec_counter % (60*m_defaultinterval) == 0))
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
	m_setcommand_scheduled = true;
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

void CeVehicle::Login()
{
	// Only login if we should.
	if (!m_loggedin)
	{
		m_loggedin = m_api->Login();
	}
	else
		m_loggedin = m_api->RefreshLogin();
}


bool CeVehicle::IsAwake()
{
	Log(LOG_STATUS, "Executing command: %s", GetCommandString(Get_Awake_State).c_str());

	if (m_api->IsAwake())
	{
		if (m_car.wake_state == Asleep)
			m_car.wake_state = SelfAwake;
		else if (m_car.wake_state == WakingUp)
			m_car.wake_state = Awake;
		else if (m_car.wake_state == SelfAwake)
			m_car.wake_state = Awake;
		Log(LOG_NORM, "Car is awake");
		return true;
	}
	else
	{
		if (m_car.wake_state == Awake)
			m_car.wake_state = Asleep;
		else if (m_car.wake_state == SelfAwake)
			m_car.wake_state = Asleep;
		Log(LOG_NORM, "Car is asleep");
		SendAlert();
	}
	return false;
}

bool CeVehicle::WakeUp()
{
	if (m_command_nr_tries == 0)
	{
		Log(LOG_STATUS, "Waking up car.");
		m_car.wake_state = WakingUp;
	}

	if (m_api->SendCommand(CVehicleApi::Wake_Up))
	{
		m_car.wake_state = Awake;
	}

	return ConditionalReturn(m_car.wake_state == Awake, Wake_Up);
}

bool CeVehicle::DoNextCommand()
{
	eApiCommandType command;
	m_commands.try_pop(command);
	bool commandOK = false;

	Log(LOG_STATUS, "Executing command: %s", GetCommandString(command).c_str());

	switch (command)
	{
	case Send_Climate_Off:
	case Send_Climate_On:
	case Send_Climate_Defrost:
	case Send_Climate_Defrost_Off:
	case Send_Charge_Start:
	case Send_Charge_Stop:
		commandOK = DoSetCommand(command);
		break;
	case Get_All_States:
		commandOK = GetAllStates();
		break;
	case Get_Climate_State:
		commandOK = GetClimateState();
		break;
	case Get_Charge_State:
		commandOK = GetChargeState();
		break;
	case Get_Location_State:
		commandOK = GetLocationState();
		break;
	default:
		commandOK = false;
	}

	// if failed try (e.g. timeout), reschedule command
	if (commandOK && m_command_nr_tries > 0)
	{
		m_commands.push(command);
	}

	return commandOK;
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
			return true;
		case Send_Climate_Off:
		case Send_Climate_On:
			m_commands.push(Get_Climate_State);
			return true;
		case Send_Climate_Defrost:
		case Send_Climate_Defrost_Off:
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
	m_car.charge_state = data.status_string;
	SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, m_car.charging, 0, m_Name + " Charge switch");
}
