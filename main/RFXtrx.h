#pragma once

#ifndef _RXFCOMLIB_F11DD459_E67E_4B26_8E44_B964E99304BF
#define _RXFCOMLIB_F11DD459_E67E_4B26_8E44_B964E99304BF

//#define IS_BIG_ENDIAN
// users with a big_endian system have to remove the comment slashes before the #define.
//

/*
                                                                   
Copyright 2011-2019, RFXCOM

ALL RIGHTS RESERVED. This code is owned by RFXCOM, and is protected under
Netherlands Copyright Laws and Treaties and shall be subject to the 
exclusive jurisdiction of the Netherlands Courts. The information from this
file may freely be used to create programs to exclusively interface with
RFXCOM products only. Any other use or unauthorized reprint of this material
is prohibited. No part of this code may be reproduced or transmitted in
any form or by any means, electronic or mechanical, including photocopying,
recording, or by any information storage and retrieval system without
express written permission from RFXCOM.

The above copyright notice shall be included in all copies or substantial
portions of this file.
'----------------------------------------------------------------------------
*/

/*
SDK version 9.24
	Chime Alfawise, dBell added
	SelectPlus3 changed to ByronBY
	Async Get Settings added
	868 protocol selection bits updated and changed
	WEATHER & SOLAR structures added
	ACH2010 moved to WEATHER
	WS5500 added

SDK version 9.23
	Async data subtypes changed

SDK version 9.22
	Falmec added
	Fan LucciAir DCII added
	Zemismart blinds added
	Async port added
	Firmware types added
	Livolo 1-10 device changed 

SDK version 9.21
	Fan LucciAir DC added
	Casafan added
	FT1211R fan controller added
	Hualite blind added
	Lighting1 Oase added

SDK version 9.20
	Lighting6 Cuveo added

SDK version 9.19a
	FS20 commands added

SDK version 9.19
	IRESPONSE868 added
	IRESPONSE updated
	433 & 868 config bits updated

SDK version 9.18
	RAW transmit/receive added
	BlindsT6 intermediate position added

SDK version 9.17
	868 config bits added (changed)
	Interface Control Freq commands removed (use freqsel instead)
	FunkBus (Gira, Jung, Berker, Insta) added

SDK version 9.16
	RAIN8 and RAIN9 added
	WIND8 added
	Cartelectronic - Linky added

SDK version 9.15
	BlindsT13 - Screenline angle change added

SDK version 9.14
	Lighting5 - Kangtai added

SDK version 9.13
	BlindsT13 - Screenline added

SDK version 9.12
	Thermostat4 updated

SDK version 9.11
	FAN - Westinghouse fan added
	Security1 - RM174RF added
	Thermostat4 added

SDK version 9.10
	FAN - SEAV remote added

SDK version 9.09
	Lighting5 - MDremote108 added

SDK version 9.08
	CARTELECTRONIC TIC and Encoder added
	Lighting5 - Livolo dim/scene added

SDK version 9.07
	Lighting5 IT added
	BlindsT12 Confexx added

SDK version 9.06
	Lighting1 HQ COCO-20 added
	Lighting5 Avantek added
	BlindsT11 ASP added

SDK version 9.05
	Itho CVE RFT fan added
	LucciAir fan added
	HUM3 Inovalley S80 plant humidity sensor added

SDK version 9.04A
	Lighting5 Legrand CAD added
	msg6-HC HomeConfort protocol enable added

SDK version 9.03
	MDremote version 107 added

SDK version 9.02
	Home Confort added

SDK version 9.01
	RFY - ASA blinds added

SDK version 9.00
	Lighting5 - sTypeRGB432W added
	ICMND - msg1 changed to freqsel
	ICMND - msg2 changed to xmitpwr

SDK version 8.03A
	Thermostat3 - Mertik G6R-H4S added

SDK version 8.03
	Subtype for Dolat blinds corrected, was 0x10 changed to 0xA

SDK version 8.02
	Blinds Dolat added
	Thermostat3 - Mertik G6R_H4TD added

SDK version 8.01
	Blinds Sunpery changed

SDK version 8.00
	Security2 - KeeLoq added
	Envivo Chime added
	msg6 - mode bits added
	sTypeUselectplus added
	Blinds Sunpery added
	TH14,RAIN7,WIND7 - Alecto WS4500 added

SDK version 7.02/7.03
	msg3_RFU changed to msg3_IMAGINTRONIX
	IRESPONSE.RFU6enabled changed to IRESPONSE.IMAGINTRONIXenabled

SDK version 7.01
	SelectPlus200689103 Black Chime added

SDK version 7.00
	TEMP7 - TSS330 added and TH9 – TSS320 added
	BlindsT8 = Chamberlain CS4330CN added
	SelectPlus200689101 White Chime added
	Interface command - start receiver added
	IRESPONSE size increased to 0x14

SDK version 6.27
	Livolo Appliance 1-10 added
	Somfy RFY commands: Enable sun+wind & Disable sun added
	Smartwares radiator valve added

SDK version 6.26
	TH13 - Alecto WS1700 and compatibles added

SDK version 6.25
	sTypeByronMP001 added
	sTypeTEMP11 added
	sTypeTRC02_2 added
	THB1 also used for BTHGN129

SDK version 6.24
	Lighting5 - Aoke realy added

SDK version 6.23
	RFY List remotes added

SDK version 6.22
	RFY venetian commands added < 0.5 and > 2 sec up/down)

SDK version 6.21
	Temp/Humidity - TH12 soil sensor added

SDK version 6.20
	Lighting2 - Kambrook added

SDK version 6.19
	msg3_RFY reversed back to msg3_RFU

SDK version 6.18
	RFY structure added
	BlindsT8 moved to RFY
	msg3_RFY added (not used)
	undecoded sTypeUrfy added
	Interface response "sTypeUnknownRFYremote" and "sTypeExtError" added

SDK version 6.17
	Blinds1 unitcode and id4 corrected

SDK version 6.16
	BlindsT8 RFY added with commands 0 to 9

SDK version 6.15
	BLINDS1 id4 added

SDK version 6.14
	BlindsT7 - Forest added

SDK version 6.13
	(skipped, to make version equal to SDK.pdf)

SDK version 6.12
	Lighting1 - Energenie5 added
	Lighting1 - COCO GDR2-2000R added
	sTypeBlindsT6 - DC106, YOOHA, Rohrmotor24 RMF added
	RAW transmit added

SDK version 6.11
	Lighting5 - RGB driver TRC02 added
	Lighting6 - Blyss rfu replaced by seqnbr2
	Endian check added

SDK version 6.10
	Security1 - SA30 added
	TEMP_HUM - TH11 EW109 added
	POWER - Revolt added

SDK version 6.09
	BBQ structure added

SDK version 6.08
	FAN structure and pTypeFan and sTypeSiemensSF01 added
	Lighting5 - Livolo added

SDK version 6.07b
	in the IRESPONSE struct: RFU5enabled changed to SXenabled
SDK version 6.07a
	#define msg3_RFU5 0x20   changed to   #define msg3_SX 0x20

SDK version 6.07
	Lighting5 colour modes added for LWRF
	TEMP_RAIN structure and pTypeTEMP_RAIN added for WS1200 - Temperature and rain sensor
	CHIME structure and pTypeChime added for Byron SX Chime

SDK version 6.06a
    RFU4 changed to RSLenabled in IRESPONSE

SDK version 6.06
	Lighting1 Energenie added
	Lighting5 MDREMOTE LED dimmer added
	Lighting5 RSL2 added
	msg3_RSL - Enable RSL protocol added
	#define sTypeUrsl 0x0E = un-decoded Conrad RSL added

SDK version 6.02b
	msg3-5 replaced by MODEbits in IRESPONSE structure
	LIGHTING4 enabled added in struct MODEbits

SDK version 6.02a
	#define sTypeBlindsT5 0x5  added

SDK version 6.02
	sTypeTH10 0xA   //Rubicson added

SDK version 6.01
  Copyright message updated

SDK version 5.03
	RAIN6 added
	Raex BlindsT4 added
	protocol enable/disable msg3_LIGHTING4 added
	Interface Message - Wrong Command added

SDK version 5.01
	baroForecast values added

SDK version 5.00
	Commands removed from Interface control:
		cmdENABLEALL 0x04, cmdUNDEC 0x05
		cmdDISX10 0x10   to    cmdDISFS20 0x1C
	CM180i CURRENT_ENERGY - ELEC4 added
	code for pTypeGAS and pTypeWATER changed (not yet used) 

SDK version 4.36
	security - #define sStatusIRbeam 0x8 added

SDK version 4.35
	defines added for commands
	union tRBUF is now typedef
	filler in REMOTE changed to cmndtype
	sTypeDigimax1 changed to sTypeDigimaxShort

SDK version 4.34
	BlindsT2 BlindsT3 added

SDK version 4.32
	BBSB new type added

SDK version 4.31
	Modebits updated

SDK version 4.30
	Energy sensor ELEC3 - CM180 added

SDK version 4.29
  sTypeTEMP10 for TFA 30.3133 added
	#define sTypeATI2 0x1   changed to    #define sTypeATIplus 0x1
	#define sTypeATIrw2 0x4  added

SDK version 4.28
  undecoded types 0x0F-0x12 added

SDK version 4.27
  Lighting1 - Philips SBC added
	Lighting6 - Blyss added
	BLINDS1 Rollertrol renamed to BlindsT0 and BlindsT1 added
	msg4_ROLLERTROL renamed to msg4_BLINDST0
	msg4_BLINDST1 added
	MODEbits.rollertrolenabled renamed to MODEbits.BLINDST0enabled
	MODEbits.BLINDST1enabled added
  undecoded types:
		sTypeUrollertrol renamed to sTypeUblinds
		sTypeUrubicson,sTypeUfineoffset and sTypeUae added

SDK version 4.26
	TEMP-HUM TH9 Viking 02035,02038 added
	TEMP TEMP9 RUBiCSON added
	Security tamper status changed
	Security1 Meiantech, msg5_MEI and cmdDISMEI added
	Disable Koppla removed

SDK version 4.23
	4 sensors WS2300 added

SDK version 4.22
	Viking temperature sensor TEMP7 added
	Blinds1 - RollerTrol added

SDK version 4.21
  Lighting1 - RisingSun added

SDK version 4.19
	TS15C added

SDK version 4.18
	UPM wind and rain added

SDK version 4.17
	FS20 added

SDK version 4.15
	Lighting5 - EMW100 added

SDK version 4.14
	Lighting5 - level added

SDK version 4.13
	added sTypeTH8 Esic Temp-Hum sensor
	Lighting6 - Novatis removed

SDK version 4.9
	added: #define recType43392 0x52
	changed; #define recType43392 0x53  to   #define trxType43392 0x53
*/

