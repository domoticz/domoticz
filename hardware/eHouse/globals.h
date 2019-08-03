#pragma once
#define  REMOVEUNUSED 1
#define SIZEOFTEXT 80
#define EH_REMOVE_NAMES 1
#define LOG _log.Log

#define             MAXMSG 0xfffful										//size of udp message buffer


//ethernet controllers full status binary catch  from UDP Packets directly from controllers
#define STATUS_ADDRH			1u
#define STATUS_ADDRL			2u
#define STATUS_CODE				3u								//normal status == 's'
#define STATUS_DIMMERS			4u   							//dimmers 3 PWM
#define STATUS_DMX_DIMMERS		STATUS_DIMMERS + 3				//17 DMX
#define STATUS_ADC_LEVELS		24u     						//16*2  //8*2*2 for 8 ADCs
#define STATUS_MORE				64u             				// 16b
#define STATUS_ADC_ETH			72u								//ADCs in 16 * 2B
#define STATUS_ADC_ETH_END		STATUS_ADC_ETH + 32u			//84	//104
#define STATUS_OUT_I2C			STATUS_ADC_ETH_END				//
#define STATUS_DMX_DIMMERS2		STATUS_OUT_I2C + 5				//ERM ONLY 15 bytes/dimmers 
#define STATUS_INPUTS_I2C		STATUS_OUT_I2C + 20u			//2 razy i2c razy 6 rej po 8	//max 96 
#define STATUS_DALI			STATUS_INPUTS_I2C + 2				//ERM ONLY 10 inptus+12 ALARMs+12 Warning+12monitoring
#define STATUS_ALARM_I2C		STATUS_INPUTS_I2C + 12u			//CM only --|---
#define STATUS_WARNING_I2C		STATUS_ALARM_I2C + 12u			//CM only --|---
#define STATUS_MONITORING_I2C           STATUS_WARNING_I2C + 12u//CM only --|---			//160
#define STATUS_PROGRAM_NR		STATUS_MONITORING_I2C + 12u		//--|---
#define STATUS_ZONE_NR			STATUS_PROGRAM_NR + 1u
#define STATUS_ADC_PROGRAM		STATUS_ZONE_NR + 1u
#define STATUS_LIGHT_LEVEL		STATUS_ADC_PROGRAM + 2u			//LIGHT LEVEL 3 * 2B
#define STATUS_SIZE             180u
#define STATUS_PROFILE_NO       (STATUS_SIZE - 1u)				//Program Nr
#define STATUS_ZONE_NO          (STATUS_SIZE - 2u)				//Zone Nr

