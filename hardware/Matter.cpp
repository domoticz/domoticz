#include "stdafx.h"
#include "Matter.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include <json/json.h>
#include <cmath>
#include <sstream>
#include <fstream>

#ifdef _DEBUG
	//#define DEBUG_CMATTER_WRITE
#endif

#ifdef DEBUG_CMATTER_WRITE
static void SaveString2Disk(const std::string& str, const std::string& filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif

static constexpr int      OUR_SCHEMA  = 11;
static constexpr int      MIN_SCHEMA  = 3;
static constexpr int      HEARTBEAT_S = 55;

// Matter cluster IDs (Matter Application Cluster Specification)
// Spec:     https://csa-iot.org/developer-resource/specifications-download-request/
// Clusters: https://github.com/project-chip/connectedhomeip/tree/master/src/app/clusters
static constexpr int CLUSTER_POWER_SOURCE              = 47;    // 0x002F
static constexpr int CLUSTER_ON_OFF                    = 6;     // 0x0006
static constexpr int CLUSTER_LEVEL_CONTROL             = 8;     // 0x0008
static constexpr int CLUSTER_DOOR_LOCK                 = 257;   // 0x0101
static constexpr int CLUSTER_WINDOW_COVERING           = 258;   // 0x0102
static constexpr int CLUSTER_THERMOSTAT                = 513;   // 0x0201
static constexpr int CLUSTER_ILLUMINANCE_MEASUREMENT   = 1024;  // 0x0400
static constexpr int CLUSTER_TEMPERATURE_MEASUREMENT   = 1026;  // 0x0402
static constexpr int CLUSTER_PRESSURE_MEASUREMENT      = 1027;  // 0x0403
static constexpr int CLUSTER_FLOW_MEASUREMENT          = 1028;  // 0x0404
static constexpr int CLUSTER_RELATIVE_HUMIDITY         = 1029;  // 0x0405
static constexpr int CLUSTER_OCCUPANCY_SENSING         = 1030;  // 0x0406
static constexpr int CLUSTER_BOOLEAN_STATE             = 69;    // 0x0045
static constexpr int CLUSTER_CO_CONCENTRATION          = 1036;  // 0x040C
static constexpr int CLUSTER_CO2_CONCENTRATION         = 1037;  // 0x040D
static constexpr int CLUSTER_NO2_CONCENTRATION         = 1043;  // 0x0413
static constexpr int CLUSTER_PM25_CONCENTRATION        = 1066;  // 0x042A
static constexpr int CLUSTER_PM10_CONCENTRATION        = 1069;  // 0x042D
static constexpr int CLUSTER_ELECTRICAL_ENERGY_MEAS    = 2316;  // 0x090C
static constexpr int CLUSTER_ELECTRICAL_POWER_MEAS     = 2317;  // 0x090D

// Matter attribute IDs used per cluster
static constexpr int ATTR_ON_OFF                       = 0;
static constexpr int ATTR_CURRENT_LEVEL                = 0;
static constexpr int ATTR_MEASURED_VALUE               = 0;     // used by most measurement clusters
static constexpr int ATTR_OCCUPANCY                    = 0;
static constexpr int ATTR_STATE_VALUE                  = 0;     // BooleanState
static constexpr int ATTR_LOCK_STATE                   = 0;     // DoorLock (0=not_fully_locked, 1=locked, 2=unlocked)
static constexpr int ATTR_LOCAL_TEMPERATURE            = 0;     // Thermostat (0.01 °C)
static constexpr int ATTR_CURRENT_POSITION_LIFT        = 10;    // 0x000A WindowCovering (0–10000, 0=open)
static constexpr int ATTR_BATTERY_PERCENT_REMAINING    = 12;    // 0x000C PowerSource
static constexpr int ATTR_VOLTAGE                      = 4;     // 0x0004 ElectricalPowerMeasurement (mV)
static constexpr int ATTR_ACTIVE_CURRENT               = 5;     // 0x0005 ElectricalPowerMeasurement (mA)
static constexpr int ATTR_ACTIVE_POWER                 = 8;     // 0x0008 ElectricalPowerMeasurement (mW)

// ChildID slot offsets within an endpoint (endpointId * 10 + slot).
// Max safe endpointId is 24 (24*10+11=251 < 255).
static constexpr int CHILD_OCCUPANCY = 1;
static constexpr int CHILD_ONOFF     = 2;
static constexpr int CHILD_CONTACT   = 3;
static constexpr int CHILD_CO2       = 4;
static constexpr int CHILD_CO        = 5;
static constexpr int CHILD_PM25      = 6;
static constexpr int CHILD_PM10      = 7;
static constexpr int CHILD_FLOW      = 8;
static constexpr int CHILD_LOCK      = 9;
static constexpr int CHILD_BLIND     = 10;
static constexpr int CHILD_NO2       = 11;

CMatter::CMatter(int ID, const std::string& serverAddress, uint16_t port)
	: ASyncWebsocket()
	, m_serverAddress(serverAddress)
	, m_port(port)
{
	m_HwdID = ID;
}

CMatter::~CMatter()
{
	StopHardware();
}

bool CMatter::StartHardware()
{
	RequestStart();
	m_bIsStarted = true;
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return true;
}

bool CMatter::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		disconnectWS();
		terminate();
		if (m_thread->joinable())
			m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CMatter::Do_Work()
{
	while (!IsStopRequested(0))
	{
		m_lastPingTime = mytime(nullptr);
		connectWS(m_serverAddress, m_port, "/ws");
		while (!IsStopRequested(1000))
		{
			if (!m_bConnected)
				break;
			time_t now = mytime(nullptr);
			if (now - m_lastPingTime >= HEARTBEAT_S)
			{
				SendWebsocketPing();
				m_lastPingTime = now;
			}
		}
		if (IsStopRequested(0))
			break;
		if (!IsStopRequested(10000))
			break;
	}
	disconnectWS();
	terminate();
}

void CMatter::OnWebsocketConnected()
{
	Log(LOG_STATUS, "Connected to matter server at %s:%d", m_serverAddress.c_str(), m_port);
	m_msgId = 0;
}

void CMatter::OnWebsocketMessage(WSOpcode opcode, const std::vector<uint8_t>& payload)
{
	std::string raw(payload.begin(), payload.end());

#ifdef DEBUG_CMATTER_WRITE
	SaveString2Disk(raw, "matter_message.json");
#endif

	Json::Value msg;
	Json::Reader reader;
	if (!reader.parse(raw, msg))
	{
		Log(LOG_ERROR, "Failed to parse WebSocket message: %s",
		    reader.getFormattedErrorMessages().c_str());
		return;
	}

	if (msg.isMember("sdk_version"))
	{
		HandleServerInfo(msg);
	}
	else if (msg.isMember("error_code") && msg["result"].isNull())
	{
		Log(LOG_ERROR, "Server error: %s", msg["details"].asString().c_str());
	}
	else if (msg.isMember("result"))
	{
		HandleResult(msg);
	}
	else if (msg.isMember("event"))
	{
		HandleEvent(msg);
	}
}

void CMatter::OnDisconnect()
{
	Log(LOG_STATUS, "Disconnected");
	m_bConnected = false;
}

void CMatter::OnError(const boost::system::error_code& error)
{
	Log(LOG_ERROR, "WebSocket error: %s", error.message().c_str());
}

void CMatter::SendCommand(const std::string& command, const Json::Value& args)
{
	Json::Value msg;
	msg["message_id"] = std::to_string(++m_msgId);
	msg["command"]    = command;
	msg["args"]       = args;
	Json::FastWriter writer;
	SendWebsocketMessage(writer.write(msg));
}

void CMatter::HandleServerInfo(const Json::Value& msg)
{
	if (!msg.isMember("schema_version") || !msg.isMember("min_supported_schema_version")) {
		Log(LOG_ERROR, "Missing schema fields in ServerInfo");
		disconnectWS();
		return;
	}
	int schema    = msg["schema_version"].asInt();
	int minSchema = msg["min_supported_schema_version"].asInt();
	if (schema < MIN_SCHEMA)
	{
		Log(LOG_ERROR, "incompatible schema server=%d (min %d), we support up to %d",
		    schema, minSchema, OUR_SCHEMA);
		disconnectWS();
		return;
	}
	if (minSchema > OUR_SCHEMA)
		Log(LOG_STATUS, "server schema %d (min %d) is newer than our supported %d — compatibility not guaranteed",
		    schema, minSchema, OUR_SCHEMA);
	Log(LOG_STATUS, "server schema %d, SDK %s",
	    schema, msg["sdk_version"].asString().c_str());
	m_bConnected = true;
	SendCommand("start_listening");
}

void CMatter::HandleResult(const Json::Value& msg)
{
	const Json::Value& result = msg["result"];

	// start_listening response: result is the nodes array directly
	if (result.isArray())
	{
		for (const auto& node : result)
		{
			if (node.isObject())
				HandleNode(node);
		}
		return;
	}

	// Alternative format: result is an object with a "nodes" array
	if (result.isObject() && result.isMember("nodes") && result["nodes"].isArray())
	{
		for (const auto& node : result["nodes"])
		{
			if (node.isObject())
				HandleNode(node);
		}
	}
}

void CMatter::HandleEvent(const Json::Value& msg)
{
	const Json::Value& ev = msg["event"];
	if (!ev.isMember("event_type") || !ev.isMember("data"))
	{
		Log(LOG_ERROR, "Missing event_type or data in event");
		return;
	}
	std::string event_type = ev["event_type"].asString();
	const Json::Value& data = ev["data"];

	if (event_type == "NODE_ADDED" || event_type == "NODE_UPDATED")
	{
		HandleNode(data);
	}
	else if (event_type == "NODE_REMOVED")
	{
		int nodeId = data["node_id"].asInt();
		std::lock_guard<std::mutex> lock(m_stateMutex);
		for (auto it = m_endpointStates.begin(); it != m_endpointStates.end(); )
		{
			if (it->first / 256 == nodeId)
				it = m_endpointStates.erase(it);
			else
				++it;
		}
	}
	else if (event_type == "ATTRIBUTE_UPDATED")
	{
		HandleAttributeUpdate(data);
	}
	else if (event_type == "SERVER_SHUTDOWN")
	{
		Log(LOG_STATUS, "Server shutdown received");
		m_bConnected = false;
		disconnectWS();
	}
}

void CMatter::HandleNode(const Json::Value& nodeData)
{
	int node_id = nodeData["node_id"].asInt();
	const Json::Value& attrs = nodeData["attributes"];

	std::lock_guard<std::mutex> lock(m_stateMutex);
	for (const auto& endpoint_id_str : attrs.getMemberNames())
	{
		int endpoint_id = std::stoi(endpoint_id_str, nullptr, 10);
		int domoticzID  = node_id * 256 + endpoint_id;
		auto& state     = m_endpointStates[domoticzID];

		const Json::Value& endpointClusters = attrs[endpoint_id_str];
		state.label = ExtractLabel(endpointClusters, node_id, endpoint_id);

		for (const auto& cluster_str : endpointClusters.getMemberNames())
		{
			int cluster_id = std::stoi(cluster_str, nullptr, 10);
			const Json::Value& clusterAttrs = endpointClusters[cluster_str];

			for (const auto& attr_str : clusterAttrs.getMemberNames())
			{
				int attr_id = std::stoi(attr_str, nullptr, 10);
				const Json::Value& v = clusterAttrs[attr_str];

				if (v.isNull())
					continue;

				ApplyAttributeToState(cluster_id, attr_id, v, state);
			}
		}

		_DetectAndSend(domoticzID);
	}
}

void CMatter::HandleAttributeUpdate(const Json::Value& data)
{
	if (!data.isMember("attribute_path"))
	{
		Log(LOG_ERROR, "Missing attribute_path in attribute update");
		return;
	}

	int node_id = data["node_id"].asInt();

	const std::string& attr_path = data["attribute_path"].asString();
	std::istringstream ss(attr_path);
	std::string ep_str, cl_str, at_str;
	if (!std::getline(ss, ep_str, '/') ||
	    !std::getline(ss, cl_str, '/') ||
	    !std::getline(ss, at_str, '/'))
	{
		Log(LOG_ERROR, "malformed attribute_path: %s", attr_path.c_str());
		return;
	}

	int endpoint_id = std::stoi(ep_str, nullptr, 10);
	int cluster_id  = std::stoi(cl_str, nullptr, 10);
	int attr_id     = std::stoi(at_str, nullptr, 10);
	int domoticzID  = node_id * 256 + endpoint_id;

	std::lock_guard<std::mutex> lock(m_stateMutex);
	auto it = m_endpointStates.find(domoticzID);
	if (it == m_endpointStates.end())
	{
		Log(LOG_ERROR, "Attribute update for unknown endpoint %d", domoticzID);
		return;
	}
	auto& state = it->second;

	const Json::Value& v = data["value"];
	if (v.isNull())
		return;

	ApplyAttributeToState(cluster_id, attr_id, v, state);
	_DetectAndSend(domoticzID);
}

void CMatter::ApplyAttributeToState(int cluster_id, int attr_id, const Json::Value& v, EndpointState& state)
{
	switch (cluster_id)
	{
		case CLUSTER_POWER_SOURCE:
			if (attr_id == ATTR_BATTERY_PERCENT_REMAINING)
				state.battery_pct = v.asInt() / 2;
			break;
		case CLUSTER_ON_OFF:
			if (attr_id == ATTR_ON_OFF)
			{
				state.onOff    = v.asBool();
				state.hasOnOff = true;
			}
			break;
		case CLUSTER_LEVEL_CONTROL:
			if (attr_id == ATTR_CURRENT_LEVEL)
			{
				state.level_pct = v.asInt() * 100.0 / 254.0;
				state.hasLevel  = true;
			}
			break;
		case CLUSTER_ILLUMINANCE_MEASUREMENT:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.lux    = (float)round(pow(10.0, (v.asInt() - 1) / 10000.0) * 10.0) / 10.0f;
				state.hasLux = true;
			}
			break;
		case CLUSTER_TEMPERATURE_MEASUREMENT:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.temp_C  = v.asInt() / 100.0f;
				state.hasTemp = true;
			}
			break;
		case CLUSTER_PRESSURE_MEASUREMENT:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.baro_hPa = v.asFloat(); // raw in 0.1 kPa = 1 hPa, no scaling needed
				state.hasBaro  = true;
			}
			break;
		case CLUSTER_RELATIVE_HUMIDITY:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.hum_pct = ground(v.asInt() / 100.0);
				state.hasHum  = true;
			}
			break;
		case CLUSTER_OCCUPANCY_SENSING:
			if (attr_id == ATTR_OCCUPANCY)
			{
				state.occupied     = (v.asInt() & 1) != 0;
				state.hasOccupancy = true;
			}
			break;
		case CLUSTER_BOOLEAN_STATE:
			if (attr_id == ATTR_STATE_VALUE)
			{
				state.contact    = v.asBool();
				state.hasContact = true;
			}
			break;
		case CLUSTER_THERMOSTAT:
			if (attr_id == ATTR_LOCAL_TEMPERATURE)
			{
				state.temp_C  = v.asInt() / 100.0f;
				state.hasTemp = true;
			}
			break;
		case CLUSTER_FLOW_MEASUREMENT:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				// raw in units of 0.1 m³/h → convert to L/min
				state.flow_lpm = v.asFloat() * 0.1f * 1000.0f / 60.0f;
				state.hasFlow  = true;
			}
			break;
		case CLUSTER_DOOR_LOCK:
			if (attr_id == ATTR_LOCK_STATE)
			{
				state.locked  = (v.asInt() == 1); // 1=locked, 2=unlocked, 0=not_fully_locked
				state.hasLock = true;
			}
			break;
		case CLUSTER_WINDOW_COVERING:
			if (attr_id == ATTR_CURRENT_POSITION_LIFT)
			{
				// 0=fully open, 10000=fully closed → invert to open percentage
				state.blind_pct = (10000.0 - v.asFloat()) / 100.0;
				state.hasBlind  = true;
			}
			break;
		case CLUSTER_CO_CONCENTRATION:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.co_ppm = v.asFloat();
				state.hasCO  = true;
			}
			break;
		case CLUSTER_CO2_CONCENTRATION:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.co2_ppm = ground(v.asFloat());
				state.hasCO2  = true;
			}
			break;
		case CLUSTER_NO2_CONCENTRATION:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.no2_ppm = v.asFloat();
				state.hasNO2  = true;
			}
			break;
		case CLUSTER_PM25_CONCENTRATION:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.pm25_ugm3 = v.asFloat();
				state.hasPM25   = true;
			}
			break;
		case CLUSTER_PM10_CONCENTRATION:
			if (attr_id == ATTR_MEASURED_VALUE)
			{
				state.pm10_ugm3 = v.asFloat();
				state.hasPM10   = true;
			}
			break;
		case CLUSTER_ELECTRICAL_POWER_MEAS:
			if (attr_id == ATTR_ACTIVE_POWER)
			{
				state.power_W  = v.asInt() / 1000.0; // mW → W
				state.hasPower = true;
			}
			else if (attr_id == ATTR_VOLTAGE)
			{
				state.voltage_V = v.asInt() / 1000.0f; // mV → V
				state.hasVolt   = true;
			}
			else if (attr_id == ATTR_ACTIVE_CURRENT)
			{
				state.current_A = v.asInt() / 1000.0f; // mA → A
				state.hasCurrent = true;
			}
			break;
		case CLUSTER_ELECTRICAL_ENERGY_MEAS:
			if (v.isObject() && v.isMember("energy") && v["energy"].isNumeric())
			{
				state.energy_kWh = v["energy"].asInt64() / 1000000.0; // mWh → kWh
				state.hasEnergy  = true;
			}
			break;
		default:
			Log(LOG_STATUS, "unhandled cluster %d (0x%04X), attr %d",
			    cluster_id, cluster_id, attr_id);
			break;
	}
}

