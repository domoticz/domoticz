#include "stdafx.h"
#include "ShellyMapping.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <set>
#include "ShellyIdentify.h"

namespace
{
	const Json::Value& NullValue()
	{
		static const Json::Value value;
		return value;
	}

	// Safe member access: a missing key, or a value that is not an object, yields null (never asserts or throws).
	const Json::Value& Member(const Json::Value& value, const char* key)
	{
		if (value.isObject() && value.isMember(key))
			return value[key];
		return NullValue();
	}

	// Safe array access.
	const Json::Value& Element(const Json::Value& value, const Json::ArrayIndex index)
	{
		if (value.isArray() && index < value.size())
			return value[index];
		return NullValue();
	}

	bool GetNumber(const Json::Value& value, const char* key, double& out)
	{
		const Json::Value& member = Member(value, key);
		if (!member.isNumeric())
			return false;
		const double number = member.asDouble();
		if (!std::isfinite(number))
			return false;
		out = number;
		return true;
	}

	bool GetBool(const Json::Value& value, const char* key, bool& out)
	{
		const Json::Value& member = Member(value, key);
		if (!member.isBool())
			return false;
		out = member.asBool();
		return true;
	}

	std::string GetString(const Json::Value& value, const char* key)
	{
		const Json::Value& member = Member(value, key);
		return member.isString() ? member.asString() : std::string();
	}

	// A flag the device reports as a boolean or as 0/1.
	bool GetFlag(const Json::Value& value, const char* key, bool& out)
	{
		const Json::Value& member = Member(value, key);
		if (member.isBool())
		{
			out = member.asBool();
			return true;
		}
		if (member.isNumeric())
		{
			const double number = member.asDouble();
			if (!std::isfinite(number))
				return false;
			out = (number != 0.0);
			return true;
		}
		return false;
	}

	// A sensor component that reports an error (missing driver, broken probe) carries no usable reading.
	bool HasErrors(const Json::Value& component)
	{
		const Json::Value& errors = Member(component, "errors");
		return errors.isArray() && !errors.empty();
	}

	// "switch:0" -> type "switch", id 0. False for keys without an index or with a non numeric index. There is no range
	// check on the id (SplitComponent applies it); an index too long for an int is returned as a large id.
	bool ParseComponentKey(const std::string& key, std::string& type, int& id)
	{
		const size_t pos = key.find(':');
		if (pos == std::string::npos || pos == 0)
			return false;
		const std::string number = key.substr(pos + 1);
		if (number.empty() || number.find_first_not_of("0123456789") != std::string::npos)
			return false;
		type = key.substr(0, pos);
		// A component number of more than six digits is out of range; the sentinel sorts after every real id.
		constexpr int kOutOfRangeComponentId = 1000000;
		id = (number.size() > 6) ? kOutOfRangeComponentId : std::atoi(number.c_str());
		return true;
	}

	bool SplitComponent(const std::string& key, std::string& type, int& id)
	{
		return ParseComponentKey(key, type, id) && ShellyIsValidChannel(id);
	}

	std::string ComponentKey(const std::string& type, const int id)
	{
		return type + ":" + std::to_string(id);
	}

	struct MeterReading
	{
		bool hasReturned = false;
		double returnedWh = 0.0;
		bool hasPower = false;
		bool hasVoltage = false;
		bool hasCurrent = false;
		bool hasFrequency = false;
		bool hasPowerFactor = false;
		bool hasEnergy = false;
		double power = 0.0;
		double voltage = 0.0;
		double current = 0.0;
		double frequency = 0.0;
		double powerFactor = 0.0; // fraction, 0..1
		double energyWh = 0.0;
	};

	class Builder
	{
	      public:
		Builder(const ShellyNode& node, const ShellyNodeConfig& config)
			: m_node(node)
			, m_config(config)
			, m_baseName(ShellyBaseName(node, config))
		{
		}

		const ShellyNode& Node() const
		{
			return m_node;
		}
		const ShellyNodeConfig& Config() const
		{
			return m_config;
		}

		std::string Label(const std::string& type, const int channel) const
		{
			return ShellyComponentLabel(m_config, type, channel);
		}

		// Returns null when the same unit was already produced in this status (the first one wins).
		ShellyDeviceUpdate* Add(const ShellyDeviceKind kind, const std::string& component, const int channel, const int phase, const int slot, const std::string& label, const std::string& suffix)
		{
			const uint8_t unit = ShellyMakeUnit(kind, slot);
			if (!m_units.insert(unit).second)
			{
				result.droppedUnits.push_back(unit);
				return nullptr;
			}
			result.devices.emplace_back();
			ShellyDeviceUpdate& update = result.devices.back();
			update.kind = kind;
			update.component = component;
			update.channel = channel;
			update.phase = phase;
			update.unit = unit;
			update.name = m_baseName;
			if (!label.empty())
				update.name += " " + label;
			if (!suffix.empty())
				update.name += " " + suffix;
			return &update;
		}

		// One device level temperature per node, on both generations (Gen2 temperature:0, Gen1 top level tmp or temperature). The
		// temperature of an individual switch, cover, light or rgb component (temperature.tC inside the component) is never mapped.
		void NoteDeviceTemperature(const double celsius)
		{
			m_hasDeviceTemperature = true;
			m_deviceTemperature = celsius;
		}

		// The node switches, covers, lights or meters something: its device temperature is the one of the unit itself and is named
		// "Device temperature". A pure sensor (H&T, door/window, flood, ...) gets "Temperature".
		void NoteSwitching()
		{
			m_switching = true;
		}

		// The node has a thermostat component, which reports the temperature itself: no second device temperature.
		void NoteThermostat()
		{
			m_hasThermostat = true;
		}

		void NoteHumidity(const double percent)
		{
			m_hasHumidity = true;
			m_humidity = percent;
		}

		// Adds the device temperature once all components were seen. A device that also measures humidity gets one
		// temperature and humidity device instead (the temperature of such a device is the ambient one).
		void Finish()
		{
			if (m_hasHumidity)
			{
				ShellyDeviceUpdate* update = Add(ShellyDeviceKind::Sensor, "device", 0, 0, static_cast<int>(ShellySensorRole::TempHum), std::string(), m_hasDeviceTemperature ? "Climate" : "Humidity");
				if (update)
				{
					update->role = static_cast<int>(ShellySensorRole::TempHum);
					update->hasHumidity = true;
					update->humidity = m_humidity;
					update->hasTemperature = m_hasDeviceTemperature;
					update->temperature = m_deviceTemperature;
				}
				return;
			}
			if (!m_hasDeviceTemperature || m_hasThermostat)
				return;
			ShellyDeviceUpdate* update = Add(ShellyDeviceKind::Temperature, "device", 0, 0, 0, std::string(), m_switching ? "Device temperature" : "Temperature");
			if (update)
				update->value = m_deviceTemperature;
		}

