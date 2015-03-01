#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include <map>

class CPhilipsHue : public CDomoticzHardwareBase
{
	enum _eHueLightType
	{
		HLTYPE_NORMAL,
		HLTYPE_DIM,
		HLTYPE_RGBW,
	};
	struct _tHueLight
	{
		int cmd;
		int level;
		int sat;
		int hue;
	};
public:
	CPhilipsHue(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username);
	~CPhilipsHue(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	static std::string RegisterUser(const std::string &IPAddress, const unsigned short Port, const std::string &username);
private:
	std::string m_IPAddress;
	unsigned short m_Port;
	std::string m_UserName;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::map<int, _tHueLight> m_lights;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	bool GetLightStates();
	void InsertUpdateSwitch(const int NodeID, const _eHueLightType LType, const bool bIsOn, const int BrightnessLevel, const int Sat, const int Hue, const std::string &Name);
	bool SwitchLight(const int nodeID, const std::string &LCmd, const int svalue);
};

