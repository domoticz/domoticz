#include "stdafx.h"
#include "FritzboxTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include "../main/SQLHelper.h"
extern CSQLHelper m_sql;
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

FritzboxTCP::FritzboxTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_bDoRestart=false;
#if defined WIN32
	int ret;
	//Init winsock
	WSADATA data;
	WORD version; 

	version = (MAKEWORD(2, 2)); 
	ret = WSAStartup(version, &data); 
	if (ret != 0) 
	{  
		ret = WSAGetLastError(); 

		if (ret == WSANOTINITIALISED) 
		{  
			_log.Log(LOG_ERROR,"Fritzbox: Winsock could not be initialized!");
		}
	}
#endif
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
}

FritzboxTCP::~FritzboxTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool FritzboxTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&FritzboxTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool FritzboxTCP::StopHardware()
{
	m_stoprequested=true;
	try {
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}

	m_bIsStarted=false;
	return true;
}

void FritzboxTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"Fritzbox: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
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
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"Fritzbox: TCP/IP Worker stopped...");
} 

void FritzboxTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData,length);
}

void FritzboxTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"Fritzbox: Error: %s",e.what());
}

void FritzboxTCP::OnError(const boost::system::error_code& error)
{
	_log.Log(LOG_ERROR,"Fritzbox: Error: %s",error.message().c_str());
}

bool FritzboxTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected)
	{
		return false;
	}
	write((const unsigned char*)pdata,length);
	return true;
}

void FritzboxTCP::WriteInt(const std::string &sendStr)
{
	if (!mIsConnected)
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

void FritzboxTCP::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	bool bDeviceExits = true;
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << SubUnit << ")";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	else
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << SubUnit << ")";
		m_sql.query(szQuery.str());
	}
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
	std::stringstream szQuery;
	std::string devname;
	unsigned long long devIdx;
	std::vector<std::vector<std::string> > result;
	if (Cmd == "CALL")
	{
		//Outgoing
		//datum;CALL;ConnectionID;Nebenstelle;GenutzteNummer;AngerufeneNummer;
		if (results.size() < 6)
			return;
		sstr << "Call From: " << results[4] << " to: " << results[5];
		devIdx = m_sql.UpdateValue(m_HwdID, "1", 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, sstr.str().c_str(), devname);
		sstr.clear();
		sstr.str("");
		sstr << "Call ID: " << results[2] << " From: " << results[4] << " to: " << results[5];
		szQuery << "INSERT INTO LightingLog (DeviceRowID, sValue) VALUES ('" << devIdx << "', '" << sstr.str() << "')";
		m_sql.query(szQuery.str());
	}
	else if (Cmd == "RING")
	{
		//Incoming
		//datum;RING;ConnectionID;Anrufer-Nr;Angerufene-Nummer;
		if (results.size() < 5)
			return;
		sstr << "Received From: " << results[3] << " to: " << results[4];
		devIdx=m_sql.UpdateValue(m_HwdID, "1", 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, sstr.str().c_str(), devname);
		sstr.clear();
		sstr.str("");
		sstr << "Received ID: " << results[2] << " From: " << results[3] << " to: " << results[4];
		szQuery << "INSERT INTO LightingLog (DeviceRowID, sValue) VALUES ('" << devIdx << "', '" << sstr.str() << "')";
		m_sql.query(szQuery.str());
	}
	else if (Cmd == "CONNECT")
	{
		//Connection made
		//datum;CONNECT;ConnectionID;Nebenstelle;Nummer;
		if (results.size() < 5)
			return;

		UpdateSwitch(1, 1, true, 100, "Call");

		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeTextStatus) << ")";
		result = m_sql.query(szQuery.str());
		if (!result.empty())
		{
			szQuery.clear();
			szQuery.str("");
			std::string idx = result[0][0];
			sstr << "Connected ID: " << results[2] << " Number: " << results[4];
			szQuery << "INSERT INTO LightingLog (DeviceRowID, sValue) VALUES ('" << idx << "', '" << sstr.str() << "')";
			m_sql.query(szQuery.str());
		}
	}
	else if (Cmd == "DISCONNECT")
	{
		//Connection closed
		//datum;DISCONNECT;ConnectionID;dauerInSekunden;
		if (results.size() < 4)
			return;

		UpdateSwitch(1, 1, false, 100, "Call");

		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeTextStatus) << ")";
		result = m_sql.query(szQuery.str());
		if (!result.empty())
		{
			szQuery.clear();
			szQuery.str("");
			std::string idx = result[0][0];
			sstr << "Disconnect ID: " << results[2] << " Duration: " << results[3] << " seconds";
			szQuery << "INSERT INTO LightingLog (DeviceRowID, sValue) VALUES ('" << idx << "', '" << sstr.str() << "')";
			m_sql.query(szQuery.str());
		}
	}
}
