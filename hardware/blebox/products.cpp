#include <boost/format.hpp>
#include <map>
#include <utility>

#include "widgets.h"

#include "products.h"

// TODO: qualityLevel
// TODO: trend
// TODO: elapsedTimeS
// TODO: state
// TODO: deviceName
// TODO: statusLed/enabled
// TODO: sensorApi/makeGeolocationCoarse
// TODO: air/mountingPlace
void blebox::products::airSensor::setup(const Json::Value &response) {
  using format = boost::format;

  box::setup(response);

  add_method<widgets::push_button>(
      0xB0, "kick", "kick", widgets::posting::simple_get("/api/air/kick"));

  std::map<std::string, uint8_t> _sensors_map{
      {"pm1", 0}, {"pm2.5", 1}, {"pm10", 2}};

  for (auto &&sensor : _sensors_map) {
    auto &&type = sensor.first;
    auto &&id = sensor.second;
    auto &&path = (format("air/sensors/[type='%s']/value") % type).str();
    add<widgets::air_reading>(id, 0xB0, path, type + ".value");
  }
}

// TODO: loadType
// TODO: overloaded
// TODO: overheated
// TODO: temperature
// TODO: currentBrightness
void blebox::products::dimmerBox::setup(const Json::Value &response) {
  _state_data = response;

  box::setup(response["device"]);

  add_method<widgets::dimmer>(
      0xB0, "dimmer/desiredBrightness", "brightness",
      [](auto &&, auto) { return std::make_tuple("POST", "/api/dimmer/set"); },
      "dimmer/desiredBrightness");
}

// TODO: change polling time when opening/closing?

// TODO: extraButtonType
// TODO: extraButtonRelayNumber
// TODO: extraButtonPulseTimeMs
// TODO: extraButtonInvert
// TODO: gateType
// TODO: gateRelayNumber
// TODO: gatePulseTimeMs
// TODO: gateInvert
// TODO: inputsType
// TODO: openLimitSwitchInputNumber
// TODO: closeLimitSwitchInputNumber
void blebox::products::gateBox::setup(const Json::Value &response) {
  box::setup(response);

  // TODO: these are posts via GET
  // TODO: merge these with percentage?
  add_method<widgets::push_button>(0xB0, "primary", "primary",
                                   widgets::posting::simple_get("/s/p"));

  add_method<widgets::push_button>(0xB1, "secondary", "secondary",
                                   widgets::posting::simple_get("/s/s"));

  // TODO: when extra button is stop, maybe setting percentage makes sense?
  add_method<widgets::percentage>(0xB2, "currentPos|gate/currentPos",
                                  "currentPos");
}

// TODO: gateController/safety/eventReason
// TODO: gateController/safety/triggered/[0]
// TODO: gateController/partialOpen
// TODO: gateController/controlType
// TODO: gateController/moveDirectionSwap
// TODO: gateController/fieldsPreferences
// TODO: gateController/statusLed/enabled
// TODO: gateController/tunnel/enabled
// TODO: gateController/calibrationParameters/isCalibrated
void blebox::products::gateController::setup(const Json::Value &response) {
  box::setup(response);

  // TODO: last param is same as first
  add_method<widgets::on_off_percentage_slider>(
      0xB0, "gateController/desiredPos/[0]", "desiredPos",
      [](auto &&, auto) {
        return std::make_tuple("POST", "/api/gatecontroller/set");
      },
      "gateController/desiredPos/[0]");

  add_method<widgets::percentage>(0xB1, "gateController/currentPos/[0]",
                                  "currentPos");
}

// TODO: sensors[]/trend
// TODO: sensors[]/state
// TODO: sensors[]/elapsedTimeS
void blebox::products::saunaBox::setup(const Json::Value &response) {
  using namespace std::literals;

  box::setup(response);

  {
    const int sensor_id = 0;
    std::string path =
        (boost::format("heat/sensors/[id=%d]/value") % sensor_id).str();
    add<widgets::temperature>(sensor_id, 0xB0, path, "temp");
  }

  // TODO: post
  add_method<widgets::thermostat>(
      0xB0, "heat/desiredTemp", "desiredTemp",
      [](auto &&, auto temp) { return std::make_tuple("GET", "/s/t/"s + temp); },
      "heat/desiredTemp");

  add_method<widgets::switch_onoff>(
      0xB1, "heat/state", "state",
      [](auto &&, auto state) {
        return std::make_tuple("GET", "/s/"s + state);
      },
      "heat/state");
}