// Must be called with m_stateMutex already held.
void CMatter::_DetectAndSend(int domoticzID)
{
	int endpointId = domoticzID % 256;
	auto it = m_endpointStates.find(domoticzID);
	if (it == m_endpointStates.end())
		return;
	auto& state = it->second;
	int battery = state.battery_pct;

	if (state.hasTemp && state.hasHum && state.hasBaro)
		SendTempHumBaroSensor(domoticzID, battery, state.temp_C, state.hum_pct, state.baro_hPa, 0, state.label);
	else if (state.hasTemp && state.hasHum)
		SendTempHumSensor(domoticzID, battery, state.temp_C, state.hum_pct, state.label);
	else if (state.hasTemp)
		SendTempSensor(domoticzID, battery, state.temp_C, state.label);
	else if (state.hasHum)
		SendHumiditySensor(domoticzID, battery, state.hum_pct, state.label);

	if (state.hasBaro && !state.hasTemp)
		SendPressureSensor(domoticzID, endpointId, battery, state.baro_hPa, state.label);

	if (state.hasPower)
		SendWattMeter(domoticzID, endpointId, battery, static_cast<float>(state.power_W), state.label);
	if (state.hasEnergy)
		SendKwhMeter(domoticzID, endpointId, battery, state.power_W, state.energy_kWh, state.label);

	if (state.hasLux)
		SendLuxSensor((uint8_t)(domoticzID >> 8), (uint8_t)endpointId, (uint8_t)battery, state.lux, state.label);

	if (state.hasOccupancy)
		SendSwitch(domoticzID, (uint8_t)(endpointId * 10 + CHILD_OCCUPANCY), battery, state.occupied, 0, state.label, "");

	if (state.hasOnOff && state.hasLevel)
		SendSwitch(domoticzID, (uint8_t)(endpointId * 10 + CHILD_ONOFF), battery, state.onOff, state.level_pct, state.label, "");
	else if (state.hasOnOff)
		SendSwitch(domoticzID, (uint8_t)(endpointId * 10 + CHILD_ONOFF), battery, state.onOff, 0, state.label, "");

	if (state.hasContact)
		SendSwitch(domoticzID, (uint8_t)(endpointId * 10 + CHILD_CONTACT), battery, state.contact, 0, state.label, "");

	if (state.hasCO2)
		SendAirQualitySensor((uint8_t)(domoticzID >> 8), (uint8_t)(endpointId * 10 + CHILD_CO2), battery, state.co2_ppm, state.label);

	if (state.hasCO)
		SendCustomSensor(domoticzID, (uint8_t)(endpointId * 10 + CHILD_CO), battery, state.co_ppm, state.label + " CO", "ppm");

	if (state.hasNO2)
		SendCustomSensor(domoticzID, (uint8_t)(endpointId * 10 + CHILD_NO2), battery, state.no2_ppm, state.label + " NO2", "ppm");

	if (state.hasPM25)
		SendCustomSensor(domoticzID, (uint8_t)(endpointId * 10 + CHILD_PM25), battery, state.pm25_ugm3, state.label + " PM2.5", "µg/m³");

	if (state.hasPM10)
		SendCustomSensor(domoticzID, (uint8_t)(endpointId * 10 + CHILD_PM10), battery, state.pm10_ugm3, state.label + " PM10", "µg/m³");

	if (state.hasFlow)
		SendWaterflowSensor(domoticzID, (uint8_t)(endpointId * 10 + CHILD_FLOW), battery, state.flow_lpm, state.label);

	if (state.hasLock)
		SendSwitch(domoticzID, (uint8_t)(endpointId * 10 + CHILD_LOCK), battery, state.locked, 0, state.label, "");

	if (state.hasBlind)
		SendPercentageSensor(domoticzID, (uint8_t)(endpointId * 10 + CHILD_BLIND), battery, (float)state.blind_pct, state.label);

	if (state.hasVolt)
		SendVoltageSensor(domoticzID, endpointId, battery, state.voltage_V, state.label);

	if (state.hasCurrent)
		SendCurrentSensor(domoticzID, battery, state.current_A, 0, 0, state.label);
}

