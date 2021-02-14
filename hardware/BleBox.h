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

class BleBox : public CDomoticzHardwareBase
{
      public:
	BleBox(int id, int pollIntervalsec);
	~BleBox() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	int GetDeviceType(const std::string &IPAddress);
	void AddNode(const std::string &name, const std::string &IPAddress, bool reloadNodes);
	void RemoveNode(int id);
	void RemoveAllNodes();
	void SetSettings(int pollIntervalsec);
	void UpdateFirmware();
	Json::Value GetApiDeviceState(const std::string &IPAddress);
	bool DoesNodeExists(const Json::Value &root, const std::string &node);
	bool DoesNodeExists(const Json::Value &root, const std::string &node, const std::string &value);
	std::string GetUptime(const std::string &IPAddress);
	void SearchNodes(const std::string &pattern);

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::string IdentifyDevice(const std::string &IPAddress);
	int GetDeviceTypeByApiName(const std::string &apiName);
	std::string GetDeviceIP(const tRBUF *id);
	std::string GetDeviceRevertIP(const tRBUF *id);
	std::string GetDeviceIP(const std::string &id);
	std::string IPToHex(const std::string &IPAddress, int type);
	Json::Value SendCommand(const std::string &IPAddress, const std::string &command, int timeOut = 3);
	void GetDevicesState();

	void SendSwitch(int NodeID, uint8_t ChildID, int BatteryLevel, bool bOn, double Level, const std::string &defaultname);

	void ReloadNodes();
	void UnloadNodes();
	bool LoadNodes();

      private:
	int m_PollInterval;
	std::shared_ptr<std::thread> m_thread;
	std::map<const std::string, const int> m_devices;
	std::mutex m_mutex;

	_tColor m_RGBWColorState;
	bool m_RGBWisWhiteState;
	int m_RGBWbrightnessState;

	bool PrepareHostList(const std::string &pattern, std::vector<std::string> &hosts);
};
