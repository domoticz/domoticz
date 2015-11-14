#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CNetAtmoWeatherStation : public CDomoticzHardwareBase
{
public:
	CNetAtmoWeatherStation(const int ID, const std::string& username, const std::string& password);
	~CNetAtmoWeatherStation(void);

	bool WriteToHardware(const char *, const unsigned char);
	void SetSetpoint(const int idx, const float temp);
	bool SetProgramState(const int newState);
private:
	std::string m_clientId;
	std::string m_clientSecret;
	std::string m_username;
	std::string m_password;
	std::string m_accessToken;
	std::string m_refreshToken;
	std::string m_thermostatDeviceID;
	std::string m_thermostatModuleID;
	bool m_bPollThermostat;
	bool m_bForceSetpointUpdate;
	time_t m_tSetpointUpdateTime;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	time_t m_nextRefreshTs;

	std::map<int,float> m_RainOffset;
	std::map<int, float> m_OldRainCounter;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
	void GetThermostatDetails();
	bool ParseNetatmoGetResponse(const std::string &sResult, const bool bIsThermostat);
	bool SetAway(const bool bIsAway);

	bool Login();
	bool RefreshToken();
	bool m_isLogged;

	int GetBatteryLevel(const std::string &ModuleType, const int battery_vp);
	bool ParseDashboard(const Json::Value &root, const int ID, const std::string &name, const std::string &ModuleType, const int battery_vp);
};

