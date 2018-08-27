#include "stdafx.h"
#include "P1MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
//#include <iostream>

#define SLEEP_MILLISECONDS 200
#define RETRY_DELAY_SECONDS 30
#define HEARTBEAT_SECONDS 12

P1MeterTCP::P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit):
m_szIPAddress(IPAddress),
m_usIPPort(usIPPort)
{
	m_HwdID = ID;
	m_bDisableCRC = disable_crc;
	m_ratelimit = ratelimit;
	m_stoprequested = false;
}

P1MeterTCP::~P1MeterTCP(void)
{
}


bool P1MeterTCP::StartHardware()
{
	m_stoprequested = false;
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&P1MeterTCP::Do_Work, this);
	SetThreadName(m_thread->native_handle(), "P1MeterTCP");
	return (m_thread != nullptr);
}


bool P1MeterTCP::StopHardware()
{
	m_stoprequested = true;
	m_bIsStarted = false;
	if (isConnected())
	{
		try {
			disconnect();
			close();
		}
		catch(...)
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
	return true;
}


void P1MeterTCP::Do_Work()
{
	int heartbeat_counter = 0;
	int retry_counter = 0;
	while (!m_stoprequested)
	{
		if (mIsConnected)
		{
			update();
		}
		else
		{
			if ((retry_counter % (RETRY_DELAY_SECONDS * 1000 / SLEEP_MILLISECONDS)) == 0)
			{
				_log.Log(LOG_STATUS, "P1MeterTCP: attempt connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				connect(m_szIPAddress, m_usIPPort);
				update();
			}
			else if ((retry_counter % (RETRY_DELAY_SECONDS * 1000 / SLEEP_MILLISECONDS)) <= (1000 / SLEEP_MILLISECONDS))
			{
				// allow for up to 1 second connect time
				update();
			}
			retry_counter++;
		}

		sleep_milliseconds(SLEEP_MILLISECONDS);
		heartbeat_counter++;
		if ((heartbeat_counter % (HEARTBEAT_SECONDS * 1000 / SLEEP_MILLISECONDS)) == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}
	}
	_log.Log(LOG_STATUS, "P1MeterTCP: TCP/IP Worker stopped...");
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
	std::lock_guard<std::mutex> l(readQueueMutex);
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

