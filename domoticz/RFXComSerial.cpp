#include "stdafx.h"
#include "RFXComSerial.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//
//Class RFXComSerial
//
RFXComSerial::RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
}

RFXComSerial::RFXComSerial(const std::string& devname,
        unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
}

RFXComSerial::~RFXComSerial()
{
	clearReadCallback();
}

bool RFXComSerial::StartHardware()
{
	//Try to open the Serial Port
	try
	{
		std::cout << "Using serial port: " << m_szSerialPort << std::endl;
		open(m_szSerialPort,m_iBaudRate);
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
	setReadCallback(boost::bind(&RFXComSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool RFXComSerial::StopHardware()
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

void RFXComSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	onRFXMessage((const unsigned char *)data,len);
}


void RFXComSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isOpen()) {
		write(pdata,length);
		//m_sharedserver.SendToAll(pdata,length);
	}
}
