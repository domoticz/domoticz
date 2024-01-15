#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"
#include "P1MeterBase.h"

#define RX_BUFFER_SIZE 512

class CRFXBase : public P1MeterBase
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
	~CRFXBase() override = default;
	std::string m_Version;
	_eRFXAsyncType m_AsyncType;
	int m_NoiseLevel;
	bool SetRFXCOMHardwaremodes(unsigned char Mode1, unsigned char Mode2, unsigned char Mode3, unsigned char Mode4, unsigned char Mode5, unsigned char Mode6);
	void SendResetCommand();
	bool WriteToHardware(const char *pdata, unsigned char length) override = 0;
	void SetAsyncType(_eRFXAsyncType AsyncType);

      protected:
	void Set_Async_Parameters(_eRFXAsyncType AsyncType);
	void Parse_Async_Data(const uint8_t *pData, int Len);
	bool onInternalMessage(const unsigned char *pBuffer, size_t Len, bool checkValid = true);
	bool StartHardware() override = 0;
	bool StopHardware() override = 0;

	std::mutex readQueueMutex;
	unsigned char m_rxbuffer[RX_BUFFER_SIZE] = { 0 };
	uint16_t m_rxbufferpos = { 0 };

      private:
	static bool CheckValidRFXData(const uint8_t *pData);
	void SendCommand(unsigned char Cmd);
	std::shared_ptr<std::thread> m_thread;
	bool m_bReceiverStarted;
};
