#pragma once

#include <vector>
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
private:
	bool onInternalMessage(const unsigned char *pBuffer, const size_t Len);
	static bool CheckValidRFXData(const uint8_t *pData);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	bool m_bReceiverStarted;
};

