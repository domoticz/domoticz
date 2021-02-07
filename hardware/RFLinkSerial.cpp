#include "stdafx.h"
#include "RFLinkSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include <boost/exception/diagnostic_information.hpp>

CRFLinkSerial::CRFLinkSerial(const int ID, const std::string& devname) :
m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_retrycntr = RFLINK_RETRY_DELAY * 5;
}

bool CRFLinkSerial::StartHardware()
{
	RequestStart();

	m_retrycntr = RFLINK_RETRY_DELAY*5; //will force reconnect first thing

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool CRFLinkSerial::StopHardware()
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


void CRFLinkSerial::Do_Work()
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
				m_LastHeartbeat = mytime(nullptr);
			}
			if (isOpen())
			{
				/*
				if (sec_counter % 40 == 0)
				{
				time_t atime = mytime(nullptr);
				//Send ping (keep alive)
				_log.Log(LOG_STATUS, "RFLink: t1=%d t2=%d t3=%d", m_LastHeartbeat, m_LastHeartbeatReceive,
				m_LastReceivedTime);

				if (atime - m_LastReceivedTime > 25) {
				//_log.Log(LOG_STATUS, "RFLink: ping...");
				write("10;PING;\n");
				} else
				if (atime - m_LastReceivedTime > 50) {
				//Timeout
				_log.Log(LOG_ERROR, "RFLink: Nothing received for more than 50 seconds, restarting...");
				m_retrycntr = 0;
				m_LastReceivedTime = atime;
				terminate();
				} else {
				if (atime - m_LastReceivedTime > 25) {
				//_log.Log(LOG_STATUS, "RFLink: ping...");
				write("10;PING;\n");
				}
				}
				}
				*/

				if (sec_counter % 50 == 0)
				{
					time_t atime = mytime(nullptr);
					//Send ping (keep alive)
					//_log.Log(LOG_STATUS, "RFLink: t1=%d t3=%d", atime, m_LastReceivedTime);
					if (difftime(atime,m_LastReceivedTime) > 50) {
						//Receive Timeout
						//_log.Log(LOG_STATUS, "RFLink: ping50...");
						write("10;PING;\n");
						m_retrycntr = 0;
						m_LastReceivedTime = atime;
					} else {
						if (difftime(atime,m_LastReceivedTime) > 25) {
						   //_log.Log(LOG_STATUS, "RFLink: ping25...");
						   write("10;PING;\n");
						}
						//else {
							//_log.Log(LOG_STATUS, "RFLink: ping0...");
						//}
					}
				}
			}
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"RFLink: serial retrying in %d seconds...", RFLINK_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr/5>=RFLINK_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_rfbufferpos=0;
				OpenSerialDevice();
			}
		}
		/*
		if (m_sendqueue.size()>0)
		{
			std::lock_guard<std::mutex> l(m_sendMutex);

			std::vector<std::string>::iterator itt=m_sendqueue.begin();
			if (itt!=m_sendqueue.end())
			{
				std::string sBytes=*itt;
				write(sBytes.c_str(),sBytes.size());
				m_sendqueue.erase(itt);
			}
		}
		*/
	}
	terminate();

	_log.Log(LOG_STATUS,"RFLink: Worker stopped...");
}

/*
void CRFLinkSerial::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0,pData,length);
	std::lock_guard<std::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}
*/

bool CRFLinkSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 57600);
		_log.Log(LOG_STATUS,"RFLink: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"RFLink: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"RFLink: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_rfbufferpos = 0;
	m_LastReceivedTime = mytime(nullptr);

	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	sOnConnected(this);

	return true;
}

void CRFLinkSerial::readCallback(const char *data, size_t len)
{
	ParseData(data, len);
}

bool CRFLinkSerial::WriteInt(const std::string &sendString)
{
	if (!isOpen())
		return false;
	write(sendString);
	return true;
}
