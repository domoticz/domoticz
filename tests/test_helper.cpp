#include <set>
#include <stdexcept>

#include <boost/assert.hpp>

#include "../json/value.h"
#include "../json/writer.h"

#include "../main/SQLHelper.h"
#include "../main/json_helper.h"

#include "test_helper.h"

auto test_helper::json_parse(const char *data) -> Json::Value {
  using namespace std::literals;

  Json::Value result;
  if (!ParseJSonStrict(data, result))
    throw std::runtime_error("Failed to parse json: "s + data);

  return result;
}

void test_helper::device_status::read_from_db(
    std::set<test_helper::device_status> &devices) {
  auto records = m_sql.safe_query(
      "SELECT HardwareID, DeviceID, Unit, Name, Type, SubType, SwitchType, "
      "SignalLevel, BatteryLevel, nValue, sValue, Description, ID, Color, "
      "LastLevel from DeviceStatus");

  for (auto &&fields : records) {
    // TODO: sort here and in query so that ID is second
    device_status device;
    device.hw_id = std::stoi(fields.at(0));
    device.node_type_identifier = fields.at(1);
    device.unit = std::stoi(fields.at(2));
    device.name = fields.at(3);
    device.type = std::stoi(fields.at(4));
    device.sub_type = std::stoi(fields.at(5));
    device.switch_type = std::stoi(fields.at(6));
    device.signal_level = std::stoi(fields.at(7));
    device.battery_level = std::stoi(fields.at(8));
    device.numeric_value = std::stoi(fields.at(9));
    device.string_value = fields.at(10);
    device.description = fields.at(11);
    device.record_id = std::stoi(fields.at(12));
    device.color = fields.at(13);
    device.last_level = std::stoi(fields.at(14));

    BOOST_ASSERT(devices.insert(device).second);
  }
}

auto test_helper::device_status::dump_json(
    const std::set<test_helper::device_status> &devices) -> std::string {
  std::ostringstream os;

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   "; // or whatever you like

  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

  Json::Value results(Json::arrayValue);

  for (auto &&device : devices) {
    Json::Value result(Json::objectValue);

    result["hw_id"] = device.hw_id;
    result["node_type_identifier"] = device.node_type_identifier;
    result["unit"] = device.unit;
    result["name"] = device.name;
    result["type"] = device.type;
    result["sub_type"] = device.sub_type;
    result["switch_type"] = device.switch_type;
    result["signal_level"] = device.signal_level;
    result["battery_level"] = device.battery_level;
    result["numeric_value"] = device.numeric_value;
    result["string_value"] = device.string_value;
    result["description"] = device.description;
    result["color"] = device.color;
    result["last_level"] = device.last_level;
    results.append(result);
  }

  writer->write(results, &os);
  os << std::endl; // add lf and flush

  // std::unique_ptr<Json::StreamWriter> const writer(
  // factory.newStreamWriter()); writer->write(value, &os); os << std::endl; //
  // add lf and flush

  return os.str();
}

auto test_helper::device_status::json() -> std::string {
  std::set<test_helper::device_status> devices;
  read_from_db(devices);
  return dump_json(devices);
}

auto test_helper::device_status::operator<(const device_status &other) const
    -> bool {
  if (node_type_identifier != other.node_type_identifier)
    return node_type_identifier < other.node_type_identifier;

  if (unit != other.unit)
    return unit < other.unit;

  if (type != other.type)
    return type < other.type;

  if (sub_type != other.sub_type)
    return sub_type < other.sub_type;

  if (switch_type != other.switch_type)
    return switch_type < other.switch_type;

  if (name != other.name)
    return name < other.name;

  if (signal_level != other.signal_level)
    return signal_level < other.signal_level;

  if (battery_level != other.battery_level)
    return battery_level < other.battery_level;

  if (numeric_value != other.numeric_value)
    return numeric_value < other.numeric_value;

  if (string_value != other.string_value)
    return string_value < other.string_value;

  if (description != other.description)
    return description < other.description;

  if (color != other.color)
    return color < other.color;

  if (last_level != other.last_level)
    return last_level < other.last_level;

  return false;
}

void test_helper::device_status::set_switch_type(const int record_id,
                                                 _eSwitchType switch_type) {
  auto records =
      m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE ID=%d",
                       switch_type, record_id);
}

// TODO: copied and pasted from session
std::string test_helper::compact_json(const Json::Value &data) {
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  builder["commentStyle"] = "None";
  return Json::writeString(builder, data);
}

std::string test_helper::rejson(const std::string &unstyled) {
  return compact_json(json_parse(unstyled.c_str()));
}