		ShellyNodeUpdate result;

	      private:
		const ShellyNode& m_node;
		const ShellyNodeConfig& m_config;
		std::string m_baseName;
		std::set<uint8_t> m_units;
		bool m_hasDeviceTemperature = false;
		bool m_switching = false;
		bool m_hasThermostat = false;
		bool m_hasHumidity = false;
		double m_humidity = 0.0;
		double m_deviceTemperature = 0.0;
	};

	void AddMeter(Builder& builder, const std::string& component, const int channel, const int phase, const int slot, const std::string& label, const MeterReading& reading,
		      const bool allowEnergy)
	{
		std::string fullLabel = label;
		if (phase > 0)
			fullLabel += std::string(" Phase ") + static_cast<char>('A' + phase - 1);

		if (reading.hasPower)
		{
			if (reading.hasEnergy && allowEnergy)
			{
				ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Energy, component, channel, phase, slot, fullLabel, "Energy");
				if (update)
				{
					update->value = reading.power;
					update->hasEnergy = true;
					update->energyKwh = reading.energyWh / 1000.0;
				}
			}
			else
			{
				ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Power, component, channel, phase, slot, fullLabel, "Power");
				if (update)
					update->value = reading.power;
			}
		}
		if (reading.hasVoltage)
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Voltage, component, channel, phase, slot, fullLabel, "Voltage");
			if (update)
				update->value = reading.voltage;
		}
		if (reading.hasCurrent)
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Current, component, channel, phase, slot, fullLabel, "Current");
			if (update)
				update->value = reading.current;
		}
		if (reading.hasFrequency)
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Frequency, component, channel, phase, slot, fullLabel, "Frequency");
			if (update)
				update->value = reading.frequency;
		}
		if (reading.hasPowerFactor)
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::PowerFactor, component, channel, phase, slot, fullLabel, "Power factor");
			if (update)
				update->value = std::min(100.0, std::max(0.0, std::fabs(reading.powerFactor) * 100.0));
		}
		// Energy that was fed back: only worth a device once something was (a meter that never exports stays at 0). The
		// instantaneous value is the power while exporting (negative power on the device).
		const int returnedSlot = (phase == 0) ? ShellyReturnedEnergySlot(slot) : -1;
		if (reading.hasReturned && reading.returnedWh > 0.0 && returnedSlot >= 0)
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Sensor, component, channel, phase, returnedSlot, fullLabel, "Returned energy");
			if (update)
			{
				update->role = static_cast<int>(ShellySensorRole::ReturnedEnergy);
				update->value = (reading.hasPower && reading.power < 0.0) ? -reading.power : 0.0;
				update->hasEnergy = true;
				update->energyKwh = reading.returnedWh / 1000.0;
			}
		}
	}

	// Gen2+ components that carry their own measurements (switch, cover, pm1, em1 use the same field names).
	MeterReading ReadGen2Meter(const Json::Value& component, const bool withReturned = false)
	{
		MeterReading reading;
		if (withReturned)
			reading.hasReturned = GetNumber(Member(component, "ret_aenergy"), "total", reading.returnedWh);
		reading.hasPower = GetNumber(component, "apower", reading.power);
		if (!reading.hasPower)
			reading.hasPower = GetNumber(component, "act_power", reading.power); // em1
		reading.hasVoltage = GetNumber(component, "voltage", reading.voltage);
		reading.hasCurrent = GetNumber(component, "current", reading.current);
		reading.hasFrequency = GetNumber(component, "freq", reading.frequency);
		reading.hasPowerFactor = GetNumber(component, "pf", reading.powerFactor);
		reading.hasEnergy = GetNumber(Member(component, "aenergy"), "total", reading.energyWh);
		return reading;
	}

	int ClampPercent(const double value)
	{
		return static_cast<int>(std::lround(std::min(100.0, std::max(0.0, value))));
	}

	// Clamped before the integer conversion so a bogus value (huge, from a broken device) cannot overflow it.
	int ClampRssi(const double dbm)
	{
		return static_cast<int>(std::lround(std::min(0.0, std::max(-200.0, dbm))));
	}

	void MapGen2Cover(Builder& builder, const Json::Value& component, const int id)
	{
		// A negative position (or none, or a non finite one) means the cover is not calibrated: the position is unknown and
		// the state decides.
		double position = 0.0;
		const bool hasPosition = GetNumber(component, "current_pos", position) && position >= 0.0;
		const std::string state = GetString(component, "state");

		ShellyBlindState blind = ShellyBlindState::Unknown;
		int level = -1;
		if (hasPosition)
		{
			level = ClampPercent(position);
			blind = (level >= 100) ? ShellyBlindState::Open : (level <= 0) ? ShellyBlindState::Closed : ShellyBlindState::Partial;
		}
		else if (state == "open")
		{
			blind = ShellyBlindState::Open;
			level = 100;
		}
		else if (state == "closed")
		{
			blind = ShellyBlindState::Closed;
			level = 0;
		}

		if (blind != ShellyBlindState::Unknown)
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Blind, "cover", id, 0, id, builder.Label("cover", id), std::string());
			if (update)
			{
				update->blindState = blind;
				update->level = level;
			}
		}
	}

	void MapGen2Light(Builder& builder, const Json::Value& component, const int id)
	{
		bool output = false;
		if (id >= SHELLY_CCT_FIRST_SLOT)
		{
			builder.result.unmapped.push_back("light (channel above 7)");
			return;
		}
		if (!GetBool(component, "output", output))
			return;
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Dimmer, "light", id, 0, id, builder.Label("light", id), std::string());
		if (!update)
			return;
		update->on = output;
		double brightness = 0.0;
		if (GetNumber(component, "brightness", brightness))
			update->level = ClampPercent(brightness);
	}

	// cct:n (tunable white, for example the Duo bulb). A Dimmer in the slots from SHELLY_CCT_FIRST_SLOT up, so it never clashes
	// with a light and is routed to CCT.Set. The colour temperature is neither required nor mapped.
	void MapGen2Cct(Builder& builder, const Json::Value& component, const int id)
	{
		bool output = false;
		if (id > SHELLY_CCT_MAX_CHANNEL)
		{
			builder.result.unmapped.push_back("cct (channel above " + std::to_string(SHELLY_CCT_MAX_CHANNEL) + ")");
			return;
		}
		if (!GetBool(component, "output", output))
			return;
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Dimmer, "cct", id, 0, SHELLY_CCT_FIRST_SLOT + id, builder.Label("light", id), std::string());
		if (!update)
			return;
		update->on = output;
		double brightness = 0.0;
		if (GetNumber(component, "brightness", brightness))
			update->level = ClampPercent(brightness);
	}

	void MapGen2Switch(Builder& builder, const Json::Value& component, const int id)
	{
		bool output = false;
		if (GetBool(component, "output", output))
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Switch, "switch", id, 0, id, builder.Label("switch", id), std::string());
			if (update)
				update->on = output;
		}
	}

	int ClampByte(const double value)
	{
		return static_cast<int>(std::lround(std::min(255.0, std::max(0.0, value))));
	}

	void AddBinary(Builder& builder, const std::string& component, const ShellyBinaryRole role, const bool on, const char* suffix)
	{
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Binary, component, 0, 0, static_cast<int>(role), std::string(), suffix);
		if (update)
		{
			update->role = static_cast<int>(role);
			update->on = on;
		}
	}

	// rgb:n and rgbw:n. The color is kept apart from the brightness, as Domoticz does.
	void MapGen2Color(Builder& builder, const Json::Value& component, const int id, const bool withWhite)
	{
		bool output = false;
		if (!GetBool(component, "output", output))
			return;
		const std::string type = withWhite ? "rgbw" : "rgb";
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Color, type, id, 0, id, builder.Label(type, id), std::string());
		if (!update)
			return;
		update->on = output;
		update->rgbw = withWhite;
		const Json::Value& rgb = Member(component, "rgb");
		if (rgb.isArray() && rgb.size() == 3 && rgb[0].isNumeric() && rgb[1].isNumeric() && rgb[2].isNumeric())
		{
			update->red = ClampByte(rgb[0].asDouble());
			update->green = ClampByte(rgb[1].asDouble());
			update->blue = ClampByte(rgb[2].asDouble());
		}
		if (withWhite)
		{
			double white = 0.0;
			GetNumber(component, "white", white);
			update->white = ClampByte(white);
		}
		double brightness;
		if (GetNumber(component, "brightness", brightness))
			update->level = ClampPercent(brightness);
	}

	// input:n. An input with a state is a contact, one whose state is null is a momentary button (it only has events).
	// Inputs without a "state" member (analog, counter) are not mapped.
	void MapGen2Input(Builder& builder, const Json::Value& component, const int id)
	{
		if (id > 7)
		{
			builder.result.unmapped.push_back("input (channel above 7)");
			return;
		}
		const Json::Value& state = Member(component, "state");
		if (state.isBool())
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Input, "input", id, 0, ShellyInputSlot(id, false), builder.Label("input", id), std::string());
			if (update)
				update->on = state.asBool();
		}
		else if (state.isNull() && component.isMember("state"))
		{
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Input, "input", id, 0, ShellyInputSlot(id, true), builder.Label("input", id), "Button");
			if (update)
				update->button = true;
		}
	}

	void MapGen2Thermostat(Builder& builder, const Json::Value& component, const int id)
	{
		double current = 0.0;
		double target = 0.0;
		const bool hasCurrent = GetNumber(component, "current_C", current);
		const bool hasTarget = GetNumber(component, "target_C", target);
		if (!hasCurrent && !hasTarget)
			return;
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Thermostat, "thermostat", id, 0, id, builder.Label("thermostat", id), std::string());
		if (!update)
			return;
		update->hasTemperature = hasCurrent;
		update->temperature = current;
		update->hasSetpoint = hasTarget;
		update->setpoint = target;
		builder.NoteThermostat();
	}

	// Energy counters of em/em1 live in separate components (emdata:n, em1data:n).
	void MapGen2ThreePhase(Builder& builder, const Json::Value& component, const Json::Value& data, const int id)
	{
		if (id > 3)
		{
			// the unit has room for 4 meters of 3 phases each
			builder.result.unmapped.push_back("em (channel above 3)");
			return;
		}
		const std::string label = builder.Label("em", id);

		MeterReading total;
		total.hasPower = GetNumber(component, "total_act_power", total.power);
		total.hasCurrent = GetNumber(component, "total_current", total.current);
		total.hasEnergy = GetNumber(data, "total_act", total.energyWh);
		total.hasReturned = GetNumber(data, "total_act_ret", total.returnedWh);
		AddMeter(builder, "em", id, 0, id, label, total, true);

		static const char prefixes[3] = { 'a', 'b', 'c' };
		for (int phase = 1; phase <= 3; phase++)
		{
			const std::string prefix = std::string(1, prefixes[phase - 1]) + "_";
			MeterReading reading;
			reading.hasPower = GetNumber(component, (prefix + "act_power").c_str(), reading.power);
			reading.hasVoltage = GetNumber(component, (prefix + "voltage").c_str(), reading.voltage);
			reading.hasCurrent = GetNumber(component, (prefix + "current").c_str(), reading.current);
			reading.hasFrequency = GetNumber(component, (prefix + "freq").c_str(), reading.frequency);
			reading.hasPowerFactor = GetNumber(component, (prefix + "pf").c_str(), reading.powerFactor);
			AddMeter(builder, "em", id, phase, (phase << 2) | id, label, reading, false);
		}
	}

	// presencezone:N (bool "value") is a read-only Motion device per zone, in Binary slots 8..15 for the zone ids 200..207 (the
	// slots 0..6 are the roles). "presence" with a boolean "state" is the sensor as a whole, in the Motion slot.
	void MapGen2Presence(Builder& builder, const std::string& key, const Json::Value& component)
	{
		if (!component.isObject())
			return;
		bool value = false;
		if (key == "presence")
		{
			if (GetFlag(component, "state", value))
				AddBinary(builder, "presence", ShellyBinaryRole::Motion, value, "Presence");
			return;
		}
		const std::string number = key.substr(13);
		if (number.empty() || number.size() > 3 || number.find_first_not_of("0123456789") != std::string::npos)
			return;
		const int zone = std::atoi(number.c_str()) - 200;
		if (zone < 0 || zone > 7)
		{
			builder.result.unmapped.push_back("presencezone (id outside 200..207)");
			return;
		}
		if (!GetFlag(component, "value", value))
			return;
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Binary, "presencezone", zone, 0, 8 + zone, std::string(), "Presence zone " + std::to_string(zone + 1));
		if (update)
		{
			update->role = static_cast<int>(ShellyBinaryRole::Motion);
			update->on = value;
		}
	}

	void MapGen2(Builder& builder, const Json::Value& status)
	{
		static const std::set<std::string> handled = { "switch", "cover", "light", "cct", "pm1", "em1", "em", "em1data", "emdata", "devicepower", "temperature", "humidity", "illuminance",
							       "rgb", "rgbw", "input", "thermostat", "smoke", "flood", "motion", "valve", "presence", "presencezone",
							       // not devices: scripts, schedules and the virtual and BTHome building blocks
							       "script", "bthomedevice", "bthomesensor", "boolean", "number", "text", "enum", "button", "group", "knx", "zigbee" };

		for (const std::string& key : status.getMemberNames())
		{
			// The presence sensor keeps its zones at ids from 200 up and its own state in "presence" (without an id).
			if (key == "presence" || key.compare(0, 13, "presencezone:") == 0)
			{
				MapGen2Presence(builder, key, status[key]);
				continue;
			}
			std::string type;
			int id = 0;
			if (!SplitComponent(key, type, id))
			{
				// A component above the channel range would share a unit with a lower one: it is reported, not mapped.
				static const std::set<std::string> channelled = { "switch", "cover", "light", "cct", "pm1", "em1", "em", "rgb", "rgbw", "input", "thermostat" };
				if (ParseComponentKey(key, type, id) && channelled.find(type) != channelled.end())
					builder.result.unmapped.push_back(type + " (channel above " + std::to_string(SHELLY_MAX_CHANNEL) + ")");
				continue;
			}
			const Json::Value& component = status[key];
			if (!component.isObject())
				continue;

			if (type == "switch" || type == "cover" || type == "light" || type == "cct" || type == "rgb" || type == "rgbw" || type == "pm1" || type == "em1" || type == "em")
				builder.NoteSwitching();

			if (type == "switch")
			{
				MapGen2Switch(builder, component, id);
				AddMeter(builder, "switch", id, 0, id, builder.Label("switch", id), ReadGen2Meter(component), true);
			}
			else if (type == "cover")
			{
				MapGen2Cover(builder, component, id);
				AddMeter(builder, "cover", id, 0, id, builder.Label("cover", id), ReadGen2Meter(component), true);
			}
			else if (type == "light")
				MapGen2Light(builder, component, id);
			else if (type == "cct")
				MapGen2Cct(builder, component, id);
			else if (type == "pm1")
			{
				MeterReading reading = ReadGen2Meter(component, true);
				AddMeter(builder, "pm1", id, 0, id, builder.Label("pm1", id), reading, true);
			}
			else if (type == "em1")
			{
				MeterReading reading = ReadGen2Meter(component);
				// The energy counter is in em1data:n
				const Json::Value& data = Member(status, ComponentKey("em1data", id).c_str());
				reading.hasEnergy = GetNumber(data, "total_act_energy", reading.energyWh);
				reading.hasReturned = GetNumber(data, "total_act_ret_energy", reading.returnedWh);
				AddMeter(builder, "em1", id, 0, id, builder.Label("em1", id), reading, true);
			}
			else if (type == "em")
			{
				MapGen2ThreePhase(builder, component, Member(status, ComponentKey("emdata", id).c_str()), id);
			}
			else if (type == "rgb" || type == "rgbw")
				MapGen2Color(builder, component, id, type == "rgbw");
			else if (type == "input")
				MapGen2Input(builder, component, id);
			else if (type == "thermostat")
				MapGen2Thermostat(builder, component, id);
			else if (type == "temperature")
			{
				double celsius;
				if (id == 0 && !HasErrors(component) && GetNumber(component, "tC", celsius))
					builder.NoteDeviceTemperature(celsius);
			}
			else if (type == "humidity")
			{
				double percent;
				if (id == 0 && !HasErrors(component) && GetNumber(component, "rh", percent))
					builder.NoteHumidity(std::min(100.0, std::max(0.0, percent)));
			}
			else if (type == "illuminance")
			{
				double lux;
				if (id == 0 && !HasErrors(component) && GetNumber(component, "lux", lux))
				{
					ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Sensor, "illuminance", 0, 0, static_cast<int>(ShellySensorRole::Lux), std::string(), "Illuminance");
					if (update)
					{
						update->role = static_cast<int>(ShellySensorRole::Lux);
						update->value = std::max(0.0, lux);
					}
				}
			}
			else if (type == "smoke" || type == "flood")
			{
				bool alarm;
				if (id == 0 && GetFlag(component, "alarm", alarm))
					AddBinary(builder, type, type == "smoke" ? ShellyBinaryRole::Smoke : ShellyBinaryRole::Flood, alarm, type == "smoke" ? "Smoke" : "Flood");
			}
			else if (type == "motion")
			{
				bool motion;
				if (id == 0 && GetFlag(component, "motion", motion))
					AddBinary(builder, type, ShellyBinaryRole::Motion, motion, "Motion");
			}
			else if (type == "valve")
			{
				// Opening counts as open, closing as closed, a failure says nothing.
				const std::string state = GetString(component, "state");
				if (id == 0 && !state.empty() && state != "failure")
					AddBinary(builder, type, ShellyBinaryRole::Valve, state == "opened" || state == "open" || state == "opening", "Valve");
			}
			else if (type == "devicepower")
			{
				double percent;
				if (id == 0 && GetNumber(Member(component, "battery"), "percent", percent))
					builder.result.batteryLevel = ClampPercent(percent);
			}

			if (handled.find(type) == handled.end())
				builder.result.unmapped.push_back(type);
		}

		double rssi;
		if (GetNumber(Member(status, "wifi"), "rssi", rssi))
			builder.result.rssiLevel = ShellyRssiToLevel(ClampRssi(rssi));
	}

	bool IsValidFlag(const Json::Value& block)
	{
		bool valid = true;
		return !GetBool(block, "is_valid", valid) || valid;
	}

	int Gen1EventLevel(const std::string& code)
	{
		if (code == "S")
			return static_cast<int>(ShellyButtonEvent::Single);
		if (code == "SS")
			return static_cast<int>(ShellyButtonEvent::Double);
		if (code == "SSS")
			return static_cast<int>(ShellyButtonEvent::Triple);
		if (code == "L")
			return static_cast<int>(ShellyButtonEvent::Long);
		return static_cast<int>(ShellyButtonEvent::None);
	}

	// inputs[]: the level of an input that is configured as a switch (or whose type is unknown) is a contact, the last event
	// (with its counter) of an input configured as a push button is the state of its selector. Never both: a push button has
	// no level worth a device, a switch has no presses. A button whose status has no event_cnt (firmware that does not count
	// events) cannot drive a selector, so it falls back to the contact device (reported through buttonWithoutCounter).
	//
	// How the input type is learned (ShellyParseNodeInfo): inputs[n].btn_type (i3, Uni), else the btn_type of the relay, light or
	// roller with the same index. This is not verified on every model: a model whose settings carry the type elsewhere stays
	// Unknown and is a contact. Shelly 2.5 in roller mode: the inputs beyond the length of rollers[] have no btn_type source,
	// so they are Unknown and map to a contact only.
	void MapGen1Inputs(Builder& builder, const Json::Value& inputs)
	{
		const Json::ArrayIndex count = std::min<Json::ArrayIndex>(inputs.isArray() ? inputs.size() : 0, 8);
		for (Json::ArrayIndex i = 0; i < count; i++)
		{
			const Json::Value& input = Element(inputs, i);
			const int id = static_cast<int>(i);
			const auto kindIt = builder.Config().inputKinds.find(id);
			const ShellyInputKind kind = (kindIt == builder.Config().inputKinds.end()) ? ShellyInputKind::Unknown : kindIt->second;
			bool level;
			double counter = 0.0;
			const bool asButton = (kind == ShellyInputKind::Button) && GetNumber(input, "event_cnt", counter);
			if (kind == ShellyInputKind::Button && !asButton)
				builder.result.buttonWithoutCounter.push_back(id);
			if (!asButton && GetFlag(input, "input", level))
			{
				ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Input, "input", id, 0, ShellyInputSlot(id, false), builder.Label("input", id), std::string());
				if (update)
					update->on = level;
			}
			if (asButton)
			{
				ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Input, "input", id, 0, ShellyInputSlot(id, true), builder.Label("input", id), "Button");
				if (update)
				{
					update->button = true;
					update->hasEventCount = true;
					update->eventCount = static_cast<long>(std::min(2147483647.0, std::max(0.0, counter)));
					update->eventLevel = Gen1EventLevel(GetString(input, "event"));
				}
			}
		}
	}

	// thermostats[] (the TRV). The units are Celsius unless the device says Fahrenheit.
	void MapGen1Thermostats(Builder& builder, const Json::Value& thermostats)
	{
		const Json::ArrayIndex count = std::min<Json::ArrayIndex>(thermostats.isArray() ? thermostats.size() : 0, SHELLY_MAX_CHANNEL + 1);
		for (Json::ArrayIndex i = 0; i < count; i++)
		{
			const Json::Value& thermostat = Element(thermostats, i);
			const Json::Value& tmp = Member(thermostat, "tmp");
			const Json::Value& target = Member(thermostat, "target_t");
			const auto toCelsius = [](const Json::Value& block, const double value) { return (GetString(block, "units") == "F") ? (value - 32.0) * 5.0 / 9.0 : value; };
			double current = 0.0;
			double setpoint = 0.0;
			const bool hasCurrent = tmp.isObject() && IsValidFlag(tmp) && GetNumber(tmp, "value", current);
			const bool hasSetpoint = target.isObject() && GetNumber(target, "value", setpoint);
			if (!hasCurrent && !hasSetpoint)
				continue;
			const int id = static_cast<int>(i);
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Thermostat, "thermostat", id, 0, id, builder.Label("thermostat", id), std::string());
			if (!update)
				continue;
			update->hasTemperature = hasCurrent;
			update->temperature = toCelsius(tmp, current);
			update->hasSetpoint = hasSetpoint;
			update->setpoint = toCelsius(target, setpoint);
			builder.NoteThermostat();
		}
	}

	void AddSensorValue(Builder& builder, const std::string& component, const ShellySensorRole role, const double value, const char* suffix)
	{
		ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Sensor, component, 0, 0, static_cast<int>(role), std::string(), suffix);
		if (update)
		{
			update->role = static_cast<int>(role);
			update->value = value;
		}
	}

	// The sensor blocks of the battery and gas/smoke/flood devices (humidity, lux, door/window, motion, flood, smoke, gas).
	void MapGen1Sensors(Builder& builder, const Json::Value& status)
	{
		double value;
		bool flag;

		const Json::Value& hum = Member(status, "hum");
		if (hum.isObject() && IsValidFlag(hum) && GetNumber(hum, "value", value))
			builder.NoteHumidity(std::min(100.0, std::max(0.0, value)));

		const Json::Value& lux = Member(status, "lux");
		if (lux.isObject() && IsValidFlag(lux) && GetNumber(lux, "value", value))
			AddSensorValue(builder, "lux", ShellySensorRole::Lux, std::max(0.0, value), "Illuminance");

		bool hasVibration = false;
		const Json::Value& sensor = Member(status, "sensor");
		if (sensor.isObject() && IsValidFlag(sensor))
		{
			const std::string state = GetString(sensor, "state");
			if (state == "open" || state == "close" || state == "closed")
				AddBinary(builder, "sensor", ShellyBinaryRole::DoorWindow, state == "open", "Door window");
			else if (GetFlag(sensor, "motion", flag))
				AddBinary(builder, "sensor", ShellyBinaryRole::Motion, flag, "Motion");
			if (GetFlag(sensor, "vibration", flag))
			{
				AddBinary(builder, "sensor", ShellyBinaryRole::Vibration, flag, "Vibration");
				hasVibration = true;
			}
		}
		const Json::Value& accel = Member(status, "accel");
		if (accel.isObject())
		{
			if (!hasVibration && GetFlag(accel, "vibration", flag))
				AddBinary(builder, "accel", ShellyBinaryRole::Vibration, flag, "Vibration");
			if (GetNumber(accel, "tilt", value))
				AddSensorValue(builder, "accel", ShellySensorRole::Tilt, value, "Tilt");
		}

		if (GetFlag(status, "flood", flag))
			AddBinary(builder, "flood", ShellyBinaryRole::Flood, flag, "Flood");
		if (GetFlag(status, "smoke", flag))
			AddBinary(builder, "smoke", ShellyBinaryRole::Smoke, flag, "Smoke");

		const Json::Value& gas = Member(status, "gas_sensor");
		if (gas.isObject())
		{
			const std::string alarm = GetString(gas, "alarm_state");
			if (!alarm.empty() && alarm != "unknown")
				AddBinary(builder, "gas_sensor", ShellyBinaryRole::Gas, alarm != "none", "Gas alarm");
		}
		const Json::Value& concentration = Member(status, "concentration");
		if (concentration.isObject() && IsValidFlag(concentration) && GetNumber(concentration, "ppm", value))
			AddSensorValue(builder, "concentration", ShellySensorRole::GasPpm, std::max(0.0, value), "Gas");
	}

	void MapGen1(Builder& builder, const Json::Value& status, const ShellyNodeConfig& config)
	{
		const Json::Value& relays = Member(status, "relays");
		const Json::Value& rollers = Member(status, "rollers");
		const Json::Value& lights = Member(status, "lights");
		const Json::Value& meters = Member(status, "meters");
		const Json::Value& emeters = Member(status, "emeters");

		const bool hasRollers = rollers.isArray() && !rollers.empty();
		const bool relaysAreSwitches = !hasRollers && !config.rollerMode;
		const bool hasLights = lights.isArray() && !lights.empty();

		const Json::ArrayIndex relayCount = std::min<Json::ArrayIndex>(relays.isArray() ? relays.size() : 0, SHELLY_MAX_CHANNEL + 1);
		if (relaysAreSwitches)
		{
			for (Json::ArrayIndex i = 0; i < relayCount; i++)
			{
				bool ison = false;
				if (!GetBool(Element(relays, i), "ison", ison))
					continue;
				const int id = static_cast<int>(i);
				ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Switch, "switch", id, 0, id, builder.Label("switch", id), std::string());
				if (update)
					update->on = ison;
			}
		}

		const Json::ArrayIndex rollerCount = std::min<Json::ArrayIndex>(rollers.isArray() ? rollers.size() : 0, SHELLY_MAX_CHANNEL + 1);
		for (Json::ArrayIndex i = 0; i < rollerCount; i++)
		{
			const Json::Value& roller = Element(rollers, i);
			// A missing or negative current_pos means the roller is not calibrated: the position is unknown and the state decides,
			// as for a Gen2 cover. The Gen1 state is "open", "close" or "stop" (the direction it moves in, or stopped; then
			// last_direction says where it went last).
			double position = 0.0;
			const bool hasPosition = GetNumber(roller, "current_pos", position) && position >= 0.0;
			int level = -1;
			ShellyBlindState blind = ShellyBlindState::Unknown;
			if (hasPosition)
			{
				level = ClampPercent(position);
				blind = (level >= 100) ? ShellyBlindState::Open : (level <= 0) ? ShellyBlindState::Closed : ShellyBlindState::Partial;
			}
			else
			{
				std::string direction = GetString(roller, "state");
				if (direction == "stop")
					direction = GetString(roller, "last_direction");
				if (direction == "open")
				{
					blind = ShellyBlindState::Open;
					level = 100;
				}
				else if (direction == "close")
				{
					blind = ShellyBlindState::Closed;
					level = 0;
				}
			}
			if (blind == ShellyBlindState::Unknown)
				continue;
			const int id = static_cast<int>(i);
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Blind, "cover", id, 0, id, builder.Label("cover", id), std::string());
			if (update)
			{
				update->level = level;
				update->blindState = blind;
			}
		}

		const Json::ArrayIndex lightCount = std::min<Json::ArrayIndex>(hasLights ? lights.size() : 0, SHELLY_MAX_CHANNEL + 1);
		for (Json::ArrayIndex i = 0; i < lightCount; i++)
		{
			const Json::Value& light = Element(lights, i);
			const std::string mode = GetString(light, "mode");
			bool ison = false;
			if (!GetBool(light, "ison", ison))
				continue;
			const int id = static_cast<int>(i);
			double ignored;
			// A light in color mode is a color light, and so is a bulb in white mode (it reports the color channels too), so a
			// bulb stays the same Domoticz device when it changes mode. A white channel of an RGBW controller (SHRGBW2 in white
			// mode, no color channels reported) is a plain dimmer: the kind per channel is stable, a mode change of the
			// controller switches between its Color device and its Dimmer devices.
			const bool hasColorChannels = GetNumber(light, "red", ignored) && GetNumber(light, "green", ignored) && GetNumber(light, "blue", ignored);
			if (mode == "color" || (mode == "white" && hasColorChannels))
			{
				ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Color, "rgb", id, 0, id, builder.Label("light", id), std::string());
				if (!update)
					continue;
				update->on = ison;
				update->whiteMode = (mode == "white");
				update->rgbw = ShellyGen1HasWhiteChannel(builder.Node().model);
				double value;
				if (!update->whiteMode)
				{
					if (GetNumber(light, "red", value))
						update->red = ClampByte(value);
					if (GetNumber(light, "green", value))
						update->green = ClampByte(value);
					if (GetNumber(light, "blue", value))
						update->blue = ClampByte(value);
				}
				if (update->rgbw)
				{
					update->white = 0;
					if (GetNumber(light, "white", value))
						update->white = ClampByte(value);
				}
				// "gain" is the brightness of the color, "brightness" that of the white
				if (GetNumber(light, update->whiteMode ? "brightness" : "gain", value))
					update->level = ClampPercent(value);
				continue;
			}
			if (id >= SHELLY_CCT_FIRST_SLOT)
			{
				builder.result.unmapped.push_back("light (channel above 7)");
				continue;
			}
			ShellyDeviceUpdate* update = builder.Add(ShellyDeviceKind::Dimmer, "light", id, 0, id, builder.Label("light", id), std::string());
			if (!update)
				continue;
			update->on = ison;
			double brightness;
			if (GetNumber(light, "brightness", brightness))
				update->level = ClampPercent(brightness);
		}

		// meters[i] belongs to relay, roller or light i.
		const char* meterComponent = hasRollers ? "cover" : (hasLights && relayCount == 0) ? "light" : "switch";
		Json::ArrayIndex meterCount = std::min<Json::ArrayIndex>(meters.isArray() ? meters.size() : 0, SHELLY_MAX_CHANNEL + 1);
		if (hasRollers)
			meterCount = std::min(meterCount, rollerCount);
		for (Json::ArrayIndex i = 0; i < meterCount; i++)
		{
			const Json::Value& meter = Element(meters, i);
			if (!IsValidFlag(meter))
				continue;
			MeterReading reading;
			reading.hasPower = GetNumber(meter, "power", reading.power);
			double total;
			if (GetNumber(meter, "total", total))
			{
				reading.hasEnergy = true;
				reading.energyWh = total / 60.0; // meters[].total is in watt-minutes (emeters[].total below is watt-hours)
			}
			const int id = static_cast<int>(i);
			AddMeter(builder, meterComponent, id, 0, id, builder.Label(meterComponent, id), reading, true);
		}

		// emeters[i] are measuring channels (the phases of a 3 phase meter).
		const Json::ArrayIndex emeterCount = std::min<Json::ArrayIndex>(emeters.isArray() ? emeters.size() : 0, SHELLY_MAX_CHANNEL + 1);
		for (Json::ArrayIndex i = 0; i < emeterCount; i++)
		{
			const Json::Value& emeter = Element(emeters, i);
			if (!IsValidFlag(emeter))
				continue;
			MeterReading reading;
			reading.hasPower = GetNumber(emeter, "power", reading.power);
			reading.hasVoltage = GetNumber(emeter, "voltage", reading.voltage);
			reading.hasCurrent = GetNumber(emeter, "current", reading.current);
			reading.hasPowerFactor = GetNumber(emeter, "pf", reading.powerFactor);
			// Unlike meters[].total (watt-minutes, see above), emeters[].total is already in watt-hours.
			reading.hasEnergy = GetNumber(emeter, "total", reading.energyWh);
			reading.hasReturned = GetNumber(emeter, "total_returned", reading.returnedWh); // watt-hours as well
			const int id = static_cast<int>(i);
			const std::string label = (emeterCount == 3 ? "Phase " : "Meter ") + std::to_string(id + 1);
			AddMeter(builder, "emeter", id, 0, id, label, reading, true);
		}

		MapGen1Inputs(builder, Member(status, "inputs"));
		MapGen1Thermostats(builder, Member(status, "thermostats"));
		MapGen1Sensors(builder, status);

		// The device level temperature (top level tmp or temperature), the same single device a Gen2 temperature:0 gives. A node
		// that switches or meters something names it "Device temperature". A thermostat (thermostats[]) already reports its
		// own temperature, so it gets no second one.
		const auto isArrayWithItems = [](const Json::Value& value) { return value.isArray() && !value.empty(); };
		if (isArrayWithItems(relays) || hasRollers || hasLights || isArrayWithItems(meters) || isArrayWithItems(emeters))
			builder.NoteSwitching();
		if (!isArrayWithItems(Member(status, "thermostats")))
		{
			double celsius;
			const Json::Value& tmp = Member(status, "tmp");
			if (tmp.isObject())
			{
				if (IsValidFlag(tmp))
				{
					if (GetNumber(tmp, "tC", celsius))
						builder.NoteDeviceTemperature(celsius);
					else if (GetNumber(tmp, "value", celsius))
						builder.NoteDeviceTemperature((GetString(tmp, "units") == "F") ? (celsius - 32.0) * 5.0 / 9.0 : celsius);
				}
			}
			else if (GetNumber(status, "temperature", celsius))
				builder.NoteDeviceTemperature(celsius);
		}

		double rssi;
		if (GetNumber(Member(status, "wifi_sta"), "rssi", rssi))
			builder.result.rssiLevel = ShellyRssiToLevel(ClampRssi(rssi));

		double battery;
		if (GetNumber(Member(status, "bat"), "value", battery))
			builder.result.batteryLevel = ClampPercent(battery);
	}
} // namespace

