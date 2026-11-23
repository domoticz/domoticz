#pragma once

// Minimal, pure CoAP (RFC 7252) message parser for the Shelly Gen1 CoIoT status frames.
//
// A CoIoT v2 device publishes its status as a request to /cit/s with a JSON payload
// {"G":[[channel,id,value],...]}. The Shelly Gen1 documentation specifies a non-confirmable request with the
// non-standard CoAP code 0.30 (0x1E); a standard POST (0.02) is accepted as well. Besides the standard options it
// adds private ones, of which only this one is used:
//   3332 device id  ("SHSW-25#AABBCC#1": model, MAC (last 3 or all 6 bytes), protocol version)
// Nothing here touches a socket; every function is bounded and never throws.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

constexpr size_t SHELLY_COAP_MAX_DATAGRAM = 1500; // larger datagrams are rejected
constexpr size_t SHELLY_COAP_MAX_OPTIONS = 32;
constexpr int SHELLY_COIOT_DEFAULT_PORT = 5683;
constexpr uint8_t SHELLY_COAP_CODE_POST = 0x02;          // 0.02
constexpr uint8_t SHELLY_COAP_CODE_COIOT_STATUS = 0x1E; // 0.30, non-standard code of a CoIoT status publish

enum class ShellyCoapType
{
	Confirmable = 0,
	NonConfirmable = 1,
	Acknowledgement = 2,
	Reset = 3
};

struct ShellyCoapMessage
{
	ShellyCoapType type = ShellyCoapType::NonConfirmable;
	uint8_t code = 0; // class << 5 | detail
	uint16_t messageId = 0;
	std::string token;
	std::string path;     // Uri-Path segments joined with '/', with a leading '/'
	std::string deviceId; // option 3332, empty when absent
	std::string payload;

	// True for the codes a status frame is sent with: the Shelly specific 0.30 and the standard POST 0.02.
	bool HasStatusCode() const
	{
		return code == SHELLY_COAP_CODE_COIOT_STATUS || code == SHELLY_COAP_CODE_POST;
	}
	bool IsStatusFrame() const
	{
		return HasStatusCode() && path == "/cit/s";
	}
};

// Parses one datagram. Returns false (and a message in unspecified state) for anything malformed, oversized or
// carrying a reserved/truncated field.
bool ShellyCoapParse(const uint8_t* data, size_t size, ShellyCoapMessage& message);

// The empty ACK (4 bytes) answering a confirmable message.
std::vector<uint8_t> ShellyCoapBuildAck(uint16_t messageId);

// The empty RST (4 bytes) rejecting a confirmable message that is not understood.
std::vector<uint8_t> ShellyCoapBuildReset(uint16_t messageId);

// One entry of the "G" array of a status frame.
struct ShellyCoiotSensor
{
	int channel = 0; // block index
	int id = 0;      // sensor id as described by /cit/d
	double value = 0;
};

// Validates a status payload ({"G":[[block,id,value],...]}) and extracts the entries. Returns false unless the
// payload is a JSON object with a "G" array of numeric triples. Never throws.
bool ShellyCoiotParseStatus(const std::string& payload, std::vector<ShellyCoiotSensor>& sensors);

// The MAC part of a device id option ("SHSW-25#AABBCC#1" gives "AABBCC"), upper case; empty when malformed.
std::string ShellyCoiotMacFromDeviceId(const std::string& deviceId);

// True when `frameMac` (3 or 6 bytes of a MAC in hex, as sent by the device) belongs to the node MAC `nodeMac`
// (normalized: 12 upper case hex digits). The frame carries the trailing bytes of the MAC on most models.
bool ShellyCoiotMacMatches(const std::string& frameMac, const std::string& nodeMac);
