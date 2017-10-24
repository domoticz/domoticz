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
#include "EvohomeRadio.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <boost/bind.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

extern std::string szUserDataFolder;

#include <ctime>

const char  CEvohomeRadio::m_szNameErr[18]={0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F};
const int CEvohomeRadio::m_evoToDczControllerMode[8]={0,5,1,2,3,-1,-1,4};//are the hidden modes actually valid?
const int  CEvohomeRadio::m_evoToDczOverrideMode[5]={zmAuto,-1,zmPerm,-1,zmTmp};//are the hidden modes actually valid?
const uint8_t CEvohomeRadio::m_dczToEvoZoneMode[3]={0,2,4};
const uint8_t CEvohomeRadio::m_dczToEvoControllerMode[6]={0,2,3,4,7,1};

char const CEvohomeMsg::szPacketType[5][8]={"Unknown","I","RQ","RP","W"};

CEvohomeRadio::CEvohomeRadio(const int ID, const std::string &UserContID)
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
	m_MaxDeviceID = 0;

	AllSensors = false;

	if (!UserContID.empty())
	{
		std::stringstream s_strid(UserContID);
		s_strid >> std::hex >> m_UControllerID;
	}

	RegisterDecoder(cmdZoneTemp,boost::bind(&CEvohomeRadio::DecodeZoneTemp,this, _1));
	RegisterDecoder(cmdSetPoint,boost::bind(&CEvohomeRadio::DecodeSetpoint,this, _1));
	RegisterDecoder(cmdSetpointOverride,boost::bind(&CEvohomeRadio::DecodeSetpointOverride,this, _1));
	RegisterDecoder(cmdDHWState,boost::bind(&CEvohomeRadio::DecodeDHWState,this, _1));
	RegisterDecoder(cmdDHWTemp,boost::bind(&CEvohomeRadio::DecodeDHWTemp,this, _1));
	RegisterDecoder(cmdControllerMode,boost::bind(&CEvohomeRadio::DecodeControllerMode,this, _1));
	RegisterDecoder(cmdSysInfo,boost::bind(&CEvohomeRadio::DecodeSysInfo,this, _1));
	RegisterDecoder(cmdZoneName,boost::bind(&CEvohomeRadio::DecodeZoneName,this, _1));
	RegisterDecoder(cmdZoneHeatDemand,boost::bind(&CEvohomeRadio::DecodeHeatDemand,this, _1));
	RegisterDecoder(cmdZoneInfo,boost::bind(&CEvohomeRadio::DecodeZoneInfo,this, _1));
	RegisterDecoder(cmdControllerHeatDemand,boost::bind(&CEvohomeRadio::DecodeHeatDemand,this, _1));
	RegisterDecoder(cmdBinding,boost::bind(&CEvohomeRadio::DecodeBinding,this, _1));
	RegisterDecoder(cmdActuatorState,boost::bind(&CEvohomeRadio::DecodeActuatorState,this, _1));
	RegisterDecoder(cmdActuatorCheck,boost::bind(&CEvohomeRadio::DecodeActuatorCheck,this, _1));
	RegisterDecoder(cmdZoneWindow,boost::bind(&CEvohomeRadio::DecodeZoneWindow,this, _1));
	RegisterDecoder(cmdExternalSensor,boost::bind(&CEvohomeRadio::DecodeExternalSensor,this, _1));
	RegisterDecoder(cmdDeviceInfo,boost::bind(&CEvohomeRadio::DecodeDeviceInfo,this, _1));
	RegisterDecoder(cmdBatteryInfo,boost::bind(&CEvohomeRadio::DecodeBatteryInfo,this, _1));
}


CEvohomeRadio::~CEvohomeRadio(void)
{
	m_bIsStarted=false;
}


void CEvohomeRadio::RegisterDecoder(unsigned int cmd, fnc_evohome_decode fndecoder)
{
	m_Decoders.insert( std::pair<int, fnc_evohome_decode >( cmd, fndecoder ) );
}

bool CEvohomeRadio::StartHardware()
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

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,DeviceID,nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==0) AND (Type==%d)", m_HwdID, (int)pTypeEvohome);

	if (!result.empty())
	{
		std::vector<std::string> sd=result[0];
		std::stringstream s_strid;
		s_strid << std::hex << sd[1];
		if (!s_strid.str().empty())  // Check whether the controller's DeviceID has been set
		{
			s_strid >> m_nDevID;
			SetControllerID(m_nDevID);
			Log(true, LOG_STATUS, "evohome: Controller ID (0x%x)", GetControllerID());
		}
		else
		{
			if (m_UControllerID)  // Check whether controller ID has been entered on hardware settings page
			{
				SetControllerID(m_UControllerID);
				Log(true, LOG_STATUS, "evohome: Controller ID specified (0x%x)", m_UControllerID);
			}
			else
				SetControllerID(0xFFFFFF);  // Dummy value to allow detection of multiple controllers
		}
		m_nControllerMode=atoi(sd[2].c_str());
	}
	else
	{
		if (m_UControllerID)
		{
			SetControllerID(m_UControllerID);
			Log(true, LOG_STATUS, "evohome: Controller ID specified (0x%x)", m_UControllerID);
		}
		else
			SetControllerID(0xFFFFFF);  // Dummy valueto allow detection of multiple controllers
	}

	//we'll put our custom relays in this range
	result = m_sql.safe_query("SELECT  Unit,Name,DeviceID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit>=64) AND (Unit<96)", m_HwdID, (int)pTypeEvohomeRelay);
	m_RelayCheck.clear();
	for (int i = 0; i<static_cast<int>(result.size()); i++)
	{
		Log(true,LOG_STATUS,"evohome: Relay: devno=%d demmand=%d",atoi(result[i][0].c_str()),atoi(result[i][4].c_str()));
		m_RelayCheck.insert( tmap_relay_check_pair( static_cast<uint8_t>(atoi(result[i][0].c_str())),_tRelayCheck(boost::get_system_time()-boost::posix_time::minutes(19),static_cast<uint8_t>(atoi(result[i][4].c_str()))) ) ); //allow 1 minute for startup before trying to restore demand
	}

	//Start worker thread
	m_bDoRestart=false;
	m_retrycntr = EVOHOME_RETRY_DELAY; //will force reconnect first thing
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEvohomeRadio::Do_Work, this)));
	m_bIsStarted=true;

	return (m_thread!=NULL);
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


