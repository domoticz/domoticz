/**
 * Evohome base class for evohome control, monitoring and integration with Domoticz
 *
 *  Basic functions isolated from the original code by fullTalgoRythm
 *  Original header and copyright notice below
 */

/**
 * Evohome class for HGI80 gateway, evohome control, monitoring and integration with Domoticz
 *
 *  Copyright 2014 - fullTalgoRythm https://github.com/fullTalgoRythm/Domoticz-evohome
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 * based in part on https://github.com/mouse256/evomon
 * and details available at http://www.domoticaforum.eu/viewtopic.php?f=7&t=5806&start=90#p72564
 */
#include "stdafx.h"
#include "EvohomeBase.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/WebServer.h"
#include <json/json.h>


extern std::string szUserDataFolder;

std::ofstream *CEvohomeBase::m_pEvoLog = nullptr;
#ifdef _DEBUG
bool CEvohomeBase::m_bDebug=true;
#else
bool CEvohomeBase::m_bDebug=false;
#endif

constexpr std::array<const char *, 7> CEvohomeBase::m_szControllerMode{ "Normal", "Economy", "Away", "Day Off", "Custom", "Heating Off", "Unknown" };
constexpr std::array<const char *, 7> CEvohomeBase::m_szWebAPIMode{ "Auto", "AutoWithEco", "Away", "DayOff", "Custom", "HeatingOff", "Unknown" };
constexpr std::array<const char *, 7> CEvohomeBase::m_szZoneMode{ "Auto", "PermanentOverride", "TemporaryOverride", "OpenWindow", "LocalOverride", "RemoteOverride", "Unknown" };

const char* CEvohomeBase::GetControllerModeName(uint8_t nControllerMode)
{
	return m_szControllerMode[std::min(nControllerMode,(uint8_t)6)];
}

const char* CEvohomeBase::GetWebAPIModeName(uint8_t nControllerMode)
{
	return m_szWebAPIMode[std::min(nControllerMode,(uint8_t)6)];
}

const char* CEvohomeBase::GetZoneModeName(uint8_t nZoneMode)
{
	return m_szZoneMode[std::min(nZoneMode, (uint8_t)6)];
}

CEvohomeBase::CEvohomeBase()
	: m_ZoneOverrideLocal(m_nMaxZones)
	, m_ZoneNames(m_nMaxZones)
{
	m_HwdID=0;
	m_nDevID=0;
	m_nMyID=0;
	m_nZoneCount=0;
	m_nControllerMode=0;
}

CEvohomeBase::~CEvohomeBase()
{
	m_bIsStarted = false;
}

bool CEvohomeBase::SetZoneCount(uint8_t nZoneCount)
{
	std::lock_guard<std::mutex> l(m_mtxZoneCount);
	bool bRet=(m_nZoneCount!=nZoneCount);
	m_nZoneCount=nZoneCount;
	return bRet;
}


bool CEvohomeBase::SetMaxZoneCount(uint8_t nZoneCount)
{
	std::lock_guard<std::mutex> l(m_mtxZoneCount);
	uint8_t nMaxZones=std::max(m_nZoneCount,nZoneCount);
	bool bRet=(m_nZoneCount!=nMaxZones);
	m_nZoneCount=nMaxZones;
	return bRet;
}


uint8_t CEvohomeBase::GetZoneCount()
{
	std::lock_guard<std::mutex> l(m_mtxZoneCount);
	return m_nZoneCount; //return value is constructed before the lock is released
}


bool CEvohomeBase::SetControllerMode(uint8_t nControllerMode)
{
	std::lock_guard<std::mutex> l(m_mtxControllerMode);
	bool bRet=(m_nControllerMode!=nControllerMode);
	m_nControllerMode=nControllerMode;
	return bRet;
}


uint8_t CEvohomeBase::GetControllerMode()
{
	std::lock_guard<std::mutex> l(m_mtxControllerMode);
	return m_nControllerMode; //return value is constructed before the lock is released
}


void CEvohomeBase::InitControllerName()
{
	std::lock_guard<std::mutex> l(m_mtxControllerName);
	if(m_szControllerName.empty())
		m_szControllerName="EvoTouch Colour";
}


void CEvohomeBase::SetControllerName(const std::string &szName)
{
	std::lock_guard<std::mutex> l(m_mtxControllerName);
	m_szControllerName=szName;
}


std::string CEvohomeBase::GetControllerName()
{
	std::lock_guard<std::mutex> l(m_mtxControllerName);
	return m_szControllerName;
}


void CEvohomeBase::InitZoneNames()
{
	std::lock_guard<std::mutex> l(m_mtxZoneName);
	char szTmp[1024];
	for(int i=0;i<(int)m_ZoneNames.size();i++)
	{
		if(m_ZoneNames[i].empty())
		{
			sprintf(szTmp,"Zone %d",i+1);
			m_ZoneNames[i]=szTmp;
		}
	}
}


