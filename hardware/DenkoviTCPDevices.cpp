#include "stdafx.h"
#include "DenkoviTCPDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

#define MAX_POLL_INTERVAL 30*1000

enum _eDaeTcpState
{
	RESPOND_RECEIVED = 0,		//0
	DAE_WIFI16_UPDATE_IO,		//1
	DAE_WIFI16_ASK_CMD,			//2
	DAE_READ_COILS_CMD,			//3
	DAE_WRITE_COIL_CMD,			//4
};

#define DAE_IO_TYPE_RELAY			2

#define READ_COILS_CMD_LENGTH				11
#define WRITE_SINGLE_COIL_CMD_LENGTH		12


CDenkoviTCPDevices::CDenkoviTCPDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const int pollInterval, const int model, const int slaveId) :
	m_szIPAddress(IPAddress),
	m_pollInterval(pollInterval)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_stoprequested = false;
	m_bOutputLog = false;
	m_iModel = model;
	m_slaveId = slaveId;
	m_Cmd = 0;
	if (m_pollInterval < 500)
		m_pollInterval = 500;
	else if (m_pollInterval > MAX_POLL_INTERVAL)
		m_pollInterval = MAX_POLL_INTERVAL;
	Init();
}


CDenkoviTCPDevices::~CDenkoviTCPDevices()
{
}

void CDenkoviTCPDevices::Init()
{
}

bool CDenkoviTCPDevices::StartHardware()
{
	Init();

	m_stoprequested = false;
	m_bIsStarted = true;
	transactionCounter = 0;
	pReq = new _sDenkoviTCPModbusRequest;
	pResp = new _sDenkoviTCPModbusResponse; 
	receivedDataLength = 0;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CDenkoviTCPDevices::Do_Work, this);
	//m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDenkoviTCPDevices::Do_Work, this)));
	m_bIsStarted = true;
	switch (m_iModel) {
	case DDEV_WIFI_16R:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-VCP: Started");
		break;
	case DDEV_WIFI_16R_Modbus:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-TCP Modbus: Started");
		break;
	}
	return (m_thread != NULL);
}

void CDenkoviTCPDevices::copyBuffer(const uint8_t * source, uint8_t * destination, size_t length) {
	for (uint8_t ii = 0; ii < length; ii++) {
		destination[ii] = source[ii];
	}
}

