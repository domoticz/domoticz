#pragma once
#ifndef BUFFEREDASYNCSERIAL_H
#define	BUFFEREDASYNCSERIAL_H

#include "ASyncSerial.h"
#include "RFXBase.h"
#include "serial/serial.h"

class RFXComSerial: public CRFXBase, AsyncSerial
{
public:
	RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate, const _eRFXAsyncType AsyncType);
    ~RFXComSerial();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	bool UploadFirmware(const std::string &szFilename);
	float GetUploadPercentage(); //returns -1 when failed
	std::string GetUploadMessage();
private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice(const bool bIsFirmwareUpgrade=false);
	void Do_Work();

	bool UpgradeFirmware();
	bool Read_TX_PKT();
	bool Write_TX_PKT(const unsigned char *pdata, size_t length, int max_retry = 3);
	bool Handle_RX_PKT(const unsigned char *pdata, size_t length);
	bool Read_Firmware_File(const char *szFilename, std::map<unsigned long, std::string>& fileBuffer);
	bool EraseMemory(const int StartAddress, const int StopAddress);

	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	serial::Serial m_serial;
	bool m_bStartFirmwareUpload;
	std::string m_szFirmwareFile;
	std::string m_szUploadMessage;
	float m_FirmwareUploadPercentage;
	bool m_bInBootloaderMode;
	bool m_bHaveRX;
	unsigned char m_rx_input_buffer[512];
	int m_rx_tot_bytes;
	int m_retrycntr;
    void readCallback(const char *data, size_t len);
};

#endif //BUFFEREDASYNCSERIAL_H
