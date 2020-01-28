#include <boost/assert.hpp>
#include <limits>


using BYTE = unsigned char; // for RFXtrx.h in mainworker.h
#include "../main/mainworker.h"

#include "../hardware/BleBox.h"

#include "worker_helper.h"

void worker_helper::start_with_hardware(CDomoticzHardwareBase *hardware) {
  // TODO: this should be in a constructor
  // Reset stopped status (in case another test case stopped the worker)
  m_mainworker.m_TaskRXMessage = StoppableTask();

  _msg_thread = std::make_unique<std::thread>(
      &MainWorker::Do_Work_On_Rx_Messages, &m_mainworker);
  add_hardware(hardware);
}

void worker_helper::stop() {
  if (!_msg_thread)
    return;

  // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  m_mainworker.m_TaskRXMessage.RequestStop();

  // TODO: this is slow (due to SwitchLight waiting for queue to be
  // processed)
  if (_msg_thread)
    _msg_thread->join();

  // TODO: this should be in a destructor
  // Reset the main worker for new tests
  m_mainworker.m_TaskRXMessage = StoppableTask();
}

// simulate how Domoticz reports brightness changes in white mode
bool worker_helper::rgbw_whitemode_set(int idx, uint8_t level_100) {
  static_assert(std::is_unsigned<decltype(level_100)>::value, "unsigned level_100");
  BOOST_ASSERT(level_100 <= 100);
  _tColor color(0, 0, 0, 0xff, 0xff, ColorModeWhite);
  return m_mainworker.SwitchLight(idx, "Set Color", level_100, color, false, 0, "admin");
}

// simulate how Domoticz reports rgbw changes in color mode
bool worker_helper::rgbw_color_set(int idx, uint8_t r, uint8_t g, uint8_t b,
                                   uint8_t level_100) {
  BOOST_ASSERT(level_100 <= 100);
  _tColor color(r, g, b, 0, 0, ColorModeRGB);
  return m_mainworker.SwitchLight(idx, "Set Color", level_100, color, false, 0, "admin");
}

bool worker_helper::rgbw_off(int idx) {
  // NOTE: calls SwitchLight(idx, "Off", -1, {0000000000:m=0}, false, 0)
  return m_mainworker.SwitchLight(std::to_string(idx), "Off", "-1", "-1", "",
                                  0, "admin");
}

bool worker_helper::rgbw_on(int idx) {
  // NOTE: calls SwitchLight(idx, "On", -1, {0000000000:m=0}, false, 0)
  return m_mainworker.SwitchLight(std::to_string(idx), "On", "-1", "-1", "", 0,
                                  "admin");
}

// TODO: ideally simulate GetJsonPage calls from WebServer
bool worker_helper::set_level(int idx, uint8_t level) {
  BOOST_ASSERT(level <= 100);
  const char *cmd = "Set Level";
  const char *color = "-1"; // converted later to _tColor
  const char *ooc = "0";    // converted later to bool
  int extra_delay = 0;
  return m_mainworker.SwitchLight(
      std::to_string(idx), cmd, std::to_string(level), color, ooc, extra_delay, "admin");
}

bool worker_helper::worker_helper::push(int idx) {
  return m_mainworker.SwitchLight(std::to_string(idx), "On", "0", "-1", "0", 0, "admin");
}

bool worker_helper::worker_helper::switch_on(int idx) {
  return m_mainworker.SwitchLight(std::to_string(idx), "On", "0", "-1", "0", 0, "admin");
}

bool worker_helper::worker_helper::switch_off(int idx) {
  return m_mainworker.SwitchLight(std::to_string(idx), "Off", "0", "-1", "0",
                                  0, "admin");
}

bool worker_helper::thermo_point_set(int idx, const float &value) {
  return m_mainworker.SetSetPoint(std::to_string(idx), value);
}

void worker_helper::add_hardware(CDomoticzHardwareBase *hardware) {
  m_mainworker.AddDomoticzHardware(hardware);
}

void worker_helper::remove_hardware(CDomoticzHardwareBase *hardware) {
  m_mainworker.RemoveDomoticzHardware(hardware);
}
