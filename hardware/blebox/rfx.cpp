// Convenience wrapper class for the typical domoticz trio:
// - device type (e.g. pTypeGeneral)
// - subtype (e.g. sTypeColor_RGB_W)
// - switch type (e.g. STYPE_BlindsPercentageInverted)

#include <boost/assert.hpp>
#include <boost/format.hpp>
#include <cstddef>
#include <cstdint>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "../../hardware/hardwaretypes.h"

using BYTE = unsigned char; // for RFXtrx.h
#include "../../main/RFXtrx.h" // for pTypeTEMP (vs useless pTypeTemperature)

#include "../../hardware/ColorSwitch.h"

#include "errors.h"
#include "rfx.h"

blebox::rfx::rfx(uint8_t dev, uint8_t sub, _eSwitchType swt) {
  set(dev, sub, swt);
}

void blebox::rfx::set(uint8_t dev, uint8_t sub, _eSwitchType swt) {
  _devtype = dev;
  _subtype = sub;
  _switchtype = swt;
}

// TODO: rename to "verify_compatible" because user can change the switch type
void blebox::rfx::verify_same_rfx(const rfx &other) const {
  if (other != *this)
    throw errors::rfx_mismatch(to_string(), other.to_string());
}

static auto rfx_majors_string(uint8_t dev, uint8_t sub) {
  using namespace std::literals;
  return std::string(RFX_Type_Desc(dev, 0)) + "/"s +
         RFX_Type_SubType_Desc(dev, sub, HTYPE_BleBox /* means "generic" */);
}

void blebox::rfx::verify_same_rfx(uint8_t dev, uint8_t sub) const {
  if (std::make_tuple(_devtype, _subtype) != std::make_tuple(dev, sub))
    throw errors::rfx_mismatch(rfx_majors_string(_devtype, _subtype),
                               rfx_majors_string(dev, sub));
}

std::string blebox::rfx::to_string() const {
  return (boost::format("%02d/%02d/%s") % int(devtype()) % int(subtype()) %
          Switch_Type_Desc(_eSwitchType(switchtype())))
      .str();
};

static auto from_hex_string(const std::string &hex) {
  using namespace std::literals;
  size_t pos;
  unsigned long value = std::stoul(hex, &pos, 16);
  if (pos != hex.size())
    throw std::invalid_argument("Not a proper node id: "s + hex);

  BOOST_ASSERT(value < std::numeric_limits<uint32_t>::max());
  return uint32_t(value);
}

static auto dev_id_from_hex(const std::string &raw) {
  return uint8_t(from_hex_string(raw) & 0xFF);
}

// Extract a likely "blebox method id" from a value in the database
std::tuple<uint8_t, std::byte>
blebox::rfx::extract_method_and_unit(const std::string &device_id_str,
                                     const std::string &raw_unit_str) {
  // Change this is more general types are supported
  // Otherwise, only allow types actually used (as sanity checks)
  BOOST_ASSERT(devtype() != pTypeGeneral || subtype() == sTypePercentage);

  uint8_t method_id;
  std::byte unit;

  switch (devtype()) {
  case pTypeThermostat:
  case pTypeColorSwitch:
  case pTypeGeneralSwitch:
    unit = decode_unit(raw_unit_str);
    method_id = dev_id_from_hex(device_id_str);
    break;
  case pTypeGeneral: {
    // pTypeGeneral has no unit field, so we store it in device_id
    auto both = from_hex_string(device_id_str);
    unit = std::byte(both & 0xFF);
    method_id = uint8_t((both & 0xFF00) >> 8);
  } break;
  case pTypeTEMP:
    unit = decode_unit(raw_unit_str);
    method_id = uint8_t((std::stoul(device_id_str) & 0xFF00) >> 8);
    break;
  case pTypeAirQuality:
    unit = decode_unit(raw_unit_str);
    method_id = uint8_t(std::stoul(device_id_str));
    break;
  default: {
    std::ostringstream os;
    os << "Posting not implemented for device type: " << int(devtype());
    throw std::runtime_error(os.str());
  }
  }

  return std::make_tuple(method_id, unit);
}

