/*
Domoticz Software : http://domoticz.com/
File : TeleinfoSerial.cpp
Author : Nicolas HILAIRE, Blaise Thauvin
Version : 2.3
Description : This class decodes the Teleinfo signal from serial/USB devices before processing them

History :
- 2013-11-01 : Creation
- 2014-10-29 : Add 'EJP' contract (Laurent MEY)
- 2014-12-13 : Add 'Tempo' contract (Kevin NICOLAS)
- 2015-06-10 : Fix bug power divided by 2 (Christophe DELPECH)
- 2016-02-05 : Fix bug power display with 'Tempo' contract (Anthony LAGUERRE)
- 2016-02-11 : Fix power display when PAPP is missing (Anthony LAGUERRE)
- 2016-02-17 : Fix bug power usage (Anthony LAGUERRE). Thanks to Multinet
- 2017-01-28 : Add 'Heures Creuses' Switch (A.L)
- 2017-03-15 : 2.0 Renamed from Teleinfo.cpp to TeleinfoSerial.cpp in order to create
			   a shared class to process Teleinfo protocol (Blaise Thauvin)
- 2017-03-21 : 2.1 Fixed bug sending too many updates
- 2017-03-26 : 2.2 Fixed bug affecting tree-phases users. Consequently, simplified code
- 2017-04-01 : 2.3 Added RateLimit, flag to ignore CRC checks, and new CRC computation algorithm available on newer meters
- 2017-12-17 : 2.4 Fix bug affecting meters not providing PAPP, thanks to H. Lertouani
*/

#include "stdafx.h"
#include "TeleinfoSerial.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#include <boost/exception/diagnostic_information.hpp>

CTeleinfoSerial::CTeleinfoSerial(const int ID, const std::string& devname, const int datatimeout, unsigned int baud_rate, const bool disable_crc, const int ratelimit)
{
	m_HwdID = ID;
	m_szSerialPort = devname;
	m_iOptParity = boost::asio::serial_port_base::parity(TELEINFO_PARITY);
	m_iOptCsize = boost::asio::serial_port_base::character_size(TELEINFO_CARACTER_SIZE);
	m_iOptFlow = boost::asio::serial_port_base::flow_control(TELEINFO_FLOW_CONTROL);
	m_iOptStop = boost::asio::serial_port_base::stop_bits(TELEINFO_STOP_BITS);
	m_bDisableCRC = disable_crc;
	m_iRateLimit = ratelimit;
	m_iDataTimeout = datatimeout;

	if (baud_rate == 0)
		m_iBaudRate = 1200;
	else
		m_iBaudRate = 9600;

	// RateLimit > DataTimeout is an inconsistent setting. In that case, decrease RateLimit (which increases update rate)
	// down to Timeout in order to avoir watchdog errors due to this user configuration mistake
	if ((m_iRateLimit > m_iDataTimeout) && (m_iDataTimeout > 0))  m_iRateLimit = m_iDataTimeout;

	Init();
}


CTeleinfoSerial::~CTeleinfoSerial()
{
	StopHardware();
}


void CTeleinfoSerial::Init()
{
	InitTeleinfo();
}


bool CTeleinfoSerial::StartHardware()
{
	RequestStart();

	Init();
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "(%s) Teleinfo device uses serial port: %s at %i bauds", m_Name.c_str(), m_szSerialPort.c_str(), m_iBaudRate);
		open(m_szSerialPort, m_iBaudRate, m_iOptParity, m_iOptCsize);
	}
	catch (boost::exception & e)
	{
		_log.Debug(DEBUG_HARDWARE, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
		_log.Log(LOG_STATUS, "Teleinfo: Serial port open failed, let's retry with CharSize:8 ...");

		try {
			open(m_szSerialPort, m_iBaudRate, m_iOptParity, boost::asio::serial_port_base::character_size(8));
			_log.Log(LOG_STATUS, "Teleinfo: Serial port open successfully with CharSize:8 ...");
		}
		catch (...) {
			_log.Log(LOG_ERROR, "Teleinfo: Error opening serial port, even with CharSize:8 !");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Teleinfo: Error opening serial port!!!");
		return false;
	}
	StartHeartbeatThread();

	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	m_bIsStarted = true;
	sOnConnected(this);

	if (m_bDisableCRC)
		_log.Log(LOG_STATUS, "(%s) CRC checks on incoming data are disabled", m_Name.c_str());
	else
		_log.Log(LOG_STATUS, "(%s) CRC checks will be performed on incoming data", m_Name.c_str());

	return true;
}


bool CTeleinfoSerial::StopHardware()
{
	StopHeartbeatThread();
	terminate();
	m_bIsStarted = false;
	return true;
}


bool CTeleinfoSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return true;
}


void CTeleinfoSerial::readCallback(const char *data, size_t len)
{
	if (!m_bEnableReceive)
	{
		_log.Log(LOG_ERROR, "(%s) Receiving is not enabled", m_Name.c_str());
		return;
	}
	ParseTeleinfoData(data, static_cast<int>(len));
}
