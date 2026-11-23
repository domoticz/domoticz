#include "stdafx.h"
#include "ShellyIdentify.h"
#include <algorithm>
#include <cctype>
#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include "../main/Helper.h"
#include "../main/json_helper.h"
#include "ShellyHttp.h"

namespace
{
	// Longest device supplied text (name, model, firmware, device id) that is accepted.
	constexpr size_t MAX_DEVICE_TEXT = 64;

	// Removes control characters and caps the length at a UTF-8 character boundary.
	std::string SanitizeDeviceText(const std::string& text)
	{
		std::string out;
		out.reserve(std::min(text.size(), MAX_DEVICE_TEXT));
		for (const char c : text)
		{
			const unsigned char uc = static_cast<unsigned char>(c);
			if (uc < 0x20 || uc == 0x7F)
				continue;
			out += c;
		}
		if (out.size() > MAX_DEVICE_TEXT)
		{
			size_t cut = MAX_DEVICE_TEXT;
			// Do not split a multi-byte sequence: step back over continuation bytes (10xxxxxx).
			while (cut > 0 && (static_cast<unsigned char>(out[cut]) & 0xC0) == 0x80)
				cut--;
			out.resize(cut);
		}
		return out;
	}

	std::string JsonString(const Json::Value& json, const char* key)
	{
		const Json::Value& v = json[key];
		return v.isString() ? SanitizeDeviceText(v.asString()) : std::string();
	}

	bool IsValidHostnameLabel(const std::string& label)
	{
		if (label.empty() || label.size() > 63 || label.front() == '-' || label.back() == '-')
			return false;
		return std::all_of(label.begin(), label.end(), [](const char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '-'; });
	}

	bool IsValidHostname(const std::string& fullHost)
	{
		// A single trailing dot marks a fully qualified name ("printer.local.").
		const std::string host = (fullHost.size() > 1 && fullHost.back() == '.') ? fullHost.substr(0, fullHost.size() - 1) : fullHost;
		if (host.empty() || host.size() > 253)
			return false;
		size_t start = 0;
		std::string lastLabel;
		while (true)
		{
			const size_t dot = host.find('.', start);
			lastLabel = host.substr(start, (dot == std::string::npos) ? std::string::npos : dot - start);
			if (!IsValidHostnameLabel(lastLabel))
				return false;
			if (dot == std::string::npos)
				break;
			start = dot + 1;
		}
		// A purely numeric last label would be a malformed IPv4 address, not a host name.
		if (std::all_of(lastLabel.begin(), lastLabel.end(), [](const char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }))
			return false;
		// A hexadecimal last label ("0x7f.1") is read as an IPv4 address by the resolver.
		return !(lastLabel.size() >= 2 && lastLabel[0] == '0' && (lastLabel[1] == 'x' || lastLabel[1] == 'X'));
	}
} // namespace

std::string ShellySanitizeText(const std::string& text)
{
	return SanitizeDeviceText(text);
}

std::string ShellyNormalizeMac(const std::string& mac)
{
	std::string out;
	out.reserve(12);
	for (const char c : mac)
	{
		const unsigned char uc = static_cast<unsigned char>(c);
		if (std::isxdigit(uc))
			out += static_cast<char>(std::toupper(uc));
		else if (c != ':' && c != '-' && c != '.' && c != ' ')
			return std::string();
	}
	return (out.size() == 12) ? out : std::string();
}

bool ShellyIsValidHost(const std::string& host)
{
	if (host.empty() || host.size() > 253)
		return false;
	if (host.find_first_of("[]%/@?# ") != std::string::npos)
		return false;
	boost::system::error_code ec;
	boost::asio::ip::make_address(host, ec);
	if (!ec)
		return true;
	return IsValidHostname(host);
}

bool ShellyIsValidPort(const int port)
{
	return port >= 1 && port <= 65535;
}

std::string ShellyHostForUrl(const std::string& host)
{
	return (host.find(':') != std::string::npos) ? "[" + host + "]" : host;
}

bool ShellyParseIdentity(const std::string& body, ShellyIdentity& identity)
{
	identity = ShellyIdentity();

	Json::Value root;
	if (!ParseJSon(body, root) || !root.isObject())
		return false;

	identity.mac = ShellyNormalizeMac(JsonString(root, "mac"));
	if (identity.mac.empty())
		return false;

	if (root.isMember("gen"))
	{
		// Gen2+: gen, model, id, mac, auth_en, fw_id, ver
		const Json::Value& gen = root["gen"];
		if (!gen.isInt())
			return false;
		const int igen = gen.asInt();
		if (igen < SHELLY_GEN2)
			return false;
		// Future RPC generations use the same protocol; treat them as the highest known one.
		identity.gen = (igen > SHELLY_GEN4) ? SHELLY_GEN4 : igen;
		identity.model = JsonString(root, "model");
		identity.deviceId = JsonString(root, "id");
		identity.name = JsonString(root, "name");
		identity.fw = JsonString(root, "ver");
		if (identity.fw.empty())
			identity.fw = JsonString(root, "fw_id");
		identity.authRequired = root["auth_en"].isBool() && root["auth_en"].asBool();
		if (identity.model.empty() && identity.deviceId.empty())
			return false;
	}
	else
	{
		// Gen1: type, mac, auth, fw, num_outputs
		identity.gen = SHELLY_GEN1;
		identity.model = JsonString(root, "type");
		if (identity.model.empty())
			return false;
		// type and mac alone are too common to recognize a Gen1 device
		if (!root.isMember("fw") && !root.isMember("num_outputs"))
			return false;
		identity.fw = JsonString(root, "fw");
		identity.authRequired = root["auth"].isBool() && root["auth"].asBool();
	}
	return true;
}

