#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>
#include "hardwaretypes.h"
#include <map>

class CNest : public CDomoticzHardwareBase
{
	struct _tNestThemostat
	{
		std::string Serial;
		std::string StructureID;
		std::string Name;
	};
public:
	CNest(const int ID, const std::string &Username, const std::string &Password);
	~CNest(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	void UpdateSmokeSensor(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	bool SetAway(const unsigned char Idx, const bool bIsAway);
	bool SetManualEcoMode(const unsigned char Idx, const bool bIsManualEcoMode);
	bool Login();
	void Logout();

	std::string m_UserName;
	std::string m_Password;
	std::string m_TransportURL;
	std::string m_AccessToken;
	std::string m_UserID;
//	std::string m_Serial;
	//std::string m_StructureID;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::map<int, _tNestThemostat> m_thermostats;
	bool m_bDoLogin;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

