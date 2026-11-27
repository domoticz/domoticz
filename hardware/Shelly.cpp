#include "stdafx.h"
#include "Shelly.h"
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <iterator>
#include "ShellyIdentify.h"
#include "ColorSwitch.h"
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"

namespace
{
	// Live driver instances and how many leases each has. An entry that is closing hands out no new leases.
	struct LeaseEntry
	{
		int users = 0;
		bool closing = false;
	};
	struct LeaseRegistry
	{
		std::mutex mutex;
		std::condition_variable released;
		std::map<CShelly*, LeaseEntry> entries;
	};
	LeaseRegistry& GetLeaseRegistry()
	{
		static LeaseRegistry registry;
		return registry;
	}
} // namespace

ShellyLease::ShellyLease(ShellyLease&& other) noexcept
	: m_hardware(other.m_hardware)
{
	other.m_hardware = nullptr;
}

ShellyLease& ShellyLease::operator=(ShellyLease&& other) noexcept
{
	if (this != &other)
	{
		Release();
		m_hardware = other.m_hardware;
		other.m_hardware = nullptr;
	}
	return *this;
}

ShellyLease::~ShellyLease()
{
	Release();
}

void ShellyLease::Release()
{
	if (!m_hardware)
		return;
	LeaseRegistry& registry = GetLeaseRegistry();
	{
		std::lock_guard<std::mutex> l(registry.mutex);
		const auto itt = registry.entries.find(m_hardware);
		if (itt != registry.entries.end() && itt->second.users > 0)
			itt->second.users--;
	}
	registry.released.notify_all();
	m_hardware = nullptr;
}

ShellyLease ShellyLease::Acquire(const int hardwareId)
{
	ShellyLease lease;
	LeaseRegistry& registry = GetLeaseRegistry();
	std::lock_guard<std::mutex> l(registry.mutex);
	for (auto& item : registry.entries)
	{
		if (!item.second.closing && item.first->m_HwdID == hardwareId)
		{
			item.second.users++;
			lease.m_hardware = item.first;
			break;
		}
	}
	return lease;
}

ShellyLease ShellyLease::Acquire(const std::string& hardwareId)
{
	if (hardwareId.empty() || hardwareId.size() > 9 || hardwareId.find_first_not_of("0123456789") != std::string::npos)
		return ShellyLease();
	return Acquire(std::atoi(hardwareId.c_str()));
}

CShelly::CShelly(const int id, const std::string& username, const std::string& password, const int pollIntervalSec, const ShellyCoiotOptions& coiot)
	: m_PollInterval((pollIntervalSec > 0) ? pollIntervalSec : 30)
	, m_Credentials{ username, password }
	, m_Coiot(coiot)
{
	m_HwdID = id;

	LeaseRegistry& registry = GetLeaseRegistry();
	std::lock_guard<std::mutex> l(registry.mutex);
	registry.entries[this] = LeaseEntry();
}

CShelly::~CShelly()
{
	// No new leases from here on; wait for the commands that still use this instance.
	{
		LeaseRegistry& registry = GetLeaseRegistry();
		std::unique_lock<std::mutex> l(registry.mutex);
		const auto itt = registry.entries.find(this);
		if (itt != registry.entries.end())
		{
			itt->second.closing = true;
			registry.released.wait(l, [&registry, this] { return registry.entries[this].users == 0; });
			registry.entries.erase(this);
		}
	}
	StopHardware();
}

bool CShelly::StartHardware()
{
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	if (m_thread)
		return true; // already started

	RequestStart();

	LoadNodes();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return true;
}

bool CShelly::StopHardware()
{
	// Serialized with StartHardware and with a second stop (the destructor), so the worker is joined once.
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);

	RequestStop();
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_bRunning = false;
	}
	m_scanner.Stop();

	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}

	std::map<std::string, std::shared_ptr<IShellyConnection>> connections;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		connections.swap(m_connections);
		m_nodes.clear();
		m_retryFailed.clear();
	}
	for (auto& item : connections)
		item.second->Stop();
	ForgetAllNodeStates();

	m_bIsStarted = false;
	return true;
}

void CShelly::Do_Work()
{
	int sec_counter = m_PollInterval - 1;

	Log(LOG_STATUS, "Worker started...");
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0)
			m_LastHeartbeat = mytime(nullptr);

		if (sec_counter % m_PollInterval == 0)
			PollAll();
		else
			PollRequested();
	}
	Log(LOG_STATUS, "Worker stopped...");
}

void CShelly::PollAll()
{
	std::vector<std::shared_ptr<IShellyConnection>> connections;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		connections.reserve(m_connections.size());
		for (const auto& item : m_connections)
			connections.push_back(item.second);
		// The whole pass polls every node, so requests made before it are served by it. Without this a poll interval of
		// one second would run PollAll every iteration and never reach PollRequested.
		m_repoll.clear();
	}

	// Forget the poll times of removed nodes. Done here and not in RemoveNode because the map belongs to the worker thread.
	for (auto itt = m_lastSleepyPoll.begin(); itt != m_lastSleepyPoll.end();)
	{
		const std::string& polledId = itt->first;
		const bool stillThere = std::any_of(connections.begin(), connections.end(), [&polledId](const std::shared_ptr<IShellyConnection>& c) { return c->GetNode().id == polledId; });
		itt = stillThere ? std::next(itt) : m_lastSleepyPoll.erase(itt);
	}

	// The nodes are polled one after the other. Every request has a timeout, so one unreachable node delays the next
	// ones by at most that, but the pass as a whole can take long with many unreachable nodes: the heartbeat is
	// therefore refreshed before every node, not only once per pass.
	for (const auto& connection : connections)
	{
		if (IsStopRequested(0))
			break;
		m_LastHeartbeat = mytime(nullptr);
		const std::string nodeId = connection->GetNode().id;
		if (IsSleepyNode(nodeId))
		{
			// A battery node answers only while it is awake: poll it now and then instead of running into the request timeout
			// every pass. What it reports while awake arrives through its push channel or the next of these polls.
			// steady_clock: a wall clock step must not make a sleepy node polled every pass or never.
			const auto now = std::chrono::steady_clock::now();
			const auto last = m_lastSleepyPoll.find(nodeId);
			if (last != m_lastSleepyPoll.end() && (now - last->second) < std::chrono::seconds(std::max(SLEEPY_POLL_SECONDS, m_PollInterval)))
				continue;
			m_lastSleepyPoll[nodeId] = now;
		}
		EnsureNodeInfo(connection);
		const ShellyResult result = connection->Poll();
		NoteGen1PollResult(connection->GetNode(), result);
		if (!result)
			Debug(DEBUG_HARDWARE, "Poll of %s failed (HTTP %d): %s", connection->GetNode().name.c_str(), result.httpStatus, result.error.c_str());
	}
	m_LastHeartbeat = mytime(nullptr);

	m_pollPass++;
	if (m_pollPass % RETRY_EVERY_PASSES == 0)
		RetryMissingConnections();
}

// A Gen1 node that answers 401 has a wrong or missing password: said once at normal level, and again after a poll
// succeeded in between. m_authFailed is guarded by m_stateMutex because ForgetNodeState() erases from other threads.
void CShelly::NoteGen1PollResult(const ShellyNode& node, const ShellyResult& result)
{
	if (node.gen != SHELLY_GEN1)
		return;
	bool firstFailure = false;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		if (result.IsUnauthorized())
			firstFailure = m_authFailed.insert(node.id).second;
		else if (result)
			m_authFailed.erase(node.id);
	}
	if (firstFailure)
		Log(LOG_ERROR, "Shelly %s: authentication failed, check the password", node.name.c_str());
}

// Asks for an early poll of a node whose state is not pushed (Gen1 without push), so the Domoticz device follows a
// command quickly instead of at the next regular poll. The poll itself runs on the worker thread.
void CShelly::RequestRepoll(const std::string& nodeId)
{
	std::lock_guard<std::mutex> l(m_mutex);
	if (!m_bRunning)
		return;
	const auto conn = m_connections.find(nodeId);
	if (conn != m_connections.end() && !conn->second->HasPush())
		m_repoll.insert(nodeId);
}

void CShelly::PollRequested()
{
	std::vector<std::shared_ptr<IShellyConnection>> connections;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		if (m_repoll.empty())
			return;
		for (const std::string& nodeId : m_repoll)
		{
			const auto conn = m_connections.find(nodeId);
			if (conn != m_connections.end())
				connections.push_back(conn->second);
		}
		m_repoll.clear();
	}

	for (const auto& connection : connections)
	{
		if (IsStopRequested(0))
			break;
		const ShellyResult result = connection->Poll();
		NoteGen1PollResult(connection->GetNode(), result);
		if (!result)
			Debug(DEBUG_HARDWARE, "Poll of %s after a command failed (HTTP %d): %s", connection->GetNode().name.c_str(), result.httpStatus, result.error.c_str());
	}
}

// Nodes whose connection could not be created or started at load time are tried again now and then.
// Only a change of state is logged, so a persistent failure does not repeat in the log.
void CShelly::RetryMissingConnections()
{
	std::vector<ShellyNode> missing;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		for (const auto& node : m_nodes)
		{
			if (m_connections.find(node.id) == m_connections.end())
				missing.push_back(node);
		}
	}

	if (missing.empty())
		return;

	// At most MAX_RETRIES_PER_PASS nodes per pass so a slow Start() cannot stall polling; the cursor rotates
	// through the missing nodes so a persistently failing node does not starve the others.
	const size_t attempts = std::min(missing.size(), static_cast<size_t>(MAX_RETRIES_PER_PASS));
	for (size_t i = 0; i < attempts; i++)
	{
		if (IsStopRequested(0))
			break;
		const ShellyNode& node = missing[(m_retryCursor + i) % missing.size()];
		const StartResult result = StartConnection(node, false);
		if (result == StartResult::Started)
		{
			bool wasFailed;
			{
				std::lock_guard<std::mutex> l(m_mutex);
				wasFailed = (m_retryFailed.erase(node.id) > 0);
			}
			if (wasFailed)
				Log(LOG_STATUS, "%s (%s): connection started", node.name.c_str(), node.ip.c_str());
		}
		else if (result == StartResult::Failed)
		{
			bool firstFailure;
			{
				std::lock_guard<std::mutex> l(m_mutex);
				firstFailure = m_retryFailed.insert(node.id).second;
			}
			if (firstFailure)
				Log(LOG_ERROR, "%s (%s): unable to start the connection, retrying later", node.name.c_str(), node.ip.c_str());
		}
	}
	m_retryCursor = (m_retryCursor + attempts) % missing.size();
}

bool CShelly::IsSleepyNode(const std::string& nodeId)
{
	std::lock_guard<std::mutex> l(m_mutex);
	const auto itt = FindNodeIt(nodeId);
	return itt != m_nodes.end() && itt->sleepy;
}

