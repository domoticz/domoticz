#pragma once

#include <boost/signals2.hpp>
#include "../main/RFXNames.h"

//Base class with functions all notification systems should have
#define RX_BUFFER_SIZE 40

class MainWorker;

class CDomoticzHardwareBase
{
	friend class RFXComSerial;
	friend class RFXComTCP;
	friend class DomoticzTCP;
	friend class P1MeterBase;
	friend class P1MeterSerial;
	friend class P1MeterTCP;
	friend class CYouLess;
	friend class CRego6XXSerial;
	friend class CDavisLoggerSerial;
	friend class CBMP085;
	friend class CPiFace;
	friend class S0MeterSerial;
	friend class S0MeterTCP;

public: 
	CDomoticzHardwareBase();
	virtual ~CDomoticzHardwareBase();

	bool Start();
	bool Stop();
	virtual void WriteToHardware(const char *pdata, const unsigned char length)=0;

	bool IsStarted() { return m_bIsStarted; }

	int m_HwdID;
	std::string Name;
	_eHardwareTypes HwdType;
	unsigned char m_SeqNr;
	unsigned char m_rxbufferpos;
	bool m_bEnableReceive;
	MainWorker *m_pMainWorker;
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand)> sDecodeRXMessage;
	boost::signals2::signal<void(CDomoticzHardwareBase *pDevice)> sOnConnected;
private:
	boost::mutex readQueueMutex;
	virtual bool StartHardware()=0;
	virtual bool StopHardware()=0;
	void onRFXMessage(const unsigned char *pBuffer, const size_t Len);
	unsigned char m_rxbuffer[RX_BUFFER_SIZE];
	bool m_bIsStarted;
};

