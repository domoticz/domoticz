#include "stdafx.h"
#include "S0MeterSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"

#include <algorithm>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
#include <string>

#ifdef _DEBUG
	//#define DEBUG_S0
	#define TOT_DEBUG_LINES 6
	const char *szDebugDataP2[TOT_DEBUG_LINES] = {
		"/27243:S0 Pulse Counter V0.1\n",
		"ID:27243:I:10:M1:264:264:M2:0:0\n",
		"ID:27243:I:10:M1:983:1247:M2:518:518\n",
		"ID:27243:I:10:M1:1121:2368:M2:0:518\n",
		"ID:27243:I:10:M1:0:2368:M2:1126:1644\n",
		"ID:27243:I:10:M1:921:3289:M2:0:1644\n",
	};
#endif

S0MeterSerial::S0MeterSerial(const int ID, const std::string& devname, const unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	InitBase();
}

S0MeterSerial::~S0MeterSerial()
{

}

bool S0MeterSerial::StartHardware()
{
	RequestStart();

	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"S0 Meter: Using serial port: %s", m_szSerialPort.c_str());
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
		_log.Log(LOG_ERROR,"S0 Meter: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"S0 Meter: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_bufferpos=0;
	ReloadLastTotals();
	/*
	std::stringstream sstr;
	sstr << "MODE=2;";

	for (int ii = 0; ii < 5; ii++)
	{
		sstr << "P" << int(ii + 1) << "=" << m_meters[ii].m_pulse_per_unit << ";";
	}
	write(sstr.str());
	*/
	setReadCallback(boost::bind(&S0MeterSerial::readCallback, this, _1, _2));
	sOnConnected(this);

#ifdef DEBUG_S0
	int ii = 0;
	for (ii = 0; ii < TOT_DEBUG_LINES; ii++)
	{
		std::string dline = szDebugDataP2[ii];
		ParseData((const unsigned char*)dline.c_str(), dline.size());
	}
#endif

	StartHeartbeatThread();

	_log.Log(LOG_STATUS, "S0 Meter: Worker started...");

	return true;
}

bool S0MeterSerial::StopHardware()
{
	terminate();
	m_bIsStarted=false;
	StopHeartbeatThread();
	_log.Log(LOG_STATUS, "S0 Meter: Worker stopped...");
	return true;
}


void S0MeterSerial::readCallback(const char *data, size_t len)
{
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

bool S0MeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

