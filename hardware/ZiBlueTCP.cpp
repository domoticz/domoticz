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
	m_usIPPort=usIPPort;
	m_retrycntr = ZiBlue_RETRY_DELAY;
}

CZiBlueTCP::~CZiBlueTCP(void)
{
}

bool CZiBlueTCP::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_retrycntr=ZiBlue_RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CZiBlueTCP::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool CZiBlueTCP::StopHardware()
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

void CZiBlueTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"ZiBlue: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(NULL);
	sOnConnected(this);
	write("10;PING;\n");
}

void CZiBlueTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"ZiBlue: disconnected");
}

void CZiBlueTCP::Do_Work()
{
	int sec_counter = 0;
	_log.Log(LOG_STATUS, "ZiBlue: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"ZiBlue: TCP/IP Worker stopped...");
}

void CZiBlueTCP::OnData(const unsigned char *pData, size_t length)
{
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
	if (!isConnected())
	{
		return false;
	}
	write(sendString);
	return true;
}

bool CZiBlueTCP::WriteInt(const uint8_t *pData, const size_t length)
{
	if (!isConnected())
	{
		return false;
	}
	write(pData, length);
	return true;
}
