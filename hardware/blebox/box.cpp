#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/assert.hpp>
#include <boost/format.hpp>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../json/value.h"

#include "../../main/json_helper.h"

#include "../BleBox.h"
#include "errors.h"
#include "session.h"
#include "version.h"
#include "widgets.h"

#include "products.h"

#include "box.h"

blebox::box::box() = default;

const char *blebox::products_config = R"(
{
  "types": {
    "airSensor": {
      "api_path": "/api/air/state",
      "api_level_range": [ 20180403, 20190911 ]
    },
    "dimmerBox": {
      "api_path": "/api/light/state",
      "api_level_range": [ 20161213, 20190911 ]
    },
    "gateBox": {
      "api_path": "/api/gate/state",
      "api_level_range": [ 20161213, 20190911 ]
    },
    "gateController": {
      "api_path": "/api/gatecontroller/state",
      "api_level_range": [ 20180604, 20190911 ]
    },
    "saunaBox": {
      "api_path": "/api/heat/state",
      "api_level_range": [ 20180604, 20190911 ]
    },
    "shutterBox": {
      "api_path": "/api/shutter/state",
      "api_level_range": [ 20180604, 20190911 ]
    },
    "switchBox": {
      "api_path": "/api/relay/state",
      "api_level_range": [ 20161213, 20190911 ]
    },
    "switchBoxD": {
      "api_path": "/api/relay/state",
      "api_level_range": [ 20161213, 20190911 ]
    },
    "tempSensor": {
      "api_path": "/api/tempsensor/state",
      "api_level_range": [ 20180604, 20190911 ]
    },
    "wLightBox": {
      "api_path": "/api/rgbw/state",
      "api_level_range": [ 20180718, 20190911 ]
    },
    "wLightBoxS": {
      "api_path": "/api/light/state",
      "api_level_range": [ 20161213, 20190911 ]
    }
  }
}
)";

// stage2 construction to call virtual methods
void blebox::box::setup(const Json::Value &box_info) {
  if (_setup_done)
    throw std::runtime_error("setup already done!");

  // Get name first (used in error messages about other fields)
  _name = expect_nonempty_string(box_info, "deviceName");

  // required for identifying domoticz hardware instance
  // TODO: length needs to be ok (db schema)?
  _unique_id = expect_nonempty_string(box_info, "id");

  // TODO: type is likely already provided after identification
  _type = expect_nonempty_string(box_info, "type");

  // TODO: check if type known?

  Json::Value temp;
  if (!ParseJSonStrict(products_config, temp))
    throw errors::bad_device_config();

  _config = temp["types"][_type];
  if (_config.empty())
    throw errors::bad_device_config();

  // optional - maybe useful for hardware-specific workarounds in future
  _hardware_version = expect_nonempty_string(box_info, "hv");

  // optional - maybe useful for firmware-specific workarounds in future
  _firmware_version = expect_nonempty_string(box_info, "fv");

  // optional - maybe useful for api-specific workarounds in future
  _api_version = extract_api_version(box_info);

  _setup_done = true;

  version minimum(min_required_api_version());
  version current(api_version());

  // TODO: more descriptive exception
  if (!current.compatible_with(minimum))
    throw errors::outdated_firmare_version(current);
}

auto blebox::box::extract_api_version(const Json::Value &box_info)
    -> std::string {
  // boxes possibly lacking apiLevel (according to API docs):
  // switchBox
  // switchBoxD
  // wLightBox
  // wLightBoxS
  // dimmerBox
  // gateBox

  // TODO: demand upgrade if no apiLevel?
  try {
    follow(box_info, "apiLevel");
  } catch (jpath_failed &) { return "1.1.0"; }

  std::string apilevel = expect_nonempty_string(box_info, "apiLevel");

  auto &&levels = _config["api_level_range"];
  int min_supported = levels[0].asInt();
  int max_supported = levels[1].asInt();

  auto version = std::stoi(apilevel);
  if (version < min_supported)
    throw errors::outdated_firmare_version(apilevel);

  if (version > max_supported)
    throw errors::outdated_app_version(apilevel);

  return "1.1.0";
}

void blebox::box::fetch_state(blebox::session &session) {
  load_state_from_response(session.fetch(api_state_path()));
}

