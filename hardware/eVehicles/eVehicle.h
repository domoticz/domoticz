/************************************************************************

eVehicles framework
Author: MrHobbes74 (github.com/MrHobbes74)

21/02/2020 1.0 Creation
13/03/2020 1.1 Added keep asleep support
28/04/2020 1.2 Added new devices (odometer, lock alert, max charge switch)
24/07/2020 1.3 Added new Mercedes Class (KidDigital)

License: Public domain

************************************************************************/
#pragma once

#include "../DomoticzHardware.h"
#include "VehicleApi.h"
#include "../../main/concurrent_queue.h"

class CeVehicle : public CDomoticzHardwareBase
{
public:
	enum eVehicleType {
		Tesla,
		Mercedes
	};

	CeVehicle(const int ID, const eVehicleType vehicletype, const std::string& username, const std::string& password, int defaultinterval, int activeinterval, bool allowwakeup, const std::string& carid);
	~CeVehicle(void);
	bool WriteToHardware(const char* pdata, const unsigned char length) override;
private:
	enum eApiCommandType {
		Send_Climate_Off,
		Send_Climate_On,
		Send_Climate_Defrost,
		Send_Climate_Defrost_Off,
		Send_Charge_Start,
		Send_Charge_Stop,
		Send_Charge_Limit,
		Get_All_States,
		Get_Location_State,
		Get_Charge_State,
		Get_Climate_State,
		Get_Awake_State,
		Wake_Up
	};

	enum eAlertType {
		NotHome,
		Charging,
		NotCharging,
		Idling,
		Sleeping
	};

	enum eWakeState {
		Asleep,
		WakingUp,
		Awake,
		SelfAwake,
		Unknown
	};

	enum eHomeState {
		AtHome,
		NotAtHome,
		AtUnknown
	};

	struct tVehicle {
		bool connected;
		bool charging;
		bool climate_on;
		bool defrost;
		eHomeState home_state;
		bool is_driving;
		int charge_limit;
		eWakeState wake_state;
		std::string charge_state;
	};

	struct tApiCommand {
		eApiCommandType command_type;
		std::string command_parameter;
	};

	void Init();
	bool ConditionalReturn(bool commandOK, eApiCommandType command);

	void Login();
	bool WakeUp();
	bool IsAwake();
	bool GetAllStates();
	bool GetLocationState();
	void UpdateLocationData(CVehicleApi::tLocationData &data);
	bool GetChargeState();
	void UpdateChargeData(CVehicleApi::tChargeData& data);
	bool GetClimateState();
	void UpdateClimateData(CVehicleApi::tClimateData& data);
	void UpdateVehicleData(CVehicleApi::tVehicleData& data);
	void UpdateCustomVehicleData(CVehicleApi::tCustomData& data);
	bool DoSetCommand(tApiCommand command);

	void AddCommand(eApiCommandType command_type, std::string command_parameter = "");
	bool DoNextCommand();
	std::string GetCommandString(const eApiCommandType command);
	
	void SendAlert();
	void SendAlert(int alertType, int value, std::string title);
	void SendSwitch(int switchType, bool value);
	void SendValueSwitch(int switchType, int value);
	void SendTemperature(int tempType, float value);
	void SendPercentage(int percType, float value);
	void SendCounter(int countType, float value);
	void SendCustom(int countType, int ChildId, float value, std::string label);
	void SendText(int countType, int ChildId, std::string value, std::string label);

	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::shared_ptr<std::thread> m_thread;

	bool m_loggedin;
	int m_defaultinterval;
	int m_activeinterval;
	bool m_allowwakeup;
	double m_home_lat;
	double m_home_lon;

	tVehicle m_car;
	CVehicleApi *m_api;
	concurrent_queue<tApiCommand> m_commands;
	bool m_setcommand_scheduled;
	int m_command_nr_tries;

	eAlertType m_currentalert;
	std::string m_currentalerttext;
};
