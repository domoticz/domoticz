#include "stdafx.h"
#include "FritzboxTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include "../main/SQLHelper.h"
#include "../hardware/hardwaretypes.h"

#include <sstream>

#define RETRY_DELAY 30

/*
For this to work you have to enable the CallMonitor on your Fritzbox.

dial:
#96*5* Enable
#96*4* Disable

Ausgehende Anrufe:
datum;CALL;ConnectionID;Nebenstelle;GenutzteNummer;AngerufeneNummer;

Eingehende Anrufe:
datum;RING;ConnectionID;Anrufer-Nr;Angerufene-Nummer;

Zustandegekommene Verbindung:
datum;CONNECT;ConnectionID;Nebenstelle;Nummer;

Ende der Verbindung:
datum;DISCONNECT;ConnectionID;dauerInSekunden;


*/

FritzboxTCP::FritzboxTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_retrycntr = RETRY_DELAY;
	m_bufferpos = 0;
}

FritzboxTCP::~FritzboxTCP(void)
{
}

bool FritzboxTCP::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&FritzboxTCP::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

bool FritzboxTCP::StopHardware()
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

void FritzboxTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"Fritzbox: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	m_bIsStarted=true;
	m_bufferpos=0;

	sOnConnected(this);
}

void FritzboxTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"Fritzbox: disconnected");
}

void FritzboxTCP::Do_Work()
{
	int sec_counter = 0;
	connect(m_szIPAddress,m_usIPPort);
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
	}
	terminate();

	_log.Log(LOG_STATUS,"Fritzbox: TCP/IP Worker stopped...");
}

void FritzboxTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseData(pData,length);
}

void FritzboxTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"Fritzbox: Error: %s",e.what());
}

void FritzboxTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "Fritzbox: Can not connect to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "Fritzbox: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "Fritzbox: %s", error.message().c_str());
}

bool FritzboxTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isConnected())
	{
		return false;
	}
	write((const unsigned char*)pdata,length);
	return true;
}

void FritzboxTCP::WriteInt(const std::string &sendStr)
{
	if (!isConnected())
	{
		return;
	}
	write(sendStr);
}

void FritzboxTCP::ParseData(const unsigned char *pData, int Len)
{
	int ii = 0;
	while (ii < Len)
	{
		const unsigned char c = pData[ii];
		if (c == 0x0d)
		{
			ii++;
			continue;
		}

		if (c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if (m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

void FritzboxTCP::UpdateSwitch(const unsigned char Idx, const uint8_t SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	double rlevel = (15.0 / 100)*Level;
	uint8_t level = (uint8_t)(rlevel);

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit ==%d)", m_HwdID, szIdx, SubUnit);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
		{
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel == level)
				return;
		}
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = SubUnit;
	if (!bOn)
	{
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

void FritzboxTCP::ParseLine()
{
	if (m_bufferpos < 2)
		return;
	std::string sLine((char*)&m_buffer);

	//_log.Log(LOG_STATUS, sLine.c_str());

	std::vector<std::string> results;
	StringSplit(sLine, ";", results);
	if (results.size() < 4)
		return; //invalid data

	std::string Cmd = results[1];
	std::stringstream sstr;
	std::string devname;
	if (Cmd == "CALL")
	{
		//Outgoing
		//datum;CALL;ConnectionID;Nebenstelle;GenutzteNummer;AngerufeneNummer;
		if (results.size() < 6)
			return;
		sstr << "Call From: " << results[4] << " to: " << results[5];
		m_sql.UpdateValue(m_HwdID, "1", 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, sstr.str().c_str(), devname);
	}
	else if (Cmd == "RING")
	{
		//Incoming
		//datum;RING;ConnectionID;Anrufer-Nr;Angerufene-Nummer;
		if (results.size() < 5)
			return;
		sstr << "Received From: " << results[3] << " to: " << results[4];
		m_sql.UpdateValue(m_HwdID, "1", 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, sstr.str().c_str(), devname);
	}
	else if (Cmd == "CONNECT")
	{
		//Connection made
		//datum;CONNECT;ConnectionID;Nebenstelle;Nummer;
		if (results.size() < 5)
			return;

		UpdateSwitch(1, 1, true, 100, "Call");

		sstr << "Connected ID: " << results[2] << " Number: " << results[4];
		m_sql.UpdateValue(m_HwdID, "1", 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, sstr.str().c_str(), devname);
		//result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, int(pTypeGeneral), int(sTypeTextStatus));
		//if (!result.empty())
		//{
		//	std::string idx = result[0][0];
		//	m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, sValue) VALUES ('%q', '%q')", idx.c_str(), sstr.str().c_str());
		//}
	}
	else if (Cmd == "DISCONNECT")
	{
		//Connection closed
		//datum;DISCONNECT;ConnectionID;dauerInSekunden;
		if (results.size() < 4)
			return;

		UpdateSwitch(1, 1, false, 100, "Call");

		sstr << "Disconnect ID: " << results[2] << " Duration: " << results[3] << " seconds";
		m_sql.UpdateValue(m_HwdID, "1", 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, sstr.str().c_str(), devname);
		//result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, int(pTypeGeneral), int(sTypeTextStatus));
		//if (!result.empty())
		//{
		//	std::string idx = result[0][0];
		//	m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, sValue) VALUES ('%q', '%q')", idx.c_str(), sstr.str().c_str());
		//}
	}
}