void CShelly::LoadNodes()
{
	ForgetAllNodeStates();
	m_lastSleepyPoll.clear();
	std::vector<ShellyNode> nodes;
	bool loadFailed = false;
	// IFNULL makes a hardware row without devices yet return one (empty) row, so an empty result is a failed read.
	auto result = m_sql.safe_query("SELECT IFNULL(Extra,'') FROM Hardware WHERE (ID==%d)", m_HwdID);
	if (result.empty() || result[0].empty())
	{
		Log(LOG_ERROR, "The stored device list could not be read from the database, not overwriting it");
		loadFailed = true;
	}
	else
	{
		std::vector<std::string> warnings;
		if (!ShellyNodesFromString(result[0][0], nodes, &warnings))
		{
			Log(LOG_ERROR, "The stored device list is not readable and is ignored. It is left untouched in the database.");
			loadFailed = true;
		}
		for (const auto& warning : warnings)
			Log(LOG_ERROR, "Stored device list: %s", warning.c_str());
	}

	// AddNode refuses nodes with an equal 24 bit key, but a stored list may come from elsewhere (restore, older version, edit).
	// Equal low 16 bits only affect the temperature sensors: the one that reports first owns the id, see ClaimTemperatureId.
	for (size_t i = 0; i < nodes.size(); i++)
	{
		for (size_t j = i + 1; j < nodes.size(); j++)
		{
			const uint32_t keyA = ShellyNodeKey(nodes[i].id);
			const uint32_t keyB = ShellyNodeKey(nodes[j].id);
			if (keyA == keyB)
				Log(LOG_ERROR, "%s (%s) and %s (%s) share Domoticz devices (equal node id), their values will overwrite each other", nodes[i].name.c_str(), nodes[i].ip.c_str(),
				    nodes[j].name.c_str(), nodes[j].ip.c_str());
			else if (ShellyNodeKeysClash(keyA, keyB))
				Log(LOG_ERROR, "%s (%s) and %s (%s) have equal low 16 bits of the MAC: only one of them gets temperature and humidity devices. The one with the lowest MAC keeps them: %s",
				    nodes[i].name.c_str(), nodes[i].ip.c_str(), nodes[j].name.c_str(), nodes[j].ip.c_str(), ((nodes[i].id <= nodes[j].id) ? nodes[i].id : nodes[j].id).c_str());
		}
	}

	// The owner of a 16 bit temperature id is decided here, by MAC (lowest wins), so it does not change with the order in which the
	// nodes report after a restart, which would move the sensor between two nodes.
	{
		std::vector<const ShellyNode*> byId;
		for (const auto& node : nodes)
			byId.push_back(&node);
		std::sort(byId.begin(), byId.end(), [](const ShellyNode* a, const ShellyNode* b) { return a->id < b->id; });
		std::lock_guard<std::mutex> l(m_stateMutex);
		for (const ShellyNode* node : byId)
		{
			const uint16_t id = static_cast<uint16_t>(ShellyNodeKey(node->id) & 0xFFFF);
			if (m_tempIdOwners.emplace(id, node->id).second)
				m_tempIdSeeded.insert(id);
		}
	}

	std::map<std::string, std::shared_ptr<IShellyConnection>> staleConnections;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_nodes = nodes;
		staleConnections.swap(m_connections);
		m_retryFailed.clear();
		m_pollPass = 0;
		m_retryCursor = 0;
		m_repoll.clear();
		m_bLoadFailed = loadFailed;
		m_bRunning = true;
	}
	// Stopped and released outside m_mutex: Stop() joins threads that may be waiting for that mutex.
	for (auto& item : staleConnections)
		item.second->Stop();
	staleConnections.clear();

	for (const auto& node : nodes)
		StartConnection(node);
}

// Creates and starts a connection (no lock held, network I/O). Returns null and sets failure on error.
std::shared_ptr<IShellyConnection> CShelly::CreateAndStartConnection(const ShellyNode& node, std::string& failure)
{
	std::shared_ptr<IShellyConnection> connection = CreateShellyConnection(node, m_Credentials, *this, m_Coiot);
	if (!connection)
	{
		failure = "generation " + std::to_string(node.gen) + " is not supported yet";
		return nullptr;
	}
	if (!connection->Start())
	{
		connection->Stop();
		failure = "the connection could not be started";
		return nullptr;
	}
	return connection;
}

// Registers a started connection when the node is still managed and the hardware is still running.
// A connection that cannot be created or started is not stored. Started means stored; Skipped means the
// node was removed, changed, already connected or the hardware is stopping (not a failure).
CShelly::StartResult CShelly::StartConnection(const ShellyNode& node, const bool logFailure)
{
	std::string failure;
	std::shared_ptr<IShellyConnection> connection = CreateAndStartConnection(node, failure);
	if (!connection)
	{
		if (logFailure)
			Log(LOG_ERROR, "%s (%s): %s", node.name.c_str(), node.ip.c_str(), failure.c_str());
		return StartResult::Failed;
	}

	bool stored = false;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		const auto itt = FindNodeIt(node.id);
		if (m_bRunning && itt != m_nodes.end() && itt->ip == node.ip && itt->port == node.port && itt->gen == node.gen && m_connections.find(node.id) == m_connections.end())
		{
			m_connections[node.id] = connection;
			stored = true;
		}
	}
	if (!stored)
	{
		connection->Stop();
		return StartResult::Skipped;
	}
	return StartResult::Started;
}

std::vector<ShellyNode>::iterator CShelly::FindNodeIt(const std::string& nodeId)
{
	return std::find_if(m_nodes.begin(), m_nodes.end(), [&nodeId](const ShellyNode& n) { return n.id == nodeId; });
}

// Caller holds m_mutex. The short SQL write is done under the lock on purpose: it keeps concurrent saves ordered
// and the stored list identical to m_nodes. No network I/O and no connection Stop() happens under the lock.
void CShelly::SaveNodes()
{
	if (m_bLoadFailed)
	{
		Log(LOG_ERROR, "The stored device list was not readable at start, not overwriting it. Delete and re-add this hardware to clear the list");
		return;
	}
	m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID==%d)", ShellyNodesToString(m_nodes).c_str(), m_HwdID);
}

// The address is deliberately not limited to private ranges (unlike the scan): the command is admin only, host
// names are allowed, the probe follows no redirects and requests one fixed path.
bool CShelly::AddNode(const std::string& ip, const int port, const std::string& name, std::string* error)
{
	// The reason of a failure, for the caller to show to the user
	const auto setError = [error](const char* reason) {
		if (error != nullptr)
			*error = reason;
	};

	if (!ShellyIsValidHost(ip) || !ShellyIsValidPort(port))
	{
		Log(LOG_ERROR, "Invalid device address or port");
		setError("Invalid device address or port");
		return false;
	}

	{
		std::lock_guard<std::mutex> l(m_mutex);
		if (!m_bRunning)
		{
			Log(LOG_ERROR, "The hardware is not running, unable to add %s", ip.c_str());
			setError("The hardware is not running");
			return false;
		}
		if (m_bLoadFailed)
		{
			Log(LOG_ERROR, "The stored device list could not be read, not adding %s. Delete and re-add this hardware to clear the list", ip.c_str());
			setError("The stored device list could not be read, see the log");
			return false;
		}
	}

	const std::string cleanName = ShellySanitizeText(name);

	ShellyIdentity identity;
	const ShellyResult probe = ShellyIdentifyDevice(ip, port, identity);
	if (!probe || !identity.IsValid())
	{
		Log(LOG_ERROR, "%s did not answer as a Shelly device (%s)", ip.c_str(), probe.error.c_str());
		setError("The address did not answer as a Shelly device, check the address, the port and the log");
		return false;
	}

	ShellyNode node = ShellyNodeFromIdentity(identity, ip, port, cleanName);
	bool knownAtFirstCheck = false;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		if (!m_bRunning)
		{
			Log(LOG_ERROR, "The hardware is not running, unable to add %s", ip.c_str());
			setError("The hardware is not running");
			return false;
		}

		const auto existing = FindNodeIt(node.id);
		knownAtFirstCheck = (existing != m_nodes.end());
		if (existing != m_nodes.end())
		{
			// Known device: keep its name.
			if (!cleanName.empty() && cleanName != existing->name)
				Log(LOG_NORM, "%s is already known as '%s', the supplied name '%s' is ignored", ip.c_str(), existing->name.c_str(), cleanName.c_str());
			node.name = existing->name;
			const bool addressChanged = (existing->ip != node.ip) || (existing->port != node.port) || (existing->gen != node.gen);
			if (!addressChanged && m_connections.find(node.id) != m_connections.end())
			{
				existing->model = node.model;
				existing->fw = node.fw;
				existing->auth = node.auth;
				existing->deviceId = node.deviceId;
				SaveNodes();
				return true;
			}
		}
		else if (!m_sql.m_bAcceptNewHardware)
		{
			Log(LOG_NORM, "Accept new hardware disabled. Ignoring new device %s", ip.c_str());
			setError("Accepting new hardware devices is disabled. Enable it in Setup > Settings > Hardware/Devices (or use Allow for 5 Minutes) and add the device again.");
			return false;
		}
	}

	// New device, changed address or generation, or a known device without a working connection.
	std::string failure;
	std::shared_ptr<IShellyConnection> connection = CreateAndStartConnection(node, failure);
	if (!connection)
	{
		Log(LOG_ERROR, "Unable to add %s: %s", ip.c_str(), failure.c_str());
		setError("The connection to the device could not be started, see the log");
		return false;
	}

	std::shared_ptr<IShellyConnection> replaced;
	bool committed = false;
	bool removedMeanwhile = false;
	std::string clashingName;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		const auto existing = FindNodeIt(node.id);
		// A node that was known at the first check but is gone now was removed by someone else: do not bring it back.
		removedMeanwhile = knownAtFirstCheck && (existing == m_nodes.end());
		if (existing == m_nodes.end())
		{
			// The Domoticz devices of a node are identified by 24 bits of its MAC: refuse a second node that would share the devices
			// of another one. Nodes that only share the low 16 bits (the temperature sensor id) are fine, see ClaimTemperatureId.
			const uint32_t newKey = ShellyNodeKey(node.id);
			const auto clash = std::find_if(m_nodes.begin(), m_nodes.end(), [newKey](const ShellyNode& n) { return ShellyNodeKey(n.id) == newKey; });
			if (clash != m_nodes.end())
				clashingName = clash->name;
		}
		if (m_bRunning && !removedMeanwhile && clashingName.empty())
		{
			if (existing != m_nodes.end())
			{
				node.name = existing->name;
				node.sleepy = existing->sleepy;
				node.sleepPeriodSec = existing->sleepPeriodSec;
				*existing = node;
				const auto old = m_connections.find(node.id);
				if (old != m_connections.end())
				{
					replaced = old->second;
					m_connections.erase(old);
				}
			}
			else
				m_nodes.push_back(node);
			m_connections[node.id] = connection;
			SaveNodes();
			committed = true;
		}
	}
	if (replaced)
		replaced->Stop();
	if (!committed)
	{
		connection->Stop();
		if (removedMeanwhile)
		{
			setError("The device was removed while it was being added");
			Log(LOG_NORM, "%s was removed while it was being added, not adding it again", ip.c_str());
		}
		else if (!clashingName.empty())
		{
			setError("The device id clashes with an already added device, see the log");
			Log(LOG_ERROR, "Unable to add %s: its device id clashes with the already added device '%s'", ip.c_str(), clashingName.c_str());
		}
		else
			setError("The hardware is not running");
		return false;
	}
	Log(LOG_STATUS, "Added %s (%s, model %s, generation %d)", node.name.c_str(), node.ip.c_str(), node.model.c_str(), node.gen);
	return true;
}

bool CShelly::RemoveNode(const std::string& nodeId)
{
	std::shared_ptr<IShellyConnection> connection;
	std::vector<ShellyNode> remaining;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		const auto itt = FindNodeIt(nodeId);
		if (itt == m_nodes.end())
			return false;

		const auto conn = m_connections.find(nodeId);
		if (conn != m_connections.end())
		{
			connection = conn->second;
			m_connections.erase(conn);
		}
		m_nodes.erase(itt);
		m_retryFailed.erase(nodeId);
		SaveNodes();
		remaining = m_nodes;
	}
	if (connection)
		connection->Stop();
	// The apply mutex is taken first so a status that is being applied finishes before the state and the devices go,
	// and a later status finds the node gone (it re-checks IsManagedNode under the same mutex).
	{
		const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(nodeId);
		std::lock_guard<std::mutex> apply(*applyMutex);
		DeleteNodeDevices(nodeId, remaining);
		ForgetNodeState(nodeId);
	}
	return true;
}

// Deletes the Domoticz devices that were made for the node. The DeviceIDs are rebuilt from the node key (see
// ShellyMapping.h) and the device type. The 16 bit temperature and humidity sensors share their id between nodes with the
// same low 16 bits of the MAC: they are only deleted when this node owns the id (see ClaimTemperatureId) or no remaining
// node uses it, so this must run before ForgetNodeState releases the ownership.
void CShelly::DeleteNodeDevices(const std::string& nodeId, const std::vector<ShellyNode>& remaining)
{
	const std::vector<std::string> ids = FindNodeDeviceIds(nodeId, remaining);
	if (ids.empty())
		return;
	std::string idxList;
	for (const std::string& id : ids)
	{
		if (!idxList.empty())
			idxList += ";";
		idxList += id;
	}
	Log(LOG_STATUS, "Removing the Domoticz devices of %s", nodeId.c_str());
	m_sql.DeleteDevices(idxList);
}

