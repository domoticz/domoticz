#pragma once

// implememtation for devices : http://blebox.eu/
// by Fantom (szczukot@poczta.onet.pl)

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"

namespace Json
{
	class Value;
} // namespace Json


struct STR_DEVICE {
	uint8_t			bleBoxTypeId;
	const char* api_name;
	const char* name;
	uint8_t			rfxType;
	uint8_t		subType;
	int			switchType;
	const char* api_state;
};

const std::array<STR_DEVICE, 9> DevicesType{
	{
		{ 0, "switchBox", "Switch Box", pTypeLighting2, sTypeAC, STYPE_OnOff, "relay" },
		{ 1, "shutterBox", "Shutter Box", pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, "shutter" },
		{ 2, "wLightBoxS", "Light Box S", pTypeLighting2, sTypeAC, STYPE_Dimmer, "light" },
		{ 3, "wLightBox", "Light Box", pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, "rgbw" },
		{ 4, "gateBox", "Gate Box", pTypeGeneral, sTypePercentage, 0, "gate" },
		{ 5, "dimmerBox", "Dimmer Box", pTypeLighting2, sTypeAC, STYPE_Dimmer, "dimmer" },
		{ 6, "switchBoxD", "Switch Box D", pTypeLighting2, sTypeAC, STYPE_OnOff, "relay" },
		{ 7, "airSensor", "Air Sensor", pTypeAirQuality, sTypeVoc, 0, "air" },
		{ 8, "tempSensor", "Temp Sensor", pTypeTEMP, sTypeTEMP5, 0, "tempsensor" },
	},
};

class BleBox : public CDomoticzHardwareBase
{
public:
	BleBox(int id, int pollIntervalsec, int showRSSI);
	~BleBox() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	int GetDeviceType(const std::string& IPAddress);
	void AddNode(const std::string& name, const std::string& IPAddress, bool reloadNodes);
	void RemoveNode(int id);
	void RemoveAllNodes();
	void SetSettings(int pollIntervalsec, const int showRSSI);
	void UpdateFirmware();
	Json::Value GetApiDeviceState(const std::string& IPAddress);
	bool DoesNodeExists(const Json::Value& root, const std::string& node);
	bool DoesNodeExists(const Json::Value& root, const std::string& node, const std::string& value);
	std::string GetUptime(const std::string& IPAddress);
	int GetRSSI(const std::string& IPAddress);
	void SearchNodes(const std::string& pattern);

private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::pair<std::string, std::string> IdentifyDevice(const std::string& IPAddress);
	uint8_t GetDeviceTypeByApiName(const std::string& apiName);
	std::string GetDeviceIP(const tRBUF* id);
	std::string GetDeviceRevertIP(const tRBUF* id);
	Json::Value SendCommand(const std::string& IPAddress, const std::string& command, int timeOut = 3);
	void GetDevicesState();

	void SendSwitch(int NodeID, uint8_t ChildID, int BatteryLevel, bool bOn, double Level, const std::string& defaultname, const BYTE rssi);

	void ReloadNodes();
	void UnloadNodes();
	bool LoadNodes();

private:
	int m_PollInterval;
	int m_ShowRSSI; // 0-none, 1-on hardware page, 2-always
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;

	struct _tBleBoxDevice
	{
		uint8_t bleBoxTypeId;
		std::string addressIP;
	};

	std::vector<_tBleBoxDevice> m_devices;

	_tColor m_RGBWColorState;
	bool m_RGBWisWhiteState{ true };
	int m_RGBWbrightnessState{ 255 };

	int IPToInt(const std::string& IPAddress);
	std::pair<std::string, uint8_t> CodeID(const int id, const uint8_t type);
	bool PrepareHostList(const std::string& pattern, std::vector<std::string>& hosts);
	int RSSIToQuality(const int rssi);
};
