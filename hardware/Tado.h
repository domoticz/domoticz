#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

class CTado : public CDomoticzHardwareBase
{

      public:
	~CTado() override = default;
	CTado(int ID, const std::string &username, const std::string &password);
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int id2, int id3, int id4, float temp);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::shared_ptr<std::thread> m_thread;
	struct _tTadoZone
	{
		std::string Id;
		std::string Name;
		std::string HomeId;
		std::string Type;
		bool OpenWindowDetectionSupported;

		bool operator<(const _tTadoZone &str) const
		{
			return (Id < str.Id);
		}
	};

	struct _tTadoHome
	{
		std::string Id;
		std::string Name;
		std::vector<_tTadoZone> Zones;

		bool operator<(const _tTadoHome &str) const
		{
			return (Id < str.Id);
		}
	};

	std::map<std::string, std::string> m_TadoEnvironment;

	enum eTadoApiMethod
	{
		Put,
		Post,
		Get,
		Delete
	};

	bool GetTadoApiEnvironment(const std::string &url);
	bool Login();
	bool GetHomes();
	bool GetZones(_tTadoHome &TadoHome);
	bool SendToTadoApi(eTadoApiMethod eMethod, const std::string &sUrl, const std::string &sPostData, std::string &sResponse, const std::vector<std::string> &vExtraHeaders,
			   Json::Value &jsDecodedResponse, bool bDecodeJsonResponse = true, bool bIgnoreEmptyResponse = false, bool bSendAuthHeaders = true);
	bool GetAuthToken(std::string &authtoken, std::string &refreshtoken, bool refreshUsingToken);
	bool GetZoneState(int HomeIndex, int ZoneIndex, const _tTadoHome &home, _tTadoZone &zone);
	bool GetHomeState(int HomeIndex, _tTadoHome &home);
	void SendSetPointSensor(int Idx, float Temp, const std::string &defaultname);
	void UpdateSwitch(int Idx, bool bOn, const std::string &defaultname);
	bool CreateOverlay(int idx, float temp, bool heatingenabled, const std::string &termination = "TADO_MODE");
	bool CancelOverlay(int Idx);
	bool MatchValueFromJSKey(const std::string &sKeyName, const std::string &sJavascriptData, std::string &sValue);
	std::vector<std::string> StringSplitEx(const std::string &inputString, const std::string &delimiter, int maxelements = 0);

      private:
	std::string m_TadoUsername;
	std::string m_TadoPassword;
	std::string m_TadoAuthToken;
	std::string m_TadoRefreshToken;

	bool m_bDoLogin; // Should we try to login?
	bool m_bDoGetHomes;
	bool m_bDoGetZones;
	bool m_bDoGetEnvironment;

	std::vector<_tTadoHome> m_TadoHomes;
};
