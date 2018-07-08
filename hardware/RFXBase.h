#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define RFLINK_READ_BUFFER_SIZE 65*1024

class CRFXBase: public CDomoticzHardwareBase
{
friend class RFXComSerial;
friend class RFXComTCP;
friend class MainWorker;
public:
	CRFXBase();
    ~CRFXBase();
	std::string m_Version;
	bool SetRFXCOMHardwaremodes(const unsigned char Mode1, const unsigned char Mode2, const unsigned char Mode3, const unsigned char Mode4, const unsigned char Mode5, const unsigned char Mode6);
	void SendResetCommand();
private:
	bool onInternalMessage(const unsigned char *pBuffer, const size_t Len);
	static bool CheckValidRFXData(const uint8_t *pData);
	void SendCommand(const unsigned char Cmd);
	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
	bool m_bReceiverStarted;
};

