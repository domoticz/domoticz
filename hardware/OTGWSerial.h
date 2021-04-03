#pragma once

#include "ASyncSerial.h"
#include "OTGWBase.h"

class OTGWSerial : public AsyncSerial, public OTGWBase
{
      public:
	OTGWSerial(int ID, const std::string &devname, unsigned int baud_rate, int Mode1, int Mode2, int Mode3, int Mode4, int Mode5, int Mode6);
	~OTGWSerial() override = default;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();

	void Do_Work();
	bool WriteInt(const unsigned char *pData, unsigned char Len) override;
	int m_retrycntr;
	std::shared_ptr<std::thread> m_thread;
	void readCallback(const char *data, size_t len);
};
