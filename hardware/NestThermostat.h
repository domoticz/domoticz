#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

class CNestThermostat : public CDomoticzHardwareBase
{
public:
	CNestThermostat(const int ID, const std::string &Username, const std::string &Password);
	~CNestThermostat(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	bool SetAway(const bool bIsAway);
	bool Login();
	void Logout();

	std::string m_UserName;
	std::string m_Password;
	std::string m_TransportURL;
	std::string m_AccessToken;
	std::string m_UserID;
	std::string m_Serial;
	std::string m_StructureID;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool m_bDoLogin;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

