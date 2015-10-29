#include "stdafx.h"
#include "RFLinkTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

#define RFLINK_RETRY_DELAY 30

CRFLinkTCP::CRFLinkTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
}

CRFLinkTCP::~CRFLinkTCP(void)
{
}

bool CRFLinkTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RFLINK_RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRFLinkTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool CRFLinkTCP::StopHardware()
{
	m_stoprequested=true;
	if (isConnected())
	{
		try {
			disconnect();
			close();
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

void CRFLinkTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"RFLink: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(NULL);
	sOnConnected(this);
	write("10;PING;\n");
}

void CRFLinkTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"RFLink: disconnected");
	m_bDoRestart = true;
}

void CRFLinkTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		time_t atime = mytime(NULL);
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat= atime;
		}
		if ((sec_counter % 20 == 0) && (mIsConnected))
		{
			//Send ping (keep alive)
			if (atime - m_LastReceivedTime > 30)
			{
				//Timeout
				_log.Log(LOG_ERROR, "RFLink: Not received anything for more then 30 seconds, restarting...");
				m_retrycntr = (RFLINK_RETRY_DELAY - 3) * 5;
				m_LastReceivedTime = atime;
				try {
					disconnect();
					close();
					m_bDoRestart = true;
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
			}
			else
				write("10;PING;\n");
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			if (mIsConnected)
			{
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
			}
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"RFLink: TCP/IP Worker stopped...");
} 

void CRFLinkTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData((const char*)pData,length);
}

void CRFLinkTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"RFLink: Error: %s",e.what());
}

void CRFLinkTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR,"RFLink: Error: %s",error.message().c_str());
	m_LastReceivedTime = mytime(NULL);
	m_bDoRestart = true;
}

bool CRFLinkTCP::WriteInt(const std::string &sendString)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(sendString);
	return true;
}
