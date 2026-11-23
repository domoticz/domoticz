#pragma once

// Native Shelly (Gen1 and Gen2+) hardware driver. One instance manages many devices (nodes).

#include <chrono>
#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
#include "DomoticzHardware.h"
#include "ShellyConnection.h"
#include "ShellyMapping.h"
#include "ShellyScan.h"
#include "ShellyTypes.h"

class CShelly;

// The last color a color light reported. A brightness command only carries the brightness, the color for the Shelly request comes from here.
struct ShellyColorMemo
{
	bool known = false;
	int red = 0;
	int green = 0;
	int blue = 0;
	int white = -1;
	bool whiteMode = false;
	int level = -1;
	bool on = false;
	time_t time = 0;
};

// Keeps one Shelly hardware instance alive while a web command uses it: the destructor of the driver waits until
// every lease is released, and no new lease is handed out once the destructor has started. Holding a lease must
// be short: one command, never across user interaction. A command may include network I/O (AddNode holds its
// lease during its device probe, at most the probe timeout), so stopping the hardware can wait for that long.
class ShellyLease
{
      public:
	ShellyLease() = default;
	ShellyLease(ShellyLease&& other) noexcept;
	ShellyLease& operator=(ShellyLease&& other) noexcept;
	ShellyLease(const ShellyLease&) = delete;
	ShellyLease& operator=(const ShellyLease&) = delete;
	~ShellyLease();

	// Empty lease when no Shelly hardware with this id exists (or it is being destroyed).
	static ShellyLease Acquire(int hardwareId);
	// Same, for the id as given in a web request ("idx"). Empty for anything that is not a plain number.
	static ShellyLease Acquire(const std::string& hardwareId);

	CShelly* operator->() const
	{
		return m_hardware;
	}
	explicit operator bool() const
	{
		return m_hardware != nullptr;
	}

      private:
	void Release();
	CShelly* m_hardware = nullptr;
};

class CShelly : public CDomoticzHardwareBase, public IShellyEventSink
{
      public:
	CShelly(int id, const std::string& username, const std::string& password, int pollIntervalSec, const ShellyCoiotOptions& coiot = ShellyCoiotOptions());
	~CShelly() override;

	bool WriteToHardware(const char* pdata, unsigned char length) override;

	// Node management (thread safe). AddNode probes the device and rejects anything that is not a Shelly,
	// anything that has no client for its generation, and anything whose connection cannot be started.
	// Network I/O never happens under the driver lock. The address is intentionally not restricted to private
	// ranges (admin only, host names allowed, no redirects followed, fixed request path), unlike the scan.
	bool AddNode(const std::string& ip, int port, const std::string& name);
	bool RemoveNode(const std::string& nodeId);
	bool RenameNode(const std::string& nodeId, const std::string& newName);
	void RemoveAllNodes();
	std::vector<ShellyNode> GetNodes();
	bool FindNode(const std::string& nodeId, ShellyNode& node);

	// Finds the node that owns the Domoticz devices with this node key (see ShellyMapping.h, ShellyNodeKey). False when
	// there is none. Used to route a command from a device back to its node, see ShellyRouteDevice.
	bool FindNodeByKey(uint32_t nodeKey, ShellyNode& node);

	// Subnet scan (see ShellyScan.h). Runs in the background and only collects candidates, nothing is added.
	// `range` is expanded with ShellyExpandScanRange. Returns false with a message when the range is invalid,
	// a scan is already running or the hardware is not running.
	bool StartScan(const std::string& range, int port, std::string& error);
	void CancelScan();
	ShellyScanStatus GetScanStatus();

	// Execute a control request on a node via its connection.
	ShellyResult SendShellyCommand(const std::string& nodeId, const ShellyCommand& command);

