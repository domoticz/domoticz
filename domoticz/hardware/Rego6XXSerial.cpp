#include "stdafx.h"
#include "Rego6XXSerial.h"

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

#define Rego6XX_THREAD_SLEEP_TIME 1
#define Rego6XX_RETRY_DELAY 30
#define Rego6XX_COMMAND_DELAY 5
#define Rego6XX_READ_BUFFER_MASK (Rego6XX_READ_BUFFER_SIZE - 1)

typedef enum {
	REGO_TYPE_NONE=0,
	REGO_TYPE_TEMP,
	REGO_TYPE_STATUS,
    REGO_TYPE_COUNTER,
    REGO_TYPE_COLDDELTA,
    REGO_TYPE_HOTDELTA,
    REGO_TYPE_TOTCOMP,
	REGO_TYPE_END
} RegoRegType;

typedef struct _tRegoRegisters 
{
	std::string name;
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
	{ "Compressor starts",      0x0046, 0x0048, 0x004A, REGO_TYPE_COUNTER,      -50.0, -1, 0 },
	{ "Radiator hours",         0x0048, 0x004A, 0x004C, REGO_TYPE_COUNTER,      -50.0, -1, 0 },
	{ "Hot water hours",        0x004A, 0x004C, 0x004E, REGO_TYPE_COUNTER,      -50.0, -1, 0 },
	{ "Cold fluid delta",		     0,	     0,	     0,	REGO_TYPE_COLDDELTA,    -50.0, -1, 0 },
	{ "Hot fluid delta",		     0,	     0,	     0,	REGO_TYPE_HOTDELTA,     -50.0, -1, 0 },
	{ "Total compressor hours",      0,      0,      0, REGO_TYPE_TOTCOMP,      -50.0, -1, 0 },
	{ "",                            0,      0,      0, REGO_TYPE_NONE,             0,  0, 0}
};


//
//Class Rego6XXSerial
//
CRego6XXSerial::CRego6XXSerial(const int ID, const std::string& devname, const int type)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
    m_regoType = type;

	m_stoprequested=false;
	m_retrycntr = 0;
	m_pollcntr = -1;
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
	clearReadCallback();
}

bool CRego6XXSerial::StartHardware()
{
	m_retrycntr=Rego6XX_RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRego6XXSerial::Do_Work, this)));

	return (m_thread!=NULL);
}

bool CRego6XXSerial::StopHardware()
{
	m_stoprequested=true;
	m_thread->join();
    // Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	return true;
}


