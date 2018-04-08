#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"
#include <map>
#include "../json/json.h"

class CTado : public CDomoticzHardwareBase
{

	public:
		~CTado(void);
		CTado(const int ID, const std::string &username, const std::string &password);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void SetSetpoint(const int idx, const float temp);
		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();

		

		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;

	private:
		struct _tTadoZone {
			std::string Id;
			std::string Name;
			std::string HomeId;
			std::string Type;
		};

		struct _tTadoHome {
			std::string Id;
			std::string Name;
		};

		std::map <std::string, std::string> m_TadoEnvironment;

		enum eTadoApiMethod {
			Put,
			Post,
			Get,
			Delete
		};

		bool GetTadoApiEnvironment(std::string url);
		bool Login();
		bool GetHomes();
		bool GetZones(const _tTadoHome & TadoHome);
		bool SendToTadoApi(const eTadoApiMethod eMethod, const std::string sUrl, const std::string sPostData, std::string & sResponse, const std::vector<std::string> & vExtraHeaders, Json::Value & jsDecodedResponse, const bool bDecodeJsonResponse = true, const bool bIgnoreEmptyResponse = false, const bool bSendAuthHeaders = true);
		bool GetAuthToken(std::string & authtoken, std::string & refreshtoken, const bool refreshUsingToken);
		bool GetZoneState(const int HomeIndex, const int ZoneIndex, const _tTadoHome home, _tTadoZone &zone);
		void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
		void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string & defaultname);
		bool CreateOverlay(const int idx, const float temp, const bool heatingenabled, const std::string termination);
		bool CancelOverlay(const int Idx);
		bool MatchValueFromJSKey(const std::string sKeyName, const std::string sJavascriptData, std::string & sValue);

		std::string m_TadoUsername;
		std::string m_TadoPassword;
		std::string m_TadoAuthToken;
		std::string m_TadoRefreshToken;

		bool m_bDoLogin; // Should we try to login?
		bool m_bDoGetHomes;
		bool m_bDoGetZones;
		bool m_bDoGetEnvironment;
		int m_timesUntilTokenRefresh;

		std::map<int, _tTadoZone> m_TadoZones;
		std::map<int, _tTadoHome> m_TadoHomes;
};