//#pragma once
#include <stdint.h>
#include "globals.h"
/*#ifdef	__cplusplus
extern "C" {
#endif*/


/*
 * eHouse PRO, LAN, RS-485, CAN/RF, AURA system controllers data structures | Intellectual property of iSys.Pl
 * Author: Robert Jarzabek, iSys.Pl
 * Mod: 2017-06-04
 * http://www.isys.pl/          Inteligentny Dom eHouse - strona producenta
 * http://www.eHouse.pro/       Automatyka Budynku
 * http://sterowanie.biz/       Sterowanie Domem, Budynkiem ...
 * http://inteligentny-dom.ehouse.pro/  Inteligentny Dom eHouse Samodzielny montaż, zrób to sam, przykłady, projekty, programowanie
 * http://home-automation.isys.pl/      eHouse Home automation producer home page
 * http://home-automation.ehouse.pro/   eHouse Home Automation - Do it yourself, programming examples, designing

 * eHouse RS-485 structures (RM,HM,EM) for direct reading status from controllers via RS-485/RS-232/USB+dongle or via UDP (under CommManager Supervision)
 * Ethernet eHouse structures (ERM,EPM,EHM,CM,LM) for direct reading status from ethernet controllers via UDP broadcast
 * eHouse WiFi controllers for direct reading status from WiFi via UDP broadcast
 * Aura (Thermometers)
 * eHouse4Can/RF structures

 */
 /*#define INTERFACE_NATIVE 		0
 #define INTERFACE_COMMANAGER            1
 #define INTERFACE_SERVER 		2
 #define INTERFACE_CAN			3
 #define INTERFACE_RS485			4
 #define INTERFACE_ETHERNET		5*/
#ifndef EHSTATUS_H

 //#define	EHSTATUS_H
 //adc results stored in separate bytes
typedef struct
{
	//    zamieniona struktura bo chrzani adc

	unsigned char MSB;
	unsigned char LSB;
	//test inverted	unsigned char LSB;
} ADCs;
//////////////////////////////////////////////////////////        
typedef struct //passing arguments to threads
{
	int No;
	unsigned char StrParam[255];
	int IntParam;
} thdata;

//////////////////////////////////////////////////////////////////////
/*ehouse 1 system controllers - RM status
//frame received + offset

Offset depends of controllers mode operation

for extended address enabled offset = 3
for extended address enabled via commmanager offset 3
//frame body= <size><to addrh><to adrl>[ehouse1status struct instance see bellow]

for normal address via rs232 offset is 1
//frame body= <size>[ehouse1status struct instance see bellow]
*/




