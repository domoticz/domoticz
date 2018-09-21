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
	CRego6XXSerial(const int ID, const std::string& devname, const int type);
    ~CRego6XXSerial();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();
private:
	std::string m_szSerialPort;
	Rego6XXTemp	    m_Rego6XXTemp;
	Rego6XXStatus	m_Rego6XXValue;

	std::shared_ptr<std::thread> m_thread;
	int m_retrycntr;
	int m_pollDelaycntr;
	unsigned int m_pollcntr;
    int m_regoType;
    unsigned int m_errorcntr;

	// Create a circular buffer.
    char m_readBuffer[Rego6XX_READ_BUFFER_SIZE];
	volatile unsigned char m_readBufferHead;
	volatile unsigned char m_readBufferTail;
    void readCallback(const char *data, size_t len);
};

#endif //BUFFEREDRego6XXASYNCSERIAL_H
