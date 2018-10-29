#include "stdafx.h"
#include "ZiBlueSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"

#include <boost/exception/diagnostic_information.hpp>

#define ZiBlue_RETRY_DELAY 30

CZiBlueSerial::CZiBlueSerial(const int ID, const std::string& devname) :
m_szSerialPort(devname)
{
	m_HwdID=ID;
	m_retrycntr = ZiBlue_RETRY_DELAY * 5;
}

CZiBlueSerial::~CZiBlueSerial()
{

}

bool CZiBlueSerial::StartHardware()
{
	RequestStart();

	m_retrycntr=ZiBlue_RETRY_DELAY*5; //will force reconnect first thing

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CZiBlueSerial::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool CZiBlueSerial::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
	return true;
}


void CZiBlueSerial::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = 0;
	while (!IsStopRequested(200))
	{
		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;
			sec_counter++;

			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			if (isOpen())
			{
/*
				if (sec_counter % 50 == 0)
				{
					time_t atime = mytime(NULL);
					//Send ping (keep alive)
					//_log.Log(LOG_STATUS, "ZiBlue: t1=%d t3=%d", atime, m_LastReceivedTime);
					if (atime - m_LastReceivedTime > 50) {
						//Receive Timeout
						//_log.Log(LOG_STATUS, "ZiBlue: ping50...");
						write("10;PING;\n");
						m_retrycntr = 0;
						m_LastReceivedTime = atime;
					} else {
						if (atime - m_LastReceivedTime > 25) {
						   //_log.Log(LOG_STATUS, "ZiBlue: ping25...");
						   write("10;PING;\n");
						}
						//else {
							//_log.Log(LOG_STATUS, "ZiBlue: ping0...");
						//}
					}
				}
*/
			}
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"ZiBlue: serial retrying in %d seconds...", ZiBlue_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr/5>=ZiBlue_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_rfbufferpos=0;
				OpenSerialDevice();
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"ZiBlue: Worker stopped...");
}

bool CZiBlueSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 115200);
		_log.Log(LOG_STATUS,"ZiBlue: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"ZiBlue: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"ZiBlue: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(NULL);

	setReadCallback(boost::bind(&CZiBlueSerial::readCallback, this, _1, _2));

	sOnConnected(this);

	OnConnected();
	return true;
}

void CZiBlueSerial::readCallback(const char *data, size_t len)
{
	ParseData(data, len);
}

bool CZiBlueSerial::WriteInt(const std::string &sendString)
{
	if (!isOpen())
		return false;
	write(sendString);
	return true;
}

bool CZiBlueSerial::WriteInt(const uint8_t *pData, const size_t length)
{
	if (!isOpen())
		return false;
	write((const char*)pData,length);
	return true;
}
