
#include "stdafx.h"
#include "ZiBlueBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"

#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>

#include "ziblue_usb_frame_api.h"

#ifdef _DEBUG
#define ENABLE_LOGGING
#define DEBUG_ZIBLUE
#endif
// Edisio Command Field
#define EDISIO_CMD_NULL 0x00		 // null instruction
#define EDISIO_CMD_ON 0x01		 // channel switched on
#define EDISIO_CMD_OFF 0x02		 // channel switched off
#define EDISIO_CMD_TOGGLE 0x03		 // channel toggled
#define EDISIO_CMD_DIM 0x04		 // channel dimmed to x% (1 additional byte hex 0x03 to 0x64)
#define EDISIO_CMD_DIM_UP 0x05		 // dim channel up 3%
#define EDISIO_CMD_DIM_DOWN 0x06	 // dim channel odnw 3%
#define EDISIO_CMD_DIM_A 0x07		 // dim channel toggled
#define EDISIO_CMD_DIM_STOP 0x08	 // dim stop
#define EDISIO_CMD_SHUTTER_OPEN 0x09	 // channel switched to open
#define EDISIO_CMD_SHUTTER_CLOSE 0x0A	 // channel switched to close
#define EDISIO_CMD_SHUTTER_CLOSE2 0x1B	 // channel switched to close (usb dongle v1.0)
#define EDISIO_CMD_SHUTTER_STOP 0x0B	 // channel switched to stop
#define EDISIO_CMD_RGB 0x0C		 // (3 bytes of additional data containing R, G and B)
#define EDISIO_CMD_RGB_C 0x0D		 // reserved
#define EDISIO_CMD_RGB_PLUS 0x0E	 // reserved
#define EDISIO_CMD_OPEN_SLOW 0x0F	 // reserved
#define EDISIO_CMD_SET_SHORT 0x10	 // paring request (only with CID value hex 0x09)
#define EDISIO_CMD_SET_5S 0x11		 // unparing one channel (only with CID value hex 0x09)
#define EDISIO_CMD_SET_10S 0x12		 // unparing all channels (only with CID value hex 0x09)
#define EDISIO_CMD_STUDY 0x16		 // paring request by gateway
#define EDISIO_CMD_DEL_BUTTON 0x17	 // reserved for gateway
#define EDISIO_CMD_DEL_ALL 0x18		 // reserved for gateway
#define EDISIO_CMD_SET_TEMPERATURE 0x19	 // temperature send by the sensor (2 bytes of additional field)
#define EDISIO_CMD_DOOR_OPEN 0x1A	 // status sent by the sensor
#define EDISIO_CMD_BROADCAST_QUERY 0x1F	 // reserved
#define EDISIO_CMD_QUERY_STATUS 0x20	 // request sent by the gateway toknow receiver status
#define EDISIO_CMD_REPORT_STATUS 0x21	 // response fromthe receiver after 'QUERY_STATUS' (n bytes additional data)
#define EDISIO_CMD_READ_CUSTOM 0x23	 // read customizable data
#define EDISIO_CMD_SAVE_CUSTOM 0x24	 // save customizable receiver data (n bytes)
#define EDISIO_CMD_REPORT_CUSTOM 0x29	 // response from receiver after READ_CUSTOM and SAVE_CUSTOM (n bytes)
#define EDISIO_CMD_SET_SHORT_DIMMER 0x2C // dimmer pairing request (n bytes)
#define EDISIO_CMD_SET_SHORT_SENSOR 0x2F // sensor paring request (n bytes)

// Edisio Model Field
//                                           Device            |     Function                         | Products ref
#define EDISIO_MODEL_EMITRBTN 0x01  // Emmiter 8 channels   | 8x On/Off/Pulse/Open/Stop/Close      | ETC1/ETC4/EBP8
#define EDISIO_MODEL_EMS_100 0x07   // Motion Sensor        | On/Off/Pulse/PilotWire/Open/Close    | EMS-100
#define EDISIO_MODEL_ETS_100 0x08   // Temperature Sensor   | Temperature                          | ETS-100
#define EDISIO_MODEL_EDS_100 0x09   // Door Sensor          | On/Off/Pulse/Open/Close              | EDS-100
#define EDISIO_MODEL_EMR_2000 0x10  // Receiver 1 Output    | 1x On/Off/Pulse                      | EMR-2000
#define EDISIO_MODEL_EMV_400 0x11   // Receiver 2 Outputs   | 2x On/Off or 1x Open/Stop/Close      | EMV-400
#define EDISIO_MODEL_EDR_D4 0x12    // Receiver 4 Outputs   | 4x On/Off or Dimmer                  | EDR-D4
#define EDISIO_MODEL_EDR_B4 0x13    // Receiver 4 Outputs   | 4x On/Off/Pulse or 1x Open/Stop/Close| EDR-B4
#define EDISIO_MODEL_EMSD_300A 0x14 // Receiver 1 Output    | 1x On/Off or Dimmer                  | EMSD-300A
#define EDISIO_MODEL_EMSD_300 0x15  // Receiver 1 Output    | 1x On/Off or Dimmer                  | EMSD-300
#define EDISIO_MODEL_EGW 0x16	    // Gateway              | Send and receive all commandes       | EGW
#define EDISIO_MODEL_EMM_230 0x17   // Emmiter 2 channels   | 2x On/Off/Pulse/Open/Stop/Close      | EMM-230
#define EDISIO_MODEL_EMM_100 0x18   // Emmiter 2 channels   | 2x On/Off/Pulse/Open/Stop/Close      | EMM-100
#define EDISIO_MODEL_ED_TH_01 0x0E  // Thermostat           | Thermostat                           | ED-TH-04
#define EDISIO_MODEL_ED_LI_01 0x0B  // Receiver 1 Output    | 1x On/Off                            | ED-LI-01
#define EDISIO_MODEL_ED_TH_02 0x0F  // Receiver 1 Output    | 1x On/Off Heater/Cooler              | ED-TH-02
#define EDISIO_MODEL_ED_TH_03 0x0C  // Receiver 1 Output FP | 1x Off/Eco/Confort/Auto (Fil Pilot)  | ED-TH-03
#define EDISIO_MODEL_ED_SH_01 0x0D  // Receiver 2 Outputs   | 1x Open/Stop/Close                   | ED-SH-01
#define EDISIO_MODEL_IR_TRANS 0x1B  // IR transmitter       | Trigger                              | EGW-PRO/EN
#define EDISIO_MODEL_STM_250 0x1E   // Enocean sensor       | Door                                 | STM-250
#define EDISIO_MODEL_STM_330 0x1F   // Enocean sensor       | Temperature                          | STM-330
#define EDISIO_MODEL_PTM_210 0x20   // Enocean sensor       | Switch                               | PTM-210
#define EDISIO_MODEL_IR_DEVICE 0x21 // Virtual device       | IR remote control                    | EGW-PRO/EN
#define EDISIO_MODEL_EN_DET 0x22    // Enocean motion       | Motion                               | EGW-PRO/EN
#define EDISIO_MODEL_EN_SOC 0x23    // Enocean socket       | Socket                               | EGW-PRO/EN