bool CEvohomeRadio::WriteToHardware(const char *pdata, const unsigned char length)
{
	if(!pdata)
		return false;
	REVOBUF *tsen=(REVOBUF*)pdata;
	switch (pdata[1])
	{
		case pTypeEvohome:
			if (length<sizeof(REVOBUF::_tEVOHOME1))
				return false;
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktwrt,GetControllerID(),cmdControllerMode).Add(ConvertMode(m_dczToEvoControllerMode,tsen->EVOHOME1.status)).Add((tsen->EVOHOME1.mode==1)?CEvohomeDateTime(tsen->EVOHOME1):CEvohomeDateTime()).Add(tsen->EVOHOME1.mode));
			break;
		case pTypeEvohomeZone:
			if (length<sizeof(REVOBUF::_tEVOHOME2))
				return false;
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktwrt,GetControllerID(),cmdSetpointOverride).Add((uint8_t)(tsen->EVOHOME2.zone-1)).Add(tsen->EVOHOME2.temperature).Add(ConvertMode(m_dczToEvoZoneMode,tsen->EVOHOME2.mode)).Add((uint16_t)0xFFFF).Add((uint8_t)0xFF).Add_if(CEvohomeDateTime(tsen->EVOHOME2),(tsen->EVOHOME2.mode==2)));
			break;
		case pTypeEvohomeWater:
			if (length<sizeof(REVOBUF::_tEVOHOME2))
				return false;
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktwrt,GetControllerID(),cmdDHWState).Add((uint8_t)(tsen->EVOHOME2.zone-1)).Add((uint8_t)tsen->EVOHOME2.temperature).Add(ConvertMode(m_dczToEvoZoneMode,tsen->EVOHOME2.mode)).Add((uint16_t)0).Add((uint8_t)0).Add_if(CEvohomeDateTime(tsen->EVOHOME2),(tsen->EVOHOME2.mode==2)));
			break;
		case pTypeEvohomeRelay:
			if (length<sizeof(REVOBUF::_tEVOHOME3))
				return false;
			SetRelayHeatDemand(tsen->EVOHOME3.devno,tsen->EVOHOME3.demand);
			break;
	}
	return true;
}


void CEvohomeRadio::RequestZoneTemp(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdZoneTemp).Add(nZone));
}


void CEvohomeRadio::RequestZoneName(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdZoneName).Add(nZone).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestZoneInfo(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdZoneInfo).Add(nZone));
}


void CEvohomeRadio::RequestSetPointOverride(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdSetpointOverride).Add(nZone));
}


void CEvohomeRadio::RequestDHWState()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdDHWState).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestDHWTemp()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdDHWTemp).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestControllerMode()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdControllerMode).Add(uint8_t(0xFF)));
}


void CEvohomeRadio::RequestSysInfo()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdSysInfo).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestZoneStartupInfo(uint8_t nZone)
{
	RequestZoneTemp(nZone);
	RequestSetPointOverride(nZone);
	RequestZoneInfo(nZone);
	RequestZoneName(nZone);
}


void CEvohomeRadio::RequestCurrentState()
{
	RequestSysInfo();
	RequestControllerMode();
	RequestDHWTemp();
	RequestDHWState();
	RequestZoneStartupInfo(0);
	RequestDeviceInfo(0);
}


void CEvohomeRadio::RequestZoneState()
{
	uint8_t nZoneCount=GetZoneCount();
	for(uint8_t i=0;i<nZoneCount;i++)
		RequestSetPointOverride(i);
	//Trying this linked to DHW heat demand instead...that won't be adequate do it here too!
	RequestDHWState();
	SendExternalSensor();
	SendZoneSensor();
}


void CEvohomeRadio::RequestZoneNames()
{
	uint8_t nZoneCount=GetZoneCount();
	for(uint8_t i=0;i<nZoneCount;i++)
		RequestZoneName(i);
}


void CEvohomeRadio::RequestDeviceInfo(uint8_t nAddr)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq,GetControllerID(),cmdDeviceInfo).Add(uint8_t(0)).Add(uint8_t(0)).Add(nAddr));
}


