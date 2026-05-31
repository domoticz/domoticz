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

struct HueV2Light {
	std::string id;
	std::string owner_rid;
	bool on = false;
	//_eHueColorMode mode;
	int level = 0;
	int hue  = 0;
	int sat = 0;
	int ct = 0;
	double x = 0;
	double y = 0;
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

struct HueV2Room {
	std::string id;                // Room/zone UUID
	std::string name;              // metadata.name
	std::string grouped_light_rid; // RID of the grouped_light service for this room
};

struct HueV2Motion {
	std::string id;
	std::string id_v1;           // e.g. "/sensors/4"
	std::string owner_rid;
	bool enabled = false;
	bool motion = false;
	std::string motion_changed; // ISO timestamp of last change
};

struct HueV2Temperature {
	std::string id;
	std::string id_v1;           // e.g. "/sensors/7"
	std::string owner_rid;
	bool enabled = false;
	float temperature = 0.0f;   // Celsius
	std::string temperature_changed;
};

struct HueV2LightLevel {
	std::string id;
	std::string id_v1;           // e.g. "/sensors/6"
	std::string owner_rid;
	bool enabled = false;
	int lightlevel = 0;         // raw lux-derived value
	bool dark = false;
	std::string lightlevel_changed;
};

struct HueV2GroupedMotion {
	std::string id;
	std::string owner_rid;
	bool enabled = false;
	bool motion = false;
	std::string motion_changed;
};

struct HueV2GroupedLightLevel {
	std::string id;
	std::string owner_rid;
	bool enabled = false;
	int lightlevel = 0;
	bool dark = false;
	std::string lightlevel_changed;
};

struct HueV2CameraMotion {
	std::string id;
	std::string owner_rid;
	bool enabled = false;
	bool motion = false;
	std::string motion_changed;
};

struct HueV2SecurityAreaMotion {
	std::string id;
	std::string owner_rid;
	bool enabled = false;
	bool motion = false;
	std::string motion_changed;
};

struct HueV2BellButton {
	std::string id;
	std::string owner_rid;
	int control_id = 0;
};

struct HueV2Button {
	std::string id;
	std::string id_v1;
	std::string owner_rid;
	int control_id = 0;
	bool supports_long_press = false;
	std::string last_event;
	std::string last_updated;
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
	bool FetchRooms();
	bool FetchDevices();
	bool FetchLights();
	bool FetchMotion();
	bool FetchTemperature();
	bool FetchLightLevel();
	bool FetchContacts();
	bool FetchTamper();
	bool FetchDevicePower();
	bool FetchGroupedMotion();
	bool FetchGroupedLightLevel();
	bool FetchCameraMotion();
	bool FetchSecurityAreaMotion();
	bool FetchBellButtons();
	bool FetchButtons();

	// Getters
	const std::vector<HueV2Room>& GetRooms() const { return m_rooms; }
	// Returns room/zone name for a V1 group integer ID, or empty string if not found
	std::string GetRoomNameByV1GroupId(int groupId) const;

	const std::vector<HueV2Device>& GetDevices() const { return m_devices; }
	const std::vector<HueV2Light>& GetLights() const { return m_lights; }
	const std::vector<HueV2Motion>& GetMotions() const { return m_motions; }
	const std::vector<HueV2Temperature>& GetTemperatures() const { return m_temperatures; }
	const std::vector<HueV2LightLevel>& GetLightLevels() const { return m_lightlevels; }
	const std::vector<HueV2Contact>& GetContacts() const { return m_contacts; }
	const std::vector<HueV2Tamper>& GetTampers() const { return m_tampers; }
	const std::vector<HueV2DevicePower>& GetDevicePowers() const { return m_devicePowers; }
	const std::vector<HueV2GroupedMotion>& GetGroupedMotions() const { return m_groupedMotions; }
	const std::vector<HueV2GroupedLightLevel>& GetGroupedLightLevels() const { return m_groupedLightLevels; }
	const std::vector<HueV2CameraMotion>& GetCameraMotions() const { return m_cameraMotions; }
	const std::vector<HueV2SecurityAreaMotion>& GetSecurityAreaMotions() const { return m_securityAreaMotions; }
	const std::vector<HueV2BellButton>& GetBellButtons() const { return m_bellButtons; }
	const std::vector<HueV2Button>& GetButtons() const { return m_buttons; }

	// Simple setters
	void SetApplicationKey(const std::string& key) { m_ApplicationKey = key; }
	void SetIPAddress(const std::string& ip) { m_IPAddress = ip; SetBaseURLv2FromParts(); }
	void SetPort(const std::string& port) { m_Port = port; SetBaseURLv2FromParts(); }
	void SetSchema(const std::string& schema) { m_html_schema = schema; SetBaseURLv2FromParts(); }

private:
	// parse helpers
	bool parseRoomJson(const Json::Value& root);
	bool parseGroupedLightV1Ids(const Json::Value& root, std::map<std::string, int>& outRidToV1Id);
	bool parseDeviceJson(const Json::Value& root);
	bool parseLightJson(const Json::Value& root);
	bool parseMotionJson(const Json::Value& root);
	bool parseTemperatureJson(const Json::Value& root);
	bool parseLightLevelJson(const Json::Value& root);
	bool parseContactJson(const Json::Value& root);
	bool parseTamperJson(const Json::Value& root);
	bool parseDevicePowerJson(const Json::Value& root);
	bool parseGroupedMotionJson(const Json::Value& root);
	bool parseGroupedLightLevelJson(const Json::Value& root);
	bool parseCameraMotionJson(const Json::Value& root);
	bool parseSecurityAreaMotionJson(const Json::Value& root);
	bool parseBellButtonJson(const Json::Value& root);
	bool parseButtonJson(const Json::Value& root);

	bool http_get_v2_with_key(const std::string& api_endpoint, const std::string& appKey, std::string& outBody);
private:
	std::string m_html_schema;
	std::string m_IPAddress;
	std::string m_Port;
	std::ostringstream m_BaseURLv2; // use stream as requested
	std::string m_ApplicationKey;

	std::vector<HueV2Room> m_rooms;
	std::map<int, std::string> m_roomNameByV1GroupId; // V1 group integer -> room/zone name
	std::vector<HueV2Device> m_devices;
	std::vector<HueV2Light> m_lights;
	std::vector<HueV2Motion> m_motions;
	std::vector<HueV2Temperature> m_temperatures;
	std::vector<HueV2LightLevel> m_lightlevels;
	std::vector<HueV2Contact> m_contacts;
	std::vector<HueV2Tamper> m_tampers;
	std::vector<HueV2DevicePower> m_devicePowers;
	std::vector<HueV2GroupedMotion> m_groupedMotions;
	std::vector<HueV2GroupedLightLevel> m_groupedLightLevels;
	std::vector<HueV2CameraMotion> m_cameraMotions;
	std::vector<HueV2SecurityAreaMotion> m_securityAreaMotions;
	std::vector<HueV2BellButton> m_bellButtons;
	std::vector<HueV2Button> m_buttons;
};