/* ****************************************************************************/
/* File:   usb_frame_api.h
 * Author: JP GAUTHIER
 * Company : ZiBLUE SAS
 * The information contained in this file is the property of the ZiBLUE company. 
 *  Unauthorized publication, use, dissemination, forwarding, printing or copying   
 * of this file and its associated compiled or linked forms is strictly prohibited.
 */
/* ****************************************************************************/
#define  SYNC1_CONTAINER_CONSTANT    'Z'
#define  SYNC2_CONTAINER_CONSTANT    'I'
#define  QUALIFIER_ASCII_INTERPRETOR  '+'
//#define  QUALIFIER_ASCII_CONSTANT2    ':'

#define  ASCII_CONTAINER_MASK         0x40


#define  REGULAR_INCOMING_BINARY_USB_FRAME_TYPE      0
#define  REGULAR_INCOMING_RF_BINARY_USB_FRAME_TYPE   0

#define  RFLINK_INCOMING_BINARY_USB_FRAME_TYPE      1
#define  RFLINK_INCOMING_RF_BINARY_USB_FRAME_TYPE   1

#define  SOURCEDEST433_868            1


struct  MESSAGE_CONTAINER_HEADER {
    unsigned char Sync1 ;
    unsigned char Sync2 ;
    unsigned char SourceDestQualifier ;
    unsigned char QualifierOrLen_lsb ; 
    unsigned char QualifierOrLen_msb ;
} sMESSAGE_CONTAINER_HEADER;

struct REGULAR_INCOMING_BINARY_USB_FRAME { // public binary API USB to RF frame sending
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
DataFlag	1	unsigned char	0: 433Mhz, 1: 868Mhz
RFLevel	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
FloorNoise	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
Protocol	1	unsigned char	See below
InfosType	1	unsigned char	See below

 */

