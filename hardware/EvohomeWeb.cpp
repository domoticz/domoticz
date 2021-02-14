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
#include "stdafx.h"
#include "EvohomeWeb.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "../httpclient/HTTPClient.h"

#define LOGONFAILTRESHOLD 3
#define MINPOLINTERVAL 10
#define MAXPOLINTERVAL 3600
#define HTTPTIMEOUT 30

#ifdef _WIN32
#define gmtime_r(timep, result) gmtime_s(result, timep)
#endif


//extern std::string szUserDataFolder;

const uint8_t CEvohomeWeb::m_dczToEvoWebAPIMode[7] = { 0,2,3,4,6,1,5 };
const std::string CEvohomeWeb::weekdays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
//const std::string CEvohomeWeb::evo_modes[7] = {"Auto", "HeatingOff", "AutoWithEco", "Away", "DayOff", "", "Custom"};



CEvohomeWeb::CEvohomeWeb(const int ID, const std::string &Username, const std::string &Password, const unsigned int refreshrate, const int UseFlags, const unsigned int installation) :
	m_username(Username),
	m_password(Password),
	m_refreshrate(refreshrate)
{
	m_HwdID = ID;
	m_bSkipReceiveCheck = true;

	m_locationId = (installation >> 12) & 15;
	m_gatewayId = (installation >> 8) & 15;
	m_systemId = (installation >> 4) & 15;


	/* Use Flags
	 *
	 * 0x1 = m_updatedev (let Honeywell server control the device name)
	 * 0x2 = m_showschedule (show next scheduled switch point as `until` time)
	 * 0x4 = m_showlocation (prefix device name with location)
	 * 0x8 = m_showhdtemps (show better precision temps from 'old' US Evohome API)
	 * 0x10 = m_precision (0 = 1 decimal, 1 = 2 decimals)
	 *
	 */

	m_updatedev = ((UseFlags & 1) == 0); // reverted value: default = true
	m_showschedule = ((UseFlags & 2) > 0);
	m_showlocation = ((UseFlags & 4) > 0);
	m_showhdtemps = ((UseFlags & 8) > 0);
	m_hdprecision = ((UseFlags & 16) > 0) ? 100 : 10;

	if (m_refreshrate < 10)
		m_refreshrate = 60;

	m_awaysetpoint = 0; // we'll fetch this from the controller device status later
	m_wdayoff = 6; // saturday
}

CEvohomeWeb::~CEvohomeWeb()
{
	m_bIsStarted = false;
}

void CEvohomeWeb::Init()
{
	m_loggedon = false;
	m_tzoffset = -1;
	m_lastDST = -1;
	m_logonfailures = 0;
	m_bequiet = false;
	m_szlocationName = "";
	m_j_fi.clear();
	m_j_stat.clear();
}


bool CEvohomeWeb::StartSession()
{
	if (!m_bequiet)
		_log.Log(LOG_STATUS, "(%s) connect to Evohome server", m_Name.c_str());
	m_loggedon = false;

	if (!m_v2refresh_token.empty())
	{
		if (!renew_login())
		{
			m_logonfailures++;
			if (m_logonfailures == LOGONFAILTRESHOLD)
				_log.Log(LOG_STATUS, "(%s) logon fail treshold reached - trottling", m_Name.c_str());
			if ((m_logonfailures * m_refreshrate) > MAXPOLINTERVAL)
				m_logonfailures--;
			return false;
		}
	}
	else if (!login(m_username, m_password))
	{
		m_logonfailures++;
		if (m_logonfailures == LOGONFAILTRESHOLD)
			_log.Log(LOG_STATUS, "(%s) logon fail treshold reached - trottling", m_Name.c_str());
		if ((m_logonfailures * m_refreshrate) > MAXPOLINTERVAL)
			m_logonfailures--;
		return false;
	}

	m_sessiontimer = mytime(nullptr) + 3599; // Honeywell will invalidate our session ID after an hour
	m_loggedon = true;
	m_logonfailures = 0;


	// (re)initialize Evohome installation info
	m_zones[0] = 0;
	if (!full_installation())
	{
		_log.Log(LOG_ERROR, "(%s) failed to retrieve installation info from server", m_Name.c_str());
		return false;
	}
	m_tcs = nullptr;
	if (
		(m_locations.size() > m_locationId) &&
		(m_locations[m_locationId].gateways.size() > m_gatewayId) &&
		(m_locations[m_locationId].gateways[m_gatewayId].temperatureControlSystems.size() > m_systemId)
		)
	{
		m_tcs = &m_locations[m_locationId].gateways[m_gatewayId].temperatureControlSystems[m_systemId];
		m_szlocationName = m_j_fi["locations"][m_locationId]["locationInfo"]["name"].asString();
	}
	else
	{
		_log.Log(LOG_ERROR, "(%s) installation at [%d,%d,%d] does not exist - verify your settings", m_Name.c_str(), m_locationId, m_gatewayId, m_systemId);
		return false;
	}

	if (m_awaysetpoint == 0) // first run - try to get our Away setpoint value from the controller device status
	{
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT Extra FROM Hardware WHERE ID=%d", this->m_HwdID);
		if (!result.empty()) // adding hardware
		{
			std::vector<std::string> splitresults;
			StringSplit(result[0][0], ";", splitresults);
			if (!splitresults.empty())
				m_awaysetpoint = strtod(splitresults[0].c_str(), nullptr);
			if (splitresults.size()>1)
				m_wdayoff = atoi(splitresults[1].c_str()) % 7;
		}
		if (m_awaysetpoint == 0)
			m_awaysetpoint = 15; // use default 'Away' setpoint value
	}

	m_bequiet = false;
	return true;
}


bool CEvohomeWeb::StartHardware()
{
	RequestStart();

	if (m_username.empty() || m_password.empty())
		return false;
	Init();
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	if (!m_thread)
		return false;
	m_bIsStarted = true;
	sOnConnected(this);
	return true;
}


bool CEvohomeWeb::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}


void CEvohomeWeb::Do_Work()
{
	int sec_counter = m_refreshrate - MINPOLINTERVAL;
	int pollcounter = LOGONFAILTRESHOLD;
	_log.Log(LOG_STATUS, "(%s) Worker started...", m_Name.c_str());
	m_lastconnect=0;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		m_lastconnect++;
		if (sec_counter % 10 == 0)
			m_LastHeartbeat = mytime(nullptr);

		if ((sec_counter % m_refreshrate == 0) && (pollcounter++ > m_logonfailures) && (m_lastconnect >= MINPOLINTERVAL))
		{
			if (m_loggedon && (m_LastHeartbeat > m_sessiontimer)) // discard our session with the honeywell server
			{
				m_loggedon = false;
				m_bequiet = true;
			}
			GetStatus();
			pollcounter = LOGONFAILTRESHOLD;
			m_lastconnect=0;
		}
	}

	_log.Log(LOG_STATUS, "(%s) Worker stopped...", m_Name.c_str());
}


bool CEvohomeWeb::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!pdata)
		return false;
	if (!m_loggedon && !StartSession())
		return false;
	if (m_j_stat.isNull() && !GetStatus())
		return false;

	m_lastconnect=0;
	switch (pdata[1])
	{
	case pTypeEvohome:
		if (length < sizeof(_tEVOHOME1))
			return false;
		return SetSystemMode(((_tEVOHOME1*)pdata)->status);
		break;
	case pTypeEvohomeZone:
		if (length < sizeof(_tEVOHOME2))
			return false;
		return SetSetpoint(pdata);
		break;
	case pTypeEvohomeWater:
		if (length < sizeof(_tEVOHOME2))
			return false;
		return SetDHWState(pdata);
		break;
	}
	return false; // bad command
}


bool CEvohomeWeb::GetStatus()
{
	if (!m_loggedon && !StartSession())
		return false;
	if (!get_status(m_tcs->locationId))
	{
		m_loggedon = false;
		return false;
	}

	_log.Log(LOG_NORM, "(%s) fetch data from server", m_Name.c_str());

	// system status
	DecodeControllerMode(m_tcs);

	if (m_showhdtemps)
		get_v1_temps();

	// cycle all zones for status
	for (auto &i : m_tcs->zones)
		DecodeZone(&i);

	// hot water status
	if (has_dhw(m_tcs))
		DecodeDHWState(m_tcs);

	return true;
}


