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
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

extern std::string szUserDataFolder;

enum evoCommands
{
	cmdSysInfo = 0x10e0,
	cmdZoneTemp = 0x30C9,
	cmdZoneName = 0x0004,
	cmdZoneHeatDemand = 0x3150,//Heat demand sent by an individual zone
	cmdZoneInfo = 0x000A,
	cmdZoneWindow = 0x12B0,//Open window/ventilation zone function
	cmdSetPoint = 0x2309,
	cmdSetpointOverride = 0x2349,
	cmdDHWState = 0x1F41,
	cmdDHWTemp = 0x1260,
	cmdControllerMode = 0x2E04,
	cmdControllerHeatDemand = 0x0008,//Heat demand sent by the controller for CH / DHW / Boiler  (F9/FA/FC)
	cmdOpenThermBridge = 0x3220,//OT Bridge Status messages
	cmdOpenThermSetpoint = 0x22D9,//OT Bridge Control Setpoint
	cmdActuatorState = 0x3EF0,
	cmdActuatorCheck = 0x3B00,
	cmdBinding = 0x1FC9,
	cmdExternalSensor = 0x0002,
	cmdDeviceInfo = 0x0418,
	cmdBatteryInfo = 0x1060,
	cmdSync = 0x1F09,
	//0x10a0 //DHW settings sent between controller and DHW sensor can also be requested by the gateway <1:DevNo><2(uint16_t):SetPoint><1:Overrun?><2:Differential>
	//0x0005
	//0x0006
	//0x0404
};

const char  CEvohomeRadio::m_szNameErr[18] = { 0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F };
const int CEvohomeRadio::m_evoToDczControllerMode[8] = { 0,5,1,2,3,-1,-1,4 };//are the hidden modes actually valid?
const int  CEvohomeRadio::m_evoToDczOverrideMode[5] = { zmAuto,-1,zmPerm,-1,zmTmp };//are the hidden modes actually valid?
const uint8_t CEvohomeRadio::m_dczToEvoZoneMode[3] = { 0,2,4 };
const uint8_t CEvohomeRadio::m_dczToEvoControllerMode[6] = { 0,2,3,4,7,1 };

char const CEvohomeMsg::szPacketType[5][8] = { "Unknown","I","RQ","RP","W" };

CEvohomeRadio::CEvohomeRadio(const int ID, const std::string& UserContID)
{
	m_HwdID = ID;
	m_nDevID = 0;
	m_nMyID = 0;
	m_nBindID = 0;
	for (int i = 0; i < 2; i++)
		m_bStartup[i] = true;

	m_nBufPtr = 0;
	m_nSendFail = 0;
	m_nZoneCount = 0;
	m_nControllerMode = 0;
	m_MaxDeviceID = 0;

	AllSensors = false;

	if (!UserContID.empty())
	{
		std::stringstream s_strid(UserContID);
		s_strid >> std::hex >> m_UControllerID;
	}

	RegisterDecoder(cmdZoneTemp, boost::bind(&CEvohomeRadio::DecodeZoneTemp, this, _1));
	RegisterDecoder(cmdSetPoint, boost::bind(&CEvohomeRadio::DecodeSetpoint, this, _1));
	RegisterDecoder(cmdSetpointOverride, boost::bind(&CEvohomeRadio::DecodeSetpointOverride, this, _1));
	RegisterDecoder(cmdDHWState, boost::bind(&CEvohomeRadio::DecodeDHWState, this, _1));
	RegisterDecoder(cmdDHWTemp, boost::bind(&CEvohomeRadio::DecodeDHWTemp, this, _1));
	RegisterDecoder(cmdControllerMode, boost::bind(&CEvohomeRadio::DecodeControllerMode, this, _1));
	RegisterDecoder(cmdSysInfo, boost::bind(&CEvohomeRadio::DecodeSysInfo, this, _1));
	RegisterDecoder(cmdZoneName, boost::bind(&CEvohomeRadio::DecodeZoneName, this, _1));
	RegisterDecoder(cmdZoneHeatDemand, boost::bind(&CEvohomeRadio::DecodeHeatDemand, this, _1));
	RegisterDecoder(cmdOpenThermBridge, boost::bind(&CEvohomeRadio::DecodeOpenThermBridge, this, _1));
	RegisterDecoder(cmdOpenThermSetpoint, boost::bind(&CEvohomeRadio::DecodeOpenThermSetpoint, this, _1));
	RegisterDecoder(cmdZoneInfo, boost::bind(&CEvohomeRadio::DecodeZoneInfo, this, _1));
	RegisterDecoder(cmdControllerHeatDemand, boost::bind(&CEvohomeRadio::DecodeHeatDemand, this, _1));
	RegisterDecoder(cmdBinding, boost::bind(&CEvohomeRadio::DecodeBinding, this, _1));
	RegisterDecoder(cmdActuatorState, boost::bind(&CEvohomeRadio::DecodeActuatorState, this, _1));
	RegisterDecoder(cmdActuatorCheck, boost::bind(&CEvohomeRadio::DecodeActuatorCheck, this, _1));
	RegisterDecoder(cmdZoneWindow, boost::bind(&CEvohomeRadio::DecodeZoneWindow, this, _1));
	RegisterDecoder(cmdExternalSensor, boost::bind(&CEvohomeRadio::DecodeExternalSensor, this, _1));
	RegisterDecoder(cmdDeviceInfo, boost::bind(&CEvohomeRadio::DecodeDeviceInfo, this, _1));
	RegisterDecoder(cmdBatteryInfo, boost::bind(&CEvohomeRadio::DecodeBatteryInfo, this, _1));
	RegisterDecoder(cmdSync, boost::bind(&CEvohomeRadio::DecodeSync, this, _1));
}


CEvohomeRadio::~CEvohomeRadio(void)
{
	m_bIsStarted = false;
}


void CEvohomeRadio::RegisterDecoder(unsigned int cmd, fnc_evohome_decode fndecoder)
{
	m_Decoders.insert(std::pair<int, fnc_evohome_decode >(cmd, fndecoder));
}

bool CEvohomeRadio::StartHardware()
{
	RequestStart();

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
		std::vector<std::string> sd = result[0];
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
		m_nControllerMode = (uint8_t)atoi(sd[2].c_str());
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
	for (int i = 0; i < static_cast<int>(result.size()); i++)
	{
		Log(true, LOG_STATUS, "evohome: Relay: devno=%d demmand=%d", atoi(result[i][0].c_str()), atoi(result[i][4].c_str()));
		m_RelayCheck.insert(tmap_relay_check_pair(static_cast<uint8_t>(atoi(result[i][0].c_str())), _tRelayCheck(boost::get_system_time() - boost::posix_time::minutes(19), static_cast<uint8_t>(atoi(result[i][4].c_str()))))); //allow 1 minute for startup before trying to restore demand
	}

	//Start worker thread
	m_retrycntr = EVOHOME_RETRY_DELAY; //will force reconnect first thing
	m_thread = std::make_shared<std::thread>(&CEvohomeRadio::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;

	return (m_thread != nullptr);
}

std::string CEvohomeMsg::Encode()
{
	std::string szRet;
	char szTmp[1024];
	sprintf(szTmp, "%s - 18:730 %s %s %04X %03d ", szPacketType[type], GetStrID(1).c_str(), GetStrID(2).c_str(), command, payloadsize);
	szRet = szTmp;
	for (int i = 0; i < payloadsize; i++)
	{
		sprintf(szTmp, "%02hhX", payload[i]);
		szRet += szTmp;
	}
	enccount++;
	return szRet;
}


bool CEvohomeRadio::WriteToHardware(const char* pdata, const unsigned char length)
{
	if (!pdata)
		return false;
	switch (pdata[1])
	{
	case pTypeEvohome:
	{
		if (length < sizeof(_tEVOHOME1))
			return false;
		_tEVOHOME1* tsen = (_tEVOHOME1*)pdata;
		AddSendQueue(
			CEvohomeMsg(
				CEvohomeMsg::pktwrt,
				GetControllerID(),
				cmdControllerMode).Add(
					ConvertMode(
						m_dczToEvoControllerMode,
						tsen->status
					)
				).Add(
				(tsen->mode == 1) ? CEvohomeDateTime(tsen) : CEvohomeDateTime()
				).Add(
					tsen->mode
				)
		);
	}
	break;
	case pTypeEvohomeZone:
	{
		if (length < sizeof(_tEVOHOME2))
			return false;
		_tEVOHOME2* tsen = (_tEVOHOME2*)pdata;
		AddSendQueue(
			CEvohomeMsg(
				CEvohomeMsg::pktwrt,
				GetControllerID(),
				cmdSetpointOverride).Add(
				(uint8_t)(tsen->zone - 1)
				).Add(
					tsen->temperature
				).Add(
					ConvertMode(
						m_dczToEvoZoneMode,
						tsen->mode
					)
				).Add(
				(uint16_t)0xFFFF
				).Add(
				(uint8_t)0xFF
				).Add_if(
					CEvohomeDateTime(tsen),
					(tsen->mode == 2)
				)
		);
	}
	break;
	case pTypeEvohomeWater:
	{
		if (length < sizeof(_tEVOHOME2))
			return false;
		_tEVOHOME2* tsen = (_tEVOHOME2*)pdata;
		AddSendQueue(
			CEvohomeMsg(
				CEvohomeMsg::pktwrt,
				GetControllerID(),
				cmdDHWState
			).Add(
			(uint8_t)(tsen->zone - 1)
			).Add(
			(uint8_t)tsen->temperature).Add(
				ConvertMode(
					m_dczToEvoZoneMode,
					tsen->mode)
			).Add(
			(uint16_t)0
			).Add(
			(uint8_t)0
			).Add_if(
				CEvohomeDateTime(tsen),
				(tsen->mode == 2)
			)
		);
	}
	break;
	case pTypeEvohomeRelay:
	{
		if (length < sizeof(_tEVOHOME3))
			return false;
		_tEVOHOME3* tsen = (_tEVOHOME3*)pdata;
		SetRelayHeatDemand(tsen->devno, tsen->demand);
	}
	break;
	}
	return true;
}


void CEvohomeRadio::RequestZoneTemp(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdZoneTemp).Add(nZone));
}


