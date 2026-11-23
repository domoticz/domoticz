#pragma once

// Subnet scan for the Shelly integration: expands a user entered IPv4 range and probes every address with
// GET /shelly (outbound unicast, so it also works from a bridged Docker container). A scan only collects
// candidates; adding them is a separate, explicit step.

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ShellyTypes.h"

// At most 1024 addresses (a /22).
constexpr size_t SHELLY_SCAN_MAX_HOSTS = 1024;
constexpr int SHELLY_SCAN_MIN_PREFIX = 22;
constexpr int SHELLY_SCAN_CONCURRENCY = 24;
constexpr int SHELLY_SCAN_TIMEOUT_SEC = 2;

// Expands a scan range into IPv4 addresses. Pure: no I/O, no logging.
// Accepted forms (surrounding white space is ignored, octets are decimal without leading zeros):
//   "192.168.1.0/24"          CIDR; network and broadcast address are skipped for prefixes up to /30
//   "192.168.1.10-50"         last octet range
//   "192.168.1.10-192.168.2.5" explicit range
//   "192.168.1.20"            a single address
// Rejected: more than SHELLY_SCAN_MAX_HOSTS addresses (CIDR prefix shorter than /22), a reversed range, and
// addresses outside the private, link-local and loopback ranges (the scan is for the LAN, not the internet).
// A range must lie inside one such block. On failure hosts is empty and error says why.
// When normalized is given it receives the trimmed range text (also on failure), the form to store or log.
bool ShellyExpandScanRange(const std::string& range, std::vector<std::string>& hosts, std::string& error, std::string* normalized = nullptr);

// Notes on trust boundaries:
// - The scan itself is restricted to the private ranges above. Adding a device by address (CShelly::AddNode) is
//   deliberately not: it is admin only, accepts host names, follows no redirects and requests one fixed path.
// - ShellyIdentifyDevice downloads the answer through HTTPClient, which has no size cap, so a hostile host in the
//   scanned range could make a probe read a large body. The probe timeout bounds the time, not the size.

struct ShellyScanCandidate
{
	size_t order = 0;       // position in the scanned range, for stable ordering
	std::string ip;
	int port = 80;
	ShellyIdentity identity;
	std::string name;       // device reported name, or a generated default
};

enum class ShellyScanState
{
	Idle,       // no scan was started
	Running,
	Finished,   // every address was probed
	Cancelled
};

struct ShellyScanStatus
{
	ShellyScanState state = ShellyScanState::Idle;
	std::string range;
	size_t total = 0;       // addresses to probe
	size_t probed = 0;      // addresses probed so far
	std::vector<ShellyScanCandidate> candidates; // ordered by address
};

// One scan at a time. Thread safe. The destructor cancels a running scan and waits for it.
class ShellyScanner
{
      public:
	ShellyScanner() = default;
	~ShellyScanner();
	ShellyScanner(const ShellyScanner&) = delete;
	ShellyScanner& operator=(const ShellyScanner&) = delete;

	// Starts a scan of an already expanded host list in the background. Returns false (with error) when a scan
	// is running, the list is empty, or no worker could be started. Replaces the previous results.
	bool Start(const std::string& range, std::vector<std::string> hosts, int port, std::string& error);
	// Asks a running scan to stop; probes in flight finish first (at most the probe timeout).
	void Cancel();
	// Cancels and waits until all workers are gone.
	void Stop();
	ShellyScanStatus GetStatus() const;

      private:
	void WorkerMain();
	void WorkerDone();
	void JoinWorkers();

      private:
	std::mutex m_lifecycleMutex;       // serializes Start/Stop/JoinWorkers
	std::vector<std::thread> m_workers; // guarded by m_lifecycleMutex

	mutable std::mutex m_mutex;        // guards the members below except the atomics
	ShellyScanState m_state = ShellyScanState::Idle;
	std::string m_range;
	std::vector<std::string> m_hosts;  // written only while no worker is active
	int m_port = 80;                   // same: workers read it without the lock
	size_t m_probed = 0;
	int m_active = 0;
	std::vector<ShellyScanCandidate> m_candidates;

	std::atomic<size_t> m_next{ 0 };
	std::atomic<bool> m_cancel{ false };
};