void CDenkoviTCPDevices::OnData(const unsigned char * pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	//boost::lock_guard<boost::mutex> l(readQueueMutex);

	/*if (!m_bEnableReceive) {
		readingNow = false;
		return; //receiving not enabled
	}*/

	switch (m_iModel) {
	case DDEV_WIFI_16R: {
		if (m_Cmd == DAE_WIFI16_ASK_CMD) {
			uint8_t firstEight, secondEight;
			if (length == 2) {
				firstEight = (unsigned char)pData[0];
				secondEight = (unsigned char)pData[1];
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
				SendGeneralSwitch(DAE_IO_TYPE_RELAY, ii + 8, 255, ((secondEight >> (8 - ii) & 0x01) != 0) ? true : false, 100, "Relay " + std::to_string(8 + ii));
		}
		break;
	}
	case DDEV_WIFI_16R_Modbus: {
		copyBuffer(pData, (uint8_t *)&pResp[receivedDataLength], length);
		receivedDataLength += (uint16_t)length;

		if (m_Cmd == DAE_READ_COILS_CMD && receivedDataLength >= READ_COILS_CMD_LENGTH) {
			receivedDataLength = 0;
			if (pReq->trId[0] != pResp->trId[0] || pReq->trId[1] != pResp->trId[1]) {
				_log.Log(LOG_ERROR, "WiFi 16 Relays-TCP Modbus: Wrong Transaction ID.");
				break;
			}
			if (pResp->length[0] != 0 || pResp->length[1] != 5) {
				_log.Log(LOG_ERROR, "WiFi 16 Relays-TCP Modbus: Wrong Length of Response.");
				break;
			}
			uint8_t firstEight, secondEight;
			firstEight = (uint8_t)pResp->data[0];
			secondEight = (uint8_t)pResp->data[1];
			for (uint8_t ii = 1; ii < 9; ii++) {
				SendGeneralSwitch(DAE_IO_TYPE_RELAY, ii, 255, (((firstEight >> (ii-1)) & 0x01) != 0) ? true : false, 100, "Relay " + std::to_string(ii));
			}
			for (uint8_t ii = 1; ii < 9; ii++) {
				SendGeneralSwitch(DAE_IO_TYPE_RELAY, 8 + ii, 255, (((secondEight >> (ii - 1)) & 0x01) != 0) ? true : false, 100, "Relay " + std::to_string(8 + ii));
			}
		}
		else if (m_Cmd == DAE_WRITE_COIL_CMD && receivedDataLength >= WRITE_SINGLE_COIL_CMD_LENGTH) {
			copyBuffer(pData, (uint8_t *)&pResp[receivedDataLength], length);
			receivedDataLength = 0;
			if (pReq->trId[0] != pResp->trId[0] || pReq->trId[1] != pResp->trId[1]) {
				_log.Log(LOG_ERROR, "WiFi 16 Relays-TCP Modbus: Wrong Transaction ID.");
				break;
			}
			if (pResp->length[0] != 0 || pResp->length[1] != 6) {
				_log.Log(LOG_ERROR, "WiFi 16 Relays-TCP Modbus: Wrong Data Received.");
				break;
			}
		}
		break; 
	}
	}
	updateIo = false;
	readingNow = false;
}

void CDenkoviTCPDevices::OnConnect() {
	sOnConnected(this);
	GetMeterDetails();
}

void CDenkoviTCPDevices::OnDisconnect() {
	switch (m_iModel) {
	case DDEV_WIFI_16R:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-VCP: Disconnected");
		break;
	case DDEV_WIFI_16R_Modbus:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-TCP Modbus: Disconnected");
		break;
	}
}

void CDenkoviTCPDevices::OnError(const boost::system::error_code& error) {
	switch (m_iModel) {
	case DDEV_WIFI_16R:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-VCP: Error occured.");
		break;
	case DDEV_WIFI_16R_Modbus:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-TCP Modbus: Error occured.");
		break;
	}
}

void CDenkoviTCPDevices::OnError(const std::exception e)
{
	switch (m_iModel) {
	case DDEV_WIFI_16R:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-VCP: Error: %s", e.what());
		break;
	case DDEV_WIFI_16R_Modbus:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-TCP Modbus: Error: %s", e.what());
		break;
	}
}

bool CDenkoviTCPDevices::StopHardware()
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

void CDenkoviTCPDevices::Do_Work()
{
	int poll_interval = m_pollInterval / 100;
	int poll_counter = poll_interval - 2;

	int msec_counter = 0;

	while (!m_stoprequested)
	{
		m_LastHeartbeat = mytime(NULL);
		if (bFirstTime)
		{
			bFirstTime = false;
			if (!mIsConnected)
			{
				m_rxbufferpos = 0;
				connect(m_szIPAddress, m_usIPPort);
			}
		}
		else
		{
			sleep_milliseconds(100);
			update();
			if (msec_counter++ >= poll_interval) {
				msec_counter = 0;
				if (readingNow == false && updateIo == false)
					GetMeterDetails();
			}
		}
	}

	switch (m_iModel) {
	case DDEV_WIFI_16R:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-VCP: Worker stopped...");
		break;
	case DDEV_WIFI_16R_Modbus:
		_log.Log(LOG_STATUS, "WiFi 16 Relays-TCP Modbus: Worker stopped...");
		break;
	}
}

bool CDenkoviTCPDevices::WriteToHardware(const char *pdata, const unsigned char length)
{
	updateIo = true;
	const _tGeneralSwitch *pSen = reinterpret_cast<const _tGeneralSwitch*>(pdata);
	if (m_bIsStarted == false)
		return false;

	switch (m_iModel) {
	case DDEV_WIFI_16R: {
		std::stringstream szCmd;
		int ioType = pSen->id;
		if (ioType != DAE_IO_TYPE_RELAY)
		{
			_log.Log(LOG_ERROR, "WiFi 16 Relays-VCP: Not a valid Relay");
			return false;
		}
		int io = pSen->unitcode;//Relay1 to Relay16
		if (io > 16)
			return false;

		szCmd << (io < 10 ? "0" : "") << io;
		if (pSen->cmnd == light2_sOff)
			szCmd << "-//";
		else
			szCmd << "+//";
		m_Cmd = DAE_WIFI16_UPDATE_IO;
		write(szCmd.str());
		return true;
	}
	case DDEV_WIFI_16R_Modbus: {
		std::stringstream szCmd;
		int ioType = pSen->id;
		if (ioType != DAE_IO_TYPE_RELAY)
		{
			_log.Log(LOG_ERROR, "WiFi 16 Relays-TCP Modbus: Not a valid Relay");
			return false;
		}
		int io = pSen->unitcode;//Relay1 to Relay16
		if (io > 16)
			return false;

		pReq->prId[0] = 0;
		pReq->prId[1] = 0;
		//pReq->trId = ByteSwap(pReq->trId);
		transactionCounter++;
		pReq->trId[0] = (uint8_t)(transactionCounter >> 8);
		pReq->trId[1] = (uint8_t)(transactionCounter);
		pReq->unitId = m_slaveId;
		pReq->address[0] = 0;
		pReq->address[1] = io - 1;
		pReq->fc = DMODBUS_WRITE_SINGLE_COIL;
		pReq->length[0] = 0;
		pReq->length[1] = 6;
		if (pSen->cmnd == light2_sOff)
			pReq->data[0] = 0x00;
		else
			pReq->data[0] = 0xFF;
		pReq->data[1] = 0x00;
		size_t dataLength = pReq->length[1] + 6;
		//SwapRequestBytes();
		m_Cmd = DAE_WRITE_COIL_CMD;
		write("");
		write("");
		write("");
		write("");
		write((uint8_t*)pReq, dataLength);
		return true;
	}
	}

	_log.Log(LOG_ERROR, "Denkovi: Unknown Device!");
	return false;
}

uint16_t CDenkoviTCPDevices::ByteSwap(uint16_t in)
{	uint16_t out;
	uint8_t *indata = (uint8_t *)&in;
	uint8_t *outdata = (uint8_t *)&out;
	outdata[0] = indata[1];
	outdata[1] = indata[0];
	return out;
}

void CDenkoviTCPDevices::SwapRequestBytes()
{
	/*uint16_t tmpVar;
	tmpVar = ((pReq->trId & 0x00ff) << 8) | ((pReq->trId & 0xff00) >> 8);
	pReq->trId = tmpVar;
	tmpVar = ((pReq->length & 0x00ff) << 8) | ((pReq->length & 0xff00) >> 8);
	pReq->length = tmpVar;
	tmpVar = ((pReq->address & 0x00ff) << 8) | ((pReq->address & 0xff00) >> 8);
	pReq->address = tmpVar;*/
	//pReq->length = ByteSwap(pReq->length);
	//pReq->trId = ByteSwap(pReq->trId);
	//pReq->address = ByteSwap(pReq->address);
}

void CDenkoviTCPDevices::SwapResponseBytes()
{
	/*uint16_t tmpVar;
	tmpVar = ((pResp->trId & 0x00ff) << 8) | ((pResp->trId & 0xff00) >> 8);
	pResp->trId = tmpVar;
	tmpVar = ((pResp->length & 0x00ff) << 8) | ((pResp->length & 0xff00) >> 8);
	pResp->length = tmpVar;*/
	//pResp->trId = ByteSwap(pResp->trId);
	//pResp->length = ByteSwap(pResp->length);
}

void CDenkoviTCPDevices::GetMeterDetails()
{
	switch (m_iModel) {
	case DDEV_WIFI_16R: {
		m_Cmd = DAE_WIFI16_ASK_CMD;
		write("ask//");
		break;
	}
	case DDEV_WIFI_16R_Modbus: {
		m_Cmd = DAE_READ_COILS_CMD;
		pReq->prId[0] = 0;
		pReq->prId[1] = 0;
		transactionCounter++;
		pReq->trId[0] = (uint8_t)(transactionCounter >> 8);
		pReq->trId[1] = (uint8_t)(transactionCounter);
		pReq->address[0] = 0;
		pReq->address[1] = 0;
		pReq->fc = DMODBUS_READ_COILS;
		pReq->length[0] = 0;
		pReq->length[1] = 6;
		pReq->unitId = m_slaveId;
		pReq->data[0] = 0;
		pReq->data[1] = 16;
		size_t dataLength = pReq->length[1] + 6;
		//SwapRequestBytes();
		write("");
		write("");
		write("");
		write("");
		write((uint8_t*)pReq, dataLength);
		break;
	}
	}
}
