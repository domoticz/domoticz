#pragma once

#include "ASyncSerial.h"
#include "MySensorsBase.h"

class MySensorsSerial : public AsyncSerial, public MySensorsBase
{
      public:
	MySensorsSerial(int ID, const std::string &devname, int Mode1);
	~MySensorsSerial() override = default;

      private:
	bool StartHardware() override;
	bool StopHardware() override;

	unsigned int m_iBaudRate;
	std::shared_ptr<std::thread> m_thread;
	int m_retrycntr;
	void Do_Work();
	bool OpenSerialDevice();

	/**
	 * Read callback, stores data in the buffer
	 */
	void readCallback(const char *data, size_t len);
	void WriteInt(const std::string &sendStr) override;
};
