#pragma once

#include <vector>
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define ENOCEAN3_READ_BUFFER_SIZE 65*1024

class CEnOceanESP3: public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eEnOcean_Receive_State
	{
		ERS_SYNCBYTE=0,
		ERS_HEADER,
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
	CEnOceanESP3(const int ID, const std::string& devname, const int type);

    ~CEnOceanESP3();
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SendDimmerTeachIn(const char *pdata, const unsigned char length);
	unsigned long m_id_base;
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();
	void Add2SendQueue(const char* pData, const size_t length);
	float GetValueRange(const float InValue, const float ScaleMax, const float ScaleMin=0, const float RangeMax=255, const float RangeMin=0);

	bool sendFrame(unsigned char frametype, unsigned char *databuf, unsigned short datalen, unsigned char *optdata, unsigned char optdatalen);
	bool sendFrameQueue(unsigned char frametype, unsigned char *databuf, unsigned short datalen, unsigned char *optdata, unsigned char optdatalen);

	void ParseRadioDatagram();

	_eEnOcean_Receive_State m_receivestate;
	int m_wantedlength;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
    int m_Type;
	std::string m_szSerialPort;

	bool m_bBaseIDRequested;

	// Create a circular buffer.
	unsigned char m_ReceivedPacketType;
	int				m_DataSize;
	int				m_OptionalDataSize;
    unsigned char m_buffer[ENOCEAN3_READ_BUFFER_SIZE];
	int m_bufferpos;
	int m_retrycntr;

	boost::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;

	/**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);

};