std::string CMatter::ExtractLabel(const Json::Value& endpointAttrs, int nodeId, int endpointId) const
{
	// cluster 0x0039 (57), attr 0x0001 (1) — bridged node label
	if (endpointAttrs.isMember("57") && endpointAttrs["57"].isMember("1"))
	{
		std::string label = endpointAttrs["57"]["1"].asString();
		if (!label.empty())
			return label;
	}
	// cluster 0x0028 (40), attr 0x0001 (1) — node label
	if (endpointAttrs.isMember("40") && endpointAttrs["40"].isMember("1"))
	{
		std::string label = endpointAttrs["40"]["1"].asString();
		if (!label.empty())
			return label;
	}
	// cluster 0x0028 (40), attr 0x0005 (5) — product name
	if (endpointAttrs.isMember("40") && endpointAttrs["40"].isMember("5"))
	{
		std::string label = endpointAttrs["40"]["5"].asString();
		if (!label.empty())
			return label;
	}
	return "Matter-" + std::to_string(nodeId) + "-" + std::to_string(endpointId);
}

bool CMatter::WriteToHardware(const char* pdata, unsigned char length)
{
	if (!pdata || length == 0) return false;

	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);

	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false;

	int domoticzID = (pCmd->LIGHTING2.id1 << 24)
			| (pCmd->LIGHTING2.id2 << 16)
			| (pCmd->LIGHTING2.id3 << 8)
			|  pCmd->LIGHTING2.id4;
	int nodeId     = domoticzID / 256;
	int endpointId = domoticzID % 256;
	uint8_t cmnd   = pCmd->LIGHTING2.cmnd;

	if (cmnd == light2_sSetLevel)
	{
		Json::Value args;
		args["node_id"]      = nodeId;
		args["endpoint_id"]  = endpointId;
		args["cluster_id"]   = 8;
		args["command_name"] = "move_to_level_with_on_off";
		Json::Value payload;
		payload["level"]           = (int)(pCmd->LIGHTING2.level * 254 / 100);
		payload["transition_time"] = 0;
		args["payload"] = payload;
		SendCommand("device_command", args);
	}
	else
	{
		std::string cmd;
		if (cmnd == light2_sOn)
			cmd = "on";
		else if (cmnd == light2_sOff)
			cmd = "off";
		else
			cmd = "toggle";

		Json::Value args;
		args["node_id"]      = nodeId;
		args["endpoint_id"]  = endpointId;
		args["cluster_id"]   = 6;
		args["command_name"] = cmd;
		args["payload"]      = Json::Value(Json::objectValue);
		SendCommand("device_command", args);
	}

	return true;
}
