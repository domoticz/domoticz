#include <boost/format.hpp>

#include <cmath>
#include <utility>

#include "../../../hardware/hardwaretypes.h"

typedef unsigned char BYTE; // for RFXtrx.h
#include "../../../main/RFXtrx.h" // for pTypeTEMP (vs useless pTypeTemperature)

#include "../../../main/SQLHelper.h"

#include "../box.h"
#include "../errors.h"
#include "../session.h"

#include "../../BleBox.h"

#include "widget.h"

namespace blebox {
const uint8_t sTypeTEMPBleBox = sTypeTEMP5;
const uint8_t sTypeBleBoxAirSensor = sTypeVoltcraft;
}

// TODO: use or merge with device_state class
blebox::widgets::widget::widget(std::byte unit, uint8_t method_id,
                                std::string field, std::string title,
                                const box &parent,
                                api_path_maker_type api_path_maker,
                                std::string post_field)
    : _parent(parent),
      _unit(unit),
      _method_id(method_id),
      _field(std::move(field)),
      _title(std::move(title)),
      _post_field(std::move(post_field)),
      _api_path_maker(std::move(api_path_maker)) {}

std::string blebox::widgets::widget::description() const {
  std::ostringstream os;
  os << parent().name() << ".";

  if (rfx::is_self_method(unit()))
    os << title();
  else
    os << int(unit()) << "." << title();

  return os.str();
}

template <typename T> static T assert_range(int value) {
  BOOST_ASSERT(value <= std::numeric_limits<T>::max());
  BOOST_ASSERT(value >= std::numeric_limits<T>::min());
  return T(value);
}

void blebox::widgets::widget::save(int hw_id) {
  std::string device_id = rfx().node_id_str(method_id(), unit());

  // TODO: use an ORM
  auto &&results =
      m_sql.safe_query("SELECT Type, SubType, SwitchType FROM DeviceStatus "
                       "WHERE HardwareID=%d AND DeviceID=%Q AND Unit=%d",
                       hw_id, device_id.c_str(), encoded_unit());

  if (results.size() > 1)
    throw std::runtime_error("too many database records for given unit/method");

  if (results.empty()) {
    const int used = 0;
    int nValue = 0;
    m_sql.InsertDevice(hw_id, device_id.c_str(), encoded_unit(),
                       rfx().devtype(), rfx().subtype(), rfx().switchtype(),
                       nValue, "(added)", description(), signal_level(),
                       battery_level(), used);
    return;
  }

  // Exists, but make sure it's the same or we'll get multiple records
  auto &&fields = results[0];
  auto db_type = assert_range<uint8_t>(std::stoi(fields[0]));
  auto db_subtype = assert_range<uint8_t>(std::stoi(fields[1]));
  auto db_switch_type =
      _eSwitchType(assert_range<uint8_t>(std::stoi(fields[2])));
  _rfx.verify_same_rfx({db_type, db_subtype, db_switch_type});
}

namespace blebox {

struct _tTemperature : public decltype(RBUF::TEMP) {
  _tTemperature() { packetlength = sizeof(decltype(*this)) - 1; }
};
} // namespace blebox

const int default_subtype = 0; // sTypeAC

void blebox::widgets::widget::send_general_switch(BleBox &bb, bool on,
                                                  uint8_t level) const {
  uint8_t packettype = pTypeGeneralSwitch;

  // skip update if nothing changed
  int nvalue;
  std::string svalue;
  bool has_values = blebox::db::device_status::get_values(
      nvalue, svalue, bb.hw_id(), std::byte(method_id()), encoded_unit(),
      packettype, default_subtype);
  if (has_values && (on == (nvalue != gswitch_sOff)) &&
      (svalue == std::to_string(int(level))))
    return;

  // TODO: move to rfx
  _tGeneralSwitch gswitch;
  gswitch.id = method_id();
  gswitch.subtype = sSwitchGeneralSwitch; // TODO: check compatibility?
  gswitch.unitcode = encoded_unit();
  gswitch.cmnd = on ? (level ? gswitch_sSetLevel : gswitch_sOn) : gswitch_sOff;
  gswitch.level = level;
  gswitch.rssi = signal_level();

  _rfx.verify_same_rfx({gswitch.type, gswitch.subtype, rfx().switchtype()});
  DecodeRX(bb, description(), &gswitch);
}