/////////////////////////////////////////////////////////
typedef struct RMeHouse1StatusT //struct offset =0;
{
	unsigned char AddrH;		//address high byte of current controller
	unsigned char AddrL;		//address low  byte 
//	unsigned char DataType;		//must be 's' for status	
	ADCs ADC[8];				//offset 3B size 8*2B		
	struct _tOUT			//18		//1 bit fields	3B size 1-24 outputs
								//offset in struct 19
	{				        //in some operating systems bit locations may be inverted in BYTES as  explained below
		unsigned char o1 : 1;			//digital output 8 [0|1]
		unsigned char o2 : 1;			//digital output 7 [0|1]
		unsigned char o3 : 1;			//digital output 6 [0|1]
		unsigned char o4 : 1;			//      |
		unsigned char o5 : 1;			//		_
		unsigned char o6 : 1;			//		_
		unsigned char o7 : 1;			//		|
		unsigned char o8 : 1;			//digital output 1  [0|1]
		unsigned char o9 : 1;		//digital output 16 [0|1]
		unsigned char o10 : 1;
		unsigned char o11 : 1;
		unsigned char o12 : 1;
		unsigned char o13 : 1;
		unsigned char o14 : 1;
		unsigned char o15 : 1;
		unsigned char o16 : 1;			//digital output 9  [0|1]
		unsigned char o17 : 1;		//digital output 24 [0|1]
		unsigned char o18 : 1;
		unsigned char o19 : 1;
		unsigned char o20 : 1;
		unsigned char o21 : 1;
		unsigned char o22 : 1;
		unsigned char o23 : 1;
		unsigned char o24 : 1;		//digital output 17 [0|1]
	} OUT;


	struct _tINPUTS		///20		//INPUTS 1-16: offset 22 - size 2 bytes
							//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES as explained below
		unsigned char i1 : 1;			//digital input 8 [0|1]
		unsigned char i2 : 1;			//digital input 7 [0|1]
		unsigned char i3 : 1;			//digital input 6 [0|1]
		unsigned char i4 : 1;
		unsigned char i5 : 1;
		unsigned char i6 : 1;
		unsigned char i7 : 1;
		unsigned char i8 : 1;
		unsigned char i9 : 1;                     //digital input 16 [0|1]
		unsigned char i10 : 1;
		unsigned char i11 : 1;
		unsigned char i12 : 1;
		unsigned char i13 : 1;
		unsigned char i14 : 1;
		unsigned char i15 : 1;
		unsigned char i16 : 1;			//digital input 9 [0|1]
	} INPUTS;


	struct _tOUT25		//22	//1 bit fields	1B size 25-32 outputs
						//offset in struct 24B
	{ //in some operating systems bit locations may be inverted in BYTES as explained below
		unsigned char o25 : 1;    //output 32
		unsigned char o26 : 1;
		unsigned char o27 : 1;
		unsigned char o28 : 1;
		unsigned char o29 : 1;
		unsigned char o30 : 1;
		unsigned char o31 : 1;
		unsigned char o32 : 1;    //output 25
	} OUT25;

	unsigned char DIMM[3];	//23////Dimmers status 1-3 size 1B per channel, offset in struct 26B
	char not_used;
	char CURRENT_PROGRAM;	//26	//RoomManager Program Nr.	offset 29B
	char not_useda;
	char not_usedb;
	char not_usedc;
	char not_usedd;
	char not_usede;
	char not_usedf;
	char not_usedg;
	char not_usedh;
	char not_usedi;
	char not_usedj;
	char not_usedk;
	char not_usedl;
	char not_usedm;
	char not_usedn;
	char not_usedo;
	char not_usedp;
	char not_usedr;
	char not_useds;
	char not_usedt;
	char not_usedu;
	char not_usedw;
	char not_usedx;
	char not_usedy;
	char not_usedz;
	char not_usedas;
	char not_usedcz;
	char not_usedds;
	char not_usedgt;
	char not_usedjg;
	unsigned char iface;
	//char ADC_PROGRAM;
} RMStatus;
/////////////////////////////////////////////////////////////////////////////
typedef struct  	//status hm status[0]='s'			//struct offset =0;
{       //ofset +1 comparing to controller status
	unsigned char AddrH;	//0	//address high byte of current controller
	unsigned char AddrL;	//1	//address low  byte 
//	unsigned char DataType;		//must be 's' for status
							//offset 3B size 8*2B
	ADCs ADC[16];		//2		//offset 3B size 16*2B

	struct _tOUT 			//2+32				//1 bit fields	3B size 1-24 outputs
									//offset in struct 35
	{ //in some operating systems bit locations may be inverted in BYTES as explained below
		unsigned char o1 : 1;			//digital output 8 [0|1]
		unsigned char o2 : 1;			//digital output 7 [0|1]
		unsigned char o3 : 1;			//digital output 6 [0|1]
		unsigned char o4 : 1;			//      |
		unsigned char o5 : 1;			//		_
		unsigned char o6 : 1;			//		_
		unsigned char o7 : 1;			//		|
		unsigned char o8 : 1;			//digital output 1  [0|1]
		unsigned char o9 : 1;		//digital output 16 [0|1]
		unsigned char o10 : 1;
		unsigned char o11 : 1;
		unsigned char o12 : 1;
		unsigned char o13 : 1;
		unsigned char o14 : 1;
		unsigned char o15 : 1;
		unsigned char o16 : 1;		//digital output 9  [0|1]
		unsigned char o17 : 1;		//digital output 24 [0|1]
		unsigned char o18 : 1;
		unsigned char o19 : 1;
		unsigned char o20 : 1;
		unsigned char o21 : 1;
		unsigned char o22 : 1;
		unsigned char o23 : 1;
		unsigned char o24 : 1;		//digital output 17 [0|1]
	} OUT;
	unsigned char CURRENT_PROGRAM;		//34+3 [36] //HeatManager Program Nr. offset 40                
	struct _tINPUTS 			//		[37,38]	//INPUTS 1-16: offset 38 - size 2 bytes
								//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES as explained below
		unsigned char i1 : 1;			//digital input 8 [0|1]
		unsigned char i2 : 1;			//digital input 7 [0|1]
		unsigned char i3 : 1;			//digital input 6 [0|1]
		unsigned char i4 : 1;
		unsigned char i5 : 1;
		unsigned char i6 : 1;
		unsigned char i7 : 1;
		unsigned char i8 : 1;
		unsigned char i9 : 1;		//digital input 16 [0|1]
		unsigned char i10 : 1;
		unsigned char i11 : 1;
		unsigned char i12 : 1;
		unsigned char i13 : 1;
		unsigned char i14 : 1;
		unsigned char i15 : 1;
		unsigned char i16 : 1;			//digital input 9 [0|1]
	} INPUTS;
	//WORD INPUTS;

	unsigned char DIMM[3];			//[39,40,41]//Dimmers status 1-3 size 1B per channel, offset in struct 41B
	unsigned char not_usedA;	//[42]
	unsigned char not_usedB;	//[43]
	unsigned char not_usedC;	//[44]



	unsigned char BOILER_SUPPLY_OVERRIDE : 1;	//0 bit+
	unsigned char HM_NOT_USED1 : 1;	//1 bit
	unsigned char HM_NOT_USED2 : 1;	//2 bit
	unsigned char HM_NOT_USED3 : 1;	//3 bit
	unsigned char HM_NOT_USED4 : 1;	//4 bit
	unsigned char HM_NOT_USED5 : 1;	//5 bit
	unsigned char HM_NOT_USED6 : 1;	//6 bit
	unsigned char HM_NOT_USED7 : 1;	//7 bit
	//byte 48     //a [46]
	unsigned char BONFIRE_STATE : 3;			//0-2 bits+
	unsigned char BONFIRE_SENSOR_ERROR : 1;   //3 bit ???
	unsigned char BOILER_FUEL_OUT : 1;		//4 bit+
	unsigned char BOILER_ALARM : 1;			//5 bit+
	unsigned char BOILER_POWER_ON : 1;		//6 bit+
	unsigned char BOILER_SUPPLY_EN : 1;		//7 bit+

	//49 byte [47]
	unsigned char BONFIRE_PUMP : 1; 	//0 bit+
	unsigned char BOILER_PUMP : 1;	//1 bit+
	unsigned char SOLAR_PUMP : 1;	//2 bit+
	unsigned char BOILER_ON : 1;	//3 bit+
	unsigned char HEATER_PUMP : 1;	//4 bit+
	unsigned char VENTILATION_ON : 1; //5 bit+
	unsigned char THREE_WAYS_CUT_OFF_INC : 1;	//6 bit+
	unsigned char THREE_WAYS_CUT_OFF_DEC : 1;	//7 bit+

	//RECu_Mode //50 index [48] ????
	unsigned char RECU_TEMP : 5;		//recu amalva temperature+
	unsigned char RECU_FULL_AUTO : 1;         //recu full auto mode pelna automatyka wentylacji
	unsigned char RECU_MANUAL : 1;		//manual - auto bit 6
	unsigned char RECU_WINTER : 1;		//winter/summer lato zima 7 bit

	//unsigned char WENT_MODE; //51 index [49] ???
	unsigned char WENT_RECU_SPEED : 2;
	unsigned char WENT_COOLING : 1;           //Cooling On/ off / 
	unsigned char WENT_UNCONDITIONAL_WENT : 1;           //Unconditional Ventilation / Bezwarnunkowe wlaczenie wentylacji
	unsigned char WENT_DGP : 1;           //DGP Fan on/off  / Wlaczenie wentylatora DGP
	unsigned char CENTRAL_HEATING_PUMP : 1;/////WENT_VENTYLATOR_GWC:1;		//auxiliary ground heat exchanger for gwc / Wlaczenie wentylatora wspomagajacego GWC
	unsigned char WENT_GWC : 1;		//ground heat exchanger for gwc on off/ Wybor Czerpni GWC lub Poludniwej
	unsigned char WENT_COOLER_PUMP : 1;		//cooler pump on for gwc / Wlaczenie chlodnicy w gwc Pompy
	unsigned char MORE_OPTIONS[14];
	//unsigned char RECU_MODE;		//recuperator mode offset 52
	//unsigned char WENT_MODE;		//ventilation mode offset 53
	//char ADC_PROGRAM;
	unsigned char iface;
} HMStatus;
////////////////////////////////////////////////////////////////////////
//offset 72-2 na adressy potem wstawic adresh i adresl
// after copy status frame overwrite Addresses h,l
typedef struct ECMStatusT 				//struct offset =72-2;
{
	unsigned char AddrH;		//address high byte of current controller
	unsigned char AddrL;		//address low  byte 
	ADCs ADC[16];				//offset in status 72

	struct 		//1 bit fields	20B size 1-160 outputs offset in status 72+32

	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char o1 : 1; unsigned char o2 : 1; unsigned char o3 : 1; unsigned char o4 : 1; unsigned char o5 : 1; unsigned char o6 : 1; unsigned char o7 : 1; unsigned char o8 : 1; unsigned char o9 : 1; unsigned char o10 : 1;
		unsigned char o11 : 1; unsigned char o12 : 1; unsigned char o13 : 1; unsigned char o14 : 1; unsigned char o15 : 1; unsigned char o16 : 1; unsigned char o17 : 1; unsigned char o18 : 1; unsigned char o19 : 1; unsigned char o20 : 1;
		unsigned char o21 : 1; unsigned char o22 : 1; unsigned char o23 : 1; unsigned char o24 : 1; unsigned char o25 : 1; unsigned char o26 : 1; unsigned char o27 : 1; unsigned char o28 : 1; unsigned char o29 : 1; unsigned char o30 : 1;
		unsigned char o31 : 1; unsigned char o32 : 1; unsigned char o33 : 1; unsigned char o34 : 1; unsigned char o35 : 1; unsigned char o36 : 1; unsigned char o37 : 1; unsigned char o38 : 1; unsigned char o39 : 1; unsigned char o40 : 1;
		unsigned char o41 : 1; unsigned char o42 : 1; unsigned char o43 : 1; unsigned char o44 : 1; unsigned char o45 : 1; unsigned char o46 : 1; unsigned char o47 : 1; unsigned char o48 : 1; unsigned char o49 : 1; unsigned char o50 : 1;
		unsigned char o51 : 1; unsigned char o52 : 1; unsigned char o53 : 1; unsigned char o54 : 1; unsigned char o55 : 1; unsigned char o56 : 1; unsigned char o57 : 1; unsigned char o58 : 1; unsigned char o59 : 1; unsigned char o60 : 1;
		unsigned char o61 : 1; unsigned char o62 : 1; unsigned char o63 : 1; unsigned char o64 : 1; unsigned char o65 : 1; unsigned char o66 : 1; unsigned char o67 : 1; unsigned char o68 : 1; unsigned char o69 : 1; unsigned char o70 : 1;
		unsigned char o71 : 1; unsigned char o72 : 1; unsigned char o73 : 1; unsigned char o74 : 1; unsigned char o75 : 1; unsigned char o76 : 1; unsigned char o77 : 1; unsigned char o78 : 1; unsigned char o79 : 1; unsigned char o80 : 1;
		unsigned char o81 : 1; unsigned char o82 : 1; unsigned char o83 : 1; unsigned char o84 : 1; unsigned char o85 : 1; unsigned char o86 : 1; unsigned char o87 : 1; unsigned char o88 : 1; unsigned char o89 : 1; unsigned char o90 : 1;
		unsigned char o91 : 1; unsigned char o92 : 1; unsigned char o93 : 1; unsigned char o94 : 1; unsigned char o95 : 1; unsigned char o96 : 1; unsigned char o97 : 1; unsigned char o98 : 1; unsigned char o99 : 1; unsigned char o100 : 1;
		unsigned char o101 : 1; unsigned char o102 : 1; unsigned char o103 : 1; unsigned char o104 : 1; unsigned char o105 : 1; unsigned char o106 : 1; unsigned char o107 : 1; unsigned char o108 : 1; unsigned char o109 : 1; unsigned char o110 : 1;
		unsigned char o111 : 1; unsigned char o112 : 1; unsigned char o113 : 1; unsigned char o114 : 1; unsigned char o115 : 1; unsigned char o116 : 1; unsigned char o117 : 1; unsigned char o118 : 1; unsigned char o119 : 1; unsigned char o120 : 1;
		unsigned char o121 : 1; unsigned char o122 : 1; unsigned char o123 : 1; unsigned char o124 : 1; unsigned char o125 : 1; unsigned char o126 : 1; unsigned char o127 : 1; unsigned char o128 : 1; unsigned char o129 : 1; unsigned char o130 : 1;
		unsigned char o131 : 1; unsigned char o132 : 1; unsigned char o133 : 1; unsigned char o134 : 1; unsigned char o135 : 1; unsigned char o136 : 1; unsigned char o137 : 1; unsigned char o138 : 1; unsigned char o139 : 1; unsigned char o140 : 1;
		unsigned char o141 : 1; unsigned char o142 : 1; unsigned char o143 : 1; unsigned char o144 : 1; unsigned char o145 : 1; unsigned char o146 : 1; unsigned char o147 : 1; unsigned char o148 : 1; unsigned char o149 : 1; unsigned char o150 : 1;
		unsigned char o151 : 1; unsigned char o152 : 1; unsigned char o153 : 1; unsigned char o154 : 1; unsigned char o155 : 1; unsigned char o156 : 1; unsigned char o157 : 1; unsigned char o158 : 1; unsigned char o159 : 1; unsigned char o160 : 1;

	} OUTExt;

	struct //INPUTS 1-96: offset in status 72+20 - size 12 bytes
		//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1;
	} INPUTSExt;

	struct 	 //ALARMS 1-96: offset - size 12 bytes offset in status 72+20+12
		 //1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1;
	} Alarms;

	struct 	//ALARMS 1-96: offset  - size 12 bytes offset in status 72+20+12+12
		//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1;
	} Warnings;

	struct 	//Monitoring 1-96: offset - size 12 bytes offset in status 72+20+12+12+12
			//1 bit fields
	{ //in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1;
	} Monitorings;




	unsigned char CURRENT_PROGRAM;		//Current Program Nr.
	unsigned char CURRENT_ZONE;			//Current Security Zone Nr.
	unsigned char CURRENT_ADC_PROGRAM;	//Current ADC program
	unsigned char DIMM[3];				//Dimmers status 1-3 size 1B per channel, offset in struct 
	unsigned char iface;
} ECMStatus;
////////////////////////////////////////////////////////////////////////
typedef struct  ECMStatusBT				//struct offset =72-2;
{
	unsigned char AddrH;				//address high byte of current controller
	unsigned char AddrL;				//address low  byte 
	ADCs ADC[16];						//offset in status 72
	unsigned char outs[20];         //1 bit fields	20B size 1-160 outputs offset in status 72+32
	unsigned char inputs[12];       //	 INPUTSExt;
	unsigned char Alarms[12];
	unsigned char Warnings[12];
	unsigned char Monitorings[12];
	unsigned char CURRENT_PROGRAM;		//Current Program Nr.
	unsigned char CURRENT_ZONE;			//Current Security Zone Nr.
	unsigned char CURRENT_ADC_PROGRAM;	//Current ADC program
	unsigned char DIMM[3];				//Dimmers status 1-3 size 1B per channel, offset in struct 
	unsigned char iface;
} ECMStatusB;