auto blebox::box::fetch_uptime(blebox::session &session) -> std::string {
  // TODO: coverage
  // TODO: call correct/recent api first
  Json::Value root = session.fetch("/api/device/uptime");
  if (root.empty()) {
    root = session.fetch("/api/device/runtime");
    if (root.empty())
      return "unknown";
    root = root["runtime"];
    if (root.empty())
      return "unknown";
  }

  int seconds = 0;

  uint64_t total_minutes = 0;
  if (!root["uptimeS"].empty()) {
    uint64_t sec = root["uptimeS"].asUInt64();
    total_minutes = sec / 60;
    seconds = sec % 60;
  } else if (!root["uptime"].empty()) {
    uint64_t sec = root["uptime"].asUInt64() / 1000;
    total_minutes = sec / 60;
    seconds = sec % 60;
  } else
    return "unknown";

  // TODO: cleanup (-Wsign-conversion)
  // TODO: test
  int days = static_cast<int>(total_minutes / (24 * 60));
  int hours =
      static_cast<int>(total_minutes / 60) - static_cast<int>(days * 24);
  int mins = static_cast<int>(total_minutes % 60);

  std::ostringstream os;

  std::string spacing = "";
  if (days > 0) {
    os << days << " days";
    spacing = " ";
  }

  if (hours > 0) {
    os << spacing << hours << " hours";
    spacing = " ";
  }

  if (mins > 0 && days < 1) {
    os << spacing << mins << " min ";
    spacing = " ";
  }

  if (seconds > 0 && hours < 1 && days < 1)
    os << spacing << seconds << " sec";

  return os.str();
}

template <class T>
static auto factory(const Json::Value &value) -> std::unique_ptr<T> {
  std::unique_ptr<T> instance = std::make_unique<T>();
  instance->setup(value);
  return instance;
}

auto blebox::box::from_value(const Json::Value &value) -> std::unique_ptr<box> {
  // e.g. airSensor has no "device" sub-structure
  const auto &info = value["device"];

  if (info.isNull()) {
    const auto &root_type = value["type"];

    if (root_type.isString()) {
      if (root_type == "airSensor")
        return factory<products::airSensor>(value);
      if (root_type == "gateBox")
        return factory<products::gateBox>(value);

      throw errors::unsupported_device(root_type.asString());
    }

    throw errors::unrecognized_device();
  }

  const auto &type = info["type"];
  if (!type.isString())
    throw std::runtime_error("Unrecognized device (no valid type)");

  // exceptions - pass whole struct to access state in one request
  if (type == "switchBox")
    return factory<products::switchBox>(value);
  if (type == "dimmerBox")
    return factory<products::dimmerBox>(value);
  if (type == "wLightBox")
    return factory<products::wLightBox>(value);
  if (type == "wLightBoxS")
    return factory<products::wLightBoxS>(value);

  if (type == "airSensor")
    return factory<products::airSensor>(info);
  if (type == "gateController")
    return factory<products::gateController>(info);
  if (type == "saunaBox")
    return factory<products::saunaBox>(info);
  if (type == "switchBoxD")
    return factory<products::switchBoxD>(info);
  if (type == "shutterBox")
    return factory<products::shutterBox>(info);
  if (type == "tempSensor")
    return factory<products::tempSensor>(info);

  throw errors::unsupported_device(type.asString());
}

// TODO: reorder max min
int blebox::box::check_int_range(int result, const std::string &field, int max,
                                 int min) const {
  using bad_field = errors::response::field;

  if (max >= min) {
    if (result > max)
      throw bad_field::exceeds_max(name(), field, result, max);

    if (result < min)
      throw bad_field::less_than_min(name(), field, result, min);
  }

  return result;
}

auto blebox::box::check_int(const Json::Value &value, const std::string &field,
                            int max, int min) const -> int {
  using bad_field = errors::response::field;

  if (value.isNull())
    throw bad_field::missing(name(), field);

  if (!value.isInt())
    throw bad_field::not_a_number(name(), field);

  int result = value.asInt();
  return check_int_range(result, field, max, min);
}

auto blebox::box::check_hex_int(const Json::Value &value,
                                const std::string &field, int max,
                                int min) const -> int {
  using bad_field = errors::response::field;

  if (value.isNull())
    throw bad_field::missing(name(), field);

  if (!value.isString())
    throw bad_field::not_a_string(name(), field);

  int result;

  try {
    result = std::stoi(value.asString(), nullptr, 16);
  } catch (std::invalid_argument &) {
    throw bad_field::not_a_hex_string(name(), field);
  }

  return check_int_range(result, field, max, min);
}