std::vector<std::string> CShelly::FindNodeDeviceIds(const std::string& nodeId, const std::vector<ShellyNode>& remaining)
{
	const uint32_t key = ShellyNodeKey(nodeId) & 0xFFFFFF;
	const std::string lighting2Id = std_format("0%06X", key);
	const std::string switchId = std_format("%08X", key);
	const std::string meterPrefix = std_format("%06X", key);

	// The 8 digit forms are told apart by the device type, so the id of a switch of one node can never match the meter
	// of another node.
	const auto result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q' OR (DeviceID=='%q' AND Type IN (%d,%d)) OR (length(DeviceID)==8 AND substr(DeviceID,1,6)=='%q' AND Type==%d))",
					     m_HwdID, lighting2Id.c_str(), switchId.c_str(), pTypeGeneralSwitch, pTypeColorSwitch, meterPrefix.c_str(), pTypeGeneral);
	std::vector<std::string> ids;
	for (const auto& row : result)
		ids.push_back(row[0]);

	// The temperature, humidity and temperature+humidity devices: DeviceID is the decimal 16 bit id. They belong to this
	// node when it owns the id, or when no other node uses the same id (the owner can change when nodes are added).
	const uint16_t tempId = static_cast<uint16_t>(key & 0xFFFF);
	bool mine = false;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		const auto owner = m_tempIdOwners.find(tempId);
		mine = (owner != m_tempIdOwners.end() && owner->second == nodeId);
	}
	if (!mine)
	{
		mine = std::none_of(remaining.begin(), remaining.end(), [tempId](const ShellyNode& n) { return static_cast<uint16_t>(ShellyNodeKey(n.id) & 0xFFFF) == tempId; });
	}
	if (mine)
	{
		const auto temps = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%d') AND (Type IN (%d,%d,%d))", m_HwdID, static_cast<int>(tempId), pTypeTEMP, pTypeHUM,
						    pTypeTEMP_HUM);
		for (const auto& row : temps)
			ids.push_back(row[0]);
	}
	return ids;
}

// Renames a node. The Domoticz devices of the node are named after it ("<node name> <component>"): the ones that still
// start with the old name get the new one, a name the user has changed by hand is left alone.
bool CShelly::RenameNode(const std::string& nodeId, const std::string& newName)
{
	const std::string cleanName = ShellySanitizeText(newName);
	if (cleanName.empty())
		return false;

	std::string oldName;
	std::vector<ShellyNode> remaining;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		const auto itt = FindNodeIt(nodeId);
		if (itt == m_nodes.end())
			return false;
		oldName = itt->name;
		if (oldName == cleanName)
			return true;
		itt->name = cleanName;
		SaveNodes();
		for (const ShellyNode& n : m_nodes)
			if (n.id != nodeId)
				remaining.push_back(n);
	}

	const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(nodeId);
	std::lock_guard<std::mutex> apply(*applyMutex);
	for (const std::string& id : FindNodeDeviceIds(nodeId, remaining))
	{
		const auto row = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID==%q)", id.c_str());
		if (row.empty())
			continue;
		const std::string current = row[0][0];
		if (current == oldName || (current.size() > oldName.size() && current.compare(0, oldName.size() + 1, oldName + " ") == 0))
		{
			const std::string renamed = cleanName + current.substr(oldName.size());
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID==%q)", renamed.c_str(), id.c_str());
			m_mainworker.m_eventsystem.WWWUpdateSingleState(std::stoull(id), renamed, m_mainworker.m_eventsystem.REASON_DEVICE);
		}
	}
	Log(LOG_STATUS, "Renamed '%s' to '%s'", oldName.c_str(), cleanName.c_str());
	return true;
}

void CShelly::RemoveAllNodes()
{
	std::map<std::string, std::shared_ptr<IShellyConnection>> connections;
	std::vector<std::string> nodeIds;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		if (!m_bRunning)
		{
			Log(LOG_ERROR, "The hardware is not running, unable to remove the devices");
			return;
		}
		connections.swap(m_connections);
		for (const auto& node : m_nodes)
			nodeIds.push_back(node.id);
		m_nodes.clear();
		m_retryFailed.clear();
		SaveNodes();
	}
	for (auto& item : connections)
		item.second->Stop();
	// See RemoveNode: wait for a status that is being applied, one node at a time.
	for (const std::string& nodeId : nodeIds)
	{
		const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(nodeId);
		std::lock_guard<std::mutex> apply(*applyMutex);
		DeleteNodeDevices(nodeId, std::vector<ShellyNode>());
		ForgetNodeState(nodeId);
	}
	ForgetAllNodeStates();
}

bool CShelly::FindNodeByKey(const uint32_t nodeKey, ShellyNode& node)
{
	std::lock_guard<std::mutex> l(m_mutex);
	const auto itt = std::find_if(m_nodes.begin(), m_nodes.end(), [nodeKey](const ShellyNode& n) { return ShellyNodeMatchesKey(n, nodeKey); });
	if (itt == m_nodes.end())
		return false;
	node = *itt;
	return true;
}

std::vector<ShellyNode> CShelly::GetNodes()
{
	std::lock_guard<std::mutex> l(m_mutex);
	return m_nodes;
}

bool CShelly::FindNode(const std::string& nodeId, ShellyNode& node)
{
	std::lock_guard<std::mutex> l(m_mutex);
	const auto itt = FindNodeIt(nodeId);
	if (itt == m_nodes.end())
		return false;
	node = *itt;
	return true;
}

bool CShelly::StartScan(const std::string& range, const int port, std::string& error)
{
	if (!ShellyIsValidPort(port))
	{
		error = "Invalid port";
		return false;
	}
	std::vector<std::string> hosts;
	std::string cleanRange;
	if (!ShellyExpandScanRange(range, hosts, error, &cleanRange))
		return false;

	const size_t count = hosts.size();
	{
		// Held across Start on purpose: it keeps StopHardware from slipping in between the running check and the
		// start of the workers. Start does no network I/O; the workers only take the scanner's own lock.
		std::lock_guard<std::mutex> l(m_mutex);
		if (!m_bRunning)
		{
			error = "The hardware is not running";
			return false;
		}
		if (!m_scanner.Start(cleanRange, std::move(hosts), port, error))
			return false;
	}
	Log(LOG_STATUS, "Scanning %s (%d addresses) for Shelly devices", cleanRange.c_str(), static_cast<int>(count));
	return true;
}

void CShelly::CancelScan()
{
	m_scanner.Cancel();
}

ShellyScanStatus CShelly::GetScanStatus()
{
	return m_scanner.GetStatus();
}

ShellyResult CShelly::SendShellyCommand(const std::string& nodeId, const ShellyCommand& command)
{
	std::shared_ptr<IShellyConnection> connection;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		const auto conn = m_connections.find(nodeId);
		if (conn != m_connections.end())
			connection = conn->second;
	}
	if (!connection)
		return ShellyResult::Failure(0, "no connection for this device");
	return connection->SendCommand(command);
}

bool CShelly::IsManagedNode(const std::string& nodeId)
{
	std::lock_guard<std::mutex> l(m_mutex);
	return m_bRunning && FindNodeIt(nodeId) != m_nodes.end();
}

void CShelly::ForgetNodeState(const std::string& nodeId)
{
	const std::string prefix = nodeId + "/";
	const auto hasPrefix = [&prefix](const std::string& key) { return key.compare(0, prefix.size(), prefix) == 0; };

	std::lock_guard<std::mutex> l(m_stateMutex);
	m_nodeStates.erase(nodeId);
	m_authFailed.erase(nodeId);
	// m_applyMutexes is kept on purpose: a thread may still hold or wait for the mutex of this node, and a re-added
	// node must get the same one. It is bounded by the number of nodes and never cleared.
	for (auto itt = m_tempIdOwners.begin(); itt != m_tempIdOwners.end();)
	{
		if (itt->second == nodeId)
		{
			m_tempIdSeeded.erase(itt->first);
			itt = m_tempIdOwners.erase(itt);
		}
		else
			++itt;
	}
	for (auto itt = m_meterCache.begin(); itt != m_meterCache.end();)
		itt = hasPrefix(itt->first) ? m_meterCache.erase(itt) : std::next(itt);
	for (auto itt = m_colorMemo.begin(); itt != m_colorMemo.end();)
		itt = hasPrefix(itt->first) ? m_colorMemo.erase(itt) : std::next(itt);
	for (auto itt = m_inputCounts.begin(); itt != m_inputCounts.end();)
		itt = hasPrefix(itt->first) ? m_inputCounts.erase(itt) : std::next(itt);
	for (auto itt = m_buttonReady.begin(); itt != m_buttonReady.end();)
		itt = hasPrefix(*itt) ? m_buttonReady.erase(itt) : std::next(itt);
	for (auto itt = m_buttonMissing.begin(); itt != m_buttonMissing.end();)
		itt = hasPrefix(*itt) ? m_buttonMissing.erase(itt) : std::next(itt);
	for (auto itt = m_switchTypeDone.begin(); itt != m_switchTypeDone.end();)
		itt = hasPrefix(*itt) ? m_switchTypeDone.erase(itt) : std::next(itt);
	for (auto itt = m_switchTypeMissing.begin(); itt != m_switchTypeMissing.end();)
		itt = hasPrefix(*itt) ? m_switchTypeMissing.erase(itt) : std::next(itt);
	for (auto itt = m_loggedUnmapped.begin(); itt != m_loggedUnmapped.end();)
		itt = hasPrefix(*itt) ? m_loggedUnmapped.erase(itt) : std::next(itt);
}

void CShelly::ForgetAllNodeStates()
{
	std::lock_guard<std::mutex> l(m_stateMutex);
	m_nodeStates.clear();
	m_authFailed.clear();
	// m_applyMutexes stays, see ForgetNodeState.
	m_tempIdOwners.clear();
	m_tempIdSeeded.clear();
	m_meterCache.clear();
	m_colorMemo.clear();
	m_inputCounts.clear();
	m_buttonReady.clear();
	m_buttonMissing.clear();
	m_switchTypeDone.clear();
	m_switchTypeMissing.clear();
	m_loggedUnmapped.clear();
}

// Fetches the static node information (names, modes) once, so devices are created with the names configured on
// the device. A status that arrives earlier is held back by OnShellyStatus; when the information cannot be fetched
// MAX_INFO_ATTEMPTS times, held and later statuses are mapped with default names.
void CShelly::EnsureNodeInfo(const std::shared_ptr<IShellyConnection>& connection)
{
	const ShellyNode node = connection->GetNode();
	if (!IsManagedNode(node.id))
		return; // removed meanwhile: do not create state for it
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		// Read only: a node removed since the check above must not get a state entry from here (the entry is created under the
		// apply mutex further down, after the check is repeated).
		const auto stateIt = m_nodeStates.find(node.id);
		if (stateIt != m_nodeStates.end())
		{
			const NodeState& state = stateIt->second;
			if (state.hasInfo)
				return;
			// Out of attempts: the node works with default names, try again only now and then (m_pollPass is only touched
			// by the worker thread, which is the only caller).
			if (state.infoAttempts >= MAX_INFO_ATTEMPTS && (m_pollPass % INFO_RETRY_EVERY_PASSES) != 0)
				return;
		}
	}

	const ShellyResult fetched = connection->FetchInfo();
	if (fetched)
		return; // OnShellyNodeInfo recorded it

	// A sleepy node is unreachable most of the time by design, which says nothing about the information: only
	// failures where it did answer (or a local failure) count toward the fallback to default names. A status that has
	// been held for HELD_STATUS_MAX_SECONDS is released with default names regardless, so the node cannot wait forever.
	const time_t now = time(nullptr);
	if (IsSleepyNode(node.id) && fetched.IsReachabilityFailure())
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		const auto itt = m_nodeStates.find(node.id);
		if (itt == m_nodeStates.end() || itt->second.heldSince == 0 || (now - itt->second.heldSince) < HELD_STATUS_MAX_SECONDS)
			return;
	}

	// A status held for the node information is released under the apply mutex of the node, so it cannot overtake
	// or be overtaken by a status that arrives on another thread at the same time.
	const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(node.id);
	std::lock_guard<std::mutex> apply(*applyMutex);
	if (!IsManagedNode(node.id))
		return; // removed while waiting for the mutex

	bool release = false;
	Json::Value pending;
	ShellyNodeConfig config;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		NodeState& state = m_nodeStates[node.id];
		if (state.hasInfo)
			return;
		if (state.infoAttempts < MAX_INFO_ATTEMPTS)
		{
			const bool expired = (state.heldSince != 0) && ((now - state.heldSince) >= HELD_STATUS_MAX_SECONDS);
			state.infoAttempts = expired ? MAX_INFO_ATTEMPTS : state.infoAttempts + 1;
			if (state.infoAttempts >= MAX_INFO_ATTEMPTS && state.hasPending)
			{
				release = true;
				pending = std::move(state.pending);
				state.pending = Json::Value();
				state.hasPending = false;
				state.heldSince = 0;
				config = state.config; // defaults unless something recorded a mode (for example the roller mode) before
				state.fallbackStatus = pending;
				state.fallbackConfig = config;
				state.hasFallback = true;
			}
		}
	}
	if (release)
		ApplyStatus(node, pending, config);
}

