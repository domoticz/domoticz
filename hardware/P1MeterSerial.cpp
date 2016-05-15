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
P1MeterSerial::P1MeterSerial(const int ID, const std::string& devname, unsigned int baud_rate):
m_szSerialPort(devname)
{
	m_HwdID=ID;
	m_iBaudRate=baud_rate;
	m_stoprequested = false;
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

//#define DEBUG_FROM_FILE

bool P1MeterSerial::StartHardware()
{
#ifdef DEBUG_FROM_FILE
	FILE *fIn=fopen("E:\\meter.txt","rb+");
	BYTE buffer[1000];
	int ret=fread((BYTE*)&buffer,1,sizeof(buffer),fIn);
	fclose(fIn);
	ParseData((const BYTE*)&buffer,ret);
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
	m_bIsStarted=true;
	m_linecount=0;
	m_exclmarkfound=0;
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
	return true;
}


void P1MeterSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
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

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SetP1USBType(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "") {
				return;
			}

			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.size() < 1)
				return;

			int Mode1 = atoi(request::findValue(&req, "P1Baudrate").c_str());
			int Mode2 = 0;
			int Mode3 = 0;
			int Mode4 = 0;
			int Mode5 = 0;
			int Mode6 = 0;
			m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);

			m_mainworker.RestartHardware(idx);

		}
	}
}
