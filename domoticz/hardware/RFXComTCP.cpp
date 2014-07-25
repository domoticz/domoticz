#include "stdafx.h"
#include "RFXComTCP.h"
#include "../main/Logger.h"
//#include <boost/bind.hpp>
//#include <boost/asio.hpp>
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define RETRY_DELAY 30

RFXComTCP::RFXComTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
}

RFXComTCP::~RFXComTCP(void)
{
}

bool RFXComTCP::StartHardware()
{
	m_stoprequested=false;

	//force connect the next first time
	m_bIsStarted=true;
	m_rxbufferpos=0;
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RFXComTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool RFXComTCP::StopHardware()
{
	m_stoprequested = true;
	if (isConnected())
	{
		try {
			disconnect();
			if (m_thread != NULL)
			{
				assert(m_thread);
				m_thread->join();
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
	_log.Log(LOG_STATUS, "RFXCOM: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
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

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);


		if (ltime.tm_sec % 12 == 0) {
			m_mainworker.HeartbeatUpdate(m_HwdID);
		}
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
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	onRFXMessage(pData, length);
}

void RFXComTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "RFXCOM: Error: %s", e.what());
}

void RFXComTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR, "RFXCOM: Error: %s", error.message().c_str());
}

void RFXComTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected)
		return;
	write((const unsigned char*)pdata, length);
}