void CEvohomeRadio::RequestZoneName(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdZoneName).Add(nZone).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestZoneInfo(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdZoneInfo).Add(nZone));
}


void CEvohomeRadio::RequestSetPointOverride(uint8_t nZone)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdSetpointOverride).Add(nZone));
}


void CEvohomeRadio::RequestDHWState()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdDHWState).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestDHWTemp()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdDHWTemp).Add(uint8_t(0)));
}


void CEvohomeRadio::RequestControllerMode()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdControllerMode).Add(uint8_t(0xFF)));
}


void CEvohomeRadio::RequestSysInfo()
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdSysInfo).Add(uint8_t(0)));
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
	uint8_t nZoneCount = GetZoneCount();
	for (uint8_t i = 0; i < nZoneCount; i++)
		RequestSetPointOverride(i);
	//Trying this linked to DHW heat demand instead...that won't be adequate do it here too!
	RequestDHWState();
	SendExternalSensor();
	SendZoneSensor();
}


void CEvohomeRadio::RequestZoneNames()
{
	uint8_t nZoneCount = GetZoneCount();
	for (uint8_t i = 0; i < nZoneCount; i++)
		RequestZoneName(i);
}


void CEvohomeRadio::RequestDeviceInfo(uint8_t nAddr)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktreq, GetControllerID(), cmdDeviceInfo).Add(uint8_t(0)).Add(uint8_t(0)).Add(nAddr));
}


void CEvohomeRadio::SendRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, GetGatewayID(), cmdControllerHeatDemand).Add(nDevNo).Add(nDemand));
}

void CEvohomeRadio::UpdateRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	SendRelayHeatDemand(nDevNo, nDemand);
	m_RelayCheck[nDevNo] = _tRelayCheck(boost::get_system_time(), nDemand);
}


void CEvohomeRadio::SetRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand)
{
	if (nDevNo < 64 || nDevNo >= 96 || GetGatewayID() == 0) //atm no reason to interfere with non custom relays
		return;
	std::lock_guard<std::mutex> l(m_mtxRelayCheck);
	UpdateRelayHeatDemand(nDevNo, nDemand);
}


void CEvohomeRadio::CheckRelayHeatDemand()
{
	for (const auto& it : m_RelayCheck)
	{
		if ((boost::get_system_time() - it.second.m_stLastCheck) > boost::posix_time::seconds(1202)) //avg seems around 1202-1203 but not clear how reference point derived
		{
			Log(true, LOG_STATUS, "evohome: Relay: Refreshing heat demand devno=%d demand=%d", it.first, it.second.m_nDemand);
			UpdateRelayHeatDemand(it.first, it.second.m_nDemand);
		}
	}
}


void CEvohomeRadio::SendRelayKeepAlive()
{
	Log(true, LOG_STATUS, "evohome: Relay: Sending keep alive");
	AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, GetGatewayID(), cmdActuatorCheck).Add((uint8_t)0xFC).Add((uint8_t)0xC8));
}


void CEvohomeRadio::SendExternalSensor()
{
	if (GetGatewayID() == 0)
		return;
	double dbTemp = 0.0, dbUV = 0.0;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (Name=='Outside')");//There could be different types depending on how data is received from WU etc.
	if (!result.empty())
	{
		std::vector<std::string> strarray;
		StringSplit(result[0][0], ";", strarray);
		if (!strarray.empty()) {
			dbTemp = atof(strarray[0].c_str());
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, GetGatewayID(), cmdExternalSensor).Add((uint8_t)2).Add(static_cast<int16_t>(dbTemp * 100.0)).Add((uint8_t)1));
		}
		else
			return;
	}
	else
		return;

	//FIXME no light level data available UV from WU is only thing vaguely close (on dev system) without a real sensor
	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (Type==%d)", (int)pTypeUV);
	if (!result.empty()) {
		dbUV = atof(result[0][0].c_str());
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, GetGatewayID(), cmdExternalSensor).Add((uint8_t)0).Add(static_cast<uint16_t>(dbUV * 39)).Add((uint8_t)2));
	}
	else
		return;
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
				AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, ID, cmdZoneTemp).Add((uint8_t)0).Add(static_cast<int16_t>(dbTemp * 100.0)));
				// Update the dummy Temp Zone device with the new temperature
				_tEVOHOME2 tsen;
				memset(&tsen, 0, sizeof(_tEVOHOME2));
				tsen.len = sizeof(_tEVOHOME2) - 1;
				tsen.type = pTypeEvohomeZone;
				tsen.subtype = sTypeEvohomeZone;
				tsen.zone = (uint8_t)i;
				tsen.temperature = static_cast<uint16_t>(dbTemp * 100);
				RFX_SETID3(ID, tsen.id1, tsen.id2, tsen.id3);
				sDecodeRXMessage(this, (const unsigned char*)&tsen, "Zone Temp", -1);
			}
		}
	}
}

bool CEvohomeRadio::HandleLoopData(const char* data, size_t len)
{
	if (m_nBufPtr + len >= m_nBufSize)
	{
		Log(false, LOG_ERROR, "evohome: Buffer overflow");
		m_nBufPtr = 0;
		return false;
	}
	memcpy(m_buf + m_nBufPtr, data, len);
	m_nBufPtr += len;
	m_nBufPtr = ProcessBuf(m_buf, m_nBufPtr);
	return true;
}

int CEvohomeRadio::ProcessBuf(char* buf, int size)
{
	int start = 0;

	for (int i = 0; i < size; ++i)
	{
		if (buf[i] == 0x11)//this appears to be a break character?
			start = i + 1;
		else if (buf[i] == 0x0A)//this is the end of packet marker...not sure if there is a CR before this?
		{
			if (i - start >= 2048) {
				Log(false, LOG_ERROR, "evohome: Message length exceeds max message size");
				start = i + 1;
				continue;
			}
			if (i != start) //skip empty messages
			{
				std::string msg = std::string(buf + start, i - start - 1);
				ProcessMsg(msg.c_str());
			}
			start = i + 1;
		}
	}
	if (start > 0)
		memmove(buf, buf + start, size - start);
	return size - start;
}


bool CEvohomeMsg::DecodePacket(const char* rawmsg)
{
	int cmdidx = 0;
	int nid = 0;
	std::string line(rawmsg);
	std::vector<std::string> tkns;
	boost::split(tkns, line, boost::is_any_of(" "), boost::token_compress_on);//There are sometimes 2 spaces between token 1 and 2 so we use token_compress_on to avoid an additional empty token
	for (int i = 0; i < (int)tkns.size(); i++)
	{
		std::string tkn(tkns[i]);
		if (i == 0)//this is some sort of status or flags but not sure what exactly
		{
		}
		else if (i == 1)
		{
			if (SetPacketType(tkn) != pktunk)
				SetFlag(flgpkt);
		}
		else if (cmdidx && i == cmdidx + 2) //payload starts 2 after command idx
		{
			if (!GetFlag(flgps))
			{
				CEvohomeRadio::Log(false, LOG_ERROR, "evohome: no payload size - possible corrupt message");
				return false;
			}
			if (tkn.length() % 2)
			{
				CEvohomeRadio::Log(false, LOG_ERROR, "evohome: uneven payload - possible corrupt message");
				return false;
			}
			if (tkn.length() / 2 > m_nBufSize)
			{
				CEvohomeRadio::Log(false, LOG_ERROR, "evohome: payload exceeds max buffer size");
				return false;
			}
			int ps = 0;
			for (size_t j = 0; j < tkn.length(); j += 2)
			{
				unsigned int nHex;
				sscanf(tkn.substr(j, 2).c_str(), "%02x", &nHex);
				payload[ps++] = static_cast<uint8_t>(nHex);
			}
			if (ps == payloadsize)
			{
				SetFlag(flgpay);
			}
			else
			{
				CEvohomeRadio::Log(false, LOG_ERROR, "evohome: payload size does not match specified size from packet header");
				return false;
			}

		}
		else if (tkn.length() == 4)
		{
			sscanf(tkn.c_str(), "%04x", &command);
			SetFlag(flgcmd);
			if (!cmdidx)
				cmdidx = i;
		}
		else if (tkn.length() == 3)
		{
			if (tkn.find('-') == std::string::npos)
			{
				if (i == 2)
				{
					timestamp = (uint8_t)atoi(tkn.c_str());
					SetFlag(flgts);
				}
				else if (cmdidx && i == cmdidx + 1)
				{
					payloadsize = (uint8_t)atoi(tkn.c_str());
					SetFlag(flgps);
				}
			}
		}
		else
		{
			int nPos = tkn.find(':');
			if (nPos != std::string::npos)
			{
				if (nid >= 3)
				{
					CEvohomeRadio::Log(false, LOG_ERROR, "evohome: too many message ids - possible corrupt message");
					continue;
				}
				if (tkn.find('-') == std::string::npos)
				{
					id[nid] = tkn;
					SetFlag(flgid1 << nid);
				}
				nid++;
			}
			else
				CEvohomeRadio::Log(false, LOG_STATUS, "evohome: WARNING unrecognised message structure - possible corrupt message '%s' (%d)", tkn.c_str(), i);
		}
	}
	return IsValid();
}


