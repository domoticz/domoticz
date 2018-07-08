#pragma once

#include "DomoticzHardware.h"

class CDaikin : public CDomoticzHardwareBase
{
public:
	CDaikin(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password);
	~CDaikin(void);

	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void GetControlInfo();
	void GetSensorInfo();
	void GetBasicInfo();
	void UpdateSwitchNew(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname);
	void InsertUpdateSwitchSelector(const unsigned char Idx, const bool bIsOn, const int level, const std::string &defaultname);
	void SetGroupOnOFF(const bool OnOFF);
	void SetLedOnOFF(const bool OnOFF);
	void SetModeLevel(const int NewLevel);
	void SetF_RateLevel(const int NewLevel);
	void SetF_DirLevel(const int NewLevel);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Username;
	std::string m_Password;
	std::string m_pow;
	std::string m_mode;
	std::string m_f_rate;
	std::string m_f_dir;
	std::string m_otemp;
	std::string m_stemp;
	std::string m_shum;
	std::string m_led;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