bool CEvohomeWeb::SetSystemMode(uint8_t sysmode)
{
	std::string sznewmode = GetWebAPIModeName(sysmode);
	if (set_system_mode(m_tcs->systemId, (int)(m_dczToEvoWebAPIMode[sysmode])))
	{
		_log.Log(LOG_NORM, "(%s) changed system status to %s", m_Name.c_str(), GetControllerModeName(sysmode));

		if (sznewmode == "HeatingOff")
		{
			// cycle my zones to reflect the HeatingOff mode
			for (auto &i : m_tcs->zones)
			{
				zone *hz = &i;
				std::string szId, sztemperature;
				szId = (*hz->installationInfo)["zoneId"].asString();
				if ((m_showhdtemps) && !hz->hdtemp.empty())
					sztemperature = hz->hdtemp;
				else if (hz->status != nullptr)
					sztemperature = (*hz->status)["temperatureStatus"]["temperature"].asString();
				else
				{
					sztemperature = "-";
					sznewmode = "Offline";
				}
				unsigned long evoID = atol(szId.c_str());
				std::stringstream ssUpdateStat;
				ssUpdateStat << sztemperature << ";5;" << sznewmode;
				std::string sdevname;
				m_sql.UpdateValue(this->m_HwdID, szId.c_str(), GetUnit_by_ID(evoID), pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, ssUpdateStat.str().c_str(), sdevname);
			}
			return true;
		}

		// cycle my zones to restore scheduled temps
		for (auto &i : m_tcs->zones)
		{
			zone *hz = &i;
			std::string zonemode;
			if (hz->status == nullptr) // don't touch invalid zone - it should already show as 'Offline'
				continue;
			if (hz->status->isMember("heatSetpointStatus"))
				zonemode = (*hz->status)["heatSetpointStatus"]["setpointMode"].asString();
			if ((zonemode.size() > 9) && (zonemode.substr(9) == "Override")) // don't touch zone if it is in Override mode
				continue;

			std::string sztemperature, szsetpoint;
			std::string szuntil;
			double setpoint = 0;

			/*  there is no strict definition for modes Away, DayOff and Custom so we'll have to wait
			 *  for the next update to get the correct values. But we can make educated guesses
			 */

			// Away unconditionally sets all zones to a preset temperature, even if Normal mode is lower
			if (sznewmode == "Away")
				setpoint = m_awaysetpoint;
			else
			{
				if ((!hz->schedule.isNull()) || get_zone_schedule(hz->zoneId))
				{
					szuntil = local_to_utc(get_next_switchpoint_ex(hz->schedule, szsetpoint));
					setpoint = strtod(szsetpoint.c_str(), nullptr);
				}

				// Eco lowers the setpoint of all zones by 3 degrees, but resets a zone mode to Normal setting
				// if the resulting setpoint is below the Away setpoint
				if ((sznewmode == "AutoWithEco") && (setpoint >= (m_awaysetpoint + 3)))
					setpoint -= 3;
			}

			if ((m_showhdtemps) && !hz->hdtemp.empty())
				sztemperature = hz->hdtemp;
			else if (hz->status != nullptr)
				sztemperature = (*hz->status)["temperatureStatus"]["temperature"].asString();
			else
			{
				sztemperature = "-";
				sznewmode = "Offline";
			}

			unsigned long evoID = atol(hz->zoneId.c_str());
			std::stringstream ssUpdateStat;
			if (setpoint < 5) // there was an error - no schedule?
				ssUpdateStat << sztemperature << ";5;Unknown";
			else
			{
				ssUpdateStat << sztemperature << ";" << setpoint << ";" << sznewmode;
				if ((m_showschedule) && !(szuntil.empty()) && (sznewmode != "Custom"))
					ssUpdateStat << ";" << szuntil;
			}
			std::string sdevname;
			m_sql.UpdateValue(this->m_HwdID, hz->zoneId.c_str(), GetUnit_by_ID(evoID), pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, ssUpdateStat.str().c_str(), sdevname);
		}
		return true;
	}
	_log.Log(LOG_ERROR, "(%s) error changing system status", m_Name.c_str());
	m_loggedon = false;
	return false;
}


bool CEvohomeWeb::SetSetpoint(const char *pdata)
{
	_tEVOHOME2 *pEvo = (_tEVOHOME2*)pdata;
	std::string zoneId(std::to_string((int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3)));

	zone* hz = get_zone_by_ID(zoneId);
	if (hz == nullptr) // zone number not known by installation (manually added?)
	{
		_log.Log(LOG_ERROR, "(%s) attempt to change setpoint on unknown zone", m_Name.c_str());
		return false;
	}

	if ((pEvo->mode) == 0) // cancel override
	{
		if (!cancel_temperature_override(zoneId))
			return false;

		std::string szsetpoint;
		std::string szuntil;
		if ((!hz->schedule.isNull()) || get_zone_schedule(hz->zoneId))
		{
			szuntil = local_to_utc(get_next_switchpoint_ex(hz->schedule, szsetpoint));
			pEvo->temperature = (int16_t)(strtod(szsetpoint.c_str(), nullptr) * 100);
		}

		if ((m_showschedule) && (!szuntil.empty()))
		{
			pEvo->year = (uint16_t)(atoi(szuntil.substr(0, 4).c_str()));
			pEvo->month = (uint8_t)(atoi(szuntil.substr(5, 2).c_str()));
			pEvo->day = (uint8_t)(atoi(szuntil.substr(8, 2).c_str()));
			pEvo->hrs = (uint8_t)(atoi(szuntil.substr(11, 2).c_str()));
			pEvo->mins = (uint8_t)(atoi(szuntil.substr(14, 2).c_str()));
		}
		else
			pEvo->year = 0;
		return true;
	}

	int temperature_int = (int)pEvo->temperature / 100;
	int temperature_frac = (int)pEvo->temperature % 100;
	std::stringstream s_setpoint;
	s_setpoint << temperature_int << "." << temperature_frac;

	if ((pEvo->mode) == 1) // permanent override
	{
		return set_temperature(zoneId, s_setpoint.str(), "");
	}
	if ((pEvo->mode) == 2) // temporary override
	{
		std::string szISODate(CEvohomeDateTime::GetISODate(pEvo));
		return set_temperature(zoneId, s_setpoint.str(), szISODate);
	}
	return false;
}


bool CEvohomeWeb::SetDHWState(const char *pdata)
{
	if (!has_dhw(m_tcs)) // Installation has no Hot Water device
	{
		_log.Log(LOG_ERROR, "(%s) attempt to set state on non existing Hot Water device", m_Name.c_str());
		return false;
	}

	_tEVOHOME2 *pEvo = (_tEVOHOME2*)pdata;

	std::string dhwId(std::to_string((int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3)));

	std::string DHWstate = (pEvo->temperature == 0) ? "off" : "on";

	if ((pEvo->mode) == 0) // cancel override (web front end does not appear to support this?)
	{
		DHWstate = "auto";
	}
	if ((pEvo->mode) <= 1) // permanent override
	{
		return set_dhw_mode(dhwId, DHWstate, "");
	}
	if ((pEvo->mode) == 2) // temporary override
	{
		std::string szISODate(CEvohomeDateTime::GetISODate(pEvo));
		return set_dhw_mode(dhwId, DHWstate, szISODate);
	}
	return false;
}


void CEvohomeWeb::DecodeControllerMode(temperatureControlSystem* tcs)
{
	unsigned long ID = (unsigned long)(strtod(tcs->systemId.c_str(), nullptr));
	std::string szsystemMode, szmodelType;
	uint8_t sysmode = 0;

	if (tcs->status == nullptr)
		szsystemMode = "Unknown";
	else
		szsystemMode = (*tcs->status)["systemModeStatus"]["mode"].asString();
	while (sysmode < 7 && strcmp(szsystemMode.c_str(), m_szWebAPIMode[sysmode]) != 0)
		sysmode++;

	_tEVOHOME1 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME1));
	tsen.len = sizeof(_tEVOHOME1) - 1;
	tsen.type = pTypeEvohome;
	tsen.subtype = sTypeEvohome;
	RFX_SETID3(ID, tsen.id1, tsen.id2, tsen.id3);
	tsen.mode = 0; // web API does not support temp override of controller mode
	tsen.status = sysmode;
	sDecodeRXMessage(this, (const unsigned char *)&tsen, "Controller mode", -1, nullptr);

	if (GetControllerName().empty() || m_updatedev)
	{
		szmodelType = (*tcs->installationInfo)["modelType"].asString();
		SetControllerName(szmodelType);
		if (szmodelType.empty())
			return;

		std::string devname;
		if (m_showlocation)
			devname = m_szlocationName + ": " + szmodelType;
		else
			devname = szmodelType;

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT HardwareID, DeviceID, Name, StrParam1 FROM DeviceStatus WHERE HardwareID=%d AND DeviceID='%s'", this->m_HwdID, tcs->systemId.c_str());
		if (!result.empty() && ((result[0][2] != devname) || (!result[0][3].empty())))
		{
			// also change lastupdate time to allow the web frontend to pick up the change
			time_t now = mytime(nullptr);
			struct tm ltime;
			localtime_r(&now, &ltime);
			// also wipe StrParam1 - we do not also want to call the old (python) script when changing system mode
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d', StrParam1='' WHERE HardwareID=%d AND DeviceID='%s'", devname.c_str(), ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec, this->m_HwdID, tcs->systemId.c_str());
		}
	}
}


