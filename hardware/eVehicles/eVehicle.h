#pragma once

#include "../DomoticzHardware.h"
#include "VehicleApi.h"
#include "../../main/concurrent_queue.h"

class CeVehicle : public CDomoticzHardwareBase
{
public:
	enum eVehicleType {
		Tesla
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
		SelfAwake
	};

	struct tVehicle {
		bool connected;
		bool charging;
		bool climate_on;
		bool defrost;
		bool is_home;
		eWakeState wake_state;
		std::string charge_state;
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
	bool DoNextCommand();
	bool DoSetCommand(eApiCommandType command);
	std::string GetCommandString(const eApiCommandType command);
	void SendAlert();

	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::shared_ptr<std::thread> m_thread;

	bool m_loggedin;
	int m_defaultinterval;
	int m_activeinterval;
	bool m_allowwakeup;

	tVehicle m_car;
	CVehicleApi *m_api;
	concurrent_queue<eApiCommandType> m_commands;
	bool m_setcommand_scheduled;
	int m_command_nr_tries;

	eAlertType m_currentalert;
	std::string m_currentalerttext;
};

