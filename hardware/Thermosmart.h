#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class CThermosmart : public CDomoticzHardwareBase
{
public:
	CThermosmart(const int ID, const std::string &Username, const std::string &Password, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	~CThermosmart(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void SetPauseStatus(const bool bIsPause);
	void SetOutsideTemp(const float temp);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	void SendOutsideTemperature();
	bool Login();
	void Logout();
	void Init();
	void SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_AccessToken;
	std::string m_ThermostatID;
	int m_OutsideTemperatureIdx;
	std::shared_ptr<std::thread> m_thread;

	bool m_bDoLogin;
	int m_LastMinute;
};

