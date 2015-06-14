#include "stdafx.h"
#include "MySensorsTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define RETRY_DELAY 30

MySensorsTCP::MySensorsTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
m_szIPAddress(IPAddress),
m_usIPPort(usIPPort),
m_retrycntr(RETRY_DELAY),
m_stoprequested(false),
m_bDoRestart(false)
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
			_log.Log(LOG_ERROR,"MySensors: Winsock could not be initialized!");
		}
	}
#endif
}

MySensorsTCP::~MySensorsTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool MySensorsTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MySensorsTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool MySensorsTCP::StopHardware()
{
	m_stoprequested=true;
	if (isConnected())
	{
		try {
			disconnect();
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}
	try {
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}

	m_bIsStarted=false;
	return true;
}

void MySensorsTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"MySensors: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_bufferpos=0;

	sOnConnected(this);
}

void MySensorsTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"MySensors: disconnected");
}

void MySensorsTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			time_t atime=time(NULL);
			if ((m_bDoRestart)&&(atime%30==0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"MySensors: TCP/IP Worker stopped...");
} 

void MySensorsTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData,length);
}

void MySensorsTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"MySensors: Error: %s",e.what());
}

void MySensorsTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR,"MySensors: Error: %s",error.message().c_str());
}

void MySensorsTCP::WriteInt(const std::string &sendStr)
{
	if (!mIsConnected)
	{
		return;
	}
	write((const unsigned char*)sendStr.c_str(),sendStr.size());
}

