#include "stdafx.h"
#include "WebServer.h"
#include "mainworker.h"
#include "SQLHelper.h"
#include "json_helper.h"
#include "Logger.h"
#include "Helper.h"
#include "RFXNames.h"
#include "../webserver/cWebem.h"
#include "../webserver/request.hpp"
#include "../webserver/reply.hpp"
#include "../httpclient/HTTPClient.h"
#include "../hardware/hardwaretypes.h"
#include "../hardware/ColorSwitch.h"
#include <json/json.h>
#include <sstream>
#include <string>
#include <set>

extern CLogger _log;

namespace http {
namespace server {

static Json::Value CreateCapability(const std::string& intf)
{
	Json::Value capability;
	capability["type"] = "AlexaInterface";
	capability["interface"] = intf;
	capability["version"] = "3";
	return capability;
}

static Json::Value CreateCapabilityWithProperties(const std::string& intf, const std::string& property_name, bool proactivelyReported, bool retrievable, bool nonControllable = false)
{
	Json::Value capability = CreateCapability(intf);
	capability["properties"]["supported"] = Json::Value(Json::arrayValue);
	Json::Value prop;
	prop["name"] = property_name;
	capability["properties"]["supported"].append(prop);
	capability["properties"]["proactivelyReported"] = proactivelyReported;
	capability["properties"]["retrievable"] = retrievable;
	if (nonControllable)
		capability["properties"]["nonControllable"] = true;
	return capability;
}

static Json::Value CreateEndpoint(const std::string& endpoint_id, const std::string& friendly_name, const std::string& description, const std::string& display_category)
{
	Json::Value endpoint;
	endpoint["endpointId"] = endpoint_id;
	endpoint["manufacturerName"] = "Domoticz";
	endpoint["friendlyName"] = friendly_name;
	endpoint["description"] = description;
	endpoint["displayCategories"] = Json::Value(Json::arrayValue);
	endpoint["displayCategories"].append(display_category);
	return endpoint;
}

static Json::Value CreateActionMapping(const std::string& action, const std::string& directive_name, const Json::Value& payload)
{
	Json::Value mapping;
	mapping["@type"] = "ActionsToDirective";
	mapping["actions"] = Json::Value(Json::arrayValue);
	mapping["actions"].append(action);
	mapping["directive"]["name"] = directive_name;
	mapping["directive"]["payload"] = payload;
	return mapping;
}

static Json::Value CreateStateMapping(const std::string& state, int value)
{
	Json::Value mapping;
	mapping["@type"] = "StatesToValue";
	mapping["states"] = Json::Value(Json::arrayValue);
	mapping["states"].append(state);
	mapping["value"] = value;
	return mapping;
}

static Json::Value CreateStateRangeMapping(const std::string& state, int min_value, int max_value)
{
	Json::Value mapping;
	mapping["@type"] = "StatesToRange";
	mapping["states"] = Json::Value(Json::arrayValue);
	mapping["states"].append(state);
	mapping["range"]["minimumValue"] = min_value;
	mapping["range"]["maximumValue"] = max_value;
	return mapping;
}

static std::string GetISO8601Timestamp()
{
	time_t now = mytime(nullptr);
	struct tm tm1;
#ifdef WIN32
	gmtime_s(&tm1, &now);
#else
	gmtime_r(&now, &tm1);
#endif
	char szTmp[80];
	strftime(szTmp, sizeof(szTmp), "%Y-%m-%dT%H:%M:%SZ", &tm1);
	return std::string(szTmp);
}

// Map common units to Alexa unit types (metric only, plus percent)
static std::string MapUnitToAlexaType(const std::string& unit)
{
	// Weight
	if (unit == "kg") return "Weight.Kilograms";
	if (unit == "g") return "Weight.Grams";

	// Distance
	if (unit == "m") return "Distance.Meters";
	if (unit == "km") return "Distance.Kilometers";

	// Volume
	if (unit == "l" || unit == "L") return "Volume.Liters";

	// Percent
	if (unit == "%") return "Percent";

	// Extended Alexa.Unit types
	if (unit == "g/m³" || unit == "g/m3") return "Alexa.Unit.Density.GramsPerCubicMeter";
	if (unit == "V") return "Alexa.Unit.ElectricPotential.Volts";
	if (unit == "A") return "Alexa.Unit.ElectricCurrent.Amperes";
	if (unit == "W") return "Alexa.Unit.Power.Watts";
	if (unit == "kW") return "Alexa.Unit.Power.Kilowatts";
	if (unit == "kWh") return "Alexa.Unit.Energy.KilowattHours";
	if (unit == "°C" || unit == "C") return "Temperature.Celsius";
	if (unit == "Pa") return "Alexa.Unit.Pressure.Pascals";
	if (unit == "hPa") return "Alexa.Unit.Pressure.Hectopascals";
	if (unit == "dB") return "Alexa.Unit.SoundPressure.Decibels";

	return ""; // No mapping available
}

// Get appropriate range and precision for sensor unit
static void GetSensorRange(const std::string& unit, double& min, double& max, double& precision)
{
	// Default values
	min = 0;
	max = 1000;
	precision = 0.01;

	// Weight
	if (unit == "kg") { max = 500; precision = 0.01; }
	else if (unit == "g") { max = 100000; precision = 1; }

	// Distance
	else if (unit == "m") { max = 1000; precision = 0.1; }
	else if (unit == "km") { max = 100; precision = 0.01; }

	// Volume
	else if (unit == "l" || unit == "L") { max = 1000; precision = 0.1; }

	// Percent
	else if (unit == "%") { max = 100; precision = 0.1; }

	// Electrical
	else if (unit == "V") { max = 500; precision = 0.01; }
	else if (unit == "A") { max = 100; precision = 0.01; }
	else if (unit == "W") { max = 10000; precision = 1; }
	else if (unit == "kW") { max = 100; precision = 0.01; }
	else if (unit == "kWh") { max = 100000; precision = 0.01; }

	// Temperature
	else if (unit == "°C" || unit == "C") { min = -50; max = 100; precision = 0.1; }

	// Pressure
	else if (unit == "Pa") { max = 200000; precision = 1; }
	else if (unit == "hPa") { max = 2000; precision = 0.1; }

	// Sound
	else if (unit == "dB") { max = 150; precision = 0.1; }

	// Absolute humidity
	else if (unit == "g/m³" || unit == "g/m3") { max = 50; precision = 0.01; }
}

// Get semantic instance name for sensor based on unit
static std::string GetSensorInstanceName(const std::string& unit)
{
	// Electrical
	if (unit == "V") return "Sensor.Voltage";
	if (unit == "A") return "Sensor.Current";
	if (unit == "W" || unit == "kW") return "Sensor.Power";
	if (unit == "kWh") return "Sensor.Energy";

	// Temperature
	if (unit == "°C" || unit == "C") return "Sensor.Temperature";

	// Pressure
	if (unit == "Pa" || unit == "hPa") return "Sensor.Pressure";

	// Sound
	if (unit == "dB") return "Sensor.SoundLevel";

	// Absolute humidity
	if (unit == "g/m³" || unit == "g/m3") return "Sensor.AbsoluteHumidity";

	// Distance
	if (unit == "m" || unit == "km") return "Sensor.Distance";

	// Weight
	if (unit == "kg" || unit == "g") return "Sensor.Weight";

	// Volume
	if (unit == "l" || unit == "L") return "Sensor.Volume";

	// Percent
	if (unit == "%") return "Sensor.Percentage";

	// Default
	return "Sensor.Value";
}

static Json::Value CreateProperty(const std::string& ns, const std::string& name, const Json::Value& value, const std::string& instance = "")
{
	Json::Value prop;
	prop["namespace"] = ns;
	if (!instance.empty())
		prop["instance"] = instance;
	prop["name"] = name;
	prop["value"] = value;
	prop["timeOfSample"] = GetISO8601Timestamp();
	prop["uncertaintyInMilliseconds"] = 500;
	return prop;
}

static Json::Value CreateEndpointHealthProperty()
{
	Json::Value health_value;
	health_value["value"] = "OK";
	Json::Value health = CreateProperty("Alexa.EndpointHealth", "connectivity", health_value);
	health["uncertaintyInMilliseconds"] = 0;
	return health;
}

// Convert HSB (Hue 0-360, Saturation 0-1, Brightness 0-1) to RGB (0-255)
static void HSBtoRGB(double hue, double saturation, double brightness, int& r, int& g, int& b)
{
	if (saturation == 0)
	{
		r = g = b = (int)(brightness * 255);
		return;
	}

	double h = hue / 60.0;
	int i = (int)h;
	double f = h - i;
	double p = brightness * (1.0 - saturation);
	double q = brightness * (1.0 - saturation * f);
	double t = brightness * (1.0 - saturation * (1.0 - f));

	switch (i % 6)
	{
	case 0: r = (int)(brightness * 255); g = (int)(t * 255); b = (int)(p * 255); break;
	case 1: r = (int)(q * 255); g = (int)(brightness * 255); b = (int)(p * 255); break;
	case 2: r = (int)(p * 255); g = (int)(brightness * 255); b = (int)(t * 255); break;
	case 3: r = (int)(p * 255); g = (int)(q * 255); b = (int)(brightness * 255); break;
	case 4: r = (int)(t * 255); g = (int)(p * 255); b = (int)(brightness * 255); break;
	case 5: r = (int)(brightness * 255); g = (int)(p * 255); b = (int)(q * 255); break;
	}
}

// Convert RGB (0-255) to HSB (Hue 0-360, Saturation 0-1, Brightness 0-1)
static void RGBtoHSB(int r, int g, int b, double& hue, double& saturation, double& brightness)
{
	double rd = r / 255.0;
	double gd = g / 255.0;
	double bd = b / 255.0;

	double max_val = std::max({rd, gd, bd});
	double min_val = std::min({rd, gd, bd});
	double delta = max_val - min_val;

	brightness = max_val;

	if (delta == 0)
	{
		hue = 0;
		saturation = 0;
		return;
	}

	saturation = delta / max_val;

	if (rd == max_val)
		hue = 60.0 * (fmod(((gd - bd) / delta), 6.0));
	else if (gd == max_val)
		hue = 60.0 * (((bd - rd) / delta) + 2.0);
	else
		hue = 60.0 * (((rd - gd) / delta) + 4.0);

	if (hue < 0)
		hue += 360.0;
}

static void CreateErrorResponse(Json::Value& root, const Json::Value& request_json, const std::string& error_type, const std::string& error_message)
{
	root["event"]["header"]["namespace"] = "Alexa";
	root["event"]["header"]["name"] = "ErrorResponse";
	root["event"]["header"]["payloadVersion"] = "3";
	if (request_json.isMember("directive") && request_json["directive"].isMember("header") && request_json["directive"]["header"].isMember("messageId"))
		root["event"]["header"]["messageId"] = request_json["directive"]["header"]["messageId"].asString();
	root["event"]["payload"]["type"] = error_type;
	root["event"]["payload"]["message"] = error_message;
}

// Build Alexa instance name from device name in cookie (e.g., "DeviceName.Mode")
static std::string BuildInstanceName(const Json::Value& cookie, const std::string& endpoint_id, const std::string& suffix)
{
	if (cookie.isMember("deviceName"))
	{
		std::string device_name = cookie["deviceName"].asString();
		device_name.erase(std::remove_if(device_name.begin(), device_name.end(),
			[](char c) { return !isalnum(c); }), device_name.end());
		return device_name + suffix;
	}
	return endpoint_id + suffix;
}

static Json::Value CreateFriendlyNames(const std::string& text)
{
	Json::Value names = Json::Value(Json::arrayValue);
	Json::Value name_us;
	name_us["@type"] = "text";
	name_us["value"]["locale"] = "en-US";
	name_us["value"]["text"] = text;
	names.append(name_us);
	Json::Value name_gb;
	name_gb["@type"] = "text";
	name_gb["value"]["locale"] = "en-GB";
	name_gb["value"]["text"] = text;
	names.append(name_gb);
	return names;
}

static Json::Value CreateAssetName(const std::string& assetId)
{
	Json::Value asset;
	asset["@type"] = "asset";
	asset["value"]["assetId"] = assetId;
	return asset;
}

static Json::Value CreateFriendlyNamesWithAsset(const std::string& assetId, const std::string& text)
{
	Json::Value names = Json::Value(Json::arrayValue);
	names.append(CreateAssetName(assetId));
	Json::Value name_us;
	name_us["@type"] = "text";
	name_us["value"]["locale"] = "en-US";
	name_us["value"]["text"] = text;
	names.append(name_us);
	Json::Value name_gb;
	name_gb["@type"] = "text";
	name_gb["value"]["locale"] = "en-GB";
	name_gb["value"]["text"] = text;
	names.append(name_gb);
	return names;
}

void CWebServer::Alexa_HandleDiscovery(WebEmSession& session, const request& req, Json::Value& root)
{
	Json::Value request_json;
	if (!ParseJSon(req.content, request_json))
	{
		root["event"]["header"]["namespace"] = "Alexa";
		CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "Invalid JSON");
		return;
	}

