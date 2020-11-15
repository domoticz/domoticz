#pragma once

#include "ASyncSerial.h"
#include "KMTronicBase.h"

class KMTronicSerial : public AsyncSerial, public KMTronicBase
{
      public:
	KMTronicSerial(int ID, const std::string &devname);
	~KMTronicSerial() override = default;

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
	bool WriteInt(const unsigned char *data, size_t len, bool bWaitForReturn) override;
};