////////////////////////////////////////////////////////////////////////
typedef struct ERMFullStatusT  				//struct offset =72-2;
{
	unsigned char Size;
	unsigned char AddrH;		//address high byte of current controller
	unsigned char AddrL;		//address low  byte 
	unsigned char Code;     //status code = 's'
	unsigned char Dimmers[3];
	unsigned char DMXDimmers[17 + 15];
	unsigned char DaliDimmers[46];

	int ADC[16];
	int ADCH[16];
	int ADCL[16];

	unsigned char More[8];
	int Temp[16];
	int TempL[16];
	int TempH[16];
	unsigned char Unit[16];
	unsigned char Type;
	unsigned char Outs[5];
	unsigned char Inputs[6];



	unsigned char CURRENT_PROGRAM;		//Current Program Nr.
	unsigned char CURRENT_ZONE;		//Current Security Zone Nr.
	unsigned char CURRENT_ADC_PROGRAM;	//Current ADC program
	unsigned char CURRENT_PROFILE;		//Current Program Nr.

} ERMFullStatus;

////////////////////////////////////////////////////////////////////////
typedef struct WIFIFullStatusT 				//struct offset =72-2;
{
	unsigned char Size;
	unsigned char AddrH;		//address high byte of current controller
	unsigned char AddrL;		//address low  byte 
	unsigned char Code;             //status code = 's'
	unsigned char Dimmers[4];
	int ADC[4];
	int ADCH[4];
	int ADCL[4];
	int Temp[4];
	int TempL[4];
	int TempH[4];
	unsigned char Unit[4];
	unsigned char Type;
	unsigned char Outs[2];
	unsigned char Inputs[2];



	unsigned char CURRENT_PROGRAM;		//Current Program Nr.
	unsigned char CURRENT_ZONE;		//Current Security Zone Nr.
	unsigned char CURRENT_ADC_PROGRAM;	//Current ADC program
	unsigned char CURRENT_PROFILE;		//Current Program Nr.
	unsigned char RSSI;
	unsigned int since;

} WIFIFullStatus;



//////////////////////////////////////////////////////////
// eHouse RS-485 status union
typedef union eHouse1StatusT	//extended address mode status for RM/ EM without input extenders
{
	RMStatus RM;
	//ERMStatus ERM;
	HMStatus HM;

	unsigned char data[200];
} eHouse1Status;
////////////////////////////////////////////////////////////////////////////////

typedef union CMStatusT	//CommManager Status
{
	ECMStatus CM;
	ECMStatus RM;
	ECMStatusB CMB;
	unsigned char data[200];//sizeof(ECMStatus)];
} CMStatus;

typedef union ERMFullStatT
{
	ERMFullStatus            eHERM;
	unsigned char data[sizeof(ERMFullStatus)];
} ERMFullStat;

typedef union WIFIFullStatT
{
	WIFIFullStatus  eHWIFI;
	unsigned char   data[sizeof(WIFIFullStatus)];
} WIFIFullStat;

//////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	unsigned char MSB;
	unsigned char LSB;
} tCANAdcValue;
/////////////////////////////////////////////////////////////////////////////////
// CAN/RF status structure
typedef struct CANStatT
{
	unsigned char AddrH;					//from SID + EID address
	unsigned char AddrL;
	tCANAdcValue ADC[4];	//8 bytes       //8 bytes - 2 status CAN package 
	unsigned char DIMM[4];           //8 bytes - 3 status CAN package
		//in some operating systems bites locations may be swapped in Bytes as explain in comments
	struct
	{
		unsigned char o1 : 1;			//digital output 8 [0|1]
		unsigned char o2 : 1;			//digital output 7 [0|1]
		unsigned char o3 : 1;			//digital output 6 [0|1]
		unsigned char o4 : 1;			//      |
		unsigned char o5 : 1;			//		_
		unsigned char o6 : 1;			//		_
		unsigned char o7 : 1;			//		|
		unsigned char o8 : 1;			//digital output 1  [0|1]
	} OUTPUTS;

	struct
	{
		unsigned char i1 : 1;			//digital input 8 [0|1]
		unsigned char i2 : 1;			//digital input 7 [0|1]
		unsigned char i3 : 1;			//digital input 6 [0|1]
		unsigned char i4 : 1;			//      |
		unsigned char i5 : 1;			//		_
		unsigned char i6 : 1;			//		_
		unsigned char i7 : 1;			//		|
		unsigned char i8 : 1;			//digital input 1  [0|1]	
	} INPUTS;
	unsigned char OUT_PROGRAM : 5;
	unsigned char Mode : 3;
	unsigned char ADCPRG : 5;
	unsigned char ModeB : 3;
	unsigned char iface;
} CANStat;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
//
//   eHouse CAN/RF status Union
//
////////////////////////////////////////////////////////
typedef union	CANStatusT		//20B
{
	CANStat status;
	unsigned char data[sizeof(CANStat)];
} CANStatus;

/////////////////////////////////////////////////////////
// WiFi status structure
/////////////////////////////////////////////////////////
typedef struct WiFiStatT				/// 22B
{
	unsigned char AddrH;					//from SID + EID address
	unsigned char AddrL;
	unsigned char cmd;
	tCANAdcValue AdcVal[4];	//8 bytes       //8 bytes - 2 status CAN package 
	unsigned char Dimmer[4];           //8 bytes - 3 status CAN package
	//in some operating systems bites locations may be swapped in Bytes as explain in comments
	struct
	{
		unsigned char o1 : 1;			//digital output 8 [0|1]
		unsigned char o2 : 1;			//digital output 7 [0|1]
		unsigned char o3 : 1;			//digital output 6 [0|1]
		unsigned char o4 : 1;			//      |
		unsigned char o5 : 1;			//		_
		unsigned char o6 : 1;			//		_
		unsigned char o7 : 1;			//		|
		unsigned char o8 : 1;			//digital output 1  [0|1]
	} OUTPUTS;

	struct
	{
		unsigned char i1 : 1;			//digital input 8 [0|1]
		unsigned char i2 : 1;			//digital input 7 [0|1]
		unsigned char i3 : 1;			//digital input 6 [0|1]
		unsigned char i4 : 1;			//      |
		unsigned char i5 : 1;			//		_
		unsigned char i6 : 1;			//		_
		unsigned char i7 : 1;			//		|
		unsigned char i8 : 1;			//digital input 1  [0|1]	
	} INPUTS;
	unsigned char OUT_PROGRAM : 5;
	unsigned char Mode : 3;
	unsigned char ADCPRG : 5;
	unsigned char ModeB : 3;
	unsigned char iface;
	unsigned char RSSI;
	unsigned int since;
} WiFiStat;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
//
//  eHouse WiFi Controller union
//
////////////////////////////////////////////////////////
typedef union WiFiStatusT
{
	struct WiFiStatT status;
	unsigned char data[sizeof(struct WiFiStatT)];
} WiFiStatus;