//types for Interface Control
#define pTypeInterfaceControl 0x00
#define sTypeInterfaceCommand 0x00
#define cmdRESET	0x00 // reset the receiver/transceiver
#define cmdSTATUS	0x02 // return firmware versions and configuration of the interface
#define cmdSETMODE	0x03 // set configuration of the interface

#define cmdSAVE		0x06 // save receiving modes of the receiver/transceiver in non-volatile memory
#define cmdStartRec	0x07 // start RFXtrx receiver

#define pTypeInterfaceMessage 0x01
#define sTypeInterfaceResponse 0x00
#define sTypeUnknownRFYremote 0x01
#define sTypeExtError 0x02
#define sTypeRFYremoteList 0x03
#define sTypeASAremoteList 0x04
#define sTypeRecStarted 0x07
#define sTypeInterfaceWrongCommand 0xFF
#define trxType310 0x50
#define trxType315 0x51
#define recType43392 0x52
#define trxType43392 0x53
#define trxType868 0x55

#define FWtyperec 0x0
#define FWtype1 0x1
#define FWtype2 0x2
#define FWtypeExt 0x3
#define FWtypeExt2 0x4
#define FWtypePro1 0x5
#define FWtypePro2 0x6
#define FWtypeProXL1 0x10

//433 config bits
#define msg3_AE 0x01			//AE Blyss
#define msg3_RUBICSON 0x02		//Rubicson,Lacrosse, Banggood
#define msg3_FINEOFFSET 0x04	//Fineoffset,Viking
#define msg3_LIGHTING4 0x08		//PT2262 and compatible
#define msg3_RSL 0x10			//RSL,Revolt
#define msg3_SX 0x20			//ByronSX,Selectplus
#define msg3_IMAGINTRONIX 0x40	//Imagintronix,Opus
#define msg3_undec 0x80			//display undecoded messages

#define msg4_MERTIK 0x01		//Mertik maxitrol
#define msg4_AD 0x02			//AD LightwaveRF
#define msg4_HID 0x04			//Hideki
#define msg4_LCROS 0x08			//LaCrosse
#define msg4_LEGRAND 0x10		//Legrand CAD
#define msg4_RFU 0x20			//RFU
#define msg4_BLINDST0 0x40		//Rollertrol,Hasta new
#define msg4_BLINDST1 0x80		//BlindsT1-4

#define msg5_X10 0x01			//X10
#define msg5_ARC 0x02			//ARC
#define msg5_AC 0x04			//AC
#define msg5_HEU 0x08			//HomeEasy EU
#define msg5_MEI 0x10			//Meiantech,Atlantic
#define msg5_OREGON 0x20		//Oregon Scientific
#define msg5_ATI 0x40			//ATI remotes
#define msg5_VISONIC 0x80		//Visonic PowerCode

#define msg6_KeeLoq 0x01		//Keeloq
#define msg6_HC	0x02			//HomeConfort
#define msg6_RFU2 0x04			//RFU
#define msg6_RFU3 0x08			//RFU
#define msg6_RFU4 0x10			//RFU
#define msg6_RFU5 0x20			//RFU
#define msg6_MCZ 0x40			//MCZ
#define msg6_Funkbus 0x80		//Funkbus

//868 config bits
#define msg3_868_RFU0 0x01		//RFU
#define msg3_868_DAVISAU 0x02	//Davis AU
#define msg3_868_DAVISUS 0x04	//Davis US
#define msg3_868_DAVISEU 0x08	//Davis EU
#define msg3_868_LACROSSE 0x10  //LACROSSE
#define msg3_868_ALECTO5500 0x20	//Alecto WS5500
#define msg3_868_ALECTO 0x40	//Alecto ACH2010
#define msg3_868_UNDEC 0x80		//Enable undecoded

#define msg4_868_EDISIO 0x01	//EDISIO
#define msg4_868_LWRF 0x02		//LightwaveRF
#define msg4_868_FS20 0x04		//FS20
#define msg4_868_RFU3 0x08		//RFU
#define msg4_868_RFU4 0x10		//RFU
#define msg4_868_RFU5 0x20		//RFU
#define msg4_868_RFU6 0x40		//RFU
#define msg4_868_RFU7 0x80		//RFU

#define msg5_868_RFU0 0x01		//RFU
#define msg5_868_RFU1 0x02		//RFU
#define msg5_868_RFU2 0x04		//RFU
#define msg5_868_RFU3 0x08		//RFU
#define msg5_868_PROGUARD  0x10 //Proguard
#define msg5_868_KEELOQ 0x20    //KEELOQ
#define msg5_868_MEIANTECH 0x40	//Meiantech,Atlantic
#define msg5_868_VISONIC 0x80	//Visonic

#define msg6_868_RFU0 0x01		//RFU
#define msg6_868_RFU1 0x02		//RFU
#define msg6_868_RFU2 0x04		//RFU
#define msg6_868_RFU3 0x08		//RFU
#define msg6_868_RFU4 0x10		//RFU
#define msg6_868_HONCHIME 0x20	//Honeywell Chime
#define msg6_868_ITHOECO 0x40	//Itho CVE ECO RFT
#define msg6_868_ITHO 0x80		//Itho CVE RFT

#define pTypeRecXmitMessage 0x02
#define sTypeReceiverLockError 0x00
#define sTypeTransmitterResponse 0x01

//undecoded types
#define pTypeUndecoded 0x03
#define sTypeUac 0x00
#define sTypeUarc 0x01
#define sTypeUati 0x02
#define sTypeUhideki 0x03
#define sTypeUlacrosse 0x04
#define sTypeUad 0x05
#define sTypeUmertik 0x06
#define sTypeUoregon1 0x07
#define sTypeUoregon2 0x08
#define sTypeUoregon3 0x09
#define sTypeUproguard 0x0A
#define sTypeUvisonic 0x0B
#define sTypeUnec 0x0C
#define sTypeUfs20 0x0D
#define sTypeUrsl 0x0E
#define sTypeUblinds 0x0F
#define sTypeUrubicson 0x10
#define sTypeUae 0x11
#define sTypeUfineoffset 0x12
#define sTypeUrgb 0x13
#define sTypeUrfy 0x14
#define sTypeUselectplus 0x15
#define sTypeUhomeconfort 0x16
#define sTypeUfunkbus 0x19