//eHouse 1 (RS-485) binary status distributed via eHouse PRO, CommManager/LevelManager
#define    STATUS_OFFSET 2                 //offset for status locations in binary buffer
//byte index location of binary status query results
#define    RM_STATUS_ADC                 1  + STATUS_OFFSET    //start of adc measurement
#define    RM_STATUS_OUT                 17 + STATUS_OFFSET    //RM start of outputs
#define    HM_STATUS_OUT                 33 + STATUS_OFFSET    //HM start of outputs
#define    RM_STATUS_IN                  20 + STATUS_OFFSET    //RM start of inputs
#define    RM_STATUS_INT                 21 + STATUS_OFFSET    //rm start of inputs (fast)
#define    RM_STATUS_OUT25               22 + STATUS_OFFSET    //rm starts of outputs => 25-32
#define    RM_STATUS_LIGHT               23 + STATUS_OFFSET    //rm light level start
#define    RM_STATUS_ZONE_PGM            26 + STATUS_OFFSET    //rm current zone
#define    RM_STATUS_PROGRAM             27 + STATUS_OFFSET    //rm current program
#define    RM_STATUS_INPUTEXT_A_ACTIVE   28 + STATUS_OFFSET    //em input extenders A status active inputs
#define    RM_STATUS_INPUTEXT_B_ACTIVE   32 + STATUS_OFFSET    //em input extenders B status active inputs
#define    RM_STATUS_INPUTEXT_C_ACTIVE   36 + STATUS_OFFSET    //em input extenders C status active inputs
#define    RM_STATUS_INPUTEXT_A          40 + STATUS_OFFSET    //em --||-
#define    RM_STATUS_INPUTEXT_B          50 + STATUS_OFFSET    //em 
#define    RM_STATUS_INPUTEXT_C          60 + STATUS_OFFSET    //em 
#define    HM_STATUS_PROGRAM             36 + STATUS_OFFSET    //hm current program
#define    HM_STATUS_KOMINEK             46 + STATUS_OFFSET    //hm status bonfire
#define    HM_STATUS_RECU                48 + STATUS_OFFSET    //hm status recu
#define    HM_WENT_MODE                  49 + STATUS_OFFSET    //hm went mode
//eHouse WiFi binary status via UDP Directly
#define     WIFI_STATUS_OFFSET      4
#define     WIFI_OUTPUT_COUNT       8
#define     WIFI_INPUT_COUNT        8
#define     WIFI_DIMM_COUNT         4
#define     WIFI_ADC_COUNT          4
#define     ADC_OFFSET_WIFI         0+WIFI_STATUS_OFFSET      ///4*2B
#define     DIMM_OFFSET_WIFI        8+WIFI_STATUS_OFFSET      ///4*1B
#define     OUT_OFFSET_WIFI         12+WIFI_STATUS_OFFSET     //1B
#define     IN_OFFSET_WIFI          13+WIFI_STATUS_OFFSET
#define     OUT_PROGRAM_OFFSET_WIFI 14+WIFI_STATUS_OFFSET      //5b
#define     MODE_OFFSET_WIFI        15+WIFI_STATUS_OFFSET   //3b
#define     ADCPRG_OFFSET_WIFI      15+WIFI_STATUS_OFFSET   //:5;
#define     ModeB_OFFSET_WIFI       16+WIFI_STATUS_OFFSET 
#define     RSSI_WIFI               17+WIFI_STATUS_OFFSET 
#define     SINCE_WIFI              18+WIFI_STATUS_OFFSET 
#define     STATUS_ADC_LEVELS_WIFI  19+WIFI_STATUS_OFFSET 
#define     next_wifi               19+(2*4)+WIFI_STATUS_OFFSET


#define EH_RS485    1
#define EH_LAN      2
#define EH_PRO      3
#define EH_WIFI     4
#define EH_CANRF    5
#define EH_AURA     6
#define EH_LORA     7

#define VISUAL_BLINDS           100
#define VISUAL_ADC_IN           110
#define VISUAL_TEMP_IN          111
#define VISUAL_LIGHT_IN         112
#define VISUAL_LM35_IN          113
#define VISUAL_LM335_IN         114
#define VISUAL_MCP9700_IN       115
#define VISUAL_MCP9701_IN	116
#define VISUAL_PERCENT_IN	117
#define VISUAL_INVERTED_PERCENT_IN 118
#define VISUAL_VOLTAGE_IN	119
#define VISUAL_DYNAMIC_TEXT	120
#define  VISUAL_INPUT_IN	121
#define VISUAL_ACTIVE_IN	122
#define VISUAL_WARNING_IN	123
#define VISUAL_MONITORING_IN	124
#define VISUAL_ALARM_IN		125
#define VISUAL_DIMMER_OUT	126
#define VISUAL_HORN_IN          127
#define VISUAL_EARLYWARNING_IN  128
#define VISUAL_SMS_IN           129
#define VISUAL_SILENT_IN        130
#define VISUAL_DIMMER_RGB       131
#define VISUAL_MCP9700_PRESET   132
#define VISUAL_PGM              133
#define VISUAL_APGM             134
#define VISUAL_AURA_IN          135
#define VISUAL_AURA_PRESET      136
#define VISUAL_ZONEPGM		    137
#define VISUAL_SECUPGM			138

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 700
#else
#define _XOPEN_SOURCE 700
#endif /* __STDC_VERSION__ */
#define GU_INTERFACE 1
#define MAX_INPUTS  256ul
#define MAX_OUTPUTS 256ul
#define MAX_DIMMERS 256ul
#define MAX_ADCS    128ul
#define MAX_SAT_OUTPUTS 256u
#define MAX_SAT_INPUTS  256u
#define MAX_AURA_DEVS 20
#define MAX_AURA_BUFF_SIZE 10
#define CLOUD_THREAD 1
#define MODBUS_THREAD 1
#define AURA_THREAD 1
#define RFID_ENABLED 1
#define SATEL_ENABLED 1
#define MAX_RFID_GW 4
#define MAX_SATEL 4
#define HEARTBEAT_INTERVAL_MS 12*1000 // 12 sec
#define PRO_HALF_IO    2		//MAX_INPUTS & MAX_OUTPUTS cont division by (only half outputs)
#ifdef WIN32
#define WIN_32
#define USE_SYS_TYPES_FD_SET 1
#endif

