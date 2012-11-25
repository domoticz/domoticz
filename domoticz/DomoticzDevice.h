#pragma once

#include <boost/signals2.hpp>

//Base class with functions all notification systems should have
#define RX_BUFFER_SIZE 40

class CDomoticzDeviceBase
{
	friend class RFXComSerial;
	friend class RFXComTCP;
public:
	CDomoticzDeviceBase();
	~CDomoticzDeviceBase() {};

	virtual bool Start()=0;
	virtual bool Stop()=0;
	virtual void WriteToDevice(const char *pdata, const unsigned char length)=0;

	bool IsStarted() { return m_bIsStarted; }

	int HwdID;
	std::string Name;
	unsigned char m_SeqNr;
	unsigned char m_rxbufferpos;
	bool m_bEnableReceive;
	boost::signals2::signal<void(const int HwdID, const unsigned char *pRXCommand)>	sDecodeRXMessage;
	boost::signals2::signal<void(CDomoticzDeviceBase *pDevice)>	sOnConnected;
private:
	void onRFXMessage(const unsigned char *pBuffer, const size_t Len);
	unsigned char m_rxbuffer[RX_BUFFER_SIZE];
	bool m_bIsStarted;
};

