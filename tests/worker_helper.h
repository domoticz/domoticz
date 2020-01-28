#pragma once

#include <memory>
#include <thread>

class CDomoticzHardwareBase;
class BleBox;
struct _tColor;

class worker_helper {
public:
  static void add_hardware(CDomoticzHardwareBase *hardware);
  static void remove_hardware(CDomoticzHardwareBase *hardware);

  void start_with_hardware(CDomoticzHardwareBase *hardware);
  void stop();

  bool rgbw_color_set(int idx, uint8_t r, uint8_t g, uint8_t b,
                      uint8_t level_100);
  bool rgbw_whitemode_set(int idx, uint8_t level_100);

  bool rgbw_off(int idx);
  bool rgbw_on(int idx);

  // methods that mostly simulate how UI triggers WriteToHarware
  // TODO: rename based on domoticz type/stype/swtype
  // TODO: pick based on current device type
  static bool set_level(int idx, uint8_t level);
  static bool push(int idx);
  static bool switch_on(int idx);
  static bool switch_off(int idx);
  static bool thermo_point_set(int idx, const float &value);

private:
  std::unique_ptr<std::thread> _msg_thread;
};