ShellyNodeConfig ShellyParseNodeInfo(const ShellyNode& node, const Json::Value& info)
{
	ShellyNodeConfig config;
	if (!info.isObject())
		return config;

	if (ShellyIsRpcGeneration(node.gen))
	{
		const Json::Value& deviceInfo = Member(info, "deviceinfo");
		const Json::Value& deviceConfig = Member(info, "config");

		config.deviceName = ShellySanitizeText(GetString(deviceInfo, "name"));
		if (config.deviceName.empty())
			config.deviceName = ShellySanitizeText(GetString(Member(Member(deviceConfig, "sys"), "device"), "name"));
		config.firmware = ShellySanitizeText(GetString(deviceInfo, "ver"));
		if (config.firmware.empty())
			config.firmware = ShellySanitizeText(GetString(deviceInfo, "fw_id"));

		if (deviceConfig.isObject())
		{
			for (const std::string& key : deviceConfig.getMemberNames())
			{
				std::string type;
				int id = 0;
				if (!SplitComponent(key, type, id))
					continue;
				const std::string name = ShellySanitizeText(GetString(deviceConfig[key], "name"));
				if (!name.empty())
					config.componentNames[key] = name;
			}

			// A battery device that sleeps between reports names its wake-up period in the system configuration.
			const Json::Value& sys = Member(deviceConfig, "sys");
			config.sleepKnown = sys.isObject();
			double period = 0.0;
			if (!GetNumber(sys, "sleep_period", period) || period <= 0.0)
			{
				period = 0.0;
				GetNumber(Member(sys, "sleep"), "wakeup_period", period);
			}
			if (period > 0.0)
			{
				config.sleepy = true;
				config.sleepPeriodSec = static_cast<int>(std::min(period, 2678400.0)); // at most 31 days
			}
		}
	}
	else
	{
		const Json::Value& sleep = Member(info, "dz_sleep");
		config.sleepKnown = GetBool(sleep, "sleepy", config.sleepy);
		if (config.sleepKnown && config.sleepy)
		{
			double period = 0.0;
			if (GetNumber(sleep, "period_sec", period) && period > 0.0)
				config.sleepPeriodSec = static_cast<int>(std::min(period, 2678400.0));
		}
		config.deviceName = ShellySanitizeText(GetString(info, "name"));
		config.firmware = ShellySanitizeText(GetString(info, "fw"));
		config.rollerMode = (GetString(info, "mode") == "roller");

		const Json::Value& relays = Member(info, "relays");
		for (Json::ArrayIndex i = 0; relays.isArray() && i < relays.size() && i <= static_cast<Json::ArrayIndex>(SHELLY_MAX_CHANNEL); i++)
		{
			const std::string name = ShellySanitizeText(GetString(Element(relays, i), "name"));
			if (!name.empty())
				config.componentNames[ComponentKey("switch", static_cast<int>(i))] = name;
		}
		// Not every firmware has a name for a roller; when it does, the cover gets it.
		const Json::Value& rollers = Member(info, "rollers");
		for (Json::ArrayIndex i = 0; rollers.isArray() && i < rollers.size() && i <= static_cast<Json::ArrayIndex>(SHELLY_MAX_CHANNEL); i++)
		{
			const std::string name = ShellySanitizeText(GetString(Element(rollers, i), "name"));
			if (!name.empty())
				config.componentNames[ComponentKey("cover", static_cast<int>(i))] = name;
		}
		const Json::Value& lights = Member(info, "lights");
		for (Json::ArrayIndex i = 0; lights.isArray() && i < lights.size() && i <= static_cast<Json::ArrayIndex>(SHELLY_MAX_CHANNEL); i++)
		{
			const std::string name = ShellySanitizeText(GetString(Element(lights, i), "name"));
			if (!name.empty())
				config.componentNames[ComponentKey("light", static_cast<int>(i))] = name;
		}

		// What each input is configured as. The input array of the device (i3, Uni) wins, else the button type of the
		// relay, light or roller with the same index. A type we do not know leaves the input Unknown (a contact).
		for (int i = 0; i <= 7; i++)
		{
			const Json::ArrayIndex index = static_cast<Json::ArrayIndex>(i);
			std::string type = GetString(Element(Member(info, "inputs"), index), "btn_type");
			if (type.empty())
				type = GetString(Element(relays, index), "btn_type");
			if (type.empty())
				type = GetString(Element(lights, index), "btn_type");
			if (type.empty())
				type = GetString(Element(rollers, index), "btn_type");
			if (type == "toggle" || type == "edge" || type == "detached")
				config.inputKinds[i] = ShellyInputKind::Switch;
			else if (type == "momentary" || type == "momentary_on_release" || type == "action" || type == "cycle")
				config.inputKinds[i] = ShellyInputKind::Button;
		}
	}
	return config;
}

