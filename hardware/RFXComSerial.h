#pragma once
#ifndef BUFFEREDASYNCSERIAL_H
#define BUFFEREDASYNCSERIAL_H

#include "ASyncSerial.h"
#include "RFXBase.h"

class RFXComSerial : public CRFXBase, AsyncSerial
{
public:
	RFXComSerial(int ID, const std::string& devname, unsigned int baud_rate, _eRFXAsyncType AsyncType);
	~RFXComSerial() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;

private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice(bool bIsFirmwareUpgrade = false);
	void Do_Work();

	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	int m_retrycntr;
	void readCallback(const char* data, size_t len);
};

#endif // BUFFEREDASYNCSERIAL_H
