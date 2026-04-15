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

private:
	void OnWebsocketConnected() override;
	void OnWebsocketMessage(WSOpcode opcode, const std::vector<uint8_t>& payload) override;
	void OnDisconnect() override;
	void OnError(const boost::system::error_code& error) override;

	struct EndpointState {
		std::string label;
		float  temp_C     = 0;   bool hasTemp      = false;
		int    hum_pct    = 0;   bool hasHum       = false;
		float  baro_hPa   = 0;   bool hasBaro      = false;
		float  lux        = 0;   bool hasLux       = false;
		double power_W    = 0;   bool hasPower     = false;
		double energy_kWh = 0;   bool hasEnergy    = false;
		float  voltage_V  = 0;   bool hasVolt      = false;
		float  current_A  = 0;   bool hasCurrent   = false;
		bool   onOff      = false; bool hasOnOff    = false;
		double level_pct  = 0;   bool hasLevel     = false;
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
		int    battery_pct = 255;
	};
	std::map<int, EndpointState> m_endpointStates;
	mutable std::mutex m_stateMutex;

	void SendCommand(const std::string& command,
	                 const Json::Value& args = Json::Value(Json::objectValue));
	void HandleServerInfo(const Json::Value& msg);
	void HandleResult(const Json::Value& msg);
	void HandleEvent(const Json::Value& msg);
	void HandleNode(const Json::Value& nodeData);
	void HandleAttributeUpdate(const Json::Value& data);
	void _DetectAndSend(int domoticzID);
	void ApplyAttributeToState(int cluster_id, int attr_id, const Json::Value& v, EndpointState& state);
	std::string ExtractLabel(const Json::Value& endpointAttrs, int nodeId, int endpointId) const;
	void Do_Work();

	std::string  m_serverAddress;
	uint16_t     m_port;
	std::atomic<uint32_t> m_msgId{0};
	std::shared_ptr<std::thread> m_thread;
	std::atomic<bool> m_bConnected{false};
	time_t m_lastPingTime = 0;
};
