#include "stdafx.h"
#include "RFLinkSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"

#define RFLINK_RETRY_DELAY 30

CRFLinkSerial::CRFLinkSerial(const int ID, const std::string& devname)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_stoprequested=false;
}

CRFLinkSerial::~CRFLinkSerial()
{
	clearReadCallback();
}

bool CRFLinkSerial::StartHardware()
{
	m_retrycntr=RFLINK_RETRY_DELAY*5; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRFLinkSerial::Do_Work, this)));

	return (m_thread!=NULL);
}

bool CRFLinkSerial::StopHardware()
{
	m_stoprequested=true;
	if (m_thread)
	{
		m_thread->join();
		// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
		sleep_milliseconds(10);
	}
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


void CRFLinkSerial::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = 0;
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
				m_LastHeartbeat = mytime(NULL);
			}
			if (isOpen())
			{
				if (sec_counter % 20 == 0)
				{
					//Send ping (keep alive)
					time_t atime = mytime(NULL);
					if (atime - m_LastReceivedTime > 30)
					{
						//Timeout
						_log.Log(LOG_ERROR, "RFLink: Not received anything for more then 30 seconds, restarting...");
						m_retrycntr = (RFLINK_RETRY_DELAY-3) * 5;
						m_LastReceivedTime = atime;
						try {
							clearReadCallback();
							close();
							doClose();
							setErrorStatus(true);
						}
						catch (...)
						{
							//Don't throw from a Stop command
						}
					}
					else
						write("10;PING;\n");
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
			boost::lock_guard<boost::mutex> l(m_sendMutex);

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
	_log.Log(LOG_STATUS,"RFLink: Serial Worker stopped...");
}

/*
void CRFLinkSerial::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0,pData,length);
	boost::lock_guard<boost::mutex> l(m_sendMutex);
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
	m_LastReceivedTime = mytime(NULL);

	setReadCallback(boost::bind(&CRFLinkSerial::readCallback, this, _1, _2));
	sOnConnected(this);

	return true;
}

void CRFLinkSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(data, len);
}

bool CRFLinkSerial::WriteInt(const std::string &sendString)
{
	if (!isOpen())
		return false;
	write(sendString);
	return true;
}
