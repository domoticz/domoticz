#include "stdafx.h"
#include "DenkoviUSBDevices.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define MAX_POLL_INTERVAL 3600*1000

#define DAE_IO_TYPE_RELAY		2

CDenkoviUSBDevices::CDenkoviUSBDevices(const int ID, const std::string& comPort, const int model) :
	m_szSerialPort(comPort)
{
	m_HwdID = ID;
	m_bOutputLog = false;
	m_iModel = model;
	m_pollInterval = 1000;

	if (m_iModel == DDEV_USB_16R)
		m_baudRate = 9600;

	/*if (m_pollInterval < 500)
		m_pollInterval = 500;
	else if (m_pollInterval > MAX_POLL_INTERVAL)
		m_pollInterval = MAX_POLL_INTERVAL;*/
	Init();
}

void CDenkoviUSBDevices::Init()
{
}

bool CDenkoviUSBDevices::StartHardware()
{
	RequestStart();

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

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });

	m_bIsStarted = true;
	setReadCallback([this](auto d, auto l) { readCallBack(d, l); });

	sOnConnected(this);
	return true;
}

void CDenkoviUSBDevices::readCallBack(const char * data, size_t len)
{
	if (!m_bEnableReceive) {
		m_readingNow = false;
		return; //receiving not enabled
	}
	switch (m_iModel) {
	case DDEV_USB_16R:
		if (m_Cmd == _edaeUsbState::DAE_USB16_ASK_CMD) {
			uint8_t firstEight, secondEight;
			if (len == 2) {
				firstEight = (unsigned char)data[0];
				secondEight = (unsigned char)data[1];
			}
			else {
				_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Response error.");
				return;
			}
			for (uint8_t ii = 1; ii < 9; ii++) {
				//z = (firstEight >> (8 - ii)) & 0x01;
				SendSwitch(DAE_IO_TYPE_RELAY, ii, 255, (((firstEight >> (8 - ii)) & 0x01) != 0) ? true : false, 0, "Relay " + std::to_string(ii), m_Name);
			}
			for (uint8_t ii = 1; ii < 9; ii++)
				SendSwitch(DAE_IO_TYPE_RELAY, ii + 8, 255, ((secondEight >> (8 - ii) & 0x01) != 0) ? true : false, 0, "Relay " + std::to_string(8 + ii), m_Name);
		} 
		break;
	}
	m_updateIo = false;
	m_readingNow = false;
}

void CDenkoviUSBDevices::OnError(const std::exception &e)
{
	_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Error: %s", e.what());
}

bool CDenkoviUSBDevices::StopHardware()
{
	if (m_thread != nullptr)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CDenkoviUSBDevices::Do_Work()
{
	//int poll_interval = m_pollInterval / 100;
	//int poll_counter = poll_interval - 2;

	int msec_counter = 0;

	_log.Log(LOG_STATUS, "Denkovi: Worker started...");

	while (!IsStopRequested(100))
	{
		m_LastHeartbeat = mytime(nullptr);
		if (msec_counter++ >= 40) {
			msec_counter = 0;
			if (m_readingNow == false && m_updateIo == false)
			{
				//Every 4 seconds
				GetMeterDetails();
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS, "Denkovi: Worker stopped...");
}

bool CDenkoviUSBDevices::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	m_updateIo = true;
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	int ioType = pSen->LIGHTING2.id4;
	int io = pSen->LIGHTING2.unitcode;
	uint8_t command = pSen->LIGHTING2.cmnd;

	if (m_bIsStarted == false)
		return false;

	switch (m_iModel) {
	case DDEV_USB_16R: {
		std::stringstream szCmd;
		//int ioType = pSen->id;
		if (ioType != DAE_IO_TYPE_RELAY)
		{
			_log.Log(LOG_ERROR, "USB 16 Relays-VCP: Not a valid Relay");
			return false;
		}
		//int io = pSen->unitcode;//Relay1 to Relay16
		if (io > 16)
			return false;

		szCmd << (io < 10 ? "0" : "") << io;
		if (command == light2_sOff)
			szCmd << "-//";
		else if (command == light2_sOn)
			szCmd << "+//";
		m_Cmd = _edaeUsbState::DAE_USB16_UPDATE_IO;
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
		m_Cmd = _edaeUsbState::DAE_USB16_ASK_CMD;
		write("ask//");
		break;
	}
}
