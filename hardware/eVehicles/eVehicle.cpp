/************************************************************************

eVehicles framework
Author: MrHobbes74 (github.com/MrHobbes74)

21/02/2020 1.0 Creation
13/03/2020 1.1 Added keep asleep support
28/04/2020 1.2 Added new devices (odometer, lock alert, max charge switch)
24/07/2020 1.3 Added new Mercedes Class (KidDigital)
09/02/2021 1.4 Added Testcar Class for easier testing of eVehicle framework

License: Public domain

************************************************************************/
#include "stdafx.h"
#include "eVehicle.h"
#include "TeslaApi.h"
#include "TestcarApi.h"
#include "MercApi.h"
#include "../../main/Helper.h"
#include "../../main/Logger.h"
#include "../hardwaretypes.h"
#include "../../main/localtime_r.h"
#include "../../main/RFXtrx.h"
#include "../../main/SQLHelper.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>

#define VEHICLE_SWITCH_CHARGE 1
#define VEHICLE_SWITCH_CLIMATE 2
#define VEHICLE_SWITCH_DEFROST 3
#define VEHICLE_SWITCH_MAX_CHARGE 4

#define VEHICLE_TEMP_INSIDE 1
#define VEHICLE_TEMP_OUTSIDE 2
#define VEHICLE_ALERT_STATUS 1
#define VEHICLE_ALERT_LOCK 2
#define VEHICLE_LEVEL_BATTERY 1
#define VEHICLE_COUNTER_ODO 1
#define VEHICLE_CUSTOM 9

#define VEHICLE_MAXTRIES 5

CeVehicle::CeVehicle(const int ID, eVehicleType vehicletype, const std::string& username, const std::string& password, int defaultinterval, int activeinterval, bool allowwakeup, const std::string& carid)
{
	m_loggedin = false;
	m_HwdID = ID;
	Init();
	m_commands.clear();
	m_currentalert = Sleeping;
	m_currentalerttext = "";

	eVehicleType checkedVehicleType = vehicletype;
	if (username == "testcar@evehicle")
		checkedVehicleType = Testcar;

	switch (checkedVehicleType)
	{
	case Tesla:
		m_api = new CTeslaApi(username, password, carid);
		break;
	case Mercedes:
		m_api = new CMercApi(username, password, carid);
		break;
	case Testcar:
		m_api = new CTestcarApi(username, password, carid);
		break;
	default:
		Log(LOG_ERROR, "Unsupported vehicle type.");
		m_api = nullptr;
		break;
	}
	if ((defaultinterval > 0) && (checkedVehicleType != Testcar))
	{
		m_defaultinterval = defaultinterval*60;
		if(m_defaultinterval < m_api->m_capabilities.seconds_to_sleep)
			Log(LOG_ERROR, "Warning: default interval of %d minutes will prevent the car to sleep.", defaultinterval);
	}
	else
		m_defaultinterval = std::max(m_api->m_capabilities.seconds_to_sleep, m_api->m_capabilities.minimum_poll_interval);

	if (activeinterval > 0)
		m_activeinterval = activeinterval*60;
	else
		m_activeinterval = m_api->m_capabilities.minimum_poll_interval;

	m_allowwakeup = allowwakeup;
}

CeVehicle::~CeVehicle()
{
	m_commands.clear();
	delete m_api;
}

void CeVehicle::Init()
{
	m_car.charging = false;
	m_car.connected = false;
	m_car.home_state = AtUnknown;
	m_car.climate_on = false;
	m_car.defrost = false;
	m_car.wake_state = Unknown;
	m_car.charge_state = "";
	m_command_nr_tries = 0;
	m_setcommand_scheduled = false;
}