void CEvohomeRadio::ProcessMsg(const char* rawmsg)
{
	CEvohomeMsg msg(rawmsg);
	Log(rawmsg, msg);
	if (msg.IsValid())
	{
		if (GetControllerID() == 0xFFFFFF) // If we still have a dummy controller update the controller DeviceID list
		{
			for (int n = 0; n < 3; n++)
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
		if (msg.id[0].GetIDType() == CEvohomeID::devGateway) //if we just got an echo of a sent packet we don't need to process it
		{
			if (!m_nMyID)//If we haven't got our own id trap it here just in case we need it later on
				m_nMyID = msg.GetID(0);
			PopSendQueue(msg);
		}
		else
			DecodePayload(msg);
	}
	else
		Log(true, LOG_ERROR, "evohome: invalid message structure - possible corrupt message");
}


bool CEvohomeRadio::DecodePayload(CEvohomeMsg& msg)
{
	std::map < unsigned int, fnc_evohome_decode >::iterator pf = m_Decoders.find(msg.command);
	if (pf != m_Decoders.end())
	{
		bool ret = pf->second(msg);
		if (!ret)
			Log(false, LOG_ERROR, "evohome: unable to decode payload for command %04x", msg.command);
		return ret;
	}
	Log(true, LOG_ERROR, "evohome: unknown command %04x", msg.command);
	return false;
}


bool CEvohomeRadio::DumpMessage(CEvohomeMsg& msg)
{
	char tag[] = "DUMP_MSG";
	Log(true, LOG_STATUS, "evohome: %s: CMD=%04x Len=%d", tag, msg.command, msg.payloadsize);
	std::string strpayload, strascii;
	char szTmp[1024];
	for (int i = 0; i < msg.payloadsize; i++)
	{
		sprintf(szTmp, "%02hhx", msg.payload[i]);
		strpayload += szTmp;
		sprintf(szTmp, "%c", msg.payload[i]);
		strascii += szTmp;
	}
	Log(true, LOG_STATUS, "evohome: %s: payload=%s (ASCII)=%s", tag, strpayload.c_str(), strascii.c_str());
	return true;
}


int CEvohomeRadio::Bind(uint8_t nDevNo, unsigned char nDevType)//use CEvohomeID::devType atm but there could be additional specialisations
{
	int nGatewayID = GetGatewayID();
	if (nGatewayID == 0)
		return 0;
	uint8_t ID1;
	uint8_t ID2;
	uint8_t ID3;

	if (nDevType == CEvohomeID::devRelay)//Binding a relay to the HGI80
	{
		std::unique_lock<std::mutex> lock(m_mtxBindNotify);
		m_nBindID = 0;
		m_nBindIDType = nDevType;
		/*if(!m_cndBindNotify.wait_for(lock,boost::posix_time::seconds(60)))
			return 0;*/
		boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(60);//monotonic?
		while (m_nBindID == 0)
		{
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, nGatewayID, cmdBinding).Add(nDevNo).Add((uint16_t)cmdControllerHeatDemand).Add(CEvohomeID(nGatewayID)).Add((uint8_t)0xFC).Add((uint16_t)cmdActuatorCheck).Add(CEvohomeID(nGatewayID)).Add((uint8_t)nDevNo).Add((uint16_t)cmdBinding).Add(CEvohomeID(nGatewayID)));
			if (m_cndBindNotify.wait_for(lock, std::chrono::duration<int>(5)) == std::cv_status::timeout && boost::get_system_time() > timeout)
				return 0;
		}

		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, m_nBindID, cmdBinding).Add((uint8_t)0).Add((uint16_t)0xFFFF).Add(CEvohomeID(nGatewayID)));
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, nGatewayID, 0x0009).Add(nDevNo).Add((uint8_t)0).Add((uint8_t)0xFF)); //not sure if 1st byte is devno 2nd byte can be 00 or 01
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, nGatewayID, 0x1100).Add((uint8_t)0xFC).Add((uint16_t)0x1804).Add((uint16_t)0).Add((uint16_t)0x7FFF).Add((uint8_t)0x01)); //probably relay settings.. usually sent after cmd 0x0009

		return m_nBindID;
	}
	else if (nDevType == CEvohomeID::devSensor)//Binding the HGI80 to the evohome controller as an outdoor sensor (there are other sensors)
	{
		std::unique_lock<std::mutex> lock(m_mtxBindNotify);
		m_nBindID = 0;
		m_nBindIDType = CEvohomeID::devController;

		boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(60);//monotonic?
		while (m_nBindID == 0)
		{
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, nGatewayID, cmdBinding).Add((uint8_t)0).Add((uint16_t)cmdExternalSensor).Add(CEvohomeID(nGatewayID)).Add((uint8_t)2).Add((uint16_t)cmdExternalSensor).Add(CEvohomeID(nGatewayID)).Add((uint8_t)0).Add((uint16_t)cmdBinding).Add(CEvohomeID(nGatewayID)));
			if (m_cndBindNotify.wait_for(lock, std::chrono::duration<int>(5)) == std::cv_status::timeout && boost::get_system_time() > timeout)
				return 0;
		}
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, m_nBindID, cmdBinding).Add((uint8_t)0).Add((uint16_t)0xFFFF).Add(CEvohomeID(nGatewayID)));
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
		std::unique_lock<std::mutex> lock(m_mtxBindNotify);
		m_nBindID = 0;
		m_nBindIDType = CEvohomeID::devController;
		RFX_SETID3(m_MaxDeviceID, ID1, ID2, ID3);

		boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(60);//monotonic?
		while (m_nBindID == 0)
		{
			AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, 0, nGatewayID, cmdBinding).Add((uint8_t)0).Add((uint16_t)cmdZoneTemp).Add(ID1).Add(ID2).Add(ID3).Add((uint8_t)2).Add((uint16_t)cmdZoneTemp).Add(ID1).Add(ID2).Add(ID3).Add((uint8_t)0).Add((uint16_t)cmdBinding).Add(CEvohomeID(nGatewayID)));
			if (m_cndBindNotify.wait_for(lock, std::chrono::duration<int>(5)) == std::cv_status::timeout && boost::get_system_time() > timeout)
				return 0;
		}
		AddSendQueue(CEvohomeMsg(CEvohomeMsg::pktinf, m_nBindID, cmdBinding).Add((uint8_t)0).Add((uint16_t)0x30C9).Add(ID1).Add(ID2).Add(ID3));
		return m_MaxDeviceID;
	}
	return 0;
}


bool CEvohomeRadio::DecodeSetpoint(CEvohomeMsg& msg)//0x2309
{
	char tag[] = "ZONE_SETPOINT";

	if (msg.GetID(0) != GetControllerID() && msg.GetID(2) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for zone %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize % 3 != 0) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding zone setpoint payload, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	tsen.type = pTypeEvohomeZone;
	tsen.subtype = sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3) //this message can be received from other than the controller...is zone always valid though? (when a local override is in action we get different values one from the controller the other from the zone valve but the zone number is still ok)
		tsen.mode = zmNotSp;
	tsen.updatetype = updSetPoint;//setpoint

	for (int i = 0; i < msg.payloadsize; i += 3) {
		tsen.zone = msg.payload[i] + 1;
		tsen.temperature = msg.payload[i + 1] << 8 | msg.payload[i + 2];
		if (tsen.temperature == 0x7FFF)
		{
			Log(true, LOG_STATUS, "evohome: %s: Warning setpoint not set for zone %d", tag, msg.payload[0]);
			continue;
		}
		SetMaxZoneCount(tsen.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
		Log(true, LOG_STATUS, "evohome: %s: Setting: %d: %d", tag, tsen.zone, tsen.temperature);
		//It appears that the controller transmits the current setpoint for all zones periodically this is presumably so
		//the zone controller can update to any changes as required
		//The zone controllers also individually transmit their own setpoint as it is currently set
		//presumably this will ordinarily update to correspond to the transmitted value from the controller
		//The exception appears to be for local overrides which may be possible to track by seeing if a change
		//occurs that does not correspond to the controller setpoint for a given zone
		if (msg.GetID(0) == GetControllerID())
			sDecodeRXMessage(this, (const unsigned char*)&tsen, "Setpoint", -1);
		else if (AllSensors)
		{
			char zstrname[40];
			sprintf(zstrname, "Zone %d", tsen.zone);
			tsen.zone += 12; //zone number offset by 12
			Log(true, LOG_STATUS, "evohome: %s: Setting: %d (0x%x): %d", tag, tsen.zone, msg.GetID(0), tsen.temperature);
			sDecodeRXMessage(this, (const unsigned char*)&tsen, zstrname, -1);
		}
	}
	return true;
}


