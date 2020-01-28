#pragma once

#include <set>
#include <unordered_set>

#include "../json/forwards.h"

#include "../main/RFXNames.h" // for _eSwitchType

namespace test_helper {

auto json_parse(const char *data) -> Json::Value;

std::string compact_json(const Json::Value &value);
std::string rejson(const std::string &unstyled);

// using 1BADBEEF to mark uninitialized since DEADBEEF doesn't fit in signed
// range

struct device_status {
  int record_id{0x1BADBEEF};
  int hw_id{0x1BADBEEF};
  std::string node_type_identifier{"1BADBEEF"};
  int unit{0x1BADBEEF};
  std::string name{"1BADBEEF"};
  int type{0x1BADBEEF};
  int sub_type{0x1BADBEEF};
  int switch_type{0x1BADBEEF};
  int signal_level{0x1BADBEEF};
  int battery_level{0x1BADBEEF};
  int numeric_value{0x1BADBEEF};
  std::string string_value{"1BADBEEF"};
  std::string description{"1BADBEEF"};
  std::string color{"1BADBEEF"};
  int last_level{0x1BADBEEF};

  auto operator<(const device_status &other) const -> bool;

  static void read_from_db(std::set<device_status> &devices);

  static auto dump_json(const std::set<device_status> &devices) -> std::string;

  static auto json() -> std::string;

  static void set_switch_type(const int record_id, _eSwitchType switch_type);
};

} // namespace test_helper
