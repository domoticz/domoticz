#pragma once
#include "DomoticzHardware.h"
#include "ASyncWebsocket.h"
#include <json/json.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <map>
#include <ctime>

class CMatter : public CDomoticzHardwareBase, protected ASyncWebsocket
{
public:
	CMatter(int ID, const std::string& serverAddress, uint16_t port = 5580);
	~CMatter() override;

	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	void SetSetpoint(int idx, float value);

private:
	void OnWebsocketConnected() override;
	void OnWebsocketMessage(WSOpcode opcode, const std::vector<uint8_t>& payload) override;
	void OnDisconnect() override;
	void OnError(const boost::system::error_code& error) override;

	struct EndpointState {
		std::string label;
		float  temp_C = 0;   bool hasTemp = false;
		int    hum_pct    = 0;   bool hasHum       = false;
		float  baro_hPa   = 0;   bool hasBaro      = false;
		float  setpoint_C = 0;   bool hasSetpoint  = false;
		float  lux        = 0;   bool hasLux       = false;
		double power_W    = 0;   bool hasPower     = false;
		double energy_kWh = 0;   bool hasEnergy    = false;
		float  voltage_V  = 0;   bool hasVolt      = false;
		float  current_A  = 0;   bool hasCurrent   = false;
		int  frequency_Hz = 0;   bool hasFrequency = false;
		bool   onOff      = false; bool hasOnOff    = false;
		double level_pct  = 0;   bool hasLevel     = false;
		int    levelMinLevel = 1;   // CLUSTER_LEVEL_CONTROL attr 2
		int    levelMaxLevel = 254; // CLUSTER_LEVEL_CONTROL attr 3
		bool   occupied   = false; bool hasOccupancy = false;
		bool   contact    = false; bool hasContact   = false;
		int    co2_ppm    = 0;   bool hasCO2       = false;
		float  co_ppm     = 0;   bool hasCO        = false;
		float  no2_ppm    = 0;   bool hasNO2       = false;
		float  pm25_ugm3  = 0;   bool hasPM25      = false;
		float  pm10_ugm3  = 0;   bool hasPM10      = false;
		float  flow_lpm   = 0;   bool hasFlow      = false;
		bool   locked     = false; bool hasLock     = false;
		double blind_pct  = 0;   bool hasBlind     = false;
		bool   switch_on  = false; bool hasSwitch  = false;
		int    systemMode    = 0; bool hasSystemMode    = false;
		int    ctrlSeqOp     = 0; bool hasCtrlSeqOp     = false;
		// Thermostat absolute setpoint limits (°C)
		float  minHeatSetpoint_C = 5.0f;
		float  maxHeatSetpoint_C = 30.0f;
		float  minCoolSetpoint_C = 16.0f;
		float  maxCoolSetpoint_C = 32.0f;
		// Thermostat presets (Matter 1.3 PRES feature)
		struct PresetEntry { std::string handle; int scenario; std::string name; };
		std::vector<PresetEntry> presets;
		std::string activePresetHandle; // hex string, empty = no active preset
		// ColorControl cluster (0x0300)
		int    colorMode          = -1;   // attr 8: ColorModeEnum (0=HS,1=XY,2=Temp; -1=unknown)
		int    colorHue           = 0;    // attr 0: CurrentHue (0-254 = 0-360°)
		int    colorSaturation    = 0;    // attr 1: CurrentSaturation (0-254 = 0-100%)
		float  colorX             = 0;    // attr 3: CurrentX / 65536
		float  colorY             = 0;    // attr 4: CurrentY / 65536
		int    colorTempMireds    = 370;  // attr 7: ColorTemperatureMireds
		int    colorTempMinMireds = 153;  // attr 0x400B: ColorTempPhysicalMinMireds
		int    colorTempMaxMireds = 500;  // attr 0x400C: ColorTempPhysicalMaxMireds
		bool   hasColorHS         = false;
		bool   hasColorXY         = false;
		bool   hasColorTemp       = false;
		bool   hasColorControl    = false;
		int    colorCapabilities  = -1;   // attr 0x400A; -1 = not yet received
	};

