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
#include "../main/Helper.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <ctime>
#include <iostream>
#include <string>

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
	m_bufferpos = 0;
	m_counter = 0;
	m_teleinfo.CRCmode1 = 255;	 // Guess the CRC mode at first run
}


bool CTeleinfoSerial::StartHardware()
{
	StartHeartbeatThread();
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
	setReadCallback(boost::bind(&CTeleinfoSerial::readCallback, this, _1, _2));
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
	ParseData(data, static_cast<int>(len));
}


void CTeleinfoSerial::MatchLine()
{
	std::string label, vString;
	std::vector<std::string> splitresults;
	unsigned long value;
	std::string sline(m_buffer);

	_log.Debug(DEBUG_HARDWARE, "Frame : #%s#", sline.c_str());

	// Is the line we got worth analysing any further?
	if ((sline.size() < 4) || (sline[0] == 0x0a))
	{
		_log.Debug(DEBUG_HARDWARE, "Frame #%s# ignored, too short or irrelevant", sline.c_str());
		return;
	}

	// Extract the elements, return if not enough and line is invalid
	StringSplit(sline, " ", splitresults);
	if (splitresults.size() < 3)
	{
		_log.Log(LOG_ERROR, "Frame #%s# passed the checksum test but failed analysis", sline.c_str());
		return;
	}

	label = splitresults[0];
	vString = splitresults[1];
	value = atoi(splitresults[1].c_str());

	if (label == "ADCO") m_teleinfo.ADCO = vString;
	else if (label == "OPTARIF") m_teleinfo.OPTARIF = vString;
	else if (label == "ISOUSC") m_teleinfo.ISOUSC = value;
	else if (label == "PAPP")
	{
		m_teleinfo.PAPP = value;
		m_teleinfo.withPAPP = true;
	}
	else if (label == "PTEC")  m_teleinfo.PTEC = vString;
	else if (label == "IINST") m_teleinfo.IINST = value;
	else if (label == "BASE") m_teleinfo.BASE = value;
	else if (label == "HCHC") m_teleinfo.HCHC = value;
	else if (label == "HCHP") m_teleinfo.HCHP = value;
	else if (label == "EJPHPM") m_teleinfo.EJPHPM = value;
	else if (label == "EJPHN") m_teleinfo.EJPHN = value;
	else if (label == "BBRHCJB") m_teleinfo.BBRHCJB = value;
	else if (label == "BBRHPJB") m_teleinfo.BBRHPJB = value;
	else if (label == "BBRHCJW") m_teleinfo.BBRHCJW = value;
	else if (label == "BBRHPJW") m_teleinfo.BBRHPJW = value;
	else if (label == "BBRHCJR") m_teleinfo.BBRHCJR = value;
	else if (label == "BBRHPJR") m_teleinfo.BBRHPJR = value;
	else if (label == "DEMAIN") m_teleinfo.DEMAIN = vString;
	else if (label == "IINST1") m_teleinfo.IINST1 = value;
	else if (label == "IINST2") m_teleinfo.IINST2 = value;
	else if (label == "IINST3") m_teleinfo.IINST3 = value;
	else if (label == "PPOT")  m_teleinfo.PPOT = value;
	else if (label == "MOTDETAT") m_counter++;

	// at 1200 baud we have roughly one frame per 1,5 second, check more frequently for alerts.
	if (m_counter >= m_iBaudRate / 600)
	{
		m_counter = 0;
		_log.Debug(DEBUG_HARDWARE, "(%s) Teleinfo frame complete, PAPP: %i, PTEC: %s", m_Name.c_str(), m_teleinfo.PAPP, m_teleinfo.PTEC.c_str());
		ProcessTeleinfo(m_teleinfo);
		mytime(&m_LastHeartbeat);// keep heartbeat happy
	}
}


void CTeleinfoSerial::ParseData(const char *pData, int Len)
{
	int ii = 0;
	while (ii < Len)
	{
		const char c = pData[ii];

		if ((c == 0x0d) || (c == 0x00) || (c == 0x02) || (c == 0x03))
		{
			ii++;
			continue;
		}

		m_buffer[m_bufferpos] = c;
		if (c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if (m_bufferpos > 0)
				m_buffer[m_bufferpos] = 0;

			//We process the line only if the checksum is ok and user did not request to bypass CRC verification
			if ((m_bDisableCRC) || isCheckSumOk(std::string(m_buffer), m_teleinfo.CRCmode1))
			{
				MatchLine();
			}
			m_bufferpos = 0;
		}
		else
		{
			m_bufferpos++;
		}
		ii++;
	}
}

