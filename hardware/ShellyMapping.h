#pragma once

// Mapping of Shelly status documents to abstract Domoticz device updates, and the device identity scheme.
//
// Everything here is a pure function (no database, no logging, no I/O) so it can be tested on its own. The driver
// (Shelly.cpp) turns the abstract updates into Domoticz devices.
//
// Device identity
// ---------------
// Every Domoticz device made for a Shelly node is identified by two values:
//
//   node key  the low 24 bits of the node MAC (ShellyNodeKey). Used as the Domoticz "node id" of the device, so it
//             ends up in the DeviceID, which is one of:
//               Lighting2 (switch, dimmer): "0" + key as 6 hex digits (id1 is 0), unit in the Unit column
//               GeneralSwitch (blind, input contact and button selector, binary sensors): key as 8 hex digits, unit in the Unit column
//               ColorSwitch:                key as 8 hex digits, unit in the Unit column
//               Thermostat6:                "0" + key as 6 hex digits, unit in the Unit column
//               Lux meter:                  "0" + key as 6 hex digits (id1 is 0), unit in the Unit column
//               Power (pTypeUsage, W):      "0" + key as 6 hex digits (id1 is 0), unit in the Unit column
//               pTypeGeneral meters (energy, voltage, current, custom sensors, returned energy):
//                                           (key << 8) | unit as 8 hex digits (key in the upper 24 bits, unit in the low 8)
//             The 16 bit temperature, humidity and temperature+humidity sensors only have the low 16 bits of the key, see
//             ShellyNodeKeysClash. Trade-off: two nodes whose MACs differ only above the low 16 bits (about 1 in 65536 for
//             random pairs) would share one such sensor. Both nodes can be added (all their other devices are distinct); the
//             driver keeps a map of 16 bit id to owning node and skips the temperature devices of the node that does not own
//             the id (logged once). The owner is decided when the node list is loaded and does not depend on which node reports
//             first: the node whose MAC string sorts lowest keeps the id (logged at start). A node added later claims a free id.
//   unit      one byte: (kind << 4) | slot. `kind` is a ShellyDeviceKind, `slot` is the component channel (0..15).
//             For the phases of a 3 phase meter slot is (phase << 2) | channel with channel 0..3 (phase 0 is the
//             total, 1..3 are phases A..C).
//
// The MAC is not recoverable from a DeviceID (24 of 48 bits). Resolve the node by comparing keys against the nodes
// of the hardware (ShellyNodeMatchesKey). Two nodes of one hardware instance with the same 24 bit key are refused by AddNode.
//
// Only Switch, Dimmer, Blind, Color and Thermostat devices are controllable. A write to an input contact, a push button
// selector, or any sensor is not a command for the Shelly: WriteToHardware returns false for it (nothing is sent, and
// Domoticz does not apply the value optimistically).
//
// Routing a command back (used by WriteToHardware): take the node id and unit of the device the command is for and
// call ShellyRouteDevice. Only Switch, Dimmer, Blind, Color and Thermostat devices are controllable.
//
//   Lighting2 (switch, dimmer):  nodeId = (id1 << 24) | (id2 << 16) | (id3 << 8) | id4, unit = unitcode
//   Thermostat6:                 same as Lighting2, unit = dunit
//   GeneralSwitch (blind):       nodeId = id,                                              unit = unitcode
//   ColorSwitch:                 nodeId = id,                                              unit = dunit
//
// Both are masked to 24 bits by ShellyRouteDevice.

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <json/json.h>
#include "ShellyTypes.h"