auto blebox::box::expect_int_at(const Json::Value &array, size_t index, int max,
                                int min) const -> int {
  const auto &value = array[int(index)];

  using namespace std::literals;
  return check_int(value, "["s + std::to_string(index) + "]", max, min);
}

auto blebox::box::expect_int(const Json::Value &data, const std::string &field,
                             int max, int min) const -> int {
  const auto &value = follow(data, field);
  return check_int(value, field, max, min);
}

auto blebox::box::expect_hex_int(const Json::Value &data,
                                 const std::string &field, int max,
                                 int min) const -> int {
  const auto &value = follow(data, field);
  return check_hex_int(value, field, max, min);
}

auto blebox::box::expect_nonempty_object(const Json::Value &data,
                                         const std::string &field) const
    -> Json::Value {
  using bad_field = errors::response::field;

  const auto &value = follow(data, field);

  if (value.isNull())
    throw bad_field::missing(name(), field);

  if (!value.isObject())
    throw bad_field::not_an_object(name(), field);

  if (value.empty())
    throw bad_field::empty(name(), field);

  return value;
}

auto blebox::box::expect_nonempty_string(const Json::Value &data,
                                         const std::string &field) const

    -> std::string {
  using bad_field = errors::response::field;

  const auto &value = follow(data, field);
  if (value.isNull())
    throw bad_field::missing(name(), field);

  if (!value.isString())
    throw bad_field::not_a_string(name(), field);

  const std::string &result = value.asString();

  if (result.empty())
    throw bad_field::empty(name(), field);

  return result;
}

auto blebox::box::expect_nonempty_array(const Json::Value &data,
                                        const std::string &field) const
    -> Json::Value {
  using bad_field = errors::response::field;

  const auto &value = follow(data, field);
  // const auto &value = data[field];
  if (value.isNull())
    throw bad_field::missing(name(), field);

  if (!value.isArray())
    throw bad_field::not_an_array(name(), field);

  if (value.empty())
    throw bad_field::empty(name(), field);

  return value;
}

void blebox::box::fetch(blebox::session &session, BleBox &bb) {
  const Json::Value &data = state();
  if (data.isNull())
    fetch_state(session);

  update_state_from_response(bb, data);
}

void blebox::box::add_missing_supported_features(int hw_id) {
  for (auto &method : _widgets)
    for (auto &unit : method.second)
      unit.second->save(hw_id);
}

namespace blebox {
static const std::regex with_int_value(R"(^\[(.*)=(\d+)\]$)");
}

auto blebox::box::follow(const Json::Value &data, const std::string &path)
    -> Json::Value {
  using namespace std::literals;

  std::vector<std::string> results;
  boost::split(results, path, boost::is_any_of("/"));

  Json::Value current_tree = data;

  for (auto &&chunk : results) {
    std::smatch what;

    static const std::regex with_string_value(R"(^\[(.*)='(.*)'\]$)");
    static const std::regex with_index(R"(^\[(\d+)\]$)");

    if (std::regex_search(chunk, what, with_string_value)) {
      std::string name(what[1]);
      std::string value(what[2]);

      bool found = false;
      for (auto &&item : current_tree)
        if (item[name] == value) {
          current_tree = item;
          found = true;
          break;
        }

      if (!found)
        throw jpath_failed("with: "s + name + "="s + value);
      continue;
    }

    if (std::regex_search(chunk, what, with_int_value)) {
      std::string name(what[1]);
      int value(std::stoi(what[2]));

      bool found = false;
      for (auto &&item : current_tree)
        if (item[name] == value) {
          current_tree = item;
          found = true;
          break;
        }

      if (!found)
        throw jpath_failed("with: "s + name + "="s + std::to_string(value));
      continue;
    }

    if (std::regex_search(chunk, what, with_index)) {
      auto index = std::stoul(what[1]);
      if (!current_tree.isArray() || index >= current_tree.size())
        throw jpath_failed("with value at "s + std::to_string(index));

      current_tree = current_tree[int(index)]; // TODO: check range
      continue;
    }

    auto &&names = current_tree.getMemberNames();
    if (std::find(names.begin(), names.end(), chunk) == names.end()) {
      std::ostringstream os;
      os << "item: " << chunk << " within: ";
      std::copy(names.begin(), names.end(),
                std::ostream_iterator<std::string>(os, " "));
      throw jpath_failed(os.str());
    }

    current_tree = current_tree[chunk];
  }
  return current_tree;
}

