#pragma once

#include "../DomoticzHardware.h"
#include <map>
#include "PhilipsHueSensors.h"

namespace Json
{
	class Value;
};

class CPhilipsHue : public CDomoticzHardwareBase
{
	enum _eHueLightType
	{
		HLTYPE_NORMAL,
		HLTYPE_DIM,
		HLTYPE_RGB_W,
		HLTYPE_SCENE,
		HLTYPE_CW_WW,
		HLTYPE_RGB_CW_WW
	};
	struct _tHueLight
	{
		bool on;
		int level;
		int sat;
		int hue;
		int ct;
	};
	struct _tHueGroup
	{
		_tHueLight gstate;
		std::vector<int> lights;
	};
	struct _tHueScene
	{
		std::string id;
		std::string name;
		std::string lastupdated;
	};

public:
	CPhilipsHue(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username, const int poll);
	~CPhilipsHue(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	static std::string RegisterUser(const std::string &IPAddress, const unsigned short Port, const std::string &username);
private:
	int m_poll_interval;
	std::string m_IPAddress;
	unsigned short m_Port;
	std::string m_UserName;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::map<int, _tHueLight> m_lights;
	std::map<int, _tHueGroup> m_groups;
	std::map<std::string, _tHueScene> m_scenes;
	std::map<int, CPHSensor> m_sensors;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	bool GetStates();
	bool GetLights(const Json::Value &root);
	bool GetGroups(const Json::Value &root);
	bool GetScenes(const Json::Value &root);
	bool GetSensors(const Json::Value &root);
	void InsertUpdateSwitch(const int NodeID, const _eHueLightType LType, const bool bIsOn, const int BrightnessLevel, const int Sat, const int Hue, const std::string &Name, const std::string &Options);
	void InsertUpdateSwitch(const int NodeID, const _eSwitchType SType, const bool bIsOn, const string &Name, uint8_t BatteryLevel);
	bool SwitchLight(const int nodeID, const std::string &LCmd, const int svalue, const int svalue2 = 0, const int svalue3 = 0);
};

