#include "stdafx.h"
#include "PhilipsHueV2Sensors.h"
#include "../../httpclient/HTTPClient.h"
#include "../../main/json_helper.h"
#include "../../main/Helper.h"
#include "../../main/Logger.h"
#include <json/json.h> // Json::Value
#include <algorithm>


#ifdef _DEBUG
//should be the same as in the main class!
//#define DEBUG_PhilipsHue_R
#define DEBUG_PhilipsHue_W
#endif

#ifdef DEBUG_PhilipsHue_W
#ifndef SaveString2Disk
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#else
extern void SaveString2Disk(std::string str, std::string filename);
#endif
#endif
#ifdef DEBUG_PhilipsHue_R
std::string ReadFile(std::string filename)
#endif

extern std::string hue_errorDescription(const Json::Value& root);

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
	m_BaseURLv2 << "/clip/v2/resource/";
}

// High-level update: attempt all endpoints; partial success still returns true if any succeeded
bool CPhilipsHueV2Sensors::UpdateAll()
{
	bool ok0 = FetchRooms();
	bool ok1 = FetchDevices();
	bool ok2 = FetchMotion();
	bool ok3 = FetchTemperature();
	bool ok4 = FetchLightLevel();
	bool ok5 = FetchLights();
	bool ok6 = FetchContacts();
	bool ok7 = FetchTamper();
	bool ok8 = FetchDevicePower();
	FetchGroupedMotion();
	FetchGroupedLightLevel();
	FetchCameraMotion();
	FetchSecurityAreaMotion();
	FetchBellButtons();
	FetchButtons();
	return ok0 || ok1 || ok2 || ok3 || ok4 || ok5 || ok6 || ok7 || ok8;
}

// Helper: GET with hue-application-key header using project HTTPClient
bool CPhilipsHueV2Sensors::http_get_v2_with_key(const std::string& api_endpoint, const std::string& appKey, std::string& outBody)
{
	std::string url = m_BaseURLv2.str() + api_endpoint;
#ifdef DEBUG_PhilipsHue_R
	outBody = ReadFile(urlToFilename("PhilipsHue", url));
#endif
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Accept: application/json");
	if (!appKey.empty())
	{
		ExtraHeaders.push_back(std::string("hue-application-key: ") + appKey);
	}
	bool ret = HTTPClient::GET(url, ExtraHeaders, outBody);
#ifdef DEBUG_PhilipsHue_W
	SaveString2Disk(outBody, urlToFilename("PhilipsHue", url));
#endif
	return ret;
}

// Fetch /clip/v2/resource/room and /clip/v2/resource/zone
bool CPhilipsHueV2Sensors::FetchRooms()
{
	m_rooms.clear();
	m_roomNameByV1GroupId.clear();
	std::string sResult;
	if (http_get_v2_with_key("room", m_ApplicationKey, sResult))
	{
		Json::Value root;
		if (ParseJSon(sResult, root) && sResult.find("\"error\":") == std::string::npos)
			parseRoomJson(root);
		else
			_log.Log(LOG_ERROR, "PhilipsHueV2: FetchRooms (room) parse/error failure");
	}
	else
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchRooms (room) HTTP GET failed");

	if (http_get_v2_with_key("zone", m_ApplicationKey, sResult))
	{
		Json::Value root;
		if (ParseJSon(sResult, root) && sResult.find("\"error\":") == std::string::npos)
			parseRoomJson(root);
		else
			_log.Log(LOG_ERROR, "PhilipsHueV2: FetchRooms (zone) parse/error failure");
	}
	else
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchRooms (zone) HTTP GET failed");

	if (!m_rooms.empty())
	{
		if (http_get_v2_with_key("grouped_light", m_ApplicationKey, sResult))
		{
			Json::Value root;
			if (ParseJSon(sResult, root) && sResult.find("\"error\":") == std::string::npos)
			{
				std::map<std::string, int> ridToV1Id;
				if (parseGroupedLightV1Ids(root, ridToV1Id))
				{
					for (const auto& room : m_rooms)
					{
						if (room.grouped_light_rid.empty() || room.name.empty())
							continue;
						auto it = ridToV1Id.find(room.grouped_light_rid);
						if (it != ridToV1Id.end())
							m_roomNameByV1GroupId[it->second] = room.name;
					}
				}
			}
			else
				_log.Log(LOG_ERROR, "PhilipsHueV2: FetchRooms (grouped_light) parse/error failure");
		}
		else
			_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchRooms (grouped_light) HTTP GET failed");
	}

	return !m_rooms.empty();
}

