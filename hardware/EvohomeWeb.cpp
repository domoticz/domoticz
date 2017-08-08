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


CEvohomeWeb::~CEvohomeWeb(void)
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
		_log.Log(LOG_STATUS, "(%s) connect to Evohome server", this->Name.c_str());
	m_loggedon = false;
	if (!login(m_username, m_password))
	{
		m_logonfailures++;
		if (m_logonfailures > LOGONFAILTRESHOLD)
			_log.Log(LOG_STATUS, "(%s) logon fail treshold reached - trottling", this->Name.c_str());
		if ((m_logonfailures * m_refreshrate) > MAXPOLINTERVAL)
			m_logonfailures--;
		return false;
	}
	if (!full_installation())
	{
		_log.Log(LOG_ERROR, "(%s) failed to retrieve installation info from server", this->Name.c_str());
		return false;
	}
	m_tcs = NULL;
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
		_log.Log(LOG_ERROR, "(%s) installation at [%d,%d,%d] does not exist - verify your settings", this->Name.c_str(), m_locationId, m_gatewayId, m_systemId);
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
			if (splitresults.size()>0)
				m_awaysetpoint = strtod(splitresults[0].c_str(), NULL);
			if (splitresults.size()>1)
				m_wdayoff = atoi(splitresults[1].c_str()) % 7;
		}
		if (m_awaysetpoint == 0)
			m_awaysetpoint = 15; // use default 'Away' setpoint value
	}

	if (m_showhdtemps)
		v1_login(m_username, m_password);

	m_zones[0] = 0;
	m_loggedon = true;
	m_logonfailures = 0;
	m_sessiontimer = mytime(NULL) + 3600 - m_refreshrate; // Honeywell will drop our session after an hour
	m_bequiet = false;
	return true;
}


bool CEvohomeWeb::StartHardware()
{
	if (m_username.empty() || m_password.empty())
		return false;
	Init();
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEvohomeWeb::Do_Work, this)));
	if (!m_thread)
		return false;
	m_stoprequested = false;
	m_bIsStarted = true;
	sOnConnected(this);
	return true;
}


bool CEvohomeWeb::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted = false;
	return true;
}


void CEvohomeWeb::Do_Work()
{
	int sec_counter = m_refreshrate - MINPOLINTERVAL;
	int pollcounter = LOGONFAILTRESHOLD;
	_log.Log(LOG_STATUS, "(%s) Worker started...", this->Name.c_str());
	m_lastconnect=0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		m_lastconnect++;
		if (sec_counter % 10 == 0) {
			m_LastHeartbeat = mytime(NULL);
			if (m_loggedon && (m_LastHeartbeat > m_sessiontimer)) // discard our session with the honeywell server
			{
				m_loggedon = false;
				m_bequiet = true;
			}
		}
		if ((sec_counter % m_refreshrate == 0) && (pollcounter++ > m_logonfailures) && (m_lastconnect>=MINPOLINTERVAL))
		{
			GetStatus();
			int pollcounter = LOGONFAILTRESHOLD;
			m_lastconnect=0;
		}
	}
	_log.Log(LOG_STATUS, "(%s) Worker stopped...", this->Name.c_str());
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
	REVOBUF *tsen = (REVOBUF*)pdata;
	switch (pdata[1])
	{
	case pTypeEvohome:
		if (length < sizeof(REVOBUF::_tEVOHOME1))
			return false;
		return SetSystemMode(tsen->EVOHOME1.status);
		break;
	case pTypeEvohomeZone:
		if (length < sizeof(REVOBUF::_tEVOHOME2))
			return false;
		return SetSetpoint(pdata);
		break;
	case pTypeEvohomeWater:
		if (length < sizeof(REVOBUF::_tEVOHOME2))
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

	_log.Log(LOG_NORM, "(%s) fetch data from server", this->Name.c_str());

	// system status
	DecodeControllerMode(m_tcs);

	if (m_showhdtemps)
		get_v1_temps();

	// cycle all zones for status
	for (std::vector<zone>::size_type i = 0; i < m_tcs->zones.size(); ++i)
		DecodeZone(&m_tcs->zones[i]);

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
		_log.Log(LOG_NORM, "(%s) changed system status to %s", this->Name.c_str(), GetControllerModeName(sysmode));

		if (sznewmode == "HeatingOff")
		{
			// cycle my zones to reflect the HeatingOff mode
			for (std::vector<zone>::size_type i = 0; i < m_tcs->zones.size(); ++i)
			{
				zone* hz = &m_tcs->zones[i];
				std::string szId, sztemperature;
				szId = (*hz->installationInfo)["zoneId"].asString();
				if ((m_showhdtemps) && !hz->hdtemp.empty())
					sztemperature = hz->hdtemp;
				else if (hz->status != NULL)
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
				uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, szId.c_str(), GetUnit_by_ID(evoID), pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, ssUpdateStat.str().c_str(), sdevname);
			}
			return true;
		}

		// cycle my zones to restore scheduled temps
		for (std::vector<zone>::size_type i = 0; i < m_tcs->zones.size(); ++i)
		{
			zone* hz = &m_tcs->zones[i];
			std::string zonemode = "";
			if (hz->status == NULL) // don't touch invalid zone - it should already show as 'Offline'
				continue;
			if (hz->status->isMember("heatSetpointStatus"))
				zonemode = (*hz->status)["heatSetpointStatus"]["setpointMode"].asString();
			if ((zonemode.size() > 9) && (zonemode.substr(9) == "Override")) // don't touch zone if it is in Override mode
				continue;

			std::string sztemperature, szsetpoint;
			std::string szuntil = "";
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
					setpoint = strtod(szsetpoint.c_str(), NULL);
				}

				// Eco lowers the setpoint of all zones by 3 degrees, but resets a zone mode to Normal setting
				// if the resulting setpoint is below the Away setpoint
				if ((sznewmode == "AutoWithEco") && (setpoint >= (m_awaysetpoint + 3)))
					setpoint -= 3;
			}

			if ((m_showhdtemps) && !hz->hdtemp.empty())
				sztemperature = hz->hdtemp;
			else if (hz->status != NULL)
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
			uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, hz->zoneId.c_str(), GetUnit_by_ID(evoID), pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, ssUpdateStat.str().c_str(), sdevname);
		}
		return true;
	}
	_log.Log(LOG_ERROR, "(%s) error changing system status", this->Name.c_str());
	m_loggedon = false;
	return false;
}


