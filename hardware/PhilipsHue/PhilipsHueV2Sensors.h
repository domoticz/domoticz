#pragma once

#include "PhilipsHueSensors.h"
#include <string>
#include <vector>
#include <sstream>
#include <map>

// Using existing json wrapper in project
// Json::Value is used (same style as PhilipsHue.cpp / PhilipsHueSensors.cpp)
namespace Json {
	class Value;
}

struct HueV2Device {
	std::string id;
	std::string name;
	std::string model_id;
	std::string product_name;
	std::string manufacturer_name;
	std::string software_version;
	std::vector<std::string> services_rids; // rids to lookup contact/tamper/device_power
};

struct HueV2Contact {
	std::string id;
	std::string owner_rid;
	bool enabled = false;
	std::string state;   // "contact" / "no_contact" ...
	std::string changed; // ISO timestamp
};

struct HueV2Tamper {
	std::string id;
	std::string owner_rid;
	std::string state;   // "tampered" / "not_tampered" ...
	std::string source;  // e.g. "battery_door"
	std::string changed; // ISO timestamp (last report)
};

struct HueV2DevicePower {
	std::string id;
	std::string id_v1;
	std::string owner_rid;
	std::string battery_state; // e.g. "normal"
	int battery_level = -1;    // 0..100 or -1 if missing
};

class CPhilipsHueV2Sensors {
public:
	// html_schema should be "http" or "https" per existing code usage
	CPhilipsHueV2Sensors(const std::string& html_schema,
		const std::string& ipAddress,
		const std::string& port,
		const std::string& applicationKey);
	~CPhilipsHueV2Sensors();

	// Not copyable
	CPhilipsHueV2Sensors(const CPhilipsHueV2Sensors&) = delete;
	CPhilipsHueV2Sensors& operator=(const CPhilipsHueV2Sensors&) = delete;

	// Build base URL into m_BaseURLv2 stream as requested
	void SetBaseURLv2FromParts();

	// Fetch and parse all v2 resources; partial success still stores parsed data
	bool UpdateAll();

	// Individual fetch functions (public for testing)
	bool FetchDevices();
	bool FetchContacts();
	bool FetchTamper();
	bool FetchDevicePower();

	// Getters
	const std::vector<HueV2Device>& GetDevices() const { return m_devices; }
	const std::vector<HueV2Contact>& GetContacts() const { return m_contacts; }
	const std::vector<HueV2Tamper>& GetTampers() const { return m_tampers; }
	const std::vector<HueV2DevicePower>& GetDevicePowers() const { return m_devicePowers; }

	// Simple setters
	void SetApplicationKey(const std::string& key) { m_ApplicationKey = key; }
	void SetIPAddress(const std::string& ip) { m_IPAddress = ip; SetBaseURLv2FromParts(); }
	void SetPort(const std::string& port) { m_Port = port; SetBaseURLv2FromParts(); }
	void SetSchema(const std::string& schema) { m_html_schema = schema; SetBaseURLv2FromParts(); }

private:
	// parse helpers
	bool parseDeviceJson(const Json::Value& root);
	bool parseContactJson(const Json::Value& root);
	bool parseTamperJson(const Json::Value& root);
	bool parseDevicePowerJson(const Json::Value& root);

private:
	std::string m_html_schema;
	std::string m_IPAddress;
	std::string m_Port;
	std::ostringstream m_BaseURLv2; // use stream as requested
	std::string m_ApplicationKey;

	std::vector<HueV2Device> m_devices;
	std::vector<HueV2Contact> m_contacts;
	std::vector<HueV2Tamper> m_tampers;
	std::vector<HueV2DevicePower> m_devicePowers;
};