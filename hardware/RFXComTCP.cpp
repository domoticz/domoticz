#include "stdafx.h"
#include "RFXComTCP.h"
#include "../main/Logger.h"
//#include <boost/bind.hpp>
//#include <boost/asio.hpp>
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define RETRY_DELAY 30

RFXComTCP::RFXComTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_bReceiverStarted = false;
}

RFXComTCP::~RFXComTCP(void)
{
}

bool RFXComTCP::StartHardware()
{
	m_stoprequested=false;
	m_bReceiverStarted = false;

	//force connect the next first time
	m_bIsStarted=true;
	m_rxbufferpos=0;
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&RFXComTCP::Do_Work, this);
	return (m_thread != nullptr);
}

bool RFXComTCP::StopHardware()
{
	m_stoprequested = true;
	if (isConnected())
	{
		try {
			disconnect();
			close();
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
	}

	m_bIsStarted = false;
	return true;
}

void RFXComTCP::OnConnect()
{
	_log.Log(LOG_STATUS, "RFXCOM: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted = true;

	sOnConnected(this);
}

void RFXComTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "RFXCOM: disconnected");
}

void RFXComTCP::Do_Work()
{
	bool bFirstTime = true;
	while (!m_stoprequested)
	{
		m_LastHeartbeat = mytime(NULL);
		if (bFirstTime)
		{
			bFirstTime = false;
			if (!mIsConnected)
			{
				m_rxbufferpos = 0;
				connect(m_szIPAddress, m_usIPPort);
			}
		}
		else
		{
			sleep_milliseconds(40);
			update();
		}
	}
	_log.Log(LOG_STATUS,"RFXCOM: TCP/IP Worker stopped...");
}

void RFXComTCP::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	onInternalMessage(pData, length);
}

void RFXComTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "RFXCOM: Error: %s", e.what());
}

void RFXComTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "RFXCOM: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "RFXCOM: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "RFXCOM: %s", error.message().c_str());
}

bool RFXComTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected)
		return false;
	write((const unsigned char*)pdata, length);
	return true;
}
