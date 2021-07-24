/* ****************************************************************************/
/* File:   usb_frame_api.h
 * Author: JP GAUTHIER
 * Company : ZiBLUE SAS
 * The information contained in this file is the property of the ZiBLUE company.
 *  Unauthorized publication, use, dissemination, forwarding, printing or copying
 * of this file and its associated compiled or linked forms is strictly prohibited.
 */
/* ****************************************************************************/
#define SYNC1_CONTAINER_CONSTANT 'Z'
#define SYNC2_CONTAINER_CONSTANT 'I'
#define QUALIFIER_ASCII_INTERPRETOR '+'
//#define  QUALIFIER_ASCII_CONSTANT2    ':'

#define ASCII_CONTAINER_MASK 0x40

#define REGULAR_INCOMING_BINARY_USB_FRAME_TYPE 0
#define REGULAR_INCOMING_RF_BINARY_USB_FRAME_TYPE 0

#define RFLINK_INCOMING_BINARY_USB_FRAME_TYPE 1
#define RFLINK_INCOMING_RF_BINARY_USB_FRAME_TYPE 1

#define SOURCEDEST433_868 1

struct MESSAGE_CONTAINER_HEADER
{
	unsigned char Sync1;
	unsigned char Sync2;
	unsigned char SourceDestQualifier;
	unsigned char QualifierOrLen_lsb;
	unsigned char QualifierOrLen_msb;
} sMESSAGE_CONTAINER_HEADER;

struct REGULAR_INCOMING_BINARY_USB_FRAME
{ // public binary API USB to RF frame sending
	unsigned char frameType;
	unsigned char cluster; // set 0 by default. Cluster destination.
	unsigned char protocol;
	unsigned char action;
	unsigned long ID; // LSB first. Little endian, mostly 1 byte is used
	unsigned char dimValue;
	unsigned char burst;
	unsigned char qualifier;
	unsigned char reserved2; // set 0
} sREGULAR_INCOMING_BINARY_USB_FRAME;

/*
 *
Binary Data[]	Size	Type	Remark
FrameType	1	unsigned char	Value = 0
DataFlag	1	unsigned char	0: 433MHz, 1: 868MHz
RFLevel	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
FloorNoise	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
Protocol	1	unsigned char	See below
InfosType	1	unsigned char	See below

 */

/* *************************************************************************** */

/*
Binary Data[]	Size	Type	Remark
FrameType	1	unsigned char	Value = 0
DataFlag	1	unsigned char	0: 433MHz, 1: 868MHz
RFLevel	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
FloorNoise	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
RFQuality	1	unsigned char
Protocol	1	unsigned char	See below
InfosType	1	unsigned char	See below
Infos[0?9]	20	Signed or unsigned short
upon context	LSB first. Define provided data by the device
*/

#define SEND_ACTION_OFF 0
#define SEND_ACTION_ON 1
#define SEND_ACTION_DIM 2
#define SEND_ACTION_BRIGHT 3
#define SEND_ACTION_ALL_OFF 4
#define SEND_ACTION_ALL_ON 5
#define SEND_ACTION_ASSOC 6
#define SEND_ACTION_DISSOC 7
#define SEND_ACTION_ASSOC_OFF 8
#define SEND_ACTION_DISSOC_OFF 9

/* ***************************************** */
#define SEND_UNDEFINED_PROTOCOL 0
#define SEND_VISONIC_PROTOCOL_433 1
#define SEND_VISONIC_PROTOCOL_868 2
#define SEND_CHACON_PROTOCOL_433 3
#define SEND_DOMIA_PROTOCOL_433 4
#define SEND_X10_PROTOCOL_433 5
#define SEND_X2D_PROTOCOL_433 6
#define SEND_X2D_PROTOCOL_868 7
#define SEND_X2D_SHUTTER_PROTOCOL_868 8
#define SEND_X2D_HA_ELEC_PROTOCOL_868 9
#define SEND_X2D_HA_GASOIL_PROTOCOL_868 10
#define SEND_SOMFY_PROTOCOL_433 11
#define SEND_BLYSS_PROTOCOL_433 12
#define SEND_PARROT 13
#define SEND_OREGON_PROTOCOL_433X 14 /* not reachable by API */
#define SEND_OWL_433X 15 /* not reachable by API */
#define SEND_KD101_PROTOCOL_433 16 /* 32 bits ID */
#define SEND_DIGIMAX_TS10_PROTOCOL_433 17 /* deprecated */
#define SEND_OREGON_PROTOCOLV1_433 18 /* not reachable by API */
#define SEND_OREGON_PROTOCOLV2_433 19 /* not reachable by API */
#define SEND_OREGON_PROTOCOLV3_433 20 /* not reachable by API */
#define SEND_TIC_433 21 /* not reachable by API */
#define SEND_FS20_868 22
#define SEND_EDISIO 23

