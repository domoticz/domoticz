#pragma once

// Device identification for the Shelly integration: probe GET /shelly, parse the identity document
// and (de)serialize the node model used for persistence.

#include <string>
#include <vector>
#include "ShellyTypes.h"

// Strips control characters from user or device supplied text and caps it at 64 bytes (at a UTF-8 boundary).
std::string ShellySanitizeText(const std::string& text);

// Normalize a MAC address: upper case hex, separators (":", "-", ".", " ") removed.
// "a8:03:2a:be:54:dc" -> "A8032ABE54DC". Returns an empty string unless the input holds exactly 12 hex digits.
std::string ShellyNormalizeMac(const std::string& mac);

// True for an IPv4 or IPv6 literal (without brackets or zone id) or an RFC 1123 host name (a single trailing dot is accepted).
bool ShellyIsValidHost(const std::string& host);

// True for 1..65535.
bool ShellyIsValidPort(int port);

// Host as it must appear in a URL: IPv6 literals are bracketed. The host must be valid.
std::string ShellyHostForUrl(const std::string& host);

// Parse the body of GET /shelly. Gen1 documents carry "type" and no "gen"; Gen2+ documents carry "gen".
// Returns false when the document is not a recognizable Shelly identity. Pure: no I/O, no logging.
bool ShellyParseIdentity(const std::string& body, ShellyIdentity& identity);

// Probe http://<ip>:<port>/shelly (no authentication needed on any generation).
// Fails (without any request) on an invalid host or port, and when the device is unreachable, answers with
// an HTTP error, answers with more than 64 KiB, or does not answer like a Shelly.
ShellyResult ShellyIdentifyDevice(const std::string& ip, int port, ShellyIdentity& identity, int timeoutSec = 3);

// Build a node from a probe result. An empty name defaults to the device reported name, then the device id,
// then "Shelly <model> <last 6 digits of the MAC>".
ShellyNode ShellyNodeFromIdentity(const ShellyIdentity& identity, const std::string& ip, int port, const std::string& name);

// Persistence helpers. Nodes are stored as {"nodes":[{...},...]}.
Json::Value ShellyNodeToJson(const ShellyNode& node);
// Rejects entries whose id is not a normalized MAC (ShellyNormalizeMac(id) == id), without a valid host (IP or
// host name), with a generation outside 1..4 or with a port outside 1..65535 (the port defaults to 80 only when
// absent). Absent sleepy/sleepPeriodSec default to false/0. Device supplied text is stripped of control
// characters and capped at 64 bytes.
bool ShellyNodeFromJson(const Json::Value& json, ShellyNode& node);
std::string ShellyNodesToString(const std::vector<ShellyNode>& nodes);
// Returns false when a non-empty string is not a valid node document (nodes is then empty). Rejected and
// repeated (same id) entries are skipped and described in `warnings` (when given); the function still returns true
// for them. The first entry of a repeated id wins.
bool ShellyNodesFromString(const std::string& str, std::vector<ShellyNode>& nodes, std::vector<std::string>* warnings = nullptr);
