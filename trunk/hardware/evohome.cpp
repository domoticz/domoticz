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
#include "evohome.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

#include <string>

#include <algorithm>
#include <iostream>
#include <vector>
#include <boost/bind.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <ctime>

#define RETRY_DELAY 30

extern std::string szUserDataFolder;

std::ofstream *CEvohome::m_pEvoLog=NULL;
#ifdef _DEBUG
bool CEvohome::m_bDebug=true;
#else
bool CEvohome::m_bDebug=false;
#endif

const char CEvohome::m_szControllerMode[7][20]={"Normal","Economy","Away","Day Off","Custom","Heating Off","Unknown"};
const char CEvohome::m_szWebAPIMode[7][20]={"Auto","AutoWithEco","Away","DayOff","Custom","HeatingOff","Unknown"};
const char  CEvohome::m_szNameErr[18]={0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F};
const char CEvohome::m_szZoneMode[7][20]={"Auto","PermanentOverride","TemporaryOverride","OpenWindow","LocalOverride","RemoteOverride","Unknown"};
const int CEvohome::m_evoToDczControllerMode[8]={0,5,1,2,3,-1,-1,4};//are the hidden modes actually valid?
const int  CEvohome::m_evoToDczOverrideMode[5]={zmAuto,-1,zmPerm,-1,zmTmp};//are the hidden modes actually valid?
const uint8_t CEvohome::m_dczToEvoZoneMode[3]={0,2,4};
const uint8_t CEvohome::m_dczToEvoControllerMode[6]={0,2,3,4,7,1};

char const CEvohomeMsg::szPacketType[5][8]={"Unknown","I","RQ","RP","W"};

const char* CEvohome::GetControllerModeName(uint8_t nControllerMode)
{
	return m_szControllerMode[(std::min)(nControllerMode,(uint8_t)6)]; //parentheses around function name apparently avoids macro expansion otherwise windef.h macros will conflict here
}

const char* CEvohome::GetWebAPIModeName(uint8_t nControllerMode)
{
	return m_szWebAPIMode[(std::min)(nControllerMode,(uint8_t)6)]; //parentheses around function name apparently avoids macro expansion windef.h macros will conflict here
}

const char* CEvohome::GetZoneModeName(uint8_t nZoneMode)
{
	return m_szZoneMode[(std::min)(nZoneMode, (uint8_t)6)]; //parentheses around function name apparently avoids macro expansion windef.h macros will conflict here
}

CEvohome::CEvohome(const int ID, const std::string &szSerialPort) :
	m_ZoneNames(m_nMaxZones),
	m_ZoneOverrideLocal(m_nMaxZones)
{
	m_HwdID=ID;
	m_nDevID=0;
	m_nMyID=0;
	m_nBindID=0;
	for(int i=0;i<2;i++)
		m_bStartup[i]=true;

	m_stoprequested=false;
	m_nBufPtr=0;
	m_nSendFail=0;
	m_nZoneCount=0;
	m_nControllerMode=0;
	
	m_iBaudRate=115200;
	if(!szSerialPort.empty())
	{
		m_szSerialPort=szSerialPort;
		m_bScriptOnly=false;
	}
	else
	{
		m_bScriptOnly=true;
		m_bSkipReceiveCheck = true;
	}
	
	RegisterDecoder(cmdZoneTemp,boost::bind(&CEvohome::DecodeZoneTemp,this, _1));
	RegisterDecoder(cmdSetPoint,boost::bind(&CEvohome::DecodeSetpoint,this, _1));
	RegisterDecoder(cmdSetpointOverride,boost::bind(&CEvohome::DecodeSetpointOverride,this, _1));
	RegisterDecoder(cmdDHWState,boost::bind(&CEvohome::DecodeDHWState,this, _1));
	RegisterDecoder(cmdDHWTemp,boost::bind(&CEvohome::DecodeDHWTemp,this, _1));
	RegisterDecoder(cmdControllerMode,boost::bind(&CEvohome::DecodeControllerMode,this, _1));
	RegisterDecoder(cmdSysInfo,boost::bind(&CEvohome::DecodeSysInfo,this, _1));
	RegisterDecoder(cmdZoneName,boost::bind(&CEvohome::DecodeZoneName,this, _1));
	RegisterDecoder(cmdZoneHeatDemand,boost::bind(&CEvohome::DecodeHeatDemand,this, _1));
	RegisterDecoder(cmdZoneInfo,boost::bind(&CEvohome::DecodeZoneInfo,this, _1));
	RegisterDecoder(cmdControllerHeatDemand,boost::bind(&CEvohome::DecodeHeatDemand,this, _1));
	RegisterDecoder(cmdBinding,boost::bind(&CEvohome::DecodeBinding,this, _1));
	RegisterDecoder(cmdActuatorState,boost::bind(&CEvohome::DecodeActuatorState,this, _1));
	RegisterDecoder(cmdActuatorCheck,boost::bind(&CEvohome::DecodeActuatorCheck,this, _1));
	RegisterDecoder(cmdZoneWindow,boost::bind(&CEvohome::DecodeZoneWindow,this, _1));
	RegisterDecoder(cmdExternalSensor,boost::bind(&CEvohome::DecodeExternalSensor,this, _1));
	RegisterDecoder(cmdDeviceInfo,boost::bind(&CEvohome::DecodeDeviceInfo,this, _1));
	RegisterDecoder(cmdBatteryInfo,boost::bind(&CEvohome::DecodeBatteryInfo,this, _1));
}

CEvohome::~CEvohome(void)
{
	m_bIsStarted=false;
}

void CEvohome::RegisterDecoder(unsigned int cmd, fnc_evohome_decode fndecoder)
{
	m_Decoders.insert( std::pair<int, fnc_evohome_decode >( cmd, fndecoder ) );
}

void CEvohome::Init()
{

}

bool CEvohome::StartHardware()
{
	if(m_bScriptOnly)
	{
		Init();
		m_bIsStarted=true;
		sOnConnected(this);
		return true;
	}
	else
	{
		if (m_bDebug && !m_pEvoLog)
		{
			try
			{
				std::string debug_file = szUserDataFolder + "evoraw.log";
				m_pEvoLog = new std::ofstream(debug_file.c_str(), std::ios::out | std::ios::app);
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "EvoHome: Could not open debug file!");
			}
		}
		m_retrycntr=RETRY_DELAY; //will force reconnect first thing
		
		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		szQuery << "SELECT Name,DeviceID,nValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==0)";
		result = m_sql.query(szQuery.str()); //-V519
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];
			std::stringstream s_strid;
			s_strid << std::hex << sd[1];
			s_strid >> m_nDevID;
			m_nControllerMode=atoi(sd[2].c_str());
		}
		
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT  Unit,Name,DeviceID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Type==" << (int)pTypeEvohomeRelay << ") AND (Unit>=64) AND (Unit<96)";//we'll put our custom relays in this range
		result = m_sql.query(szQuery.str());
		m_RelayCheck.clear();
		for (int i = 0; i<static_cast<int>(result.size()); i++)
		{
			Log(true,LOG_STATUS,"evohome: Relay: devno=%d demmand=%d",atoi(result[i][0].c_str()),atoi(result[i][4].c_str()));
			m_RelayCheck.insert( tmap_relay_check_pair( static_cast<uint8_t>(atoi(result[i][0].c_str())),_tRelayCheck(boost::get_system_time()-boost::posix_time::minutes(19),static_cast<uint8_t>(atoi(result[i][4].c_str()))) ) ); //allow 1 minute for startup before trying to restore demand
		}
		
		//Start worker thread
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEvohome::Do_Work, this)));

		return (m_thread!=NULL);
	}
}