// What a Domoticz device made from a Shelly component is.
enum class ShellyDeviceKind : uint8_t
{
	Switch = 1,      // Lighting2 sTypeAC on/off switch (switch:n, relay)
	Dimmer = 2,      // Lighting2 sTypeAC with level, switch type Dimmer (light:n brightness, lights[])
	Blind = 3,       // GeneralSwitch blind with percentage, open/close/stop (cover:n, rollers[])
	Energy = 4,      // pTypeGeneral sTypeKwh: power (W) and energy counter (kWh)
	Power = 5,       // pTypeUsage W, used where there is no energy counter (per phase power)
	Voltage = 6,     // pTypeGeneral sTypeVoltage (V)
	Current = 7,     // pTypeGeneral sTypeCurrent (A)
	Frequency = 8,   // custom sensor (Hz)
	PowerFactor = 9, // percentage sensor (power factor times 100)
	Temperature = 10, // the one device level temperature of a node (degrees Celsius), same on both generations: Gen1 top level tmp/temperature,
	                  // Gen2 temperature:0. Named "Device temperature" when the node switches, covers, lights or meters something, "Temperature"
	                  // for a pure sensor. The temperature of one switch/cover/light/rgb component is never mapped, and a Gen1 node with
	                  // thermostats[] gets none (the thermostat device has the temperature). A node with humidity gets a TempHum sensor instead.
	Color = 11,      // ColorSwitch (rgb:n, rgbw:n, Gen1 lights[] in color mode, and a bulb in either mode); slot is the channel.
	                 // The kind per channel is stable: a white channel of an RGBW controller (SHRGBW2 in white mode) is a Dimmer, its
	                 // color channel a Color, so a mode change moves between two device sets instead of changing one device's kind.
	Thermostat = 12, // pTypeThermostat6 setpoint + temperature (thermostat:n, Gen1 thermostats[]); slot is the channel
	Input = 13,      // slot 0..7: read-only contact of input n (input:n, inputs[]); slot 8..15: push button selector of input slot-8
	Binary = 14,     // read-only on/off sensor (GeneralSwitch contact/motion/smoke), slot is a ShellyBinaryRole
	Sensor = 15      // slot is a ShellySensorRole (ambient temperature and humidity, lux, gas, tilt) or, from ShellyReturnedEnergySlot(), returned energy
};

// Slot of a Binary device.
enum class ShellyBinaryRole : uint8_t
{
	DoorWindow = 0, // on is open
	Motion = 1,
	Flood = 2,
	Smoke = 3,
	Vibration = 4,
	Gas = 5,        // on is a gas alarm
	Valve = 6       // on is open
};

// Slot of a Sensor device. Slots from ShellyReturnedEnergySlot(0) up are the returned energy of channel 0..11.
enum class ShellySensorRole : uint8_t
{
	TempHum = 0,    // ambient temperature and humidity (either may be missing)
	Lux = 1,
	GasPpm = 2,
	Tilt = 3,       // degrees
	ReturnedEnergy = 4
};
constexpr int SHELLY_MAX_RETURNED_CHANNEL = 11;

// Dimmer slots 0..7 are lights (channel = slot), slots 8..15 are cct components (channel = slot - 8, routed as "cct").
constexpr int SHELLY_CCT_FIRST_SLOT = 8;
constexpr int SHELLY_CCT_MAX_CHANNEL = 7;
constexpr const char* SHELLY_COMPONENT_CCT = "cct";

// Slot of the Input kind: contact of input `channel` (0..7), or the push button selector of it.
int ShellyInputSlot(int channel, bool button);
// Slot of the Sensor kind holding the returned energy of a channel (0..11), -1 outside that range.
int ShellyReturnedEnergySlot(int channel);

// Push button levels of the selector device (Off is 0, the rest is a multiple of 10).
enum class ShellyButtonEvent : int
{
	None = -1,
	Single = 10,
	Double = 20,
	Triple = 30,
	Long = 40
};

enum class ShellyBlindState
{
	Unknown,
	Open,
	Closed,
	Partial
};

// One Domoticz device value derived from a status.
struct ShellyDeviceUpdate
{
	ShellyDeviceKind kind = ShellyDeviceKind::Switch;
	std::string component; // source component: "switch", "light", "cover", "pm1", "em1", "em", "emeter" or "device"
	int channel = 0;       // component index
	int phase = 0;         // 0: none or total, 1..3: phase A..C
	uint8_t unit = 0;      // see ShellyMakeUnit
	std::string name;      // default device name (only used when the device is created)

