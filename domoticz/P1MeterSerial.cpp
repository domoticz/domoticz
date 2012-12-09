#include "stdafx.h"
#include "P1MeterSerial.h"

//NOTE!!!, this code is partly based on the great work of RHekkers:
//https://github.com/rhekkers

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

typedef enum { ID, STD, LINE17, LINE18, EXCLMARK } MatchType;
typedef struct {
	MatchType type;
	char* key;
	char* topic;
	int start;
	int width;
} Match;

Match matchlist[] = {
	{ID, P1_SMID, "", 0, 0},
	{STD, P1PU1, "powerusage1", 10, 9},
	{STD, P1PU2, "powerusage2", 10, 9},
	{STD, P1PD1, "powerdeliv1", 10, 9},
	{STD, P1PD2, "powerdeliv2", 10, 9},
	{STD, P1TIP, "tariff", 12, 4},
	{STD, P1PUC, "powerusagec", 10, 7},
	{STD, P1PDC, "powerdelivc", 10, 7},
	{LINE17, P1GTS, "gastimestamp", 11, 12},
	{LINE18, "(", "gasusage", 1, 9},
	{EXCLMARK,"!","", 0, 0}
};

//
//Class P1MeterSerial
//
P1MeterSerial::P1MeterSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
}

P1MeterSerial::P1MeterSerial(const std::string& devname,
        unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
}

P1MeterSerial::~P1MeterSerial()
{
	clearReadCallback();
}

bool P1MeterSerial::StartHardware()
{
	//Try to open the Serial Port
	try
	{
		std::cout << "Using serial port: " << m_szSerialPort << std::endl;
		open(m_szSerialPort,m_iBaudRate);
	}
	catch (boost::exception & e)
	{
		std::cerr << "Error opening serial port!\n";
#ifdef _DEBUG
		std::cerr << "-----------------" << std::endl << boost::diagnostic_information(e) << "-----------------" << std::endl;
#endif
		return false;
	}
	catch ( ... )
	{
		std::cerr << "Error opening serial port!!!";
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
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	return true;
}

void P1MeterSerial::MatchLine()
{
	uint8_t i;
	uint8_t found=0;
	Match t;
	char value[13]="";

	for(i=0;(i<sizeof(matchlist)/sizeof(Match))&(!found);i++)
	{
		t = matchlist[i];
		switch(t.type)
		{
		case ID:
			if(strncmp(t.key, (const char*)&m_rxbuffer, strlen(t.key)) == 0) {
				m_linecount=1;
			}
			break;
		case STD:
			if(strncmp(t.key, (const char*)&m_rxbuffer, strlen(t.key)) == 0) {
				found=1;
			}
			break;
		case LINE17:
			if(strncmp(t.key, (const char*)&m_rxbuffer, strlen(t.key)) == 0) {
				m_linecount = 17;
				found=1;
			}
			break;
		case LINE18:
			if((m_linecount == 18)&&(strncmp(t.key, (const char*)&m_rxbuffer, strlen(t.key)) == 0)) {
				found=1;
			}
			break;
		case EXCLMARK:
			if(strncmp(t.key, (const char*)&m_rxbuffer, strlen(t.key)) == 0) {
				m_exclmarkfound=1;
			}
			break;
		} //switch

		if((found)&&(m_exclmarkfound))
		{
			strncpy(value, (const char*)&m_rxbuffer+t.start, t.width);
			//send(t.topic, value);
		}
	}
}

void P1MeterSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled

	m_sharedserver.SendToAll((const char*)data,len);

	size_t ii=0;
	while (ii<len)
	{
		const unsigned char c = data[ii];
		if(c == 0x0d)
			continue;

		m_rxbuffer[m_rxbufferpos] = c;
		if(c == 0x0a || m_rxbufferpos == sizeof(m_rxbuffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if(m_rxbufferpos > 0) m_rxbuffer[m_rxbufferpos] = 0;
			m_linecount++;
			MatchLine();
			m_rxbufferpos = 0;
		}
		else
		{
			m_rxbufferpos++;
		}
		ii++;
	}
}

void P1MeterSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
}
