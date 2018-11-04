#include "stdafx.h"
#include "Rego6XXSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../main/SQLHelper.h"
#include "../webserver/cWebem.h"

// This code is inspired by the Rago600 project:
// http://rago600.sourceforge.net/
// And the TaloLogger:
// http://zil.olammi.iki.fi/sw/taloLogger/howto.php

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include "hardwaretypes.h"

#include <ctime>

#define Rego6XX_RETRY_DELAY 30
#define Rego6XX_COMMAND_DELAY 5
#define Rego6XX_READ_BUFFER_MASK (Rego6XX_READ_BUFFER_SIZE - 1)
#define Rego6XX_MAX_ERRORS_UNITL_RESTART 20

typedef enum {
	REGO_TYPE_NONE=0,
	REGO_TYPE_TEMP,
	REGO_TYPE_STATUS,
    REGO_TYPE_COUNTER,
	REGO_TYPE_END
} RegoRegType;

typedef struct _tRegoRegisters
{
	char name[25];
	unsigned short regNum_type1;
	unsigned short regNum_type2;
	unsigned short regNum_type3;
	RegoRegType type;
    float lastTemp;
    int lastValue;
    time_t lastSent;
} RegoRegisters;

typedef union _tRegoCommand
{
    unsigned char raw[9];
    struct
    {
        unsigned char address;
        unsigned char command;
        unsigned char regNum[3];
        unsigned char value[3];
        unsigned char crc;
    } data;
} RegoCommand;

typedef union _tRegoReply
{
    char raw[5];
    struct
    {
        char address;
        char value[3];
        char crc;
    } data;
} RegoReply;

