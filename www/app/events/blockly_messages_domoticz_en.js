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

define(['blockly'], function() {
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
    Blockly.DOMOTICZUSERVARIABLES_TOOLTIP = 'A list of user defined variables. Can be used in an IF statement to check current value, or in a "Set" block in a DO statement to change the value.';
    Blockly.DOMOTICZVARIABLES_TEMPERATURE_TOOLTIP = 'This is a temperature measurement device. Use it in an IF statement to check valid temperature values,\ni.e. "if Bathroom temperature > 20". Cannot be used in Set blocks.';
    Blockly.DOMOTICZVARIABLES_HUMIDITY_TOOLTIP = 'This is a humidity measurement device. Use it in an IF statement to check valid humidity values, i.e. "if Bathroom humidity > 50". The percentage sign is omitted. Cannot be used in Set blocks.';
    Blockly.DOMOTICZVARIABLES_DEWPOINT_TOOLTIP = 'This is a dewpoint measurement device. Use it in an IF statement to check valid temperature values,\ni.e. "if Bathroom temperature > 20". Cannot be used in Set blocks.';
    Blockly.DOMOTICZVARIABLES_BAROMETER_TOOLTIP = 'This is a barometer measurement device. Use it in an IF statement to check valid barometer values,\ni.e. "if Outside barometer > 1020". Cannot be used in Set blocks.';
    Blockly.DOMOTICZVARIABLES_WEATHER_TOOLTIP = 'This is a weather measurement device. Use it in an IF statement to check valid values. Cannot be used in Set blocks.';
    Blockly.DOMOTICZVARIABLES_UTILITY_TOOLTIP = 'This is a utility measurement device. Use it in an IF statement to check valid utility values,\ni.e. "if EnergyMeter value > 2000". Cannot be used in Set blocks.';
    Blockly.DOMOTICZVARIABLES_TEXT_TOOLTIP = 'Use this block in a "Do" statement to change the value of a text device.';
    Blockly.DOMOTICZVARIABLES_SCENES_TOOLTIP = ' Scenes can only be used in a Do statement (i.e cannot be checked for a status).';
    Blockly.DOMOTICZVARIABLES_GROUPS_TOOLTIP = ' Groups can only be used in a Do statement (i.e cannot be checked for a status).';
    Blockly.DOMOTICZ_LOGIC_SET_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device.';
    Blockly.DOMOTICZ_LOGIC_SETAFTER_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device after a specified number of seconds.\n E.g. if you want to switch a lamp after 10 seconds.';
    Blockly.DOMOTICZ_LOGIC_SETDELAYED_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device for a specified number of minutes.\n E.g. if you want to switch a lamp on for only 10 minutes.';
    Blockly.DOMOTICZ_LOGIC_SETRANDOM_TOOLTIP = 'Use this block in a "Do" statement to change the state of a designated device randomly within a specified timeframe.\n E.g. switch on a lamp somewhere in the next 30 minutes.';
    Blockly.DOMOTICZ_LOGIC_SETLEVEL_TOOLTIP = 'Use this block in a "Do" statement to change the dim level of a designated dimmable device.\n Input value must be a percentage, i.e. 0-100.';
    Blockly.DOMOTICZ_LOGIC_TIMEVALUE_TOOLTIP = 'Enter a valid time in 24 hour format, e.g. 23:59';
    Blockly.DOMOTICZ_LOGIC_WEEKDAY_TOOLTIP = 'Use this block in an IF statement to check the day of the week.';
    Blockly.DOMOTICZ_LOGIC_SUNRISESUNSET_TOOLTIP = 'Use this block to check against sunrise/sunset time as specified in the Domoticz preferences.';
    Blockly.DOMOTICZ_LOGIC_NOTIFICATION_TOOLTIP = 'Use this block in a DO statement to send a notification based on the IF statement. Use text blocks from this group to specify subject and message.\nDepending on the preferences setting the message will go to either email or notification service or both.';
    Blockly.DOMOTICZ_LOGIC_EMAIL_TOOLTIP = 'Use this block in a DO statement to send a email based on the IF statement. Use text blocks from this group to specify subject, message and to fields.';
    Blockly.DOMOTICZ_LOGIC_SMS_TOOLTIP = 'Use this block in a DO statement to send a SMS based on the IF statement. Use text blocks from this group to specify subject field.';
    Blockly.DOMOTICZ_LOGIC_IFTTT_TOOLTIP = 'Use this block in a DO statement to trigger a IFTTT Maker/Webhook event. Use text blocks from this group to specify EventName and optional values.';
    Blockly.DOMOTICZ_LOGIC_SCRIPT_TOOLTIP = 'Use this block in a DO statement to start a script. (/home/user/myscript.sh)';
    Blockly.DOMOTICZ_LOGIC_NOTIFICATION_PRIORITY_TOOLTIP = 'Use this block in a DO statement to set the priority of a notificatio.';
    Blockly.DOMOTICZ_SECURITY_STATUS_TOOLTIP = 'The current state of the Domoticz security system. Can only be checked, not set through an event.';
    Blockly.DOMOTICZ_LOGIC_OPENURL_TOOLTIP = 'Enter a url to open. This does not actually retrieve anything from the url, use it to trigger an external api.';
    Blockly.DOMOTICZ_LOGIC_URL_TEXT_TOOLTIP = 'Enter a valid url without the "http://" prefix.';
    Blockly.DOMOTICZ_LOGIC_WRITETOLOG_TOOLTIP = 'Use this block with a text, uservariable or device block to write a text or value into the Domoticz log.';
    Blockly.DOMOTICZ_LOGIC_CAMERA_TOOLTIP = 'Use this block in a "Do" statement to send a Camera snapshot after a specified number of seconds.\n E.g. if you want to send a snapshot after 10 seconds.';
    Blockly.DOMOTICZ_LOGIC_SETPOINT_TOOLTIP = 'Use this block in a "Do" statement to set a Setpoint value.';
    Blockly.DOMOTICZVARIABLES_ZWAVEALARMS_TOOLTIP = 'This is a ZWave Alarm Type device. Use it in an IF statement to check its value,\ni.e. "if Burglar Alarm (0x07) value == 1". Cannot be used in Set blocks.';
    Blockly.DOMOTICZCONTROLS_MSG_IF = 'If';
    Blockly.DOMOTICZCONTROLS_MSG_ELSEIF = 'Else if';
    Blockly.DOMOTICZCONTROLS_MSG_DO = 'Do';
    Blockly.DOMOTICZCONTROLS_MSG_TEMP = 'temp.';
    Blockly.DOMOTICZCONTROLS_MSG_HUM = 'hum.';
    Blockly.DOMOTICZCONTROLS_MSG_DEWPOINT = 'dew.';
    Blockly.DOMOTICZCONTROLS_MSG_BARO = 'baro.';
    Blockly.DOMOTICZCONTROLS_MSG_UTILITY = 'actual.';
    Blockly.DOMOTICZCONTROLS_MSG_TEXT = 'text.';
    Blockly.DOMOTICZCONTROLS_MSG_SET = 'Set ';
    Blockly.DOMOTICZCONTROLS_MSG_SEND = 'Send ';
    Blockly.DOMOTICZCONTROLS_MSG_TO = 'To ';
    Blockly.DOMOTICZCONTROLS_MSG_FOR = 'For ';
    Blockly.DOMOTICZCONTROLS_MSG_AFTER = 'After ';
    Blockly.DOMOTICZCONTROLS_MSG_SECONDS = 'seconds';
    Blockly.DOMOTICZCONTROLS_MSG_MINUTES = 'minutes';
    Blockly.DOMOTICZCONTROLS_MSG_RANDOM = 'Random within';
    Blockly.DOMOTICZCONTROLS_MSG_SETLEVEL = 'Level (%)';
    Blockly.DOMOTICZCONTROLS_MSG_TIME = 'Time';
    Blockly.DOMOTICZCONTROLS_MSG_DAY = 'Day ';
    Blockly.DOMOTICZCONTROLS_MSG_SECURITY = 'Security status';
    Blockly.DOMOTICZCONTROLS_MSG_GROUP = 'group';
    Blockly.DOMOTICZCONTROLS_MSG_SCENE = 'scene';
    Blockly.DOMOTICZCONTROLS_MSG_SENDCAMERA = 'Send camera snapshot';
    Blockly.DOMOTICZCONTROLS_MSG_SETSETPOINT = 'Set SetPoint';
    Blockly.DOMOTICZCONTROLS_MSG_WITHSUBJECT = 'with subject';
    Blockly.DOMOTICZCONTROLS_MSG_DEGREES = 'Degrees';
    Blockly.DOMOTICZCONTROLS_MSG_ZWAVEALARMS = 'Value';
});

