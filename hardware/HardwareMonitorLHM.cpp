#include "stdafx.h"

#include "HardwareMonitorLHM.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../webserver/Base64.h"

static bool IsValidLHMAddress(const std::string& addr)
{
	if (addr.empty())
		return false;
	// Reject characters that could cause URL injection
	for (char c : addr)
	{
		if (c == '/' || c == '@' || c == '?' || c == '#' || c == ' ')
			return false;
	}
	if (addr.find("://") != std::string::npos)
		return false;
	return true;
}

CHardwareMonitorLHM::CHardwareMonitorLHM(const int ID, const std::string& address, uint16_t port, const std::string& username, const std::string& password, bool bExtendedSensors)
	: CHardwareMonitorBase(ID)
	, m_szLHMAddress(IsValidLHMAddress(address) ? address : "127.0.0.1")
	, m_usLHMPort(port == 0 ? 8085 : port)
	, m_szUsername(username)
	, m_szPassword(password)
	, m_bExtendedSensors(bExtendedSensors)
{
	if (!address.empty() && !IsValidLHMAddress(address))
		Log(LOG_ERROR, "Invalid LHM address '%s', using default 127.0.0.1", address.c_str());
}

void CHardwareMonitorLHM::FetchData()
{
	FetchLHMData();
}

static constexpr int MAX_JSON_DEPTH = 10;
static constexpr int MAX_SENSORS_PER_TYPE = 100;

// Detect storage hardware from LHM SensorId path
static bool IsStorageSensor(const std::string& sensorId)
{
	return (sensorId.find("/nvme/") != std::string::npos
		|| sensorId.find("/hdd/") != std::string::npos
		|| sensorId.find("/ata/") != std::string::npos
		|| sensorId.find("/ssd/") != std::string::npos);
}

// Detect GPU hardware from LHM SensorId path
static bool IsGPUSensor(const std::string& sensorId)
{
	return (sensorId.find("/gpu") != std::string::npos);
}

// Detect CPU hardware from LHM SensorId path
static bool IsCPUSensor(const std::string& sensorId)
{
	return (sensorId.find("/intelcpu/") != std::string::npos
		|| sensorId.find("/amdcpu/") != std::string::npos
		|| sensorId.find("/cpu/") != std::string::npos);
}

// Detect RAM hardware from LHM SensorId path
static bool IsRAMSensor(const std::string& sensorId)
{
	return (sensorId.find("/ram/") != std::string::npos);
}

bool CHardwareMonitorLHM::FetchLHMData()
{
	std::string sURL = "http://" + m_szLHMAddress + ":" + std::to_string(m_usLHMPort) + "/data.json";
	std::string sResult;

	std::vector<std::string> ExtraHeaders;
	if (!m_szUsername.empty())
	{
		std::string sAuth = m_szUsername + ":" + m_szPassword;
		ExtraHeaders.push_back("Authorization: Basic " + base64_encode(sAuth));
	}

	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult, true))
	{
		Log(LOG_ERROR, "Failed to connect to Libre Hardware Monitor at %s. "
			"Ensure LHM is running and 'Remote Web Server' is enabled in Options > Remote Web Server.",
			sURL.c_str());
		return false;
	}

	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		Log(LOG_ERROR, "Failed to parse JSON from Libre Hardware Monitor at %s", sURL.c_str());
		return false;
	}

	// Normal mode indices (always active)
	int tempIdx = 0, fanIdx = 0;
	int cpuLoadIdx = 0, memLoadIdx = 0, memDataIdx = 0, storageDataIdx = 0;
	// Extended mode indices (only used when m_bExtendedSensors is true)
	int storageTempIdx = 0, gpuFanIdx = 0;
	int loadIdx = 0, voltIdx = 0;
	int powerIdx = 0, clockIdx = 0, dataIdx = 0, smallDataIdx = 0;
	int throughputIdx = 0, levelIdx = 0, factorIdx = 0;

	ParseLHMNode(root, "",
		tempIdx, fanIdx, cpuLoadIdx, memLoadIdx, memDataIdx, storageDataIdx,
		storageTempIdx, gpuFanIdx, loadIdx, voltIdx,
		powerIdx, clockIdx, dataIdx, smallDataIdx,
		throughputIdx, levelIdx, factorIdx);

	bool bAnySensor = (tempIdx + fanIdx + cpuLoadIdx + memLoadIdx + memDataIdx + storageDataIdx
		+ storageTempIdx + gpuFanIdx + loadIdx + voltIdx
		+ powerIdx + clockIdx + dataIdx + smallDataIdx
		+ throughputIdx + levelIdx + factorIdx) > 0;

	if (!bAnySensor)
		Log(LOG_STATUS, "Connected to Libre Hardware Monitor but no supported sensors found");

	return true;
}