void CeVehicle::SendAlert()
{
	eAlertType alert;
	std::string title;

	if ((m_car.home_state == AtHome) && (m_car.wake_state != Asleep))
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
		title = "Asleep";
	}
	else
	{
		alert = NotHome;
		if ((m_car.wake_state == WakingUp) || (m_car.home_state == AtUnknown))
			title = "Waking Up";
		else
			title = "Not Home";

		if (m_car.charging && !m_car.charge_state.empty())
			title = title + ", " + m_car.charge_state;
	}

	if((alert != m_currentalert) || (title != m_currentalerttext))
	{
		SendAlert(VEHICLE_ALERT_STATUS, alert, title);
		m_currentalert = alert;
		m_currentalerttext = title;
	}
}

void CeVehicle::SendAlert(int alertType, int value, const std::string &title)
{
	if (alertType == VEHICLE_ALERT_STATUS)
		SendAlertSensor(VEHICLE_ALERT_STATUS, 255, value, title, m_Name + " State");
	if ((alertType == VEHICLE_ALERT_LOCK) && m_api->m_capabilities.has_lock_status)
		SendAlertSensor(VEHICLE_ALERT_LOCK, 255, value, title, m_Name + " Open Alert");
}

void CeVehicle::SendSwitch(int switchType, bool value)
{
	if ((switchType == VEHICLE_SWITCH_CHARGE) && m_api->m_capabilities.has_charge_command)
		CDomoticzHardwareBase::SendSwitch(VEHICLE_SWITCH_CHARGE, 1, 255, value, 0, m_Name + " Charge switch", m_Name);
	if ((switchType == VEHICLE_SWITCH_CLIMATE) && m_api->m_capabilities.has_charge_command)
		CDomoticzHardwareBase::SendSwitch(VEHICLE_SWITCH_CLIMATE, 1, 255, value, 0, m_Name + " Climate switch", m_Name);
	if ((switchType == VEHICLE_SWITCH_DEFROST) && m_api->m_capabilities.has_charge_command)
		CDomoticzHardwareBase::SendSwitch(VEHICLE_SWITCH_DEFROST, 1, 255, value, 0, m_Name + " Defrost switch", m_Name);
}

void CeVehicle::SendValueSwitch(int switchType, int value)
{
	if ((switchType == VEHICLE_SWITCH_MAX_CHARGE) && m_api->m_capabilities.has_charge_limit)
		CDomoticzHardwareBase::SendSwitch(VEHICLE_SWITCH_MAX_CHARGE, 1, 255, (value == 100), 0, m_Name + " Max charge limit switch", m_Name);
}

void CeVehicle::SendTemperature(int tempType, float value)
{
	if ((tempType == VEHICLE_TEMP_INSIDE) && m_api->m_capabilities.has_inside_temp)
		SendTempSensor(VEHICLE_TEMP_INSIDE, 255, value, m_Name + " Temperature");
	if ((tempType == VEHICLE_TEMP_OUTSIDE) && m_api->m_capabilities.has_outside_temp)
		SendTempSensor(VEHICLE_TEMP_OUTSIDE, 255, value, m_Name + " Outside Temperature");
}

void CeVehicle::SendPercentage(int percType, float value)
{
	if ((percType == VEHICLE_LEVEL_BATTERY) && m_api->m_capabilities.has_battery_level)
	{
		int iBattLevel = static_cast<int>(value);
		float fBattLevel = value;
		if (fBattLevel < 0.0f || fBattLevel > 100.0f)	// Filter out wrong readings
		{
			fBattLevel = 0.0f;
			iBattLevel = 255;	// Means no batterylevel available (and hides batterylevel indicator in UI)
		}
		SendPercentageSensor(VEHICLE_LEVEL_BATTERY, 1, iBattLevel, fBattLevel, m_Name + " Battery Level");
	}
}

void CeVehicle::SendCounter(int countType, float value)
{
	if ((countType == VEHICLE_COUNTER_ODO) && m_api->m_capabilities.has_odo)
		SendCustomSensor(VEHICLE_COUNTER_ODO, 1, 255, value, m_Name + " Odometer", m_api->m_config.distance_unit);
}

