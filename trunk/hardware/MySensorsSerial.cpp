#include "stdafx.h"
#include "MySensorsSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//#define DEBUG_MYSENSORS

MySensorsSerial::MySensorsSerial(const int ID, const std::string& devname)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=115200;

}

MySensorsSerial::~MySensorsSerial()
{
	clearReadCallback();
}

bool MySensorsSerial::StartHardware()
{
	StartHeartbeatThread();
#ifndef DEBUG_MYSENSORS
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"MySensors: Using serial port: %s", m_szSerialPort.c_str());
#ifndef WIN32
		openOnlyBaud(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even),
			boost::asio::serial_port_base::character_size(7)
			);
#else
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even),
			boost::asio::serial_port_base::character_size(7)
			);
#endif
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"MySensors: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"MySensors: Error opening serial port!!!");
		return false;
	}
#else
	//std::ifstream infile;
	//std::string sLine;
	//infile.open("D:\\MySensors.txt");
	//if (!infile.is_open())
	//	return false;
	//while (!infile.eof())
	//{
	//	getline(infile, sLine);
	//	sLine += "\n";
	//	ParseData((const unsigned char*)sLine.c_str(), sLine.size());
	//}
	//infile.close();

#endif
	m_bIsStarted=true;
	m_bufferpos=0;
	setReadCallback(boost::bind(&MySensorsSerial::readCallback, this, _1, _2));
	sOnConnected(this);

	return true;
}

bool MySensorsSerial::StopHardware()
{
	m_bIsStarted=false;
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	StopHeartbeatThread();
	return true;
}


void MySensorsSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

void MySensorsSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
}

