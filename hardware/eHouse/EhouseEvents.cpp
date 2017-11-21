/* 
 * File:   Events.cpp
 * ONLY for usage with eHouse system controllers | Intellectual property of iSys.Pl
 * eHouse4Ethernet Server - described in UdpListener.cpp
 * eHouse1 (RS-485) Server  //described in rs485.cpp
 * eHouse1 under CommManager Supervision Server - described in UdpListener.cpp
 * TCP Client for submitting events to Ethernet eHouse Controllers and eHouse 1 via CommManager in case of eHouse under CM supervision version described in TcpClient.c
 * 
 * EventQueue Process 
 * 
 * Author: Robert Jarzabek, iSys.Pl
 * http://en.isys.pl/          eHouse Home automation Producer Web Page
 * http://eHouse.biz/      eHouse Home automation SHOP 
 * http://smart.ehouse.pro/   eHouse Home Automation - Do it yourself, programming examples, designing
 * Recently updated 11.11.2017
 * globals.h - global configuration / main settings
 * most important params for eHouse events manager
 * 
 * COMMANAGER_IP_HIGH - set CommManager Ip addr h if differs from 192.168.0.254 (192.168.%COMMANAGER_IP_HIGH%.%COMMANAGER_IP_LOW%)
 * COMMANAGER_IP_LOW - set CommManager Ip addr l
 * EHOUSE_PRO_HIGH - default eHouse Pro Server IP addr h (0)
 * EHOUSE_PRO_LOW  - default eHouse Pro Server IP addr l (150)
 * RE_SEND_RETRIES - No of retries in case of execution failure
 * RE_TIME_REPLAY  - Timer for retry in (RE_TIME_RETRY-RE_SEND_RETRIES) *0.1s
 * 
 * Events.cpp - Event Queue processing 
 */

#include "stdafx.h"
#include "../main/Logger.h"
#include "../eHouseTCP.h"
#ifndef WIN32

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
//#include <stropts.h>
#include <unistd.h>
#include <dirent.h>
#include <net/if.h>
//#include <linux/netdevice.h>
#else
#include <Windows.h>
#endif
#include "globals.h"
#include "status.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>



unsigned int EventsCountInQueue=0;      //Events In queue count to bypass processing EventQueue when it is empty
extern eHouseProNames              eHouseProN;
void ExecEvent(unsigned int i);
signed int AddToLocalEvent(unsigned char *Even,unsigned char offset);

/////////////////////////////////////////////////////////////////////////////////
signed int eHouseTCP::GetIndexOfEvent(unsigned char *TempEvent)
{
unsigned int i;
for (i=0;i<EVENT_QUEUE_MAX;i++)
	{
        if (memcmp(TempEvent,(struct EventQueueT *) &EvQ[i]->LocalEventsToRun, EVENT_SIZE)==0)
            return i;	//Exist on Event Queue
        }
return -1;
}
unsigned char DisablePerformEvent=0;
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::ExecQueuedEvents(void)
{
//signed char k;
unsigned int i;
unsigned char localy=1;
DisablePerformEvent=0;
if (!EventsCountInQueue) return;
EventsCountInQueue=0;
for (i=0;i<EVENT_QUEUE_MAX;i++)
	{
        if (EvQ[i]->LocalEventsToRun[2])	//not empty event
                {
                EventsCountInQueue++;	
                if (EvQ[i]->LocalEventsTimeOuts==RUN_NOW)	//Perform It Now !!!
                        {
						if (EvQ[i]->LocalEventsDrop) _log.Log(LOG_STATUS, "[Perform Queue Event] Q[%d] Now -> Retry: %d",i,EvQ[i]->LocalEventsDrop);
                        EvQ[i]->LocalEventsDrop++;
                        if (EvQ[i]->LocalEventsDrop>RE_SEND_RETRIES)
                              {
							  if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Exec Event] Q[%d] Drop Event",i);
                              memset(EvQ[i],0,sizeof(EvQ[i]));//delete event permanently and immediatelly                
                              }
                        else 
                            {
                            ExecEvent(i);                        
                            break;
                            }
                    	}
                else 
                        if (EvQ[i]->LocalEventsTimeOuts) 
                                EvQ[i]->LocalEventsTimeOuts--;
                        else
                            {
                            memset(EvQ[i],0,sizeof(EvQ[i]));
							if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Exec Event] Remove Event");
                            }
                }

        }
}
//////////////////////////////////////////////////////////////////////////////////
/*
 * Exec Event from EventsQueue EvQ[i]
 * 
 * 
 */
