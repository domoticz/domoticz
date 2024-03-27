#include "stdafx.h"
#include "RFXComSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>

#include <string>
#include <algorithm>
#include <iostream>
#include <ctime>

#ifndef WIN32
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#define RETRY_DELAY 30

#define RFX_WRITE_DELAY 300

extern std::string szStartupFolder;

const unsigned char PKT_STX = 0x55;
const unsigned char PKT_ETX = 0x04;
const unsigned char PKT_DLE = 0x05;

#define PKT_writeblock 256
#define PKT_readblock 4
#define PKT_eraseblock 2048
#define PKT_maxpacket 261
#define PKT_bytesperaddr 2

#define PKT_pmrangelow	0x001800
#define PKT_pmrangehigh	0x00A7FF

#define PKT_pmrangelow868	0x001000
#define PKT_pmrangehigh868 0x0147FF

#define PKT_userresetvector 0x100
#define PKT_bootdelay 0x102

#define COMMAND_WRITEPM 2
#define COMMAND_ERASEPM 3

const unsigned char PKT_STARTBOOT[5] = { 0x01, 0x01, 0x00, 0x00, 0xFF };
const unsigned char PKT_RESET[2] = { 0x00, 0x00 };
const unsigned char PKT_VERSION[2] = { 0x00, 0x02 };
const unsigned char PKT_VERIFY_OK[5] = { 0x08, 0x01, 0x00, 0x00, 0x00 };

//
//Class RFXComSerial
//
RFXComSerial::RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate, const _eRFXAsyncType AsyncType) :
	m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_iBaudRate = baud_rate;

	m_AsyncType = AsyncType;

	m_bReceiverStarted = false;
	m_retrycntr = RETRY_DELAY;
}

bool RFXComSerial::StartHardware()
{
	RequestStart();

	//return OpenSerialDevice();
	//somehow retry does not seem to work?!
	m_bReceiverStarted = false;

	m_retrycntr = RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool RFXComSerial::StopHardware()
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

void RFXComSerial::Do_Work()
{
	int sec_counter = 0;

	Log(LOG_STATUS, "Worker started...");

	while (IsStopRequested(1000) == false)
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (!isOpen())
		{
			if (m_retrycntr == 0)
			{
				Log(LOG_STATUS, "retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				OpenSerialDevice();
			}
		}
	}
	terminate(); //Close serial port (if open)

	Log(LOG_STATUS, "Worker stopped...");
}


bool RFXComSerial::OpenSerialDevice(const bool bIsFirmwareUpgrade)
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, m_iBaudRate);
		Log(LOG_STATUS, "Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception& e)
	{
		Log(LOG_ERROR, "Error opening serial port!");
#ifdef _DEBUG
		Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error opening serial port!!!");
		return false;
	}
	m_bIsStarted = true;
	m_rxbufferpos = 0;
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	if (!bIsFirmwareUpgrade)
		sOnConnected(this);
	return true;
}

void RFXComSerial::readCallback(const char* data, size_t len)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	try
	{
		bool bRet = onInternalMessage((const unsigned char*)data, len);
		if (bRet == false)
		{
			//close serial connection, and restart
			terminate();

		}
	}
	catch (...)
	{
	}
}

