//bt_openwebnet.h
//class bt_openwebnet is a modification of GNU bticino C++ openwebnet client
//from openwebnet class
//see www.bticino.it; www.myhome-bticino.it

#pragma once

#include <string>
#include <vector>

#define OPENWEBNET_MSG_OPEN_OK "*#*1##"
#define OPENWEBNET_MSG_OPEN_KO  "*#*0##"
#define OPENWEBNET_COMMAND_SESSION "*99*0##"
#define OPENWEBNET_EVENT_SESSION "*99*1##"
#define OPENWEBNET_END_FRAME "##"
#define OPENWEBNET_COMMAND_SOCKET_DURATION 30


typedef enum {
	TEMPERATURE_CONTROL_DIMENSION_TEMPERATURE = 0,
	TEMPERATURE_CONTROL_DIMENSION_FAN_COIL_SPEED = 11,
	TEMPERATURE_CONTROL_DIMENSION_COMPLETE_PROBE_STATUS = 12,
	TEMPERATURE_CONTROL_DIMENSION_LOCAL_SET_OFFSET = 13,
	TEMPERATURE_CONTROL_DIMENSION_SET_POINT_TEMPERATURE = 14,
	TEMPERATURE_CONTROL_DIMENSION_VALVES_STATUS = 19,
	TEMPERATURE_CONTROL_DIMENSION_ACTUATOR_STATUS = 20,
	TEMPERATURE_CONTROL_DIMENSION_SPLIT_CONTROL = 22,
	TEMPERATURE_CONTROL_DIMENSION_END_DATE_HOLIDAY_SCENARIO = 30
}_eTemperatureControlDimension;

typedef enum {
	WHO_SCENARIO = 0,
	WHO_LIGHTING = 1,
	WHO_AUTOMATION = 2,
	WHO_LOAD_CONTROL = 3,
	WHO_TEMPERATURE_CONTROL = 4,
	WHO_BURGLAR_ALARM = 5,
	WHO_DOOR_ENTRY_SYSTEM = 6,
	WHO_MULTIMEDIA = 7,
	WHO_AUXILIARY = 9,
	WHO_GATEWAY_INTERFACES_MANAGEMENT = 13,
	WHO_LIGHT_SHUTTER_ACTUATOR_LOCK = 14,
	WHO_SCENARIO_SCHEDULER_SWITCH = 15,
	WHO_AUDIO = 16,
	WHO_SCENARIO_PROGRAMMING = 17,
	WHO_ENERGY_MANAGEMENT = 18,
	WHO_SOUND_DIFFUSION = 22,
	WHO_LIHGTING_MANAGEMENT = 24,
	WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION = 25,
	WHO_ZIGBEE_DIAGNOSTIC = 1000,
	WHO_AUTOMATIC_DIAGNOSTIC = 1001,
	WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES = 1004,
	WHO_DEVICE_DIAGNOSTIC = 1013,
	WHO_ENERGY_MANAGEMENT_DIAGNOSTIC = 1018
}_eWho;

//"What" enumerations

typedef enum  {
	SCENARIO_WHAT_SCENARIO1 = 1,
	SCENARIO_WHAT_SCENARIO2 = 2,
	SCENARIO_WHAT_SCENARIO3 = 3,
	SCENARIO_WHAT_SCENARIO4 = 4,
	SCENARIO_WHAT_SCENARIO5 = 5,
	SCENARIO_WHAT_SCENARIO6 = 6,
	SCENARIO_WHAT_SCENARIO7 = 7,
	SCENARIO_WHAT_SCENARIO8 = 8,
	SCENARIO_WHAT_SCENARIO9 = 9,
	SCENARIO_WHAT_SCENARIO10 = 10,
	SCENARIO_WHAT_SCENARIO11 = 11,
	SCENARIO_WHAT_SCENARIO12 = 12,
	SCENARIO_WHAT_SCENARIO13 = 13,
	SCENARIO_WHAT_SCENARIO14 = 14,
	SCENARIO_WHAT_SCENARIO15 = 15,
	SCENARIO_WHAT_SCENARIO16 = 16,
	SCENARIO_WHAT_SCENARIO17 = 17,
	SCENARIO_WHAT_SCENARIO18 = 18,
	SCENARIO_WHAT_SCENARIO19 = 19,
	SCENARIO_WHAT_SCENARIO20 = 20,
	SCENARIO_WHAT_START_RECORDING = 40,
	SCENARIO_WHAT_END_RECORDING = 41,
	SCENARIO_WHAT_ERASE_SCENARIO = 42,
	SCENARIO_WHAT_LOCK_SCENARIO_CENTRAL_UNIT = 43,
	SCENARIO_WHAT_UNLOCK_SCENARIO_CENTRAL_UNIT = 44,
	SCENARIO_WHAT_UNAVAILABLE_SCENARIOS_CENTRAL_UNIT = 45,
	SCENARIO_WHAT_ERASE_SCENARIOS_CENTRAL_UNIT = 46
}_eScenarioWhat;