std::string ShellyBaseName(const ShellyNode& node, const ShellyNodeConfig& config)
{
	if (!node.name.empty())
		return node.name;
	if (!config.deviceName.empty())
		return config.deviceName;
	if (!node.deviceId.empty())
		return node.deviceId;
	std::string name = node.model.empty() ? "Shelly" : "Shelly " + node.model;
	if (node.id.size() >= 6)
		name += " " + node.id.substr(node.id.size() - 6);
	return name;
}

std::string ShellyComponentLabel(const ShellyNodeConfig& config, const std::string& type, const int channel)
{
	const auto itt = config.componentNames.find(ComponentKey(type, channel));
	if (itt != config.componentNames.end() && !itt->second.empty())
		return itt->second;
	const char* kind = "Meter";
	if (type == "switch")
		kind = "Switch";
	else if (type == "light")
		kind = "Light";
	else if (type == "cover")
		kind = "Cover";
	else if (type == "rgb" || type == "rgbw")
		kind = "Light";
	else if (type == "thermostat")
		kind = "Thermostat";
	else if (type == "input")
		kind = "Input";
	return std::string(kind) + " " + std::to_string(channel + 1);
}

std::string ShellyInputButtonName(const ShellyNode& node, const ShellyNodeConfig& config, const int channel)
{
	return ShellyBaseName(node, config) + " " + ShellyComponentLabel(config, "input", channel) + " Button";
}