/*
 * File:   globals.h
 * * ONLY for usage with eHouse system controllers | Intellectual property of iSys.Pl | Any other usage, copy etc. is forbidden
 * Author: Robert Jarzabek
 * http://www.iSys.Pl/ Strona producenta iSys - Intelligent Systems
 * http://Inteligentny-Dom.eHouse.Pro/ Inteligentny Dom eHouse - ZrĂłb to sam, Programowanie, Samodzielny montaĹĽ, projektowanie
 * http://www.eHouse.Pro/ Automatyka Budynku eHouse
 * http://sterowanie.biz/ Sterowanie domem, budynkiem z eHouse
 * http://Home-Automation.iSys.pl/ eHouse Home Automation producer homepage
 * http://Home-Automation.eHouse.Pro/ eHouse Home Automation "DIY" Do It Yourself, Examples, Designs, Self Installation, Programming
 *
 * Global configuration
 *
 * Created on June 3, 2013, 9:43 AM
 */
#ifndef WIN32
 //	#include <unistd.h>
 //	#include <sys/time.h>
#else
 //	#include <time.h>
#endif
//#include <fcntl.h>
//#include <string.h>
//#include <stdlib.h>

//#include <sys/timeb.h>
//#include <stdio.h>
//#include <time.h>
//extern int nanosleep(const struct timespec *req, struct timespec *rem);
#ifndef EHGLOBALS_H
#define	EHGLOBALS_H

#define TCP_CLIENT_THREAD 1
//#define UDP_USE_THREAD 1              //Use UDP Thread 
#define EHOUSE_TCP_CLIENT_THREAD 1      //Use Multi-Thread for TCP Clients
#endif


#ifdef	__cplusplus
extern "C" {
#endif

	//#define D_HOST      "localhost"
	//#define D_QUEUE     32
	//#define D_SOCKETS   16
	//#define D_INFO      25616 



#ifdef	__cplusplus
}
#endif
#define STATUS_ARRAYS_SIZE				0x20         //  1f       //32  eH/eHE
#define STATUS_WIFI_ARRAYS_SIZE			100     //    eHWIFI array size
#define TCP_CLIENT_COUNT				10
#define ONKYO_COUNT						3

#define EHOUSE1_RM_MAX                  (STATUS_ARRAYS_SIZE)             //do not modify this values here
#define ETHERNET_EHOUSE_RM_MAX          (STATUS_ARRAYS_SIZE)            //do not modify this values here
#define EHOUSE_CAN_MAX                  0x80
#define EHOUSE_RF_MAX                   EHOUSE_CAN_MAX
#define EHOUSE_WIFI_MAX                   EHOUSE_CAN_MAX
#define EVENT_QUEUE_MAX                 0x20ul             //maximal size of Event Queue
#define MAX_LOCAL_EVENTS_BUFF           EVENT_QUEUE_MAX //-----||-------------||-------
#define MAX_EVENTS_IN_PACK              10u             //Maximal Event in one packet for Ethernet controllers
#define EVENT_SIZE                      10u             //size of direct event
#define LOCAL_EVENT                     0
#define ETHERNET_EVENT                  1               //event to be send via Ethernet TCP/IP
#define RS485_EVENT                     2               //event to be send via RS-485 directly
#define CAN_EVENT                       3               //event kto be send via CAN
//#define EHOUSE_PRO_EVENT              4               //local event
#define COMMAND_RUN_ADVANCED_EVENT 		'b'             //Extended event command
#define COMMAND_CONFIRM_ADVANCED_EVENT 	'f'             //extended event confirmation