	// Build discovery response structure
	root["event"]["header"]["namespace"] = "Alexa.Discovery";
	root["event"]["header"]["name"] = "Discover.Response";
	root["event"]["header"]["payloadVersion"] = "3";
	root["event"]["header"]["messageId"] = request_json["directive"]["header"]["messageId"].asString();
	root["event"]["payload"]["endpoints"] = Json::Value(Json::arrayValue);

	// Get user ID for access control
	int iUser = FindUser(session.username.c_str());
	if (iUser == -1)
	{
		_log.Log(LOG_ERROR, "Alexa Discovery: User '%s' not found", session.username.c_str());
		return;
	}

	// Determine if user has control permissions (only SWITCHER and ADMIN can control)
	bool bControlPermitted = (session.rights == URIGHTS_SWITCHER || session.rights == URIGHTS_ADMIN);

	// Get devices - all devices for admin, shared devices for regular users
	std::vector<std::vector<std::string>> devices_result;
	if (m_users[iUser].userrights == URIGHTS_ADMIN)
	{
		devices_result = m_sql.safe_query(
			"SELECT DISTINCT d.ID, d.Name, d.Type, d.SubType, d.SwitchType, d.Options, d.sValue "
			"FROM DeviceStatus d "
			"WHERE d.Used = 1 "
			"AND d.ID NOT IN (SELECT DeviceRowID FROM DeviceToPlansMap WHERE PlanID IN (SELECT ID FROM Plans WHERE Name = '$Hidden Devices') AND DevSceneType = 0) "
			"ORDER BY d.ID");
	}
	else
	{
		unsigned long userID = m_users[iUser].ID;
		devices_result = m_sql.safe_query(
			"SELECT DISTINCT d.ID, d.Name, d.Type, d.SubType, d.SwitchType, d.Options, d.sValue "
			"FROM DeviceStatus d "
			"INNER JOIN SharedDevices s ON d.ID = s.DeviceRowID "
			"WHERE s.SharedUserID = %lu AND d.Used = 1 "
			"AND d.ID NOT IN (SELECT DeviceRowID FROM DeviceToPlansMap WHERE PlanID IN (SELECT ID FROM Plans WHERE Name = '$Hidden Devices') AND DevSceneType = 0) "
			"ORDER BY d.ID",
			userID);
	}

	// First pass: identify thermostat component groupings
	// Map from temperature device idx (or setpoint idx for standalone) to setpoint/mode/humidity device indices
	std::map<std::string, std::map<std::string, std::string>> thermostat_components;
	std::set<std::string> linked_devices; // Devices that are part of thermostats (don't expose separately)

	for (const auto& device_row : devices_result)
	{
		std::string device_idx = device_row[0];
		std::string device_name = device_row[1];
		int device_type = atoi(device_row[2].c_str());

		// Look for Setpoint devices
		if (device_type == pTypeSetpoint)
		{
			bool found_temp = false;

			// Find matching temperature sensor with same base name
			for (const auto& temp_row : devices_result)
			{
				std::string temp_idx = temp_row[0];
				std::string temp_name = temp_row[1];
				int temp_type = atoi(temp_row[2].c_str());

				if ((temp_type == pTypeTEMP || temp_type == pTypeTEMP_HUM || temp_type == pTypeTEMP_BARO) &&
				    temp_name == device_name)
				{
					// Found matching temperature sensor
					found_temp = true;
					thermostat_components[temp_idx]["setpoint"] = device_idx;
					linked_devices.insert(device_idx); // Hide setpoint device

					// Look for Mode selector with " Mode" suffix
					std::string mode_name = device_name + " Mode";
					for (const auto& mode_row : devices_result)
					{
						std::string mode_idx = mode_row[0];
						std::string mode_device_name = mode_row[1];
						int mode_switch_type = atoi(mode_row[4].c_str());

						if (mode_device_name == mode_name && mode_switch_type == STYPE_Selector)
						{
							thermostat_components[temp_idx]["mode"] = mode_idx;
							linked_devices.insert(mode_idx); // Hide mode selector
							break;
						}
					}

					// Look for humidity sensor with same base name
					for (const auto& hum_row : devices_result)
					{
						std::string hum_idx = hum_row[0];
						std::string hum_name = hum_row[1];
						int hum_type = atoi(hum_row[2].c_str());

						if ((hum_type == pTypeHUM || hum_type == pTypeTEMP_HUM) &&
						    hum_name == device_name && hum_idx != temp_idx)
						{
							thermostat_components[temp_idx]["humidity"] = hum_idx;
							linked_devices.insert(hum_idx); // Hide humidity sensor
							break;
						}
					}
					break;
				}
			}

			// If no temperature sensor found, create standalone setpoint thermostat
			if (!found_temp)
			{
				thermostat_components[device_idx]["setpoint"] = device_idx;

				// Look for Mode selector
				std::string mode_name = device_name + " Mode";
				for (const auto& mode_row : devices_result)
				{
					std::string mode_idx = mode_row[0];
					std::string mode_device_name = mode_row[1];
					int mode_switch_type = atoi(mode_row[4].c_str());

					if (mode_device_name == mode_name && mode_switch_type == STYPE_Selector)
					{
						thermostat_components[device_idx]["mode"] = mode_idx;
						linked_devices.insert(mode_idx); // Hide mode selector
						break;
					}
				}
			}
		}
	}