bool ShellyIsGen1BulbModel(const std::string& model)
{
	return model == "SHBLB-1" || model == "SHCB-1" || model == "SHBDUO-1" || model == "SHVIN-1";
}

bool ShellyGen1HasWhiteChannel(const std::string& model)
{
	return model == "SHRGBW2" || model == "SHBLB-1" || model == "SHCB-1";
}

ShellyNodeUpdate ShellyMapStatus(const ShellyNode& node, const Json::Value& status, const ShellyNodeConfig& config)
{
	Builder builder(node, config);
	uint32_t key;
	if (status.isObject() && ShellyNodeKeyFromMac(node.id, key))
	{
		if (ShellyIsRpcGeneration(node.gen))
			MapGen2(builder, status);
		else if (node.gen == SHELLY_GEN1)
			MapGen1(builder, status, config);
		builder.Finish();
	}
	return std::move(builder.result);
}

bool ShellyNodeKeyFromMac(const std::string& mac, uint32_t& key)
{
	if (mac.size() != 12 || mac.find_first_not_of("0123456789ABCDEFabcdef") != std::string::npos)
		return false;
	key = static_cast<uint32_t>(std::strtoul(mac.substr(6).c_str(), nullptr, 16));
	return true;
}

uint32_t ShellyNodeKey(const std::string& mac)
{
	uint32_t key = 0;
	return ShellyNodeKeyFromMac(mac, key) ? key : 0;
}

