#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

class CThermosmart : public CDomoticzHardwareBase
{
public:
	CThermosmart(const int ID, const std::string &Username, const std::string &Password);
	~CThermosmart(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool Login();
	void Logout();

	std::string m_UserName;
	std::string m_Password;
	std::string m_AccessToken;
	std::string m_ThermostatID;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool m_bDoLogin;
	int m_LastMinute;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