void CRego6XXSerial::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(Rego6XX_THREAD_SLEEP_TIME));
		if (m_stoprequested)
			break;
		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				std::cout << "Rego6XX serial retrying in " << std::dec << Rego6XX_RETRY_DELAY << " seconds..." << std::endl;
			}
			m_retrycntr++;
			if (m_retrycntr>=Rego6XX_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_pollcntr = -1;
				m_pollDelaycntr = 0;
				m_readBufferHead = 0;
				m_readBufferTail = 0;
				OpenSerialDevice();
			}
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
				m_pollcntr++;
                if(g_allRegisters[m_pollcntr].type == REGO_TYPE_COLDDELTA)
                {
    		        time_t atime=time(NULL);
                    if((atime - g_allRegisters[8].lastSent <= 600) && // Cold fluid in is fresh
                       (atime - g_allRegisters[9].lastSent <= 600)) // Cold fluid out is fresh 
                    {
        			    m_Rego6XXTemp.ID = g_allRegisters[m_pollcntr].name;
 					    m_Rego6XXTemp.temperature =  g_allRegisters[8].lastTemp - g_allRegisters[9].lastTemp;
                        if((fabs(m_Rego6XXTemp.temperature - g_allRegisters[m_pollcntr].lastTemp) > 0.09) || // Only send changes.
                             (atime - g_allRegisters[m_pollcntr].lastSent >= 300)) // Send at least every 5 minutes
                        {
                            g_allRegisters[m_pollcntr].lastSent = atime;
                            g_allRegisters[m_pollcntr].lastTemp = m_Rego6XXTemp.temperature;
					        sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXTemp);//decode message
					        m_sharedserver.SendToAll((const char*)&m_Rego6XXTemp,sizeof(m_Rego6XXTemp));
                        }
                    }
                }
                else if(g_allRegisters[m_pollcntr].type == REGO_TYPE_HOTDELTA)
                {
    		        time_t atime=time(NULL);
                    if((atime - g_allRegisters[7].lastSent <= 600) && // Hot fluid in is fresh
                       (atime - g_allRegisters[6].lastSent <= 600)) // Hot fluid out is fresh 
                    {
        			    m_Rego6XXTemp.ID = g_allRegisters[m_pollcntr].name;
 					    m_Rego6XXTemp.temperature =  g_allRegisters[6].lastTemp - g_allRegisters[7].lastTemp;
                        if((fabs(m_Rego6XXTemp.temperature - g_allRegisters[m_pollcntr].lastTemp) > 0.09) || // Only send changes.
                             (atime - g_allRegisters[m_pollcntr].lastSent >= 300)) // Send at least every 5 minutes
                        {
                            g_allRegisters[m_pollcntr].lastSent = atime;
                            g_allRegisters[m_pollcntr].lastTemp = m_Rego6XXTemp.temperature;
					        sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXTemp);//decode message
					        m_sharedserver.SendToAll((const char*)&m_Rego6XXTemp,sizeof(m_Rego6XXTemp));
                        }
                    }
                }
                else if(g_allRegisters[m_pollcntr].type == REGO_TYPE_TOTCOMP)
                {
    		        time_t atime=time(NULL);
                    if((atime - g_allRegisters[20].lastSent <= 7200) && // Radiator hours is fresh
                       (atime - g_allRegisters[21].lastSent <= 7200)) // Hot water hours is fresh 
                    {
        			    m_Rego6XXValue.ID = g_allRegisters[m_pollcntr].name;
 					    m_Rego6XXValue.value =  g_allRegisters[20].lastValue + g_allRegisters[21].lastValue;
                        if((m_Rego6XXValue.value != g_allRegisters[m_pollcntr].lastValue) || // Only send changes.
                             (atime - g_allRegisters[m_pollcntr].lastSent >= 3600)) // Send at least every 60 minutes
                        {
                            g_allRegisters[m_pollcntr].lastSent = atime;
                            g_allRegisters[m_pollcntr].lastValue = m_Rego6XXValue.value;
					        sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXValue);//decode message
					        m_sharedserver.SendToAll((const char*)&m_Rego6XXValue,sizeof(m_Rego6XXValue));
                        }
                    }
                }
				else if(g_allRegisters[m_pollcntr].type != REGO_TYPE_NONE)
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
                        cmd.data.regNum[0] = (g_allRegisters[m_pollcntr].regNum_type1 & 0xC000) >> 14 ;
					    cmd.data.regNum[1] = (g_allRegisters[m_pollcntr].regNum_type1 & 0x3F80) >> 7 ;
					    cmd.data.regNum[2] = g_allRegisters[m_pollcntr].regNum_type1 & 0x007F;
                        break;
                    case 2:
                        cmd.data.regNum[0] = (g_allRegisters[m_pollcntr].regNum_type1 & 0xC000) >> 14 ;
					    cmd.data.regNum[1] = (g_allRegisters[m_pollcntr].regNum_type1 & 0x3F80) >> 7 ;
					    cmd.data.regNum[2] = g_allRegisters[m_pollcntr].regNum_type1 & 0x007F;
                        break;
                    default:
                		std::cerr << "Rego6XX Unknown type!" << std::endl;
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
					m_pollcntr = -1;
				}
			}
			else
			{
				// Try to parse data
				ParseData();
			}
		}
	}
	std::cout << "Serial Worker stopped..." << std::endl;
} 


