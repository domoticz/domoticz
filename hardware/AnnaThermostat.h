#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CAnnaThermostat : public CDomoticzHardwareBase
{
public:
	CAnnaThermostat(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password);
	~CAnnaThermostat(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool SetAway(const bool bIsAway);

	std::string m_IPAddress;
	unsigned short m_IPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_ThermostatID;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

