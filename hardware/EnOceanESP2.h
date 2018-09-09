#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define ENOCEAN2_READ_BUFFER_SIZE 40

class CEnOceanESP2: public AsyncSerial, public CDomoticzHardwareBase
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
	CEnOceanESP2(const int ID, const std::string& devname, const int type);
    ~CEnOceanESP2();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SendDimmerTeachIn(const char *pdata, const unsigned char length);
	unsigned long m_id_base;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();
	void Add2SendQueue(const char* pData, const size_t length);
	float GetValueRange(const float InValue, const float ScaleMax, const float ScaleMin=0, const float RangeMax=255, const float RangeMin=0);
	void readCallback(const char *data, size_t len);
private:
	_eEnOcean_Receive_State m_receivestate;
	int m_wantedlength;

	std::shared_ptr<std::thread> m_thread;
    int m_Type;
	std::string m_szSerialPort;

	// Create a circular buffer.
    unsigned char m_buffer[ENOCEAN2_READ_BUFFER_SIZE];
	int m_bufferpos;
	int m_retrycntr;

	std::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;
};

