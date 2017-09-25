/************************************************************************
*
Legrand MyHome / OpenWebNet USB Interface board driver for Domoticz
Date: 05-10-2016
Written by: Stéphane Lebrasseur
License: Public domain
************************************************************************/

#include "stdafx.h"
#include "OpenWebNetUSB.h"
#include "openwebnet/bt_openwebnet.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include "../main/SQLHelper.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include <ctime>


COpenWebNetUSB::COpenWebNetUSB(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_szSerialPort = devname;
	m_iBaudRate = baud_rate;
	m_retrycntr = RETRY_DELAY - 2;
	m_bHaveReceived = false;
	m_bEnableReceive = true;
}


COpenWebNetUSB::~COpenWebNetUSB()
{
}


bool COpenWebNetUSB::StartHardware()
{
	m_retrycntr = RETRY_DELAY - 2; //will force reconnect first thing

								   //Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenWebNetUSB::Do_Work, this)));

	return (m_thread != NULL);

	return true;
}

void COpenWebNetUSB::Do_Work()
{
	while (!m_stoprequested)
	{
		sleep_seconds(OPENWEBNET_HEARTBEAT_DELAY);
		m_LastHeartbeat = mytime(NULL);
	}
	_log.Log(LOG_STATUS, "COpenWebNetUSB: Heartbeat worker stopped...");
}


bool COpenWebNetUSB::StopHardware()
{
	m_stoprequested = true;
	if (m_thread != NULL)
		m_thread->join();
	// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
	sleep_milliseconds(10);
	terminate();
	m_bIsStarted = false;
	return true;
}

/**
Convert domoticz command in a OpenWebNet command, then send it to device
**/
bool COpenWebNetUSB::WriteToHardware(const char *pdata, const unsigned char length)
{
	_tGeneralSwitch *pCmd = (_tGeneralSwitch*)pdata;

	unsigned char packetlength = pCmd->len;
	unsigned char packettype = pCmd->type;
	unsigned char subtype = pCmd->subtype;

	int who = 0;
	int what = 0;
	stringstream whereStr;
	stringstream devIdStr;
	
	switch (subtype) {
		case sSwitchBlindsT1:
		case sSwitchLightT1:
			//Bus command
			whereStr << (int)(pCmd->id & 0xFFFF);
			break;
		case sSwitchBlindsT2:
		case sSwitchLightT2:
			//Zigbee command
			//Transmission unicast (no prefix) in ZigBee (#9 suffix) , command light ON and ID = 0x000501F8 (0d328184) and you want to control the UNIT 2 : * 1 * 1 * 32818402#9##
			//Transmission unicast (no prefix) in ZigBee (#9 suffix) , command light ON and ID = 0x000501F8 (0d328184) and you want to control the all UNIT : * 1 * 1 * 32818400#9##
			//Transmission broadcast (0# prefix) in ZigBee (#9 suffix), command light OFF and you want to control the UNIT 1 : * 1 * 0 * 0#01#9##
			//Transmission broadcast (0# prefix) in ZigBee (#9 suffix), command light OFF and you want to control the all UNIT : * 1 * 0 * 0#00#9##
			whereStr << pCmd->id * 100 + pCmd->unitcode << ZIGBEE_SUFFIX;
			break; 
		default:
			_log.Log(LOG_STATUS, "COpenWebNetUSB unknown command: packettype=%d subtype=%d", packettype, subtype);
			return false;
	}

	// Test packet type
	switch (packettype) {
		case pTypeGeneralSwitch:
			// Test general switch subtype
			switch (subtype) {
				case sSwitchBlindsT1:
				case sSwitchBlindsT2:
					//Blinds/Window command
					who = WHO_AUTOMATION;
			
					if (pCmd->cmnd == gswitch_sOff)
					{
						what = AUTOMATION_WHAT_UP;
					}
					else if (pCmd->cmnd == gswitch_sOn)
					{
						what = AUTOMATION_WHAT_DOWN;
					}
					else if (pCmd->cmnd == gswitch_sStop)
					{
						what = AUTOMATION_WHAT_STOP;
					}
					break;
				case sSwitchLightT1:
				case sSwitchLightT2:
					//Light/Switch command
					who = WHO_LIGHTING;
			
					if (pCmd->cmnd == gswitch_sOff)
					{
						what = LIGHTING_WHAT_OFF;
					}
					else if (pCmd->cmnd == gswitch_sOn)
					{
						what = LIGHTING_WHAT_ON;
					}
					else if (pCmd->cmnd == gswitch_sSetLevel)
					{
						// setting level of dimmer
						int level = pCmd->level;

						// Set command based on level value
						if (level == 0)
						{
							what = LIGHTING_WHAT_OFF;
						}
						else if (level == 255)
						{
							what = LIGHTING_WHAT_ON;
						}
						else
						{
							// For dimmers we only allow level 0-99

							if (level<20) {
								what = LIGHTING_WHAT_OFF;
							}
							else if (level<30) {
								what = LIGHTING_WHAT_20;
							}
							else if (level<40) {
								what = LIGHTING_WHAT_30;
							}
							else if (level<50) {
								what = LIGHTING_WHAT_40;
							}
							else if (level<60) {
								what = LIGHTING_WHAT_50;
							}
							else if (level<70) {
								what = LIGHTING_WHAT_60;
							}
							else if (level<80) {
								what = LIGHTING_WHAT_70;
							}
							else if (level<90) {
								what = LIGHTING_WHAT_80;
							}
							else if (level<100) {
								what = LIGHTING_WHAT_90;
							}
							else {
								what = LIGHTING_WHAT_100;
							}
						}
					}

				default:
					break;
			}
			break;
	
		default:
			_log.Log(LOG_STATUS, "COpenWebNetUSB unknown command: packettype=%d subtype=%d", packettype, subtype);
			return false;
	}

	int used = 1;
	if (!FindDevice(pCmd->id, pCmd->unitcode, subtype, &used)) {
		_log.Log(LOG_ERROR, "COpenWebNetUSB: command received for unknown device : %d/%s", who, whereStr.str().c_str());
		return false;
	}


	stringstream whoStr;
	stringstream whatStr;
	whoStr << who;
	whatStr << what;

	vector<bt_openwebnet> responses;
	bt_openwebnet request(whoStr.str(), whatStr.str(), whereStr.str(), "");
	if (sendCommand(request, responses))
	{
		if (responses.size() > 0)
		{
			return responses.at(0).IsOKFrame();
		}
	}

	return true;
}

