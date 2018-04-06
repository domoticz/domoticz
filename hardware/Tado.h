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
		CTado::CTado(const int ID, const std::string &username, const std::string &password);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void SetSetpoint(const int idx, const float temp);
		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();

		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;

		/* void SetProgramState(const int newState);
		void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
		void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);
		bool SetAway(const unsigned char Idx, const bool bIsAway);
		void Logout();
		void GetMeterDetails();
		*/

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

		bool GetTadoApiEnvironment(std::string url);
		bool Login();
		bool GetHomes();
		bool GetZones(const _tTadoHome & TadoHome);
		bool GetAuthToken(std::string & authtoken, std::string & refreshtoken, const bool refreshUsingToken);
		bool GetZoneState(const int HomeIndex, const int ZoneIndex, const _tTadoHome home, _tTadoZone &zone);
		void SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
		void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string & defaultname);

		bool CancelSetpointOverlay(const int Idx);

		std::string m_TadoUsername;
		std::string m_TadoPassword;
		std::string m_TadoAuthToken;
		std::string m_TadoRefreshToken;
		std::string m_TadoApiClientSecret;
		std::string m_TadoApiEndpoint;
		std::string m_TadoRestApiV2Endpoint;
		std::string m_TadoApiClientId;

		bool m_bDoLogin; // Should we try to login?
		int m_timesUntilTokenRefresh;

		std::map<int, _tTadoZone> m_TadoZones;
		std::map<int, _tTadoHome> m_TadoHomes;

};