/* ***************************************** */
#define RECEIVED_PROTOCOL_UNDEFINED 0
#define RECEIVED_PROTOCOL_X10 1
#define RECEIVED_PROTOCOL_VISONIC 2
#define RECEIVED_PROTOCOL_BLYSS 3
#define RECEIVED_PROTOCOL_CHACON 4
#define RECEIVED_PROTOCOL_OREGON 5
#define RECEIVED_PROTOCOL_DOMIA 6
#define RECEIVED_PROTOCOL_OWL 7
#define RECEIVED_PROTOCOL_X2D 8
#define RECEIVED_PROTOCOL_RFY 9
#define RECEIVED_PROTOCOL_KD101 10
#define RECEIVED_PROTOCOL_PARROT 11
#define RECEIVED_PROTOCOL_DIGIMAX 12 /* deprecated */
#define RECEIVED_PROTOCOL_TIC 13
#define RECEIVED_PROTOCOL_FS20 14
#define RECEIVED_PROTOCOL_JAMMING 15
#define RECEIVED_PROTOCOL_EDISIO 16

#define REGULAR_INCOMING_RF_BINARY_USB_FRAME_INFOS_WORDS_NUMBER 10

#define INFOS_TYPE0 0
#define INFOS_TYPE1 1
#define INFOS_TYPE2 2
#define INFOS_TYPE3 3
#define INFOS_TYPE4 4
#define INFOS_TYPE5 5
#define INFOS_TYPE6 6
#define INFOS_TYPE7 7
#define INFOS_TYPE8 8
#define INFOS_TYPE9 9
#define INFOS_TYPE10 10
#define INFOS_TYPE11 11
#define INFOS_TYPE12 12 /* deprecated */
#define INFOS_TYPE13 13
#define INFOS_TYPE14 14
#define INFOS_TYPE15 15

struct INCOMING_RF_INFOS_TYPE0
{ // used by X10 / Domia Lite protocols
	unsigned short subtype;
	unsigned short id;
};
struct INCOMING_RF_INFOS_TYPE1
{ // Used by X10 (32 bits ID), CHACON, JAMMING
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
};
struct INCOMING_RF_INFOS_TYPE2
{ // Used by  VISONIC /Focus/Atlantic/Meian Tech
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier;
};
struct INCOMING_RF_INFOS_TYPE3
{ //  Used by RFY PROTOCOL
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier;
};

struct INCOMING_RF_INFOS_TYPE4
{ // Used by  Scientific Oregon  protocol ( thermo/hygro sensors)
	unsigned short subtype;
	unsigned short idPHY;
	unsigned short idChannel;
	unsigned short qualifier;
	signed short temp;    // UNIT:  1/10 of degree Celsius
	unsigned short hygro; // 0...100  UNIT: %
};
struct INCOMING_RF_INFOS_TYPE5
{ // Used by  Scientific Oregon  protocol  ( Atmospheric  pressure  sensors)
	unsigned short subtype;
	unsigned short idPHY;
	unsigned short idChannel;
	unsigned short qualifier;
	signed short temp;	 // UNIT:  1/10 of degree Celsius
	unsigned short hygro;	 // 0...100  UNIT: %
	unsigned short pressure; //  UNIT: hPa
};

struct INCOMING_RF_INFOS_TYPE6
{ // Used by  Scientific Oregon  protocol  (Wind sensors)
	unsigned short subtype;
	unsigned short idPHY;
	unsigned short idChannel;
	unsigned short qualifier;
	unsigned short speed;	  // Averaged Wind speed   (Unit : 1/10 m/s, e.g. 213 means 21.3m/s)
	unsigned short direction; //  Wind direction  0-359° (Unit : angular degrees)
};
struct INCOMING_RF_INFOS_TYPE7
{ // Used by  Scientific Oregon  protocol  ( UV  sensors)
	unsigned short subtype;
	unsigned short idPHY;
	unsigned short idChannel;
	unsigned short qualifier;
	unsigned short light; // UV index  1..10  (Unit : -)
};
struct INCOMING_RF_INFOS_TYPE8
{ // Used by  OWL  ( Energy/power sensors)
	unsigned short subtype;
	unsigned short idPHY;
	unsigned short idChannel;
	unsigned short qualifier;
	unsigned short energyLsb; // LSB: energy measured since the RESET of the device  (32 bits value). Unit : Wh
	unsigned short energyMsb; // MSB: energy measured since the RESET of the device  (32 bits value). Unit : Wh
	unsigned short power;	  //  Instantaneous measured (total)power. Unit : W (with U=230V, P=P1+P2+P3))
	unsigned short powerI1;	  // Instantaneous measured at input 1 power. Unit : W (with U=230V, P1=UxI1)
	unsigned short powerI2;	  // Instantaneous measured at input 2 power. Unit : W (with U=230V, P2=UxI2)
	unsigned short powerI3;	  // Instantaneous measured at input 3 power. Unit : W (with U=230V, P2=UxI3)
};

