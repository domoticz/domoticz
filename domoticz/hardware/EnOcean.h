#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define ENOCEAN_READ_BUFFER_SIZE 32

class CEnOcean: public AsyncSerial, public CDomoticzHardwareBase
{
public:
    /**
    * Opens a serial device.
    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
    * \param baud_rate serial baud rate
    * \param opt_parity serial parity, default even
    * \param opt_csize serial character size, default 7bit
    * \param opt_flow serial flow control, default none
    * \param opt_stop serial stop bits, default 1
    * \throws boost::system::system_error if cannot open the
    * serial device
    */
	CEnOcean(const int ID, const std::string& devname, const int type);

    ~CEnOcean();
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
    int m_Type;
	std::string m_szSerialPort;

	// Create a circular buffer.
    unsigned char m_buffer[ENOCEAN_READ_BUFFER_SIZE];
	int m_bufferpos;
	int m_wantedlength;
	int m_retrycntr;

	/**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);

};

