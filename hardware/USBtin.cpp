/*
File : USBtin.cpp
Author : X.PONCET
Version : 1.00
Description : This class manage the USBtin CAN gateway.
- Serial connexion management
- CAN connexion management, with some basic command.
- Receiving CAN Frame and switching then to appropriate CAN Layer
- Sending CAN Frame with writeframe, writeframe is virtualized inside each CAN Layer
Supported Layer :
* MultiblocV8 CAN : Scheiber spécific communication

History :
- 2017-10-01 : Creation by X.PONCET

- 2018-01-22 : Update :
# for feature add to MultiblocV8: manual creation up to 127 virtual switch, ability to learn eatch switch to any blocks output

- 2021-04-14 : Formatted code to default style

- 2021-08-10 : Debug mode is shared with all Can Layer

*/
#include "stdafx.h"
#include "USBtin.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

#include <algorithm>
#include <boost/exception/diagnostic_information.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <time.h>

#define USBTIN_BAUD_RATE 115200
#define USBTIN_PARITY boost::asio::serial_port_base::parity::none
#define USBTIN_CARACTER_SIZE 8
#define USBTIN_FLOW_CONTROL boost::asio::serial_port_base::flow_control::none
#define USBTIN_STOP_BITS boost::asio::serial_port_base::stop_bits::one

#define USBTIN_CR 0x0D
#define USBTIN_BELSIGNAL 0x07
#define USBTIN_FIRMWARE_VERSION 0x76
#define USBTIN_HARDWARE_VERSION 0x56
#define USBTIN_SERIAL_NUMBER 0x4E
#define USBTIN_EXT_TRAME_RECEIVE 0x54
#define USBTIN_NOR_TRAME_RECEIVE 0x74
#define USBTIN_EXT_RTR_TRAME_RECEIVE 0x52
#define USBTIN_NOR_RTR_TRAME_RECEIVE 0x72
#define USBTIN_GOODSENDING_NOR_TRAME 0x5A
#define USBTIN_GOODSENDING_EXT_TRAME 0x7A

#define USBTIN_FW_VERSION "v"
#define USBTIN_HW_VERSION "V"
#define USBTIN_RETRY_DELAY 30

#define NoCanDefine 0x00
#define Multibloc_V8 0x01
#define FreeCan 0x02

USBtin::USBtin(const int ID, const std::string &devname, unsigned int BusCanType, unsigned int DebugMode)
	: m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_USBtinRetrycntr = USBTIN_RETRY_DELAY * 5;
	Bus_CANType = BusCanType;
	if (DebugMode == 0)
		m_BOOL_USBtinDebug = false;
	else
		m_BOOL_USBtinDebug = true;
	Init();
}

USBtin::~USBtin()
{
	StopHardware();
}

void USBtin::Init()
{
	m_USBtinBufferpos = 0;
}

bool USBtin::StartHardware()
{
	RequestStart();

	m_USBtinBelErrorCount = 0;
	m_USBtinRetrycntr = USBTIN_RETRY_DELAY * 5; // will force reconnect first thing
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool USBtin::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void USBtin::Do_Work()
{
	int m_V8secCounterBase = 0;
	int msec_counter = 0;
	m_EtapeInitCan = 0;

	while (!IsStopRequested(200))
	{
		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;
			m_V8secCounterBase++;

			if (m_V8secCounterBase % 12 == 0)
			{
				m_LastHeartbeat = mytime(nullptr);
			}

			if (isOpen()) // Serial port open, we can initiate the Can BUS :
			{
				switch (m_EtapeInitCan)
				{
					case 0:
						Log(LOG_STATUS, "Serial port is now open !");
						CloseCanPort(); // more cleaner to close in first, sometimes the gateway maybe already open...
						memset(&m_USBtinBuffer, 0, sizeof(m_USBtinBuffer));
						m_USBtinBelErrorCount = 0;
						m_EtapeInitCan++;
						break;
					case 1:
						GetHWVersion();
						m_EtapeInitCan++;
						break;
					case 2:
						GetFWVersion();
						m_EtapeInitCan++;
						break;
					case 3:
						GetSerialNumber();
						m_EtapeInitCan++;
						break;
					case 4:
						SetBaudRate250Kbd();
						// Log(LOG_STATUS, "BusCantType value: %d ",Bus_CANType);
						if ((Bus_CANType & Multibloc_V8) == Multibloc_V8)
							Log(LOG_STATUS, "MultiblocV8 is Selected !");
						if ((Bus_CANType & FreeCan) == FreeCan)
							Log(LOG_STATUS, "FreeCAN is Selected !");

						if (Bus_CANType == 0)
							Log(LOG_ERROR, "WARNING: No Can management Selected !");
						m_EtapeInitCan++;
						break;
					case 5: // openning can port :
						// Activate the good CAN Layer :
						if ((Bus_CANType & Multibloc_V8) == Multibloc_V8)
						{
							ManageThreadV8(true,m_BOOL_USBtinDebug);
							switch_id_base = m_V8switch_id_base;
						}
						OpenCanPort();
						m_EtapeInitCan++;
						break;

					case 6: // All is good !
						// here nothing to do, the CAN is ok and run....
						break;
				}
			}
		}

		if (!isOpen()) // serial not open
		{
			if (m_USBtinRetrycntr == 0)
			{
				Log(LOG_STATUS, "serial retrying in %d seconds...", USBTIN_RETRY_DELAY);
			}
			m_USBtinRetrycntr++;
			if (m_USBtinRetrycntr / 5 >= USBTIN_RETRY_DELAY)
			{
				m_USBtinRetrycntr = 0;
				m_USBtinBufferpos = 0;
				m_USBtinBuffer[m_USBtinBufferpos] = 0;
				OpenSerialDevice();
			}
		}
	}
	CloseCanPort(); // for security
	terminate();
	m_EtapeInitCan = 0;

	Log(LOG_STATUS, "Can Gateway stopped, goodbye !");
}

