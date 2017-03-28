/*
Domoticz Software : http://domoticz.com/
File : TeleinfoSerial.cpp
Author : Nicolas HILAIRE
Version : 2.1
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
*/

#include "stdafx.h"
#include "TeleinfoSerial.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

#ifdef _DEBUG
#define DEBUG_TeleinfoSerial
#endif


#define NBFRAMES 8				 //number of frames to collect before processing one

#define TE_ADCO "ADCO"			 //meter id
#define TE_OPTARIF "OPTARIF"	 //pricing option
#define TE_ISOUSC "ISOUSC"		 //current power subscribe   //A
#define TE_BASE "BASE"			 //total power usage normal tariff in base option
#define TE_HCHC "HCHC"			 // total power usage low tariff in HC option
#define TE_HCHP "HCHP"			 // total power usage normal tariff in HC option
#define TE_EJPHPM "EJPHPM"		 // total power usage normal tariff in PM option
#define TE_EJPHN "EJPHN"		 // total power usage low tariff in HN option
#define TE_BBRHCJB "BBRHCJB"	 // total power usage low tariff in HC option tempo blue
#define TE_BBRHPJB "BBRHPJB"	 // total power usage normal tariff in HC option tempo blue
#define TE_BBRHCJW "BBRHCJW"	 // total power usage low tariff in HC option tempo white
#define TE_BBRHPJW "BBRHPJW"	 // total power usage normal tariff in HC option tempo white
#define TE_BBRHCJR "BBRHCJR"	 // total power usage low tariff in HC option tempo red
#define TE_BBRHPJR "BBRHPJR"	 // total power usage normal tariff in HC option tempo red
#define TE_PTEC   "PTEC"		 //current tariff period
#define TE_IINST "IINST"		 //instant current power usage. 
#define TE_IINST1 "IINST1"		 //instant current power usage pahse 1
#define TE_IINST2 "IINST2"		 //instant current power usage phase 2
#define TE_IINST3 "IINST3"		 //instant current power usage phase 2
#define TE_PPOT "PPOT"			 //Potental on all 3 phases
#define TE_DEMAIN "DEMAIN"		 //tariff tomorrow
#define TE_PEJP "PEJP"			 //prior notice "pointe mobile" tariff
#define TE_PAPP "PAPP"			 //apparent power
#define TE_MOTDETAT "MOTDETAT"	 //mot d'etat

CTeleinfoSerial::Match CTeleinfoSerial::m_matchlist[27] =
{
	{ STD, TELEINFO_TYPE_ADCO, TE_ADCO, 12 },
	{ STD, TELEINFO_TYPE_OPTARIF, TE_OPTARIF, 4 },
	{ STD, TELEINFO_TYPE_ISOUSC, TE_ISOUSC, 2 },
	{ STD, TELEINFO_TYPE_BASE, TE_BASE, 9 },
	{ STD, TELEINFO_TYPE_HCHC, TE_HCHC, 9 },
	{ STD, TELEINFO_TYPE_HCHP, TE_HCHP, 9 },
	{ STD, TELEINFO_TYPE_EJPHPM, TE_EJPHPM, 9 },
	{ STD, TELEINFO_TYPE_EJPHN, TE_EJPHN, 9 },
	{ STD, TELEINFO_TYPE_BBRHCJB, TE_BBRHCJB, 9 },
	{ STD, TELEINFO_TYPE_BBRHPJB, TE_BBRHPJB, 9 },
	{ STD, TELEINFO_TYPE_BBRHCJW, TE_BBRHCJW, 9 },
	{ STD, TELEINFO_TYPE_BBRHPJW, TE_BBRHPJW, 9 },
	{ STD, TELEINFO_TYPE_BBRHCJR, TE_BBRHCJR, 9 },
	{ STD, TELEINFO_TYPE_BBRHPJR, TE_BBRHPJR, 9 },
	{ STD, TELEINFO_TYPE_PTEC, TE_PTEC, 4 },
	{ STD, TELEINFO_TYPE_IINST1, TE_IINST1, 3 },
	{ STD, TELEINFO_TYPE_IINST2, TE_IINST2, 3 },
	{ STD, TELEINFO_TYPE_IINST3, TE_IINST3, 3 },
        { STD, TELEINFO_TYPE_IINST, TE_IINST, 3 },
	{ STD, TELEINFO_TYPE_PPOT, TE_PPOT, 2 },
	{ STD, TELEINFO_TYPE_PEJP, TE_PEJP, 2 },
	{ STD, TELEINFO_TYPE_DEMAIN, TE_DEMAIN, 4 },
	{ STD, TELEINFO_TYPE_PAPP, TE_PAPP, 5 },
	{ STD, TELEINFO_TYPE_MOTDETAT, TE_MOTDETAT, 6 }
};

