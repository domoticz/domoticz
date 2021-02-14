
#include "stdafx.h"
#include "ZiBlueBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"

#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>

#include "ziblue_usb_frame_api.h"

#ifdef _DEBUG
	#define ENABLE_LOGGING
	#define DEBUG_ZIBLUE
#endif

typedef struct _STR_TABLE_SINGLE {
	unsigned long    id;
	const char   *str1;
	const char   *str2;
} STR_TABLE_SINGLE;

extern const char *findTableIDSingle1(const STR_TABLE_SINGLE *t, unsigned long id);

const char *szZiBlueProtocolRFLink(const unsigned char id)
{
	static const STR_TABLE_SINGLE Table[] = {
		{ 1, "VISONIC_433" },	     //
		{ 2, "VISONIC_868" },	     //
		{ 3, "CHACON_433" },	     //
		{ 4, "DOMIA_433" },	     //
		{ 5, "X10_433" },	     //
		{ 6, "X2D_433" },	     //
		{ 7, "X2D_868" },	     //
		{ 8, "X2D_SHUTTER_868" },    //
		{ 9, "X2D_HA_ELEC_868" },    //
		{ 10, "X2D_HA_GAS_868" },    //
		{ 11, "SOMFY_RTS_433" },     //
		{ 12, "BLYSS_433" },	     //
		{ 13, "PARROT_433_OR_868" }, //
		{ 14, "reserved" },	     //
		{ 15, "reserved" },	     //
		{ 16, "KD101_433" },	     //
		{ 0, nullptr },		     //
	};
	return findTableIDSingle1(Table, id);
}

//0: OFF, 1 : ON, 2 : BRIGHT, 3 : DIM, 4 : ALL_LIGHTS_ON, 5 : ALL_LIGHTS_OFF

const char *szZiBlueProtocol(const unsigned char id)
{
	static const STR_TABLE_SINGLE Table[] = {
		{ 1, "X10" },	  //
		{ 2, "VISONIC" }, //
		{ 3, "BLYSS" },	  //
		{ 4, "CHACON" },  //
		{ 5, "OREGON" },  //
		{ 6, "DOMIA" },	  //
		{ 7, "OWL" },	  //
		{ 8, "X2D" },	  //
		{ 9, "RTS" },	  //
		{ 10, "KD101" },  //
		{ 11, "PARROT" }, //
		{ 0, nullptr },	  //
	};
	return findTableIDSingle1(Table, id);
}

struct _tZiBlueStringIntHelper
{
	std::string szType;
	int gType;
};

const _tZiBlueStringIntHelper ziblue_switches[] =
{
	{ "X10", sSwitchTypeX10 },
	//{ "Kaku", sSwitchTypeARC },
	{ "AC", sSwitchTypeAC },
	{ "Blyss", sSwitchTypeBlyss },
	{ "RTS", sSwitchTypeRTS },
	{ "", -1 }
};

const _tZiBlueStringIntHelper ziBlueswitchcommands[] =
{
	{ "ON", gswitch_sOn },
	{ "OFF", gswitch_sOff },
	{ "ALLON", gswitch_sGroupOn },
	{ "ALLOFF", gswitch_sGroupOff },
	{ "DIM", gswitch_sDim },
	{ "BRIGHT", gswitch_sBright },
	{ "", -1 }
};

int GetGeneralZiBlueFromString(const _tZiBlueStringIntHelper *pTable, const std::string &szType)
{
	int ii = 0;
	while (pTable[ii].gType!=-1)
	{
		if (pTable[ii].szType == szType)
			return pTable[ii].gType;
		ii++;
	}
	return -1;
}

std::string GetGeneralZiBlueFromInt(const _tZiBlueStringIntHelper *pTable, const int gType)
{
	int ii = 0;
	while (pTable[ii].gType!=-1)
	{
		if (pTable[ii].gType == gType)
			return pTable[ii].szType;
		ii++;
	}
	return "";
}

CZiBlueBase::CZiBlueBase()
{
	Init();
}

void CZiBlueBase::Init()
{
	m_bRFLinkOn = false;
	m_rfbufferpos = 0;
	m_State = ZIBLUE_STATE_ZI_1;
	memset(&m_rfbuffer, 0, sizeof(m_rfbuffer));
}

void CZiBlueBase::OnConnected()
{
	Init();
	WriteInt("ZIA++FORMAT BINARY\r");
	WriteInt("ZIA++FORMAT RFLINK BINARY\r");
	m_bRFLinkOn = true;
	WriteInt("ZIA++REPEATER OFF\r");
}

