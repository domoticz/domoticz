#include "stdafx.h"
#include "EvohomeTCP.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"

CEvohomeTCP::CEvohomeTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &UserContID) :
CEvohomeRadio(ID, UserContID), m_szIPAddress(IPAddress), m_usIPPort(usIPPort)
{
}

bool CEvohomeTCP::StopHardware()
{
	m_stoprequested=true;

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

	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}

	m_bIsStarted=false;
	if(m_bDebug && m_pEvoLog)
	{
		delete m_pEvoLog;
		m_pEvoLog=NULL;
	}
	return true;
}

void CEvohomeTCP::OnConnect()
{
    CEvohomeRadio::OnConnect();
	_log.Log(LOG_STATUS,"evohome TCP/IP: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;
	sOnConnected(this);
}

void CEvohomeTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"evohome TCP/IP: disconnected");
	m_bDoRestart = true;
}

void CEvohomeTCP::Do_Work()
{
    startup = true;
    stLastRelayCheck = boost::posix_time::min_date_time;
    nStartup = 0;
    nStarts = 0;

	bool bFirstTime = true;
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
			_log.Log(LOG_STATUS, "evohome TCP/IP: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				_log.Log(LOG_STATUS, "evohome TCP/IP: trying to connect to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
				try {
					disconnect();
					close();
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
				connect(m_szIPAddress, m_usIPPort);
			}
            update();
            Idle_Work();
		}
	}
	_log.Log(LOG_STATUS,"evohome TCP/IP: TCP/IP Worker stopped...");
}

void CEvohomeTCP::OnData(const unsigned char *pData, size_t length)
{
    boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		//_log.Log(LOG_NORM,"evohome: received %ld bytes",len);
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

void CEvohomeTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"evohome TCP/IP: Error: %s",e.what());
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
		_log.Log(LOG_ERROR, "evohome TCP/IP: Can not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "evohome TCP/IP: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "evohome TCP/IP: %s", error.message().c_str());
}
