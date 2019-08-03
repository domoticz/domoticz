#include "stdafx.h"
#include "P1MeterSerial.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"

//NOTE!!!, this code is partly based on the great work of RHekkers:
//https://github.com/rhekkers

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//
//Class P1MeterSerial
//
P1MeterSerial::P1MeterSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const bool disable_crc, const int ratelimit):
m_szSerialPort(devname)
{
	m_HwdID=ID;
	m_iBaudRate=baud_rate;
	m_bDisableCRC = disable_crc;
	m_ratelimit = ratelimit;
}

P1MeterSerial::P1MeterSerial(const std::string& devname,
        unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop),
		m_iBaudRate(baud_rate)
{
}

P1MeterSerial::~P1MeterSerial()
{

}

#ifdef _DEBUG
	//#define DEBUG_FROM_FILE
#endif

bool P1MeterSerial::StartHardware()
{
	RequestStart();

#ifdef DEBUG_FROM_FILE
	FILE *fIn=fopen("E:\\meter.txt","rb+");
	BYTE buffer[1400];
	int ret=fread((BYTE*)&buffer,1,sizeof(buffer),fIn);
	fclose(fIn);
	ParseData((const BYTE*)&buffer, ret, 1);
#endif

	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"P1 Smart Meter: Using serial port: %s", m_szSerialPort.c_str());
		if (m_iBaudRate==9600)
		{
			open(
				m_szSerialPort,
				m_iBaudRate,
				boost::asio::serial_port_base::parity(
				boost::asio::serial_port_base::parity::even),
				boost::asio::serial_port_base::character_size(7)
				);
		}
		else
		{
			//DSMRv4
			open(
				m_szSerialPort,
				m_iBaudRate,
				boost::asio::serial_port_base::parity(
				boost::asio::serial_port_base::parity::none),
				boost::asio::serial_port_base::character_size(8)
				);
			if (m_bDisableCRC) {
				_log.Log(LOG_STATUS,"P1 Smart Meter: CRC validation disabled through hardware control");
			}
		}
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"P1 Smart Meter: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"P1 Smart Meter: Error opening serial port!!!");
		return false;
	}

	Init();

	m_bIsStarted=true;

	m_thread = std::make_shared<std::thread>(&P1MeterSerial::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	setReadCallback(boost::bind(&P1MeterSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool P1MeterSerial::StopHardware()
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


void P1MeterSerial::readCallback(const char *data, size_t len)
{
	if (!m_bEnableReceive)
		return; //receiving not enabled
	ParseP1Data((const unsigned char*)data, static_cast<int>(len), m_bDisableCRC, m_ratelimit);
}

bool P1MeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void P1MeterSerial::Do_Work()
{
	int sec_counter = 0;
	int msec_counter = 0;
	_log.Log(LOG_STATUS, "P1 Smart Meter: Worker started...");
	while (!IsStopRequested(200))
	{
		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;

			sec_counter++;
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat=mytime(NULL);
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS, "P1 Smart Meter: Worker stopped...");

}