void CZiBlueBase::OnDisconnected()
{
	Init();
}


#define round(a) ( int ) ( a + .5 )

bool CZiBlueBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);

	if (pSwitch->type != pTypeGeneralSwitch)
		return false;
	std::string switchtype = GetGeneralZiBlueFromInt(ziblue_switches, pSwitch->subtype);
	if (switchtype.empty())
	{
		_log.Log(LOG_ERROR, "ZiBlue: trying to send unknown switch type: %d", pSwitch->subtype);
		return false;
	}

	if (pSwitch->type == pTypeGeneralSwitch)
	{
		std::string switchcmnd = GetGeneralZiBlueFromInt(ziBlueswitchcommands, pSwitch->cmnd);

		// check setlevel command
		if (pSwitch->cmnd == gswitch_sSetLevel) {
			// Get device level to set
			float fvalue = (15.0F / 100.0F) * float(pSwitch->level);
			if (fvalue > 15.0F)
				fvalue = 15.0F; // 99 is fully on
			int svalue = round(fvalue);
			//_log.Log(LOG_ERROR, "RFLink: level: %d", svalue);
			char buffer[50] = { 0 };
			sprintf(buffer, "%d", svalue);
			switchcmnd = buffer;
		}

		if (switchcmnd.empty()) {
			_log.Log(LOG_ERROR, "ZiBlue: trying to send unknown switch command: %d", pSwitch->cmnd);
			return false;
		}

		struct _tOutgoingFrame
		{
			//struct  MESSAGE_CONTAINER_HEADER
			unsigned char Sync1;
			unsigned char Sync2;
			unsigned char SourceDestQualifier;
			unsigned char QualifierOrLen_lsb;
			unsigned char QualifierOrLen_msb;

			//struct REGULAR_INCOMING_BINARY_USB_FRAME
			unsigned char frameType;
			unsigned char cluster; // set 0 by default. Cluster destination.
			unsigned char protocol;
			unsigned char action;
			unsigned char ID_1;
			unsigned char ID_2;
			unsigned char ID_3;
			unsigned char ID_4;
			unsigned char dimValue;
			unsigned char burst;
			unsigned char qualifier;
			unsigned char reserved2; // set 0
			_tOutgoingFrame()
			{
				memset(this, 0, sizeof(_tOutgoingFrame));
				Sync1 = SYNC1_CONTAINER_CONSTANT;
				Sync2 = SYNC2_CONTAINER_CONSTANT;
				SourceDestQualifier = 1;
				QualifierOrLen_lsb = sizeof(REGULAR_INCOMING_BINARY_USB_FRAME) & 0xFF;
				QualifierOrLen_msb = (sizeof(REGULAR_INCOMING_BINARY_USB_FRAME) & 0xFF00) >> 8;
			}
		};
		_tOutgoingFrame oFrame;

		std::stringstream sstr;
		if (switchtype == "X10")
		{
			oFrame.protocol = SEND_X10_PROTOCOL_433;
			oFrame.ID_1 = ((pSwitch->id << 4) + pSwitch->unitcode);
		}
		else if (switchtype == "AC")
		{
			oFrame.protocol = SEND_CHACON_PROTOCOL_433;
			oFrame.ID_1 = (uint8_t)((pSwitch->id & 0xFF000000) >> 24);
			oFrame.ID_2 = (uint8_t)((pSwitch->id & 0x00FF0000) >> 16);
			oFrame.ID_3 = (uint8_t)((pSwitch->id & 0x0000FF00) >> 8);
			oFrame.ID_4 = (uint8_t)((pSwitch->id & 0x000000FF));
		}
		else
		{
			_log.Log(LOG_ERROR, "ZiBlue: unsupported switch protocol");
			return false;
		}
		switch (pSwitch->cmnd)
		{
		case gswitch_sOff:
			oFrame.action = SEND_ACTION_OFF;
			break;
		case gswitch_sOn:
			oFrame.action = SEND_ACTION_ON;
			break;
		case gswitch_sSetLevel:
			oFrame.action = SEND_ACTION_DIM;
			oFrame.dimValue = pSwitch->level;
			break;
		case gswitch_sDim:
			oFrame.action = SEND_ACTION_DIM;
			oFrame.dimValue = pSwitch->level;
			break;
		case gswitch_sBright:
			oFrame.action = SEND_ACTION_BRIGHT;
			break;
		case gswitch_sGroupOff:
			oFrame.action = SEND_ACTION_ALL_OFF;
			break;
		case gswitch_sGroupOn:
			oFrame.action = SEND_ACTION_ALL_ON;
			break;
		}

		WriteInt((const uint8_t*)&oFrame, sizeof(_tOutgoingFrame));
		return true;
	}
	return false;
}

