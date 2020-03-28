#pragma once

typedef enum {
    ACT_ONOFF_PLUG = 1, // TODO: also used for channel 0?
    ACT_ONOFF_WIRED_WALL, // TODO: what is this? single or dual channel wired wall switch?
    GATEWAY_SOUND_ALARM_RINGTONE,
    GATEWAY_SOUND_ALARM_CLOCK,
    GATEWAY_SOUND_DOORBELL,
    GATEWAY_SOUND_MP3,
    GATEWAY_SOUND_VOLUME_CONTROL,
    SELECTOR_WIRED_WALL_SINGLE,
    SELECTOR_WIRED_WALL_DUAL_CHANNEL_0,
    SELECTOR_WIRED_WALL_DUAL_CHANNEL_1
} XiaomiUnitCode;

/****************************************************************************
 ********************************* SWITCHES *********************************
 ****************************************************************************/

// Xiaomi Switch
#define MODEL_SELECTOR_XIAOMI ""
#define NAME_SELECTOR_XIAOMI "Xiaomi Switch"

// Xiaomi Wireless Switch
#define MODEL_SELECTOR_WIRELESS_SINGLE_1 "switch"
#define MODEL_SELECTOR_WIRELESS_SINGLE_2 "remote.b1acn01"
#define NAME_SELECTOR_WIRELESS_SINGLE "Xiaomi Wireless Switch"

// Xiaomi Square Wireless Switch
#define MODEL_SELECTOR_WIRELESS_SINGLE_SQUARE "sensor_switch.aq2"
#define NAME_SELECTOR_WIRELESS_SINGLE_SQUARE "Xiaomi Square Wireless Switch"

// Xiaomi Wireless Single Wall Switch
#define MODEL_SELECTOR_WIRELESS_WALL_SINGLE_1 "86sw1"
#define MODEL_SELECTOR_WIRELESS_WALL_SINGLE_2 "remote.b186acn01"
#define NAME_SELECTOR_WIRELESS_WALL_SINGLE "Xiaomi Wireless Single Wall Switch"

// Xiaomi Wireless Dual Wall Switch
#define MODEL_SELECTOR_WIRELESS_WALL_DUAL_1 "86sw2"
#define MODEL_SELECTOR_WIRELESS_WALL_DUAL_2 "remote.b286acn01"
#define NAME_SELECTOR_WIRELESS_WALL_DUAL "Xiaomi Wireless Dual Wall Switch"

// Xiaomi Wired Single Wall Switch
#define MODEL_SELECTOR_WIRED_WALL_SINGLE_1 "ctrl_neutral1"
#define MODEL_SELECTOR_WIRED_WALL_SINGLE_2 "ctrl_ln1"
#define MODEL_SELECTOR_WIRED_WALL_SINGLE_3 "ctrl_ln1.aq1"
#define NAME_SELECTOR_WIRED_WALL_SINGLE "Xiaomi Wired Single Wall Switch"

// Xiaomi Wired Dual Wall Switch
#define MODEL_SELECTOR_WIRED_WALL_DUAL_1 "ctrl_neutral2"
#define MODEL_SELECTOR_WIRED_WALL_DUAL_2 "ctrl_ln2"
#define MODEL_SELECTOR_WIRED_WALL_DUAL_3 "ctrl_ln2.aq1"
#define NAME_SELECTOR_WIRED_WALL_DUAL "Xiaomi Wired Dual Wall Switch"
#define NAME_SELECTOR_WIRED_WALL_DUAL_CHANNEL_0 "Xiaomi Wired Dual Wall Switch Channel 0"
#define NAME_SELECTOR_WIRED_WALL_DUAL_CHANNEL_1 "Xiaomi Wired Dual Wall Switch Channel 1"

// Xiaomi Smart Push Button
#define MODEL_SELECTOR_WIRELESS_SINGLE_SMART_PUSH "sensor_switch.aq3"
#define NAME_SELECTOR_WIRELESS_SINGLE_SMART_PUSH "Xiaomi Smart Push Button"

