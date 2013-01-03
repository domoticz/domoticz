#include "stdafx.h"
#include "P1MeterSerial.h"

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
P1MeterSerial::P1MeterSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
}

P1MeterSerial::P1MeterSerial(const std::string& devname,
        unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
}

P1MeterSerial::~P1MeterSerial()
{
	clearReadCallback();
}

bool P1MeterSerial::StartHardware()
{
	//Try to open the Serial Port
	try
	{
		std::cout << "Using serial port: " << m_szSerialPort << std::endl;
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(
			boost::asio::serial_port_base::parity::even),
			boost::asio::serial_port_base::character_size(7)
			);
	}
	catch (boost::exception & e)
	{
		std::cerr << "Error opening serial port!\n";
#ifdef _DEBUG
		std::cerr << "-----------------" << std::endl << boost::diagnostic_information(e) << "-----------------" << std::endl;
#endif
		return false;
	}
	catch ( ... )
	{
		std::cerr << "Error opening serial port!!!";
		return false;
	}
	m_bIsStarted=true;
	m_linecount=0;
	m_exclmarkfound=0;
	setReadCallback(boost::bind(&P1MeterSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool P1MeterSerial::StopHardware()
{
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	return true;
}


void P1MeterSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled

	//m_sharedserver.SendToAll((const char*)data,len);
	ParseData((const unsigned char*)data, (int)len);
}

void P1MeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
}
