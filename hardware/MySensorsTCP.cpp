#include "stdafx.h"
#include "MySensorsTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define RETRY_DELAY 30

MySensorsTCP::MySensorsTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort),
	m_retrycntr(RETRY_DELAY),
	m_stoprequested(false),
	m_bDoRestart(false)
{
	m_HwdID = ID;
}

MySensorsTCP::~MySensorsTCP(void)
{
}

bool MySensorsTCP::StartHardware()
{
	m_LineReceived.clear();

	LoadDevicesFromDatabase();

	m_stoprequested = false;
	m_bDoRestart = false;

	//force connect the next first time
	m_retrycntr = RETRY_DELAY;
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&MySensorsTCP::Do_Work, this);
	StartSendQueue();
	return (m_thread != nullptr);
}

bool MySensorsTCP::StopHardware()
{
	m_stoprequested = true;
	StopSendQueue();
	if (isConnected())
	{
		try {
			disconnect();
		}
		catch (...)
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

	m_bIsStarted = false;
	return true;
}

void MySensorsTCP::OnConnect()
{
	_log.Log(LOG_STATUS, "MySensors: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart = false;
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
	if (!m_stoprequested)
		m_bDoRestart = true;
}

void MySensorsTCP::Do_Work()
{
	bool bFirstTime = true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}

		if (bFirstTime)
		{
			bFirstTime = false;
			_log.Log(LOG_STATUS, "MySensors: trying to connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			connect(m_szIPAddress, m_usIPPort);
		}
		else
		{
			time_t atime = time(NULL);
			if ((m_bDoRestart) && (atime % 30 == 0))
			{
				_log.Log(LOG_STATUS, "MySensors: trying to connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				connect(m_szIPAddress, m_usIPPort);
			}
			update();
			if (isConnected())
			{
				if (sec_counter % 10 == 0)
				{
					//Send a Heartbeat message
					std::string sRequest = "0;0;3;0;18;PING\n";
					WriteInt(sRequest);
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "MySensors: TCP/IP Worker stopped...");
}

void MySensorsTCP::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
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
	if (!mIsConnected)
	{
		return;
	}
	write((const unsigned char*)sendStr.c_str(), sendStr.size());
}

