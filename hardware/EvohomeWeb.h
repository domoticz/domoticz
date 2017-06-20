/**
 * Json client for UK/EMEA Evohome API
 *
 *  Adapted for integration with Domoticz
 *
 *  Copyright 2017 - gordonb3 https://github.com/gordonb3/evohomeclient
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 *  @license GPL-3.0+ <https://github.com/gordonb3/evohomeclient/blob/master/LICENSE>
 */
#pragma once

#include "EvohomeBase.h"
#include <map>
#include "../json/json.h"


class CEvohomeWeb : public CEvohomeBase
{
	struct zone
	{
		std::string locationId;
		std::string gatewayId;
		std::string systemId;
		std::string zoneId;
		Json::Value *installationInfo;
		Json::Value *status;
		Json::Value schedule;
		std::string hdtemp;
	};

	struct temperatureControlSystem
	{
		std::string locationId;
		std::string gatewayId;
		std::string systemId;
		Json::Value *installationInfo;
		Json::Value *status;
		std::map<int, zone> zones;
	};

	struct gateway
	{
		std::string locationId;
		std::string gatewayId;
		Json::Value *installationInfo;
		Json::Value *status;
		std::map<int, temperatureControlSystem> temperatureControlSystems;
	};


	struct location
	{
		std::string locationId;
		Json::Value *installationInfo;
		Json::Value *status;
		std::map<int, gateway> gateways;
	};
public:
	CEvohomeWeb(const int ID, const std::string &Username, const std::string &Password, const unsigned int refreshrate, const int UseFlags, const unsigned int installation);
	~CEvohomeWeb(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	// base functions
	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	// evohome web commands
	bool StartSession();
	bool GetStatus();
	bool SetSystemMode(uint8_t sysmode);
	bool SetSetpoint(const char *pdata);
	bool SetDHWState(const char *pdata);

	// evohome client library - don't ask about naming convention - these are imported from another project
	bool login(const std::string &user, const std::string &password);
	bool user_account();

	void get_gateways(int location);
	void get_temperatureControlSystems(int location, int gateway);
	void get_zones(int location, int gateway, int temperatureControlSystem);

	bool full_installation();
	bool get_status(int location);
	bool get_status(std::string locationId);

	zone* get_zone_by_ID(std::string zoneId);

	bool has_dhw(temperatureControlSystem *tcs);

	bool get_schedule(std::string zoneId);
	std::string get_next_switchpoint(temperatureControlSystem* tcs, int zone);
	std::string get_next_switchpoint(zone* hz);
	std::string get_next_switchpoint(Json::Value &schedule);
	std::string get_next_switchpoint_ex(Json::Value &schedule, std::string &current_setpoint);


	bool set_system_mode(std::string systemId, int mode);
	bool set_temperature(std::string zoneId, std::string temperature, std::string time_until);
	bool cancel_temperature_override(std::string zoneId);
	bool set_dhw_mode(std::string dhwId, std::string mode, std::string time_until);

	bool verify_datetime(std::string datetime);

	// status readers
	void DecodeControllerMode(temperatureControlSystem* tcs);
	void DecodeDHWState(temperatureControlSystem* tcs);
	void DecodeZone(zone* hz);

	// helpers
	uint8_t GetUnit_by_ID(unsigned long evoID);
	std::string local_to_utc(std::string local_time);

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

	std::string m_username;
	std::string m_password;
	bool m_updatedev;
	bool m_showschedule;
	int m_refreshrate;

	int m_tzoffset;
	int m_lastDST;
	bool m_loggedon;
	int m_logonfailures;
	unsigned long m_zones[13];
	time_t m_sessiontimer;
	bool m_bequiet;
	bool m_showlocation;
	std::string m_szlocationName;
	int m_lastconnect;

	uint8_t m_locationId;
	uint8_t m_gatewayId;
	uint8_t m_systemId;
	double m_awaysetpoint;
	bool m_showhdtemps;
	uint8_t m_hdprecision;
	int m_wdayoff;


	static const uint8_t m_dczToEvoWebAPIMode[7];
	static const std::string weekdays[7];
	std::vector<std::string> m_SessionHeaders;

	Json::Value m_j_fi;
	Json::Value m_j_stat;

	std::string m_evouid;
	std::map<int, location> m_locations;

	temperatureControlSystem* m_tcs;

	// Evohome v1 API
	std::string m_v1uid;
	std::vector<std::string> m_v1SessionHeaders;

	bool v1_login(const std::string &user, const std::string &password);
	void get_v1_temps();
};