bool CEvohomeRadio::DecodeSetpointOverride(CEvohomeMsg& msg)//0x2349
{
	char tag[] = "ZONE_SETPOINT_MODE";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for zone %d", tag, msg.payload[0]);
		return true;
	}
	//reply is 7 bytes or 13 bytes with a date?
	if (msg.payloadsize != 7 && msg.payloadsize != 13) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding payload unknown size: %d", tag, msg.payloadsize);
		return false;
	}
	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	tsen.type = pTypeEvohomeZone;
	tsen.subtype = sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3) //will be id of controller so must use zone number

		tsen.zone = msg.payload[0] + 1;//controller is 0 so let our zones start from 1...
	if (tsen.zone > m_nMaxZones)
	{
		Log(false, LOG_ERROR, "evohome: %s: Error zone number out of bounds: %d", tag, tsen.zone);
		return false;
	}
	tsen.updatetype = updSetPoint;//setpoint
	tsen.temperature = msg.payload[1] << 8 | msg.payload[2];
	if (tsen.temperature == 0x7FFF)
	{
		Log(true, LOG_STATUS, "evohome: %s: Warning setpoint not set for zone %d", tag, tsen.zone);
		return true;
	}
	SetMaxZoneCount(tsen.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
	if (m_ZoneOverrideLocal[tsen.zone - 1] == zmWind || m_ZoneOverrideLocal[tsen.zone - 1] == zmLocal)
	{
		Log(true, LOG_STATUS, "evohome: %s: A local override is in effect for zone %d", tag, tsen.zone);
		return true;
	}
	int nMode = ConvertMode(m_evoToDczOverrideMode, msg.payload[3]);
	if (nMode == -1)
	{
		Log(false, LOG_STATUS, "evohome: %s: WARNING unexpected mode %d", tag, msg.payload[3]);
		return false;
	}
	tsen.mode = static_cast<uint8_t>(nMode);
	tsen.controllermode = (uint8_t)ConvertMode(m_evoToDczControllerMode, GetControllerMode());
	if (msg.payloadsize == 13)
	{
		CEvohomeDateTime::DecodeDateTime(tsen, msg.payload, 7);
		Log(true, LOG_STATUS, "evohome: %s: Setting: %d (0x%x): %d (%d=%s) %s", tag, tsen.zone, msg.GetID(0), tsen.temperature, tsen.mode, GetZoneModeName(tsen.mode), CEvohomeDateTime::GetStrDate(tsen).c_str());
	}
	else
	{
		Log(true, LOG_STATUS, "evohome: %s: Setting: %d (0x%x): %d (%d=%s)", tag, tsen.zone, msg.GetID(0), tsen.temperature, tsen.mode, GetZoneModeName(tsen.mode));
	}

	sDecodeRXMessage(this, (const unsigned char*)&tsen, "Setpoint", -1);
	return true;
}


bool CEvohomeRadio::DecodeZoneTemp(CEvohomeMsg& msg)//0x30C9
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


	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for zone temp %d", tag, msg.payload[0]);
		return true;
	}
	//blocks of 3 bytes
	if (msg.payloadsize < 3) {//check below does not trap 0 byte payload
		Log(false, LOG_ERROR, "evohome: %s: Error decoding zone temperature payload, size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 3 != 0) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding zone temperature payload, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}

	bool bRefresh = false;
	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	tsen.type = pTypeEvohomeZone;
	tsen.subtype = sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3);
	for (int i = 0; i < msg.payloadsize; i += 3) {
		//if this is broadcast direct from the sensor then the zoneID is always 0 and we need to match on the device id if possible (use zone 0 to indicate this)
		//otherwise use the zone number...controller is 0 so let our zones start from 1...
		if (msg.GetID(0) == GetControllerID())
			tsen.zone = msg.payload[i] + 1;
		else
			tsen.zone = 0;
		tsen.temperature = msg.payload[i + 1] << 8 | msg.payload[i + 2];
		//this is sent for zones that use a zone temperature instead of the internal sensor.
		Log(true, LOG_STATUS, "evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.zone, tsen.temperature);
		if (tsen.temperature != 0x7FFF)//afaik this is the error value just ignore it right now as we have no way to report errors...also perhaps could be returned if DHW is not installed?
		{
			sDecodeRXMessage(this, (const unsigned char*)&tsen, "Zone Temp", -1);
			if (msg.GetID(0) == GetControllerID())
				bRefresh = SetMaxZoneCount(tsen.zone);//this should increase on startup as we poll all zones so we don't respond to changes here
		}
	}

	if (msg.GetID(0) == GetControllerID()) {
		//msg from main unit..will be id of controller so must use zone number
		//DHW broadcasts on change?...I think DHW temp broadcasts when it changes but not the DHW state unless it's been manually changed on the controller
		if (msg.type == CEvohomeMsg::pktinf)
		{
			if (bRefresh) //this should only change when we're running normally to allow detection of new zones
				RequestZoneNames();//This can conflict with our startup polling but will still succeed ok
			RequestZoneState();//This can conflict with our startup polling but will still succeed ok
		}
	}
	else {//no zone number here so we'd need to do a lookup based on the ID to use this
		//msg from sensor itself
		if (AllSensors)
		{
			std::string zstrid(CEvohomeID::GetHexID(msg.GetID(0)));

			result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q') AND (Type == %d)", m_HwdID, zstrid.c_str(), (int)pTypeEvohomeZone);
			if (!result.empty()) // Update existing temp sensor with value directly from sensor
			{
				tsen.zone = (uint8_t)atoi(result[0][0].c_str());
				Log(true, LOG_STATUS, "evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.zone, tsen.temperature);
				sDecodeRXMessage(this, (const unsigned char*)&tsen, "Zone Temp", -1);
			}
			else // If matching relay with same deviceID then create a new Zone Temp sensor with zone number offset by 12
			{
				result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q') AND (Type == %d)", m_HwdID, zstrid.c_str(), (int)pTypeEvohomeRelay);
				if (!result.empty())
				{
					tsen.zone = (uint8_t)(atoi(result[0][0].c_str()) + 12);
					char zstrname[40];
					sprintf(zstrname, "Zone %d", atoi(result[0][0].c_str()));
					Log(true, LOG_STATUS, "evohome: %s: Zone sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.zone, tsen.temperature);
					sDecodeRXMessage(this, (const unsigned char*)&tsen, zstrname, -1);
				}
			}
		}
		if (msg.payloadsize != 3) {
			Log(true, LOG_STATUS, "evohome: %s: WARNING: got a sensor temperature msg with an unexpected payload size: %d", tag, msg.payloadsize);
		}
		//in this case the zoneID is always 0 and hence is worthless
		if (msg.payload[0] != 0) {
			Log(true, LOG_STATUS, "evohome: %s: WARNING: sensor reading with zone != 0: 0x%x - %d", tag, msg.GetID(0), msg.payload[0]);
		}
	}
	if ((msg.GetID(0) > m_MaxDeviceID))
		m_MaxDeviceID = msg.GetID(0);

	return true;
}


bool CEvohomeRadio::DecodeDHWState(CEvohomeMsg& msg)//1F41
{
	char tag[] = "DHW_STATE";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for DHW state %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 6 && msg.payloadsize != 12) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding DHW state / mode, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}

	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	tsen.type = pTypeEvohomeWater;
	tsen.subtype = sTypeEvohomeWater;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3) //will be id of controller so must use zone number

		tsen.zone = msg.payload[0] + 1;////NA to DHW...controller is 0 so let our zones start from 1...
	tsen.updatetype = updSetPoint;//state
	tsen.temperature = msg.payload[1];//just on or off for DHW
	if (tsen.temperature == 0xFF)// temperature = 255 if DHW not installed
		return true;
	int nMode = ConvertMode(m_evoToDczOverrideMode, msg.payload[2]);
	if (nMode == -1)
	{
		Log(false, LOG_STATUS, "evohome: %s: WARNING unexpected mode %d", tag, msg.payload[2]);
		return false;
	}
	tsen.mode = static_cast<uint8_t>(nMode);
	tsen.controllermode = (uint8_t)ConvertMode(m_evoToDczControllerMode, GetControllerMode());
	if (msg.payloadsize == 12)
	{
		CEvohomeDateTime::DecodeDateTime(tsen, msg.payload, 6);
		Log(true, LOG_STATUS, "evohome: %s: Setting: %d: %d (%d=%s) %s", tag, tsen.zone, tsen.temperature, tsen.mode, GetZoneModeName(tsen.mode), CEvohomeDateTime::GetStrDate(tsen).c_str());
	}
	else
	{
		Log(true, LOG_STATUS, "evohome: %s: Setting: %d: %d (%d=%s)", tag, tsen.zone, tsen.temperature, tsen.mode, GetZoneModeName(tsen.mode));
	}

	sDecodeRXMessage(this, (const unsigned char*)&tsen, "DHW", -1);
	return true;
}


