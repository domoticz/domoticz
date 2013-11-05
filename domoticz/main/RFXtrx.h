#pragma once

#ifndef _RXFCOMLIB_F11DD459_E67E_4B26_8E44_B964E99304BF
#define _RXFCOMLIB_F11DD459_E67E_4B26_8E44_B964E99304BF
//----------------------------------------------------------------------------
//                     Software License Agreement                      
//                                                                     
// Copyright 2011-2013, RFXCOM
//
// ALL RIGHTS RESERVED. This code is owned by RFXCOM, and is protected under
// Netherlands Copyright Laws and Treaties and shall be subject to the 
// exclusive jurisdiction of the Netherlands Courts. The information from this
// file may freely be used to create programs to exclusively interface with
// RFXCOM products only. Any other use or unauthorized reprint of this material
// is prohibited. No part of this file may be reproduced or transmitted in
// any form or by any means, electronic or mechanical, including photocopying,
// recording, or by any information storage and retrieval system without
// express written permission from RFXCOM.
//
// The above copyright notice shall be included in all copies or substantial
// portions of this Software.
//-----------------------------------------------------------------------------

/*
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

#define cmd310		0x50 // select 310MHz in the 310/315 transceiver
#define cmd315		0x51 // select 315MHz in the 310/315 transceiver
#define cmd800		0x55 // select 868.00MHz ASK in the 868 transceiver
#define cmd800F		0x56 // select 868.00MHz FSK in the 868 transceiver
#define cmd830		0x57 // select 868.30MHz ASK in the 868 transceiver
#define cmd830F		0x58 // select 868.30MHz FSK in the 868 transceiver
#define cmd835		0x59 // select 868.35MHz ASK in the 868 transceiver
#define cmd835F		0x5A // select 868.35MHz FSK in the 868 transceiver
#define cmd895		0x5B // select 868.95MHz in the 868 transceiver

#define pTypeInterfaceMessage 0x01
#define sTypeInterfaceResponse 0x00
#define sTypeInterfaceWrongCommand 0xFF
#define recType310 0x50
#define recType315 0x51
#define recType43392 0x52
#define trxType43392 0x53
#define recType86800 0x55
#define recType86800FSK 0x56
#define recType86830 0x57
#define recType86830FSK 0x58
#define recType86835 0x59
#define recType86835FSK 0x5A
#define recType86895 0x5B

#define msg3_AE 0x01
#define msg3_RUBICSON 0x02
#define msg3_FINEOFFSET 0x04
#define msg3_LIGHTING4 0x08
#define msg3_RSL 0x10
#define msg3_SX 0x20
#define msg3_RFU6 0x40
#define msg3_undec 0x80

#define msg4_MERTIK 0x01
#define msg4_AD 0x02
#define msg4_HID 0x04
#define msg4_LCROS 0x08
#define msg4_FS20 0x10
#define msg4_PROGUARD 0x20
#define msg4_BLINDST0 0x40
#define msg4_BLINDST1 0x80

#define msg5_X10 0x01
#define msg5_ARC 0x02
#define msg5_AC 0x04
#define msg5_HEU 0x08
#define msg5_MEI 0x10
#define msg5_OREGON 0x20
#define msg5_ATI 0x40
#define msg5_VISONIC 0x80

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

#define light1_sOff 0x0
#define light1_sOn 0x1
#define light1_sDim 0x2
#define light1_sBright 0x3
#define light1_sAllOff 0x5
#define light1_sAllOn 0x6
#define light1_sChime 0x7

#define pTypeLighting2 0x11
#define sTypeAC 0x0
#define sTypeHEU 0x1
#define sTypeANSLUT 0x2
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
#define light5_sLivoloAllOff 0x00
#define light5_sLivoloGang1Toggle 0x01
#define light5_sLivoloGang2Toggle 0x02	//dim+ for dimmer
#define light5_sLivoloGang3Toggle 0x03	//dim- for dimmer

#define pTypeLighting6 0x15
#define sTypeBlyss 0x0
#define light6_sOn 0x0
#define light6_sOff 0x1
#define light6_sGroupOn 0x2
#define light6_sGroupOff 0x3

#define pTypeChime 0x16
#define sTypeByronSX 0x0
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
#define fan_sTimer 0x1
#define fan_sMin 0x2
#define fan_sLearn 0x3
#define fan_sPlus 0x4
#define fan_sConfirm 0x5
#define fan_sLight 0x6

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
#define thermostat3_sOff 0x0
#define thermostat3_sOn 0x1
#define thermostat3_sUp 0x2
#define thermostat3_sDown 0x3
#define thermostat3_sRunUp 0x4
#define thermostat3_Off2nd 0x4
#define thermostat3_sRunDown 0x5
#define thermostat3_On2nd 0x5
#define thermostat3_sStop 0x6

//types for BBQ temperature
#define pTypeBBQ 0x4E
#define sTypeBBQ1 0x1  //Maverick ET-732

//types for temperature+rain
#define pTypeTEMP_RAIN 0x4F
#define sTypeTR1 0x1  //WS1200

//types for temperature
#define pTypeTEMP 0x50
#define sTypeTEMP1 0x1  //THR128/138,THC138
#define sTypeTEMP2 0x2  //THC238/268,THN132,THWR288,THRN122,THN122,AW129/131
#define sTypeTEMP3 0x3  //THWR800
#define sTypeTEMP4 0x4	//RTHN318
#define sTypeTEMP5 0x5  //LaCrosse TX3
#define sTypeTEMP6 0x6  //TS15C
#define sTypeTEMP7 0x7  //Viking 02811
#define sTypeTEMP8 0x8  //LaCrosse WS2300
#define sTypeTEMP9 0x9  //RUBiCSON
#define sTypeTEMP10 0xA  //TFA 30.3133

//types for humidity
#define pTypeHUM 0x51
#define sTypeHUM1 0x1  //LaCrosse TX3
#define sTypeHUM2 0x2  //LaCrosse WS2300

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
#define sTypeTH9 0x9  //Viking 02035,02038 (02035 has no humidity)
#define sTypeTH10 0xA   //Rubicson

//types for barometric
#define pTypeBARO 0x53

//types for temperature+humidity+baro
#define pTypeTEMP_HUM_BARO 0x54
#define sTypeTHB1 0x1   //BTHR918
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

//types for wind
#define pTypeWIND 0x56
#define sTypeWIND1 0x1   //WTGR800
#define sTypeWIND2 0x2   //WGR800
#define sTypeWIND3 0x3   //STR918,WGR918
#define sTypeWIND4 0x4   //TFA
#define sTypeWIND5 0x5   //UPM
#define sTypeWIND6 0x6   //WS2300

//types for uv
#define pTypeUV 0x57
#define sTypeUV1 0x1   //UVN128,UV138
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

//types for weight scales
#define pTypeWEIGHT 0x5D
#define sTypeWEIGHT1 0x1   //BWR102
#define sTypeWEIGHT2 0x2   //GR101

//types for gas
#define pTypeGAS 0x5E

//types for water
#define pTypeWATER 0x5F

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

typedef union tRBUF {
	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	cmnd;
		BYTE	msg1;
		BYTE	msg2;
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
		//BYTE	msg3;
		BYTE	AEenabled : 1;
		BYTE	RUBICSONenabled : 1;
		BYTE	FINEOFFSETenabled : 1;
		BYTE	LIGHTING4enabled : 1;
		BYTE	RSLenabled : 1;
		BYTE	SXenabled : 1;
		BYTE	RFU6 : 1;
		BYTE	UNDECODEDenabled : 1;
		//BYTE	msg4;
		BYTE	MERTIKenabled : 1;
		BYTE	LWRFenabled : 1;
		BYTE	HIDEKIenabled : 1;
		BYTE	LACROSSEenabled : 1;
		BYTE	FS20enabled : 1;
		BYTE	PROGUARDenabled : 1;
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
		BYTE	msg6;
		BYTE	msg7;
		BYTE	msg8;
		BYTE	msg9;
	} IRESPONSE;

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
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	rfu;
		BYTE	filler : 4;
		BYTE	rssi : 4;
	} LIGHTING6;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	sound;
		BYTE	filler  : 4;
		BYTE	rssi : 4;
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
		BYTE	filler  : 4;
		BYTE	rssi : 4;
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
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	filler : 4;
		BYTE	rssi : 4;
	} BLINDS1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		BYTE	status;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} SECURITY1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	housecode;
		BYTE	cmnd;
		BYTE	filler : 4;
		BYTE	rssi : 4;
	} CAMERA1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id;
		BYTE	cmnd;
		BYTE	toggle : 1;
		BYTE	cmndtype : 3;
		BYTE	rssi : 4;
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
		BYTE	status : 2;
		BYTE	filler : 5;
		BYTE	mode : 1;
		BYTE	filler1 : 4;
		BYTE	rssi : 4;
	} THERMOSTAT1;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	unitcode;
		BYTE	cmnd;
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	filler : 4;
		BYTE	rssi : 4;
	} THERMOSTAT3;

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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} BBQ;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
		BYTE	temperaturel;
		BYTE	raintotal1;
		BYTE	raintotal2;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} TEMP_RAIN;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
		BYTE	temperaturel;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} HUM;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
		BYTE	temperaturel;
		BYTE	humidity; 
		BYTE	humidity_status;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} BARO;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
		BYTE	temperaturel;
		BYTE	humidity; 
		BYTE	humidity_status;
		BYTE	baroh;
		BYTE	barol;
		BYTE	forecast;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
		BYTE	temperaturel;
		BYTE	chillh : 7;
		BYTE	chillsign : 1;
		BYTE	chilll;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} WIND;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	uv;
		BYTE	temperatureh : 7;
		BYTE	tempsign : 1;
		BYTE	temperaturel;
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
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
		BYTE	battery_level : 4;
		BYTE	rssi : 4;
	} CURRENT_ENERGY;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id1;
		BYTE	id2;
		BYTE	weighthigh;
		BYTE	weightlow;
		BYTE	filler : 4;
		BYTE	rssi : 4;
	} WEIGHT;

	struct {
		BYTE	packetlength;
		BYTE	packettype;
		BYTE	subtype;
		BYTE	seqnbr;
		BYTE	id;
		BYTE	msg1;
		BYTE	msg2;
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
		BYTE	filler : 4;
		BYTE	rssi : 4;
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
	BYTE	filler : 4;
	BYTE	rssi : 4;
    } FS20;
} RBUF;

#endif //_RXFCOMLIB_F11DD459_E67E_4B26_8E44_B964E99304BF