void CEvohomeRadio::SendRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,GetGatewayID(),cmdControllerHeatDemand).Add(nDevNo).Add(nDemand));
}

void CEvohomeRadio::UpdateRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	SendRelayHeatDemand(nDevNo,nDemand);
	m_RelayCheck[nDevNo]=_tRelayCheck(boost::get_system_time(),nDemand);
}


void CEvohomeRadio::SetRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	if(nDevNo<64 || nDevNo>=96 || GetGatewayID()==0) //atm no reason to interfere with non custom relays
		return;
	boost::lock_guard<boost::mutex> l(m_mtxRelayCheck);
	UpdateRelayHeatDemand(nDevNo,nDemand);
}


void CEvohomeRadio::CheckRelayHeatDemand()
{
	for(tmap_relay_check_it it = m_RelayCheck.begin(); it != m_RelayCheck.end(); ++it)
	{
		if((boost::get_system_time()-it->second.m_stLastCheck)>boost::posix_time::seconds(1202)) //avg seems around 1202-1203 but not clear how reference point derived
		{
			Log(true,LOG_STATUS,"evohome: Relay: Refreshing heat demand devno=%d demand=%d",it->first,it->second.m_nDemand);
			UpdateRelayHeatDemand(it->first,it->second.m_nDemand);
		}
	}
}


void CEvohomeRadio::SendRelayKeepAlive()
{
	Log(true,LOG_STATUS,"evohome: Relay: Sending keep alive");
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,GetGatewayID(),cmdActuatorCheck).Add((uint8_t)0xFC).Add((uint8_t)0xC8));
}


void CEvohomeRadio::SendExternalSensor()
{
	if(GetGatewayID()==0)
		return;
	double dbTemp=0.0,dbUV=0.0;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (Name=='Outside')");//There could be different types depending on how data is received from WU etc.
	if (!result.empty())
	{
		std::vector<std::string> strarray;
		StringSplit(result[0][0], ";", strarray);
		if (!strarray.empty())
			dbTemp=atof(strarray[0].c_str());
		else
			return;
	}
	else
		return;

	//FIXME no light level data available UV from WU is only thing vaguely close (on dev system) without a real sensor
	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (Type==%d)", (int)pTypeUV);
	if (!result.empty())
		dbUV=atof(result[0][0].c_str());
	else
		return;

	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf,0,GetGatewayID(),cmdExternalSensor).Add((uint8_t)0).Add(static_cast<uint16_t>(dbUV*39)).Add((uint8_t)2).Add((uint8_t)2).Add(static_cast<int16_t>(dbTemp*100.0)).Add((uint8_t)1));
}


void CEvohomeRadio::SendZoneSensor()
{
	unsigned int ID;

	if (GetGatewayID() == 0)
		return;
	double dbTemp = 0.0;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Max(Unit) FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit>=40) AND (Unit<52)", m_HwdID, (int)pTypeEvohomeZone);

	int nDevCount = 0;
	if (!result.empty())
	{
		nDevCount = atoi(result[0][0].c_str());
	}
	else return;
	for (int i = 40; i <= nDevCount; ++i)
	{
		result = m_sql.safe_query("SELECT DeviceID, Name  FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit==%d)", m_HwdID, (int)pTypeEvohomeZone, i); // Get Zone Name and DeviceID
		if (!result.empty()) //Check that sensor number exists - this allows for deletion
		{
			//std::vector<std::string> sd = result[0];
			std::stringstream s_strid;
			std::string SensorName = result[0][1].c_str();
			//s_strid << std::hex << sd[0].c_str();
			s_strid << std::hex << result[0][0].c_str();
			s_strid >> ID;
			result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (Type !=%d) AND (Name=='%q')", (int)pTypeEvohomeZone, SensorName.c_str()); // Get temperature from external sensor with matching Name
			if (result.size() == 1)
			{
				std::vector<std::string> strarray;
				StringSplit(result[0][0], ";", strarray);
				if (!strarray.empty())
					dbTemp = atof(strarray[0].c_str());
				Log(true, LOG_STATUS, "evohome: Send Temp Zone msg Zone: %d DeviceID: 0x%x Name:%s Temp:%f ", i, ID, SensorName.c_str(), dbTemp);
				AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, ID, cmdZoneTemp).Add((uint8_t)0).Add(static_cast<int16_t>(dbTemp*100.0)));
				// Update the dummy Temp Zone device with the new temperature
				REVOBUF tsen;
				memset(&tsen, 0, sizeof(REVOBUF));
				tsen.EVOHOME2.len = sizeof(tsen.EVOHOME2) - 1;
				tsen.EVOHOME2.type = pTypeEvohomeZone;
				tsen.EVOHOME2.subtype = sTypeEvohomeZone;
				tsen.EVOHOME2.zone = i;
				tsen.EVOHOME2.temperature = static_cast<uint16_t>(dbTemp * 100);
				RFX_SETID3(ID, tsen.EVOHOME2.id1, tsen.EVOHOME2.id2, tsen.EVOHOME2.id3);
				sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "Zone Temp", -1);
			}
		}
	}
}

bool CEvohomeRadio::HandleLoopData(const char *data, size_t len)
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

