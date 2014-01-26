#include "stdafx.h"
#include "OTGWSerial.h"
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

#define RETRY_DELAY 30
#define OTGW_READ_INTERVAL 10

//
//Class OTGWSerial
//
OTGWSerial::OTGWSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	m_stoprequestedpoller=false;
	SetModes(Mode1,Mode2,Mode3,Mode4,Mode5);
}

OTGWSerial::~OTGWSerial()
{
	clearReadCallback();
}

bool OTGWSerial::StartHardware()
{
	m_retrycntr=RETRY_DELAY; //will force reconnect first thing
	StartPollerThread();
	return true;
}

bool OTGWSerial::StopHardware()
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
	StopPollerThread();
	return true;
}


void OTGWSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, (int)len);
}

void OTGWSerial::StartPollerThread()
{
	m_pollerthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&OTGWSerial::Do_PollWork, this)));
}

void OTGWSerial::StopPollerThread()
{
	if (m_pollerthread!=NULL)
	{
		assert(m_pollerthread);
		m_stoprequestedpoller = true;
		m_pollerthread->join();
	}
}

bool OTGWSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_NORM,"OTGW Using serial port: %s", m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
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
	setReadCallback(boost::bind(&OTGWSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void OTGWSerial::Do_PollWork()
{
	bool bFirstTime=true;
	while (!m_stoprequestedpoller)
	{
		sleep_seconds(1);

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_NORM,"OTGW: serial setup retry in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				if (OpenSerialDevice())
					bFirstTime=true;
			}
		}
		else
		{
			time_t atime=time(NULL);
			if ((atime%30==0)||(bFirstTime))	//updates every 30 seconds
			{
				bFirstTime=false;
				SendOutsideTemperature();
				GetGatewayDetails();
			}
		}
	}
	_log.Log(LOG_NORM,"OTGW Worker stopped...");
}

void OTGWSerial::GetGatewayDetails()
{
	char szCmd[10];
	strcpy(szCmd,"PS=1\r\n");
	WriteToHardware((const char*)&szCmd,strlen(szCmd));
}
void OTGWSerial::SendOutsideTemperature()
{
	float temp;
	if (!GetOutsideTemperatureFromDomoticz(temp))
		return;
	char szCmd[30];
	sprintf(szCmd,"OT=%.1f\r\n",temp);
	write((const char*)&szCmd,strlen(szCmd));
}

void OTGWSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isOpen()) {
		write(pdata,length);
	}
}