struct INCOMING_RF_INFOS_TYPE9
{ // Used by  OREGON  ( Rain sensors)
	unsigned short subtype;
	unsigned short idPHY;
	unsigned short idChannel;
	unsigned short qualifier;
	unsigned short totalRainLsb; // LSB: rain measured since the RESET of the device  (32 bits value). Unit : 0.1 mm
	unsigned short totalRainMsb; // MSB: rain measured since the RESET of the device
	unsigned short rain;	     // Instantaneous measured rain. Unit : 0.01 mm/h
};

struct INCOMING_RF_INFOS_TYPE10
{ // Used by Thermostats  X2D protocol
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier; // D0 : Tamper Flag, D1: Alarm Flag, D2: Low Batt Flag, D3: Supervisor Frame, D4: Test  D6:7 : X2D variant
	unsigned short function;
	unsigned short mode;	//
	unsigned short data[4]; // provision
};
struct INCOMING_RF_INFOS_TYPE11
{ // Used by Alarm/remote control devices  X2D protocol
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier; // D0 : Tamper Flag, D1: Alarm Flag, D2: Low Batt Flag, D3: Supervisor Frame, D4: Test  D6:7 : X2D variant
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned short data[4]; //  provision
};

struct INCOMING_RF_INFOS_TYPE12
{ // Used by  DIGIMAX TS10 protocol // deprecated
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier;
	signed short temp;     // UNIT:  1/10 of degree
	signed short setPoint; // UNIT:  1/10 of degree
};

struct INCOMING_RF_INFOS_TYPE13
{				// Used by  Cartelectronic TIC/Pulses devices (Teleinfo/TeleCounters)
	unsigned short subtype; // subtype/version (0: Teleinfo mode, 1: Encoder Mode, 2: Linky Mode)
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier; // Teleinfo mode:
				  // DO: battery flag (1: low)
				  // D1: Apparent power valid
				  // D2: teleinfo error (1: error)
				  // D3-D4: (0: no change price time warning, 1: white, 2: blue, 3 red/PEJP)
				  // D5: reserved for future usage
				  // D6: Teleinfo desactivated
				  // D7: Production
				  // Encoder mode
				  // DO: battery flag (1: low)
				  // Linky mode
				  // DO: battery flag (1: low)
				  // D1: Apparent power valid
				  // D2: teleinfo present
				  // D3-D4: PEJP or color price forcast for tomorrow(0: no change price time warning, 1: white, 2: blue, 3 red/PEJP)
				  // D5-D6: color price for today (0: no change price time warning, 1: white, 2: blue, 3 red/PEJP)
	unsigned short infos;	  // Teleinfo mode:
			      // D0-D7: contract type/current price time (1: HC, 0: HP)
			      // Linky mode:
			      // D0-D3: spare
			      // D4-D7: current index
			      // D8-D15: average voltage
	unsigned short counter1Lsb; // unit Wh (HC)
	unsigned short counter1Msb;
	unsigned short counter2Lsb; // unit Wh (HP)
	unsigned short counter2Msb;
	unsigned short apparentPower; // unit: Watt (in fact, it is VA)
};

struct INCOMING_RF_INFOS_TYPE14
{ // Used by FS20. Same file as INCOMING_RF_INFOS_TYPE2
	unsigned short subtype;
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier;
};
struct INCOMING_RF_INFOS_TYPE15
{ // Used by EDISIO
	unsigned short subtype; // Command field
	unsigned short idLsb;
	unsigned short idMsb;
	unsigned short qualifier; // channel identifier
	unsigned short infos;	  // MID (D0-D7): Model field, BL (D8-D15): Battery level (1/10V)
	uint32_t additionalData;
};