// Devices made with default names (the node information was missing) get the configured names once it arrives, but
// only when the stored name still is the generated default: a name the user changed is left alone. Temperature
// sensors are not renamed (they are identified by the low 16 bits of the key only).
void CShelly::RenameDefaultNamedDevices(const ShellyNode& node, const Json::Value& status, const ShellyNodeConfig& oldConfig, const ShellyNodeConfig& newConfig)
{
	const ShellyNodeUpdate before = ShellyMapStatus(node, status, oldConfig);
	const ShellyNodeUpdate after = ShellyMapStatus(node, status, newConfig);
	const uint32_t key = ShellyNodeKey(node.id);
	const std::string lighting2Id = std_format("%X%02X%02X%02X", 0, (key >> 16) & 0xFF, (key >> 8) & 0xFF, key & 0xFF);
	const std::string switchId = std_format("%08X", key);

	for (const ShellyDeviceUpdate& old : before.devices)
	{
		const bool isTempHum = (old.kind == ShellyDeviceKind::Sensor && old.role == static_cast<int>(ShellySensorRole::TempHum));
		if (old.kind == ShellyDeviceKind::Temperature || isTempHum || old.name.empty())
			continue;
		const auto itt = std::find_if(after.devices.begin(), after.devices.end(), [&old](const ShellyDeviceUpdate& d) { return d.unit == old.unit; });
		const std::string meterId = std_format("%08X", (key << 8) | old.unit);
		if (itt == after.devices.end())
		{
			// The unit encodes the kind, so a device that has no counterpart after the information arrived (for example
			// after a roller mode change) is no longer updated. This runs once per node information release.
			// Lighting2, Thermostat6, the lux meter and the W usage meter (Power) all have the "0" + 6 hex digits DeviceID.
			const bool isLighting2 = (old.kind == ShellyDeviceKind::Switch || old.kind == ShellyDeviceKind::Dimmer || old.kind == ShellyDeviceKind::Thermostat
						  || old.kind == ShellyDeviceKind::Power || (old.kind == ShellyDeviceKind::Sensor && old.role == static_cast<int>(ShellySensorRole::Lux)));
			const bool isSwitchId = (old.kind == ShellyDeviceKind::Blind || old.kind == ShellyDeviceKind::Color || old.kind == ShellyDeviceKind::Input || old.kind == ShellyDeviceKind::Binary);
			const std::string& oldDeviceId = isLighting2 ? lighting2Id : (isSwitchId ? switchId : meterId);
			Log(LOG_STATUS, "%s: device '%s' (unit %d, DeviceID %s) is no longer used because the mode of the Shelly changed, it can be deleted", node.name.c_str(), old.name.c_str(),
			    static_cast<int>(old.unit), oldDeviceId.c_str());
		}
		if (itt == after.devices.end() || itt->name.empty() || itt->name == old.name)
			continue;
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (Unit==%d) AND (Name=='%q') AND (DeviceID IN ('%q','%q','%q'))", itt->name.c_str(), m_HwdID,
				 static_cast<int>(old.unit), old.name.c_str(), lighting2Id.c_str(), switchId.c_str(), meterId.c_str());
	}
}

std::shared_ptr<std::mutex> CShelly::GetApplyMutex(const std::string& nodeId)
{
	std::lock_guard<std::mutex> l(m_stateMutex);
	std::shared_ptr<std::mutex>& item = m_applyMutexes[nodeId];
	if (!item)
		item = std::make_shared<std::mutex>();
	return item;
}

void CShelly::OnShellyNodeInfo(const ShellyNode& node, const Json::Value& info)
{
	if (!IsManagedNode(node.id))
		return;
	const ShellyNodeConfig config = ShellyParseNodeInfo(node, info);

	// Recording the information and releasing the held status happen under the apply mutex of the node (see OnShellyStatus).
	const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(node.id);
	std::lock_guard<std::mutex> apply(*applyMutex);
	if (!IsManagedNode(node.id))
		return; // removed while waiting for the mutex

	bool release = false;
	bool rename = false;
	Json::Value pending;
	Json::Value fallbackStatus;
	ShellyNodeConfig fallbackConfig;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		NodeState& state = m_nodeStates[node.id];
		state.hasInfo = true;
		state.config = config;
		if (state.hasFallback)
		{
			rename = true;
			fallbackStatus = std::move(state.fallbackStatus);
			fallbackConfig = std::move(state.fallbackConfig);
			state.fallbackStatus = Json::Value();
			state.hasFallback = false;
		}
		if (state.hasPending)
		{
			release = true;
			pending = std::move(state.pending);
			state.pending = Json::Value();
			state.hasPending = false;
		}
		state.heldSince = 0;
	}

	bool sleepyChanged = false;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		const auto itt = FindNodeIt(node.id);
		if (itt != m_nodes.end())
		{
			bool changed = false;
			if (!config.firmware.empty() && itt->fw != config.firmware)
			{
				itt->fw = config.firmware;
				changed = true;
			}
			if (config.sleepKnown)
			{
				const int period = config.sleepy ? config.sleepPeriodSec : 0;
				if (itt->sleepy != config.sleepy || itt->sleepPeriodSec != period)
				{
					sleepyChanged = (itt->sleepy != config.sleepy);
					itt->sleepy = config.sleepy;
					itt->sleepPeriodSec = period;
					changed = true;
				}
			}
			if (changed)
				SaveNodes();
		}
	}
	if (sleepyChanged)
	{
		if (config.sleepy)
			Log(LOG_STATUS, "%s: battery device that sleeps between reports (wake-up period %d seconds), it is polled only now and then", node.name.c_str(), config.sleepPeriodSec);
		else
			Log(LOG_STATUS, "%s: no longer a sleeping device, it is polled normally", node.name.c_str());
	}

	if (rename)
		RenameDefaultNamedDevices(node, fallbackStatus, fallbackConfig, config);

	if (release)
		ApplyStatus(node, pending, config);
}

void CShelly::OnShellyStatus(const ShellyNode& node, const Json::Value& status)
{
	if (!IsManagedNode(node.id))
		return;

	// Deciding what to do with the status and applying it is one step per node: the push and the poll thread can
	// both deliver statuses, and the one that was delivered last must also be applied last.
	const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(node.id);
	std::lock_guard<std::mutex> apply(*applyMutex);
	if (!IsManagedNode(node.id))
		return; // removed while waiting for the mutex

	ShellyNodeConfig config;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		NodeState& state = m_nodeStates[node.id];
		if (!state.hasInfo && state.infoAttempts < MAX_INFO_ATTEMPTS && state.heldSince != 0 && (time(nullptr) - state.heldSince) >= HELD_STATUS_MAX_SECONDS)
		{
			// Waited long enough (a sleepy node does not count failed attempts): continue with default names.
			state.infoAttempts = MAX_INFO_ATTEMPTS;
			state.pending = Json::Value();
			state.hasPending = false;
			state.heldSince = 0;
		}
		if (!state.hasInfo && state.infoAttempts < MAX_INFO_ATTEMPTS)
		{
			if (state.heldSince == 0)
				state.heldSince = time(nullptr);
			// Held status: until the node information arrived (or MAX_INFO_ATTEMPTS fetches failed) no device is written
			// for this node, so devices are created with their configured names. Only the newest status is kept, an older
			// held one is dropped. The held status is applied by OnShellyNodeInfo, or by EnsureNodeInfo after the last
			// failed attempt (then with default names). Failed attempts of a node that does not answer at all do not
			// count for a sleepy node, so a status of such a node can wait until the information could be fetched.
			state.pending = status;
			state.hasPending = true;
			return;
		}
		config = state.config;
		if (!state.hasInfo)
		{
			// Mapped with default names: remembered so the names can be corrected when the information arrives later.
			state.fallbackStatus = status;
			state.fallbackConfig = config;
			state.hasFallback = true;
		}
	}
	ApplyStatus(node, status, config);
}

// A press of a push button input. Gen1 devices report presses in their status (event and event counter), see ApplyInput.
void CShelly::OnShellyEvent(const ShellyNode& node, const Json::Value& event)
{
	ShellyInputEvent press;
	if (!ShellyMapInputEvent(node, event, press) || !IsManagedNode(node.id))
		return;

	const std::shared_ptr<std::mutex> applyMutex = GetApplyMutex(node.id);
	std::lock_guard<std::mutex> apply(*applyMutex);
	if (!IsManagedNode(node.id))
		return;

	ShellyNodeConfig config;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		const auto itt = m_nodeStates.find(node.id);
		if (itt != m_nodeStates.end())
			config = itt->second.config;
	}
	FireButton(node, press.unit, press.level, ShellyInputButtonName(node, config, press.channel));
}

void CShelly::OnShellyAvailability(const ShellyNode& node, const bool online)
{
	if (!IsManagedNode(node.id))
		return; // removed meanwhile: a late callback must not log or touch the state

	// A sleepy (battery) node is unreachable most of the time by design: only a debug message. The node of the callback is
	// the copy the connection was made with and can be stale, the driver's own entry knows what the node information said.
	if (IsSleepyNode(node.id))
		Debug(DEBUG_HARDWARE, "%s (%s) is %s", node.name.c_str(), node.ip.c_str(), online ? "online" : "unreachable");
	else
		Log(online ? LOG_STATUS : LOG_ERROR, "%s (%s) is %s", node.name.c_str(), node.ip.c_str(), online ? "online" : "unreachable");

	// Domoticz has no per device unreachable state: sensors time out on their own when no update arrives (the
	// standard sensor timeout) and switches keep their last state. The meter cache is cleared so the first status
	// after a change is always written, which also refreshes the last update time of every sensor.
	const std::string prefix = node.id + "/";
	std::lock_guard<std::mutex> l(m_stateMutex);
	for (auto itt = m_meterCache.begin(); itt != m_meterCache.end();)
		itt = (itt->first.compare(0, prefix.size(), prefix) == 0) ? m_meterCache.erase(itt) : std::next(itt);
}