void CEvohomeWeb::DecodeZone(zone* hz)
{
	// no sense in using REVOBUF EVOHOME2 to send this to mainworker as this requires breaking up our data
	// only for mainworker to reassemble it.

	std::string szId, sztemperature, szsetpoint, szmode;
	std::string szuntil;
	std::stringstream ssUpdateStat;

	szId = (*hz->installationInfo)["zoneId"].asString();
	if (hz->status == nullptr)
	{
		sztemperature = "-";
		szsetpoint = "-";
		szmode = "Offline";
	}
	else
	{
		sztemperature = ((m_showhdtemps) && !hz->hdtemp.empty()) ? hz->hdtemp : (*hz->status)["temperatureStatus"]["temperature"].asString();
		szsetpoint = (*hz->status)["heatSetpointStatus"]["targetTemperature"].asString();
		if ((m_showhdtemps) && hz->hdtemp.empty())
			szmode = "Offline";
		else
		{
			szmode = (*hz->status)["heatSetpointStatus"]["setpointMode"].asString();
			if (szmode == "TemporaryOverride")
				szuntil = (*hz->status)["heatSetpointStatus"]["until"].asString();
		}
	}

	unsigned long evoID = atol(szId.c_str());
	std::string szsysmode;
	if (m_tcs->status == nullptr)
		szsysmode = "Unknown";
	else
		szsysmode = (*m_tcs->status)["systemModeStatus"]["mode"].asString();
	if ((szsysmode == "Away") && (szmode == "FollowSchedule"))
	{
		double new_awaysetpoint = strtod(szsetpoint.c_str(), nullptr);
		if (m_awaysetpoint != new_awaysetpoint)
		{
			m_awaysetpoint = new_awaysetpoint;
			m_sql.safe_query("UPDATE Hardware SET Extra='%0.2f;%d' WHERE ID=%d", m_awaysetpoint, m_wdayoff, this->m_HwdID);
			_log.Log(LOG_STATUS, "(%s) change Away setpoint to '%0.2f' because of non matching setpoint (%s)", m_Name.c_str(), m_awaysetpoint, (*hz->installationInfo)["name"].asString().c_str());
		}
		ssUpdateStat << sztemperature << ";" << szsetpoint << ";" << szsysmode;
	}
	else if ((szsysmode == "HeatingOff") && (szmode != "Offline"))
		ssUpdateStat << sztemperature << ";" << szsetpoint << ";" << szsysmode;
	else
	{
		ssUpdateStat << sztemperature << ";" << szsetpoint << ";";
		if (szmode != "FollowSchedule")
			ssUpdateStat << szmode;
		else
		{
			ssUpdateStat << szsysmode;
			if (m_showschedule && szuntil.empty())
			{
				if (szsysmode != "Custom") // can't use schedule to find next switchpoint
					szuntil = local_to_utc(get_next_switchpoint(hz));
			}
			if (szsysmode == "DayOff")
			{
				if (szuntil.empty()) // make sure our schedule is populated
					get_next_switchpoint(hz);
				if (szsetpoint != hz->schedule["currentSetpoint"].asString())
				{
					bool found = false;
					m_wdayoff--;
					for (uint8_t i = 0; (i < 7) && !found; i++)
					{
						hz->schedule["nextSwitchpoint"] = ""; // force a recalculation
						m_wdayoff++;
						if (m_wdayoff>6)
							m_wdayoff -= 7;
						get_next_switchpoint(hz);
						found = (szsetpoint == hz->schedule["currentSetpoint"].asString());
					}
					if (found)
					{
						m_sql.safe_query("UPDATE Hardware SET Extra='%0.2f;%d' WHERE ID=%d", m_awaysetpoint, m_wdayoff, this->m_HwdID);

						_log.Log(LOG_STATUS, "(%s) change Day Off schedule reference to '%s' because of non matching setpoint (%s)", m_Name.c_str(), weekdays[m_wdayoff].c_str(), (*hz->installationInfo)["name"].asString().c_str());
					}
					if (m_showschedule)
						szuntil = local_to_utc(get_next_switchpoint(hz));
				}
			}
		}
		if (!szuntil.empty())
			ssUpdateStat << ";" << szuntil;
	}

	std::string sdevname;
	uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, szId.c_str(), GetUnit_by_ID(evoID), pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, ssUpdateStat.str().c_str(), sdevname);

	if (m_updatedev && (DevRowIdx != -1))
	{
		std::stringstream ssnewname;
		if (m_showlocation)
			ssnewname << m_szlocationName << ": ";
		ssnewname << (*hz->installationInfo)["name"].asString();

		if (sdevname != ssnewname.str())
		{
			// also wipe StrParam1 - we do not want a double action from the old (python) script when changing the setpoint
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', StrParam1='' WHERE (ID == %" PRIu64 ")", ssnewname.str().c_str(), DevRowIdx);
			if (sdevname.find("zone ") != std::string::npos)
				_log.Log(LOG_STATUS, "(%s) register new zone '%s'", m_Name.c_str(), ssnewname.str().c_str());
		}
	}

	// Notify MQTT and various push mechanisms
	m_mainworker.sOnDeviceReceived(this->m_HwdID, DevRowIdx, (*hz->installationInfo)["name"].asString(), nullptr);
}


void CEvohomeWeb::DecodeDHWState(temperatureControlSystem* tcs)
{
	// Hot Water is essentially just another zone
	if ((tcs->status == nullptr)
	    || (!tcs->status->isMember("dhw") || !(*tcs->status)["dhw"].isMember("temperatureStatus")
		|| !(*tcs->status)["dhw"].isMember("stateStatus")))
		return;

	std::string szId, szmode;
	std::string szuntil;
	std::stringstream ssUpdateStat;

	szId = (*tcs->status)["dhw"]["dhwId"].asString();
	ssUpdateStat << (*tcs->status)["dhw"]["temperatureStatus"]["temperature"].asString() << ";";
	ssUpdateStat << (*tcs->status)["dhw"]["stateStatus"]["state"].asString() << ";";
	szmode = (*tcs->status)["dhw"]["stateStatus"]["mode"].asString();
	if (szmode == "FollowSchedule")
		ssUpdateStat << "Auto";
	else
		ssUpdateStat << szmode;
	if (szmode == "TemporaryOverride")
		szuntil = (*tcs->status)["dhw"]["stateStatus"]["until"].asString();

	if (m_updatedev) // create/update and return the first free unit
	{
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"SELECT ID,DeviceID,Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) ORDER BY Unit",
			this->m_HwdID, pTypeEvohomeWater);
		std::string ndevname;
		if (m_showlocation)
			ndevname = m_szlocationName + ": Hot Water";
		else
			ndevname = "Hot Water";

		if (result.empty())
		{
			// create device
			std::string sdevname;
			uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, szId.c_str(), 1, pTypeEvohomeWater, sTypeEvohomeWater, 10, 255, 50, "0.0;Off;Auto", sdevname);
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")", ndevname.c_str(), DevRowIdx);
		}
		else if ((result[0][1] != szId) || (result[0][2] != ndevname))
		{
			// also wipe StrParam1 - we do not want a double action from the old (python) script when changing the setpoint
			m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%q', Name='%q', StrParam1='' WHERE (ID == %" PRIu64 ")", szId.c_str(), ndevname.c_str(), std::stoull(result[0][0]));
		}
	}

	if (m_showschedule && szuntil.empty())
	{
		zone* hz = &tcs->dhw[0];
		std::string szsysmode = (*tcs->status)["systemModeStatus"]["mode"].asString();
		if (szsysmode != "Custom") // can't use schedule to find next switchpoint
		{
			if (!(hz->schedule.isNull()) || get_dhw_schedule(hz->zoneId))
				szuntil = local_to_utc(get_next_switchpoint(hz));
		}
	}
	if (!szuntil.empty())
		ssUpdateStat << ";" << szuntil;

	std::string sdevname;
	uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, szId.c_str(), 1, pTypeEvohomeWater, sTypeEvohomeWater, 10, 255, 50, ssUpdateStat.str().c_str(), sdevname);

	// Notify MQTT and various push mechanisms
	m_mainworker.sOnDeviceReceived(this->m_HwdID, DevRowIdx, "Hot Water", nullptr);
}


/*
 * Code for serial and python scripts appear to assume that zones are always returned in the same order
 * I'm not sure that is really true, so I'll use a map to match the evohome ID and our zone number.
 */
uint8_t CEvohomeWeb::GetUnit_by_ID(unsigned long evoID)
{
	size_t row;
	if (m_zones[0] == 0) // first run - construct
	{
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"SELECT Unit,DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) ORDER BY Unit",
			this->m_HwdID, pTypeEvohomeZone);
		for (row = 1; row <= m_nMaxZones; row++)
			m_zones[row] = 0;
		for (row = 0; row < result.size(); row++)
		{
			int unit = atoi(result[row][0].c_str());
			m_zones[unit] = atol(result[row][1].c_str());
			if (m_zones[unit] == uint64_t(unit) + 92000) // mark manually added, unlinked zone as free
				m_zones[unit] = 0;
		}
		m_zones[0] = 1;
	}
	unsigned char unit = 0;
	for (row = 1; row <= m_nMaxZones; row++)
	{
		unit++;
		if (m_zones[row] == evoID)
			return (uint8_t)(unit);
	}
	if (m_updatedev) // create/update and return the first free unit
	{
		unit = 0;
		for (row = 1; row <= m_nMaxZones; row++)
		{
			unit++;
			if (m_zones[row] == 0)
			{
				std::string sdevname;
				unsigned long nid = 92000 + row;
				char ID[40];
				sprintf(ID, "%lu", nid);
				uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, ID, unit, pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, "0.0;0.0;Auto", sdevname);
				if (DevRowIdx == -1)
					return (uint8_t)-1;
				char devname[8];
				sprintf(devname, "zone %d", (int)row);
				sprintf(ID, "%lu", evoID);
				m_sql.safe_query("UPDATE DeviceStatus SET Name='%q',DeviceID='%q' WHERE (ID == %" PRIu64 ")", devname, ID, DevRowIdx);
				m_zones[row] = evoID;
				return (uint8_t)(unit);
			}
		}
		_log.Log(LOG_ERROR, "(%s) cannot add new zone because you have no free zones left", m_Name.c_str());
	}
	return (uint8_t)-1;
}