bool CZiBlueBase::SendSwitchInt(const int ID, const int switchunit, const int BatteryLevel, const std::string &switchType, const std::string &switchcmd, const int level)
{
	int intswitchtype = GetGeneralZiBlueFromString(ziblue_switches, switchType);
	if (intswitchtype == -1)
	{
		_log.Log(LOG_ERROR, "ZiBlue: Unhandled switch type: %s", switchType.c_str());
		return false;
	}

	int cmnd = GetGeneralZiBlueFromString(ziBlueswitchcommands, switchcmd);

	int svalue=level;
	if (cmnd==-1) {
		if (switchcmd.compare(0, 10, "SET_LEVEL=") == 0 ){
			cmnd=gswitch_sSetLevel;
			std::string str2 = switchcmd.substr(10);
			svalue=atoi(str2.c_str()); 
	  		_log.Log(LOG_STATUS, "ZiBlue: %d level: %d", cmnd, svalue);
		}
	}
    
	if (cmnd==-1)
	{
		_log.Log(LOG_ERROR, "ZiBlue: Unhandled switch command: %s", switchcmd.c_str());
		return false;
	}

	_tGeneralSwitch gswitch;
	gswitch.subtype = intswitchtype;
	gswitch.id = ID;
	gswitch.unitcode = switchunit;
	gswitch.cmnd = cmnd;
    gswitch.level = svalue; //level;
	gswitch.battery_level = BatteryLevel;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, nullptr, BatteryLevel, m_Name.c_str());
	return true;
}

void CZiBlueBase::ParseData(const char *data, size_t len)
{
	size_t ii = 0;
	while (ii < len)
	{
		switch (m_State)
		{
		case ZIBLUE_STATE_ZI_1:
			if (data[ii] == SYNC1_CONTAINER_CONSTANT)
				m_State = ZIBLUE_STATE_ZI_2;
			break;
		case ZIBLUE_STATE_ZI_2:
			if (data[ii] != SYNC2_CONTAINER_CONSTANT)
				m_State = ZIBLUE_STATE_ZI_1;
			else
				m_State = ZIBLUE_STATE_SDQ;
			break;
		case ZIBLUE_STATE_SDQ:
			m_SDQ = data[ii];
			m_State = ZIBLUE_STATE_QL_1;
			break;
		case ZIBLUE_STATE_QL_1:
			m_QL_1 = data[ii];
			m_State = ZIBLUE_STATE_QL_2;
			break;
		case ZIBLUE_STATE_QL_2:
			m_QL_2 = data[ii];
			m_Length = (m_QL_2 << 8) + m_QL_1;
			if (m_SDQ != 'A')
			{
				//Binair
				if (m_Length >= 1000)
				{
					m_State = ZIBLUE_STATE_ZI_1;
					break;
				}
				m_State = ZIBLUE_STATE_DATA;
			}
			else
			{
				//Ascii, only accept JSON
				if (
					(m_QL_1 != '3') ||
					(m_QL_2 != '3')
					)
				{
					m_State = ZIBLUE_STATE_ZI_1;
					break;
				}
				m_State = ZIBLUE_STATE_DATA_ASCII;
			}
			m_rfbufferpos = 0;
			break;
		case ZIBLUE_STATE_DATA:
			m_rfbuffer[m_rfbufferpos++] = data[ii];
			if (m_rfbufferpos == m_Length)
			{
				ParseBinary(m_SDQ, (const uint8_t*)&m_rfbuffer, m_Length);
				m_State = ZIBLUE_STATE_ZI_1;
			}
			else if (m_rfbufferpos >= m_Length)
			{
				m_State = ZIBLUE_STATE_ZI_1;
			}
			break;
		case ZIBLUE_STATE_DATA_ASCII:
			if (len > 1)
			{
				if (
					(data[ii] == '\n') &&
					(data[ii + 1] == '\r')
					)
				{
					//Frame done
					ParseBinary(m_SDQ, (const uint8_t*)&m_rfbuffer, m_rfbufferpos);
					m_State = ZIBLUE_STATE_ZI_1;
				}
			}
			m_rfbuffer[m_rfbufferpos++] = data[ii];
			if (m_rfbufferpos>= ZiBlue_READ_BUFFER_SIZE)
			{
				m_State = ZIBLUE_STATE_ZI_1;
			}
			break;
		}
		ii++;
	}
}