int CEvohomeRadio::ProcessBuf(char * buf, int size)
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
			if(SetPacketType(tkn)!=pktunk)
				SetFlag(flgpkt);
		}
		else if(cmdidx && i==cmdidx+2) //payload starts 2 after command idx
		{
			if(!GetFlag(flgps))
			{
				CEvohomeRadio::Log(false,LOG_ERROR,"evohome: no payload size - possible corrupt message");
				return false;
			}
			if(tkn.length()%2)
			{
				CEvohomeRadio::Log(false,LOG_ERROR,"evohome: uneven payload - possible corrupt message");
				return false;
			}
			if(tkn.length()/2>m_nBufSize)
			{
				CEvohomeRadio::Log(false,LOG_ERROR,"evohome: payload exceeds max buffer size");
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
				CEvohomeRadio::Log(false,LOG_ERROR,"evohome: payload size does not match specified size from packet header");
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
					CEvohomeRadio::Log(false,LOG_ERROR,"evohome: too many message ids - possible corrupt message");
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
				CEvohomeRadio::Log(false,LOG_STATUS,"evohome: WARNING unrecognised message structure - possible corrupt message '%s' (%d)",tkn.c_str(),i);
		}
	}
	return IsValid();
}


void CEvohomeRadio::ProcessMsg(const char * rawmsg)
{
	CEvohomeMsg msg(rawmsg);
	Log(rawmsg,msg);
	if(msg.IsValid())
	{
		if (GetControllerID()== 0xFFFFFF) // If we still have a dummy controller update the controller DeviceID list
		{
			for (int n = 0; n<3; n++)
			{
				if (msg.id[n].GetIDType() == CEvohomeID::devController)//id type 1 is for controllers
				{
					for (uint8_t i = 0; i < 5; i++)
					{
						if (MultiControllerID[i] == msg.GetID(n))
							break;
						else if (MultiControllerID[i] == 0)
						{
							MultiControllerID[i] = msg.GetID(n);
							_log.Log(LOG_STATUS, "evohome: controller detected, ID:0x%x", MultiControllerID[i]);
							break;
						}

					}

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


bool CEvohomeRadio::DecodePayload(CEvohomeMsg &msg)
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


bool CEvohomeRadio::DumpMessage(CEvohomeMsg &msg)
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


int CEvohomeRadio::Bind(uint8_t nDevNo, unsigned char nDevType)//use CEvohomeID::devType atm but there could be additional specialisations
{
	int nGatewayID=GetGatewayID();
	if(nGatewayID==0)
		return 0;
	uint8_t ID1;
	uint8_t ID2;
	uint8_t ID3;

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
	else if (nDevType == CEvohomeID::devZone)//Binding the HGI80 to the evohome controller as a zone temperature sensor
	{
		// Check that the max Device ID includes the zone temperature sensors
		std::vector<std::vector<std::string> > result;

		result = m_sql.safe_query("SELECT MAX(DeviceID) FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit>=40) AND (Unit<52)", m_HwdID, (int)pTypeEvohomeZone);
		unsigned int ID = 0;
		if (!result.empty())
		{
			std::stringstream s_strid;
			s_strid << std::hex << result[0][0].c_str();
			s_strid >> ID;
		}
		if (ID > m_MaxDeviceID)
			m_MaxDeviceID = ID + 1;
		else
			m_MaxDeviceID++;
		boost::unique_lock<boost::mutex> lock(m_mtxBindNotify);
		m_nBindID = 0;
		m_nBindIDType = CEvohomeID::devController;
		RFX_SETID3(m_MaxDeviceID, ID1, ID2, ID3);

		boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(60);//monotonic?
		while (m_nBindID == 0)
		{
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, nGatewayID, cmdBinding).Add((uint8_t)0).Add((uint16_t)cmdZoneTemp).Add(ID1).Add(ID2).Add(ID3).Add((uint8_t)2).Add((uint16_t)cmdZoneTemp).Add(ID1).Add(ID2).Add(ID3).Add((uint8_t)0).Add((uint16_t)cmdBinding).Add(CEvohomeID(nGatewayID)));
			boost::system_time const wait = boost::get_system_time() + boost::posix_time::seconds(5);//monotonic?
			if (!m_cndBindNotify.timed_wait(lock, wait) && boost::get_system_time()>timeout)
				return 0;
		}
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, m_nBindID, cmdBinding).Add((uint8_t)0).Add((uint16_t)0x30C9).Add(ID1).Add(ID2).Add(ID3));
		return m_MaxDeviceID;
	}
	return 0;
}


bool CEvohomeRadio::DecodeSetpoint(CEvohomeMsg &msg)//0x2309
{
	char tag[] = "ZONE_SETPOINT";

	if (msg.GetID(0) != GetControllerID() && msg.GetID(2) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "Setpoint", -1);
		else if (AllSensors)
		{
			char zstrname[40];

			sprintf(zstrname, "Zone %d", tsen.EVOHOME2.zone);
			tsen.EVOHOME2.zone += 12; //zone number offset by 12
			Log(true, LOG_STATUS, "evohome: %s: Setting: %d (0x%x): %d", tag, tsen.EVOHOME2.zone, msg.GetID(0), tsen.EVOHOME2.temperature);
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, zstrname, -1);
		}
	}

	return true;
}