void CShelly::ApplyStatus(const ShellyNode& node, const Json::Value& status, const ShellyNodeConfig& config)
{
	const ShellyNodeUpdate update = ShellyMapStatus(node, status, config);

	// Best effort: warn once per node and start when MQTT Auto Discovery already made devices for this Shelly. MQTT AD
	// keys Shelly devices on the device id ("shellyplus1-aabbccddeeff"), which contains the MAC, so a lookup on both finds them.
	bool checkOverlap;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		checkOverlap = m_loggedUnmapped.insert(node.id + "/mqttad").second;
	}
	if (checkOverlap)
	{
		std::vector<std::string> keys;
		if (!node.deviceId.empty())
			keys.push_back(node.deviceId);
		if (node.id.size() == 12)
			keys.push_back(node.id);
		for (const std::string& key : keys)
		{
			if (key.find_first_of("%_'\"\\") != std::string::npos)
				continue;
			const auto found = m_sql.safe_query("SELECT d.ID FROM DeviceStatus d INNER JOIN Hardware h ON (h.ID == d.HardwareID) WHERE (h.Type==%d) AND (d.DeviceID LIKE '%%%q%%') LIMIT 1",
							    static_cast<int>(HTYPE_MQTTAutoDiscovery), key.c_str());
			if (!found.empty())
			{
				Log(LOG_STATUS, "Shelly %s: this device is also present through MQTT Auto Discovery; use one path per device", node.name.c_str());
				break;
			}
		}
	}

	for (const std::string& type : update.unmapped)
	{
		bool first;
		{
			std::lock_guard<std::mutex> l(m_stateMutex);
			first = m_loggedUnmapped.insert(node.id + "/" + type).second;
		}
		if (first)
			Log(LOG_STATUS, "%s: component type '%s' is not supported yet and is skipped", node.name.c_str(), type.c_str());
	}

	for (const uint8_t unit : update.droppedUnits)
	{
		bool first;
		{
			std::lock_guard<std::mutex> l(m_stateMutex);
			first = m_loggedUnmapped.insert(node.id + "/unit " + std::to_string(unit)).second;
		}
		if (first)
			Debug(DEBUG_HARDWARE, "%s: more than one value maps to device unit 0x%02X, only the first is used", node.name.c_str(), unit);
	}

	for (const int channel : update.buttonWithoutCounter)
	{
		bool first;
		{
			std::lock_guard<std::mutex> l(m_stateMutex);
			first = m_loggedUnmapped.insert(node.id + "/button without counter " + std::to_string(channel)).second;
		}
		if (first)
			Debug(DEBUG_HARDWARE, "%s: input %d is a push button but its status has no event counter, it is a contact device instead of a push button selector", node.name.c_str(), channel + 1);
	}

	// The apply mutex of the node is held here, across SendSwitch and sDecodeRXMessage. That cannot deadlock: those
	// calls reach mainworker, which only queues commands, and SendCommand (the path into WriteToHardware) never
	// calls back into this driver synchronously, so nothing on this path waits for the apply mutex again.
	for (const ShellyDeviceUpdate& device : update.devices)
		ApplyDeviceUpdate(node, update, device);

	// An owner that was only seeded from the stored list, and whose status has no temperature device, never claims the id: free it for
	// a node that does report one. Once an owner did claim the id, it keeps it.
	const bool reportsTemperature = std::any_of(update.devices.begin(), update.devices.end(), [](const ShellyDeviceUpdate& device) {
		return device.kind == ShellyDeviceKind::Temperature || (device.kind == ShellyDeviceKind::Sensor && device.role == static_cast<int>(ShellySensorRole::TempHum));
	});
	if (!reportsTemperature)
	{
		const uint16_t id = static_cast<uint16_t>(ShellyNodeKey(node.id) & 0xFFFF);
		std::lock_guard<std::mutex> l(m_stateMutex);
		const auto itt = m_tempIdOwners.find(id);
		if (itt != m_tempIdOwners.end() && itt->second == node.id && m_tempIdSeeded.erase(id) > 0)
			m_tempIdOwners.erase(itt);
	}
}

bool CShelly::IsRedundantMeterUpdate(const ShellyNode& node, const ShellyDeviceUpdate& device)
{
	const std::string key = node.id + "/" + std::to_string(device.unit);
	const time_t now = mytime(nullptr);
	std::lock_guard<std::mutex> l(m_stateMutex);
	MeterCache& cache = m_meterCache[key];
	double first = device.value;
	double second = device.hasEnergy ? device.energyKwh : 0.0;
	if (device.kind == ShellyDeviceKind::Thermostat)
	{
		first = device.hasTemperature ? device.temperature : 0.0;
		second = device.hasSetpoint ? device.setpoint : 0.0;
	}
	else if (device.kind == ShellyDeviceKind::Sensor && device.role == static_cast<int>(ShellySensorRole::TempHum))
	{
		first = device.hasTemperature ? device.temperature : 0.0;
		second = device.hasHumidity ? device.humidity : 0.0;
	}
	if (cache.time != 0 && cache.first == first && cache.second == second && (now - cache.time) < METER_REPEAT_SECONDS)
		return true;
	cache.first = first;
	cache.second = second;
	cache.time = now;
	return false;
}

// The temperature, humidity and temperature+humidity devices only have the low 16 bits of the node key in their id. The
// node that reports first owns the id; another node with the same low 16 bits gets no temperature devices (the rest of
// its devices are unaffected). Logged once per node.
bool CShelly::ClaimTemperatureId(const ShellyNode& node)
{
	const uint16_t id = static_cast<uint16_t>(ShellyNodeKey(node.id) & 0xFFFF);
	std::string owner;
	bool first = false;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		const auto itt = m_tempIdOwners.find(id);
		if (itt == m_tempIdOwners.end())
		{
			m_tempIdOwners[id] = node.id;
			return true;
		}
		if (itt->second == node.id)
		{
			m_tempIdSeeded.erase(id);
			return true;
		}
		owner = itt->second;
		first = m_loggedUnmapped.insert(node.id + "/temperature id clash").second;
	}
	if (first)
		Log(LOG_ERROR, "%s: the temperature sensor id (low 16 bits of the MAC) is already used by the Shelly with MAC %s, the temperature and humidity of this device are not written", node.name.c_str(),
		    owner.c_str());
	return false;
}

void CShelly::InvalidateDedupe(const ShellyNode& node, const uint8_t unit)
{
	const std::string key = node.id + "/" + std::to_string(unit);
	std::lock_guard<std::mutex> l(m_stateMutex);
	m_meterCache.erase(key);
	// The color memo also feeds brightness commands, so only its repeat window is reset.
	const auto itt = m_colorMemo.find(key);
	if (itt != m_colorMemo.end())
		itt->second.time = 0;
}

bool CShelly::GetColorMemo(const ShellyNode& node, const uint8_t unit, ShellyColorMemo& memo)
{
	std::lock_guard<std::mutex> l(m_stateMutex);
	const auto itt = m_colorMemo.find(node.id + "/" + std::to_string(unit));
	if (itt == m_colorMemo.end() || !itt->second.known)
		return false;
	memo = itt->second;
	return true;
}

void CShelly::ApplyColor(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	const uint32_t key = ShellyNodeKey(node.id);
	const time_t now = mytime(nullptr);

	ShellyColorMemo memo;
	memo.known = true;
	memo.red = device.red;
	memo.green = device.green;
	memo.blue = device.blue;
	memo.white = device.white;
	memo.whiteMode = device.whiteMode;
	memo.level = device.level;
	memo.on = device.on;
	memo.time = now;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		ShellyColorMemo& old = m_colorMemo[node.id + "/" + std::to_string(device.unit)];
		// Only a change, or a repeat after a while, is passed on: every poll would rewrite the stored color otherwise.
		const bool same = old.known && old.red == memo.red && old.green == memo.green && old.blue == memo.blue && old.white == memo.white && old.whiteMode == memo.whiteMode
				  && old.level == memo.level && old.on == memo.on;
		if (same && (now - old.time) < METER_REPEAT_SECONDS)
			return;
		old = memo;
	}

	// An unknown brightness is full for a light that is on, and 0 for one that is off.
	const int level = (device.level >= 0) ? device.level : (device.on ? 100 : 0);
	const bool hasColor = device.whiteMode || device.red != 0 || device.green != 0 || device.blue != 0 || device.white > 0;

	_tColorSwitch cmd;
	cmd.id = key;
	// Decided by the mapping from the component type or model, so the subtype of a device cannot flip between statuses.
	cmd.subtype = device.rgbw ? sTypeColor_RGB_W_Z : sTypeColor_RGB;
	cmd.dunit = device.unit;
	cmd.value = static_cast<uint32_t>(level);
	if (!device.on)
		cmd.command = Color_LedOff;
	else if (!hasColor)
		cmd.command = Color_SetBrightnessLevel;
	else
	{
		cmd.command = Color_SetColor;
		if (device.whiteMode)
			cmd.color.mode = ColorModeWhite;
		else
		{
			cmd.color.mode = ColorModeCustom;
			cmd.color.r = static_cast<uint8_t>(device.red);
			cmd.color.g = static_cast<uint8_t>(device.green);
			cmd.color.b = static_cast<uint8_t>(device.blue);
			cmd.color.ww = static_cast<uint8_t>(std::max(0, device.white));
		}
	}
	sDecodeRXMessage(this, reinterpret_cast<const unsigned char*>(&cmd), device.name.c_str(), update.batteryLevel, nullptr);
}

void CShelly::ApplyThermostat(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	if ((!device.hasTemperature && !device.hasSetpoint) || IsRedundantMeterUpdate(node, device))
		return;
	const uint32_t key = ShellyNodeKey(node.id);

	_tThermostat6 thermostat;
	thermostat.subtype = sTypeThermostat6Temp;
	thermostat.id1 = 0;
	thermostat.id2 = static_cast<uint8_t>((key >> 16) & 0xFF);
	thermostat.id3 = static_cast<uint8_t>((key >> 8) & 0xFF);
	thermostat.id4 = static_cast<uint8_t>(key & 0xFF);
	thermostat.dunit = device.unit;
	thermostat.battery_level = static_cast<uint8_t>(update.batteryLevel);
	thermostat.temperature = static_cast<float>(device.temperature);
	thermostat.setpoint = static_cast<float>(device.setpoint);
	thermostat.update_flags = static_cast<uint8_t>((device.hasTemperature ? 0x01 : 0) | (device.hasSetpoint ? 0x02 : 0));
	sDecodeRXMessage(this, reinterpret_cast<const unsigned char*>(&thermostat), device.name.c_str(), -1, nullptr);
}

// A read-only on/off device: a GeneralSwitch of the given switch type. It is never controllable (ShellyRouteDevice refuses it).
void CShelly::ApplyContact(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device, const int switchType)
{
	const uint32_t key = ShellyNodeKey(node.id);
	const int nodeId = static_cast<int>(key);
	const std::string deviceId = std_format("%08X", key);
	const auto send = [&]() { SendGeneralSwitch(nodeId, device.unit, update.batteryLevel, device.on ? gswitch_sOn : gswitch_sOff, 0, device.name, std::string(), update.rssiLevel); };
	SetNewDeviceSwitchType(node, device.unit, deviceId, pTypeGeneralSwitch, switchType, send);
}

void CShelly::ApplyBinary(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	int switchType = STYPE_Contact;
	switch (static_cast<ShellyBinaryRole>(device.role))
	{
		case ShellyBinaryRole::DoorWindow:
			switchType = STYPE_DoorContact;
			break;
		case ShellyBinaryRole::Motion:
			switchType = STYPE_Motion;
			break;
		case ShellyBinaryRole::Smoke:
			switchType = STYPE_SMOKEDETECTOR;
			break;
		default:
			break;
	}
	ApplyContact(node, update, device, switchType);
}

// Pushes a press to the selector switch of a push button. The selector goes back to Off first, then to the level of the
// press, so a press that repeats the previous one is a change too. SendSelectorSwitch pushes an RX message every time (it
// only skips the final database write when the level is unchanged), so an automation on the selector sees the Off message
// before each level, and the device is left at the level of the last press until the next one.
void CShelly::FireButton(const ShellyNode& node, const uint8_t unit, const int level, const std::string& name)
{
	const int nodeId = static_cast<int>(ShellyNodeKey(node.id));
	SendSelectorSwitch(nodeId, unit, "0", name, 0, false, "Off|Single press|Double press|Triple press|Long press", std::string(), true, std::string());
	if (level > 0)
		SendSelectorSwitch(nodeId, unit, std::to_string(level), name, 0, false, "Off|Single press|Double press|Triple press|Long press", std::string(), true, std::string());
}

