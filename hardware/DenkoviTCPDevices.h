#pragma once

#include "DomoticzHardware.h"
#include "ASyncTCP.h"
#include <iosfwd>

enum _eDenkoviTCPDevice
{
	DDEV_WIFI_16R = 0,						//0
	DDEV_WIFI_16R_Modbus					//1
};

#define DMODBUS_READ_COILS						1
#define DMODBUS_WRITE_SINGLE_COIL				5
#define DMODBUS_WRITE_MULTIPLE_COILS			15
#define DMODBUS_READ_REGISTERS					3
#define DMODBUS_WRITE_SINGLE_REGISTER			6
#define DMODBUS_WRITE_MULTIPLE_REGISTERS		16

struct _sDenkoviTCPModbusRequest {
	uint8_t trId[2] = { 0,0 };//transaction ID
	uint8_t prId[2] = { 0,0 };//protocol ID
	uint8_t length[2] = { 0,6 };//message length
	uint8_t unitId = 1;//unit ID
	uint8_t fc = 1;//function code
	uint8_t address[2] = { 0,0 };//address of first Register/Coil
	uint8_t data[100] = { 16 };//different data for different Function Code
};

struct _sDenkoviTCPModbusResponse {
	uint8_t trId[2] = { 0,0 };//transaction ID
	uint8_t prId[2] = { 0,0 };//protocol ID
	uint8_t length[2] = { 0,6 };//message length
	uint8_t unitId = 1;//unit ID
	uint8_t fc = 1;//function code
	uint8_t dataLength = 2;//data buffer length
	uint8_t data[100] = { 16 };//different data for different Function Code
};

class CDenkoviTCPDevices : public CDomoticzHardwareBase, ASyncTCP
{
public:
	CDenkoviTCPDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const int pollInterval, const int model, const int slaveId);
	~CDenkoviTCPDevices(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void readCallBack(const char * data, size_t len);
	void ConvertResponse(const std::string pData, const size_t length);
	void CreateRequest(uint8_t * pData, size_t length);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	int m_pollInterval;
	int m_slaveId;
	int m_iModel;
	std::shared_ptr<std::thread> m_thread;
	int m_Cmd;
	bool m_bReadingNow = false;
	bool m_bUpdateIo = false;
	_sDenkoviTCPModbusRequest m_pReq;
	_sDenkoviTCPModbusResponse m_pResp;
	std::string m_respBuff;
	uint8_t m_reqBuff[100];
	uint16_t m_uiReceivedDataLength = 0;
	uint16_t m_uiTransactionCounter = 0;
protected:
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
};
