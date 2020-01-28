#pragma once
#include <cstddef> // for std::byte
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../../main/RFXNames.h" // for _eSwitchType
#include "../workarounds/gcc_6.h"

namespace blebox {
namespace db {
typedef std::vector<std::string> sql_record;
typedef std::vector<sql_record> sql_result;

class device_status {
public:
  device_status(int id, int unit, std::string name, std::string value,
                std::string description)
      : _id(id),
        _unit(unit),
        _name(std::move(name)),
        _value(std::move(value)),
        _description(std::move(description)) {}

  device_status(int record_id);
  device_status(int hw_id, const std::string &dev_id, int unit);

  auto id() const -> const int & { return _id; }
  auto unit() const -> const int & { return _unit; }
  auto name() const -> const std::string & { return _name; }
  auto value() const -> const std::string & { return _value; }
  auto description() const -> const std::string & { return _description; }
  auto switchtype() const -> const _eSwitchType & { return _switchtype; }

  // Color switch
  auto color() const -> const std::string & { return _color; }
  auto last_level() const -> int { return _last_level; }

  static void remove(int id);
  static void remove_all(int hw_id);
  static auto all(int hw_id, std::vector<device_status> &output) -> bool;
  static void use(int hw_id);

  static auto get_values(int &nvalue, std::string &svalue, int hw_id,
                         std::byte node_id, int ChildID, int packet_type,
                         int sub_type) -> bool;

  struct record_not_found : std::runtime_error {
    explicit record_not_found(int record_id)
        : std::runtime_error(std::string("device_status not found: ") +
                             std::to_string(record_id)) {}

    explicit record_not_found(int hw_id, const std::string &dev_id, int unit);
  };

  static sql_result all_for_hw(int hw_id);

private:
  // TODO: more fields? (from db schema for DeviceStatus)
  int _id{0x7BADBEEF};
  int _unit{0x7BADBEEF}; // TODO: uint8_t in domoticz core, but int in database
  std::string _name;
  std::string _value;
  std::string _description;
  std::string _color;
  int _last_level{0x7BADBEEF};
  _eSwitchType _switchtype{STYPE_END};
};
} // namespace db
} // namespace blebox