// parse /clip/v2/resource/room or /clip/v2/resource/zone (same structure)
bool CPhilipsHueV2Sensors::parseRoomJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseRoomJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject())
			continue;
		HueV2Room r;
		if (item.isMember("id"))
			r.id = item["id"].asString();
		if (item.isMember("metadata") && item["metadata"].isObject() && item["metadata"].isMember("name"))
			r.name = item["metadata"]["name"].asString();
		if (item.isMember("services") && item["services"].isArray())
		{
			for (const auto& srv : item["services"])
			{
				if (srv.isObject() && srv.isMember("rtype") && srv["rtype"].asString() == "grouped_light")
				{
					if (srv.isMember("rid"))
						r.grouped_light_rid = srv["rid"].asString();
					break;
				}
			}
		}
		m_rooms.push_back(r);
	}
	return true;
}

// Parse /clip/v2/resource/grouped_light to extract UUID -> V1 group number mappings
bool CPhilipsHueV2Sensors::parseGroupedLightV1Ids(const Json::Value& root, std::map<std::string, int>& outRidToV1Id)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
		return false;
	for (const auto& item : root["data"])
	{
		if (!item.isObject())
			continue;
		if (!item.isMember("id") || !item.isMember("id_v1"))
			continue;
		const std::string rid = item["id"].asString();
		const std::string id_v1 = item["id_v1"].asString();
		const std::string prefix = "/groups/";
		if (id_v1.size() <= prefix.size() || id_v1.substr(0, prefix.size()) != prefix)
			continue;
		const std::string numStr = id_v1.substr(prefix.size());
		if (numStr.empty())
			continue;
		try {
			int groupNum = std::stoi(numStr);
			outRidToV1Id[rid] = groupNum;
		}
		catch (...) {}
	}
	return !outRidToV1Id.empty();
}

// Return room/zone name for a V1 group integer ID, empty string if not available
std::string CPhilipsHueV2Sensors::GetRoomNameByV1GroupId(int groupId) const
{
	auto it = m_roomNameByV1GroupId.find(groupId);
	if (it != m_roomNameByV1GroupId.end())
		return it->second;
	return {};
}