void eHouseTCP::ExecEvent(unsigned int i)
{
unsigned char devh,devl;
unsigned char EventBuff[255];
unsigned int EventBuffSize;
unsigned int EventSize;
unsigned char EventsToRun[MAX_EVENTS_IN_PACK+10];
unsigned int m=0;
EventBuffSize=0;
EventSize=0;
devh=EvQ[i]->LocalEventsToRun[0];                        //get address from DirectEvent
devl=EvQ[i]->LocalEventsToRun[1];
int ind=IndexOfEthDev(devh,devl);
if (ind==0)
    {
    //printf("[Exec Event] RS-485, CAN, RF via PRO \r\n");
    int mmm=0;
    for (m=0;m<EVENT_QUEUE_MAX;m++)
         {
         if ((IndexOfEthDev(devh,devl)==0) && (EvQ[m]->LocalEventsTimeOuts==RUN_NOW))
                {
                EvQ[m]->LocalEventsTimeOuts=RE_TIME_REPLAY;      //Retry timer interval
                memcpy(&EventBuff[EventBuffSize],EvQ[m]->LocalEventsToRun,EVENT_SIZE);   //Add to send buffer                
        //        printf("[ via PRO ] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n",EvQ[m].LocalEventsToRun[0],EvQ[m].LocalEventsToRun[1],EvQ[m].LocalEventsToRun[2],EvQ[m].LocalEventsToRun[3],EvQ[m].LocalEventsToRun[4],EvQ[m].LocalEventsToRun[5],EvQ[m].LocalEventsToRun[6],EvQ[m].LocalEventsToRun[7],EvQ[m].LocalEventsToRun[8],EvQ[m].LocalEventsToRun[9]);
                EventBuffSize+=EVENT_SIZE;
                EventsToRun[EventSize]=m;
                EventSize++;
                if (EventSize>=MAX_EVENTS_IN_PACK) break;       //As many events as possible
                }
            }
        SendTCPEvent(EventBuff,EventSize,SrvAddrH,SrvAddrL,EventsToRun);       //Submit via tcp client to desired Ethernet eHouse controller
        return;
    }
else
    {
      //  printf("[Exec Event] Ethernet \r\n");
        memset(&EventBuff,0,sizeof(EventBuff));
        EventSize=0;
        memset(EventsToRun,0,sizeof(EventsToRun));
        int mmm=0;
        for (m=0;m<EVENT_QUEUE_MAX;m++)
            {
            if ((EvQ[m]->LocalEventsToRun[0]==devh) && (EvQ[m]->LocalEventsToRun[1]==devl) && (EvQ[m]->LocalEventsTimeOuts==RUN_NOW))
                    {
          //          printf("[Directly ] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n",EvQ[m].LocalEventsToRun[0],EvQ[m].LocalEventsToRun[1],EvQ[m].LocalEventsToRun[2],EvQ[m].LocalEventsToRun[3],EvQ[m].LocalEventsToRun[4],EvQ[m].LocalEventsToRun[5],EvQ[m].LocalEventsToRun[6],EvQ[m].LocalEventsToRun[7],EvQ[m].LocalEventsToRun[8],EvQ[m].LocalEventsToRun[9]);
                    EvQ[m]->LocalEventsTimeOuts=RE_TIME_REPLAY;								 //Retry timer interval
                    memcpy(&EventBuff[EventBuffSize],EvQ[m]->LocalEventsToRun,EVENT_SIZE);   //Add to send buffer
                    EventBuffSize+=EVENT_SIZE;
                    EventsToRun[EventSize]=m;
                    EventSize++;
                    if (EventSize>=MAX_EVENTS_IN_PACK) break;								//As many events as possible
                    }
            }
        if (devl==202)
            {
            m=4;
            }
        if (devl==201)
            {
            m=3;
            }

        SendTCPEvent(EventBuff,EventSize,devh,devl,EventsToRun);       //Submit via tcp client to desired Ethernet eHouse controller
    }
}
/////////////////////////////////////////////////////////////////////////////////
signed int eHouseTCP::hex2bin(const unsigned char *st,int offset)
 {
 char    i;
 int  tmp=0;
 for (i=0;i<2;i++)
        {
        tmp=tmp<<4;
        switch (st[offset])
                {
                    case '0': tmp=tmp+0;break;
                    case '1': tmp=tmp+1;break;
                    case '2': tmp=tmp+2;break;
                    case '3': tmp=tmp+3;break;
                    case '4': tmp=tmp+4;break;
                    case '5': tmp=tmp+5;break;
                    case '6': tmp=tmp+6;break;
                    case '7': tmp=tmp+7;break;
                    case '8': tmp=tmp+8;break;
                    case '9': tmp=tmp+9;break;
                    case 'A': tmp=tmp+10;break;
                    case 'B': tmp=tmp+11;break;
                    case 'C': tmp=tmp+12;break;
                    case 'D': tmp=tmp+13;break;
                    case 'E': tmp=tmp+14;break;
                    case 'F': tmp=tmp+15;break;
                    case 'a': tmp=tmp+10;break;
                    case 'b': tmp=tmp+11;break;
                    case 'c': tmp=tmp+12;break;
                    case 'd': tmp=tmp+13;break;
                    case 'e': tmp=tmp+14;break;
                    case 'f': tmp=tmp+15;break;                    
            default: return -1;
                }
        offset++;
        }
 return tmp;
 }
 