//types for Lighting
#define pTypeLighting1 0x10
#define sTypeX10 0x0
#define sTypeARC 0x1
#define sTypeAB400D 0x2
#define sTypeWaveman 0x3
#define sTypeEMW200 0x4
#define sTypeIMPULS 0x5
#define sTypeRisingSun 0x6
#define sTypePhilips 0x7
#define sTypeEnergenie 0x8
#define sTypeEnergenie5 0x9
#define sTypeGDR2 0x0A
#define sTypeHQ 0x0B
#define sTypeOase 0x0C

#define light1_sOff 0x0
#define light1_sOn 0x1
#define light1_sDim 0x2
#define light1_sBright 0x3
#define light1_sProgram 0x4
#define light1_sAllOff 0x5
#define light1_sAllOn 0x6
#define light1_sChime 0x7

#define pTypeLighting2 0x11
#define sTypeAC 0x0
#define sTypeHEU 0x1
#define sTypeANSLUT 0x2
#define sTypeKambrook 0x03

#define light2_sOff 0x0
#define light2_sOn 0x1
#define light2_sSetLevel 0x2
#define light2_sGroupOff 0x3
#define light2_sGroupOn 0x4
#define light2_sSetGroupLevel 0x5

#define pTypeLighting3 0x12
#define sTypeKoppla 0x0
#define light3_sBright 0x0
#define light3_sDim 0x8
#define light3_sOn 0x10
#define light3_sLevel1 0x11
#define light3_sLevel2 0x12
#define light3_sLevel3 0x13
#define light3_sLevel4 0x14
#define light3_sLevel5 0x15
#define light3_sLevel6 0x16
#define light3_sLevel7 0x17
#define light3_sLevel8 0x18
#define light3_sLevel9 0x19
#define light3_sOff 0x1A
#define light3_sProgram 0x1B

#define pTypeLighting4 0x13
#define sTypePT2262 0x0

#define pTypeLighting5 0x14
#define sTypeLightwaveRF 0x0
#define sTypeEMW100 0x1
#define sTypeBBSB 0x2
#define sTypeMDREMOTE 0x03
#define sTypeRSL 0x04
#define sTypeLivolo 0x05
#define sTypeTRC02 0x06
#define sTypeAoke 0x07
#define sTypeTRC02_2 0x08
#define sTypeEurodomest 0x09
#define sTypeLivolo1to10 0x0A
#define sTypeRGB432W 0x0B
#define sTypeMDREMOTE107 0x0C
#define sTypeLegrandCAD 0x0D
#define sTypeAvantek 0x0E
#define sTypeIT 0x0F
#define sTypeMDREMOTE108 0x10
#define sTypeKangtai 0x11

#define light5_sOff 0x0
#define light5_sOn 0x1
#define light5_sGroupOff 0x2
#define light5_sLearn 0x2
#define light5_sGroupOn 0x3
#define light5_sMood1 0x3
#define light5_sMood2 0x4
#define light5_sMood3 0x5
#define light5_sMood4 0x6
#define light5_sMood5 0x7
#define light5_sUnlock 0xA
#define light5_sLock 0xB
#define light5_sAllLock 0xC
#define light5_sClose 0xD
#define light5_sStop 0xE
#define light5_sOpen 0xF
#define light5_sSetLevel 0x10
#define light5_sColourPalette 0x11
#define light5_sColourTone 0x12
#define light5_sColourCycle 0x13
#define light5_sPower 0x0
#define light5_sLight 0x1
#define light5_sBright 0x2
#define light5_sDim 0x3
#define light5_s100 0x4
#define light5_s50 0x5
#define light5_s25 0x6
#define light5_sModePlus 0x7
#define light5_sSpeedMin 0x8
#define light5_sSpeedPlus 0x9
#define light5_sModeMin 0xA

//Livolo All off, used for all types
#define light5_sLivoloAllOff 0x00

//Livolo 1-3 appliance modules
#define light5_sLivoloGang1Toggle 0x01
#define light5_sLivoloGang2Toggle 0x02
#define light5_sLivoloGang3Toggle 0x03

//Livolo dimmer
//#define light5_sLivoloToggle1 0x01
#define light5_sLivoloBright1 0x02
#define light5_sLivoloDim1 0x03

//Livolo 1-10 appliance modules, 7 and 9 is a dimmer
#define light5_sLivoloToggle1 0x01
#define light5_sLivoloToggle2 0x02
#define light5_sLivoloToggle3 0x03
#define light5_sLivoloToggle4 0x04
#define light5_sLivoloToggle5 0x05
#define light5_sLivoloToggle6 0x06
#define light5_sLivoloToggle7 0x07
#define light5_sLivoloBright7 0x08
#define light5_sLivoloDim7 0x09
#define light5_sLivoloToggle8 0x0A
#define light5_sLivoloToggle9 0x0B
#define light5_sLivoloBright9 0x0C
#define light5_sLivoloDim9 0x0D
#define light5_sLivoloToggle10 0x0E
#define light5_sLivoloScene1 0x0F
#define light5_sLivoloScene2 0x10
#define light5_sLivoloScene3 0x11
#define light5_sLivoloScene4 0x12
#define light5_sLivoloOkSet 0x13

#define light5_sRGBoff 0x00
#define light5_sRGBon 0x01
#define light5_sRGBbright 0x02
#define light5_sRGBdim 0x03
#define light5_sRGBcolorplus 0x04
#define light5_sRGBcolormin 0x05
#define light5_sMD107_Power 0x0
#define light5_sMD107_Bright 0x1
#define light5_sMD107_Dim 0x2
#define light5_sMD107_100 0x3
#define light5_sMD107_80 0x4
#define light5_sMD107_60 0x5
#define light5_sMD107_40 0x6
#define light5_sMD107_20 0x7
#define light5_sMD107_10 0x8
#define light5_sLegrandToggle 0x00

#define pTypeLighting6 0x15
#define sTypeBlyss 0x0
#define sTypeCuveo 0x1
#define light6_sOn 0x0
#define light6_sOff 0x1
#define light6_sGroupOn 0x2
#define light6_sGroupOff 0x3

#define pTypeChime 0x16
#define sTypeByronSX 0x0
#define sTypeByronMP001 0x1
#define sTypeSelectPlus 0x2
#define sTypeByronBY 0x3
#define sTypeEnvivo 0x4
#define sTypeAlfawise 0x5
#define chime_sound0 0x1
#define chime_sound1 0x3
#define chime_sound2 0x5
#define chime_sound3 0x9
#define chime_sound4 0xD
#define chime_sound5 0xE
#define chime_sound6 0x6
#define chime_sound7 0x2

#define pTypeFan 0x17
#define sTypeSiemensSF01 0x0
#define sTypeItho 0x1
#define sTypeLucciAir 0x2
#define sTypeSeavTXS4 0x3
#define sTypeWestinghouse 0x4
#define sTypeLucciAirDC 0x5
#define sTypeCasafan 0x6
#define sTypeFT1211R 0x7
#define sTypeFalmec 0x8
#define sTypeLucciAirDCII 0x9

#define fan_sTimer 0x1
#define fan_sMin 0x2
#define fan_sLearn 0x3
#define fan_sPlus 0x4
#define fan_sConfirm 0x5
#define fan_sLight 0x6
#define fan_Itho1 0x1
#define fan_Itho2 0x2
#define fan_Itho3 0x3
#define fan_IthoTimer 0x4
#define fan_IthoNotAtHome 0x5
#define fan_IthoLearn 0x6
#define fan_IthoEraseAll 0x7
#define fan_LucciHi 0x1
#define fan_LucciMed 0x2
#define fan_LucciLow 0x3
#define fan_LucciOff 0x4
#define fan_LucciLight 0x5
#define fan_T1 0x1
#define fan_T2 0x2
#define fan_T3 0x3
#define fan_T4 0x4
#define fan_WestinghouseHi 0x1
#define fan_WestinghouseMed 0x2
#define fan_WestinghouseLow 0x3
#define fan_WestinghouseOff 0x4
#define fan_WestinghouseLight 0x5
#define fan_LucciDCPower 0x1
#define fan_LucciDCPlus 0x2
#define fan_LucciDCMin 0x3
#define fan_LucciDCLight 0x4
#define fan_LucciDCReverse 0x5
#define fan_LucciDCNaturalflow 0x6
#define fan_LucciDCPair 0x7
#define fan_CasafanHi 0x1
#define fan_CasafanMed 0x2
#define fan_CasafanLow 0x3
#define fan_CasafanOff 0x4
#define fan_CasafanLight 0x5
#define fan_FT1211Rpower 0x1
#define fan_FT1211Rlight 0x2
#define fan_FT1211R1 0x3
#define fan_FT1211R2 0x4
#define fan_FT1211R3 0x5
#define fan_FT1211R4 0x6
#define fan_FT1211R5 0x7
#define fan_FT1211Rfr 0x8
#define fan_FT1211R1H 0x9
#define fan_FT1211R4H 0xA
#define fan_FT1211R8H 0xB
#define fan_FalmecPower 0x1
#define fan_FalmecSpeed1 0x2
#define fan_FalmecSpeed2 0x3
#define fan_FalmecSpeed3 0x4
#define fan_FalmecSpeed4 0x5
#define fan_FalmecTimer1 0x6
#define fan_FalmecTimer2 0x7
#define fan_FalmecTimer3 0x8
#define fan_FalmecTimer4 0x9
#define fan_FalmecLightOn 0xA
#define fan_FalmecLightOff 0xB
#define fan_LucciDCIIOff 0x1
#define fan_LucciDCII1 0x2
#define fan_LucciDCII2 0x3
#define fan_LucciDCII3 0x4
#define fan_LucciDCII4 0x5
#define fan_LucciDCII5 0x6
#define fan_LucciDCII6 0x7
#define fan_LucciDCIILight 0x8
#define fan_LucciDCIIReverse 0x9