// Fetch /clip/v2/resource/device
bool CPhilipsHueV2Sensors::FetchDevices()
{
	m_devices.clear();
	std::string sResult;
	if (!http_get_v2_with_key("device", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchDevices HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchDevices JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseDeviceJson(root);
}

// Fetch /clip/v2/resource/contact
bool CPhilipsHueV2Sensors::FetchLights()
{
	return true; //using V1 API for this now
	m_lights.clear();
	std::string sResult;
	if (!http_get_v2_with_key("light", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchLights HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchLights JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseLightJson(root);
}

// Fetch /clip/v2/resource/motion
bool CPhilipsHueV2Sensors::FetchMotion()
{
	m_motions.clear();
	std::string sResult;
	if (!http_get_v2_with_key("motion", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchMotion HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchMotion JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseMotionJson(root);
}

// Fetch /clip/v2/resource/temperature
bool CPhilipsHueV2Sensors::FetchTemperature()
{
	m_temperatures.clear();
	std::string sResult;
	if (!http_get_v2_with_key("temperature", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchTemperature HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchTemperature JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseTemperatureJson(root);
}

// Fetch /clip/v2/resource/light_level
bool CPhilipsHueV2Sensors::FetchLightLevel()
{
	m_lightlevels.clear();
	std::string sResult;
	if (!http_get_v2_with_key("light_level", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchLightLevel HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchLightLevel JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseLightLevelJson(root);
}

// Fetch /clip/v2/resource/grouped_motion
bool CPhilipsHueV2Sensors::FetchGroupedMotion()
{
	m_groupedMotions.clear();
	std::string body;
	if (!http_get_v2_with_key("grouped_motion", m_ApplicationKey, body))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to fetch grouped_motion");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(body, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to parse grouped_motion JSON");
		return false;
	}
	if (root.isMember("errors") && !root["errors"].empty())
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseGroupedMotionJson(root);
}

// Fetch /clip/v2/resource/grouped_light_level
bool CPhilipsHueV2Sensors::FetchGroupedLightLevel()
{
	m_groupedLightLevels.clear();
	std::string body;
	if (!http_get_v2_with_key("grouped_light_level", m_ApplicationKey, body))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to fetch grouped_light_level");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(body, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to parse grouped_light_level JSON");
		return false;
	}
	if (root.isMember("errors") && !root["errors"].empty())
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseGroupedLightLevelJson(root);
}

// Fetch /clip/v2/resource/camera_motion
bool CPhilipsHueV2Sensors::FetchCameraMotion()
{
	m_cameraMotions.clear();
	std::string body;
	if (!http_get_v2_with_key("camera_motion", m_ApplicationKey, body))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to fetch camera_motion");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(body, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to parse camera_motion JSON");
		return false;
	}
	if (root.isMember("errors") && !root["errors"].empty())
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseCameraMotionJson(root);
}

// Fetch /clip/v2/resource/contact
bool CPhilipsHueV2Sensors::FetchContacts()
{
	m_contacts.clear();
	std::string sResult;
	if (!http_get_v2_with_key("contact", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchContacts HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchContacts JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseContactJson(root);
}

// Fetch /clip/v2/resource/tamper
bool CPhilipsHueV2Sensors::FetchTamper()
{
	m_tampers.clear();
	std::string sResult;
	if (!http_get_v2_with_key("tamper", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchTamper HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchTamper JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseTamperJson(root);
}

// Fetch /clip/v2/resource/device_power
bool CPhilipsHueV2Sensors::FetchDevicePower()
{
	m_devicePowers.clear();
	std::string sResult;
	if (!http_get_v2_with_key("device_power", m_ApplicationKey, sResult))
	{
		_log.Log(LOG_DEBUG_INT, "PhilipsHueV2: FetchDevicePower HTTP GET failed");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(sResult, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: FetchDevicePower JSON parse failed");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
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
		if (!item.isObject())
			continue;
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
				if (srv.isMember("rid")) {
					d.services_rids.push_back(srv["rid"].asString());
				}
			}
		}
		m_devices.push_back(d);
	}
	return true;
}

// parse /clip/v2/resource/light
bool CPhilipsHueV2Sensors::parseLightJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseLightJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2Light light;
		if (item.isMember("id"))
			light.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			light.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("on") && item["on"].isObject() && item["on"].isMember("on"))
			light.on = item["on"]["on"].asBool();

		bool hasBri = false;
		bool hasHueSat = false;
		bool hasTemp = false;

		if (item.isMember("dimming") && item["dimming"].isObject() && item["dimming"].isMember("brightness"))
		{
			light.level = (int)std::ceil(item["dimming"]["brightness"].asFloat());
			hasBri = true;
		}
		if (item.isMember("color_temperature") && item["color_temperature"].isObject() && item["color_temperature"].isMember("mirek"))
		{
			//CT
			int CT = item["color_temperature"]["mirek"].asInt();
			// Clamp to conform to HUE API
			CT = std::max(153, CT);
			CT = std::min(500, CT);
			light.ct = CT;
			hasTemp = true;
		}
		//sat,hue,xy
		
		//effects

		//LType = HLTYPE_NORMAL;
		//if (hasBri) LType = HLTYPE_DIM;
		//if (hasBri && hasHueSat && !hasTemp) LType = HLTYPE_RGB_W;
		//if (hasBri && !hasHueSat && hasTemp) LType = HLTYPE_CW_WW;
		//if (hasBri && hasHueSat && hasTemp) LType = HLTYPE_RGB_CW_WW;


		m_lights.push_back(light);
	}
	return true;
}

// parse /clip/v2/resource/motion
bool CPhilipsHueV2Sensors::parseMotionJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseMotionJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject())
			continue;
		HueV2Motion m;
		if (item.isMember("id"))
			m.id = item["id"].asString();
		if (item.isMember("id_v1") && item["id_v1"].isString())
			m.id_v1 = item["id_v1"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			m.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled"))
			m.enabled = item["enabled"].asBool();
		if (item.isMember("motion") && item["motion"].isObject())
		{
			const auto& motObj = item["motion"];
			if (motObj.isMember("motion"))
				m.motion = motObj["motion"].asBool();
			if (motObj.isMember("motion_report") && motObj["motion_report"].isObject())
			{
				if (motObj["motion_report"].isMember("changed"))
					m.motion_changed = motObj["motion_report"]["changed"].asString();
			}
		}
		m_motions.push_back(m);
	}
	return true;
}

// parse /clip/v2/resource/temperature
bool CPhilipsHueV2Sensors::parseTemperatureJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseTemperatureJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject())
			continue;
		HueV2Temperature t;
		if (item.isMember("id"))
			t.id = item["id"].asString();
		if (item.isMember("id_v1") && item["id_v1"].isString())
			t.id_v1 = item["id_v1"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			t.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled"))
			t.enabled = item["enabled"].asBool();
		if (item.isMember("temperature") && item["temperature"].isObject())
		{
			const auto& temp = item["temperature"];
			if (temp.isMember("temperature"))
				t.temperature = temp["temperature"].asFloat();
			if (temp.isMember("temperature_report") && temp["temperature_report"].isObject())
			{
				if (temp["temperature_report"].isMember("changed"))
					t.temperature_changed = temp["temperature_report"]["changed"].asString();
			}
		}
		m_temperatures.push_back(t);
	}
	return true;
}