bool CEvohome::StopHardware()
{
	if(m_bScriptOnly)
	{
		m_bIsStarted=false;
		return true;
	}
	else
	{
		m_stoprequested=true;
		m_thread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
		if (isOpen())
		{
			try {
				clearReadCallback();
				close();
				doClose();
				setErrorStatus(true);
			} catch(...)
			{
				//Don't throw from a Stop command
			}
		}
		m_bIsStarted=false;
		if(m_bDebug && m_pEvoLog)
		{
			delete m_pEvoLog;
			m_pEvoLog=NULL;
		}
		return true;
	}
}

bool CEvohome::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort,m_iBaudRate);
		_log.Log(LOG_STATUS,"evohome: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"evohome: Error opening serial port: %s", m_szSerialPort.c_str());
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"evohome: Error opening serial port!!!");
		return false;
	}
	m_nBufPtr=0;
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CEvohome::ReadCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}
	
void CEvohome::Do_Work()
{
	boost::system_time stLastRelayCheck(boost::posix_time::min_date_time);
	int nStartup=0;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (m_stoprequested)
			break;

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"evohome: serial setup retry in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
				if (isOpen())//do some startup stuff
				{
					if(GetControllerID()!=0)//can't proceed without it
						RequestCurrentState();
				}
			}
		}
		else
		{
			Do_Send();//FIXME afaik if there is a collision then we should increase the back off period exponentially if it happens again
			if(nStartup<300)//if we haven't got zone names in 300s we auto init them (zone creation blocked until we have something)...
			{
				nStartup++;
				if(nStartup==300)
				{
					InitControllerName();
					InitZoneNames();
				}
			}
			boost::lock_guard<boost::mutex> l(m_mtxRelayCheck);
			if(!m_RelayCheck.empty() && GetGatewayID()!=0)
			{
				CheckRelayHeatDemand();
				if((boost::get_system_time()-stLastRelayCheck)>boost::posix_time::seconds(604)) //not sure if it makes a difference but avg time is about 604-605 seconds but not clear how reference point derived - seems steady once started
				{
					SendRelayKeepAlive();
					stLastRelayCheck=boost::get_system_time();
				}
			}
		}
	}
	_log.Log(LOG_STATUS,"evohome: Serial Worker stopped...");
} 

std::string CEvohomeMsg::Encode()
{
	std::string szRet;
	char szTmp[1024];	
	sprintf(szTmp,"%s - 18:730 %s %s %04X %03d ",szPacketType[type],GetStrID(1).c_str(),GetStrID(2).c_str(),command,payloadsize);
	szRet=szTmp;
	for(int i=0;i<payloadsize;i++)
	{
		sprintf(szTmp,"%02hhX",payload[i]);
		szRet+=szTmp;
	}
	enccount++;
	return szRet;
}

bool CEvohome::WriteToHardware(const char *pdata, const unsigned char length)
{
	if(!pdata)
		return false;
	REVOBUF *tsen=(REVOBUF*)pdata;
	switch (pdata[1])
	{
		case pTypeEvohome:
			if (length<sizeof(REVOBUF::_tEVOHOME1))
				return false;
			if(!m_bScriptOnly)//This is a switch so the on action script will be run anyway
				AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktwrt,GetControllerID(),cmdControllerMode).Add(ConvertMode(m_dczToEvoControllerMode,tsen->EVOHOME1.status)).Add((tsen->EVOHOME1.mode==1)?CEvohomeDateTime(tsen->EVOHOME1):CEvohomeDateTime()).Add(tsen->EVOHOME1.mode));
			break;
		case pTypeEvohomeZone:
			if (length<sizeof(REVOBUF::_tEVOHOME2))
				return false;
			if(!m_bScriptOnly)
				AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktwrt,GetControllerID(),cmdSetpointOverride).Add((uint8_t)(tsen->EVOHOME2.zone-1)).Add(tsen->EVOHOME2.temperature).Add(ConvertMode(m_dczToEvoZoneMode,tsen->EVOHOME2.mode)).Add((uint16_t)0xFFFF).Add((uint8_t)0xFF).Add_if(CEvohomeDateTime(tsen->EVOHOME2),(tsen->EVOHOME2.mode==2)));
			else
				RunScript(pdata,length);
			break;
		case pTypeEvohomeWater:
			if (length<sizeof(REVOBUF::_tEVOHOME2))
				return false;
			if(!m_bScriptOnly)
				AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktwrt,GetControllerID(),cmdDHWState).Add((uint8_t)(tsen->EVOHOME2.zone-1)).Add((uint8_t)tsen->EVOHOME2.temperature).Add(ConvertMode(m_dczToEvoZoneMode,tsen->EVOHOME2.mode)).Add((uint16_t)0).Add((uint8_t)0).Add_if(CEvohomeDateTime(tsen->EVOHOME2),(tsen->EVOHOME2.mode==2)));
			else
				RunScript(pdata,length);
			break;
		case pTypeEvohomeRelay:
			if (length<sizeof(REVOBUF::_tEVOHOME3))
				return false;
			if(!m_bScriptOnly)//Only supported by HGI80
				SetRelayHeatDemand(tsen->EVOHOME3.devno,tsen->EVOHOME3.demand);
			break;
	}
	return true;
}

void CEvohome::RunScript(const char *pdata, const unsigned char length)
{
	if(!pdata)
		return;
	REVOBUF *tsen=(REVOBUF*)pdata;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT  HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1 FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << (int)tsen->EVOHOME2.zone << ") AND (Type==" << (int)tsen->EVOHOME2.type << ")";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size()>0)
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
			s_strid << (int)tsen->EVOHOME2.zone;
			boost::replace_all(OnAction, "{unit}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << (int)tsen->EVOHOME2.mode;
			boost::replace_all(OnAction, "{mode}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << tsen->EVOHOME2.temperature / 100.0f;
			boost::replace_all(OnAction, "{setpoint}", s_strid.str());
			s_strid.clear();
			s_strid.str("");
			s_strid << (int)tsen->EVOHOME2.temperature;
			boost::replace_all(OnAction, "{state}", s_strid.str());
			boost::replace_all(OnAction, "{until}", CEvohomeDateTime::GetISODate(tsen->EVOHOME2));
			//Execute possible script
			std::string scriptname;
			if (OnAction.find("script:///") != std::string::npos)
				scriptname = OnAction.substr(9);
			else
				scriptname = OnAction.substr(8);
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
				m_sql.AddTaskItem(_tTaskItem::ExecuteScript(1,scriptname,scriptparams));
			}
			else
				_log.Log(LOG_ERROR,"evohome: Error script not found '%s'",scriptname.c_str());
		}
	}
}

void CEvohome::RequestZoneTemp(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdZoneTemp).Add(nZone));
}

void CEvohome::RequestZoneName(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdZoneName).Add(nZone).Add(uint8_t(0)));
}

void CEvohome::RequestZoneInfo(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdZoneInfo).Add(nZone));
}

void CEvohome::RequestSetPointOverride(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdSetpointOverride).Add(nZone));
}

void CEvohome::RequestDHWState()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdDHWState).Add(uint8_t(0)));
}

void CEvohome::RequestDHWTemp()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdDHWTemp).Add(uint8_t(0)));
}

void CEvohome::RequestControllerMode()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdControllerMode).Add(uint8_t(0xFF)));
}

void CEvohome::RequestSysInfo()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdSysInfo).Add(uint8_t(0)));
}

void CEvohome::RequestZoneStartupInfo(uint8_t nZone)
{
	RequestZoneTemp(nZone);
	RequestSetPointOverride(nZone);
	RequestZoneInfo(nZone);
	RequestZoneName(nZone);
}

void CEvohome::RequestCurrentState()
{
	RequestSysInfo();
	RequestControllerMode();
	RequestDHWTemp();
	RequestDHWState();	
	RequestZoneStartupInfo(0);
	RequestDeviceInfo(0);
}

void CEvohome::RequestZoneState()
{
	uint8_t nZoneCount=GetZoneCount();
	for(uint8_t i=0;i<nZoneCount;i++)
		RequestSetPointOverride(i);
	//Trying this linked to DHW heat demand instead...that won't be adequate do it here too!
	RequestDHWState();
	SendExternalSensor();
}