bool ShellyNodeMatchesKey(const ShellyNode& node, const uint32_t key)
{
	uint32_t nodeKey;
	return ShellyNodeKeyFromMac(node.id, nodeKey) && nodeKey == key;
}

bool ShellyNodeKeysClash(const uint32_t keyA, const uint32_t keyB)
{
	return (keyA == keyB) || ((keyA & 0xFFFF) == (keyB & 0xFFFF));
}

uint8_t ShellyMakeUnit(const ShellyDeviceKind kind, const int slot)
{
	return static_cast<uint8_t>((static_cast<int>(kind) << 4) | (slot & 0x0F));
}

ShellyDeviceKind ShellyUnitKind(const uint8_t unit)
{
	return static_cast<ShellyDeviceKind>(unit >> 4);
}

int ShellyUnitSlot(const uint8_t unit)
{
	return unit & 0x0F;
}

bool ShellyRouteDevice(const uint32_t id, const uint8_t unit, ShellyDeviceRoute& route)
{
	const ShellyDeviceKind kind = ShellyUnitKind(unit);
	switch (kind)
	{
		case ShellyDeviceKind::Switch:
			route.component = ShellyCmd::COMPONENT_SWITCH;
			break;
		case ShellyDeviceKind::Dimmer:
			route.component = ShellyUnitSlot(unit) >= SHELLY_CCT_FIRST_SLOT ? SHELLY_COMPONENT_CCT : ShellyCmd::COMPONENT_LIGHT;
			break;
		case ShellyDeviceKind::Blind:
			route.component = ShellyCmd::COMPONENT_COVER;
			break;
		case ShellyDeviceKind::Color:
			route.component = ShellyCmd::COMPONENT_RGB;
			break;
		case ShellyDeviceKind::Thermostat:
			route.component = ShellyCmd::COMPONENT_THERMOSTAT;
			break;
		default:
			return false;
	}
	route.kind = kind;
	route.nodeKey = id & 0xFFFFFF;
	route.channel = ShellyUnitSlot(unit);
	if (kind == ShellyDeviceKind::Dimmer && route.channel >= SHELLY_CCT_FIRST_SLOT)
		route.channel -= SHELLY_CCT_FIRST_SLOT;
	return true;
}

