/*
File : USBtin.h
Author : X.PONCET
Version : 1.00
Description : This class manage the USBtin CAN gateway.
- Serial connexion management
- CAN connexion management, with some basic command.
- Receiving CAN Frame and switching then to appropriate CAN Layer
- Sending CAN Frame with writeframe, writeframe is virtualized inside each CAN Layer
Supported Layer :
* MultiblocV8 CAN : Scheiber sp�cific communication

History :
- 2017-10-01 : Creation by X.PONCET

*/

#pragma once
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
	unsigned int Bus_CANType;
	unsigned long switch_id_base;

private:
	volatile bool m_stoprequested;
	unsigned int m_EtapeInitCan;
	int m_USBtinRetrycntr;
	int m_USBtinBelErrorCount;
	char m_USBtinBuffer[390]; //buffer capable de stocker 15 trames en 1 fois
	int m_USBtinBufferpos;
	bool m_BOOL_USBtinDebug; //1 = activ

	boost::asio::serial_port_base::parity m_iOptParity;
	boost::asio::serial_port_base::character_size m_iOptCsize;
	boost::asio::serial_port_base::flow_control m_iOptFlow;
	boost::asio::serial_port_base::stop_bits m_iOptStop;
	std::shared_ptr<std::thread> m_thread;

	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	// Read callback, stores data in the buffer
	void readCallback(const char *data, size_t len);
	void ParseData(const char *pData, int Len);
	void Init();
	bool writeFrame(const std::string&) override;
	void ConfigCanPort();
	void GetHWVersion();
	void GetFWVersion();
	void GetSerialNumber();
	void SetBaudRate250Kbd();
	void OpenCanPort();
	void CloseCanPort();

};