bool CEvohomeWeb::SetSetpoint(const char *pdata)
{
	REVOBUF *pEvo = (REVOBUF*)pdata;

	std::stringstream ssID;
	ssID << std::dec << (int)RFX_GETID3(pEvo->EVOHOME2.id1, pEvo->EVOHOME2.id2, pEvo->EVOHOME2.id3);
	std::string zoneId(ssID.str());

	zone* hz = get_zone_by_ID(zoneId);
	if (hz == NULL) // zone number not known by installation (manually added?)
	{
		_log.Log(LOG_ERROR, "(%s) attempt to change setpoint on unknown zone", this->Name.c_str());
		return false;
	}

	if ((pEvo->EVOHOME2.mode) == 0) // cancel override
	{
		if (!cancel_temperature_override(zoneId))
			return false;

		std::string szsetpoint;
		std::string szuntil = "";
		if ((!hz->schedule.isNull()) || get_zone_schedule(hz->zoneId))
		{
			szuntil = local_to_utc(get_next_switchpoint_ex(hz->schedule, szsetpoint));
			pEvo->EVOHOME2.temperature = (int16_t)(strtod(szsetpoint.c_str(), NULL) * 100);
		}

		if ((m_showschedule) && (!szuntil.empty()))
		{
			pEvo->EVOHOME2.year = (uint16_t)(atoi(szuntil.substr(0, 4).c_str()));
			pEvo->EVOHOME2.month = (uint8_t)(atoi(szuntil.substr(5, 2).c_str()));
			pEvo->EVOHOME2.day = (uint8_t)(atoi(szuntil.substr(8, 2).c_str()));
			pEvo->EVOHOME2.hrs = (uint8_t)(atoi(szuntil.substr(11, 2).c_str()));
			pEvo->EVOHOME2.mins = (uint8_t)(atoi(szuntil.substr(14, 2).c_str()));
		}
		else
			pEvo->EVOHOME2.year = 0;
		return true;
	}

	int temperature_int = (int)pEvo->EVOHOME2.temperature / 100;
	int temperature_frac = (int)pEvo->EVOHOME2.temperature % 100;
	std::stringstream s_setpoint;
	s_setpoint << temperature_int << "." << temperature_frac;

	if ((pEvo->EVOHOME2.mode) == 1) // permanent override
	{
		return set_temperature(zoneId, s_setpoint.str(), "");
	}
	if ((pEvo->EVOHOME2.mode) == 2) // temporary override
	{
		std::string szISODate(CEvohomeDateTime::GetISODate(pEvo->EVOHOME2));
		return set_temperature(zoneId, s_setpoint.str(), szISODate);
	}
	return false;
}