void CEvohome::RequestZoneNames()
{
	uint8_t nZoneCount=GetZoneCount();
	for(uint8_t i=0;i<nZoneCount;i++)
		RequestZoneName(i);
}

void CEvohome::RequestDeviceInfo(uint8_t nAddr)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdDeviceInfo).Add(uint8_t(0)).Add(uint8_t(0)).Add(nAddr));
}

void CEvohome::SendRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,GetGatewayID(),cmdControllerHeatDemand).Add(nDevNo).Add(nDemand));
}

void CEvohome::UpdateRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	SendRelayHeatDemand(nDevNo,nDemand);
	m_RelayCheck[nDevNo]=_tRelayCheck(boost::get_system_time(),nDemand);
}

void CEvohome::SetRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	if(nDevNo<64 || nDevNo>=96 || GetGatewayID()==0) //atm no reason to interfere with non custom relays
		return;
	boost::lock_guard<boost::mutex> l(m_mtxRelayCheck);
	UpdateRelayHeatDemand(nDevNo,nDemand);
}

void CEvohome::CheckRelayHeatDemand()
{
	for(tmap_relay_check_it it = m_RelayCheck.begin(); it != m_RelayCheck.end(); it++)
	{
		if((boost::get_system_time()-it->second.m_stLastCheck)>boost::posix_time::seconds(1202)) //avg seems around 1202-1203 but not clear how reference point derived
		{
			Log(true,LOG_STATUS,"evohome: Relay: Refreshing heat demand devno=%d demand=%d",it->first,it->second.m_nDemand);
			UpdateRelayHeatDemand(it->first,it->second.m_nDemand);
		}
	}
}

void CEvohome::SendRelayKeepAlive()
{
	Log(true,LOG_STATUS,"evohome: Relay: Sending keep alive");
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,GetGatewayID(),cmdActuatorCheck).Add((uint8_t)0xFC).Add((uint8_t)0xC8));
}

void CEvohome::SendExternalSensor()
{
	if(GetGatewayID()==0)
		return;
	double dbTemp=0.0,dbUV=0.0;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT sValue FROM DeviceStatus WHERE (Name=='Outside')";//There could be different types depending on how data is received from WU etc.
	result = m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::string> strarray;
		StringSplit(result[0][0], ";", strarray);
		if (strarray.size() >= 1)
			dbTemp=atof(strarray[0].c_str());
		else
			return;
	}
	else
		return;
	
	//FIXME no light level data available UV from WU is only thing vaguely close (on dev system) without a real sensor 
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT sValue FROM DeviceStatus WHERE (Type==" << (int)pTypeUV << ")";
	result = m_sql.query(szQuery.str());
	if (result.size()>0)
		dbUV=atof(result[0][0].c_str());
	else
		return;
	
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,GetGatewayID(),cmdExternalSensor).Add((uint8_t)0).Add(static_cast<uint16_t>(dbUV*39)).Add((uint8_t)2).Add((uint8_t)2).Add(static_cast<int16_t>(dbTemp*100.0)).Add((uint8_t)1));
}

void CEvohome::ReadCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		//_log.Log(LOG_NORM,"evohome: received %ld bytes",len);
		if (!HandleLoopData(data,len))
		{
			//error in data, try again...
			try {
				clearReadCallback();
				close();
				doClose();
				setErrorStatus(true);
			} catch(...)
			{
				//Don't throw from a Stop command
			}
		}
	}
	catch (...)
	{

	}
}

bool CEvohome::HandleLoopData(const char *data, size_t len)
{
	if(m_nBufPtr+len>=m_nBufSize)
	{
		Log(false,LOG_ERROR,"evohome: Buffer overflow");
		m_nBufPtr=0;
		return false;
	}
	memcpy(m_buf + m_nBufPtr, data, len);
	m_nBufPtr += len;
	m_nBufPtr = ProcessBuf(m_buf, m_nBufPtr);
	return true;
}

int CEvohome::ProcessBuf(char * buf, int size)
{
	int start = 0;

	for (int i = 0; i < size; ++i) 
	{
		if(buf[i]==0x11)//this appears to be a break character?
			start=i+1;
		else if(buf[i]==0x0A)//this is the end of packet marker...not sure if there is a CR before this?
		{
			if (i - start >= 2048) {
				Log(false,LOG_ERROR,"evohome: Message length exceeds max message size");
				start = i + 1;
				continue;
			}
			if(i!=start) //skip empty messages
			{
				std::string msg = std::string(buf + start, i - start -1);
				ProcessMsg(msg.c_str());
			}
			start = i + 1;
		}
	}
	if (start > 0) 
		memmove(buf,buf+start,size-start);
	return size - start;
}

bool CEvohomeMsg::DecodePacket(const char * rawmsg)
{
	int cmdidx=0;
	int nid=0;
	std::string line(rawmsg);
	std::vector<std::string> tkns;
	boost::split(tkns,line,boost::is_any_of(" "),boost::token_compress_on);//There are sometimes 2 spaces between token 1 and 2 so we use token_compress_on to avoid an additional empty token
	for (size_t i = 0; i < tkns.size(); i++)
	{
		std::string tkn(tkns[i]);
		if(i==0)//this is some sort of status or flags but not sure what exactly
		{
		}
		else if(i==1)
		{
			if(SetPacketType(tkn.c_str())!=pktunk)
				SetFlag(flgpkt);
		}
		else if(cmdidx && i==cmdidx+2) //payload starts 2 after command idx
		{
			if(!GetFlag(flgps))
			{
				CEvohome::Log(false,LOG_ERROR,"evohome: no payload size - possible corrupt message");
				return false;
			}
			if(tkn.length()%2)
			{
				CEvohome::Log(false,LOG_ERROR,"evohome: uneven payload - possible corrupt message");
				return false;
			}
			if(tkn.length()/2>m_nBufSize)
			{
				CEvohome::Log(false,LOG_ERROR,"evohome: payload exceeds max buffer size");
				return false;
			}
			int ps=0;
			for(size_t j=0;j<tkn.length();j+=2)
			{
				unsigned int nHex;
				sscanf(tkn.substr(j,2).c_str(),"%02x",&nHex);
				payload[ps++]=static_cast<uint8_t>(nHex);
			}
			if(ps==payloadsize)
			{
				SetFlag(flgpay);
			}
			else
			{
				CEvohome::Log(false,LOG_ERROR,"evohome: payload size does not match specified size from packet header");
				return false;
			}
			
		}
		else if(tkn.length()==4)
		{
			sscanf(tkn.c_str(),"%04x",&command);
			SetFlag(flgcmd);
			if(!cmdidx)
				cmdidx=i;
		}
		else if(tkn.length()==3)
		{
			if(tkn.find('-')==std::string::npos)
			{
				if(i==2)
				{
					timestamp=atoi(tkn.c_str());
					SetFlag(flgts);
				}
				else if(cmdidx && i==cmdidx+1)
				{
					payloadsize=atoi(tkn.c_str());
					SetFlag(flgps);
				}
			}
		}
		else
		{
			int nPos=tkn.find(':');
			if(nPos!=std::string::npos)
			{
				if(nid>=3)
				{
					CEvohome::Log(false,LOG_ERROR,"evohome: too many message ids - possible corrupt message");
					continue;
				}
				if(tkn.find('-')==std::string::npos)
				{
					id[nid]=tkn;
					SetFlag(flgid1<<nid);
				}
				nid++;
			}
			else
				CEvohome::Log(false,LOG_STATUS,"evohome: WARNING unrecognised message structure - possible corrupt message '%s' (%d)",tkn.c_str(),i);
		}
	}
	return IsValid();
}

