#include "stdafx.h"
#include "P1MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include <iostream>

#define RETRY_DELAY 30

P1MeterTCP::P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit):
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_socket=INVALID_SOCKET;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_retrycntr = RETRY_DELAY;
	m_bDisableCRC = disable_crc;
	m_ratelimit = ratelimit;
}

P1MeterTCP::~P1MeterTCP(void)
{
}

bool P1MeterTCP::StartHardware()
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
	m_thread = std::make_shared<std::thread>(&P1MeterTCP::Do_Work, this);
	return (m_thread != nullptr);
}

bool P1MeterTCP::StopHardware()
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

bool P1MeterTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		_log.Log(LOG_ERROR,"P1 Smart Meter: could not create a TCP/IP socket!");
		return false;
	}

	// connect to the server
	int nRet;
	nRet = connect(m_socket,(const sockaddr*)&m_addr, sizeof(m_addr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_socket);
		m_socket=INVALID_SOCKET;
		_log.Log(LOG_ERROR,"P1 Smart Meter: could not connect to: %s:%d",m_szIPAddress.c_str(),m_usIPPort);
		return false;
	}

	_log.Log(LOG_STATUS,"P1 Smart Meter: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	if (m_bDisableCRC) {
		_log.Log(LOG_STATUS,"P1 Smart Meter: CRC validation disabled through hardware control");
	}

	Init();

	sOnConnected(this);
	return true;
}

void P1MeterTCP::disconnect()
{
	m_stoprequested=true;
	if (m_socket==INVALID_SOCKET)
		return;
	closesocket(m_socket);	//will terminate the thread
	m_socket=INVALID_SOCKET;
}


void P1MeterTCP::Do_Work()
{
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		if (
			(m_socket == INVALID_SOCKET)&&
			(!m_stoprequested)
			)
		{
			sleep_seconds(1);
			sec_counter++;

			if (sec_counter % 12 == 0) {
				m_LastHeartbeat=mytime(NULL);
			}

			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				if (!ConnectInternal())
				{
					_log.Log(LOG_STATUS,"P1 Smart Meter: retrying in %d seconds...", RETRY_DELAY);
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
			m_LastHeartbeat=mytime(NULL);
			if ((bread==0)||(bread<0)) {
				_log.Log(LOG_ERROR,"P1 Smart Meter: TCP/IP connection closed!");
				closesocket(m_socket);
				m_socket=INVALID_SOCKET;
				if (!m_stoprequested)
				{
					_log.Log(LOG_STATUS,"P1 Smart Meter: retrying in %d seconds...", RETRY_DELAY);
					m_retrycntr=0;
					continue;
				}
			}
			else
			{
				std::lock_guard<std::mutex> l(readQueueMutex);
				ParseData((const unsigned char*)&data, bread, m_bDisableCRC, m_ratelimit);
			}
		}
	}
	_log.Log(LOG_STATUS,"P1 Smart Meter: TCP/IP Worker stopped...");
}

void P1MeterTCP::write(const char *data, size_t size)
{
}

bool P1MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isConnected())
		return false;
	write(pdata,length);
	return true;
}