/////////////////////////////////////////////////////////////////////////////////
//Text Hex Coded Event
void eHouseTCP::AddTextEvents(unsigned char *ev,int size)
{
unsigned char ck=0,  i=0;
unsigned char offset=0;               
int tmp;
unsigned char evnt[EVENT_SIZE+1];
if (size<20) return;              
                while (offset<size-1)
                        {
                        tmp=hex2bin(ev,offset);
                        if (tmp>=0)
                                evnt[i]=(unsigned char)tmp;
                        else 
                                return;
                        i++;
                        if (i==10)
                                {
                                i=0;
                                AddToLocalEvent(evnt,0);
                                }
                        offset+=2;
                        }
    
}



/**
 * AddEvent To Event Queue 
 * param: Even buffer of events - offset shift in buffer to copy
 */


///////////////////////////////////////////////////////////////////////////////////
//  Add Event to Queue for Buffering and Multiple Event Submissions

signed int eHouseTCP::AddToLocalEvent(unsigned char *Even,unsigned char offset)
{ //start
signed int i;
//signed int k;
unsigned char m;
unsigned char Event[EVENT_SIZE+1];
memcpy(Event,(unsigned char *)&Even[offset],EVENT_SIZE); //copy event from selected offset
if (Event[2]==0)  return 0;
if (Event[2]==0xff)  return 0;
//EventsCountInQueue++;
i=GetIndexOfEvent(Event);
m = 0;
_log.Log(LOG_STATUS, "[Add Event]: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", Event[0], Event[1], Event[2], Event[3], Event[4], Event[5], Event[6], Event[7], Event[8], Event[9]);

if (i>=0)       //event already exists in EventQueue
	{
		if (EvQ[i]->LocalEventsTimeOuts>=RUN_NOW)                //Wasn't confirmed of Successful Execution so far
                        {
                        
                        }
		else 	EvQ[i]->LocalEventsTimeOuts=RUN_NOW-1;		//Increase time of event not to disappear from queue if it recently run
                EventsCountInQueue++;
				if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Add Event] Already Exists Q[%d]",i);
		return i;
	}
//Event was not found in event Queue
for (i=0;i<EVENT_QUEUE_MAX;i++)
		{
		if (!EvQ[i]->LocalEventsToRun[2])	// Free space then add
			{
			if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Add Event] Adding Q[%d]",i);
                        memcpy(&EvQ[i]->LocalEventsToRun,Event,EVENT_SIZE);
			EventsCountInQueue++;                   //Event count in queue to perform EventQueue
			EvQ[i]->LocalEventsTimeOuts=RUN_NOW;     //Immediate Run
			EvQ[i]->LocalEventsDrop=0;               //First try
			EvQ[i]->iface=0;                     //local interface not a gateway
                        if (EvQ[i]->LocalEventsToRun[0]==SrvAddrH)
                            EvQ[i]->iface=EV_ETHERNET_EVENT;
                        else EvQ[i]->iface=EV_VIA_EHOUSE_PRO;
                        return i;
                        }
		}
if (DEBUG_TCPCLIENT)  _log.Log(LOG_STATUS, "[Add Event] End of proc");
return -1;
}