void CEvohome::ProcessMsg(const char * rawmsg)
{
	CEvohomeMsg msg(rawmsg);
	if(msg.IsValid())
	{
		Log(rawmsg,msg);
		if(!GetControllerID())//no controller id ..just use the 1st one we find
		{
			for(int n=0;n<3;n++)
			{
				if(msg.id[n].GetIDType()==CEvohomeID::devController)//id type 1 is for controllers 
				{
					SetControllerID(msg.GetID(n));
					RequestCurrentState(); //and also get startup info as this should only happen during initial setup
					break;
				}
			}
		}
		if(msg.id[0].GetIDType()==CEvohomeID::devGateway) //if we just got an echo of a sent packet we don't need to process it
		{
			if(!m_nMyID)//If we haven't got our own id trap it here just in case we need it later on
				m_nMyID=msg.GetID(0);
			PopSendQueue(msg);
		}
		else
			DecodePayload(msg);
	}
	else
		Log(true,LOG_ERROR,"evohome: invalid message structure - possible corrupt message");
}

bool CEvohome::DecodePayload(CEvohomeMsg &msg)
{
	std::map < unsigned int, fnc_evohome_decode >::iterator pf = m_Decoders.find( msg.command );
	if (pf!=m_Decoders.end())
	{
		bool ret=pf->second(msg);
		if(!ret)
			Log(false,LOG_ERROR,"evohome: unable to decode payload for command %04x",msg.command);
		return ret;
	}
	Log(true,LOG_ERROR,"evohome: unknown command %04x",msg.command);
	return false;
}

bool CEvohome::DumpMessage(CEvohomeMsg &msg)
{
	char tag[] = "DUMP_MSG";
	Log(true,LOG_STATUS,"evohome: %s: CMD=%04x Len=%d",tag, msg.command,msg.payloadsize);
	std::string strpayload,strascii;
	char szTmp[1024];
	for(int i=0;i<msg.payloadsize;i++)
	{
		sprintf(szTmp,"%02hhx",msg.payload[i]);
		strpayload+=szTmp;
		sprintf(szTmp,"%c",msg.payload[i]);
		strascii+=szTmp;
	}
	Log(true,LOG_STATUS,"evohome: %s: payload=%s (ASCII)=%s",tag, strpayload.c_str(), strascii.c_str());
	return true;
}

int CEvohome::Bind(uint8_t nDevNo, unsigned char nDevType)//use CEvohomeID::devType atm but there could be additional specialisations
{
	int nGatewayID=GetGatewayID();
	if(nGatewayID==0)
		return 0;
		
	if(nDevType==CEvohomeID::devRelay)//Binding a relay to the HGI80
	{
		boost::unique_lock<boost::mutex> lock(m_mtxBindNotify);
		m_nBindID=0;
		m_nBindIDType=nDevType;
		/*if(!m_cndBindNotify.wait_for(lock,boost::posix_time::seconds(60)))
			return 0;*/
		boost::system_time const timeout=boost::get_system_time() + boost::posix_time::seconds(60);//monotonic?
		while(m_nBindID==0)
		{
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,nGatewayID,cmdBinding).Add(nDevNo).Add((uint16_t)cmdControllerHeatDemand).Add(CEvohomeID(nGatewayID)).Add((uint8_t)0xFC).Add((uint16_t)cmdActuatorCheck).Add(CEvohomeID(nGatewayID)).Add((uint8_t)nDevNo).Add((uint16_t)cmdBinding).Add(CEvohomeID(nGatewayID)));
			boost::system_time const wait=boost::get_system_time() + boost::posix_time::seconds(5);//monotonic?
			if(!m_cndBindNotify.timed_wait(lock,wait) && boost::get_system_time()>timeout)
				return 0;
		}
		
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,m_nBindID,cmdBinding).Add((uint8_t)0).Add((uint16_t)0xFFFF).Add(CEvohomeID(nGatewayID)));
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,nGatewayID,0x0009).Add(nDevNo).Add((uint8_t)0).Add((uint8_t)0xFF)); //not sure if 1st byte is devno 2nd byte can be 00 or 01
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,nGatewayID,0x1100).Add((uint8_t)0xFC).Add((uint16_t)0x1804).Add((uint16_t)0).Add((uint16_t)0x7FFF).Add((uint8_t)0x01)); //probably relay settings.. usually sent after cmd 0x0009
		
		return m_nBindID;
	}
	else if(nDevType==CEvohomeID::devSensor)//Binding the HGI80 to the evohome controller as an outdoor sensor (there are other sensors)
	{
		boost::unique_lock<boost::mutex> lock(m_mtxBindNotify);
		m_nBindID=0;
		m_nBindIDType=CEvohomeID::devController;
		
		boost::system_time const timeout=boost::get_system_time() + boost::posix_time::seconds(60);//monotonic?
		while(m_nBindID==0)
		{
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,nGatewayID,cmdBinding).Add((uint8_t)0).Add((uint16_t)cmdExternalSensor).Add(CEvohomeID(nGatewayID)).Add((uint8_t)2).Add((uint16_t)cmdExternalSensor).Add(CEvohomeID(nGatewayID)).Add((uint8_t)0).Add((uint16_t)cmdBinding).Add(CEvohomeID(nGatewayID)));
			boost::system_time const wait=boost::get_system_time() + boost::posix_time::seconds(5);//monotonic?
			if(!m_cndBindNotify.timed_wait(lock,wait) && boost::get_system_time()>timeout)
				return 0;
		}
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,m_nBindID,cmdBinding).Add((uint8_t)0).Add((uint16_t)0xFFFF).Add(CEvohomeID(nGatewayID)));
		return m_nBindID;
	}
	
	return 0;
}

bool CEvohome::DecodeSetpoint(CEvohomeMsg &msg)//0x2309
{	
	char tag[] = "ZONE_SETPOINT";
	
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for zone %d",tag, msg.payload[0]);
		return true;
	}	
	if (msg.payloadsize % 3 != 0) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding zone setpoint payload, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	tsen.EVOHOME2.type=pTypeEvohomeZone;
	tsen.EVOHOME2.subtype=sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3) //this message can be received from other than the controller...is zone always valid though? (when a local override is in action we get different values one from the controller the other from the zone valve but the zone number is still ok)
	tsen.EVOHOME2.mode=zmNotSp;
	tsen.EVOHOME2.updatetype=updSetPoint;//setpoint
	
	for (int i = 0 ; i < msg.payloadsize ; i += 3) {
		tsen.EVOHOME2.zone = msg.payload[i]+1;
		tsen.EVOHOME2.temperature =  msg.payload[i + 1] << 8 | msg.payload[i + 2];
		if(tsen.EVOHOME2.temperature==0x7FFF)
		{
			Log(true,LOG_STATUS,"evohome: %s: Warning setpoint not set for zone %d",tag, msg.payload[0]);
			continue;
		}
		SetMaxZoneCount(tsen.EVOHOME2.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d: %d", tag, tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature);
		//It appears that the controller transmits the current setpoint for all zones periodically this is presumably so 
		//the zone controller can update to any changes as required
		//The zone controllers also individually transmit their own setpoint as it is currently set
		//presumably this will ordinarily update to correspond to the transmitted value from the controller
		//The exception appears to be for local overrides which may be possible to track by seeing if a change
		//occurs that does not correspond to the controller setpoint for a given zone
		if (msg.GetID(0) == GetControllerID())
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
	}
	
	return true;
}