//types for Curtain
#define pTypeCurtain 0x18
#define sTypeHarrison 0x0
#define curtain_sOpen 0x0
#define curtain_sClose 0x1
#define curtain_sStop 0x2
#define curtain_sProgram 0x3

//types for Blinds
#define pTypeBlinds 0x19
#define sTypeBlindsT0 0x0	//RollerTrol, Hasta new
#define sTypeBlindsT1 0x1	//Hasta old
#define sTypeBlindsT2 0x2	//A-OK RF01
#define sTypeBlindsT3 0x3	//A-OK AC114
#define sTypeBlindsT4 0x4	//RAEX YR1326
#define sTypeBlindsT5 0x5	//Media Mount
#define sTypeBlindsT6 0x6	//DC106, YOOHA, Rohrmotor24 RMF
#define sTypeBlindsT7 0x7	//Forest
#define sTypeBlindsT8 0x8	//Chamberlain CS4330CN
#define sTypeBlindsT9 0x9	//Sunpery
#define sTypeBlindsT10 0xA	//Dolat DLM-1
#define sTypeBlindsT11 0xB	//ASP
#define sTypeBlindsT12 0xC	//Confexx
#define sTypeBlindsT13 0xD	//Screenline
#define sTypeBlindsT14 0xE	//Hualite
#define sTypeBlindsT15 0xF	//RFU
#define sTypeBlindsT16 0x10	//Zemismart

#define blinds_sOpen 0x0
#define blinds_sClose 0x1
#define blinds_sStop 0x2
#define blinds_sConfirm 0x3
#define blinds_sLimit 0x4
#define blinds_slowerLimit 0x5
#define blinds_sDeleteLimits 0x6
#define blinds_sChangeDirection 0x7
#define blinds_sLeft 0x8
#define blinds_sRight 0x9
#define blinds_s6Im 0x4
#define blinds_s9ChangeDirection 0x6
#define blinds_s9ImA 0x7
#define blinds_s9ImCenter 0x8
#define blinds_s9ImB 0x9
#define blinds_s9EraseCurrentCh 0xA
#define blinds_s9EraseAllCh 0xB
#define blinds_s10LearnMaster 0x4
#define blinds_s10EraseCurrentCh 0x5
#define blinds_s10ChangeDirection 0x6
#define blinds_s13anglePlus 0x4
#define blinds_s13angleMinus 0x5
#define blinds_s16EraseCurrentCh 0x4
#define blinds_s16ChangeDirection 0x5

//types for RFY
#define pTypeRFY 0x1A
#define sTypeRFY 0x0	//RFY
#define sTypeRFYext 0x1	//RFY extended
#define sTypeASA 0x3	//ASA
#define rfy_sStop 0x0
#define rfy_sUp 0x1
#define rfy_sUpStop 0x2
#define rfy_sDown 0x3
#define rfy_sDownStop 0x4
#define rfy_sUpDown 0x5
#define rfy_sListRemotes 0x6
#define rfy_sProgram 0x7
#define rfy_s2SecProgram 0x8
#define rfy_s7SecProgram 0x9
#define rfy_s2SecStop 0xA
#define rfy_s5SecStop 0xB
#define rfy_s5SecUpDown 0xC
#define rfy_sEraseThis 0xD
#define rfy_sEraseAll 0xE
#define rfy_s05SecUp 0xF
#define rfy_s05SecDown 0x10
#define rfy_s2SecUp 0x11
#define rfy_s2SecDown 0x12
#define rfy_sEnableSunWind 0x13
#define rfy_sDisableSun 0x14

//types for Home Confort
#define pTypeHomeConfort 0x1B
#define sTypeHomeConfortTEL010 0x0
#define HomeConfort_sOff 0x0
#define HomeConfort_sOn 0x1
#define HomeConfort_sGroupOff 0x2
#define HomeConfort_sGroupOn 0x3

//types for Funkbus
#define pTypeFunkbus 0x1E
#define sTypeFunkbusRemoteGira 0x00
#define sTypeFunkbusRemoteInsta 0x01
#define Funkbus_sChannelMin 0x00
#define Funkbus_sChannelPlus 0x01
#define Funkbus_sAllOff 0x02
#define Funkbus_sAllOn 0x03
#define Funkbus_sScene 0x04
#define Funkbus_sMasterMin 0x05
#define Funkbus_sMasterPlus 0x06

//types for Security1
#define pTypeSecurity1 0x20
#define sTypeSecX10 0x0				//X10 security
#define sTypeSecX10M 0x1			//X10 security motion
#define sTypeSecX10R 0x2			//X10 security remote
#define sTypeKD101 0x3				//KD101 smoke detector
#define sTypePowercodeSensor 0x04	//Visonic PowerCode sensor - primary contact
#define sTypePowercodeMotion 0x05	//Visonic PowerCode motion
#define sTypeCodesecure 0x06		//Visonic CodeSecure
#define sTypePowercodeAux 0x07		//Visonic PowerCode sensor - auxiliary contact
#define sTypeMeiantech 0x8			//Meiantech
#define sTypeSA30 0x9				//SA30 smoke detector
#define sTypeRM174RF 0xA			//RM174RF smoke detector

//status security
#define sStatusNormal 0x0
#define sStatusNormalDelayed 0x1
#define sStatusAlarm 0x2
#define sStatusAlarmDelayed 0x3
#define sStatusMotion 0x4
#define sStatusNoMotion 0x5
#define sStatusPanic 0x6
#define sStatusPanicOff 0x7
#define sStatusIRbeam 0x8
#define sStatusArmAway 0x9
#define sStatusArmAwayDelayed 0xA
#define sStatusArmHome 0xB
#define sStatusArmHomeDelayed 0xC
#define sStatusDisarm 0xD
#define sStatusLightOff 0x10
#define sStatusLightOn 0x11
#define sStatusLight2Off 0x12
#define sStatusLight2On 0x13
#define sStatusDark 0x14
#define sStatusLight 0x15
#define sStatusBatLow 0x16
#define sStatusPairKD101 0x17
#define sStatusNormalTamper 0x80
#define sStatusNormalDelayedTamper 0x81
#define sStatusAlarmTamper 0x82
#define sStatusAlarmDelayedTamper 0x83
#define sStatusMotionTamper 0x84
#define sStatusNoMotionTamper 0x85

//types for Security2
#define pTypeSecurity2 0x21
#define sTypeSec2Classic 0x0

//types for Camera
#define pTypeCamera 0x28
#define sTypeNinja 0x0		//X10 Ninja/Robocam
#define camera_sLeft 0x0
#define camera_sRight 0x1
#define camera_sUp 0x2
#define camera_sDown 0x3
#define camera_sPosition1 0x4
#define camera_sProgramPosition1 0x5
#define camera_sPosition2 0x6
#define camera_sProgramPosition2 0x7
#define camera_sPosition3 8
#define camera_sProgramPosition3 0x9
#define camera_sPosition4 0xA
#define camera_sProgramPosition4 0xB
#define camera_sCenter 0xC
#define camera_sProgramCenterPosition 0xD
#define camera_sSweep 0xE
#define camera_sProgramSweep 0xF

