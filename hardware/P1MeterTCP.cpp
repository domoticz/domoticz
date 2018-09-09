#include "stdafx.h"
#include "P1MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include <boost/asio/placeholders.hpp>

P1MeterTCP::P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit):
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort),
	m_heartbeat_timer(mIos)
{
	m_HwdID = ID;
	m_bDisableCRC = disable_crc;
	m_ratelimit = ratelimit;
}

P1MeterTCP::~P1MeterTCP(void)
{
}


bool P1MeterTCP::StartHardware()
{
	RequestStart();

	m_bIsStarted = true;

	//Connect
	_log.Log(LOG_STATUS, "P1MeterTCP: attempt connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	SetThreadName(m_tcpthread->native_handle(), "P1MeterTCP");

	//Start heartbeat timer
	Do_Work(boost::system::error_code());

	return (m_tcpthread != nullptr);
}


bool P1MeterTCP::StopHardware()
{
	RequestStop();
	terminate();
	_log.Log(LOG_STATUS, "P1MeterTCP: TCP/IP Worker stopped...");

	m_bIsStarted = false;
	return true;
}


void P1MeterTCP::Do_Work(const boost::system::error_code& error)
{
	if (!IsStopRequested(0) && !error)
	{
		m_LastHeartbeat = mytime(NULL);

		// Schedule next heartbeat
		m_heartbeat_timer.expires_from_now(std::chrono::seconds(12));
		m_heartbeat_timer.async_wait(boost::bind(&P1MeterTCP::Do_Work, this, boost::asio::placeholders::error));
	}

}


bool P1MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	// read only device - just return true or false depending on connection status
	return isConnected();
}


void P1MeterTCP::OnConnect()
{
	// reset all values and buffers - they may contain invalid data
	Init();
	_log.Log(LOG_STATUS, "P1MeterTCP: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	if (m_bDisableCRC)
	{
		_log.Log(LOG_STATUS, "P1 Smart Meter: CRC validation disabled through hardware control");
	}
}


void P1MeterTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "P1MeterTCP: disconnected");
}


void P1MeterTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData((const unsigned char*)pData, length, m_bDisableCRC, m_ratelimit);
}


void P1MeterTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "P1MeterTCP: Error: %s", e.what());
}


void P1MeterTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "P1MeterTCP: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "P1MeterTCP: Connection reset!");
	}
	else
	{
		_log.Log(LOG_ERROR, "P1MeterTCP: %s", error.message().c_str());
	}
}