// parse /clip/v2/resource/light_level
bool CPhilipsHueV2Sensors::parseLightLevelJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseLightLevelJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject())
			continue;
		HueV2LightLevel ll;
		if (item.isMember("id"))
			ll.id = item["id"].asString();
		if (item.isMember("id_v1") && item["id_v1"].isString())
			ll.id_v1 = item["id_v1"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			ll.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled"))
			ll.enabled = item["enabled"].asBool();
		if (item.isMember("light") && item["light"].isObject())
		{
			const auto& light = item["light"];
			if (light.isMember("light_level"))
				ll.lightlevel = light["light_level"].asInt();
			if (light.isMember("dark"))
				ll.dark = light["dark"].asBool();
			if (light.isMember("light_level_report") && light["light_level_report"].isObject())
			{
				if (light["light_level_report"].isMember("changed"))
					ll.lightlevel_changed = light["light_level_report"]["changed"].asString();
			}
		}
		m_lightlevels.push_back(ll);
	}
	return true;
}

// parse /clip/v2/resource/grouped_motion
bool CPhilipsHueV2Sensors::parseGroupedMotionJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseGroupedMotionJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2GroupedMotion m;
		if (item.isMember("id")) m.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			m.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled")) m.enabled = item["enabled"].asBool();
		if (item.isMember("motion") && item["motion"].isObject())
		{
			const auto& motObj = item["motion"];
			if (motObj.isMember("motion"))
				m.motion = motObj["motion"].asBool();
			if (motObj.isMember("motion_report") && motObj["motion_report"].isObject())
				if (motObj["motion_report"].isMember("changed"))
					m.motion_changed = motObj["motion_report"]["changed"].asString();
		}
		if (!m.owner_rid.empty())
			m_groupedMotions.push_back(m);
	}
	return true;
}