 /* *************************************************************************** */

/*
Binary Data[]	Size	Type	Remark
FrameType	1	unsigned char	Value = 0
DataFlag	1	unsigned char	0: 433Mhz, 1: 868Mhz
RFLevel	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
FloorNoise	1	signed char	Unit : dB  (high signal :-40dB to low : -110dB)
RFQuality	1	unsigned char	
Protocol	1	unsigned char	See below
InfosType	1	unsigned char	See below
Infos[0?9]	20	Signed or unsigned short
upon context	LSB first. Define provided data by the device
*/

#define   SEND_ACTION_OFF                0
#define   SEND_ACTION_ON                 1
#define   SEND_ACTION_DIM                2
#define   SEND_ACTION_BRIGHT             3
#define   SEND_ACTION_ALL_OFF            4
#define   SEND_ACTION_ALL_ON             5
#define   SEND_ACTION_ASSOC              6
#define   SEND_ACTION_DISSOC             7
#define   SEND_ACTION_ASSOC_OFF          8
#define   SEND_ACTION_DISSOC_OFF         9

/* ***************************************** */ 
#define   SEND_UNDEFINED_PROTOCOL        0
#define   SEND_VISONIC_PROTOCOL_433      1
#define   SEND_VISONIC_PROTOCOL_868      2
#define   SEND_CHACON_PROTOCOL_433       3
#define   SEND_DOMIA_PROTOCOL_433        4
#define   SEND_X10_PROTOCOL_433          5
#define   SEND_X2D_PROTOCOL_433          6
#define   SEND_X2D_PROTOCOL_868          7
#define   SEND_X2D_SHUTTER_PROTOCOL_868  8
#define   SEND_X2D_HA_ELEC_PROTOCOL_868  9
#define   SEND_X2D_HA_GASOIL_PROTOCOL_868  10
#define   SEND_SOMFY_PROTOCOL_433        11
#define   SEND_BLYSS_PROTOCOL_433        12
#define   SEND_PARROT                    13
#define   SEND_OREGON_PROTOCOL_433X       14 /* not reachable by API */
#define   SEND_OWL_433X                   15 /* not reachable by API */
#define   SEND_KD101_PROTOCOL_433        16 /* 32 bits ID */
#define   SEND_DIGIMAX_TS10_PROTOCOL_433 17 /* deprecated */
#define   SEND_OREGON_PROTOCOLV1_433     18 /* not reachable by API */
#define   SEND_OREGON_PROTOCOLV2_433     19 /* not reachable by API */
#define   SEND_OREGON_PROTOCOLV3_433     20 /* not reachable by API */


/* ***************************************** */
#define  RECEIVED_PROTOCOL_UNDEFINED 0
#define  RECEIVED_PROTOCOL_X10       1
#define  RECEIVED_PROTOCOL_VISONIC   2
#define  RECEIVED_PROTOCOL_BLYSS     3
#define  RECEIVED_PROTOCOL_CHACON    4
#define  RECEIVED_PROTOCOL_OREGON    5
#define  RECEIVED_PROTOCOL_DOMIA     6
#define  RECEIVED_PROTOCOL_OWL       7
#define  RECEIVED_PROTOCOL_X2D       8
#define  RECEIVED_PROTOCOL_RFY       9
#define  RECEIVED_PROTOCOL_KD101     10
#define  RECEIVED_PROTOCOL_PARROT    11
#define  RECEIVED_PROTOCOL_DIGIMAX   12 /* deprecated */


#define  REGULAR_INCOMING_RF_BINARY_USB_FRAME_INFOS_WORDS_NUMBER       10

#define INFOS_TYPE0     0
#define INFOS_TYPE1     1
#define INFOS_TYPE2     2
#define INFOS_TYPE3     3
#define INFOS_TYPE4     4
#define INFOS_TYPE5     5
#define INFOS_TYPE6     6
#define INFOS_TYPE7     7
#define INFOS_TYPE8     8
#define INFOS_TYPE9     9
#define INFOS_TYPE10    10
#define INFOS_TYPE11    11
#define INFOS_TYPE12    12 /* deprecated */



struct  INCOMING_RF_INFOS_TYPE0 { // used by X10 / Domia Lite protocols
    unsigned short subtype ;
    unsigned short id ;
};
struct  INCOMING_RF_INFOS_TYPE1 { // Used by X10 (32 bits ID) and CHACON
    unsigned short subtype ;
    unsigned short idLsb ;
    unsigned short idMsb ;
};
struct  INCOMING_RF_INFOS_TYPE2 { // Used by  VISONIC /Focus/Atlantic/Meian Tech 
    unsigned short subtype ;
    unsigned short idLsb ;
    unsigned short idMsb ;
    unsigned short qualifier ;
};
struct  INCOMING_RF_INFOS_TYPE3 { //  Used by RFY PROTOCOL
    unsigned short subtype ;
    unsigned short idLsb ;
    unsigned short idMsb ;
    unsigned short qualifier ;
};


struct  INCOMING_RF_INFOS_TYPE4 { // Used by  Scientific Oregon  protocol ( thermo/hygro sensors)
    unsigned short subtype ;
    unsigned short idPHY ;
    unsigned short idChannel ;
    unsigned short qualifier ;
    signed short   temp ; // UNIT:  1/10 of degree Celsius
    unsigned short hygro ; // 0...100  UNIT: % 
};
struct  INCOMING_RF_INFOS_TYPE5 { // Used by  Scientific Oregon  protocol  ( Atmospheric  pressure  sensors)
    unsigned short subtype ;
    unsigned short idPHY ;
    unsigned short idChannel ;
    unsigned short qualifier ;
    signed short   temp ; // UNIT:  1/10 of degree Celsius
    unsigned short hygro ; // 0...100  UNIT: % 
    unsigned short pressure ; //  UNIT: hPa 
}; 

struct  INCOMING_RF_INFOS_TYPE6 { // Used by  Scientific Oregon  protocol  (Wind sensors)
    unsigned short subtype ;
    unsigned short idPHY ;
    unsigned short idChannel ;
    unsigned short qualifier ;
    unsigned short speed ; // Averaged Wind speed   (Unit : 1/10 m/s, e.g. 213 means 21.3m/s)
    unsigned short direction ; //  Wind direction  0-359� (Unit : angular degrees)
};
struct  INCOMING_RF_INFOS_TYPE7 { // Used by  Scientific Oregon  protocol  ( UV  sensors)
    unsigned short subtype ;
    unsigned short idPHY ;
    unsigned short idChannel ;
    unsigned short qualifier ;
    unsigned short light ; // UV index  1..10  (Unit : -)
};
struct  INCOMING_RF_INFOS_TYPE8 { // Used by  OWL  ( Energy/power sensors)
    unsigned short subtype ;
    unsigned short idPHY ;
    unsigned short idChannel ;
    unsigned short qualifier ;
    unsigned short energyLsb ; // LSB: energy measured since the RESET of the device  (32 bits value). Unit : Wh
    unsigned short energyMsb ; // MSB: energy measured since the RESET of the device  (32 bits value). Unit : Wh
    unsigned short power ;       //  Instantaneous measured (total)power. Unit : W (with U=230V, P=P1+P2+P3))
    unsigned short powerI1 ;     // Instantaneous measured at input 1 power. Unit : W (with U=230V, P1=UxI1)
    unsigned short powerI2 ;     // Instantaneous measured at input 2 power. Unit : W (with U=230V, P2=UxI2)
    unsigned short powerI3 ;     // Instantaneous measured at input 3 power. Unit : W (with U=230V, P2=UxI3)
};

struct  INCOMING_RF_INFOS_TYPE9 { // Used by  OREGON  ( Rain sensors)
    unsigned short subtype ;
    unsigned short idPHY ;
    unsigned short idChannel ;
    unsigned short qualifier ;
    unsigned short totalRainLsb ; // LSB: rain measured since the RESET of the device  (32 bits value). Unit : 0.1 mm
    unsigned short totalRainMsb ; // MSB: rain measured since the RESET of the device   
    unsigned short rain ;     // Instantaneous measured rain. Unit : 0.01 mm/h
};

struct  INCOMING_RF_INFOS_TYPE10 { // Used by Thermostats  X2D protocol
    unsigned short subtype ;
    unsigned short idLsb ;
    unsigned short idMsb ;
    unsigned short qualifier ; // D0 : Tamper Flag, D1: Alarm Flag, D2: Low Batt Flag, D3: Supervisor Frame, D4: Test  D6:7 : X2D variant
    unsigned short function ;
    unsigned short mode ; //     
    unsigned short data[4] ;   // provision
};
struct  INCOMING_RF_INFOS_TYPE11 { // Used by Alarm/remote control devices  X2D protocol
    unsigned short subtype ;
    unsigned short idLsb ;
    unsigned short idMsb ;
    unsigned short qualifier ; // D0 : Tamper Flag, D1: Alarm Flag, D2: Low Batt Flag, D3: Supervisor Frame, D4: Test  D6:7 : X2D variant
    unsigned short reserved1 ;
    unsigned short reserved2 ;
    unsigned short data[4] ;   //  provision
};


struct  INCOMING_RF_INFOS_TYPE12 { // Used by  DIGIMAX TS10 protocol // deprecated
    unsigned short subtype ;
    unsigned short idLsb ;
    unsigned short idMsb ;
    unsigned short qualifier ;
    signed short   temp ; // UNIT:  1/10 of degree
    signed short   setPoint ; // UNIT:  1/10 of degree
};