	bool on = false;       // Switch, Dimmer
	int level = -1;        // Dimmer: 0..100 (-1 when the device reports none). Blind: position 0..100, 100 is fully open (-1 unknown)
	ShellyBlindState blindState = ShellyBlindState::Unknown; // Blind

	double value = 0.0;    // Energy and Power: watt. Voltage: volt. Current: ampere. Frequency: hertz. PowerFactor: percent. Temperature: degrees Celsius
	bool hasEnergy = false;
	double energyKwh = 0.0; // Energy: counter in kWh. Sensor ReturnedEnergy: returned energy counter, value is the returned power in watt (0 when importing)

	int role = 0;           // Binary: ShellyBinaryRole. Sensor: ShellySensorRole. Value: lux for Lux, ppm for GasPpm, degrees for Tilt
	bool button = false;    // Input: the input has no state of its own (momentary button), the device is a selector for its events

	// Color: red, green, blue 0..255 (0 in white mode), white 0..255 (-1 when the light has no white channel), level is the brightness
	int red = 0;
	int green = 0;
	int blue = 0;
	int white = -1;
	bool whiteMode = false; // Color: the light shows white, red/green/blue are not used
	bool rgbw = false;      // Color: the light has a white channel. Decided from the component type (Gen2) or the model (Gen1), never from a status

	// Thermostat (setpoint, temperature) and Sensor TempHum (temperature, humidity); each may be missing
	bool hasTemperature = false;
	double temperature = 0.0;  // degrees Celsius
	bool hasSetpoint = false;
	double setpoint = 0.0;     // degrees Celsius
	bool hasHumidity = false;
	double humidity = 0.0;     // percent

	// Input (button): the last event the status reports. A device that counts its events (Gen1 event_cnt) gives the count, so
	// a repeated event can be told from an old one. eventLevel is a ShellyButtonEvent (None when the code is not one we map).
	bool hasEventCount = false;
	long eventCount = 0;
	int eventLevel = -1;
};

// A discrete input event (Gen2+ NotifyEvent entry) as a button press.
struct ShellyInputEvent
{
	int channel = 0;
	int level = -1; // ShellyButtonEvent
	uint8_t unit = 0;
};

// Reads a NotifyEvent entry ({"component":"input:0","id":0,"event":"single_push",...}). False for anything that is not an
// input press we map (other components, btn_down/btn_up, unknown names, channel above 7). Never throws.
bool ShellyMapInputEvent(const ShellyNode& node, const Json::Value& event, ShellyInputEvent& out);

struct ShellyNodeUpdate
{
	std::vector<ShellyDeviceUpdate> devices;
	int rssiLevel = 12;      // Domoticz 0..11, 12 when unknown
	int batteryLevel = 255;  // 0..100, 255 when unknown or mains powered
	// Component types ("rgb", "input", ...) found in the status that are not mapped (yet). The driver logs each once.
	std::vector<std::string> unmapped;
	// Units that two values of one status mapped to (the first one wins, the other is dropped). The driver logs each once.
	std::vector<uint8_t> droppedUnits;
	// Gen1 input channels configured as a push button whose status has no event counter: they were mapped as a contact. The driver logs each once.
	std::vector<int> buttonWithoutCounter;
};

// How an input is configured on the device: a switch (it has a state, a contact) or a push button (it only has presses).
enum class ShellyInputKind : uint8_t
{
	Unknown = 0, // not configured or not reported: treated as a contact
	Switch,      // Gen1 btn_type toggle, edge, detached
	Button       // Gen1 btn_type momentary, momentary_on_release, action, cycle
};

