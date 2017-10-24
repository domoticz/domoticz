#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>
#include "hardwaretypes.h"

namespace Json
{
	class Value;
};

class CToonThermostat : public CDomoticzHardwareBase
{
public:
	CToonThermostat(const int ID, const std::string &Username, const std::string &Password, const int &Agreement);
	~CToonThermostat(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	bool ParseThermostatData(const Json::Value &root);
	bool ParseDeviceStatusData(const Json::Value &root);
	bool ParsePowerUsage(const Json::Value &root);
	bool ParseGasUsage(const Json::Value &root);

	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);

	bool GetUUIDIdx(const std::string &UUID, int &idx);
	bool AddUUID(const std::string &UUID, int &idx);
	bool GetUUIDFromIdx(const int idx, std::string &UUID);

	bool SwitchLight(const std::string &UUID, const int SwitchState);
	bool SwitchAll(const int SwitchState);

	double GetElectricOffset(const int idx, const double currentKwh);

	bool Login();
	void Logout();
	std::string GetRandom();

	std::string m_UserName;
	std::string m_Password;
	int m_Agreement;
	std::string m_ClientID;
	std::string m_ClientIDChecksum;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	unsigned long m_LastUsage1;
	unsigned long m_LastUsage2;
	unsigned long m_OffsetUsage1;
	unsigned long m_OffsetUsage2;
	unsigned long m_LastDeliv1;
	unsigned long m_LastDeliv2;
	unsigned long m_OffsetDeliv1;
	unsigned long m_OffsetDeliv2;

	bool m_bDoLogin;
	P1Power	m_p1power;
	P1Gas	m_p1gas;
	time_t m_lastSharedSendElectra;
	time_t m_lastSharedSendGas;
	unsigned long m_lastgasusage;
	unsigned long m_lastelectrausage;
	unsigned long m_lastelectradeliv;

	int m_poll_counter;
	int m_retry_counter;

	std::map<int, double> m_LastElectricCounter;
	std::map<int, double> m_OffsetElectricUsage;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

