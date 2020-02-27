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

	CeVehicle(const int ID, const eVehicleType vehicletype, const std::string& username, const std::string& password, int defaultinterval, int activeinterval, const std::string& carid);
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
		Wake_Up
	};

	enum eAlertType {
		NotHome,
		Charging,
		NotCharging,
		Home,
		Offline,
	};

	struct tVehicle {
		bool connected;
		bool charging;
		bool climate_on;
		bool defrost;
		bool is_home;
		bool awake;
	};

	void Init();
	bool ConditionalReturn(bool commandOK, eApiCommandType command);

	bool WakeUp();
	bool GetAllStates();
	bool GetLocationState();
	void UpdateLocationData(CVehicleApi::tLocationData &data);
	bool GetChargeState();
	void UpdateChargeData(CVehicleApi::tChargeData& data);
	bool GetClimateState();
	void UpdateClimateData(CVehicleApi::tClimateData& data);
	bool DoCommand(eApiCommandType command);
	bool DoSetCommand(eApiCommandType command);

	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	std::string GetCommandString(const eApiCommandType command);

	std::shared_ptr<std::thread> m_thread;

	bool m_loggedin;
	int m_trycounter;
	int m_defaultinterval;
	int m_activeinterval;

	tVehicle m_car;
	CVehicleApi *m_api;
	concurrent_queue<eApiCommandType> m_commands;
};

