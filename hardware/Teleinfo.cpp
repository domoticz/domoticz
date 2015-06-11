/*
Domoticz Software : http://domoticz.com/
File : Teleinfo.cpp
Author : Nicolas HILAIRE
Version : 1.3
Description : This class manage the Teleinfo Signal


History :
- 2013-11-01 : Creation
- 2014-10-29 : Add 'EJP' contract (Laurent MEY)
- 2014-12-13 : Add 'Tempo' contract (Kevin NICOLAS)
- 2015-06-10 : Fix bug power divided by 2 (Christophe DELPECH)
*/

#include "stdafx.h"
#include "Teleinfo.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//Teleinfo for EDF power meter. Only "Base" and "Heures creuses" are suported

//Teleinfo official specification :
//http://www.planete-domotique.com/notices/ERDF-NOI-CPT_O2E.pdf

//Example of data received by EDF power meter
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

#define TE_ADCO "ADCO" //meter id
#define TE_OPTARIF "OPTARIF"//pricing option
#define TE_ISOUSC "ISOUSC"//current power subscribe   //A
#define TE_BASE "BASE"//total power usage normal tariff in base option
#define TE_HCHC "HCHC"// total power usage low tariff in HC option
#define TE_HCHP "HCHP"// total power usage normal tariff in HC option
#define TE_EJPHPM "EJPHPM"// total power usage normal tariff in PM option
#define TE_EJPHN "EJPHN"// total power usage low tariff in HN option
#define TE_BBRHCJB "BBRHCJB"// total power usage low tariff in HC option tempo blue
#define TE_BBRHPJB "BBRHPJB"// total power usage normal tariff in HC option tempo blue
#define TE_BBRHCJW "BBRHCJW"// total power usage low tariff in HC option tempo white
#define TE_BBRHPJW "BBRHPJW"// total power usage normal tariff in HC option tempo white
#define TE_BBRHCJR "BBRHCJR"// total power usage low tariff in HC option tempo red
#define TE_BBRHPJR "BBRHPJR"// total power usage normal tariff in HC option tempo red
#define TE_PTEC   "PTEC"//current tariff period
#define TE_IINST "IINST"//instant current power usage
#define TE_IMAX "IMAX"//maximal current power usage
#define TE_PAPP "PAPP"//apparent power
#define TE_MOTDETAT "MOTDETAT"//mot d'etat

Teleinfo::Match Teleinfo::m_matchlist[19] = {
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
	{ STD, TELEINFO_TYPE_IINST, TE_IINST, 3 },
	{ STD, TELEINFO_TYPE_IMAX, TE_IMAX, 3 },
	{ STD, TELEINFO_TYPE_PAPP, TE_PAPP, 5 },
	{ STD, TELEINFO_TYPE_MOTDETAT, TE_MOTDETAT, 6 }
};

Teleinfo::Teleinfo(const int ID, const std::string& devname, unsigned int baud_rate)
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

Teleinfo::~Teleinfo(void)
{
	StopHardware();
}

void Teleinfo::Init()
{
	m_bufferpos = 0;

	memset(&m_buffer, 0, sizeof(m_buffer));
	memset(&m_p1power, 0, sizeof(m_p1power));

	m_p1power.len = sizeof(P1Power) - 1;
	m_p1power.type = pTypeP1Power;
	m_p1power.subtype = sTypeP1Power;
	m_p1power.ID = 1;

	m_counter = 0;
}

bool Teleinfo::StartHardware()
{
	StartHeartbeatThread();
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "Teleinfo: Using serial port: %s", m_szSerialPort.c_str());
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
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Teleinfo: Error opening serial port!!!");
		return false;
	}
	setReadCallback(boost::bind(&Teleinfo::readCallback, this, _1, _2));
	m_bIsStarted = true;
	sOnConnected(this);

	return true;
}

bool Teleinfo::StopHardware()
{
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}
	StopHeartbeatThread();
	m_bIsStarted = false;
	return true;
}


void Teleinfo::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