// Edisio Command Field
#define EDISIO_CMD_NULL 0x00             // null instruction
#define EDISIO_CMD_ON 0x01               // channel switched on
#define EDISIO_CMD_OFF 0x02              // channel switched off
#define EDISIO_CMD_TOGGLE 0x03           // channel toggled
#define EDISIO_CMD_DIM 0x04              // channel dimmed to x% (1 additional byte hex 0x03 to 0x64)
#define EDISIO_CMD_DIM_UP 0x05           // dim channel up 3%
#define EDISIO_CMD_DIM_DOWN 0x06         // dim channel odnw 3%
#define EDISIO_CMD_DIM_A 0x07            // dim channel toggled
#define EDISIO_CMD_DIM_STOP 0x08         // dim stop
#define EDISIO_CMD_SHUTTER_OPEN 0x09     // channel switched to open
#define EDISIO_CMD_SHUTTER_CLOSE 0x0A    // channel switched to close
#define EDISIO_CMD_SHUTTER_CLOSE2 0x1B   // channel switched to close (usb dongle v1.0)
#define EDISIO_CMD_SHUTTER_STOP 0x0B     // channel switched to stop
#define EDISIO_CMD_RGB 0x0C              // (3 bytes of additional data containing R, G and B)
#define EDISIO_CMD_RGB_C 0x0D            // reserved
#define EDISIO_CMD_RGB_PLUS 0x0E         // reserved
#define EDISIO_CMD_OPEN_SLOW 0x0F        // reserved
#define EDISIO_CMD_SET_SHORT 0x10        // paring request (only with CID value hex 0x09)
#define EDISIO_CMD_SET_5S 0x11           // unparing one channel (only with CID value hex 0x09)
#define EDISIO_CMD_SET_10S 0x12          // unparing all channels (only with CID value hex 0x09)
#define EDISIO_CMD_STUDY 0x16            // paring request by gateway
#define EDISIO_CMD_DEL_BUTTON 0x17       // reserved for gateway
#define EDISIO_CMD_DEL_ALL 0x18          // reserved for gateway
#define EDISIO_CMD_SET_TEMPERATURE 0x19  // temperature send by the sensor (2 bytes of additional field)
#define EDISIO_CMD_DOOR_OPEN 0x1A        // status sent by the sensor
#define EDISIO_CMD_BROADCAST_QUERY 0x1F  // reserved
#define EDISIO_CMD_QUERY_STATUS 0x20     // request sent by the gateway toknow receiver status
#define EDISIO_CMD_REPORT_STATUS 0x21    // response fromthe receiver after 'QUERY_STATUS' (n bytes additional data)
#define EDISIO_CMD_READ_CUSTOM 0x23      // read customizable data
#define EDISIO_CMD_SAVE_CUSTOM 0x24      // save customizable receiver data (n bytes)
#define EDISIO_CMD_REPORT_CUSTOM 0x29    // response from receiver after READ_CUSTOM and SAVE_CUSTOM (n bytes)
#define EDISIO_CMD_SET_SHORT_DIMMER 0x2C // dimmer pairing request (n bytes)
#define EDISIO_CMD_SET_SHORT_SENSOR 0x2F // sensor paring request (n bytes)