// parse /clip/v2/resource/grouped_light_level
bool CPhilipsHueV2Sensors::parseGroupedLightLevelJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseGroupedLightLevelJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2GroupedLightLevel ll;
		if (item.isMember("id")) ll.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			ll.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled")) ll.enabled = item["enabled"].asBool();
		if (item.isMember("light") && item["light"].isObject())
		{
			const auto& light = item["light"];
			if (light.isMember("light_level"))
				ll.lightlevel = light["light_level"].asInt();
			if (light.isMember("dark"))
				ll.dark = light["dark"].asBool();
			if (light.isMember("light_level_report") && light["light_level_report"].isObject())
				if (light["light_level_report"].isMember("changed"))
					ll.lightlevel_changed = light["light_level_report"]["changed"].asString();
		}
		if (!ll.owner_rid.empty())
			m_groupedLightLevels.push_back(ll);
	}
	return true;
}

// parse /clip/v2/resource/camera_motion
bool CPhilipsHueV2Sensors::parseCameraMotionJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseCameraMotionJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2CameraMotion m;
		if (item.isMember("id")) m.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			m.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled")) m.enabled = item["enabled"].asBool();
		if (item.isMember("motion") && item["motion"].isObject())
		{
			const auto& motObj = item["motion"];
			if (motObj.isMember("motion"))
				m.motion = motObj["motion"].asBool();
			if (motObj.isMember("motion_report") && motObj["motion_report"].isObject())
				if (motObj["motion_report"].isMember("changed"))
					m.motion_changed = motObj["motion_report"]["changed"].asString();
		}
		if (!m.owner_rid.empty())
			m_cameraMotions.push_back(m);
	}
	return true;
}

// Fetch /clip/v2/resource/security_area_motion
bool CPhilipsHueV2Sensors::FetchSecurityAreaMotion()
{
	m_securityAreaMotions.clear();
	std::string body;
	if (!http_get_v2_with_key("security_area_motion", m_ApplicationKey, body))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to fetch security_area_motion");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(body, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to parse security_area_motion JSON");
		return false;
	}
	if (root.isMember("errors") && !root["errors"].empty())
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseSecurityAreaMotionJson(root);
}

// parse /clip/v2/resource/security_area_motion
bool CPhilipsHueV2Sensors::parseSecurityAreaMotionJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseSecurityAreaMotionJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2SecurityAreaMotion m;
		if (item.isMember("id")) m.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			m.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("enabled")) m.enabled = item["enabled"].asBool();
		if (item.isMember("motion") && item["motion"].isObject())
		{
			const auto& motObj = item["motion"];
			if (motObj.isMember("motion"))
				m.motion = motObj["motion"].asBool();
		}
		if (!m.owner_rid.empty())
			m_securityAreaMotions.push_back(m);
	}
	return true;
}

// Fetch /clip/v2/resource/bell_button
bool CPhilipsHueV2Sensors::FetchBellButtons()
{
	m_bellButtons.clear();
	std::string body;
	if (!http_get_v2_with_key("bell_button", m_ApplicationKey, body))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to fetch bell_button");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(body, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to parse bell_button JSON");
		return false;
	}
	if (root.isMember("errors") && !root["errors"].empty())
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseBellButtonJson(root);
}

// parse /clip/v2/resource/bell_button
bool CPhilipsHueV2Sensors::parseBellButtonJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseBellButtonJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2BellButton b;
		if (item.isMember("id")) b.id = item["id"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			b.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("metadata") && item["metadata"].isObject() && item["metadata"].isMember("control_id"))
		{
			int parsed = item["metadata"]["control_id"].asInt();
			if (parsed >= 1 && parsed <= 256)
				b.control_id = parsed;
		}
		if (!b.owner_rid.empty())
			m_bellButtons.push_back(b);
	}
	return true;
}