/*
 * Helper to convert local time strings to UTC time strings
 */
std::string CEvohomeWeb::local_to_utc(const std::string &local_time)
{
	if (local_time.size() < 19)
		return "";
	if (m_tzoffset == -1)
	{
		// calculate timezone offset once
		time_t now = mytime(nullptr);
		struct tm utime;
		gmtime_r(&now, &utime);
		utime.tm_isdst = -1;
		m_tzoffset = (int)difftime(mktime(&utime), now);
	}
	struct tm ltime;
	ltime.tm_isdst = -1;
	ltime.tm_year = atoi(local_time.substr(0, 4).c_str()) - 1900;
	ltime.tm_mon = atoi(local_time.substr(5, 2).c_str()) - 1;
	ltime.tm_mday = atoi(local_time.substr(8, 2).c_str());
	ltime.tm_hour = atoi(local_time.substr(11, 2).c_str());
	ltime.tm_min = atoi(local_time.substr(14, 2).c_str());
	ltime.tm_sec = atoi(local_time.substr(17, 2).c_str()) + m_tzoffset;
	mktime(&ltime);
	if (m_lastDST == -1)
		m_lastDST = ltime.tm_isdst;
	else if ((m_lastDST != ltime.tm_isdst) && (m_lastDST != -1)) // DST changed - must recalculate timezone offset
	{
		ltime.tm_hour -= (ltime.tm_isdst - m_lastDST);
		m_lastDST = ltime.tm_isdst;
		m_tzoffset = -1;
	}
	char until[100];
	sprintf(until, "%04d-%02d-%02dT%02d:%02d:%02dZ", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	return std::string(until);
}


/*
 * Evohome client API starts here
 */

 /*
  * Copyright (c) 2016 Gordon Bos <gordon@bosvangennip.nl> All rights reserved.
  *
  * Json client for UK/EMEA Evohome API
  *
  *
  * Source code subject to GNU GENERAL PUBLIC LICENSE version 3
  */


#define EVOHOME_HOST "https://tccna.honeywell.com"


/************************************************************************
 *									*
 *	Evohome authentication						*
 *									*
 ************************************************************************/

bool CEvohomeWeb::login(const std::string &user, const std::string &password)
{
	_log.Debug(DEBUG_HARDWARE, "(%s) logon to v2 API", m_Name.c_str());

	std::vector<std::string> LoginHeaders;
	LoginHeaders.push_back("Authorization: Basic YjAxM2FhMjYtOTcyNC00ZGJkLTg4OTctMDQ4YjlhYWRhMjQ5OnRlc3Q=");
	LoginHeaders.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	LoginHeaders.push_back("charsets: utf-8");

	std::string sz_postdata = "installationInfo-Type=application%2Fx-www-form-urlencoded;charset%3Dutf-8";
	sz_postdata.append("&Host=rs.alarmnet.com%2F");
	sz_postdata.append("&Cache-Control=no-store%20no-cache");
	sz_postdata.append("&Pragma=no-cache");
	sz_postdata.append("&grant_type=password");
	sz_postdata.append("&scope=EMEA-V1-Basic%20EMEA-V1-Anonymous%20EMEA-V1-Get-Current-User-Account");
	sz_postdata.append("&Username=");
	sz_postdata.append(CURLEncode::URLEncode(user));
	sz_postdata.append("&Password=");
	sz_postdata.append(CURLEncode::URLEncode(password));
	sz_postdata.append("&Connection=Keep-Alive");

	std::string sz_response = send_receive_data(EVOHOME_HOST"/Auth/OAuth/Token", sz_postdata, LoginHeaders);

	if (sz_response.empty())
	{
		_log.Log(LOG_ERROR, "(%s) login attempt did not return any response", m_Name.c_str());
		return false;
	}

	if (sz_response[0] == '[') // response is an unnamed array - likely an error message
	{
		sz_response[0] = ' ';
		size_t len = sz_response.size();
		len--;
		sz_response[len] = ' ';
	}

	Json::Value j_login;
	if (!ParseJSon(sz_response, j_login))
	{
		_log.Log(LOG_ERROR, "(%s) failed parsing response from login to portal", m_Name.c_str());
		return false;
	}

	if (!j_login.isMember("access_token"))
	{
		std::string szError;
		if (j_login.isMember("error"))
			szError = j_login["error"].asString();
		if (j_login.isMember("message"))
			szError = j_login["message"].asString();
		if (!szError.empty())
		{
			_log.Log(LOG_ERROR, "(%s) login failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) login returned unknown data", m_Name.c_str());
		return false;
	}

	m_v2refresh_token = j_login["refresh_token"].asString();
	int v2token_expiration_time = atoi(j_login["expires_in"].asString().c_str());
	m_sessiontimer = mytime(nullptr) + v2token_expiration_time; // Honeywell will invalidate our session ID after an hour

	std::stringstream atoken;
	atoken << "Authorization: bearer " << j_login["access_token"].asString();

	m_SessionHeaders.clear();
	m_SessionHeaders.push_back(atoken.str());
	m_SessionHeaders.push_back("applicationId: b013aa26-9724-4dbd-8897-048b9aada249");
	m_SessionHeaders.push_back("accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	m_SessionHeaders.push_back("content-type: application/json");
	m_SessionHeaders.push_back("charsets: utf-8");

	return user_account();
}


/* 
 * Renew the Authorization token
 */
bool CEvohomeWeb::renew_login()
{
	if (m_v2refresh_token.empty())
		return false;

	_log.Debug(DEBUG_HARDWARE, "(%s) refresh v2 session token", m_Name.c_str());

	std::vector<std::string> LoginHeaders;
	LoginHeaders.push_back("Authorization: Basic YjAxM2FhMjYtOTcyNC00ZGJkLTg4OTctMDQ4YjlhYWRhMjQ5OnRlc3Q=");
	LoginHeaders.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	LoginHeaders.push_back("charsets: utf-8");

	std::string sz_postdata = "installationInfo-Type=application%2Fx-www-form-urlencoded;charset%3Dutf-8";
	sz_postdata.append("&Host=rs.alarmnet.com%2F");
	sz_postdata.append("&Cache-Control=no-store%20no-cache");
	sz_postdata.append("&Pragma=no-cache");
	sz_postdata.append("&grant_type=refresh_token");
	sz_postdata.append("&scope=EMEA-V1-Basic%20EMEA-V1-Anonymous");
	sz_postdata.append("&refresh_token=");
	sz_postdata.append(m_v2refresh_token);
	sz_postdata.append("&Connection=Keep-Alive");

	std::string sz_response = send_receive_data(EVOHOME_HOST"/Auth/OAuth/Token", sz_postdata, LoginHeaders);

	if (sz_response[0] == '[') // response is an unnamed array - likely an error message
	{
		sz_response[0] = ' ';
		size_t len = sz_response.size();
		len--;
		sz_response[len] = ' ';
	}

	Json::Value j_login;
	if (!ParseJSon(sz_response, j_login))
	{
		_log.Log(LOG_ERROR, "(%s) failed parsing response from renewing login", m_Name.c_str());
		return false;
	}

	if (!j_login.isMember("access_token"))
	{
		std::string szError;
		if (j_login.isMember("error"))
			szError = j_login["error"].asString();
		if (j_login.isMember("message"))
			szError = j_login["message"].asString();
		if (!szError.empty())
		{
			if (szError == "invalid_grant") // refresh token is no longer valid
				m_v2refresh_token = "";

			_log.Log(LOG_ERROR, "(%s) renewing login failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) renewing login returned unknown data", m_Name.c_str());
		return false;
	}

	m_v2refresh_token = j_login["refresh_token"].asString();
	int v2token_expiration_time = atoi(j_login["expires_in"].asString().c_str());
	m_sessiontimer = mytime(nullptr) + v2token_expiration_time; // Honeywell will invalidate our session ID after an hour

	std::stringstream atoken;
	atoken << "Authorization: bearer " << j_login["access_token"].asString();

	m_SessionHeaders.clear();
	m_SessionHeaders.push_back(atoken.str());
	m_SessionHeaders.push_back("applicationId: b013aa26-9724-4dbd-8897-048b9aada249");
	m_SessionHeaders.push_back("accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	m_SessionHeaders.push_back("content-type: application/json");
	m_SessionHeaders.push_back("charsets: utf-8");

	return true;
}


/*
 * Retrieve evohome user info
 */
bool CEvohomeWeb::user_account()
{
	std::string sz_response = send_receive_data(EVOHOME_HOST"/WebAPI/emea/api/v1/userAccount", m_SessionHeaders);

	if (sz_response[0] == '[') // response is an unnamed array - likely an error message
	{
		sz_response[0] = ' ';
		size_t len = sz_response.size();
		len--;
		sz_response[len] = ' ';
	}

	Json::Value j_account;
	if (!ParseJSon(sz_response, j_account))
	{
		_log.Log(LOG_ERROR, "(%s) failed parsing response from retrieve user account info", m_Name.c_str());
		return false;
	}

	std::string szError;
	if (j_account.isMember("error"))
		szError = j_account["error"].asString();
	if (j_account.isMember("message"))
		szError = j_account["message"].asString();
	if (!szError.empty())
	{
		_log.Log(LOG_ERROR, "(%s) retrieve user account info failed with message: %s", m_Name.c_str(), szError.c_str());
		return false;
	}

	if (!j_account.isMember("userId"))
	{
		_log.Log(LOG_ERROR, "(%s) retrieve user account info did not return an ID", m_Name.c_str());
		return false;
	}

	m_evouid = j_account["userId"].asString();
	return true;
}


/************************************************************************
 *									*
 *	Evohome heating installations retrieval				*
 *									*
 ************************************************************************/

void CEvohomeWeb::get_dhw(int location, int gateway, int temperatureControlSystem)
{
	std::vector<zone>().swap(m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw);

	if (!has_dhw(&m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem]))
		return;

	Json::Value *j_tcs = m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].installationInfo;

	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw.resize(1);
	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw[0].installationInfo = &(*j_tcs)["dhw"];
	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw[0].zoneId = (*j_tcs)["dhw"]["dhwId"].asString();;
	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw[0].systemId = m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].systemId;
	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw[0].gatewayId = m_locations[location].gateways[gateway].gatewayId;
	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].dhw[0].locationId = m_locations[location].locationId;
}


void CEvohomeWeb::get_zones(int location, int gateway, int temperatureControlSystem)
{
	std::vector<zone>().swap(m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones);
	Json::Value *j_tcs = m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].installationInfo;

	if (!(*j_tcs)["zones"].isArray())
		return;

	size_t l = (*j_tcs)["zones"].size();
	m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones.resize(l);
	for (size_t i = 0; i < l; ++i)
	{
		m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones[i].installationInfo = &(*j_tcs)["zones"][(int)(i)];
		m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones[i].zoneId = (*j_tcs)["zones"][(int)(i)]["zoneId"].asString();
		m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones[i].systemId = m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].systemId;
		m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones[i].gatewayId = m_locations[location].gateways[gateway].gatewayId;
		m_locations[location].gateways[gateway].temperatureControlSystems[temperatureControlSystem].zones[i].locationId = m_locations[location].locationId;
	}
}