void CShelly::ApplyInput(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	if (!device.button)
	{
		ApplyContact(node, update, device, STYPE_Contact);
		return;
	}

	// Gen1 and Gen2 alike: the selector is created at the first status of a button input, not at its first press.
	EnsureButtonSelector(node, device.unit, device.name);

	if (!device.hasEventCount)
		return; // Gen2: the presses arrive as events (OnShellyEvent)

	// Gen1: the count tells a new press from an old one. The first status only records it, so a press from before the start is not replayed.
	const std::string key = node.id + "/" + std::to_string(device.unit);
	bool changed = false;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		const auto itt = m_inputCounts.find(key);
		changed = (itt != m_inputCounts.end() && itt->second != device.eventCount);
		m_inputCounts[key] = device.eventCount;
	}
	if (changed && device.eventLevel > 0)
		FireButton(node, device.unit, device.eventLevel, device.name);
}

// Makes sure the selector of a push button exists (created at Off). Looked up once per selector and run.
void CShelly::EnsureButtonSelector(const ShellyNode& node, const uint8_t unit, const std::string& name)
{
	const std::string key = node.id + "/" + std::to_string(unit);
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		if (m_buttonReady.find(key) != m_buttonReady.end())
			return;
		// Not created before because new hardware is not accepted: no database probes per status until it is accepted again.
		if (m_buttonMissing.find(key) != m_buttonMissing.end())
		{
			if (!m_sql.m_bAcceptNewHardware)
				return;
			m_buttonMissing.erase(key);
		}
	}
	const std::string deviceId = std_format("%08X", ShellyNodeKey(node.id));
	if (!DeviceRowExists(deviceId, unit, pTypeGeneralSwitch))
	{
		// An input that used to be a contact and is a push button now leaves the contact device behind: say so once.
		const int contactUnit = static_cast<int>(ShellyMakeUnit(ShellyDeviceKind::Input, ShellyUnitSlot(unit) - 8));
		const auto contact = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d)", m_HwdID, deviceId.c_str(), contactUnit,
						      pTypeGeneralSwitch);
		if (!contact.empty())
		{
			bool first;
			{
				std::lock_guard<std::mutex> l(m_stateMutex);
				first = m_loggedUnmapped.insert(key + "/orphaned contact").second;
			}
			if (first)
				Log(LOG_STATUS, "%s: input '%s' is now a push button selector, the contact device '%s' (unit %d, DeviceID %s) is no longer updated and can be deleted", node.name.c_str(),
				    name.c_str(), contact[0][0].c_str(), contactUnit, deviceId.c_str());
		}
		FireButton(node, unit, 0, name);
	}
	const bool exists = DeviceRowExists(deviceId, unit, pTypeGeneralSwitch);
	std::lock_guard<std::mutex> l(m_stateMutex);
	if (exists)
	{
		m_buttonReady.insert(key);
		m_buttonMissing.erase(key);
	}
	else
		m_buttonMissing.insert(key);
}

void CShelly::ApplySensor(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	const uint32_t key = ShellyNodeKey(node.id);
	const int nodeId = static_cast<int>(key);

	switch (static_cast<ShellySensorRole>(device.role))
	{
		case ShellySensorRole::TempHum:
			// The 16 bit temperature and humidity id only has the low 16 bits of the key (see ShellyMapping.h)
			if (!ClaimTemperatureId(node))
				break;
			if (device.hasTemperature && device.hasHumidity)
				SendTempHumSensor(static_cast<int>(key & 0xFFFF), update.batteryLevel, static_cast<float>(device.temperature), static_cast<int>(std::lround(device.humidity)), device.name, update.rssiLevel);
			else if (device.hasHumidity)
				SendHumiditySensor(static_cast<int>(key & 0xFFFF), update.batteryLevel, static_cast<int>(std::lround(device.humidity)), device.name, update.rssiLevel);
			else if (device.hasTemperature)
				SendTempSensor(static_cast<int>(key & 0xFFFF), update.batteryLevel, static_cast<float>(device.temperature), device.name, update.rssiLevel);
			break;
		case ShellySensorRole::Lux:
		{
			// The helper only has 8 bits of node id: the light meter is built here with the whole key
			_tLightMeter meter;
			meter.id2 = static_cast<uint8_t>((key >> 16) & 0xFF);
			meter.id3 = static_cast<uint8_t>((key >> 8) & 0xFF);
			meter.id4 = static_cast<uint8_t>(key & 0xFF);
			meter.dunit = device.unit;
			meter.fLux = static_cast<float>(device.value);
			meter.battery_level = static_cast<uint8_t>(update.batteryLevel);
			sDecodeRXMessage(this, reinterpret_cast<const unsigned char*>(&meter), device.name.c_str(), update.batteryLevel, nullptr);
			break;
		}
		case ShellySensorRole::GasPpm:
			SendCustomSensor(nodeId, device.unit, update.batteryLevel, static_cast<float>(device.value), device.name, "ppm", update.rssiLevel);
			break;
		case ShellySensorRole::Tilt:
			SendCustomSensor(nodeId, device.unit, update.batteryLevel, static_cast<float>(device.value), device.name, "deg", update.rssiLevel);
			break;
		case ShellySensorRole::ReturnedEnergy:
			SendKwhMeter(nodeId, device.unit, update.batteryLevel, device.value, device.energyKwh, device.name, update.rssiLevel);
			break;
	}
}

bool CShelly::DeviceRowExists(const std::string& deviceId, const uint8_t unit, const int deviceType)
{
	const auto result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d)", m_HwdID, deviceId.c_str(), static_cast<int>(unit), deviceType);
	return !result.empty();
}

// Runs sendUpdate, and when that created the device (it did not exist before) gives it the wanted switch type.
// A device that already exists is never changed: the user may have chosen another switch type. The lookups are done
// once per device and run, no lock is held across SQL.
void CShelly::SetNewDeviceSwitchType(const ShellyNode& node, const uint8_t unit, const std::string& deviceId, const int deviceType, const int switchType, const std::function<void()>& sendUpdate)
{
	const std::string key = node.id + "/" + std::to_string(unit);
	bool done;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		done = (m_switchTypeDone.find(key) != m_switchTypeDone.end());
	}
	bool missing;
	{
		std::lock_guard<std::mutex> l(m_stateMutex);
		missing = (m_switchTypeMissing.find(key) != m_switchTypeMissing.end());
	}
	if (done || (missing && !m_sql.m_bAcceptNewHardware))
	{
		// Known device, or one that was not created because new hardware is not accepted: no database probes per
		// update. Accepting new hardware again makes the next update probe once more.
		sendUpdate();
		return;
	}

	const bool existed = DeviceRowExists(deviceId, unit, deviceType);
	sendUpdate();
	if (!existed)
	{
		if (!DeviceRowExists(deviceId, unit, deviceType))
		{
			std::lock_guard<std::mutex> l(m_stateMutex);
			m_switchTypeMissing.insert(key);
			return; // not created (new hardware is not accepted): try again once it is
		}
		m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d)", switchType, m_HwdID, deviceId.c_str(), static_cast<int>(unit),
				 deviceType);
	}
	std::lock_guard<std::mutex> l(m_stateMutex);
	m_switchTypeDone.insert(key);
	m_switchTypeMissing.erase(key);
}

void CShelly::ApplySwitch(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	const uint32_t key = ShellyNodeKey(node.id);
	const int nodeId = static_cast<int>(key);
	// Lighting2 DeviceID as the base class builds it (id1 is always 0 here)
	const std::string deviceId = std_format("%X%02X%02X%02X", 0, (key >> 16) & 0xFF, (key >> 8) & 0xFF, key & 0xFF);

	const bool isDimmer = (device.kind == ShellyDeviceKind::Dimmer);
	// SendSwitch turns a percentage into a Lighting2 step with int(16 / 100 * percentage), which gives 16 for 100 percent
	// while the largest step is 15. The percentage is therefore turned into the nearest step here (at least 1 for a
	// light that is on) and the middle of that step's range is passed, which SendSwitch maps back to exactly that step.
	// The step is shown as step * 100 / 15 percent and the command side (DecodeShellyCommand) uses the same scale, so the
	// value does not drift on repeated updates.
	double level = 0.0;
	if (isDimmer && device.on && device.level > 0)
	{
		const int step = std::min(15, std::max(1, static_cast<int>(std::lround(std::min(100, device.level) * 15.0 / 100.0))));
		level = (step + 0.5) * (100.0 / 16.0);
	}
	const auto send = [&]() { SendSwitch(nodeId, device.unit, update.batteryLevel, device.on, level, device.name, std::string(), update.rssiLevel); };

	if (isDimmer)
		SetNewDeviceSwitchType(node, device.unit, deviceId, pTypeLighting2, STYPE_Dimmer, send);
	else
		send();
}

void CShelly::ApplyBlind(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	const uint32_t key = ShellyNodeKey(node.id);
	const int nodeId = static_cast<int>(key);
	const std::string deviceId = std_format("%08X", key);

	uint8_t command = gswitch_sSetLevel;
	uint8_t level = static_cast<uint8_t>(std::min(100, std::max(0, device.level)));
	if (device.blindState == ShellyBlindState::Open)
	{
		command = gswitch_sOpen;
		level = 100;
	}
	else if (device.blindState == ShellyBlindState::Closed)
	{
		command = gswitch_sClose;
		level = 0;
	}

	const auto send = [&]() { SendBlindSwitch(nodeId, device.unit, command, level, device.name, std::string(), update.batteryLevel, static_cast<uint8_t>(update.rssiLevel)); };
	SetNewDeviceSwitchType(node, device.unit, deviceId, pTypeGeneralSwitch, STYPE_BlindsPercentageWithStop, send);
}

// Creates or updates the Domoticz device for one value. Meters are skipped when they repeat the previous value
// within METER_REPEAT_SECONDS (a push channel can deliver many identical statuses); switches, dimmers and blinds are
// always passed on (the base class and the database layer drop what did not change).
void CShelly::ApplyDeviceUpdate(const ShellyNode& node, const ShellyNodeUpdate& update, const ShellyDeviceUpdate& device)
{
	const uint32_t key = ShellyNodeKey(node.id);
	const int nodeId = static_cast<int>(key);

	switch (device.kind)
	{
		case ShellyDeviceKind::Switch:
		case ShellyDeviceKind::Dimmer:
			ApplySwitch(node, update, device);
			return;
		case ShellyDeviceKind::Blind:
			ApplyBlind(node, update, device);
			return;
		case ShellyDeviceKind::Color:
			ApplyColor(node, update, device);
			return;
		case ShellyDeviceKind::Thermostat:
			ApplyThermostat(node, update, device);
			return;
		case ShellyDeviceKind::Input:
			ApplyInput(node, update, device);
			return;
		case ShellyDeviceKind::Binary:
			ApplyBinary(node, update, device);
			return;
		default:
			break;
	}

	if (IsRedundantMeterUpdate(node, device))
		return;

	switch (device.kind)
	{
		case ShellyDeviceKind::Energy:
			SendKwhMeter(nodeId, device.unit, update.batteryLevel, device.value, device.energyKwh, device.name, update.rssiLevel);
			break;
		case ShellyDeviceKind::Power:
			SendWattMeter(nodeId, device.unit, update.batteryLevel, static_cast<float>(device.value), device.name, update.rssiLevel);
			break;
		case ShellyDeviceKind::Voltage:
			SendVoltageSensor(nodeId, device.unit, update.batteryLevel, static_cast<float>(device.value), device.name);
			break;
		case ShellyDeviceKind::Current:
		{
			_tGeneralDevice gdevice;
			gdevice.subtype = sTypeCurrent;
			gdevice.id = device.unit;
			gdevice.intval1 = static_cast<int32_t>((key << 8) | device.unit);
			gdevice.floatval1 = static_cast<float>(device.value);
			gdevice.rssi = static_cast<uint8_t>(update.rssiLevel);
			gdevice.battery_level = static_cast<uint8_t>(update.batteryLevel);
			sDecodeRXMessage(this, reinterpret_cast<const unsigned char*>(&gdevice), device.name.c_str(), update.batteryLevel, nullptr);
			break;
		}
		case ShellyDeviceKind::Frequency:
			SendCustomSensor(nodeId, device.unit, update.batteryLevel, static_cast<float>(device.value), device.name, "Hz", update.rssiLevel);
			break;
		case ShellyDeviceKind::PowerFactor:
			SendPercentageSensor(static_cast<int>((key << 8) | device.unit), device.unit, update.batteryLevel, static_cast<float>(device.value), device.name);
			break;
		case ShellyDeviceKind::Temperature:
			// The temperature helper only has 16 bits of node id: a node whose low 16 bits are owned by another one is skipped
			if (!ClaimTemperatureId(node))
				break;
			SendTempSensor(static_cast<int>(key & 0xFFFF), update.batteryLevel, static_cast<float>(device.value), device.name, update.rssiLevel);
			break;
		case ShellyDeviceKind::Sensor:
			ApplySensor(node, update, device);
			break;
		default:
			break;
	}
}

