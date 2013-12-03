#include "stdafx.h"
#include "RFXComSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

#define RETRY_DELAY 30

//
//Class RFXComSerial
//
RFXComSerial::RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	m_stoprequested=false;
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
	//return OpenSerialDevice();
	//somehow retry does not seem to work?!

	m_retrycntr=RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RFXComSerial::Do_Work, this)));

	return (m_thread!=NULL);

}

bool RFXComSerial::StopHardware()
{
	m_stoprequested=true;
	if (m_thread!=NULL)
		m_thread->join();
    // Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
    sleep_milliseconds(10);
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
	m_bIsStarted=false;
	return true;
}

void RFXComSerial::Do_Work()
{
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_NORM,"RFXCOM serial retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
			}
		}
	}
	_log.Log(LOG_NORM,"RFXCOM: Serial Worker stopped...");
} 


bool RFXComSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort,m_iBaudRate);
		_log.Log(LOG_NORM,"RFXCOM Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"RFXCOM Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"RFXCOM Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&RFXComSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void RFXComSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		bool bRet=onRFXMessage((const unsigned char *)data,len);
		if (bRet==false)
		{
			//close serial connection, and restart
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

		}
	}
	catch (...)
	{

	}
}


void RFXComSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isOpen()) {
		write(pdata,length);
	}
}
