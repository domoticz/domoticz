#pragma once

#include "DomoticzHardware.h"

class CICYThermostat : public CDomoticzHardwareBase
{
	enum _eICYCompanyMode {
		CMODE_UNKNOWN=0,
		CMODE_PORTAL,
		CMODE_ENI,
		CMODE_SEC,
	};
public:
	CICYThermostat(const int ID, const std::string &Username, const std::string &Password);
	~CICYThermostat(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool GetSerialAndToken();
private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_SerialNumber;
	std::string m_Token;
	std::shared_ptr<std::thread> m_thread;
	_eICYCompanyMode m_companymode;
};

