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

#define MAX_POLL_INTERVAL 3600*1000

#define DAE_IO_TYPE_RELAY			2

#define READ_COILS_CMD_LENGTH				11
#define WRITE_SINGLE_COIL_CMD_LENGTH		12

namespace
{
	constexpr std::array<const char *, 3> szDenkoviHardwareNamesTCP{
		"WiFi 16 Relays-VCP",	     //
		"WiFi 16 Relays-TCP Modbus", //
		"smartDEN IP-16R-MT",	     //
	};
} // namespace

CDenkoviTCPDevices::CDenkoviTCPDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const int pollInterval, const int model, const int slaveId) :
	m_szIPAddress(IPAddress),
	m_pollInterval(pollInterval)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_bOutputLog = false;
	m_iModel = model;
	m_slaveId = slaveId;
	m_Cmd = _eDaeTcpState::RESPOND_RECEIVED;
	if (m_pollInterval < 500)
		m_pollInterval = 500;
	else if (m_pollInterval > MAX_POLL_INTERVAL)
		m_pollInterval = MAX_POLL_INTERVAL;
	Init();
}

void CDenkoviTCPDevices::Init()
{
}

bool CDenkoviTCPDevices::StartHardware()
{
	RequestStart();

	Init();

	m_bIsStarted = true;
	m_uiTransactionCounter = 0;
	m_uiReceivedDataLength = 0;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	m_bIsStarted = true;
	Log(LOG_STATUS, "%s: Started.",szDenkoviHardwareNamesTCP[m_iModel]);
	return (m_thread != nullptr);
}

void CDenkoviTCPDevices::ConvertResponse(const std::string &pData, const size_t length)
{
	m_pResp.trId[0] = pData[0];
	m_pResp.trId[1] = pData[1];
	m_pResp.prId[0] = pData[2];
	m_pResp.prId[1] = pData[3];
	m_pResp.length[0] = pData[4];
	m_pResp.length[1] = pData[5];
	m_pResp.unitId = pData[6];
	m_pResp.fc = pData[7];
	m_pResp.dataLength = pData[8];
	for (uint8_t ii = 9; ii < length; ii++)
		m_pResp.data[ii - 9] = pData[ii];

}

void CDenkoviTCPDevices::CreateRequest(uint8_t * pData, size_t length)
{
	pData[0] = m_pReq.trId[0];
	pData[1] = m_pReq.trId[1];
	pData[2] = m_pReq.prId[0];
	pData[3] = m_pReq.prId[1];
	pData[4] = m_pReq.length[0];
	pData[5] = m_pReq.length[1];
	pData[6] = m_pReq.unitId;
	pData[7] = m_pReq.fc;
	pData[8] = m_pReq.address[0];
	pData[9] = m_pReq.address[1];
	for (size_t ii = 10; ii < length; ii++)
		pData[ii] = m_pReq.data[ii - 10];

}
 
