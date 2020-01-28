#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assert.hpp>
#include <boost/format.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>

#include "../../json/config.h"
#include "../../json/value.h"

using BYTE = unsigned char; // for RFXtrx.h
#include "../../main/RFXtrx.h" // for pTypeTEMP

#include "../../hardware/ColorSwitch.h"

#include "../hardwaretypes.h"

#include "box.h"
#include "db/device_status.h"
#include "rfx.h"
#include "session.h"

#include "widgets.h"

namespace blebox::widgets::posting {
// TODO: pass widget as first param?
path_maker not_implemented() {
  return [](const widgets::widget &,
            std::string) -> std::tuple<std::string, std::string> {
    throw std::runtime_error("POST not implemented");
  };
}

// used by gateBox
path_maker simple_get(const std::string &path) {
  return [path](const widgets::widget &, std::string) {
    return std::make_tuple("GET", path);
  };
}

} // namespace blebox::widgets::posting

// {{{ on_off_percentage_sliders
blebox::rfx blebox::widgets::on_off_percentage_slider::get_rfx() const {
  return blebox::rfx(pTypeGeneralSwitch, sSwitchGeneralSwitch,
                     STYPE_BlindsPercentage);
}

blebox::widgets::on_off_percentage_slider::expect_int_fn
blebox::widgets::on_off_percentage_slider::expect_value_fn() const {
  return &box::expect_int;
}

Json::Value
blebox::widgets::on_off_percentage_slider::as_json_type(int value) const {
  return Json::Value(value);
};

void blebox::widgets::on_off_percentage_slider::fetch(
    BleBox &bb, const Json::Value &data) const {
  auto raw_value =
      std::invoke(expect_value_fn(), parent(), data, field(), max(), 0);

  // TODO: sometimes status is set from "Opened" (correct) to "On" (incorrect)
  auto percentage = uint8_t(raw_value * 100 / max());
  send_general_switch(bb, percentage > 0, percentage);
}

Json::Value blebox::widgets::on_off_percentage_slider::post_raw_rfx(
    blebox::session &session, const char *raw_data,
    const blebox::db::device_status * /*record*/) {
  auto &&data = reinterpret_cast<const _tGeneralSwitch *>(raw_data);

  if (data->level > 100)
    throw std::out_of_range("level parameter is out of range");

  int blebox_level;
  if (data->cmnd == gswitch_sOn)
    blebox_level = max();
  else if (data->cmnd == gswitch_sOff)
    blebox_level = 0;
  else
    blebox_level = data->level * max() / 100;

  BOOST_ASSERT(blebox_level <= max());

  auto value = as_json_type(blebox_level);
  auto root = fetch_or_post(session, value);
  verify_response_value_matches(value, root);
  return root;
}

blebox::rfx blebox::widgets::dimmer::get_rfx() const {
  return blebox::rfx(pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Dimmer);
}

blebox::widgets::on_off_percentage_slider::expect_int_fn
blebox::widgets::small_hex_string::expect_value_fn() const {
  return &box::expect_hex_int;
}

Json::Value blebox::widgets::small_hex_string::as_json_type(int value) const {
  return (boost::format("%X") % value).str();
};

// }}}
// {{{ push_button
blebox::rfx blebox::widgets::push_button::get_rfx() const {
  return blebox::rfx(pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_PushOn);
}

Json::Value blebox::widgets::push_button::post_raw_rfx(
    blebox::session &session, const char * /*raw_data*/,
    const blebox::db::device_status * /*record*/) {
  return fetch_or_post(session, "");
}
// }}}
// {{{ switch_onoff
blebox::rfx blebox::widgets::switch_onoff::get_rfx() const {
  return blebox::rfx(pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_OnOff);
}

void blebox::widgets::switch_onoff::fetch(BleBox &bb,
                                          const Json::Value &data) const {
  auto value = uint8_t(parent().expect_int(data, field(), 1));
  const bool on = value == 1;
  send_general_switch(bb, on, 0);
}

Json::Value blebox::widgets::switch_onoff::post_raw_rfx(
    blebox::session &session, const char *raw_data,
    const blebox::db::device_status * /*record*/) {
  auto &&data = reinterpret_cast<const _tGeneralSwitch *>(raw_data);

  int state = (data->cmnd == gswitch_sOn) ? 1 : 0;
  auto root = fetch_or_post(session, state);
  verify_response_value_matches(state, root);
  return root;
}
// }}}
// {{{ percentage
blebox::rfx blebox::widgets::percentage::get_rfx() const {
  return blebox::rfx(pTypeGeneral, sTypePercentage);
}

