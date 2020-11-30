#pragma once

#include "ASyncSerial.h"
#include "S0MeterBase.h"

class S0MeterSerial : public AsyncSerial, public S0MeterBase
{
      public:
	S0MeterSerial(int ID, const std::string &devname, unsigned int baud_rate);
	~S0MeterSerial() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void readCallback(const char *data, size_t len);
};
