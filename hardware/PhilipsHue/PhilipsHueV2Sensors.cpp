#include "stdafx.h"
#include "PhilipsHueV2Sensors.h"
#include "../../httpclient/HTTPClient.h"
#include "../../main/json_helper.h"
#include "../../main/Logger.h"
#include <json/json.h> // Json::Value
#include <algorithm>

using namespace std;

// Constructor
CPhilipsHueV2Sensors::CPhilipsHueV2Sensors(const std::string& html_schema,
	const std::string& ipAddress,
	const std::string& port,
	const std::string& applicationKey) :
	m_html_schema(html_schema),
	m_IPAddress(ipAddress),
	m_Port(port),
	m_ApplicationKey(applicationKey)
{
	SetBaseURLv2FromParts();
}

// Destructor
CPhilipsHueV2Sensors::~CPhilipsHueV2Sensors()
{
}

// Build base URL using stream as requested
void CPhilipsHueV2Sensors::SetBaseURLv2FromParts()
{
	m_BaseURLv2.str("");
	m_BaseURLv2.clear();
	m_BaseURLv2 << m_html_schema << "://" << m_IPAddress;
	if (!m_Port.empty()) m_BaseURLv2 << ":" << m_Port;
	// callers append /clip/v2/resource/...
}

// High-level update: attempt all endpoints; partial success still returns true if any succeeded
bool CPhilipsHueV2Sensors::UpdateAll()
{
	bool ok1 = FetchDevices();
	bool ok2 = FetchContacts();
	bool ok3 = FetchTamper();
	bool ok4 = FetchDevicePower();
	return ok1 || ok2 || ok3 || ok4;
}

// Helper: GET with hue-application-key header using project HTTPClient
static bool http_get_with_key(const std::string& url, const std::string& appKey, std::string& outBody)
{
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	if (!appKey.empty())
	{
		ExtraHeaders.push_back(std::string("hue-application-key: ") + appKey);
	}
	return HTTPClient::GET(url, ExtraHeaders, outBody);
}

// Fetch /clip/v2/resource/device
bool CPhilipsHueV2Sensors::FetchDevices()
{
	m_devices.clear();
	std::string url = m_BaseURLv2.str() + "/clip/v2/resource/device";
	std::string sResult;
	if (!http_get_with_key(url, m_ApplicationKey, sResult))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchDevices HTTP GET failed (%s)", url.c_str());
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchDevices JSON parse failed");
		return false;
	}
	return parseDeviceJson(root);
}

// Fetch /clip/v2/resource/contact
bool CPhilipsHueV2Sensors::FetchContacts()
{
	m_contacts.clear();
	std::string url = m_BaseURLv2.str() + "/clip/v2/resource/contact";
	std::string sResult;
	if (!http_get_with_key(url, m_ApplicationKey, sResult))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchContacts HTTP GET failed (%s)", url.c_str());
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchContacts JSON parse failed");
		return false;
	}
	return parseContactJson(root);
}

// Fetch /clip/v2/resource/tamper
bool CPhilipsHueV2Sensors::FetchTamper()
{
	m_tampers.clear();
	std::string url = m_BaseURLv2.str() + "/clip/v2/resource/tamper";
	std::string sResult;
	if (!http_get_with_key(url, m_ApplicationKey, sResult))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchTamper HTTP GET failed (%s)", url.c_str());
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchTamper JSON parse failed");
		return false;
	}
	return parseTamperJson(root);
}

// Fetch /clip/v2/resource/device_power
bool CPhilipsHueV2Sensors::FetchDevicePower()
{
	m_devicePowers.clear();
	std::string url = m_BaseURLv2.str() + "/clip/v2/resource/device_power";
	std::string sResult;
	if (!http_get_with_key(url, m_ApplicationKey, sResult))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchDevicePower HTTP GET failed (%s)", url.c_str());
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchDevicePower JSON parse failed");
		return false;
	}
	return parseDevicePowerJson(root);
}

