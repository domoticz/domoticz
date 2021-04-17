#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

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
	CNestOAuthAPI(int ID, const std::string &apikey, const std::string &extradata);
	~CNestOAuthAPI() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);
	bool SetManualEcoMode(unsigned char node_id, bool bIsOn);
	bool PushToNestApi(const std::string &sMethod, const std::string &sUrl, const Json::Value &jPostData, std::string &sResult);
	void SetProgramState(int newState);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void SendSetPointSensor(unsigned char Idx, float Temp, const std::string &defaultname);
	void UpdateSwitch(unsigned char Idx, bool bOn, const std::string &defaultname);
	void UpdateSmokeSensor(unsigned char Idx, bool bOn, const std::string &defaultname);
	bool SetAway(unsigned char Idx, bool bIsAway);
	bool ValidateNestApiAccessToken(const std::string &accesstoken);
	bool Login();
	void Logout();
	std::string FetchNestApiAccessToken(const std::string &productid, const std::string &secret, const std::string &pincode);
	bool SetOAuthAccessToken(unsigned int ID, std::string &newToken);

      private:
	std::string m_UserName;
	std::string m_Password;
	std::string m_TransportURL;
	std::string m_AccessToken;
	std::string m_UserID;
	std::shared_ptr<std::thread> m_thread;
	std::map<int, _tNestThemostat> m_thermostats;
	std::map<int, _tNestStructure> m_structures;
	bool m_bDoLogin;

	std::string m_OAuthApiAccessToken;
	std::string m_ProductId;
	std::string m_ProductSecret;
	std::string m_PinCode;
};
