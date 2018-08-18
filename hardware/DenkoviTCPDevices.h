#pragma once

#include "DomoticzHardware.h"
#include "ASyncTCP.h"
#include <iosfwd>

enum _eDenkoviTCPDevice
{
	DDEV_WIFI_16R = 0,						//0
	DDEV_WIFI_16R_Modbus,						//1
	DDEV_DAEnet_IP401
};

#define DMODBUS_READ_COILS						1
#define DMODBUS_WRITE_SINGLE_COIL				5
#define DMODBUS_WRITE_MULTIPLE_COILS			15
#define DMODBUS_READ_REGISTERS					3
#define DMODBUS_WRITE_SINGLE_REGISTER			6
#define DMODBUS_WRITE_MULTIPLE_REGISTERS		16

struct _sDenkoviTCPModbusRequest {
	uint8_t trId[2] = {0,0};//transaction ID
	uint8_t prId[2] = {0,0};//protocol ID
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

/*struct _sDenkoviTCPModbusRequest {
	uint16_t trId = 0;//transaction ID
	uint16_t prId = 0;//protocol ID
	uint16_t length = 6;//message length
	uint8_t unitId = 1;//unit ID
	uint8_t fc = 1;//function code
	uint16_t address = 0;//address of first Register/Coil
	uint8_t data[100] = {16};//different data for different Function Code
};

struct _sDenkoviTCPModbusResponse {
	uint16_t trId = 0;//transaction ID
	uint16_t prId = 0;//protocol ID
	uint16_t length = 5;//message length
	uint8_t unitId = 1;//unit ID
	uint8_t fc = 1;//function code
	uint8_t dataLength = 2;//data buffer length
	uint8_t data[100] = { 16 };//different data for different Function Code
};*/

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
	uint16_t ByteSwap(uint16_t in);
	void SwapRequestBytes();
	void SwapResponseBytes();
	void copyBuffer(const uint8_t * source, uint8_t * destination, size_t length);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	int m_pollInterval;
	int m_slaveId;
	volatile bool m_stoprequested;
	int m_iModel;
	//boost::shared_ptr<boost::thread> m_thread;
	std::shared_ptr<std::thread> m_thread;
	int m_Cmd;
	bool readingNow = false;
	bool updateIo = false;
	bool bFirstTime = true;
	//_sDenkoviTCPModbusRequest sReq;
	_sDenkoviTCPModbusRequest *pReq;// = new _sDenkoviTCPModbusRequest;// &sReq;
	//_sDenkoviTCPModbusResponse sResp;
	_sDenkoviTCPModbusResponse *pResp;// = new _sDenkoviTCPModbusResponse;// &sResp;
	uint16_t receivedDataLength = 0;
	uint16_t transactionCounter = 0;

protected:
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);
};
