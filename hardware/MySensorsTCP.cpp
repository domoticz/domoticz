#include "stdafx.h"
#include "MySensorsTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include <boost/asio/placeholders.hpp>

#define RETRY_DELAY 30

MySensorsTCP::MySensorsTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort),
	m_retrycntr(RETRY_DELAY),
	m_heartbeat_timer(mIos)
{
	m_HwdID = ID;
}

MySensorsTCP::~MySensorsTCP(void)
{
}

bool MySensorsTCP::StartHardware()
{
	RequestStart();

	m_LineReceived.clear();

	LoadDevicesFromDatabase();

	//force connect the next first time
	m_retrycntr = RETRY_DELAY;
	m_bIsStarted = true;

	//Connect
	_log.Log(LOG_STATUS, "MySensors: trying to connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	SetThreadName(m_tcpthread->native_handle(), "MySensorsTCP");

	//Start heartbeat timer
	Do_Work(boost::system::error_code());
	StartSendQueue();

	return (m_tcpthread != nullptr);
}

bool MySensorsTCP::StopHardware()
{
	StopSendQueue();
	RequestStop();
	terminate();
	_log.Log(LOG_STATUS, "MySensors: TCP/IP Worker stopped...");

	m_bIsStarted = false;
	return true;
}

void MySensorsTCP::OnConnect()
{
	_log.Log(LOG_STATUS, "MySensors: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted = true;
	m_LineReceived.clear();

	sOnConnected(this);

	//Request gateway version
	std::string sRequest = "0;0;3;0;2;\n";
	WriteInt(sRequest);
}

void MySensorsTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "MySensors: disconnected");
}

void MySensorsTCP::Do_Work(const boost::system::error_code& error)
{
	if (!IsStopRequested(0) && !error)
	{

		m_LastHeartbeat = mytime(NULL);

		if (isConnected())
		{
			//Send a Heartbeat message
			std::string sRequest = "0;0;3;0;18;PING\n";
			WriteInt(sRequest);
		}

		// Schedule next heartbeat
		m_heartbeat_timer.expires_from_now(std::chrono::seconds(10));
		m_heartbeat_timer.async_wait(boost::bind(&MySensorsTCP::Do_Work, this, boost::asio::placeholders::error));
	}
}

void MySensorsTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData(pData, length);
}

void MySensorsTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "MySensors: %s", e.what());
}

void MySensorsTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "MySensors: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "MySensors: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "MySensors: %s", error.message().c_str());
}

void MySensorsTCP::WriteInt(const std::string &sendStr)
{
	if (!isConnected())
	{
		return;
	}
	write((const unsigned char*)sendStr.c_str(), sendStr.size());
}