typedef enum  {
	LIGHTING_WHAT_OFF = 0,
	LIGHTING_WHAT_ON = 1,
	LIGHTING_WHAT_20 = 2,
	LIGHTING_WHAT_30 = 3,
	LIGHTING_WHAT_40 = 4,
	LIGHTING_WHAT_50 = 5,
	LIGHTING_WHAT_60 = 6,
	LIGHTING_WHAT_70 = 7,
	LIGHTING_WHAT_80 = 8,
	LIGHTING_WHAT_90 = 9,
	LIGHTING_WHAT_100 = 10,
	LIGHTING_WHAT_ON_TIMED_1_MIN = 11,
	LIGHTING_WHAT_ON_TIMED_2_MIN = 12,
	LIGHTING_WHAT_ON_TIMED_3_MIN = 13,
	LIGHTING_WHAT_ON_TIMED_4_MIN = 14,
	LIGHTING_WHAT_ON_TIMED_5_MIN = 15,
	LIGHTING_WHAT_ON_TIMED_15_MIN = 16,
	LIGHTING_WHAT_ON_TIMED_30_SEC = 17,
	LIGHTING_WHAT_ON_TIMED_0_5_SEC = 18,
	LIGHTING_WHAT_BLINK_0_5_SEC = 20,
	LIGHTING_WHAT_BLINK_1_SEC = 21,
	LIGHTING_WHAT_BLINK_1_5_SEC = 22,
	LIGHTING_WHAT_BLINK_2_SEC = 23,
	LIGHTING_WHAT_BLINK_2_5_SEC = 24,
	LIGHTING_WHAT_BLINK_3_SEC = 25,
	LIGHTING_WHAT_BLINK_3_5_SEC = 26,
	LIGHTING_WHAT_BLINK_4_SEC = 27,
	LIGHTING_WHAT_BLINK_4_5_SEC = 28,
	LIGHTING_WHAT_BLINK_5_SEC = 29,
	LIGHTING_WHAT_UP_ONE_LEVEL = 30,
	LIGHTING_WHAT_DOWN_ONE_LEVEL = 31,
	LIGHTING_WHAT_COMMAND_TRANSLATION = 1000
}_eLightingWhat;


typedef enum {
	AUXILIARY_WHAT_OFF = 0,
	AUXILIARY_WHAT_ON = 1
}_eAuxiliaryWhat;

typedef enum {
	AUTOMATION_WHAT_STOP = 0,
	AUTOMATION_WHAT_UP = 1,
	AUTOMATION_WHAT_DOWN = 2,
	AUTOMATION_WHAT_STOP_ADVANCED = 10,
	AUTOMATION_WHAT_UP_ADVANCED = 11,
	AUTOMATION_WHAT_DOWN_ADVANCED = 12
}_eAutomationWhat;

typedef enum {
	LOAD_CONTROL_WHAT_DISABLED = 0,
	LOAD_CONTROL_WHAT_ENABLED = 1,
	LOAD_CONTROL_WHAT_FORCED = 2,
	LOAD_CONTROL_WHAT_REMOVE_FORCED = 3
}_eLoadControlWhat;

