#include "stdafx.h"
#include "ShellyScan.h"
#include <algorithm>
#include <cstdint>
#include "ShellyIdentify.h"

namespace
{
	// Strict dotted quad: four decimal octets of 1..3 digits, no sign, no leading zeros, nothing else.
	bool ParseIPv4(const std::string& text, uint32_t& value)
	{
		uint32_t result = 0;
		size_t pos = 0;
		for (int octet = 0; octet < 4; octet++)
		{
			const size_t start = pos;
			unsigned int part = 0;
			while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9' && pos - start < 4)
			{
				part = part * 10 + static_cast<unsigned int>(text[pos] - '0');
				pos++;
			}
			const size_t digits = pos - start;
			if (digits == 0 || digits > 3 || part > 255)
				return false;
			if (digits > 1 && text[start] == '0')
				return false;
			result = (result << 8) | part;
			if (octet < 3)
			{
				if (pos >= text.size() || text[pos] != '.')
					return false;
				pos++;
			}
		}
		if (pos != text.size())
			return false;
		value = result;
		return true;
	}

	std::string FormatIPv4(const uint32_t value)
	{
		return std::to_string((value >> 24) & 0xFF) + "." + std::to_string((value >> 16) & 0xFF) + "." + std::to_string((value >> 8) & 0xFF) + "." + std::to_string(value & 0xFF);
	}

	bool ParseDecimal(const std::string& text, const unsigned int maxValue, unsigned int& value)
	{
		if (text.empty() || text.size() > 3)
			return false;
		unsigned int result = 0;
		for (const char c : text)
		{
			if (c < '0' || c > '9')
				return false;
			result = result * 10 + static_cast<unsigned int>(c - '0');
		}
		if (text.size() > 1 && text[0] == '0')
			return false;
		if (result > maxValue)
			return false;
		value = result;
		return true;
	}

	// Identifies the LAN block an address belongs to, 0 when it is not in a block the scan may touch.
	int ScanBlock(const uint32_t ip)
	{
		if ((ip >> 24) == 10)
			return 1;
		if ((ip >> 20) == ((172u << 4) | 1)) // 172.16.0.0/12
			return 2;
		if ((ip >> 16) == ((192u << 8) | 168))
			return 3;
		if ((ip >> 16) == ((169u << 8) | 254))
			return 4;
		if ((ip >> 24) == 127)
			return 5;
		return 0;
	}

	std::string Trim(const std::string& text)
	{
		const char* ws = " \t\r\n";
		const size_t first = text.find_first_not_of(ws);
		if (first == std::string::npos)
			return std::string();
		return text.substr(first, text.find_last_not_of(ws) - first + 1);
	}
} // namespace

bool ShellyExpandScanRange(const std::string& range, std::vector<std::string>& hosts, std::string& error, std::string* normalized)
{
	hosts.clear();
	error.clear();

	const std::string text = Trim(range);
	if (normalized)
		*normalized = text;
	if (text.empty())
	{
		error = "No address range given";
		return false;
	}

	uint64_t first = 0;
	uint64_t last = 0; // inclusive

	const size_t slash = text.find('/');
	const size_t dash = text.find('-');
	if (slash != std::string::npos)
	{
		if (dash != std::string::npos)
		{
			error = "Invalid address range";
			return false;
		}
		uint32_t ip = 0;
		unsigned int prefix = 0;
		if (!ParseIPv4(text.substr(0, slash), ip) || !ParseDecimal(text.substr(slash + 1), 32, prefix))
		{
			error = "Invalid network, use for example 192.168.1.0/24";
			return false;
		}
		if (prefix < static_cast<unsigned int>(SHELLY_SCAN_MIN_PREFIX))
		{
			error = "The network is too large, use /" + std::to_string(SHELLY_SCAN_MIN_PREFIX) + " or smaller (at most " + std::to_string(SHELLY_SCAN_MAX_HOSTS) + " addresses)";
			return false;
		}
		const uint32_t mask = (prefix == 0) ? 0u : (0xFFFFFFFFu << (32 - prefix));
		const uint64_t network = ip & mask;
		const uint64_t broadcast = network | (~mask & 0xFFFFFFFFu);
		first = network;
		last = broadcast;
		if (prefix <= 30)
		{
			first++;
			last--;
		}
	}
	else if (dash != std::string::npos)
	{
		uint32_t startIp = 0;
		if (!ParseIPv4(text.substr(0, dash), startIp))
		{
			error = "Invalid address range, use for example 192.168.1.10-50";
			return false;
		}
		const std::string tail = text.substr(dash + 1);
		uint32_t endIp = 0;
		unsigned int lastOctet = 0;
		if (tail.find('.') != std::string::npos)
		{
			if (!ParseIPv4(tail, endIp))
			{
				error = "Invalid address range, use for example 192.168.1.10-192.168.1.50";
				return false;
			}
		}
		else if (ParseDecimal(tail, 255, lastOctet))
			endIp = (startIp & 0xFFFFFF00u) | lastOctet;
		else
		{
			error = "Invalid address range, use for example 192.168.1.10-50";
			return false;
		}
		if (endIp < startIp)
		{
			error = "The end of the range is before its start";
			return false;
		}
		first = startIp;
		last = endIp;
	}
	else
	{
		uint32_t ip = 0;
		if (!ParseIPv4(text, ip))
		{
			error = "Invalid address, use for example 192.168.1.0/24";
			return false;
		}
		first = last = ip;
	}

	if (last < first)
	{
		error = "No addresses in this range";
		return false;
	}
	if (last - first + 1 > SHELLY_SCAN_MAX_HOSTS)
	{
		error = "The range is too large (at most " + std::to_string(SHELLY_SCAN_MAX_HOSTS) + " addresses)";
		return false;
	}

	const int block = ScanBlock(static_cast<uint32_t>(first));
	if (block == 0 || block != ScanBlock(static_cast<uint32_t>(last)))
	{
		error = "Only private, link-local and loopback ranges (10.x.x.x, 172.16-31.x.x, 192.168.x.x, 169.254.x.x, 127.x.x.x) can be scanned";
		return false;
	}

	hosts.reserve(static_cast<size_t>(last - first + 1));
	for (uint64_t ip = first; ip <= last; ip++)
		hosts.push_back(FormatIPv4(static_cast<uint32_t>(ip)));
	return true;
}

