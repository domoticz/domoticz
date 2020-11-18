#pragma once

#include "DomoticzHardware.h"

class CAtagOne : public CDomoticzHardwareBase
{
      public:
	CAtagOne(int ID, const std::string &Username, const std::string &Password, int Mode1, int Mode2, int Mode3, int Mode4, int Mode5, int Mode6);
	~CAtagOne() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);

      private:
	void SetPauseStatus(bool bIsPause);
	void SetOutsideTemp(float temp);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	void SendOutsideTemperature();
	bool Login();
	void Logout();
	std::string GetRequestVerificationToken(const std::string &url);
	void Init();
	void SetModes(int Mode1, int Mode2, int Mode3, int Mode4, int Mode5, int Mode6);
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

      private:
	std::string m_UserName;
	std::string m_Password;

	std::string m_ThermostatID;

	bool m_bDoLogin;

	int m_OutsideTemperatureIdx;
	std::shared_ptr<std::thread> m_thread;

	int m_LastMinute;
};
