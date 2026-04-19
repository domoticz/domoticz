#include "stdafx.h"
#include "Matter.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "ColorSwitch.h"
#include <json/json.h>
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <fstream>

#ifdef _DEBUG
	#define DEBUG_CMATTER_WRITE
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
static constexpr int CLUSTER_DESCRIPTION               = 29;    // 0x001D
static constexpr int CLUSTER_BASIC_INFORMATION         = 40;    // 0x0028
static constexpr int CLUSTER_GENERAL_DIAGNOSTICS       = 51;    // 0x0033
static constexpr int CLUSTER_THREAD_DIAGNOSTICS        = 53;    // 0x0035
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
static constexpr int CLUSTER_GENERIC_SWITCH              = 59;    // 0x003B GenericSwitch (button/latch)
static constexpr int CLUSTER_COLOR_CONTROL               = 768;   // 0x0300 ColorControl
static constexpr int CLUSTER_ELECTRICAL_POWER_MEAS      = 144;   // 0x0090 ElectricalPowerMeasurement (Matter 1.3+)
static constexpr int CLUSTER_ELECTRICAL_ENERGY_MEAS     = 145;   // 0x0091 ElectricalEnergyMeasurement (Matter 1.3+)

// Matter attribute IDs used per cluster
static constexpr int ATTR_ON_OFF                       = 0;
static constexpr int ATTR_CURRENT_LEVEL                = 0;
static constexpr int ATTR_MEASURED_VALUE               = 0;     // used by most measurement clusters
static constexpr int ATTR_OCCUPANCY                    = 0;
static constexpr int ATTR_STATE_VALUE                  = 0;     // BooleanState
static constexpr int ATTR_LOCK_STATE                   = 0;     // DoorLock (0=not_fully_locked, 1=locked, 2=unlocked)
static constexpr int ATTR_LOCAL_TEMPERATURE            = 0;     // Thermostat (0.01 °C)

static constexpr int ATTR_SWITCH_CURRENT_POSITION	   = 1;

static constexpr int ATTR_OCCUPIED_HEATING_SETPOINT    = 18;    // 0x0011 Thermostat (0.01 °C, writable)
static constexpr int ATTR_ABS_MIN_HEAT_SETPOINT        = 3;     // 0x0003 Thermostat
static constexpr int ATTR_ABS_MAX_HEAT_SETPOINT        = 4;     // 0x0004 Thermostat
static constexpr int ATTR_ABS_MIN_COOL_SETPOINT        = 5;     // 0x0005 Thermostat
static constexpr int ATTR_ABS_MAX_COOL_SETPOINT        = 6;     // 0x0006 Thermostat
static constexpr int ATTR_CTRL_SEQ_OF_OPERATION        = 27;    // 0x001B Thermostat ControlSequenceOfOperationEnum (writable)
static constexpr int ATTR_SYSTEM_MODE                  = 28;    // 0x001C Thermostat SystemModeEnum (writable)
static constexpr int ATTR_ACTIVE_PRESET_HANDLE         = 78;    // 0x004E Thermostat (bytes, writable)
static constexpr int ATTR_PRESETS                      = 80;    // 0x0050 Thermostat list of PresetStruct
static constexpr int ATTR_CURRENT_POSITION_LIFT        = 10;    // 0x000A WindowCovering (0–10000, 0=open)
static constexpr int ATTR_THREAD_CHANNEL               = 0;     // 0x0000 ThreadNetworkDiagnostics
static constexpr int ATTR_THREAD_ROUTING_ROLE          = 1;     // 0x0001 ThreadNetworkDiagnostics (RoutingRoleEnum)
static constexpr int ATTR_THREAD_NETWORK_NAME          = 2;     // 0x0002 ThreadNetworkDiagnostics
static constexpr int ATTR_THREAD_NEIGHBOR_TABLE        = 7;     // 0x0007 ThreadNetworkDiagnostics (array of NeighborTableStruct)
static constexpr int ATTR_BATTERY_VOLTAGE              = 11;    // 0x000B PowerSource (mV × 1, so 2636 = 2.636 V)
static constexpr int ATTR_BATTERY_PERCENT_REMAINING    = 12;    // 0x000C PowerSource (half-percent units, divide by 2)
static constexpr int ATTR_CUMULATIVE_ENERGY_IMPORTED   = 1;     // 0x0001 ElectricalEnergyMeasurement (struct, field "0" = mWh)
static constexpr int ATTR_VOLTAGE                      = 4;     // 0x0004 ElectricalPowerMeasurement (mV)
static constexpr int ATTR_ACTIVE_CURRENT               = 5;     // 0x0005 ElectricalPowerMeasurement (mA)
static constexpr int ATTR_ACTIVE_POWER                 = 8;     // 0x0008 ElectricalPowerMeasurement (mW)
static constexpr int ATTR_ACTIVE_FREQUENCY			   = 14;    // 0x000E ElectricalPowerFrequency (Hz)
static constexpr int ATTR_COLOR_CONTROL_HUE          = 0;     // 0x0000 ColorControl CurrentHue (0-254 = 0-360°)
static constexpr int ATTR_COLOR_CONTROL_SAT          = 1;     // 0x0001 ColorControl CurrentSaturation (0-254 = 0-100%)
static constexpr int ATTR_COLOR_CURRENT_X            = 3;     // 0x0003 ColorControl CurrentX (* 65536)
static constexpr int ATTR_COLOR_CURRENT_Y            = 4;     // 0x0004 ColorControl CurrentY (* 65536)
static constexpr int ATTR_COLOR_TEMP_MIREDS          = 7;     // 0x0007 ColorControl ColorTemperatureMireds
static constexpr int ATTR_COLOR_MODE                 = 8;     // 0x0008 ColorControl ColorModeEnum
static constexpr int ATTR_COLOR_TEMP_PHYS_MIN        = 16395; // 0x400B ColorControl ColorTempPhysicalMinMireds
static constexpr int ATTR_COLOR_TEMP_PHYS_MAX        = 16396; // 0x400C ColorControl ColorTempPhysicalMaxMireds
static constexpr int ATTR_COLOR_CAPABILITIES         = 16394; // 0x400A ColorControl ColorCapabilitiesBitmap

// ChildID slot offsets — unique sensor type identifiers within a domoticzID.
// domoticzID already encodes the endpoint (nodeId * 256 + endpointId), so no
// per-endpoint multiplier is needed here.
static constexpr int CHILD_OCCUPANCY = 1;
static constexpr int CHILD_ONOFF     = 2;
static constexpr int CHILD_CONTACT   = 3;
static constexpr int CHILD_CO2       = 4;
static constexpr int CHILD_CO        = 5;
static constexpr int CHILD_PM25      = 6;
static constexpr int CHILD_PM10      = 7;
static constexpr int CHILD_FLOW      = 8;
static constexpr int CHILD_LOCK      = 9;
static constexpr int CHILD_BLIND        = 10;
static constexpr int CHILD_NO2          = 11;
static constexpr int CHILD_SYSTEM_MODE  = 12;
static constexpr int CHILD_PRESET       = 13;
static constexpr int CHILD_CTRL_SEQ     = 14;
static constexpr int CHILD_SWITCH       = 15;
static constexpr int CHILD_FREQ         = 16;