bool CEvohome::DecodeSetpointOverride(CEvohomeMsg &msg)//0x2349
{
	char tag[] = "ZONE_SETPOINT_MODE";

	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for zone %d",tag, msg.payload[0]);
		return true;
	}	
	//reply is 7 bytes or 13 bytes with a date?
	if (msg.payloadsize != 7 && msg.payloadsize != 13) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding payload unknown size: %d", tag, msg.payloadsize);
		return false;
	}
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	tsen.EVOHOME2.type=pTypeEvohomeZone;
	tsen.EVOHOME2.subtype=sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3) //will be id of controller so must use zone number
	
	tsen.EVOHOME2.zone = msg.payload[0]+1;//controller is 0 so let our zones start from 1...
	if(tsen.EVOHOME2.zone>m_nMaxZones)
	{
		Log(false,LOG_ERROR,"evohome: %s: Error zone number out of bounds: %d", tag, tsen.EVOHOME2.zone);
		return false;
	}
	tsen.EVOHOME2.updatetype = updSetPoint;//setpoint
	tsen.EVOHOME2.temperature = msg.payload[1] << 8 | msg.payload[2];
	if(tsen.EVOHOME2.temperature==0x7FFF)
	{
		Log(true,LOG_STATUS,"evohome: %s: Warning setpoint not set for zone %d",tag, tsen.EVOHOME2.zone);
		return true;
	}
	SetMaxZoneCount(tsen.EVOHOME2.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
	if(m_ZoneOverrideLocal[tsen.EVOHOME2.zone-1]==zmWind || m_ZoneOverrideLocal[tsen.EVOHOME2.zone-1]==zmLocal)
	{
		Log(true,LOG_STATUS,"evohome: %s: A local override is in effect for zone %d",tag,tsen.EVOHOME2.zone);
		return true;
	}
	int nMode=ConvertMode(m_evoToDczOverrideMode,msg.payload[3]);
	if(nMode==-1)
	{
		Log(false,LOG_STATUS,"evohome: %s: WARNING unexpected mode %d",tag,msg.payload[3]);
		return false;
	}
	tsen.EVOHOME2.mode=static_cast<uint8_t>(nMode);
	tsen.EVOHOME2.controllermode=ConvertMode(m_evoToDczControllerMode,GetControllerMode());
	if(msg.payloadsize == 13)
	{	
		CEvohomeDateTime::DecodeDateTime(tsen.EVOHOME2,msg.payload,7);
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d: %d (%d=%s) %s", tag, tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature, tsen.EVOHOME2.mode, GetZoneModeName(tsen.EVOHOME2.mode),CEvohomeDateTime::GetStrDate(tsen.EVOHOME2).c_str());
	}
	else
	{
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d: %d (%d=%s)", tag, tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature, tsen.EVOHOME2.mode, GetZoneModeName(tsen.EVOHOME2.mode));
	}
	
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
	return true;
}

bool CEvohome::DecodeZoneTemp(CEvohomeMsg &msg)//0x30C9
{
	char tag[] = "ZONE_TEMP";

	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for zone temp %d",tag, msg.payload[0]);
		return true;
	}
	//blocks of 3 bytes
	if (msg.payloadsize < 3) {//check below does not trap 0 byte payload
		Log(false,LOG_ERROR,"evohome: %s: Error decoding zone temperature payload, size too small: %d", tag, msg.payloadsize);
		return false;	
	}
	if (msg.payloadsize % 3 != 0) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding zone temperature payload, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	
	bool bRefresh=false;
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	tsen.EVOHOME2.type=pTypeEvohomeZone;
	tsen.EVOHOME2.subtype=sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3);
	for (int i = 0 ; i < msg.payloadsize ; i += 3) {
		//if this is broadcast direct from the sensor then the zoneID is always 0 and we need to match on the device id if possible (use zone 0 to indicate this)
		//otherwise use the zone number...controller is 0 so let our zones start from 1...
		if (msg.GetID(0) == GetControllerID())
			tsen.EVOHOME2.zone = msg.payload[i]+1;
		else
			tsen.EVOHOME2.zone = 0;
		tsen.EVOHOME2.temperature = msg.payload[i + 1] << 8 | msg.payload[i + 2];
		//this is sent for zones that use a zone temperature instead of the internal sensor.
		Log(true,LOG_STATUS,"evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature);
		if(tsen.EVOHOME2.temperature!=0x7FFF)//afaik this is the error value just ignore it right now as we have no way to report errors...also perhaps could be returned if DHW is not installed?
		{
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
			if (msg.GetID(0) == GetControllerID())
				bRefresh=SetMaxZoneCount(tsen.EVOHOME2.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
		}
	}

	if (msg.GetID(0) == GetControllerID()) {
		//msg from main unit..will be id of controller so must use zone number
		//DHW broadcasts on change?...I think DHW temp broadcasts when it changes but not the DHW state unless it's been manually changed on the controller
		if(msg.type==CEvohomeMsg::pktinf)
		{
			if(bRefresh) //this should only change when we're running normally to allow detection of new zones
				RequestZoneNames();//This can conflict with our startup polling but will still succeed ok
			RequestZoneState();//This can conflict with our startup polling but will still succeed ok
		}
	}
	else {//no zone number here so we'd need to do a lookup based on the ID to use this
		//msg from sensor itself
		if (msg.payloadsize != 3) {
			Log(true,LOG_STATUS,"evohome: %s: WARNING: got a sensor temperature msg with an unexpected payload size: %d", tag, msg.payloadsize);
		}
		//in this case the zoneID is always 0 and hence is worthless
		if (msg.payload[0] != 0) {
			Log(true,LOG_STATUS,"evohome: %s: WARNING: sensor reading with zone != 0: 0x%x - %d", tag, msg.GetID(0), msg.payload[0]);
		}
	}

	return true;
}

bool CEvohome::DecodeDHWState(CEvohomeMsg &msg)//1F41
{
	char tag[] = "DHW_STATE";
	
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for DHW state %d",tag, msg.payload[0]);
		return true;	
	}	
	if (msg.payloadsize != 6 && msg.payloadsize != 12) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding DHW state / mode, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	tsen.EVOHOME2.type=pTypeEvohomeWater;
	tsen.EVOHOME2.subtype=sTypeEvohomeWater;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3) //will be id of controller so must use zone number
	
	tsen.EVOHOME2.zone = msg.payload[0]+1;////NA to DHW...controller is 0 so let our zones start from 1...
	tsen.EVOHOME2.updatetype = updSetPoint;//state
	tsen.EVOHOME2.temperature = msg.payload[1];//just on or off for DHW
	if(tsen.EVOHOME2.temperature == 0x7F)//FIXME this is just a guess?
		return false;
	int nMode=ConvertMode(m_evoToDczOverrideMode,msg.payload[2]);
	if(nMode==-1)
	{
		Log(false,LOG_STATUS,"evohome: %s: WARNING unexpected mode %d",tag, msg.payload[2]);
		return false;
	}
	tsen.EVOHOME2.mode=static_cast<uint8_t>(nMode);
	tsen.EVOHOME2.controllermode=ConvertMode(m_evoToDczControllerMode,GetControllerMode());
	if(msg.payloadsize == 12)
	{	
		CEvohomeDateTime::DecodeDateTime(tsen.EVOHOME2,msg.payload,6);
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d: %d (%d=%s) %s", tag, tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature, tsen.EVOHOME2.mode, GetZoneModeName(tsen.EVOHOME2.mode),CEvohomeDateTime::GetStrDate(tsen.EVOHOME2).c_str());
	}
	else
	{
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d: %d (%d=%s)", tag, tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature, tsen.EVOHOME2.mode, GetZoneModeName(tsen.EVOHOME2.mode));
	}
	
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
	return true;
}