void blebox::widgets::widget::send_percentage(BleBox &bb,
                                              uint8_t percentage) const {
  _tGeneralDevice gDevice;
  gDevice.subtype = sTypePercentage;

  // NOTE: gDevice does not have a unit field, so we store it here
  gDevice.intval1 =
      (int(method_id()) << 8) | rfx::encode_general_dev_unit(unit());

  gDevice.floatval1 = float(percentage);

  _rfx.verify_same_rfx({gDevice.type, gDevice.subtype});
  DecodeRX(bb, description(), &gDevice);
}

void blebox::widgets::widget::send_temperature(BleBox &bb,
                                               const int16_t &u16_temp) const {
  blebox::_tTemperature temp;
  temp.packettype = pTypeTEMP;

  temp.subtype = blebox::sTypeTEMPBleBox;

  temp.seqnbr = 0; // ?

  temp.id1 = method_id();
  temp.id2 = encoded_unit();

  temp.rssi = 0xF;          // 4 bits
  temp.battery_level = 0xF; // 4 bits

  temp.tempsign = (u16_temp >= 0) ? 0 : 1;
  auto at10 = uint16_t(std::abs(std::round(u16_temp / 10.0F)));
  temp.temperatureh = (at10 >> 8);
  temp.temperaturel = at10 & 0xFF;

  _rfx.verify_same_rfx({temp.packettype, temp.subtype});
  DecodeRX(bb, description(), &temp);
}

// TODO: move to rfx class?
void blebox::widgets::widget::send_rgbw(BleBox &bb, uint32_t color, uint8_t level) const {
  uint8_t r = (color & 0xFF000000U) >> 24U;
  uint8_t g = (color & 0x00FF0000U) >> 16U;
  uint8_t b = (color & 0x0000FF00U) >> 8U;
  uint8_t w = color & 0x000000FFU;

  _tColorSwitch lcmd;
  lcmd.id = method_id();
  lcmd.command = color == 0 ? Color_LedOff : Color_SetColor;
  lcmd.dunit = encoded_unit();
  lcmd.value = level;
  lcmd.color.r = r;
  lcmd.color.g = g;
  lcmd.color.b = b;
  lcmd.color.ww = w;
  lcmd.color.mode = ColorModeCustom;

  // switch type isn't used during send (given record exists)
  _rfx.verify_same_rfx({lcmd.type, lcmd.subtype, rfx().switchtype()});
  DecodeRX(bb, description(), &lcmd);
}

void blebox::widgets::widget::send_thermo(BleBox &bb,
                                          const float &value) const {
  _tThermostat thermos;
  thermos.subtype = rfx().subtype();
  thermos.id1 = 0;
  thermos.id4 = method_id();
  thermos.temp = value;

  // NOTE: unit is ignored during SetSetPoint (hardcoded, set to "1"), so we
  // store it in the ID
  thermos.dunit =
      encoded_unit(); // NOTE: ignored by SetSetPoint (will be set to 1)
  thermos.id3 = encoded_unit(); // store unit in id3, since dunit is clobbered

  _rfx.verify_same_rfx({thermos.type, thermos.subtype});
  DecodeRX(bb, description(), &thermos);
}

void blebox::widgets::widget::send_air(BleBox &bb, const int32_t &level) const {
  _tAirQualityMeter meter;
  meter.subtype = rfx().subtype();
  meter.id2 = encoded_unit();
  meter.id1 = method_id();
  meter.airquality = level;

  _rfx.verify_same_rfx({meter.type, meter.subtype});
  DecodeRX(bb, description(), &meter);
}