//types for Remotes
#define pTypeRemote 0x30
#define sTypeATI 0x0		//ATI Remote Wonder
#define sTypeATIplus 0x1	//ATI Remote Wonder Plus
#define sTypeMedion 0x2		//Medion Remote
#define sTypePCremote 0x3	//PC Remote
#define sTypeATIrw2 0x4		//ATI Remote Wonder II

//types for Thermostat
#define pTypeThermostat1 0x40
#define sTypeDigimax 0x0		//Digimax
#define sTypeDigimaxShort 0x1	//Digimax with short format
#define thermostat1_sNoStatus 0x0
#define thermostat1_sDemand 0x1
#define thermostat1_sNoDemand 0x2
#define thermostat1_sInitializing 0x3

#define pTypeThermostat2 0x41
#define sTypeHE105 0x0
#define sTypeRTS10 0x1
#define thermostat2_sOff 0x0
#define thermostat2_sOn 0x1
#define thermostat2_sProgram 0x2

#define pTypeThermostat3 0x42
#define sTypeMertikG6RH4T1 0x0	//Mertik G6R-H4T1
#define sTypeMertikG6RH4TB 0x1	//Mertik G6R-H4TB
#define sTypeMertikG6RH4TD 0x2	//Mertik G6R-H4TD
#define sTypeMertikG6RH4S 0x3	//Mertik G6R-H4S
#define thermostat3_sOff 0x0
#define thermostat3_sOn 0x1
#define thermostat3_sUp 0x2
#define thermostat3_sDown 0x3
#define thermostat3_sRunUp 0x4
#define thermostat3_Off2nd 0x4
#define thermostat3_sRunDown 0x5
#define thermostat3_On2nd 0x5
#define thermostat3_sStop 0x6

#define pTypeThermostat4 0x43
#define sTypeMCZ1 0x0	//MCZ 1 fan model
#define sTypeMCZ2 0x1	//MCZ 2 fan model
#define sTypeMCZ3 0x2	//MCZ 3 fan model
#define thermostat4_sOff 0x0
#define thermostat4_sManual 0x1
#define thermostat4_sAuto 0x2
#define thermostat4_sEco 0x3

//types for Radiator valve
#define pTypeRadiator1 0x48
#define sTypeSmartwares 0x0	//Homewizard smartwares

#define Radiator1_sNight 0x0
#define Radiator1_sDay 0x1
#define Radiator1_sSetTemp 0x2

//types for BBQ temperature
#define pTypeBBQ 0x4E
#define sTypeBBQ1 0x1  //Maverick ET-732

//types for temperature+rain
#define pTypeTEMP_RAIN 0x4F
#define sTypeTR1 0x1  //WS1200

//types for temperature
#define pTypeTEMP 0x50
#define sTypeTEMP1 0x1  //THR128/138,THC138, Davis
#define sTypeTEMP2 0x2  //THC238/268,THN132,THWR288,THRN122,THN122,AW129/131
#define sTypeTEMP3 0x3  //THWR800
#define sTypeTEMP4 0x4	//RTHN318
#define sTypeTEMP5 0x5  //LaCrosse TX3
#define sTypeTEMP6 0x6  //TS15C
#define sTypeTEMP7 0x7  //Viking 02811,TSS330
#define sTypeTEMP8 0x8  //LaCrosse WS2300
#define sTypeTEMP9 0x9  //RUBiCSON
#define sTypeTEMP10 0xA  //TFA 30.3133
#define sTypeTEMP11 0xB  //WT0122

//types for humidity
#define pTypeHUM 0x51
#define sTypeHUM1 0x1  //LaCrosse TX3, Davis
#define sTypeHUM2 0x2  //LaCrosse WS2300
#define sTypeHUM3 0x03  //Inovalley S80 plant humidity sensor

//status types for humidity
#define humstat_normal 0x0
#define humstat_comfort 0x1
#define humstat_dry 0x2
#define humstat_wet 0x3

//types for temperature+humidity
#define pTypeTEMP_HUM 0x52
#define sTypeTH1 0x1  //THGN122/123,THGN132,THGR122/228/238/268
#define sTypeTH2 0x2  //THGR810,THGN800
#define sTypeTH3 0x3  //RTGR328
#define sTypeTH4 0x4  //THGR328
#define sTypeTH5 0x5  //WTGR800
#define sTypeTH6 0x6  //THGR918,THGRN228,THGN500
#define sTypeTH7 0x7  //TFA TS34C, Cresta
#define sTypeTH8 0x8  //WT450H
#define sTypeTH9 0x9  //Viking 02035,02038 (02035 has no humidity), TSS320
#define sTypeTH10 0xA   //Rubicson
#define sTypeTH11 0xB   //EW109
#define sTypeTH12 0xC   //Imagintronix
#define sTypeTH13 0xD   //Alecto WS1700 and compatibles
#define sTypeTH14 0xE   //Alecto

//types for barometric
#define pTypeBARO 0x53

//types for temperature+humidity+baro
#define pTypeTEMP_HUM_BARO 0x54
#define sTypeTHB1 0x1   //BTHR918,BTHGN129
#define sTypeTHB2 0x2   //BTHR918N,BTHR968
#define baroForecastNoInfo 0x00
#define baroForecastSunny 0x01
#define baroForecastPartlyCloudy 0x02
#define baroForecastCloudy 0x03
#define baroForecastRain 0x04

//types for rain
#define pTypeRAIN 0x55
#define sTypeRAIN1 0x1   //RGR126/682/918
#define sTypeRAIN2 0x2   //PCR800
#define sTypeRAIN3 0x3   //TFA
#define sTypeRAIN4 0x4   //UPM
#define sTypeRAIN5 0x5   //WS2300
#define sTypeRAIN6 0x6   //TX5
#define sTypeRAIN7 0x7   //Alecto
#define sTypeRAIN8 0x8   //Davis

//types for wind
#define pTypeWIND 0x56
#define sTypeWIND1 0x1   //WTGR800
#define sTypeWIND2 0x2   //WGR800
#define sTypeWIND3 0x3   //STR918,WGR918
#define sTypeWIND4 0x4   //TFA
#define sTypeWIND5 0x5   //UPM, Davis
#define sTypeWIND6 0x6   //WS2300
#define sTypeWIND7 0x7   //Alecto WS4500

//types for uv
#define pTypeUV 0x57
#define sTypeUV1 0x1   //UVN128,UV138, Davis
#define sTypeUV2 0x2   //UVN800
#define sTypeUV3 0x3   //TFA

//types for date & time
#define pTypeDT 0x58
#define sTypeDT1 0x1   //RTGR328N

//types for current
#define pTypeCURRENT 0x59
#define sTypeELEC1 0x1   //CM113,Electrisave

//types for energy
#define pTypeENERGY 0x5A
#define sTypeELEC2 0x1   //CM119/160
#define sTypeELEC3 0x2   //CM180

//types for current-energy
#define pTypeCURRENTENERGY 0x5B
#define sTypeELEC4 0x1   //CM180i

//types for power
#define pTypePOWER 0x5C
#define sTypeELEC5 0x1   //revolt

//types for weight scales
#define pTypeWEIGHT 0x5D
#define sTypeWEIGHT1 0x1   //BWR102
#define sTypeWEIGHT2 0x2   //GR101

//types for gas
#define pTypeGAS 0x5E

//types for water
#define pTypeWATER 0x5F

//types for CARTELECTRONIC
#define pTypeCARTELECTRONIC 0x60
#define sTypeTIC 0x1
#define sTypeCEencoder 0x2
#define sTypeLinky 0x3

//types for Async port configuration
#define pTypeASYNCPORT 0x61
#define sTypeASYNCconfig 0x01
#define asyncdisable 0x0
#define asyncreceiveP1 0x1
#define asyncreceiveTeleinfo 0x2
#define asyncreceiveRAW 0xFE //not yet implemented
#define asyncreceiveGetSettings 0xFF
#define asyncbaud110 0x0
#define asyncbaud300 0x1
#define asyncbaud600 0x2
#define asyncbaud1200 0x3
#define asyncbaud2400 0x4
#define asyncbaud4800 0x5
#define asyncbaud9600 0x6
#define asyncbaud14400 0x7
#define asyncbaud19200 0x8
#define asyncbaud38400 0x9
#define asyncbaud57600 0xA
#define asyncbaud115200 0xB
#define asyncParityNo 0x0
#define asyncParityOdd 0x1
#define asyncParityEven 0x2
#define asyncDatabits7 0x7
#define asyncDatabits8 0x8
#define asyncStopbits1 0x1
#define asyncStopbits2 0x2
#define asyncPolarityNormal 0x0
#define asyncPolarityInvers 0x1

