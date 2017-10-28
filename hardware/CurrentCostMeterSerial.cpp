#include "stdafx.h"
#include "CurrentCostMeterSerial.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"

#include <string>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//
//Class CurrentCostMeterSerial
//
CurrentCostMeterSerial::CurrentCostMeterSerial(const int ID, const std::string& devname, unsigned int baudRate):
	m_stoprequested(false),
	m_szSerialPort(devname),
	m_baudRate(baudRate)
{
	m_HwdID=ID;
}

CurrentCostMeterSerial::~CurrentCostMeterSerial()
{

}

bool CurrentCostMeterSerial::StartHardware()
{
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CurrentCostMeterSerial::Do_Work, this)));

	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"CurrentCost Smart Meter: Using serial port: %s", m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_baudRate,
			boost::asio::serial_port_base::parity(
			boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"CurrentCost Smart Meter: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"CurrentCost Smart Meter: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CurrentCostMeterSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool CurrentCostMeterSerial::StopHardware()
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


void CurrentCostMeterSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData(data, static_cast<int>(len));
}

bool CurrentCostMeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void CurrentCostMeterSerial::Do_Work()
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
		void CWebServer::SetCurrentCostUSBType(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "") {
				return;
			}

			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.size() < 1)
				return;

			int Mode1 = atoi(request::findValue(&req, "CCBaudrate").c_str());
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
