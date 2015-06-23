#pragma once

#include "../DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CNetAtmoWeatherStation : public CDomoticzHardwareBase
{
public:
	CNetAtmoWeatherStation(const int ID, const std::string& username, const std::string& password, const std::string& clientIdSecret);
	~CNetAtmoWeatherStation(void);

	bool WriteToHardware(const char *,const unsigned char) { return false; }

	std::string GetClientId() { return m_clientId; }
	std::string GetClientSecret() { return m_clientSecret; }
	std::string GetUsername() { return m_username; }
	std::string GetPassword() { return m_password; }
	std::string GetApplication();

private:
	std::string m_clientId;
	std::string m_clientSecret;
	std::string m_username;
	std::string m_password;
	std::string m_accessToken;
	std::string m_refreshToken;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	time_t m_nextRefreshTs;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();

	bool Login();
	bool RefreshToken();
	bool m_isLogged;

	int GetBatteryLevel(const std::string &ModuleType, const int battery_vp);
	bool ParseDashboard(const Json::Value &root, const int ID, const std::string &name, const std::string &ModuleType, const int battery_vp);

	//void getData(const std::string& type, const std::string& name, const std::set<std::string>& dataTypes, const std::string& deviceId, const std::string& moduleId, const int battery_vp);
};

