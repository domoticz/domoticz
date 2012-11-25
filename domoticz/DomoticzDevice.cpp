#include "stdafx.h"
#include "DomoticzDevice.h"

CDomoticzDeviceBase::CDomoticzDeviceBase()
{
	HwdID=0; //should be uniquely assigned
	m_bEnableReceive=false;
	m_rxbufferpos=0;
	m_SeqNr=0;
	m_bIsStarted=false;
};

void CDomoticzDeviceBase::onRFXMessage(const unsigned char *pBuffer, const size_t Len)
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
/*
			{
				WriteMessage("----------------------------------");
				// current date/time based on current system
				time_t now = time(0);

				// convert now to string form
				char *szDate = asctime(localtime(&now));
				std::cout << szDate << " RX: Len: " << std::dec << (int)pBuffer[ii] << " ";
			}
*/
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
			sDecodeRXMessage(HwdID, (const unsigned char *)&m_rxbuffer);//decode message
			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
}
