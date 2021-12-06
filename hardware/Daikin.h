#pragma once

#include "DomoticzHardware.h"

class CDaikin : public CDomoticzHardwareBase
{
public:
	CDaikin(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &username, const std::string &password, const int poll);
	~CDaikin() override = default;

	bool WriteToHardware(const char *pdata, unsigned char length) override;

private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void GetControlInfo();
	void GetSensorInfo();
	void GetBasicInfo();
	void GetYearPower();
	void UpdateSwitchNew(unsigned char Idx, int SubUnit, bool bOn, double Level, const std::string &defaultname);
	void InsertUpdateSwitchSelector(uint32_t Idx, bool bIsOn, int level, const std::string &defaultname);
	bool SetSetpoint(int idx, float temp);
	bool SetGroupOnOFF(bool OnOFF);
	bool SetLedOnOFF(bool OnOFF);
	bool SetModeLevel(int NewLevel);
	bool SetF_RateLevel(int NewLevel);
	bool SetF_DirLevel(int NewLevel);
	void AggregateSetControlInfo(const char *Temp, const char *OnOFF, const char *ModeLevel, const char *FRateLevel, const char *FDirLevel, const char *Hum);
	void HTTPSetControlInfo();

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
	std::shared_ptr<std::thread> m_thread;
	int m_sec_counter;
	int m_poll;
	std::string m_dt[8]; // Memorized Temp target for each mode.
	std::string m_dh[8]; // Memorized Humidity target for each mode.
	std::string m_sci_Temp;
	std::string m_sci_OnOFF;
	std::string m_sci_ModeLevel;
	std::string m_sci_FRateLevel;
	std::string m_sci_FDirLevel;
	std::string m_sci_Hum;
	bool m_force_sci;
	time_t m_last_setcontrolinfo;
	time_t m_first_setcontrolinfo;
};
