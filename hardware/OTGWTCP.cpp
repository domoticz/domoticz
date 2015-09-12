#include "stdafx.h"
#include "OTGWTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

#define RETRY_DELAY 30

OTGWTCP::OTGWTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
	SetModes(Mode1,Mode2,Mode3,Mode4,Mode5,Mode6);
}

OTGWTCP::~OTGWTCP(void)
{
}

bool OTGWTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&OTGWTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool OTGWTCP::StopHardware()
{
	m_stoprequested=true;
	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	try {
		if (m_thread)
		{
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}

	m_bIsStarted=false;
	return true;
}

void OTGWTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"OTGW: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_bufferpos=0;

	sOnConnected(this);
	GetGatewayDetails();
}

void OTGWTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"OTGW: disconnected");
}

void OTGWTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			connect(m_szIPAddress,m_usIPPort);
			if (mIsConnected)
			{
				GetGatewayDetails();
			}
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
			if (mIsConnected)
			{
				time_t atime=time(NULL);
				if (sec_counter % 30 == 0)//updates every 30 seconds
				{
					bFirstTime=false;
					SendOutsideTemperature();
					SendTime();
					GetGatewayDetails();
				}
			}
		}
	}
	_log.Log(LOG_STATUS,"OTGW: TCP/IP Worker stopped...");
} 

void OTGWTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData,length);
}

void OTGWTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"OTGW: Error: %s",e.what());
}

void OTGWTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR,"OTGW: Error: %s",error.message().c_str());
}

bool OTGWTCP::WriteInt(const unsigned char *pData, const unsigned char Len)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(pData, Len);
	return true;
}
