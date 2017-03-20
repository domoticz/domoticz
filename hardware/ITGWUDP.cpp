#include "stdafx.h"
#include "ITGWUDP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

#define RFLINK_RETRY_DELAY 30

CITGWUDP::CITGWUDP(const int ID, const std::string &IPAddress):
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=49880;
	m_retrycntr = RFLINK_RETRY_DELAY;
}

CITGWUDP::~CITGWUDP(void)
{
}

bool CITGWUDP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RFLINK_RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CITGWUDP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool CITGWUDP::StopHardware()
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

void CITGWUDP::OnConnect()
{
	_log.Log(LOG_STATUS,"ITGW: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(NULL);
	sOnConnected(this);
	write("\n");
}

void CITGWUDP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"ITGW: disconnected");
	m_bDoRestart = true;
}

void CITGWUDP::Do_Work()
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
				_log.Log(LOG_ERROR, "ITGW: Nothing received for more then 30 seconds, restarting...");
				m_retrycntr = 0;
				m_LastReceivedTime = atime;
				m_bDoRestart = true;
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
			}
			else
				write("\n");
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
			_log.Log(LOG_STATUS, "ITGW: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				_log.Log(LOG_STATUS, "ITGW: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
				connect(m_szIPAddress, m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"ITGW: TCP/IP Worker stopped...");
}

void CITGWUDP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData((const char*)pData,length);
}

void CITGWUDP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"ITGW: Error: %s",e.what());
}

void CITGWUDP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "ITGW: Can not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "ITGW: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "ITGW: %s", error.message().c_str());
}

bool CITGWUDP::WriteInt(const std::string &sendString)
{
	write(sendString);
	return true;
}
