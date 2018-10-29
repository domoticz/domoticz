#include "stdafx.h"
#include "OTGWTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"

#define RETRY_DELAY 30

OTGWTCP::OTGWTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6):
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_retrycntr = RETRY_DELAY;
	SetModes(Mode1,Mode2,Mode3,Mode4,Mode5,Mode6);
}

OTGWTCP::~OTGWTCP(void)
{
}

bool OTGWTCP::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&OTGWTCP::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool OTGWTCP::StopHardware()
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

void OTGWTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"OTGW: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted=true;
	m_bufferpos=0;
	sOnConnected(this);
	m_bRequestVersion = true;
}

void OTGWTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"OTGW: disconnected");
}

void OTGWTCP::Do_Work()
{
	int sec_counter = 25;
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (isConnected())
		{
			if ((sec_counter % 28 == 0) && (m_bRequestVersion))
			{
				m_bRequestVersion = false;
				GetVersion();
			}
			else if (sec_counter % 30 == 0)//updates every 30 seconds
			{
				SendOutsideTemperature();
				SendTime();
				GetGatewayDetails();
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"OTGW: TCP/IP Worker stopped...");
}

void OTGWTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData(pData,length);
}

void OTGWTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"OTGW: Error: %s",e.what());
}

void OTGWTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "OTGW: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "OTGW: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "OTGW: %s", error.message().c_str());
}

bool OTGWTCP::WriteInt(const unsigned char *pData, const unsigned char Len)
{
	if (!isConnected())
	{
		return false;
	}
	write(pData, Len);
	return true;
}
