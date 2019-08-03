#include "stdafx.h"
#include "MySensorsSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <ctime>
#include <iostream>
#include <string>

//#define DEBUG_MYSENSORS

#define RETRY_DELAY 30

MySensorsSerial::MySensorsSerial(const int ID, const std::string& devname, const int Mode1) :
	m_retrycntr(RETRY_DELAY)
{
	switch (Mode1)
	{
	case 1: // Arduino Pro 3.3V and SenseBender are running on 8 MHz, and the highest reliable baudrate is 38400
		m_iBaudRate = 38400;
		break;
	default:
		m_iBaudRate = 115200;
		break;
	}
	m_szSerialPort = devname;
	m_HwdID = ID;
}

MySensorsSerial::~MySensorsSerial()
{

}

bool MySensorsSerial::StartHardware()
{
	RequestStart();

	m_LineReceived.clear();
	LoadDevicesFromDatabase();

	//return OpenSerialDevice();
	//somehow retry does not seem to work?!

	m_retrycntr = RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&MySensorsSerial::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	StartSendQueue();
	return (m_thread != nullptr);
}

bool MySensorsSerial::StopHardware()
{
	StopSendQueue();
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void MySensorsSerial::Do_Work()
{
	int sec_counter = 0;

	_log.Log(LOG_STATUS, "MySensors: Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}

		if (!isOpen())
		{
			if (m_retrycntr == 0)
			{
				_log.Log(LOG_STATUS, "MySensors: retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				OpenSerialDevice();
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS, "MySensors: Worker stopped...");
}

bool MySensorsSerial::OpenSerialDevice()
{
#ifndef DEBUG_MYSENSORS
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "MySensors: Using serial port: %s", m_szSerialPort.c_str());
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
		_log.Log(LOG_ERROR, "MySensors: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "MySensors: Error opening serial port!!!");
		return false;
	}
#else
	std::ifstream infile;
	std::string sLine;
	//infile.open("D:\\MySensors.txt");
	infile.open("D:\\log-gw.txt");

	if (!infile.is_open())
		return false;
	while (!infile.eof())
	{
		getline(infile, sLine);

		std::vector<std::string> results;
		StringSplit(sLine, ";", results);
		if (results.size() != 6)
		{
			StringSplit(sLine, " ", results);
			if (results.size() >= 7)
			{
				//std::string orgstr = sLine;
				sLine = "";
				sLine += results[2] + ";";
				sLine += results[3] + ";";
				sLine += results[4] + ";";
				sLine += results[5] + ";";
				sLine += results[6] + ";";
				//Add the rest
				for (size_t ii = 7; ii < results.size(); ii++)
				{
					sLine += results[ii];
					if (ii != results.size() - 1)
						sLine += " ";
				}
				StringSplit(sLine, ";", results);
			}
		}
		if (results.size() != 6)
			continue;

		sLine += '\n';
		ParseData((const unsigned char*)sLine.c_str(), sLine.size());
	}
	infile.close();

#endif
	m_bIsStarted = true;
	m_LineReceived.clear();
	setReadCallback(boost::bind(&MySensorsSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
	}

void MySensorsSerial::readCallback(const char *data, size_t len)
{
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

void MySensorsSerial::WriteInt(const std::string &sendStr)
{
	if (!isOpen())
		return;
	writeString(sendStr);
}

