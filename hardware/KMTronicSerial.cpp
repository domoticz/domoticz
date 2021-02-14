#include "stdafx.h"
#include "KMTronicSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include <ctime>

//#define DEBUG_KMTronic

#define RETRY_DELAY 30

KMTronicSerial::KMTronicSerial(const int ID, const std::string& devname)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate = 9600;
	m_iQueryState = 0;
	m_retrycntr = RETRY_DELAY - 2;
	m_bHaveReceived = false;
}

bool KMTronicSerial::StartHardware()
{
	RequestStart();

	m_bDoInitialQuery = true;
	m_iQueryState = 0;

	m_retrycntr = RETRY_DELAY-2; //will force reconnect first thing

	//Start worker thread
	m_bIsStarted = true;
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool KMTronicSerial::StopHardware()
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

void KMTronicSerial::Do_Work()
{
	int sec_counter = 0;

	_log.Log(LOG_STATUS, "KMTronic: Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (!isOpen())
		{
			if (m_retrycntr == 0)
			{
				_log.Log(LOG_STATUS, "KMTronic: retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				if (OpenSerialDevice())
				{
					GetRelayStates();
				}
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS, "KMTronic: Worker stopped...");
}

bool KMTronicSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "KMTronic: Using serial port: %s", m_szSerialPort.c_str());
#ifndef WIN32
		openOnlyBaud(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
#else
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
#endif
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "KMTronic: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "KMTronic: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted = true;
	m_bufferpos = 0;
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	sOnConnected(this);
	return true;
}

void KMTronicSerial::readCallback(const char *data, size_t len)
{
	if (!m_bIsStarted)
		return;

	if (len > sizeof(m_buffer))
		return;

	m_bHaveReceived = true;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	memcpy(m_buffer, data, len);
	m_bufferpos = len;
}

bool KMTronicSerial::WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn)
{
	if (!isOpen())
		return false;
	m_bHaveReceived = false;
	write((const char*)data, len);
	if (!bWaitForReturn)
		return true;
	sleep_milliseconds(100);
	return (m_bHaveReceived == true);
}

void KMTronicSerial::GetRelayStates()
{
	unsigned char SendBuf[3];
	int ii;

	m_TotRelais = 0;

	//First check if we are the USB 4/8 version
	SendBuf[0] = 0xFF;
	SendBuf[1] = 0x09;
	SendBuf[2] = 0x00;

	//Check if we have the 485 boards (max 6)
	bool bIs485 = false;
	for (int iBoard = 0; iBoard < 6; iBoard++)
	{
		SendBuf[1] = (uint8_t)(0xA1 + iBoard);
		if (WriteInt(SendBuf, 3, true))
		{
			bIs485 = true;
			if (m_buffer[1] == 0xA1 + iBoard)
			{
				m_bufferpos -= 2;
				if (m_bufferpos > Max_KMTronic_Relais)
					m_bufferpos = Max_KMTronic_Relais;
				for (ii = 0; ii < 8; ii++)
				{
					bool bIsOn = (m_buffer[2 + ii] == 1);
					if (m_bRelaisStatus[ii] != bIsOn)
					{
						m_bRelaisStatus[ii] = bIsOn;
					}
					std::stringstream sstr;
					int iRelay = (iBoard * 8) + ii + 1;
					sstr << "Board" << int(iBoard + 1) << " - " << int(ii + 1);
					SendSwitch(iRelay, 1, 255, bIsOn, 0, sstr.str(), m_Name);
					_log.Debug(DEBUG_HARDWARE, "KMTronic: %s = %s", sstr.str().c_str(), (bIsOn) ? "On" : "Off");
					if (iRelay > m_TotRelais)
						m_TotRelais = iRelay;
				}
			}
		}
	}
	if (bIs485)
	{
		//It could be that maybe one of the boards is turned off for various reasons,
		//for this, we assume we have all 6 boards available
		m_TotRelais = 48;
		return;
	}

	//Check if we are the USB 4/8 version
	SendBuf[0] = 0xFF;
	SendBuf[1] = 0x09;
	SendBuf[2] = 0x00;

	if (WriteInt(SendBuf, 3, true))
	{
		if (m_bufferpos > Max_KMTronic_Relais)
			m_bufferpos = Max_KMTronic_Relais;
		m_TotRelais = m_bufferpos;
		for (ii = 0; ii < m_TotRelais; ii++)
		{
			bool bIsOn = (m_buffer[ii] == 1);
			if (m_bRelaisStatus[ii] != bIsOn)
			{
				m_bRelaisStatus[ii] = bIsOn;
			}
			std::stringstream sstr;
			int iRelay = (ii + 1);
			sstr << "Relay " << iRelay;
			SendSwitch(iRelay, 1, 255, bIsOn, 0, sstr.str(), m_Name);
			_log.Debug(DEBUG_HARDWARE, "KMTronic: %s = %s", sstr.str().c_str(), (bIsOn) ? "On" : "Off");
			if (iRelay > m_TotRelais)
				m_TotRelais = iRelay;
		}
		return;
	}

	//Check if we can get status of the 1/2 board
	SendBuf[0] = 0xFF;
	SendBuf[2] = 0x03;
	for (ii = 0; ii < Max_KMTronic_Relais; ii++)
	{
		SendBuf[1] = (uint8_t)(ii+1);
		if (WriteInt(SendBuf, 3,true))
		{
			if (m_bufferpos == 3)
			{
				if (m_buffer[1] == (ii + 1))
				{
					bool bIsOn = (m_buffer[2] == 1);
					if (m_bRelaisStatus[ii] != bIsOn)
					{
						m_bRelaisStatus[ii] = bIsOn;
					}
					std::stringstream sstr;
					int iRelay = ii + 1;
					sstr << "Relay " << iRelay;
					SendSwitch(iRelay, 1, 255, bIsOn, 0, sstr.str(), m_Name);
					_log.Log(LOG_STATUS, "KMTronic: %s = %s", sstr.str().c_str(), (bIsOn) ? "On" : "Off");
					if (iRelay > m_TotRelais)
						m_TotRelais = iRelay;
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "KMTronic: Invalid data received!");
			}
		}
	}
}
