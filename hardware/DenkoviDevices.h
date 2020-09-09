#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

enum _eDenkoviDevice
{
	DDEV_DAEnet_IP4 = 0,							//0
	DDEV_SmartDEN_IP_16_Relays,						//1
	DDEV_SmartDEN_IP_32_In,							//2
	DDEV_SmartDEN_IP_Maxi,							//3
	DDEV_SmartDEN_IP_Watchdog,						//4
	DDEV_SmartDEN_Logger,							//5
	DDEV_SmartDEN_Notifier,							//6
	DDEV_DAEnet_IP3,								//7
	DDEV_DAEnet_IP2,								//8
	DDEV_DAEnet_IP2_8_RELAYS,						//9
	DDEV_SmartDEN_Opener,							//10
	DDEV_SmartDEN_PLC,								//11
	DDEV_SmartDEN_IP_16_R_MT,						//12
	DDEV_SmartDEN_IP_16_R_MQ						//13
};

class CDenkoviDevices : public CDomoticzHardwareBase
{
public:
	CDenkoviDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password, const int pollInterval, const int model);
	~CDenkoviDevices(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

	int DenkoviGetIntParameter(std::string tmpstr, const std::string &tmpParameter);
	std::string DenkoviGetStrParameter(std::string tmpstr, const std::string &tmpParameter);
	float DenkoviGetFloatParameter(std::string tmpstr, const std::string &tmpParameter);
	int DenkoviCheckForIO(std::string tmpstr, const std::string &tmpIoType);
	std::string DAEnetIP3GetIo(std::string tmpstr, const std::string &tmpParameter);
	std::string DAEnetIP3GetAi(std::string tmpstr, const std::string &tmpParameter, const int &ciType);
	uint8_t DAEnetIP2GetIoPort(std::string tmpstr, const int &port);
	std::string DAEnetIP2GetName(std::string tmpstr, const int &nmr);
	uint16_t DAEnetIP2GetAiValue(std::string tmpstr, const int &aiNmr);
	float DAEnetIP2CalculateAi(int adc, const int &valType);
	void SendDenkoviTextSensor(const int NodeID, const int ChildID, const int BatteryLevel, const std::string &textMessage, const std::string &defaultname);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	int m_pollInterval;
	int m_iModel; 
	std::shared_ptr<std::thread> m_thread; 
	const char *szDenkoviHardwareNames[14] = {
		"DAEnetIP4", 
		"smartDEN IP-16R", 
		"smartDEN IP-32IN",
		"smartDEN IP-Maxi",
		"smartDEN IP-Watchdog",
		"smartDEN Logger",
		"smartDEN Notifier",
		"DAEnetIP3",
		"DAEnetIP2 (DAEnetIP2 v2)",
		"DAEnetIP2 (DAEnetIP2 v2) 8 Relay Module - LM35DZ",
		"smartDEN Opener",
		"smartDEN IP-PLC",
		"smartDEN IP-16R-MT",
		"smartDEN IP-16R-MQ"
		};  

};
