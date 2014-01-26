#include "stdafx.h"
#include "OTGWTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>

#define RETRY_DELAY 30

OTGWTCP::OTGWTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5)
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
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
	SetModes(Mode1,Mode2,Mode3,Mode4,Mode5);
}

OTGWTCP::~OTGWTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool OTGWTCP::StartHardware()
{
	m_stoprequested=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&OTGWTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool OTGWTCP::StopHardware()
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
	m_stoprequested = true;
/*
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
*/
	m_bIsStarted=false;
	return true;
}

void OTGWTCP::OnConnect()
{
	_log.Log(LOG_NORM,"OTGW connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	m_bIsStarted=true;
	m_bufferpos=0;

	sOnConnected(this);
	GetGatewayDetails();
}

void OTGWTCP::OnDisconnect()
{
	_log.Log(LOG_NORM,"OTGW disconnected");
}

void OTGWTCP::Do_Work()
{
	bool bFirstTime=true;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (bFirstTime)
		{
			bFirstTime=false;
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			update();
			if (mIsConnected)
			{
				time_t atime=time(NULL);
				if (atime%30==0)//updates every 30 seconds
				{
					bFirstTime=false;
					SendOutsideTemperature();
					GetGatewayDetails();
				}
			}
		}
	}
	_log.Log(LOG_NORM,"OTGW TCP/IP Worker stopped...");
} 

void OTGWTCP::GetGatewayDetails()
{
	char szCmd[10];
	strcpy(szCmd,"PS=1\r\n");
	write((const unsigned char*)&szCmd,strlen(szCmd));
}

void OTGWTCP::SendOutsideTemperature()
{
	float temp;
	if (!GetOutsideTemperatureFromDomoticz(temp))
		return;
	char szCmd[30];
	sprintf(szCmd,"OT=%.1f\r\n",temp);
	write((const unsigned char*)&szCmd,strlen(szCmd));
}

void OTGWTCP::SetSetpoint(const int idx, const float temp)
{
	char szCmd[30];
	if (idx==5)
	{
		//Room Set Point
		//Make this a temporarily Set Point, this will be overridden when the thermostat changes/applying it's program
		_log.Log(LOG_NORM,"OTGW: Setting Room SetPoint to: %.1f",temp);
		sprintf(szCmd,"TT=%.1f\r\n",temp);
		write((const unsigned char*)&szCmd,strlen(szCmd));
	}
}

void OTGWTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData,length);
}

void OTGWTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"OTGW Error: %s",e.what());
}

void OTGWTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR,"OTGW Error: %s",error.message().c_str());
}

void OTGWTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected)
		return;
//	write(pdata,length,0);
}