bool CEvohomeRadio::DecodeDHWTemp(CEvohomeMsg& msg)//1260
{
	char tag[] = "DHW_TEMP";

	// Filter out messages from other controllers
	// This also filters out messages sent direct from sensor as can't uniquely identify its DeviceID so added workaround of regularly requesting temp from controller
	if (msg.GetID(0) != GetControllerID())
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for DHW temp %d", tag, msg.payload[0]);
		return true;
	}
	//blocks of 3 bytes...only ever seen a 3 byte message with DevNo 0x00
	if (msg.payloadsize < 3) {//check below does not trap 0 byte payload
		Log(false, LOG_ERROR, "evohome: %s: Error decoding DHW temperature payload, size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 3 != 0) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding DHW temperature payload, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize != 3) {
		Log(true, LOG_STATUS, "evohome: %s: WARNING: got a sensor temperature msg with an unexpected payload size: %d", tag, msg.payloadsize);
	}
	//for DHW DevNo is always 0 and not relevant
	if (msg.payload[0] != 0) {
		Log(true, LOG_STATUS, "evohome: %s: WARNING: sensor reading with zone != 0: 0x%x - %d", tag, msg.GetID(0), msg.payload[0]);
	}

	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	tsen.type = pTypeEvohomeWater;
	tsen.subtype = sTypeEvohomeWater;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3);

	for (int i = 0; i < msg.payloadsize; i += 3) {
		tsen.zone = msg.payload[i] + 1;//we're using zone 0 to trigger a lookup on ID rather than zone number (not relevant for DHW)
		tsen.temperature = msg.payload[i + 1] << 8 | msg.payload[i + 2];
		Log(true, LOG_STATUS, "evohome: %s: DHW sensor msg: 0x%x: %d: %d", tag, msg.GetID(0), tsen.zone, tsen.temperature);
		if (tsen.temperature != 0x7FFF)// DHW is not installed, discard value
			sDecodeRXMessage(this, (const unsigned char*)&tsen, "DHW Temp", -1);
	}

	return true;
}


bool CEvohomeRadio::DecodeControllerMode(CEvohomeMsg& msg)//2E04
{
	char tag[] = "CONTROLLER_MODE";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for controller mode %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 8) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding controller mode, size incorrect: %d", tag, msg.payloadsize);
		return false;
	}

	_tEVOHOME1 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME1));
	tsen.len = sizeof(_tEVOHOME1) - 1;
	tsen.type = pTypeEvohome;
	tsen.subtype = sTypeEvohome;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3);

	int nControllerMode = msg.payload[0];
	int nMode = ConvertMode(m_evoToDczControllerMode, (uint8_t)nControllerMode);//this converts to the modes originally setup with the web client ver
	if (nMode == -1)
	{
		Log(false, LOG_STATUS, "evohome: %s: WARNING unexpected mode %d", tag, nControllerMode);
		return false;
	}
	tsen.status = static_cast<uint8_t>(nMode);
	CEvohomeDateTime::DecodeDateTime(tsen, msg.payload, 1);
	tsen.mode = msg.payload[7];//1 is tmp 0 is perm
	Log(true, LOG_STATUS, "evohome: %s: Setting: (%d=%s) (%d=%s) %s", tag, tsen.status, GetControllerModeName(tsen.status), tsen.mode, tsen.mode ? "Temporary" : "Permanent", CEvohomeDateTime::GetStrDate(tsen).c_str());
	sDecodeRXMessage(this, (const unsigned char*)&tsen, "Controller Mode", -1);

	if (SetControllerMode((uint8_t)nControllerMode))//if only the until time changed we should be ok as the unit will broadcast a new controller mode when the current mode ends
		RequestZoneState();//This can conflict with our startup polling but will still succeed ok
	return true;
}


bool CEvohomeRadio::DecodeSysInfo(CEvohomeMsg& msg)//10e0
{
	char tag[] = "SYSINFO";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for sysinfo %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 38) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding sysinfo, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	//Not sure what the first 10 bytes are for..some are masked anyway...the first bytes were always 000002FF in the captures I saw
	int nAppVer = msg.payload[5]; // does byte 4 go with the 1st date
	CEvohomeDate edt, edtp2;
	msg.Get(edt, 10).Get(edtp2);
	msg.payload[38] = '\0';//presumably not null terminated if name consumes all available bytes in the payload
	SetControllerName((const char*)&msg.payload[msg.GetPos()]);
	Log(false, LOG_STATUS, "evohome: %s: d1 %s App Ver %d (%s) Name %s", tag, edt.GetStrDate().c_str(), nAppVer, edtp2.GetStrDate().c_str(), &msg.payload[msg.GetPos()]);
	return true;
}


bool CEvohomeRadio::DecodeZoneName(CEvohomeMsg& msg)
{
	char tag[] = "ZONE_NAME";
	std::vector<std::vector<std::string> > result;

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 2) {
		Log(true, LOG_STATUS, "evohome: %s: Request for zone name %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize != 22) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding zone name, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	if (memcmp(&msg.payload[2], m_szNameErr, 18) == 0)
	{
		Log(true, LOG_STATUS, "evohome: %s: Warning zone name not set: %d", tag, msg.payload[0] + 1);
		m_bStartup[0] = false;
		return true;
	}
	msg.payload[22] = '\0';//presumably not null terminated if name consumes all available bytes in the payload
	int nZone = msg.payload[0] + 1;
	SetMaxZoneCount((uint8_t)nZone);//this should increase on startup as we poll all zones so we don't respond to changes here
	SetZoneName(msg.payload[0], (const char*)&msg.payload[2]);
	Log(true, LOG_STATUS, "evohome: %s: %d: Name %s", tag, nZone, &msg.payload[2]);
	if (m_bStartup[0] && nZone < m_nMaxZones)
		RequestZoneStartupInfo((uint8_t)nZone);

	// Update any zone names which are still the defaults
	result = m_sql.safe_query("SELECT Name FROM Devicestatus WHERE ((HardwareID==%d) AND (Type==%d) AND (Unit == %d) AND (Name == 'Zone Temp')) OR ((HardwareID==%d) AND (Type==%d) AND (Unit == %d) AND (Name == 'Setpoint'))", m_HwdID, (int)pTypeEvohomeZone, nZone, m_HwdID, (int)pTypeEvohomeZone, nZone);
	if (!result.empty())
		m_sql.safe_query("UPDATE Devicestatus SET Name='%q' WHERE (HardwareID==%d) AND (Type==%d) AND (Unit == %d)", (const char*)&msg.payload[2], m_HwdID, (int)pTypeEvohomeZone, nZone);
	result = m_sql.safe_query("SELECT Name FROM Devicestatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit == %d) AND (Name == 'Zone')", m_HwdID, (int)pTypeEvohomeRelay, nZone);
	if (!result.empty())
		m_sql.safe_query("UPDATE Devicestatus SET Name='%q' WHERE (HardwareID==%d) AND (Type==%d) AND (Unit == %d)", (const char*)&msg.payload[2], m_HwdID, (int)pTypeEvohomeRelay, nZone);

	return true;
}


bool CEvohomeRadio::DecodeZoneInfo(CEvohomeMsg& msg)
{
	char tag[] = "ZONE_INFO";

	if (msg.GetID(0) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for zone info %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize < 6) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding zone info, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 6 != 0) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding zone info, incorrect packet size: %d", tag, msg.payloadsize);
		return false;
	}
	for (int i = 0; i < msg.payloadsize; i += 6) {
		uint8_t nZone, nFlags;
		CEvohomeTemp min, max;
		msg.Get(nZone).Get(nFlags).Get(min).Get(max);
		nZone++;
		if (!min.IsValid() && !max.IsValid())
		{
			Log(true, LOG_STATUS, "evohome: %s: Warning zone info not set %d", tag, nZone);
			continue;
		}
		SetMaxZoneCount(nZone);//this should increase on startup as we poll all zones so we don't respond to changes here
		Log(true, LOG_STATUS, "evohome: %s: %d: Min %.1f Max %.1f Flags %d %s", tag, nZone, min.GetTemp(), max.GetTemp(), nFlags, CEvohomeZoneFlags::GetFlags(nFlags).c_str());
	}

	return true;
}


