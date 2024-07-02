#pragma once

#include "DomoticzHardware.h"

class CAnnaThermostat : public CDomoticzHardwareBase
{
	struct AnnaLocationInfo
	{
		std::string m_ALocationID;
		std::string m_ALocationName;
		std::string m_ALocationType;
	};

      public:
	CAnnaThermostat(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &Username, const std::string &Password);
	~CAnnaThermostat() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);
	void SetProgramState(int newState);
	bool AnnaSetPreset(uint8_t level);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void OnError(const std::exception &e);
	bool CheckLoginData();
	void GetMeterDetails();
	bool SetAway(bool bIsAway);
	bool AnnaToggleProximity(bool bToggle);
	bool AnnaGetLocation();
	bool InitialMessageMigrateCheck();
	void FixUnit();

      private:
	std::string m_IPAddress;
	unsigned short m_IPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_ThermostatID;
	std::string m_ProximityID;
	std::string m_PresetID;
	std::shared_ptr<std::thread> m_thread;
	AnnaLocationInfo m_AnnaLocation;
};
