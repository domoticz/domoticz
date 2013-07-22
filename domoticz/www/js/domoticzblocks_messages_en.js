/**
 * Visual Blocks Language
 *
 * Copyright 2012 Google Inc.
 * http://blockly.googlecode.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview English strings.
 * @author fraser@google.com (Neil Fraser)
 */
'use strict';

/**
 * Due to the frequency of long strings, the 80-column wrap rule need not apply
 * to message files.
 */

// Control Blocks.
Blockly.DOMOTICZCONTROLS_IF_TOOLTIP = 'If statements about one or more devices or time schedules are true, then do some actions (set Lamp = "On").';
// Logic Blocks.
Blockly.LANG_LOGIC_COMPARE_TOOLTIP_EQ = 'Return true if both inputs equal each other.';
Blockly.LANG_LOGIC_COMPARE_TOOLTIP_NEQ = 'Return true if both inputs are not equal to each other.';
Blockly.LANG_LOGIC_COMPARE_TOOLTIP_LT = 'Return true if the first input is smaller\n' +
    'than the second input.';
Blockly.LANG_LOGIC_COMPARE_TOOLTIP_LTE = 'Return true if the first input is smaller\n' +
    'than or equal to the second input.';
Blockly.LANG_LOGIC_COMPARE_TOOLTIP_GT = 'Return true if the first input is greater\n' +
    'than the second input.';
Blockly.LANG_LOGIC_COMPARE_TOOLTIP_GTE = 'Return true if the first input is greater\n' +
    'than or equal to the second input.';
Blockly.LANG_LOGIC_OPERATION_TOOLTIP_AND = 'Return true if both inputs are true.';
Blockly.LANG_LOGIC_OPERATION_TOOLTIP_OR = 'Return true if either inputs are true.';
Blockly.LANG_LOGIC_NEGATE_INPUT_NOT = 'not';
Blockly.LANG_LOGIC_NEGATE_TOOLTIP = 'Returns true if the input is false.\n' +
    'Returns false if the input is true.';
Blockly.LANG_LOGIC_BOOLEAN_TOOLTIP = 'Returns either true or false.';
Blockly.DOMOTICZSWITCHES_TOOLTIP = 'A list of switchable devices. Can be used in an IF statement to check current state, or in a "Set" block in a DO statement to change the state.';
Blockly.DOMOTICZVARIABLES_TEMPERATURE_TOOLTIP = 'This is a temperature measurement device. Use it in an IF statement to check valid temperature values,\ni.e. "if Bathroom temperature > 20". Cannot be used in Set blocks.';
Blockly.DOMOTICZVARIABLES_HUMIDITY_TOOLTIP = 'This is a humidity measurement device. Use it in an IF statement to check valid humidity values, i.e. "if Bathroom humidity > 50". The percentage sign is omitted. Cannot be used in Set blocks.';
Blockly.DOMOTICZVARIABLES_BAROMETER_TOOLTIP = 'This is a barometer measurement device. Use it in an IF statement to check valid barometer values,\ni.e. "if Outside barometer > 1020". Cannot be used in Set blocks.';
Blockly.DOMOTICZVARIABLES_WEATHER_TOOLTIP = 'NOT WORKING YET. This is a weather measurement device. Use it in an IF statement to check valid values. Cannot be used in Set blocks.';
Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP = 'NOT WORKING YET. This is a utility measurement device. Use it in an IF statement to check valid values. Cannot be used in Set blocks.';
Blockly.DOMOTICZVARIABLES_SCENES_TOOLTIP = ' Not implemented yet.';
Blockly.DOMOTICZ_LOGIC_SET_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device.'; 
Blockly.DOMOTICZ_LOGIC_SETDELAYED_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device for a specified number of minutes.\n E.g. if you want to switch a lamp on for only 10 minutes.'; 
Blockly.DOMOTICZ_LOGIC_SETRANDOM_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device randomly within a specified timeframe.\n E.g. switch on a lamp somewhere in the next 30 minutes.'; 
Blockly.DOMOTICZ_LOGIC_SETLEVEL_TOOLTIP = 'Use this block in a "Do" statement to change the dim level of a designated dimmable device.\n Input value must be a percentage, i.e. 0-100.'; 
Blockly.DOMOTICZ_LOGIC_TIMEVALUE_TOOLTIP = 'Enter a valid time in 24 hour format, e.g. 23:59';
Blockly.DOMOTICZ_LOGIC_WEEKDAY_TOOLTIP = 'Use this block in an IF statement to check the day of the week.';
Blockly.DOMOTICZ_LOGIC_SUNRISESUNSET_TOOLTIP = 'Use this block to check against sunrise/sunset time as specified in the Domoticz preferences.';
Blockly.DOMOTICZ_LOGIC_NOTIFICATION_TOOLTIP = 'Use this block in a DO statement to send a notification based on the IF statement. Use text blocks from this group to specify subject and message.\nDepending on the preferences setting the message will go to either email or notification service or both.';     
Blockly.DOMOTICZCONTROLS_MSG_IF = 'If';
Blockly.DOMOTICZCONTROLS_MSG_DO = 'Do';
Blockly.DOMOTICZCONTROLS_MSG_TEMP = 'temp.';
Blockly.DOMOTICZCONTROLS_MSG_HUM = 'hum.';
Blockly.DOMOTICZCONTROLS_MSG_BARO = 'baro.';
Blockly.DOMOTICZCONTROLS_MSG_SET = 'Set ';
Blockly.DOMOTICZCONTROLS_MSG_FOR = 'For ';
Blockly.DOMOTICZCONTROLS_MSG_MINUTES = 'minutes';
Blockly.DOMOTICZCONTROLS_MSG_RANDOM = 'Random within';
Blockly.DOMOTICZCONTROLS_MSG_SETLEVEL = 'Set Level (%)';
Blockly.DOMOTICZCONTROLS_MSG_TIME = 'Time';
Blockly.DOMOTICZCONTROLS_MSG_DAY = 'Day ';








