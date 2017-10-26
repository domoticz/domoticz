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

#define USBTIN_BAUD_RATE         115200
#define USBTIN_PARITY            boost::asio::serial_port_base::parity::none
#define USBTIN_CARACTER_SIZE      8
#define USBTIN_FLOW_CONTROL      boost::asio::serial_port_base::flow_control::none
#define USBTIN_STOP_BITS         boost::asio::serial_port_base::stop_bits::one

#define	TIME_3sec				3000000
#define	TIME_1sec				1000000
#define	TIME_500ms				500000
#define	TIME_200ms				200000
#define	TIME_100ms				100000
#define	TIME_10ms				10000
#define	TIME_5ms				5000

#define	USBTIN_CR							0x0D
#define	USBTIN_BELSIGNAL					0x07
#define	USBTIN_FIRMWARE_VERSION				0x76
#define	USBTIN_HARDWARE_VERSION				0x56
#define	USBTIN_SERIAL_NUMBER				0x4E
#define	USBTIN_EXT_TRAME_RECEIVE			0x54
#define	USBTIN_NOR_TRAME_RECEIVE			0x74
#define	USBTIN_EXT_RTR_TRAME_RECEIVE		0x52
#define	USBTIN_NOR_RTR_TRAME_RECEIVE		0x72
#define USBTIN_GOODSENDING_NOR_TRAME		0x5A
#define USBTIN_GOODSENDING_EXT_TRAME		0x7A

#define	USBTIN_FW_VERSION	"v"
#define	USBTIN_HW_VERSION	"V"
#define USBTIN_RETRY_DELAY  30

#define NoCanDefine		0x00
#define	Multibloc_V8	0x01
#define FreeCan			0x02

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
