/*
File : USBtin.h
Author : Xavier PONCET BIJONNET
Version : 1.0
Description : This class manage the USBtin Gateway with Scheiber CanBus protocol

History :
- 2017-01-01 : Creation base

*/

#pragma once
#include <vector>
#include "ASyncSerial.h"
#include "USBtin_MultiblocV8.h"
#include "DomoticzHardware.h"

class USBtin : public USBtin_MultiblocV8,AsyncSerial
{
	

public:
	USBtin(const int ID, const std::string& devname,unsigned int BusCanType,unsigned int DebugMode/*,unsigned int baud_rate = USBTIN_BAUD_RATE*/);
	~USBtin();
	std::string m_szSerialPort;
	void Restart();
	
	
private:
	bool StartHardware();
	bool StopHardware();
	
	volatile bool m_stoprequested;
	bool OpenSerialDevice();
	unsigned int EtapeInitCan;
	
	int m_retrycntr;	
	int BelErrorCount;
	char m_bufferUSBtin[390]; //buffer capable de stocker 15 trames en 1 fois
	int m_bufferpos;
	unsigned int Bus_CANType;
	bool BOOL_Debug; //1 = activ
	
	boost::asio::serial_port_base::parity m_iOptParity;
	boost::asio::serial_port_base::character_size m_iOptCsize;
	boost::asio::serial_port_base::flow_control m_iOptFlow;
	boost::asio::serial_port_base::stop_bits m_iOptStop;
	
	boost::shared_ptr<boost::thread> m_thread;
	void Do_Work();
	// Read callback, stores data in the buffer
	void readCallback(const char *data, size_t len);
	void ParseData(const char *pData, int Len);
	void Init();
	bool writeFrame(const std::string&);
	void ConfigCanPort();
	void GetHWVersion();
	void GetFWVersion();
	void GetSerialNumber();
	void SetBaudRate250Kbd();
	void OpenCanPort();
	void CloseCanPort();
};
