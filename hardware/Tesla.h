#pragma once

#include "DomoticzHardware.h"
#include "TeslaApi.h"
#include "../main/concurrent_queue.h"

class CTesla : public CDomoticzHardwareBase
{
public:
	CTesla(const int ID, const std::string& username, const std::string& password, const std::string& VIN);
	~CTesla(void);
	bool WriteToHardware(const char* pdata, const unsigned char length) override;
private:
	enum eCommandType {
		Send_Climate_Off,
		Send_Climate_Comfort,
		Send_Climate_Defrost,
		Send_Climate_Defrost_Off,
		Send_Charge_Start,
		Send_Charge_Stop,
		Get_Climate,
		Get_Charge,
		Get_Drive
	};

	enum eAlertType {
		NotHome,
		Charging,
		NotCharging,
		Home,
		Offline,
	};

	struct tTesla {
		std::string VIN;
		bool connected;
		bool charging;
		bool climate_on;
		bool defrost;
		bool is_home;
		bool awake;
	};

	void Init();
	bool Reset(std::string errortext);
	bool WakeUp();
	bool GetDriveStatus();
	bool GetChargeStatus();
	bool GetClimateStatus();
	bool DoCommand(eCommandType command);
	bool DoSetCommand(eCommandType command);
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	std::string GetCommandString(const eCommandType command);
private:
	std::shared_ptr<std::thread> m_thread;

	std::string m_username;
	std::string m_password;
	bool m_loggedin;
	int m_trycounter;

	tTesla m_car;
	CTeslaApi m_api;
	concurrent_queue<eCommandType> m_commands;
};

