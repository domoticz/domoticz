#include "stdafx.h"
#include "P1MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"

P1MeterTCP::P1MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const bool disable_crc, const int ratelimit, const std::string& DecryptionKey):
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort)
{
	m_HwdID = ID;
	m_bDisableCRC = disable_crc;
	m_ratelimit = ratelimit;

	if (!DecryptionKey.empty())
	{
		m_bIsEncrypted = true;
		m_szHexKey = HexToBytes(DecryptionKey);
	}
}

bool P1MeterTCP::StartHardware()
{
	RequestStart();

	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}


bool P1MeterTCP::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}


void P1MeterTCP::Do_Work()
{
	int sec_counter = 0;
	Log(LOG_STATUS, "attempt connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress, m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	terminate();

	Log(LOG_STATUS, "TCP/IP Worker stopped...");
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
	Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	if (m_bDisableCRC)
	{
		Log(LOG_STATUS, "P1 Smart Meter: CRC validation disabled through hardware control");
	}
}


void P1MeterTCP::OnDisconnect()
{
	Log(LOG_STATUS, "disconnected");
}


void P1MeterTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseP1Data((const unsigned char*)pData, length, m_bDisableCRC, m_ratelimit);
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
	{
		Log(LOG_ERROR, "%s", error.message().c_str());
	}
}

