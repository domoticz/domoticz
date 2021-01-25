#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

class CToonThermostat : public CDomoticzHardwareBase
{
      public:
	CToonThermostat(int ID, const std::string &Username, const std::string &Password, const int &Agreement);
	~CToonThermostat() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);
	void SetProgramState(int newState);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

	bool ParseThermostatData(const Json::Value &root);
	bool ParseDeviceStatusData(const Json::Value &root);
	bool ParsePowerUsage(const Json::Value &root);
	bool ParseGasUsage(const Json::Value &root);

	void SendSetPointSensor(unsigned char Idx, float Temp, const std::string &defaultname);
	void UpdateSwitch(unsigned char Idx, bool bOn, const std::string &defaultname);

	bool GetUUIDIdx(const std::string &UUID, int &idx);
	bool AddUUID(const std::string &UUID, int &idx);
	bool GetUUIDFromIdx(int idx, std::string &UUID);

	bool SwitchLight(const std::string &UUID, int SwitchState);
	bool SwitchAll(int SwitchState);

	double GetElectricOffset(int idx, double currentKwh);

	bool Login();
	void Logout();
	std::string GetRandom();

      private:
	std::string m_UserName;
	std::string m_Password;
	int m_Agreement;
	std::string m_ClientID;
	std::string m_ClientIDChecksum;
	std::shared_ptr<std::thread> m_thread;

	uint32_t m_LastUsage1;
	uint32_t m_LastUsage2;
	uint32_t m_OffsetUsage1;
	uint32_t m_OffsetUsage2;
	uint32_t m_LastDeliv1;
	uint32_t m_LastDeliv2;
	uint32_t m_OffsetDeliv1;
	uint32_t m_OffsetDeliv2;

	bool m_bDoLogin;
	P1Power m_p1power;
	P1Gas m_p1gas;
	time_t m_lastSharedSendElectra;
	time_t m_lastSharedSendGas;
	uint32_t m_lastgasusage;
	uint32_t m_lastelectrausage;
	uint32_t m_lastelectradeliv;

	int m_poll_counter;
	int m_retry_counter;

	std::map<int, double> m_LastElectricCounter;
	std::map<int, double> m_OffsetElectricUsage;
};