void CHardwareMonitorLHM::ParseLHMNode(
	const Json::Value& node,
	const std::string& hwDevice,
	int& tempIdx, int& fanIdx, int& cpuLoadIdx, int& memLoadIdx,
	int& memDataIdx, int& storageDataIdx,
	int& storageTempIdx, int& gpuFanIdx,
	int& loadIdx, int& voltIdx, int& powerIdx, int& clockIdx,
	int& dataIdx, int& smallDataIdx, int& throughputIdx,
	int& levelIdx, int& factorIdx, int depth)
{
	if (!node.isObject())
		return;

	if (depth > MAX_JSON_DEPTH)
	{
		Log(LOG_ERROR, "LHM JSON tree depth exceeded maximum of %d, aborting parse", MAX_JSON_DEPTH);
		return;
	}

	// Determine if this is a hardware device node (has HardwareId)
	std::string currentDevice = hwDevice;
	if (node.isMember("HardwareId") && node.isMember("Text"))
		currentDevice = node["Text"].asString();

	// Check if this is a leaf sensor node (has Type, SensorId, and empty Children)
	if (node.isMember("Type") && node.isMember("SensorId") && node.isMember("Children"))
	{
		const Json::Value& children = node["Children"];
		if (children.isArray() && children.empty())
		{
			std::string sType = node["Type"].asString();
			std::string sName = node.isMember("Text") ? node["Text"].asString() : "";
			std::string sSensorId = node["SensorId"].asString();

			// Use RawValue for most consistent parsing (base units for throughput etc.)
			std::string sValue;
			if (node.isMember("RawValue") && !node["RawValue"].isNull())
				sValue = node["RawValue"].asString();
			else if (node.isMember("Value") && !node["Value"].isNull())
				sValue = node["Value"].asString();
			else
				return;

			// Extract numeric part: "42,0 °C" → "42,0"
			size_t spacePos = sValue.find(' ');
			std::string numericStr = (spacePos != std::string::npos) ? sValue.substr(0, spacePos) : sValue;

			// Handle European comma decimal separator: "42,0" → "42.0"
			stdreplace(numericStr, ",", ".");

			// Build sensor name with hardware device prefix for disambiguation
			std::string devName = sName;
			if (!currentDevice.empty() && !sName.empty())
				devName = currentDevice + " - " + sName;
			else if (!currentDevice.empty())
				devName = currentDevice;

			// Classify hardware source from SensorId
			bool bIsStorage = IsStorageSensor(sSensorId);
			bool bIsGPU = IsGPUSensor(sSensorId);
			bool bIsCPU = IsCPUSensor(sSensorId);
			bool bIsRAM = IsRAMSensor(sSensorId);

			// --- Normal mode sensors (always polled) ---
			// Offset map:
			//   Temperature (CPU/GPU/MB): 1000   Fan (CPU/system):       1200
			//   CPULoad:                  2500   MemoryLoad:             2600
			//   MemoryData:               2700   StorageData:            2800
			//
			// --- Extended mode adds (separate offsets, no ID conflicts) ---
			//   StorageTemperature: 2300   GPUFan:    2400
			//   Load:    1100   Voltage: 1300   Power:   1600   Clock:     1700
			//   Data:    1800   SmallData: 1900  Throughput: 2000
			//   Level:   2100   Factor:  2200

			if (sType == "Temperature")
			{
				if (bIsStorage)
				{
					// Storage temps: extended only, separate offset
					if (m_bExtendedSensors && storageTempIdx < MAX_SENSORS_PER_TYPE)
						UpdateSystemSensor("StorageTemperature", storageTempIdx++, devName, numericStr);
				}
				else if (tempIdx < MAX_SENSORS_PER_TYPE)
				{
					// CPU, GPU, Motherboard temps: always
					UpdateSystemSensor("Temperature", tempIdx++, devName, numericStr);
				}
			}
			else if (sType == "Fan")
			{
				if (bIsGPU)
				{
					// GPU fans: extended only, separate offset
					if (m_bExtendedSensors && gpuFanIdx < MAX_SENSORS_PER_TYPE)
						UpdateSystemSensor("GPUFan", gpuFanIdx++, devName, numericStr);
				}
				else if (fanIdx < MAX_SENSORS_PER_TYPE)
				{
					// CPU and system fans: always
					UpdateSystemSensor("Fan", fanIdx++, devName, numericStr);
				}
			}
			else if (sType == "Load")
			{
				if (bIsCPU && cpuLoadIdx < MAX_SENSORS_PER_TYPE)
				{
					// CPU usage: always (own offset)
					UpdateSystemSensor("CPULoad", cpuLoadIdx++, devName, numericStr);
				}
				else if (bIsRAM && memLoadIdx < MAX_SENSORS_PER_TYPE)
				{
					// Memory usage %: always (own offset)
					UpdateSystemSensor("MemoryLoad", memLoadIdx++, devName, numericStr);
				}
				else if (m_bExtendedSensors && loadIdx < MAX_SENSORS_PER_TYPE)
				{
					// GPU load, network utilization, etc.: extended only
					UpdateSystemSensor("Load", loadIdx++, devName, numericStr);
				}
			}
			else if (sType == "Data")
			{
				if (bIsRAM && memDataIdx < MAX_SENSORS_PER_TYPE)
				{
					// Memory used/available GB: always (own offset)
					UpdateSystemSensor("MemoryData", memDataIdx++, devName, numericStr);
				}
				else if (bIsStorage && storageDataIdx < MAX_SENSORS_PER_TYPE)
				{
					// Disk used/available GB: always (own offset)
					UpdateSystemSensor("StorageData", storageDataIdx++, devName, numericStr);
				}
				else if (m_bExtendedSensors && dataIdx < MAX_SENSORS_PER_TYPE)
				{
					// Other data sensors: extended only
					UpdateSystemSensor("Data", dataIdx++, devName, numericStr);
				}
			}
			else if (m_bExtendedSensors)
			{
				// All remaining types are extended-only
				if (sType == "Voltage" && voltIdx < MAX_SENSORS_PER_TYPE)
				{
					UpdateSystemSensor("Voltage", voltIdx++, devName, numericStr);
				}
				else if (sType == "Power" && powerIdx < MAX_SENSORS_PER_TYPE)
				{
					UpdateSystemSensor("Power", powerIdx++, devName, numericStr);
				}
				else if (sType == "Clock" && clockIdx < MAX_SENSORS_PER_TYPE)
				{
					UpdateSystemSensor("Clock", clockIdx++, devName, numericStr);
				}
				else if (sType == "SmallData" && smallDataIdx < MAX_SENSORS_PER_TYPE)
				{
					UpdateSystemSensor("SmallData", smallDataIdx++, devName, numericStr);
				}
				else if (sType == "Throughput" && throughputIdx < MAX_SENSORS_PER_TYPE)
				{
					// RawValue is in B/s, convert to mbps (megabits per second)
					float bytesPerSec = static_cast<float>(atof(numericStr.c_str()));
					float mbps = (bytesPerSec * 8.0F) / 1000000.0F;
					char mbpsBuf[32];
					snprintf(mbpsBuf, sizeof(mbpsBuf), "%.2f", mbps);
					UpdateSystemSensor("Throughput", throughputIdx++, devName, std::string(mbpsBuf));
				}
				else if (sType == "Level" && levelIdx < MAX_SENSORS_PER_TYPE)
				{
					UpdateSystemSensor("Level", levelIdx++, devName, numericStr);
				}
				else if (sType == "Factor" && factorIdx < MAX_SENSORS_PER_TYPE)
				{
					UpdateSystemSensor("Factor", factorIdx++, devName, numericStr);
				}
			}

			return;
		}
	}

	// Recurse into children
	if (node.isMember("Children"))
	{
		const Json::Value& children = node["Children"];
		if (children.isArray())
		{
			for (const auto& child : children)
			{
				ParseLHMNode(child, currentDevice,
					tempIdx, fanIdx, cpuLoadIdx, memLoadIdx, memDataIdx, storageDataIdx,
					storageTempIdx, gpuFanIdx, loadIdx, voltIdx,
					powerIdx, clockIdx, dataIdx, smallDataIdx,
					throughputIdx, levelIdx, factorIdx, depth + 1);
			}
		}
	}
}