bool CEvohomeWeb::SetDHWState(const char *pdata)
{
	if (!has_dhw(m_tcs)) // Installation has no Hot Water device
	{
		_log.Log(LOG_ERROR, "(%s) attempt to set state on non existing Hot Water device", this->Name.c_str());
		return false;
	}

	REVOBUF *pEvo = (REVOBUF*)pdata;

	std::stringstream ssID;
	ssID << std::dec << (int)RFX_GETID3(pEvo->EVOHOME2.id1, pEvo->EVOHOME2.id2, pEvo->EVOHOME2.id3);
	std::string dhwId(ssID.str());

	std::string DHWstate = (pEvo->EVOHOME2.temperature == 0) ? "off" : "on";

	if ((pEvo->EVOHOME2.mode) == 0) // cancel override (web front end does not appear to support this?)
	{
		DHWstate = "auto";
	}
	if ((pEvo->EVOHOME2.mode) <= 1) // permanent override
	{
		return set_dhw_mode(dhwId, DHWstate, "");
	}
	if ((pEvo->EVOHOME2.mode) == 2) // temporary override
	{
		std::string szISODate(CEvohomeDateTime::GetISODate(pEvo->EVOHOME2));
		return set_dhw_mode(dhwId, DHWstate, szISODate);
	}
	return false;
}


void CEvohomeWeb::DecodeControllerMode(temperatureControlSystem* tcs)
{
	unsigned long ID = (unsigned long)(strtod(tcs->systemId.c_str(), NULL));
	std::string szsystemMode, szmodelType;
	uint8_t sysmode = 0;

	if (tcs->status == NULL)
		szsystemMode = "Unknown";
	else
		szsystemMode = (*tcs->status)["systemModeStatus"]["mode"].asString();
	while (sysmode < 7 && strcmp(szsystemMode.c_str(), m_szWebAPIMode[sysmode]) != 0)
		sysmode++;

	REVOBUF tsen;
	memset(&tsen, 0, sizeof(REVOBUF));
	tsen.EVOHOME1.len = sizeof(tsen.EVOHOME1) - 1;
	tsen.EVOHOME1.type = pTypeEvohome;
	tsen.EVOHOME1.subtype = sTypeEvohome;
	RFX_SETID3(ID, tsen.EVOHOME1.id1, tsen.EVOHOME1.id2, tsen.EVOHOME1.id3);
	tsen.EVOHOME1.mode = 0; // web API does not support temp override of controller mode
	tsen.EVOHOME1.status = sysmode;
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME1, "Controller mode", -1);

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
			time_t now = mytime(NULL);
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
	std::string szuntil= "";
	std::stringstream ssUpdateStat;

	szId = (*hz->installationInfo)["zoneId"].asString();
	if (hz->status == NULL)
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
	if (m_tcs->status == NULL)
		szsysmode = "Unknown";
	else
		szsysmode = (*m_tcs->status)["systemModeStatus"]["mode"].asString();
	if ((szsysmode == "Away") && (szmode == "FollowSchedule"))
	{
		double new_awaysetpoint = strtod(szsetpoint.c_str(), NULL);
		if (m_awaysetpoint != new_awaysetpoint)
		{
			m_awaysetpoint = new_awaysetpoint;
			m_sql.safe_query("UPDATE Hardware SET Extra='%0.2f;%d' WHERE ID=%d", m_awaysetpoint, m_wdayoff, this->m_HwdID);
			_log.Log(LOG_STATUS, "(%s) change Away setpoint to '%0.2f' because of non matching setpoint (%s)", this->Name.c_str(), m_awaysetpoint, (*hz->installationInfo)["name"].asString().c_str());
		}
		ssUpdateStat << sztemperature << ";" << szsetpoint << ";" << szsysmode;
	}
	else if (szsysmode == "HeatingOff")
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

						_log.Log(LOG_STATUS, "(%s) change Day Off schedule reference to '%s' because of non matching setpoint (%s)", this->Name.c_str(), weekdays[m_wdayoff].c_str(), (*hz->installationInfo)["name"].asString().c_str());
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
				_log.Log(LOG_STATUS, "(%s) register new zone '%s'", this->Name.c_str(), ssnewname.str().c_str());
		}
	}
}


