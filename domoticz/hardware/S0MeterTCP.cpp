#include "stdafx.h"
#include "S0MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>

#define RETRY_DELAY 30

S0MeterTCP::S0MeterTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort, const int M1Type, const int M1PPH, const int M2Type, const int M2PPH)
{
	m_HwdID=ID;
#if defined WIN32
	int ret;
	//Init winsock
	WSADATA data;
	WORD version; 

	version = (MAKEWORD(2, 2)); 
	ret = WSAStartup(version, &data); 
	if (ret != 0) 
	{  
		ret = WSAGetLastError(); 

		if (ret == WSANOTINITIALISED) 
		{  
			_log.Log(LOG_ERROR,"Winsock could not be initialized!");
		}
	}
#endif
	//Quick hack, meters 3 till 5 uses same parameter as 1 for now, till we can configure it
	m_meters[0].m_type=M1Type;
	m_meters[1].m_type=M2Type;
	m_meters[2].m_type=M1Type;
	m_meters[3].m_type=M1Type;
	m_meters[4].m_type=M1Type;
	m_meters[0].m_pulse_per_unit=1000.0;
	m_meters[1].m_pulse_per_unit=1000.0;
	m_meters[2].m_pulse_per_unit=1000.0;
	m_meters[3].m_pulse_per_unit=1000.0;
	m_meters[4].m_pulse_per_unit=1000.0;

	if (M1PPH!=0)
		m_meters[0].m_pulse_per_unit=float(M1PPH);
	if (M2PPH!=0)
		m_meters[1].m_pulse_per_unit=float(M1PPH);
	m_socket=INVALID_SOCKET;
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
}

S0MeterTCP::~S0MeterTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool S0MeterTCP::StartHardware()
{
	m_stoprequested=false;

	memset(&m_addr,0,sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_usIPPort);

	unsigned long ip;
	ip=inet_addr(m_szIPAddress.c_str());

	// if we have a error in the ip, it means we have entered a string
	if(ip!=INADDR_NONE)
	{
		m_addr.sin_addr.s_addr=ip;
	}
	else
	{
		// change Hostname in serveraddr
		hostent *he=gethostbyname(m_szIPAddress.c_str());
		if(he==NULL)
		{
			return false;
		}
		else
		{
			memcpy(&(m_addr.sin_addr),he->h_addr_list[0],4);
		}
	}

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&S0MeterTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool S0MeterTCP::StopHardware()
{
	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	m_bIsStarted=false;
	return true;
}

bool S0MeterTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		_log.Log(LOG_ERROR,"could not create a TCP/IP socket!");
		return false;
	}

	// connect to the server
	int nRet;
	nRet = connect(m_socket,(const sockaddr*)&m_addr, sizeof(m_addr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_socket);
		m_socket=INVALID_SOCKET;
		_log.Log(LOG_ERROR,"S0 Meter could not connect to: %s:%ld",m_szIPAddress.c_str(),m_usIPPort);
		return false;
	}

	_log.Log(LOG_NORM,"S0 Meter connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	m_bIsStarted=true;
	m_bufferpos=0;
	ReloadLastTotals();

	sOnConnected(this);
	return true;
}

void S0MeterTCP::disconnect()
{
	m_stoprequested=true;
	if (m_socket==INVALID_SOCKET)
		return;
	closesocket(m_socket);	//will terminate the thread
	m_socket=INVALID_SOCKET;
}


void S0MeterTCP::Do_Work()
{
	while (!m_stoprequested)
	{
		if (
			(m_socket == INVALID_SOCKET)&&
			(!m_stoprequested)
			)
		{
			sleep_seconds(1);
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				if (!ConnectInternal())
				{
					_log.Log(LOG_NORM,"retrying in %d seconds...", RETRY_DELAY);
					continue;
				}
			}
		}
		else
		{
			unsigned char data[1028];
			int bread=recv(m_socket,(char*)&data,sizeof(data),0);
			if (m_stoprequested)
				break;
			if ((bread==0)||(bread<0)) {
				_log.Log(LOG_ERROR,"S0 Meter TCP/IP connection closed!");
				closesocket(m_socket);
				m_socket=INVALID_SOCKET;
				if (!m_stoprequested)
				{
					_log.Log(LOG_NORM,"retrying in %d seconds...", RETRY_DELAY);
					m_retrycntr=0;
					continue;
				}
			}
			else
			{
				boost::lock_guard<boost::mutex> l(readQueueMutex);
				ParseData((const unsigned char*)&data,bread);
			}
		}
	}
	_log.Log(LOG_NORM,"S0 Meter TCP/IP Worker stopped...");
} 

void S0MeterTCP::write(const char *data, size_t size)
{
}

void S0MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
}