typedef struct _STR_TABLE_SINGLE
{
	unsigned long id;
	const char *str1;
	const char *str2;
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
		{ 17, "reserved" },	     //
		{ 18, "reserved" },	     //
		{ 19, "reserved" },	     //
		{ 20, "reserved" },	     //
		{ 21, "reserved" },	     //
		{ 22, "FS20_868" },	     //
		{ 22, "EDISIO" },	     //

		{ 0, nullptr },		     //
	};
	return findTableIDSingle1(Table, id);
}

// 0: OFF, 1 : ON, 2 : BRIGHT, 3 : DIM, 4 : ALL_LIGHTS_ON, 5 : ALL_LIGHTS_OFF

const char *szZiBlueProtocol(const unsigned char id)
{
	static const STR_TABLE_SINGLE Table[] = {
		{ RECEIVED_PROTOCOL_X10,		"X10"		},
		{ RECEIVED_PROTOCOL_VISONIC,	"VISONIC"	},
		{ RECEIVED_PROTOCOL_BLYSS,		"BLYSS"		},
		{ RECEIVED_PROTOCOL_CHACON,		"CHACON"	},
		{ RECEIVED_PROTOCOL_OREGON,		"OREGON"	},
		{ RECEIVED_PROTOCOL_DOMIA,		"DOMIA"		},
		{ RECEIVED_PROTOCOL_OWL,		"OWL"		},
		{ RECEIVED_PROTOCOL_X2D,		"X2D"		},
		{ RECEIVED_PROTOCOL_RFY,		"RTS"		},
		{ RECEIVED_PROTOCOL_KD101,		"KD101"		},
		{ RECEIVED_PROTOCOL_PARROT,		"PARROT"	},
		{ RECEIVED_PROTOCOL_DIGIMAX,	"DIGIMAX"	}, // deprecated
		{ RECEIVED_PROTOCOL_TIC,		"TIC"		},
		{ RECEIVED_PROTOCOL_FS20,		"FS20"		},
		{ RECEIVED_PROTOCOL_JAMMING,	"JAMMING"	},
		{ RECEIVED_PROTOCOL_EDISIO,		"EDISIO"	},
		{ 0,							nullptr		},
	};
	return findTableIDSingle1(Table, id);
}

struct _tZiBlueStringIntHelper
{
	std::string szType;
	int gType;
};

	const _tZiBlueStringIntHelper ziblue_switches[] = {
	{ "VISONIC 433", sSwitchTypeVisonic433 },
	{ "VISONIC 868", sSwitchTypeVisonic868 },
	{ "CHACON", sSwitchTypeAC },
	{ "DOMIA", sSwitchTypeARC },
	{ "X10", sSwitchTypeX10 },
	{ "X2D 433", sSwitchTypeX2D433 },
	{ "X2D 868", sSwitchTypeX2D868 },
	{ "X2D SHUTTER", sSwitchTypeX2DShutter },
	{ "X2D ELEC", sSwitchTypeX2DElec },
	{ "X2D GAS", sSwitchTypeX2DGas },
	{ "RTS", sSwitchTypeRTS },
	{ "BLYSS", sSwitchTypeBlyss },
	{ "PARROT", sSwitchTypeParrot },
	{ "KD101", sSwitchTypeKD101 },
	{ "FS20 868", sSwitchTypeFS20 },
	{ "EDISIO", sSwitchTypeForest },
	{ "JAMMING SIMU", sSwitchTypeRFCustom },
	{ "JAMMING", -2 },
	{ "", -1 }
};

const _tZiBlueStringIntHelper ziBlueswitchcommands[] = {
	{ "ON", gswitch_sOn },
	{ "OFF", gswitch_sOff },
	{ "ALLON", gswitch_sGroupOn },
	{ "ALLOFF", gswitch_sGroupOff },
	{ "DIM", gswitch_sDim },
	{ "BRIGHT", gswitch_sBright },
	{ "STOP", gswitch_sStop },
	{ "", -1 }
};