ShellyScanner::~ShellyScanner()
{
	Stop();
}

bool ShellyScanner::Start(const std::string& range, std::vector<std::string> hosts, const int port, std::string& error)
{
	if (hosts.empty())
	{
		error = "No addresses to scan";
		return false;
	}
	if (!ShellyIsValidPort(port))
	{
		error = "Invalid port";
		return false;
	}

	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	{
		std::lock_guard<std::mutex> l(m_mutex);
		if (m_active > 0)
		{
			error = "A scan is already running";
			return false;
		}
	}
	JoinWorkers();

	const size_t workerCount = std::min(hosts.size(), static_cast<size_t>(SHELLY_SCAN_CONCURRENCY));
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_hosts = std::move(hosts);
		m_range = range;
		m_port = port;
		m_probed = 0;
		m_candidates.clear();
		m_next = 0;
		m_cancel = false;
		m_state = ShellyScanState::Running;
		m_active = static_cast<int>(workerCount);
	}

	size_t started = 0;
	for (size_t i = 0; i < workerCount; i++)
	{
		try
		{
			m_workers.emplace_back([this] { WorkerMain(); });
			started++;
		}
		catch (const std::exception&)
		{
			break;
		}
	}

	if (started == 0)
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_active = 0;
		m_state = ShellyScanState::Idle;
		m_hosts.clear();
		m_range.clear();
		m_probed = 0;
		m_candidates.clear();
		error = "Unable to start the scan";
		return false;
	}
	for (size_t i = started; i < workerCount; i++)
		WorkerDone(); // workers that could not be started will never report themselves
	return true;
}

void ShellyScanner::Cancel()
{
	m_cancel = true;
}

void ShellyScanner::Stop()
{
	Cancel();
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	JoinWorkers();
}

void ShellyScanner::JoinWorkers()
{
	for (auto& worker : m_workers)
	{
		if (worker.joinable())
			worker.join();
	}
	m_workers.clear();
}

void ShellyScanner::WorkerMain()
{
	// Reports this worker as finished exactly once, whatever way the loop is left.
	struct DoneGuard
	{
		ShellyScanner* scanner;
		~DoneGuard()
		{
			scanner->WorkerDone();
		}
	} doneGuard{ this };

	for (;;)
	{
		if (m_cancel)
			break;
		const size_t index = m_next.fetch_add(1);
		if (index >= m_hosts.size())
			break;

		bool counted = false;
		try
		{
			const std::string& ip = m_hosts[index];
			const int port = m_port; // set before the workers were started and not changed while they run

			ShellyIdentity identity;
			const ShellyResult probe = ShellyIdentifyDevice(ip, port, identity, SHELLY_SCAN_TIMEOUT_SEC);
			const bool found = probe && identity.IsValid();

			ShellyScanCandidate candidate;
			if (found)
			{
				candidate.order = index;
				candidate.ip = ip;
				candidate.port = port;
				candidate.identity = identity;
				candidate.name = ShellyNodeFromIdentity(identity, ip, port, std::string()).name;
			}

			std::lock_guard<std::mutex> l(m_mutex);
			m_probed++;
			counted = true;
			if (!found)
				continue;
			// The same device can answer on several addresses: keep the one lowest in the range, whichever finished first.
			const auto same = std::find_if(m_candidates.begin(), m_candidates.end(), [&identity](const ShellyScanCandidate& c) { return c.identity.mac == identity.mac; });
			if (same == m_candidates.end())
				m_candidates.push_back(std::move(candidate));
			else if (index < same->order)
				*same = std::move(candidate);
		}
		catch (...)
		{
			if (!counted)
			{
				try
				{
					std::lock_guard<std::mutex> l(m_mutex);
					m_probed++;
				}
				catch (...)
				{
				}
			}
		}
	}
}

void ShellyScanner::WorkerDone()
{
	std::lock_guard<std::mutex> l(m_mutex);
	if (--m_active == 0)
		m_state = (m_probed < m_hosts.size()) ? ShellyScanState::Cancelled : ShellyScanState::Finished;
}

ShellyScanStatus ShellyScanner::GetStatus() const
{
	ShellyScanStatus status;
	std::lock_guard<std::mutex> l(m_mutex);
	status.state = m_state;
	status.range = m_range;
	status.total = m_hosts.size();
	status.probed = m_probed;
	status.candidates = m_candidates;
	std::sort(status.candidates.begin(), status.candidates.end(), [](const ShellyScanCandidate& a, const ShellyScanCandidate& b) { return a.order < b.order; });
	return status;
}
