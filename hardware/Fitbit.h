#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CFitbit : public CDomoticzHardwareBase
{
public:
	CFitbit(const int ID, const std::string& username, const std::string& password);
	~CFitbit(void);

	bool WriteToHardware(const char *, const unsigned char);
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
	bool ParseFitbitGetResponse(const std::string &sResult);

	bool Login();
	bool RefreshToken(const bool bForce = false);
	bool LoadRefreshToken();
	void StoreRefreshToken();
	bool m_isLogged;
};