CTeleinfoSerial::CTeleinfoSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID = ID;
	m_szSerialPort = devname;
	m_iBaudRate = baud_rate;
	m_iOptParity = boost::asio::serial_port_base::parity(TELEINFO_PARITY);
	m_iOptCsize = boost::asio::serial_port_base::character_size(TELEINFO_CARACTER_SIZE);
	m_iOptFlow = boost::asio::serial_port_base::flow_control(TELEINFO_FLOW_CONTROL);
	m_iOptStop = boost::asio::serial_port_base::stop_bits(TELEINFO_STOP_BITS);
	m_bLabel_PAPP_Exist = false;
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
		_log.Log(LOG_STATUS, "Teleinfo: %s uses serial port: %s", Name.c_str(), m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_iBaudRate,
			m_iOptParity,
			m_iOptCsize
			);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "Teleinfo: Error opening serial port!");
		#ifdef DEBUG_TeleinfoSerial
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
		#else
		(void)e;
		#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Teleinfo: Error opening serial port!!!");
		return false;
	}
	setReadCallback(boost::bind(&CTeleinfoSerial::readCallback, this, _1, _2));
	m_bIsStarted = true;
	sOnConnected(this);

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
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bEnableReceive)
		return;					 //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}


void CTeleinfoSerial::MatchLine()
{
	if ((strlen((const char*)&m_buffer)<1) || (m_buffer[0] == 0x0a))
		return;

	uint8_t i;
	uint8_t found = 0;
	CTeleinfoSerial::Match t;
	char value[20] = "";
	std::string vString;
	#ifdef DEBUG_TeleinfoSerial
	_log.Log(LOG_NORM,"Frame : #%s#", m_buffer);
	#endif
	for (i = 0; (i<sizeof(m_matchlist) / sizeof(CTeleinfoSerial::Match))&(!found); i++)
	{
		t = m_matchlist[i];
		switch (t.matchtype)
		{
			case STD:
				if (strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0)
				{
					found = 1;
				}
				break;
		}						 //switch

		if (!found)
			continue;

		if (t.matchtype == STD)
		{
			//We get the width car after the space
			unsigned char * pos = (unsigned char *)strchr((char*)m_buffer, ' ');
			if (pos == NULL)
				continue;
			int position = int(pos - (unsigned char*)&m_buffer);
			strncpy(value, (char*)&(m_buffer[position + 1]), t.width);
			value[t.width] = 0;
		}
		unsigned long ulValue = (unsigned long)atoi(value);
		vString = value;
		switch (t.type)
		{
			case TELEINFO_TYPE_ADCO:
				break;
			case TELEINFO_TYPE_OPTARIF:
				teleinfo.OPTARIF = vString;
				break;
			case TELEINFO_TYPE_ISOUSC:
				teleinfo.ISOUSC = ulValue;
				break;
			case TELEINFO_TYPE_BASE:
				teleinfo.BASE = ulValue;
				break;
			case TELEINFO_TYPE_HCHC:
				teleinfo.HCHC = ulValue;
				break;
			case TELEINFO_TYPE_HCHP:
				teleinfo.HCHP = ulValue;
				break;
			case TELEINFO_TYPE_EJPHPM:
				teleinfo.EJPHPM = ulValue;
				break;
			case TELEINFO_TYPE_EJPHN:
				teleinfo.EJPHN = ulValue;
				break;
			case TELEINFO_TYPE_BBRHCJB:
				teleinfo.BBRHCJB = ulValue;
				break;
			case TELEINFO_TYPE_BBRHPJB:
				teleinfo.BBRHPJB = ulValue;
				break;
			case TELEINFO_TYPE_BBRHCJW:
				teleinfo.BBRHCJW = ulValue;
				break;
			case TELEINFO_TYPE_BBRHPJW:
				teleinfo.BBRHPJW = ulValue;
				break;
			case TELEINFO_TYPE_BBRHCJR:
				teleinfo.BBRHCJR = ulValue;
				break;
			case TELEINFO_TYPE_BBRHPJR:
				teleinfo.BBRHPJR = ulValue;
				break;
			case TELEINFO_TYPE_DEMAIN:
				teleinfo.DEMAIN = vString;
				break;
			case TELEINFO_TYPE_PTEC:
				teleinfo.PTEC = vString;
				break;
			case TELEINFO_TYPE_IINST:
				teleinfo.IINST = ulValue;
				break;
			case TELEINFO_TYPE_IINST1:
				teleinfo.IINST1 = ulValue;
				break;
			case TELEINFO_TYPE_IINST2:
				teleinfo.IINST2 = ulValue;
				break;
			case TELEINFO_TYPE_IINST3:
				teleinfo.IINST3 = ulValue;
				break;
			case TELEINFO_TYPE_PPOT:
				teleinfo.PPOT = ulValue;
				break;
			case TELEINFO_TYPE_PAPP:
				teleinfo.PAPP = ulValue;
				break;
			case TELEINFO_TYPE_MOTDETAT:
				m_counter++;
				if (m_counter >= NBFRAMES)
				{
				#ifdef DEBUG_TeleinfoSerial
					_log.Log(LOG_NORM,"Teleinfo frame complete");
				#endif
					m_counter = 0;
					ProcessTeleinfo(teleinfo);
				}
				break;
			default:
				_log.Log(LOG_ERROR, "Teleinfo: label '%s' not handled!", t.key);
				break;
		}
		return;
	}
}