void CEvohomeBase::SetZoneName(const uint8_t nZone, const std::string &szName)
{
	std::lock_guard<std::mutex> l(m_mtxZoneName);
	if(nZone>=m_ZoneNames.size()) //should be pre-sized to max zones
		return;
	m_ZoneNames[nZone]=szName;
}


std::string CEvohomeBase::GetZoneName(uint8_t nZone)
{
	std::lock_guard<std::mutex> l(m_mtxZoneName);
	if(nZone>=m_ZoneNames.size()) //should be pre-sized to max zones
		return "Out of bounds";
	return m_ZoneNames[nZone];
}


unsigned int CEvohomeBase::GetControllerID()
{
	std::lock_guard<std::mutex> l(m_mtxControllerID);
	return m_nDevID;
}


void CEvohomeBase::SetControllerID(unsigned int nID)
{
	std::lock_guard<std::mutex> l(m_mtxControllerID);
	m_nDevID=nID;
}


unsigned int CEvohomeBase::GetGatewayID()
{
	std::lock_guard<std::mutex> l(m_mtxGatewayID);
	return m_nMyID;
}


void CEvohomeBase::SetGatewayID(unsigned int nID)
{
	std::lock_guard<std::mutex> l(m_mtxGatewayID);
	m_nMyID=nID;
}


void CEvohomeBase::LogDate()
{
        char szTmp[256];
	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	strftime(szTmp, 256, "%Y-%m-%d %H:%M:%S ", &ltime);
	*m_pEvoLog << szTmp;
}


void CEvohomeBase::Log(const char *szMsg, CEvohomeMsg &msg)
{
	if(m_bDebug && m_pEvoLog)
	{
		LogDate();
		*m_pEvoLog << szMsg;
		*m_pEvoLog << " (";
		for(int i=0;i<msg.payloadsize;i++)
		{
			unsigned char c = msg.payload[i];
			if (c < 0x20 || c > 0x7E) c = '.';
			*m_pEvoLog << c;
		}
		*m_pEvoLog << ")";
		*m_pEvoLog << std::endl;
	}
}


void CEvohomeBase::Log(bool bDebug, int nLogLevel, const char* format, ... )
{
        va_list argList;
        char cbuffer[1024];
        va_start(argList, format);
        vsnprintf(cbuffer, 1024, format, argList);
        va_end(argList);

	if(!bDebug || m_bDebug)
		_log.Log(static_cast<_eLogLevel>(nLogLevel), "%s", cbuffer);
	if(m_bDebug && m_pEvoLog)
	{
		LogDate();
		*m_pEvoLog << cbuffer;
		*m_pEvoLog << std::endl;
	}
}


//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_CreateEvohomeSensor(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string ssensortype = request::findValue(&req, "sensortype");
			if ((idx.empty()) || (ssensortype.empty()))
				return;

			bool bCreated = false;
			int iSensorType = atoi(ssensortype.c_str());

			int HwdID = atoi(idx.c_str());

			//Make a unique number for ID
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");
			unsigned long nid = 1; //could be the first device ever

			if (!result.empty())
			{
				nid = atol(result[0][0].c_str());
			}
			nid += 92000;
			char ID[40];
			sprintf(ID, "%lu", nid);

			//get zone count
			result = m_sql.safe_query("SELECT COUNT(*) FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d)", HwdID, (int)iSensorType);

			int nDevCount = 0;
			if (!result.empty())
			{
				nDevCount = atol(result[0][0].c_str());
			}

			std::string devname;

			switch (iSensorType)
			{
			case pTypeEvohome: //Controller...should be 1 controller per hardware
				if (nDevCount >= 1)
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of controllers reached";
					return;
				}
				m_sql.UpdateValue(HwdID, ID, 0, pTypeEvohome, sTypeEvohome, 10, 255, 0, "Normal", devname);
				bCreated = true;
				break;
			case pTypeEvohomeZone://max of 12 zones
				if (nDevCount >= CEvohomeBase::m_nMaxZones)
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of supported zones reached";
					return;
				}
				m_sql.UpdateValue(HwdID, ID, (uint8_t)nDevCount + 1, pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, "0.0;0.0;Auto", devname);
				bCreated = true;
				break;
			case pTypeEvohomeWater://DHW...should be 1 per hardware
				if (nDevCount >= 1)
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of DHW zones reached";
					return;
				}
				m_sql.UpdateValue(HwdID, ID, 1, pTypeEvohomeWater, sTypeEvohomeWater, 10, 255, 50, "0.0;Off;Auto", devname);
				bCreated = true;
				break;
			}
			if (bCreated)
			{
				root["status"] = "OK";
				root["title"] = "CreateEvohomeSensor";
			}
		}
	} // namespace server
} // namespace http