int GetGeneralZiBlueFromString(const _tZiBlueStringIntHelper *pTable, const std::string &szType)
{
	int ii = 0;
	while (pTable[ii].gType != -1)
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
	while (pTable[ii].gType != -1)
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

static std::string jsonConfig=R"([
	{"idx":"1","name":"Visonic 433MHz","parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"2","name":"Visonic 868MHz","parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"3","name":"Chacon",        "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"4","name":"Domia",         "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"5","name":"X10",           "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"6","name":"X2D 433MHz",    "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"7","name":"X2D 868MHz",    "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"8","name":"X2D SHUTTER",   "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"9","name":"X2D ELEC",      "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"10","name":"X2D GAS",      "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"11","name":"RTS",          "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"},{"name":"qualifier","display":"Qualifier","type":"bool"}]},
	{"idx":"12","name":"Blyss",        "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"13","name":"Parrot",       "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"16","name":"KD101",        "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"22","name":"FS20 868",     "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]},
	{"idx":"23","name":"EDISIO",       "parameters":[{"name":"housecode","display":"House Code","type":"housecode16"},{"name":"unitcode","display":"Unit Code","type":"unitcode16"}]}
	])";

std::string CZiBlueBase::GetManualSwitchesJsonConfiguration() const
{
	return jsonConfig;
}
void CZiBlueBase::GetManualSwitchParameters(const std::multimap<std::string, std::string> &Parameters, _eSwitchType & SwitchTypeInOut, int & LightTypeInOut,
										int & dTypeOut, int &dSubTypeOut, std::string &devIDOut, std::string &sUnitOut) const
{
	dTypeOut = pTypeGeneralSwitch;
	switch (LightTypeInOut)
	{
		case SEND_VISONIC_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeVisonic433;
			break;
		case SEND_VISONIC_PROTOCOL_868:
			dSubTypeOut = sSwitchTypeVisonic868;
			break;
		case SEND_CHACON_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeAC;
			break;
		case SEND_DOMIA_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeARC;
			break;
		case SEND_X10_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeX10;
			break;
		case SEND_X2D_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeX2D433;
			break;
		case SEND_X2D_PROTOCOL_868:
			dSubTypeOut = sSwitchTypeX2D868;
			break;
		case SEND_X2D_SHUTTER_PROTOCOL_868:
			dSubTypeOut = sSwitchTypeX2DShutter;
			break;
		case SEND_X2D_HA_ELEC_PROTOCOL_868:
			dSubTypeOut = sSwitchTypeX2DElec;
			break;
		case SEND_X2D_HA_GASOIL_PROTOCOL_868:
			dSubTypeOut = sSwitchTypeX2DGas;
			break;
		case SEND_SOMFY_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeRTS;
			break;
		case SEND_BLYSS_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeBlyss;
			break;
		case SEND_PARROT:
			dSubTypeOut = sSwitchTypeParrot;
			break;
		case SEND_KD101_PROTOCOL_433:
			dSubTypeOut = sSwitchTypeKD101;
			break;
		case SEND_FS20_868:
			dSubTypeOut = sSwitchTypeFS20;
			break;
	}
	auto it = Parameters.find("housecode");
	if (it != Parameters.end())
	{
		int ID = atoi(it->second.c_str());
		std::stringstream s_strid;
		s_strid << std::hex << std::setfill('0') << std::setw(8) << ID;
		devIDOut = s_strid.str();
	}
	it = Parameters.find("unitcode");
	if (it != Parameters.end())
	{
		sUnitOut = it->second;
	}
	if (LightTypeInOut == SEND_SOMFY_PROTOCOL_433)
	{
		it = Parameters.find("qualifier");
		if (it != Parameters.end())
		{
			int id = atoi(devIDOut.c_str());
			int qualifier = it->second == "true" ? 1 : 0; 
			id |= qualifier << 16;
			std::stringstream temp;
			temp << std::setfill('0') << std::setw(8) << id;
			devIDOut = temp.str();
		}
	}
}

void CZiBlueBase::OnConnected()
{
	Init();
	WriteInt("ZIA++FORMAT BINARY\r");
	WriteInt("ZIA++FORMAT RFLINK BINARY\r");
	m_bRFLinkOn = true;
	WriteInt("ZIA++REPEATER OFF\r");
	SendSwitchInt(0, -2, 255, "JAMMING", "OFF", 0, "Jamming");
	SendSwitchInt(1, sSwitchTypeRFCustom, 255, "JAMMING SIMU", "OFF", 0, "Jamming Simulation");
}

void CZiBlueBase::OnDisconnected()
{
	Init();
}

#define round(a) (int)(a + .5)

