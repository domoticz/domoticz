#pragma once

#define SensorZLLSwitchEventInitialPress 0
#define SensorZLLSwitchEventRepeat 1
#define SensorZLLSwitchEventShortRelease 2
#define SensorZLLSwitchEventLongRelease 3
#define SensorZLLSwitchEventSimple 5

#define EVENT_INITIAL_PRESS "initial_press"
#define EVENT_REPEAT "repeat"
#define EVENT_SHORT_RELEASE "short_release"
#define EVENT_LONG_RELEASE "long_release"

namespace Json
{
	class Value;
} // namespace Json

class CPHSensorState
{
public:
  CPHSensorState() = default;
  ;
  CPHSensorState(const Json::Value &state, const Json::Value &capabilities);

  bool operator==(const CPHSensorState &other);

  void ExtractButtonInfo(const Json::Value &state, const Json::Value &capabilities);
  int32_t GetSelectorLevel(const CPHSensorState &previous_state);
  std::map<std::string, std::string> GetButtonOptions();

  std::string m_lastupdated;
  int m_buttonevent = 0;
  bool m_presence = false;
  int m_temperature = 0;
  int m_lightlevel = 0;
  bool m_dark = false;
  bool m_daylight = false;
  bool m_button_valid = false;
  int m_button_count = 0;
  bool m_button_can_long_press = false;
  int m_button_nr = 0;
  int m_button_eventtype = SensorZLLSwitchEventSimple;
};

class CPHSensorConfig
{
public:
  CPHSensorConfig() = default;
  ;
  CPHSensorConfig(const Json::Value &config);

  bool m_on = false;
  bool m_reachable = false;
  int m_battery = 255;
  int m_tholddark = 0;
  int m_tholdoffset = 0;
  int m_sunriseoffset = 0;
  int m_sunsetoffset = 0;
  bool m_configured = false;
};


class CPHSensor
{
public:
  CPHSensor() = default;
  ;
  CPHSensor(const Json::Value &sensor);

  std::string m_name;
  std::string m_type;
  std::string m_model_id;
  std::string m_manufacturer_name;
  std::string m_sw_version;
  std::string m_uniqueid;
  CPHSensorState m_state;
  CPHSensorConfig m_config;
};