bool CEvohomeRadio::DecodeSetpointOverride(CEvohomeMsg &msg)//0x2349
{
	char tag[] = "ZONE_SETPOINT_MODE";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d (0x%x): %d (%d=%s) %s", tag, tsen.EVOHOME2.zone, msg.GetID(0), tsen.EVOHOME2.temperature, tsen.EVOHOME2.mode, GetZoneModeName(tsen.EVOHOME2.mode),CEvohomeDateTime::GetStrDate(tsen.EVOHOME2).c_str());
	}
	else
	{
		Log(true,LOG_STATUS,"evohome: %s: Setting: %d (0x%x): %d (%d=%s)", tag, tsen.EVOHOME2.zone, msg.GetID(0), tsen.EVOHOME2.temperature, tsen.EVOHOME2.mode, GetZoneModeName(tsen.EVOHOME2.mode));
	}

	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "Setpoint", -1);
	return true;
}


bool CEvohomeRadio::DecodeZoneTemp(CEvohomeMsg &msg)//0x30C9
{
	char tag[] = "ZONE_TEMP";
	std::vector<std::vector<std::string> > result;

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
	{
		std::string zstrid(CEvohomeID::GetHexID(msg.GetID(0)));
		result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q')", m_HwdID, zstrid.c_str());
		if (result.empty()) // Check whether DeviceID has already been registered
			return true;
	}


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
		Log(true, LOG_STATUS, "evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature);
		if(tsen.EVOHOME2.temperature!=0x7FFF)//afaik this is the error value just ignore it right now as we have no way to report errors...also perhaps could be returned if DHW is not installed?
		{
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "Zone Temp", -1);
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
		if (AllSensors)
		{
			std::vector<std::vector<std::string> > result;
			std::string zstrid(CEvohomeID::GetHexID(msg.GetID(0)));

			result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q') AND (Type == %d)", m_HwdID, zstrid.c_str(), (int)pTypeEvohomeZone);
			if (!result.empty()) // Update existing temp sensor with value directly from sensor
			{
				tsen.EVOHOME2.zone = atoi(result[0][0].c_str());
				Log(true, LOG_STATUS, "evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature);
				sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "Zone Temp", -1);
			}
			else // If matching relay with same deviceID then create a new Zone Temp sensor with zone number offset by 12
			{
				result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q') AND (Type == %d)", m_HwdID, zstrid.c_str(), (int)pTypeEvohomeRelay);
				if (!result.empty())
				{
					tsen.EVOHOME2.zone = atoi(result[0][0].c_str()) + 12;
					char zstrname[40];
					sprintf(zstrname, "Zone %d", atoi(result[0][0].c_str()));
					Log(true, LOG_STATUS, "evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.EVOHOME2.zone, tsen.EVOHOME2.temperature);
					sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, zstrname, -1);
				}
			}
		}
		if (msg.payloadsize != 3) {
			Log(true,LOG_STATUS,"evohome: %s: WARNING: got a sensor temperature msg with an unexpected payload size: %d", tag, msg.payloadsize);
		}
		//in this case the zoneID is always 0 and hence is worthless
		if (msg.payload[0] != 0) {
			Log(true,LOG_STATUS,"evohome: %s: WARNING: sensor reading with zone != 0: 0x%x - %d", tag, msg.GetID(0), msg.payload[0]);
		}
	}
	if ((msg.GetID(0) > m_MaxDeviceID))
		m_MaxDeviceID = msg.GetID(0);

	return true;
}


bool CEvohomeRadio::DecodeDHWState(CEvohomeMsg &msg)//1F41
{
	char tag[] = "DHW_STATE";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
	if(tsen.EVOHOME2.temperature == 0xFF)// temperature = 255 if DHW not installed
		return true;
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

	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "DHW", -1);
	return true;
}


bool CEvohomeRadio::DecodeDHWTemp(CEvohomeMsg &msg)//1260
{
	char tag[] = "DHW_TEMP";

	// Filter out messages from other controllers
	// This also filters out messages sent direct from sensor as can't uniquely identify its DeviceID so added workaround of regularly requesting temp from controller
	if (msg.GetID(0) != GetControllerID())
		return true;

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
		if(tsen.EVOHOME2.temperature!=0x7FFF)// DHW is not installed, discard value
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "DHW Temp", -1);
	}

	return true;
}


bool CEvohomeRadio::DecodeControllerMode(CEvohomeMsg &msg)//2E04
{
	char tag[] = "CONTROLLER_MODE";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME1, "Controller Mode", -1);

	if(SetControllerMode(nControllerMode))//if only the until time changed we should be ok as the unit will broadcast a new controller mode when the current mode ends
		RequestZoneState();//This can conflict with our startup polling but will still succeed ok
	return true;
}


