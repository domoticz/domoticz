#include "stdafx.h"
#include "RFLinkTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>

CRFLinkTCP::CRFLinkTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_usIPPort = usIPPort;
}

bool CRFLinkTCP::StartHardware()
{
	RequestStart();

	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr = RFLINK_RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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
	Log(LOG_STATUS,"connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(nullptr);
	sOnConnected(this);
	write("10;PING;\n");
	write("10;VERSION;\n");
}

void CRFLinkTCP::OnDisconnect()
{
	// Note: No need to set m_bDoRestart = true here, the connection is automatically reinited by ASyncTCP
	Log(LOG_STATUS,"disconnected");
}

void CRFLinkTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	Log(LOG_STATUS, "trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		time_t atime = mytime(nullptr);
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if ((sec_counter % 20 == 0) && (isConnected()))
		{
			time_t atime = mytime(nullptr);
			//Send ping (keep alive)
			if (atime - m_LastReceivedTime > 30)
			{
				//Timeout
				Log(LOG_ERROR, "Nothing received for more than 30 seconds, restarting...");
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
			Log(LOG_STATUS, "trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			disconnect();
			connect(m_szIPAddress, m_usIPPort);
		}
	}
	terminate();

	Log(LOG_STATUS,"TCP/IP Worker stopped...");
}

void CRFLinkTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData((const char*)pData,length);
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
		Log(LOG_ERROR, "Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		Log(LOG_STATUS, "Connection reset!");
	}
	else
		Log(LOG_ERROR, "%s", error.message().c_str());
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
