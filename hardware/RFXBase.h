#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"
#include "P1MeterBase.h"

#define RX_BUFFER_SIZE 256

class CRFXBase: public P1MeterBase
{
friend class RFXComSerial;
friend class RFXComTCP;
friend class MainWorker;
public:
	enum _eRFXAsyncType
	{
		ATYPE_DISABLED = 0,
		ATYPE_P1_DSMR_1,
		ATYPE_P1_DSMR_2,
		ATYPE_P1_DSMR_3,
		ATYPE_P1_DSMR_4,
		ATYPE_P1_DSMR_5,
		ATYPE_TELEINFO_1200,
		ATYPE_TELEINFO_19200,
	};
	CRFXBase();
    ~CRFXBase();
	std::string m_Version;
	_eRFXAsyncType m_AsyncType;
	int m_NoiseLevel;
	bool SetRFXCOMHardwaremodes(const unsigned char Mode1, const unsigned char Mode2, const unsigned char Mode3, const unsigned char Mode4, const unsigned char Mode5, const unsigned char Mode6);
	void SendResetCommand();
	virtual bool WriteToHardware(const char *pdata, const unsigned char length) = 0;
	void SetAsyncType(_eRFXAsyncType const AsyncType);
protected:
	void Set_Async_Parameters(const _eRFXAsyncType AsyncType);
	void Parse_Async_Data(const uint8_t *pData, const int Len);
	bool onInternalMessage(const unsigned char *pBuffer, const size_t Len, const bool checkValid = true);
	virtual bool StartHardware() = 0;
	virtual bool StopHardware() = 0;

	std::mutex readQueueMutex;
	unsigned char m_rxbuffer[RX_BUFFER_SIZE] = { 0 };
	unsigned char m_rxbufferpos = { 0 };
private:
	static bool CheckValidRFXData(const uint8_t *pData);
	void SendCommand(const unsigned char Cmd);
	std::shared_ptr<std::thread> m_thread;
	bool m_bReceiverStarted;
};