void CTeleinfoSerial::ParseData(const unsigned char *pData, int Len)
{
	int ii = 0;
	while (ii<Len)
	{
		const unsigned char c = pData[ii];

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

			//We check the line only if the checksum is ok
			if (isCheckSumOk(teleinfo.CRCmode1))
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
	mode1 = (mode2 & 0x3F) + 0x20;

	if (mode1 == checksum)
		if (isMode1 == (int)true)// This will evaluate to false when isMode still equals to 255 at second run
			line_ok = true;
	else
	{
		isMode1 = true;
		_log.Log(LOG_STATUS, "(%s) Teleinfo CRC check mode set to 1", Name.c_str());
	}
	else if (mode2 == checksum)
	if (isMode1 == false)
		line_ok = true;
	else
	{
		isMode1 = false;
		_log.Log(LOG_STATUS, "(%s) TeleinfoCRC check mode set to 2", Name.c_str());
	}
	else						 // Don't send an error on the first run as the line is probably truncated, wait for mode to be initialised
	if (isMode1 != 255) _log.Log(LOG_ERROR, "(%s) CRC check failed on Teleinfo line '%s' using both modes 1 and 2. Line skipped.",
				Name.c_str(), m_buffer);

	#ifdef DEBUG_TeleinfoSerial
	if (line_ok) _log.Log(LOG_NORM, "(%s) CRC check passed on Teleinfo line '%s'. Line processed", Name.c_str(), m_buffer);
	#endif
	return line_ok;
}