// TODO: controlType
// TODO: inputsSwap
// TODO: moveDirection
// TODO: moveTimeoutMs
// TODO: deviceName
// TODO: tunnel/enabled
// TODO: statusLed/enabled
// TODO: calibrationParameters/isCalibrated
// TODO: calibrationParameters/maxTiltTimeUpMs
// TODO: calibrationParameters/maxTiltTimeDownMs
// TODO: calibrationParameters/maxTiltTimeDownMs
void blebox::products::shutterBox::setup(const Json::Value &response) {
  using namespace std::literals;

  box::setup(response);

  add_method<widgets::on_off_percentage_slider>(
      0xB0, "shutter/desiredPos/position", "desired_position",
      [](auto &&, auto state) { return std::make_tuple("GET", "/s/p/"s + state); },
      "shutter/desiredPos/position");

  add_method<widgets::percentage>(0xB1, "shutter/currentPos/position",
                                  "actual_position");
}

// TODO: relays/relay
// TODO: relays/stateAfterRestart
void blebox::products::switchBox::setup(const Json::Value &response) {
  using namespace std::literals;

  _state_data = response;

  box::setup(response["device"]);

  add<widgets::switch_onoff>(
      0, 0xB0, "relays/[relay=0]/state", "state",
      [](auto &&, auto state) { return std::make_tuple("GET", "/s/"s + state); },
      "relays/[relay=0]/state");
}

// TODO: relays/relay
// TODO: relays/stateAfterRestart
// TODO: relays/name
// TODO: switch/outputMode
void blebox::products::switchBoxD::setup(const Json::Value &response) {
  box::setup(response);

  using namespace std::literals;

  // TODO: post is via get
  add<widgets::switch_onoff>(
      0, 0xB0, "relays/[relay=0]/state", "state",
      [](auto &&, auto state) { return std::make_tuple("GET", "/s/0/"s + state); },
      "relays/[relay=0]/state");

  add<widgets::switch_onoff>(
      1, 0xB0, "relays/[relay=1]/state", "state",
      [](auto &&, auto state) { return std::make_tuple("GET", "/s/1/"s + state); },
      "relays/[relay=1]/state");
}

// TODO: deviceName
// TODO: statusLed/enabled
// TODO: tunnel/enabled
// TODO: trend
// TODO: state
// TODO: elapsedTimeS

// TODO: handle state = 1 (still reading)
// TODO: test negative and null case (for value)
// TODO: add failing devices to database too? (Not sure there's a way to
// signal failing hardware, other than ignoring it and not receiving any new
// readings).
void blebox::products::tempSensor::setup(const Json::Value &response) {
  box::setup(response);

  // TODO: fix value
  const int sensor_id = 0;
  std::string path =
      (boost::format("tempSensor/sensors/[id=%d]/value") % sensor_id).str();
  add<widgets::temperature>(sensor_id, 0xB0, path, "temp");
}

// TODO: deviceName
// TODO: tunnel/enabled
// TODO: statusLed/enabled
// TODO: rgbw/colorMode
// TODO: rgbw/effectId
// TODO: rgbw/effectId
// TODO: rgbw/lastOnColor
// TODO: rgbw/durationMs/colorFade
// TODO: rgbw/durationMs/effectFade
// TODO: rgbw/durationMs/effectStep
// TODO: rgbw/effectNames
// TODO: rgbw/favColors
void blebox::products::wLightBox::setup(const Json::Value &response) {
  using namespace std::literals;

  _state_data = response;

  box::setup(response["device"]);
  add_method<widgets::color_switch>(
      0xB0, "rgbw/desiredColor", "desired_color",
      [](auto &&, const std::string &state) {
        // return std::make_tuple("POST", "/api/rgbw/set");

        if (state == "on"s || state == "off"s)
          return std::make_tuple(
              "GET",
              "/s/onoff/last"s); // TODO: detect state before toggle
        else
          return std::make_tuple("GET", "/s/"s + state);
      },
      "rgbw/desiredColor");
}

// TODO: light/currentColor
// TODO: light/fadeSpeed  (int): slider + hex string
void blebox::products::wLightBoxS::setup(const Json::Value &response) {
  using namespace std::literals;

  _state_data = response;

  box::setup(response["device"]);

  // TODO: post is via get
  add_method<widgets::small_hex_string>(
      0xB0, "light/desiredColor", "desired_color",
      [](auto &&, auto) { return std::make_tuple("POST", "/api/rgbw/set"); },
      "light/desiredColor");
}