void CEvohomeWeb::DecodeDHWState(temperatureControlSystem* tcs)
{
	// Hot Water is essentially just another zone
	if ((tcs->status == NULL) || (!tcs->status->isMember("dhw") || !(*tcs->status)["dhw"].isMember("temperatureStatus") || !(*tcs->status)["dhw"].isMember("stateStatus")))
		return;

	std::string szId, szmode;
	std::string szuntil= "";
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

		if (result.size() < 1) // create device
		{
			std::string sdevname;
			uint64_t DevRowIdx = m_sql.UpdateValue(this->m_HwdID, szId.c_str(), 1, pTypeEvohomeWater, sTypeEvohomeWater, 10, 255, 50, "0.0;Off;Auto", sdevname);
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")", ndevname.c_str(), DevRowIdx);
		}
		else if ((result[0][1] != szId) || (result[0][2] != ndevname))
		{
			uint64_t DevRowIdx;
			std::stringstream s_str(result[0][0]);
			s_str >> DevRowIdx;
			// also wipe StrParam1 - we do not want a double action from the old (python) script when changing the setpoint
			m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%q', Name='%q', StrParam1='' WHERE (ID == %" PRIu64 ")", szId.c_str(), ndevname.c_str(), DevRowIdx);
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
			if (m_zones[unit] == (unsigned long)(unit + 92000)) // mark manually added, unlinked zone as free
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
					return -1;
				char devname[8];
				sprintf(devname, "zone %zu", row);
				sprintf(ID, "%lu", evoID);
				m_sql.safe_query("UPDATE DeviceStatus SET Name='%q',DeviceID='%q' WHERE (ID == %" PRIu64 ")", devname, ID, DevRowIdx);
				m_zones[row] = evoID;
				return (uint8_t)(unit);
			}
		}
		_log.Log(LOG_ERROR, "(%s) cannot add new zone because you have no free zones left", this->Name.c_str());
	}
	return -1;
}


/*
 * Helper to convert local time strings to UTC time strings
 */
