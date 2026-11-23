#include "stdafx.h"
#include "ShellyCoap.h"
#include <cctype>
#include <cmath>
#include <memory>
#include <json/json.h>

namespace
{
	constexpr unsigned OPT_URI_PATH = 11;
	constexpr unsigned OPT_COIOT_DEVICE = 3332;

	// Reads an option delta or length nibble with its extension bytes. False when truncated or reserved (15).
	bool ReadExtended(const uint8_t* data, const size_t size, size_t& pos, const unsigned nibble, unsigned& value)
	{
		if (nibble < 13)
		{
			value = nibble;
			return true;
		}
		if (nibble == 13)
		{
			if (pos + 1 > size)
				return false;
			value = 13u + data[pos++];
			return true;
		}
		if (nibble == 14)
		{
			if (pos + 2 > size)
				return false;
			value = 269u + ((static_cast<unsigned>(data[pos]) << 8) | data[pos + 1]);
			pos += 2;
			return true;
		}
		return false;
	}
} // namespace

bool ShellyCoapParse(const uint8_t* data, const size_t size, ShellyCoapMessage& message)
{
	message = ShellyCoapMessage();
	if (data == nullptr || size < 4 || size > SHELLY_COAP_MAX_DATAGRAM)
		return false;
	if ((data[0] >> 6) != 1) // version
		return false;
	message.type = static_cast<ShellyCoapType>((data[0] >> 4) & 0x03);
	const unsigned tokenLength = data[0] & 0x0F;
	if (tokenLength > 8)
		return false;
	message.code = data[1];
	message.messageId = static_cast<uint16_t>((data[2] << 8) | data[3]);
	size_t pos = 4;
	if (pos + tokenLength > size)
		return false;
	message.token.assign(reinterpret_cast<const char*>(data + pos), tokenLength);
	pos += tokenLength;

	unsigned optionNumber = 0;
	size_t optionCount = 0;
	while (pos < size && data[pos] != 0xFF)
	{
		if (++optionCount > SHELLY_COAP_MAX_OPTIONS)
			return false;
		const unsigned deltaNibble = data[pos] >> 4;
		const unsigned lengthNibble = data[pos] & 0x0F;
		++pos;
		unsigned delta = 0, length = 0;
		if (!ReadExtended(data, size, pos, deltaNibble, delta) || !ReadExtended(data, size, pos, lengthNibble, length))
			return false;
		optionNumber += delta;
		if (optionNumber > 65535 || pos + length > size)
			return false;
		const std::string value(reinterpret_cast<const char*>(data + pos), length);
		pos += length;
		switch (optionNumber)
		{
		case OPT_URI_PATH:
			message.path += "/" + value;
			break;
		case OPT_COIOT_DEVICE:
			message.deviceId = value;
			break;
		default:
			break;
		}
	}
	if (pos < size)
	{
		++pos; // payload marker
		if (pos >= size) // a marker without a payload is a format error
			return false;
		message.payload.assign(reinterpret_cast<const char*>(data + pos), size - pos);
	}
	return true;
}

std::vector<uint8_t> ShellyCoapBuildAck(const uint16_t messageId)
{
	return { 0x60, 0x00, static_cast<uint8_t>(messageId >> 8), static_cast<uint8_t>(messageId & 0xFF) };
}

std::vector<uint8_t> ShellyCoapBuildReset(const uint16_t messageId)
{
	return { 0x70, 0x00, static_cast<uint8_t>(messageId >> 8), static_cast<uint8_t>(messageId & 0xFF) };
}

bool ShellyCoiotParseStatus(const std::string& payload, std::vector<ShellyCoiotSensor>& sensors)
{
	sensors.clear();
	if (payload.empty() || payload.size() > SHELLY_COAP_MAX_DATAGRAM)
		return false;
	try
	{
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		builder["stackLimit"] = 8;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		Json::Value root;
		if (!reader->parse(payload.data(), payload.data() + payload.size(), &root, nullptr))
			return false;
		if (!root.isObject() || !root["G"].isArray())
			return false;
		for (const Json::Value& entry : root["G"])
		{
			if (!entry.isArray() || entry.size() != 3 || !entry[0].isNumeric() || !entry[1].isNumeric() || !entry[2].isNumeric())
				return false;
			ShellyCoiotSensor sensor;
			const double ch = entry[0].asDouble();
			const double id = entry[1].asDouble();
			sensor.value = entry[2].asDouble();
			if (!std::isfinite(ch) || !std::isfinite(id) || ch < 0 || ch > 65535 || id < 0 || id > 65535)
				return false;
			sensor.channel = static_cast<int>(ch);
			sensor.id = static_cast<int>(id);
			sensors.push_back(sensor);
		}
		return true;
	}
	catch (...)
	{
		sensors.clear();
		return false;
	}
}

std::string ShellyCoiotMacFromDeviceId(const std::string& deviceId)
{
	const size_t first = deviceId.find('#');
	if (first == std::string::npos)
		return std::string();
	size_t second = deviceId.find('#', first + 1);
	if (second == std::string::npos)
		second = deviceId.size();
	std::string mac = deviceId.substr(first + 1, second - first - 1);
	if (mac.size() != 6 && mac.size() != 12)
		return std::string();
	for (char& c : mac)
	{
		if (!std::isxdigit(static_cast<unsigned char>(c)))
			return std::string();
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}
	return mac;
}

bool ShellyCoiotMacMatches(const std::string& frameMac, const std::string& nodeMac)
{
	if ((frameMac.size() != 6 && frameMac.size() != 12) || nodeMac.size() != 12)
		return false;
	return nodeMac.compare(nodeMac.size() - frameMac.size(), frameMac.size(), frameMac) == 0;
}