void CEvohomeWeb::get_temperatureControlSystems(int location, int gateway)
{
	std::vector<temperatureControlSystem>().swap(m_locations[location].gateways[gateway].temperatureControlSystems);
	Json::Value *j_gw = m_locations[location].gateways[gateway].installationInfo;

	if (!(*j_gw)["temperatureControlSystems"].isArray())
		return;

	size_t l = (*j_gw)["temperatureControlSystems"].size();
	m_locations[location].gateways[gateway].temperatureControlSystems.resize(l);
	for (size_t i = 0; i < l; ++i)
	{
		m_locations[location].gateways[gateway].temperatureControlSystems[i].installationInfo = &(*j_gw)["temperatureControlSystems"][(int)(i)];
		m_locations[location].gateways[gateway].temperatureControlSystems[i].systemId = (*j_gw)["temperatureControlSystems"][(int)(i)]["systemId"].asString();
		m_locations[location].gateways[gateway].temperatureControlSystems[i].gatewayId = m_locations[location].gateways[gateway].gatewayId;
		m_locations[location].gateways[gateway].temperatureControlSystems[i].locationId = m_locations[location].locationId;

		get_zones(location, gateway, (int)(i));
		get_dhw(location, gateway, (int)(i));
	}
}


void CEvohomeWeb::get_gateways(int location)
{
	std::vector<gateway>().swap(m_locations[location].gateways);
	Json::Value *j_loc = m_locations[location].installationInfo;

	if (!(*j_loc)["gateways"].isArray())
		return;

	size_t l = (*j_loc)["gateways"].size();
	m_locations[location].gateways.resize(l);
	for (size_t i = 0; i < l; ++i)
	{
		m_locations[location].gateways[i].installationInfo = &(*j_loc)["gateways"][(int)(i)];
		m_locations[location].gateways[i].gatewayId = (*j_loc)["gateways"][(int)(i)]["gatewayInfo"]["gatewayId"].asString();
		m_locations[location].gateways[i].locationId = m_locations[location].locationId;

		get_temperatureControlSystems(location, (int)(i));
	}
}