bool CZiBlueBase::ParseBinary(const uint8_t SDQ, const uint8_t *data, size_t len)
{
	m_LastReceivedTime = mytime(nullptr);

	uint8_t reserved = (SDQ & 0x80) >> 7;
	uint8_t vtype = (SDQ & 0x70) >> 4; //0x0 = binary, 0x4 = ascii
	if (vtype != 0)
		return false;

	uint8_t SourceDest = SDQ & 0x0F;

	if (SourceDest > 1)
		return false; //not supported

	//0 = MUX Management
	//1 = 433 / 868

	uint8_t FrameType = data[0];

	if (FrameType==1)
	{
		//RFLink
		if (len < 24)
			return false;
		const uint8_t *pData = (const uint8_t*)data;

		FrameType = pData[0];
		uint32_t frequency = (uint32_t)(pData[4] << 24) + (uint32_t)(pData[3]<<16) + (uint32_t)(pData[2]<<8) + (uint32_t)pData[1];
		int8_t RFLevel = pData[5];
		int8_t FloorNoise = pData[6];
		uint8_t PulseElementSize = pData[7];
		uint16_t number = (pData[9] << 8) + pData[8];
		uint8_t Repeats = pData[10];
		uint8_t Delay = pData[11];
		uint8_t Multiply = pData[12];
		uint32_t Time = (uint32_t)(pData[16] << 24) + (uint32_t)(pData[15]<<16) + (uint32_t)(pData[14]<<8) + (uint32_t)pData[13];
		_log.Log(LOG_NORM, "ZiBlue: frameType: %d, frequency: %d kHz", FrameType, frequency);
		_log.Log(LOG_NORM, "ZiBlue: rfLevel: %d dBm, floorNoise: %d dBm, PulseElementSize: %d", RFLevel, FloorNoise, PulseElementSize);
		_log.Log(LOG_NORM, "ZiBlue: number: %d, Repeats: %d, Delay: %d, Multiply: %d", number, Repeats, Delay, Multiply);
	}
	else if (FrameType==0)
	{
		//Normal RF
		int dlen = len - 8;
		REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER *pIncomming = (REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER*)data;
#ifdef DEBUG_ZIBLUE
		_log.Log(LOG_NORM, "ZiBlue: frameType: %d, cluster: %d, dataFlag: %d (%s Mhz)", pIncomming->frameType, pIncomming->cluster, pIncomming->dataFlag, (pIncomming->dataFlag == 0) ? "433" : "868");
		_log.Log(LOG_NORM, "ZiBlue: rfLevel: %d dBm, floorNoise:: %d dBm, rfQuality: %d", pIncomming->rfLevel, pIncomming->floorNoise, pIncomming->rfQuality);
		_log.Log(LOG_NORM, "ZiBlue: protocol: %d (%s), infoType: %d", pIncomming->protocol, szZiBlueProtocol(pIncomming->protocol), pIncomming->infoType);
#endif
		switch (pIncomming->infoType)
		{
		case INFOS_TYPE0:
			// used by X10 / Domia Lite protocols
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE0))
			{
				INCOMING_RF_INFOS_TYPE0 *pSen = (INCOMING_RF_INFOS_TYPE0*)(data + 8);
				uint8_t houseCode = (pSen->id & 0xF0) >> 4;;
				uint8_t dev = pSen->id & 0x0F;
				std::string switchCmd;
				switch (pSen->subtype)
				{
				case SEND_ACTION_OFF:
					switchCmd = "OFF";
					break;
				case SEND_ACTION_ON:
					switchCmd = "ON";
					break;
				case SEND_ACTION_BRIGHT:
					switchCmd = "BRIGHT";
					break;
				case SEND_ACTION_DIM:
					switchCmd = "DIM";
					break;
				case SEND_ACTION_ALL_ON:
					switchCmd = "ALLON";
					break;
				case SEND_ACTION_ALL_OFF:
					switchCmd = "ALLOFF";
					break;
				}
				const char *szProtocol = szZiBlueProtocol(pIncomming->protocol);
				if (szProtocol != nullptr)
				{
					SendSwitchInt(houseCode, dev, 255, std::string(szProtocol), switchCmd, 0);
				}
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d (%s), houseCode: %c, dev: %d", pSen->subtype, switchCmd.c_str(), (uint8_t)('A' + houseCode), dev + 1);
#endif
			}
			break;
		case INFOS_TYPE1:
			// Used by X10 (24/32 bits ID),  CHACON , KD101, BLYSS
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE1))
			{
				INCOMING_RF_INFOS_TYPE1 *pSen = (INCOMING_RF_INFOS_TYPE1*)(data + 8);
				int DevID = (pSen->idMsb << 16) + pSen->idLsb;
				std::string switchCmd;
				switch (pSen->subtype)
				{
				case SEND_ACTION_OFF:
					switchCmd = "OFF";
					break;
				case SEND_ACTION_ON:
					switchCmd = "ON";
					break;
/*
				case SEND_ACTION_BRIGHT:
					switchCmd = "BRIGHT";
					break;
				case SEND_ACTION_DIM:
					switchCmd = "DIM";
					break;
				case SEND_ACTION_ALL_ON:
					switchCmd = "ALLON";
					break;
				case SEND_ACTION_ALL_OFF:
					switchCmd = "ALLOFF";
					break;
*/
				}
				const char *szProtocol = szZiBlueProtocol(pIncomming->protocol);
				if (szProtocol != nullptr)
				{
					SendSwitchInt(DevID, 1, 255, std::string(szProtocol), switchCmd, 0);
				}
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d (%s), DevID: %04X", pSen->subtype, switchCmd.c_str(), DevID);
#endif
			}
			break;
		case INFOS_TYPE2:
			// Used by  VISONIC /Focus/Atlantic/Meian Tech 
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE2))
			{
				INCOMING_RF_INFOS_TYPE2 *pSen = (INCOMING_RF_INFOS_TYPE2*)(data + 8);
				int DevID = (pSen->idMsb << 16) + pSen->idLsb;
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: DevID: %04X", DevID);
#endif
				if (pSen->subtype == 0)
				{
					//detector/sensor( PowerCode device)
					uint8_t Tamper_Flag = (pSen->qualifier & 0x01) != 0;
					uint8_t Alarm_Flag = (pSen->qualifier & 0x02) != 0;
					uint8_t LowBat_Flag = (pSen->qualifier & 0x04) != 0;
					uint8_t Supervisor_Frame_Flag = (pSen->qualifier & 0x08) != 0;
					if (Supervisor_Frame_Flag != 0)
						return false;
#ifdef DEBUG_ZIBLUE
					_log.Log(LOG_NORM, "ZiBlue: Tamper_Flag: %d, Alarm_Flag: %d, LowBat_Flag: %d, Supervisor_Frame_Flag:%d", Tamper_Flag, Alarm_Flag, LowBat_Flag, Supervisor_Frame_Flag);
#endif
				}
				else
				{
					//remote control (CodeSecure device)
					uint16_t Key_Id = pSen->qualifier;//values : 0x08, 0x10, 0x20, 0x40. (Nota : D0:7 of the id always set to 0)
#ifdef DEBUG_ZIBLUE
					_log.Log(LOG_NORM, "ZiBlue: Key_Id: %04X", Key_Id);
#endif
				}
			}
			break;
		case INFOS_TYPE3:
			//  Used by RFY PROTOCOL
			break;
		case INFOS_TYPE4:
			// Used by  Scientific Oregon  protocol ( thermo/hygro sensors)
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE4))
			{
				INCOMING_RF_INFOS_TYPE4 *pSen = (INCOMING_RF_INFOS_TYPE4*)(data + 8);
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, temp: %.1f, hygro: %d",
					pSen->subtype,
					pSen->idPHY,
					pSen->idChannel,
					pSen->qualifier,
					float(pSen->temp) / 10.0f,
					pSen->hygro);