// Static information learned from OnShellyNodeInfo: names and device modes. May be empty (defaults are used).
struct ShellyNodeConfig
{
	std::map<int, ShellyInputKind> inputKinds;        // Gen1 only: input index -> configured type (Gen2 tells it in the status: state null is a button)
	std::string deviceName;                          // name of the device itself as configured on it
	std::string firmware;                            // firmware version as reported
	bool rollerMode = false;                         // Gen1: the device runs in roller mode, its relays are not switches
	std::map<std::string, std::string> componentNames; // "switch:0", "light:1", "cover:0" -> name configured on the device
	bool sleepKnown = false;                         // the information said whether the device sleeps (a partial document leaves the stored value alone)
	bool sleepy = false;                             // battery device that is awake only now and then (Gen1 sleep_mode, Gen2 sys sleep)
	int sleepPeriodSec = 0;                          // its wake-up period, 0 when unknown
};

// Reads the document delivered to OnShellyNodeInfo (Gen2+: {"deviceinfo", "config"}, Gen1: the /settings document).
// Never throws, tolerates missing or malformed parts, strips control characters from names.
ShellyNodeConfig ShellyParseNodeInfo(const ShellyNode& node, const Json::Value& info);

// Maps a full status (node.gen >= 2: Shelly.GetStatus result, node.gen == 1: /status document) to device updates.
// Never throws; anything missing or of the wrong type is skipped.
ShellyNodeUpdate ShellyMapStatus(const ShellyNode& node, const Json::Value& status, const ShellyNodeConfig& config);

// Name every device of a node starts with: the name given in Domoticz, else the name configured on the device, else the
// device id, else "Shelly <model> <last 6 of the MAC>".
std::string ShellyBaseName(const ShellyNode& node, const ShellyNodeConfig& config);
// Name configured on the device for the component ("switch", "input", ...), else "<Kind> <n>" (n starts at 1).
std::string ShellyComponentLabel(const ShellyNodeConfig& config, const std::string& type, int channel);
// Default name of the push button selector of an input, the same one ShellyMapStatus gives it.
std::string ShellyInputButtonName(const ShellyNode& node, const ShellyNodeConfig& config, int channel);
// Gen1 bulbs (the /light/ endpoint with mode white/color).
bool ShellyIsGen1BulbModel(const std::string& model);
// Gen1 color lights that have a white channel next to red, green and blue.
bool ShellyGen1HasWhiteChannel(const std::string& model);

// Low 24 bits of a normalized MAC (12 hex digits). Returns false for anything else.
bool ShellyNodeKeyFromMac(const std::string& mac, uint32_t& key);
// Same, returns 0 for an invalid MAC.
uint32_t ShellyNodeKey(const std::string& mac);
bool ShellyNodeMatchesKey(const ShellyNode& node, uint32_t key);
// True when two different nodes would share Domoticz devices: equal node keys, or equal low 16 bits (the temperature
// sensors are identified by 16 bits only). Equal keys are refused by AddNode; equal low 16 bits are allowed, the driver
// then lets only one of the nodes own the temperature sensor (see the top of this file).
bool ShellyNodeKeysClash(uint32_t keyA, uint32_t keyB);

uint8_t ShellyMakeUnit(ShellyDeviceKind kind, int slot);
ShellyDeviceKind ShellyUnitKind(uint8_t unit);
int ShellyUnitSlot(uint8_t unit);

// Where a command for a controllable Domoticz device has to go.
struct ShellyDeviceRoute
{
	uint32_t nodeKey = 0;
	std::string component; // ShellyCmd::COMPONENT_SWITCH, COMPONENT_LIGHT, COMPONENT_COVER (the Gen1 client accepts COMPONENT_COVER for a roller),
	                       // COMPONENT_RGB (the driver switches to COMPONENT_RGBW for a light with a white channel) or COMPONENT_THERMOSTAT
	int channel = 0;       // component index
	ShellyDeviceKind kind = ShellyDeviceKind::Switch;
};

// Reverse of the identity scheme above. Returns false for units that are not a controllable device (sensors, meters)
// and for units no mapping creates. See the top of this file for what to pass as `id`.
bool ShellyRouteDevice(uint32_t id, uint8_t unit, ShellyDeviceRoute& route);

// Domoticz signal level (0..11) for a Wi-Fi RSSI in dBm.
int ShellyRssiToLevel(int dbm);