////////////////////////////////////////////////////////////////////////////////        
//Event Queue structure for processing of events        
typedef struct EventQueueT
{
	unsigned char LocalEventsToRun[EVENT_SIZE];     //direct event command
	unsigned char LocalEventsDrop;                  //nr of retries until dropped
	unsigned int LocalEventsTimeOuts;              //internal timer  =RUN_NOW then execute if 0 then remove event from queue
	unsigned char iface;                        //interface to send RS-485,Ethernet (TCP/IP), CAN
	unsigned char FromIPH;
	unsigned char FromIPL;
} EventQueue;

///////////////////////////////////////////////////////////
typedef struct CtrlADCT
{
	signed int ADCLow;				//low threshold
	signed int ADCHigh;				//high threshold
	signed int ADCValue;			//measurement value
	signed int ADCValuesBK[8];	//previous values
	signed int ADCValuesAVPREV;	//previous average backup
	signed int ADCAverage;			//Average value
	//signed int ADCPrev[8];			//ADC results
	unsigned char RotateIndex;		//Rotate index
	signed int    CMTUCalibration;	//calibration of capacitance
	signed int    MeasuredCapacity;	//calibration of capacitance
	unsigned char admin : 1;          //admin flag
	unsigned char under : 1;		//below low threshold
	unsigned char over : 1;		//above high threshold
	unsigned char ok : 1;			//in ok  range
	unsigned char changed : 1;	//changed flag
	unsigned char AlarmH : 1;		//alarm will be issued bigger frequency of status
	unsigned char AlarmL : 1;		//alarm will be issued bigger frequency of status
	unsigned char AlarmStateH : 1;
	unsigned char AlarmStateL : 1;
	unsigned char inverted : 1;	//inverted scale
	unsigned char crossed : 1;	//level was crossed
	unsigned char DisableEvent : 1;   //disable threshold events execution
	unsigned char DisableVent : 1;    //Disable Ventilation if flag is set
	unsigned char DisableHeating : 1; //Disable Heating if flag is set
	unsigned char DisableRecu : 1;    //Disable Ventilation if flag is set
	unsigned char DisableCooling : 1; //Disable Heating if flag is set
	unsigned char DisableFan : 1;    //Disable Ventilation if flag is set
	unsigned char DisableLight : 1;
	unsigned int flagsu;
	unsigned long flagsa;
	unsigned long flagsb;
	unsigned char door;
	unsigned int AlarmDelay;	//delay before alarm initiate
	unsigned int AlarmDelayInit;    //delay before alarm initialization
	unsigned int MaxValue;
	unsigned int MinValue;
	unsigned int WindDirection;
	unsigned char ADCType;
	//unsigned int AlarmDelayCounter;
} CtrlADC;

////////////////////////////////////////////////////////////////////////////////
struct AdcValue
{
	unsigned char MSB;
	unsigned char LSB;
};
////////////////////////////////////////////////////////////////////////////////
/*
ehouse.Pro status format: (hex coded)
hh - address high
ll - address low
192.168.hh.ll

ff ff hh ll    --address part
aaaaaaaaaa-    256B (128*2B) ADC sensors 	 -aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

oooooooooo-    64B  (256*2B/8)	Outputs          -ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
iiiiiiiiii-    64B (256*2B/8) Inputs             -iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
AAAAAAAAAA-    64B (256*2B/8) Alarms  (Horn)     -AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
WWWWWWWWWW-    64B (256*2B/8) Warnings (Light)   -WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
MMMMMMMMMM-    64B (256*2B/8) Monitoring         -MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
SSSSSSSSSS-    64B (256*2B/8) Silent             -SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
LLLLLLLLLL-    64B (256*2B/8) Early Warning      -LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
GGGGGGGGGG-    64B (256*2B/8) SMS                -GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
DDDDDDDDDD-	   256B Dimmers                  -DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD

P -  Program        	2B
ADC - Program		2B
Roller - Program	2B
Secu Zone		2B
HWStates		2B

Additional Info 512B  for panels (commands) (Audio Playbeck Info, Video Playback Info)
*/
////////////////////////////////////////////////////////////////////////////////
#define HORN_LOC    1
#define WARN_LOC    2
#define EARLY_LOC   4
#define SMS_LOC     8
#define SILENT_LOC  0x10
#define ALARM_LOC   0x20
#define MONITOR_LOC 0x40