bool CEvohomeRadio::DecodeZoneWindow(CEvohomeMsg& msg)
{
	char tag[] = "ZONE_WINDOW";

	if (msg.GetID(0) != GetControllerID() && msg.GetID(2) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize == 1) {
		Log(true, LOG_STATUS, "evohome: %s: Request for zone window %d", tag, msg.payload[0]);
		return true;
	}
	if (msg.payloadsize < 3) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	tsen.type = pTypeEvohomeZone;
	tsen.subtype = sTypeEvohomeZone;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3)

		uint8_t nWindow, nMisc;
	msg.Get(tsen.zone).Get(nWindow).Get(nMisc);//not sure what the last byte is seems to always be 0
	if (tsen.zone >= m_nMaxZones)
	{
		Log(false, LOG_ERROR, "evohome: %s: Error zone number out of bounds: %d", tag, tsen.zone + 1);
		return false;
	}
	tsen.mode = (uint8_t)(nWindow ? zmWind : zmAuto);
	tsen.updatetype = updOverride;//zone modde override (window / local)
	m_ZoneOverrideLocal[tsen.zone] = static_cast<zoneModeType>(tsen.mode);

	tsen.zone++;
	SetMaxZoneCount(tsen.zone);//this should increase on startup as we poll all zones so we don't respond to changes here

	if (nWindow != 0 && nWindow != 0xC8)
		Log(true, LOG_STATUS, "evohome: %s: Unexpected zone state Window=%d", tag, nWindow);
	if (nMisc != 0)
		Log(true, LOG_STATUS, "evohome: %s: Unexpected zone state nMisc=%d", tag, nMisc);
	Log(true, LOG_STATUS, "evohome: %s: %d: Window %d", tag, tsen.zone, nWindow);

	if (msg.GetID(0) == GetControllerID())
		sDecodeRXMessage(this, (const unsigned char*)&tsen, "Zone Window", -1);

	return true;
}


bool CEvohomeRadio::DecodeBinding(CEvohomeMsg& msg)
{
	char tag[] = "BINDING";
	if (msg.payloadsize < 6) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding binding, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 6 != 0) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding binding, incorrect packet size: %d", tag, msg.payloadsize);
		return false;
	}
	for (int i = 0; i < msg.payloadsize; i += 6) {
		uint8_t nDevNo;
		uint16_t nCmd;
		CEvohomeID idDev;
		msg.Get(nDevNo).Get(nCmd).Get(idDev);
		Log(true, LOG_STATUS, "evohome: %s: Dev No %d: Cmd 0x%04x DeviceID 0x%06x (%s)", tag, nDevNo, nCmd, idDev.GetID(), idDev.GetStrID().c_str());
	}
	if (msg.type == CEvohomeMsg::pktwrt)
	{
		std::lock_guard<std::mutex> lock(m_mtxBindNotify);
		if (msg.id[0].GetIDType() == m_nBindIDType)
		{
			m_nBindID = msg.GetID(0);
			m_cndBindNotify.notify_one();
		}
	}

	return true;
}


void CEvohomeRadio::RXRelay(uint8_t nDevNo, uint8_t nDemand, int nID)
{
	_tEVOHOME3 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME3));
	tsen.len = sizeof(_tEVOHOME3) - 1;
	tsen.type = pTypeEvohomeRelay;
	tsen.subtype = sTypeEvohomeRelay;
	RFX_SETID3(nID, tsen.id1, tsen.id2, tsen.id3);
	tsen.devno = nDevNo;
	tsen.demand = nDemand;
	tsen.updatetype = CEvohomeRadio::updDemand;
	sDecodeRXMessage(this, (const unsigned char*)&tsen, NULL, -1);
}

bool CEvohomeRadio::DecodeHeatDemand(CEvohomeMsg& msg)
{
	//there is an interesting page on temperature control @ http://newton.ex.ac.uk/teaching/cdhw/Feedback/ControlTypes.html
	char tag[] = "HEAT_DEMAND";

	if (msg.GetID(0) != GetControllerID() && msg.GetID(2) != GetControllerID()) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize != 2) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding heat demand, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	int nDevNo = msg.payload[0];
	int nDemand = msg.payload[1];
	std::string szSourceType("Unknown"), szDevType("Unknown");
	if (msg.command == 0x0008)
	{
		//Demand (0x0008 for relays) is handled as a % (i.e. nDemand/200*100) (200=0xc8) where 200/200 corresponds to 100% of the cycle period (start of period determined by 3B00 ACTUATOR_CHECK message)
		//So where 100% is always on then 50% is half on half off per cycle i.e. 6 cycles per hour = 10 minutes per cycle with 5 mins on & 5 mins off etc,
		//presumably there are some settings (0x1100) that give the number of cycles per hour which would determine the length of each period
		if (msg.GetID(0) == GetControllerID())
			szSourceType = "Controller";
		else if (msg.GetID(0) == GetGatewayID())
			szSourceType = "Gateway";
	}
	else if (msg.command == 0x3150) //heat demand for zone is also a % (as above) of the proportional band (a temperature difference) so if the proportional band is 2 degrees and we are 1 degree from set point the demand is 50%
	{
		if (nDevNo <= 12)
			nDevNo++; //Need to add 1 to give correct zone numbers
		RXRelay(static_cast<uint8_t>(nDevNo), static_cast<uint8_t>(nDemand), msg.GetID(0)); // adding DeviceID creates multiple devices for zones with mutiple sensors
		szSourceType = "Zone";
	}

	if (nDevNo == 0xfc)//252...afaik this is also some sort of broadcast channel so maybe there is more to this device number than representing the boiler
	{
		szDevType = "Boiler"; //Boiler demand
		if (msg.command == 0x0008) //discard this type of boiler demand message sent from the controller as they are always zero and the 0x3150 messages contain actual demand
			return true;
	}
	else if (nDevNo == 0xfa)//250
	{
		szDevType = "DHW"; //DHW zone valve
		RequestDHWState();//Trying to link the DHW state refresh to the heat demand...will be broadcast when the controller turns on or off DHW but maybe not if the heat demand doesn't change? (e.g. no heat demand but controller DHW state goes from off to on)
	}
	else if (nDevNo == 0xf9)//249
		szDevType = "Heating"; //Central Heating zone valve

	if (msg.command == 0x0008)
	{
		if (nDevNo < 12)
			nDevNo++; //Need to add 1 to give correct zone numbers
		RXRelay(static_cast<uint8_t>(nDevNo), static_cast<uint8_t>(nDemand), msg.GetID(0));
	}

	Log(true, LOG_STATUS, "evohome: %s: %s (0x%x) DevNo 0x%02x %d (0x%x)", tag, szSourceType.c_str(), msg.GetID(0), nDevNo, nDemand, msg.command);

	return true;
}

void CEvohomeRadio::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string& defaultname)
{
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szIdx);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
			return;
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = 1;
	int level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = (uint8_t)level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char*)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

bool CEvohomeRadio::DecodeActuatorCheck(CEvohomeMsg& msg)
{
	char tag[] = "ACTUATOR_CHECK"; //this is used to synchronise time periods for each relay bound to a controller i.e. all relays get this message and use it to determine when to start each cycle (demand is just a % of the cycle length)
	if (msg.payloadsize != 2) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	int nDevNo = msg.payload[0];
	int nDemand = msg.payload[1];
	std::string szDevType("Unknown");

	if (nDevNo == 0xfc)//252...afaik this is also some sort of broadcast channel so maybe there is more to this device number than representing the boiler
		szDevType = "All";// not sure if any other device would be valid here??

	Log(true, LOG_STATUS, "evohome: %s: DevNo 0x%02x %s: %d", tag, nDevNo, szDevType.c_str(), nDemand);
	return true;
}


bool CEvohomeRadio::DecodeActuatorState(CEvohomeMsg& msg)
{
	char tag[] = "ACTUATOR_STATE";
	// If there is an OT Bridge the controller RQs the OT Bridge every minute with a payload size of 1
	if (msg.payloadsize == 1) {
		//      A payload of size 1 is normally just 0x00
		return true;
	}
	// The OT Bridge responds to the RQ with a RP with payload size of 6
	if (msg.payloadsize == 6) {
		// msg.payload[1] is the Relative modulation level
		bool bExists = CheckPercentageSensorExists(17, 1);
		if ((msg.payload[1] != 0) || (bExists))
		{
			SendPercentageSensor(17, 1, 255, static_cast<float>(msg.payload[1]), "Relative modulation level");
		}
		// msg.payload[3] is the Flame Status
		bool bFlameOn{ false };
		if (msg.payload[3] == 0x0A) {
			bFlameOn = true;
		}
		// Record the Flame status in the same way that the OTGW hardware device currently does
		UpdateSwitch(113, bFlameOn, "FlameOn");

		Log(true, LOG_STATUS, "evohome: %s: OT Bridge full payload %02X%02X%02X%02X%02X%02X, packet size: %d", tag, msg.payload[0], msg.payload[1], msg.payload[2], msg.payload[3], msg.payload[4], msg.payload[5], msg.payloadsize);
		return true;
	}
	// All other relays should have a payload size of 3	
	if (msg.payloadsize != 3) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	int nDevNo = msg.payload[0];
	int nDemand = msg.payload[1]; //(0 for off or 0xc8 i.e. 100% for on)
	//The relay does not appear to always announce its state but this is probably due to the wireless signal and collisions etc

	Log(true, LOG_STATUS, "evohome: %s: ID:0x%06x (%s) DevNo 0x%02x: %d", tag, msg.GetID(0), msg.GetStrID(0).c_str(), nDevNo, nDemand);
	RXRelay(static_cast<uint8_t>(0xFF), static_cast<uint8_t>(nDemand));//devno is always 0 and therefore not valid
	
	//Log all received relay activity (green led) to a device, note this is different from Heat Demand
        bool bRelayOn( false );
        if (nDemand == 0xc8) {
                bRelayOn = true;
        }
        char zstrname[40];
        sprintf(zstrname, "Zone Relay %06x", msg.GetID(0));
        UpdateSwitch(msg.GetID(0), bRelayOn, zstrname);

	return true;
}

