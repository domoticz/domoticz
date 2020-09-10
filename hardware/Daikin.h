#pragma once

#include "DomoticzHardware.h"

class CDaikin : public CDomoticzHardwareBase
{
public:
	CDaikin(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password);
	~CDaikin(void);

	bool WriteToHardware(const char *pdata, const unsigned char length) override;
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
	bool SetSetpoint(const int idx, const float temp);
	bool SetGroupOnOFF(const bool OnOFF);
	bool SetLedOnOFF(const bool OnOFF);
	bool SetModeLevel(const int NewLevel);
	bool SetF_RateLevel(const int NewLevel);
	bool SetF_DirLevel(const int NewLevel);
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
	std::string m_dt[8];     // Memorized Temp target for each mode. 
        std::string m_dh[8];     // Memorized Humidity target for each mode. 
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
