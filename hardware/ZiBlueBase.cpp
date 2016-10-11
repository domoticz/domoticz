
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
#include "../json/json.h"

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

extern const char *findTableIDSingle1(const STR_TABLE_SINGLE *t, const unsigned long id);

const char *szZiBlueProtocolRFLink(const unsigned char id)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ 1, "VISONIC_433" },
		{ 2, "VISONIC_868" },
		{ 3, "CHACON_433" },
		{ 4, "DOMIA_433" },
		{ 5, "X10_433" },
		{ 6, "X2D_433" },
		{ 7, "X2D_868" },
		{ 8, "X2D_SHUTTER_868" },
		{ 9, "X2D_HA_ELEC_868" },
		{ 10, "X2D_HA_GAS_868" },
		{ 11, "SOMFY_RTS_433" },
		{ 12, "BLYSS_433" },
		{ 13, "PARROT_433_OR_868" },
		{ 14, "reserved" },
		{ 15, "reserved" },
		{ 16, "KD101_433" },
		{ 0, NULL }
	};
	return findTableIDSingle1(Table, id);
}

//0: OFF, 1 : ON, 2 : BRIGHT, 3 : DIM, 4 : ALL_LIGHTS_ON, 5 : ALL_LIGHTS_OFF

const char *szZiBlueProtocol(const unsigned char id)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ 1, "X10" },
		{ 2, "VISONIC" },
		{ 3, "BLYSS" },
		{ 4, "CHACON" },
		{ 5, "OREGON" },
		{ 6, "DOMIA" },
		{ 7, "OWL" },
		{ 8, "X2D" },
		{ 9, "RTS" },
		{ 10, "KD101" },
		{ 11, "PARROT" },
		{ 0, NULL }
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
	{ "Blyss", sSwitchTypeBlyss },
	{ "RTS", sSwitchTypeRTS },
	{ "", -1 }
};

const _tZiBlueStringIntHelper rfswitchcommands[] =
{
	{ "ON", gswitch_sOn },
	{ "OFF", gswitch_sOff },
	{ "DIM", gswitch_sDim },
	{ "", -1 }
};

const _tZiBlueStringIntHelper rfblindcommands[] =
{
	{ "UP", blinds_sOpen },
	{ "DOWN", blinds_sClose },
	{ "STOP", blinds_sStop },
	{ "CONFIRM", blinds_sConfirm },
	{ "LIMIT", blinds_sLimit },
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

CZiBlueBase::~CZiBlueBase()
{
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
	WriteInt("ZIA++FORMAT BINARY");
	//WriteInt("ZIA++FORMAT RFLINK BINARY");
	//m_bRFLinkOn = true;
	WriteInt("ZIA++REPEATER OFF");
}

void CZiBlueBase::OnDisconnected()
{
	Init();
}


#define round(a) ( int ) ( a + .5 )

bool CZiBlueBase::WriteToHardware(const char *pdata, const unsigned char length)
{
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

	int cmnd = GetGeneralZiBlueFromString(rfswitchcommands, switchcmd);

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
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, BatteryLevel);
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
			if (m_Length >= 1000)
			{
				m_State = ZIBLUE_STATE_ZI_1;
			}
			else
			{
				m_rfbufferpos = 0;
				m_State = ZIBLUE_STATE_DATA;
			}
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
		}
		ii++;
		continue;
	}
}

bool CZiBlueBase::ParseBinary(const uint8_t SDQ, const uint8_t *data, size_t len)
{
	m_LastReceivedTime = mytime(NULL);

	uint8_t reserved = (SDQ & 0x80) >> 7;
	uint8_t vtype = (SDQ & 0x70) >> 4; //0x0 = binary, 0x4 = ascii
	if (vtype != 0)
		return false;

	uint8_t SourceDest = SDQ & 0x0F;

	if (SourceDest > 1)
		return false; //not supported

	//0 = MUX Management
	//1 = 433 / 868

	if (m_bRFLinkOn)
	{
		if (len < 17)
			return false;
		const uint8_t *pData = (const uint8_t*)data;

		uint8_t FrameType = pData[0];
		pData++;
		uint32_t frequency = (uint32_t)(pData[0] << 24) + (uint32_t)(pData[1]) + (uint32_t)(pData[2]) + (uint32_t)pData[3];
		pData += 16;
		int8_t RFLevel = pData[0];
		int8_t FloorNoise = pData[1];
		uint8_t PulseElementSize = pData[2];
		uint16_t number = (pData[3] << 8) + pData[4];
		uint8_t Repeats = pData[5];
		uint8_t Delay = pData[6];
		uint8_t Multiply = pData[7];
		pData += 8;
		uint32_t Time = (uint32_t)(pData[0] << 24) + (uint32_t)(pData[1]) + (uint32_t)(pData[2]) + (uint32_t)pData[3];
		pData += 16;
		_log.Log(LOG_NORM, "ZiBlue: frameType: %d, frequency: %d kHz", FrameType, frequency);
		_log.Log(LOG_NORM, "ZiBlue: rfLevel: %d dBm, floorNoise: %d dBm, PulseElementSize: %d", RFLevel, FloorNoise, PulseElementSize);
		_log.Log(LOG_NORM, "ZiBlue: number: %d, Repeats: %d, Delay: %d, Multiply: %d", number, Repeats, Delay, Multiply);
	}
	else
	{
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
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d, houseCode: %c, dev: %d", 'A' + houseCode, dev + 1);
#endif
			}
			break;
		case INFOS_TYPE1:
			// Used by X10 (32 bits ID) and CHACON
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE1))
			{
				INCOMING_RF_INFOS_TYPE1 *pSen = (INCOMING_RF_INFOS_TYPE1*)(data + 8);
				int DevID = (pSen->idMsb << 16) + pSen->idLsb;
#ifdef DEBUG_ZIBLUE
				_log.Log(LOG_NORM, "ZiBlue: subtype: %d, DevID: %04X", pSen->subtype, DevID);
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
				if (pSen->hygro > 0)
					SendTempHumSensor(pSen->idPHY, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0f, pSen->hygro, "Temp+Hum");
				else
					SendTempSensor(pSen->idPHY, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0f, "Temp");
			}
			break;
		case INFOS_TYPE5:
			// Used by  Scientific Oregon  protocol  ( Atmospheric  pressure  sensors)
			if (dlen == sizeof(INCOMING_RF_INFOS_TYPE5))
			{
				INCOMING_RF_INFOS_TYPE5 *pSen = (INCOMING_RF_INFOS_TYPE5*)(data + 8);
				SendTempHumBaroSensor(pSen->idPHY, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0f, pSen->hygro, pSen->pressure, 0, "Temp+Hum+Baro");
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
		}
	}
#ifdef DEBUG_ZIBLUE
	_log.Log(LOG_NORM, "ZiBlue: ----------------");
#endif
    return true;
}