#endif
				if (pSen->hygro > 0)
					SendTempHumSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0F, pSen->hygro, "Temp+Hum");
				else
					SendTempSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0F, "Temp");
			}
			break;
		case INFOS_TYPE5:
			// Used by  Scientific Oregon  protocol  ( Atmospheric  pressure  sensors)
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE5))
			{
				INCOMING_RF_INFOS_TYPE5 *pSen = (INCOMING_RF_INFOS_TYPE5*)(data + 8);
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, temp: %.1f, hygro: %d, pressure: %d",
					pSen->subtype,
					pSen->idPHY,
					pSen->idChannel,
					pSen->qualifier,
					float(pSen->temp) / 10.0f,
					pSen->hygro,
					pSen->pressure
					);
#endif
				SendTempHumBaroSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0F, pSen->hygro, pSen->pressure, 0, "Temp+Hum+Baro");
			}
			break;
		case INFOS_TYPE6:
			// Used by  Scientific Oregon  protocol  (Wind sensors)
			break;
		case INFOS_TYPE7:
			// Used by  Scientific Oregon  protocol  ( UV  sensors)
			break;
		case INFOS_TYPE8:
			// Used by  OWL  ( Energy/power sensors)
			break;
		case INFOS_TYPE9:
			// Used by  OREGON  ( Rain sensors)
			break;
		case INFOS_TYPE10:
			// Used by Thermostats  X2D protocol
			break;
		case INFOS_TYPE11:
			// Used by Alarm/remote control devices  X2D protocol
			break;
		case INFOS_TYPE12:
			// deprecated // Used by  DIGIMAX TS10 protocol
			break;
		case INFOS_TYPE13:
			// Used by  Cartelectronic TIC/Pulses devices (Teleinfo/TeleCounters)
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE13))
			{
				INCOMING_RF_INFOS_TYPE13 *pSen = (INCOMING_RF_INFOS_TYPE13*)(data + 8);
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d, idLsb: %04X, idMsb: %04X, qualifier: %04X, infos: %04X, counter1Lsb: %d, counter1Msb: %d, counter2Lsb: %d, counter2Msb: %d, apparentPower: %d",
					pSen->subtype,
					pSen->idLsb,
					pSen->idMsb,
					pSen->qualifier,
					pSen->infos,
					pSen->counter1Lsb,
					pSen->counter1Msb,
					pSen->counter2Lsb,
					pSen->counter2Msb,
					pSen->apparentPower
				);
