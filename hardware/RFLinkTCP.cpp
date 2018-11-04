#include "stdafx.h"
#include "RFLinkTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

CRFLinkTCP::CRFLinkTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_usIPPort = usIPPort;
}

CRFLinkTCP::~CRFLinkTCP(void)
{
}

bool CRFLinkTCP::StartHardware()
{
	RequestStart();

	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr = RFLINK_RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CRFLinkTCP::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool CRFLinkTCP::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
	return true;
}

void CRFLinkTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"RFLink: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(NULL);
	sOnConnected(this);
	write("10;PING;\n");
}

void CRFLinkTCP::OnDisconnect()
{
	// Note: No need to set m_bDoRestart = true here, the connection is automatically reinited by ASyncTCP
	_log.Log(LOG_STATUS,"RFLink: disconnected");
}

void CRFLinkTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	_log.Log(LOG_STATUS, "RFLink: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		time_t atime = mytime(NULL);
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if ((sec_counter % 20 == 0) && (isConnected()))
		{
			time_t atime = mytime(NULL);
			//Send ping (keep alive)
			if (atime - m_LastReceivedTime > 30)
			{
				//Timeout
				_log.Log(LOG_ERROR, "RFLink: Nothing received for more than 30 seconds, restarting...");
				m_retrycntr = 0;
				m_LastReceivedTime = atime;
				//TODO: Add method to ASyncTCP to schedule a reconnect
				m_bDoRestart = true;
			}
			else
				write("10;PING;\n");
		}

		if ((m_bDoRestart) && (sec_counter % 30 == 0))
		{
			_log.Log(LOG_STATUS, "RFLink: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			disconnect();
			connect(m_szIPAddress, m_usIPPort);
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"RFLink: TCP/IP Worker stopped...");
}

void CRFLinkTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData((const char*)pData,length);
}

void CRFLinkTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"RFLink: Error: %s",e.what());
}

void CRFLinkTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "RFLink: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "RFLink: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "RFLink: %s", error.message().c_str());
}

bool CRFLinkTCP::WriteInt(const std::string &sendString)
{
	if (!isConnected())
	{
		return false;
	}
	write(sendString);
	return true;
}