bool CZiBlueBase::WriteToHardware(const char *pdata, const unsigned char length)
{
	const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch *>(pdata);

	if (pSwitch->type != pTypeGeneralSwitch)
		return false;
	std::string protocol = GetGeneralZiBlueFromInt(ziblue_switches, pSwitch->subtype);
	if (protocol.empty())
	{
		Log(LOG_ERROR, "trying to send unknown switch type: %d", pSwitch->subtype);
		return false;
	}
	else if (protocol == "JAMMING")
	{
		// JAMMING is read only
		return false;
	}

	if (pSwitch->type == pTypeGeneralSwitch)
	{
		std::string switchcmnd = GetGeneralZiBlueFromInt(ziBlueswitchcommands, pSwitch->cmnd);
		if (switchcmnd.empty())
		{
			Log(LOG_ERROR, "trying to send unknown switch command: %d", pSwitch->cmnd);
			return false;
		}
		// check setlevel command
		if (pSwitch->cmnd == gswitch_sSetLevel)
		{
			// Get device level to set
			float fvalue = (15.0F / 100.0F) * float(pSwitch->level);
			if (fvalue > 15.0F)
				fvalue = 15.0F; // 99 is fully on
			int svalue = round(fvalue);
			char buffer[50] = { 0 };
			sprintf(buffer, "%d", svalue);
			switchcmnd = buffer;
		}

		struct _tOutgoingFrame
		{
			// struct  MESSAGE_CONTAINER_HEADER
			unsigned char Sync1;
			unsigned char Sync2;
			unsigned char SourceDestQualifier;
			unsigned char QualifierOrLen_lsb;
			unsigned char QualifierOrLen_msb;

			// struct REGULAR_INCOMING_BINARY_USB_FRAME
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
		if (protocol == "X10")
		{
			oFrame.protocol = SEND_X10_PROTOCOL_433;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
			// no return state, so change its value now
			sDecodeRXMessage(this, (const unsigned char *)&pSwitch, "X10", 100, m_Name.c_str());
		}
		else if (protocol == "CHACON")
		{
			oFrame.protocol = SEND_CHACON_PROTOCOL_433;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
			// no return state, so change its value now
			sDecodeRXMessage(this, (const unsigned char *)&pSwitch, "Chacon", 100, m_Name.c_str());
		}
		else if (protocol == "DOMIA")
		{
			oFrame.protocol = SEND_DOMIA_PROTOCOL_433;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
			// no return state, so change its value now
			sDecodeRXMessage(this, (const unsigned char *)&pSwitch, "Domia", 100, m_Name.c_str());
		}
		else if (protocol == "RTS")
		{
			oFrame.protocol = SEND_SOMFY_PROTOCOL_433;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
			oFrame.qualifier = (pSwitch->id & 0x00FF0000) != 0 ? 1 : 0;
			// no return state, so change its value now
			sDecodeRXMessage(this, (const unsigned char *)&pSwitch, "RTS", 100, m_Name.c_str());
		}
		else if (protocol == "VISONIC 433")
		{
			oFrame.protocol = SEND_VISONIC_PROTOCOL_433;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "VISONIC 868")
		{
			oFrame.protocol = SEND_VISONIC_PROTOCOL_868;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "X2D 433")
		{
			oFrame.protocol = SEND_X2D_PROTOCOL_433;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "X2D 868")
		{
			oFrame.protocol = SEND_X2D_PROTOCOL_868;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "X2D SHUTTER")
		{
			oFrame.protocol = SEND_X2D_SHUTTER_PROTOCOL_868;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "X2D ELEC")
		{
			oFrame.protocol = SEND_X2D_HA_ELEC_PROTOCOL_868;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "X2D GAS")
		{
			oFrame.protocol = SEND_X2D_HA_GASOIL_PROTOCOL_868;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "BLYSS")
		{
			oFrame.protocol = SEND_BLYSS_PROTOCOL_433;
			oFrame.ID_1 = ((((pSwitch->id & 0xFF) - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "PARROT")
		{
			oFrame.protocol = SEND_PARROT;
			oFrame.ID_1 = (((pSwitch->id - 65) << 4) + (pSwitch->unitcode - 1));
			sDecodeRXMessage(this, (const unsigned char *)&pSwitch, "Parrot", 100, m_Name.c_str());
		}
		else if (protocol == "KD101")
		{
			oFrame.protocol = SEND_KD101_PROTOCOL_433;
			oFrame.ID_1 = ((((pSwitch->id & 0xFF) - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "FS20 868")
		{
			oFrame.protocol = SEND_FS20_868;
			oFrame.ID_1 = ((((pSwitch->id & 0xFF) - 65) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "EDISIO")
		{
			oFrame.protocol = SEND_EDISIO;
			oFrame.ID_1 = (((pSwitch->id - 10) << 4) + (pSwitch->unitcode - 1));
		}
		else if (protocol == "JAMMING SIMU") // active jamming simulation
		{
			WriteInt("ZIA++JAMMING SIMULATE\r");
			return true;
		}
		else
		{
			Log(LOG_ERROR, "unsupported switch protocol");
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
			case gswitch_sStop:
				oFrame.action = SEND_ACTION_DIM;
				oFrame.dimValue = 50; 
				break;
		}

		WriteInt((const uint8_t *)&oFrame, sizeof(_tOutgoingFrame));
		return true;
	}
	Log(LOG_ERROR, "Switch is not General Type: %d", pSwitch->type);
	return false;
}

bool CZiBlueBase::SendSwitchInt(const int ID, const int switchunit, const int BatteryLevel, const std::string &switchType, const std::string &switchcmd, const int level, const std::string& defaultName)
{
	int intswitchtype = GetGeneralZiBlueFromString(ziblue_switches, switchType);
	if (intswitchtype == -1)
	{
		Log(LOG_ERROR, "Unhandled switch type: %s", switchType.c_str());
		return false;
	}

	int cmnd = GetGeneralZiBlueFromString(ziBlueswitchcommands, switchcmd);

	int svalue = level;
	if (cmnd == -1)
	{
		if (switchcmd.compare(0, 10, "SET_LEVEL=") == 0)
		{
			cmnd = gswitch_sSetLevel;
			std::string str2 = switchcmd.substr(10);
			svalue = atoi(str2.c_str());
			Log(LOG_STATUS, "%d level: %d", cmnd, svalue);
		}
	}

	if (cmnd == -1)
	{
		Log(LOG_ERROR, "Unhandled switch command: %s", switchcmd.c_str());
		return false;
	}

	_tGeneralSwitch gswitch;
	gswitch.subtype = intswitchtype;
	gswitch.id = ID;
	gswitch.unitcode = switchunit;
	gswitch.cmnd = cmnd;
	gswitch.level = svalue; // level;
	gswitch.battery_level = BatteryLevel;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, defaultName.c_str(), BatteryLevel, m_Name.c_str());
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
					// Binair
					if (m_Length >= 1000)
					{
						m_State = ZIBLUE_STATE_ZI_1;
						break;
					}
					m_State = ZIBLUE_STATE_DATA;
				}
				else
				{
					// Ascii, only accept JSON
					if ((m_QL_1 != '3') || (m_QL_2 != '3'))
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
					ParseBinary(m_SDQ, (const uint8_t *)&m_rfbuffer, m_Length);
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
					if ((data[ii] == '\n') && (data[ii + 1] == '\r'))
					{
						// Frame done
						ParseBinary(m_SDQ, (const uint8_t *)&m_rfbuffer, m_rfbufferpos);
						m_State = ZIBLUE_STATE_ZI_1;
					}
				}
				m_rfbuffer[m_rfbufferpos++] = data[ii];
				if (m_rfbufferpos >= ZiBlue_READ_BUFFER_SIZE)
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
	uint8_t vtype = (SDQ & 0x70) >> 4; // 0x0 = binary, 0x4 = ascii
	if (vtype != 0)
		return false;

	uint8_t SourceDest = SDQ & 0x0F;

	if (SourceDest > 1)
		return false; // not supported

	// 0 = MUX Management
	// 1 = 433 / 868

	uint8_t FrameType = data[0];

	if (FrameType == 1)
	{
		// RFLink
		if (len < 24)
			return false;
		const uint8_t *pData = (const uint8_t *)data;

		FrameType = pData[0];
		uint32_t frequency = (uint32_t)(pData[4] << 24) + (uint32_t)(pData[3] << 16) + (uint32_t)(pData[2] << 8) + (uint32_t)pData[1];
		int8_t RFLevel = pData[5];
		int8_t FloorNoise = pData[6];
		uint8_t PulseElementSize = pData[7];
		uint16_t number = (pData[9] << 8) + pData[8];
		uint8_t Repeats = pData[10];
		uint8_t Delay = pData[11];
		uint8_t Multiply = pData[12];
		uint32_t Time = (uint32_t)(pData[16] << 24) + (uint32_t)(pData[15] << 16) + (uint32_t)(pData[14] << 8) + (uint32_t)pData[13];
		Log(LOG_NORM, "frameType: %d, frequency: %d kHz", FrameType, frequency);
		Log(LOG_NORM, "rfLevel: %d dBm, floorNoise: %d dBm, PulseElementSize: %d", RFLevel, FloorNoise, PulseElementSize);
		Log(LOG_NORM, "number: %d, Repeats: %d, Delay: %d, Multiply: %d", number, Repeats, Delay, Multiply);
	}
	else if (FrameType == 0)
	{
		// Normal RF
		int dlen = len - 8;
		REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER *pIncomming = (REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER *)data;
#ifdef DEBUG_ZIBLUE
		Log(LOG_NORM, "frameType: %d, cluster: %d, dataFlag: %d (%s MHz)", pIncomming->frameType, pIncomming->cluster, pIncomming->dataFlag, (pIncomming->dataFlag == 0) ? "433" : "868");
		Log(LOG_NORM, "rfLevel: %d dBm, floorNoise:: %d dBm, rfQuality: %d", pIncomming->rfLevel, pIncomming->floorNoise, pIncomming->rfQuality);
		Log(LOG_NORM, "protocol: %d (%s), infoType: %d", pIncomming->protocol, szZiBlueProtocol(pIncomming->protocol), pIncomming->infoType);
#endif
		switch (pIncomming->infoType)
		{
			case INFOS_TYPE0:
				// used by X10 / Domia Lite protocols
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE0))
				{
					INCOMING_RF_INFOS_TYPE0 *pSen = (INCOMING_RF_INFOS_TYPE0 *)(data + 8);
					uint8_t houseCode = (pSen->id & 0xF0) >> 4;
					uint8_t dev = pSen->id & 0x0F;
					std::string switchCmd;
					switch (pSen->subtype)
					{
						case 0:
							switchCmd = "OFF";
							break;
						case 1:
							switchCmd = "ON";
							break;
						case 2:
							switchCmd = "BRIGHT";
							break;
						case 3:
							switchCmd = "DIM";
							break;
						case 4:
							switchCmd = "ALLOFF";
							break;
						case 5:
							switchCmd = "ALLON";
							break;
						default:
							switchCmd = "UNKNOWN";
							break;
					}
					const char *szProtocol = szZiBlueProtocol(pIncomming->protocol);
					if (szProtocol != nullptr)
					{
						SendSwitchInt(houseCode, dev, 255, std::string(szProtocol), switchCmd, 0, std::string(szProtocol) );
					}
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d (%s), houseCode: %c, dev: %d", pSen->subtype, switchCmd.c_str(), (uint8_t)('A' + houseCode), dev + 1);
#endif
				}
				break;
			case INFOS_TYPE1:
				// Used by X10 (24/32 bits ID),  CHACON , KD101, BLYSS, JAMMING
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE1))
				{
					INCOMING_RF_INFOS_TYPE1 *pSen = (INCOMING_RF_INFOS_TYPE1 *)(data + 8);
					int DevID = (pSen->idMsb << 16) + pSen->idLsb;
					std::string switchCmd;
					const char *szProtocol = szZiBlueProtocol(pIncomming->protocol);
					switch (pSen->subtype)
					{
						case 0:
							switchCmd = "OFF";
							break;
						case 1:
							switchCmd = "ON";
							break;
						case 2:
							switchCmd = "BRIGHT";
							break;
						case 4:
							switchCmd = "ALLOFF";
							break;
						case 5:
							switchCmd = "ALLON";
							break;
						default:
							switchCmd = "UNKNOWN";
							break;
					}
					if (pIncomming->protocol == RECEIVED_PROTOCOL_JAMMING)
					{
						SendSwitchInt(0, -2, 255, "JAMMING", switchCmd, 0, "Jamming");
						if (switchCmd == "OFF")
							SendSwitchInt(1, sSwitchTypeRFCustom, 255, "JAMMING SIMU", "OFF", 0, "Jamming Simulation");
						break;
					}
					if (szProtocol != nullptr)
					{
						SendSwitchInt(DevID, 1, 255, std::string(szProtocol), switchCmd, 0, std::string(szProtocol));
					}
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d (%s), DevID: %04X", pSen->subtype, switchCmd.c_str(), DevID);
#endif
				}
				break;
			case INFOS_TYPE2:
				// Used by  VISONIC /Focus/Atlantic/Meian Tech
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE2))
				{
					INCOMING_RF_INFOS_TYPE2 *pSen = (INCOMING_RF_INFOS_TYPE2 *)(data + 8);
					int DevID = (pSen->idMsb << 16) + pSen->idLsb;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "DevID: %04X", DevID);
#endif
					if (pSen->subtype == 0)
					{
						// detector/sensor( PowerCode device)
						uint8_t Tamper_Flag = (pSen->qualifier & 0x01) != 0;
						uint8_t Alarm_Flag = (pSen->qualifier & 0x02) != 0;
						uint8_t LowBat_Flag = (pSen->qualifier & 0x04) != 0;
						uint8_t Supervisor_Frame_Flag = (pSen->qualifier & 0x08) != 0;
						if (Supervisor_Frame_Flag != 0)
							return false;
#ifdef DEBUG_ZIBLUE
						Log(LOG_NORM, "Tamper_Flag: %d, Alarm_Flag: %d, LowBat_Flag: %d, Supervisor_Frame_Flag:%d", Tamper_Flag, Alarm_Flag, LowBat_Flag,
						    Supervisor_Frame_Flag);
#endif
					}
					else
					{
						// remote control (CodeSecure device)
						uint16_t Key_Id = pSen->qualifier; // values : 0x08, 0x10, 0x20, 0x40. (Nota : D0:7 of the id always set to 0)
#ifdef DEBUG_ZIBLUE
						Log(LOG_NORM, "Key_Id: %04X", Key_Id);
#endif
						const char *szProtocol = szZiBlueProtocol(pIncomming->protocol);
						std::string switchCmd;
						switch (pSen->qualifier)
						{
							case 0x08:
								switchCmd = "UNKNOWN";
								break;
							case 0x10:
								switchCmd = "ON";
								break;
							case 0x20:
								switchCmd = "STOP";
								break;
							case 0x40:
								switchCmd = "OFF";
								break;
							default:
								switchCmd = "UNKNOWN";
								break;
						}
						SendSwitchInt(pSen->idMsb ^ pSen->idLsb, 1, 255, std::string(szProtocol), switchCmd, 0, "Visonic Remote");
					}
				}
				break;
			case INFOS_TYPE3:
				//  Used by RFY PROTOCOL
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE3))
				{
					INCOMING_RF_INFOS_TYPE3 *pSen = (INCOMING_RF_INFOS_TYPE3 *)(data + 8);
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idLSB: %04X, idMSB: %04X, qualifier: %04X", pSen->subtype, pSen->idLsb, pSen->idMsb, 
						pSen->qualifier);
#endif
					std::string cmd = "UNKNOWN";
					if (pSen->qualifier == 1)
					{
						cmd = "OFF";
					}
					else if (pSen->qualifier == 4)
					{
						cmd = "STOP";
					}
					else if (pSen->qualifier == 7)
					{
						cmd = "ON";
					}
					SendSwitchInt(pSen->idLsb ^ pSen->idMsb, 1, 255, "RTS", cmd, 0, "RTS Remote");
				}
				break;
			case INFOS_TYPE4:
				// Used by  Scientific Oregon  protocol ( thermo/hygro sensors)
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE4))
				{
					INCOMING_RF_INFOS_TYPE4 *pSen = (INCOMING_RF_INFOS_TYPE4 *)(data + 8);
					uint16_t random = pSen->idChannel & 0xFF80;
					uint16_t channel = pSen->idChannel & 0x007F;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, random: %X, channel: %d, temp: %.1f, hygro: %d", pSen->subtype, pSen->idPHY, pSen->idChannel, pSen->qualifier,
					    random, channel, float(pSen->temp) / 10.0f, pSen->hygro);
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
					INCOMING_RF_INFOS_TYPE5 *pSen = (INCOMING_RF_INFOS_TYPE5 *)(data + 8);
					uint16_t random = pSen->idChannel & 0xFF80;
					uint16_t channel = pSen->idChannel & 0x007F;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, random: %X, channel: %d,temp: %.1f, hygro: %d, pressure: %d", pSen->subtype, pSen->idPHY, pSen->idChannel,
					    pSen->qualifier, random, channel, float(pSen->temp) / 10.0f, pSen->hygro, pSen->pressure);