void CeVehicle::SendCustom(int customType, int ChildId, float value, const std::string &label)
{
	if ((customType == VEHICLE_CUSTOM) && m_api->m_capabilities.has_custom_data)
		SendCustomSensor(VEHICLE_CUSTOM, ChildId, 255, value, m_Name + " " + label, "");
}

void CeVehicle::SendCustomSwitch(int customType, int ChildId, bool value, const std::string &label)
{
	if ((customType == VEHICLE_CUSTOM) && m_api->m_capabilities.has_custom_data)
		{
			CDomoticzHardwareBase::SendGeneralSwitch(((VEHICLE_CUSTOM << 8) | ChildId), 1, 255, value, 0, m_Name + " " + label, m_Name);
		}
}

void CeVehicle::SendCustomText(int customType, int ChildId, const std::string &value, const std::string &label)
{
	if ((customType == VEHICLE_CUSTOM) && m_api->m_capabilities.has_custom_data)
		SendTextSensor(VEHICLE_CUSTOM, ChildId, 255, value, m_Name + " " + label);
}

bool CeVehicle::ConditionalReturn(bool commandOK, eApiCommandType command)
{
	if(commandOK)
	{
		m_command_nr_tries = 0;
		SendAlert();
		return true;
	}
	if (m_command_nr_tries > VEHICLE_MAXTRIES)
	{
		Init();
		SendSwitch(VEHICLE_SWITCH_CHARGE, m_car.charging);
		SendSwitch(VEHICLE_SWITCH_CLIMATE, m_car.climate_on);
		SendSwitch(VEHICLE_SWITCH_DEFROST, m_car.defrost);
		SendValueSwitch(VEHICLE_SWITCH_MAX_CHARGE, m_car.charge_limit);
		m_commands.clear();
		Log(LOG_ERROR, "Multiple tries requesting %s. Assuming car asleep.", GetCommandString(command).c_str());
		m_car.wake_state = Asleep;
		SendAlert();
		return false;
	}
	if (command == Wake_Up)
	{
		Log(LOG_ERROR, "Car not yet awake. Will retry.");
		SendAlert();
	}
	else
		Log(LOG_ERROR, "Timeout requesting %s. Will retry.", GetCommandString(command).c_str());

	m_command_nr_tries++;
	return true;
}

bool CeVehicle::StartHardware()
{
	int nValue;
	std::string sValue;
	std::vector<std::string> strarray;

	RequestStart();

	Init();

	if (m_sql.GetPreferencesVar("Location", nValue, sValue))
		StringSplit(sValue, ";", strarray);

	if (strarray.size() != 2)
	{
		Log(LOG_ERROR, "No location set in Domoticz. Assuming car is not home.");
		m_car.home_state = NotAtHome;
	}
	else
	{
		std::string Latitude = strarray[0];
		std::string Longitude = strarray[1];
		m_api->m_config.home_latitude = std::stod(Latitude);
		m_api->m_config.home_longitude = std::stod(Longitude);

		Log(LOG_STATUS, "Using Domoticz home location (Lat %s, Lon %s) as car's home location.", Latitude.c_str(), Longitude.c_str());
	}

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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
	case Send_Charge_Limit:
		return("Set Charge limit");
	default:
		return "";
	}
}