	for (const auto& device_row : devices_result)
	{
		std::string device_idx = device_row[0];
		std::string device_name = device_row[1];
		int device_type = atoi(device_row[2].c_str());
		int device_subtype = atoi(device_row[3].c_str());
		int switch_type = atoi(device_row[4].c_str());
		std::string options_str = device_row[5];
		std::string sValue = device_row[6];

		// Skip devices that are part of thermostat groupings
		if (linked_devices.find(device_idx) != linked_devices.end())
		{
			continue;
		}

		// Parse options using BuildDeviceOptions (handles base64 decoding)
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(options_str);

		if (IsLightOrSwitch(device_type, device_subtype)
		    || ((device_type == pTypeRego6XXValue) && (device_subtype == sTypeRego6XXStatus)))
		{
			// Handle selector switches
			if (switch_type == STYPE_Selector)
			{
				Json::Value endpoint = CreateEndpoint("selector_" + device_idx, device_name, "Selector Switch", "OTHER");

				// Create instance name from device name (remove spaces)
				std::string instance_name = device_name;
				instance_name.erase(std::remove(instance_name.begin(), instance_name.end(), ' '), instance_name.end());
				instance_name += ".Mode";

				// Add ModeController capability
				Json::Value mode_capability = CreateCapabilityWithProperties("Alexa.ModeController", "mode", false, true, !bControlPermitted);
				mode_capability["instance"] = instance_name;

				// Add capability resources (friendly names for the capability itself)
				mode_capability["capabilityResources"]["friendlyNames"] = CreateFriendlyNames(device_name);

				// Parse LevelNames from options
				std::map<std::string, std::string> selector_statuses;
				GetSelectorSwitchStatuses(options, selector_statuses);

				if (selector_statuses.empty())
				{
					_log.Log(LOG_ERROR, "(Alexa) Selector switch '%s' (idx %s) has no level names configured", device_name.c_str(), device_idx.c_str());
					continue;
				}

				// Check if Off level should be hidden
				bool hide_off_level = false;
				auto it_hide = options.find("LevelOffHidden");
				if (it_hide != options.end() && it_hide->second == "true")
				{
					hide_off_level = true;
				}

				mode_capability["configuration"]["ordered"] = false;
				mode_capability["configuration"]["supportedModes"] = Json::Value(Json::arrayValue);

				for (const auto& status : selector_statuses)
				{
					int level_value = atoi(status.first.c_str());

					// Skip level 0 if LevelOffHidden is true
					if (level_value == 0 && hide_off_level)
					{
						continue;
					}

					Json::Value mode;
					mode["value"] = "Level." + std::to_string(level_value);
					mode["modeResources"]["friendlyNames"] = CreateFriendlyNames(status.second);
					mode_capability["configuration"]["supportedModes"].append(mode);
				}
				endpoint["capabilities"] = Json::Value(Json::arrayValue);
				endpoint["capabilities"].append(mode_capability);
				endpoint["capabilities"].append(CreateCapability("Alexa"));

				// Add cookie with metadata
				endpoint["cookie"]["WhatAmI"] = "selector";
				endpoint["cookie"]["switchtype"] = switch_type;
				endpoint["cookie"]["deviceName"] = device_name;

				root["event"]["payload"]["endpoints"].append(endpoint);
			}
			// Handle blinds
			else if (switch_type == STYPE_Blinds || switch_type == STYPE_BlindsPercentage ||
			         switch_type == STYPE_BlindsPercentageWithStop || switch_type == STYPE_BlindsWithStop)
			{
				Json::Value endpoint = CreateEndpoint("blind_" + device_idx, device_name, "Blind", "INTERIOR_BLIND");

				bool has_percentage = (switch_type == STYPE_BlindsPercentage || switch_type == STYPE_BlindsPercentageWithStop);
				bool has_stop = (switch_type == STYPE_BlindsPercentageWithStop || switch_type == STYPE_BlindsWithStop);

				// Add RangeController capability for blind position
				Json::Value range_capability = CreateCapabilityWithProperties("Alexa.RangeController", "rangeValue", false, has_percentage, !bControlPermitted);
				range_capability["instance"] = "Blind.Lift";

				// Add capability resources using Alexa assets
				range_capability["capabilityResources"]["friendlyNames"] = CreateFriendlyNamesWithAsset("Alexa.Setting.Opening", "Position");

				// Configure range
				range_capability["configuration"]["supportedRange"]["minimumValue"] = 0;
				range_capability["configuration"]["supportedRange"]["maximumValue"] = 100;
				range_capability["configuration"]["supportedRange"]["precision"] = 1;

				// Add presets for Open and Closed
				range_capability["configuration"]["presets"] = Json::Value(Json::arrayValue);

				// Closed preset
				Json::Value closed_preset;
				closed_preset["rangeValue"] = 0;
				closed_preset["presetResources"]["friendlyNames"] = CreateFriendlyNamesWithAsset("Alexa.Value.Close", "Closed");
				range_capability["configuration"]["presets"].append(closed_preset);

				// Open preset
				Json::Value open_preset;
				open_preset["rangeValue"] = 100;
				open_preset["presetResources"]["friendlyNames"] = CreateFriendlyNamesWithAsset("Alexa.Value.Open", "Open");
				range_capability["configuration"]["presets"].append(open_preset);

				// Add semantics for actions
				range_capability["semantics"]["actionMappings"] = Json::Value(Json::arrayValue);

				Json::Value close_payload;
				close_payload["rangeValue"] = 0;
				range_capability["semantics"]["actionMappings"].append(CreateActionMapping("Alexa.Actions.Close", "SetRangeValue", close_payload));

				Json::Value open_payload;
				open_payload["rangeValue"] = 100;
				range_capability["semantics"]["actionMappings"].append(CreateActionMapping("Alexa.Actions.Open", "SetRangeValue", open_payload));

				Json::Value lower_payload;
				lower_payload["rangeValueDelta"] = -10;
				lower_payload["rangeValueDeltaDefault"] = false;
				range_capability["semantics"]["actionMappings"].append(CreateActionMapping("Alexa.Actions.Lower", "AdjustRangeValue", lower_payload));

				Json::Value raise_payload;
				raise_payload["rangeValueDelta"] = 10;
				raise_payload["rangeValueDeltaDefault"] = false;
				range_capability["semantics"]["actionMappings"].append(CreateActionMapping("Alexa.Actions.Raise", "AdjustRangeValue", raise_payload));

				// Add state mappings
				range_capability["semantics"]["stateMappings"] = Json::Value(Json::arrayValue);
				range_capability["semantics"]["stateMappings"].append(CreateStateMapping("Alexa.States.Closed", 0));
				range_capability["semantics"]["stateMappings"].append(CreateStateRangeMapping("Alexa.States.Open", 1, 100));

				endpoint["capabilities"] = Json::Value(Json::arrayValue);
				endpoint["capabilities"].append(range_capability);

				// Add PowerController if blind has stop button
				if (has_stop)
				{
					Json::Value power_capability = CreateCapabilityWithProperties("Alexa.PowerController", "powerState", false, false, !bControlPermitted);
					endpoint["capabilities"].append(power_capability);
				}

				endpoint["capabilities"].append(CreateCapability("Alexa"));

				// Add cookie with metadata
				endpoint["cookie"]["WhatAmI"] = "blind";
				endpoint["cookie"]["switchtype"] = switch_type;
				endpoint["cookie"]["deviceName"] = device_name;

				root["event"]["payload"]["endpoints"].append(endpoint);
			}
			// Handle door locks
			else if (switch_type == STYPE_DoorLock || switch_type == STYPE_DoorLockInverted)
			{
				Json::Value endpoint = CreateEndpoint("lock_" + device_idx, device_name, "Door Lock", "SMARTLOCK");

				// Add LockController capability
				Json::Value lock_capability = CreateCapabilityWithProperties("Alexa.LockController", "lockState", false, true, !bControlPermitted);
				endpoint["capabilities"] = Json::Value(Json::arrayValue);
				endpoint["capabilities"].append(lock_capability);
				endpoint["capabilities"].append(CreateCapability("Alexa"));

				// Add cookie with metadata
				endpoint["cookie"]["WhatAmI"] = "lock";
				endpoint["cookie"]["switchtype"] = switch_type;
				endpoint["cookie"]["deviceName"] = device_name;

				root["event"]["payload"]["endpoints"].append(endpoint);
			}
			// Handle simple On/Off and Dimmer switches
			else if (switch_type == STYPE_OnOff || switch_type == STYPE_Dimmer)
			{
				// Check if this is a color device
				bool has_color = (device_type == pTypeColorSwitch);
				bool has_color_temp = (device_subtype == sTypeColor_RGB_CW_WW || device_subtype == sTypeColor_RGB_CW_WW_Z);

				Json::Value endpoint = CreateEndpoint("switch_" + device_idx, device_name,
					has_color ? "RGB Light" : ((switch_type == STYPE_Dimmer) ? "Dimmable Light" : "Switch"),
					"LIGHT");

				// Add PowerController capability
				Json::Value power_capability = CreateCapabilityWithProperties("Alexa.PowerController", "powerState", false, true, !bControlPermitted);
				endpoint["capabilities"] = Json::Value(Json::arrayValue);
				endpoint["capabilities"].append(power_capability);

				// Add BrightnessController for dimmers
				if (switch_type == STYPE_Dimmer)
				{
					Json::Value brightness_capability = CreateCapabilityWithProperties("Alexa.BrightnessController", "brightness", false, true, !bControlPermitted);
					endpoint["capabilities"].append(brightness_capability);
				}

				// Add ColorController for RGB devices
				if (has_color)
				{
					Json::Value color_capability = CreateCapabilityWithProperties("Alexa.ColorController", "color", false, true, !bControlPermitted);
					endpoint["capabilities"].append(color_capability);
				}

				// Add ColorTemperatureController for RGBWW/RGBWWZ devices
				if (has_color_temp)
				{
					Json::Value color_temp_capability = CreateCapabilityWithProperties("Alexa.ColorTemperatureController", "colorTemperatureInKelvin", false, true, !bControlPermitted);
					endpoint["capabilities"].append(color_temp_capability);
				}

				endpoint["capabilities"].append(CreateCapability("Alexa"));

				// Add cookie with metadata
				endpoint["cookie"]["WhatAmI"] = "switch";
				endpoint["cookie"]["switchtype"] = switch_type;
				endpoint["cookie"]["deviceName"] = device_name;
				endpoint["cookie"]["hasColor"] = has_color;

				root["event"]["payload"]["endpoints"].append(endpoint);
			}
		}
		// Handle standalone setpoint devices (thermostats without temperature sensor)
		else if (device_type == pTypeSetpoint)
		{
			// Check if this is a standalone thermostat
			bool is_thermostat = (thermostat_components.find(device_idx) != thermostat_components.end());
			if (!is_thermostat)
			{
				continue; // Skip if not in thermostat_components (shouldn't happen)
			}

			Json::Value endpoint = CreateEndpoint("setpoint_" + device_idx, device_name, "Thermostat", "THERMOSTAT");
			endpoint["capabilities"] = Json::Value(Json::arrayValue);

			// Add ThermostatController
			Json::Value thermo_capability = CreateCapabilityWithProperties("Alexa.ThermostatController", "targetSetpoint", false, true, !bControlPermitted);

			// Add thermostatMode to supported properties
			thermo_capability["properties"]["supported"].append(Json::Value());
			thermo_capability["properties"]["supported"][1]["name"] = "thermostatMode";

			// Add configuration for supported modes
			thermo_capability["configuration"]["supportsScheduling"] = false;
			thermo_capability["configuration"]["supportedModes"] = Json::Value(Json::arrayValue);
			thermo_capability["configuration"]["supportedModes"].append("HEAT");
			thermo_capability["configuration"]["supportedModes"].append("OFF");

			endpoint["capabilities"].append(thermo_capability);
			endpoint["capabilities"].append(CreateCapability("Alexa.EndpointHealth"));
			endpoint["capabilities"].append(CreateCapability("Alexa"));

			// Add cookie with metadata
			endpoint["cookie"]["WhatAmI"] = "thermostat";
			endpoint["cookie"]["sensorType"] = device_type;
			endpoint["cookie"]["deviceName"] = device_name;
			endpoint["cookie"]["setpointIdx"] = device_idx;

			auto& components = thermostat_components[device_idx];
			if (components.find("mode") != components.end())
			{
				endpoint["cookie"]["modeIdx"] = components["mode"];
			}

			root["event"]["payload"]["endpoints"].append(endpoint);
		}
		// Handle Evohome zones (single-device thermostats)
		else if (device_type == pTypeEvohomeZone)
		{
			Json::Value endpoint = CreateEndpoint("zone_" + device_idx, device_name, "Thermostat", "THERMOSTAT");
			endpoint["capabilities"] = Json::Value(Json::arrayValue);

			// Add ThermostatController (setpoint only, no mode control)
			Json::Value thermo_capability = CreateCapabilityWithProperties("Alexa.ThermostatController", "targetSetpoint", false, true, !bControlPermitted);
			thermo_capability["configuration"]["supportsScheduling"] = false;
			endpoint["capabilities"].append(thermo_capability);

			// Add TemperatureSensor (zones have current temperature)
			endpoint["capabilities"].append(CreateCapabilityWithProperties("Alexa.TemperatureSensor", "temperature", false, true));
			endpoint["capabilities"].append(CreateCapability("Alexa.EndpointHealth"));
			endpoint["capabilities"].append(CreateCapability("Alexa"));

			endpoint["cookie"]["WhatAmI"] = "evohome_zone";
			endpoint["cookie"]["deviceName"] = device_name;

			root["event"]["payload"]["endpoints"].append(endpoint);
		}
		// Handle temperature and environmental sensors (including thermostats)
		else if (device_type == pTypeTEMP || device_type == pTypeTEMP_HUM || device_type == pTypeHUM ||
		         device_type == pTypeTEMP_HUM_BARO || device_type == pTypeTEMP_BARO)
		{
			std::string sensor_idx = device_idx;
			std::string sensor_name = device_name;
			int sensor_type = device_type;

			// Check if this is a thermostat (has linked setpoint)
			bool is_thermostat = (thermostat_components.find(device_idx) != thermostat_components.end());

			// Determine description and category based on sensor type
			std::string description, category;
			if (is_thermostat)
			{
				description = "Thermostat";
				category = "THERMOSTAT";
			}
			else if (sensor_type == pTypeHUM)
			{
				description = "Humidity Sensor";
				category = "OTHER";
			}
			else
			{
				description = "Temperature Sensor";
				category = "TEMPERATURE_SENSOR";
			}

			Json::Value endpoint = CreateEndpoint("sensor_" + sensor_idx, sensor_name, description, category);

			endpoint["capabilities"] = Json::Value(Json::arrayValue);

			// Add ThermostatController for thermostats
			if (is_thermostat)
			{
				Json::Value thermo_capability = CreateCapabilityWithProperties("Alexa.ThermostatController", "targetSetpoint", false, true, !bControlPermitted);

				// Add thermostatMode to supported properties
				thermo_capability["properties"]["supported"].append(Json::Value());
				thermo_capability["properties"]["supported"][1]["name"] = "thermostatMode";

				// Add configuration for supported modes
				thermo_capability["configuration"]["supportsScheduling"] = false;
				thermo_capability["configuration"]["supportedModes"] = Json::Value(Json::arrayValue);
				thermo_capability["configuration"]["supportedModes"].append("HEAT");
				thermo_capability["configuration"]["supportedModes"].append("OFF");

				endpoint["capabilities"].append(thermo_capability);
			}

			// Add TemperatureSensor for Temp and Temp+Hum devices
			if (sensor_type == pTypeTEMP || sensor_type == pTypeTEMP_HUM || sensor_type == pTypeTEMP_HUM_BARO || sensor_type == pTypeTEMP_BARO)
			{
				endpoint["capabilities"].append(CreateCapabilityWithProperties("Alexa.TemperatureSensor", "temperature", false, true));
			}

			// Add HumiditySensor for Temp+Hum and humidity-only sensors, or if thermostat has linked humidity
			if (sensor_type == pTypeTEMP_HUM || sensor_type == pTypeTEMP_HUM_BARO || sensor_type == pTypeHUM ||
			    (is_thermostat && thermostat_components[device_idx].find("humidity") != thermostat_components[device_idx].end()))
			{
				endpoint["capabilities"].append(CreateCapabilityWithProperties("Alexa.HumiditySensor", "relativeHumidity", false, true));
			}

			endpoint["capabilities"].append(CreateCapability("Alexa.EndpointHealth"));
			endpoint["capabilities"].append(CreateCapability("Alexa"));

			// Add cookie with metadata
			endpoint["cookie"]["WhatAmI"] = is_thermostat ? "thermostat" : "sensor";
			endpoint["cookie"]["sensorType"] = sensor_type;
			endpoint["cookie"]["deviceName"] = sensor_name;

			// Add thermostat component indices
			if (is_thermostat)
			{
				auto& components = thermostat_components[device_idx];
				endpoint["cookie"]["setpointIdx"] = components["setpoint"];
				if (components.find("mode") != components.end())
				{
					endpoint["cookie"]["modeIdx"] = components["mode"];
				}
				if (components.find("humidity") != components.end())
				{
					endpoint["cookie"]["humidityIdx"] = components["humidity"];
				}
			}

			root["event"]["payload"]["endpoints"].append(endpoint);
		}
		// Handle Weight sensors
		else if (device_type == pTypeWEIGHT)
		{
			Json::Value endpoint = CreateEndpoint("weight_" + device_idx, device_name, "Weight Sensor", "OTHER");
			endpoint["capabilities"] = Json::Value(Json::arrayValue);

			// Add RangeController for weight (read-only)
			Json::Value weight_capability = CreateCapabilityWithProperties("Alexa.RangeController", "rangeValue", false, true);
			weight_capability["instance"] = "Weight.Weight";
			weight_capability["capabilityResources"]["friendlyNames"] = CreateFriendlyNames("Weight");
			weight_capability["properties"]["nonControllable"] = true;
			weight_capability["configuration"]["supportedRange"]["minimumValue"] = 0;
			weight_capability["configuration"]["supportedRange"]["maximumValue"] = 500;
			weight_capability["configuration"]["supportedRange"]["precision"] = 0.01;
			weight_capability["configuration"]["unitOfMeasure"] = "Weight.Kilograms";

			endpoint["capabilities"].append(weight_capability);
			endpoint["capabilities"].append(CreateCapability("Alexa.EndpointHealth"));
			endpoint["capabilities"].append(CreateCapability("Alexa"));

			endpoint["cookie"]["WhatAmI"] = "weight";
			endpoint["cookie"]["deviceName"] = device_name;

			root["event"]["payload"]["endpoints"].append(endpoint);
		}
		// Handle General sensors (custom sensors with various units)
		else if (device_type == pTypeGeneral)
		{
			// Exclude non-numeric sensor types
			if (device_subtype == sTypeTextStatus ||
			    device_subtype == sTypeSetPoint ||
#ifdef WITH_OPENZWAVE
			    device_subtype == sTypeZWaveThermostatMode ||
			    device_subtype == sTypeZWaveThermostatFanMode ||
			    device_subtype == sTypeZWaveThermostatOperatingState ||
			    device_subtype == sTypeZWaveAlarm ||
#endif
			    device_subtype == sTypeAlert ||
			    device_subtype == sTypeCounterIncremental ||
			    device_subtype == sTypeManagedCounter)
			{
				continue;
			}

			// Parse unit from sValue (format: "value unit")
			std::string unit;
			size_t space_pos = sValue.find(' ');
			if (space_pos != std::string::npos && space_pos + 1 < sValue.length())
			{
				unit = sValue.substr(space_pos + 1);
			}

			std::string alexa_unit = MapUnitToAlexaType(unit);

			// Skip sensors with unmapped units
			if (alexa_unit.empty())
			{
				_log.Log(LOG_STATUS, "(Alexa) Skipping General sensor '%s' (idx %s) with unsupported unit: %s",
					device_name.c_str(), device_idx.c_str(), unit.c_str());
				continue;
			}

			// Get appropriate range for this sensor type
			double min_value, max_value, precision;
			GetSensorRange(unit, min_value, max_value, precision);

			// Get semantic instance name
			std::string instance_name = GetSensorInstanceName(unit);

			Json::Value endpoint = CreateEndpoint("general_" + device_idx, device_name, "Sensor", "OTHER");
			endpoint["capabilities"] = Json::Value(Json::arrayValue);

			// Add RangeController for general sensor (read-only)
			Json::Value sensor_capability = CreateCapabilityWithProperties("Alexa.RangeController", "rangeValue", false, true);
			sensor_capability["instance"] = instance_name;
			sensor_capability["capabilityResources"]["friendlyNames"] = CreateFriendlyNames(device_name);
			sensor_capability["properties"]["nonControllable"] = true;
			sensor_capability["configuration"]["supportedRange"]["minimumValue"] = min_value;
			sensor_capability["configuration"]["supportedRange"]["maximumValue"] = max_value;
			sensor_capability["configuration"]["supportedRange"]["precision"] = precision;

			// Add unit
			sensor_capability["configuration"]["unitOfMeasure"] = alexa_unit;

			endpoint["capabilities"].append(sensor_capability);
			endpoint["capabilities"].append(CreateCapability("Alexa.EndpointHealth"));
			endpoint["capabilities"].append(CreateCapability("Alexa"));

			endpoint["cookie"]["WhatAmI"] = "general";
			endpoint["cookie"]["deviceName"] = device_name;

			root["event"]["payload"]["endpoints"].append(endpoint);
		}
	}

