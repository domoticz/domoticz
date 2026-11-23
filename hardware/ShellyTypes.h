#pragma once

// Shared data model for the native Shelly integration.
// Generation agnostic: used by device identification, the Gen1/Gen2 clients and the driver.

#include <string>
#include <utility>
#include <json/json.h>

enum ShellyGeneration
{
	SHELLY_GEN_UNKNOWN = 0,
	SHELLY_GEN1 = 1, // Block devices, REST + CoIoT, basic auth
	SHELLY_GEN2 = 2, // RPC devices (Plus/Pro), digest auth
	SHELLY_GEN3 = 3, // RPC devices, same protocol as Gen2
	SHELLY_GEN4 = 4  // RPC devices, same protocol as Gen2
};

// True for every generation that speaks the JSON-RPC protocol.
inline bool ShellyIsRpcGeneration(const int gen)
{
	return gen >= SHELLY_GEN2;
}

// Outcome of a request to a device. Lets callers tell apart unreachable (httpStatus 0), unauthorized (401),
// rejected (another HTTP status or an RPC error) and unparseable answers through `error`.
struct ShellyResult
{
	bool ok = false;
	int httpStatus = 0;           // 0 when no HTTP answer was received (never 200 for a non-HTTP failure)
	bool transport = false;       // true only for a result produced by the HTTP layer (ShellyHttpResponse::ToResult), i.e.
	                              // a real exchange with the device was attempted; false for local rejections and RPC errors
	std::string error;            // empty when ok
	Json::Value data;             // parsed response body (null when there was none or it is not JSON)
	std::string body;             // raw response body, kept for failures too (RPC errors travel in the body)
	std::string wwwAuthenticate;  // WWW-Authenticate challenge of a 401 answer, empty otherwise

	explicit operator bool() const
	{
		return ok;
	}
	static ShellyResult Success(const int httpStatus = 200)
	{
		ShellyResult r;
		r.ok = true;
		r.httpStatus = httpStatus;
		return r;
	}
	static ShellyResult Success(const int httpStatus, Json::Value data)
	{
		ShellyResult r = Success(httpStatus);
		r.data = std::move(data);
		return r;
	}
	static ShellyResult Failure(const int httpStatus, std::string error)
	{
		ShellyResult r;
		r.httpStatus = httpStatus;
		r.error = std::move(error);
		return r;
	}
	bool IsUnauthorized() const
	{
		return httpStatus == 401;
	}
	// True when a failed request counts against a node's availability: a real HTTP exchange (`transport`) that got no
	// answer at all (httpStatus 0) or a server error (5xx). Bad credentials (401), other 4xx answers, RPC application
	// errors (whatever their code) and local rejections ("unsupported command", "invalid parameter", "stopped") never do.
	bool IsReachabilityFailure() const
	{
		return !ok && transport && (httpStatus == 0 || httpStatus >= 500);
	}
};

// Result of probing GET /shelly on a device.
struct ShellyIdentity
{
	int gen = SHELLY_GEN_UNKNOWN;
	std::string mac;      // normalized: upper case hex, no separators
	std::string deviceId; // Gen2+: "shellyplus1-aabbcc"; Gen1: empty (not reported by /shelly)
	std::string model;    // Gen2+: "SNSW-001X16EU"; Gen1: "SHSW-25"
	std::string name;     // user assigned device name when reported, may be empty
	std::string fw;       // firmware version / id as reported
	bool authRequired = false;

	bool IsValid() const
	{
		return gen != SHELLY_GEN_UNKNOWN && !mac.empty();
	}
};

// One Shelly device managed by a hardware instance.
// `id` is the normalized MAC and is the stable key used to build Domoticz DeviceIDs.
struct ShellyNode
{
	std::string id;
	std::string deviceId;
	std::string ip;   // IPv4/IPv6 literal or host name (see ShellyIsValidHost); use ShellyBaseUrl() to build URLs
	int port = 80;
	int gen = SHELLY_GEN_UNKNOWN;
	std::string model;
	std::string name;
	std::string fw;
	bool auth = false;
	bool sleepy = false;      // battery powered device that is only reachable while awake
	int sleepPeriodSec = 0;   // expected wake-up period of a sleepy device, 0 when unknown
};

// Credentials shared by all nodes of one hardware instance.
struct ShellyCredentials
{
	std::string username; // Gen1 basic auth user; Gen2+ digest user is always "admin"
	std::string password;
};

// Component index range accepted by both generations (0..SHELLY_MAX_CHANNEL).
constexpr int SHELLY_MAX_CHANNEL = 15;
inline bool ShellyIsValidChannel(const int channel)
{
	return channel >= 0 && channel <= SHELLY_MAX_CHANNEL;
}

// A control request in generation agnostic form. The Gen1 and Gen2 clients translate it; both use the
// constants below for every string, so a command built by the driver works on either generation.
//
//   component   action                params (key: value)
//   ---------   -------------------   -------------------------------------------------
//   switch      on, off, toggle       none
//   light       on, off, toggle       brightness (0-100) optional with "on"
//   light       set_brightness        brightness: 0-100
//   rgb, rgbw   on, off, toggle       none
//   rgb, rgbw   set_color             red, green, blue: 0-255; white: 0-255 (rgbw);
//                                     brightness: 0-100 optional
//   cover       open, close, stop     none
//   cover       set_position          position: 0-100 (100 is fully open)
//   roller      open, close, stop     none (Gen1 name of cover; Gen2 translators treat it as cover)
//   roller      set_position          position: 0-100 (100 is fully open)
//   thermostat  set_temperature       temperature: target in degrees Celsius
//
// `channel` is the component index (switch:0, relay 0, ...). A client reports an unsupported component/action
// combination as ShellyResult::Failure(0, "unsupported command").
struct ShellyCommand
{
	std::string component;
	int channel = 0;
	std::string action;
	Json::Value params;    // see the table above; null when the action has no parameters
};

namespace ShellyCmd
{
	// components
	constexpr const char* COMPONENT_SWITCH = "switch";
	constexpr const char* COMPONENT_LIGHT = "light";
	constexpr const char* COMPONENT_COVER = "cover";
	constexpr const char* COMPONENT_ROLLER = "roller";
	constexpr const char* COMPONENT_RGB = "rgb";
	constexpr const char* COMPONENT_RGBW = "rgbw";
	constexpr const char* COMPONENT_THERMOSTAT = "thermostat";

	// actions
	constexpr const char* ACTION_ON = "on";
	constexpr const char* ACTION_OFF = "off";
	constexpr const char* ACTION_TOGGLE = "toggle";
	constexpr const char* ACTION_SET_BRIGHTNESS = "set_brightness";
	constexpr const char* ACTION_SET_POSITION = "set_position";
	constexpr const char* ACTION_OPEN = "open";
	constexpr const char* ACTION_CLOSE = "close";
	constexpr const char* ACTION_STOP = "stop";
	constexpr const char* ACTION_SET_COLOR = "set_color";
	constexpr const char* ACTION_SET_TEMPERATURE = "set_temperature";

	// param keys
	constexpr const char* PARAM_BRIGHTNESS = "brightness"; // 0-100
	constexpr const char* PARAM_POSITION = "position";     // 0-100
	constexpr const char* PARAM_RED = "red";               // 0-255
	constexpr const char* PARAM_GREEN = "green";           // 0-255
	constexpr const char* PARAM_BLUE = "blue";             // 0-255
	constexpr const char* PARAM_WHITE = "white";           // 0-255
	constexpr const char* PARAM_TEMPERATURE = "temperature"; // degrees Celsius
} // namespace ShellyCmd
