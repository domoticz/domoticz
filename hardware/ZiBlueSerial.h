#pragma once

#include "ASyncSerial.h"
#include "ZiBlueBase.h"

class CZiBlueSerial : public AsyncSerial, public CZiBlueBase
{
      public:
	CZiBlueSerial(int ID, const std::string &devname);
	~CZiBlueSerial() override = default;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool WriteInt(const std::string &sendString) override;
	bool WriteInt(const uint8_t *pData, size_t length) override;
	std::shared_ptr<std::thread> m_thread;
	std::string m_szSerialPort;
	void readCallback(const char *data, size_t len);
};