//types for Async data
#define pTypeASYNCDATA 0x62
#define sTypeASYNCoverrun 0xFF
#define sTypeASYNCp1 0x01
#define sTypeASYNCteleinfo 0x02
#define sTypeASYNCraw 0x03

//RFXSensor
#define pTypeRFXSensor 0x70
#define sTypeRFXSensorTemp 0x0
#define sTypeRFXSensorAD 0x1
#define sTypeRFXSensorVolt 0x2
#define sTypeRFXSensorMessage 0x3

//RFXMeter
#define pTypeRFXMeter 0x71
#define sTypeRFXMeterCount 0x0
#define sTypeRFXMeterInterval 0x1
#define sTypeRFXMeterCalib 0x2
#define sTypeRFXMeterAddr 0x3
#define sTypeRFXMeterCounterReset 0x4
#define sTypeRFXMeterCounterSet 0xB
#define sTypeRFXMeterSetInterval 0xC
#define sTypeRFXMeterSetCalib 0xD
#define sTypeRFXMeterSetAddr 0xE
#define sTypeRFXMeterIdent 0xF

//FS20
#define pTypeFS20 0x72
#define sTypeFS20 0x0
#define sTypeFHT8V 0x1
#define sTypeFHT80 0x2
#define fs20_sOff 0x0
#define fs20_sDimlevel_1 0x1
#define fs20_sDimlevel_2 0x2
#define fs20_sDimlevel_3 0x3
#define fs20_sDimlevel_4 0x4
#define fs20_sDimlevel_5 0x5
#define fs20_sDimlevel_6 0x6
#define fs20_sDimlevel_7 0x7
#define fs20_sDimlevel_8 0x8
#define fs20_sDimlevel_9 0x9
#define fs20_sDimlevel_10 0xA
#define fs20_sDimlevel_11 0xB
#define fs20_sDimlevel_12 0xC
#define fs20_sDimlevel_13 0xD
#define fs20_sDimlevel_14 0xE
#define fs20_sDimlevel_15 0xF
#define fs20_sOn_100 0x10
#define fs20_sOn_last_dim 0x11
#define fs20_sToggle_On_Off 0x12
#define fs20_sBright 0x13
#define fs20_sDim 0x14
#define fs20_sStart_dim_cycle 0x15
#define fs20_sProgram_timer 0x16
#define fs20_sRequest_status 0x17
#define fs20_sOff_for_time_period 0x18
#define fs20_sOn_100_for_time_period 0x19 
#define fs20_sOn_last_dim_level_period 0x1A 
#define fs20_sReset 0x1B

//WEATHER STATIONS
#define pTypeWEATHER 0x76
#define sTypeWEATHER1 0x1   //Alecto ACH2010
#define sTypeWEATHER2 0x2   //Alecto WS5500

//types for Solar
#define pTypeSOLAR 0x77
#define sTypeSOLAR1 0x1   //Davis

//RAW transit/receive
#define pTypeRAW 0x7F
#define sTypeRAW1 0x0
#define sTypeRAW2 0x1
#define sTypeRAW3 0x2
#define sTypeRAW4 0x3

typedef union tRBUF {
	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	cmnd;
		BYTE	freqsel;
		BYTE	xmitpwr;
		BYTE	msg3;
		BYTE	msg4;
		BYTE	msg5;
		BYTE	msg6;
		BYTE	msg7;
		BYTE	msg8;
		BYTE	msg9;
	} ICMND;

	struct {	//response on a mode command from the application
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	cmnd;
		BYTE	msg1;	//receiver/transceiver type
		BYTE	msg2;	//firmware version

#ifdef IS_BIG_ENDIAN
		//BYTE	msg3;
		BYTE	UNDECODEDenabled : 1;
		BYTE	IMAGINTRONIXenabled : 1;
		BYTE	SXenabled : 1;
		BYTE	RSLenabled : 1;
		BYTE	LIGHTING4enabled : 1;
		BYTE	FINEOFFSETenabled : 1;
		BYTE	RUBICSONenabled : 1;
		BYTE	AEenabled : 1;

		//BYTE	msg4;
		BYTE	BLINDST1enabled : 1;
		BYTE	BLINDST0enabled : 1;
		BYTE	MSG4Reserved5 : 1;
		BYTE	LEGRANDenabled : 1;
		BYTE	LACROSSEenabled : 1;
		BYTE	HIDEKIenabled : 1;
		BYTE	LWRFenabled : 1;
		BYTE	MERTIKenabled : 1;

		//BYTE	msg5;
		BYTE	VISONICenabled : 1;
		BYTE	ATIenabled : 1;
		BYTE	OREGONenabled : 1;
		BYTE	MEIANTECHenabled : 1;
		BYTE	HEEUenabled : 1;
		BYTE	ACenabled : 1;
		BYTE	ARCenabled : 1;
		BYTE	X10enabled : 1; //note: keep this order

		//BYTE    msg6;
        BYTE    MSG6Reserved7 : 1;
        BYTE    MSG6Reserved6 : 1;
        BYTE    MSG6Reserved5 : 1;
        BYTE    MSG6Reserved4 : 1;
        BYTE    MSG6Reserved3 : 1;
        BYTE    MSG6Reserved2 : 1;
		BYTE    HCEnabled : 1;
        BYTE    KEELOQenabled : 1;
#else
		//BYTE	msg3;
		BYTE	AEenabled : 1;
		BYTE	RUBICSONenabled : 1;
		BYTE	FINEOFFSETenabled : 1;
		BYTE	LIGHTING4enabled : 1;
		BYTE	RSLenabled : 1;
		BYTE	SXenabled : 1;
		BYTE	IMAGINTRONIXenabled : 1;
		BYTE	UNDECODEDenabled : 1;

		//BYTE	msg4;
		BYTE	MERTIKenabled : 1;
		BYTE	LWRFenabled : 1;
		BYTE	HIDEKIenabled : 1;
		BYTE	LACROSSEenabled : 1;
		BYTE	LEGRANDenabled : 1;
		BYTE	MSG4Reserved5 : 1;
		BYTE	BLINDST0enabled : 1;
		BYTE	BLINDST1enabled : 1;

		//BYTE	msg5;
		BYTE	X10enabled : 1; //note: keep this order
		BYTE	ARCenabled : 1;
		BYTE	ACenabled : 1;
		BYTE	HEEUenabled : 1;
		BYTE	MEIANTECHenabled : 1;
		BYTE	OREGONenabled : 1;
		BYTE	ATIenabled : 1;
		BYTE	VISONICenabled : 1;

        //BYTE	msg6;
        BYTE    KEELOQenabled : 1;
		BYTE    HCEnabled : 1;
        BYTE    MSG6Reserved2 : 1;
        BYTE    MSG6Reserved3 : 1;
        BYTE    MSG6Reserved4 : 1;
        BYTE    MSG6Reserved5 : 1;
        BYTE    MSG6Reserved6 : 1;
        BYTE    MSG6Reserved7 : 1;
#endif

		BYTE	msg7;
		BYTE	msg8;
		BYTE	msg9;
		BYTE	msg10;
        BYTE	msg11;
        BYTE	msg12;
        BYTE	msg13;
        BYTE	msg14;
        BYTE	msg15;
        BYTE	msg16;
	} IRESPONSE;

	struct {	//response on a mode command from the application
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	cmnd;
		BYTE	msg1;	//receiver/transceiver type
		BYTE	msg2;	//firmware version

#ifdef IS_BIG_ENDIAN
		//BYTE	msg3;
		BYTE	UNDECODEDenabled : 1;
		BYTE	ALECTOenabled : 1;
		BYTE	ALECTO5500enabled : 1;
		BYTE	LACROSSEenabled : 1;
		BYTE	DAVISEUenabled : 1;
		BYTE	DAVISUSenabled : 1;
		BYTE	DAVISAUenabled : 1;
		BYTE	MSG3Reserved0 : 1;

		//BYTE	msg4;
		BYTE	MSG4Reserved7 : 1;
		BYTE	MSG4Reserved6 : 1;
		BYTE	MSG4Reserved5 : 1;
		BYTE	MSG4Reserved4 : 1;
		BYTE	MSG4Reserved3 : 1;
		BYTE	FS20enabled : 1;
		BYTE	LWRFenabled : 1;
		BYTE	EDISIOenabled : 1;

		//BYTE	msg5;
		BYTE	VISONICenabled : 1;
		BYTE	MEIANTECHenabled : 1;
		BYTE	KEELOQenabled : 1;
		BYTE	PROGUARDenabled : 1;
		BYTE	MSG5Reserved3 : 1;
		BYTE	MSG5Reserved2 : 1;
		BYTE	MSG5Reserved1 : 1;
		BYTE	MSG5Reserved0 : 1; //note: keep this order

		//BYTE    msg6;
		BYTE    ITHOenabled : 1;
		BYTE    ITHOecoenabled : 1;
		BYTE    HONEYWELLenabled : 1;
		BYTE    MSG6Reserved4 : 1;
		BYTE    MSG6Reserved3 : 1;
		BYTE    MSG6Reserved2 : 1;
		BYTE    MSG6Reserved1 : 1;
		BYTE    MSG6Reserved0 : 1;
#else
		//BYTE	msg3;
		BYTE	MSG3Reserved0 : 1;
		BYTE	DAVISAUenabled : 1;
		BYTE	DAVISUSenabled : 1;
		BYTE	DAVISEUenabled : 1;
		BYTE	LACROSSEenabled : 1;
		BYTE	ALECTO5500enabled : 1;
		BYTE	ALECTOenabled : 1;
		BYTE	UNDECODEDenabled : 1;

		//BYTE	msg4;
		BYTE	EDISIOenabled : 1;
		BYTE	LWRFenabled : 1;
		BYTE	FS20enabled : 1;
		BYTE	MSG4Reserved3 : 1;
		BYTE	MSG4Reserved4 : 1;
		BYTE	MSG4Reserved5 : 1;
		BYTE	MSG4Reserved6 : 1;
		BYTE	MSG4Reserved7 : 1;

		//BYTE	msg5;
		BYTE	MSG5Reserved0 : 1; //note: keep this order
		BYTE	MSG5Reserved1 : 1;
		BYTE	MSG5Reserved2 : 1;
		BYTE	MSG5Reserved3 : 1;
		BYTE	PROGUARDenabled : 1;
		BYTE    KEELOQenabled : 1;
		BYTE	MEIANTECHenabled : 1;
		BYTE	VISONICenabled : 1;

		//BYTE	msg6;
		BYTE    MSG6Reserved0 : 1;
		BYTE    MSG6Reserved1 : 1;
		BYTE    MSG6Reserved2 : 1;
		BYTE    MSG6Reserved3 : 1;
		BYTE    MSG6Reserved4 : 1;
		BYTE    HONEYWELLenabled : 1;
		BYTE    ITHOecoenabled : 1;
		BYTE    ITHOenabled : 1;
#endif

		BYTE	msg7;
		BYTE	msg8;
		BYTE	msg9;
		BYTE	msg10;
		BYTE	msg11;
		BYTE	msg12;
		BYTE	msg13;
		BYTE	msg14;
		BYTE	msg15;
		BYTE	msg16;
	} IRESPONSE868;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	msg1;
		BYTE	msg2;
		BYTE	msg3;
		BYTE	msg4;
		BYTE	msg5;
		BYTE	msg6;
		BYTE	msg7;
		BYTE	msg8;
		BYTE	msg9;
		BYTE	msg10;
		BYTE	msg11;
		BYTE	msg12;
		BYTE	msg13;
		BYTE	msg14;
		BYTE	msg15;
		BYTE	msg16;
		BYTE	msg17;
		BYTE	msg18;
		BYTE	msg19;
		BYTE	msg20;
		BYTE	msg21;
		BYTE	msg22;
		BYTE	msg23;
		BYTE	msg24;
		BYTE	msg25;
		BYTE	msg26;
		BYTE	msg27;
		BYTE	msg28;
		BYTE	msg29;
		BYTE	msg30;
		BYTE	msg31;
		BYTE	msg32;
		BYTE	msg33;
	} UNDECODED;

	struct {	//receiver/transmitter messages
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	msg;
	} RXRESPONSE;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	housecode;
		BYTE	unitcode;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} LIGHTING1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	id4;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	level;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} LIGHTING2;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	system;
		BYTE	channel8_1;
		BYTE	channel10_9;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} LIGHTING3;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	cmd1;
		BYTE	cmd2;
		BYTE	cmd3;
		BYTE	pulseHigh;
		BYTE	pulseLow;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} LIGHTING4;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	level;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} LIGHTING5;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	groupcode;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	cmndseqnbr;
		BYTE	seqnbr2;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} LIGHTING6;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	sound;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler  : 4;
		BYTE	rssi : 4;
