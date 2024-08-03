#pragma once

#include "DomoticzHardware.h"
#include "../main/BaroForecastCalculator.h"

namespace Json
{
	class Value;
} // namespace Json

class CNetatmo : public CDomoticzHardwareBase
{
      private:
        struct _tNetatmoDevice
        {
                std::string ID;
                std::string ModuleName;
                std::string StationName;
                std::vector<std::string> ModulesIDs;
                std::string home_id;
                int SignalLevel;
                int BatteryLevel;
                std::string roomNetatmoID;
                std::string MAC;
                //Json::Value Modules;
        };
	enum _eNetatmoType
	{
		NETYPE_WEATHER_STATION = 0,
		NETYPE_AIRCARE,
		NETYPE_ENERGY,

                NETYPE_MEASURE,
                NETYPE_SETTHERMPOINT,

                NETYPE_THERMOSTAT,                   //OLD API
                NETYPE_HOME,                         //OLD API
                NETYPE_CAMERAS,                      //OLD API
                NETYPE_HOMESDATA,
                NETYPE_STATUS,

                NETYPE_EVENTS,
                NETYPE_SETSTATE,
                NETYPE_SETROOMTHERMPOINT,
                NETYPE_SETTHERMMODE,
                NETYPE_SETPERSONSAWAY,
                NETYPE_SETPERSONSHOME,
                NETYPE_NEWHOMESCHEDULE,
                NETYPE_SYNCHOMESCHEDULE,
                NETYPE_SWITCHHOMESCHEDULE,
                NETYPE_ADDWEBHOOK,
                NETYPE_DROPWEBHOOK,
                NETYPE_PUBLICDATA,
	};
	std::string m_clientId;
	std::string m_clientSecret;
	std::string m_scopes;
        std::string m_Scopes;
	std::string m_redirectUri;
	std::string m_authCode;
	std::string m_username;
	std::string m_password;
	std::string m_accessToken;
	std::string m_refreshToken;
        std::vector<_tNetatmoDevice> m_known_thermotats;
	std::map<int, uint64_t> m_thermostat_IDX;
        std::map<uint64_t, int> m_thermostatDeviceID;
        std::map<int, std::string> m_thermostatModuleID;
        bool m_bPollWeatherData;
        bool m_bPollHomecoachData;
        bool m_bPollHomeData;
        bool m_bPollHomesData;
        bool m_bPollHomeStatus;
        bool m_bPollHome;
        //bool m_bPollMeasureData;
        bool m_bPollThermostat;
	bool m_bFirstTimeThermostat;
	bool m_bFirstTimeWeatherData;
	bool m_bForceSetpointUpdate;
	time_t m_tSetpointUpdateTime;

	std::shared_ptr<std::thread> m_thread;

	time_t m_nextRefreshTs;

	std::map<int, float> m_RainOffset;
	std::map<int, float> m_OldRainCounter;

	std::map<int, bool> m_bNetatmoRefreshed;

	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::string MakeRequestURL(_eNetatmoType NetatmoType, std::string data);

        void GetWeatherDetails();
        void GetHomecoachDetails();
        void GetHomesDataDetails();
        void GetHomeStatusDetails();
        void GetHomeDetails();
//	void GetMeterDetails();
	bool ParseHomeData(const std::string& sResult);
	void GetThermostatDetails();
	int  GetBatteryLevel(const std::string& ModuleType, int battery_percent);
//
        void Get_Picture();
        void Get_Measure(std::string gateway, std::string module_id, std::string scale, std::string type);
        void Get_Events(std::string home_id, std::string device_types, std::string event_id, std::string person_id, std::string device_id, std::string module_id, bool offset, bool size, std::string locale);

        bool ParseStationData(const std::string &sResult, bool bIsThermostat);
        bool ParseHomeStatus(const std::string &sResult, Json::Value& root, std::string& home_id);
        bool ParseEvents(const std::string& sResult, Json::Value& root );

        bool ParseHomeData(const std::string &sResult, Json::Value& root );

        bool SetAway(int idx, bool bIsAway);
        bool SetSchedule(int scheduleId);

        bool Login();
        bool RefreshToken(bool bForce = false);
        bool LoadRefreshToken();
        void StoreRefreshToken();
        bool m_isLogged;
        bool m_bForceLogin;

	_eNetatmoType m_weatherType;
        _eNetatmoType m_homecoachType;
	_eNetatmoType m_energyType;

	int m_ActHome;
        std::vector<std::string> m_homeid;
	std::string m_Home_ID;
	std::string m_Home_name;
        std::string m_Place;

	std::map<std::string, std::string> m_Camera_Name;
        std::map<std::string, std::string> m_Camera_ID;
        std::map<std::string, std::string> m_Smoke_Name;
        std::map<std::string, std::string> m_Smoke_ID;

        std::map<std::string, std::string> m_ThermostatName;
        std::map<std::string, std::string> m_RoomNames;
        std::map<std::string, std::string> m_Types;
        std::map<std::string, Json::Value> m_Room;
        std::map<std::string, std::string> m_Room_mode;
        std::map<std::string, std::string> m_Room_setpoint;
        std::map<std::string, std::string> m_Room_Temp;
        std::map<std::string, std::string> m_RoomIDs;
        std::map<std::string, std::string> m_Module_category;
        std::map<std::string, std::string> m_ModuleNames;
        std::map<std::string, int> m_Module_Bat_Level;
        std::map<std::string, int> m_Module_RF_Level;

        std::map<uint64_t, int> m_ModuleIDs;
        std::map<uint64_t, int> m_DeviceID;
        std::map<uint8_t, std::string> m_DeviceModuleID;
        std::map<uint8_t, std::string> m_LightDeviceID;
        std::map<std::string, std::string> m_DeviceHomeID;
        std::map<std::string, std::string> m_Persons;
        std::map<std::string, std::string> m_PersonsNames;
        std::map<std::string, int> m_PersonsIDs;
        std::map<std::string, std::string> m_PersonUrl;
        std::map<int, std::string> m_ScheduleNames;
        std::map<int, std::string> m_ScheduleIDs;
        int m_selectedScheduleID;
        std::map<int, std::string> m_ZoneNames;
        std::map<int, std::string> m_ZoneIDs;
        std::map<int, std::string> m_ZoneTypes;
        std::map<int, std::string> m_Events_Device_ID;
        std::map<int, std::string> m_thermostats;
	std::map<int, CBaroForecastCalculator> m_forecast_calculators;

        uint64_t convert_mac(std::string mac);
        std::string bool_as_text(bool b);
        uint64_t UpdateValueInt(int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue, const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user);

        bool ParseDashboard(const Json::Value &root, int DevIdx, int ID, std::string &name, const std::string &ModuleType, int battery_percent, int rf_status, std::string& Hardware_ID, std::string& home_id);

      public:
        CNetatmo(int ID, const std::string &username, const std::string &password);
        ~CNetatmo() override = default;

        bool WriteToHardware(const char *, unsigned char) override;
        //void SetSetpoint(0, (uint8_t)((ID & 0x00FF0000) >> 16), (ID & 0XFF00) >> 8, ID & 0XFF, 1, sp_temp, sName);
        void SetSetpoint(unsigned long ID, float temp);
        bool SetProgramState(int idx, int newState);
        void Get_Respons_API(const _eNetatmoType& NType, std::string& sResult, std::string& home_id, bool& bRet, Json::Value& root, std::string extra_data);
};