typedef struct EHOUSEPRO_BITS_TT
{ //EHOUSEPRO_BITS
	unsigned int MarkEHousePro;                 //mark for panel eHouse Pro status 0xffff
	unsigned char devadrh;                      //ip addr lower
	unsigned char devadrl;                      //ip address lowest
	int16_t AdcVal[MAX_ADCS];               //256 bytes   - by 2 byte status
	struct 		//1 bit fields	32B size 1-256

	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char o1 : 1; unsigned char o2 : 1; unsigned char o3 : 1; unsigned char o4 : 1; unsigned char o5 : 1; unsigned char o6 : 1; unsigned char o7 : 1; unsigned char o8 : 1; unsigned char o9 : 1; unsigned char o10 : 1;
		unsigned char o11 : 1; unsigned char o12 : 1; unsigned char o13 : 1; unsigned char o14 : 1; unsigned char o15 : 1; unsigned char o16 : 1; unsigned char o17 : 1; unsigned char o18 : 1; unsigned char o19 : 1; unsigned char o20 : 1;
		unsigned char o21 : 1; unsigned char o22 : 1; unsigned char o23 : 1; unsigned char o24 : 1; unsigned char o25 : 1; unsigned char o26 : 1; unsigned char o27 : 1; unsigned char o28 : 1; unsigned char o29 : 1; unsigned char o30 : 1;
		unsigned char o31 : 1; unsigned char o32 : 1; unsigned char o33 : 1; unsigned char o34 : 1; unsigned char o35 : 1; unsigned char o36 : 1; unsigned char o37 : 1; unsigned char o38 : 1; unsigned char o39 : 1; unsigned char o40 : 1;
		unsigned char o41 : 1; unsigned char o42 : 1; unsigned char o43 : 1; unsigned char o44 : 1; unsigned char o45 : 1; unsigned char o46 : 1; unsigned char o47 : 1; unsigned char o48 : 1; unsigned char o49 : 1; unsigned char o50 : 1;
		unsigned char o51 : 1; unsigned char o52 : 1; unsigned char o53 : 1; unsigned char o54 : 1; unsigned char o55 : 1; unsigned char o56 : 1; unsigned char o57 : 1; unsigned char o58 : 1; unsigned char o59 : 1; unsigned char o60 : 1;
		unsigned char o61 : 1; unsigned char o62 : 1; unsigned char o63 : 1; unsigned char o64 : 1; unsigned char o65 : 1; unsigned char o66 : 1; unsigned char o67 : 1; unsigned char o68 : 1; unsigned char o69 : 1; unsigned char o70 : 1;
		unsigned char o71 : 1; unsigned char o72 : 1; unsigned char o73 : 1; unsigned char o74 : 1; unsigned char o75 : 1; unsigned char o76 : 1; unsigned char o77 : 1; unsigned char o78 : 1; unsigned char o79 : 1; unsigned char o80 : 1;
		unsigned char o81 : 1; unsigned char o82 : 1; unsigned char o83 : 1; unsigned char o84 : 1; unsigned char o85 : 1; unsigned char o86 : 1; unsigned char o87 : 1; unsigned char o88 : 1; unsigned char o89 : 1; unsigned char o90 : 1;
		unsigned char o91 : 1; unsigned char o92 : 1; unsigned char o93 : 1; unsigned char o94 : 1; unsigned char o95 : 1; unsigned char o96 : 1; unsigned char o97 : 1; unsigned char o98 : 1; unsigned char o99 : 1; unsigned char o100 : 1;
		unsigned char o101 : 1; unsigned char o102 : 1; unsigned char o103 : 1; unsigned char o104 : 1; unsigned char o105 : 1; unsigned char o106 : 1; unsigned char o107 : 1; unsigned char o108 : 1; unsigned char o109 : 1; unsigned char o110 : 1;
		unsigned char o111 : 1; unsigned char o112 : 1; unsigned char o113 : 1; unsigned char o114 : 1; unsigned char o115 : 1; unsigned char o116 : 1; unsigned char o117 : 1; unsigned char o118 : 1; unsigned char o119 : 1; unsigned char o120 : 1;
		unsigned char o121 : 1; unsigned char o122 : 1; unsigned char o123 : 1; unsigned char o124 : 1; unsigned char o125 : 1; unsigned char o126 : 1; unsigned char o127 : 1; unsigned char o128 : 1; unsigned char o129 : 1; unsigned char o130 : 1;
		unsigned char o131 : 1; unsigned char o132 : 1; unsigned char o133 : 1; unsigned char o134 : 1; unsigned char o135 : 1; unsigned char o136 : 1; unsigned char o137 : 1; unsigned char o138 : 1; unsigned char o139 : 1; unsigned char o140 : 1;
		unsigned char o141 : 1; unsigned char o142 : 1; unsigned char o143 : 1; unsigned char o144 : 1; unsigned char o145 : 1; unsigned char o146 : 1; unsigned char o147 : 1; unsigned char o148 : 1; unsigned char o149 : 1; unsigned char o150 : 1;
		unsigned char o151 : 1; unsigned char o152 : 1; unsigned char o153 : 1; unsigned char o154 : 1; unsigned char o155 : 1; unsigned char o156 : 1; unsigned char o157 : 1; unsigned char o158 : 1; unsigned char o159 : 1; unsigned char o160 : 1;
		unsigned char o161 : 1; unsigned char o162 : 1; unsigned char o163 : 1; unsigned char o164 : 1; unsigned char o165 : 1; unsigned char o166 : 1; unsigned char o167 : 1; unsigned char o168 : 1; unsigned char o169 : 1; unsigned char o170 : 1;
		unsigned char o171 : 1; unsigned char o172 : 1; unsigned char o173 : 1; unsigned char o174 : 1; unsigned char o175 : 1; unsigned char o176 : 1; unsigned char o177 : 1; unsigned char o178 : 1; unsigned char o179 : 1; unsigned char o180 : 1;
		unsigned char o181 : 1; unsigned char o182 : 1; unsigned char o183 : 1; unsigned char o184 : 1; unsigned char o185 : 1; unsigned char o186 : 1; unsigned char o187 : 1; unsigned char o188 : 1; unsigned char o189 : 1; unsigned char o190 : 1;
		unsigned char o191 : 1; unsigned char o192 : 1; unsigned char o193 : 1; unsigned char o194 : 1; unsigned char o195 : 1; unsigned char o196 : 1; unsigned char o197 : 1; unsigned char o198 : 1; unsigned char o199 : 1; unsigned char o1100 : 1;
		unsigned char o201 : 1; unsigned char o202 : 1; unsigned char o203 : 1; unsigned char o204 : 1; unsigned char o205 : 1; unsigned char o206 : 1; unsigned char o207 : 1; unsigned char o208 : 1; unsigned char o209 : 1; unsigned char o210 : 1;
		unsigned char o211 : 1; unsigned char o212 : 1; unsigned char o213 : 1; unsigned char o214 : 1; unsigned char o215 : 1; unsigned char o216 : 1; unsigned char o217 : 1; unsigned char o218 : 1; unsigned char o219 : 1; unsigned char o220 : 1;
		unsigned char o221 : 1; unsigned char o222 : 1; unsigned char o223 : 1; unsigned char o224 : 1; unsigned char o225 : 1; unsigned char o226 : 1; unsigned char o227 : 1; unsigned char o228 : 1; unsigned char o229 : 1; unsigned char o230 : 1;
		unsigned char o231 : 1; unsigned char o232 : 1; unsigned char o233 : 1; unsigned char o234 : 1; unsigned char o235 : 1; unsigned char o236 : 1; unsigned char o237 : 1; unsigned char o238 : 1; unsigned char o239 : 1; unsigned char o240 : 1;
		unsigned char o241 : 1; unsigned char o242 : 1; unsigned char o243 : 1; unsigned char o244 : 1; unsigned char o245 : 1; unsigned char o246 : 1; unsigned char o247 : 1; unsigned char o248 : 1; unsigned char o249 : 1; unsigned char o250 : 1;
		unsigned char o251 : 1; unsigned char o252 : 1; unsigned char o253 : 1; unsigned char o254 : 1; unsigned char o255 : 1; unsigned char o256 : 1;



	} OUTPUTS;

	struct //INPUTS 1-256: size 32 bytes
		//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} INPUTS;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} HORN;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} WARNING;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} MONITORING;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} SILENT;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} EARLYWARNING;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} SMS;
	struct //INPUTS 1-256: size 32 bytes
			//1 bit fields
	{//in some operating systems bit locations may be inverted in BYTES
		unsigned char i1 : 1; unsigned char i2 : 1; unsigned char i3 : 1; unsigned char i4 : 1; unsigned char i5 : 1; unsigned char i6 : 1; unsigned char i7 : 1; unsigned char i8 : 1; unsigned char i9 : 1; unsigned char i10 : 1;
		unsigned char i11 : 1; unsigned char i12 : 1; unsigned char i13 : 1; unsigned char i14 : 1; unsigned char i15 : 1; unsigned char i16 : 1; unsigned char i17 : 1; unsigned char i18 : 1; unsigned char i19 : 1; unsigned char i20 : 1;
		unsigned char i21 : 1; unsigned char i22 : 1; unsigned char i23 : 1; unsigned char i24 : 1; unsigned char i25 : 1; unsigned char i26 : 1; unsigned char i27 : 1; unsigned char i28 : 1; unsigned char i29 : 1; unsigned char i30 : 1;
		unsigned char i31 : 1; unsigned char i32 : 1; unsigned char i33 : 1; unsigned char i34 : 1; unsigned char i35 : 1; unsigned char i36 : 1; unsigned char i37 : 1; unsigned char i38 : 1; unsigned char i39 : 1; unsigned char i40 : 1;
		unsigned char i41 : 1; unsigned char i42 : 1; unsigned char i43 : 1; unsigned char i44 : 1; unsigned char i45 : 1; unsigned char i46 : 1; unsigned char i47 : 1; unsigned char i48 : 1; unsigned char i49 : 1; unsigned char i50 : 1;
		unsigned char i51 : 1; unsigned char i52 : 1; unsigned char i53 : 1; unsigned char i54 : 1; unsigned char i55 : 1; unsigned char i56 : 1; unsigned char i57 : 1; unsigned char i58 : 1; unsigned char i59 : 1; unsigned char i60 : 1;
		unsigned char i61 : 1; unsigned char i62 : 1; unsigned char i63 : 1; unsigned char i64 : 1; unsigned char i65 : 1; unsigned char i66 : 1; unsigned char i67 : 1; unsigned char i68 : 1; unsigned char i69 : 1; unsigned char i70 : 1;
		unsigned char i71 : 1; unsigned char i72 : 1; unsigned char i73 : 1; unsigned char i74 : 1; unsigned char i75 : 1; unsigned char i76 : 1; unsigned char i77 : 1; unsigned char i78 : 1; unsigned char i79 : 1; unsigned char i80 : 1;
		unsigned char i81 : 1; unsigned char i82 : 1; unsigned char i83 : 1; unsigned char i84 : 1; unsigned char i85 : 1; unsigned char i86 : 1; unsigned char i87 : 1; unsigned char i88 : 1; unsigned char i89 : 1; unsigned char i90 : 1;
		unsigned char i91 : 1; unsigned char i92 : 1; unsigned char i93 : 1; unsigned char i94 : 1; unsigned char i95 : 1; unsigned char i96 : 1; unsigned char i97 : 1; unsigned char i98 : 1; unsigned char i99 : 1; unsigned char i100 : 1;
		unsigned char i101 : 1; unsigned char i102 : 1; unsigned char i103 : 1; unsigned char i104 : 1; unsigned char i105 : 1; unsigned char i106 : 1; unsigned char i107 : 1; unsigned char i108 : 1; unsigned char i109 : 1; unsigned char i110 : 1;
		unsigned char i111 : 1; unsigned char i112 : 1; unsigned char i113 : 1; unsigned char i114 : 1; unsigned char i115 : 1; unsigned char i116 : 1; unsigned char i117 : 1; unsigned char i118 : 1; unsigned char i119 : 1; unsigned char i120 : 1;
		unsigned char i121 : 1; unsigned char i122 : 1; unsigned char i123 : 1; unsigned char i124 : 1; unsigned char i125 : 1; unsigned char i126 : 1; unsigned char i127 : 1; unsigned char i128 : 1; unsigned char i129 : 1; unsigned char i130 : 1;
		unsigned char i131 : 1; unsigned char i132 : 1; unsigned char i133 : 1; unsigned char i134 : 1; unsigned char i135 : 1; unsigned char i136 : 1; unsigned char i137 : 1; unsigned char i138 : 1; unsigned char i139 : 1; unsigned char i140 : 1;
		unsigned char i141 : 1; unsigned char i142 : 1; unsigned char i143 : 1; unsigned char i144 : 1; unsigned char i145 : 1; unsigned char i146 : 1; unsigned char i147 : 1; unsigned char i148 : 1; unsigned char i149 : 1; unsigned char i150 : 1;
		unsigned char i151 : 1; unsigned char i152 : 1; unsigned char i153 : 1; unsigned char i154 : 1; unsigned char i155 : 1; unsigned char i156 : 1; unsigned char i157 : 1; unsigned char i158 : 1; unsigned char i159 : 1; unsigned char i160 : 1;
		unsigned char i161 : 1; unsigned char i162 : 1; unsigned char i163 : 1; unsigned char i164 : 1; unsigned char i165 : 1; unsigned char i166 : 1; unsigned char i167 : 1; unsigned char i168 : 1; unsigned char i169 : 1; unsigned char i170 : 1;
		unsigned char i171 : 1; unsigned char i172 : 1; unsigned char i173 : 1; unsigned char i174 : 1; unsigned char i175 : 1; unsigned char i176 : 1; unsigned char i177 : 1; unsigned char i178 : 1; unsigned char i179 : 1; unsigned char i180 : 1;
		unsigned char i181 : 1; unsigned char i182 : 1; unsigned char i183 : 1; unsigned char i184 : 1; unsigned char i185 : 1; unsigned char i186 : 1; unsigned char i187 : 1; unsigned char i188 : 1; unsigned char i189 : 1; unsigned char i190 : 1;
		unsigned char i191 : 1; unsigned char i192 : 1; unsigned char i193 : 1; unsigned char i194 : 1; unsigned char i195 : 1; unsigned char i196 : 1; unsigned char i197 : 1; unsigned char i198 : 1; unsigned char i199 : 1; unsigned char i1100 : 1;
		unsigned char i201 : 1; unsigned char i202 : 1; unsigned char i203 : 1; unsigned char i204 : 1; unsigned char i205 : 1; unsigned char i206 : 1; unsigned char i207 : 1; unsigned char i208 : 1; unsigned char i209 : 1; unsigned char i210 : 1;
		unsigned char i211 : 1; unsigned char i212 : 1; unsigned char i213 : 1; unsigned char i214 : 1; unsigned char i215 : 1; unsigned char i216 : 1; unsigned char i217 : 1; unsigned char i218 : 1; unsigned char i219 : 1; unsigned char i220 : 1;
		unsigned char i221 : 1; unsigned char i222 : 1; unsigned char i223 : 1; unsigned char i224 : 1; unsigned char i225 : 1; unsigned char i226 : 1; unsigned char i227 : 1; unsigned char i228 : 1; unsigned char i229 : 1; unsigned char i230 : 1;
		unsigned char i231 : 1; unsigned char i232 : 1; unsigned char i233 : 1; unsigned char i234 : 1; unsigned char i235 : 1; unsigned char i236 : 1; unsigned char i237 : 1; unsigned char i238 : 1; unsigned char i239 : 1; unsigned char i240 : 1;
		unsigned char i241 : 1; unsigned char i242 : 1; unsigned char i243 : 1; unsigned char i244 : 1; unsigned char i245 : 1; unsigned char i246 : 1; unsigned char i247 : 1; unsigned char i248 : 1; unsigned char i249 : 1; unsigned char i250 : 1;
		unsigned char i251 : 1; unsigned char i252 : 1; unsigned char i253 : 1; unsigned char i254 : 1; unsigned char i255 : 1; unsigned char i256 : 1;
	} ALARM;
	unsigned char Dimmer[MAX_DIMMERS];              //256B

	unsigned int ProgramNr;                         //2B
	unsigned int AdcProgramNr;                      //2B
	unsigned int RollerProgram;                     //2B  int 4 byte for PC / for raspberry pi
	unsigned int SecuZone;                          //2B
	unsigned int HWStates;                          //2B
	unsigned char Future[170];                       //20B for future use
	//unsigned char AdditionalInfo[256];              //Additional info for panels, audio playback status, video status

}EHOUSEPRO_BITS;
///////////////////////////////////////////////////////////////////////////////
typedef struct EHOUSEPROT
{//EHOUSEPRO_STAT
	unsigned int MarkEHousePro;                 //mark for panel eHouse Pro status 0xffff
	unsigned char devadrh;                      //ip addr lower
	unsigned char devadrl;                      //ip address lowest

	int16_t AdcVal[MAX_ADCS];               //256 bytes   - by 2 byte status

	unsigned char Outputs[MAX_OUTPUTS / 8];           //64B
	unsigned char Inputs[MAX_INPUTS / 8];             //64B
	unsigned char Horn[MAX_INPUTS / 8];               //64B
	unsigned char Warning[MAX_INPUTS / 8];            //64B

	unsigned char Monitoring[MAX_INPUTS / 8];		//64B
	unsigned char Silent[MAX_INPUTS / 8];		//64B
	unsigned char EarlyWarning[MAX_INPUTS / 8];	//64B
	unsigned char SMS[MAX_INPUTS / 8];                //64B

	unsigned char Alarm[MAX_INPUTS / 8];              //64B

	unsigned char Dimmer[MAX_DIMMERS];              //256B

	unsigned int ProgramNr;                         //2B
	unsigned int AdcProgramNr;                      //2B
	unsigned int RollerProgram;                     //2B
	unsigned int SecuZone;                          //2B
	unsigned int HWStates;                          //2B

	//instead off Future[180]
	unsigned char SatelSecuZone;                        //current security zone
	unsigned char SatelProgramNr;                        //current security zone
	unsigned char SatelRollerProgram;                        //current security zone
	unsigned char SatelHWStates;                        //current security zone
	unsigned char SatelAdcProgramNr;                        //current security zone

	unsigned char SatelE;                        //current security zone
	unsigned char SatelF;                        //current security zone
	unsigned char SatelG;                        //current security zone
	unsigned char SatelH;
	unsigned char SatelI;


	unsigned char SatelOutputs[MAX_SAT_OUTPUTS / 8];       //satel output states
	unsigned char SatelInputs[MAX_SAT_INPUTS / 8];      //satel input states
	unsigned char SatelInputsAlarm[MAX_SAT_INPUTS / 8];  //satel inputs violation
	unsigned char SatelInputsAlarmMemory[MAX_SAT_INPUTS / 8];  //satel inputs ignore
	unsigned char SatelTamper[MAX_SAT_INPUTS / 8];      //satel input states
	unsigned char SatelTamperAlarm[MAX_SAT_INPUTS / 8];  //satel inputs violation
	unsigned char SatelTamperAlarmMemory[MAX_SAT_INPUTS / 8];  //satel inputs violation
	unsigned char SatelInputsBypas[MAX_SAT_INPUTS / 8];  //satel inputs ignore until clear alarm
	unsigned char SatelInputsIsolate[MAX_SAT_INPUTS / 8];  // isolate input untill manual unbypas

	unsigned char SatelNoViolationTrouble[MAX_SAT_INPUTS / 8];   //7
	unsigned char SatelLongViolationTrouble[MAX_SAT_INPUTS / 8]; //8
	unsigned char SatelInputsMasked[MAX_SAT_INPUTS / 8];                //0x28
	unsigned char SatelInputsMaskedMemory[MAX_SAT_INPUTS / 8];          //0x29
	//int16_t AdcValL[MAX_ADCS];               //256 bytes   - by 2 byte status
	//int16_t AdcValH[MAX_ADCS];               //256 bytes   - by 2 byte status



	unsigned char Future[170];                          //20B for future use
	//unsigned char AdditionalInfo[256];              //Additional info for panels, audio playback status, video status
}EHOUSEPRO_STAT; ///sum ~1.5KB