#endif
					SendTempHumBaroSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->temp) / 10.0F, pSen->hygro, pSen->pressure, 0,
							      "Temp+Hum+Baro");
				}
				break;
			case INFOS_TYPE6:
				// Used by  Scientific Oregon  protocol  (Wind sensors)
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE6))
				{
					INCOMING_RF_INFOS_TYPE6 *pSen = (INCOMING_RF_INFOS_TYPE6 *)(data + 8);
					uint16_t random = pSen->idChannel & 0xFF80;
					uint16_t channel = pSen->idChannel & 0x007F;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, speed: %.1f, direction: %d", pSen->subtype, pSen->idPHY, pSen->idChannel,
					    pSen->qualifier, float(pSen->speed), pSen->direction);
#endif
					SendWind(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, pSen->direction, float(pSen->speed),
						0, 0, 0, false, false, "Wind");
				}
				break;
			case INFOS_TYPE7:
				// Used by  Scientific Oregon  protocol  ( UV  sensors)
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE7))
				{
					INCOMING_RF_INFOS_TYPE7 *pSen = (INCOMING_RF_INFOS_TYPE7 *)(data + 8);
					uint16_t random = pSen->idChannel & 0xFF80;
					uint16_t channel = pSen->idChannel & 0x007F;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, uv: %.1f", pSen->subtype, pSen->idPHY, pSen->idChannel,
					    pSen->qualifier, float(pSen->light)/10);
