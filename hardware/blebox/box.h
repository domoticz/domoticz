#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "../../json/value.h"
#include "ip.h"

#include "widgets.h" // IWYU pragma: keep

class BleBox;

namespace blebox {
extern const char *products_config;
class session;

// TODO: rename to "product" or move out of products folder
class box {
public:
  static const int DefaultApiTimeout = 3;

  box();
  virtual ~box() = default;

  virtual void setup(const Json::Value &box_info);

  Json::Value post_raw(int hw_id, blebox::session &session,
                       const char *raw_data);
  void update_state_from_response(BleBox &bb, const Json::Value &value);

  virtual void fetch(blebox::session &session, BleBox &bb) final;
  virtual void add_missing_supported_features(int hw_id) final;

  // TODO: convert to widgets
  auto name() const -> const std::string & { return _name; }

  auto type() const -> const std::string & {
    verify_setup();
    return _type;
  }
  auto unique_id() const -> const std::string & {
    verify_setup();
    return _unique_id;
  }
  auto hardware_version() const -> const std::string & {
    verify_setup();
    return _hardware_version;
  }
  auto firmware_version() const -> const std::string & {
    verify_setup();
    return _firmware_version;
  }
  auto api_version() const -> const std::string & {
    verify_setup();
    return _api_version;
  }

  // TODO: implement invalidate(X) to force HTTP/GET to box to fetch new
  // values in given area
  static auto fetch_uptime(::blebox::session &session) -> std::string;

  static auto from_session(blebox::session &session)
      -> std::unique_ptr<blebox::box>;

protected:
  auto state() const -> const Json::Value & { return _state_data; }

  void fetch_state(blebox::session &session);

  virtual auto min_required_api_version() const -> std::string {
    return "1.1.0";
  }

  // TODO: Organize public/private/protected sections
public:
  // TODO: refactor out
  static Json::Value follow(const Json::Value &data, const std::string &path);
  static void follow_and_set(Json::Value &data, const std::string &path,
                             const Json::Value &value);

  template <typename T>
  static void set_form_field(Json::Value &data, const std::string &path,
                             const T &value) {
    follow_and_set(data, path, Json::Value(value));
  }

  // Validation
  int expect_int_at(const Json::Value &array, size_t index, int max,
                    int min) const;

  int expect_int(const Json::Value &data, const std::string &field,
                 int max = -1, int min = 0) const;

  int expect_hex_int(const Json::Value &data, const std::string &field,
                     int max = -1, int min = 0) const;

  auto expect_nonempty_string(const Json::Value &data,
                              const std::string &field) const -> std::string;

  auto expect_nonempty_array(const Json::Value &data,
                             const std::string &field) const -> Json::Value;

  auto expect_nonempty_object(const Json::Value &data,
                              const std::string &field) const -> Json::Value;

  int check_int(const Json::Value &value, const std::string &field, int max,
                int min) const;

  int check_hex_int(const Json::Value &value, const std::string &field, int max,
                    int min) const;

  struct jpath_failed : public std::runtime_error {
    explicit jpath_failed(const std::string &details);
  };

protected:
  Json::Value _config;

  // eg. switchbox can fetch this while getting device info
  Json::Value _state_data;

  typedef std::byte unit_type;
  typedef uint8_t method_id_type;
  typedef std::map<method_id_type, std::unique_ptr<widgets::widget>>
      method_map_type;
  typedef std::map<unit_type, method_map_type> widgets_type;
  widgets_type _widgets;

  // TODO: cleanup (constructors)
  // TODO: rename to add_widget?
  template <typename T>
  void add_method(uint8_t method_id, const std::string &field,
                  const std::string &title,
                  widgets::posting::path_maker api_path_maker =
                      widgets::posting::not_implemented(),
                  const std::string &post_path = "") {
    add<T>(uint8_t(self_method()), method_id, field, title, api_path_maker,
           post_path);
  }

  template <typename T>
  void add(uint8_t u_unit, uint8_t method_id, const std::string &field,
           const std::string &title,
           widgets::posting::path_maker api_path_maker =
               widgets::posting::not_implemented(),
           const std::string &post_path = "") {
    auto unit = std::byte(u_unit);
    auto &&method = std::make_unique<T>(unit, method_id, field, title, *this,
                                        api_path_maker, post_path);

    method->setup();

    auto &&methods = _widgets.find(unit);
    if (methods == _widgets.end()) {
      method_map_type new_methods;
      new_methods.emplace(method_id, std::move(method));
      _widgets.emplace(unit, std::move(new_methods));
      return;
    }

    auto &method_map = methods->second;

    auto &&feature = method_map.find(method_id);
    if (feature != method_map.end())
      throw std::runtime_error("feature already added");

    method_map.emplace(method_id, std::move(method));
  }

private:
  std::string _name;
  std::string _unique_id;
  std::string _type;
  std::string _hardware_version;
  std::string _firmware_version;
  std::string _api_version;

  Json::Value _box_info;

  bool _setup_done{false};

  void verify_setup() const {
    if (!_setup_done)
      throw std::runtime_error("setup not called!");
  }

  virtual auto extract_api_version(const Json::Value &box_info) -> std::string;

  int check_int_range(int result, const std::string &field, int max,
                      int min) const;

  blebox::widgets::widget &widget_at(std::byte unit, uint8_t method_id) const;

  static auto from_value(const Json::Value &value) -> std::unique_ptr<box>;

  void load_state_from_response(const Json::Value &value);

  std::string api_state_path() const;

  std::byte self_method() const;
};
} // namespace blebox
