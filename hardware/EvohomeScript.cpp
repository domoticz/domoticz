/**
 * Class for running external Evohome script(s)
 *
 *  Functions extracted from the original code by fullTalgoRythm
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
#include "EvohomeScript.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
//#include "../main/RFXtrx.h"
//#include "../main/Helper.h"
#include "../main/SQLHelper.h"
//#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <string>

extern std::string szUserDataFolder;


CEvohomeScript::CEvohomeScript(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
}


CEvohomeScript::~CEvohomeScript(void)
{
	m_bIsStarted=false;
}


void CEvohomeScript::Init()
{

}


bool CEvohomeScript::StartHardware()
{
	Init();
	m_bIsStarted=true;
	sOnConnected(this);
	return true;
}


bool CEvohomeScript::StopHardware()
{
	m_bIsStarted=false;
	return true;
}


bool CEvohomeScript::WriteToHardware(const char *pdata, const unsigned char length)
{
	if(!pdata)
		return false;
	switch (pdata[1])
	{
		case pTypeEvohome:
			if (length<sizeof(_tEVOHOME1))
				return false;
			//This is a switch so the on action script will be run anyway
			break;
		case pTypeEvohomeZone:
			if (length<sizeof(_tEVOHOME2))
				return false;
			RunScript(pdata,length);
			break;
		case pTypeEvohomeWater:
			if (length<sizeof(_tEVOHOME2))
				return false;
			RunScript(pdata,length);
			break;
		case pTypeEvohomeRelay:
			if (length<sizeof(_tEVOHOME3))
				return false;
			//Only supported by HGI80
			break;
	}
	return true;
}


void CEvohomeScript::RunScript(const char *pdata, const unsigned char /*length*/)
{
	if(!pdata)
		return;
	_tEVOHOME2 *tsen=(_tEVOHOME2*)pdata;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT  HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1 FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d)",	m_HwdID, (int)tsen->zone, (int)tsen->type);
	if (!result.empty())
	{
		unsigned long ID;
		std::vector<std::string> sd=result[0];
		std::stringstream s_strid;
		s_strid << std::hex << sd[1];
		s_strid >> ID;
		
		std::string OnAction(sd[6]);
		if (OnAction.find("script://")!=std::string::npos)
		{
			s_strid.clear();
			s_strid.str("");
			s_strid << ID;
			boost::replace_all(OnAction, "{deviceid}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << (int)tsen->zone;
			boost::replace_all(OnAction, "{unit}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << (int)tsen->mode;
			boost::replace_all(OnAction, "{mode}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << tsen->temperature / 100.0f;
			boost::replace_all(OnAction, "{setpoint}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << (int)tsen->temperature;
			boost::replace_all(OnAction, "{state}", s_strid.str());
			boost::replace_all(OnAction, "{until}", CEvohomeDateTime::GetISODate(tsen));
			//Execute possible script
			std::string scriptname = OnAction.substr(9);
#if !defined WIN32
			if (scriptname.find("/") != 0)
				scriptname = szUserDataFolder + "scripts/" + scriptname;
#endif
			std::string scriptparams="";
			//Add parameters
			int pindex=scriptname.find(' ');
			if (pindex!=std::string::npos)
			{
				scriptparams=scriptname.substr(pindex+1);
				scriptname=scriptname.substr(0,pindex);
			}
			
			if (file_exist(scriptname.c_str()))
			{
				m_sql.AddTaskItem(_tTaskItem::ExecuteScript(0.2f,scriptname,scriptparams));
			}
			else
				_log.Log(LOG_ERROR,"evohome: Error script not found '%s'",scriptname.c_str());
		}
	}
}