 /* *************************************************************************** */

struct REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER { // public binary API   RF to USB
    unsigned char frameType; // Value = 0
    unsigned char cluster; // cluster origin. Reserved field
    unsigned char dataFlag; // 0: 433Mhz, 1: 868Mhz
    signed char   rfLevel; // Unit : dBm  (high signal :-40dBm to low : -110dB)
    signed char   floorNoise; //Unit : dBm  (high signal :-40dBm to low : -110dB)
    unsigned char rfQuality; // factor or receiving quality : 1...10 : 1 : worst quality, 10 : best quality
    unsigned char protocol; // protocol under scope
    unsigned char infoType; // type of payload
};
    
    
struct REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME { // public binary API
    struct REGULAR_INCOMING_RF_TO_BINARY_USB_FRAME_HEADER header ;
    union
      {
      signed short word[REGULAR_INCOMING_RF_BINARY_USB_FRAME_INFOS_WORDS_NUMBER];
      struct  INCOMING_RF_INFOS_TYPE0 type0 ;
      struct  INCOMING_RF_INFOS_TYPE1 type1 ;
      struct  INCOMING_RF_INFOS_TYPE2 type2 ;
      struct  INCOMING_RF_INFOS_TYPE2 type3 ;
      struct  INCOMING_RF_INFOS_TYPE4 type4 ;
      struct  INCOMING_RF_INFOS_TYPE5 type5 ;
      struct  INCOMING_RF_INFOS_TYPE6 type6 ;
      struct  INCOMING_RF_INFOS_TYPE7 type7 ;
      struct  INCOMING_RF_INFOS_TYPE8 type8 ;
      struct  INCOMING_RF_INFOS_TYPE9 type9 ;
      struct  INCOMING_RF_INFOS_TYPE10 type10 ;
      struct  INCOMING_RF_INFOS_TYPE11 type11 ;
      struct  INCOMING_RF_INFOS_TYPE12 type12 ;
      } infos;
} sREGULAR_INCOMING_RF_TO_BINARY_USB_FRAME;

