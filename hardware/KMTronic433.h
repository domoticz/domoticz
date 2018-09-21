#pragma once

#include "ASyncSerial.h"
#include "KMTronicBase.h"

class KMTronic433 : public AsyncSerial, public KMTronicBase
{
public:
	KMTronic433(const int ID, const std::string& devname);
	~KMTronic433();
private:
	bool StartHardware() override;
	bool StopHardware() override;

	void GetRelayStates();
	int m_iQueryState;
	bool m_bHaveReceived;

	std::shared_ptr<std::thread> m_thread;
	int m_retrycntr;
	void Do_Work();
	bool OpenSerialDevice();

	/**
	* Read callback, stores data in the buffer
	*/
	void readCallback(const char *data, size_t len);
	bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn) override;
};