	// Get scenes/groups that are not protected
	std::vector<std::vector<std::string>> scenes_result;
	scenes_result = m_sql.safe_query(
		"SELECT s.ID, s.Name, s.SceneType, s.Protected "
		"FROM Scenes s "
		"WHERE s.Protected = 0 "
		"ORDER BY s.Name");

	for (const auto& scene_row : scenes_result)
	{
		std::string scene_idx = scene_row[0];
		int scene_type = atoi(scene_row[2].c_str());

		Json::Value endpoint = CreateEndpoint("scene_" + scene_idx, scene_row[1], (scene_type == 0) ? "Scene" : "Group", (scene_type == 0) ? "SCENE_TRIGGER" : "SWITCH");

		// Scenes use SceneController, Groups use PowerController
		if (scene_type == 0) // Scene
		{
			// Add SceneController capability
			Json::Value scene_capability = CreateCapability("Alexa.SceneController");
			scene_capability["supportsDeactivation"] = false;
			if (!bControlPermitted)
				scene_capability["proactivelyReported"] = false;

			endpoint["capabilities"] = Json::Value(Json::arrayValue);
			endpoint["capabilities"].append(scene_capability);
		}
		else // Group
		{
			// Add PowerController capability
			Json::Value power_capability = CreateCapabilityWithProperties("Alexa.PowerController", "powerState", false, true, !bControlPermitted);

			endpoint["capabilities"] = Json::Value(Json::arrayValue);
			endpoint["capabilities"].append(power_capability);
		}

		// Add Alexa capability
		endpoint["capabilities"].append(CreateCapability("Alexa"));

		// Add cookie with metadata
		endpoint["cookie"]["WhatAmI"] = "scene";
		endpoint["cookie"]["SceneType"] = scene_type;

		root["event"]["payload"]["endpoints"].append(endpoint);
	}
}

void CWebServer::Alexa_HandleAcceptGrant(WebEmSession& session, const request& req, Json::Value& root)
{
	Json::Value request_json;
	if (!ParseJSon(req.content, request_json))
	{
		root["event"]["header"]["namespace"] = "Alexa";
		CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "Invalid JSON");
		return;
	}

	// Get authenticated username from session
	std::string username = session.username;
	_log.Log(LOG_STATUS, "Alexa: AcceptGrant for user '%s'", username.c_str());

	// Extract authorization code
	std::string auth_code = request_json["directive"]["payload"]["grant"]["code"].asString();

