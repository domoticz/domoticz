#include "stdafx.h"
#include "S0MeterSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//
//Class S0MeterSerial
//
S0MeterSerial::S0MeterSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const int M1Type, const int M1PPH, const int M2Type, const int M2PPH)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;

	m_s0_m1_type=M1Type;
	m_s0_m2_type=M2Type;
	m_pulse_per_unit_1=2000.0;
	m_pulse_per_unit_2=2000.0;

	if (M1PPH!=0)
		m_pulse_per_unit_1=float(M1PPH);
	if (M2PPH!=0)
		m_pulse_per_unit_2=float(M2PPH);
}

S0MeterSerial::~S0MeterSerial()
{
	clearReadCallback();
}

bool S0MeterSerial::StartHardware()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_NORM,"S0 Meter Using serial port: %s", m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even),
			boost::asio::serial_port_base::character_size(7),
			boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none),
			boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one)
			);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_bufferpos=0;
	ReloadLastTotals();
	setReadCallback(boost::bind(&S0MeterSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool S0MeterSerial::StopHardware()
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
	return true;
}


void S0MeterSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, (int)len);
}

void S0MeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
}