bool CEvohomeRadio::DecodeSysInfo(CEvohomeMsg &msg)//10e0
{
	char tag[] = "SYSINFO";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

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


bool CEvohomeRadio::DecodeZoneName(CEvohomeMsg &msg)
{
	char tag[] = "ZONE_NAME";
	std::vector<std::vector<std::string> > result;

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
		Log(true,LOG_STATUS,"evohome: %s: Warning zone name not set: %d", tag, msg.payload[0]+1);
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

	// Update any zone names which are still the defaults
	result = m_sql.safe_query("SELECT Name FROM Devicestatus WHERE ((HardwareID==%d) AND (Type==%d) AND (Unit == %d) AND (Name == 'Zone Temp')) OR ((HardwareID==%d) AND (Type==%d) AND (Unit == %d) AND (Name == 'Setpoint'))", m_HwdID, (int)pTypeEvohomeZone, nZone, m_HwdID, (int)pTypeEvohomeZone, nZone);
	if (!result.empty())
		m_sql.safe_query("UPDATE Devicestatus SET Name='%q' WHERE (HardwareID==%d) AND (Type==%d) AND (Unit == %d)", (const char*)&msg.payload[2], m_HwdID, (int)pTypeEvohomeZone, nZone);
	result = m_sql.safe_query("SELECT Name FROM Devicestatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit == %d) AND (Name == 'Zone')", m_HwdID, (int)pTypeEvohomeRelay, nZone);
	if (!result.empty())
		m_sql.safe_query("UPDATE Devicestatus SET Name='%q' WHERE (HardwareID==%d) AND (Type==%d) AND (Unit == %d)", (const char*)&msg.payload[2], m_HwdID, (int)pTypeEvohomeRelay, nZone);

	return true;
}


bool CEvohomeRadio::DecodeZoneInfo(CEvohomeMsg &msg)
{
	char tag[] = "ZONE_INFO";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

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


bool CEvohomeRadio::DecodeZoneWindow(CEvohomeMsg &msg)
{
	char tag[] = "ZONE_WINDOW";

	if (msg.GetID(0) != GetControllerID() && msg.GetID(2) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
	Log(true, LOG_STATUS, "evohome: %s: %d: Window %d", tag, tsen.EVOHOME2.zone, nWindow);

	if (msg.GetID(0) == GetControllerID())
		sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "Zone Window", -1);

	return true;
}


bool CEvohomeRadio::DecodeBinding(CEvohomeMsg &msg)
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


void CEvohomeRadio::RXRelay(uint8_t nDevNo, uint8_t nDemand, int nID)
{
	REVOBUF tsen;
	memset(&tsen,0,sizeof(REVOBUF));
	tsen.EVOHOME3.len=sizeof(tsen.EVOHOME3)-1;
	tsen.EVOHOME3.type=pTypeEvohomeRelay;
	tsen.EVOHOME3.subtype=sTypeEvohomeRelay;
	RFX_SETID3(nID,tsen.EVOHOME3.id1,tsen.EVOHOME3.id2,tsen.EVOHOME3.id3);
	tsen.EVOHOME3.devno=nDevNo;
	tsen.EVOHOME3.demand=nDemand;
	tsen.EVOHOME3.updatetype = CEvohomeRadio::updDemand;
	sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME3, NULL, -1);
}

bool CEvohomeRadio::DecodeHeatDemand(CEvohomeMsg &msg)
{
	//there is an interesting page on temperature control @ http://newton.ex.ac.uk/teaching/cdhw/Feedback/ControlTypes.html
	char tag[] = "HEAT_DEMAND";

	if (msg.GetID(0) != GetControllerID() && msg.GetID(2) != GetControllerID()) // Filter out messages from other controllers
		return true;

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
		if (msg.GetID(0) == GetControllerID())
			szSourceType = "Controller";
		else if (msg.GetID(0) == GetGatewayID())
			szSourceType = "Gateway";
	}
	else if(msg.command==0x3150) //heat demand for zone is also a % (as above) of the proportional band (a temperature difference) so if the proportional band is 2 degrees and we are 1 degree from set point the demand is 50%
	{
		if (nDevNo <= 12)
			nDevNo++; //Need to add 1 to give correct zone numbers
		RXRelay(static_cast<uint8_t>(nDevNo), static_cast<uint8_t>(nDemand), msg.GetID(0)); // adding DeviceID creates multiple devices for zones with mutiple sensors
		szSourceType = "Zone";
	}

	if(nDevNo==0xfc)//252...afaik this is also some sort of broadcast channel so maybe there is more to this device number than representing the boiler
		szDevType="Boiler"; //Boiler demand
	else if(nDevNo==0xfa)//250
	{
		szDevType="DHW"; //DHW zone valve
		RequestDHWState();//Trying to link the DHW state refresh to the heat demand...will be broadcast when the controller turns on or off DHW but maybe not if the heat demand doesn't change? (e.g. no heat demand but controller DHW state goes from off to on)
	}
	else if(nDevNo==0xf9)//249
		szDevType="Heating"; //Central Heating zone valve

	Log(true,LOG_STATUS,"evohome: %s: %s (0x%x) DevNo 0x%02x %d (0x%x)", tag, szSourceType.c_str(), msg.GetID(0), nDevNo, nDemand, msg.command);

	if(msg.command==0x0008)
	{
		if (nDevNo < 12)
			nDevNo++; //Need to add 1 to give correct zone numbers
		RXRelay(static_cast<uint8_t>(nDevNo),static_cast<uint8_t>(nDemand), msg.GetID(0));
	}
	return true;
}


bool CEvohomeRadio::DecodeActuatorCheck(CEvohomeMsg &msg)
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


