#pragma once

#include "../DomoticzHardware.h"
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
	enum _eHueColorMode
	{
		HLMODE_NONE,
		HLMODE_HS,
		HLMODE_CT,
		HLMODE_XY
	};
	struct _tHueLightState
	{
		bool on;
		_eHueColorMode mode;
		int level;
		int hue;
		int sat;
		int ct;
		double x;
		double y;
	};
	struct _tHueGroup
	{
		_tHueLightState gstate;
		std::vector<int> lights;
	};
	struct _tHueScene
	{
		std::string id;
		std::string name;
		std::string lastupdated;
	};

public:
	CPhilipsHue(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username, const int poll, const int Options);
	~CPhilipsHue(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	static std::string RegisterUser(const std::string &IPAddress, const unsigned short Port, const std::string &username);
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	bool GetStates();
	bool GetLights(const Json::Value &root);
	bool GetGroups(const Json::Value &root);
	bool GetScenes(const Json::Value &root);
	bool GetSensors(const Json::Value &root);
	void InsertUpdateSwitch(const int NodeID, const _eHueLightType LType, const _tHueLightState tstate, const std::string &Name, const std::string &Options, const std::string &modelid, const bool AddMissingDevice);
	void InsertUpdateSwitch(const int NodeID, const _eSwitchType SType, const bool bIsOn, const std::string &Name, const uint8_t BatteryLevel);
	bool SwitchLight(const int nodeID, const std::string &LCmd, const int svalue, const int svalue2 = 0, const int svalue3 = 0);
	static void LightStateFromJSON(const Json::Value &lightstate, _tHueLightState &tlight, _eHueLightType &LType);
	static void RgbFromXY(const double x, const double y, const double bri, const std::string &modelid, uint8_t &r8, uint8_t &g8, uint8_t &b8);
	static bool StatesSimilar(const _tHueLightState &s1, const _tHueLightState &s2);
private:
	int m_poll_interval;
	bool m_add_groups;
	bool m_add_scenes;
	std::string m_IPAddress;
	unsigned short m_Port;
	std::string m_UserName;
	std::shared_ptr<std::thread> m_thread;
	std::map<int, _tHueLightState> m_lights;
	std::map<int, _tHueGroup> m_groups;
	std::map<std::string, _tHueScene> m_scenes;
	std::map<int, CPHSensor> m_sensors;
};

