#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

class CToonThermostat : public CDomoticzHardwareBase
{
public:
	CToonThermostat(const int ID, const std::string &Username, const std::string &Password);
	~CToonThermostat(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	void SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);

	bool GetUUIDIdx(const std::string &UUID, int &idx);
	bool AddUUID(const std::string &UUID, int &idx);
	bool GetUUIDFromIdx(const int idx, std::string &UUID);

	bool SwitchLight(const std::string &UUID, const int SwitchState);
	bool SwitchAll(const int SwitchState);

	bool Login();
	void Logout();
	std::string GetRandom();

	std::string m_UserName;
	std::string m_Password;
	std::string m_ClientID;
	std::string m_ClientIDChecksum;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool m_bDoLogin;
	P1Power	m_p1power;
	P1Gas	m_p1gas;
	time_t m_lastSharedSendElectra;
	time_t m_lastSharedSendGas;
	unsigned long m_lastgasusage;
	unsigned long m_lastelectrausage;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