namespace
{
	enum class ShellyOp
	{
		Off,
		On,
		SetLevel, // percent
		Open,
		Close,
		Stop,
		SetColor,      // color light: color and brightness (percent, -1 keeps the current one)
		SetTemperature // thermostat: setpoint in degrees Celsius
	};

	enum class ShellyPacket
	{
		Lighting2,
		GeneralSwitch,
		ColorSwitch,
		Thermostat
	};

	// What a command packet asks for, as the abstract values used for routing (see ShellyMapping.h).
	struct ShellyDecodedCommand
	{
		uint32_t id = 0;
		uint8_t unit = 0;
		ShellyOp op = ShellyOp::Off;
		int percent = 0; // SetLevel: 0..100
		ShellyPacket packet = ShellyPacket::Lighting2;
		uint8_t subtype = 0;
		// SetColor
		bool hasColor = false; // false: keep the current color
		bool whiteMode = false;
		int kelvin = -1;       // white mode: color temperature asked for (-1 none)
		int red = 0;
		int green = 0;
		int blue = 0;
		int white = 0;
		// SetTemperature
		double temperature = 0.0;
	};

	// Lighting2 (switch, dimmer): level is the step 0..15 (see ApplySwitch), converted to percent as step * 100 / 15.
	// GeneralSwitch (blind): level is already a percentage, 100 is open. In both the command values On/Open and Off/Close
	// are numerically equal (light2_sOn == 1, gswitch_sOpen == 1, ...).
	bool DecodeShellyCommand(const char* pdata, const unsigned char length, ShellyDecodedCommand& out)
	{
		if (pdata == nullptr || length < 3)
			return false;

		const uint8_t packetType = static_cast<uint8_t>(pdata[1]);
		if (packetType == pTypeLighting2)
		{
			if (length < sizeof(tRBUF::LIGHTING2))
				return false;
			const tRBUF* cmd = reinterpret_cast<const tRBUF*>(pdata);
			if (cmd->LIGHTING2.subtype != sTypeAC)
				return false;
			out.id = (static_cast<uint32_t>(cmd->LIGHTING2.id1) << 24) | (static_cast<uint32_t>(cmd->LIGHTING2.id2) << 16) | (static_cast<uint32_t>(cmd->LIGHTING2.id3) << 8)
				 | static_cast<uint32_t>(cmd->LIGHTING2.id4);
			out.unit = cmd->LIGHTING2.unitcode;
			out.packet = ShellyPacket::Lighting2;
			switch (cmd->LIGHTING2.cmnd)
			{
				case light2_sOff:
				case light2_sGroupOff:
					out.op = ShellyOp::Off;
					return true;
				case light2_sOn:
				case light2_sGroupOn:
					out.op = ShellyOp::On;
					return true;
				case light2_sSetLevel:
				case light2_sSetGroupLevel:
					out.op = ShellyOp::SetLevel;
					out.percent = std::min(100, static_cast<int>(std::lround(cmd->LIGHTING2.level * 100.0 / 15.0)));
					return true;
				default:
					return false;
			}
		}
		if (packetType == pTypeGeneralSwitch)
		{
			if (length < sizeof(_tGeneralSwitch))
				return false;
			const _tGeneralSwitch* cmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
			if (cmd->subtype != sSwitchGeneralSwitch)
				return false;
			out.id = static_cast<uint32_t>(cmd->id);
			out.unit = static_cast<uint8_t>(cmd->unitcode);
			out.packet = ShellyPacket::GeneralSwitch;
			switch (cmd->cmnd)
			{
				case gswitch_sOpen:
					out.op = ShellyOp::Open;
					return true;
				case gswitch_sClose:
					out.op = ShellyOp::Close;
					return true;
				case gswitch_sStop:
					out.op = ShellyOp::Stop;
					return true;
				case gswitch_sSetLevel:
					out.op = ShellyOp::SetLevel;
					out.percent = std::min(100, static_cast<int>(cmd->level));
					return true;
				default:
					return false;
			}
		}
		if (packetType == pTypeColorSwitch)
		{
			if (length < sizeof(_tColorSwitch))
				return false;
			const _tColorSwitch* cmd = reinterpret_cast<const _tColorSwitch*>(pdata);
			if (cmd->subtype != sTypeColor_RGB && cmd->subtype != sTypeColor_RGB_W && cmd->subtype != sTypeColor_RGB_W_Z)
				return false;
			out.id = cmd->id;
			out.unit = cmd->dunit;
			out.packet = ShellyPacket::ColorSwitch;
			out.subtype = cmd->subtype;
			out.percent = static_cast<int>(std::min<uint32_t>(100, cmd->value));
			switch (cmd->command)
			{
				case Color_LedOff:
					out.op = ShellyOp::Off;
					return true;
				case Color_LedOn:
					out.op = ShellyOp::On;
					return true;
				case Color_LedFull:
				case Color_FullBrightness:
					out.op = ShellyOp::SetLevel;
					out.percent = 100;
					return true;
				case Color_SetBrightnessLevel:
					out.op = ShellyOp::SetLevel;
					return true;
				case Color_SetColorToWhite:
					out.op = ShellyOp::SetColor;
					out.whiteMode = true;
					out.percent = -1;
					return true;
				case Color_SetColor:
					out.op = ShellyOp::SetColor;
					switch (cmd->color.mode)
					{
						case ColorModeRGB:
						case ColorModeCustom:
							out.hasColor = true;
							out.red = cmd->color.r;
							out.green = cmd->color.g;
							out.blue = cmd->color.b;
							out.white = std::max(cmd->color.cw, cmd->color.ww); // a single white channel: the web UI sets both
							return true;
						case ColorModeWhite:
							out.hasColor = true;
							out.whiteMode = true;
							return true;
						case ColorModeTemp:
							// An RGBW light has no color temperature: white. A bulb takes it (0 is cold, 255 warm; 6500 K to 3000 K).
							// The scale is linear in Kelvin, unlike the mired based interpolation of the Hue and Matter drivers: the
							// middle of the slider is 4750 K here (about 4100 K when interpolated in mired). Shelly bulbs only accept 3000..6500 K.
							out.hasColor = true;
							out.whiteMode = true;
							out.kelvin = 6500 - static_cast<int>(std::lround(cmd->color.t * 3500.0 / 255.0));
							return true;
						default:
							return true; // brightness only
					}
				default:
					return false;
			}
		}
		if (packetType == pTypeThermostat6)
		{
			if (length < sizeof(_tThermostat6))
				return false;
			const _tThermostat6* cmd = reinterpret_cast<const _tThermostat6*>(pdata);
			// Only a setpoint is a command: a packet that carries a reading is the device reporting
			if (cmd->subtype != sTypeThermostat6Temp || (cmd->update_flags & 0x02) == 0 || (cmd->update_flags & 0x01) != 0 || !std::isfinite(cmd->setpoint))
				return false;
			out.id = (static_cast<uint32_t>(cmd->id1) << 24) | (static_cast<uint32_t>(cmd->id2) << 16) | (static_cast<uint32_t>(cmd->id3) << 8) | static_cast<uint32_t>(cmd->id4);
			out.unit = cmd->dunit;
			out.packet = ShellyPacket::Thermostat;
			out.op = ShellyOp::SetTemperature;
			out.temperature = cmd->setpoint;
			return true;
		}
		return false;
	}

	// The brightness/color command of a color light. `memo` is what the light last reported: a brightness command carries no color.
	// `bulbWhite` is set for a Gen1 bulb: its white mode is a mode of the light (mode=white with brightness and color temperature),
	// not a mix of color channels. The Gen1 command builder reads the "mode" and "temp" params (see ShellyGen1BuildCommandPath).
	bool BuildColorCommand(const ShellyDecodedCommand& decoded, const ShellyColorMemo& memo, const bool rgbw, const bool bulbWhite, ShellyCommand& command)
	{
		command.component = rgbw ? ShellyCmd::COMPONENT_RGBW : ShellyCmd::COMPONENT_RGB;
		if (decoded.op == ShellyOp::On)
		{
			command.action = ShellyCmd::ACTION_ON;
			return true;
		}
		if (decoded.op == ShellyOp::Off)
		{
			command.action = ShellyCmd::ACTION_OFF;
			return true;
		}
		if (decoded.op != ShellyOp::SetLevel && decoded.op != ShellyOp::SetColor)
			return false;

		int percent = decoded.percent;
		if (percent < 0)
			percent = (memo.known && memo.level >= 0) ? memo.level : 100; // keep the brightness
		if (percent == 0)
		{
			command.action = ShellyCmd::ACTION_OFF;
			return true;
		}

		int red = 0;
		int green = 0;
		int blue = 0;
		int white = 0;
		bool whiteMode = false;
		if (decoded.op == ShellyOp::SetColor && (decoded.hasColor || decoded.whiteMode))
		{
			whiteMode = decoded.whiteMode;
			red = decoded.red;
			green = decoded.green;
			blue = decoded.blue;
			white = decoded.white;
		}
		else if (memo.known)
		{
			whiteMode = memo.whiteMode;
			red = memo.red;
			green = memo.green;
			blue = memo.blue;
			white = std::max(0, memo.white);
		}
		else
			return false; // the color is not known yet: wait for the first status

		if (whiteMode && bulbWhite)
		{
			command.action = ShellyCmd::ACTION_SET_COLOR;
			command.params["mode"] = "white";
			if (decoded.op == ShellyOp::SetColor && decoded.kelvin > 0)
				command.params["temp"] = std::min(6500, std::max(3000, decoded.kelvin));
			command.params[ShellyCmd::PARAM_BRIGHTNESS] = std::min(100, percent);
			return true;
		}
		if (whiteMode)
		{
			// the white channel at full, or all channels at full for a light without one
			red = green = blue = rgbw ? 0 : 255;
			white = 255;
		}
		command.action = ShellyCmd::ACTION_SET_COLOR;
		command.params[ShellyCmd::PARAM_RED] = std::min(255, std::max(0, red));
		command.params[ShellyCmd::PARAM_GREEN] = std::min(255, std::max(0, green));
		command.params[ShellyCmd::PARAM_BLUE] = std::min(255, std::max(0, blue));
		if (rgbw)
			command.params[ShellyCmd::PARAM_WHITE] = std::min(255, std::max(0, white));
		command.params[ShellyCmd::PARAM_BRIGHTNESS] = std::min(100, percent);
		return true;
	}

