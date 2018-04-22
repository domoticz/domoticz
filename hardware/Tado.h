#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"
#include <map>
#include "../json/json.h"

using namespace std;

class CTado : public CDomoticzHardwareBase
{

	public:
		~CTado(void);
		CTado(const int ID, const string &username, const string &password);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void SetSetpoint(const int idx, const float temp);
		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();

		vector<string> StringSplit(const string & inputString, const string delimiter, const int maxelements = 0);

		string StringTrim(string str);

		

		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;

	private:
		struct _tTadoZone {
			string Id;
			string Name;
			string HomeId;
			string Type;

			bool operator < (const _tTadoZone& str) const
			{
				return (Id < str.Id);
			}
		};

		struct _tTadoHome {
			string Id;
			string Name;
			vector<_tTadoZone> Zones;

			bool operator < (const _tTadoHome& str) const
			{
				return (Id < str.Id);
			}
		};

		map <string, string> m_TadoEnvironment;

		enum eTadoApiMethod {
			Put,
			Post,
			Get,
			Delete
		};

		bool GetTadoApiEnvironment(string url);
		bool Login();
		bool GetHomes();
		bool GetZones(_tTadoHome & TadoHome);
		bool SendToTadoApi(const eTadoApiMethod eMethod, const string sUrl, const string sPostData, string & sResponse, const vector<string> & vExtraHeaders, Json::Value & jsDecodedResponse, const bool bDecodeJsonResponse = true, const bool bIgnoreEmptyResponse = false, const bool bSendAuthHeaders = true);
		bool GetAuthToken(string & authtoken, string & refreshtoken, const bool refreshUsingToken);
		bool GetZoneState(const int HomeIndex, const int ZoneIndex, const _tTadoHome home, _tTadoZone & zone);
		bool GetHomeState(const int HomeIndex, _tTadoHome & home);
		void SendSetPointSensor(const unsigned char Idx, const float Temp, const string &defaultname);
		void UpdateSwitch(const unsigned char Idx, const bool bOn, const string & defaultname);
		bool CreateOverlay(const int idx, const float temp, const bool heatingenabled, const string termination);
		bool CancelOverlay(const int Idx);
		bool MatchValueFromJSKey(const string sKeyName, const string sJavascriptData, string & sValue);

		string StringReplaceAll(string str, const string & from, const string & to);

		string m_TadoUsername;
		string m_TadoPassword;
		string m_TadoAuthToken;
		string m_TadoRefreshToken;

		bool m_bDoLogin; // Should we try to login?
		bool m_bDoGetHomes;
		bool m_bDoGetZones;
		bool m_bDoGetEnvironment;

		vector<_tTadoHome> m_TadoHomes;
};