RegoRegisters g_allRegisters[] = {
	{ "GT1 Radiator",           0x0209,	0x020B,	0x020D,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT2 Out",		        0x020A,	0x020C,	0x020E,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT3 Hot water",	        0x020B,	0x020D,	0x020F,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT4 Forward",	        0x020C,	0x020E,	0x0210,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT5 Room",			    0x020D,	0x020F,	0x0211,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT6 Compressor",	        0x020E,	0x0210,	0x0212,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT8 Hot fluid out",      0x020F,	0x0211,	0x0213,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT9 Hot fluid in",		0x0210,	0x0212,	0x0214,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT10 Cold fluid in",		0x0211,	0x0213,	0x0215,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT11 Cold fluid out",	0x0212,	0x0214,	0x0216,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT3x Ext hot water",		0x0213, 0x0215, 0x0217,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "P3 Cold fluid",	    	0x01FD,	0x01FF,	0x0201,	REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "Compressor",				0x01FE,	0x0200,	0x0202,	REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "Add heat 1",     		0x01FF,	0x0201,	0x0203,	REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "Add heat 2",		        0x0200,	0x0202,	0x0204,	REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "P1 Radiator",    		0x0203,	0x0205,	0x0207,	REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "P2 Heat fluid",  		0x0204, 0x0206, 0x0208, REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "Three-way valve",        0x0205, 0x0207, 0x0209, REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "Alarm",                  0x0206, 0x0208, 0x020A, REGO_TYPE_STATUS,       -50.0, -1, 0 },
	{ "Operating hours",        0x0046, 0x0048, 0x004A, REGO_TYPE_COUNTER,      -50.0, -1, 0 },
	{ "Radiator hours",         0x0048, 0x004A, 0x004C, REGO_TYPE_COUNTER,      -50.0, -1, 0 },
	{ "Hot water hours",        0x004A, 0x004C, 0x004E, REGO_TYPE_COUNTER,      -50.0, -1, 0 },
	{ "GT1 Target",             0x006E,	0x006E,	0x006E,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT3 Target",             0x002B,	0x002B,	0x002B,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "GT4 Target",             0x006D,	0x006D,	0x006D,	REGO_TYPE_TEMP,         -50.0, -1, 0 },
	{ "",                            0,      0,      0, REGO_TYPE_NONE,             0,  0, 0}
};


//
//Class Rego6XXSerial
//
CRego6XXSerial::CRego6XXSerial(const int ID, const std::string& devname, const int type) :
m_szSerialPort(devname)
{
	m_HwdID=ID;
    m_regoType = type;
    m_errorcntr = 0;

	m_retrycntr = 0;
	m_pollcntr = 0;
	m_pollDelaycntr = 0;
	m_readBufferHead = 0;
	m_readBufferTail = 0;

	m_Rego6XXTemp.len=sizeof(Rego6XXTemp)-1;
	m_Rego6XXTemp.type=pTypeRego6XXTemp;
	m_Rego6XXTemp.subtype=sTypeRego6XXTemp;
	m_Rego6XXTemp.temperature=0.0;

	m_Rego6XXValue.len=sizeof(Rego6XXStatus)-1;
	m_Rego6XXValue.type=pTypeRego6XXValue;
	m_Rego6XXValue.value=0;
}

CRego6XXSerial::~CRego6XXSerial()
{
	StopHardware();
}

bool CRego6XXSerial::StartHardware()
{
	RequestStart();

	m_retrycntr=Rego6XX_RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CRego6XXSerial::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool CRego6XXSerial::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
	return true;
}


void CRego6XXSerial::Do_Work()
{
	int sec_counter = 0;
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"Rego6XX: serial retrying in %d seconds...", Rego6XX_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=Rego6XX_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_pollcntr = 0;
				m_pollDelaycntr = 0;
				m_readBufferHead = 0;
				m_readBufferTail = 0;
				OpenSerialDevice();
			}
		}
		else if(m_errorcntr > Rego6XX_MAX_ERRORS_UNITL_RESTART)
        {
            // Reopen the port and clear the error counter.
			terminate();

		    _log.Log(LOG_ERROR,"Rego6XX: Reopening serial port");
		    sleep_seconds(2);

			m_retrycntr=0;
			m_pollcntr = 0;
			m_pollDelaycntr = 0;
			m_readBufferHead = 0;
			m_readBufferTail = 0;
            m_errorcntr = 0;

		    OpenSerialDevice();
        }
        else
		{
			m_pollDelaycntr++;

			if (m_pollDelaycntr>=Rego6XX_COMMAND_DELAY)
			{
                // Before issueing a new command, the recieve buffer must be empty.
                // It seems that there are errors sometimes and this will take care
                // of any problems even if it bypasses the circular buffer concept.
                // There should not be any data left to recieve anyway since commands
                // are sent with 5 seconds in between.
				m_readBufferHead = 0;
				m_readBufferTail = 0;

   				m_pollDelaycntr = 0;
				if(g_allRegisters[m_pollcntr].type != REGO_TYPE_NONE)
				{
					RegoCommand cmd;
					cmd.data.address = 0x81;
					cmd.data.command = 0x02;
                    switch(m_regoType)
                    {
                    case 0:
                        cmd.data.regNum[0] = (g_allRegisters[m_pollcntr].regNum_type1 & 0xC000) >> 14 ;
					    cmd.data.regNum[1] = (g_allRegisters[m_pollcntr].regNum_type1 & 0x3F80) >> 7 ;
					    cmd.data.regNum[2] = g_allRegisters[m_pollcntr].regNum_type1 & 0x007F;
                        break;
                    case 1:
                        cmd.data.regNum[0] = (g_allRegisters[m_pollcntr].regNum_type2 & 0xC000) >> 14 ;
					    cmd.data.regNum[1] = (g_allRegisters[m_pollcntr].regNum_type2 & 0x3F80) >> 7 ;
					    cmd.data.regNum[2] = g_allRegisters[m_pollcntr].regNum_type2 & 0x007F;
                        break;
                    case 2:
                        cmd.data.regNum[0] = (g_allRegisters[m_pollcntr].regNum_type3 & 0xC000) >> 14 ;
					    cmd.data.regNum[1] = (g_allRegisters[m_pollcntr].regNum_type3 & 0x3F80) >> 7 ;
					    cmd.data.regNum[2] = g_allRegisters[m_pollcntr].regNum_type3 & 0x007F;
                        break;
                    default:
		                _log.Log(LOG_ERROR,"Rego6XX: Unknown type!");
                        break;
                    }
					cmd.data.value[0] = 0;
					cmd.data.value[1] = 0;
					cmd.data.value[2] = 0;
					cmd.data.crc = 0;

					for ( int i = 2; i < 8; i++ )
						cmd.data.crc ^= cmd.raw[ i ];

					WriteToHardware((char*)cmd.raw, sizeof(cmd.raw));
				}
				else
				{
					m_pollcntr = 0;
				}
			}
			else
			{
				// Try to parse data
				if(ParseData())
                {
                    // Get the next message
       				m_pollcntr++;

                    m_errorcntr = 0;
                }
                else
                {
                    m_errorcntr++;
                }

			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"Rego6XX: Worker stopped...");
}


bool CRego6XXSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 19200);
		_log.Log(LOG_STATUS,"Rego6XX: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"Rego6XX: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"Rego6XX: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CRego6XXSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void CRego6XXSerial::readCallback(const char *data, size_t len)
{
	if (!m_bEnableReceive)
		return; //receiving not enabled

	try
	{
		for ( size_t i = 0; i < len; i++ )
		{
			unsigned char head = m_readBufferHead;
			unsigned char new_head = (head + 1) & Rego6XX_READ_BUFFER_MASK;

			if (new_head != m_readBufferTail)
			{
				m_readBuffer[head] = data[i];
				m_readBufferHead = new_head;
				//std::cout << std::hex << (unsigned short)((unsigned char)(data[i])) << std::dec << " ";
			}
		}

		//if(len > 0)
		//{
		//	std::cout << std::endl;
		//}
	}
	catch (...)
	{

	}
}

bool CRego6XXSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return false;
	write(pdata,length);
	return true;
}

