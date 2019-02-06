#pragma once

#include "DomoticzHardware.h"

// setup for different formats XML from the ANNA/Smile

#define ANNA_VERSION_UNKNOWN 0
#define ANNA_VERSION_LF 1

class CAnnaThermostat : public CDomoticzHardwareBase
{
public:
	CAnnaThermostat(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password);
	~CAnnaThermostat(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
	static int m_AnnaVersion;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
    void AnnaVersionCheck();
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool SetAway(const bool bIsAway);
private:
   	std::string m_IPAddress;
	unsigned short m_IPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_ThermostatID;
	std::shared_ptr<std::thread> m_thread;
};
 
	