#pragma once

#include <string>

#include "rfx.h"
#include "widgets/widget.h"

class BleBox;
namespace Json {
class Value;
}
namespace blebox::db {
class device_status;
}
namespace blebox {
class session;
}

namespace blebox::widgets {

namespace posting {
using path_maker = widget::api_path_maker_type;

path_maker not_implemented();
path_maker simple_get(const std::string &path);
} // namespace posting

class on_off_percentage_slider : public widget {
public:
  using widget::widget;

protected:
  typedef int (box::*expect_int_fn)(const Json::Value &, const std::string &,
                                    int, int) const;

private:
  blebox::rfx get_rfx() const override;
  void fetch(BleBox &bb, const Json::Value &data) const final;

  virtual expect_int_fn expect_value_fn() const;

  virtual int max() const { return 100; }
  virtual Json::Value as_json_type(int value) const;

  Json::Value post_raw_rfx(blebox::session &session, const char *raw_data,
                           const blebox::db::device_status *record) final;
};

class dimmer : public on_off_percentage_slider {
public:
  using on_off_percentage_slider::on_off_percentage_slider;

private:
  int max() const final { return 255; }
  blebox::rfx get_rfx() const final;
};

class small_hex_string : public dimmer {
public:
  using dimmer::dimmer;

private:
  Json::Value as_json_type(int value) const final;

  expect_int_fn expect_value_fn() const final;
};

class push_button : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  void fetch(BleBox & /*bb*/, const Json::Value & /*data*/) const final {
    // nothing - we can't determine the state from API
  }

  Json::Value post_raw_rfx(blebox::session &session, const char *raw_data,
                           const blebox::db::device_status *record) final;
};

class switch_onoff : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  void fetch(BleBox &bb, const Json::Value &data) const final;

  Json::Value post_raw_rfx(blebox::session &session, const char *raw_data,
                           const blebox::db::device_status * /*record*/) final;
};

class percentage : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  void fetch(BleBox &bb, const Json::Value &data) const final;
};

class color_switch : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  void fetch(BleBox &bb, const Json::Value &data) const final;
  Json::Value post_raw_rfx(blebox::session &session, const char *raw_data,
                           const blebox::db::device_status *record) final;
};

class temperature : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  void fetch(BleBox &bb, const Json::Value &data) const final;
};

class thermostat : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  void fetch(BleBox &bb, const Json::Value &data) const final;
  Json::Value post_raw_rfx(blebox::session &session, const char *raw_data,
                           const blebox::db::device_status *record) final;
};

class air_reading : public widget {
public:
  using widget::widget;
  blebox::rfx get_rfx() const final;

private:
  std::string description() const final;
  void fetch(BleBox &bb, const Json::Value &data) const final;
};
} // namespace blebox::widgets