bool USBtin::OpenSerialDevice()
{
	// Try to open the Serial Port
	try
	{
		Log(LOG_STATUS, "Using serial port: %s", m_szSerialPort.c_str());
		open(m_szSerialPort, 115200);
	}
	catch (boost::exception &e)
	{
		Log(LOG_ERROR, "Error opening serial port!");
#ifdef _DEBUG
		Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error opening serial port!!!");
		return false;
	}

	m_bIsStarted = true;
	m_USBtinBufferpos = 0;
	memset(&m_USBtinBuffer, 0, sizeof(m_USBtinBuffer));
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });

	sOnConnected(this);

	return true;
}

void USBtin::readCallback(const char *data, size_t len)
{
	if (!m_bEnableReceive)
		return; // receiving not enabled
	if (len > sizeof(m_USBtinBuffer))
	{
		Log(LOG_ERROR, "Warning Error buffer size reaches/ maybe Can is overrun...");
		return;
	}
	ParseData(data, static_cast<int>(len));
}

void USBtin::ParseData(const char *pData, int Len)
{
	char value[30];

	int ii = 0;
	while (ii < Len)
	{
		m_USBtinBuffer[m_USBtinBufferpos] = pData[ii];
		// BEL signal received : appears if the command is allready active or if errors occured on CAN side...
		if (USBTIN_BELSIGNAL == m_USBtinBuffer[m_USBtinBufferpos])
		{
			// reset first char
			m_USBtinBelErrorCount++;
			if (m_USBtinBelErrorCount > 3)
			{ // If more than 3 BEL receive : restart the Gateway !
				Log(LOG_ERROR, "3x times BEL signal receive : restart gateway");
				RestartWithDelay(3);
			}
			else
			{
				Log(LOG_ERROR, "BEL signal (commande allready active or Gateway error) !");
			}
			m_USBtinBufferpos = 0;
		}
		// else CR receive = good reception of a response :
		else if (USBTIN_CR == m_USBtinBuffer[m_USBtinBufferpos])
		{
			m_USBtinBelErrorCount = 0;
			if (m_USBtinBuffer[0] == USBTIN_HARDWARE_VERSION)
			{
				memset(value, 0, sizeof(value));
				strncpy(value, (char *)&(m_USBtinBuffer[1]), 4);
				Log(LOG_STATUS, "Hardware Version: %s", value);
			}
			else if (m_USBtinBuffer[0] == USBTIN_FIRMWARE_VERSION)
			{
				memset(value, 0, sizeof(value));
				strncpy(value, (char *)&(m_USBtinBuffer[1]), 4);
				Log(LOG_STATUS, "Firware Version: %s", value);
			}
			else if (m_USBtinBuffer[0] == USBTIN_SERIAL_NUMBER)
			{
				memset(value, 0, sizeof(value));
				strncpy(value, (char *)&(m_USBtinBuffer[1]), 4);
				Log(LOG_STATUS, "Serial Number: %s", value);
			}
			else if (m_USBtinBuffer[0] == USBTIN_CR)
			{
				Log(LOG_STATUS, "return OK :-)");
			}
			else if (m_USBtinBuffer[0] == USBTIN_EXT_TRAME_RECEIVE)
			{							 // Receive Extended Frame :
				memset(value, 0, sizeof(value));
				strncpy(value, (char *)&(m_USBtinBuffer[1]), 8); // take the "Extended ID" CAN parts and paste it in the char table
				int IDhexNumber;
				sscanf(value, "%x", &IDhexNumber); // IDhexNumber now contains the the digital value of the Ext ID

				memset(value, 0, sizeof(value));
				strncpy(value, (char *)&(m_USBtinBuffer[9]), 1); // read the DLC (lenght of message)
				int DLChexNumber;
				sscanf(value, "%x", &DLChexNumber);

				unsigned int Buffer_Octets[8] = {}; // buffer of 8 bytes(max in the frame)

				unsigned int ValData;
				if (DLChexNumber > 0)
				{ // bytes presents
					for (int i = 0; i <= DLChexNumber; i++)
					{
						ValData = 0;

						memset(value, 0, sizeof(value));
						strncpy(value, (char *)&(m_USBtinBuffer[10 + (2 * i)]), 2); // to fill the Buffer of 8 bytes
						sscanf(value, "%x", &ValData);

						Buffer_Octets[i] = ValData;
					}
				}

				if ((Bus_CANType & Multibloc_V8) == Multibloc_V8)
				{ // multibloc V8 Management !
					Traitement_MultiblocV8(IDhexNumber, DLChexNumber, Buffer_Octets);
					// So in debug mode we can check good reception after treatment :
					if (m_BOOL_USBtinDebug == true)
						Log(LOG_NORM, "Traitement trame multiblocV8 : #%s#", m_USBtinBuffer);
				}

				if (Bus_CANType == 0)
				{ // No management !
					if (m_BOOL_USBtinDebug == true)
						Log(LOG_NORM, "Frame receive not managed: #%s#", m_USBtinBuffer);
				}
			}
			else if (m_USBtinBuffer[0] == USBTIN_NOR_TRAME_RECEIVE)
			{	// Receive Normale Frame (ie: ID is not extended)
				// Log(LOG_NORM,"Normale Frame receive : #%s#",m_buffer);
			}
			else if (m_USBtinBuffer[0] == USBTIN_GOODSENDING_NOR_TRAME || m_USBtinBuffer[0] == USBTIN_GOODSENDING_EXT_TRAME)
			{
				// The Gateway answers USBTIN_GOODSENDING_NOR_TRAME or USBTIN_GOODSENDING_EXT_TRAME each times a frame is sent correctly
				if (m_BOOL_USBtinDebug == true)
					Log(LOG_NORM, "Frame Send OK !"); // So in debug mode we CAN check it, convenient way to check
				// if the CAN communication is in good life ;-)
			}
			else
			{ // over things here...
				if (m_BOOL_USBtinDebug == true)
					Log(LOG_ERROR, "receive command not supported : #%s#", m_USBtinBuffer);
			}
			// rreset of the pointer here
			m_USBtinBufferpos = 0;
		}
		else
		{
			m_USBtinBufferpos++;
		}

		ii++;
	}
}

bool USBtin::writeFrame(const std::string &data)
{
	char length = (uint8_t)data.size();
	if (!isOpen())
	{
		return false;
	}
	if (m_BOOL_USBtinDebug == true)
		Log(LOG_NORM, "write frame to Can Gateway: #%s# ", data.c_str());
	std::string frame;
	frame.append(data);
	frame.append("\x0D"); // add the "carry return" at end
	write(frame);
	return true;
}

void USBtin::GetHWVersion()
{
	std::string data("V");
	writeFrame(data);
	sleep_milliseconds(200);
}
void USBtin::GetSerialNumber()
{
	std::string data("N");
	writeFrame(data);
}
void USBtin::GetFWVersion()
{
	std::string data("v");
	writeFrame(data);
}
void USBtin::SetBaudRate250Kbd()
{
	Log(LOG_STATUS, "Setting Can speed to 250Kb/s");
	std::string data("S5");
	writeFrame(data);
}
void USBtin::OpenCanPort()
{
	Log(LOG_STATUS, "Openning Canport...");
	std::string data("O");
	writeFrame(data);
}
void USBtin::CloseCanPort()
{
	Log(LOG_STATUS, "Closing Canport...");
	std::string data("C");
	writeFrame(data);
}