std::string CEvohomeWeb::local_to_utc(std::string local_time)
{
	if (local_time.size() < 19)
		return "";
	if (m_tzoffset == -1)
	{
		// calculate timezone offset once
		time_t now = mytime(NULL);
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
	char until[22];
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
	std::stringstream pdata;
	pdata << "installationInfo-Type=application%2Fx-www-form-urlencoded;charset%3Dutf-8";
	pdata << "&Host=rs.alarmnet.com%2F";
	pdata << "&Cache-Control=no-store%20no-cache";
	pdata << "&Pragma=no-cache";
	pdata << "&grant_type=password";
	pdata << "&scope=EMEA-V1-Basic%20EMEA-V1-Anonymous%20EMEA-V1-Get-Current-User-Account";
	pdata << "&Username=" << CURLEncode::URLEncode(user);
	pdata << "&Password=" << CURLEncode::URLEncode(password);
	pdata << "&Connection=Keep-Alive";

	std::vector<std::string> LoginHeaders;
	LoginHeaders.push_back("Authorization: Basic YjAxM2FhMjYtOTcyNC00ZGJkLTg4OTctMDQ4YjlhYWRhMjQ5OnRlc3Q=");
	LoginHeaders.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	LoginHeaders.push_back("charsets: utf-8");

	std::string s_res;
	if (!HTTPClient::POST(EVOHOME_HOST"/Auth/OAuth/Token", pdata.str(), LoginHeaders, s_res))
	{
		_log.Log(LOG_ERROR, "(%s) HTTP client error at login!", this->Name.c_str());
		return false;
	}

	if (s_res.find("<title>") != std::string::npos) // got an HTML page
	{
		int i = s_res.find("<title>");
		char* html = &s_res[i];
		i = 7;
		char c = html[i];
		std::stringstream edata;
		while (c != '<')
		{
			edata << c;
			i++;
			c = html[i];
		}
		_log.Log(LOG_ERROR, "(%s) login failed with message: %s", this->Name.c_str(), edata.str().c_str());
		return false;
	}

	Json::Value j_login;
	Json::Reader jReader;
	bool ret = jReader.parse(s_res.c_str(), j_login);

	std::string szError = "";
	if (j_login.isMember("error"))
		szError = j_login["error"].asString();
	if (j_login.isMember("message"))
		szError = j_login["message"].asString();
	if (!szError.empty())
	{
		_log.Log(LOG_ERROR, "(%s) login failed with message: %s", this->Name.c_str(), szError.c_str());
		return false;
	}


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
 * Retrieve evohome user info
 */
bool CEvohomeWeb::user_account()
{
	std::string s_res;
	if (!HTTPClient::GET(EVOHOME_HOST"/WebAPI/emea/api/v1/userAccount", m_SessionHeaders, s_res))
	{
		_log.Log(LOG_ERROR, "(%s) HTTP client error at retrieve user account info!", this->Name.c_str());
		return false;
	}
	Json::Value j_account;
	Json::Reader jReader;
	bool ret = jReader.parse(s_res.c_str(), j_account);

	if (!j_account.isMember("userId"))
		return false;

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
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/location/installationInfo?userId=" << m_evouid << "&includeTemperatureControlSystems=True";

	std::string s_res;
	if (!HTTPClient::GET(url.str(), m_SessionHeaders, s_res))
	{
		_log.Log(LOG_ERROR, "(%s) HTTP client error at retrieve installation!", this->Name.c_str());
		return false;
	}

	// evohome API does not correctly format the json output
	std::stringstream ss_jdata;
	ss_jdata << "{\"locations\": " << s_res << "}";
	Json::Reader jReader;
	m_j_fi.clear();
	bool ret = jReader.parse(ss_jdata.str(), m_j_fi);

	if (!m_j_fi["locations"].isArray())
		return false; // invalid return

	size_t l = m_j_fi["locations"].size();
	if (l == 0)
		return false; // invalid return
	if (m_j_fi["locations"][0].isMember("message"))
	{
		_log.Log(LOG_ERROR, "(%s) retrieve installation returned message: %s", this->Name.c_str(), m_j_fi["locations"][0]["message"].asString().c_str());
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

bool CEvohomeWeb::get_status(std::string locationId)
{
	if (m_locations.size() == 0)
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
	Json::Value *j_loc, *j_gw, *j_tcs;
	if ((m_locations.size() == 0) || m_locations[location].locationId.empty())
		return false;

	bool valid_json = true;
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/location/" << m_locations[location].locationId << "/status?includeTemperatureControlSystems=True";
	std::string s_res;
	if (!HTTPClient::GET(url.str(), m_SessionHeaders, s_res))
	{
		_log.Log(LOG_ERROR, "(%s) HTTP client error at get status!", this->Name.c_str());
		return false;
	}

	Json::Reader jReader;
	m_j_stat.clear();
	bool ret = jReader.parse(s_res, m_j_stat);
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
		_log.Log(LOG_ERROR, "(%s) status request did not return a valid response", this->Name.c_str());
	return valid_json;
}


/************************************************************************
 *									*
 *	Locate Evohome objects by ID					*
 *									*
 ************************************************************************/

CEvohomeWeb::zone* CEvohomeWeb::get_zone_by_ID(std::string zoneId)
{
	if (m_locations.size() == 0)
		full_installation();
	for (size_t l = 0; l < m_locations.size(); l++)
	{
		for (size_t g = 0; g < m_locations[l].gateways.size(); g++)
		{
			for (size_t t = 0; t < m_locations[l].gateways[g].temperatureControlSystems.size(); t++)
			{
				for (size_t z = 0; z < m_locations[l].gateways[g].temperatureControlSystems[t].zones.size(); z++)
				{
					if (m_locations[l].gateways[g].temperatureControlSystems[t].zones[z].zoneId == zoneId)
						return &m_locations[l].gateways[g].temperatureControlSystems[t].zones[z];
				}
				if (m_locations[l].gateways[g].temperatureControlSystems[t].dhw.size() > 0)
				{
					if (m_locations[l].gateways[g].temperatureControlSystems[t].dhw[0].zoneId == zoneId)
						return &m_locations[l].gateways[g].temperatureControlSystems[t].dhw[0];
				}
			}
		}
	}
	return NULL;
}


/************************************************************************
 *									*
 *	Schedule handlers						*
 *									*
 ************************************************************************/

bool CEvohomeWeb::get_zone_schedule(std::string zoneId)
{
	return get_zone_schedule(zoneId, "temperatureZone");
}
bool CEvohomeWeb::get_dhw_schedule(std::string dhwId)
{
	return get_zone_schedule(dhwId, "domesticHotWater");
}
bool CEvohomeWeb::get_zone_schedule(std::string zoneId, std::string zoneType)
{
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/" << zoneType << "/" << zoneId << "/schedule";
	std::string s_res;
	if ((!HTTPClient::GET(url.str(), m_SessionHeaders, s_res)) || (!s_res.find("\"id\"")))
		return false;
	Json::Reader jReader;
	zone* hz = get_zone_by_ID(zoneId);
	if (hz == NULL)
		return false;
	bool ret = jReader.parse(s_res, hz->schedule);
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
	if ((hz->schedule.isNull()) && !get_zone_schedule(hz->zoneId))
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
	time_t now = mytime(NULL);
	localtime_r(&now, &ltime);
	int year = ltime.tm_year;
	int month = ltime.tm_mon;
	int day = ltime.tm_mday;
	int wday = ltime.tm_wday;
	char rdata[30];
	sprintf(rdata, "%04d-%02d-%02dT%02d:%02d:%02dZ", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	std::string szdatetime = std::string(rdata);
	if (szdatetime <= schedule["nextSwitchpoint"].asString()) // our current cached values are still valid
	{
		current_setpoint = schedule["currentSetpoint"].asString();
		return schedule["nextSwitchpoint"].asString();
	}

	// Hack: DayOff needs to reference a specific weekday rather than current
	if (m_tcs->status != NULL)
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
		if (!found)
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
			if (!found)
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


bool CEvohomeWeb::verify_datetime(std::string datetime)
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
	char c_date[12];
	sprintf(c_date, "%04d-%02d-%02d", mtime.tm_year + 1900, mtime.tm_mon + 1, mtime.tm_mday);
	char c_time[12];
	sprintf(c_time, "%02d:%02d:%02d", mtime.tm_hour, mtime.tm_min, mtime.tm_sec);
	return ((s_date == std::string(c_date)) && (s_time == std::string(c_time)));
}


bool CEvohomeWeb::set_system_mode(std::string systemId, int mode)
{
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/temperatureControlSystem/" << systemId << "/mode";
	std::stringstream data;
	data << "{\"SystemMode\":" << mode << ",\"TimeUntil\":null,\"Permanent\":true}";
	std::string s_res;
	if ((HTTPClient::PUT(url.str(), data.str(), m_SessionHeaders, s_res)) && (s_res.find("\"id\"")))
		return true;
	return false;
}


bool CEvohomeWeb::set_temperature(std::string zoneId, std::string temperature, std::string time_until)
{
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/temperatureZone/" << zoneId << "/heatSetpoint";
	std::stringstream data;
	data << "{\"HeatSetpointValue\":" << temperature;
	if (time_until == "")
		data << ",\"SetpointMode\":1,\"TimeUntil\":null}";
	else
	{
		if (!verify_datetime(time_until))
			return false;
		data << ",\"SetpointMode\":2,\"TimeUntil\":\"" << time_until.substr(0, 10) << "T" << time_until.substr(11, 8) << "Z\"}";
	}
	std::string s_res;
	if ((HTTPClient::PUT(url.str(), data.str(), m_SessionHeaders, s_res)) && (s_res.find("\"id\"")))
		return true;
	return false;
}


bool CEvohomeWeb::cancel_temperature_override(std::string zoneId)
{
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/temperatureZone/" << zoneId << "/heatSetpoint";
	std::string s_data = "{\"HeatSetpointValue\":0.0,\"SetpointMode\":0,\"TimeUntil\":null}";
	std::string s_res;
	if ((HTTPClient::PUT(url.str(), s_data, m_SessionHeaders, s_res)) && (s_res.find("\"id\"")))
		return true;
	return false;
}


bool CEvohomeWeb::has_dhw(CEvohomeWeb::temperatureControlSystem *tcs)
{
	return (*tcs->installationInfo).isMember("dhw");
}


bool CEvohomeWeb::set_dhw_mode(std::string dhwId, std::string mode, std::string time_until)
{
	std::stringstream data;
	if (mode == "auto")
		data << "{\"State\":0,\"Mode\":0,\"UntilTime\":null}";
	else
	{
		data << "{\"State\":";
		if (mode == "on")
			data << 1;
		else
			data << 0;
		if (time_until == "")
			data << ",\"Mode\":1,\"UntilTime\":null}";
		else
		{
			if (!verify_datetime(time_until))
				return false;
			data << ",\"Mode\":2,\"UntilTime\":\"" << time_until.substr(0, 10) << "T" << time_until.substr(11, 8) << "Z\"}";
		}
	}
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/emea/api/v1/domesticHotWater/" << dhwId << "/state";
	std::string s_res;
	if ((HTTPClient::PUT(url.str(), data.str(), m_SessionHeaders, s_res)) && (s_res.find("\"id\"")))
		return true;
	return false;
}


bool CEvohomeWeb::v1_login(const std::string &user, const std::string &password)
{
	std::stringstream pdata;
	pdata << "{'Username': '" << user << "', 'Password': '" << password << "'";
	pdata << ", 'ApplicationId': '91db1612-73fd-4500-91b2-e63b069b185c'}";

	std::vector<std::string> LoginHeaders;
	LoginHeaders.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	LoginHeaders.push_back("content-type: application/json");

	std::string s_res;
	if (!HTTPClient::POST(EVOHOME_HOST"/WebAPI/api/Session", pdata.str(), LoginHeaders, s_res))
	{
		_log.Log(LOG_ERROR, "(%s) HTTP client error at login!", this->Name.c_str());
		return false;
	}

	if (s_res.find("<title>") != std::string::npos) // got an HTML page
	{
		int i = s_res.find("<title>");
		char* html = &s_res[i];
		i = 7;
		char c = html[i];
		std::stringstream edata;
		while (c != '<')
		{
			edata << c;
			i++;
			c = html[i];
		}
		_log.Log(LOG_ERROR, "(%s) login failed with message: %s", this->Name.c_str(), edata.str().c_str());
		return false;
	}

	if (s_res[0] == '[') // got unnamed array as reply
	{
		s_res[0] = ' ';
		size_t len = s_res.size();
		len--;
		s_res[len] = ' ';
	}

	Json::Value j_login;
	Json::Reader jReader;
	if (!jReader.parse(s_res.c_str(), j_login))
	{
		_log.Log(LOG_ERROR, "(%s) cannot parse return data from v1 login", this->Name.c_str());
		return false;
	}

	std::string szError = "";
	if (j_login.isMember("error"))
		szError = j_login["error"].asString();
	if (j_login.isMember("message"))
		szError = j_login["message"].asString();
	if (!szError.empty())
	{
		_log.Log(LOG_ERROR, "(%s) v1 login failed with message: %s", this->Name.c_str(), szError.c_str());
		return false;
	}

	if (!j_login.isMember("sessionId") || !j_login.isMember("userInfo") || !j_login["userInfo"].isObject() || !j_login["userInfo"].isMember("userID"))
	{
		_log.Log(LOG_ERROR, "(%s) v1 login returned unknown data", this->Name.c_str());
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


void CEvohomeWeb::get_v1_temps()
{
	std::stringstream url;
	url << EVOHOME_HOST << "/WebAPI/api/locations/?userId=" << m_v1uid << "&allData=True";
	std::string s_res;
	if (!HTTPClient::GET(url.str(), m_v1SessionHeaders, s_res))
	{
		_log.Log(LOG_ERROR, "(%s) HTTP client error at retrieve of v1 temperatures!", this->Name.c_str());
		return;
	}

	// evohome old API does not correctly format the json output
	std::stringstream ss_jdata;
	ss_jdata << "{\"locations\": " << s_res << "}";
	Json::Reader jReader;
	Json::Value j_fi;
	if (!jReader.parse(ss_jdata.str(), j_fi))
		return;

	size_t l = 0;
	if (j_fi["locations"].isArray())
		l = j_fi["locations"].size();
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
			if (v2zone != NULL)
			{
				double v1temp = (*j_dev)["thermostat"]["indoorTemperature"].asDouble();
				if (v1temp > 127) // allow rounding error
				{
					_log.Log(LOG_ERROR, "(%s) zone %s does not report a temperature", this->Name.c_str(), zoneId.c_str());
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

