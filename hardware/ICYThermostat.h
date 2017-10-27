#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

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
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);

	bool GetSerialAndToken();

	std::string m_UserName;
	std::string m_Password;
	std::string m_SerialNumber;
	std::string m_Token;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	_eICYCompanyMode m_companymode;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

