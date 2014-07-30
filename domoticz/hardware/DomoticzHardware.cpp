#include "stdafx.h"
#include <iostream>
#include "DomoticzHardware.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"

CDomoticzHardwareBase::CDomoticzHardwareBase()
{
	m_HwdID=0; //should be uniquely assigned
	m_bEnableReceive=false;
	m_rxbufferpos=0;
	m_SeqNr=0;
	m_pUserData=NULL;
	m_bIsStarted=false;
	m_stopHeartbeatrequested = false;
	mytime(&m_LastHeartbeat);
	mytime(&m_LastHeartbeatReceive);
};

CDomoticzHardwareBase::~CDomoticzHardwareBase()
{
}

bool CDomoticzHardwareBase::Start()
{
	return StartHardware();
}

bool CDomoticzHardwareBase::Stop()
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	return StopHardware();
}


bool CDomoticzHardwareBase::onRFXMessage(const unsigned char *pBuffer, const size_t Len)
{
	if (!m_bEnableReceive)
		return true; //receiving not enabled

	size_t ii=0;
	while (ii<Len)
	{
		if (m_rxbufferpos == 0)	//1st char of a packet received
		{
			if (pBuffer[ii]==0) //ignore first char if 00
				return true;
		}
		m_rxbuffer[m_rxbufferpos]=pBuffer[ii];
		m_rxbufferpos++;
		if (m_rxbufferpos>=sizeof(m_rxbuffer))
		{
			//something is out of sync here!!
			//restart
			_log.Log(LOG_ERROR,"input buffer out of sync, going to restart!....");
			m_rxbufferpos=0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer);//decode message
			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
	return true;
}

void CDomoticzHardwareBase::StartHeartbeatThread()
{
	m_stopHeartbeatrequested = false;
	m_Heartbeatthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDomoticzHardwareBase::Do_Heartbeat_Work, this)));
}

void CDomoticzHardwareBase::StopHeartbeatThread()
{
	m_stopHeartbeatrequested = true;
	if (m_Heartbeatthread)
	{
		m_Heartbeatthread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
	}
}

void CDomoticzHardwareBase::Do_Heartbeat_Work()
{
	int secCounter = 0;
	while (!m_stopHeartbeatrequested)
	{
		sleep_milliseconds(200);
		if (m_stopHeartbeatrequested)
			break;
		secCounter++;
		if (secCounter == 5)
		{
			secCounter = 0;
			time_t atime = mytime(NULL);
			if (atime % 12 == 0) {
				mytime(&m_LastHeartbeat);
			}
		}
	}
}

void CDomoticzHardwareBase::SetHeartbeatReceived()
{
	mytime(&m_LastHeartbeatReceive);
}
