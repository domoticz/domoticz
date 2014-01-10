#pragma once

#include <vector>
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define ENOCEAN_READ_BUFFER_SIZE 40

class CEnOcean: public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eEnOcean_Receive_State
	{
		ERS_SYNC1=0,
		ERS_SYNC2,
		ERS_LENGTH,
		ERS_DATA,
		ERS_CHECKSUM
	};
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
	unsigned long m_id_base;
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();
	void Add2SendQueue(const char* pData, const size_t length);

	_eEnOcean_Receive_State m_receivestate;
	int m_wantedlength;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
    int m_Type;
	std::string m_szSerialPort;

	// Create a circular buffer.
    unsigned char m_buffer[ENOCEAN_READ_BUFFER_SIZE];
	int m_bufferpos;
	int m_retrycntr;

	boost::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;

	/**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);

};

