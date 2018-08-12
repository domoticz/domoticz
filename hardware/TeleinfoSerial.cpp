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

#ifdef _DEBUG
#define DEBUG_TeleinfoSerial
#endif

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
}


bool CTeleinfoSerial::StartHardware()
{
	StartHeartbeatThread();
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "(%s) Teleinfo device uses serial port: %s at %i bauds", m_Name.c_str(), m_szSerialPort.c_str(), m_iBaudRate);
		open(m_szSerialPort, m_iBaudRate, m_iOptParity, m_iOptCsize);
	}
	catch (boost::exception & e)
	{

#ifdef DEBUG_TeleinfoSerial
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		_log.Log(LOG_STATUS, "Teleinfo: Serial port open failed, let's retry with CharSize:8 ...");

		try	{
			open(m_szSerialPort,m_iBaudRate,m_iOptParity,boost::asio::serial_port_base::character_size(8));
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
	teleinfo.CRCmode1 = 255;	 // Guess the CRC mode at first run

	if (m_bDisableCRC)
		_log.Log(LOG_STATUS, "(%s) CRC checks on incoming data are disabled", m_Name.c_str());
	else
		_log.Log(LOG_STATUS, "(%s) CRC checks will be performed on incoming data", m_Name.c_str());

	return true;
}


bool CTeleinfoSerial::StopHardware()
{
	terminate();
	StopHeartbeatThread();
	m_bIsStarted = false;
	return true;
}


bool CTeleinfoSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	return true;
}


void CTeleinfoSerial::readCallback(const char *data, size_t len)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
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
	const char* line = m_buffer;

	#ifdef DEBUG_TeleinfoSerial
	_log.Log(LOG_NORM,"Frame : #%s#", line);
	#endif

	// Is the line we got worth analysing any further?
	if ((strlen((const char*)&line)<4) || (line[0] == 0x0a))
		return;

	// Extract the elements, return if not enough and line is invalid
	StringSplit(line, " ", splitresults);
	if (splitresults.size() <3)
	{
		_log.Log(LOG_ERROR,"Frame #%s# passed the checksum test but failed analysis", line);
		return;
	}

	label = splitresults[0];
	vString = splitresults[1];
	value = atoi(splitresults[1].c_str());

	if (label == "ADCO") teleinfo.ADCO = vString;
	else if (label == "OPTARIF") teleinfo.OPTARIF = vString;
	else if (label == "ISOUSC") teleinfo.ISOUSC = value;
	else if (label == "PAPP")
	{
		teleinfo.PAPP = value;
		teleinfo.withPAPP = true;
	}
	else if (label == "PTEC")  teleinfo.PTEC = vString;
	else if (label == "IINST") teleinfo.IINST = value;
	else if (label == "BASE") teleinfo.BASE = value;
	else if (label == "HCHC") teleinfo.HCHC = value;
	else if (label == "HCHP") teleinfo.HCHP = value;
	else if (label == "EJPHPM") teleinfo.EJPHPM = value;
	else if (label == "EJPHN") teleinfo.EJPHN = value;
	else if (label == "BBRHCJB") teleinfo.BBRHCJB = value;
	else if (label == "BBRHPJB") teleinfo.BBRHPJB = value;
	else if (label == "BBRHCJW") teleinfo.BBRHCJW = value;
	else if (label == "BBRHPJW") teleinfo.BBRHPJW = value;
	else if (label == "BBRHCJR") teleinfo.BBRHCJR = value;
	else if (label == "BBRHPJR") teleinfo.BBRHPJR = value;
	else if (label == "DEMAIN") teleinfo.DEMAIN = vString;
	else if (label == "IINST1") teleinfo.IINST1 = value;
	else if (label == "IINST2") teleinfo.IINST2 = value;
	else if (label == "IINST3") teleinfo.IINST3 = value;
	else if (label == "PPOT")  teleinfo.PPOT = value;
	else if (label == "MOTDETAT") m_counter++;

	// at 1200 baud we have roughly one frame per 1,5 second, check more frequently for alerts.
	if (m_counter >= m_iBaudRate/600)
	{
		m_counter = 0;
		#ifdef DEBUG_TeleinfoSerial
		_log.Log(LOG_NORM,"(%s) Teleinfo frame complete, PAPP: %i, PTEC: %s", m_Name.c_str(), teleinfo.PAPP, teleinfo.PTEC.c_str());
		#endif
		ProcessTeleinfo(teleinfo);
		mytime(&m_LastHeartbeat);// keep heartbeat happy
	}
}


