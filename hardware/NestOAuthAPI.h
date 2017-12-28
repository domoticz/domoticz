#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"
#include <map>

class CNestOAuthAPI : public CDomoticzHardwareBase
{
	struct _tNestThemostat
	{
		std::string Serial;
		std::string StructureID;
		std::string Name;
	};
public:
	CNestOAuthAPI(const int ID, const std::string &apikey, const std::string &extradata);
	~CNestOAuthAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
	void SetProgramState(const int newState);
private:
	void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	void UpdateSmokeSensor(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	bool SetAway(const unsigned char Idx, const bool bIsAway);
	bool Login();
	void Logout();
	std::string FetchNestApiAccessToken(const std::string &productid, const std::string &secret, const std::string &pincode);
	bool SetOAuthAccessToken(const unsigned int ID, std::string &newToken);

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

	std::string m_OAuthApiAccessToken;
	std::string m_ProductId;
	std::string m_ProductSecret;
	std::string m_PinCode;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

