#include "stdafx.h"
#include "CurrentCostMeterSerial.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"

#include <ctime>
#include <iostream>
#include <string>

//
//Class CurrentCostMeterSerial
//
CurrentCostMeterSerial::CurrentCostMeterSerial(const int ID, const std::string& devname, unsigned int baudRate):
	m_szSerialPort(devname),
	m_baudRate(baudRate)
{
	m_HwdID=ID;
}

bool CurrentCostMeterSerial::StartHardware()
{
	RequestStart();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	//Try to open the Serial Port
	try
	{
		Log(LOG_STATUS,"Using serial port: %s", m_szSerialPort.c_str());
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
		Log(LOG_ERROR,"Error opening serial port!");
#ifdef _DEBUG
		Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		Log(LOG_ERROR,"Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	sOnConnected(this);
	return true;
}

bool CurrentCostMeterSerial::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}


void CurrentCostMeterSerial::readCallback(const char *data, size_t len)
{
	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData(data, static_cast<int>(len));
}

bool CurrentCostMeterSerial::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void CurrentCostMeterSerial::Do_Work()
{
	int sec_counter = 0;
	int msec_counter = 0;

	Log(LOG_STATUS, "Worker started...");

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
		}
	}
	terminate();

	Log(LOG_STATUS, "Worker stopped...");
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
			if (idx.empty())
			{
				return;
			}

			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.empty())
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
	} // namespace server
} // namespace http