#endif
				
				int total1 = pSen->counter1Msb;
				total1 = total1 << 16;
				total1 += pSen->counter1Lsb;
				int total2 = pSen->counter2Msb;
				total2 = total2 << 16;
				total2 += pSen->counter2Lsb;
				double power1 = 0;
				double power2 = 0;
				if (m_LastReceivedKWhMeterValue.find(pSen->idLsb^pSen->idMsb ^ 1) != m_LastReceivedKWhMeterValue.end())
				{
					power1 = (total1 - m_LastReceivedKWhMeterValue[pSen->idLsb^pSen->idMsb ^ 1]) / ((m_LastReceivedTime - m_LastReceivedKWhMeterTime[pSen->idLsb^pSen->idMsb ^ 1]) / 3600.0);
					power2 = (total2 - m_LastReceivedKWhMeterValue[pSen->idLsb^pSen->idMsb ^ 2]) / ((m_LastReceivedTime - m_LastReceivedKWhMeterTime[pSen->idLsb^pSen->idMsb ^ 2]) / 3600.0);
					power1 = round(power1);
					power2 = round(power2);
				}
				SendKwhMeter(pSen->idLsb^pSen->idMsb, 1, (pSen->qualifier & 0x01) ? 0 : 100, power1, total1 / 1000.0, "HC");
				SendKwhMeter(pSen->idLsb^pSen->idMsb, 2, (pSen->qualifier & 0x01) ? 0 : 100, power2, total2 / 1000.0, "HP");
				SendWattMeter(pSen->idLsb^pSen->idMsb, 3, (pSen->qualifier & 0x01) ? 0 : 100, (float)pSen->apparentPower, "Apparent Power");
				m_LastReceivedKWhMeterTime[pSen->idLsb^pSen->idMsb ^ 1] = m_LastReceivedTime;
				m_LastReceivedKWhMeterValue[pSen->idLsb^pSen->idMsb ^ 1] = total1;
				m_LastReceivedKWhMeterTime[pSen->idLsb^pSen->idMsb ^ 2] = m_LastReceivedTime;
				m_LastReceivedKWhMeterValue[pSen->idLsb^pSen->idMsb ^ 2] = total2;
			}
			break;
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "ZiBlue: Unknown Frametype received: %d", FrameType);
	}
#ifdef DEBUG_ZIBLUE
	_log.Log(LOG_NORM, "ZiBlue: ----------------");
#endif
    return true;
}