#endif
					SendSolarRadiationSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->light)/10, "UV Index");
				}
				break;
			case INFOS_TYPE8:
				// Used by  OWL  ( Energy/power sensors)
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE8))
				{
					INCOMING_RF_INFOS_TYPE8 *pSen = (INCOMING_RF_INFOS_TYPE8 *)(data + 8);
					uint16_t random = pSen->idChannel & 0xFFF0;
					uint16_t channel = pSen->idChannel & 0x000F;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, power: %d, p1: %d, p2: %d, p3: %d", pSen->subtype, pSen->idPHY, pSen->idChannel,
					    pSen->qualifier, pSen->power, pSen->powerI1, pSen->powerI2, pSen->powerI3);
#endif
					SendWattMeter(pSen->idPHY ^ pSen->idChannel, 0, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->power) / 10, "Power");
					int energy = (pSen->energyMsb << 16) + pSen->energyLsb;
					SendKwhMeter(pSen->idPHY ^ pSen->idChannel, 1, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->power), float(energy) / 1000, "Energy");
					if ((pSen->qualifier & 0x02) != 0)
					{
						SendWattMeter(pSen->idPHY ^ pSen->idChannel, 2, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->power) / 10, "Power I1");
						SendWattMeter(pSen->idPHY ^ pSen->idChannel, 3, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->power) / 10, "Power I2");
						SendWattMeter(pSen->idPHY ^ pSen->idChannel, 4, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->power) / 10, "Power I3");
					}
				}
				break;
			case INFOS_TYPE9:
				// Used by  OREGON  ( Rain sensors)
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE9))
				{
					INCOMING_RF_INFOS_TYPE9 *pSen = (INCOMING_RF_INFOS_TYPE9 *)(data + 8);
					uint16_t random = pSen->idChannel & 0xFF80;
					uint16_t channel = pSen->idChannel & 0x007F;
					int totalRain = pSen->totalRainLsb + (int(pSen->totalRainMsb) << 16);
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, idPHY: %04X, idChannel: %04X, qualifier: %04X, rain: %d, totalRain: %d", pSen->subtype, pSen->idPHY, pSen->idChannel,
					    pSen->qualifier, pSen->rain, totalRain);
