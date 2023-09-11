#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class CThermosmart : public CDomoticzHardwareBase
{
      public:
	CThermosmart(int ID, const std::string &Username, const std::string &Password, int Mode1);
	~CThermosmart() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);

      private:
	void SendSetPointSensor(unsigned char Idx, float Temp, const std::string &defaultname);
	void SetPauseStatus(bool bIsPause);
	void SetOutsideTemp(float temp);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	void SendOutsideTemperature();
	bool Login();
	void Logout();
	void Init();
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
