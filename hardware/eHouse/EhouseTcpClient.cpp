/*
 * File:   TcpClient.c
 * ONLY for usage with eHouse system controllers | Intellectual property of iSys.Pl | Any other usage, copy etc. is forbidden 
 * eHouse4Ethernet Server - described in UdpListener.c
 * eHouse1 (RS-485) Server  //described in rs485.c
 * eHouse1 under CommManager Supervision Server - described in UdpListener.c
 * TCP Client for submitting events to Ethernet eHouse Controllers and eHouse 1 via CommManager in case of eHouse under CM supervision version
 * 
 * Author: Robert Jarzabek, iSys.Pl
 * http://www.isys.pl/          Inteligentny Dom eHouse - strona producenta
 * http://www.eHouse.pro/       Automatyka Budynku
 * http://sterowanie.biz/       Sterowanie Domem, Budynkiem ...
 * http://inteligentny-dom.ehouse.pro/  Inteligentny Dom eHouse Samodzielny montaż, zrób to sam, przykłady, projekty, programowanie
 * http://home-automation.isys.pl/      eHouse Home automation producer home page
 * http://home-automation.ehouse.pro/   eHouse Home Automation - Do it yourself, programming examples, designing
 * Created on June 5, 2013, 10:00 AM
 * Updated 2017-11-09
 * globals.h - global configuration / main settings
 * most important params 
 * 
 */

//#include <iostream>
#include "stdafx.h"
#include "../main/Logger.h"
#include<stdio.h> //printf
#include <time.h>
#include<string.h>    //strlen
#include <sys/types.h>
#include "../eHouseTCP.h"
#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h> 
#include<netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#else
//#include <winapi.h>
#include <Windows.h>
#include <winsock.h>
#include <winsock2.h>

#endif

#include "globals.h"
#include "status.h"
unsigned char SrvAddrH = 0, SrvAddrL = 200, SrvAddrU=192, SrvAddrM=168;
unsigned char EHOUSE_TCP_CLIENT_TIMEOUT = 0;
unsigned int EHOUSE_TCP_CLIENT_TIMEOUT_US = 100000;
int EHOUSE_TCP_PORT = 9876;
unsigned char DEBUG_TCPCLIENT = 0;




#include <errno.h>
//#include <exception>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


#include <pthread.h>
#define USE_GOTO 1              //Less stack utilization and faster execution & more efficient
#ifdef USE_GOTO                 //Maybe less elegant but better :)
#define eHTerminate(x) {goto terminate;}
#else
void KillSocket(int SocketIndex);       //Function if you dont like GOTO instruction
#define eHTerminate(x) {KillSocket(x);return;}
#endif
//signed int eHouseTCP::GetIndexOfWiFiDev(unsigned char AddrH,unsigned char AddrL);
//TCP Client Connection to Ethernet eHouse controllers - structure
typedef struct
        {
        int Socket;                             //TCP Client Sockets for paralel operations
        unsigned char Events[255u];             //Event buffer for current socket
        //unsigned char TimeOut;                //TimeOut for current client connection in 0.1s require external thread or non blocking socket
        //Active connections to Ehouse Controllers to avoid multiple connection to the same device
        signed int ActiveConnections;        //index of status matrix eHE[] or ETHERNET_EHOUSE_RM_MAX for CM
        unsigned char AddrH;                    //destination IP byte 3
        unsigned char AddrL;                    //destination IP byte 4
        unsigned char EventSize;                //size of event to submit
        unsigned char EventsToRun[MAX_EVENTS_IN_PACK];            //Events to send in One Pack
        unsigned char OK;
        unsigned char NotFinished;
//        unsigned char Stat;                   //Status of client
        } TcpClientCon;
//#define         TC_NOT_CONNECTED 0
//#define         TC_CONNECTED     1
TcpClientCon    TC[MAX_CLIENT_SOCKETS];    //TCP Client Instances in case of multi-threading
#ifdef EHOUSE_TCP_CLIENT_THREAD
pthread_t       EhouseTcpClientThread[MAX_CLIENT_SOCKETS];            //TCP ClientThread instances in case of multithread
//thdata 
//int *EhouseTcpClientThreadParam[MAX_CLIENT_SOCKETS];       //TCP ClientThread instances params
#endif
/////////////////////////////////////////////////////////////////////////////////////////
/* 
 * Initialize Client Socket To Set Free / Invalid at the start of program
 * Preconditions none, Perform once on start of application
 */