#define SIZEOFCTRL 80//

typedef struct SatelStatusT
{
	unsigned char ZonesViolation[MAX_SAT_INPUTS / 8];            //0
	unsigned char ZonesTamper[MAX_SAT_INPUTS / 8];               //1
	unsigned char ZonesAlarm[MAX_SAT_INPUTS / 8];                //2
	unsigned char ZonesTamperAlarm[MAX_SAT_INPUTS / 8];          //3
	unsigned char ZonesAlarmMemory[MAX_SAT_INPUTS / 8];          //4
	unsigned char ZonesTamperAlarmMemory[MAX_SAT_INPUTS / 8];    //5
	unsigned char ZonesBypass[MAX_SAT_INPUTS / 8];               //6
	unsigned char ZonesNoViolationTrouble[MAX_SAT_INPUTS / 8];   //7
	unsigned char ZonesLongViolationTrouble[MAX_SAT_INPUTS / 8]; //8
	unsigned char OutputsState[MAX_SAT_OUTPUTS / 8];              //
	unsigned char ZonesIsolate[MAX_SAT_INPUTS / 8];               //0x26
	unsigned char ZonesMasked[MAX_SAT_INPUTS / 8];                //0x28
	unsigned char ZonesMaskedMemory[MAX_SAT_INPUTS / 8];          //0x29

	unsigned char AarmedPartitionsSuppressed[4];    //0x9
	unsigned char AarmedPartitionsReally[4];        //0xa
	unsigned char PartitionsArmedInMode2[4];        //0xb
	unsigned char PartitionsArmedInMode3[4];//        0x0C	
	unsigned char PartitionsWith1stCodeEntered[4];//  0x0D	
	unsigned char PartitionsEntryTime[4];//           0x0E
	unsigned char PartitionsExitTimeMore10s[4];//     0x0F
	unsigned char PartitionsExitTimeLess10s[4];//     0x10	
	unsigned char PartitionsTemporaryBlocked[4];//    0x11
	unsigned char PartitionsBlockedForGuardRound[4];    //0x12	
	unsigned char PartitionsAlarm[4];//               0x13
	unsigned char PartitionsFireAlarm[4];//           0x14
	unsigned char PartitionsAlarmMemory[4];//         0x15
	unsigned char PartitionsFireAlarmMemory[4];//     0x16
	unsigned char DoorsOpened[8];//                   0x18
	unsigned char DoorsOpenedLong[8];//               0x19
	unsigned char RTCAndBasicStatusBits[9];//         0x1A
	unsigned char ToublesPart1[47];//                  0x1B
	unsigned char TroublesPart2[26];//                 0x1C
	unsigned char TroublesPart3[60];//                 0x1D
	unsigned char TroublesPart4[30];//                 0x1E
	unsigned char TroublesPart5[31];//                 0x1F
	unsigned char TroublesMemoryPart1[47];//           0x20
	unsigned char TroublesMemoryPart2[39];//           0x21
	unsigned char TroublesMemoryPart3[60];//           0x22
	unsigned char TroublesMemoryPart4[30];//           0x23
	unsigned char TroublesMemoryPart5[48];//           0x24
	unsigned char PartitionsWithViolatedZones[4];//    0x25
	unsigned char PartitionsWithVerifiedAlarms[4];//   0x27
	unsigned char PartitionsArmedInMode1[4];//         0x2A
	unsigned char PartitionsWithWarningAlarms[4];//    0x2B
	unsigned char TroublesPart6[45];//                 0x2C
	unsigned char TroublesPart7[47];//                 0x2D
	unsigned char TroublesMemoryPart6[45];//           0x2E
	unsigned char TroublesMemoryPart7[48];//           0x2F
	unsigned char ModuleVersion[12];//                 0x7C
	unsigned char Temperature[3];//                    0x7D
	unsigned char IntegraVersion[3];//                 0x7E    

	unsigned char ArmInMode0[SIZEOFCTRL];//                    0x80
	unsigned char ArmInMode1[SIZEOFCTRL];//                    0x81
	unsigned char ArmInMode2[SIZEOFCTRL];//                    0x82
	unsigned char ArmInMode3[SIZEOFCTRL];//                    0x83
	unsigned char DisArm[SIZEOFCTRL];//                        0x84
	unsigned char ClearAlarm[SIZEOFCTRL];//                    0x85
	//unsigned char ZoneBypas[SIZEOFCTRL];//                     0x86
	unsigned char ZoneUnBypas[SIZEOFCTRL];//                   0x87
	unsigned char SetOutsOn[SIZEOFCTRL];//                     0x88
	unsigned char SetOutsOff[SIZEOFCTRL];//                    0x89
	unsigned char OpenDoor[SIZEOFCTRL];//                       0x8a
	unsigned char ClearTroubleMem[SIZEOFCTRL];//               0x8b
	unsigned char ReadEvent[SIZEOFCTRL];//                     0x8c
	unsigned char Enter1stCode[SIZEOFCTRL];//                  0x8d
	unsigned char SetRTCClock[SIZEOFCTRL];//                   0x8e    
	unsigned char GetEventText[SIZEOFCTRL];//                  0x8f
	unsigned char ZoneIsolate[SIZEOFCTRL];//                   0x90
	unsigned char OutputSwitch[SIZEOFCTRL];//                  0x91

	unsigned char ForceArmInMode0[SIZEOFCTRL];//               0xa0
	unsigned char ForceArmInMode1[SIZEOFCTRL];//               0xa1
	unsigned char ForceArmInMode2[SIZEOFCTRL];//               0xa2
	unsigned char ForceArmInMode3[SIZEOFCTRL];//               0xa3
	unsigned char ReturnStatus[SIZEOFCTRL];//                  0xef

	unsigned char  ProgramNr;
	unsigned char AdcProgramNr;
	unsigned char SecuZone;
	unsigned char SecuProgram;
	unsigned char RollerProgram;
} SatelStatus;