#endif
	} CHIME;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler  : 4;
		BYTE	rssi : 4;
#endif
	} FAN;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	housecode;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	filler;
	} CURTAIN1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
#ifdef IS_BIG_ENDIAN
		BYTE	id4 : 4;
		BYTE	unitcode : 4;
		BYTE	cmnd;
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	unitcode : 4;
		BYTE	id4 : 4;
		BYTE	cmnd;
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} BLINDS1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	rfu1;
		BYTE	rfu2;
		BYTE	rfu3;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} RFY;

	struct {
		BYTE packetlength;
		BYTE packettype;
		BYTE subtype;
		BYTE seqnbr;
		BYTE id1;
		BYTE id2;
		BYTE id3;
		BYTE housecode;
		BYTE unitcode;
		BYTE cmnd;
		BYTE rfu1;
		BYTE rfu2;
#ifdef IS_BIG_ENDIAN
		BYTE    rssi : 4;
		BYTE    filler : 4;
#else
		BYTE    filler : 4;
		BYTE    rssi : 4;
#endif
	} HOMECONFORT;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	groupcode;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	cmndtime;
#ifdef IS_BIG_ENDIAN
		BYTE	filler1 : 4;
		BYTE	devtype : 4;
#else
		BYTE	devtype : 4;
		BYTE	filler1 : 4;
#endif
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} FUNKBUS;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	status;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} SECURITY1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	id4;
		BYTE	id5;
		BYTE	id6;
		BYTE	id7;
		BYTE	id8;
		BYTE	id9;
		BYTE	id10;
		BYTE	id11;
		BYTE	id12;
		BYTE	id13;
		BYTE	id14;
		BYTE	id15;
		BYTE	id16;
		BYTE	id17;
		BYTE	id18;
		BYTE	id19;
		BYTE	id20;
		BYTE	id21;
		BYTE	id22;
		BYTE	id23;
		BYTE	id24;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} SECURITY2;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	housecode;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} CAMERA1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	cmndtype : 3;
		BYTE	toggle : 1;
#else
		BYTE	toggle : 1;
		BYTE	cmndtype : 3;
		BYTE	rssi : 4;
#endif
	} REMOTE;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	temperature;
		BYTE	set_point;
#ifdef IS_BIG_ENDIAN
		BYTE	mode : 1;
		BYTE	filler : 5;
		BYTE	status : 2;

		BYTE	rssi : 4;
		BYTE	filler1 : 4;
#else
		BYTE	status : 2;
		BYTE	filler : 5;
		BYTE	mode : 1;

		BYTE	filler1 : 4;
		BYTE	rssi : 4;
#endif
	} THERMOSTAT1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	unitcode;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} THERMOSTAT2;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	unitcode1;
		BYTE	unitcode2;
		BYTE	unitcode3;
		BYTE	cmnd;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} THERMOSTAT3;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	unitcode1;
		BYTE	unitcode2;
		BYTE	unitcode3;
		BYTE	beep;
		BYTE	fan1_speed;
#ifdef IS_BIG_ENDIAN
		BYTE	fan3_speed : 4;
		BYTE	fan2_speed : 4;
#else
		BYTE	fan2_speed : 4;
		BYTE	fan3_speed : 4;
#endif
		BYTE	flame_power;
		BYTE	mode;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} THERMOSTAT4;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	id4;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	temperature;
		BYTE	tempPoint5;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} RADIATOR1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	sensor1h;
		BYTE	sensor1l;
		BYTE	sensor2h;
		BYTE	sensor2l;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} BBQ;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
#ifdef IS_BIG_ENDIAN
		BYTE	tempsign : 1;
		BYTE	temperatureh : 7;

		BYTE	temperaturel;
		BYTE	raintotal1;
		BYTE	raintotal2;

		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;

		BYTE	temperaturel;
		BYTE	raintotal1;
		BYTE	raintotal2;

		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} TEMP_RAIN;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
#ifdef IS_BIG_ENDIAN
		BYTE	tempsign : 1;
		BYTE	temperatureh : 7;

		BYTE	temperaturel;

		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;

		BYTE	temperaturel;

		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} TEMP;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	humidity; 
		BYTE	humidity_status;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} HUM;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