// Edisio Model Field
//                                           Device            |     Function                         | Products ref
#define EDISIO_MODEL_EMITRBTN 0x01     // Emmiter 8 channels   | 8x On/Off/Pulse/Open/Stop/Close      | ETC1/ETC4/EBP8
#define EDISIO_MODEL_EMS_100 0x07      // Motion Sensor        | On/Off/Pulse/PilotWire/Open/Close    | EMS-100
#define EDISIO_MODEL_ETS_100 0x08      // Temperature Sensor   | Temperature                          | ETS-100
#define EDISIO_MODEL_EDS_100 0x09      // Door Sensor          | On/Off/Pulse/Open/Close              | EDS-100
#define EDISIO_MODEL_EMR_2000 0x10     // Receiver 1 Output    | 1x On/Off/Pulse                      | EMR-2000
#define EDISIO_MODEL_EMV_400 0x11      // Receiver 2 Outputs   | 2x On/Off or 1x Open/Stop/Close      | EMV-400
#define EDISIO_MODEL_EDR_D4 0x12       // Receiver 4 Outputs   | 4x On/Off or Dimmer                  | EDR-D4
#define EDISIO_MODEL_EDR_B4 0x13       // Receiver 4 Outputs   | 4x On/Off/Pulse or 1x Open/Stop/Close| EDR-B4
#define EDISIO_MODEL_EMSD_300A 0x14    // Receiver 1 Output    | 1x On/Off or Dimmer                  | EMSD-300A
#define EDISIO_MODEL_EMSD_300 0x15     // Receiver 1 Output    | 1x On/Off or Dimmer                  | EMSD-300
#define EDISIO_MODEL_EGW 0x16          // Gateway              | Send and receive all commandes       | EGW
#define EDISIO_MODEL_EMM_230 0x17      // Emmiter 2 channels   | 2x On/Off/Pulse/Open/Stop/Close      | EMM-230
#define EDISIO_MODEL_EMM_100 0x18      // Emmiter 2 channels   | 2x On/Off/Pulse/Open/Stop/Close      | EMM-100
#define EDISIO_MODEL_ED_TH_01 0x0E     // Thermostat           | Thermostat                           | ED-TH-04
#define EDISIO_MODEL_ED_LI_01 0x0B     // Receiver 1 Output    | 1x On/Off                            | ED-LI-01
#define EDISIO_MODEL_ED_TH_02 0x0F     // Receiver 1 Output    | 1x On/Off Heater/Cooler              | ED-TH-02
#define EDISIO_MODEL_ED_TH_03 0x0C     // Receiver 1 Output FP | 1x Off/Eco/Confort/Auto (Fil Pilot)  | ED-TH-03
#define EDISIO_MODEL_ED_SH_01 0x0D     // Receiver 2 Outputs   | 1x Open/Stop/Close                   | ED-SH-01
#define EDISIO_MODEL_IR_TRANS 0x1B     // IR transmitter       | Trigger                              | EGW-PRO/EN
#define EDISIO_MODEL_STM_250 0x1E      // Enocean sensor       | Door                                 | STM-250
#define EDISIO_MODEL_STM_330 0x1F      // Enocean sensor       | Temperature                          | STM-330
#define EDISIO_MODEL_PTM_210 0x20      // Enocean sensor       | Switch                               | PTM-210
#define EDISIO_MODEL_IR_DEVICE 0x21    // Virtual device       | IR remote control                    | EGW-PRO/EN
#define EDISIO_MODEL_EN_DET 0x22       // Enocean motion       | Motion                               | EGW-PRO/EN
#define EDISIO_MODEL_EN_SOC 0x23       // Enocean socket       | Socket                               | EGW-PRO/EN
/* *************************************************************************** */

struct REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER
{				 // public binary API   RF to USB
	unsigned char frameType; // Value = 0
	unsigned char cluster;	 // cluster origin. Reserved field
	unsigned char dataFlag;	 // 0: 433Mhz, 1: 868MHz
	signed char rfLevel;	 // Unit : dBm  (high signal :-40dBm to low : -110dB)
	signed char floorNoise;	 // Unit : dBm  (high signal :-40dBm to low : -110dB)
	unsigned char rfQuality; // factor or receiving quality : 1...10 : 1 : worst quality, 10 : best quality
	unsigned char protocol;	 // protocol under scope
	unsigned char infoType;	 // type of payload
};

struct REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME
{ // public binary API
	struct REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER header;
	union {
		signed short word[REGULAR_INCOMING_RF_BINARY_USB_FRAME_INFOS_WORDS_NUMBER];
		struct INCOMING_RF_INFOS_TYPE0 type0;
		struct INCOMING_RF_INFOS_TYPE1 type1;
		struct INCOMING_RF_INFOS_TYPE2 type2;
		struct INCOMING_RF_INFOS_TYPE2 type3;
		struct INCOMING_RF_INFOS_TYPE4 type4;
		struct INCOMING_RF_INFOS_TYPE5 type5;
		struct INCOMING_RF_INFOS_TYPE6 type6;
		struct INCOMING_RF_INFOS_TYPE7 type7;
		struct INCOMING_RF_INFOS_TYPE8 type8;
		struct INCOMING_RF_INFOS_TYPE9 type9;
		struct INCOMING_RF_INFOS_TYPE10 type10;
		struct INCOMING_RF_INFOS_TYPE11 type11;
		struct INCOMING_RF_INFOS_TYPE12 type12;
		struct INCOMING_RF_INFOS_TYPE13 type13;
		struct INCOMING_RF_INFOS_TYPE14 type14;
		struct INCOMING_RF_INFOS_TYPE15 type15;
	} infos;
} sREGULAR_INCOMING_RF_TO_BINARY_USB_FRAME;