	if (!auth_code.empty())
	{
		// TODO: Get CLIENT_ID and CLIENT_SECRET from configuration
		std::string client_id = "";
		std::string client_secret = "";

		if (!client_id.empty() && !client_secret.empty())
		{
			// Exchange authorization code for tokens
			std::stringstream post_data;
			post_data << "grant_type=authorization_code"
				<< "&code=" << auth_code
				<< "&client_id=" << client_id
				<< "&client_secret=" << client_secret;

			std::vector<std::string> headers;
			headers.push_back("Content-Type: application/x-www-form-urlencoded");

			std::string response_data;
			std::vector<std::string> response_headers;

			// Make HTTPS POST to Amazon LWA
			if (HTTPClient::POST("https://api.amazon.com/auth/o2/token",
				post_data.str(), headers, response_data, response_headers))
			{
				// Parse token response
				Json::Value token_response;
				if (!ParseJSon(response_data, token_response))
				{
					std::string access_token = token_response["access_token"].asString();
					std::string refresh_token = token_response["refresh_token"].asString();
					int expires_in = token_response["expires_in"].asInt();

					_log.Log(LOG_STATUS, "Alexa: Received access token for user '%s' (expires in %d seconds)", username.c_str(), expires_in);
					_log.Log(LOG_STATUS, "Alexa: Received refresh token for user '%s'", username.c_str());

					// TODO: Store tokens in database for user '%s'
				}
			}
		}
	}

	// Always return success response (even if token exchange fails)
	root["event"]["header"]["namespace"] = "Alexa.Authorization";
	root["event"]["header"]["name"] = "AcceptGrant.Response";
	root["event"]["header"]["payloadVersion"] = "3";
	root["event"]["header"]["messageId"] = request_json["directive"]["header"]["messageId"].asString();
	root["event"]["payload"] = Json::Value(Json::objectValue);
}

static void Alexa_HandleControl_scene(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_namespace, const std::string& directive_name)
{
	// Check if scene is protected
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT Protected FROM Scenes WHERE (ID = %llu)", device_idx);
	if (result.empty() || atoi(result[0][0].c_str()) != 0)
	{
		CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Scene not found or protected");
		return;
	}

	// Check if this is a ReportState request
	if (directive_namespace == "Alexa" && directive_name == "ReportState")
	{
		// Query the scene/group info
		result = m_sql.safe_query("SELECT nValue, SceneType FROM Scenes WHERE (ID = %llu)", device_idx);
		if (result.empty())
		{
			CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Scene/Group not found");
			return;
		}

		// Only groups (not scenes) support state reporting
		int scene_type = atoi(result[0][1].c_str());
		if (scene_type == 0) // Scene
		{
			CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "ReportState not supported for scenes");
			return;
		}

		int nValue = atoi(result[0][0].c_str());
		std::string power_state = (nValue != 0) ? "ON" : "OFF";

		// Return StateReport
		root["event"]["header"]["name"] = "StateReport";

		// Add powerState to context
		root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", power_state));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Handle control directives
	if (directive_namespace == "Alexa.SceneController")
	{
		// SceneController: Activate (for Scenes only)
		if (directive_name == "Activate")
		{
			// Switch the scene
			std::string idx_str = std::to_string(device_idx);
			_log.Log(LOG_STATUS, "User: %s initiated a scene command via Alexa", session.username.c_str());
			if (!m_mainworker.SwitchScene(idx_str, "On", session.username))
			{
				root["event"]["header"]["namespace"] = "Alexa";
				CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Scene activation failed");
				return;
			}

			// Return ActivationStarted response
			root["event"]["header"]["namespace"] = "Alexa.SceneController";
			root["event"]["header"]["name"] = "ActivationStarted";
			root["event"]["payload"]["cause"]["type"] = "VOICE_INTERACTION";
			root["event"]["payload"]["timestamp"] = GetISO8601Timestamp();
			return;
		}
		else
		{
			CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "SceneController only supports Activate");
			return;
		}
	}
	else if (directive_namespace == "Alexa.PowerController")
	{
		// PowerController: TurnOn/TurnOff (for Groups)
		if (directive_name == "TurnOn" || directive_name == "TurnOff")
		{
			// Switch the group
			std::string idx_str = std::to_string(device_idx);
			std::string switchcmd = (directive_name == "TurnOn") ? "On" : "Off";
			_log.Log(LOG_STATUS, "User: %s initiated a group command via Alexa", session.username.c_str());
			if (!m_mainworker.SwitchScene(idx_str, switchcmd, session.username))
			{
				CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Group switch failed");
				return;
			}

			// Add powerState to context
			root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", (directive_name == "TurnOn") ? "ON" : "OFF"));
			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
		else
		{
			CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "PowerController only supports TurnOn/TurnOff");
			return;
		}
	}

	// Unsupported directive for scenes
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "Unsupported directive for scene/group");
}

