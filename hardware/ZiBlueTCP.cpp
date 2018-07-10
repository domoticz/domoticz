#include "stdafx.h"
#include "ZiBlueTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

#define ZiBlue_RETRY_DELAY 30

CZiBlueTCP::CZiBlueTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_retrycntr = ZiBlue_RETRY_DELAY;
}

CZiBlueTCP::~CZiBlueTCP(void)
{
}

bool CZiBlueTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=ZiBlue_RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CZiBlueTCP::Do_Work, this);
	return (m_thread != nullptr);
}

bool CZiBlueTCP::StopHardware()
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

void CZiBlueTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"ZiBlue: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(NULL);
	sOnConnected(this);
	write("10;PING;\n");
}

void CZiBlueTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"ZiBlue: disconnected");
	m_bDoRestart = true;
}

void CZiBlueTCP::Do_Work()
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
/*
		if ((sec_counter % 20 == 0) && (mIsConnected))
		{
			//Send ping (keep alive)
			if (atime - m_LastReceivedTime > 30)
			{
				//Timeout
				_log.Log(LOG_ERROR, "ZiBlue: Nothing received for more than 30 seconds, restarting...");
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
				write("10;PING;\n");
		}
*/
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
			_log.Log(LOG_STATUS, "ZiBlue: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				_log.Log(LOG_STATUS, "ZiBlue: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
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
	_log.Log(LOG_STATUS,"ZiBlue: TCP/IP Worker stopped...");
}

void CZiBlueTCP::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	ParseData((const char*)pData,length);
}

void CZiBlueTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"ZiBlue: Error: %s",e.what());
}

void CZiBlueTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "ZiBlue: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "ZiBlue: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "ZiBlue: %s", error.message().c_str());
}

bool CZiBlueTCP::WriteInt(const std::string &sendString)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(sendString);
	return true;
}

bool CZiBlueTCP::WriteInt(const uint8_t *pData, const size_t length)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(pData, length);
	return true;
}
