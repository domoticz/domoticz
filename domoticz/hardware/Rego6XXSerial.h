#pragma once
#ifndef BUFFEREDRego6XXASYNCSERIAL_H
#define	BUFFEREDRego6XXASYNCSERIAL_H

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define Rego6XX_READ_BUFFER_SIZE 32

typedef struct _tRego6XXTemp {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	char          ID[25];
	float         temperature;
} Rego6XXTemp;

typedef struct _tRego6XXStatus {
	unsigned char   len;
	unsigned char   type;
	unsigned char   subtype;
	char            ID[25];
	int             value;
} Rego6XXStatus;


class CRego6XXSerial: public AsyncSerial, public CDomoticzHardwareBase
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
	CRego6XXSerial(const int ID, const std::string& devname, const int type);

    ~CRego6XXSerial();
	std::string m_szSerialPort;

	Rego6XXTemp	    m_Rego6XXTemp;
	Rego6XXStatus	m_Rego6XXValue;
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();
	void ParseData();

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;
	int m_pollDelaycntr;
	int m_pollcntr;
    int m_regoType;

	// Create a circular buffer.
    char m_readBuffer[Rego6XX_READ_BUFFER_SIZE];
	volatile unsigned char m_readBufferHead;
	volatile unsigned char m_readBufferTail;

	/**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);

};

#endif //BUFFEREDRego6XXASYNCSERIAL_H