typedef union eHouseProStatusUT
{
	EHOUSEPRO_STAT status;
	EHOUSEPRO_BITS bits;
	unsigned char data[sizeof(EHOUSEPRO_STAT)];
} eHouseProStatusU;



//#endif        

#ifdef GU_INTERFACE
#include <time.h>
#ifndef WIN32
#include <sys/time.h>        
#endif
#ifndef EH_REMOVE_NAMES 
typedef char StringN[SIZEOFTEXT];
#else
typedef char StringN[2];
#endif
//Names for GUI interface eHouse1
typedef struct eHouse1NamesT
{
	char		INITIALIZED;
	int         AddrH;          //Address H
	int         AddrL;          //Address L
	struct timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[35];           //Outputs Names
	StringN Inputs[16];         //Inputs Names
	StringN ADCs[16];           //ADC measurement Names
	StringN Programs[24];       //Program Names
	StringN Dimmers[3];        //Dimers Names
	unsigned char BinaryStatus[200];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
	unsigned char FromIPH;
	unsigned char FromIPL;
	double SensorTempsLM335[16];
	double SensorTempsLM35[16];
	double SensorTempsMCP9700[16];
	double SensorTempsMCP9701[16];
	double SensorPercents[16];
	double SensorInvPercents[16];
	double SensorVolts[16];
	double AdcValue[16];

} eHouse1Names;
///////////////////////////////////////////////////////////////////////////////
//Names for GUI interface eHouse4CAN
typedef struct eHouseCANNamesT
{
	int         AddrH;          //Address H
	int         AddrL;          //Address L
	struct timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[8];           //Outputs Names
	StringN Inputs[8];         //Inputs Names
	StringN ADCs[4];           //ADC measurement Names
	StringN Programs[24];       //Program Names
	StringN ADCPrograms[24];    //ADC PRogram names
	StringN Dimmmers[4];        //Dimers Names



	char   ADCConfig[4];         //adc type config
	double Vcc[4];
	double SensorTempsLM335[4];
	double SensorTempsLM35[4];
	double SensorTempsMCP9700[4];
	double SensorTempsMCP9701[4];
	double SensorPercents[4];
	double SensorInvPercents[4];
	double SensorVolts[4];
	double AdcValue[4];
	double Calibration[4];
	double Offset[4];
	double CalibrationV[4];
	double OffsetV[4];
	unsigned char BinaryStatus[20];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
} eHouseCANNames;


////////////////////////////////////////////////////////////////////////////////
//Names for GUI interface eHouse4Ethernet

typedef struct EtherneteHouseNamesT
{
	char        INITIALIZED;
	int         AddrH;          //Address H
	int         AddrL;          //Address L
	char   DevAddr[7];             //Address Combined
	struct timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[35];           //Outputs Names
	StringN Inputs[24];         //Inputs Names
	StringN ADCs[16];           //ADC measurement Names
	StringN Programs[24];       //Program Names
	StringN ADCPrograms[24];       //Program Names
	StringN Dimmers[3];        //Dimers Names
	StringN Rollers[16];
	char ADCConfig[16];         //adc type config
	double Vcc[16];
	double SensorTempsLM335[16];
	double SensorTempsLM35[16];
	double SensorTempsMCP9700[16];
	double SensorTempsMCP9701[16];
	double SensorPercents[16];
	double SensorInvPercents[16];
	double SensorVolts[16];
	double AdcValue[16];
	double Calibration[16];
	double Offset[16];
	double CalibrationV[16];
	double OffsetV[16];
	unsigned char BinaryStatus[200];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
} EtherneteHouseNames;

