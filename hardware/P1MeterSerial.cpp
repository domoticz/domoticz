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
	m_stoprequested = false;
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
	m_stoprequested = false;
}

P1MeterSerial::~P1MeterSerial()
{

}

#ifdef _DEBUG
	//#define DEBUG_FROM_FILE
#endif

bool P1MeterSerial::StartHardware()
{
#ifdef DEBUG_FROM_FILE
	FILE *fIn=fopen("E:\\meter.txt","rb+");
	BYTE buffer[1400];
	int ret=fread((BYTE*)&buffer,1,sizeof(buffer),fIn);
	fclose(fIn);
	ParseData((const BYTE*)&buffer, ret, 1);
#endif
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&P1MeterSerial::Do_Work, this)));

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
	setReadCallback(boost::bind(&P1MeterSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool P1MeterSerial::StopHardware()
{
	terminate();
	m_stoprequested = true;
	if (m_thread)
	{
		m_thread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
	}
	m_bIsStarted = false;
    _log.Log(LOG_STATUS, "P1 Smart Meter: Serial Worker stopped...");
	return true;
}


void P1MeterSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled
	ParseData((const unsigned char*)data, static_cast<int>(len), m_bDisableCRC, m_ratelimit);
}

bool P1MeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void P1MeterSerial::Do_Work()
{
	int sec_counter = 0;
	int msec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_milliseconds(200);
		if (m_stoprequested)
			break;
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
}
