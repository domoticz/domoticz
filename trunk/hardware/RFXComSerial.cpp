#include "stdafx.h"
#include "RFXComSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

#define RETRY_DELAY 30

//
//Class RFXComSerial
//
RFXComSerial::RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	m_stoprequested=false;
	m_bReceiverStarted = false;
}

RFXComSerial::RFXComSerial(const std::string& devname,
        unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
}

RFXComSerial::~RFXComSerial()
{
	clearReadCallback();
}

bool RFXComSerial::StartHardware()
{
	//return OpenSerialDevice();
	//somehow retry does not seem to work?!
	m_bReceiverStarted = false;

	m_retrycntr=RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RFXComSerial::Do_Work, this)));

	return (m_thread!=NULL);

}

bool RFXComSerial::StopHardware()
{
	m_stoprequested=true;
	if (m_thread!=NULL)
		m_thread->join();
    // Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
    sleep_milliseconds(10);
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
	m_bIsStarted=false;
	return true;
}

void RFXComSerial::Do_Work()
{
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (m_stoprequested)
			break;
		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"RFXCOM: retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
			}
		}

	}
	_log.Log(LOG_STATUS,"RFXCOM: Serial Worker stopped...");
} 


bool RFXComSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort,m_iBaudRate);
		_log.Log(LOG_STATUS,"RFXCOM: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"RFXCOM: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"RFXCOM: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_rxbufferpos=0;
	setReadCallback(boost::bind(&RFXComSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool RFXComSerial::onInternalMessage(const unsigned char *pBuffer, const size_t Len)
{
	if (!m_bEnableReceive)
		return true; //receiving not enabled

	size_t ii = 0;
	while (ii < Len)
	{
		if (m_rxbufferpos == 0)	//1st char of a packet received
		{
			if (pBuffer[ii] == 0) //ignore first char if 00
				return true;
		}
		m_rxbuffer[m_rxbufferpos] = pBuffer[ii];
		m_rxbufferpos++;
		if (m_rxbufferpos >= sizeof(m_rxbuffer))
		{
			//something is out of sync here!!
			//restart
			_log.Log(LOG_ERROR, "input buffer out of sync, going to restart!....");
			m_rxbufferpos = 0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			if (!m_bReceiverStarted)
			{
				if (m_rxbuffer[1] == pTypeInterfaceMessage)
				{
					const tRBUF *pResponse = (tRBUF *)&m_rxbuffer;
					if (pResponse->IRESPONSE.subtype == cmdStartRec)
					{
						m_bReceiverStarted = strstr((char*)&pResponse->IRESPONSE.msg1, "Copyright RFXCOM") != NULL;
					}
					else
					{
						_log.Log(LOG_STATUS, "RFXCOM: Please upgrade your RFXTrx Firmware!...");
						m_bReceiverStarted = true;
					}
				}
			}
			else
				sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer);//decode message
			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
	return true;
}

void RFXComSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		bool bRet = onInternalMessage((const unsigned char *)data, len);
		if (bRet==false)
		{
			//close serial connection, and restart
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

		}
	}
	catch (...)
	{

	}
}


bool RFXComSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return false;
	write(pdata,length);
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		char * CWebServer::SetRFXCOMMode()
		{
			m_retstr = "/index.html";

			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID=" << idx << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();

			unsigned char Mode1 = atoi(result[0][0].c_str());
			unsigned char Mode2 = atoi(result[0][1].c_str());
			unsigned char Mode3 = atoi(result[0][2].c_str());
			unsigned char Mode4 = atoi(result[0][3].c_str());
			unsigned char Mode5 = atoi(result[0][4].c_str());
			unsigned char Mode6 = atoi(result[0][5].c_str());

			tRBUF Response;
			Response.ICMND.msg1 = Mode1;
			Response.ICMND.msg2 = Mode2;
			Response.ICMND.msg3 = Mode3;
			Response.ICMND.msg4 = Mode4;
			Response.ICMND.msg5 = Mode5;
			Response.ICMND.msg6 = Mode6;

			Response.IRESPONSE.UNDECODEDenabled = (m_pWebEm->FindValue("undecon") == "on") ? 1 : 0;
			Response.IRESPONSE.X10enabled = (m_pWebEm->FindValue("X10") == "on") ? 1 : 0;
			Response.IRESPONSE.ARCenabled = (m_pWebEm->FindValue("ARC") == "on") ? 1 : 0;
			Response.IRESPONSE.ACenabled = (m_pWebEm->FindValue("AC") == "on") ? 1 : 0;
			Response.IRESPONSE.HEEUenabled = (m_pWebEm->FindValue("HomeEasyEU") == "on") ? 1 : 0;
			Response.IRESPONSE.MEIANTECHenabled = (m_pWebEm->FindValue("Meiantech") == "on") ? 1 : 0;
			Response.IRESPONSE.OREGONenabled = (m_pWebEm->FindValue("OregonScientific") == "on") ? 1 : 0;
			Response.IRESPONSE.ATIenabled = (m_pWebEm->FindValue("ATIremote") == "on") ? 1 : 0;
			Response.IRESPONSE.VISONICenabled = (m_pWebEm->FindValue("Visonic") == "on") ? 1 : 0;
			Response.IRESPONSE.MERTIKenabled = (m_pWebEm->FindValue("Mertik") == "on") ? 1 : 0;
			Response.IRESPONSE.LWRFenabled = (m_pWebEm->FindValue("ADLightwaveRF") == "on") ? 1 : 0;
			Response.IRESPONSE.HIDEKIenabled = (m_pWebEm->FindValue("HidekiUPM") == "on") ? 1 : 0;
			Response.IRESPONSE.LACROSSEenabled = (m_pWebEm->FindValue("LaCrosse") == "on") ? 1 : 0;
			Response.IRESPONSE.FS20enabled = (m_pWebEm->FindValue("FS20") == "on") ? 1 : 0;
			Response.IRESPONSE.PROGUARDenabled = (m_pWebEm->FindValue("ProGuard") == "on") ? 1 : 0;
			Response.IRESPONSE.BLINDST0enabled = (m_pWebEm->FindValue("BlindT0") == "on") ? 1 : 0;
			Response.IRESPONSE.BLINDST1enabled = (m_pWebEm->FindValue("BlindT1T2T3T4") == "on") ? 1 : 0;
			Response.IRESPONSE.AEenabled = (m_pWebEm->FindValue("AEBlyss") == "on") ? 1 : 0;
			Response.IRESPONSE.RUBICSONenabled = (m_pWebEm->FindValue("Rubicson") == "on") ? 1 : 0;
			Response.IRESPONSE.FINEOFFSETenabled = (m_pWebEm->FindValue("FineOffsetViking") == "on") ? 1 : 0;
			Response.IRESPONSE.LIGHTING4enabled = (m_pWebEm->FindValue("Lighting4") == "on") ? 1 : 0;
			Response.IRESPONSE.RSLenabled = (m_pWebEm->FindValue("RSL") == "on") ? 1 : 0;
			Response.IRESPONSE.SXenabled = (m_pWebEm->FindValue("ByronSX") == "on") ? 1 : 0;
			Response.IRESPONSE.IMAGINTRONIXenabled = (m_pWebEm->FindValue("ImaginTronix") == "on") ? 1 : 0;
			Response.IRESPONSE.KEELOQenabled = (m_pWebEm->FindValue("Keeloq") == "on") ? 1 : 0;

			m_mainworker.SetRFXCOMHardwaremodes(atoi(idx.c_str()), Response.ICMND.msg1, Response.ICMND.msg2, Response.ICMND.msg3, Response.ICMND.msg4, Response.ICMND.msg5, Response.ICMND.msg6);

			return (char*)m_retstr.c_str();
		}
	}
}
