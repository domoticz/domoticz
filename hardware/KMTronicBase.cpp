#include "stdafx.h"
#include "KMTronicBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "hardwaretypes.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ctime>

#define round(a) ( int ) ( a + .5 )

KMTronicBase::KMTronicBase()
{
	m_bufferpos = 0;
	m_TotRelais = 0;
	std::fill(std::begin(m_bRelaisStatus), std::end(m_bRelaisStatus), false);
}

void KMTronicBase::ParseData(const unsigned char *pData, int Len)
{
	int ii=0;
	while (ii<Len)
	{
		const unsigned char c = pData[ii];
		if(c == 0x0d)
		{
			ii++;
			continue;
		}

		if(c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}


bool KMTronicBase::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		//Light command

		int node_id = pCmd->LIGHTING2.id4;
		if (node_id > m_TotRelais)
			return false;

		if ((pCmd->LIGHTING2.cmnd == light2_sOn) || (pCmd->LIGHTING2.cmnd == light2_sOff))
		{
			unsigned char SendBuf[3];
			SendBuf[0] = 0xFF;
			SendBuf[1] = (uint8_t)node_id;
			SendBuf[2] = (pCmd->LIGHTING2.cmnd == light2_sOn) ? 1 : 0;
			WriteInt(SendBuf, 3,false);
			return true;
		}

	}
	return false;
}


void KMTronicBase::ParseLine()
{
	if (m_bufferpos<2)
		return;
}