// parse /clip/v2/resource/device (root is Json::Value)
bool CPhilipsHueV2Sensors::parseDeviceJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseDeviceJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2Device d;
		if (item.isMember("id")) d.id = item["id"].asString();
		if (item.isMember("metadata") && item["metadata"].isObject() && item["metadata"].isMember("name"))
			d.name = item["metadata"]["name"].asString();

		if (item.isMember("product_data") && item["product_data"].isObject())
		{
			const auto& pd = item["product_data"];
			if (pd.isMember("model_id")) d.model_id = pd["model_id"].asString();
			if (pd.isMember("product_name")) d.product_name = pd["product_name"].asString();
			if (pd.isMember("manufacturer_name")) d.manufacturer_name = pd["manufacturer_name"].asString();
			if (pd.isMember("software_version")) d.software_version = pd["software_version"].asString();
		}
		if (item.isMember("services") && item["services"].isArray())
		{
			for (const auto& srv : item["services"])
			{
				if (srv.isMember("rid")) d.services_rids.push_back(srv["rid"].asString());
			}
		}
		m_devices.push_back(d);
	}
	return true;
}

// parse /clip/v2/resource/contact
bool CPhilipsHueV2Sensors::parseContactJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseContactJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2Contact c;
		if (item.isMember("id")) c.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			c.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled")) c.enabled = item["enabled"].asBool();
		if (item.isMember("contact_report") && item["contact_report"].isObject())
		{
			const auto& cr = item["contact_report"];
			if (cr.isMember("state")) c.state = cr["state"].asString();
			if (cr.isMember("changed")) c.changed = cr["changed"].asString();
		}
		m_contacts.push_back(c);
	}
	return true;
}

// parse /clip/v2/resource/tamper
bool CPhilipsHueV2Sensors::parseTamperJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseTamperJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2Tamper t;
		if (item.isMember("id")) t.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			t.owner_rid = item["owner"]["rid"].asString();
		// tamper_reports may be empty; pick last if present
		if (item.isMember("tamper_reports") && item["tamper_reports"].isArray() && item["tamper_reports"].size() > 0)
		{
			const auto& arr = item["tamper_reports"];
			const Json::Value& last = arr[arr.size() - 1];
			if (last.isMember("state")) t.state = last["state"].asString();
			if (last.isMember("source")) t.source = last["source"].asString();
			if (last.isMember("changed")) t.changed = last["changed"].asString();
		}
		m_tampers.push_back(t);
	}
	return true;
}

// parse /clip/v2/resource/device_power
bool CPhilipsHueV2Sensors::parseDevicePowerJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseDevicePowerJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2DevicePower p;
		if (item.isMember("id")) p.id = item["id"].asString();
		// optional id_v1 mapping (e.g. "/sensors/25")
		if (item.isMember("id_v1") && item["id_v1"].isString())
			p.id_v1 = item["id_v1"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			p.owner_rid = item["owner"]["rid"].asString();
		// default = missing
		p.battery_level = -1;
		if (item.isMember("power_state") && item["power_state"].isObject())
		{
			const auto& ps = item["power_state"];
			if (ps.isMember("battery_state") && ps["battery_state"].isString())
				p.battery_state = ps["battery_state"].asString();

			if (ps.isMember("battery_level") && ps["battery_level"].isInt())
				p.battery_level = ps["battery_level"].asInt();
		}
		// Clamp battery_level if present
		if (p.battery_level >= 0)
			p.battery_level = std::clamp(p.battery_level, 0, 100);

		// Debug: parsed device_power
		_log.Debug(DEBUG_HARDWARE, "PhilipsHueV2: parsed device_power id=%s id_v1=%s owner=%s battery_state=%s battery_level=%d",
			p.id.c_str(),
			p.id_v1.c_str(),
			p.owner_rid.c_str(),
			p.battery_state.c_str(),
			p.battery_level);

		m_devicePowers.push_back(p);
	}
	return true;
}

/*
- Example URLs:
	m_BaseURLv2 << "/clip/v2/resource/device"
	m_BaseURLv2 << "/clip/v2/resource/contact"
	m_BaseURLv2 << "/clip/v2/resource/tamper"
	m_BaseURLv2 << "/clip/v2/resource/device_power"
*/