#endif
					SendRainRateSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(pSen->rain) / 100, "RainRate");
					SendRainSensor(pSen->idPHY ^ pSen->idChannel, (pSen->qualifier & 0x01) ? 0 : 100, float(totalRain) / 10, "TotalRain");
				}
				break;
			case INFOS_TYPE10:
				// Used by Thermostats  X2D protocol
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE10))
				{
					INCOMING_RF_INFOS_TYPE10 *pSen = (INCOMING_RF_INFOS_TYPE10 *)(data + 8);
					int DevID = pSen->idLsb + (int(pSen->idMsb) << 16);
					uint8_t Tamper_Flag = (pSen->qualifier & 0x01) != 0;
					uint8_t Anomaly_Flag = (pSen->qualifier & 0x02) != 0;
					uint8_t LowBat_Flag = (pSen->qualifier & 0x04) != 0;
					uint8_t TestAssoc_Flag = (pSen->qualifier & 0x10) != 0;
					uint8_t DomesticFrame_Flag = (pSen->qualifier & 0x20) != 0;
					uint8_t x2dVariant = (pSen->qualifier & 0xC0) >> 6;
					// Function values:
					//0 : SPECIAL, 1 : HEATING_SPEED, 2 : OPERATING_MODE, 12 : REGULATION, 26 : THERMIC_AREA_STATE 
					int function = pSen->function & 0xFF;
					// State values (OPERATING_MODE ):
					// 0: ECO, 1 : MODERAT, 2 : MEDIO, 3 : COMFORT, 4 : STOP, 5 : OUT OF FROST, 6 : SPECIAL, 7 : AUTO, 8 : CENTRALISED
					// State values (other cases):
					// 0: OFF, 1: ON
					int state = pSen->mode & 0xFF; 
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM,
					    "subtype: %d, id: %04X, qualifier: %04X, tamperFlag: %d, alarmFlag: %d, lowBatteryFlag: %d,"
					    "testAssocFlag: %d, domesticFrameFlag: %d, x2variant: %d, function: %d, state: %d",
					    pSen->subtype, DevID, pSen->qualifier, Tamper_Flag, Anomaly_Flag, LowBat_Flag, TestAssoc_Flag,
						DomesticFrame_Flag, x2dVariant, function, state);
#endif
				}
				break;
			case INFOS_TYPE11:
				// Used by Alarm/remote control devices  X2D protocol
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE11))
				{
					INCOMING_RF_INFOS_TYPE11 *pSen = (INCOMING_RF_INFOS_TYPE11 *)(data + 8);
					int id = pSen->idLsb + (int(pSen->idMsb) << 16);
					int Tamper_Flag = (pSen->qualifier & 0x01) != 0;
					int Alarm_Flag = (pSen->qualifier & 0x02) != 0;
					int LowBat_Flag = (pSen->qualifier & 0x04) != 0;
					int TestAssoc_Flag = (pSen->qualifier & 0x10) != 0;
					int DomesticFrame_Flag = (pSen->qualifier & 0x20) != 0;
					int x2dVariant = (pSen->qualifier & 0xC0) >> 6;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM, "subtype: %d, id: %04X, qualifier: %04X, tamperFlag: %d, alarmFlag: %d, lowBatteryFlag: %d,"
						"testAssocFlag: %d, domesticFrameFlag: %d, x2variant: %d", pSen->subtype, id, pSen->qualifier, 
						Tamper_Flag, Alarm_Flag, LowBat_Flag, TestAssoc_Flag, DomesticFrame_Flag, x2dVariant);