ShellyResult ShellyIdentifyDevice(const std::string& ip, const int port, ShellyIdentity& identity, const int timeoutSec)
{
	identity = ShellyIdentity();
	if (!ShellyIsValidHost(ip))
		return ShellyResult::Failure(0, "invalid host");
	if (!ShellyIsValidPort(port))
		return ShellyResult::Failure(0, "invalid port");

	const std::string url = ShellyBaseUrl(ip, port) + "/shelly";
	ShellyHttpOptions options;
	options.timeoutSec = timeoutSec;
	const ShellyHttpResponse response = ShellyHttpGet(url, options);
	if (!response.IsSuccess())
		return response.ToResult();
	if (response.body.empty())
		return ShellyResult::Failure(response.status, "empty response");
	if (!ShellyParseIdentity(response.body, identity))
		return ShellyResult::Failure(response.status, "not a Shelly identity document");
	return ShellyResult::Success(response.status);
}

ShellyNode ShellyNodeFromIdentity(const ShellyIdentity& identity, const std::string& ip, const int port, const std::string& name)
{
	ShellyNode node;
	node.id = identity.mac;
	node.deviceId = identity.deviceId;
	node.ip = ip;
	node.port = port;
	node.gen = identity.gen;
	node.model = identity.model;
	node.fw = identity.fw;
	node.auth = identity.authRequired;
	if (!name.empty())
		node.name = name;
	else if (!identity.name.empty())
		node.name = identity.name;
	else if (!identity.deviceId.empty())
		node.name = identity.deviceId;
	else
		node.name = "Shelly " + identity.model + " " + identity.mac.substr(identity.mac.size() > 6 ? identity.mac.size() - 6 : 0);
	return node;
}

Json::Value ShellyNodeToJson(const ShellyNode& node)
{
	Json::Value json(Json::objectValue);
	json["id"] = node.id;
	json["deviceId"] = node.deviceId;
	json["ip"] = node.ip;
	json["port"] = node.port;
	json["gen"] = node.gen;
	json["model"] = node.model;
	json["name"] = node.name;
	json["fw"] = node.fw;
	json["auth"] = node.auth;
	json["sleepy"] = node.sleepy;
	json["sleepPeriodSec"] = node.sleepPeriodSec;
	return json;
}

bool ShellyNodeFromJson(const Json::Value& json, ShellyNode& node)
{
	if (!json.isObject())
		return false;
	ShellyNode n;
	n.id = JsonString(json, "id");
	n.ip = json["ip"].isString() ? json["ip"].asString() : std::string();
	if (n.id.empty() || ShellyNormalizeMac(n.id) != n.id || !ShellyIsValidHost(n.ip))
		return false;
	n.deviceId = JsonString(json, "deviceId");

	if (json.isMember("port"))
	{
		if (!json["port"].isInt())
			return false;
		n.port = json["port"].asInt();
	}
	else
		n.port = 80;
	if (!ShellyIsValidPort(n.port))
		return false;

	if (!json["gen"].isInt())
		return false;
	n.gen = json["gen"].asInt();
	if (n.gen < SHELLY_GEN1 || n.gen > SHELLY_GEN4)
		return false;

	n.model = JsonString(json, "model");
	n.name = JsonString(json, "name");
	n.fw = JsonString(json, "fw");
	n.auth = json["auth"].isBool() && json["auth"].asBool();
	n.sleepy = json["sleepy"].isBool() && json["sleepy"].asBool();
	n.sleepPeriodSec = (json["sleepPeriodSec"].isInt() && json["sleepPeriodSec"].asInt() > 0) ? json["sleepPeriodSec"].asInt() : 0;
	node = n;
	return true;
}

std::string ShellyNodesToString(const std::vector<ShellyNode>& nodes)
{
	Json::Value root(Json::objectValue);
	Json::Value arr(Json::arrayValue);
	for (const auto& node : nodes)
		arr.append(ShellyNodeToJson(node));
	root["nodes"] = arr;
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	return Json::writeString(builder, root);
}

bool ShellyNodesFromString(const std::string& str, std::vector<ShellyNode>& nodes, std::vector<std::string>* warnings)
{
	nodes.clear();
	if (str.empty())
		return true;
	Json::Value root;
	if (!ParseJSon(str, root) || !root.isObject() || !root["nodes"].isArray())
		return false;
	int index = 0;
	for (const auto& item : root["nodes"])
	{
		ShellyNode node;
		if (ShellyNodeFromJson(item, node))
		{
			const bool duplicate = std::any_of(nodes.begin(), nodes.end(), [&node](const ShellyNode& n) { return n.id == node.id; });
			if (!duplicate)
				nodes.push_back(node);
			else if (warnings)
				warnings->push_back("node entry " + std::to_string(index) + " repeats device " + node.id + " and was ignored");
		}
		else if (warnings)
			warnings->push_back("node entry " + std::to_string(index) + " is invalid and was ignored");
		index++;
	}
	return true;
}