void CDenkoviTCPDevices::OnData(const unsigned char * pData, size_t length)
{
	switch (m_iModel) {
	case DDEV_WIFI_16R: {
		if (m_Cmd == _eDaeTcpState::DAE_WIFI16_ASK_CMD) {
			uint8_t firstEight, secondEight;
			if (length == 2) {
				firstEight = (unsigned char)pData[0];
				secondEight = (unsigned char)pData[1];
			}
			else {
				Log(LOG_ERROR, "%s: Response error!",szDenkoviHardwareNamesTCP[m_iModel]);
				return;
			}
			uint8_t z = 0;
			for (uint8_t ii = 1; ii < 9; ii++) {
				z = (firstEight >> (8 - ii)) & 0x01;
				SendSwitch(DAE_IO_TYPE_RELAY, ii, 255, (((firstEight >> (8 - ii)) & 0x01) != 0) ? true : false, 0, "Relay " + std::to_string(ii), m_Name);
			}
			for (uint8_t ii = 1; ii < 9; ii++)
				SendSwitch(DAE_IO_TYPE_RELAY, ii + 8, 255, ((secondEight >> (8 - ii) & 0x01) != 0) ? true : false, 0, "Relay " + std::to_string(8 + ii), m_Name);
		}
		break;
	}
	case DDEV_SmartDEN_IP_16_R_MT_MODBUS:
	case DDEV_WIFI_16R_Modbus: {
		m_respBuff.append((const char * )pData, length);
		m_uiReceivedDataLength += (uint16_t)length;

		if (m_Cmd == _eDaeTcpState::DAE_READ_COILS_CMD && m_uiReceivedDataLength >= READ_COILS_CMD_LENGTH) {
			ConvertResponse(m_respBuff, m_uiReceivedDataLength);
			m_respBuff.clear();
			m_uiReceivedDataLength = 0;
			if (m_pReq.trId[0] != m_pResp.trId[0] || m_pReq.trId[1] != m_pResp.trId[1]) {
				Log(LOG_ERROR, "%s: Wrong Transaction ID!",szDenkoviHardwareNamesTCP[m_iModel]);
				break;
			}
			if (m_pResp.length[0] != 0 || m_pResp.length[1] != 5) {
				Log(LOG_ERROR, "%s: Wrong Length of Response!",szDenkoviHardwareNamesTCP[m_iModel]);
				break;
			}
			uint8_t firstEight, secondEight;
			firstEight = (uint8_t)m_pResp.data[0];
			secondEight = (uint8_t)m_pResp.data[1];
			for (uint8_t ii = 1; ii < 9; ii++) {
				SendSwitch(DAE_IO_TYPE_RELAY, ii, 255, (((firstEight >> (ii - 1)) & 0x01) != 0) ? true : false, 0, "Relay " + std::to_string(ii), m_Name);
			}
			for (uint8_t ii = 1; ii < 9; ii++) {
				SendSwitch(DAE_IO_TYPE_RELAY, 8 + ii, 255, (((secondEight >> (ii - 1)) & 0x01) != 0) ? true : false, 0, "Relay " + std::to_string(8 + ii), m_Name);
			}
		}
		else if (m_Cmd == _eDaeTcpState::DAE_WRITE_COIL_CMD && m_uiReceivedDataLength >= WRITE_SINGLE_COIL_CMD_LENGTH) {
			ConvertResponse(m_respBuff, m_uiReceivedDataLength);
			m_respBuff.clear();
			m_uiReceivedDataLength = 0;
			if (m_pReq.trId[0] != m_pResp.trId[0] || m_pReq.trId[1] != m_pResp.trId[1]) {
				Log(LOG_ERROR, "%s: Wrong Transaction ID!",szDenkoviHardwareNamesTCP[m_iModel]);
				break;
			}
			if (m_pResp.length[0] != 0 || m_pResp.length[1] != 6) {
				Log(LOG_ERROR, "%s: Wrong Data Received!",szDenkoviHardwareNamesTCP[m_iModel]);
				break;
			}
		}
		break;
	}
	}
	m_bUpdateIo = false;
	m_bReadingNow = false;
}

void CDenkoviTCPDevices::OnConnect() {
	sOnConnected(this);
	GetMeterDetails();
}

void CDenkoviTCPDevices::OnDisconnect() {
	Log(LOG_STATUS, "%s: Disconnected.",szDenkoviHardwareNamesTCP[m_iModel]);	  
}

void CDenkoviTCPDevices::OnError(const boost::system::error_code& /*error*/) {
	Log(LOG_ERROR, "%s: Error occured!",szDenkoviHardwareNamesTCP[m_iModel]);	 
}

bool CDenkoviTCPDevices::StopHardware()
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