// Xiaomi Cube
#define MODEL_SELECTOR_CUBE_V1 "cube"
#define NAME_SELECTOR_CUBE_V1 "Xiaomi Cube"
#define MODEL_SELECTOR_CUBE_AQARA "sensor_cube.aqgl01" 
#define NAME_SELECTOR_CUBE_AQARA "Aqara Cube"

/****************************************************************************
 ********************************* SENSORS **********************************
 ****************************************************************************/

// Motion Sensor Xiaomi
#define MODEL_SENSOR_MOTION_XIAOMI "motion"
#define NAME_SENSOR_MOTION_XIAOMI "Xiaomi Motion Sensor"

// Motion Sensor Aqara
#define MODEL_SENSOR_MOTION_AQARA "sensor_motion.aq2"
#define NAME_SENSOR_MOTION_AQARA "Aqara Motion Sensor"

// Xiaomi Door Sensor
#define MODEL_SENSOR_DOOR "magnet"
#define MODEL_SENSOR_DOOR_AQARA "sensor_magnet.aq2"
#define NAME_SENSOR_DOOR "Xiaomi Door Sensor"

// Xiaomi Temperature/Humidity
#define MODEL_SENSOR_TEMP_HUM_V1 "sensor_ht"
#define NAME_SENSOR_TEMP_HUM_V1 "Xiaomi Temperature/Humidity"

// Xiaomi Aqara Weather
#define MODEL_SENSOR_TEMP_HUM_AQARA "weather.v1"
#define NAME_SENSOR_TEMP_HUM_AQARA "Xiaomi Aqara Weather"

// Aqara Vibration Sensor
#define MODEL_SENSOR_VIBRATION "vibration"
#define NAME_SENSOR_VIBRATION "Aqara Vibration Sensor"

// Smoke Detector
#define MODEL_SENSOR_SMOKE "smoke"
#define NAME_SENSOR_SMOKE "Xiaomi Smoke Detector"

// Xiaomi Gas Detector
#define MODEL_SENSOR_GAS "natgas"
#define NAME_SENSOR_GAS "Xiaomi Gas Detector"

// Xiaomi Water Leak Detector
#define MODEL_SENSOR_WATER "sensor_wleak.aq1"
#define NAME_SENSOR_WATER "Xiaomi Water Leak Detector"

/****************************************************************************
 ********************************* ACTUATORS ********************************
 ****************************************************************************/

// Xiaomi Smart Plug
#define MODEL_ACT_ONOFF_PLUG "plug"
#define NAME_ACT_ONOFF_PLUG "Xiaomi Smart Plug"

// Xiaomi Smart Wall Plug
#define MODEL_ACT_ONOFF_PLUG_WALL_1 "86plug"
#define MODEL_ACT_ONOFF_PLUG_WALL_2 "ctrl_86plug.aq1"
#define NAME_ACT_ONOFF_PLUG_WALL "Xiaomi Smart Wall Plug"

// Xiaomi Curtain
#define MODEL_ACT_BLINDS_CURTAIN "curtain"
#define NAME_ACT_BLINDS_CURTAIN "Xiaomi Curtain"


/*****************************************§***********************************
 ********************************* GATEWAY **********************************
 ****************************************************************************/

 // Xiaomi Gateway
#define MODEL_GATEWAY_1 "gateway"
#define MODEL_GATEWAY_2 "gateway.v3"
#define MODEL_GATEWAY_3 "acpartner.v3"
#define NAME_GATEWAY "Xiaomi RGB Gateway"
#define NAME_GATEWAY_MP3 "Xiaomi Gateway MP3"
#define NAME_GATEWAY_SOUND_DOORBELL "Xiaomi Gateway Doorbell"
#define NAME_GATEWAY_SOUND_ALARM_CLOCK "Xiaomi Gateway Alarm Clock"
#define NAME_GATEWAY_SOUND_ALARM_RINGTONE "Xiaomi Gateway Alarm Ringtone"