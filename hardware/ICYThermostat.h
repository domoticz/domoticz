#pragma once

#include "DomoticzHardware.h"

class CICYThermostat : public CDomoticzHardwareBase
{
	enum _eICYCompanyMode
	{
		CMODE_UNKNOWN = 0,
		CMODE_PORTAL,
		CMODE_ENI,
		CMODE_SEC,
	};

      public:
	CICYThermostat(int ID, const std::string &Username, const std::string &Password);
	~CICYThermostat() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	bool GetSerialAndToken();

      private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_SerialNumber;
	std::string m_Token;
	std::shared_ptr<std::thread> m_thread;
	_eICYCompanyMode m_companymode;
};
