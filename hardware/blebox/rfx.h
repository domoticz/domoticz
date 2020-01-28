#pragma once

#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <string>
#include <tuple>

#include "../../main/RFXNames.h"

#include "workarounds/gcc_6.h"

namespace blebox {
class rfx {
public:
  rfx() = default;
  rfx(uint8_t dev, uint8_t sub, _eSwitchType swt = STYPE_OnOff);

  // TODO: remove?
  void set(uint8_t dev, uint8_t sub, _eSwitchType swt = STYPE_OnOff);

  auto devtype() const { return _devtype; }
  auto subtype() const { return _subtype; }
  auto switchtype() const { return _switchtype; }

  std::string to_string() const;

  void verify_same_rfx(const rfx &other) const;
  void verify_same_rfx(uint8_t dev, uint8_t sub) const;

  std::tuple<uint8_t, std::byte>
  extract_method_and_unit(const std::string &device_id_str,
                          const std::string &raw_unit);

  using dev_info = std::tuple<uint8_t, uint8_t, std::string, uint8_t, uint8_t>;

  static dev_info record_info_from(const char *raw_data);
  static std::byte decode_unit(unsigned long value);
  static std::byte decode_unit(const std::string &db_value);
  static uint8_t encode_unit(std::byte unit, uint8_t subtype);
  static uint8_t encode_general_dev_unit(std::byte unit);

  bool operator==(const rfx &other) const;

  bool operator!=(const rfx &other) const { return !(*this == other); }

  struct unsupported_type : std::runtime_error {
    explicit unsupported_type(uint8_t dtype);
  };

  auto node_id_str(uint8_t method_id, std::byte unit) const -> std::string;

  static bool is_self_method(std::byte unit);
  static std::byte self_method() { return self_method_id; }

private:
  uint8_t _devtype{0};
  uint8_t _subtype{0}; // sTypeAC
  _eSwitchType _switchtype{STYPE_OnOff};

  bool is_compatible(const rfx &other) const;

  // this is a "unit" value that refers to self
  // (instead of an index of one of this box's  sensors)

#if __cplusplus < 201703L
  static const std::byte self_method_id{static_cast<std::byte>(0xFF)};
#else
  static const std::byte self_method_id{0xFF};
#endif
};
} // namespace blebox
