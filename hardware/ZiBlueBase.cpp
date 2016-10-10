
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
#endif

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
	m_rfbufferpos = 0;
	m_State = ZIBLUE_STATE_ZI_1;
	memset(&m_rfbuffer, 0, sizeof(m_rfbuffer));
}

void CZiBlueBase::OnConnected()
{
	Init();
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

	int dlen = len - 8;

	REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER *pIncomming = (REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER*)data;

	_log.Log(LOG_NORM, "ZiBlue: Received Info Type: %d, %s", pIncomming->infoType, (pIncomming->dataFlag == 0) ? "433" : "868");
	switch (pIncomming->infoType)
	{
	case INFOS_TYPE0:
		// used by X10 / Domia Lite protocols
		break;
	case INFOS_TYPE1:
		// Used by X10 (32 bits ID) and CHACON
		break;
	case INFOS_TYPE2:
		// Used by  VISONIC /Focus/Atlantic/Meian Tech 
		if (dlen == sizeof(INCOMING_RF_INFOS_TYPE2))
		{
			INCOMING_RF_INFOS_TYPE2 *pSen = (INCOMING_RF_INFOS_TYPE2*)(data + 8);
			int DevID = (pSen->idMsb << 16) + pSen->idLsb;
			if (pSen->subtype == 0)
			{
				//detector/sensor( PowerCode device)
				uint8_t Tamper_Flag = (pSen->qualifier & 0x01) != 0;
				uint8_t Alarm_Flag = (pSen->qualifier & 0x02) != 0;
				uint8_t LowBat_Flag = (pSen->qualifier & 0x04) != 0;
				uint8_t Supervisor_Frame_Flag = (pSen->qualifier & 0x08) != 0;
				if (Supervisor_Frame_Flag != 0)
					return false;
			}
			else
			{
				//remote control (CodeSecure device)
				uint8_t Key_Id = pSen->qualifier;//values : 0x08, 0x10, 0x20, 0x40. (Nota : D0:7 of the id always set to 0)
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
			if (pSen->hygro>0)
				SendTempHumSensor(pSen->idPHY, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0f, pSen->hygro, "Temp+Hum");
			else
				SendTempSensor(pSen->idPHY, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0f, "Temp+Hum");
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

    return true;
}
