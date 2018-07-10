#include "stdafx.h"
#include "S0MeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

#define S0METER_RETRY_DELAY 30

S0MeterTCP::S0MeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_retrycntr = S0METER_RETRY_DELAY;
	InitBase();
}

S0MeterTCP::~S0MeterTCP(void)
{
}

bool S0MeterTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr= S0METER_RETRY_DELAY;
	m_bIsStarted=true;

	m_bufferpos = 0;
	ReloadLastTotals();

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&S0MeterTCP::Do_Work, this);
	return (m_thread != nullptr);
}

bool S0MeterTCP::StopHardware()
{
	m_stoprequested=true;
	if (isConnected())
	{
		try {
			disconnect();
			close();
		} catch(...)
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

	m_bIsStarted=false;
	return true;
}

void S0MeterTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"S0 Meter: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	m_bufferpos = 0;
	sOnConnected(this);
}

void S0MeterTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"S0 Meter: disconnected");
	m_bDoRestart = true;
}

void S0MeterTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		time_t atime = mytime(NULL);
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat= atime;
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			if (mIsConnected)
			{
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
			}
			_log.Log(LOG_ERROR, "S0 Meter: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				_log.Log(LOG_ERROR, "S0 Meter: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"S0 Meter: TCP/IP Worker stopped...");
}

bool S0MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void S0MeterTCP::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	ParseData((const unsigned char*)pData,length);
}

void S0MeterTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"S0 Meter: Error: %s",e.what());
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
		_log.Log(LOG_ERROR, "S0 Meter: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "S0 Meter: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "S0 Meter: %s", error.message().c_str());
}

bool S0MeterTCP::WriteInt(const std::string &sendString)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(sendString);
	return true;
}