bool CEvohome::DecodeDHWTemp(CEvohomeMsg &msg)//1260
{
	char tag[] = "DHW_TEMP";
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for DHW temp %d",tag, msg.payload[0]);
		return true;
	}	
	//blocks of 3 bytes...only ever seen a 3 byte message with DevNo 0x00
	if (msg.payloadsize < 3) {//check below does not trap 0 byte payload
		Log(false,LOG_ERROR,"evohome: %s: Error decoding DHW temperature payload, size too small: %d", tag, msg.payloadsize);
		return false;	
	}
	if (msg.payloadsize % 3 != 0) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding DHW temperature payload, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize != 3) {
		Log(true,LOG_STATUS,"evohome: %s: WARNING: got a sensor temperature msg with an unexpected payload size: %d", tag, msg.payloadsize);
	}
	//for DHW DevNo is always 0 and not relevant
	if (msg.payload[0] != 0) {
		Log(true,LOG_STATUS,"evohome: %s: WARNING: sensor reading with zone != 0: 0x%x - %d", tag, msg.GetID(0), msg.payload[0]);
	}
		
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	tsen.EVOHOME2.type=pTypeEvohomeWater;
	tsen.EVOHOME2.subtype=sTypeEvohomeWater;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3);
	
	for (int i = 0 ; i < msg.payloadsize ; i += 3) {
		tsen.EVOHOME2.zone = msg.payload[i]+1;//we're using zone 0 to trigger a lookup on ID rather than zone number (not relevant for DHW)
		tsen.EVOHOME2.temperature = msg.payload[i + 1] << 8 | msg.payload[i + 2];
		Log(true,LOG_STATUS,"evohome: %s: DHW sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature);
		if(tsen.EVOHOME2.temperature!=0x7FFF)//afaik this is the error value just ignore it right now as we have no way to report errors...also perhaps could be returned if DHW is not installed?
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
	}

	return true;
}

bool CEvohome::DecodeControllerMode(CEvohomeMsg &msg)//2E04
{
	char tag[] = "CONTROLLER_MODE";
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for controller mode %d",tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 8) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding controller mode, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME1.len=sizeof(tsen.EVOHOME1)-1;
	tsen.EVOHOME1.type=pTypeEvohome;
	tsen.EVOHOME1.subtype=sTypeEvohome;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME1.id1,tsen.EVOHOME1.id2,tsen.EVOHOME1.id3);
	
	int nControllerMode=msg.payload[0];
	int nMode=ConvertMode(m_evoToDczControllerMode,nControllerMode);//this converts to the modes originally setup with the web client ver
	if(nMode==-1)
	{
		Log(false,LOG_STATUS,"evohome: %s: WARNING unexpected mode %d",tag, nControllerMode);
		return false;
	}
	tsen.EVOHOME1.status=static_cast<uint8_t>(nMode);
	CEvohomeDateTime::DecodeDateTime(tsen.EVOHOME1,msg.payload,1);
	tsen.EVOHOME1.mode=msg.payload[7];//1 is tmp 0 is perm
	Log(true,LOG_STATUS,"evohome: %s: Setting: (%d=%s) (%d=%s) %s", tag, tsen.EVOHOME1.status, GetControllerModeName(tsen.EVOHOME1.status),tsen.EVOHOME1.mode,tsen.EVOHOME1.mode?"Temporary":"Permanent",CEvohomeDateTime::GetStrDate(tsen.EVOHOME1).c_str());
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME1);//Decode message
	
	if(SetControllerMode(nControllerMode))//if only the until time changed we should be ok as the unit will broadcast a new controller mode when the current mode ends
		RequestZoneState();//This can conflict with our startup polling but will still succeed ok
	return true;
}

bool CEvohome::DecodeSysInfo(CEvohomeMsg &msg)//10e0
{
	char tag[] = "SYSINFO";
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for sysinfo %d",tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 38) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding sysinfo, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	//Not sure what the first 10 bytes are for..some are masked anyway...the first bytes were always 000002FF in the captures I saw
	int nAppVer=msg.payload[5]; // does byte 4 go with the 1st date 
	CEvohomeDate edt,edtp2;
	msg.Get(edt, 10).Get(edtp2);
	msg.payload[38]='\0';//presumably not null terminated if name consumes all available bytes in the payload
	SetControllerName((const char*)&msg.payload[msg.GetPos()]);
	Log(false, LOG_STATUS, "evohome: %s: d1 %s App Ver %d (%s) Name %s", tag, edt.GetStrDate().c_str(), nAppVer, edtp2.GetStrDate().c_str(), &msg.payload[msg.GetPos()]);
	return true;
}

bool CEvohome::DecodeZoneName(CEvohomeMsg &msg)
{
	char tag[] = "ZONE_NAME";
	if (msg.payloadsize == 2){
		Log(true,LOG_STATUS,"evohome: %s: Request for zone name %d",tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 22) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding zone name, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	if(memcmp(&msg.payload[2],m_szNameErr,18)==0)
	{
		Log(true,LOG_STATUS,"evohome: %s: Warning zone name not set: %d", tag, msg.payload[0]);
		m_bStartup[0]=false;
		return true;
	}
	msg.payload[22]='\0';//presumably not null terminated if name consumes all available bytes in the payload
	int nZone=msg.payload[0]+1;
	SetMaxZoneCount(nZone);//this should increase on startup as we poll all zones so we don't respond to changes here
	SetZoneName(msg.payload[0],(const char*)&msg.payload[2]);
	Log(true,LOG_STATUS,"evohome: %s: %d: Name %s",tag,nZone,&msg.payload[2]);
	if(m_bStartup[0] && nZone<m_nMaxZones)
		RequestZoneStartupInfo(nZone);
	return true;
}

bool CEvohome::DecodeZoneInfo(CEvohomeMsg &msg)
{
	char tag[] = "ZONE_INFO";
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for zone info %d",tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize < 6) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding zone info, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 6 != 0) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding zone info, incorrect packet size: %d", tag, msg.payloadsize);
		return false;
	}
	for (int i = 0 ; i < msg.payloadsize ; i += 6) {
		uint8_t nZone,nFlags;
		CEvohomeTemp min,max;
		msg.Get(nZone).Get(nFlags).Get(min).Get(max);
		nZone++;
		if(!min.IsValid() && !max.IsValid())
		{
			Log(true,LOG_STATUS,"evohome: %s: Warning zone info not set %d",tag, nZone);
			continue;
		}
		SetMaxZoneCount(nZone);//this should increase on startup as we poll all zones so we don't respond to changes here
		Log(true,LOG_STATUS,"evohome: %s: %d: Min %.1f Max %.1f Flags %d %s",tag,nZone,min.GetTemp(),max.GetTemp(),nFlags,CEvohomeZoneFlags::GetFlags(nFlags).c_str());
	}
	
	return true;
}

bool CEvohome::DecodeZoneWindow(CEvohomeMsg &msg)
{
	char tag[] = "ZONE_WINDOW";
	if (msg.payloadsize == 1){
		Log(true,LOG_STATUS,"evohome: %s: Request for zone window %d",tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize < 3) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	tsen.EVOHOME2.type=pTypeEvohomeZone;
	tsen.EVOHOME2.subtype=sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3)
	
	uint8_t nWindow,nMisc;
	msg.Get(tsen.EVOHOME2.zone).Get(nWindow).Get(nMisc);//not sure what the last byte is seems to always be 0
	if(tsen.EVOHOME2.zone>=m_nMaxZones)
	{
		Log(false,LOG_ERROR,"evohome: %s: Error zone number out of bounds: %d", tag, tsen.EVOHOME2.zone+1);
		return false;
	}
	tsen.EVOHOME2.mode=(nWindow?zmWind:zmAuto);
	tsen.EVOHOME2.updatetype = updOverride;//zone modde override (window / local)
	m_ZoneOverrideLocal[tsen.EVOHOME2.zone]=static_cast<zoneModeType>(tsen.EVOHOME2.mode);
	
	tsen.EVOHOME2.zone++;
	SetMaxZoneCount(tsen.EVOHOME2.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
	
	if(nWindow!=0 && nWindow!=0xC8)
		Log(true,LOG_STATUS,"evohome: %s: Unexpected zone state Window=%d",tag,nWindow);
	if(nMisc!=0)
		Log(true,LOG_STATUS,"evohome: %s: Unexpected zone state nMisc=%d",tag,nMisc);
	Log(true,LOG_STATUS,"evohome: %s: %d: Window %d",tag,tsen.EVOHOME2.zone,nWindow);
	
	if (msg.GetID(0) == GetControllerID())
		sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
	
	return true;
}

bool CEvohome::DecodeBinding(CEvohomeMsg &msg)
{
	char tag[] = "BINDING";
	if (msg.payloadsize < 6) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding binding, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 6 != 0) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding binding, incorrect packet size: %d", tag, msg.payloadsize);
		return false;
	}
	for (int i = 0 ; i < msg.payloadsize ; i += 6) {
		uint8_t nDevNo;
		uint16_t nCmd;
		CEvohomeID idDev;
		msg.Get(nDevNo).Get(nCmd).Get(idDev);
		Log(true,LOG_STATUS,"evohome: %s: Dev No %d: Cmd 0x%04x DeviceID 0x%06x (%s)",tag,nDevNo,nCmd,idDev.GetID(),idDev.GetStrID().c_str());
	}
	if(msg.type==CEvohomeMsg::pktwrt)
	{
		boost::lock_guard<boost::mutex> lock(m_mtxBindNotify);
		if(msg.id[0].GetIDType()==m_nBindIDType)
		{
			m_nBindID=msg.GetID(0);
			m_cndBindNotify.notify_one();
		}
	}
	
	return true;
}