bool CEvohomeRadio::DecodeOpenThermBridge(CEvohomeMsg& msg)
{
	char tag[] = "OPENTHERM_BRIDGE";

	// Only look for responses from the OT Bridge and Filter out messages from other controllers 
	if (msg.GetID(1) != GetControllerID())
		return true;

	// All OT messages should have a payload size of 5	
	if (msg.payloadsize != 5) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	// The OT command response is in byte 4 and 5
	int nOTResponse = msg.payload[3] << 8 | msg.payload[4];
	float fOTResponse = static_cast<float>(nOTResponse) / 256.0f;

	// The OT commands are as per the OT Specification
	// 05 (ID.05) = Fault Code
	if (msg.payload[2] == 0x05) {
		SendCustomSensor(0, 5, 255, static_cast<float>(msg.payload[3]), "Application fault flags", "");
		SendCustomSensor(1, 5, 255, static_cast<float>(msg.payload[4]), "OEM fault code", "");

		Log(true, LOG_STATUS, "evohome: %s: Application-specific flags %02X OEM fault code %d", tag, msg.payload[3], msg.payload[4]);
		return true;
	}
	// 11 (ID.17) = Relative modulation level
	if (msg.payload[2] == 0x11) {
		bool bExists = CheckPercentageSensorExists(17, 1);
		if ((fOTResponse != 0) || (bExists))
		{
			SendPercentageSensor(17, 1, 255, fOTResponse, "Relative modulation level");
		}
		Log(true, LOG_STATUS, "evohome: %s: Relative modulation level = %.2f %%", tag, fOTResponse);
		return true;
	}
	// 12 (ID.18) = CH water pressure
	if (msg.payload[2] == 0x12) {
		SendPressureSensor(0, 18, 255, fOTResponse, "CH Water Pressure");

		Log(true, LOG_STATUS, "evohome: %s: CH water pressure = %.2f bar", tag, fOTResponse);
		return true;
	}
	// 13 (ID.19) = DHW flow rate
	if (msg.payload[2] == 0x13) {
		SendWaterflowSensor(0, 19, 255, fOTResponse, "DHW flow rate");

		Log(true, LOG_STATUS, "evohome: %s: DHW flow rate = %.2f l/min", tag, fOTResponse);
		return true;
	}
	// 19 (ID.25) = Boiler Water Temperature
	if (msg.payload[2] == 0x19) {
		SendTempSensor(25, 255, fOTResponse, "Boiler Water Temperature");

		Log(true, LOG_STATUS, "evohome: %s: Boiler Water Temperature = %.2f C", tag, fOTResponse);
		return true;
	}
	// 1A (ID.26) = DHW Temperature
	if (msg.payload[2] == 0x1a) {
		SendTempSensor(26, 255, fOTResponse, "DHW Temperature");

		Log(true, LOG_STATUS, "evohome: %s: DHW Temperature = %.2f C", tag, fOTResponse);
		return true;
	}
	// 1C (ID.28) = Return Water Temperature
	if (msg.payload[2] == 0x1c) {
		SendTempSensor(28, 255, fOTResponse, "Return Water Temperature");

		Log(true, LOG_STATUS, "evohome: %s: Return Water Temperature = %.2f C", tag, fOTResponse);
		return true;
	}
	// 73 (ID.115) = OEM diagnostic code
	if (msg.payload[2] == 0x73) {
		SendCustomSensor(0, 115, 255, static_cast<float>(nOTResponse), "OEM diagnostic code", "");

		Log(true, LOG_STATUS, "evohome: %s: OEM diagnostic code = %d", tag, nOTResponse);
		return true;
	}
	return true;
}

bool CEvohomeRadio::DecodeOpenThermSetpoint(CEvohomeMsg& msg)
{
	char tag[] = "OPENTHERM_SETPOINT";

	// Only look for responses from the OT Bridge and Filter out messages from other controllers
	if (msg.GetID(1) != GetControllerID())
		return true;

	// All OT messages should have a payload size of 3
	if (msg.payloadsize != 3) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}

	// The OT Control Setpoint is in byte 2 and 3
	float fOTSetpoint = static_cast<float>(msg.payload[1] << 8 | msg.payload[2]) / 100.0f;

	SendTempSensor(1, 255, fOTSetpoint, "Control Setpoint");
	Log(true, LOG_STATUS, "evohome: %s: Boiler Water Temperature = %.2f C", tag, fOTSetpoint);

	return true;
}

bool CEvohomeRadio::DecodeExternalSensor(CEvohomeMsg& msg)
{
	char tag[] = "EXTERNAL_SENSOR";
	if (msg.payloadsize < 4) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, packet size too small: %d", tag, msg.payloadsize);
		return false;
	}
	if (msg.payloadsize % 4 != 0) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, incorrect packet size: %d", tag, msg.payloadsize);
		return false;
	}
	for (int i = 0; i < msg.payloadsize; i += 4) {
		uint8_t nDevNo, nType;
		CEvohomeTemp temp;
		msg.Get(nDevNo).Get(temp).Get(nType);
		Log(true, LOG_STATUS, "evohome: %s: %d: Temp %.1f Type %d", tag, nDevNo, temp.GetTemp(), nType);
	}
	return true;
}


bool CEvohomeRadio::DecodeDeviceInfo(CEvohomeMsg& msg)
{
	char tag[] = "DEVICE_INFO";
	if (msg.payloadsize == 3) {
		Log(true, LOG_STATUS, "evohome: %s: Request for device info %d", tag, msg.payload[2]);
		return true;
	}
	if (msg.payloadsize != 22) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	uint8_t nAddr, nDevNo, nDevType;
	CEvohomeID idDev;
	msg.Get(nAddr, 2).Get(nDevNo, 5).Get(nDevType).Get(idDev, 19);
	if (!idDev.IsValid())
	{
		m_bStartup[1] = false;
		return true;
	}
	Log(true, LOG_STATUS, "evohome: %s: %d: Addr=%d Type=%d ID:0x%06x (%s)", tag, nDevNo, nAddr, nDevType, idDev.GetID(), idDev.GetStrID().c_str());

	// Report the Fault Log entries for the devices
	sprintf(tag, "FAULT_LOG");

	uint8_t nFaultType, nFaultCode, nFaultYear, nFaultMonth, nFaultDay, nFaultHour, nFaultMinute, nFaultSecond;
	uint64_t nFaultDateTime;
	uint8_t nAlertType;
	char sFaultType[15], sFaultCode[20], sDevType[15], sFaultDateTime[21];

	msg.Get(nFaultType, 1).Get(nFaultCode, 4);
	nFaultDateTime = static_cast<long>(msg.payload[10]) << 32 | static_cast<long>(msg.payload[11]) << 24 | msg.payload[12] << 16 | msg.payload[13] << 8 | msg.payload[14];

	nFaultYear = (nFaultDateTime & 0b1111111UL << 24) >> 24;
	nFaultMonth = (nFaultDateTime & 0b1111UL << 36) >> 36;
	nFaultDay = (nFaultDateTime & 0b11111UL << 31) >> 31;
	nFaultHour = static_cast<uint8_t>((nFaultDateTime & 0b11111UL << 19) >> 19);
	nFaultMinute = static_cast<uint8_t>((nFaultDateTime & 0b111111UL << 13) >> 13);
	nFaultSecond = static_cast<uint8_t>((nFaultDateTime & 0b111111UL << 7) >> 7);
	sprintf(sFaultDateTime, "20%02d-%02d-%02d %02d:%02d:%02d", nFaultYear, nFaultMonth, nFaultDay, nFaultHour, nFaultMinute, nFaultSecond);

	// Code to plain speak as used on the Controller Fault Logbook
	if ((nFaultType == 0x00) || (nFaultType == 0xc0)) {
		sprintf(sFaultType, "FAULT");
		nAlertType = 4;
	}
	else if (nFaultType == 0x40) {
		sprintf(sFaultType, "RESTORE");
		nAlertType = 0;
	}
	else { 
		sprintf(sFaultType, "UNKNOWN(%02x)", nFaultType);
		nAlertType = 2;
	}

	if (nFaultCode == 0x04) { sprintf(sFaultCode, "BATTERY LOW"); }
	else if (nFaultCode == 0x06) { sprintf(sFaultCode, "COMMS FAULT"); }
	else if (nFaultCode == 0x0a) { sprintf(sFaultCode, "SENSOR ERROR"); }
	else { sprintf(sFaultCode, "UNKNOWN(%02x)", nFaultCode); }

	if (nDevType == 0x04) { sprintf(sDevType, "ACTUATOR"); }
	else if (nDevType == 0x01) { sprintf(sDevType, "SENSOR"); }
	else if (nDevType == 0x05) { sprintf(sDevType, "HOT WATER"); }
        else if (nDevType == 0x00) { sprintf(sDevType, "CONTROLLER"); }
 	else { sprintf(sDevType, "UNKNOWN(%02x)", nDevType); }

	// Log all Fault Logbook entries to the Status log
        Log(false, LOG_STATUS, "evohome: %s: %s %s Device:%06x %s %s", tag, sFaultDateTime, sFaultType, idDev.GetID(), sDevType, sFaultCode);

	// Log current error condition to an Alert device
        if (nAddr == 0x00) {
		char szAlertMsg[70];
		sprintf(szAlertMsg, "%s: %s on %s(%06x) at %s", sFaultType, sFaultCode, sDevType, idDev.GetID(), sFaultDateTime);
		SendAlertSensor(418, 255, nAlertType, szAlertMsg, "Fault Logbook");
        }

	if (m_bStartup[1])
		RequestDeviceInfo(nAddr + 1);
	return true;
}


