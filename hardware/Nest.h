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
	CNest(int ID, const std::string &Username, const std::string &Password);
	~CNest() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);
	void SetProgramState(int newState);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void UpdateSwitch(unsigned char Idx, bool bOn, const std::string &defaultname);
	void UpdateSmokeSensor(unsigned char Idx, bool bOn, const std::string &defaultname);
	bool SetAway(unsigned char Idx, bool bIsAway);
	bool SetManualEcoMode(unsigned char Idx, bool bIsManualEcoMode);
	bool Login();
	void Logout();

      private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_TransportURL;
	std::string m_AccessToken;
	std::string m_UserID;
	//	std::string m_Serial;
	// std::string m_StructureID;
	std::shared_ptr<std::thread> m_thread;
	std::map<int, _tNestThemostat> m_thermostats;
	bool m_bDoLogin;
};