#ifdef IS_BIG_ENDIAN
		BYTE	tempsign : 1;
		BYTE	temperatureh : 7;

		BYTE	temperaturel;
		BYTE	humidity; 
		BYTE	humidity_status;

		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;

		BYTE	temperaturel;
		BYTE	humidity; 
		BYTE	humidity_status;

		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} TEMP_HUM;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	baro1;
		BYTE	baro2;
		BYTE	forecast;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} BARO;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
#ifdef IS_BIG_ENDIAN
		BYTE	tempsign : 1;
		BYTE	temperatureh : 7;
#else
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
#endif
		BYTE	temperaturel;
		BYTE	humidity; 
		BYTE	humidity_status;
		BYTE	baroh;
		BYTE	barol;
		BYTE	forecast;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} TEMP_HUM_BARO;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	rainrateh;
		BYTE	rainratel;
		BYTE	raintotal1;
		BYTE	raintotal2;
		BYTE	raintotal3;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} RAIN;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	directionh;
		BYTE	directionl;
		BYTE	av_speedh;
		BYTE	av_speedl;
		BYTE	gusth;
		BYTE	gustl;
#ifdef IS_BIG_ENDIAN
		BYTE	tempsign : 1;
		BYTE	temperatureh : 7;

		BYTE	temperaturel;

		BYTE	chillsign : 1;
		BYTE	chillh : 7;

		BYTE	chilll;

		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;

		BYTE	temperaturel;

		BYTE	chillh : 7;
		BYTE	chillsign : 1;

		BYTE	chilll;

		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} WIND;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	uv;
#ifdef IS_BIG_ENDIAN
		BYTE	tempsign : 1;
		BYTE	temperatureh : 7;

		BYTE	temperaturel;

		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;

		BYTE	temperaturel;

		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} UV;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	yy;
		BYTE	mm;
		BYTE	dd;
		BYTE	dow;
		BYTE	hr;
		BYTE	min;
		BYTE	sec;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} DT;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	count;
		BYTE	ch1h;
		BYTE	ch1l;
		BYTE	ch2h;
		BYTE	ch2l;
		BYTE	ch3h;
		BYTE	ch3l;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} CURRENT;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	count;
		BYTE	instant1;
		BYTE	instant2;
		BYTE	instant3;
		BYTE	instant4;
		BYTE	total1;
		BYTE	total2;
		BYTE	total3;
		BYTE	total4;
		BYTE	total5;
		BYTE	total6;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} ENERGY;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	count;
		BYTE	ch1h;
		BYTE	ch1l;
		BYTE	ch2h;
		BYTE	ch2l;
		BYTE	ch3h;
		BYTE	ch3l;
		BYTE	total1;
		BYTE	total2;
		BYTE	total3;
		BYTE	total4;
		BYTE	total5;
		BYTE	total6;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} CURRENT_ENERGY;

	struct {
        BYTE	packetlength;
        BYTE	packettype;
        BYTE	subtype;
        BYTE	seqnbr;
        BYTE	id1;
        BYTE	id2;
        BYTE	voltage;
        BYTE	currentH;
        BYTE	currentL;
        BYTE	powerH;
        BYTE	powerL;
        BYTE	energyH;
        BYTE	energyL;
        BYTE	pf;
        BYTE	freq;
#ifdef IS_BIG_ENDIAN
        BYTE	rssi : 4;
        BYTE	filler : 4;
#else
        BYTE	filler : 4;
        BYTE	rssi : 4;
#endif
    } POWER;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	weighthigh;
		BYTE	weightlow;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} WEIGHT;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	id4;
		BYTE	id5;
		BYTE	contract_type;
		BYTE	counter1_0;
		BYTE	counter1_1;
		BYTE	counter1_2;
		BYTE	counter1_3;
		BYTE	counter2_0;
		BYTE	counter2_1;
		BYTE	counter2_2;
		BYTE	counter2_3;
		BYTE	power_H;
		BYTE	power_L;
		BYTE	state;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} TIC;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	id4;
		BYTE	counter1_0;
		BYTE	counter1_1;
		BYTE	counter1_2;
		BYTE	counter1_3;
		BYTE	counter2_0;
		BYTE	counter2_1;
		BYTE	counter2_2;
		BYTE	counter2_3;
		BYTE	state;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} CEENCODER;

	struct {
		BYTE packetlength;
		BYTE packettype;
		BYTE subtype;
		BYTE seqnbr;
		BYTE id1;
		BYTE id2;
		BYTE id3;
		BYTE id4;
		BYTE runidx_0;
		BYTE runidx_1;
		BYTE runidx_2;
		BYTE runidx_3;
		BYTE prodidx1_0;
		BYTE prodidx1_1;
		BYTE prodidx1_2;
		BYTE prodidx1_3;
#ifdef IS_BIG_ENDIAN
		BYTE rfu : 4;
		BYTE currentidx : 4;
#else
		BYTE currentidx : 4;
		BYTE rfu : 4;
#endif
		BYTE av_voltage;
		BYTE power_H;
		BYTE power_L;
		BYTE state;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
#endif
	} LINKY;

	struct {
		BYTE packetlength;
		BYTE packettype;
		BYTE subtype;
		BYTE seqnbr;
		BYTE cmnd;
		BYTE baudrate;
		BYTE parity;
		BYTE databits;
		BYTE stopbits;
		BYTE polarity;
		BYTE filler1;
		BYTE filler2;
	} ASYNCPORT;

	struct {
		BYTE packetlength;
		BYTE packettype;
		BYTE subtype;
		BYTE seqnbr;
		BYTE datachar[252];
	} ASYNCDATA;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id;
		BYTE	msg1;
		BYTE	msg2;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} RFXSENSOR;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	count1;
		BYTE	count2;
		BYTE	count3;
		BYTE	count4;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	filler : 4;
#else
		BYTE	filler : 4;
		BYTE	rssi : 4;
#endif
	} RFXMETER;

	struct {
	BYTE	packetlength;
	BYTE	packettype;
	BYTE	subtype;
	BYTE	seqnbr;
	BYTE	hc1;
	BYTE	hc2;
	BYTE	addr;
	BYTE	cmd1;
	BYTE	cmd2;
#ifdef IS_BIG_ENDIAN
	BYTE	rssi : 4;
	BYTE	filler : 4;
#else
	BYTE	filler : 4;
	BYTE	rssi : 4;
#endif
    } FS20;

	struct {
		BYTE packetlength;
		BYTE packettype;
		BYTE subtype;
		BYTE seqnbr;
		BYTE id1;
		BYTE id2;
		BYTE directionhigh;
		BYTE directionlow;
		BYTE av_speedhigh;
		BYTE av_speedlow;
		BYTE gusthigh;
		BYTE gustlow;
#ifdef IS_BIG_ENDIAN
		BYTE	temperaturesign : 1;
		BYTE	temperaturehigh : 7;
#else
		BYTE	temperaturehigh : 7;
		BYTE	temperaturesign : 1;
#endif
		BYTE temperaturelow;
#ifdef IS_BIG_ENDIAN
		BYTE	chillsign : 1;
		BYTE	chillhigh : 7;
#else
		BYTE	chillhigh : 7;
		BYTE	chillsign : 1;
#endif
		BYTE chilllow;
		BYTE humidity;
		BYTE humidity_status;
		BYTE rainratehigh;
		BYTE rainratelow;
		BYTE raintotal1; //high byte
		BYTE raintotal2;
		BYTE raintotal3; //low byte
		BYTE uv;
		BYTE solarhigh;
		BYTE solarlow;
		BYTE barohigh;
		BYTE barolow;
		BYTE forecast;
		BYTE rfu1;
		BYTE rfu2;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE battery_level : 4;
		BYTE rssi : 4;
#endif
	} WEATHER;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	solarhigh;
		BYTE	solarlow;
		BYTE	rfu1;
		BYTE	rfu2;
#ifdef IS_BIG_ENDIAN
		BYTE	rssi : 4;
		BYTE	battery_level : 4;
#else
		BYTE battery_level : 4;
		BYTE rssi : 4;
#endif
	} SOLAR;

	struct {
	BYTE	packetlength;
	BYTE	packettype;
	BYTE	subtype;
	BYTE	seqnbr;
	BYTE	repeat;
	struct{
		BYTE	uint_msb;
		BYTE	uint_lsb;
	} pulse[124];
    } RAW;
} RBUF;

#endif //_RXFCOMLIB_F11DD459_E67E_4B26_8E44_B964E99304BF