// Non-contiguous SystemModeEnum values in selector-level order (level = index * 10)
static constexpr int SYSTEM_MODE_VALUES[]    = {0, 1, 3, 4, 5, 6, 7, 8, 9};
static constexpr int SYSTEM_MODE_VALUES_SIZE = sizeof(SYSTEM_MODE_VALUES) / sizeof(SYSTEM_MODE_VALUES[0]);

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
		if (m_thread->joinable())
			m_thread->join();
		m_thread.reset();
		terminate();
	}
	m_bIsStarted = false;
	return true;
}

void CMatter::Do_Work()
{
	int sec_counter = 0;
	while (!IsStopRequested(0))
	{
		m_lastPingTime = mytime(nullptr);
		connectWS(m_serverAddress, m_port, "/ws");
		while (!IsStopRequested(1000))
		{
			sec_counter++;
			if (sec_counter % 12 == 0)
				m_LastHeartbeat = mytime(nullptr);

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
		if (IsStopRequested(10000))
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

	Debug(DEBUG_RECEIVED, raw);

#ifdef DEBUG_CMATTER_WRITE
	static int cntr = 1;
	char szFilename[MAX_PATH];
	sprintf_s(szFilename, "E:\\matter_message_%04d.json", cntr++);
	SaveString2Disk(raw, szFilename);
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
	std::set<int> nodeIds;

	auto processNode = [&](const Json::Value& node) {
		if (!node.isObject()) return;
		HandleNode(node);
		if (node.isMember("node_id"))
			nodeIds.insert(node["node_id"].asInt());
	};

	if (result.isArray())
	{
		for (const auto& node : result)
			processNode(node);
	}
	else if (result.isObject() && result.isMember("nodes") && result["nodes"].isArray())
	{
		for (const auto& node : result["nodes"])
			processNode(node);
	}

	std::lock_guard<std::mutex> lock(m_stateMutex);
	for (int nodeId : nodeIds)
		_DetectAndSendNode(nodeId);
}

void CMatter::HandleEvent(const Json::Value& msg)
{
	if (!msg.isMember("event"))
	{
		Log(LOG_ERROR, "Missing 'event' field in message");
		return;
	}
	if (!msg.isMember("data"))
	{
		Log(LOG_ERROR, "Missing 'data' field for event '%s'", msg["event"].asString().c_str());
		return;
	}
	std::string event_type = msg["event"].asString();
	std::transform(event_type.begin(), event_type.end(), event_type.begin(), ::toupper);
	const Json::Value& data = msg["data"];

	if (event_type == "NODE_ADDED" || event_type == "NODE_UPDATED")
	{
		HandleNode(data);
		if (data.isObject() && data.isMember("node_id"))
		{
			std::lock_guard<std::mutex> lock(m_stateMutex);
			_DetectAndSendNode(data["node_id"].asInt());
		}
	}
	else if (event_type == "NODE_REMOVED")
	{
		// data may be an object {"node_id":N}, a scalar N, or an array [N]
		int nodeId = -1;
		if (data.isObject() && data.isMember("node_id"))
			nodeId = data["node_id"].asInt();
		else if (data.isIntegral())
			nodeId = data.asInt();
		else if (data.isArray() && data.size() >= 1)
			nodeId = data[0].asInt();

		if (nodeId < 0)
		{
			Log(LOG_ERROR, "Cannot parse node_id from node_removed event");
			return;
		}
		std::lock_guard<std::mutex> lock(m_stateMutex);
		m_nodes.erase(nodeId);
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
	if (!nodeData.isMember("node_id") || !nodeData.isMember("attributes"))
		return;
	int node_id = nodeData["node_id"].asInt();
	if (node_id < 1 || node_id > 255)
	{
		Log(LOG_DEBUG_INT, "Ignoring node_id %d (controller/out-of-range)", node_id);
		return;
	}
	const Json::Value& attrs = nodeData["attributes"];
	if (!attrs.isObject())
		return;

	// Collect endpoint IDs and set labels before iterating attributes.
	std::set<int> endpointIds;
	for (const auto& path : attrs.getMemberNames())
	{
		size_t p1 = path.find('/');
		if (p1 == std::string::npos) continue;
		try {
			int ep = std::stoi(path.substr(0, p1));
			if (ep >= 0 && ep <= 255)
				endpointIds.insert(ep);
		} catch (...) {}
	}

	std::lock_guard<std::mutex> lock(m_stateMutex);
	auto& node = m_nodes[node_id];
	node.nodeId = node_id;
	if (nodeData.isMember("available") && nodeData["available"].isBool())
		node.available = nodeData["available"].asBool();
	else
		node.available = true;

	for (int endpoint_id : endpointIds)
		node.endpoints[endpoint_id].label = ExtractLabel(attrs, node_id, endpoint_id);

	// Single pass: route each attribute to the endpoint state and, for endpoint 0,
	// also extract node-level metadata.
	for (const auto& path : attrs.getMemberNames())
	{
		size_t p1 = path.find('/');
		if (p1 == std::string::npos) continue;
		size_t p2 = path.find('/', p1 + 1);
		if (p2 == std::string::npos) continue;

		int ep, cluster_id, attr_id;
		try {
			ep         = std::stoi(path.substr(0, p1));
			cluster_id = std::stoi(path.substr(p1 + 1, p2 - p1 - 1));
			attr_id    = std::stoi(path.substr(p2 + 1));
		} catch (...) { continue; }

		if (ep < 0 || ep > 255) continue;

		const Json::Value& v = attrs[path];
		if (v.isNull()) continue;

		if (endpointIds.count(ep))
			ApplyAttributeToState(cluster_id, attr_id, v, node.endpoints[ep]);

		if (ep == 0)
			ApplyNodeMetadata(cluster_id, attr_id, v, node);
	}
}

void CMatter::HandleAttributeUpdate(const Json::Value& data)
{
	// Protocol format: data is a positional array [node_id, "endpoint/cluster/attr", value]
	if (!data.isArray() || data.size() < 3)
	{
		Log(LOG_ERROR, "Unexpected attribute_updated data format");
		return;
	}

	int node_id = data[0].asInt();
	if (node_id < 1 || node_id > 255)
		return;

	const std::string attr_path = data[1].asString();
	size_t p1 = attr_path.find('/');
	size_t p2 = (p1 != std::string::npos) ? attr_path.find('/', p1 + 1) : std::string::npos;
	if (p1 == std::string::npos || p2 == std::string::npos)
	{
		Log(LOG_ERROR, "malformed attribute_path: %s", attr_path.c_str());
		return;
	}

	int endpoint_id, cluster_id, attr_id;
	try {
		endpoint_id = std::stoi(attr_path.substr(0, p1));
		cluster_id  = std::stoi(attr_path.substr(p1 + 1, p2 - p1 - 1));
		attr_id     = std::stoi(attr_path.substr(p2 + 1));
	} catch (...) {
		Log(LOG_ERROR, "malformed attribute_path components: %s", attr_path.c_str());
		return;
	}
	if (endpoint_id < 0 || endpoint_id > 255)
	{
		Log(LOG_ERROR, "endpoint_id %d out of range in %s", endpoint_id, attr_path.c_str());
		return;
	}

	const Json::Value& v = data[2];
	if (v.isNull())
		return;

	std::lock_guard<std::mutex> lock(m_stateMutex);
	auto nodeIt = m_nodes.find(node_id);
	if (nodeIt == m_nodes.end())
	{
		Log(LOG_DEBUG_INT, "attribute_updated for unknown node %d (%s) — update before start_listening?", node_id, attr_path.c_str());
		return;
	}
	auto epIt = nodeIt->second.endpoints.find(endpoint_id);
	if (epIt == nodeIt->second.endpoints.end())
	{
		Log(LOG_DEBUG_INT, "attribute_updated for unknown endpoint %d/%d (%s)", node_id, endpoint_id, attr_path.c_str());
		return;
	}

	ApplyAttributeToState(cluster_id, attr_id, v, epIt->second);
	if (endpoint_id == 0)
		ApplyNodeMetadata(cluster_id, attr_id, v, nodeIt->second);
	_DetectAndSendNode(node_id);
}

void CMatter::ApplyAttributeToState(int cluster_id, int attr_id, const Json::Value& v, EndpointState& state)
{
	switch (cluster_id)
	{
		case CLUSTER_DESCRIPTION:
		case 31: //0x001F AccessControl
		case CLUSTER_BASIC_INFORMATION:
		case 42: //0x002A OtaSoftwareUpdateRequestor
		case 48: //0x0030 GeneralCommissioning
		case 49: //0x0031 NetworkCommissioning
		case CLUSTER_GENERAL_DIAGNOSTICS:
		case CLUSTER_THREAD_DIAGNOSTICS: // handled in ApplyNodeMetadata
		case 60: //0x003C AdministratorCommissioning
		case 62: //0x003E OperationalCredentials
		case 63: //0x003F GroupKeyManagement
		case 70: //0x0046 IcdManagement
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
				int raw   = std::max(0, std::min(254, v.asInt()));
				int range = std::max(1, state.levelMaxLevel - state.levelMinLevel);
				state.level_pct = std::max(0.0, std::min(100.0, (raw - state.levelMinLevel) * 100.0 / range));
				state.hasLevel  = true;
			}
			else if (attr_id == 2) // MinLevel
				state.levelMinLevel = std::max(0, v.asInt());
			else if (attr_id == 3) // MaxLevel
				state.levelMaxLevel = std::min(254, v.asInt());
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
		case CLUSTER_GENERIC_SWITCH:
			if (attr_id == ATTR_SWITCH_CURRENT_POSITION) // 0=released, ≥1=pressed/latched
			{
				state.switch_on = (v.asInt() != 0);
				state.hasSwitch = true;
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
			else if (attr_id == ATTR_OCCUPIED_HEATING_SETPOINT)
			{
				state.setpoint_C = v.asInt() / 100.0f;
				state.hasSetpoint = true;
			}
			else if (attr_id == ATTR_SYSTEM_MODE)
			{
				state.systemMode    = v.asInt();
				state.hasSystemMode = true;
			}
			else if (attr_id == ATTR_CTRL_SEQ_OF_OPERATION && v.isIntegral())
			{
				state.ctrlSeqOp    = v.asInt();
				state.hasCtrlSeqOp = true;
			}
			else if (attr_id == ATTR_ABS_MIN_HEAT_SETPOINT && v.isIntegral())
				state.minHeatSetpoint_C = v.asInt() / 100.0f;
			else if (attr_id == ATTR_ABS_MAX_HEAT_SETPOINT && v.isIntegral())
				state.maxHeatSetpoint_C = v.asInt() / 100.0f;
			else if (attr_id == ATTR_ABS_MIN_COOL_SETPOINT && v.isIntegral())
				state.minCoolSetpoint_C = v.asInt() / 100.0f;
			else if (attr_id == ATTR_ABS_MAX_COOL_SETPOINT && v.isIntegral())
				state.maxCoolSetpoint_C = v.asInt() / 100.0f;
			else if (attr_id == ATTR_ACTIVE_PRESET_HANDLE)
			{
				state.activePresetHandle = (v.isString()) ? v.asString() : "";
			}
			else if (attr_id == ATTR_PRESETS && v.isArray())
			{
				state.presets.clear();
				for (const auto& preset : v)
				{
					if (!preset.isObject()) continue;
					EndpointState::PresetEntry entry;
					if (preset.isMember("0") && preset["0"].isString())
						entry.handle = preset["0"].asString();
					if (preset.isMember("1") && preset["1"].isIntegral())
						entry.scenario = preset["1"].asInt();
					if (preset.isMember("2") && preset["2"].isString())
						entry.name = preset["2"].asString();
					state.presets.push_back(std::move(entry));
				}
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
			else if (attr_id == ATTR_ACTIVE_FREQUENCY)
			{
				state.frequency_Hz = v.asInt(); // Hz
				state.hasFrequency = true;
			}
			break;
		case CLUSTER_ELECTRICAL_ENERGY_MEAS:
			// CumulativeEnergyImported (attr 1) is a struct; field "0" carries the mWh value.
			// Example from device: {"0":16275000,"4":12345} where "4" is endSystime.
			if (attr_id == ATTR_CUMULATIVE_ENERGY_IMPORTED && v.isObject() && v.isMember("0"))
			{
				state.energy_kWh = v["0"].asInt64() / 1000000.0; // mWh → kWh
				state.hasEnergy  = true;
			}
			break;
		case CLUSTER_COLOR_CONTROL:
			if (attr_id == ATTR_COLOR_CONTROL_HUE && v.isIntegral())
			{
				state.colorHue        = v.asInt();
				state.hasColorHS      = true;
				state.hasColorControl = true;
			}
			else if (attr_id == ATTR_COLOR_CONTROL_SAT && v.isIntegral())
			{
				state.colorSaturation = v.asInt();
				state.hasColorHS      = true;
				state.hasColorControl = true;
			}
			else if (attr_id == ATTR_COLOR_CURRENT_X && v.isIntegral())
			{
				state.colorX         = v.asInt() / 65536.0f;
				state.hasColorXY     = true;
				state.hasColorControl = true;
			}
			else if (attr_id == ATTR_COLOR_CURRENT_Y && v.isIntegral())
			{
				state.colorY         = v.asInt() / 65536.0f;
				state.hasColorXY     = true;
				state.hasColorControl = true;
			}
			else if (attr_id == ATTR_COLOR_TEMP_MIREDS && v.isIntegral())
			{
				state.colorTempMireds  = v.asInt();
				state.hasColorTemp     = true;
				state.hasColorControl  = true;
			}
			else if (attr_id == ATTR_COLOR_MODE && v.isIntegral())
			{
				state.colorMode        = v.asInt();
				state.hasColorControl  = true;
			}
			else if (attr_id == ATTR_COLOR_TEMP_PHYS_MIN && v.isIntegral() && v.asInt() > 0)
				state.colorTempMinMireds = v.asInt();
			else if (attr_id == ATTR_COLOR_TEMP_PHYS_MAX && v.isIntegral() && v.asInt() > 0)
				state.colorTempMaxMireds = v.asInt();
			else if (attr_id == ATTR_COLOR_CAPABILITIES && v.isIntegral())
				state.colorCapabilities = v.asInt();
			break;
		default:
			break;
	}
}

void CMatter::ApplyNodeMetadata(int cluster_id, int attr_id, const Json::Value& v, NodeState& node)
{
	switch (cluster_id)
	{
		case CLUSTER_BASIC_INFORMATION:
			if      (attr_id == 1  && v.isString())   node.vendorName             = v.asString();
			else if (attr_id == 2  && v.isIntegral())  node.vendorId               = static_cast<uint16_t>(v.asUInt());
			else if (attr_id == 3  && v.isString())   node.productName            = v.asString();
			else if (attr_id == 4  && v.isIntegral())  node.productId              = static_cast<uint16_t>(v.asUInt());
			else if (attr_id == 5  && v.isString())   node.nodeLabel              = v.asString();
			else if (attr_id == 8  && v.isString())   node.hardwareVersionString  = v.asString();
			else if (attr_id == 10 && v.isString())   node.softwareVersionString  = v.asString();
			break;
		case CLUSTER_POWER_SOURCE:
			if (attr_id == ATTR_BATTERY_PERCENT_REMAINING && v.isIntegral())
				node.battery_pct = v.asInt() / 2;
			else if (attr_id == ATTR_BATTERY_VOLTAGE && v.isIntegral())
			{
				node.batteryVoltage_V  = v.asInt() / 1000.0f;
				node.hasBatteryVoltage = true;
			}
			break;
		case CLUSTER_GENERAL_DIAGNOSTICS:
			if (attr_id == 2 && v.isIntegral())
			{
				node.uptime_s = v.asUInt64();
			}
			break;
		case CLUSTER_THREAD_DIAGNOSTICS:
			if (attr_id == ATTR_THREAD_CHANNEL && v.isIntegral())
				node.threadChannel = static_cast<uint8_t>(v.asUInt());
			else if (attr_id == ATTR_THREAD_ROUTING_ROLE && v.isIntegral())
				node.threadRoutingRole = static_cast<NodeState::RoutingRoleEnum>(v.asUInt());
			else if (attr_id == ATTR_THREAD_NETWORK_NAME && v.isString())
				node.threadNetworkName = v.asString();
			else if (attr_id == ATTR_THREAD_NEIGHBOR_TABLE && v.isArray())
			{
				node.threadNeighbors.clear();
				for (const auto& entry : v)
				{
					if (!entry.isObject()) continue;
					if (node.threadNeighbors.size() >= 64) break; // Thread spec max routers = 64
					NodeState::ThreadNeighbor n;
					if (entry.isMember("0"))  n.extAddress       = entry["0"].asUInt64();
					if (entry.isMember("1"))  n.age              = entry["1"].asUInt();
					if (entry.isMember("2"))  n.rloc16           = static_cast<uint16_t>(entry["2"].asUInt());
					if (entry.isMember("3"))  n.linkFrameCounter = entry["3"].asUInt();
					if (entry.isMember("4"))  n.mleFrameCounter  = entry["4"].asUInt();
					if (entry.isMember("5"))  n.lqi              = static_cast<uint8_t>(entry["5"].asUInt());
					if (entry.isMember("6"))  n.averageRssi      = static_cast<int8_t>(entry["6"].asInt());
					if (entry.isMember("7"))  n.lastRssi         = static_cast<int8_t>(entry["7"].asInt());
					if (entry.isMember("8"))  n.frameErrorRate   = static_cast<uint8_t>(entry["8"].asUInt());
					if (entry.isMember("9"))  n.messageErrorRate = static_cast<uint8_t>(entry["9"].asUInt());
					if (entry.isMember("10")) n.rxOnWhenIdle     = entry["10"].asBool();
					if (entry.isMember("11")) n.fullThreadDevice = entry["11"].asBool();
					if (entry.isMember("12")) n.fullNetworkData  = entry["12"].asBool();
					if (entry.isMember("13")) n.isChild          = entry["13"].asBool();
					node.threadNeighbors.push_back(n);
				}
			}
			break;
		default:
			break;
	}
}

// Must be called with m_stateMutex already held.
void CMatter::_DetectAndSendNode(int nodeId)
{
	auto nodeIt = m_nodes.find(nodeId);
	if (nodeIt == m_nodes.end())
		return;
	const auto& node = nodeIt->second;

	if (!node.available)
	{
		Log(LOG_DEBUG_INT, "Node %d is unavailable – skipping device update", nodeId);
		return;
	}

	// Aggregate environmental sensor readings across all endpoints of this node.
	float setpoint_C = 0;  bool hasSetpoint = false;
	float temp_C   = 0;  bool hasTemp = false;
	int   hum_pct  = 0;  bool hasHum  = false;
	float baro_hPa = 0;  bool hasBaro = false;
	int   battery  = node.battery_pct;
	std::string label;

	for (const auto& kv : node.endpoints)
	{
		const auto& s = kv.second;
		if (!hasTemp && s.hasTemp)  { temp_C   = s.temp_C;   hasTemp = true; }
		if (!hasHum  && s.hasHum)   { hum_pct  = s.hum_pct;  hasHum  = true; }
		if (!hasBaro && s.hasBaro)  { baro_hPa = s.baro_hPa; hasBaro = true; }
		if (!hasSetpoint && s.hasSetpoint) { setpoint_C = s.setpoint_C; hasSetpoint = true; }
		if (label.empty() && !s.label.empty()) label = s.label;
	}

	// Aggregated environmental sensors use nodeId as domoticzID (1-255),
	// giving users a direct mapping: device ID == Matter node number.
	if (hasSetpoint)
	{
		if (hasTemp && hasHum && hasBaro)
			SendThermostatSensor(0, 0, 0, nodeId, 1, battery, setpoint_C, temp_C, hum_pct, baro_hPa, label);
		else if (hasTemp && hasHum)
			SendThermostatSensor(0, 0, 0, nodeId, 1, battery, setpoint_C, temp_C, hum_pct, label);
		else if (hasTemp)
			SendThermostatSensor(0, 0, 0, nodeId, 1, battery, setpoint_C, temp_C, label);
		else
			SendSetPointSensor(0, 0, 0, nodeId, 1, battery, setpoint_C, label);
	}
	else if (hasTemp && hasHum && hasBaro)
		SendTempHumBaroSensor(nodeId, battery, temp_C, hum_pct, baro_hPa, 0, label);
	else if (hasTemp && hasHum)
		SendTempHumSensor(nodeId, battery, temp_C, hum_pct, label);
	else if (hasTemp)
		SendTempSensor(nodeId, battery, temp_C, label);
	else if (hasHum)
		SendHumiditySensor(nodeId, battery, hum_pct, label);

	if (hasBaro && !hasTemp)
		SendPressureSensor(nodeId, 0, battery, baro_hPa, label);

	// Per-endpoint non-environmental sensors (on/off, power, CO2, etc.)
	for (const auto& kv : node.endpoints)
		_DetectAndSend(nodeId, kv.first);
}

// If wasNew, updates the device's SwitchType in the DB (called after device creation).
// Centralises the snprintf + UPDATE so both SendGeneralSwitchInt and the color-switch
// path share identical formatting and SQL.
void CMatter::_ApplySwitchTypeOnCreate(int domoticzID, int unit, bool wasNew, _eSwitchType switchType)
{
	if (!wasNew || switchType == STYPE_OnOff || !m_sql.m_bAcceptNewHardware)
		return;
	char szDevID[16];
	snprintf(szDevID, sizeof(szDevID), "%08X", (unsigned int)domoticzID);

	//check and wait for device creation
	for (int i = 0; i < 20; i++)
	{
    	auto result = m_sql.safe_query(
			"SELECT ID FROM DeviceStatus WHERE HardwareID=%d AND DeviceID='%q' AND Unit=%d",
			m_HwdID, szDevID, unit);
		if (!result.empty())
			break;
		sleep_milliseconds(50);
	}

	m_sql.safe_query(
		"UPDATE DeviceStatus SET SwitchType=%d "
		"WHERE HardwareID=%d AND DeviceID='%q' AND Unit=%d",
		(int)switchType, m_HwdID, szDevID, unit);
}

// Calls SendGeneralSwitch and, on first creation only, sets the correct SwitchType.
// Avoids touching the DB on every subsequent update cycle.
void CMatter::SendGeneralSwitchInt(int domoticzID, int unit, int battery, int value, int level,
                                   const std::string& label, _eSwitchType switchType)
{
	char szDevID[16];
	snprintf(szDevID, sizeof(szDevID), "%08X", (unsigned int)domoticzID);
	bool isNew = m_sql.safe_query(
		"SELECT ID FROM DeviceStatus WHERE HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d",
		m_HwdID, szDevID, unit, pTypeGeneralSwitch, sSwitchTypeAC).empty();

	SendGeneralSwitch(domoticzID, unit, battery, value, level, label, "");
	_ApplySwitchTypeOnCreate(domoticzID, unit, isNew, switchType);
}

// Must be called with m_stateMutex already held.
// Sends only non-environmental (non-temp/hum/baro) sensors for one endpoint.
// Environmental sensors are aggregated and sent at the node level by _DetectAndSendNode.
// domoticzID = nodeId * 256 + endpointId so commands can be routed back to the
// correct endpoint via WriteToHardware (nodeId = domoticzID/256, endpointId = domoticzID%256).
void CMatter::_DetectAndSend(int nodeId, int endpointId)
{
	auto nodeIt = m_nodes.find(nodeId);
	if (nodeIt == m_nodes.end())
		return;
	auto epIt = nodeIt->second.endpoints.find(endpointId);
	if (epIt == nodeIt->second.endpoints.end())
		return;
	const auto& state = epIt->second;
	int battery = nodeIt->second.battery_pct;
	int domoticzID = nodeId * 256 + endpointId;

	if (state.hasPower && !state.hasEnergy)
		SendWattMeter(domoticzID, endpointId, battery, static_cast<float>(state.power_W), state.label);
	if (state.hasEnergy)
		SendKwhMeter(domoticzID, endpointId, battery, state.power_W, state.energy_kWh, state.label);

	if (state.hasLux)
		SendLuxSensor((uint8_t)nodeId, (uint8_t)endpointId, (uint8_t)battery, state.lux, state.label);

	if (state.hasOccupancy)
		SendGeneralSwitchInt(domoticzID, CHILD_OCCUPANCY, battery, state.occupied ? 1 : 0, 0, state.label, STYPE_Motion);

	if (state.hasOnOff)
	{
		if (state.hasColorControl)
		{
			char szDevID[16];
			snprintf(szDevID, sizeof(szDevID), "%08X", (unsigned int)domoticzID);
			bool isNewColorSwitch = m_sql.safe_query(
				"SELECT ID FROM DeviceStatus WHERE HardwareID=%d AND DeviceID='%q' AND Unit=%d",
				m_HwdID, szDevID, CHILD_ONOFF).empty();

			// Use ColorCapabilities bitmap (attr 0x400A) to determine true RGB support.
			// HS bit (0x0001) = device supports hue/saturation; XY alone is insufficient.
			bool supportsHS   = (state.colorCapabilities >= 0) && ((state.colorCapabilities & 0x0001) != 0);
			bool supportsTemp = (state.colorCapabilities >= 0)
			                    ? ((state.colorCapabilities & 0x0010) != 0)
			                    : state.hasColorTemp;
			uint8_t subtype;
			if (supportsHS && supportsTemp)
				subtype = sTypeColor_RGB_CW_WW_Z;
			else if (supportsHS)
				subtype = sTypeColor_RGB_CW_WW;
			else
				subtype = sTypeColor_CW_WW; // ColorTemp-only or XY-without-HS

			_tColorSwitch lcmd;
			lcmd.id      = (uint32_t)domoticzID;
			lcmd.dunit   = CHILD_ONOFF;
			lcmd.subtype = subtype;

			if (!state.onOff)
			{
				lcmd.command = Color_LedOff;
				lcmd.value   = 0;
			}
			else
			{
				lcmd.command = Color_SetColor;
				lcmd.value   = state.hasLevel
					? (uint32_t)std::round(state.level_pct)
					: 100U;

				int effMode = (state.colorMode >= 0) ? state.colorMode
				                                     : (state.hasColorHS ? 0 : (state.hasColorTemp ? 2 : 1));
				if (effMode == 0 && state.hasColorHS)
				{
					// Matter HS: hue 0-254 = 0-360°, saturation 0-254 = 0-100%
					int r = 0, g = 0, b = 0;
					hsb2rgb(state.colorHue * 360.0f / 254.0f, state.colorSaturation / 254.0f, 1.0f, r, g, b, 255.0);
					lcmd.color = _tColor((uint8_t)r, (uint8_t)g, (uint8_t)b, 0, 0, ColorModeRGB);
				}
				else if (effMode == 1 && state.hasColorXY)
				{
					uint8_t r = 0, g = 0, b = 0;
					_tColor::RgbFromXY(state.colorX, state.colorY, r, g, b);
					lcmd.color = _tColor(r, g, b, 0, 0, ColorModeRGB);
				}
				else if (state.hasColorTemp)
				{
					int mrange = std::max(1, state.colorTempMaxMireds - state.colorTempMinMireds);
					uint8_t ct = (uint8_t)std::max(0.0f, std::min(255.0f,
						(state.colorTempMireds - state.colorTempMinMireds) * 255.0f / mrange));
					lcmd.color = _tColor(ct, ColorModeTemp);
				}
				else
					lcmd.color = _tColor(255, ColorModeWhite);
			}

			m_mainworker.PushAndWaitRxMessage(this, (const uint8_t*)&lcmd,
				state.label.c_str(), battery, m_Name.c_str());
			_ApplySwitchTypeOnCreate(domoticzID, CHILD_ONOFF, isNewColorSwitch, STYPE_Dimmer);
		}
		else if (state.hasLevel)
			SendGeneralSwitchInt(domoticzID, CHILD_ONOFF, battery, state.onOff ? 1 : 0, (uint8_t)ground(state.level_pct), state.label, STYPE_Dimmer);
		else
			SendGeneralSwitchInt(domoticzID, CHILD_ONOFF, battery, state.onOff ? 1 : 0, 0, state.label, STYPE_OnOff);
	}

	if (state.hasContact)
		SendGeneralSwitchInt(domoticzID, CHILD_CONTACT, battery, state.contact ? 1 : 0, 0, state.label, STYPE_DoorContact);

	if (state.hasCO2)
		SendAirQualitySensor((uint8_t)nodeId, (uint8_t)endpointId, battery, state.co2_ppm, state.label);

	if (state.hasCO)
		SendCustomSensor(domoticzID, CHILD_CO, battery, state.co_ppm, state.label + " CO", "ppm");

	if (state.hasNO2)
		SendCustomSensor(domoticzID, CHILD_NO2, battery, state.no2_ppm, state.label + " NO2", "ppm");

	if (state.hasPM25)
		SendCustomSensor(domoticzID, CHILD_PM25, battery, state.pm25_ugm3, state.label + " PM2.5", "µg/m³");

	if (state.hasPM10)
		SendCustomSensor(domoticzID, CHILD_PM10, battery, state.pm10_ugm3, state.label + " PM10", "µg/m³");

	if (state.hasFlow)
		SendWaterflowSensor(domoticzID, CHILD_FLOW, battery, state.flow_lpm, state.label);

	if (state.hasLock)
		SendGeneralSwitchInt(domoticzID, CHILD_LOCK, battery, state.locked ? 1 : 0, 0, state.label, STYPE_DoorLock);

	if (state.hasBlind)
		SendPercentageSensor(domoticzID, CHILD_BLIND, battery, (float)state.blind_pct, state.label);

	if (state.hasSystemMode)
	{
		static const std::string SystemMode_Names   = "Off|Auto|Cool|Heat|EmergencyHeat|Precooling|FanOnly|Dry|Sleep";
		static const std::string SystemMode_Actions = "||||||||";
		int level = 0;
		for (int i = 0; i < SYSTEM_MODE_VALUES_SIZE; i++)
		{
			if (SYSTEM_MODE_VALUES[i] == state.systemMode) { level = i * 10; break; }
		}
		SendSelectorSwitch(domoticzID, CHILD_SYSTEM_MODE, std::to_string(level), state.label + " System Mode", 0, true, SystemMode_Names, SystemMode_Actions, false, "");
	}

	if (state.hasCtrlSeqOp)
	{
		static const std::string CtrlSeq_Names   = "CoolingOnly|CoolingWithReheat|HeatingOnly|HeatingWithReheat|CoolingAndHeating|CoolingAndHeatingWithReheat";
		static const std::string CtrlSeq_Actions = "|||||";
		SendSelectorSwitch(domoticzID, CHILD_CTRL_SEQ, std::to_string(state.ctrlSeqOp * 10), state.label + " Control Sequence", 0, true, CtrlSeq_Names, CtrlSeq_Actions, false, "");
	}

	if (!state.presets.empty())
	{
		// PresetScenarioEnum → display name (matching HASS conventions)
		static const std::pair<int, const char*> scenarioNames[] = {
			{1, "Home"}, {2, "Away"}, {3, "Sleep"}, {4, "Wake"}, {5, "Vacation"}, {6, "GoingToSleep"}
		};
		auto scenarioName = [](int scenario) -> std::string {
			for (const auto& p : scenarioNames)
				if (p.first == scenario) return p.second;
			return "";
		};

		std::string names = "None";
		std::string actions = "";
		int activeLevel = 0;
		for (int i = 0; i < (int)state.presets.size(); i++)
		{
			const auto& p = state.presets[i];
			std::string name = p.name.empty() ? scenarioName(p.scenario) : p.name;
			if (name.empty()) name = "Preset" + std::to_string(i + 1);
			names  += "|" + name;
			actions += "|";
			if (!p.handle.empty() && p.handle == state.activePresetHandle)
				activeLevel = (i + 1) * 10;
		}
		SendSelectorSwitch(domoticzID, CHILD_PRESET, std::to_string(activeLevel), state.label + " Preset", 0, true, names, actions, false, "");
	}

	if (state.hasVolt)
		SendVoltageSensor(domoticzID, endpointId, battery, state.voltage_V, state.label);

	if (state.hasCurrent)
		SendCurrentSensor(domoticzID, battery, state.current_A, 0, 0, state.label);

	if (state.hasFrequency)
		SendCustomSensor(domoticzID, CHILD_FREQ, battery, static_cast<float>(state.frequency_Hz), state.label + " Frequency", "Hz");

	if (state.hasSwitch)
		SendGeneralSwitchInt(domoticzID, CHILD_SWITCH, battery, state.switch_on ? 1 : 0, 0, state.label, STYPE_PushOn);
}

std::string CMatter::ExtractLabel(const Json::Value& attrs, int nodeId, int endpointId) const
{
	// Helper: try a path in attrs, return the string value if non-empty
	auto tryPath = [&](const std::string& p) -> std::string {
		if (attrs.isMember(p)) {
			const Json::Value& v = attrs[p];
			if (v.isString()) return v.asString();
		}
		return {};
	};

	std::string ep = std::to_string(endpointId);
	std::string label;

	// cluster 0x0039 (57), attr 1 — BridgedDeviceBasicInformation: NodeLabel
	label = tryPath(ep + "/57/1");
	if (!label.empty()) return label;

	// cluster 0x0028 (40), attr 5 — BasicInformation: NodeLabel (user-set)
	label = tryPath(ep + "/40/5");
	if (!label.empty()) return label;

	// cluster 0x0028 (40), attr 3 — BasicInformation: ProductName
	label = tryPath(ep + "/40/3");
	if (!label.empty()) return label;

	// For non-zero endpoints the basic info lives on endpoint 0 — fall back there
	if (endpointId != 0)
	{
		label = tryPath("0/57/1");
		if (!label.empty()) return label;
		label = tryPath("0/40/5");
		if (!label.empty()) return label;
		label = tryPath("0/40/3");
		if (!label.empty()) return label;
	}

	return "Matter-" + std::to_string(nodeId) + "-" + std::to_string(endpointId);
}

void CMatter::SetSetpoint(int idx, float value)
{
	if (!m_bConnected)
	{
		Log(LOG_STATUS, "SetSetpoint: not connected to Matter server");
		return;
	}

	// idx is ID4 from the device record; for setpoint sensors we store nodeId there
	// (via SendSetPointSensor(0, 0, 0, nodeId, 1, ...))
	int nodeId = idx;

	// Convert to Celsius if the user has Fahrenheit configured
	float temp_celsius = value;
	if (m_sql.m_tempsign[0] == 'F')
		temp_celsius = static_cast<float>(ConvertToCelsius(value));

	// Find the endpoint that has the thermostat cluster and read its limits
	int endpointId = -1;
	float minTemp = 5.0f, maxTemp = 30.0f;
	{
		std::lock_guard<std::mutex> lock(m_stateMutex);
		auto nodeIt = m_nodes.find(nodeId);
		if (nodeIt == m_nodes.end())
		{
			Log(LOG_STATUS, "SetSetpoint: node %d not found", nodeId);
			return;
		}
		for (const auto& kv : nodeIt->second.endpoints)
		{
			if (kv.second.hasSetpoint)
			{
				endpointId = kv.first;
				// Use cooling limits for Cool (3) and Auto/HEAT_COOL (1) modes
				const auto& ep = kv.second;
				bool isCooling = (ep.systemMode == 3 || ep.systemMode == 1);
				minTemp = isCooling ? ep.minCoolSetpoint_C : ep.minHeatSetpoint_C;
				maxTemp = isCooling ? ep.maxCoolSetpoint_C : ep.maxHeatSetpoint_C;
				break;
			}
		}
	}

	if (endpointId < 0)
	{
		Log(LOG_STATUS, "SetSetpoint: no thermostat endpoint on node %d", nodeId);
		return;
	}

	if (temp_celsius < minTemp)
	{
		Log(LOG_STATUS, "SetSetpoint: clamping %.1f°C to min %.1f°C", temp_celsius, minTemp);
		temp_celsius = minTemp;
	}
	else if (temp_celsius > maxTemp)
	{
		Log(LOG_STATUS, "SetSetpoint: clamping %.1f°C to max %.1f°C", temp_celsius, maxTemp);
		temp_celsius = maxTemp;
	}

	// Matter thermostat cluster uses 0.01 °C units
	int matter_value = static_cast<int>(std::round(temp_celsius * 100.0f));

	// Write OccupiedHeatingSetpoint (cluster 0x0201, attr 0x0011)
	Json::Value args;
	args["node_id"]        = nodeId;
	args["attribute_path"] = std::to_string(endpointId) + "/" + std::to_string(CLUSTER_THERMOSTAT) + "/" + std::to_string(ATTR_OCCUPIED_HEATING_SETPOINT);
	args["value"]          = matter_value;
	SendCommand("write_attribute", args);
}

bool CMatter::WriteToHardware(const char* pdata, unsigned char length)
{
	if (!pdata || length == 0) return false;

	if (!m_bConnected)
	{
		Log(LOG_STATUS, "WriteToHardware: not connected to Matter server");
		return false;
	}

	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);

	if (pCmd->ICMND.packettype == pTypeColorSwitch)
	{
		const _tColorSwitch* pColor = reinterpret_cast<const _tColorSwitch*>(pdata);
		int domoticzID = (int)pColor->id;
		int nodeId     = domoticzID / 256;
		int endpointId = domoticzID % 256;

		int levelMin = 1, levelMax = 254, colorTempMin = 153, colorTempMax = 500;
		bool supportsHS = false;
		{
			std::lock_guard<std::mutex> lock(m_stateMutex);
			auto nodeIt = m_nodes.find(nodeId);
			if (nodeIt == m_nodes.end() || nodeIt->second.endpoints.find(endpointId) == nodeIt->second.endpoints.end())
			{
				Log(LOG_STATUS, "WriteToHardware: node %d endpoint %d not found", nodeId, endpointId);
				return false;
			}
			const auto& ep = nodeIt->second.endpoints.at(endpointId);
			levelMin     = ep.levelMinLevel;
			levelMax     = ep.levelMaxLevel;
			colorTempMin = ep.colorTempMinMireds;
			colorTempMax = ep.colorTempMaxMireds;
			supportsHS   = (ep.colorCapabilities >= 0) && ((ep.colorCapabilities & 0x0001) != 0);
		}

		auto sendOnOff = [&](const std::string& cmd) {
			Json::Value args;
			args["node_id"]      = nodeId;
			args["endpoint_id"]  = endpointId;
			args["cluster_id"]   = CLUSTER_ON_OFF;
			args["command_name"] = cmd;
			args["payload"]      = Json::Value(Json::objectValue);
			SendCommand("device_command", args);
		};

		auto sendLevel = [&](uint32_t level100) {
			int range        = std::max(1, levelMax - levelMin);
			int matter_level = levelMin + (int)std::round((double)level100 * range / 100.0);
			matter_level     = std::max(levelMin, std::min(levelMax, matter_level));
			Json::Value args;
			args["node_id"]      = nodeId;
			args["endpoint_id"]  = endpointId;
			args["cluster_id"]   = CLUSTER_LEVEL_CONTROL;
			args["command_name"] = "move_to_level_with_on_off";
			Json::Value payload;
			payload["level"]           = matter_level;
			payload["optionsMask"]     = 1;
			payload["optionsOverride"] = 1;
			payload["transition_time"] = 0;
			args["payload"] = payload;
			SendCommand("device_command", args);
		};

		if (pColor->command == Color_LedOff)   { sendOnOff("off"); return true; }
		if (pColor->command == Color_LedOn)    { sendOnOff("on");  return true; }

		if (pColor->command == Color_SetBrightnessLevel)
		{
			sendLevel(pColor->value);
			return true;
		}

		if (pColor->command == Color_SetColor)
		{
			sendLevel(pColor->value);

			if (pColor->color.mode == ColorModeTemp)
			{
				int mrange = std::max(1, colorTempMax - colorTempMin);
				int mireds = colorTempMin + (int)std::round((double)pColor->color.t * mrange / 255.0);
				mireds     = std::max(colorTempMin, std::min(colorTempMax, mireds));
				Json::Value args;
				args["node_id"]      = nodeId;
				args["endpoint_id"]  = endpointId;
				args["cluster_id"]   = CLUSTER_COLOR_CONTROL;
				args["command_name"] = "move_to_color_temperature";
				Json::Value payload;
				payload["colorTemperatureMireds"] = mireds;
				payload["transitionTime"]         = 0;
				payload["optionsMask"]            = 1;
				payload["optionsOverride"]        = 1;
				args["payload"] = payload;
				SendCommand("device_command", args);
			}
			else if (pColor->color.mode == ColorModeRGB || pColor->color.mode == ColorModeCustom)
			{
				if (supportsHS)
				{
					// Prefer move_to_hue_and_saturation on devices that support it (HS bit in ColorCapabilities)
					float hsbvals[3] = {0, 0, 0};
					rgb2hsb(pColor->color.r, pColor->color.g, pColor->color.b, hsbvals);
					// hsbvals[0]=hue 0-1, hsbvals[1]=saturation 0-1; Matter range is 0-254
					int hue = std::max(0, std::min(254, (int)std::round(hsbvals[0] * 254.0f)));
					int sat = std::max(0, std::min(254, (int)std::round(hsbvals[1] * 254.0f)));
					Json::Value args;
					args["node_id"]      = nodeId;
					args["endpoint_id"]  = endpointId;
					args["cluster_id"]   = CLUSTER_COLOR_CONTROL;
					args["command_name"] = "move_to_hue_and_saturation";
					Json::Value payload;
					payload["hue"]             = hue;
					payload["saturation"]      = sat;
					payload["transitionTime"]  = 0;
					payload["optionsMask"]     = 1;
					payload["optionsOverride"] = 1;
					args["payload"] = payload;
					SendCommand("device_command", args);
				}
				else
				{
					double x = 0, y = 0, Y = 0;
					_tColor::XYFromRGB(pColor->color.r, pColor->color.g, pColor->color.b, x, y, Y);
					if (std::isnan(x) || std::isinf(x) || std::isnan(y) || std::isinf(y))
					{
						Log(LOG_ERROR, "Matter: XY color conversion produced invalid values (r=%d g=%d b=%d)",
						    pColor->color.r, pColor->color.g, pColor->color.b);
						return false;
					}
					int cx = std::max(0, std::min(65535, (int)std::round(x * 65536)));
					int cy = std::max(0, std::min(65535, (int)std::round(y * 65536)));
					Json::Value args;
					args["node_id"]      = nodeId;
					args["endpoint_id"]  = endpointId;
					args["cluster_id"]   = CLUSTER_COLOR_CONTROL;
					args["command_name"] = "move_to_color";
					Json::Value payload;
					payload["colorX"]          = cx;
					payload["colorY"]          = cy;
					payload["transitionTime"]  = 0;
					payload["optionsMask"]     = 1;
					payload["optionsOverride"] = 1;
					args["payload"] = payload;
					SendCommand("device_command", args);
				}
			}
			return true;
		}
		return false;
	}

	// Selector switch — SystemMode
	if (pCmd->ICMND.packettype == pTypeGeneralSwitch && pCmd->ICMND.subtype == sSwitchTypeSelector)
	{
		const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		if (pSwitch->unitcode == CHILD_SYSTEM_MODE)
		{
			int index = pSwitch->level / 10;
			if (index < 0 || index >= SYSTEM_MODE_VALUES_SIZE)
				return false;
			int enumValue = SYSTEM_MODE_VALUES[index];
			int domoticzID = (int)pSwitch->id;
			int nodeId     = domoticzID / 256;
			int endpointId = domoticzID % 256;
			{
				std::lock_guard<std::mutex> lock(m_stateMutex);
				auto nodeIt = m_nodes.find(nodeId);
				if (nodeIt == m_nodes.end() || nodeIt->second.endpoints.find(endpointId) == nodeIt->second.endpoints.end())
				{
					Log(LOG_STATUS, "WriteToHardware: node %d endpoint %d not found", nodeId, endpointId);
					return false;
				}
			}
			Json::Value args;
			args["node_id"]        = nodeId;
			args["attribute_path"] = std::to_string(endpointId) + "/" + std::to_string(CLUSTER_THERMOSTAT) + "/" + std::to_string(ATTR_SYSTEM_MODE);
			args["value"]          = enumValue;
			SendCommand("write_attribute", args);
			return true;
		}
		if (pSwitch->unitcode == CHILD_CTRL_SEQ)
		{
			int enumValue  = pSwitch->level / 10;
			if (enumValue < 0 || enumValue > 5)
				return false;
			int domoticzID = (int)pSwitch->id;
			int nodeId     = domoticzID / 256;
			int endpointId = domoticzID % 256;
			Json::Value args;
			args["node_id"]        = nodeId;
			args["attribute_path"] = std::to_string(endpointId) + "/" + std::to_string(CLUSTER_THERMOSTAT) + "/" + std::to_string(ATTR_CTRL_SEQ_OF_OPERATION);
			args["value"]          = enumValue;
			SendCommand("write_attribute", args);
			return true;
		}
		if (pSwitch->unitcode == CHILD_PRESET)
		{
			int domoticzID = (int)pSwitch->id;
			int nodeId     = domoticzID / 256;
			int endpointId = domoticzID % 256;
			// level 0 = None (clear preset), level 10/20/... = preset by index
			int presetIndex = pSwitch->level / 10 - 1;
			std::string handle;
			{
				std::lock_guard<std::mutex> lock(m_stateMutex);
				auto nodeIt = m_nodes.find(nodeId);
				if (nodeIt == m_nodes.end() || nodeIt->second.endpoints.find(endpointId) == nodeIt->second.endpoints.end())
				{
					Log(LOG_STATUS, "WriteToHardware: node %d endpoint %d not found", nodeId, endpointId);
					return false;
				}
				const auto& ep = nodeIt->second.endpoints.at(endpointId);
				if (presetIndex < -1 || presetIndex >= (int)ep.presets.size())
				{
					Log(LOG_STATUS, "WriteToHardware: preset index %d out of range (have %d presets)", presetIndex, (int)ep.presets.size());
					return false;
				}
				if (presetIndex >= 0)
					handle = ep.presets[presetIndex].handle;
			}
			Json::Value args;
			args["node_id"]        = nodeId;
			args["attribute_path"] = std::to_string(endpointId) + "/" + std::to_string(CLUSTER_THERMOSTAT) + "/" + std::to_string(ATTR_ACTIVE_PRESET_HANDLE);
			// bytes are encoded as hex strings in the matter.js WebSocket protocol
			args["value"] = handle.empty() ? Json::Value(Json::nullValue) : Json::Value(handle);
			SendCommand("write_attribute", args);
			return true;
		}
		return false;
	}

	if (pCmd->ICMND.packettype != pTypeGeneralSwitch)
		return false;

	const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
	int domoticzID = (int)pSwitch->id;
	int nodeId     = domoticzID / 256;
	int endpointId = domoticzID % 256;
	uint8_t cmnd   = pSwitch->cmnd;

	int levelMin = 1, levelMax = 254;
	{
		std::lock_guard<std::mutex> lock(m_stateMutex);
		auto nodeIt = m_nodes.find(nodeId);
		if (nodeIt == m_nodes.end() || nodeIt->second.endpoints.find(endpointId) == nodeIt->second.endpoints.end())
		{
			Log(LOG_STATUS, "WriteToHardware: node %d endpoint %d not found", nodeId, endpointId);
			return false;
		}
		const auto& ep = nodeIt->second.endpoints.at(endpointId);
		levelMin = ep.levelMinLevel;
		levelMax = ep.levelMaxLevel;
	}

	if (cmnd == gswitch_sSetLevel)
	{
		int range        = std::max(1, levelMax - levelMin);
		int matter_level = levelMin + (int)std::round(pSwitch->level * range / 100.0);
		matter_level     = std::max(levelMin, std::min(levelMax, matter_level));

		Json::Value args;
		args["node_id"]      = nodeId;
		args["endpoint_id"]  = endpointId;
		args["cluster_id"]   = 8;
		args["command_name"] = "move_to_level_with_on_off";
		Json::Value payload;
		payload["level"]           = matter_level;
		payload["optionsMask"]     = 1;
		payload["optionsOverride"] = 1;
		payload["transition_time"] = 0;
		args["payload"] = payload;
		SendCommand("device_command", args);
	}
	else
	{
		std::string cmd;
		if (cmnd == gswitch_sOn)
			cmd = "on";
		else if (cmnd == gswitch_sOff)
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