blebox::rfx::dev_info blebox::rfx::record_info_from(const char *raw_data) {
  const auto &header = *reinterpret_cast<const _tGeneralDevice *>(raw_data);
  uint8_t dtype = header.type;

  // TODO: remove this switch somehow
  switch (dtype) {
  case pTypeGeneralSwitch: { // 0xF4
    auto &&data = reinterpret_cast<const _tGeneralSwitch *>(raw_data);
    return std::make_tuple(uint8_t(data->id), data->unitcode,
                      (boost::format("%08X") % int(data->id)).str(), data->type,
                      data->subtype);
  }

  case pTypeColorSwitch: { // 0xF1
    auto &&color = reinterpret_cast<const _tColorSwitch *>(raw_data);
    return std::make_tuple(uint8_t(color->id), color->dunit,
                      (boost::format("%08X") % int(color->id)).str(),
                      color->type, color->subtype);
  }
  case pTypeThermostat: { // 0xF2
    auto &&thermo = reinterpret_cast<const _tThermostat *>(raw_data);
    // TODO: node id ok?
    return std::tie(thermo->id4, thermo->id3, "", thermo->type,
                    thermo->subtype);
  }

  default:
    throw blebox::rfx::unsupported_type(dtype);
  }
}

bool blebox::rfx::operator==(const rfx &other) const {
  static std::set<_eSwitchType> compatible(
      {STYPE_BlindsPercentage, STYPE_BlindsPercentageInverted});

  bool same_majors = std::tie(_devtype, _subtype) ==
                     std::make_tuple(other.devtype(), other.subtype());
  bool same_switches = switchtype() == other.switchtype();

  return same_majors &&
         (same_switches || (compatible.count(switchtype()) &&
                            compatible.count(other.switchtype())));
}

auto blebox::rfx::node_id_str(uint8_t method_id, std::byte unit) const
    -> std::string {
  using namespace std::literals;

  switch (devtype()) {
  case pTypeTEMP:
    return (boost::format("%d") %
            (int(method_id) * 256 + encode_general_dev_unit(unit)))
        .str();

  case pTypeThermostat:
    return (boost::format("%05X%02X") % int(encode_general_dev_unit(unit)) %
            int(method_id))
        .str();

  case pTypeAirQuality:
    return (boost::format("%d") % int(method_id)).str();

  case pTypeGeneralSwitch:
  case pTypeColorSwitch:
    return (boost::format("%08X") % int(method_id)).str();

  case pTypeGeneral:
    return (boost::format("%08X") % (int(method_id) * 256 + int(unit))).str();

  default:
    throw std::runtime_error("node_id_str() not implemented for "s +
                             std::to_string(int(method_id)) + "["s +
                             std::to_string(int(unit)) + "]");
  }
}

blebox::rfx::unsupported_type::unsupported_type(uint8_t dtype)
    : std::runtime_error((boost::format("Unsupported type: %d (%s)") %
                          int(dtype) % RFX_Type_Desc(dtype, 1))
                             .str()) {}

// called with member value from RFX structure, e.g. _tGeneralSwitch.unitcode
std::byte blebox::rfx::decode_unit(unsigned long value) {
  if (value == 0xFF)
    return std::byte(0xFF);

  BOOST_ASSERT(value >= 0xA0 && value <= 0xAF);
  return std::byte(value - 0xA0);
}

// called with raw string value from DB (int in schema, but there's no ORM)
std::byte blebox::rfx::decode_unit(const std::string &db_value) {
  int value = std::stoi(db_value);
  if (value == 0xFF)
    return std::byte(0xFF);

  // severly limit allowed values for now to detect bugs faster
  BOOST_ASSERT(value >= 0xA0 && value <= 0xAF);
  return std::byte(value - 0xA0);
}

// value to store in database
uint8_t blebox::rfx::encode_unit(std::byte unit, uint8_t subtype) {
  if (subtype == sTypePercentage)
    return 1; // TODO: this is the case for any pTypeGeneral

  if (unit == std::byte(0xFF))
    return 0xFF;

  // severly limit allowed values for now to detect bugs faster
  BOOST_ASSERT(int(unit) < 0x10);
  return unit == std::byte(0xFF) ? 0xFF : 0xA0 + uint8_t(unit);
}

// value to store in send_* structures
uint8_t blebox::rfx::encode_general_dev_unit(std::byte unit) {
  if (unit == std::byte(0xFF))
    return 0xFF;

  // severly limit allowed values for now to detect bugs faster
  BOOST_ASSERT(int(unit) < 0x10);
  return unit == std::byte(0xFF) ? 0xFF : 0xA0 + uint8_t(unit);
}

bool blebox::rfx::is_self_method(std::byte unit) {
  return unit == self_method_id;
}