/**
Find OpenWebNetDevice in DB
**/
bool COpenWebNetUSB::FindDevice(int deviceID, int deviceUnit, int subType, int* used)
{
	vector<vector<string> > result;
	
	//make device ID
	unsigned char ID1 = (unsigned char)((deviceID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((deviceID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((deviceID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)(deviceID & 0xFF);

	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);

	if (used != NULL)
	{
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d) and Used == %d",
			m_HwdID, szIdx, deviceUnit, pTypeGeneralSwitch, subType, *used);
	}
	else
	{
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
			m_HwdID, szIdx, deviceUnit, pTypeGeneralSwitch, subType);
	}

	if (result.size() > 0)
	{
		return true;
	}

	return false;
}

bool COpenWebNetUSB::writeRead(const char* command, unsigned int commandSize, bool silent)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	m_readBufferSize = 0;
	memset(m_readBuffer, 0, OPENWEBNET_SERIAL_BUFFER_SIZE);
	m_bHaveReceived = false;

	if (!isOpen()) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet writeRead error : connection not opened");
		}
		return false;
	}
	AsyncSerial::write(command, commandSize);
	while (!m_bHaveReceived) {
		sleep_milliseconds(SLEEP_READ_TIME);
	}

	return true;
}

bool COpenWebNetUSB::sendCommand(bt_openwebnet& command, vector<bt_openwebnet>& response, bool silent)
{
	m_bWriting = true;

	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "COpenWebNetUSB: Using serial port: %s", m_szSerialPort.c_str());
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
		_log.Log(LOG_ERROR, "COpenWebNetUSB: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		m_bWriting = false;
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "COpenWebNetUSB: Error opening serial port!!!");
		m_bWriting = false;
		return false;
	}

	setReadCallback(boost::bind(&COpenWebNetUSB::readCallback, this, _1, _2));
	sOnConnected(this);
	m_bIsStarted = true;

	
	if (!writeRead(OPENWEBNET_COMMAND_SESSION, strlen(OPENWEBNET_COMMAND_SESSION), silent)) {
		m_bWriting = false;
		return false;
	}

	string responseStr((const char*)m_readBuffer, m_readBufferSize);
	bt_openwebnet responseSession(responseStr);
	_log.Log(LOG_STATUS, "COpenWebNet : sent=%s received=%s", OPENWEBNET_COMMAND_SESSION, responseStr.c_str());

	bool bCheckPassword = false;
	if (!responseSession.IsOKFrame()) {
		if (!silent) {
			_log.Log(LOG_STATUS, "COpenWebNet : failed to begin session, no ACK received (%s)", responseStr.c_str());
		}
		m_bWriting = false;
		return false;
	}

	if (!writeRead(command.frame_open.c_str(), command.frame_open.length(), silent)) {
		m_bWriting = false;
		return false;
	}
	if (!silent) {
		_log.Log(LOG_STATUS, "COpenWebNet : sent=%s received=%s", command.frame_open.c_str(), m_readBuffer);
	}

	if (!ParseData((char*)m_readBuffer, m_readBufferSize, response)) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet : Cannot parse answer : %s", m_readBuffer);
		}
		m_bWriting = false;
		return false;
	}

	if (!response.empty() && response[0].IsPwdFrame()) {
		_log.Log(LOG_STATUS, "COpenWebNet : password required, you have to configure your gateway security settings to allow Domoticz");
	}

	m_bWriting = false;
	return true;
}

bool COpenWebNetUSB::ParseData(char* data, int length, vector<bt_openwebnet>& messages)
{
	string buffer = string(data, length);
	size_t begin = 0;
	size_t end = string::npos;
	do {
		end = buffer.find(OPENWEBNET_END_FRAME, begin);
		if (end != string::npos) {
			bt_openwebnet message(buffer.substr(begin, end - begin + 2));
			messages.push_back(message);
			begin = end + 2;
		}
	} while (end != string::npos);

	return true;
}

void COpenWebNetUSB::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	if (!m_bIsStarted)
		return;

	if (m_bEnableReceive)
	{
		if (len < OPENWEBNET_SERIAL_BUFFER_SIZE) {
			memcpy(m_readBuffer, data, len);
			m_readBufferSize = len;
		}
		else {
			memcpy(m_readBuffer, data, OPENWEBNET_SERIAL_BUFFER_SIZE);
			m_readBufferSize = OPENWEBNET_SERIAL_BUFFER_SIZE;
		}
	}

	m_bHaveReceived = true;
}
