#pragma once

#include "ASyncSerial.h"
#include "CurrentCostMeterBase.h"

class CurrentCostMeterSerial : public AsyncSerial, public CurrentCostMeterBase
{
      public:
	/**
	 * Opens a CurrentCost serial device.
	 * \param devname serial device name, example "/dev/ttyS0" or "COM1"
	 * \throws boost::system::system_error if cannot open the
	 * serial device
	 */
	CurrentCostMeterSerial(int ID, const std::string &devname, unsigned int baudRate);
	~CurrentCostMeterSerial() override = default;

	bool WriteToHardware(const char *pdata, unsigned char length) override;

      protected:
	bool StartHardware() override;
	bool StopHardware() override;

      private:
	void Do_Work();
	/**
	 * Read callback, stores data in the buffer
	 */
	void readCallback(const char *data, size_t len);

	std::shared_ptr<std::thread> m_thread;
	std::string m_szSerialPort;
	unsigned int m_baudRate;
};