void eHouseTCP::EhouseInitTcpClient(void)
{
    unsigned char i=0;
    for (i=0;i<MAX_CLIENT_SOCKETS;i++)
        {
        memset((void *)&TC[i]/*.Events*/,0,sizeof(TC[i]/*.Events*/));
        TC[i].Socket=-1;
        TC[i].ActiveConnections=-1;
        }
}
////////////////////////////////////////////////////////////////////////////////
/*
 Check index of  for Ethernet eHouse device by addres h, l bytes
 * returns index of status array eHE or -1 if not found
 */
/*signed int IndexOfEthDev(unsigned char AddrH,unsigned char AddrL)
{
    unsigned char i=0;
if ((ECM.CM.AddrH==AddrH) && (ECM.CM.AddrL==AddrL)) return ETHERNET_EHOUSE_RM_MAX;
for (i=0;i<ETHERNET_EHOUSE_RM_MAX;i++)
        {
        if ((eHE[i].CM.AddrH==AddrH) && (eHE[i].CM.AddrL==AddrL)) return i;            
        }
return -1;
}*/
////////////////////////////////////////////////////////////////////////////////
/*signed int GetIndexOfWiFiDev(unsigned char AddrH,unsigned char AddrL)
{
    unsigned char i=0;
for (i=0;i<EHOUSE_WIFI_MAX;i++)
        {
        if ((eHWiFi[i].status.AddrH==AddrH) && (eHWiFi[i].status.AddrL==AddrL)) return i;            
        }
return -1;
}*/
////////////////////////////////////////////////////////////////////////////////
/* Initialize event/events for Submission for one eHouse Ethernet controller
 * 
 * 
 * @param Events - string contains Events stored 1 by 1 (10B for each event) 
 * @param EventCount - count of Events in buffer
 * @param AddrH - Target host IP 3 byte for Ethernet controllers direct / for eHouse1/Can via CM or eHouse Pro Server
 * @param AddrL - Target host IP 4 byte for Ethernet controllers direct / for eHouse1/CAN via eHouse Pro Server
 * @return - 0 if OK, negative value in case of some errors (all sockets busy, target controller already connected)
 */
