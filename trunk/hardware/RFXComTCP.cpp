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

bool RFXComTCP::onInternalMessage(const unsigned char *pBuffer, const size_t Len)
{
	if (!m_bEnableReceive)
		return true; //receiving not enabled

	size_t ii = 0;
	while (ii < Len)
	{
		if (m_rxbufferpos == 0)	//1st char of a packet received
		{
			if (pBuffer[ii] == 0) //ignore first char if 00
				return true;
		}
		m_rxbuffer[m_rxbufferpos] = pBuffer[ii];
		m_rxbufferpos++;
		if (m_rxbufferpos >= sizeof(m_rxbuffer))
		{
			//something is out of sync here!!
			//restart
			_log.Log(LOG_ERROR, "input buffer out of sync, going to restart!....");
			m_rxbufferpos = 0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			if (!m_bReceiverStarted)
			{
				if (m_rxbuffer[1] == pTypeInterfaceMessage)
				{
					const tRBUF *pResponse = (tRBUF *)&m_rxbuffer;
					if (pResponse->IRESPONSE.subtype == cmdStartRec)
					{
						m_bReceiverStarted = strstr((char*)&pResponse->IRESPONSE.msg1, "Copyright RFXCOM") != NULL;
					}
					else
					{
						_log.Log(LOG_STATUS, "RFXCOM: Please upgrade your RFXTrx Firmware!...");
						m_bReceiverStarted = true;
					}
				}
			}
			else
				sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer);//decode message
			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
	return true;
}

void RFXComTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	onInternalMessage(pData, length);
}

void RFXComTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "RFXCOM: Error: %s", e.what());
}

void RFXComTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR, "RFXCOM: Error: %s", error.message().c_str());
}

bool RFXComTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected)
		return false;
	write((const unsigned char*)pdata, length);
	return true;
}