	struct NodeState {
		int nodeId = 0;
		std::string vendorName;
		uint16_t vendorId = 0;
		std::string productName;
		uint16_t productId = 0;
		std::string nodeLabel;
		std::string hardwareVersionString;
		std::string softwareVersionString;
		int      battery_pct      = 255;
		float    batteryVoltage_V = 0;  bool hasBatteryVoltage = false;
		uint64_t uptime_s      = 0;
		uint16_t threadRloc16      = 0;
		uint8_t  threadChannel     = 0;
		time_t   lastSeen          = 0;
		enum class RoutingRoleEnum : uint8_t {
			Unspecified     = 0,
			Unassigned      = 1,
			SleepyEndDevice = 2,
			EndDevice       = 3,
			REED            = 4,
			Router          = 5,
			Leader          = 6,
		};
		RoutingRoleEnum threadRoutingRole = RoutingRoleEnum::Unspecified;
		std::string threadNetworkName;
		std::string fabricLabel;       // BasicInformation attr 5 (NodeLabel)
		std::string ipAddresses;       // comma-separated, from GeneralDiagnostics attr 0 NetworkInterfaces IPv6
		struct ThreadNeighbor {
			uint64_t extAddress       = 0;
			uint32_t age              = 0;
			uint16_t rloc16           = 0;
			uint32_t linkFrameCounter = 0;
			uint32_t mleFrameCounter  = 0;
			uint8_t  lqi              = 0;
			int8_t   averageRssi      = 0;
			int8_t   lastRssi         = 0;
			uint8_t  frameErrorRate   = 0;
			uint8_t  messageErrorRate = 0;
			bool     rxOnWhenIdle     = false;
			bool     fullThreadDevice = false;
			bool     fullNetworkData  = false;
			bool     isChild          = false;
		};
		std::vector<ThreadNeighbor> threadNeighbors;
		struct RouteTableEntry { uint64_t extAddress = 0; uint16_t rloc16 = 0; bool linkEstablished = true; };
		std::vector<RouteTableEntry> threadRouteTable;
		bool available = true;
		std::map<int, EndpointState> endpoints; // endpointId -> state
	};

	std::map<int, NodeState> m_nodes; // nodeId -> state

	mutable std::mutex m_stateMutex;

	void SendCommand(const std::string& command,
	                 const Json::Value& args = Json::Value(Json::objectValue));
	void HandleServerInfo(const Json::Value& msg);
	void HandleResult(const Json::Value& msg);
	void HandleEvent(const Json::Value& msg);
	void HandleNode(const Json::Value& nodeData);
	void HandleAttributeUpdate(const Json::Value& data);
	void _DetectAndSend(int nodeId, int endpointId);
	void _DetectAndSendNode(int nodeId);
	void SendGeneralSwitchInt(int domoticzID, int unit, int battery, int value, int level,
	                          const std::string& label, _eSwitchType switchType);
	void _ApplySwitchTypeOnCreate(int domoticzID, int unit, bool wasNew, _eSwitchType switchType);
	void ApplyAttributeToState(int cluster_id, int attr_id, const Json::Value& v, EndpointState& state);
	void ApplyNodeMetadata(int cluster_id, int attr_id, const Json::Value& v, NodeState& node);
	std::string ExtractLabel(const Json::Value& endpointAttrs, int nodeId, int endpointId) const;
	void Do_Work();

	std::string  m_serverAddress;
	uint16_t     m_port;
	std::atomic<uint32_t> m_msgId{0};
	std::shared_ptr<std::thread> m_thread;
	std::atomic<time_t> m_lastPingTime{0};
public:
	std::string  m_szSoftwareVersion;
	bool   m_bWifiCredentialsSet   = false;
	bool   m_bThreadCredentialsSet = false;
	std::string m_szFabricId;
	std::string m_szCompressedFabricId;
	std::atomic<bool> m_bConnected{false};

	Json::Value GetNodesJSON(int hwdID) const;
	Json::Value GetNetworkGraphJSON() const;
	void CommissionNode(const std::string& code);
	void RemoveNode(int nodeId);
	void RefreshNodeInfo(int nodeId);
	Json::Value GetServerInfoJSON() const;
	void SetWifiCredentials(const std::string& ssid, const std::string& credentials);
	void SetThreadDataset(const std::string& dataset);
	void SetFabricLabel(const std::string& label);
};