bool CEvohomeRadio::DecodeActuatorState(CEvohomeMsg &msg)
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
	RXRelay(static_cast<uint8_t>(0xFF),static_cast<uint8_t>(nDemand));//devno is always 0 and therefore not valid
	return true;
}


bool CEvohomeRadio::DecodeExternalSensor(CEvohomeMsg &msg)
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


bool CEvohomeRadio::DecodeDeviceInfo(CEvohomeMsg &msg)
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
	if(m_bStartup[1])
		RequestDeviceInfo(nAddr+1);
	return true;
}


bool CEvohomeRadio::DecodeBatteryInfo(CEvohomeMsg &msg)
{
	char tag[] = "BATTERY_INFO";

	if ((msg.GetID(2) != GetControllerID()) && (msg.id[0].GetIDType() != CEvohomeID::devSensor)) // Filter out messages from other controllers
		return true;

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

	if (nBattery == 0xFF)
		nBattery = 100; // recode full battery (0xFF) to 100 for consistency across device types
	else
		nBattery = nBattery / 2;  // recode battery level values to 0-100 from original 0-200 values

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
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, NULL, nBattery);

			if (AllSensors)
			{
				tsen.EVOHOME2.type = pTypeEvohomeZone;
				tsen.EVOHOME2.subtype = sTypeEvohomeZone;
				tsen.EVOHOME2.zone += 12;
				sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, NULL, nBattery); // Update Zone device battery level
			}

			tsen.EVOHOME3.len = sizeof(tsen.EVOHOME3) - 1;
			tsen.EVOHOME3.type = pTypeEvohomeRelay;
			tsen.EVOHOME3.subtype = sTypeEvohomeRelay;
			RFX_SETID3(msg.GetID(0), tsen.EVOHOME3.id1, tsen.EVOHOME3.id2, tsen.EVOHOME3.id3);
			tsen.EVOHOME3.devno = nDevNo;
			tsen.EVOHOME3.demand = 0;
			tsen.EVOHOME3.updatetype = CEvohomeRadio::updBattery;
			tsen.EVOHOME3.battery_level = nBattery;
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME3, NULL, nBattery);
		}
		else
		{
			szType="Dev";
			tsen.EVOHOME2.type = pTypeEvohomeZone;
			tsen.EVOHOME2.subtype = sTypeEvohomeZone;
			tsen.EVOHOME2.zone = nDevNo;
			sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, NULL, nBattery);  // Update Relay device battery level
		}
	}
	else if(msg.id[0].GetIDType()==CEvohomeID::devSensor)
	{
		szType="DHW";
		nDevNo++;
		tsen.EVOHOME2.type=pTypeEvohomeWater;
		tsen.EVOHOME2.subtype=sTypeEvohomeWater;
		tsen.EVOHOME2.zone=nDevNo;
		RFX_SETID3(GetControllerID(), tsen.EVOHOME2.id1, tsen.EVOHOME2.id2, tsen.EVOHOME2.id3);
		sDecodeRXMessage(this, (const unsigned char *)&tsen.EVOHOME2, "DHW Temp", nBattery);  // Update DHW Zone sensor
	}
	Log(true,LOG_STATUS,"evohome: %s: %s=%d charge=%d(%%) level=%d (%s)",tag,szType.c_str(),nDevNo,nBattery,nLowBat,(nLowBat==0)?"Low":"OK");

	return true;
}


void CEvohomeRadio::AddSendQueue(const CEvohomeMsg &msg)
{
	boost::lock_guard<boost::mutex> l(m_mtxSend);
	m_SendQueue.push_back(msg);//may throw bad_alloc
}


void CEvohomeRadio::PopSendQueue(const CEvohomeMsg &msg)
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


void CEvohomeRadio::Send()
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
            Do_Send(out);
			m_SendQueue.pop_front();
		}
	}
}

void CEvohomeRadio::OnConnect()
{
    if(GetControllerID()!=0)//can't proceed without it
        RequestCurrentState();
}