bool CEvohomeWeb::full_installation()
{
	std::vector<location>().swap(m_locations);

	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/location/installationInfo?userId=";
	sz_url.append(m_evouid);
	sz_url.append("&includeTemperatureControlSystems=True");

	std::string sz_response = send_receive_data(sz_url, m_SessionHeaders);
	m_j_fi.clear();

	bool parseOK;
	if (sz_response[0] == '[')
	{
		// evohome v1 API returns an unnamed json array which is not accepted by our parser
		std::string sz_jdata = "{\"locations\": ";
		sz_jdata.append(sz_response);
		sz_jdata.append("}");
		parseOK = ParseJSon(sz_jdata, m_j_fi);
	}
	else
		parseOK = ParseJSon(sz_response, m_j_fi);

	if (!parseOK)
	{
		_log.Log(LOG_ERROR, "(%s) cannot parse return data from retrieve installation info", m_Name.c_str());
		return false;
	}

	if (!m_j_fi.isMember("locations"))
	{
		std::string szError;
		if (m_j_fi.isMember("error"))
			szError = m_j_fi["error"].asString();
		if (m_j_fi.isMember("message"))
			szError = m_j_fi["message"].asString();
		if (!szError.empty())
		{
			_log.Log(LOG_ERROR, "(%s) retrieve installation info failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) retrieve installation info returned an unhandled response", m_Name.c_str());
		return false;
	}

	size_t l = m_j_fi["locations"].size();
	if (l == 0)
	{
		_log.Log(LOG_ERROR, "(%s) retrieve installation info returned an empty set", m_Name.c_str());
		return false;
	}
	if (m_j_fi["locations"][0].isMember("message"))
	{
		std::string szError = m_j_fi["locations"][0]["message"].asString();
		_log.Log(LOG_ERROR, "(%s) retrieve installation info returned message: %s", m_Name.c_str(), szError.c_str());
		return false;
	}

	for (size_t i = 0; i < l; ++i)
	{
		location newloc = location();
		m_locations.push_back(newloc);
		m_locations[i].installationInfo = &m_j_fi["locations"][(int)(i)];
		m_locations[i].locationId = m_j_fi["locations"][(int)(i)]["locationInfo"]["locationId"].asString();

		get_gateways((int)(i));
	}
	return true;
}


/************************************************************************
 *									*
 *	Evohome system status retrieval					*
 *									*
 ************************************************************************/

bool CEvohomeWeb::get_status(const std::string &locationId)
{
	if ((m_locations.empty()) && !full_installation())
		return false;

	for (size_t i = 0; i < m_locations.size(); i++)
	{
		if (m_locations[i].locationId == locationId)
			return get_status(i);
	}
	return false;
}
bool CEvohomeWeb::get_status(int location)
{
	if ((m_locations.empty()) && !full_installation())
		return false;
	if (m_locations[location].locationId.empty())
		return false;

	Json::Value *j_loc, *j_gw, *j_tcs;
	bool valid_json = true;

	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/location/";
	sz_url.append(m_locations[location].locationId);
	sz_url.append("/status?includeTemperatureControlSystems=True");

	std::string sz_response = send_receive_data(sz_url, m_SessionHeaders);

	if (sz_response[0] == '[') // response is an unnamed array - likely an error message
	{
		sz_response[0] = ' ';
		size_t len = sz_response.size();
		len--;
		sz_response[len] = ' ';
	}
	m_j_stat.clear();
	if (!ParseJSon(sz_response, m_j_stat))
	{
		_log.Log(LOG_ERROR, "(%s) cannot parse return data from status request", m_Name.c_str());
		return false;
	}

	if (!m_j_stat.isMember("gateways"))
	{
		std::string szError;
		if (m_j_stat.isMember("error"))
			szError = m_j_stat["error"].asString();
		if (m_j_stat.isMember("message"))
			szError = m_j_stat["message"].asString();
		if (!szError.empty())
		{
			_log.Log(LOG_ERROR, "(%s) status request failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) status request returned an unhandled response", m_Name.c_str());
		return false;
	}

	m_locations[location].status = &m_j_stat;
	j_loc = m_locations[location].status;

	// get gateway status
	if ((*j_loc)["gateways"].isArray())
	{
		size_t lgw = (*j_loc)["gateways"].size();
		for (size_t igw = 0; igw < lgw; ++igw)
		{
			m_locations[location].gateways[igw].status = &(*j_loc)["gateways"][(int)(igw)];
			j_gw = m_locations[location].gateways[igw].status;

			// get temperatureControlSystem status
			if ((*j_gw)["temperatureControlSystems"].isArray())
			{
				size_t ltcs = (*j_gw)["temperatureControlSystems"].size();
				for (size_t itcs = 0; itcs < ltcs; itcs++)
				{
					m_locations[location].gateways[igw].temperatureControlSystems[itcs].status = &(*j_gw)["temperatureControlSystems"][(int)(itcs)];
					j_tcs = m_locations[location].gateways[igw].temperatureControlSystems[itcs].status;

					// get zone status
					if ((*j_tcs)["zones"].isArray())
					{
						size_t lz = (*j_tcs)["zones"].size();
						for (size_t iz = 0; iz < lz; iz++)
						{
							m_locations[location].gateways[igw].temperatureControlSystems[itcs].zones[iz].status = &(*j_tcs)["zones"][(int)(iz)];
							m_locations[location].gateways[igw].temperatureControlSystems[itcs].zones[iz].hdtemp="";
						}
					}
					else
						valid_json = false;

					if (has_dhw(&m_locations[location].gateways[igw].temperatureControlSystems[itcs]))
					{
						m_locations[location].gateways[igw].temperatureControlSystems[itcs].dhw[0].status = &(*j_tcs)["dhw"];
					}
				}
			}
			else
				valid_json = false;
		}
	}
	else
		valid_json = false;

	if (!valid_json)
		_log.Log(LOG_ERROR, "(%s) status request did not return a valid response", m_Name.c_str());
	return valid_json;
}


/************************************************************************
 *									*
 *	Locate Evohome objects by ID					*
 *									*
 ************************************************************************/

CEvohomeWeb::zone* CEvohomeWeb::get_zone_by_ID(const std::string &zoneId)
{
	if ((m_locations.empty()) && !full_installation())
		return nullptr;

	for (auto &m_location : m_locations)
	{
		for (auto &gateway : m_location.gateways)
		{
			for (auto &temperatureControlSystem : gateway.temperatureControlSystems)
			{
				for (auto &zone : temperatureControlSystem.zones)
				{
					if (zone.zoneId == zoneId)
						return &zone;
				}
				if (!temperatureControlSystem.dhw.empty())
				{
					if (temperatureControlSystem.dhw[0].zoneId == zoneId)
						return &temperatureControlSystem.dhw[0];
				}
			}
		}
	}
	return nullptr;
}


/************************************************************************
 *									*
 *	Schedule handlers						*
 *									*
 ************************************************************************/

bool CEvohomeWeb::get_zone_schedule(const std::string &zoneId)
{
	return get_zone_schedule(zoneId, "temperatureZone");
}
bool CEvohomeWeb::get_dhw_schedule(const std::string &dhwId)
{
	return get_zone_schedule(dhwId, "domesticHotWater");
}
bool CEvohomeWeb::get_zone_schedule(const std::string &zoneId, const std::string &zoneType)
{
	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/";
	sz_url.append(zoneType);
	sz_url.append("/");
	sz_url.append(zoneId);
	sz_url.append("/schedule");

	std::string sz_response = send_receive_data(sz_url, m_SessionHeaders);

	if (!sz_response.find("\"id\"")) // ignore errors - a schedule may simply not exist
		return false;

	zone* hz = get_zone_by_ID(zoneId);
	if (hz == nullptr)
		return false;
	bool ret = ParseJSon(sz_response, hz->schedule);
	if (ret)
	{
		(hz->schedule)["zoneId"] = zoneId;
		(hz->schedule)["name"] = (*hz->installationInfo)["name"].asString();
	}
	return ret;
}


std::string CEvohomeWeb::get_next_switchpoint(CEvohomeWeb::temperatureControlSystem* tcs, int zone)
{
	if ((tcs->zones[zone].schedule.isNull()) && !get_zone_schedule(tcs->zones[zone].zoneId))
		return "";
	return get_next_switchpoint(tcs->zones[zone].schedule);
}
std::string CEvohomeWeb::get_next_switchpoint(zone* hz)
{
	std::string zoneType = ((*hz->installationInfo).isMember("dhwId")) ? "domesticHotWater" : "temperatureZone";
	if ((hz->schedule.isNull()) && !get_zone_schedule(hz->zoneId, zoneType))
		return "";
	return get_next_switchpoint(hz->schedule);
}
std::string CEvohomeWeb::get_next_switchpoint(Json::Value &schedule)
{
	std::string current_setpoint;
	return get_next_switchpoint_ex(schedule, current_setpoint);
}
std::string CEvohomeWeb::get_next_switchpoint_ex(Json::Value &schedule, std::string &current_setpoint)
{
	if (schedule.isNull())
		return "";

	struct tm ltime;
	time_t now = mytime(nullptr);
	localtime_r(&now, &ltime);
	int year = ltime.tm_year;
	int month = ltime.tm_mon;
	int day = ltime.tm_mday;
	int wday = ltime.tm_wday;
	char rdata[100];
	sprintf(rdata, "%04d-%02d-%02dT%02d:%02d:%02dZ", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	std::string szdatetime = std::string(rdata);
	if (szdatetime <= schedule["nextSwitchpoint"].asString()) // our current cached values are still valid
	{
		current_setpoint = schedule["currentSetpoint"].asString();
		return schedule["nextSwitchpoint"].asString();
	}

	// Hack: DayOff needs to reference a specific weekday rather than current
	if (m_tcs->status != nullptr)
	{
		std::string szsystemMode = (*m_tcs->status)["systemModeStatus"]["mode"].asString();
		if (szsystemMode == "DayOff")
			wday = m_wdayoff;
	}

	std::string sztime;
	bool found = false;
	current_setpoint = "";
	for (uint8_t d = 0; ((d < 7) && !found); d++)
	{
		int tryday = (wday + d) % 7;
		std::string s_tryday = (std::string)weekdays[tryday];
		Json::Value* j_day;
		// find day
		for (size_t i = 0; ((i < schedule["dailySchedules"].size()) && !found); i++)
		{
			j_day = &schedule["dailySchedules"][(int)(i)];
			if (((*j_day).isMember("dayOfWeek")) && ((*j_day)["dayOfWeek"] == s_tryday))
				found = true;
		}
		if ((!found) || (j_day == nullptr))
			continue;

		found = false;
		for (size_t i = 0; ((i < (*j_day)["switchpoints"].size()) && !found); ++i)
		{
			sztime = (*j_day)["switchpoints"][(int)(i)]["timeOfDay"].asString();
			ltime.tm_isdst = -1;
			ltime.tm_year = year;
			ltime.tm_mon = month;
			ltime.tm_mday = day + d;
			ltime.tm_hour = atoi(sztime.substr(0, 2).c_str());
			ltime.tm_min = atoi(sztime.substr(3, 2).c_str());
			ltime.tm_sec = atoi(sztime.substr(6, 2).c_str());
			time_t ntime = mktime(&ltime);
			if (ntime > now)
				found = true;
			else if ((*j_day)["switchpoints"][(int)(i)].isMember("temperature"))
				current_setpoint = (*j_day)["switchpoints"][(int)(i)]["temperature"].asString();
			else
				current_setpoint = (*j_day)["switchpoints"][(int)(i)]["dhwState"].asString();
		}
	}

	if (current_setpoint.empty()) // got a direct match for the next switchpoint, need to go back in time to find the current setpoint
	{
		found = false;
		for (uint8_t d = 1; ((d < 7) && !found); d++)
		{
			int tryday = (wday - d + 7) % 7;
			std::string s_tryday = (std::string)weekdays[tryday];
			Json::Value* j_day;
			// find day
			for (size_t i = 0; ((i < schedule["dailySchedules"].size()) && !found); i++)
			{
				j_day = &schedule["dailySchedules"][(int)(i)];
				if (((*j_day).isMember("dayOfWeek")) && ((*j_day)["dayOfWeek"] == s_tryday))
					found = true;
			}
			if ((!found) || (j_day == nullptr))
				continue;

			found = false;
			size_t l = (*j_day)["switchpoints"].size();
			if (l > 0)
			{
				l--;
				if ((*j_day)["switchpoints"][(int)(l)].isMember("temperature"))
					current_setpoint = (*j_day)["switchpoints"][(int)(l)]["temperature"].asString();
				else
					current_setpoint = (*j_day)["switchpoints"][(int)(l)]["dhwState"].asString();
				found = true;
			}
		}
	}

	if (!found)
		return "";

	sprintf(rdata, "%04d-%02d-%02dT%sA", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, sztime.c_str()); // localtime => use CET to indicate that it is not UTC
	szdatetime = std::string(rdata);
	schedule["currentSetpoint"] = current_setpoint;
	schedule["nextSwitchpoint"] = szdatetime;
	return szdatetime;
}


/************************************************************************
 *									*
 *	Evohome overrides						*
 *									*
 ************************************************************************/

bool CEvohomeWeb::verify_datetime(const std::string &datetime)
{
	if (datetime.length() < 19)
		return false;
	std::string s_date = datetime.substr(0, 10);
	std::string s_time = datetime.substr(11, 8);
	struct tm mtime;
	mtime.tm_isdst = -1;
	mtime.tm_year = atoi(datetime.substr(0, 4).c_str()) - 1900;
	mtime.tm_mon = atoi(datetime.substr(5, 2).c_str()) - 1;
	mtime.tm_mday = atoi(datetime.substr(8, 2).c_str());
	mtime.tm_hour = atoi(datetime.substr(11, 2).c_str());
	mtime.tm_min = atoi(datetime.substr(14, 2).c_str());
	mtime.tm_sec = atoi(datetime.substr(17, 2).c_str());
	time_t ntime = mktime(&mtime);
	if (ntime == -1)
		return false;
	char c_date[50];
	sprintf(c_date, "%04d-%02d-%02d", mtime.tm_year + 1900, mtime.tm_mon + 1, mtime.tm_mday);
	char c_time[50];
	sprintf(c_time, "%02d:%02d:%02d", mtime.tm_hour, mtime.tm_min, mtime.tm_sec);
	return ((s_date == std::string(c_date)) && (s_time == std::string(c_time)));
}


bool CEvohomeWeb::set_system_mode(const std::string &systemId, int mode)
{
	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/temperatureControlSystem/";
	sz_url.append(systemId);
	sz_url.append("/mode");

	std::string sz_putdata = "{\"SystemMode\":";
	sz_putdata.append(std::to_string(mode));
	sz_putdata.append(R"(,"TimeUntil":null,"Permanent":true})");

	std::string sz_response = put_receive_data(sz_url, sz_putdata, m_SessionHeaders);
	if (!sz_response.find("\"id\""))
	{
		if (sz_response[0] == '[') // response is an unnamed array - likely an error message
		{
			sz_response[0] = ' ';
			size_t len = sz_response.size();
			len--;
			sz_response[len] = ' ';
		}

		Json::Value j_response;
		if (ParseJSon(sz_response, j_response) && (j_response.isMember("message")))
		{
			std::string szError = j_response["message"].asString();
			_log.Log(LOG_ERROR, "(%s) set system mode failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) set system mode returned an unhandled response", m_Name.c_str());
		return false;
	}
	return true;
}


bool CEvohomeWeb::set_temperature(const std::string &zoneId, const std::string &temperature, const std::string &time_until)
{
	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/temperatureZone/";
	sz_url.append(zoneId);
	sz_url.append("/heatSetpoint");

	std::string sz_putdata = "{\"HeatSetpointValue\":";
	sz_putdata.append(temperature);
	if (time_until.empty())
		sz_putdata.append(R"(,"SetpointMode":1,"TimeUntil":null})");
	else
	{
		if (!verify_datetime(time_until))
		{
			_log.Log(LOG_ERROR, "(%s) ignored zone temperature override because of invalid 'time_until' value", m_Name.c_str());
			return false;
		}
		sz_putdata.append(R"(,"SetpointMode":2,"TimeUntil":")");
		sz_putdata.append(time_until.substr(0, 10));
		sz_putdata.append("T");
		sz_putdata.append(time_until.substr(11, 8));
		sz_putdata.append("Z\"}");
	}

	std::string sz_response = put_receive_data(sz_url, sz_putdata, m_SessionHeaders);

	if (!sz_response.find("\"id\""))
	{
		if (sz_response[0] == '[') // response is an unnamed array - likely an error message
		{
			sz_response[0] = ' ';
			size_t len = sz_response.size();
			len--;
			sz_response[len] = ' ';
		}

		Json::Value j_response;
		if (ParseJSon(sz_response, j_response) && (j_response.isMember("message")))
		{
			std::string szError = j_response["message"].asString();
			_log.Log(LOG_ERROR, "(%s) set zone temperature override failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) set zone temperature override returned an unhandled response", m_Name.c_str());
		return false;
	}
	return true;
}


bool CEvohomeWeb::cancel_temperature_override(const std::string &zoneId)
{
	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/temperatureZone/";
	sz_url.append(zoneId);
	sz_url.append("/heatSetpoint");

	std::string sz_putdata = R"({"HeatSetpointValue":0.0,"SetpointMode":0,"TimeUntil":null})";

	std::string sz_response = put_receive_data(sz_url, sz_putdata, m_SessionHeaders);

	if (!sz_response.find("\"id\""))
	{
		if (sz_response[0] == '[') // response is an unnamed array - likely an error message
		{
			sz_response[0] = ' ';
			size_t len = sz_response.size();
			len--;
			sz_response[len] = ' ';
		}

		Json::Value j_response;
		if (ParseJSon(sz_response, j_response) && (j_response.isMember("message")))
		{
			std::string szError = j_response["message"].asString();
			_log.Log(LOG_ERROR, "(%s) cancel zone temperature override failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) cancel zone temperature override returned an unhandled response", m_Name.c_str());
		return false;
	}
	return true;
}


bool CEvohomeWeb::has_dhw(CEvohomeWeb::temperatureControlSystem *tcs)
{
	return (*tcs->installationInfo).isMember("dhw");
}


bool CEvohomeWeb::set_dhw_mode(const std::string &dhwId, const std::string &mode, const std::string &time_until)
{
	std::string sz_url = EVOHOME_HOST"/WebAPI/emea/api/v1/domesticHotWater/";
	sz_url.append(dhwId);
	sz_url.append("/state");

	std::string sz_putdata = "{\"State\":";
	if (mode == "auto")
		sz_putdata.append(R"(0,"Mode":0,"UntilTime":null})");
	else
	{
		if (mode == "on")
			sz_putdata.append("1");
		else
			sz_putdata.append("0");
		if (time_until.empty())
			sz_putdata.append(R"(,"Mode":1,"UntilTime":null})");
		else
		{
			if (!verify_datetime(time_until))
			{
				_log.Log(LOG_ERROR, "(%s) ignored hot water override because of invalid 'time_until' value", m_Name.c_str());
				return false;
			}
			sz_putdata.append(R"(,"Mode":2,"UntilTime":")");
			sz_putdata.append(time_until.substr(0, 10));
			sz_putdata.append("T");
			sz_putdata.append(time_until.substr(11, 8));
			sz_putdata.append("Z\"}");
		}
	}

	std::string sz_response = put_receive_data(sz_url, sz_putdata, m_SessionHeaders);

	if (!sz_response.find("\"id\""))
	{
		if (sz_response[0] == '[') // response is an unnamed array - likely an error message
		{
			sz_response[0] = ' ';
			size_t len = sz_response.size();
			len--;
			sz_response[len] = ' ';
		}

		Json::Value j_response;
		if (ParseJSon(sz_response, j_response) && (j_response.isMember("message")))
		{
			std::string szError = j_response["message"].asString();
			_log.Log(LOG_ERROR, "(%s) set hot water override failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) set hot water override returned an unhandled response", m_Name.c_str());
		return false;
	}
	return true;
}


/************************************************************************
 *									*
 *	v1 API								*
 *									*
 ************************************************************************/

bool CEvohomeWeb::v1_login(const std::string &user, const std::string &password)
{
	_log.Debug(DEBUG_HARDWARE, "(%s) logon to v1 API", m_Name.c_str());

	std::vector<std::string> LoginHeaders;
	LoginHeaders.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	LoginHeaders.push_back("content-type: application/json");

	std::string sz_postdata = "{'Username': '";
	sz_postdata.append(user);
	sz_postdata.append("', 'Password': '");
	sz_postdata.append(password);
	sz_postdata.append("', 'ApplicationId': '91db1612-73fd-4500-91b2-e63b069b185c'}");

	std::string sz_response = send_receive_data(EVOHOME_HOST"/WebAPI/api/Session", sz_postdata, LoginHeaders);

	if (sz_response[0] == '[') // response is an unnamed array - likely an error message
	{
		sz_response[0] = ' ';
		size_t len = sz_response.size();
		len--;
		sz_response[len] = ' ';
	}

	Json::Value j_login;
	if (!ParseJSon(sz_response, j_login))
	{
		_log.Log(LOG_ERROR, "(%s) cannot parse return data from v1 login", m_Name.c_str());
		return false;
	}

	if (!j_login.isMember("sessionId") || !j_login.isMember("userInfo") || !j_login["userInfo"].isObject() || !j_login["userInfo"].isMember("userID"))
	{
		std::string szError;
		if (j_login.isMember("error"))
			szError = j_login["error"].asString();
		if (j_login.isMember("message"))
			szError = j_login["message"].asString();
		if (!szError.empty())
		{
			_log.Log(LOG_ERROR, "(%s) v1 login failed with message: %s", m_Name.c_str(), szError.c_str());
			return false;
		}

		_log.Log(LOG_ERROR, "(%s) v1 login returned an unhandled response", m_Name.c_str());
		return false;
	}

	std::string sessionId = j_login["sessionId"].asString();
	std::stringstream atoken;
	atoken << "sessionId: " << sessionId;
	m_v1uid = j_login["userInfo"]["userID"].asString();

	m_v1SessionHeaders.clear();
	m_v1SessionHeaders.push_back(atoken.str());
	m_v1SessionHeaders.push_back("applicationId: 91db1612-73fd-4500-91b2-e63b069b185c");
	m_v1SessionHeaders.push_back("accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");

	return true;
}


void CEvohomeWeb::v1_renew_session()
{
	if (m_v1uid.empty()) {
		v1_login(m_username, m_password);
		return;
	}

	std::string sz_url = EVOHOME_HOST"/WebAPI/api/Session";
	std::string sz_response = put_receive_data(sz_url, "", m_v1SessionHeaders);

	if (sz_response.empty())
		return;

	// no need to parse the whole json string - we only need the "code"
	size_t pos = sz_response.find("\"code\"");
	if (pos == std::string::npos)
		return;

	std::string sz_code = sz_response.substr(pos+8, 2);
	if (sz_code != "-1")
	{
		// session is no longer valid
		_log.Debug(DEBUG_HARDWARE, "(%s) discard expired v1 API session ID", m_Name.c_str());
		m_v1uid = "";
	}
}


void CEvohomeWeb::get_v1_temps()
{
	v1_renew_session();

	if (m_v1uid.empty() && !v1_login(m_username, m_password))
		return;

	std::string sz_url = EVOHOME_HOST"/WebAPI/api/locations/?userId=";
	sz_url.append(m_v1uid);
	sz_url.append("&allData=True");

	std::string sz_response = send_receive_data(sz_url, m_v1SessionHeaders);
	Json::Value j_fi;
	bool parseOK;
	if (sz_response[0] == '[')
	{
		// evohome old API returns an unnamed json array which is not accepted by our parser
		std::string sz_jdata = "{\"locations\": ";
		sz_jdata.append(sz_response);
		sz_jdata.append("}");
		parseOK = ParseJSon(sz_jdata, j_fi);
	}
	else
		parseOK = ParseJSon(sz_response, j_fi);

	if (!parseOK)
	{
		_log.Log(LOG_ERROR, "(%s) cannot parse return data from v1 get temps", m_Name.c_str());
		return;
	}

	Json::Value *j_error;
	if (j_fi.isMember("locations") && (!j_fi["locations"].empty()))
		j_error = &j_fi["locations"][0];
	else
		j_error = &j_fi;

	std::string szError;
	if ((*j_error).isMember("error"))
		szError = (*j_error)["error"].asString();
	if ((*j_error).isMember("message"))
		szError = (*j_error)["message"].asString();
	if (!szError.empty())
	{
		if ((*j_error).isMember("code") && ((*j_error)["code"].asString() == "401"))
		{
			// authorization error: session is no longer valid
			m_v1uid = "";
		}

		_log.Log(LOG_ERROR, "(%s) v1 get temps failed with message: %s", m_Name.c_str(), szError.c_str());
		return;
	}

	if (!j_fi.isMember("locations"))
	{
		_log.Log(LOG_ERROR, "(%s) v1 get temps returned an unhandled response", m_Name.c_str());
		return;
	}

	size_t l = j_fi["locations"].size();
	for (size_t i = 0; i < l; ++i)
	{
		Json::Value *j_loc = &j_fi["locations"][(int)(i)];
		if (!(*j_loc).isMember("devices") || !(*j_loc)["devices"].isArray())
			continue;
		for (size_t j = 0; j < (*j_loc)["devices"].size(); j++)
		{
			Json::Value *j_dev = &(*j_loc)["devices"][(int)(j)];
			if (!(*j_dev).isMember("deviceID") || !(*j_dev).isMember("thermostat") || !(*j_dev)["thermostat"].isMember("indoorTemperature"))
				continue;
			std::string zoneId = (*j_dev)["deviceID"].asString();

			zone *v2zone = get_zone_by_ID(zoneId);
			if (v2zone != nullptr)
			{
				double v1temp = (*j_dev)["thermostat"]["indoorTemperature"].asDouble();
				if (v1temp > 127) // allow rounding error
				{
					_log.Log(LOG_ERROR, "(%s) zone %s does not report a temperature", m_Name.c_str(), zoneId.c_str());
					v2zone->hdtemp = "";
					continue;
				}
				std::stringstream sstemp;
				sstemp << ((floor((v1temp * m_hdprecision) + 0.5) / m_hdprecision) + 0.0001);
				v2zone->hdtemp = sstemp.str().substr(0,5);
			}
		}
	}
}


/************************************************************************
 *									*
 *	HTTP helpers							*
 *									*
 ************************************************************************/

std::string CEvohomeWeb::send_receive_data(const std::string &url, std::vector<std::string> &headers)
{
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> vHeaderData;

	bool httpOK = HTTPClient::GETBinary(url, headers, vHTTPResponse, vHeaderData, HTTPTIMEOUT);

	return process_response(vHTTPResponse, vHeaderData, httpOK);
}

std::string CEvohomeWeb::send_receive_data(const std::string &url, const std::string &postdata, std::vector<std::string> &headers)
{
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> vHeaderData;

	bool httpOK = HTTPClient::POSTBinary(url, postdata, headers, vHTTPResponse, vHeaderData, true, HTTPTIMEOUT);

	return process_response(vHTTPResponse, vHeaderData, httpOK);
}

std::string CEvohomeWeb::put_receive_data(const std::string &url, const std::string &putdata, std::vector<std::string> &headers)
{
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> vHeaderData;

	bool httpOK = HTTPClient::PUTBinary(url, putdata, headers, vHTTPResponse, vHeaderData, HTTPTIMEOUT);

	return process_response(vHTTPResponse, vHeaderData, httpOK);
}


std::string CEvohomeWeb::process_response(std::vector<unsigned char> vHTTPResponse, std::vector<std::string> vHeaderData, bool httpOK)
{
	std::string sz_response;
	std::string sz_retcode;
	std::string sz_rettext;

	sz_response.insert(sz_response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());

	if (!vHeaderData.empty())
	{

		size_t pos = vHeaderData[0].find(' ');
		if (pos != std::string::npos)
		{
			pos++;
			while (((vHeaderData[0][pos] & 0xF0) == 0x30) && ((vHeaderData[0][pos] & 0x0F) < 10))
			{
				sz_retcode.append(1,vHeaderData[0][pos]);
				pos++;
			}
			pos++;
			if (pos < vHeaderData[0].size())
				sz_rettext = vHeaderData[0].substr(pos);
		}

		if (sz_retcode.size() == 3)
//		if (vHeaderData[0][0] == 'H')
		{
			// HTTPClient GET method sets httpOK to false if HTTP return code > 400
			// reset the value to true - we want to process the body
			httpOK = true;
		}

		if (sz_retcode.empty())
		{
			// fallthrough in case of an unexpected header content
			sz_retcode = vHeaderData[0];
		}

		if (!httpOK)
		{
			 // sz_retcode contains a Curl status code
			 sz_response = R"({"code":")";
			 sz_response.append(sz_retcode);
			 sz_response.append(R"(","message":")");
			 if (!sz_rettext.empty())
				 sz_response.append(sz_rettext);
			 else
			 {
				 sz_response.append("HTTP client error ");
				 sz_response.append(sz_retcode);
			 }
			 sz_response.append("\"}");
			 return sz_response;
		}

		if ((sz_retcode != "200") && (!sz_response.empty()))
		{
			if ((sz_response[0] == '[') || (sz_response[0] == '{'))
			{
				// append code to the json response so it will take preference over any existing (textual) message code
				size_t pos = sz_response.find_last_of('}');
				if (pos != std::string::npos)
				{
					sz_response.insert(pos, R"(,"code":"")");
					sz_response.insert(pos+9, sz_retcode);
					return sz_response;
				}
			}
		}
	}

	if (sz_response.empty())
	{
		if (sz_retcode.empty())
			return R"({"code":"-1","message":"Evohome portal did not return any data or status"})";

		sz_response = R"({"code":")";
		sz_response.append(sz_retcode);
		sz_response.append(R"(","message":")");
		if (!sz_rettext.empty())
			sz_response.append(sz_rettext);
		else
		{
			sz_response.append("HTTP ");
			sz_response.append(sz_retcode);
		}
		sz_response.append(sz_retcode);
		sz_response.append("\"}");
		return sz_response;
	}

	if ((sz_response[0] == '[') || (sz_response[0] == '{')) // okay, appears to be json
		return sz_response;

	if (sz_response.find("<title>") != std::string::npos) // received an HTML page
	{
		std::stringstream ss_error;
		ss_error << R"({"code":")";
		if (!sz_retcode.empty())
			ss_error << sz_retcode;
		else
			ss_error << "-1";
		ss_error << R"(","message":")";
		int i = sz_response.find("<title>");
		char* html = &sz_response[i];
		i = 7;
		char c = html[i];
		while (c != '<')
		{
			ss_error << c;
			i++;
			c = html[i];
		}
		ss_error << "\"}";
		return ss_error.str();
	}

	if (sz_response.find("<html>") != std::string::npos) // received an HTML page without a title
	{
		std::stringstream ss_error;
		ss_error << R"({"code":")";
		if (!sz_retcode.empty())
			ss_error << sz_retcode;
		else
			ss_error << "-1";
		ss_error << R"(","message":")";
		size_t i = 0;
		char* html = &sz_response[0];
		char c;
		while (i < sz_response.size())
		{
			c = html[i];
			if (c == '<')
			{
				while (c != '>')
				{
					i++;
					c = html[i];
				}
				i++;
			}
			else if (c != '<')
			{
				ss_error << c;
				i++;
			}
		}
		ss_error << "\"}";
		return ss_error.str();
	}
	return R"({"code":"-1","message":"unhandled response"})";
}