typedef enum {
	SOUND_DIFFUSION_WHAT_TURN_OFF = 0,
	SOUND_DIFFUSION_WHAT_TURN_ON = 1,
	SOUND_DIFFUSION_WHAT_SOURCE_TURNED_ON = 2,
	SOUND_DIFFUSION_WHAT_INCREASE_VOLUME = 3,
	SOUND_DIFFUSION_WHAT_DECREASE_VOLUME = 4,
	SOUND_DIFFUSION_WHAT_TUNER_HIGHER_FREQUENCY = 5,
	SOUND_DIFFUSION_WHAT_TUNER_LOWER_FREQUENCY = 6,
	SOUND_DIFFUSION_WHAT_FOLLOWING_STATION = 9,
	SOUND_DIFFUSION_WHAT_PREVIOUS_STATION = 10,
	SOUND_DIFFUSION_WHAT_SLIDING_REQUEST = 22,
	SOUND_DIFFUSION_WHAT_START_TELLING_RDS = 31,
	SOUND_DIFFUSION_WHAT_STOP_TELLING_RDS = 32,
	SOUND_DIFFUSION_WHAT_STORE_TUNED_FREQUENCY = 33,
	SOUND_DIFFUSION_WHAT_TURN_ON_FOLLOW_ME = 34,
	SOUND_DIFFUSION_WHAT_TURN_ON_AMPLIFIER_FOR_SOURCE = 35,
	SOUND_DIFFUSION_WHAT_INCREMENT_LOW_TONES = 36,
	SOUND_DIFFUSION_WHAT_DECREMENT_LOW_TONES = 37,
	SOUND_DIFFUSION_WHAT_INCREMENT_MID_TONES = 38,
	SOUND_DIFFUSION_WHAT_DECREMENT_MID_TONES = 39,
	SOUND_DIFFUSION_WHAT_INCREMENT_HIGH_TONES = 40,
	SOUND_DIFFUSION_WHAT_DECREMENT_HIGH_TONES = 41,
	SOUND_DIFFUSION_WHAT_BALANCE_LEFT_TO_RIGHT = 42,
	SOUND_DIFFUSION_WHAT_BALANCE_RIGHT_TO_LEFT = 43,
	SOUND_DIFFUSION_WHAT_NEXT_PRESET = 55,
	SOUND_DIFFUSION_WHAT_PREVIOUS_PRESET = 56
}_eSoundDiffusionWhat;

typedef enum {
	DRY_CONTACT_IR_DETECTION_WHAT_ON = 31,
	DRY_CONTACT_IR_DETECTION_WHAT_OFF = 32
}_eDryContactIrDetectionWhat;

typedef enum {
	DRY_CONTACT_IR_DETECTION_WHAT_PARAM_STATUS_REQUESTED = 0,
	DRY_CONTACT_IR_DETECTION_WHAT_PARAM_EVENT_OCCURRED = 1
}_eDryContactIrDetectionWhatParameters;

typedef enum {
	SCENARIO_PROGRAMMING_WHAT_START_SCENE = 1,
	SCENARIO_PROGRAMMING_WHAT_STOP_SCENE = 2,
	SCENARIO_PROGRAMMING_WHAT_ENABLE_SCENE = 3,
	SCENARIO_PROGRAMMING_WHAT_DISABLE_SCENE = 4
}_eScenarioProgrammingWhat;

//"Where" enumerations

typedef enum {
	LOAD_CONTROL_WHERE_GENERAL = 0,
	LOAD_CONTROL_WHERE_CONTRL_UNIT = 10
}_eLoadControlWhere;

//"Dimensions" enumerations

typedef enum {
	LIGHTING_DIMENSION_SET_UP_LEVEL_WITH_GIVEN_SPEED = 1,
	LIGHTING_DIMENSION_TEMPORISATION = 2,
	LIGHTING_DIMENSION_REQUIRED_ONLY_ON_LIGHT = 3,
	LIGHTING_DIMENSION_STATUS_DIMMER_100_LEVELS_WITH_GIVEN_SPEED = 4,
	LIGHTING_DIMENSION_WORKING_TIME_LAMP = 8,
	LIGHTING_DIMENSION_MAX_WORKING_TIME_LAMP = 9
}_eLightingDimension;

typedef enum  {
	AUTOMATION_DIMENSION_SHUTTER_STATUS = 10,
	AUTOMATION_DIMENSION_GOTO_LEVEL = 11
}_eAutomationDimension;

typedef enum{
	LOAD_CONTROL_WHAT_ALL_DIMENSIONS = 0,
	LOAD_CONTROL_WHAT_VOLTAGE = 1,
	LOAD_CONTROL_WHAT_CURRENT = 2,
	LOAD_CONTROL_WHAT_POWER = 3,
	LOAD_CONTROL_WHAT_ENERGY = 4
}_eLoadControlDimension;


class bt_openwebnet {

private:

  // various constants
  const static int MAX_LENGTH_OPEN  = 1024;
  const static int ERROR_FRAME      = 1;
  const static int NULL_FRAME       = 2;
  const static int NORMAL_FRAME     = 3;
  const static int MEASURE_FRAME    = 4;
  const static int STATE_FRAME      = 5;
  const static int OK_FRAME         = 6;
  const static int KO_FRAME         = 7;
  const static int WRITE_FRAME      = 8;
  const static int PWD_FRAME        = 9;