void CEvohomeRadio::Idle_Work()
{
    Send();//FIXME afaik if there is a collision then we should increase the back off period exponentially if it happens again
    if(nStartup<300)//if we haven't got zone names in 300s we auto init them (zone creation blocked until we have something)...
    {
        nStartup++;
        if(nStartup==300)
        {
            if (startup)
            {
                InitControllerName();
                InitZoneNames();
                RequestZoneNames();

                if (GetControllerID() == 0xFFFFFF)  //Check whether multiple controllers have been detected
                {
                    uint8_t MultiControllerCount = 0;
                    for (uint8_t i = 0; i < 5; i++)
                        if (MultiControllerID[i] != 0)
                            MultiControllerCount++;
                    if (MultiControllerCount > 1) // If multiple controllers detected then stop and user required to set controller ID on hardware settings page
                    {
                        _log.Log(LOG_ERROR, "evohome serial: multiple controllers detected!  Please set controller ID in hardware settings.");
                        StopHardware();
                    }
                    else if (MultiControllerCount == 1) // If only 1 controller detected then proceed, otherwise continue searching for controller
                    {
                        Log(true, LOG_STATUS, "evohome serial: Multi-controller check passed. Controller ID: 0x%x", MultiControllerID[0]);
                        SetControllerID(MultiControllerID[0]);
                        startup = false;
                    }
                    RequestCurrentState(); //and also get startup info as this should only happen during initial setup
                }
                else
                    startup = false;
            }
            else//Request each individual zone temperature every 300s as the controller omits multi-room zones
            {
                uint8_t nZoneCount = GetZoneCount();
                for (uint8_t i = 0; i < nZoneCount; i++)
                    RequestZoneTemp(i);
                RequestDHWTemp();  // Request DHW temp from controller as workaround for being unable to identify DeviceID
            }
            if (AllSensors == false) // Check whether individual zone sensors has been activated
            {
                std::vector<std::vector<std::string> > result;

                result = m_sql.safe_query("SELECT HardwareID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit >= 13) AND (Unit <= 24)", m_HwdID, (int)pTypeEvohomeZone);
                if (!result.empty())
                    AllSensors = true;
                // Check if the dummy sensor exists and delete
                result = m_sql.safe_query("SELECT HardwareID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == 'FFFFFF') AND (Type==%d) AND (Unit == 13)", m_HwdID, (int)pTypeEvohomeZone);
                if (!result.empty())
                    m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='FFFFFF' AND (Type==%d) AND (Unit == 13))", m_HwdID, (int)pTypeEvohomeZone);
            }
            if (nStarts < 20)
                nStarts++;
            else if (nStarts == 20) // After 1h all devices should have been detected so re-request zone names and trigger device naming
            {
                RequestZoneNames();
                nStarts++;
            }
            nStartup = 0;
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

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_BindEvohome(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string type = request::findValue(&req, "devtype");
			int HwdID = atoi(idx.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(HwdID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_EVOHOME_SERIAL && pHardware->HwdType != HTYPE_EVOHOME_TCP)
				return;
			CEvohomeRadio *pEvoHW = reinterpret_cast<CEvohomeRadio*>(pHardware);

			int nDevNo = 0;
			int nID = 0;
			if (type == "Relay")
			{
				//get dev count
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT COUNT(*) FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit>=64) AND (Unit<96)", HwdID, (int)pTypeEvohomeRelay);
				int nDevCount = 0;
				if (!result.empty())
				{
					nDevCount = atol(result[0][0].c_str());
				}

				if (nDevCount >= 32)//arbitrary maximum
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of relays reached";
					return;
				}

				nDevNo = nDevCount + 64;
				nID = pEvoHW->Bind(nDevNo, CEvohomeID::devRelay);
			}
			else if (type == "OutdoorSensor")
				nID = pEvoHW->Bind(0, CEvohomeID::devSensor);
			else if (type == "AllSensors")
			{
				// Check if All Sensors has already been activated, if not create a temporary dummy sensor
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT HardwareID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit > 12) AND (Unit <= 24)", HwdID, (int)pTypeEvohomeZone);
				if (result.size() == 0)
				{
					std::string devname = "Zone Temp";
					m_sql.UpdateValue(HwdID, "FFFFFF", 13, pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, "0.0;0.0;Auto", devname);
				}
				root["status"] = "OK";
				root["title"] = "CreateEvohomeSensor";
				root["Used"] = false;
				return;
			}
			else if (type == "ZoneSensor")
			{
				//Check whether any devices already exist
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT MAX(Unit) FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit>=40) AND (Unit<52)", HwdID,(int)pTypeEvohomeZone);
				int nDevCount = 0;
				if (!result.empty())
				{
					nDevCount = atoi(result[0][0].c_str());
				}
				else nDevCount = 39;// If first device, assign Unit=40 (+1 below)

				if (nDevCount == 51)// Allow a maximum of 12 sensors
				{
					root["status"] = "ERR";
					root["message"] = "Maximum number of Zone sensors reached";
					return;
				}

				nDevNo = nDevCount + 1;
				nID = pEvoHW->Bind(nDevNo, CEvohomeID::devZone);
			}
			if (nID == 0)
			{
				root["status"] = "ERR";
				root["message"] = "Timeout when binding device";
				return;
			}

			if (type == "Relay")
			{
				std::string devid(CEvohomeID::GetHexID(nID));

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID,DeviceID,Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", HwdID, devid.c_str());
				if (!result.empty())
				{
					root["status"] = "ERR";
					root["message"] = "Device already exists";
					root["Used"] = true;
					root["Name"] = result[0][2];
					return;
				}

				std::string devname;
				m_sql.UpdateValue(HwdID, devid.c_str(), nDevNo, pTypeEvohomeRelay, sTypeEvohomeRelay, 10, 255, 0, "Off", devname);
				pEvoHW->SetRelayHeatDemand(nDevNo, 0);//initialize heat demand
			}
			else if (type == "ZoneSensor")
			{
				char ID[40];
				sprintf(ID, "%x", nID);

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID,DeviceID,Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", HwdID, ID);
				if (!result.empty())
				{
					root["status"] = "ERR";
					root["message"] = "Device already exists";
					root["Used"] = true;
					root["Name"] = result[0][2];
					return;
				}

				std::string devname; // = "Zone Temp";
				m_sql.UpdateValue(HwdID, ID, nDevNo, pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, "0.0;0.0;Auto", devname);
			}
			root["status"] = "OK";
			root["title"] = "BindEvohome";
			root["Used"] = false;
		}
	}
}
