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
 Recent update: 2018-08-16
 */

#include "stdafx.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../eHouseTCP.h"
#ifndef WIN32
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "globals.h"
#include "status.h"

#define USE_GOTO 1              //Lesser stack utilization and faster execution & more efficient
#ifdef USE_GOTO                 //Maybe less elegant but better :)
#define eHTerminate(x) {goto terminate;}
#else

#define eHTerminate(x) {KillSocket(x);return;}
#endif
 /////////////////////////////////////////////////////////////////////////////////////////
 /*
  * Initialize Client Socket To Set Free / Invalid at the start of program
  * Preconditions none, Perform once on start of application
  */
void eHouseTCP::EhouseInitTcpClient(void)
{
	unsigned char i = 0;
	for (i = 0; i < MAX_CLIENT_SOCKETS; i++)
	{
		memset((void *)&m_TC[i], 0, sizeof(m_TC[i]));
		m_TC[i].Socket = -1;
		m_TC[i].ActiveConnections = -1;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Submit binary pack of events

char eHouseTCP::SubmitEvent(const unsigned char *Events, unsigned char EventCount)
{
	unsigned char Event[255];
	memset(Event, 0, sizeof(Event));
	memcpy(Event, Events, EventCount * EVENT_SIZE);
	if (Event[0] != m_SrvAddrH) //other than lan, wifi
		return SendTCPEvent((unsigned char *)&Event, EventCount, m_SrvAddrH, m_SrvAddrL, (unsigned char *)&Event);	// Via eHouse PRO Server
	else
		return SendTCPEvent((unsigned char *)&Event, EventCount, Event[0], Event[1], (unsigned char *)&Event);	// Directly to IP controllers
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define INDEX_PRO  0
#define INDEX_ETH  1
#define INDEX_WIFI 100
signed int eHouseTCP::IndexOfEthDev(unsigned char AddrH, unsigned char AddrL)
{
	unsigned char i = 0;
	if (m_ViaTCP)
	{
		if ((m_SrvAddrU != 192) || (m_SrvAddrM != 168)) return INDEX_PRO;
	}

	if ((m_SrvAddrH == AddrH) && (m_SrvAddrL == AddrL)) return INDEX_PRO;

	for (i = 0; i < ETHERNET_EHOUSE_RM_MAX; i++)
	{
		if (strlen((char *)&m_eHEn[i]) > 0)
			if ((m_eHERMs[i]->eHERM.AddrH == AddrH) && (m_eHERMs[i]->eHERM.AddrL == AddrL))
				return INDEX_ETH + i;
	}

	for (i = 0; i < EHOUSE_WIFI_MAX; i++)
	{
		if (strlen((char *)&m_eHWIFIn[i]) > 0)
			if ((m_eHWiFi[i]->status.AddrH == AddrH) && (m_eHWiFi[i]->status.AddrL == AddrL)) return INDEX_WIFI + i;
	}

	return INDEX_PRO;
}
////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
char eHouseTCP::SendTCPEvent(const unsigned char *Events, unsigned char EventCount, unsigned char AddrH, unsigned char AddrL, const unsigned char *EventsToRun)
{
	int tcp_client_socket_index;
	unsigned char i;
	int ind;
	ind = IndexOfEthDev(AddrH, AddrL);     //get index of Ethernet Controller (the same as status array index except CM)
	tcp_client_socket_index = -1;
	for (i = 0; i < MAX_CLIENT_SOCKETS; i++)     //search first free socket (TCP Client instance)
	{
		if (ind >= 0)                             //status exists in eHE we have index
			if (m_TC[i].ActiveConnections == ind)
			{
				//              printf("Already sending: 192.168.%d.%d\r\n",AddrH,AddrL);
				return -2;     //!!!! still connected to specified controller
			}
		if (m_TC[i].ActiveConnections >= 0) continue;
		if (m_TC[i].NotFinished) continue;
		if ((m_TC[i].Socket == -1) & (tcp_client_socket_index < 0))  //free socket exists & only once
		{
			tcp_client_socket_index = i;      //we take it if not connected to this ip addr
			break;
		}
	}
	if (tcp_client_socket_index < 0)
		return -1;  //No free socket available (TCP Client instance) will be processed later
//Can Proceed then submit to TCPClient queue
	m_TC[tcp_client_socket_index].NotFinished = 90; //~30s         //Not Complete flag for thread garbage in case of error
	m_TC[tcp_client_socket_index].OK = 0;
	m_TC[tcp_client_socket_index].ActiveConnections = ind; //set active connection for current IP
	m_TC[tcp_client_socket_index].AddrH = AddrH;                            //target address for TCP Client H
	m_TC[tcp_client_socket_index].AddrL = AddrL;                            //target address for TCP Client L
	m_TC[tcp_client_socket_index].EventSize = EventCount;        //size of buffer
	//TC[tcp_client_socket_index].TimeOut = 20;           //not used here require external thread to decrement timer

	//using socket timeout - verify in desired system if it act properly with Connect() function
														//depends on implementation
	//TC[tcp_client_socket_index].Stat = TC_NOT_CONNECTED;//is this necessary
	memcpy((void *)&m_TC[tcp_client_socket_index].Events, Events, m_TC[tcp_client_socket_index].EventSize * EVENT_SIZE);  //copy Events to buffer for thread or SubmitData function
	memcpy((void *)&m_TC[tcp_client_socket_index].EventsToRun, EventsToRun, EventCount);
	//int param = tcp_client_socket_index;
#ifndef EHOUSE_TCP_CLIENT_THREAD
	EhouseSubmitData(tcp_client_socket_index);
	ExecQueuedEvents();
#else
	m_EhouseTcpClientThread[tcp_client_socket_index] = std::make_shared<std::thread>(&eHouseTCP::EhouseSubmitData, this, tcp_client_socket_index);
	SetThreadNameInt(m_EhouseTcpClientThread[tcp_client_socket_index]->native_handle());

	m_EhouseTcpClientThread[tcp_client_socket_index]->detach();
	sleep_milliseconds(100);
	ExecQueuedEvents();

#endif
	return 0;
}

/* Finalize TCP IP Client Sockets
 */
void eHouseTCP::performTCPClientThreads()
{
	int i = 0;
	for (i = 0; i < MAX_CLIENT_SOCKETS; i++)
		if ((m_TC[i].NotFinished) > 0)
		{
			m_TC[i].NotFinished--;
			if (m_TC[i].NotFinished) continue;
			/*if (EhouseTcpClientThread[i] -> joinable())
			{
				EhouseTcpClientThread[i]->try_join_for()
					join();
				//EhouseTcpClientThread[i] = NULL;
			}*/
			m_TC[i].NotFinished = 0;
			m_TC[i].Socket = -1;
			m_TC[i].OK = 0;
			m_TC[i].ActiveConnections = -1;

		}
}
////////////////////////////////////////////////////////////////////////////////
#ifndef EHOUSE_TCP_CLIENT_THREAD
void eHouseTCP::EhouseSubmitData(int SocketIndex)
#else
void eHouseTCP::EhouseSubmitData(int SocketIndex)
#endif
{
#ifdef EHOUSE_TCP_CLIENT_THREAD
	if (SocketIndex >= MAX_CLIENT_SOCKETS)
	{
		_log.Log(LOG_ERROR, "[eHouse TCP Client] Too many sockets");
		return;
	}
#endif
	TcpClientCon    *ClientCon = &m_TC[SocketIndex];
	//      unsigned char iters=5;
  //ReTRYSubmission:

#ifndef WIN32
	struct timeval timeout;
	timeout.tv_sec = m_EHOUSE_TCP_CLIENT_TIMEOUT;    //Timeout for socket set in globals.h
	timeout.tv_usec = m_EHOUSE_TCP_CLIENT_TIMEOUT_US;
#else
	int timeout = 20000;
#endif

	struct sockaddr_in server;
	char line[20];
	unsigned char challange[255];
	int status = 0;
	int iter;
	memset(&server, 0, sizeof(server));               //clear server structure
	memset(&challange, 0, sizeof(challange));         //clear buffer
	sprintf(line, "%d.%d.%d.%d", m_SrvAddrU, m_SrvAddrM, ClientCon->AddrH, ClientCon->AddrL);
	ClientCon->Socket = socket(AF_INET, SOCK_STREAM, 0);

	if (ClientCon->Socket < 0)       //Check if socket was created
	{
		_log.Log(LOG_ERROR, "[TCP Cli %d] Could not create socket", SocketIndex);
		ClientCon->NotFinished++;
		return;
	}

	if (setsockopt(ClientCon->Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
		sizeof(timeout)) < 0)   //Set socket Read operation Timeout
		LOG(LOG_ERROR, "[TCP Cli] Set Read Timeout failed");

	if (setsockopt(ClientCon->Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
		sizeof(timeout)) < 0)   //Set Socket Write operation Timeout
		LOG(LOG_ERROR, "[TCP Cli] Set Write Timeout failed");
	//TCP_NODELAYACK
	char kkk = 1;
	status = 1L;
	if (setsockopt(ClientCon->Socket, IPPROTO_TCP, TCP_NODELAY, &kkk, sizeof(kkk)) < 0)   //Set socket send data immediately
	{
		//_log.Log(LOG_ERROR, "[TCP Cli %d] Cant Set TCP NODELAY", SocketIndex);
	}
	server.sin_addr.s_addr = m_SrvAddrU | (m_SrvAddrM << 8) | (ClientCon->AddrH << 16) | (ClientCon->AddrL << 24);
	server.sin_family = AF_INET;                    //tcp v4
	server.sin_port = htons((uint16_t)m_EHOUSE_TCP_PORT);       //assign eHouse Port
	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Connecting to: %s", SocketIndex, line);
	if (connect(ClientCon->Socket, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		_log.Log(LOG_ERROR, "[TCP Cli %d] error connecting: %s", SocketIndex, line);
		eHTerminate(SocketIndex)
	}
	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Authorizing", SocketIndex);
	//        TC[SocketIndex].Stat=TC_NOT_CONNECTED;//is this necessary
	iter = 5;
	while ((status = recv(ClientCon->Socket, (char *)&challange, 6, 0)) < 6)       //receive challenge code
	{
		//_log.Log(LOG_STATUS, "[TCP Cli %d] Not Receive Complete Data: %d",SocketIndex,status);
		if (status < 0) eHTerminate(SocketIndex)    //Error in socket
			if (!(iter--)) eHTerminate(SocketIndex)       //To many retries then Close
//                if (TC[SocketIndex].TimeOut==0) eHTerminate(SocketIndex) //Use socket timeouts
	}
	if (status == 6)  //challenge received frcom Ethernet eHouse controller
	{       //only Hashed password with VendorCode available for OpenSource
		{
			//_log.Log(LOG_STATUS, "[TCP Cli %d] Xored Password",SocketIndex);
			challange[6] = (challange[0] ^ m_PassWord[0] ^ m_VendorCode[0]);
			challange[7] = (challange[1] ^ m_PassWord[1] ^ m_VendorCode[1]);
			challange[8] = (challange[2] ^ m_PassWord[2] ^ m_VendorCode[2]);
			challange[9] = (challange[3] ^ m_PassWord[3] ^ m_VendorCode[3]);
			challange[10] = (challange[4] ^ m_PassWord[4] ^ m_VendorCode[4]);
			challange[11] = (challange[5] ^ m_PassWord[5] ^ m_VendorCode[5]);
		}
		challange[12] = 13;
		challange[13] = ClientCon->EventSize * EVENT_SIZE;        //Add Size of Events Buffer
		memcpy(&challange[14], (char *)&ClientCon->Events, ClientCon->EventSize * EVENT_SIZE);        //Add Event Code
	}
	else eHTerminate(SocketIndex)    //Wrong data size of received data
		if (m_DEBUG_TCPCLIENT) _log.Debug(DEBUG_HARDWARE, "[TCP Cli %d] Sending ch-re", SocketIndex);

	//Send Challange + response + Events    - Only Xor password
	status = 0;
	iter = 5;
	while ((status = send(ClientCon->Socket, (char *)&challange, (ClientCon->EventSize * EVENT_SIZE) + 14, 0)) != (int)(ClientCon->EventSize * EVENT_SIZE) + 14)
	{
		//_log.Log(LOG_STATUS, "[TCP Cli %d] NotSend Complete Data: %d",SocketIndex,status);
//                if (!TC[SocketIndex].TimeOut) eHTerminate(SocketIndex)  //not used here socket timeouts used
		if (!(iter--)) eHTerminate(SocketIndex)       //To many retries then Close
			if (status < 0) eHTerminate(SocketIndex)        //Error in connection
	}
	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Receive Confirmation", SocketIndex);
	challange[0] = 0;         //clear one byte is sufficient
	iter = 5;
	while ((status = recv(ClientCon->Socket, (char *)&challange, 1, 0)) < 1)   //receive confirmation of events
	{
		if (status < 0) eHTerminate(SocketIndex)                    //error in connection
//                if (!TC[SocketIndex].TimeOut) eHTerminate(SocketIndex)  //using socket timeouts
if (!(iter--)) eHTerminate(SocketIndex)                   //To many retries so Close
	}
	if (challange[0] == '+')  //confirmation from Ethernet eHouse controller
	{
		_log.Log(LOG_STATUS, "[TCP Cli %d] Events Accepted", SocketIndex);
		unsigned char k = 0;
		for (k = 0; k < ClientCon->EventSize; k++)
		{
			unsigned int m = ClientCon->EventsToRun[k];
			if (m < EVENT_QUEUE_MAX)
			{
				m_EvQ[m]->LocalEventsTimeOuts = RUN_NOW - 1;
				ClientCon->OK = 1;
			}
			//TC[SocketIndex].EventsToRun[k]=0;
		}
		//				TC[SocketIndex].NotFinished = 0;          //Not Complete flag for thread garbage in case of error
		//				TC[SocketIndex].OK = 1;
	}
	else
	{
		_log.Log(LOG_STATUS, "[TCP Cli %d] No Confirmation", SocketIndex);
	}
#ifdef WIN32
	timeout = 100;
#else
	timeout.tv_sec = 0;    //Timeout for socket set in globals.h
	timeout.tv_usec = 100000;
#endif
	//We are trying close connection as fast as possible
	if (setsockopt(ClientCon->Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)   //Set socket Read operation Timeout
		LOG(LOG_ERROR, "[TCP Cli] Set Read Timeout failed");

	if (setsockopt(ClientCon->Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)   //Set Socket Write operation Timeout
		LOG(LOG_ERROR, "[TCP Cli] Set Write Timeout failed");
	//TCP_NODELAYACK
	kkk = 1;
	status = 1L;

	if (setsockopt(ClientCon->Socket, IPPROTO_TCP, TCP_NODELAY, &kkk, sizeof(kkk)) < 0)   //Set socket send data immediately
	{
		//_log.Log(LOG_STATUS, "[TCP Cli %d] Cant Set TCP NODELAY", SocketIndex);
	}

#if defined WIN32
	u_long iMode = 1;
	ioctlsocket(m_socket, FIONBIO, &iMode);
#else
	fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif

	//Trying to Send termination of socket (just in case)
	challange[0] = 0; //byte = 0 is termination of connection request in this stage
	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Sending termination of connection", SocketIndex);
	if ((status = send(ClientCon->Socket, (char *)&challange, 1, 0)) != 1)
	{
		if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Not sent termination (No Problem)", SocketIndex);
		if (status < 0) eHTerminate(SocketIndex)
	}
	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Termination Sent OK", SocketIndex);

#ifdef USE_GOTO
// Terminate connection and initialize TCP Client Socket for next operations
	terminate :      //Could be replace by KillSocket function and return uncomment definition USE_GOTO
#else
	KillSocket(SocketIndex);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::KillSocket(int SocketIndex)
{

#endif
	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] Closing Connection", SocketIndex);
	memset((void *)&ClientCon->Events, 0, sizeof(ClientCon->Events));  //copy Events to buffer for thread
	memset((void *)&ClientCon->EventsToRun, 0, sizeof(ClientCon->EventsToRun));
	ClientCon->EventSize = 0;
#ifndef WIN32
	close(ClientCon->Socket);          //close socket handle
#else
	closesocket(ClientCon->Socket);          //close socket handle
#endif
	ClientCon->Socket = -1;              //reset TCP client
	//ClientCon->ActiveConnections=-1;   //set active connection from index of eHE[]
	ClientCon->NotFinished = 6;
	if (ClientCon->OK)
	{
		ClientCon->NotFinished = 3;

	}
	if ((SocketIndex < 0) || (SocketIndex >= MAX_CLIENT_SOCKETS))
		return;


	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[TCP Cli %d] End TCP Client", SocketIndex);
	return;
	//                TC[SocketIndex].TimeOut=0;    //used socket timeouts


	if (m_DEBUG_TCPCLIENT) _log.Log(LOG_STATUS, "[!!!!!!!!! TCP Cli ||||| %d] End TCP Client NOT EXITED", SocketIndex);
	return;
}

