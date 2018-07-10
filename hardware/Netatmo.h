#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CNetatmo : public CDomoticzHardwareBase
{
public:
	CNetatmo(const int ID, const std::string& username, const std::string& password);
	~CNetatmo(void);

	bool WriteToHardware(const char *, const unsigned char) override;
	void SetSetpoint(int idx, const float temp);
	bool SetProgramState(const int idx, const int newState);
private:
	enum _eNetatmoType
	{
		NETYPE_WEATHER_STATION=0,
		NETYPE_HOMECOACH,
		NETYPE_ENERGY
	};
	std::string m_clientId;
	std::string m_clientSecret;
	std::string m_username;
	std::string m_password;
	std::string m_accessToken;
	std::string m_refreshToken;
	std::map<int, std::string > m_thermostatDeviceID;
	std::map<int, std::string > m_thermostatModuleID;
	bool m_bPollThermostat;
	bool m_bPollWeatherData;
	bool m_bFirstTimeThermostat;
	bool m_bFirstTimeWeatherData;
	bool m_bForceSetpointUpdate;
	time_t m_tSetpointUpdateTime;

	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;

	time_t m_nextRefreshTs;

	std::map<int,float> m_RainOffset;
	std::map<int, float> m_OldRainCounter;

	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	std::string MakeRequestURL(const _eNetatmoType NetatmoType);
	void GetMeterDetails();
	void GetThermostatDetails();
	bool ParseNetatmoGetResponse(const std::string &sResult, const _eNetatmoType NetatmoType, const bool bIsThermostat);
	bool ParseHomeData(const std::string &sResult);
	bool ParseHomeStatus(const std::string &sResult);
	bool SetAway(const int idx, const bool bIsAway);

	bool Login();
	bool RefreshToken(const bool bForce = false);
	bool LoadRefreshToken();
	void StoreRefreshToken();
	bool m_isLogged;
	bool m_bForceLogin;
	_eNetatmoType m_NetatmoType;

	int m_ActHome;
	std::string m_Home_ID;
	std::map<std::string, std::string> m_RoomNames;
	std::map<std::string, int> m_RoomIDs;
	std::map<std::string, std::string> m_ModuleNames;
	std::map<std::string, int> m_ModuleIDs;

	int GetBatteryLevel(const std::string &ModuleType, int battery_percent);
	bool ParseDashboard(const Json::Value &root, const int DevIdx, const int ID, const std::string &name, const std::string &ModuleType, const int battery_percent, const int rf_status);
};