void CeVehicle::Do_Work()
{
	int sec_counter = 0;
	int fail_counter = 0;
	int interval = 1000;
	bool initial_check = true;
	bool bIsAborted = false;
	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(interval))
	{
		interval = 1000;
		sec_counter++;
		time_t now = mytime(nullptr);
		m_LastHeartbeat = now;

		if (m_api == nullptr)
		{
			Log(LOG_ERROR, "Aborting worker as there is no eVehicle provided!");
			RequestStop();
			continue;
		}

		// Only login if we should
		if ((m_loggedin == false && bIsAborted == false) || (sec_counter % 68400 == 0))
		{
			Login();
			if(!m_loggedin)
			{
				fail_counter++;
				if(fail_counter > 3)
				{
					Log(LOG_ERROR, "Aborting due to too many failed authentication attempts (and prevent getting blocked)!");
					fail_counter = 0;
					bIsAborted = true;
					continue;
				}
			}
			else
			{
				sec_counter = 1;
				fail_counter = 0;
				bIsAborted = false;
			}
			continue;
		}
		if (bIsAborted)
		{
			if (sec_counter % 7200 == 0)
			{
				Log(LOG_ERROR, "Worker inactive due to inability to authenticate! Please check credentials!");
			}
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
						{
							AddCommand(Get_All_States);
							interval = 5000;
						}
				}
				else
				{
					tApiCommand item;
					m_commands.try_pop(item);
					Debug(DEBUG_NORM, "Car asleep, not allowed to wake up, command %s ignored.", GetCommandString(item.command_type).c_str());
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
				AddCommand(Get_All_States);
			initial_check = false;
		}
		else if ((sec_counter % m_api->m_capabilities.minimum_poll_interval) == 0)
		{
			// check awake state every minimum poll interval
			if (IsAwake())
			{
				if (!m_allowwakeup && m_car.wake_state == SelfAwake)
				{
					AddCommand(Get_All_States);
				}
			}
		}

		// now schedule timed commands
		if ((sec_counter % (m_defaultinterval) == 0))
		{
			// check all states every default interval
			AddCommand(Get_All_States);
		}
		else if (sec_counter % (m_activeinterval) == 0)
		{
			if ((m_car.home_state == AtHome) && (m_car.charging || m_car.climate_on || m_car.defrost))
			{
				// check relevant states every active interval
				if (m_car.charging)
					AddCommand(Get_Charge_State);
				if (m_car.climate_on || m_car.defrost)
					AddCommand(Get_Climate_State);
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

	bool bIsOn = false;
	int iID = 0;
	int level = 0;
	int cmd = 0;

	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);

	switch (pCmd->ICMND.packettype)
	{
	case pTypeLighting2:
		bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		cmd = pCmd->LIGHTING2.cmnd;
		iID = pCmd->LIGHTING2.id4;
		level = pCmd->LIGHTING2.level;
		break;
	default:
		return false;
	}

	AddCommand(Get_Location_State);
	m_setcommand_scheduled = true;

	switch (iID)
	{
	case VEHICLE_SWITCH_CHARGE:
		AddCommand(Get_Charge_State);
		if (bIsOn)
			AddCommand(Send_Charge_Start);
		else
			AddCommand(Send_Charge_Stop);
		break;
	case VEHICLE_SWITCH_CLIMATE:
		if (bIsOn)
			AddCommand(Send_Climate_On);
		else
			AddCommand(Send_Climate_Off);
		break;
	case VEHICLE_SWITCH_DEFROST:
		if (bIsOn)
			AddCommand(Send_Climate_Defrost);
		else
			AddCommand(Send_Climate_Defrost_Off);
		break;
	case VEHICLE_SWITCH_MAX_CHARGE:
		if (bIsOn)
			AddCommand(Send_Charge_Limit, "100");
		else
			AddCommand(Send_Charge_Limit, "0");
		break;
	default:
		Log(LOG_ERROR, "Unknown switch %d", iID);
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
	bool status_changed = false;

	Debug(DEBUG_NORM, "Executing command: %s", GetCommandString(Get_Awake_State).c_str());
	if (m_api->IsAwake())
	{
		if ((m_car.wake_state == Asleep) || (m_car.wake_state == Unknown))
		{
			m_car.wake_state = SelfAwake;
			status_changed = true;
		}
		else
			m_car.wake_state = Awake;
	}
	else
	{
		if (m_car.wake_state != WakingUp)
		{
			if (m_car.wake_state != Asleep)
				status_changed = true;
			m_car.wake_state = Asleep;
		}
		SendAlert();
	}

	if(status_changed)
	{
		if(m_car.wake_state == Asleep)
			Log(LOG_STATUS, "Car awake detection: Car asleep");
		else
			Log(LOG_STATUS, "Car awake detection: Car awake");
	}

	return ((m_car.wake_state != Asleep) && (m_car.wake_state != WakingUp));
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

void CeVehicle::AddCommand(eApiCommandType command_type, std::string command_parameter)
{
	tApiCommand command;

	command.command_type = command_type;
	command.command_parameter = std::move(command_parameter);

	m_commands.push(command);
}

bool CeVehicle::DoNextCommand()
{
	tApiCommand command;
	m_commands.try_pop(command);
	bool commandOK = false;

	Log(LOG_STATUS, "Executing command: %s", GetCommandString(command.command_type).c_str());

	switch (command.command_type)
	{
	case Send_Climate_Off:
	case Send_Climate_On:
	case Send_Climate_Defrost:
	case Send_Climate_Defrost_Off:
	case Send_Charge_Start:
	case Send_Charge_Stop:
	case Send_Charge_Limit:
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

bool CeVehicle::DoSetCommand(const tApiCommand &command)
{
	CVehicleApi::eCommandType api_command;

	if (m_car.home_state != AtHome)
	{
		Log(LOG_ERROR, "Car not home. No commands allowed.");
		SendSwitch(VEHICLE_SWITCH_CHARGE, m_car.charging);
		SendSwitch(VEHICLE_SWITCH_CLIMATE, m_car.climate_on);
		SendSwitch(VEHICLE_SWITCH_DEFROST, m_car.defrost);
		SendValueSwitch(VEHICLE_SWITCH_MAX_CHARGE, m_car.charge_limit);
		return true;
	}

	switch (command.command_type)
	{
	case Send_Charge_Start:
		if (!m_car.connected)
		{
			Log(LOG_ERROR, "Charge cable not connected. No charge commands possible.");
			SendSwitch(VEHICLE_SWITCH_CHARGE, m_car.charging);
			return false;
		}
		api_command = CVehicleApi::Charge_Start;
		break;
	case Send_Charge_Stop:
		if (!m_car.connected)
		{
			Log(LOG_ERROR, "Charge cable not connected. No charge commands possible.");
			SendSwitch(VEHICLE_SWITCH_CHARGE, m_car.charging);
			return false;
		}
		api_command = CVehicleApi::Charge_Stop;
		break;
	case Send_Charge_Limit:
		api_command = CVehicleApi::Set_Charge_Limit;
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

	if (m_api->SendCommand(api_command, command.command_parameter))
	{
		switch (command.command_type)
		{
		case Send_Charge_Start:
		case Send_Charge_Stop:
		case Send_Charge_Limit:
			AddCommand(Get_Charge_State);
			break;
		case Send_Climate_Off:
		case Send_Climate_On:
		case Send_Climate_Defrost:
		case Send_Climate_Defrost_Off:
			AddCommand(Get_Climate_State);
			break;
		}
		return ConditionalReturn(true, command.command_type);
	}

	return ConditionalReturn(false, command.command_type);
}

bool CeVehicle::GetAllStates()
{
	CVehicleApi::tAllCarData reply{};

	if (m_api->GetAllData(reply))
	{
		UpdateLocationData(reply.location);
		UpdateChargeData(reply.charge);
		UpdateClimateData(reply.climate);
		UpdateVehicleData(reply.vehicle);
		UpdateCustomVehicleData(reply.custom);
		return ConditionalReturn(true, Get_All_States);
	}

	return ConditionalReturn(false, Get_All_States);
}

bool CeVehicle::GetLocationState()
{
	CVehicleApi::tLocationData reply{};

	if (m_api->GetLocationData(reply))
	{
		UpdateLocationData(reply);
		return ConditionalReturn(true, Get_Location_State);
	}

	return ConditionalReturn(false, Get_Location_State);
}

void CeVehicle::UpdateLocationData(CVehicleApi::tLocationData& data)
{
	if (data.is_home)
		m_car.home_state = AtHome;
	else
		m_car.home_state = NotAtHome;

	m_car.is_driving = data.is_driving;

	Debug(DEBUG_NORM, "Location: %f %f Speed: %d Home: %s", data.latitude, data.longitude, data.speed, data.is_home ? "true" : "false");
}

bool CeVehicle::GetClimateState()
{
	CVehicleApi::tClimateData reply{};

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
	SendTemperature(VEHICLE_TEMP_INSIDE, data.inside_temp);
	SendTemperature(VEHICLE_TEMP_OUTSIDE, data.outside_temp);
	SendSwitch(VEHICLE_SWITCH_CLIMATE, m_car.climate_on);
	SendSwitch(VEHICLE_SWITCH_DEFROST, m_car.defrost);
}

bool CeVehicle::GetChargeState()
{
	CVehicleApi::tChargeData reply{};

	if (m_api->GetChargeData(reply))
	{
		UpdateChargeData(reply);
		return ConditionalReturn(true, Get_Charge_State);
	}

	return ConditionalReturn(false, Get_Charge_State);
}

void CeVehicle::UpdateChargeData(CVehicleApi::tChargeData& data)
{
	SendPercentage(VEHICLE_LEVEL_BATTERY, data.battery_level);
	m_car.connected = data.is_connected;
	m_car.charging = data.is_charging;
	m_car.charge_state = data.status_string;
	m_car.charge_limit = data.charge_limit;
	SendSwitch(VEHICLE_SWITCH_CHARGE, m_car.charging);
	SendValueSwitch(VEHICLE_SWITCH_MAX_CHARGE, m_car.charge_limit);
}

void CeVehicle::UpdateVehicleData(CVehicleApi::tVehicleData& data)
{
	SendCounter(VEHICLE_COUNTER_ODO, data.odo);
	if (data.car_open && !m_car.is_driving)
		SendAlert(VEHICLE_ALERT_LOCK, 4, data.car_open_message);
	else
		SendAlert(VEHICLE_ALERT_LOCK, 1, data.car_open_message);
}

void CeVehicle::UpdateCustomVehicleData(CVehicleApi::tCustomData& data)
{
	if (!data.customdata.empty())
	{
		try
		{
			uint8_t cnt = 0;
			do
			{
				Json::Value iter;

				iter = data.customdata[cnt];

				//_log.Debug(DEBUG_NORM, "Starting to process custom data %d - %s", cnt, iter.asString().c_str());

				if (!(iter["id"].empty() || iter["value"].empty() || iter["label"].empty()))
				{
						int iChildID = iter["id"].asInt();
						Json::Value jValue = iter["value"];
						std::string sLabel = iter["label"].asString();
						std::string sValue = jValue.asString();
						bool isBool = jValue.isBool();
						bool bValue = false;

						// Determine if Value might a boolean value
						if(isBool)
						{
							bValue = jValue.asBool();
						}
						else	// Not a native JSON boolean anyway
						{
							std::string sBoolValue = sValue;
							stdlower(sBoolValue);
							isBool = (sBoolValue == "true" || sBoolValue == "false");
							bValue = (sBoolValue == "true");
						}

						_log.Debug(DEBUG_NORM, "Processing custom data %d - %s - %s (%s)", iChildID, sValue.c_str(), sLabel.c_str(), (isBool ? "Boolean" : (is_number(sValue) ? "Number": "Text" )));

						if (is_number(sValue))
						{
							float fValue = static_cast<float>(std::atof(sValue.c_str()));
							SendCustom(VEHICLE_CUSTOM, iChildID, fValue, sLabel);
						}
						else if (isBool)
						{
							SendCustomSwitch(VEHICLE_CUSTOM, iChildID, bValue, sLabel);
						}
						else
						{
							SendCustomText(VEHICLE_CUSTOM, iChildID, sValue, sLabel);
						}
				}
				cnt++;
			} while (!data.customdata[cnt].empty());
			data.customdata.clear();
		}
		catch(const std::exception& e)
		{
			_log.Debug(DEBUG_NORM, "Crashed processing custom data %s!", e.what());
			data.customdata.clear();
		}
	}
}
