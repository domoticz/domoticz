#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

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
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	void UpdateSmokeSensor(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	bool SetAway(const unsigned char Idx, const bool bIsAway);
	bool SetManualEcoMode(const unsigned char Idx, const bool bIsManualEcoMode);
	bool Login();
	void Logout();
private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_TransportURL;
	std::string m_AccessToken;
	std::string m_UserID;
//	std::string m_Serial;
	//std::string m_StructureID;
	std::shared_ptr<std::thread> m_thread;
	std::map<int, _tNestThemostat> m_thermostats;
	bool m_bDoLogin;
};

