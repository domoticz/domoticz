#pragma once

#include "ASyncSerial.h"
#include "CurrentCostMeterBase.h"

class CurrentCostMeterSerial: public AsyncSerial, public CurrentCostMeterBase
{
public:
    /**
    * Opens a CurrentCost serial device.
    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
    * \throws boost::system::system_error if cannot open the
    * serial device
    */
	CurrentCostMeterSerial(const int ID, const std::string& devname, unsigned int baudRate);
    virtual ~CurrentCostMeterSerial();

	virtual bool WriteToHardware(const char *pdata, const unsigned char length) override;
protected:
	virtual bool StartHardware() override;
	virtual bool StopHardware() override;

private:
	void Do_Work();
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);

	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
	std::string m_szSerialPort;
	unsigned int m_baudRate;

};
