#pragma once

#include <vector>
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define RFLINK_READ_BUFFER_SIZE 65*1024

class CRFXBase: public CDomoticzHardwareBase
{
friend class RFXComSerial;
friend class RFXComTCP;
public:
	CRFXBase();
    ~CRFXBase();
	std::string m_Version;
};

