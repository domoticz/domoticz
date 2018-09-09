#include "stdafx.h"
#include "RFLinkTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include <boost/asio/placeholders.hpp>

CRFLinkTCP::CRFLinkTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress),
	m_heartbeat_timer(mIos)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
}

CRFLinkTCP::~CRFLinkTCP(void)
{
}

bool CRFLinkTCP::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_bIsStarted=true;

	//Connect
	_log.Log(LOG_STATUS, "RFLink: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	SetThreadName(m_tcpthread->native_handle(), "RFLinkTCP");

	//Start heartbeat timer
	m_sec_counter = 0;
	Do_Work(boost::system::error_code());

	return (m_tcpthread != nullptr);
}

bool CRFLinkTCP::StopHardware()
{
	RequestStop();
	terminate();
	m_bIsStarted=false;
	return true;
}

void CRFLinkTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"RFLink: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
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

void CRFLinkTCP::Do_Work(const boost::system::error_code& error)
{
	if (!IsStopRequested(0) && !error)
	{
		m_sec_counter++;

		time_t atime = mytime(NULL);
		if (m_sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if ((m_sec_counter % 20 == 0) && (isConnected()))
		{
			time_t atime = mytime(NULL);
			//Send ping (keep alive)
			if (atime - m_LastReceivedTime > 30)
			{
				//Timeout
				_log.Log(LOG_ERROR, "RFLink: Nothing received for more than 30 seconds, restarting...");
				m_LastReceivedTime = atime;
				schedule_reconnect();
			}
			else
				write("10;PING;\n");
		}

		// Schedule next heartbeat
		m_heartbeat_timer.expires_from_now(std::chrono::seconds(1));
		m_heartbeat_timer.async_wait(boost::bind(&CRFLinkTCP::Do_Work, this, boost::asio::placeholders::error));
	}
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
