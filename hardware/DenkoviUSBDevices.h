#pragma once

#include "DomoticzHardware.h"
#include "ASyncSerial.h"
#include <iosfwd>

enum _eDenkoviUSBDevice
{
	DDEV_USB_16R = 0,						//0
	DDEV_USB_16R_Modbus						//1
};

class CDenkoviUSBDevices : public CDomoticzHardwareBase, AsyncSerial
{
	enum class _edaeUsbState
	{
		RESPOND_RECEIVED = 0,		//0
		DAE_USB16_UPDATE_IO,		//1
		DAE_USB16_ASK_CMD			//2
	};
public:
	CDenkoviUSBDevices(const int ID, const std::string& comPort, const int model);
	~CDenkoviUSBDevices(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
	void readCallBack(const char * data, size_t len);
private:
	std::string m_szSerialPort;
	int m_baudRate;
	int m_pollInterval;
	int m_iModel;
	std::shared_ptr<std::thread> m_thread;
	_edaeUsbState m_Cmd;
	bool m_readingNow = false;
	bool m_updateIo = false;

protected:
	void OnError(const std::exception e);
};
