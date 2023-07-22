#include "stdafx.h"
#include "S0MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>

#define S0METER_RETRY_DELAY 30

S0MeterTCP::S0MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_retrycntr = S0METER_RETRY_DELAY;
	InitBase();
}

bool S0MeterTCP::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_retrycntr= S0METER_RETRY_DELAY;
	m_bIsStarted=true;

	m_bufferpos = 0;
	ReloadLastTotals();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool S0MeterTCP::StopHardware()
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

void S0MeterTCP::OnConnect()
{
	Log(LOG_STATUS,"connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted=true;
	m_bufferpos = 0;
	sOnConnected(this);
}

void S0MeterTCP::OnDisconnect()
{
	Log(LOG_STATUS,"disconnected");
}

void S0MeterTCP::Do_Work()
{
	int sec_counter = 0;
	Log(LOG_ERROR, "trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	terminate();

	Log(LOG_STATUS,"TCP/IP Worker stopped...");
}

bool S0MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void S0MeterTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData((const unsigned char*)pData,length);
}

void S0MeterTCP::OnError(const boost::system::error_code& error)
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

bool S0MeterTCP::WriteInt(const std::string &sendString)
{
	if (!isConnected())
	{
		return false;
	}
	write(sendString);
	return true;
}
