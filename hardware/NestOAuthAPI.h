#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

namespace Json
{
	class Value;
};

class CNestOAuthAPI : public CDomoticzHardwareBase
{

	struct _tNestStructure
	{
		std::string Name;
		std::string StructureId;
	};

	struct _tNestThemostat
	{
		std::string Serial;
		std::string StructureID;
		std::string Name;
		bool CanHeat;
		bool CanCool;
	};
public:
	CNestOAuthAPI(const int ID, const std::string &apikey, const std::string &extradata);
	~CNestOAuthAPI(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
	bool SetManualEcoMode(const unsigned char node_id, const bool bIsOn);
	bool PushToNestApi(const std::string &sMethod, const std::string &sUrl, const Json::Value &jPostData, std::string & sResult);
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
	bool ValidateNestApiAccessToken(const std::string & accesstoken);
	bool Login();
	void Logout();
	std::string FetchNestApiAccessToken(const std::string &productid, const std::string &secret, const std::string &pincode);
	bool SetOAuthAccessToken(const unsigned int ID, std::string &newToken);
private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_TransportURL;
	std::string m_AccessToken;
	std::string m_UserID;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
	std::map<int, _tNestThemostat> m_thermostats;
	std::map<int, _tNestStructure> m_structures;
	bool m_bDoLogin;

	std::string m_OAuthApiAccessToken;
	std::string m_ProductId;
	std::string m_ProductSecret;
	std::string m_PinCode;
};

