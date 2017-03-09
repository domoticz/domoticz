#include "stdafx.h"
#include "RFXBase.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"

CRFXBase::CRFXBase()
{
}

CRFXBase::~CRFXBase()
{
}

bool CRFXBase::onInternalMessage(const unsigned char *pBuffer, const size_t Len)
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
		if (m_rxbufferpos >= sizeof(m_rxbuffer) - 1)
		{
			//something is out of sync here!!
			//restart
			_log.Log(LOG_ERROR, "RFXCOM: input buffer out of sync, going to restart!....");
			m_rxbufferpos = 0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			if (CheckValidRFXData((uint8_t*)&m_rxbuffer))
				sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer, NULL, -1);
			else
				_log.Log(LOG_ERROR, "RFXCOM: Invalid data received!....");

			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
	return true;
}

bool CRFXBase::CheckValidRFXData(const uint8_t *pData)
{
	uint8_t pLen = pData[0];
	uint8_t pType = pData[1];
	if (pLen < 1)
		return false;
	switch (pType)
	{
	case pTypeInterfaceControl:
		return (pLen == 0x0D);
	case pTypeInterfaceMessage:
		return (pLen == 0x14);
	case pTypeRecXmitMessage:
		return (pLen == 0x04);
	case pTypeUndecoded:
		return (pLen > 2);
	case pTypeLighting1:
		return (pLen == 0x07);
	case pTypeLighting2:
		return (pLen == 0x0B);
	case pTypeLighting3:
		return (pLen == 0x08);
	case pTypeLighting4:
		return (pLen == 0x09);
	case pTypeLighting5:
		return (pLen == 0x0A);
	case pTypeLighting6:
		return (pLen == 0x0B);
	case pTypeChime:
		return (pLen == 0x07);
	case pTypeFan:
		return (pLen == 0x08);
	case pTypeCurtain:
		return (pLen == 0x07);
	case pTypeBlinds:
		return (pLen == 0x09);
	case pTypeRFY:
		return (pLen == 0x0C);
	case pTypeHomeConfort:
		return (pLen == 0x0C);
	case pTypeSecurity1:
		return (pLen == 0x08);
	case pTypeSecurity2:
		return (pLen == 0x1C);
	case pTypeCamera:
		return (pLen == 0x06);
	case pTypeRemote:
		return (pLen == 0x06);
	case pTypeThermostat1:
		return (pLen == 0x09);
	case pTypeThermostat2:
		return (pLen == 0x06);
	case pTypeThermostat3:
		return (pLen == 0x08);
	case pTypeThermostat4:
		return (pLen == 0x0C);
	case pTypeRadiator1:
		return (pLen == 0x0C);
	case pTypeBBQ:
		return (pLen == 0x0A);
	case pTypeTEMP_RAIN:
		return (pLen == 0x0A);
	case pTypeTEMP:
		return (pLen == 0x08);
	case pTypeHUM:
		return (pLen == 0x08);
	case pTypeTEMP_HUM:
		return (pLen == 0x0A);
	case pTypeBARO:
		return (pLen == 0x09);
	case pTypeTEMP_HUM_BARO:
		return (pLen == 0x0D);
	case pTypeRAIN:
		return (pLen == 0x0B);
	case pTypeWIND:
		return (pLen == 0x10);
	case pTypeUV:
		return (pLen == 0x09);
	case pTypeDT:
		return (pLen == 0x0D);
	case pTypeCURRENT:
		return (pLen == 0x0D);
	case pTypeENERGY:
		return (pLen == 0x11);
	case pTypeCURRENTENERGY:
		return (pLen == 0x13);
	case pTypePOWER:
		return (pLen == 0x0F);
	case pTypeWEIGHT:
		return (pLen == 0x08);
	case pTypeCARTELECTRONIC:
		return (pLen == 0x15 || pLen == 0x11);
	case pTypeRFXSensor:
		return (pLen == 0x07);
	case pTypeRFXMeter:
		return (pLen == 0x0A);
	case pTypeFS20:
		return (pLen == 0x09);
	default:
		return false;//unknown Type
	}
	return false;
}