  // assign who, what, where and when for normal frame
  void Set_who_what_where_when();
  // assign who, where, and dimension for dimension frame request
  void Set_who_where_dimension();
  // assign who and where for request state frame
  void Set_who_where();
  // assign who, where, dimension and value for write dimension frame
  void Set_who_where_dimension_values();
  // assign who for frame result of elaborate password
  void Set_who();
  // assign level, interface for extended frame
  void Set_level_interface();
  // assign address
  void Set_address();
  // assign what parameters
  void Set_whatParameters();
  // assign where Parameters
  void Set_whereParameters();

  // check frame syntax
  void IsCorrect();

  //misc functions
  std::string DeleteControlCharacters(const std::string& in_frame);
  std::string FirstToken(const std::string& text, const std::string& delimiter);
  static std::string vectorToString(const std::vector<std::string>& strings);
  void tokenize(const std::string& strToTokenize, const char token, std::string& out_firstToken, std::vector<std::string>& out_otherTokens);

  //fields description
  static std::string getDimensionsDescription(const std::string& who, const std::string& dimension, const std::vector<std::string>& values);
  static std::string getWhereDescription(const std::string& who,const std::string& what, const std::string& where, const std::vector<std::string>& whereParameters);
  static std::string getWhatDescription(const std::string& who, const std::string& what, const std::vector<std::string>& whatParameters);
  static std::string getWhoDescription(const std::string& who);

  // contents of normal frame
  std::string who;
  std::vector<std::string> addresses;
  std::string what;
  std::vector<std::string> whatParameters;
  std::string where;
  std::vector<std::string> whereParameters;
  std::string level;
  std::string sInterface;
  std::string when;
  std::string dimension;
  std::vector<std::string> values;

  // frame length
  int length_frame_open;

public:

  // frame
	std::string frame_open;

  // type of frame open
  int frame_type;

  //indicates extended frame
  bool extended;

  // constructors
  bt_openwebnet();
  bt_openwebnet(const std::string& message);
  bt_openwebnet(int who, int what, int where, int group);
  bt_openwebnet(const std::string& who, const std::string& what, const std::string& where, const std::string& when);

  void CreateNullMsgOpen();
  //normal open
  void CreateMsgOpen(const std::string& who, const std::string& what, const std::string& where, const std::string& when);
  void CreateMsgOpen(const std::string& who, const std::string& what, const std::string& where, const std::string& lev, const std::string& strInterface, const std::string& when);
  //state request
  void CreateStateMsgOpen(const std::string& who, const std::string& where);
  void CreateStateMsgOpen(const std::string& who, const std::string& where, const std::string& lev, const std::string& strInterface);
  //dimension request
  void CreateDimensionMsgOpen(const std::string& who, const  std::string& where, const std::string& dimension);
  void CreateDimensionMsgOpen(const std::string& who, const std::string& where, const std::string& lev, const std::string& strInterface, const std::string& dimension);
  //dimension write
  void CreateWrDimensionMsgOpen(const std::string& who, const std::string& where, const std::string& dimension, const std::vector<std::string>& value);
  void CreateWrDimensionMsgOpen(const std::string& who, const std::string& where, const std::string& lev, const std::string& strInterface, const std::string& dimension, const std::vector<std::string>& value);

  void CreateTimeReqMsgOpen();

	void CreateSetTimeMsgOpen();
  //general message
  void CreateMsgOpen(const std::string& message);

  // compares two open messages
  bool IsEqual(const bt_openwebnet& msg_to_compare);

  // frame type?
  bool IsErrorFrame() const;
  bool IsNullFrame() const;
  bool IsNormalFrame() const;
  bool IsMeasureFrame() const;
  bool IsStateFrame() const;
  bool IsWriteFrame() const;
  bool IsPwdFrame() const;
  bool IsOKFrame() const;
  bool IsKOFrame() const;

  //converts frame into string
  static std::string frameToString(const bt_openwebnet& frame);


  // extract who, addresses, what, where, level, interface, when
  // dimensions and values of frame open
  std::string Extract_who() const;
  std::string Extract_address(unsigned int i) const;
  std::string Extract_what() const;
  std::string Extract_where() const;
  std::string Extract_level() const;
  std::string Extract_interface() const;
  std::string Extract_when() const;
  std::string Extract_dimension() const;
  std::string Extract_value(unsigned int i) const;

  std::string Extract_OpenOK();
  std::string Extract_OpenKO();

  std::vector<std::string> Extract_addresses() const;
  std::vector<std::string> Extract_whatParameters() const;
  std::vector<std::string> Extract_whereParameters() const;
  std::vector<std::string> Extract_values() const;

  std::string Extract_frame() const;

  // destructor
  ~bt_openwebnet();

};