void blebox::widgets::widget::DecodeRX(
    BleBox &bb, const std::string &name,
    const blebox::_tTemperature *buffer) const {
  auto cmd = reinterpret_cast<const uint8_t *>(buffer);
  BOOST_ASSERT(cmd[1] == pTypeTEMP);
  bb.InternalDecodeRX(name, cmd, sizeof(*buffer));
}

void blebox::widgets::widget::DecodeRX(BleBox &bb, const std::string &name,
                                       const _tColorSwitch *buffer) const {
  auto cmd = reinterpret_cast<const uint8_t *>(buffer);
  BOOST_ASSERT(cmd[1] == pTypeColorSwitch);
  bb.InternalDecodeRX(name, cmd, sizeof(*buffer));
}

void blebox::widgets::widget::DecodeRX(BleBox &bb, const std::string &name,
                                       const _tAirQualityMeter *buffer) const {
  auto cmd = reinterpret_cast<const uint8_t *>(buffer);
  BOOST_ASSERT(cmd[1] == pTypeAirQuality);
  bb.InternalDecodeRX(name, cmd, sizeof(*buffer));
}

void blebox::widgets::widget::DecodeRX(BleBox &bb, const std::string &name,
                                       const _tGeneralDevice *buffer) const {
  auto cmd = reinterpret_cast<const uint8_t *>(buffer);
  BOOST_ASSERT(cmd[1] == pTypeGeneral);
  bb.InternalDecodeRX(name, cmd, sizeof(*buffer));
}

void blebox::widgets::widget::DecodeRX(BleBox &bb, const std::string &name,
                                       const _tThermostat *buffer) const {
  auto cmd = reinterpret_cast<const uint8_t *>(buffer);
  BOOST_ASSERT(cmd[1] == pTypeThermostat);
  bb.InternalDecodeRX(name, cmd, sizeof(*buffer));
}

void blebox::widgets::widget::DecodeRX(BleBox &bb, const std::string &name,
                                       const _tGeneralSwitch *buffer) const {
  auto cmd = reinterpret_cast<const uint8_t *>(buffer);
  BOOST_ASSERT(cmd[1] == pTypeGeneralSwitch);
  bb.InternalDecodeRX(name, cmd, sizeof(*buffer));
}

Json::Value
blebox::widgets::widget::fetch_or_post(blebox::session &session,
                                       const Json::Value &value) const {
#if __cplusplus >= 201703L
  auto [method, path] = api_method_path(*this, value.asString());
#else
  std::string method;
  std::string path;
  std::tie(method, path) = api_method_path(*this, value.asString());
#endif

  if (method == "GET")
    return session.fetch(path);
  else if (method == "POST") {
    Json::Value form;
    box::set_form_field(form, post_field(), value);
    return session.post_json(path, form);
  }

  using namespace std::literals;
  throw std::runtime_error("Unknown HTTP method for '"s + std::string(path) +
                           "': "s + method);
}

void blebox::widgets::widget::verify_response_value_matches(
    const Json::Value &expected, const Json::Value &root) {

  // special case, pretty much just for airSensor.kick api method
  if(root["_blebox.domoticz.info"] == "empty response")
    return;

  const auto &actual = box::follow(root, post_field());

  if (actual != expected) {
    std::ostringstream os;
    os << "failed to set value for " << field()
       << "(expected: " << expected.asString() << ", got: " << actual.asString()
       << ")";
    throw std::runtime_error(os.str());
  }
}

Json::Value blebox::widgets::widget::post_raw_rfx(
    blebox::session & /*session*/, const char * /*raw_data*/,
    const blebox::db::device_status * /*record*/) {
  throw std::runtime_error("post not implemented");
}

uint8_t blebox::widgets::widget::encoded_unit() const {
  return rfx::encode_unit(unit(), rfx().subtype());
}