bool CEvohomeRadio::DecodeBatteryInfo(CEvohomeMsg& msg)
{
	char tag[] = "BATTERY_INFO";

	if ((msg.GetID(2) != GetControllerID()) && (msg.id[0].GetIDType() != CEvohomeID::devSensor)) // Filter out messages from other controllers
		return true;

	if (msg.payloadsize != 3) {
		Log(false, LOG_ERROR, "evohome: %s: Error decoding command, unknown packet size: %d", tag, msg.payloadsize);
		return false;
	}
	std::string szType("Unknown");
	uint8_t nDevNo, nBattery, nLowBat;
	msg.Get(nDevNo).Get(nBattery).Get(nLowBat);
	_tEVOHOME2 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME2));
	tsen.len = sizeof(_tEVOHOME2) - 1;
	RFX_SETID3(msg.GetID(0), tsen.id1, tsen.id2, tsen.id3)
		tsen.updatetype = updBattery;

	if (nBattery == 0xFF)
		nBattery = 100; // recode full battery (0xFF) to 100 for consistency across device types
	else
		nBattery = nBattery / 2;  // recode battery level values to 0-100 from original 0-200 values

	if (nLowBat == 0)
		nBattery = 0;
	tsen.battery_level = nBattery;
	if (msg.id[0].GetIDType() == CEvohomeID::devZone)
	{
		if (msg.id[2].GetIDType() == CEvohomeID::devController)
		{
			szType = "Zone";
			nDevNo++;
			tsen.type = pTypeEvohomeZone;
			tsen.subtype = sTypeEvohomeZone;
			tsen.zone = nDevNo;
			sDecodeRXMessage(this, (const unsigned char*)&tsen, NULL, nBattery);

			if (AllSensors)
			{
				tsen.type = pTypeEvohomeZone;
				tsen.subtype = sTypeEvohomeZone;
				tsen.zone += 12;
				sDecodeRXMessage(this, (const unsigned char*)&tsen, NULL, nBattery); // Update Zone device battery level
			}

			_tEVOHOME3 tsen2;
			memset(&tsen2, 0, sizeof(_tEVOHOME3));
			tsen2.len = sizeof(tsen) - 1;
			tsen2.type = pTypeEvohomeRelay;
			tsen2.subtype = sTypeEvohomeRelay;
			RFX_SETID3(msg.GetID(0), tsen2.id1, tsen2.id2, tsen2.id3);
			tsen2.devno = nDevNo;
			tsen2.demand = 0;
			tsen2.updatetype = CEvohomeRadio::updBattery;
			tsen2.battery_level = nBattery;
			sDecodeRXMessage(this, (const unsigned char*)&tsen2, NULL, nBattery);
		}
		else
		{
			szType = "Dev";
			tsen.type = pTypeEvohomeZone;
			tsen.subtype = sTypeEvohomeZone;
			tsen.zone = nDevNo;
			sDecodeRXMessage(this, (const unsigned char*)&tsen, NULL, nBattery);  // Update Relay device battery level
		}
	}
	else if (msg.id[0].GetIDType() == CEvohomeID::devSensor)
	{
		szType = "DHW";
		nDevNo++;
		tsen.type = pTypeEvohomeWater;
		tsen.subtype = sTypeEvohomeWater;
		tsen.zone = nDevNo;
		RFX_SETID3(GetControllerID(), tsen.id1, tsen.id2, tsen.id3);
		sDecodeRXMessage(this, (const unsigned char*)&tsen, "DHW Temp", nBattery);  // Update DHW Zone sensor
	}
	Log(true, LOG_STATUS, "evohome: %s: %s=%d charge=%d(%%) level=%d (%s)", tag, szType.c_str(), nDevNo, nBattery, nLowBat, (nLowBat == 0) ? "Low" : "OK");

	return true;
}

bool CEvohomeRadio::DecodeSync(CEvohomeMsg& msg) //0x1F09
{
	char tag[] = "SYNC"; //these messages communicate the time until the next scheduled controller message sequence which allows devices to sleep

	if (msg.payloadsize != 1 && msg.payloadsize != 3)
	{
		Log(false, LOG_ERROR, "evohome: %s: unexpected payload size: %d", tag, msg.payloadsize);
		return false;
	}
	else
	{
		if (msg.payloadsize == 1)
		{
			Log(true, LOG_STATUS, "evohome: %s: Type 0x%02x", tag, msg.payload[0]);
		}
		else
		{
			Log(true, LOG_STATUS, "evohome: %s: Type 0x%02x (%d)", tag, msg.payload[0], msg.payload[1] << 8 | msg.payload[2]);
		}
		return true;
	}
}

void CEvohomeRadio::AddSendQueue(const CEvohomeMsg& msg)
{
	std::lock_guard<std::mutex> l(m_mtxSend);
	m_SendQueue.push_back(msg);//may throw bad_alloc
}


void CEvohomeRadio::PopSendQueue(const CEvohomeMsg& msg)
{
	std::lock_guard<std::mutex> l(m_mtxSend);
	if (!m_SendQueue.empty())
	{
		if (m_SendQueue.front() == msg || m_nSendFail >= 10)
		{
			m_SendQueue.pop_front();
			m_nSendFail = 0;
		}
		else
			m_nSendFail++;
	}
}


void CEvohomeRadio::Send()
{
	//ideally we need some way to send only if we're not in the middle of receiving a packet but as everything is buffered i'm not sure if this will be effective
	if (m_nBufPtr > 0)
		return;
	std::lock_guard<std::mutex> sl(m_mtxSend);
	if (!m_SendQueue.empty())
	{
		if (m_SendQueue.front().BadMsg())//message rejected by HGI80 which will not be picked up as a failed send
			m_SendQueue.pop_front();
		else
		{
			std::string out(m_SendQueue.front().Encode() + "\r\n");
			Do_Send(out);
			m_SendQueue.pop_front();
		}
	}
}

void CEvohomeRadio::OnConnect()
{
	if (GetControllerID() != 0)//can't proceed without it
		RequestCurrentState();
}

void CEvohomeRadio::Idle_Work()
{
	Send();//FIXME afaik if there is a collision then we should increase the back off period exponentially if it happens again
	if (nStartup < 300)//if we haven't got zone names in 300s we auto init them (zone creation blocked until we have something)...
	{
		nStartup++;
		if (nStartup == 300)
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
	std::lock_guard<std::mutex> l(m_mtxRelayCheck);
	if (!m_RelayCheck.empty() && GetGatewayID() != 0)
	{
		CheckRelayHeatDemand();
		if ((boost::get_system_time() - stLastRelayCheck) > boost::posix_time::seconds(604)) //not sure if it makes a difference but avg time is about 604-605 seconds but not clear how reference point derived - seems steady once started
		{
			SendRelayKeepAlive();
			stLastRelayCheck = boost::get_system_time();
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_BindEvohome(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string type = request::findValue(&req, "devtype");
			int HwdID = atoi(idx.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(HwdID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_EVOHOME_SERIAL && pHardware->HwdType != HTYPE_EVOHOME_TCP)
				return;
			CEvohomeRadio* pEvoHW = reinterpret_cast<CEvohomeRadio*>(pHardware);

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
				nID = pEvoHW->Bind((uint8_t)nDevNo, CEvohomeID::devRelay);
			}
			else if (type == "OutdoorSensor")
				nID = pEvoHW->Bind(0, CEvohomeID::devSensor);
			else if (type == "AllSensors")
			{
				// Check if All Sensors has already been activated, if not create a temporary dummy sensor
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT HardwareID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit > 12) AND (Unit <= 24)", HwdID, (int)pTypeEvohomeZone);
				if (result.empty())
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
				result = m_sql.safe_query("SELECT MAX(Unit) FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit>=40) AND (Unit<52)", HwdID, (int)pTypeEvohomeZone);
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
				nID = pEvoHW->Bind((uint8_t)nDevNo, CEvohomeID::devZone);
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
				m_sql.UpdateValue(HwdID, devid.c_str(), (uint8_t)nDevNo, pTypeEvohomeRelay, sTypeEvohomeRelay, 10, 255, 0, "Off", devname);
				pEvoHW->SetRelayHeatDemand((uint8_t)nDevNo, 0);//initialize heat demand
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
				m_sql.UpdateValue(HwdID, ID, (const unsigned char)nDevNo, pTypeEvohomeZone, sTypeEvohomeZone, 10, 255, 0, "0.0;0.0;Auto", devname);
			}
			root["status"] = "OK";
			root["title"] = "BindEvohome";
			root["Used"] = false;
		}
	}
}