void blebox::widgets::percentage::fetch(BleBox &bb,
                                        const Json::Value &data) const {
  // TODO: workaround for gateBox having different responses
  // TODO: make this a feature available in all fetch() methods?
  std::vector<std::string> paths;
  boost::split(paths, field(), boost::is_any_of("|"));

  auto last = paths.back();
  paths.pop_back();
  for (auto &&path : paths) {
    try {
      auto value = uint8_t(parent().expect_int(data, path, 100));
      send_percentage(bb, value);
      return;
    } catch (box::jpath_failed &) {}
  }

  auto value = uint8_t(parent().expect_int(data, last, 100));
  send_percentage(bb, value);
}
// }}}
// {{{ color_switch
blebox::rfx blebox::widgets::color_switch::get_rfx() const {
  return blebox::rfx(pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer);
}

void blebox::widgets::color_switch::fetch(BleBox &bb,
                                          const Json::Value &data) const {
  const std::string currentColor =
      parent().expect_nonempty_string(data, field());

  auto color = uint32_t(strtoul(currentColor.c_str(), nullptr, 16));
  send_rgbw(bb, color);
}

static auto to_blebox_color(const _tColor &color) {
  auto &&hex = boost::format("%02x%02x%02x%02x");
  auto &&fmt = hex % int(color.r) % int(color.g) % int(color.b) % int(color.ww);
  return fmt.str();
}

Json::Value blebox::widgets::color_switch::post_raw_rfx(
    blebox::session &session, const char *raw_data,
    const blebox::db::device_status *record) {
  using namespace std::literals;

  auto &&data = reinterpret_cast<const _tColorSwitch *>(raw_data);
  BOOST_ASSERT(100 >= data->value);

  std::string command_or_color;
  std::string expected_color;

  switch (data->command) {
  case Color_LedOn: {
    command_or_color = "on";
    expected_color = to_blebox_color(_tColor(record->color()));
  } break;

  case Color_LedOff: {
    command_or_color = "off";
    expected_color = "00000000";
  } break;

  case Color_SetColor: { // 10
    _tColor col = data->color;
    BOOST_ASSERT(data->value <= 100);
    col.ww = uint8_t(data->value * 255 / 100);
    expected_color = command_or_color = to_blebox_color(col);
  } break;

  case Color_SetBrightnessLevel: { // 15
    const uint8_t raw_brightness = uint8_t(data->value) * 255 / 100;
    uint32_t new_raw_color =
        adjust_rgbw_brightness(_tColor(record->color()), raw_brightness);
    auto &&new_color = (boost::format("%08x") % new_raw_color).str();

    expected_color = command_or_color = new_color;
  } break;

  default:
    throw std::runtime_error("Unsupported color command: "s +
                             std::to_string(data->command));
  }

  auto root = fetch_or_post(session, command_or_color);
  verify_response_value_matches(expected_color, root);
  return root;
}
// }}}
// {{{ temperature
blebox::rfx blebox::widgets::temperature::get_rfx() const {
  return blebox::rfx(pTypeTEMP, sTypeTEMPBleBox);
}

void blebox::widgets::temperature::fetch(BleBox &bb,
                                         const Json::Value &data) const {
  auto value = int16_t(parent().expect_int(data, field(), 12500, -5500));
  send_temperature(bb, value);
}

// }}}
// {{{ thermostat
blebox::rfx blebox::widgets::thermostat::get_rfx() const {
  return blebox::rfx(pTypeThermostat, sTypeThermSetpoint);
}

void blebox::widgets::thermostat::fetch(BleBox &bb,
                                        const Json::Value &data) const {
  // TODO: obtain min/max from fv?
  auto value = int16_t(parent().expect_int(data, field(), 12500, -5500));

  send_thermo(bb, value / 100.0F);
}

Json::Value blebox::widgets::thermostat::post_raw_rfx(
    blebox::session &session, const char *raw_data,
    const blebox::db::device_status * /*record*/) {
  using namespace std::literals;

  auto &&thermo = reinterpret_cast<const _tThermostat *>(raw_data);
  auto blebox_temp = int(std::round(thermo->temp * 100.0F));
  auto blebox_temp_str = std::to_string(blebox_temp);

  // TODO: obtain min/max from fv?
  if (blebox_temp >= 12500 || blebox_temp <= -5500)
    throw std::out_of_range("Raw temperature value is out of range:"s +
                            blebox_temp_str);

  auto root = fetch_or_post(session, blebox_temp);
  verify_response_value_matches(blebox_temp, root);
  return root;
}

// }}}
// {{{ air_reading
blebox::rfx blebox::widgets::air_reading::get_rfx() const {
  return blebox::rfx(pTypeAirQuality, sTypeBleBoxAirSensor);
}

std::string blebox::widgets::air_reading::description() const {
  // Ignore unit since title has better field name ("pm-X.value")
  using namespace std::literals;
  return parent().name() + "."s + title();
}

void blebox::widgets::air_reading::fetch(BleBox &bb,
                                         const Json::Value &data) const {
  const auto &value = box::follow(data, field());

  // TODO: report error?
  if (value.isNull())
    return;

  send_air(bb, parent().check_int(value, field(), 3000, 0));
}

// }}}