bool CRego6XXSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 19200);
		std::cout << "Rego6XX Using serial port: " << m_szSerialPort << std::endl;
	}
	catch (boost::exception & e)
	{
		std::cerr << "Rego6XX Error opening serial port!" << std::endl;
#ifdef _DEBUG
		std::cerr << "-----------------" << std::endl << boost::diagnostic_information(e) << "-----------------" << std::endl;
#endif
		return false;
	}
	catch ( ... )
	{
		std::cerr << "Rego6XX Error opening serial port!!!" << std::endl;
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CRego6XXSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void CRego6XXSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

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

void CRego6XXSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isOpen()) {
		write(pdata,length);
		//std::cout << "Rego6XX Write: " ;
        //for ( int i = 0; i < length; i++ )
        //{
        //    std::cout << std::hex << (unsigned short)((unsigned char)(*pdata)) << " ";
        //    pdata++;
        //}
		//std::cout << std::dec << std::endl;
	}
}

void CRego6XXSerial::ParseData()
{
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
		        time_t atime=time(NULL);
                signed short data = 0;
				data = (m_readBuffer[(tail + 1) & Rego6XX_READ_BUFFER_MASK] << 14) |
					   (m_readBuffer[(tail + 2) & Rego6XX_READ_BUFFER_MASK] << 7) | 
						m_readBuffer[(tail + 3) & Rego6XX_READ_BUFFER_MASK];

				if(g_allRegisters[m_pollcntr].type == REGO_TYPE_TEMP)
				{
        			m_Rego6XXTemp.ID = g_allRegisters[m_pollcntr].name;
					m_Rego6XXTemp.temperature =  (float)(data * 0.1);
                    if((m_Rego6XXTemp.temperature >= -48.2) && // -48.3 means no sensor.
                        ((fabs(m_Rego6XXTemp.temperature - g_allRegisters[m_pollcntr].lastTemp) > 0.09) || // Only send changes.
                         (atime - g_allRegisters[m_pollcntr].lastSent >= 300))) // Send at least every 5 minutes
                    {
                        g_allRegisters[m_pollcntr].lastSent = atime;
                        g_allRegisters[m_pollcntr].lastTemp = m_Rego6XXTemp.temperature;
					    sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXTemp);//decode message
					    m_sharedserver.SendToAll((const char*)&m_Rego6XXTemp,sizeof(m_Rego6XXTemp));
                    }
				}
				else if(g_allRegisters[m_pollcntr].type == REGO_TYPE_STATUS)
				{
        			m_Rego6XXValue.ID = g_allRegisters[m_pollcntr].name;
					m_Rego6XXValue.value = data; 
                	m_Rego6XXValue.subtype=sTypeRego6XXStatus;
                    if((m_Rego6XXValue.value != g_allRegisters[m_pollcntr].lastValue) || // Only send changes.
                       (atime - g_allRegisters[m_pollcntr].lastSent >= (3600 * 23))) // Send at least every 23 hours
                    {
                        g_allRegisters[m_pollcntr].lastSent = atime;
                        g_allRegisters[m_pollcntr].lastValue = m_Rego6XXValue.value;
    					sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXValue);//decode message
	    				m_sharedserver.SendToAll((const char*)&m_Rego6XXValue,sizeof(m_Rego6XXValue));
                    }
				}
				else if(g_allRegisters[m_pollcntr].type == REGO_TYPE_COUNTER)
				{
        			m_Rego6XXValue.ID = g_allRegisters[m_pollcntr].name;
					m_Rego6XXValue.value = data; 
                	m_Rego6XXValue.subtype=sTypeRego6XXCounter;
                    if((m_Rego6XXValue.value != g_allRegisters[m_pollcntr].lastValue) || // Only send changes.
                       (atime - g_allRegisters[m_pollcntr].lastSent >= 3600)) // Send at least every 60 minutes
                    {
                        g_allRegisters[m_pollcntr].lastSent = atime;
                        g_allRegisters[m_pollcntr].lastValue = m_Rego6XXValue.value;
    					sDecodeRXMessage(this, (const unsigned char *)&m_Rego6XXValue);//decode message
	    				m_sharedserver.SendToAll((const char*)&m_Rego6XXValue,sizeof(m_Rego6XXValue));
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
}
