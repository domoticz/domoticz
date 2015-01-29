#include "stdafx.h"
#include "MySensorsSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//#define DEBUG_MYSENSORS

MySensorsSerial::MySensorsSerial(const int ID, const std::string& devname)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=115200;

}

MySensorsSerial::~MySensorsSerial()
{
	clearReadCallback();
}

bool MySensorsSerial::StartHardware()
{
	LoadDevicesFromDatabase();

	StartHeartbeatThread();
#ifndef DEBUG_MYSENSORS
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"MySensors: Using serial port: %s", m_szSerialPort.c_str());
#ifndef WIN32
		openOnlyBaud(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
#else
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
#endif
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"MySensors: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n-----------------",boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"MySensors: Error opening serial port!!!");
		return false;
	}
#else
	std::ifstream infile;
	std::string sLine;
	//infile.open("D:\\MySensors.txt");
	infile.open("D:\\log-gw.txt");
	
	std::string orgstr;

	if (!infile.is_open())
		return false;
	while (!infile.eof())
	{
		getline(infile, sLine);

		std::vector<std::string> results;
		StringSplit(sLine, ";", results);
		if (results.size() != 6)
		{
			StringSplit(sLine, " ", results);
			if (results.size() >= 7)
			{
				orgstr=sLine;
				sLine = "";
				sLine += results[2] + ";";
				sLine += results[3] + ";";
				sLine += results[4] + ";";
				sLine += results[5] + ";";
				sLine += results[6] + ";";
				//Add the rest
				for (size_t ii = 7; ii < results.size(); ii++)
				{
					sLine += results[ii];
					if (ii != results.size() - 1)
						sLine += " ";
				}
				StringSplit(sLine, ";", results);
			}
		}
		if (results.size() != 6)
			continue;

		sLine += "\n";
		ParseData((const unsigned char*)sLine.c_str(), sLine.size());
	}
	infile.close();

#endif
	m_bIsStarted=true;
	m_bufferpos=0;
	setReadCallback(boost::bind(&MySensorsSerial::readCallback, this, _1, _2));
	sOnConnected(this);

	return true;
}

bool MySensorsSerial::StopHardware()
{
	m_bIsStarted=false;
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
	StopHeartbeatThread();
	return true;
}


void MySensorsSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

void MySensorsSerial::WriteInt(const std::string &sendStr)
{
	if (!isOpen())
		return;
	writeString(sendStr);
}

void MySensorsSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return;
	tRBUF *pCmd = (tRBUF *)pdata;
	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		//Light command
		int node_id = pCmd->LIGHTING2.id4;
		int child_sensor_id = pCmd->LIGHTING2.unitcode;
		std::stringstream sstr;
		std::string lState = (pCmd->LIGHTING2.cmnd == light2_sOn) ? "1" : "0";
		SendCommand(node_id, child_sensor_id, MT_Set, V_LIGHT, lState);
		//SendCommand(node_id, child_sensor_id, MT_Set, V_DIMMER, "100");
	}
}

