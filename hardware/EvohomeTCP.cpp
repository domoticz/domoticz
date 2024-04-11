#include "stdafx.h"
#include "EvohomeTCP.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

CEvohomeTCP::CEvohomeTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &UserContID) :

	CEvohomeRadio(ID, UserContID),
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort)
{
}

bool CEvohomeTCP::StopHardware()
{
	try {
		if (m_thread)
		{
			RequestStop();
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	m_bIsStarted=false;
	if(m_bDebug && m_pEvoLog)
	{
		delete m_pEvoLog;
		m_pEvoLog = nullptr;
	}
	return true;
}

void CEvohomeTCP::OnConnect()
{
    CEvohomeRadio::OnConnect();
	Log(LOG_STATUS,"Connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted=true;
	sOnConnected(this);
}

void CEvohomeTCP::OnDisconnect()
{
	Log(LOG_STATUS,"Disconnected");
}

void CEvohomeTCP::Do_Work()
{
    startup = true;
    stLastRelayCheck = boost::posix_time::min_date_time;
    nStartup = 0;
    nStarts = 0;

	int sec_counter = 0;

	Log(LOG_STATUS, "Trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		Idle_Work();
	}

	terminate();

	Log(LOG_STATUS,"Worker stopped...");
}

void CEvohomeTCP::OnData(const unsigned char *pData, size_t length)
{
	try
	{
		//Log(LOG_NORM,"Received %ld bytes",len);
		HandleLoopData((const char *)pData, length);
	}
	catch (...)
	{

	}
}

void CEvohomeTCP::Do_Send(std::string str)
{
    write(str);
}

void CEvohomeTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		Log(LOG_ERROR, "Can't connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		Log(LOG_STATUS, "Connection reset!");
	}
	else
		Log(LOG_ERROR, "Error: %s", error.message().c_str());
}
