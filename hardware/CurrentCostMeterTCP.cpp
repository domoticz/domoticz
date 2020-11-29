#include "stdafx.h"
#include "CurrentCostMeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include <iostream>

#define RETRY_DELAY 30

CurrentCostMeterTCP::CurrentCostMeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_retrycntr(RETRY_DELAY),
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort),
	m_socket(INVALID_SOCKET)
{
	m_HwdID=ID;
}

bool CurrentCostMeterTCP::StartHardware()
{
	RequestStart();

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
		if (he == nullptr)
		{
			return false;
		}

		memcpy(&(m_addr.sin_addr), he->h_addr_list[0], 4);
	}

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool CurrentCostMeterTCP::isConnected()
{
	return (m_socket != INVALID_SOCKET);
}

bool CurrentCostMeterTCP::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		disconnect(); //force socket close to unblock the read request in the thread
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
	return true;
}

bool CurrentCostMeterTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		Log(LOG_ERROR,"could not create a TCP/IP socket!");
		return false;
	}

	// connect to the server
	int nRet;
	nRet = connect(m_socket,(const sockaddr*)&m_addr, sizeof(m_addr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_socket);
		m_socket=INVALID_SOCKET;
		Log(LOG_ERROR,"could not connect to: %s:%d",m_szIPAddress.c_str(),m_usIPPort);
		return false;
	}

	Log(LOG_STATUS,"connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	Init();

	sOnConnected(this);
	return true;
}

void CurrentCostMeterTCP::disconnect()
{
	if (m_socket==INVALID_SOCKET)
		return;
	closesocket(m_socket);
	m_socket=INVALID_SOCKET;
}


void CurrentCostMeterTCP::Do_Work()
{
	int sec_counter = 0;
	while (!IsStopRequested(100))
	{
		if (m_socket == INVALID_SOCKET)
		{
			if (!IsStopRequested(900)) //+100 = 1000
			{
				sec_counter++;

				if (sec_counter % 12 == 0) {
					m_LastHeartbeat = mytime(nullptr);
				}

				m_retrycntr++;
				if (m_retrycntr >= RETRY_DELAY)
				{
					m_retrycntr = 0;
					if (!ConnectInternal())
					{
						Log(LOG_STATUS, "retrying in %d seconds...", RETRY_DELAY);
						continue;
					}
				}
			}
		}
		else
		{
			char data[1028];
			int bread=recv(m_socket,data,sizeof(data),0);
			if (IsStopRequested(100))
				break;
			m_LastHeartbeat = mytime(nullptr);
			if ((bread==0)||(bread<0))
			{
				disconnect();
				Log(LOG_ERROR, "TCP/IP connection closed!, retrying in %d seconds...", RETRY_DELAY);
				m_retrycntr = 0;
				continue;
			}
			ParseData(data, bread);
		}
	}
	Log(LOG_STATUS,"TCP/IP Worker stopped...");
}

void CurrentCostMeterTCP::write(const char* /*data*/, size_t /*size*/)
{
}

bool CurrentCostMeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isConnected())
		return false;
	write(pdata,length);
	return true;
}