int ShellyInputSlot(const int channel, const bool button)
{
	if (channel < 0 || channel > 7)
		return -1;
	return button ? 8 + channel : channel;
}

int ShellyReturnedEnergySlot(const int channel)
{
	if (channel < 0 || channel > SHELLY_MAX_RETURNED_CHANNEL)
		return -1;
	return static_cast<int>(ShellySensorRole::ReturnedEnergy) + channel;
}

bool ShellyMapInputEvent(const ShellyNode& node, const Json::Value& event, ShellyInputEvent& out)
{
	if (!ShellyIsRpcGeneration(node.gen))
		return false;
	std::string type;
	int channel = 0;
	if (!SplitComponent(GetString(event, "component"), type, channel) || type != "input")
		return false;

	const std::string name = GetString(event, "event");
	int level = static_cast<int>(ShellyButtonEvent::None);
	if (name == "single_push")
		level = static_cast<int>(ShellyButtonEvent::Single);
	else if (name == "double_push")
		level = static_cast<int>(ShellyButtonEvent::Double);
	else if (name == "triple_push")
		level = static_cast<int>(ShellyButtonEvent::Triple);
	else if (name == "long_push")
		level = static_cast<int>(ShellyButtonEvent::Long);
	const int slot = ShellyInputSlot(channel, true);
	if (level < 0 || slot < 0)
		return false;

	out.channel = channel;
	out.level = level;
	out.unit = ShellyMakeUnit(ShellyDeviceKind::Input, slot);
	return true;
}

int ShellyRssiToLevel(const int dbm)
{
	// -100 dBm and worse is 0, -45 dBm and better is 11.
	const int clamped = std::min(0, std::max(-200, dbm));
	const int level = ((clamped + 100) * 11) / 55;
	return std::min(11, std::max(0, level));
}
