/**
 * Skeleton class for Integrated Evohome client for Domoticz
 *
 *  Copyright 2017 - gordonb3 https://github.com/gordonb3/evohomeclient
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <https://github.com/gordonb3/evohomeclient/blob/master/LICENSE>
 */
#include "stdafx.h"
#include "EvohomeWeb.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"

#include <string>


extern std::string szUserDataFolder;


CEvohomeWeb::CEvohomeWeb(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
}


CEvohomeWeb::~CEvohomeWeb(void)
{
	m_bIsStarted=false;
}


void CEvohomeWeb::Init()
{

}


bool CEvohomeWeb::StartHardware()
{
	Init();
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEvohomeWeb::Do_Work, this)));
	if (!m_thread)
		return false;
	m_bIsStarted=true;
	sOnConnected(this);
	return true;
}


bool CEvohomeWeb::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted=false;
	return true;
}


void CEvohomeWeb::Do_Work()
{
	_log.Log(LOG_STATUS, "EvohomeWeb: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 10 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (sec_counter % 60 == 0)
		{
			GetStatus();
		}
	}
	_log.Log(LOG_STATUS,"EvohomeWeb: Worker stopped...");

}


bool CEvohomeWeb::WriteToHardware(const char *pdata, const unsigned char length)
{
	if(!pdata)
		return false;
	REVOBUF *tsen=(REVOBUF*)pdata;
	switch (pdata[1])
	{
		case pTypeEvohome:
			if (length<sizeof(REVOBUF::_tEVOHOME1))
				return false;
			SetSystemMode();
/*
   Hazard!
   This is a switch so it will also run the on action script if we transfer the EvohomeScript devices
*/
			break;
		case pTypeEvohomeZone:
			if (length<sizeof(REVOBUF::_tEVOHOME2))
				return false;
			SetTemperature();
			break;
		case pTypeEvohomeWater:
			if (length<sizeof(REVOBUF::_tEVOHOME2))
				return false;
			SetDHWState()
			break;
	}
	return true;
}


void CEvohomeWeb::GetStatus()
{
// cmd_update();
}


void CEvohomeWeb::SetSystemMode()
{
// cmd_set_system_mode()
}


void CEvohomeWeb::SetTemperature()
{
// cmd_set_temperature()
}


void CEvohomeWeb::SetDHWState()
{
// cmd_set_dhw_state()
}