// Fetch /clip/v2/resource/button
bool CPhilipsHueV2Sensors::FetchButtons()
{
	m_buttons.clear();
	std::string body;
	if (!http_get_v2_with_key("button", m_ApplicationKey, body))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to fetch button");
		return false;
	}
	Json::Value root;
	if (!ParseJSon(body, root))
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: failed to parse button JSON");
		return false;
	}
	if (root.isMember("errors") && !root["errors"].empty())
	{
		_log.Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}
	return parseButtonJson(root);
}

// parse /clip/v2/resource/button
bool CPhilipsHueV2Sensors::parseButtonJson(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("data") || !root["data"].isArray())
	{
		_log.Log(LOG_ERROR, "PhilipsHueV2: parseButtonJson unexpected structure");
		return false;
	}
	for (const auto& item : root["data"])
	{
		if (!item.isObject()) continue;
		HueV2Button b;
		if (item.isMember("id")) b.id = item["id"].asString();
		if (item.isMember("id_v1")) b.id_v1 = item["id_v1"].asString();
		if (item.isMember("owner") && item["owner"].isObject() && item["owner"].isMember("rid"))
			b.owner_rid = item["owner"]["rid"].asString();
		if (item.isMember("metadata") && item["metadata"].isObject() && item["metadata"].isMember("control_id"))
		{
			int parsed = item["metadata"]["control_id"].asInt();
			if (parsed >= 1 && parsed <= 256)
				b.control_id = parsed;
		}
		if (item.isMember("button") && item["button"].isObject())
		{
			const auto& btnObj = item["button"];
			if (btnObj.isMember("event_values") && btnObj["event_values"].isArray())
			{
				for (const auto& ev : btnObj["event_values"])
				{
					std::string evStr = ev.asString();
					if (evStr == "long_release" || evStr == "long_press")
					{
						b.supports_long_press = true;
						break;
					}
				}
			}
			if (btnObj.isMember("button_report") && btnObj["button_report"].isObject())
			{
				const auto& rep = btnObj["button_report"];
				if (rep.isMember("updated")) b.last_updated = rep["updated"].asString();
				if (rep.isMember("event")) b.last_event = rep["event"].asString();
			}
			if (b.last_event.empty() && btnObj.isMember("last_event"))
				b.last_event = btnObj["last_event"].asString();
		}
		if (!b.owner_rid.empty())
			m_buttons.push_back(b);
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
		if (item.isMember("tamper_reports") && item["tamper_reports"].isArray() && item["tamper_reports"].size() > 0)
		{
			const auto& arr = item["tamper_reports"];
			// Use first report, matching HA behavior
			const Json::Value& first = arr[0];
			if (first.isMember("state")) t.state = first["state"].asString();
			if (first.isMember("source")) t.source = first["source"].asString();
			if (first.isMember("changed")) t.changed = first["changed"].asString();
		}
		else
		{
			_log.Debug(DEBUG_HARDWARE, "PhilipsHueV2: tamper device %s has no tamper_reports", t.id.c_str());
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
		if (!item.isObject())
			continue;
		HueV2DevicePower p;
		if (item.isMember("id"))
			p.id = item["id"].asString();
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
	m_BaseURLv2 << "/clip/v2/resource/light"
	m_BaseURLv2 << "/clip/v2/resource/motion"
	m_BaseURLv2 << "/clip/v2/resource/temperature"
	m_BaseURLv2 << "/clip/v2/resource/light_level"
	m_BaseURLv2 << "/clip/v2/resource/contact"
	m_BaseURLv2 << "/clip/v2/resource/tamper"
	m_BaseURLv2 << "/clip/v2/resource/device_power"
*/