/*char SubmitEvent(const unsigned char *Events, unsigned char EventCount)
{
    unsigned char Event[255];
    memset(Event,0,sizeof(Event));
    memcpy(Event,Events,EventCount*EVENT_SIZE);
    if (Event[0]!=SrvAddrH) //other than lan, wifi
        return SendTCPEvent((unsigned char *)&Event,EventCount,SrvAddrH,SrvAddrL,(unsigned char *)&Event);
    else
        return SendTCPEvent((unsigned char *)&Event,EventCount,Event[0],Event[1],(unsigned char *)&Event);
        
}*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define INDEX_PRO  0
#define INDEX_ETH  1
#define INDEX_WIFI 100
signed int eHouseTCP::IndexOfEthDev(unsigned char AddrH,unsigned char AddrL)
{
    unsigned char i=0;
	if (ViaTCP)
		{
		if  ((SrvAddrU != 192) || (SrvAddrM != 168)) return INDEX_PRO;
		//if  ((SrvAddrU != 192) || (SrvAddrM != 168)) return INDEX_PRO;
		}
    if ((SrvAddrH==AddrH) && (SrvAddrL==AddrL)) return INDEX_PRO;

	

for (i=0;i<ETHERNET_EHOUSE_RM_MAX;i++)
        {
		if (strlen((char *)&eHEn[i]) > 0) 
			if ((eHERMs[i]->eHERM.AddrH==AddrH) && (eHERMs[i]->eHERM.AddrL==AddrL)) 
                return INDEX_ETH+i;            
        }
for (i=0;i<EHOUSE_WIFI_MAX;i++)
        {
		if (strlen((char *)&eHWIFIn[i]) > 0)
			if ((eHWiFi[i]->status.AddrH==AddrH) && (eHWiFi[i]->status.AddrL==AddrL)) return INDEX_WIFI+i;            
        }
    
return INDEX_PRO;
}
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////
char eHouseTCP::SendTCPEvent(const unsigned char *Events,unsigned char EventCount,unsigned char AddrH,unsigned char AddrL,const unsigned char *EventsToRun)
{
int tcp_client_socket_index;
unsigned char i;
int ind;
ind=IndexOfEthDev(AddrH,AddrL);     //get index of Ethernet Controller (the same as status array index except CM)
tcp_client_socket_index=-1;
for (i=0;i<MAX_CLIENT_SOCKETS;i++)     //search first free socket (TCP Client instance)
        {
        if (ind>=0)                             //status exists in eHE we have index
            if (TC[i].ActiveConnections==ind)
            {  
//              printf("Already sending: 192.168.%d.%d\r\n",AddrH,AddrL);
                return -2;     //!!!! still connected to specified controller
            }
    if (TC[i].ActiveConnections>=0) continue;
    if (TC[i].NotFinished) continue;
    if ((TC[i].Socket==-1) & (tcp_client_socket_index<0))  //free socket exists & only once
                {
                tcp_client_socket_index=i;      //we take it if not connected to this ip addr
                break;
                }
        }
     if (tcp_client_socket_index<0)
	return -1;  //No free socket available (TCP Client instance) will be processed later
    //Can Proceed then submit to TCPClient queue
    TC[tcp_client_socket_index].NotFinished=1;          //Not Complete flag for thread garbage in case of error
    TC[tcp_client_socket_index].OK=0;
    TC[tcp_client_socket_index].ActiveConnections=ind; //set active connection for current IP
    TC[tcp_client_socket_index].AddrH=AddrH;                            //target address for TCP Client H
    TC[tcp_client_socket_index].AddrL=AddrL;                            //target address for TCP Client L
    TC[tcp_client_socket_index].EventSize=EventCount;        //size of buffer
    //TC[tcp_client_socket_index].TimeOut=20;           //not used here require external thread to decrement timer
                       
    //using socket timeout - verify in desired system if it act properly with Connect() function
                                                        //depends on implementation
    //TC[tcp_client_socket_index].Stat=TC_NOT_CONNECTED;//is this necessary 
    memcpy((void *)&TC[tcp_client_socket_index].Events,Events,TC[tcp_client_socket_index].EventSize*EVENT_SIZE);  //copy Events to buffer for thread or SubmitData function
    memcpy((void *)&TC[tcp_client_socket_index].EventsToRun,EventsToRun,EventCount);
    int param=tcp_client_socket_index;
#ifndef EHOUSE_TCP_CLIENT_THREAD
    SubmitData(tcp_client_socket_index);
#else
//printf("Send TCP Thread Ind:%d,  TCPC:%d\r\n",ind,tcp_client_socket_index);    
    pthread_create(
            &EhouseTcpClientThread[tcp_client_socket_index],
            NULL,
            EhouseSubmitData,
            &param
            );
       pthread_detach(EhouseTcpClientThread[tcp_client_socket_index]);
       msl(10);
       ExecQueuedEvents();
#endif
    return 0;
}
/*
void TestTCPClientMultiThread(void)
{
    unsigned char tcp_client_socket_index=0;
    TC[tcp_client_socket_index].AddrH=0;
    TC[tcp_client_socket_index].AddrL=254;
    TC[tcp_client_socket_index].EventSize=1;
    TC[tcp_client_socket_index].
    pthread_create(
            &TcpClientThread[tcp_client_socket_index],
            NULL,
            SubmitData,
            (void *)&TcpClientThreadParam[tcp_client_socket_index]);
}

*/
/* Perform complete submission of data via TCPClient to Ethernet eHouse Controller
 * Uses Xor Password authorization with dynamic challenge code received from server
 * Verify in target operating system if timeout is supported by the platform especially during Connect
 * TCP_CLIENT_TIMEOUT,TCP_CLIENT_TIMEOUT_US is set in globals.h for proper timeout values (default 1sec)
 * PassWord definition contains password for challange response (6B)
 * VendorCode "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0" defined for additional hashing (6B) - do not modify
 * 
 * @param SocketIndex - Free socket index to pass (TCP Client Instance)
 */
