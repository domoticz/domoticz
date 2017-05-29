/**
 * Place holder for Integrated Evohome client for Domoticz
 *
 *  Copyright 2017 - gordonb3 https://github.com/gordonb3/evohomeclient
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <https://github.com/gordonb3/evohomeclient/blob/master/LICENSE>
 *
 *
 *
 *
 */

#pragma once

#include "EvohomeBase.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include <map>

#include "../json/json.h"


class CEvohomeWeb : public CEvohomeBase
{
public:
	CEvohomeWeb(const int ID, const std::string &Username, const std::string &Password, const unsigned int refreshrate, const bool notupdatedev, const bool showschedule, const bool showlocation, const unsigned int installation);
	~CEvohomeWeb(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);


private:
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
	unsigned long m_zones [13];
	time_t m_sessiontimer;
	bool m_bequiet;
	bool m_showlocation;
	std::string m_szlocationName;

	uint8_t m_locationId;
	uint8_t m_gatewayId;
	uint8_t m_systemId;
	double m_awaysetpoint;


	static const uint8_t m_dczToEvoWebAPIMode[7];
	static const std::string weekdays[7];
	std::vector<std::string> LoginHeaders;
	std::vector<std::string> SessionHeaders;

	Json::Value j_fi;
	Json::Value j_stat;

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

	// evohome client library
	std::map<std::string,std::string> auth_info;
	std::map<std::string,std::string> account_info;
	struct curl_slist *evoheader;

	std::string send_receive_data(std::string url, curl_slist *header);
	std::string send_receive_data(std::string url, std::string postdata, curl_slist *header);
	std::string put_receive_data(std::string url, std::string postdata, curl_slist *header);

	bool login(std::string user, std::string password);
	bool user_account();

	void get_gateways(int location);
	void get_temperatureControlSystems(int location, int gateway);
	void get_zones(int location, int gateway, int temperatureControlSystem);

	struct zone
	{
		std::string locationId;
		std::string gatewayId;
		std::string systemId;
		std::string zoneId;
		Json::Value *installationInfo;
		Json::Value *status;
		Json::Value schedule;
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

	std::map<int, location> locations;

	bool full_installation();
	bool get_status(int location);
	bool get_status(std::string locationId);

	location* get_location_by_ID(std::string locationId);
	gateway* get_gateway_by_ID(std::string gatewayId);
	temperatureControlSystem* get_temperatureControlSystem_by_ID(std::string systemId);
	zone* get_zone_by_ID(std::string zoneId);
	temperatureControlSystem* get_zone_temperatureControlSystem(zone* zone);

	bool has_dhw(int location, int gateway, int temperatureControlSystem);
	bool has_dhw(temperatureControlSystem *tcs);
	bool is_single_heating_system();

	bool get_schedule(std::string zoneId);

	std::string get_next_switchpoint(temperatureControlSystem* tcs, int zone);
	std::string get_next_switchpoint(std::string zoneId);
	std::string get_next_switchpoint(zone* hz);
	std::string get_next_switchpoint(Json::Value schedule);
	std::string get_next_switchpoint_ex(Json::Value schedule, std::string &current_setpoint);

	bool set_system_mode(std::string systemId, int mode, std::string date_until);
	bool set_system_mode(std::string systemId, int mode);

	std::string json_get_val(std::string s_json, const char* key);
	std::string json_get_val(Json::Value *j_json, const char* key);
	std::string json_get_val(std::string s_json, const char* key1, const char* key2);
	std::string json_get_val(Json::Value *j_json, const char* key1, const char* key2);

	bool set_temperature(std::string zoneId, std::string temperature, std::string time_until);
	bool set_temperature(std::string zoneId, std::string temperature);
	bool cancel_temperature_override(std::string zoneId);

	bool set_dhw_mode(std::string dhwId, std::string mode, std::string time_until);
	bool set_dhw_mode(std::string systemId, std::string mode);

	bool verify_date(std::string date);
	bool verify_datetime(std::string datetime);


	// status readers
	void DecodeControllerMode(temperatureControlSystem* tcs);
	void DecodeDHWState(temperatureControlSystem* tcs);
	void DecodeZone(zone* hz);

	// helpers
	uint8_t GetUnit_by_ID(unsigned long evoID);
	std::string local_to_utc(std::string local_time);

	temperatureControlSystem* m_tcs;
};