bool CRego6XXSerial::ParseData()
{
    bool messageOK = false;
    while(((m_readBufferHead - m_readBufferTail) & Rego6XX_READ_BUFFER_MASK) >= 5)
    {
		// get a potential message
		// Peek at the tail
		unsigned char tail = m_readBufferTail;
		if(m_readBuffer[tail] == 0x01)
		{
			// Check crc
			if(m_readBuffer[(tail + 4) & Rego6XX_READ_BUFFER_MASK] ==
				(m_readBuffer[(tail + 1) & Rego6XX_READ_BUFFER_MASK] ^
				 m_readBuffer[(tail + 2) & Rego6XX_READ_BUFFER_MASK] ^
				 m_readBuffer[(tail + 3) & Rego6XX_READ_BUFFER_MASK]))
			{
				// This is a proper message
                messageOK = true;
		        time_t atime=mytime(NULL);
                signed short data = 0;
				data = (m_readBuffer[(tail + 1) & Rego6XX_READ_BUFFER_MASK] << 14) |
					   (m_readBuffer[(tail + 2) & Rego6XX_READ_BUFFER_MASK] << 7) |
						m_readBuffer[(tail + 3) & Rego6XX_READ_BUFFER_MASK];

				if(g_allRegisters[m_pollcntr].type == REGO_TYPE_TEMP)
				{
        			strcpy(m_Rego6XXTemp.ID, g_allRegisters[m_pollcntr].name);
					m_Rego6XXTemp.temperature =  (float)(data * 0.1);
                    if((m_Rego6XXTemp.temperature >= -48.2) && // -48.3 means no sensor.
                        ((fabs(m_Rego6XXTemp.temperature - g_allRegisters[m_pollcntr].lastTemp) > 0.09) || // Only send changes.
			 (difftime(atime,g_allRegisters[m_pollcntr].lastSent) >= 300))) // Send at least every 5 minutes
                    {
                        g_allRegisters[m_pollcntr].lastSent = atime;
                        g_allRegisters[m_pollcntr].lastTemp = m_Rego6XXTemp.temperature;
					    sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXTemp, NULL, 255);
                    }
				}
				else if(g_allRegisters[m_pollcntr].type == REGO_TYPE_STATUS)
				{
        			strcpy(m_Rego6XXValue.ID, g_allRegisters[m_pollcntr].name);
					m_Rego6XXValue.value = data;
                	m_Rego6XXValue.subtype=sTypeRego6XXStatus;
                    if((m_Rego6XXValue.value != g_allRegisters[m_pollcntr].lastValue) || // Only send changes.
			(difftime(atime,g_allRegisters[m_pollcntr].lastSent) >= (3600 * 23))) // Send at least every 23 hours
                    {
                        g_allRegisters[m_pollcntr].lastSent = atime;
                        g_allRegisters[m_pollcntr].lastValue = m_Rego6XXValue.value;
    					sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXValue, NULL, 255);
                    }
				}
				else if(g_allRegisters[m_pollcntr].type == REGO_TYPE_COUNTER)
				{
        			strcpy(m_Rego6XXValue.ID, g_allRegisters[m_pollcntr].name);
					m_Rego6XXValue.value = data;
                	m_Rego6XXValue.subtype=sTypeRego6XXCounter;
                    if((m_Rego6XXValue.value != g_allRegisters[m_pollcntr].lastValue) || // Only send changes.
			(difftime(atime,g_allRegisters[m_pollcntr].lastSent) >= 3000)) // Send at least every 50 minutes
                    {
                        g_allRegisters[m_pollcntr].lastSent = atime;
                        g_allRegisters[m_pollcntr].lastValue = m_Rego6XXValue.value;
    					sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXValue, NULL, 255);
                    }
				}

				// Remove the message from the buffer
				m_readBufferTail = (tail + 5) & Rego6XX_READ_BUFFER_MASK;
			}
			else
			{
				// It wasn't a proper message after the header. Remove and try again
				m_readBufferTail = (tail + 1) & Rego6XX_READ_BUFFER_MASK;
			}
		}
		else
		{
			// Remove this byte and try again
			m_readBufferTail = (tail + 1) & Rego6XX_READ_BUFFER_MASK;
		}
    }
    return messageOK;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SetRego6XXType(WebEmSession & session, const request& req, std::string & redirect_uri)
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
			if (result.empty())
				return;

			unsigned char currentMode1 = atoi(result[0][0].c_str());

			std::string sRego6XXType = request::findValue(&req, "Rego6XXType");
			unsigned char newMode1 = atoi(sRego6XXType.c_str());

			if (currentMode1 != newMode1)
			{
				m_sql.UpdateRFXCOMHardwareDetails(atoi(idx.c_str()), newMode1, 0, 0, 0, 0, 0);
			}
		}
	}
}