void CEvohome::RXRelay(uint8_t nDevNo, uint8_t nDemand, int nID)
{
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME3.len=sizeof(tsen.EVOHOME3)-1;
	tsen.EVOHOME3.type=pTypeEvohomeRelay;
	tsen.EVOHOME3.subtype=sTypeEvohomeRelay;
	RFX_SETID3(nID,tsen.EVOHOME3.id1,tsen.EVOHOME3.id2,tsen.EVOHOME3.id3);
	tsen.EVOHOME3.devno=nDevNo;
	tsen.EVOHOME3.demand=nDemand;
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME3);//Decode messages
}

bool CEvohome::DecodeHeatDemand(CEvohomeMsg &msg)
{
	//there is an interesting page on temperature control @ http://newton.ex.ac.uk/teaching/cdhw/Feedback/ControlTypes.html
	char tag[] = "HEAT_DEMAND";
	if (msg.payloadsize != 2){
		Log(false,LOG_ERROR,"evohome: %s: Error decoding heat demand, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	int nDevNo = msg.payload[0];
	int nDemand = msg.payload[1];
	std::string szSourceType("Unknown"), szDevType("Unknown");
	if(msg.command==0x0008)
	{
		//Demand (0x0008 for relays) is handled as a % (i.e. nDemand/200*100) (200=0xc8) where 200/200 corresponds to 100% of the cycle period (start of period determined by 3B00 ACTUATOR_CHECK message)
		//So where 100% is always on then 50% is half on half off per cycle i.e. 6 cycles per hour = 10 minutes per cycle with 5 mins on & 5 mins off etc,
		//presumably there are some settings (0x1100) that give the number of cycles per hour which would determine the length of each period
		if(msg.GetID(0)==GetControllerID())
			szSourceType="Controller";
		else if(msg.GetID(0)==GetGatewayID())
			szSourceType="Gateway";
	}
	else if(msg.command==0x3150) //heat demand for zone is also a % (as above) of the proportional band (a temperature difference) so if the proportional band is 2 degrees and we are 1 degree from set point the demand is 50%
		szSourceType="Zone";
	
	if(nDevNo==0xfc)//252...afaik this is also some sort of broadcast channel so maybe there is more to this device number than representing the boiler
		szDevType="Boiler"; //Boiler demand
	else if(nDevNo==0xfa)//250
	{
		szDevType="DHW"; //DHW zone valve
		RequestDHWState();//Trying to link the DHW state refresh to the heat demand...will be broadcast when the controller turns on or off DHW but maybe not if the heat demand doesn't change? (e.g. no heat demand but controller DHW state goes from off to on)
	}
	else if(nDevNo==0xf9)//249
		szDevType="Heating"; //Central Heating zone valve
	else //The controller uses this when a relay is bound to a zone as an actuator (boiler always seems to fire as well though)
	{
		std::ostringstream os;
		os << "Zone " << nDevNo+1;
		szDevType=os.str();
	}

	Log(true,LOG_STATUS,"evohome: %s: %s DevNo 0x%02x %s: %d", tag, szSourceType.c_str(), nDevNo, szDevType.c_str(), nDemand);

	if(msg.command==0x0008)
		RXRelay(static_cast<uint8_t>(nDevNo),static_cast<uint8_t>(nDemand));
	return true;
}

bool CEvohome::DecodeActuatorCheck(CEvohomeMsg &msg)
{
	char tag[] = "ACTUATOR_CHECK"; //this is used to synchronise time periods for each relay bound to a controller i.e. all relays get this message and use it to determine when to start each cycle (demand is just a % of the cycle length)
	if (msg.payloadsize != 2){
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	int nDevNo = msg.payload[0];
	int nDemand = msg.payload[1];
	std::string szDevType("Unknown");
	
	if(nDevNo==0xfc)//252...afaik this is also some sort of broadcast channel so maybe there is more to this device number than representing the boiler
		szDevType="All";// not sure if any other device would be valid here??

	Log(true,LOG_STATUS,"evohome: %s: DevNo 0x%02x %s: %d", tag, nDevNo, szDevType.c_str(), nDemand);
	return true;
}

bool CEvohome::DecodeActuatorState(CEvohomeMsg &msg)
{
	char tag[] = "ACTUATOR_STATE";
	if (msg.payloadsize != 3){
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	int nDevNo = msg.payload[0];
	int nDemand = msg.payload[1]; //(0 for off or 0xc8 i.e. 100% for on)
	//The relay does not appear to always announce its state but this is probably due to the wireless signal and collisions etc
	
	Log(true,LOG_STATUS,"evohome: %s: ID:0x%06x (%s) DevNo 0x%02x: %d", tag, msg.GetID(0), msg.GetStrID(0).c_str(), nDevNo, nDemand);
	RXRelay(static_cast<uint8_t>(0xFF),static_cast<uint8_t>(nDemand),msg.GetID(0));//devno is always 0 and therefore not valid
	return true;
}

bool CEvohome::DecodeExternalSensor(CEvohomeMsg &msg)
{
	char tag[] = "EXTERNAL_SENSOR";
	if (msg.payloadsize < 4) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 4 != 0) {
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, incorrect packet size: %d", tag, msg.payloadsize);
		return false;
	}	
	for (int i = 0 ; i < msg.payloadsize ; i += 4) {
		uint8_t nDevNo,nType;
		CEvohomeTemp temp;
		msg.Get(nDevNo).Get(temp).Get(nType);
		Log(true,LOG_STATUS,"evohome: %s: %d: Temp %.1f Type %d",tag,nDevNo,temp.GetTemp(),nType);
	}
	return true;
}

bool CEvohome::DecodeDeviceInfo(CEvohomeMsg &msg)
{
	char tag[] = "DEVICE_INFO";
	if (msg.payloadsize == 3){
		Log(true,LOG_STATUS,"evohome: %s: Request for device info %d",tag, msg.payload[2]);
		return true;
	}
	if (msg.payloadsize != 22){
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	uint8_t nAddr,nDevNo,nDevType;
	CEvohomeID idDev;
	msg.Get(nAddr,2).Get(nDevNo,5).Get(nDevType).Get(idDev,19);
	if(!idDev.IsValid())
	{
		m_bStartup[1]=false;
		return true;
	}
	Log(true,LOG_STATUS,"evohome: %s: %d: Addr=%d ?=%d ID:0x%06x (%s)", tag,nDevNo,nAddr,nDevType,idDev.GetID(),idDev.GetStrID().c_str());
	if(nDevType==4 && (nDevNo==0xF9 || nDevNo==0xFA || nDevNo==0xFC)) //not sure what nDevType actually represents so far 4 seems to cover TRVs and Relays but could have entirely alternate meaning
		RXRelay(nDevNo,static_cast<uint8_t>(0xFF),idDev.GetID());//We've found an internal relay so associate it's deviceid with its devno
	if(m_bStartup[1])
		RequestDeviceInfo(nAddr+1);
	return true;
}

bool CEvohome::DecodeBatteryInfo(CEvohomeMsg &msg)
{
	char tag[] = "BATTERY_INFO";
	if (msg.payloadsize != 3){
		Log(false,LOG_ERROR,"evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	std::string szType("Unknown");
	uint8_t nDevNo,nBattery,nLowBat;
	msg.Get(nDevNo).Get(nBattery).Get(nLowBat);
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME2.len=sizeof(tsen.EVOHOME2)-1;
	RFX_SETID3(msg.GetID(0),tsen.EVOHOME2.id1,tsen.EVOHOME2.id2,tsen.EVOHOME2.id3)
	tsen.EVOHOME2.updatetype = updBattery;
	
	double dbCharge=0;
	if(nBattery!=0xFF)
		dbCharge=(double)nBattery/2.0; //Presumed to be the charge level where sent
	if(nLowBat==0)
		nBattery=0;
	tsen.EVOHOME2.battery_level=nBattery;
	if(msg.id[0].GetIDType()==CEvohomeID::devZone)
	{
		if(msg.id[2].GetIDType()==CEvohomeID::devController)
		{
			szType="Zone";
			nDevNo++;
			tsen.EVOHOME2.type=pTypeEvohomeZone;
			tsen.EVOHOME2.subtype=sTypeEvohomeZone;
			tsen.EVOHOME2.zone=nDevNo;
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
		}
		else
			szType="Dev";
	}
	else if(msg.id[0].GetIDType()==CEvohomeID::devSensor)
	{
		szType="DHW";
		nDevNo++;
		tsen.EVOHOME2.type=pTypeEvohomeWater;
		tsen.EVOHOME2.subtype=sTypeEvohomeWater;
		tsen.EVOHOME2.zone=nDevNo;
		sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2);//Decode message
	}
	Log(true,LOG_STATUS,"evohome: %s: %s=%d charge=%d (%.1f %%) level=%d (%s)",tag,szType.c_str(),nDevNo,nBattery,dbCharge,nLowBat,(nLowBat==0)?"Low":"OK");
	
	return true;
}

void CEvohome::AddSendQueue(const CEvohomeMsg &msg)
{
	boost::lock_guard<boost::mutex> l(m_mtxSend);
	m_SendQueue.push_back(msg);//may throw bad_alloc
}

void CEvohome::PopSendQueue(const CEvohomeMsg &msg)
{
	boost::lock_guard<boost::mutex> l(m_mtxSend);
	if(!m_SendQueue.empty())
	{
		if(m_SendQueue.front()==msg || m_nSendFail>=10)
		{
			m_SendQueue.pop_front();
			m_nSendFail=0;
		}
		else
			m_nSendFail++;
	}
}

void CEvohome::Do_Send()
{
	boost::lock_guard<boost::mutex> rl(readQueueMutex);//ideally we need some way to send only if we're not in the middle of receiving a packet but as everything is buffered i'm not sure if this will be effective
	if(m_nBufPtr>0)
		return;
	boost::lock_guard<boost::mutex> sl(m_mtxSend);
	if(!m_SendQueue.empty())
	{
		if(m_SendQueue.front().BadMsg())//message rejected by HGI80 which will not be picked up as a failed send
			m_SendQueue.pop_front();
		else
		{
			std::string out(m_SendQueue.front().Encode()+"\r\n");
			write(out.c_str(),out.length());
		}
	}
}

bool CEvohome::SetZoneCount(uint8_t nZoneCount)
{
	boost::lock_guard<boost::mutex> l(m_mtxZoneCount);
	bool bRet=(m_nZoneCount!=nZoneCount);
	m_nZoneCount=nZoneCount;
	return bRet;
}

bool CEvohome::SetMaxZoneCount(uint8_t nZoneCount)
{
	boost::lock_guard<boost::mutex> l(m_mtxZoneCount);
	int nMaxZones=(std::max)(m_nZoneCount,nZoneCount); //parentheses around function name apparently avoids macro expansion windef.h macros will conflict here
	bool bRet=(m_nZoneCount!=nMaxZones);
	m_nZoneCount=nMaxZones;
	return bRet;
}

uint8_t CEvohome::GetZoneCount()
{
	boost::lock_guard<boost::mutex> l(m_mtxZoneCount);
	return m_nZoneCount; //return value is constructed before the lock is released
}

bool CEvohome::SetControllerMode(uint8_t nControllerMode)
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerMode);
	bool bRet=(m_nControllerMode!=nControllerMode);
	m_nControllerMode=nControllerMode;
	return bRet;
}

uint8_t CEvohome::GetControllerMode()
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerMode);
	return m_nControllerMode; //return value is constructed before the lock is released
}

void CEvohome::InitControllerName()
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerName);
	if(m_szControllerName.empty())
		m_szControllerName="EvoTouch Colour";
}