	// False when the operation does not exist for that kind of device (for example Stop on a switch).
	bool BuildShellyCommand(const ShellyDeviceRoute& route, const ShellyDecodedCommand& decoded, const ShellyColorMemo& memo, const bool bulbWhite, ShellyCommand& command)
	{
		command.component = route.component;
		command.channel = route.channel;
		command.params = Json::Value();

		switch (route.kind)
		{
			case ShellyDeviceKind::Color:
				if (decoded.packet != ShellyPacket::ColorSwitch)
					return false;
				return BuildColorCommand(decoded, memo, decoded.subtype != sTypeColor_RGB, bulbWhite, command);
			case ShellyDeviceKind::Thermostat:
				if (decoded.packet != ShellyPacket::Thermostat || decoded.op != ShellyOp::SetTemperature)
					return false;
				command.action = ShellyCmd::ACTION_SET_TEMPERATURE;
				command.params[ShellyCmd::PARAM_TEMPERATURE] = decoded.temperature;
				return true;
			case ShellyDeviceKind::Switch:
				if (decoded.packet != ShellyPacket::Lighting2)
					return false;
				if (decoded.op == ShellyOp::On || (decoded.op == ShellyOp::SetLevel && decoded.percent > 0))
					command.action = ShellyCmd::ACTION_ON;
				else if (decoded.op == ShellyOp::Off || decoded.op == ShellyOp::SetLevel)
					command.action = ShellyCmd::ACTION_OFF;
				else
					return false;
				return true;
			case ShellyDeviceKind::Dimmer:
				if (decoded.packet != ShellyPacket::Lighting2)
					return false;
				if (decoded.op == ShellyOp::On)
					command.action = ShellyCmd::ACTION_ON;
				else if (decoded.op == ShellyOp::Off || (decoded.op == ShellyOp::SetLevel && decoded.percent <= 0))
					command.action = ShellyCmd::ACTION_OFF;
				else if (decoded.op == ShellyOp::SetLevel)
				{
					// "on" with a brightness turns the light on at that level, also when it is off now
					command.action = ShellyCmd::ACTION_ON;
					command.params[ShellyCmd::PARAM_BRIGHTNESS] = decoded.percent;
				}
				else
					return false;
				return true;
			case ShellyDeviceKind::Blind:
				if (decoded.packet != ShellyPacket::GeneralSwitch)
					return false;
				switch (decoded.op)
				{
					case ShellyOp::Open:
						command.action = ShellyCmd::ACTION_OPEN;
						return true;
					case ShellyOp::Close:
						command.action = ShellyCmd::ACTION_CLOSE;
						return true;
					case ShellyOp::Stop:
						command.action = ShellyCmd::ACTION_STOP;
						return true;
					case ShellyOp::SetLevel:
						command.action = ShellyCmd::ACTION_SET_POSITION;
						command.params[ShellyCmd::PARAM_POSITION] = decoded.percent;
						return true;
					default:
						return false;
				}
			default:
				return false;
		}
	}
} // namespace

// Called from the web and scheduler threads. The lease keeps the driver alive for the duration of the command; no
// driver lock is held across the network request (SendShellyCommand only looks the connection up under it). When
// this returns true, mainworker pushes the command to the RX queue, so the device is updated optimistically; the
// status that follows (push, or the early poll requested below for a node without push) corrects it to what the
// Shelly really did.
bool CShelly::WriteToHardware(const char* pdata, const unsigned char length)
{
	ShellyDecodedCommand decoded;
	if (!DecodeShellyCommand(pdata, length, decoded))
		return false;

	ShellyDeviceRoute route;
	if (!ShellyRouteDevice(decoded.id, decoded.unit, route))
	{
		Debug(DEBUG_HARDWARE, "Command for unit 0x%02X is not for a controllable device", decoded.unit);
		// Sensors, meters, input contacts and push button selectors are read-only mirrors of the Shelly: a write to them
		// returns false, so nothing is sent and Domoticz does not apply the value (the next status shows the real state).
		return false;
	}

	ShellyLease lease = ShellyLease::Acquire(m_HwdID);
	if (!lease || lease.operator->() != this)
		return false;

	ShellyNode node;
	if (!FindNodeByKey(route.nodeKey, node))
	{
		// While the hardware stops the node list is already gone: not worth an error
		if (IsStopRequested(0))
			Debug(DEBUG_HARDWARE, "Command for a device (id 0x%06X) while the hardware is stopping", route.nodeKey);
		else
			Log(LOG_ERROR, "Command for an unknown device (id 0x%06X)", route.nodeKey);
		return false;
	}

	ShellyColorMemo memo;
	bool hasMemo = true;
	if (route.kind == ShellyDeviceKind::Color)
		hasMemo = GetColorMemo(node, decoded.unit, memo);

	ShellyCommand command;
	const bool bulbWhite = (node.gen == SHELLY_GEN1) && ShellyIsGen1BulbModel(node.model);
	if (!BuildShellyCommand(route, decoded, memo, bulbWhite, command))
	{
		// A brightness or color command that does not carry a color needs the last reported one.
		if (route.kind == ShellyDeviceKind::Color && !hasMemo && (decoded.op == ShellyOp::SetLevel || decoded.op == ShellyOp::SetColor))
		{
			Log(LOG_ERROR, "%s: the color command is refused, there is no status of the device yet (the color is not known)", node.name.c_str());
			return false;
		}
		Log(LOG_ERROR, "%s: the command is not supported for this device", node.name.c_str());
		return false;
	}

	const ShellyResult result = SendShellyCommand(node.id, command);
	if (!result)
	{
		Log(LOG_ERROR, "%s (%s): command %s failed (HTTP %d): %s", node.name.c_str(), node.ip.c_str(), command.action.c_str(), result.httpStatus, result.error.c_str());
		return false;
	}

	// The command is applied optimistically by Domoticz. The status that follows may equal the one before the command (the
	// Shelly refused or corrected it), so it must not be dropped as a repeat.
	InvalidateDedupe(node, decoded.unit);
	RequestRepoll(node.id);
	return true;
}

// Web commands. Every command is admin only (see the registration) and works on a lease of the driver, so the
// instance cannot be destroyed while a command runs.
namespace http {
	namespace server {
		namespace
		{
			void SetShellyError(Json::Value& root, const std::string& title, const std::string& message)
			{
				root["status"] = "ERR";
				root["title"] = title;
				root["message"] = message;
			}

			// Empty means the default port 80. Anything but a plain number in 1..65535 is rejected.
			bool ParseShellyPort(const std::string& text, int& port)
			{
				if (text.empty())
				{
					port = 80;
					return true;
				}
				if (text.size() > 5 || text.find_first_not_of("0123456789") != std::string::npos)
					return false;
				port = std::atoi(text.c_str());
				return ShellyIsValidPort(port);
			}

			Json::Value ShellyNodeToWebJson(const ShellyNode& node)
			{
				Json::Value json;
				json["id"] = node.id;
				json["deviceId"] = node.deviceId;
				json["name"] = node.name;
				json["ip"] = node.ip;
				json["port"] = node.port;
				json["gen"] = node.gen;
				json["model"] = node.model;
				json["fw"] = node.fw;
				json["authRequired"] = node.auth;
				json["sleepy"] = node.sleepy;
				return json;
			}

			const char* ShellyScanStateName(const ShellyScanState state)
			{
				switch (state)
				{
					case ShellyScanState::Running:
						return "running";
					case ShellyScanState::Finished:
						return "finished";
					case ShellyScanState::Cancelled:
						return "cancelled";
					default:
						return "idle";
				}
			}
		} // namespace

		void CWebServer::Cmd_ShellyGetNodes(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			const std::string title = "ShellyGetNodes";
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, title, "Hardware not found");
				return;
			}

			root["status"] = "OK";
			root["title"] = title;
			root["result"] = Json::Value(Json::arrayValue);
			int ii = 0;
			for (const auto& node : hardware->GetNodes())
				root["result"][ii++] = ShellyNodeToWebJson(node);
		}

		void CWebServer::Cmd_ShellyAddNode(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			const std::string title = "ShellyAddNode";
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, title, "Hardware not found");
				return;
			}

			const std::string ip = HTMLSanitizer::Sanitize(request::findValue(&req, "ip"));
			const std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			int port = 80;
			if (ip.empty() || !ShellyIsValidHost(ip))
			{
				SetShellyError(root, title, "Invalid device address");
				return;
			}
			if (!ParseShellyPort(request::findValue(&req, "port"), port))
			{
				SetShellyError(root, title, "Invalid port");
				return;
			}

			std::string reason;
			if (!hardware->AddNode(ip, port, name, &reason))
			{
				SetShellyError(root, title, reason.empty() ? "The device could not be added, check the address and the log" : reason);
				return;
			}
			root["status"] = "OK";
			root["title"] = title;
			for (const auto& node : hardware->GetNodes())
			{
				if (node.ip == ip && node.port == port)
				{
					root["result"] = ShellyNodeToWebJson(node);
					break;
				}
			}
		}

		void CWebServer::Cmd_ShellyRemoveNode(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			const std::string title = "ShellyRemoveNode";
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, title, "Hardware not found");
				return;
			}

			const std::string nodeId = ShellyNormalizeMac(request::findValue(&req, "nodeid"));
			if (nodeId.empty())
			{
				SetShellyError(root, title, "Invalid device id");
				return;
			}
			if (!hardware->RemoveNode(nodeId))
			{
				SetShellyError(root, title, "Device not found");
				return;
			}
			root["status"] = "OK";
			root["title"] = title;
		}

		void CWebServer::Cmd_ShellyRenameNode(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			const std::string title = "ShellyRenameNode";
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, title, "Hardware not found");
				return;
			}

			const std::string nodeId = ShellyNormalizeMac(request::findValue(&req, "nodeid"));
			if (nodeId.empty())
			{
				SetShellyError(root, title, "Invalid device id");
				return;
			}
			const std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			if (ShellySanitizeText(name).empty())
			{
				SetShellyError(root, title, "Invalid name");
				return;
			}
			if (!hardware->RenameNode(nodeId, name))
			{
				SetShellyError(root, title, "Device not found");
				return;
			}
			root["status"] = "OK";
			root["title"] = title;
		}

		void CWebServer::Cmd_ShellyClearNodes(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, "ShellyClearNodes", "Hardware not found");
				return;
			}

			hardware->RemoveAllNodes();
			root["status"] = "OK";
			root["title"] = "ShellyClearNodes";
		}

		void CWebServer::Cmd_ShellyScan(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			const std::string title = "ShellyScan";
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, title, "Hardware not found");
				return;
			}

			int port = 80;
			if (!ParseShellyPort(request::findValue(&req, "port"), port))
			{
				SetShellyError(root, title, "Invalid port");
				return;
			}

			std::string error;
			if (!hardware->StartScan(request::findValue(&req, "range"), port, error))
			{
				SetShellyError(root, title, error);
				return;
			}
			root["status"] = "OK";
			root["title"] = title;
		}

		void CWebServer::Cmd_ShellyScanStatus(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, "ShellyScanStatus", "Hardware not found");
				return;
			}

			const ShellyScanStatus scan = hardware->GetScanStatus();
			const std::vector<ShellyNode> nodes = hardware->GetNodes();

			root["status"] = "OK";
			root["title"] = "ShellyScanStatus";
			root["state"] = ShellyScanStateName(scan.state);
			root["range"] = scan.range;
			root["total"] = static_cast<Json::UInt64>(scan.total);
			root["probed"] = static_cast<Json::UInt64>(scan.probed);
			root["found"] = static_cast<Json::UInt64>(scan.candidates.size());
			root["result"] = Json::Value(Json::arrayValue);
			int ii = 0;
			for (const auto& candidate : scan.candidates)
			{
				Json::Value item;
				item["ip"] = candidate.ip;
				item["port"] = candidate.port;
				item["gen"] = candidate.identity.gen;
				item["model"] = candidate.identity.model;
				item["name"] = candidate.name;
				item["mac"] = candidate.identity.mac;
				item["fw"] = candidate.identity.fw;
				item["authRequired"] = candidate.identity.authRequired;
				const auto known = std::find_if(nodes.begin(), nodes.end(), [&candidate](const ShellyNode& n) { return n.id == candidate.identity.mac; });
				item["alreadyAdded"] = (known != nodes.end());
				item["ipChanged"] = (known != nodes.end()) && (known->ip != candidate.ip || known->port != candidate.port || known->gen != candidate.identity.gen);
				root["result"][ii++] = item;
			}
		}

		void CWebServer::Cmd_ShellyScanCancel(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			ShellyLease hardware = ShellyLease::Acquire(request::findValue(&req, "idx"));
			if (!hardware)
			{
				SetShellyError(root, "ShellyScanCancel", "Hardware not found");
				return;
			}

			hardware->CancelScan();
			root["status"] = "OK";
			root["title"] = "ShellyScanCancel";
		}
	} // namespace server
} // namespace http