void Teleinfo::MatchLine()
{
	if ((strlen((const char*)&m_buffer)<1) || (m_buffer[0] == 0x0a))
		return;

	uint8_t i;
	uint8_t found = 0;
	Teleinfo::Match t;
	char value[20] = "";
	std::string vString;

	//_log.Log(LOG_NORM,"Frame : #%s#", m_buffer);
	for (i = 0; (i<sizeof(m_matchlist) / sizeof(Teleinfo::Match))&(!found); i++)
	{
		t = m_matchlist[i];
		switch (t.matchtype)
		{
		case STD:
			if (strncmp(t.key, (const char*)&m_buffer, strlen(t.key)) == 0) {
				found = 1;
			}
			break;
		} //switch

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
		switch (t.type)
		{
		case TELEINFO_TYPE_ADCO:
			/*
			//structure initialization
			memset(&m_p1power,0,sizeof(m_p1power));
			m_p1power.len=sizeof(P1Power)-1;
			m_p1power.type=pTypeP1Power;
			m_p1power.subtype=sTypeP1Power;
			*/
			break;
		case TELEINFO_TYPE_OPTARIF:
			break;
		case TELEINFO_TYPE_ISOUSC:
			break;
		case TELEINFO_TYPE_BASE:
			if (ulValue != 0)
				m_p1power.powerusage1 = ulValue;
			break;
		case TELEINFO_TYPE_HCHC:
			if (ulValue != 0)
				m_p1power.powerusage2 = ulValue;
			break;
		case TELEINFO_TYPE_HCHP:
			if (ulValue != 0)
				m_p1power.powerusage1 = ulValue;
			break;
		case TELEINFO_TYPE_EJPHPM:
			if (ulValue != 0)
				m_p1power.powerusage2 = ulValue;
			break;
		case TELEINFO_TYPE_EJPHN:
			if (ulValue != 0)
				m_p1power.powerusage1 = ulValue;
			break;
		case TELEINFO_TYPE_BBRHCJB:
			if (ulValue != 0)
				m_p1power.powerusage2 = ulValue;
			break;
		case TELEINFO_TYPE_BBRHPJB:
			if (ulValue != 0)
				m_p1power.powerusage1 = ulValue;
			break;
		case TELEINFO_TYPE_BBRHCJW:
			if (ulValue != 0)
				m_p1power.powerusage2 = ulValue;
			break;
		case TELEINFO_TYPE_BBRHPJW:
			if (ulValue != 0)
				m_p1power.powerusage1 = ulValue;
			break;
		case TELEINFO_TYPE_BBRHCJR:
			if (ulValue != 0)
				m_p1power.powerusage2 = ulValue;
			break;
		case TELEINFO_TYPE_BBRHPJR:
			if (ulValue != 0)
				m_p1power.powerusage1 = ulValue;
			break;
		case TELEINFO_TYPE_PTEC:
			break;
		case TELEINFO_TYPE_IINST:
			//we convert A to W setting RFXMeter/Counter Dividers Energy to 1000 / voltage => 1000/230 = 4.35
			//m_p1power.usagecurrent = ulValue;
			break;
		case TELEINFO_TYPE_IMAX:
			break;
		case TELEINFO_TYPE_PAPP:
			//we count to prevent add each block but only one every 10 seconds
			m_p1power.usagecurrent += ulValue;
			m_counter++;
			m_bLabel_PAPP_Exist = true;
			if (m_counter >= NumberOfFrameToSendOne)
			{
				//_log.Log(LOG_NORM,"Teleinfo frame complete");
				//_log.Log(LOG_NORM,"powerusage1 = %lu", m_p1power.powerusage1);
				//_log.Log(LOG_NORM,"powerusage2 = %lu", m_p1power.powerusage2);
				//_log.Log(LOG_NORM,"usagecurrent = %lu", m_p1power.usagecurrent);
				m_p1power.usagecurrent /= m_counter;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1power);
				m_counter = 0;
				m_p1power.usagecurrent = 0;
			}
			break;
		case TELEINFO_TYPE_MOTDETAT:
			if (m_bLabel_PAPP_Exist == false)
			{
				m_counter++;
				if (m_counter >= NumberOfFrameToSendOne)
				{
					//_log.Log(LOG_NORM,"Teleinfo frame complete");
					//_log.Log(LOG_NORM,"powerusage1 = %lu", m_p1power.powerusage1);
					//_log.Log(LOG_NORM,"powerusage2 = %lu", m_p1power.powerusage2);
					//_log.Log(LOG_NORM,"usagecurrent = %lu", m_p1power.usagecurrent);
					m_p1power.usagecurrent /= m_counter;
					sDecodeRXMessage(this, (const unsigned char *)&m_p1power);
					m_counter = 0;
					m_p1power.usagecurrent = 0;
				}
			}
			break;
		default:
			_log.Log(LOG_ERROR, "Teleinfo: label '%s' not handled!", t.key);
			break;
		}
		return;
	}
}

void Teleinfo::ParseData(const unsigned char *pData, int Len)
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
			if (isCheckSumOk())
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


/* Explanation of the checksum computation issued from the official EDF specification

a "checksum" is calculated on the set of characters from the beginning of the label field to the end of the field given character SP included.
We first make ??the sum of all ASCII codes of all characters.
to avoid introduce ASCII (00 to 1F hex) functions, it retains only the six least significant bits of
result (this translates into a logical AND between the amount previously calculated and 03Fh).
Finally, we added 20 hexadecimal. The result will always be a printable ASCII character (sign, digit,
capital letter) of from 0x20 to hexadecimal 0x5F

La "checksum" est calcul»e sur l'ensemble des caractÀres allant du d»but du champ »tiquette á la fin du champ
donn»e, caractÀre SP inclus. On fait tout d'abord la somme des codes ASCII de tous ces caractÀres. Pour »viter
d'introduire des fonctions ASCII (00 á 1F en hexad»cimal), on ne conserve que les six bits de poids faible du
r»sultat obtenu (cette op»ration se traduit par un ET logique entre la somme pr»c»demment calcul»e et 03Fh).
Enfin, on ajoute 20 en hexad»cimal. Le r»sultat sera donc toujours un caractÀre ASCII imprimable (signe, chiffre,
lettre majuscule) allant de 20 á 5F en hexad»cimal.
*/

bool Teleinfo::isCheckSumOk()
{
	unsigned int checksum = 0x00;
	int i;

	for (i = 0; i < int(strlen((char*)m_buffer)) - 2; i++)
	{
		checksum += m_buffer[i];
	}
	checksum = (checksum & 0x3F) + 0x20;
	return (checksum == m_buffer[strlen((char*)m_buffer) - 1]);
}

bool Teleinfo::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