void CDenkoviTCPDevices::Do_Work()
{
	int poll_interval = m_pollInterval / 500;
	int halfsec_counter = 0;
	connect(m_szIPAddress, m_usIPPort);
	while (!IsStopRequested(500))
	{
		halfsec_counter++;

		if (halfsec_counter % 24 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if (halfsec_counter % poll_interval == 0) {
			if (m_bReadingNow == false && m_bUpdateIo == false)
				GetMeterDetails();
		}
	}
	terminate();
	Log(LOG_STATUS, "%s: Stopped.",szDenkoviHardwareNamesTCP[m_iModel]); 
}

bool CDenkoviTCPDevices::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	m_bUpdateIo = true;
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	int ioType = pSen->LIGHTING2.id4;
	int io = pSen->LIGHTING2.unitcode;
	uint8_t command = pSen->LIGHTING2.cmnd;

	if (m_bIsStarted == false)
		return false;

	switch (m_iModel) {
	case DDEV_WIFI_16R: {
		std::stringstream szCmd;
		//int ioType = pSen->id;
		if (ioType != DAE_IO_TYPE_RELAY)
		{
			Log(LOG_ERROR, "%s: Not a valid Relay!",szDenkoviHardwareNamesTCP[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//Relay1 to Relay16
		if (io > 16)
			return false;

		szCmd << (io < 10 ? "0" : "") << io;
		if (command == light2_sOff)
			szCmd << "-//";
		else
			szCmd << "+//";
		m_Cmd = _eDaeTcpState::DAE_WIFI16_UPDATE_IO;
		write(szCmd.str());
		return true;
	}
	case DDEV_SmartDEN_IP_16_R_MT_MODBUS:
	case DDEV_WIFI_16R_Modbus: {
		std::stringstream szCmd;
		//int ioType = pSen->id;
		if (ioType != DAE_IO_TYPE_RELAY)
		{
			Log(LOG_ERROR, "%s: Not a valid Relay",szDenkoviHardwareNamesTCP[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//Relay1 to Relay16
		if (io > 16)
			return false;

		m_pReq.prId[0] = 0;
		m_pReq.prId[1] = 0;
		m_uiTransactionCounter++;
		m_pReq.trId[0] = (uint8_t)(m_uiTransactionCounter >> 8);
		m_pReq.trId[1] = (uint8_t)(m_uiTransactionCounter);
		m_pReq.unitId = (uint8_t)m_slaveId;
		m_pReq.address[0] = 0;
		m_pReq.address[1] = (uint8_t)(io - 1);
		m_pReq.fc = DMODBUS_WRITE_SINGLE_COIL;
		m_pReq.length[0] = 0;
		m_pReq.length[1] = 6;
		if (command == light2_sOff)
			m_pReq.data[0] = 0x00;
		else
			m_pReq.data[0] = 0xFF;
		m_pReq.data[1] = 0x00;
		size_t dataLength = m_pReq.length[1] + 6;
		CreateRequest(m_reqBuff,dataLength);
		m_Cmd = _eDaeTcpState::DAE_WRITE_COIL_CMD;
		write("");
		write("");
		write("");
		write("");
		write(m_reqBuff, dataLength);
		return true;
	}
	}

	Log(LOG_ERROR, "Unknown Device!");
	return false;
}

void CDenkoviTCPDevices::GetMeterDetails()
{
	switch (m_iModel) {
	case DDEV_WIFI_16R: {
		m_Cmd = _eDaeTcpState::DAE_WIFI16_ASK_CMD;
		write("ask//");
		break;
	}
	case DDEV_SmartDEN_IP_16_R_MT_MODBUS:
	case DDEV_WIFI_16R_Modbus: {
		m_Cmd = _eDaeTcpState::DAE_READ_COILS_CMD;
		m_pReq.prId[0] = 0;
		m_pReq.prId[1] = 0;
		m_uiTransactionCounter++;
		m_pReq.trId[0] = (uint8_t)(m_uiTransactionCounter >> 8);
		m_pReq.trId[1] = (uint8_t)(m_uiTransactionCounter);
		m_pReq.address[0] = 0;
		m_pReq.address[1] = 0;
		m_pReq.fc = DMODBUS_READ_COILS;
		m_pReq.length[0] = 0;
		m_pReq.length[1] = 6;
		m_pReq.unitId = (uint8_t)m_slaveId;
		m_pReq.data[0] = 0;
		m_pReq.data[1] = 16;
		size_t dataLength = m_pReq.length[1] + 6;
		CreateRequest(m_reqBuff, dataLength);
		write("");
		write("");
		write("");
		write("");
		write(m_reqBuff, dataLength);
		break;
	}
	}
}