static void Alexa_HandleControl_RangeController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "SetRangeValue")
	{
		int rangeValue = request_json["directive"]["payload"]["rangeValue"].asInt();
		_log.Log(LOG_STATUS, "User: %s set blind %llu to %d%% via Alexa", session.username.c_str(), device_idx, rangeValue);

		if (m_mainworker.SwitchLight(device_idx, "Set Level", rangeValue, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		root["context"]["properties"].append(CreateProperty("Alexa.RangeController", "rangeValue", rangeValue, "Blind.Lift"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}
	else if (directive_name == "AdjustRangeValue")
	{
		int rangeDelta = request_json["directive"]["payload"]["rangeValueDelta"].asInt();

		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Type, SubType, SwitchType, nValue, sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
		if (result.empty())
		{
			CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found");
			return;
		}

		unsigned char dType = atoi(result[0][0].c_str());
		unsigned char dSubType = atoi(result[0][1].c_str());
		_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
		unsigned char nValue = atoi(result[0][3].c_str());
		std::string sValue = result[0][4];

		std::string lstatus;
		int level;
		bool bHaveDimmer, bHaveGroupCmd;
		int maxDimLevel;
		GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, level, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

		int newLevel = level + rangeDelta;
		if (newLevel < 0) newLevel = 0;
		if (newLevel > 100) newLevel = 100;

		_log.Log(LOG_STATUS, "User: %s adjusted blind %llu by %d to %d%% via Alexa", session.username.c_str(), device_idx, rangeDelta, newLevel);

		if (m_mainworker.SwitchLight(device_idx, "Set Level", newLevel, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		root["context"]["properties"].append(CreateProperty("Alexa.RangeController", "rangeValue", newLevel, "Blind.Lift"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Unsupported directive for RangeController
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "RangeController only supports SetRangeValue/AdjustRangeValue");
}

static void Alexa_HandleControl_PowerController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "TurnOn" || directive_name == "TurnOff")
	{
		Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
		std::string whatAmI = cookie.get("WhatAmI", "").asString();

		if (whatAmI == "blind")
		{
			// PowerController for blinds - stop command
			_log.Log(LOG_STATUS, "User: %s stopped blind %llu via Alexa", session.username.c_str(), device_idx);

			if (m_mainworker.SwitchLight(device_idx, "Stop", 0, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
			{
				CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
				return;
			}

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Type, SubType, SwitchType, nValue, sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
			if (!result.empty())
			{
				unsigned char dType = atoi(result[0][0].c_str());
				unsigned char dSubType = atoi(result[0][1].c_str());
				_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
				unsigned char nValue = atoi(result[0][3].c_str());
				std::string sValue = result[0][4];

				std::string lstatus;
				int level;
				bool bHaveDimmer, bHaveGroupCmd;
				int maxDimLevel;
				GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, level, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

				root["context"]["properties"].append(CreateProperty("Alexa.RangeController", "rangeValue", level, "Blind.Lift"));
			}
			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
		else if (whatAmI == "switch")
		{
			// PowerController for simple switches
			std::string switchcmd = (directive_name == "TurnOn") ? "On" : "Off";
			_log.Log(LOG_STATUS, "User: %s set switch %llu to %s via Alexa", session.username.c_str(), device_idx, switchcmd.c_str());

			if (m_mainworker.SwitchLight(device_idx, switchcmd, 0, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
			{
				CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
				return;
			}

			root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", (directive_name == "TurnOn") ? "ON" : "OFF"));
			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
	}

	// Unsupported directive for PowerController
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "PowerController only supports TurnOn/TurnOff");
}

static void Alexa_HandleControl_BrightnessController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "SetBrightness")
	{
		int brightness = request_json["directive"]["payload"]["brightness"].asInt();
		_log.Log(LOG_STATUS, "User: %s set dimmer %llu to %d%% via Alexa", session.username.c_str(), device_idx, brightness);

		// Use "Set Level" for brightness changes (works better with color devices)
		// Use "Off" for zero brightness
		std::string switchcmd = (brightness > 0) ? "Set Level" : "Off";
		if (m_mainworker.SwitchLight(device_idx, switchcmd, brightness, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		root["context"]["properties"].append(CreateProperty("Alexa.BrightnessController", "brightness", brightness));
		root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", (brightness > 0) ? "ON" : "OFF"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}
	else if (directive_name == "AdjustBrightness")
	{
		int brightnessDelta = request_json["directive"]["payload"]["brightnessDelta"].asInt();

		// Query current brightness
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Type, SubType, SwitchType, nValue, sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
		if (result.empty())
		{
			CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found");
			return;
		}

		unsigned char dType = atoi(result[0][0].c_str());
		unsigned char dSubType = atoi(result[0][1].c_str());
		_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
		unsigned char nValue = atoi(result[0][3].c_str());
		std::string sValue = result[0][4];

		std::string lstatus;
		int level;
		bool bHaveDimmer, bHaveGroupCmd;
		int maxDimLevel;
		GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, level, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

		int new_brightness = level + brightnessDelta;
		if (new_brightness < 0) new_brightness = 0;
		if (new_brightness > 100) new_brightness = 100;

		_log.Log(LOG_STATUS, "User: %s adjusted dimmer %llu by %d%% to %d%% via Alexa", session.username.c_str(), device_idx, brightnessDelta, new_brightness);

		// Use "Set Level" for brightness changes, "Off" for zero
		std::string switchcmd = (new_brightness > 0) ? "Set Level" : "Off";
		if (m_mainworker.SwitchLight(device_idx, switchcmd, new_brightness, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		root["context"]["properties"].append(CreateProperty("Alexa.BrightnessController", "brightness", new_brightness));
		root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", (new_brightness > 0) ? "ON" : "OFF"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Unsupported directive for BrightnessController
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "BrightnessController only supports SetBrightness/AdjustBrightness");
}

static void Alexa_HandleControl_ColorController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "SetColor")
	{
		double hue = request_json["directive"]["payload"]["color"]["hue"].asDouble();
		double saturation = request_json["directive"]["payload"]["color"]["saturation"].asDouble();
		double color_brightness = request_json["directive"]["payload"]["color"]["brightness"].asDouble();

		// Query current device brightness level (not the color brightness)
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Type, SubType, SwitchType, nValue, sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
		if (result.empty())
		{
			CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found");
			return;
		}

		unsigned char dType = atoi(result[0][0].c_str());
		unsigned char dSubType = atoi(result[0][1].c_str());
		_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
		unsigned char nValue = atoi(result[0][3].c_str());
		std::string sValue = result[0][4];

		std::string lstatus;
		int current_level;
		bool bHaveDimmer, bHaveGroupCmd;
		int maxDimLevel;
		GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, current_level, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

		// Convert HSB to RGB (use color_brightness for the conversion, but preserve device level)
		int r, g, b;
		HSBtoRGB(hue, saturation, color_brightness, r, g, b);

		_log.Log(LOG_STATUS, "User: %s set color of device %llu to H:%.1f S:%.2f (RGB: %d,%d,%d) via Alexa, keeping brightness at %d%%",
			session.username.c_str(), device_idx, hue, saturation, r, g, b, current_level);

		// Set color - preserve current device brightness level
		_tColor color;
		color.mode = ColorModeRGB;
		color.r = r;
		color.g = g;
		color.b = b;

		if (m_mainworker.SwitchLight(device_idx, "Set Color", current_level, color, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		// Return color in response
		Json::Value color_value;
		color_value["hue"] = hue;
		color_value["saturation"] = saturation;
		color_value["brightness"] = color_brightness;

		root["context"]["properties"].append(CreateProperty("Alexa.ColorController", "color", color_value));
		root["context"]["properties"].append(CreateProperty("Alexa.BrightnessController", "brightness", current_level));
		root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", (current_level > 0) ? "ON" : "OFF"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Unsupported directive for ColorController
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "ColorController only supports SetColor");
}

static void Alexa_HandleControl_ColorTemperatureController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "SetColorTemperature")
	{
		int color_temp_kelvin = request_json["directive"]["payload"]["colorTemperatureInKelvin"].asInt();

		// Alexa supports 1000K to 10000K, but typical range is 2200K (warm) to 6500K (cool)
		// Domoticz uses 0-255 for cool white and warm white levels
		// Map Kelvin to CW/WW: lower K = more warm, higher K = more cool
		// 2200K = 100% WW, 0% CW
		// 6500K = 0% WW, 100% CW

		int ww = 0, cw = 0;
		if (color_temp_kelvin <= 2200)
		{
			ww = 255;
			cw = 0;
		}
		else if (color_temp_kelvin >= 6500)
		{
			ww = 0;
			cw = 255;
		}
		else
		{
			// Linear interpolation between 2200K and 6500K
			double ratio = (color_temp_kelvin - 2200) / (6500.0 - 2200.0);
			cw = (int)(ratio * 255);
			ww = 255 - cw;
		}

		_log.Log(LOG_STATUS, "User: %s set color temperature of device %llu to %dK (CW:%d WW:%d) via Alexa",
			session.username.c_str(), device_idx, color_temp_kelvin, cw, ww);

		// Query current brightness
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Type, SubType, SwitchType, nValue, sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
		if (result.empty())
		{
			CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found");
			return;
		}

		unsigned char dType = atoi(result[0][0].c_str());
		unsigned char dSubType = atoi(result[0][1].c_str());
		_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());
		unsigned char nValue = atoi(result[0][3].c_str());
		std::string sValue = result[0][4];

		std::string lstatus;
		int current_level;
		bool bHaveDimmer, bHaveGroupCmd;
		int maxDimLevel;
		GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, current_level, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

		// Set color temperature using white mode
		_tColor color;
		color.mode = ColorModeWhite;
		color.t = 0;  // Color temperature (not used in this mode)
		color.cw = cw;
		color.ww = ww;

		if (m_mainworker.SwitchLight(device_idx, "Set Color", current_level, color, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		root["context"]["properties"].append(CreateProperty("Alexa.ColorTemperatureController", "colorTemperatureInKelvin", color_temp_kelvin));
		root["context"]["properties"].append(CreateProperty("Alexa.BrightnessController", "brightness", current_level));
		root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", (current_level > 0) ? "ON" : "OFF"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}
	else if (directive_name == "DecreaseColorTemperature" || directive_name == "IncreaseColorTemperature")
	{
		// Query current color temperature from device
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Color FROM DeviceStatus WHERE (ID = %llu)", device_idx);
		if (result.empty() || result[0][0].empty())
		{
			CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found or no color data");
			return;
		}

		Json::Value color_json;
		if (!ParseJSon(result[0][0], color_json))
		{
			CreateErrorResponse(root, request_json, "INTERNAL_ERROR", "Failed to parse color data");
			return;
		}

		int cw = color_json.get("cw", 0).asInt();
		int ww = color_json.get("ww", 0).asInt();

		// Convert CW/WW back to Kelvin
		int current_kelvin;
		if (cw == 0 && ww == 0)
		{
			current_kelvin = 4000; // Default middle value
		}
		else
		{
			double ratio = cw / 255.0;
			current_kelvin = (int)(2200 + ratio * (6500 - 2200));
		}

		// Adjust by 500K
		int new_kelvin = current_kelvin + ((directive_name == "IncreaseColorTemperature") ? 500 : -500);
		if (new_kelvin < 2200) new_kelvin = 2200;
		if (new_kelvin > 6500) new_kelvin = 6500;

		// Recursively call SetColorTemperature
		Json::Value new_request = request_json;
		new_request["directive"]["header"]["name"] = "SetColorTemperature";
		new_request["directive"]["payload"]["colorTemperatureInKelvin"] = new_kelvin;
		Alexa_HandleControl_ColorTemperatureController(session, new_request, root, device_idx, "SetColorTemperature");
		return;
	}

	// Unsupported directive for ColorTemperatureController
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "ColorTemperatureController only supports SetColorTemperature/IncreaseColorTemperature/DecreaseColorTemperature");
}

// Helper to report thermostat state (setpoint, mode, optional humidity)
static void ReportThermostatState(Json::Value& root, const Json::Value& cookie, uint64_t setpoint_idx, bool is_evohome_zone, const std::string& setpoint_sValue = "")
{
	// Query and report setpoint
	std::string sValue = setpoint_sValue;
	if (sValue.empty())
	{
		std::vector<std::vector<std::string>> setpoint_result;
		setpoint_result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID = %llu)", setpoint_idx);
		if (!setpoint_result.empty())
		{
			sValue = setpoint_result[0][0];
		}
	}

	if (!sValue.empty())
	{
		double setpoint = atof(sValue.c_str());
		Json::Value setpoint_value;
		setpoint_value["value"] = setpoint;
		setpoint_value["scale"] = "CELSIUS";
		root["context"]["properties"].append(CreateProperty("Alexa.ThermostatController", "targetSetpoint", setpoint_value));
	}

	// Query and report mode (not for Evohome zones)
	if (!is_evohome_zone)
	{
		std::string mode = "HEAT"; // Default
		std::string mode_idx_str = cookie.get("modeIdx", "").asString();
		if (!mode_idx_str.empty())
		{
			uint64_t mode_idx = std::stoull(mode_idx_str);
			std::vector<std::vector<std::string>> mode_result;
			mode_result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (ID = %llu)", mode_idx);
			if (!mode_result.empty())
			{
				int level = atoi(mode_result[0][0].c_str());
				mode = (level == 0) ? "OFF" : "HEAT";
			}
		}
		root["context"]["properties"].append(CreateProperty("Alexa.ThermostatController", "thermostatMode", mode));
	}
}

static void Alexa_HandleControl_ThermostatController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
	std::string what_am_i = cookie.get("WhatAmI", "").asString();
	bool is_evohome_zone = (what_am_i == "evohome_zone");

	if (directive_name == "SetTargetTemperature")
	{
		double target_temp = request_json["directive"]["payload"]["targetSetpoint"]["value"].asDouble();

		if (is_evohome_zone)
		{
			// Use SetSetPointEvo for Evohome zones
			std::string idx_str = std::to_string(device_idx);
			m_mainworker.SetSetPointEvo(idx_str, static_cast<float>(target_temp), "", "");

			// For Evohome, just report the new setpoint (no mode)
			Json::Value temp_value;
			temp_value["value"] = target_temp;
			temp_value["scale"] = "CELSIUS";
			root["context"]["properties"].append(CreateProperty("Alexa.ThermostatController", "targetSetpoint", temp_value));
		}
		else
		{
			// Use regular SetSetPoint for multi-device thermostats
			std::string setpoint_idx_str = cookie.get("setpointIdx", "").asString();
			uint64_t setpoint_idx = std::stoull(setpoint_idx_str);
			m_mainworker.SetSetPoint(setpoint_idx_str, static_cast<float>(target_temp));

			// Report updated state (pass known setpoint to avoid redundant query)
			ReportThermostatState(root, cookie, setpoint_idx, is_evohome_zone, std::to_string(target_temp));
		}

		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	if (directive_name == "AdjustTargetTemperature")
	{
		double delta = request_json["directive"]["payload"]["targetSetpointDelta"]["value"].asDouble();

		// Query current setpoint
		double current_temp;
		if (is_evohome_zone)
		{
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
			if (result.empty())
			{
				CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Zone not found");
				return;
			}
			// Parse sValue: "temp;setpoint;status;until"
			std::vector<std::string> values;
			StringSplit(result[0][0], ";", values);
			if (values.size() < 2)
			{
				CreateErrorResponse(root, request_json, "INTERNAL_ERROR", "Invalid zone data");
				return;
			}
			current_temp = atof(values[1].c_str());
		}
		else
		{
			std::string setpoint_idx_str = cookie.get("setpointIdx", "").asString();
			uint64_t setpoint_idx = std::stoull(setpoint_idx_str);
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID = %llu)", setpoint_idx);
			if (result.empty())
			{
				CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Setpoint device not found");
				return;
			}
			current_temp = atof(result[0][0].c_str());
		}

		double new_temp = current_temp + delta;

		// Send setpoint command
		if (is_evohome_zone)
		{
			std::string idx_str = std::to_string(device_idx);
			m_mainworker.SetSetPointEvo(idx_str, static_cast<float>(new_temp), "", "");

			// For Evohome, just report the new setpoint
			Json::Value temp_value;
			temp_value["value"] = new_temp;
			temp_value["scale"] = "CELSIUS";
			root["context"]["properties"].append(CreateProperty("Alexa.ThermostatController", "targetSetpoint", temp_value));
		}
		else
		{
			std::string setpoint_idx_str = cookie.get("setpointIdx", "").asString();
			uint64_t setpoint_idx = std::stoull(setpoint_idx_str);
			m_mainworker.SetSetPoint(setpoint_idx_str, static_cast<float>(new_temp));

			// Report updated state (pass known setpoint to avoid redundant query)
			ReportThermostatState(root, cookie, setpoint_idx, is_evohome_zone, std::to_string(new_temp));
		}

		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	if (directive_name == "SetThermostatMode")
	{
		std::string mode = request_json["directive"]["payload"]["thermostatMode"]["value"].asString();

		if (is_evohome_zone)
		{
			CreateErrorResponse(root, request_json, "NOT_SUPPORTED_IN_CURRENT_MODE", "Evohome zones do not support mode control");
			return;
		}

		// Multi-device thermostat mode control
		std::string mode_idx_str = cookie.get("modeIdx", "").asString();

		if (mode_idx_str.empty())
		{
			CreateErrorResponse(root, request_json, "NOT_SUPPORTED_IN_CURRENT_MODE", "Thermostat does not have mode control");
			return;
		}

		// Map Alexa mode to selector level (0=OFF, 10=HEAT)
		int level = (mode == "OFF") ? 0 : 10;

		// Set the mode selector level
		uint64_t mode_idx = std::stoull(mode_idx_str);
		std::string switchcmd = (level == 0) ? "Off" : "Set Level";

		_log.Log(LOG_STATUS, "User: %s setting thermostat mode selector %llu to level %d (mode=%s, cmd=%s) via Alexa",
			session.username.c_str(), mode_idx, level, mode.c_str(), switchcmd.c_str());

		if (m_mainworker.SwitchLight(mode_idx, switchcmd, level, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Failed to set thermostat mode");
			return;
		}

		// Report updated state
		std::string setpoint_idx_str = cookie.get("setpointIdx", "").asString();
		uint64_t setpoint_idx = std::stoull(setpoint_idx_str);
		ReportThermostatState(root, cookie, setpoint_idx, false);

		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Unsupported directive
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "ThermostatController only supports SetTargetTemperature/AdjustTargetTemperature/SetThermostatMode");
}

static void Alexa_HandleControl_ModeController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "SetMode")
	{
		Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
		std::string endpoint_id = request_json["directive"]["endpoint"]["endpointId"].asString();
		std::string mode_value = request_json["directive"]["payload"]["mode"].asString();

		// Parse level from "Level.X" format
		int level = 0;
		if (mode_value.find("Level.") == 0)
		{
			level = atoi(mode_value.substr(6).c_str());
		}
		else
		{
			level = atoi(mode_value.c_str());
		}

		// Switch the selector to the specified level
		_log.Log(LOG_STATUS, "User: %s set selector switch %llu to level %d via Alexa", session.username.c_str(), device_idx, level);

		if (m_mainworker.SwitchLight(device_idx, "Set Level", level, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		// Build instance name from device name in cookie
		std::string instance_name = BuildInstanceName(cookie, endpoint_id, ".Mode");

		// Add mode to context
		root["context"]["properties"].append(CreateProperty("Alexa.ModeController", "mode", mode_value, instance_name));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Unsupported directive for ModeController
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "ModeController only supports SetMode");
}

static void Alexa_HandleControl_LockController(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx, const std::string& directive_name)
{
	if (directive_name == "Lock" || directive_name == "Unlock")
	{
		Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
		int switch_type = cookie["switchtype"].isInt() ? cookie["switchtype"].asInt() : atoi(cookie["switchtype"].asString().c_str());

		// For normal locks: Lock = On, Unlock = Off
		// For inverted locks: Lock = Off, Unlock = On
		bool is_inverted = (switch_type == STYPE_DoorLockInverted);
		std::string switchcmd = (directive_name == "Lock") ? (is_inverted ? "Off" : "On") : (is_inverted ? "On" : "Off");

		_log.Log(LOG_STATUS, "User: %s set lock %llu to %s via Alexa", session.username.c_str(), device_idx, directive_name.c_str());

		if (m_mainworker.SwitchLight(device_idx, switchcmd, 0, NoColor, false, 0, session.username) == MainWorker::SL_ERROR)
		{
			CreateErrorResponse(root, request_json, "ENDPOINT_UNREACHABLE", "Unable to control device - hardware communication error");
			return;
		}

		std::string lock_state = (directive_name == "Lock") ? "LOCKED" : "UNLOCKED";
		root["context"]["properties"].append(CreateProperty("Alexa.LockController", "lockState", lock_state));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "LockController only supports Lock/Unlock");
}

static void Alexa_HandleControl_ReportState(WebEmSession& session, const Json::Value& request_json, Json::Value& root, uint64_t device_idx)
{
	Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
	std::string endpoint_id = request_json["directive"]["endpoint"]["endpointId"].asString();

	// Query device state
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT Type, SubType, SwitchType, nValue, sValue FROM DeviceStatus WHERE (ID = %llu)", device_idx);
	if (result.empty())
	{
		CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found");
		return;
	}

	unsigned char dType = atoi(result[0][0].c_str());
	unsigned char dSubType = atoi(result[0][1].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(result[0][2].c_str());

	// Begin creating a StateReport
	root["event"]["header"]["name"] = "StateReport";

	// First, switches...
	if (IsLightOrSwitch(dType, dSubType)
	    || ((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXStatus)))
	{
		unsigned char nValue = atoi(result[0][3].c_str());
		std::string sValue = result[0][4];

		std::string lstatus;
		int level;
		bool bHaveDimmer, bHaveGroupCmd;
		int maxDimLevel;
		GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, level, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

		if (switchtype == STYPE_Selector)
		{
			// Build instance name from device name in cookie
			std::string instance_name = BuildInstanceName(cookie, endpoint_id, ".Mode");

			// Add mode to context with Level.X format
			root["context"]["properties"].append(CreateProperty("Alexa.ModeController", "mode", "Level." + std::to_string(level), instance_name));
			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
		else if (switchtype == STYPE_Blinds || switchtype == STYPE_BlindsPercentage ||
			 switchtype == STYPE_BlindsPercentageWithStop || switchtype == STYPE_BlindsWithStop)
		{
			root["context"]["properties"].append(CreateProperty("Alexa.RangeController", "rangeValue", level, "Blind.Lift"));
			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
		else if (switchtype == STYPE_DoorLock || switchtype == STYPE_DoorLockInverted)
		{
			// Door locks
			bool is_inverted = (switchtype == STYPE_DoorLockInverted);
			// For normal locks: On = Locked, Off = Unlocked
			// For inverted locks: Off = Locked, On = Unlocked
			bool is_locked = is_inverted ? (lstatus == "Off") : (lstatus != "Off");
			std::string lock_state = is_locked ? "LOCKED" : "UNLOCKED";
			root["context"]["properties"].append(CreateProperty("Alexa.LockController", "lockState", lock_state));
			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
		else if (switchtype == STYPE_OnOff || switchtype == STYPE_Dimmer)
		{
			// Simple On/Off or Dimmer switch
			std::string power_state = (lstatus == "Off") ? "OFF" : "ON";
			root["context"]["properties"].append(CreateProperty("Alexa.PowerController", "powerState", power_state));

			// Add brightness for dimmers
			if (switchtype == STYPE_Dimmer)
			{
				root["context"]["properties"].append(CreateProperty("Alexa.BrightnessController", "brightness", level));

				// Add color for RGB devices
				if (dType == pTypeColorSwitch)
				{
					// Query color from database
					std::vector<std::vector<std::string>> color_result;
					color_result = m_sql.safe_query("SELECT Color FROM DeviceStatus WHERE (ID = %llu)", device_idx);
					if (!color_result.empty() && !color_result[0][0].empty())
					{
						Json::Value color_json;
						if (!ParseJSon(color_result[0][0], color_json))
						{
							int r = color_json.get("r", 0).asInt();
							int g = color_json.get("g", 0).asInt();
							int b = color_json.get("b", 0).asInt();

							// Convert RGB to HSB
							double hue, saturation, brightness;
							RGBtoHSB(r, g, b, hue, saturation, brightness);

							Json::Value color_value;
							color_value["hue"] = hue;
							color_value["saturation"] = saturation;
							color_value["brightness"] = brightness;

							root["context"]["properties"].append(CreateProperty("Alexa.ColorController", "color", color_value));

							// Add color temperature for RGBWW/RGBWWZ devices
							if (dSubType == sTypeColor_RGB_CW_WW || dSubType == sTypeColor_RGB_CW_WW_Z)
							{
								int cw = color_json.get("cw", 0).asInt();
								int ww = color_json.get("ww", 0).asInt();

								// Convert CW/WW to Kelvin
								int color_temp_kelvin;
								if (cw == 0 && ww == 0)
								{
									color_temp_kelvin = 4000; // Default
								}
								else
								{
									double ratio = cw / 255.0;
									color_temp_kelvin = (int)(2200 + ratio * (6500 - 2200));
								}

								root["context"]["properties"].append(CreateProperty("Alexa.ColorTemperatureController", "colorTemperatureInKelvin", color_temp_kelvin));
							}
						}
					}
				}
			}

			root["context"]["properties"].append(CreateEndpointHealthProperty());
			return;
		}
	}

	// Handle temperature and environmental sensors
	if (dType == pTypeTEMP || dType == pTypeTEMP_HUM || dType == pTypeHUM || dType == pTypeTEMP_HUM_BARO || dType == pTypeTEMP_BARO)
	{
		root["event"]["header"]["name"] = "StateReport";

		// Parse temperature/humidity from sValue (already queried above)
		std::string sValue = result[0][4];
		std::vector<std::string> values;
		StringSplit(sValue, ";", values);

		if (dType == pTypeTEMP || dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO || dType == pTypeTEMP_BARO)
		{
			if (!values.empty())
			{
				double temp = atof(values[0].c_str());
				Json::Value temp_value;
				temp_value["value"] = temp;
				temp_value["scale"] = "CELSIUS";
				root["context"]["properties"].append(CreateProperty("Alexa.TemperatureSensor", "temperature", temp_value));
			}
		}

		if (dType == pTypeTEMP_HUM || dType == pTypeTEMP_HUM_BARO)
		{
			if (values.size() > 1)
			{
				int humidity = atoi(values[1].c_str());
				root["context"]["properties"].append(CreateProperty("Alexa.HumiditySensor", "relativeHumidity", humidity));
			}
		}

		if (dType == pTypeHUM)
		{
			// For humidity-only sensors, humidity is stored in nValue
			unsigned char nValue = atoi(result[0][3].c_str());
			root["context"]["properties"].append(CreateProperty("Alexa.HumiditySensor", "relativeHumidity", (int)nValue));
		}

		// Check if this is a thermostat (has setpoint)
		Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
		std::string setpoint_idx_str = cookie.get("setpointIdx", "").asString();
		if (!setpoint_idx_str.empty())
		{
			uint64_t setpoint_idx = std::stoull(setpoint_idx_str);
			ReportThermostatState(root, cookie, setpoint_idx, false);

			// Query humidity if linked
			std::string humidity_idx_str = cookie.get("humidityIdx", "").asString();
			if (!humidity_idx_str.empty())
			{
				uint64_t humidity_idx = std::stoull(humidity_idx_str);
				std::vector<std::vector<std::string>> hum_result;
				hum_result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (ID = %llu)", humidity_idx);
				if (!hum_result.empty())
				{
					int humidity = atoi(hum_result[0][0].c_str());
					root["context"]["properties"].append(CreateProperty("Alexa.HumiditySensor", "relativeHumidity", humidity));
				}
			}
		}

		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Handle standalone setpoint thermostats
	if (dType == pTypeSetpoint)
	{
		root["event"]["header"]["name"] = "StateReport";

		// Use helper to report thermostat state (pass sValue to avoid redundant query)
		std::string sValue = result[0][4];
		Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];
		ReportThermostatState(root, cookie, device_idx, false, sValue);

		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Handle Evohome zones
	if (dType == pTypeEvohomeZone)
	{
		root["event"]["header"]["name"] = "StateReport";

		// Parse zone data from sValue: "temp;setpoint;status;until"
		std::string sValue = result[0][4];
		std::vector<std::string> values;
		StringSplit(sValue, ";", values);

		if (values.size() >= 2)
		{
			// Report current temperature
			double temp = atof(values[0].c_str());
			Json::Value temp_value;
			temp_value["value"] = temp;
			temp_value["scale"] = "CELSIUS";
			root["context"]["properties"].append(CreateProperty("Alexa.TemperatureSensor", "temperature", temp_value));

			// Report target setpoint
			double setpoint = atof(values[1].c_str());
			Json::Value setpoint_value;
			setpoint_value["value"] = setpoint;
			setpoint_value["scale"] = "CELSIUS";
			root["context"]["properties"].append(CreateProperty("Alexa.ThermostatController", "targetSetpoint", setpoint_value));
		}

		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Handle Weight sensors
	if (dType == pTypeWEIGHT)
	{
		root["event"]["header"]["name"] = "StateReport";

		// Parse weight value from sValue (in kg)
		std::string sValue = result[0][4];
		double weight = atof(sValue.c_str());

		root["context"]["properties"].append(CreateProperty("Alexa.RangeController", "rangeValue", weight, "Weight.Weight"));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Handle General sensors
	if (dType == pTypeGeneral)
	{
		root["event"]["header"]["name"] = "StateReport";

		// Parse sensor value and unit from sValue
		std::string sValue = result[0][4];
		double value = atof(sValue.c_str());

		// Parse unit from sValue (format: "value unit")
		std::string unit;
		size_t space_pos = sValue.find(' ');
		if (space_pos != std::string::npos && space_pos + 1 < sValue.length())
		{
			unit = sValue.substr(space_pos + 1);
		}

		// Get semantic instance name
		std::string instance_name = GetSensorInstanceName(unit);

		root["context"]["properties"].append(CreateProperty("Alexa.RangeController", "rangeValue", value, instance_name));
		root["context"]["properties"].append(CreateEndpointHealthProperty());
		return;
	}

	// Unsupported device type for ReportState
	CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "ReportState not supported for this device type");
}

bool CWebServer::CheckDeviceAccess(const WebEmSession& session, uint64_t device_idx, bool& bControlPermitted)
{
	bControlPermitted = true;

	switch (session.rights)
	{
	case URIGHTS_ADMIN:
		// Admin can access anything
		return true;

	case URIGHTS_VIEWER:
		bControlPermitted = false;
		[[fallthrough]];

	case URIGHTS_SWITCHER:
	{
		// Find user and check device permissions
		int iUser = FindUser(session.username.c_str());
		if ((iUser < 0) || (iUser >= (int)m_users.size()))
			return false;

		// If TotSensors is 0, user has access to all devices
		if (m_users[iUser].TotSensors == 0)
			return true;

		// Check if device is in user's shared devices
		std::vector<std::vector<std::string>> result =
			m_sql.safe_query("SELECT COUNT(*) FROM SharedDevices WHERE (SharedUserID == '%d') AND (DeviceRowID == '%llu')", m_users[iUser].ID, device_idx);
		return (!result.empty() && atoi(result[0][0].c_str()) > 0);
	}

	default:
		// Invalid user rights
		return false;
	}
}

// Check access to multiple devices at once (for cookie-based device references)
bool CWebServer::CheckDeviceAccess(const WebEmSession& session, const std::vector<uint64_t>& device_indices, bool& bControlPermitted)
{
	bControlPermitted = true;

	if (device_indices.empty())
		return true;

	switch (session.rights)
	{
	case URIGHTS_ADMIN:
		// Admin can access anything
		return true;

	case URIGHTS_VIEWER:
		bControlPermitted = false;
		[[fallthrough]];

	case URIGHTS_SWITCHER:
	{
		// Find user and check device permissions
		int iUser = FindUser(session.username.c_str());
		if ((iUser < 0) || (iUser >= (int)m_users.size()))
			return false;

		// If TotSensors is 0, user has access to all devices
		if (m_users[iUser].TotSensors == 0)
			return true;

		// Build set of unique device IDs
		std::set<uint64_t> unique_devices(device_indices.begin(), device_indices.end());

		// Build IN clause for SQL query
		std::stringstream ss;
		ss << "SELECT COUNT(*) FROM SharedDevices WHERE (SharedUserID == '" << m_users[iUser].ID << "') AND DeviceRowID IN (";
		bool first = true;
		for (uint64_t device_id : unique_devices)
		{
			if (!first) ss << ",";
			ss << device_id;
			first = false;
		}
		ss << ")";

		std::vector<std::vector<std::string>> result = m_sql.safe_query(ss.str().c_str());
		// User must have access to ALL unique devices
		return (!result.empty() && atoi(result[0][0].c_str()) == (int)unique_devices.size());
	}

	default:
		// Invalid user rights
		return false;
	}
}

// Extract all device indices from cookie fields ending in "Idx" and validate access


void CWebServer::Alexa_HandleControl(WebEmSession& session, const request& req, Json::Value& root)
{
	Json::Value request_json;
	if (!ParseJSon(req.content, request_json))
	{
		root["event"]["header"]["namespace"] = "Alexa";
		CreateErrorResponse(root, request_json, "INVALID_DIRECTIVE", "Invalid JSON");
		return;
	}

	std::string directive_namespace = request_json["directive"]["header"]["namespace"].asString();
	std::string directive_name = request_json["directive"]["header"]["name"].asString();
	std::string endpoint_id = request_json["directive"]["endpoint"]["endpointId"].asString();
	Json::Value cookie = request_json["directive"]["endpoint"]["cookie"];

	// Build base response structure (can be modified by handlers)
	root["event"]["header"]["namespace"] = "Alexa";
	root["event"]["header"]["name"] = "Response";
	root["event"]["header"]["payloadVersion"] = "3";
	root["event"]["header"]["messageId"] = request_json["directive"]["header"]["messageId"].asString();
	root["event"]["endpoint"]["endpointId"] = endpoint_id;
	root["event"]["payload"] = Json::Value(Json::objectValue);
	root["context"]["properties"] = Json::Value(Json::arrayValue);

	// Parse endpoint ID into prefix and index
	std::string prefix;
	std::string device_idx_str;
	size_t underscore_pos = endpoint_id.rfind('_');
	if (underscore_pos != std::string::npos)
	{
		prefix = endpoint_id.substr(0, underscore_pos + 1);
		device_idx_str = endpoint_id.substr(underscore_pos + 1);
	}
	else
	{
		device_idx_str = endpoint_id;
	}

	uint64_t device_idx = std::stoull(device_idx_str);

	// Handle scenes/groups
	if (prefix == "scene_")
	{
		Alexa_HandleControl_scene(session, request_json, root, device_idx, directive_namespace, directive_name);
		return;
	}

	// Check access control for main device and all cookie-referenced devices
	std::vector<uint64_t> device_indices = {device_idx};
	for (const auto& key : cookie.getMemberNames())
	{
		if (key.length() > 3 && key.substr(key.length() - 3) == "Idx")
		{
			std::string idx_str = cookie[key].asString();
			if (!idx_str.empty())
			{
				device_indices.push_back(std::stoull(idx_str));
			}
		}
	}

	bool bControlPermitted;
	if (!CheckDeviceAccess(session, device_indices, bControlPermitted))
	{
		CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device not found or access denied");
		return;
	}

	// ReportState is allowed for read-only users
	if (directive_namespace == "Alexa" && directive_name == "ReportState")
	{
		Alexa_HandleControl_ReportState(session, request_json, root, device_idx);
		return;
	}

	// Control operations require write permission
	if (!bControlPermitted)
	{
		CreateErrorResponse(root, request_json, "NO_SUCH_ENDPOINT", "Device control not permitted");
		return;
	}

	// Dispatch to controller-specific handlers
	typedef void (*ControllerHandler)(WebEmSession&, const Json::Value&, Json::Value&, uint64_t, const std::string&);

	static const std::map<std::string, ControllerHandler> controller_handlers = {
		{"Alexa.RangeController", Alexa_HandleControl_RangeController},
		{"Alexa.PowerController", Alexa_HandleControl_PowerController},
		{"Alexa.BrightnessController", Alexa_HandleControl_BrightnessController},
		{"Alexa.ColorController", Alexa_HandleControl_ColorController},
		{"Alexa.ColorTemperatureController", Alexa_HandleControl_ColorTemperatureController},
		{"Alexa.ThermostatController", Alexa_HandleControl_ThermostatController},
		{"Alexa.ModeController", Alexa_HandleControl_ModeController},
		{"Alexa.LockController", Alexa_HandleControl_LockController}
	};

	// Look up handler in dispatch map
	auto it = controller_handlers.find(directive_namespace);
	if (it != controller_handlers.end())
	{
		it->second(session, request_json, root, device_idx, directive_name);
		return;
	}

	// TODO: Implement other control handlers
	_log.Log(LOG_ERROR, "Alexa: Unimplemented control: %s/%s", directive_namespace.c_str(), directive_name.c_str());
	CreateErrorResponse(root, request_json, "INTERNAL_ERROR", "Control not yet implemented: " + directive_namespace + "/" + directive_name);
}

void CWebServer::GetAlexaPage(WebEmSession& session, const request& req, reply& rep)
{
	Json::Value root;

	// Parse request body for Alexa directive
	Json::Value request_json;
	if (!ParseJSon(req.content, request_json))
	{
		rep.status = http::server::reply::bad_request;
		return;
	}

	// Get directive namespace
	std::string directive_namespace = request_json["directive"]["header"]["namespace"].asString();

	// Route to appropriate handler
	if (directive_namespace == "Alexa.Discovery")
	{
		Alexa_HandleDiscovery(session, req, root);
	}
	else if (directive_namespace == "Alexa.Authorization")
	{
		Alexa_HandleAcceptGrant(session, req, root);
	}
	else if (directive_namespace.find("Alexa") == 0)
	{
		Alexa_HandleControl(session, req, root);
	}
	else
	{
		rep.status = http::server::reply::not_found;
		return;
	}

	reply::set_content(&rep, root.toStyledString());
	rep.status = http::server::reply::ok;
}

} // namespace server
} // namespace http