bool RFXComSerial::WriteToHardware(const char* pdata, const unsigned char length)
{
	if (!isOpen())
		return false;
	write(pdata, length);
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SetRFXCOMMode(WebEmSession& session, const request& req, std::string& redirect_uri)
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

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, [Type] FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.empty())
				return;

			unsigned char Mode1 = stoi(result[0][0]);
			unsigned char Mode2 = stoi(result[0][1]);
			unsigned char Mode3 = stoi(result[0][2]);
			unsigned char Mode4 = stoi(result[0][3]);
			unsigned char Mode5 = stoi(result[0][4]);
			unsigned char Mode6 = stoi(result[0][5]);

			_eHardwareTypes HWType = (_eHardwareTypes) stoi(result[0][6]);

			tRBUF Response;
			Response.ICMND.freqsel = Mode1;
			Response.ICMND.xmitpwr = Mode2;
			Response.ICMND.msg3 = Mode3;
			Response.ICMND.msg4 = Mode4;
			Response.ICMND.msg5 = Mode5;
			Response.ICMND.msg6 = Mode6;

			if (HWType != HTYPE_RFXtrx868)
			{
				Response.IRESPONSE.UNDECODEDenabled = (request::findValue(&req, "undecon") == "on") ? 1 : 0;
				Response.IRESPONSE.X10enabled = (request::findValue(&req, "X10") == "on") ? 1 : 0;
				Response.IRESPONSE.ARCenabled = (request::findValue(&req, "ARC") == "on") ? 1 : 0;
				Response.IRESPONSE.ACenabled = (request::findValue(&req, "AC") == "on") ? 1 : 0;
				Response.IRESPONSE.HEEUenabled = (request::findValue(&req, "HomeEasyEU") == "on") ? 1 : 0;
				Response.IRESPONSE.MEIANTECHenabled = (request::findValue(&req, "Meiantech") == "on") ? 1 : 0;
				Response.IRESPONSE.OREGONenabled = (request::findValue(&req, "OregonScientific") == "on") ? 1 : 0;
				Response.IRESPONSE.ATIenabled = (request::findValue(&req, "ATIremote") == "on") ? 1 : 0;
				Response.IRESPONSE.VISONICenabled = (request::findValue(&req, "Visonic") == "on") ? 1 : 0;
				Response.IRESPONSE.MERTIKenabled = (request::findValue(&req, "Mertik") == "on") ? 1 : 0;
				Response.IRESPONSE.LWRFenabled = (request::findValue(&req, "ADLightwaveRF") == "on") ? 1 : 0;
				Response.IRESPONSE.HIDEKIenabled = (request::findValue(&req, "HidekiUPM") == "on") ? 1 : 0;
				Response.IRESPONSE.LACROSSEenabled = (request::findValue(&req, "LaCrosse") == "on") ? 1 : 0;
				Response.IRESPONSE.LEGRANDenabled = (request::findValue(&req, "Legrand") == "on") ? 1 : 0;
				Response.IRESPONSE.MSG4Reserved5 = (request::findValue(&req, "ProGuard") == "on") ? 1 : 0;
				Response.IRESPONSE.BLINDST0enabled = (request::findValue(&req, "BlindT0") == "on") ? 1 : 0;
				Response.IRESPONSE.BLINDST1enabled = (request::findValue(&req, "BlindT1T2T3T4") == "on") ? 1 : 0;
				Response.IRESPONSE.AEenabled = (request::findValue(&req, "AEBlyss") == "on") ? 1 : 0;
				Response.IRESPONSE.RUBICSONenabled = (request::findValue(&req, "Rubicson") == "on") ? 1 : 0;
				Response.IRESPONSE.FINEOFFSETenabled = (request::findValue(&req, "FineOffsetViking") == "on") ? 1 : 0;
				Response.IRESPONSE.LIGHTING4enabled = (request::findValue(&req, "Lighting4") == "on") ? 1 : 0;
				Response.IRESPONSE.RSLenabled = (request::findValue(&req, "RSL") == "on") ? 1 : 0;
				Response.IRESPONSE.SXenabled = (request::findValue(&req, "ByronSX") == "on") ? 1 : 0;
				Response.IRESPONSE.IMAGINTRONIXenabled = (request::findValue(&req, "ImaginTronix") == "on") ? 1 : 0;
				Response.IRESPONSE.KEELOQenabled = (request::findValue(&req, "Keeloq") == "on") ? 1 : 0;
				Response.IRESPONSE.HCEnabled = (request::findValue(&req, "HC") == "on") ? 1 : 0;

				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(stoi(idx));
				if (pHardware)
				{
					CRFXBase* pBase = dynamic_cast<CRFXBase*>(pHardware);
					pBase->SetRFXCOMHardwaremodes(Response.ICMND.freqsel, Response.ICMND.xmitpwr, Response.ICMND.msg3, Response.ICMND.msg4, Response.ICMND.msg5, Response.ICMND.msg6);

					if (pBase->m_Version.find("Pro XL") != std::string::npos)
					{
						std::string AsyncMode = request::findValue(&req, "combo_rfx_xl_async_type");
						if (AsyncMode.empty())
							AsyncMode = "0";
						result = m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID='%q')", AsyncMode.c_str(), idx.c_str());
						pBase->SetAsyncType((CRFXBase::_eRFXAsyncType) stoi(AsyncMode));
					}
				}
			}
			else
			{
				//For now disable setting the protocols on a 868MHz device
			}
		}
	} // namespace server
} // namespace http
