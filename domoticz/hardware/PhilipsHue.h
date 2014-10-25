#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CPhilipsHue : public CDomoticzHardwareBase
{
	enum _eHueLightType
	{
		HLTYPE_NORMAL,
		HLTYPE_DIM,
		HLTYPE_RGBW,
	};
public:
	CPhilipsHue(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username);
	~CPhilipsHue(void);
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	std::string m_IPAddress;
	unsigned short m_Port;
	std::string m_UserName;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool m_bGetInitialLights;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	bool GetLightStates();
	void SendSwitchIfNotExists(const int NodeID, const _eHueLightType LType, const int BrightnessLevel, const std::string &Name);
	bool SwitchLight(const int nodeID, const std::string &LCmd, const int svalue);
};