// TODO: test edge cases, etc.
void blebox::box::follow_and_set(Json::Value &data, const std::string &path,
                                 const Json::Value &value) {
  std::vector<std::string> results;
  boost::split(results, path, boost::is_any_of("/"));

  if (results.empty())
    throw jpath_failed("no json path to follow (empty)");

  auto field = results.back();
  results.pop_back();

  auto &&current_tree = &data;
  for (auto &&chunk : results) {
    std::smatch what;
    if (std::regex_search(chunk, what, with_int_value)) {
      std::string name(what[1]);
      int member_value(std::stoi(what[2]));

      {
        bool found = false;
        for (auto &&item : *current_tree)
          if (item[name] == member_value) {
            current_tree = &item;
            found = true;
            break;
          }

        if (!found) {
          if (current_tree->isNull()) {
            Json::Value entry;
            entry[name] = member_value;

            Json::Value array;
            array[0] = entry;
            (*current_tree) = array;
            current_tree = &(*current_tree)[0];
          } else {
            Json::Value entry;
            entry[name] = member_value;
            current_tree = &(current_tree->append(entry));
          }
        }
      }
    } else
      current_tree = &((*current_tree)[chunk]);
  }

  // TODO: lazy, but sufficient for now
  if (field == "[0]") {
    Json::Value array;
    array[0] = value;
    (*current_tree) = array;
  } else {
    (*current_tree)[field] = value;
  }
}

Json::Value blebox::box::post_raw(int hw_id, blebox::session &session,
                                  const char *raw_data) {
  uint8_t method_id;
  uint8_t raw_unit;
  std::string deviceid;
  uint8_t devtype;
  uint8_t subtype;
  std::tie(method_id, raw_unit, deviceid, devtype, subtype) =
      rfx::record_info_from(raw_data);

  std::unique_ptr<blebox::db::device_status> record;
  if (!deviceid.empty())
    record =
        std::make_unique<blebox::db::device_status>(hw_id, deviceid, raw_unit);

  std::byte unit = rfx::decode_unit(raw_unit);
  auto &&widget = widget_at(unit, method_id);
  widget.rfx().verify_same_rfx(devtype, subtype);
  return widget.post_raw_rfx(session, raw_data, record.get());
}

auto blebox::box::from_session(blebox::session &session)
    -> std::unique_ptr<box> {
  const Json::Value value = session.fetch("/api/device/state");
  auto &&dev = from_value(value);
  return std::move(dev);
}

blebox::widgets::widget &blebox::box::widget_at(std::byte unit,
                                                uint8_t method_id) const {
  try {
    return *_widgets.at(unit).at(method_id);
  } catch (std::out_of_range &) {
    std::ostringstream os;
    os << "Unsupported feature: "
       << "0x" << std::hex << int(method_id) << std::dec;
    if (!rfx::is_self_method(unit))
      os << "[" << int(unit) << "]";
    os << " for " << type();
    throw std::runtime_error(os.str());
  }
}

void blebox::box::update_state_from_response(BleBox &bb,
                                             const Json::Value &data) {

  if(data["_blebox.domoticz.info"] == "empty response")
    return;

  load_state_from_response(data);

  auto &&result = db::device_status::all_for_hw(bb.hw_id());
  for (auto &&fields : result) {
    // TODO: check ranges

    auto dtype = uint8_t(std::stoul(fields[2]));
    auto subtype = uint8_t(std::stoul(fields[3]));
    auto switchtype = _eSwitchType(std::stoul(fields[4]));
    rfx rfx(dtype, subtype, switchtype);

    std::byte unit;
    uint8_t method_id;
    std::tie(method_id, unit) =
        rfx.extract_method_and_unit(fields[0], fields[1]);

    auto &&widget = widget_at(unit, method_id);
    widget.rfx().verify_same_rfx(rfx);
    try {
      widget.fetch(bb, data);
    } catch (std::runtime_error &ex) {
      using namespace std::literals;
      throw std::runtime_error(ex.what() + ", JSON: "s +
                               std::string(JSonToRawString(data), 0, 1000));
    }
  }
}

void blebox::box::load_state_from_response(const Json::Value &value) {
  _state_data = value;

  if (_state_data.empty())
    throw errors::response::empty(api_state_path());
}

std::string blebox::box::api_state_path() const {
  return _config["api_path"].asString();
}

blebox::box::jpath_failed::jpath_failed(const std::string &details)
    : std::runtime_error(std::string("cannot find JSON path ") + details) {}

std::byte blebox::box::self_method() const { return rfx::self_method(); }