void CTeleinfoSerial::ParseData(const char *pData, int Len)
{
	int ii = 0;
	while (ii<Len)
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
			if ((m_bDisableCRC) || isCheckSumOk(teleinfo.CRCmode1))
				MatchLine();

			m_bufferpos = 0;
		}
		else
		{
			m_bufferpos++;
		}
		ii++;
	}
}


//Example of data received from power meter
//ADCO 271028237723 C
//OPTARIF HC.. <
//ISOUSC 45 ?
//HCHC 013149843 '
//HCHP 013016759 3
//PTEC HP..
//IINST 002 Y
//IMAX 049 L
//PAPP 00450 *
//HHPHC D /
//MOTDETAT 000000 B

/* Explanation of the checksum computation issued from the official EDF specification

a "checksum" is calculated on the set of characters from the beginning of the label field to the end of the field given character SP included.
We first make ??the sum of all ASCII codes of all characters.
to avoid introduce ASCII (00 to 1F hex) functions, it retains only the six least significant bits of
result (this translates into a logical AND between the amount previously calculated and 03Fh).
Finally, we added 20 hexadecimal. The result will always be a printable ASCII character (sign, digit,
capital letter) of from 0x20 to hexadecimal 0x5F

Le "checksum" est calcule sur l'ensemble des caracteres allant du debut du champ etiquette a la fin du champ
donnee, caractere SP inclus. On fait tout d'abord la somme des codes ASCII de tous ces caracteres. Pour eviter
d'introduire des fonctions ASCII (00  1F en hexadcimal), on ne conserve que les six bits de poids faible du
resultat obtenu (cette operation se traduit par un ET logique entre la somme precedemment calculee et 03Fh).
Enfin, on ajoute 20 en hexadecimal. Le resultat sera donc toujours un caractre ASCII imprimable (signe, chiffre,
lettre majuscule) allant de 20 a 5F en hexadcimal.

Un deuxime mode de calcul existe qui prend aussi le caractre de sparation final dans le calcul.
*/

bool CTeleinfoSerial::isCheckSumOk(int &isMode1)
{
	unsigned int checksum, mode1 = 0x00, mode2 = 0x00;
	int i;
	bool line_ok = false;

	checksum = m_buffer[strlen((char*)m_buffer) - 1];
	for (i = 0; i < int(strlen((char*)m_buffer)) - 2; i++)
	{
		mode1 += m_buffer[i];
	}
	mode2 = ((mode1 + m_buffer[i]) & 0x3F) + 0x20;
	mode1 = (mode1 & 0x3F) + 0x20;

	if (mode1 == checksum)
	{
		line_ok = true;
		if (isMode1 != (int)true)// This will evaluate to false when isMode still equals to 255 at second run
		{
			isMode1 = true;
			_log.Log(LOG_STATUS, "(%s) Teleinfo CRC check mode set to 1", m_Name.c_str());
		}
	}
	else if (mode2 == checksum)
	{
		line_ok = true;
		if (isMode1 != false)	 // if this is first run, will still be at 255
		{
			isMode1 = false;
			_log.Log(LOG_STATUS, "(%s) TeleinfoCRC check mode set to 2", m_Name.c_str());
		}
	}
	else						 // Don't send an error on the first run as the line is probably truncated, wait for mode to be initialised
	if (isMode1 != 255)
		_log.Log(LOG_ERROR, "(%s) CRC check failed on Teleinfo line '%s' using both modes 1 and 2. Line skipped.", m_Name.c_str(), m_buffer);

	#ifdef DEBUG_TeleinfoSerial
	if (line_ok) _log.Log(LOG_NORM, "(%s) CRC check passed on Teleinfo line '%s'. Line processed", m_Name.c_str(), m_buffer);
	#endif
	return line_ok;
}
