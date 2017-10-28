#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CAtagOne : public CDomoticzHardwareBase
{
public:
	CAtagOne(const int ID, const std::string &Username, const std::string &Password, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	~CAtagOne(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
private:
	void SetPauseStatus(const bool bIsPause);
	void SetOutsideTemp(const float temp);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	void SendOutsideTemperature();
	bool Login();
	void Logout();
	std::string GetRequestVerificationToken(const std::string &url);

	std::string m_UserName;
	std::string m_Password;

	std::string m_ThermostatID;

	bool m_bDoLogin;

	int m_OutsideTemperatureIdx;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	int m_LastMinute;

	void Init();
	void SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