#define RUN_NOW                         5 //*0.2s = ~1s // run now event  timer counter - Execute Imedialtelly
#define RE_SEND_RETRIES                 5u              //No of retries in case of execution failure
#define RE_TIME_REPLAY                  25              //Timer for retry in RE_TIME_RETRY-RE_SEND_RETRIES *0.1s
#define MAX_CLIENT_SOCKETS              10                           //Maximal number of client sockets if configured for multi-threading TCP Client
#define MODBUS_MAX_CLIENT_SOCKETS       20                           //Maximal number of client sockets if configured for multi-threading TCP Client for MODBUS
#define EV_LOCAL_EVENT                  0       //current server
#define EV_ETHERNET_EVENT               1       //Ethernet Event
#define EV_RS485_EVENT                  2       //RS-485 eHouse1 Event
#define EV_CAN_EVENT                    3       //CAN Event locally
#define EV_VIA_CM                       4       //Via CommManager 
#define EV_VIA_EHOUSE_PRO               5       //PIC32 Ehouse Pro Controller Server
#define EV_RF_EVENT                     6
#define EV_ZIGBEE_EVENT                 7
///function prototypes 
#ifdef UDP_USE_THREAD
extern void *UDPListener(void *ptr);  //for eHouse4Ethernet devices and eHouse1 via CommManager
#else

extern void UDPListener(void);  //for eHouse4Ethernet devices and eHouse1 via CommManager
#endif
#define GSMLOOP 1
#define SIZE_OF_EHOUSE_PRO_STATUS 4000

#define MAX_LOCAL_INTERFACES 10

#define EV_LOCAL_EVENT                  0       //current server
#define EV_ETHERNET_EVENT               1       //Ethernet Event
#define EV_RS485_EVENT                  2       //RS-485 eHouse1 Event
#define EV_CAN_EVENT                    3       //CAN Event locally via UART<-> can gateway
#define EV_VIA_CM                       4       //Via CommManager 
#define EV_VIA_EHOUSE_PRO               5       //PIC32 Ehouse Pro Controller Server
#define EV_CAN_EVENT_PRO                6       //Can event via eHouse Pro controller


#define    AURA_STAT_BATERY_LOW     0x1ul
#define    AURA_STAT_RSI_LOW        0x2ul    
#define    AURA_STAT_LOCK           0x4ul
#define    AURA_STAT_RAIN           0x8ul    
#define    AURA_STAT_WINDOW_CLOSE   0x10ul
#define    AURA_STAT_WINDOW_SMALL   0x20ul
#define    AURA_STAT_WINDOW_OPEN    0x40ul
#define    AURA_STAT_WINDOW_UNPROOF 0x80ul
#define    AURA_STAT_I1             0x100ul
#define    AURA_STAT_I2             0x200ul
#define    AURA_STAT_O1             0x400ul
#define    AURA_STAT_O2             0x800ul
#define    AURA_STAT_LOCAL_CTRL     0x1000ul
#define    AURA_STAT_LOUD           0x2000ul
#define    AURA_STAT_1               0x4000ul
#define    AURA_STAT_2               0x8000ul

#define    AURA_STAT_ALARM_SOFT          0x10000ul
#define    AURA_STAT_ALARM_FLOOD         0x20000ul
#define    AURA_STAT_ALARM_HARD          0x40000ul
#define    AURA_STAT_ALARM_SMOKE         0x80000ul    
#define    AURA_STAT_ALARM_HUMIDITY_H    0x100000ul 
#define    AURA_STAT_ALARM_HUMIDITY_L    0x200000ul
#define    AURA_STAT_ALARM_TEMP_H        0x400000ul
#define    AURA_STAT_ALARM_TEMP_L        0x800000ul
#define    AURA_STAT_ALARM_TAMPER        0x1000000ul
#define    AURA_STAT_ALARM_MEMORY        0x2000000ul
#define    AURA_STAT_ALARM_FREEZE        0x4000000ul

#define    AURA_STAT_ALARM_FIRE          0x8000000ul    
#define    AURA_STAT_ALARM_CO            0x10000000ul
#define    AURA_STAT_ALARM_CO2           0x20000000ul
#define    AURA_STAT_ALARM_GAS           0x40000000ul
#define    AURA_STAT_ALARM_WIND          0x80000000ul