void CEvohome::SetControllerName(std::string szName)
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerName);
	m_szControllerName=szName;
}

std::string CEvohome::GetControllerName()
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerName);
	return m_szControllerName;
}

void CEvohome::InitZoneNames()
{
	boost::lock_guard<boost::mutex> l(m_mtxZoneName);
	char szTmp[1024];
	for(size_t i=0;i<m_ZoneNames.size();i++)
	{
		if(m_ZoneNames[i].empty())
		{
			sprintf(szTmp,"Zone %lu",i+1);
			m_ZoneNames[i]=szTmp;
		}
	}
}

void CEvohome::SetZoneName(uint8_t nZone, std::string szName)
{
	boost::lock_guard<boost::mutex> l(m_mtxZoneName);
	if(nZone>=m_ZoneNames.size()) //should be pre-sized to max zones
		return;
	m_ZoneNames[nZone]=szName;
}

std::string CEvohome::GetZoneName(uint8_t nZone)
{
	boost::lock_guard<boost::mutex> l(m_mtxZoneName);
	if(nZone>=m_ZoneNames.size()) //should be pre-sized to max zones
		return "Out of bounds";
	return m_ZoneNames[nZone];
}

int CEvohome::GetControllerID()
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerID);
	return m_nDevID;
}

void CEvohome::SetControllerID(int nID)
{
	boost::lock_guard<boost::mutex> l(m_mtxControllerID);
	m_nDevID=nID;
}

int CEvohome::GetGatewayID()
{
	boost::lock_guard<boost::mutex> l(m_mtxGatewayID);
	return m_nMyID;
}

void CEvohome::SetGatewayID(int nID)
{
	boost::lock_guard<boost::mutex> l(m_mtxGatewayID);
	m_nMyID=nID;
}

void CEvohome::LogDate()
{
        char szTmp[256];
	time_t atime = mytime(NULL);
        struct tm ltime;
        localtime_r(&atime, &ltime);
        
        strftime (szTmp,256,"%Y-%m-%d %H:%M:%S ",&ltime);
        *m_pEvoLog << szTmp;
}

void CEvohome::Log(const char *szMsg, CEvohomeMsg &msg)
{
	if(m_bDebug && m_pEvoLog)
	{
		LogDate();
		*m_pEvoLog << szMsg;
		*m_pEvoLog << " (";
		for(int i=0;i<msg.payloadsize;i++)
			*m_pEvoLog << msg.payload[i];
		*m_pEvoLog << ")";
		*m_pEvoLog << std::endl;
	}
}

void CEvohome::Log(bool bDebug, int nLogLevel, const char* format, ... )
{
        va_list argList;
        char cbuffer[1024];
        va_start(argList, format);
        vsnprintf(cbuffer, 1024, format, argList);
        va_end(argList);

	if(!bDebug || m_bDebug)
		_log.Log(static_cast<_eLogLevel>(nLogLevel),cbuffer);
	if(m_bDebug && m_pEvoLog)
	{
		LogDate();
		*m_pEvoLog << cbuffer;
		*m_pEvoLog << std::endl;
	}
}
