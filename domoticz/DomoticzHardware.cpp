#include "stdafx.h"
#include "DomoticzHardware.h"

CDomoticzHardwareBase::CDomoticzHardwareBase()
{
	m_HwdID=0; //should be uniquely assigned
	m_bEnableReceive=false;
	m_rxbufferpos=0;
	m_SeqNr=0;
	m_bIsStarted=false;
};

bool CDomoticzHardwareBase::StartSharing(const std::string port, const std::string username, const std::string password, const int rights)
{
	try {
		m_sharedserver.StartServer("0.0.0.0",port, username, password, rights);
	} catch(...)
	{
		return false;
	}
	return true;
}

bool CDomoticzHardwareBase::Start()
{
	return StartHardware();
}

bool CDomoticzHardwareBase::Stop()
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	StopSharing();
	return StopHardware();
}

void CDomoticzHardwareBase::StopSharing()
{
	m_sharedserver.StopServer();
}

void CDomoticzHardwareBase::onRFXMessage(const unsigned char *pBuffer, const size_t Len)
{
	if (!m_bEnableReceive)
		return; //receiving not enabled

	size_t ii=0;
	while (ii<Len)
	{
		if (m_rxbufferpos == 0)	//1st char of a packet received
		{
			if (pBuffer[ii]==0) //ignore first char if 00
				return;
		}
		m_rxbuffer[m_rxbufferpos]=pBuffer[ii];
		m_rxbufferpos++;
		if (m_rxbufferpos>=sizeof(m_rxbuffer))
		{
			//something is out of sync here!!
			//restart
			std::cout << "input buffer out of sync, going to restart!...." << std::endl;
			m_rxbufferpos=0;
			return;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer);//decode message
			m_sharedserver.SendToAll((const char*)m_rxbuffer,m_rxbufferpos);

			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
}
