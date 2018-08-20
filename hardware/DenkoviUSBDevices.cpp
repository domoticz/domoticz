#include "stdafx.h"
#include "DenkoviUSBDevices.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

#define MAX_POLL_INTERVAL 30*1000

enum _edaeUsbState
{
	RESPOND_RECEIVED = 0,		//0
	DAE_USB16_UPDATE_IO,					//1
	DAE_USB16_ASK_CMD				//2
};

#define DAE_IO_TYPE_RELAY		2

CDenkoviUSBDevices::CDenkoviUSBDevices(const int ID, const std::string& comPort, const int model) :
	m_szSerialPort(comPort)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_bOutputLog = false;
	m_iModel = model;
	m_pollInterval = 1000;
	/*if (m_pollInterval < 500)
		m_pollInterval = 500;
	else if (m_pollInterval > MAX_POLL_INTERVAL)
		m_pollInterval = MAX_POLL_INTERVAL;*/
	Init();
}


CDenkoviUSBDevices::~CDenkoviUSBDevices()
{
}

void CDenkoviUSBDevices::Init()
{
}

bool CDenkoviUSBDevices::StartHardware()
{
	m_stoprequested = false;
	m_thread = std::make_shared<std::thread>(&CDenkoviUSBDevices::Do_Work, this);
	//m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDenkoviUSBDevices::Do_Work, this)));

	if (m_iModel == DDEV_USB_16R)
		m_baudRate = 9600;

	//Try to open the Serial Port
	try
	{
		open(
			m_szSerialPort,
			m_baudRate,
			boost::asio::serial_port_base::parity(
				boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8),
			boost::asio::serial_port_base::flow_control(
			boost::asio::serial_port_base::flow_control::none),
			boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one)
		);
	}
	catch (boost::exception & e)
	{
		if (m_iModel == DDEV_USB_16R)
			_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Error opening serial port!");
		(void)e;
		return false;
	}
	catch (...)
	{
		if (m_iModel == DDEV_USB_16R)
			_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Error opening serial port!");
		return false;
	}
	m_bIsStarted = true;
	setReadCallback(boost::bind(&CDenkoviUSBDevices::readCallBack, this, _1, _2));
	
	sOnConnected(this);
	return true;
}

void CDenkoviUSBDevices::readCallBack(const char * data, size_t len)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	//boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive) {
		m_readingNow = false;
		return; //receiving not enabled
	}
	uint8_t tmp = (unsigned char)data[0];

	switch (m_iModel) {
	case DDEV_USB_16R:
		if (m_Cmd == DAE_USB16_ASK_CMD) {
			uint8_t firstEight, secondEight;
			if (len == 2) {
				firstEight = (unsigned char)data[0];
				secondEight = (unsigned char)data[1];
			}
			else {
				_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Response error.");
				return;
			}
			uint8_t z = 0;
			for (uint8_t ii = 1; ii < 9; ii++) {
				z = (firstEight >> (8 - ii)) & 0x01;
				SendGeneralSwitch(DAE_IO_TYPE_RELAY, ii, 255, (((firstEight >> (8 - ii)) & 0x01) != 0) ? true : false, 100, "Relay " + std::to_string(ii));
			}
			for (uint8_t ii = 1; ii < 9; ii++)
				SendGeneralSwitch(DAE_IO_TYPE_RELAY, ii + 8, 255, ((secondEight >> (8 - ii) & 0x01) != 0) ? true : false, 100, "Relay " + std::to_string(8+ii));
		} 
		break;
	}
	m_updateIo = false;
	m_readingNow = false;
}


void CDenkoviUSBDevices::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	//boost::lock_guard<boost::mutex> l(readQueueMutex);
	uint8_t firstEight, secondEight;
	if (length == 2) {
		firstEight = (unsigned char)pData[0];
		secondEight = (unsigned char)pData[1];
	}
}

void CDenkoviUSBDevices::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Error: %s", e.what());
}

bool CDenkoviUSBDevices::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted = false;
	return true;
}

void CDenkoviUSBDevices::Do_Work()
{
	int poll_interval = m_pollInterval / 100;
	int poll_counter = poll_interval - 2;

	int msec_counter = 0;

	while (!m_stoprequested)
	{
		m_LastHeartbeat = mytime(NULL);
		sleep_milliseconds(40);
		if (msec_counter++ >= 100) {
			msec_counter = 0;
			if (m_readingNow == false && m_updateIo == false)
				GetMeterDetails();
		}
	}

	switch (m_iModel) {
	case DDEV_USB_16R:
		_log.Log(LOG_STATUS, "USB 16 Relays-VCP: Worker stopped...");
		break;
	}
}

bool CDenkoviUSBDevices::WriteToHardware(const char *pdata, const unsigned char length)
{
	m_updateIo = true;
	const _tGeneralSwitch *pSen = reinterpret_cast<const _tGeneralSwitch*>(pdata);
	if (m_bIsStarted == false)
		return false;

	switch (m_iModel) {
	case DDEV_USB_16R: {
		std::stringstream szCmd;
		int ioType = pSen->id;
		if (ioType != DAE_IO_TYPE_RELAY)
		{
			_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Not a valid Relay");
			return false;
		}
		int io = pSen->unitcode;//Relay1 to Relay16
		if (io > 16)
			return false;

		szCmd << (io < 10 ? "0" : "") << io;
		if (pSen->cmnd == light2_sOff)
			szCmd << "-//";
		else if (pSen->cmnd == light2_sOn)
			szCmd << "+//";
		m_Cmd = DAE_USB16_UPDATE_IO;
		write(szCmd.str());
		return true;
	}
	}
	_log.Log(LOG_ERROR, "Denkovi: Unknown Device!");
	return false;
}

void CDenkoviUSBDevices::GetMeterDetails()
{
	m_readingNow = true;
	std::string sResult, sResult2;
	std::stringstream szURL, szURL2;

	switch (m_iModel) {
	case DDEV_USB_16R:
		m_Cmd = DAE_USB16_ASK_CMD;
		write("ask//");
		break;
	}
}
