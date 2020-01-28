#pragma once

#include <functional>
#include <string>

#include "../../../json/forwards.h"
#include "../db/device_status.h"

#include "../rfx.h"

class BleBox;

struct _tColor;
struct _tColorSwitch;
struct _tAirQualityMeter;
struct _tGeneralDevice;
struct _tThermostat;
struct _tGeneralSwitch;

namespace blebox {
struct _tTemperature;

extern const uint8_t sTypeTEMPBleBox;
extern const uint8_t sTypeBleBoxAirSensor;
} // namespace blebox

namespace blebox {
class box;
class session;
} // namespace blebox

namespace blebox::widgets {
class widget {
public:
  // TODO: rename to include method
  using api_path_maker_type =
      std::function<std::tuple<std::string, std::string>(const widget &,
                                                         std::string)>;

  widget(std::byte unit, uint8_t method_id, std::string field,
         std::string title, const box &parent,
         api_path_maker_type api_path_maker, std::string post_field);
  virtual ~widget() = default;

  void setup() { _rfx = get_rfx(); }

  void save(int hw_id);
  virtual void fetch(BleBox &bb, const Json::Value &data) const = 0;
  virtual Json::Value post_raw_rfx(blebox::session &session,
                                   const char *raw_data,
                                   const blebox::db::device_status *record);

  std::byte unit() const { return _unit; }
  uint8_t encoded_unit() const;
  uint8_t method_id() const { return _method_id; }

  const blebox::rfx &rfx() const { return _rfx; }

  const std::string &field() const { return _field; }
  const std::string &title() const { return _title; }

  const std::string &post_field() const { return _post_field; }

  // TODO: rename to "default name"
  virtual std::string description() const;
  const box &parent() const { return _parent; }

protected:
  void send_percentage(BleBox &bb, uint8_t percentage) const;
  void send_temperature(BleBox &bb, const int16_t &u16_temp) const;
  void send_rgbw(BleBox &bb, uint32_t color, uint8_t level) const;
  void send_thermo(BleBox &bb, const float &value) const;
  void send_air(BleBox &bb, const int32_t &level) const;
  void send_general_switch(BleBox &bb, bool is_on, uint8_t percentage) const;

  Json::Value fetch_or_post(blebox::session &session,
                            const Json::Value &value) const;

  auto api_method_path(const widget &widget, const std::string &state) const {
    return _api_path_maker(widget, state);
  }


  void verify_response_value_matches(const Json::Value &jvalue,
                                     const Json::Value &root);

private:
  const box &_parent;

  std::byte _unit;
  uint8_t _method_id;

  std::string _node_id_str;

  std::string _field;
  std::string _title;

  std::string _post_field;

  auto battery_level() const -> uint8_t { return 255; }
  auto signal_level() const -> uint8_t { return 12; }

  blebox::rfx _rfx;

  api_path_maker_type _api_path_maker;

  // type safe versions of sDecodeRXMessage
  // TODO: move to cpp unit due to header dependency

  void DecodeRX(BleBox &bb, const std::string &default_name,
                const _tTemperature *buffer) const;
  void DecodeRX(BleBox &bb, const std::string &default_name,
                const _tColorSwitch *buffer) const;
  void DecodeRX(BleBox &bb, const std::string &default_name,
                const _tAirQualityMeter *buffer) const;
  void DecodeRX(BleBox &bb, const std::string &default_name,
                const _tGeneralDevice *buffer) const;
  void DecodeRX(BleBox &bb, const std::string &default_name,
                const _tThermostat *buffer) const;
  void DecodeRX(BleBox &bb, const std::string &name,
                const _tGeneralSwitch *buffer) const;

  virtual blebox::rfx get_rfx() const = 0;
};
} // namespace blebox::widgets