/////////////////////////////////////////////
typedef struct AuraNamesT
{
	unsigned char	      INITIALIZED;
	unsigned char         AddrH;    //Address H
	unsigned char         AddrL;    //Address L
	unsigned long         ID;       //Aura Identifier
	char   DevAddr[7];              //Address Combined
	struct timeval tim;             //Recent status
	StringN Name;                   //Controller Name
	StringN Outs[4];                //Outputs Names
	StringN Inputs[4];              //Inputs Names
	StringN ADCs[8];                //ADC measurement Names
	unsigned char  ADCUnit[8];
	unsigned char ParamSymbol[8];      //unit symbol
	int Vcc;
	int ParamValue[8];
	int ParamPreset[8];
	StringN Dimmers[3];             //Dimers Names
	unsigned char   Dtype;          //device type
	unsigned char BinaryStatus[100];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
	unsigned char StatusTimeOut;
} AuraNames;
/////////////////////////////////////////////

typedef struct WiFieHouseNamesT
{
	char  INITIALIZED;
	int         AddrH;          //Address H
	int         AddrL;          //Address L
	char   DevAddr[7];             //Address Combined
	struct timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[8];           //Outputs Names
	StringN Inputs[8];         //Inputs Names
	StringN ADCs[4];           //ADC measurement Names
	StringN Programs[24];       //Program Names
	StringN ADCPrograms[24];       //Program Names
	StringN Dimmers[4];        //Dimers Names
	StringN Rollers[2];        //Rollers Names
	char    ADCConfig[4];         //adc type config
	double Vcc[4];
	double SensorTempsLM335[4];
	double SensorTempsLM35[16];
	double SensorTempsMCP9700[4];
	double SensorTempsMCP9701[16];
	double SensorPercents[4];
	double SensorInvPercents[4];
	double SensorVolts[4];
	double AdcValue[4];
	double Calibration[4];
	double Offset[4];
	double CalibrationV[4];
	double OffsetV[4];
	unsigned char BinaryStatus[50];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
} WiFieHouseNames;



/////////////////////////////////////////////////////////////////////////////////
typedef struct eHouseProNamesT
{
	char INITIALIZED;
	int    AddrH[5];          //Address H
	int    AddrL[5];          //Address L
	char   DevAddr[7];          //Address Combined
	struct timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[256];           //Outputs Names
	StringN Inputs[256];         //Inputs Names
	StringN ADCs[128];           //ADC measurement Names
	StringN Programs[256];       //Program Names
	StringN SecuPrograms[256];   //Rollers+Zone Program Names
	StringN Zones[256];          //Zone Names
	StringN ADCPrograms[256];    //ADCProgram Names //heating measurement
	StringN Dimmers[256];         //Dimers Names
	StringN Rollers[256];        //Rollers Names
	StringN InputsSMS[256];         //Inputs Names for SMS
	StringN ZonesSMS[256];          //Zone Names for SMS
	unsigned char BinaryStatus[SIZE_OF_EHOUSE_PRO_STATUS / 2];
	StringN ADCsDS[128];           //ADC measurement Names
	char    ADCIndexs[128];
	StringN ADCType[128];
	char    ADCUnit[128];
	int    ADCResultValue[128];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
}eHouseProNames;

typedef struct SatelT
{
	int     AddrH;          //Address H
	int     AddrL;          //Address L
	char    DevAddr[7];          //Address Combined
	struct  timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[256];           //Outputs Names
	StringN Inputs[256];         //Inputs Names
	StringN ADCs[128];           //ADC measurement Names
	StringN Programs[256];       //Program Names
	StringN SecuPrograms[256];   //Rollers+Zone Program Names
	StringN Zones[256];          //Zone Names
	StringN ADCPrograms[256];    //ADCProgram Names //heating measurement
	StringN Dimmers[256];         //Dimers Names
	StringN Rollers[256];        //Rollers Names
	StringN InputsSMS[256];         //Inputs Names for SMS
	StringN ZonesSMS[256];          //Zone Names for SMS
	unsigned char BinaryStatus[SIZE_OF_EHOUSE_PRO_STATUS / 2];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
} SatelNames;





typedef struct CommManagerNamesT
{
	char INITIALIZED;
	int         AddrH;          //Address H
	int         AddrL;          //Address L
	char   DevAddr[7];          //Address Combined
	struct timeval tim;         //Recent status
	StringN Name;               //Controller Name
	StringN Outs[80];           //Outputs Names
	StringN Inputs[48];         //Inputs Names
	StringN ADCs[16];           //ADC measurement Names
	StringN Programs[24];       //Program Names
	StringN SecuPrograms[24];   //Rollers+Zone Program Names
	StringN Zones[24];          //Zone Names
	StringN ADCPrograms[24];    //ADCProgram Names //heating measurement
	StringN Dimmers[3];         //Dimers Names
	StringN Rollers[24];        //Rollers Names
	char ADCConfig[16];         //adc type config
	double Vcc[16];
	double SensorTempsLM335[16];
	double SensorTempsLM35[16];
	double SensorTempsMCP9700[16];
	double SensorTempsMCP9701[16];
	double SensorPercents[16];
	double SensorInvPercents[16];
	double SensorVolts[16];
	double AdcValue[16];
	double Calibration[16];
	double Offset[16];
	double CalibrationV[16];
	double OffsetV[16];
	unsigned char BinaryStatus[200];
	unsigned int BinaryStatusLength;
	unsigned int TCPQuery;
} CommManagerNames;

typedef struct AURAT
{
	unsigned char   Addr;
	unsigned char   FRT;
	unsigned int    DType;
	unsigned char   Freq;
	unsigned char   PeriodB;
	unsigned char   PowerRF;
	unsigned char   FrameSize;
	unsigned char   Buffer[50];
	uint64_t        BuffPart[MAX_AURA_BUFF_SIZE];
	uint64_t        DataPart[MAX_AURA_BUFF_SIZE];
	uint32_t        ID;
	unsigned char   ParamsCount;
	unsigned char   ParamType[20];
	unsigned char   ParamSize[20];
	unsigned int    ParamValue[20];
	unsigned int    ParamMask[20];
	unsigned char   SP[20];
	unsigned int    Flags;
	unsigned char   FW;
	unsigned char   HW;
	int   RSSI;
	unsigned char   LQI;
	//data to store from sensor
	double Temp;
	double LocalTempSet;
	double PrevLocalTempSet;
	double TempSet;
	double ServerTempSet;
	double PrevServerTempSet;
	double PercentSet;

	double volt;

	unsigned char Close : 1;
	unsigned char OpenL : 1;
	unsigned char OpenSmall : 1;
	unsigned char OpenP : 1;
	unsigned char NonProof : 1;
	unsigned char AlarmSoft : 1;
	unsigned char On : 1;
	unsigned char Off : 1;
	unsigned char Flood : 1;
	unsigned char Smoke : 1;
	unsigned char AlarmTamper : 1;
	unsigned char AlarmFire : 1;
	unsigned char AlarmHardware : 1;
	unsigned char AlarmMemory : 1;
	unsigned char Rain : 1;
	unsigned char AlarmFreeze : 1;
	unsigned char LocalControl : 1;
	unsigned char Switch : 1;
	unsigned char Direction;
	unsigned char DoorState;
	unsigned char CentralTemp;
	unsigned char CentralCont;
	unsigned char Units;
	unsigned char Hist;
	unsigned char Lock;
	unsigned char Door;

	unsigned char BATERY_LOW : 1;
	unsigned char RSI_LOW : 1;
	unsigned char LOCK : 1;
	unsigned char RAIN : 1;
	unsigned char WINDOW_CLOSE : 1;
	unsigned char WINDOW_SMALL : 1;
	unsigned char WINDOW_OPEN : 1;
	unsigned char WINDOW_UNPROOF : 1;
	unsigned char I1 : 1;
	unsigned char I2 : 1;
	unsigned char O1 : 1;
	unsigned char O2 : 1;
	unsigned char LOCAL_CTRL : 1;
	unsigned char LOUD : 1;
	unsigned char _1 : 1;
	unsigned char _2 : 1;

	unsigned char ALARM_SOFT : 1;
	unsigned char ALARM_FLOOD : 1;
	unsigned char ALARM_HARD : 1;
	unsigned char ALARM_SMOKE : 1;
	unsigned char ALARM_HUMIDITY_H : 1;
	unsigned char ALARM_HUMIDITY_L : 1;
	unsigned char ALARM_TEMP_H : 1;
	unsigned char ALARM_TEMP_L : 1;
	unsigned char ALARM_TAMPER : 1;
	unsigned char ALARM_MEMORY : 1;
	unsigned char ALARM_FREEZE : 1;

	unsigned char ALARM_FIRE : 1;
	unsigned char ALARM_CO : 1;
	unsigned char ALARM_CO2 : 1;
	unsigned char ALARM_GAS : 1;
	unsigned char ALARM_WIND : 1;

} AURA;


#endif
//////////////////////////////////////

#endif
