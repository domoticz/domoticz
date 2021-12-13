#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

enum _eDenkoviDevice
{
	DDEV_DAEnet_IP4 = 0,	    // 0
	DDEV_SmartDEN_IP_16_Relays, // 1
	DDEV_SmartDEN_IP_32_In,	    // 2
	DDEV_SmartDEN_IP_Maxi,	    // 3
	DDEV_SmartDEN_IP_Watchdog,  // 4
	DDEV_SmartDEN_Logger,	    // 5
	DDEV_SmartDEN_Notifier,	    // 6
	DDEV_DAEnet_IP3,	    // 7
	DDEV_DAEnet_IP2,	    // 8
	DDEV_DAEnet_IP2_8_RELAYS,   // 9
	DDEV_SmartDEN_Opener,	    // 10
	DDEV_SmartDEN_PLC,	    // 11
	DDEV_SmartDEN_IP_16_R_MT,   // 12
	DDEV_SmartDEN_IP_16_R_MQ    // 13
};

class CDenkoviDevices : public CDomoticzHardwareBase
{
      public:
	CDenkoviDevices(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &password, int pollInterval, int model);
	~CDenkoviDevices() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
    bool dHttpGet(const std::string &url, std::string &response);

	int DenkoviGetIntParameter(std::string tmpstr, const std::string &tmpParameter);
	std::string DenkoviGetStrParameter(std::string tmpstr, const std::string &tmpParameter);
	float DenkoviGetFloatParameter(std::string tmpstr, const std::string &tmpParameter);
	int DenkoviCheckForIO(std::string tmpstr, const std::string &tmpIoType);
	std::string DAEnetIP3GetIo(const std::string &tmpstr, const std::string &tmpParameter);
	std::string DAEnetIP3GetAi(const std::string &tmpstr, const std::string &tmpParameter, const int &ciType);
	uint8_t DAEnetIP2GetIoPort(const std::string &tmpstr, const int &port);
	std::string DAEnetIP2GetName(const std::string &tmpstr, const int &nmr);
	uint16_t DAEnetIP2GetAiValue(const std::string &tmpstr, const int &aiNmr);
	float DAEnetIP2CalculateAi(int adc, const int &valType);
	void SendDenkoviTextSensor(int NodeID, int ChildID, int BatteryLevel, const std::string &textMessage, const std::string &defaultname);

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	int m_pollInterval;
	int m_iModel;
	std::shared_ptr<std::thread> m_thread;
};
