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
#else
#endif
#include "globals.h"
#include "status.h"

 /////////////////////////////////////////////////////////////////////////////////
signed int eHouseTCP::GetIndexOfEvent(unsigned char *TempEvent)
{
	unsigned int i;
	for (i = 0; i < EVENT_QUEUE_MAX; i++)
	{
		if (memcmp(TempEvent, (struct EventQueueT *) &EvQ[i]->LocalEventsToRun, EVENT_SIZE) == 0)
			return i;	//Exist on Event Queue
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::ExecQueuedEvents(void)
{
	unsigned int i;
	unsigned char localy = 1;
	DisablePerformEvent = 0;
	if (!EventsCountInQueue) return;	//nothing availabla - we don't waste time here
	EventsCountInQueue = 0;
	for (i = 0; i < EVENT_QUEUE_MAX; i++)
	{
		if (EvQ[i]->LocalEventsToRun[2])	//not empty event
		{
			EventsCountInQueue++;
			if (EvQ[i]->LocalEventsTimeOuts == RUN_NOW)				//Should be Performed Right Now !!!
			{
				if (EvQ[i]->LocalEventsDrop) _log.Log(LOG_STATUS, "[Perform Queue Event] Q[%d] Now -> Retry: %d", i, EvQ[i]->LocalEventsDrop);
				EvQ[i]->LocalEventsDrop++;						//retries in case of failure
				if (EvQ[i]->LocalEventsDrop > RE_SEND_RETRIES)	//to much retries (no controller or no communication)
				{
					if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Exec Event] Q[%d] Drop Event", i);
					memset(EvQ[i], 0, sizeof(EventQueueT));		//delete event permanently and immediatelly                
				}
				else
				{
					ExecEvent(i);								//Send event to hardware
					break;
				}
			}
			else
				if (EvQ[i]->LocalEventsTimeOuts)			//scheduled event
					EvQ[i]->LocalEventsTimeOuts--;		//decrement timer
				else										//Timer reach 0 - remove event from Queue
				{
					memset(EvQ[i], 0, sizeof(EventQueueT));
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
	unsigned char devh, devl;
	unsigned char EventBuff[255];
	unsigned int EventBuffSize;
	unsigned int EventSize;
	unsigned char EventsToRun[MAX_EVENTS_IN_PACK + 10];
	unsigned int m = 0;
	EventBuffSize = 0;
	EventSize = 0;
	devh = EvQ[i]->LocalEventsToRun[0];                        //get address from DirectEvent
	devl = EvQ[i]->LocalEventsToRun[1];
	int ind = IndexOfEthDev(devh, devl);						//should be transmited directly to controller or not?
	if (ind == 0)											//Via eHouse Pro Server
	{
		//printf("[Exec Event] RS-485, CAN, RF via PRO \r\n");
		int mmm = 0;
		for (m = 0; m < EVENT_QUEUE_MAX; m++)					//for all events in queue
		{
			if ((IndexOfEthDev(devh, devl) == 0) && (EvQ[m]->LocalEventsTimeOuts == RUN_NOW))	//group all events in the queue for the same controller scheduled for sending
			{
				EvQ[m]->LocalEventsTimeOuts = RE_TIME_REPLAY;      //Retry timer interval - next time if failed
				memcpy(&EventBuff[EventBuffSize], EvQ[m]->LocalEventsToRun, EVENT_SIZE);   //Add to send buffer                
		//        printf("[ via PRO ] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n",EvQ[m].LocalEventsToRun[0],EvQ[m].LocalEventsToRun[1],EvQ[m].LocalEventsToRun[2],EvQ[m].LocalEventsToRun[3],EvQ[m].LocalEventsToRun[4],EvQ[m].LocalEventsToRun[5],EvQ[m].LocalEventsToRun[6],EvQ[m].LocalEventsToRun[7],EvQ[m].LocalEventsToRun[8],EvQ[m].LocalEventsToRun[9]);
				EventBuffSize += EVENT_SIZE;
				EventsToRun[EventSize] = m;
				EventSize++;
				if (EventSize >= MAX_EVENTS_IN_PACK) break;       //As many events as possible
			}
		}
		SendTCPEvent(EventBuff, EventSize, SrvAddrH, SrvAddrL, EventsToRun);       //Submit via tcp client to desired Ethernet eHouse controller
		return;
	}
	else
	{
		//  printf("[Exec Event] Ethernet \r\n");
		memset(&EventBuff, 0, sizeof(EventBuff));
		EventSize = 0;
		memset(EventsToRun, 0, sizeof(EventsToRun));
		int mmm = 0;
		for (m = 0; m < EVENT_QUEUE_MAX; m++)
		{
			if ((EvQ[m]->LocalEventsToRun[0] == devh) && (EvQ[m]->LocalEventsToRun[1] == devl) && (EvQ[m]->LocalEventsTimeOuts == RUN_NOW))
			{
				//          printf("[Directly ] %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\r\n",EvQ[m].LocalEventsToRun[0],EvQ[m].LocalEventsToRun[1],EvQ[m].LocalEventsToRun[2],EvQ[m].LocalEventsToRun[3],EvQ[m].LocalEventsToRun[4],EvQ[m].LocalEventsToRun[5],EvQ[m].LocalEventsToRun[6],EvQ[m].LocalEventsToRun[7],EvQ[m].LocalEventsToRun[8],EvQ[m].LocalEventsToRun[9]);
				EvQ[m]->LocalEventsTimeOuts = RE_TIME_REPLAY;								 //Retry timer interval
				memcpy(&EventBuff[EventBuffSize], EvQ[m]->LocalEventsToRun, EVENT_SIZE);   //Add to send buffer
				EventBuffSize += EVENT_SIZE;
				EventsToRun[EventSize] = m;			//We store index of events to control it
				EventSize++;
				if (EventSize >= MAX_EVENTS_IN_PACK) break;								//As many events as possible
			}
		}
		SendTCPEvent(EventBuff, EventSize, devh, devl, EventsToRun);       //Submit via tcp client to desired Ethernet eHouse controller
	}
}
///////////////////////////////////////////////////////////////////////////////////////////
//
//      convert hex string to unsigned char with offset and value checking for text events
//
///////////////////////////////////////////////////////////////////////////////////////////
signed int eHouseTCP::hex2bin(const unsigned char *st, int offset)
{
	char i;
	int  tmp = 0;
	for (i = 0; i < 2; i++)
	{
		tmp = tmp << 4;
		i = st[offset];
		if ((i >= 'A') && (i <= 'F')) tmp += i - 'A' + 10;
		else  if ((i >= 'a') && (i <= 'f')) tmp += i - 'a' + 10;
		else  if ((i >= '0') && (i <= '9')) tmp += i - '0';
		else return -1;
		offset++;
	}
	return tmp;
}

/////////////////////////////////////////////////////////////////////////////////
// add multiple events to event queue - stored in text format with validation
////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::AddTextEvents(unsigned char *ev, int size)
{
	unsigned char i = 0;
	unsigned char offset = 0;
	unsigned char evnt[EVENT_SIZE + 1];
	if (size < 20) return;						//ignore if tho short
	while (offset < size - 1)
	{
		int tmp = hex2bin(ev, offset);			// decode Text Hex Coded Event
		if (tmp >= 0)						// valid hex char
			evnt[i] = (unsigned char)tmp;	// construct binary
		else
			return;		//ignore if wrong data (non hex digit)
		i++;
		if (i == 10)		//only if valid eHouse text event 10 binary = 20 hex char event
		{
			i = 0;
			AddToLocalEvent(evnt, 0);	//add to queue
		}
		offset += 2;
	}
}



/**
 * AddEvent To Event Queue
 * param: Even buffer of events - offset shift in buffer to copy
 */


 ///////////////////////////////////////////////////////////////////////////////////
 //  Add Event to Queue for Buffering and Multiple Event Submissions - binary mode
 //////////////////////////////////////////////////////////////////////////////////
signed int eHouseTCP::AddToLocalEvent(unsigned char *Even, unsigned char offset)
{ //start
	signed int i;
	//signed int k;
	unsigned char Event[EVENT_SIZE + 1];
	memcpy(Event, (unsigned char *)&Even[offset], EVENT_SIZE); //copy event from selected offset
	if (Event[2] == 0)  return 0;
	if (Event[2] == 0xff)  return 0;
	i = GetIndexOfEvent(Event);
	_log.Log(LOG_STATUS, "[Add Event]: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", Event[0], Event[1], Event[2], Event[3], Event[4], Event[5], Event[6], Event[7], Event[8], Event[9]);

	if (i >= 0)       //event already exists in EventQueue
	{
		if (EvQ[i]->LocalEventsTimeOuts >= RUN_NOW)                //Wasn't confirmed - No Successful Execution so far
		{

		}
		else 	EvQ[i]->LocalEventsTimeOuts = RUN_NOW - 1;		//Increase time of event not to disappear from queue if it recently run
		EventsCountInQueue++;
		if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Add Event] Already Exists Q[%d]", i);
		return i;
	}
	//Event was not found in event Queue
	for (i = 0; i < EVENT_QUEUE_MAX; i++)
	{
		if (!EvQ[i]->LocalEventsToRun[2])	// Free space then add
		{
			if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[Add Event] Adding Q[%d]", i);
			memcpy(&EvQ[i]->LocalEventsToRun, Event, EVENT_SIZE);
			EventsCountInQueue++;                     //Event count in queue to perform EventQueue
			EvQ[i]->LocalEventsTimeOuts = RUN_NOW;     //Immediate Run
			EvQ[i]->LocalEventsDrop = 0;               //First try
			EvQ[i]->iface = 0;                         //local interface not a gateway
			if (EvQ[i]->LocalEventsToRun[0] == SrvAddrH)
				EvQ[i]->iface = EV_ETHERNET_EVENT;
			else EvQ[i]->iface = EV_VIA_EHOUSE_PRO;
			return i;
		}
	}
	if (DEBUG_TCPCLIENT)  _log.Log(LOG_STATUS, "[Add Event] End of proc");
	return -1;
}
