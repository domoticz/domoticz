#include "stdafx.h"
#include "EvohomeSerial.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"

CEvohomeSerial::CEvohomeSerial(const int ID, const std::string &szSerialPort, const int baudrate, const std::string &UserContID) :
CEvohomeRadio(ID, UserContID)
{
    if(baudrate!=0)
	{
	  m_iBaudRate=baudrate;
	}
	else
	{
	  // allow migration of hardware created before baud rate was configurable
	  m_iBaudRate=115200;
	}
	m_szSerialPort=szSerialPort;
}

bool CEvohomeSerial::StopHardware()
{
	m_stoprequested=true;
	if (m_thread != NULL)
		m_thread->join();
	// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
	sleep_milliseconds(10);
	terminate();
	m_bIsStarted=false;
	if(m_bDebug && m_pEvoLog)
	{
		delete m_pEvoLog;
		m_pEvoLog=NULL;
	}
	return true;
}

bool CEvohomeSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"evohome serial: Opening serial port: %s@%d", m_szSerialPort.c_str(), m_iBaudRate);
		open(m_szSerialPort,m_iBaudRate);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"evohome serial: Error opening serial port: %s", m_szSerialPort.c_str());
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"evohome serial: Error opening serial port!!!");
		return false;
	}
	m_nBufPtr=0;
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CEvohomeSerial::ReadCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void CEvohomeSerial::ReadCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		//_log.Log(LOG_NORM,"evohome: received %ld bytes",len);
		if (!HandleLoopData(data,len))
		{
			//error in data, try again...
			terminate();
		}
	}
	catch (...)
	{

	}
}

void CEvohomeSerial::Do_Work()
{
	startup = true;
	stLastRelayCheck = boost::posix_time::min_date_time;
	nStartup = 0;
	nStarts = 0;

	int sec_counter = 0;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (m_stoprequested)
			break;

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"evohome serial: serial setup retry in %d seconds...", EVOHOME_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=EVOHOME_RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
				if (isOpen())//do some startup stuff
				{
					OnConnect();
				}
			}
		}
		else
		{
			Idle_Work();
		}
	}
	_log.Log(LOG_STATUS,"evohome serial: Serial Worker stopped...");
}

void CEvohomeSerial::Do_Send(std::string str)
{
    write(str.c_str(), str.length());
}