#endif
				}
				break;
			case INFOS_TYPE12:
				// deprecated // Used by  DIGIMAX TS10 protocol
#ifdef DEBUG_ZIBLUE
				Log(LOG_NORM, "DEPRECATED DIGIMAX TS10 protocol");
#endif
				break;
			case INFOS_TYPE13:
				// Used by  Cartelectronic TIC/Pulses devices (Teleinfo/TeleCounters)
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE13))
				{
					INCOMING_RF_INFOS_TYPE13 *pSen = (INCOMING_RF_INFOS_TYPE13 *)(data + 8);
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM,
					    "subtype: %d, idLsb: %04X, idMsb: %04X, qualifier: %04X, infos: %04X, counter1Lsb: %d, counter1Msb: %d, counter2Lsb: %d, counter2Msb: %d, apparentPower: "
					    "%d",
					    pSen->subtype, pSen->idLsb, pSen->idMsb, pSen->qualifier, pSen->infos, pSen->counter1Lsb, pSen->counter1Msb, pSen->counter2Lsb, pSen->counter2Msb,
					    pSen->apparentPower);
#endif

					int total1 = pSen->counter1Msb;
					total1 = total1 << 16;
					total1 += pSen->counter1Lsb;
					int total2 = pSen->counter2Msb;
					total2 = total2 << 16;
					total2 += pSen->counter2Lsb;
					double power1 = 0;
					double power2 = 0;
					if (m_LastReceivedKWhMeterValue.find(pSen->idLsb ^ pSen->idMsb ^ 1) != m_LastReceivedKWhMeterValue.end())
					{
						power1 = (total1 - m_LastReceivedKWhMeterValue[pSen->idLsb ^ pSen->idMsb ^ 1]) /
							 ((m_LastReceivedTime - m_LastReceivedKWhMeterTime[pSen->idLsb ^ pSen->idMsb ^ 1]) / 3600.0);
						power2 = (total2 - m_LastReceivedKWhMeterValue[pSen->idLsb ^ pSen->idMsb ^ 2]) /
							 ((m_LastReceivedTime - m_LastReceivedKWhMeterTime[pSen->idLsb ^ pSen->idMsb ^ 2]) / 3600.0);
						power1 = round(power1);
						power2 = round(power2);
					}
					SendKwhMeter(pSen->idLsb ^ pSen->idMsb, 1, (pSen->qualifier & 0x01) ? 0 : 100, power1, total1 / 1000.0, "HC");
					SendKwhMeter(pSen->idLsb ^ pSen->idMsb, 2, (pSen->qualifier & 0x01) ? 0 : 100, power2, total2 / 1000.0, "HP");
					SendWattMeter(pSen->idLsb ^ pSen->idMsb, 3, (pSen->qualifier & 0x01) ? 0 : 100, (float)pSen->apparentPower, "Apparent Power");
					m_LastReceivedKWhMeterTime[pSen->idLsb ^ pSen->idMsb ^ 1] = m_LastReceivedTime;
					m_LastReceivedKWhMeterValue[pSen->idLsb ^ pSen->idMsb ^ 1] = total1;
					m_LastReceivedKWhMeterTime[pSen->idLsb ^ pSen->idMsb ^ 2] = m_LastReceivedTime;
					m_LastReceivedKWhMeterValue[pSen->idLsb ^ pSen->idMsb ^ 2] = total2;
				}
				break;
			case INFOS_TYPE14:
				// Used by FS20
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE14))
				{
					INCOMING_RF_INFOS_TYPE14 *pSen = (INCOMING_RF_INFOS_TYPE14 *)(data + 8);
					uint16_t houseCode = pSen->idLsb;
					uint8_t adress = ((pSen->idMsb&0xFF00) >> 8);
					uint8_t cmd = pSen->subtype & 0xFF;
					if (cmd == 0x11)
					{
						//ON
					}
					else
					{
						//0x00: OFF
					}
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM,
					    "subtype: %d, idLsb: %04X, idMsb: %04X, qualifier: %04X",
					    pSen->subtype, pSen->idLsb, pSen->idMsb, pSen->qualifier);
#endif
				}
				break;
			case INFOS_TYPE15:
				// Used by EDISIO
				if (dlen == sizeof(INCOMING_RF_INFOS_TYPE15))
				{
					INCOMING_RF_INFOS_TYPE15 *pSen = (INCOMING_RF_INFOS_TYPE15 *)(data + 8);
					char mid = pSen->infos & 0xFF;
					char batteryLevel = (pSen->infos & 0xFF00) >> 8;
#ifdef DEBUG_ZIBLUE
					Log(LOG_NORM,
					    "subtype: %d, idLsb: %04X, idMsb: %04X, qualifier: %04X, MID: %04X, battery: %04X, additionalData: %d",
					    pSen->subtype, pSen->idLsb, pSen->idMsb, pSen->qualifier, mid, batteryLevel, pSen->additionalData);
#endif
				}
				break;
		}
	}
	else
	{
		Log(LOG_ERROR, "Unknown Frametype received: %d", FrameType);
	}
#ifdef DEBUG_ZIBLUE
	Log(LOG_NORM, "----------------");
#endif
	return true;
}
