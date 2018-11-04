#include "stdafx.h"
#include "RFXBase.h"
#include "../main/mainworker.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"

CRFXBase::CRFXBase()
{
	m_NoiseLevel = 0;
	m_AsyncType = ATYPE_DISABLED;
}

CRFXBase::~CRFXBase()
{
}

bool CRFXBase::onInternalMessage(const unsigned char *pBuffer, const size_t Len, const bool checkValid/* = true*/)
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
			Log(LOG_ERROR, "input buffer out of sync, going to restart!....");
			m_rxbufferpos = 0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			if (!checkValid || CheckValidRFXData((uint8_t*)&m_rxbuffer))
				sDecodeRXMessage(this, (const uint8_t*)&m_rxbuffer, NULL, -1);
			else
				Log(LOG_ERROR, "Invalid data received!....");

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
	case pTypeASYNCPORT:
		return (pLen == 0x0B);
	case pTypeASYNCDATA:
		return (pLen > 0x03);
	default:
		return false;//unknown Type
	}
	return false;
}

void CRFXBase::SetAsyncType(_eRFXAsyncType const AsyncType)
{
	m_AsyncType = AsyncType;
	Set_Async_Parameters(m_AsyncType);
}

void CRFXBase::Set_Async_Parameters(const _eRFXAsyncType AsyncType)
{
	tRBUF cmd;
	cmd.ASYNCPORT.packetlength = sizeof(cmd.ASYNCPORT) - 1;
	cmd.ASYNCPORT.packettype = pTypeASYNCPORT;
	cmd.ASYNCPORT.subtype = sTypeASYNCconfig;
	cmd.ASYNCPORT.seqnbr = m_SeqNr++;

	uint8_t baudrate = 0, parity = 0, databits = 0, stopbits = 0, polarity = 0;
	uint8_t cmnd = asyncdisable;

	switch (AsyncType)
	{
	case ATYPE_P1_DSMR_1:
	case ATYPE_P1_DSMR_2:
	case ATYPE_P1_DSMR_3:
		baudrate = asyncbaud9600;
		parity = asyncParityEven;
		databits = asyncDatabits7;
		stopbits = asyncStopbits1;
		polarity = asyncPolarityInvers;
		cmnd = asyncreceiveP1;
		Log(LOG_STATUS, "Async mode set to: 'DSMR 1/2/3'");
		break;
	case ATYPE_P1_DSMR_4:
	case ATYPE_P1_DSMR_5:
		baudrate = asyncbaud115200;
		parity = asyncParityNo;
		databits = asyncDatabits8;
		stopbits = asyncStopbits1;
		polarity = asyncPolarityInvers;
		cmnd = asyncreceiveP1;
		Log(LOG_STATUS, "Async mode set to: 'DSMR 4/5'");
		break;
	case ATYPE_TELEINFO_1200://not handled yet!
		baudrate = asyncbaud1200;
		parity = asyncParityEven;
		databits = asyncDatabits7;
		stopbits = asyncStopbits1;
		polarity = asyncPolarityInvers;
		cmnd = asyncreceiveTeleinfo;
		Log(LOG_STATUS, "Async mode set to: 'Teleinfo 1200' (not implemented yet!)");
		break;
	case ATYPE_TELEINFO_19200://not handled yet!
		baudrate = asyncbaud19200;
		parity = asyncParityEven;
		databits = asyncDatabits7;
		stopbits = asyncStopbits1;
		polarity = asyncPolarityInvers;
		Log(LOG_STATUS, "Async mode set to: 'Teleinfo 19200' (not implemented yet!)");
		break;
	case ATYPE_DISABLED:
		Log(LOG_STATUS, "Async Disabled");
		break;
	default:
		Log(LOG_ERROR, "unknown ASync type received!....");
		return; //not handled yet!
	}

	cmd.ASYNCPORT.cmnd = cmnd;
	cmd.ASYNCPORT.baudrate = baudrate;
	cmd.ASYNCPORT.parity = parity;
	cmd.ASYNCPORT.databits = databits;
	cmd.ASYNCPORT.stopbits = stopbits;
	cmd.ASYNCPORT.polarity = polarity;
	cmd.ASYNCPORT.filler1 = 0;
	cmd.ASYNCPORT.filler2 = 0;

	WriteToHardware((const char*)&cmd, sizeof(cmd.ASYNCPORT));
}

void CRFXBase::Parse_Async_Data(const uint8_t *pData, const int Len)
{
	switch (m_AsyncType)
	{
	case ATYPE_P1_DSMR_1:
	case ATYPE_P1_DSMR_2:
	case ATYPE_P1_DSMR_3:
	case ATYPE_P1_DSMR_4:
	case ATYPE_P1_DSMR_5:
	default:
		ParseP1Data(pData, Len, false, 0);
		break;
	}
}

void CRFXBase::SendCommand(const unsigned char Cmd)
{
	tRBUF cmd;
	cmd.ICMND.packetlength = 13;
	cmd.ICMND.packettype = 0;
	cmd.ICMND.subtype = 0;
	cmd.ICMND.seqnbr = m_SeqNr++;
	cmd.ICMND.cmnd = Cmd;
	cmd.ICMND.freqsel = 0;
	cmd.ICMND.xmitpwr = 0;
	cmd.ICMND.msg3 = 0;
	cmd.ICMND.msg4 = 0;
	cmd.ICMND.msg5 = 0;
	cmd.ICMND.msg6 = 0;
	cmd.ICMND.msg7 = 0;
	cmd.ICMND.msg8 = 0;
	cmd.ICMND.msg9 = 0;
	WriteToHardware((const char*)&cmd, sizeof(cmd.ICMND));
}

bool CRFXBase::SetRFXCOMHardwaremodes(const unsigned char Mode1, const unsigned char Mode2, const unsigned char Mode3, const unsigned char Mode4, const unsigned char Mode5, const unsigned char Mode6)
{
	tRBUF Response;
	Response.ICMND.packetlength = sizeof(Response.ICMND) - 1;
	Response.ICMND.packettype = pTypeInterfaceControl;
	Response.ICMND.subtype = sTypeInterfaceCommand;
	Response.ICMND.seqnbr = m_SeqNr++;
	Response.ICMND.cmnd = cmdSETMODE;
	Response.ICMND.freqsel = Mode1;
	Response.ICMND.xmitpwr = Mode2;
	Response.ICMND.msg3 = Mode3;
	Response.ICMND.msg4 = Mode4;
	Response.ICMND.msg5 = Mode5;
	Response.ICMND.msg6 = Mode6;
	if (!WriteToHardware((const char*)&Response, sizeof(Response.ICMND)))
		return false;
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&Response, NULL, -1);
	//Save it also
	SendCommand(cmdSAVE);

	m_rxbufferpos = 0;

	return true;
}

void CRFXBase::SendResetCommand()
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	m_bEnableReceive = false;
	m_rxbufferpos = 0;
	//Send Reset
	SendCommand(cmdRESET);
	//wait at least 500ms
	sleep_milliseconds(500);
	m_rxbufferpos = 0;
	m_bEnableReceive = true;

	SendCommand(cmdStartRec);
	sleep_milliseconds(50);
	SendCommand(cmdSTATUS);
}