	// IShellyEventSink
	void OnShellyStatus(const ShellyNode& node, const Json::Value& status) override;
	void OnShellyAvailability(const ShellyNode& node, bool online) override;
	void OnShellyNodeInfo(const ShellyNode& node, const Json::Value& info) override;
	void OnShellyEvent(const ShellyNode& node, const Json::Value& event) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	void PollAll();
	void PollRequested();
	void NoteGen1PollResult(const ShellyNode& node, const ShellyResult& result);
	void RequestRepoll(const std::string& nodeId);
	std::shared_ptr<std::mutex> GetApplyMutex(const std::string& nodeId);
	void RetryMissingConnections();
	void LoadNodes();
	void SaveNodes();
	enum class StartResult
	{
		Started,
		Skipped, // stored elsewhere, removed, changed or the hardware is stopping: not a failure
		Failed
	};
	std::shared_ptr<IShellyConnection> CreateAndStartConnection(const ShellyNode& node, std::string& failure);
	StartResult StartConnection(const ShellyNode& node, bool logFailure = true);
	std::vector<ShellyNode>::iterator FindNodeIt(const std::string& nodeId);

	// Device mapping (Shelly.cpp). The abstract mapping is in ShellyMapping.h; these are the SQL facing parts.
	bool IsManagedNode(const std::string& nodeId);
	void EnsureNodeInfo(const std::shared_ptr<IShellyConnection>& connection);
	void ApplyStatus(const ShellyNode& node, const Json::Value& status, const ShellyNodeConfig& config);
	void ApplyDeviceUpdate(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplySwitch(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplyBlind(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplyColor(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplyThermostat(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplyInput(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplyBinary(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplySensor(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device);
	void ApplyContact(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device, int switchType);
	void FireButton(const ShellyNode& node, uint8_t unit, int level, const std::string& name);
	void EnsureButtonSelector(const ShellyNode& node, uint8_t unit, const std::string& name);
	// Temperature, humidity and temperature+humidity sensors have a 16 bit id: false (logged once) when another node owns it.
	bool ClaimTemperatureId(const ShellyNode& node);
	// Forgets what was last written for a device so the next status is written even when it equals the previous one.
	void InvalidateDedupe(const ShellyNode& node, uint8_t unit);
	bool GetColorMemo(const ShellyNode& node, uint8_t unit, ShellyColorMemo& memo);
	bool IsSleepyNode(const std::string& nodeId);
	bool IsRedundantMeterUpdate(const ShellyNode& node, const ShellyDeviceUpdate& device);
	bool DeviceRowExists(const std::string& deviceId, uint8_t unit, int deviceType);
	void SetNewDeviceSwitchType(const ShellyNode& node, uint8_t unit, const std::string& deviceId, int deviceType, int switchType, const std::function<void()>& sendUpdate);
	void RenameDefaultNamedDevices(const ShellyNode& node, const Json::Value& status, const ShellyNodeConfig& oldConfig, const ShellyNodeConfig& newConfig);
	void ForgetNodeState(const std::string& nodeId);
	void DeleteNodeDevices(const std::string& nodeId, const std::vector<ShellyNode>& remaining);
	std::vector<std::string> FindNodeDeviceIds(const std::string& nodeId, const std::vector<ShellyNode>& remaining);
	void ForgetAllNodeStates();

      private:
	const int m_PollInterval;
	ShellyCredentials m_Credentials;
	const ShellyCoiotOptions m_Coiot;
	std::mutex m_lifecycleMutex; // serializes StartHardware/StopHardware (and the destructor); never taken by anything else
	std::shared_ptr<std::thread> m_thread; // guarded by m_lifecycleMutex
	// Guards the members below. Held only briefly: never across network I/O or connection Stop(). SaveNodes() does
	// its short SQL write under it to keep saves ordered, and StartScan holds it while starting the scan workers
	// (no network I/O) so a scan cannot start after StopHardware.
	std::mutex m_mutex;

	bool m_bRunning = false;     // false before StartHardware and after StopHardware
	bool m_bLoadFailed = false;  // stored node list was unreadable: do not overwrite it
	std::vector<ShellyNode> m_nodes;
	std::map<std::string, std::shared_ptr<IShellyConnection>> m_connections; // keyed by ShellyNode::id

	std::set<std::string> m_retryFailed; // guarded by m_mutex: nodes whose failed start was already logged
	std::set<std::string> m_repoll;      // guarded by m_mutex: nodes to poll soon (after a command, for nodes without push)

	ShellyScanner m_scanner;

	// Mapping state. Guarded by m_stateMutex, which is held briefly and never across SQL, a callback or m_mutex.
	// Nothing here is persistent: it is rebuilt from the next statuses. When a node is removed (RemoveNode,
	// RemoveAllNodes) its Domoticz devices are deleted with it. The shared 16 bit temperature and humidity sensors go
	// only when the node owns the id.
	struct NodeState
	{
		bool hasInfo = false;        // OnShellyNodeInfo was received
		int infoAttempts = 0;        // failed FetchInfo calls; after MAX_INFO_ATTEMPTS statuses are mapped with default names
		ShellyNodeConfig config;
		bool hasPending = false;     // a status waits for the node info
		Json::Value pending;
		time_t heldSince = 0;        // when the first status was held (0 = none); the wait ends HELD_STATUS_MAX_SECONDS later
		bool hasFallback = false;    // statuses were mapped with default names; the newest one is kept to rename the devices when the info arrives
		Json::Value fallbackStatus;
		ShellyNodeConfig fallbackConfig;
	};
	struct MeterCache
	{
		double first = 0.0;
		double second = 0.0;
		time_t time = 0;
	};
	static constexpr int MAX_INFO_ATTEMPTS = 3;
	static constexpr int INFO_RETRY_EVERY_PASSES = 10; // after MAX_INFO_ATTEMPTS, FetchInfo is tried again every this many poll passes
	static constexpr int HELD_STATUS_MAX_SECONDS = 600; // a held status waits at most this long for the node info (sleepy nodes do not count attempts)
	static constexpr int METER_REPEAT_SECONDS = 300;
	static constexpr int SLEEPY_POLL_SECONDS = 300; // a sleepy node is polled at most this often (it only answers while awake)
	std::mutex m_stateMutex;
	// One mutex per node: a status is mapped and applied (and a held status released) under it, so the push and the
	// poll thread never apply two statuses of one node interleaved or out of order. Lock order: apply mutex, then
	// m_stateMutex (briefly); never the other way round and never under m_mutex. Guarded by m_stateMutex. Entries are never
	// removed (a thread may still hold or wait for one, and a re-added node must get the same mutex); the map is bounded by
	// the number of distinct nodes ever seen by this instance.
	std::map<std::string, std::shared_ptr<std::mutex>> m_applyMutexes;
	std::map<uint16_t, std::string> m_tempIdOwners;       // 16 bit temperature sensor id (low 16 bits of the node key) -> id of the node that owns it
	std::set<uint16_t> m_tempIdSeeded;                    // ids in m_tempIdOwners that were seeded from the stored list and not claimed by a reporting node yet
	std::map<std::string, NodeState> m_nodeStates;        // by node id
	std::map<std::string, MeterCache> m_meterCache;       // by node id + "/" + unit
	// Switches whose type was set or found. Not invalidated when the user deletes the Domoticz device: a device that is
	// created again by a later status keeps the default switch type until the hardware is restarted (or the node is
	// removed and added again), which clears this set.
	std::set<std::string> m_switchTypeDone;               // node id + "/" + unit
	std::set<std::string> m_switchTypeMissing;            // node id + "/" + unit of switches that were not created (new hardware not accepted)
	std::set<std::string> m_loggedUnmapped;               // node id + "/" + component type
	std::map<std::string, ShellyColorMemo> m_colorMemo;   // by node id + "/" + unit
	std::map<std::string, long> m_inputCounts;            // event counter of a push button last seen, by node id + "/" + unit
	std::set<std::string> m_buttonReady;                  // push button selectors that exist, node id + "/" + unit
	std::set<std::string> m_buttonMissing;                // push button selectors that were not created (new hardware not accepted), node id + "/" + unit

	// Worker thread only (reset by LoadNodes before the worker starts).
	static constexpr int RETRY_EVERY_PASSES = 10;
	static constexpr int MAX_RETRIES_PER_PASS = 2;
	int m_pollPass = 0;
	std::set<std::string> m_authFailed; // guarded by m_stateMutex: Gen1 nodes (id) whose last poll answered 401, so the error is logged once
	size_t m_retryCursor = 0;
	std::map<std::string, std::chrono::steady_clock::time_point> m_lastSleepyPoll; // node id -> time of the last poll of a sleepy node
};