void eHouseTCP::performTCPClientThreads()
{
    int i=0;
for (i=0;i<MAX_CLIENT_SOCKETS;i++)
  if (TC[i].NotFinished>1)
    {
    //if (EhouseTcpClientThread[i]) pthread_join(EhouseTcpClientThread[i],NULL);
    TC[i].NotFinished=0;
    TC[i].Socket=-1;
    TC[i].OK=0;
    TC[i].ActiveConnections=-1;
    }
}
////////////////////////////////////////////////////////////////////////////////
char VendorCode[6]={0,0,0,0,0,0};
char PassWord[6]="";
#ifndef EHOUSE_TCP_CLIENT_THREAD
void EhouseSubmitData(int SocketIndex)
#else
void *EhouseSubmitData(void *ptr)
#endif
        {
#ifdef EHOUSE_TCP_CLIENT_THREAD
    int SocketIndex=*(int *)ptr;
    if (SocketIndex>=MAX_CLIENT_SOCKETS)
        {        
		_log.Log(LOG_STATUS, "[eHouse TCP Client] Too many sockets");
        pthread_exit(0);
        }
    TcpClientCon    *ClientCon=&TC[SocketIndex];
    
#endif    
  //      unsigned char iters=5;
//ReTRYSubmission:        
        struct timeval timeout;      
        timeout.tv_sec = EHOUSE_TCP_CLIENT_TIMEOUT;    //Timeout for socket set in globals.h
        timeout.tv_usec = EHOUSE_TCP_CLIENT_TIMEOUT_US;
        struct sockaddr_in server;
        char line[20];
        unsigned char challange[255];
        int status=0;
        int iter;
        memset(&server,0,sizeof(server));               //clear server structure
        memset(&challange,0,sizeof(challange));         //clear buffer
        sprintf(line,"%d.%d.%d.%d",SrvAddrU,SrvAddrM,ClientCon->AddrH,ClientCon->AddrL);
        ClientCon->Socket = socket(AF_INET , SOCK_STREAM , 0);
        
        if (ClientCon->Socket < 0)       //Check if socket was created
                {
			_log.Log(LOG_ERROR, "[TCP Cli %d] Could not create socket",SocketIndex);
                ClientCon->NotFinished++;
#ifndef EHOUSE_TCP_CLIENT_THREAD                
                return ;                              //!!!! Counldn't Create Socket
#else
                pthread_exit(0);
#endif                
                }
                
        if (setsockopt (TC[SocketIndex].Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)   //Set socket Read operation Timeout
                LOG(LOG_STATUS,"[TCP Cli] Set Read Timeout failed\n");

         if (setsockopt (TC[SocketIndex].Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)   //Set Socket Write operation Timeout
                LOG(LOG_STATUS,"[TCP Cli] Set Write Timeout failed\n");
/*        linger li;
        li.l_linger=0;
        li.l_onoff=0;   //off
        if (setsockopt (TC[SocketIndex].Socket, SOL_SOCKET, SO_LINGER, &li,
                sizeof(li)) < 0)        //Set Socket linger off - close imediatelly
                perror("[TCP Cli] Set Linger failed\n");
        */
        //TCP_NODELAYACK
        long kkk=1L;
        status=1L;
/*        if (setsockopt (TC[SocketIndex].Socket, IPPROTO_TCP, TCP_NODELAY, &kkk,
                sizeof(status)) < 0)   //Set socket send data imediatelly
                perror("[TCP Cli] Set TCP No Delay failed\n");*/
								///0xa8c0ull
        server.sin_addr.s_addr= SrvAddrU | (SrvAddrM<<8)    |(ClientCon->AddrH<<16)|(ClientCon->AddrL<<24); //target ip address //inet_addr(line);        //
        server.sin_family = AF_INET;                    //tcp v4
        server.sin_port = htons(EHOUSE_TCP_PORT);       //assign eHouse Port
		if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Trying Connecting to: %s",SocketIndex,line);
        if (connect(ClientCon->Socket , (struct sockaddr *)&server , sizeof(server)) < 0)
                {
				_log.Log(LOG_ERROR, "[TCP Cli %d] error connecting: %s",SocketIndex,line);
                eHTerminate(SocketIndex)
                }
		if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Connected OK",SocketIndex);
//        TC[SocketIndex].Stat=TC_NOT_CONNECTED;//is this necessary 
        iter=5;
        while ((status=recv(ClientCon->Socket,(char *)&challange,6,0))<6)       //receive challenge code
                {
				//_log.Log(LOG_STATUS, "[TCP Cli %d] Not Receive Complete Data: %d",SocketIndex,status);
                if (status<0) eHTerminate(SocketIndex)    //Error in socket
                if (!(iter--)) eHTerminate(SocketIndex)       //To many retries then Close
//                if (TC[SocketIndex].TimeOut==0) eHTerminate(SocketIndex) //Use socket timeouts
                }
        if (status==6)  //challenge received frcom Ethernet eHouse controller
                {       //only Hashed password with VendorCode available for OpenSource
                        {
						//_log.Log(LOG_STATUS, "[TCP Cli %d] Xored Password",SocketIndex);
                        challange[6]= (challange[0]^PassWord[0]^VendorCode[0]);
                        challange[7]= (challange[1]^PassWord[1]^VendorCode[1]);
                        challange[8]= (challange[2]^PassWord[2]^VendorCode[2]);
                        challange[9]= (challange[3]^PassWord[3]^VendorCode[3]);
                        challange[10]=(challange[4]^PassWord[4]^VendorCode[4]);
                        challange[11]=(challange[5]^PassWord[5]^VendorCode[5]);
                        }                
                challange[12]=13;
                challange[13]=ClientCon->EventSize*EVENT_SIZE;        //Add Size of Events Buffer
                memcpy(&challange[14],(char *)&ClientCon->Events,ClientCon->EventSize*EVENT_SIZE);        //Add Event Code
                }
        else eHTerminate(SocketIndex)    //Wrong data size of received data
			if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Sending ch-re",SocketIndex);
        
        //Send Challange + response + Events    - Only Xor password
        status=0;
        iter=5;
        while ((status=send(ClientCon->Socket,(char *)&challange,(ClientCon->EventSize*EVENT_SIZE)+14,0))!=(int)(ClientCon->EventSize*EVENT_SIZE)+14)
              {
			//_log.Log(LOG_STATUS, "[TCP Cli %d] NotSend Complete Data: %d",SocketIndex,status);
//                if (!TC[SocketIndex].TimeOut) eHTerminate(SocketIndex)  //not used here socket timeouts used
                if (!(iter--)) eHTerminate(SocketIndex)       //To many retries then Close        
                if (status<0) eHTerminate(SocketIndex)        //Error in connection
                }
		if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Receive Confirmation",SocketIndex);
        challange[0]=0;         //clear one byte is sufficient
        iter=5;
        while ((status=recv(ClientCon->Socket,(char *)&challange,1,0))<1)   //receive confirmation of events
                {
                if (status<0) eHTerminate(SocketIndex)                    //error in connection
//                if (!TC[SocketIndex].TimeOut) eHTerminate(SocketIndex)  //using socket timeouts
                if (!(iter--)) eHTerminate(SocketIndex)                   //To many retries so Close                        
                }
        if (challange[0]=='+')  //confirmation from Ethernet eHouse controller
                {
				_log.Log(LOG_STATUS, "[TCP Cli %d] Events Accepted",SocketIndex);
                unsigned char k=0;
                for (k=0;k< ClientCon->EventSize;k++)
                        {
                        unsigned int m=ClientCon->EventsToRun[k];
                        if (m<EVENT_QUEUE_MAX)
                                {
                                EvQ[m]->LocalEventsTimeOuts=RUN_NOW-1;
                                ClientCon->OK=1;
                                }
                        //TC[SocketIndex].EventsToRun[k]=0;
                        }
                //ClientCon->
                }
        else
                {
				_log.Log(LOG_STATUS, "[TCP Cli %d] No Confirmation",SocketIndex);
                }
        //Trying to Send termination of socket (just in case)
        challange[0]=0; //byte = 0 is termination of connection request in this stage
		if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Sending termination of connection",SocketIndex);
        if ((status=send(ClientCon->Socket,(char *)&challange,1,0))!=1)
                {
			if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Not sent termination (No Problem)",SocketIndex);
                if (status<0) eHTerminate(SocketIndex)
                }
		if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Termination Sent OK",SocketIndex);
#ifdef USE_GOTO        
// Terminate connection and initialize TCP Client Socket for next operations
        terminate:      //Could be replace by KillSocket function and return uncomment definition USE_GOTO
#else
        KillSocket(SocketIndex);
}
void KillSocket(int SocketIndex)
        {       

#endif                
				if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Closing Connection",SocketIndex);
                memset((void *)&ClientCon->Events,0,sizeof(ClientCon->Events));  //copy Events to buffer for thread
                memset((void *)&ClientCon->EventsToRun,0,sizeof(ClientCon->EventsToRun));
                ClientCon->EventSize=0;
#ifndef WIN32
                close(ClientCon->Socket);          //close socket handle
#else
				closesocket(ClientCon->Socket);          //close socket handle
#endif
                ClientCon->Socket=-1;              //reset TCP client
                //ClientCon->ActiveConnections=-1;   //set active connection from index of eHE[]
                ClientCon->NotFinished++;
                if ((SocketIndex<0)  || (SocketIndex>=MAX_CLIENT_SOCKETS))
#ifndef EHOUSE_TCP_CLIENT_THREAD
                    return;                      //if invalid socket manipulated somewere else 
#else
                pthread_exit(0);
#endif                        
                
                
				if (DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] End TCP Client",SocketIndex);
#ifdef EHOUSE_TCP_CLIENT_THREAD

                pthread_exit(0);
#endif                
//                TC[SocketIndex].TimeOut=0;    //used socket timeouts 
                
                
				_log.Log(LOG_STATUS, "[TCP Cli||||| %d] End TCP Client",SocketIndex);
return 0;
        }

