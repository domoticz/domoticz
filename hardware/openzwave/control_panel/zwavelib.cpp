//-----------------------------------------------------------------------------
//
//	zwavelib.cpp
//
//	OpenZWave Control Panel support routines
//
//	Copyright (c) 2010 Greg Satz <satz@iranger.com>
//	All rights reserved.
//
// SOFTWARE NOTICE AND LICENSE
// This work (including software, documents, or other related items) is being 
// provided by the copyright holders under the following license. By obtaining,
// using and/or copying this work, you (the licensee) agree that you have read,
// understood, and will comply with the following terms and conditions:
//
// Permission to use, copy, and distribute this software and its documentation,
// without modification, for any purpose and without fee or royalty is hereby 
// granted, provided that you include the full text of this NOTICE on ALL
// copies of the software and documentation or portions thereof.
//
// THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS 
// MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR 
// PURPOSE OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE 
// ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
//
// COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR 
// CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR 
// DOCUMENTATION.
//
// The name and trademarks of copyright holders may NOT be used in advertising 
// or publicity pertaining to the software without specific, written prior 
// permission.  Title to copyright in this software and any associated 
// documentation will at all times remain with copyright holders.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#ifdef WITH_OPENZWAVE
#include <string.h>
#include "ozwcp.h"

const char *valueGenreStr (OpenZWave::ValueID::ValueGenre vg)
{
  switch (vg) {
  case OpenZWave::ValueID::ValueGenre_Basic:
    return "basic";
  case OpenZWave::ValueID::ValueGenre_User:
    return "user";
  case OpenZWave::ValueID::ValueGenre_Config:
    return "config";
  case OpenZWave::ValueID::ValueGenre_System:
    return "system";
  case OpenZWave::ValueID::ValueGenre_Count:
    return "count";
  }
  return "unknown";
}

OpenZWave::ValueID::ValueGenre valueGenreNum (char const *str)
{
  if (strcmp(str, "basic") == 0)
    return OpenZWave::ValueID::ValueGenre_Basic;
  else if (strcmp(str, "user") == 0)
    return OpenZWave::ValueID::ValueGenre_User;
  else if (strcmp(str, "config") == 0)
    return OpenZWave::ValueID::ValueGenre_Config;
  else if (strcmp(str, "system") == 0)
    return OpenZWave::ValueID::ValueGenre_System;
  else if (strcmp(str, "count") == 0)
    return OpenZWave::ValueID::ValueGenre_Count;
  else
    return (OpenZWave::ValueID::ValueGenre)255;
}

const char *valueTypeStr (OpenZWave::ValueID::ValueType vt)
{
  switch (vt) {
  case OpenZWave::ValueID::ValueType_Bool:
    return "bool";
  case OpenZWave::ValueID::ValueType_Byte:
    return "byte";
  case OpenZWave::ValueID::ValueType_Decimal:
    return "decimal";
  case OpenZWave::ValueID::ValueType_Int:
    return "int";
  case OpenZWave::ValueID::ValueType_List:
    return "list";
  case OpenZWave::ValueID::ValueType_Schedule:
    return "schedule";
  case OpenZWave::ValueID::ValueType_String:
    return "string";
  case OpenZWave::ValueID::ValueType_Short:
    return "short";
  case OpenZWave::ValueID::ValueType_Button:
    return "button";
  case OpenZWave::ValueID::ValueType_Raw:
    return "raw";
  }
  return "unknown";
}

OpenZWave::ValueID::ValueType valueTypeNum (char const *str)
{
  if (strcmp(str, "bool") == 0)
    return OpenZWave::ValueID::ValueType_Bool;
  else if (strcmp(str, "byte") == 0)
    return OpenZWave::ValueID::ValueType_Byte;
  else if (strcmp(str, "decimal") == 0)
    return OpenZWave::ValueID::ValueType_Decimal;
  else if (strcmp(str, "int") == 0)
    return OpenZWave::ValueID::ValueType_Int;
  else if (strcmp(str, "list") == 0)
    return OpenZWave::ValueID::ValueType_List;
  else if (strcmp(str, "schedule") == 0)
    return OpenZWave::ValueID::ValueType_Schedule;
  else if (strcmp(str, "short") == 0)
    return OpenZWave::ValueID::ValueType_Short;
  else if (strcmp(str, "string") == 0)
    return OpenZWave::ValueID::ValueType_String;
  else if (strcmp(str, "button") == 0)
    return OpenZWave::ValueID::ValueType_Button;
  else if (strcmp(str, "raw") == 0)
    return OpenZWave::ValueID::ValueType_Raw;
  else
    return (OpenZWave::ValueID::ValueType)255;
}

const char *nodeBasicStr (uint8 basic)
{
  switch (basic) {
  case 1:
    return "Controller";
  case 2:
    return "Static Controller";
  case 3:
    return "Slave";
  case 4:
    return "Routing Slave";
  }
  return "unknown";
}

uint8 cclassNum(char const *str)
{
	if (strcmp(str, "NO OPERATION") == 0)
		return 0x00;
	else if (strcmp(str, "BASIC") == 0)
		return 0x20;
	else if (strcmp(str, "CONTROLLER REPLICATION") == 0)
		return 0x21;
	else if (strcmp(str, "APPLICATION STATUS") == 0)
		return 0x22;
	else if (strcmp(str, "ZIP SERVICES") == 0)
		return 0x23;
	else if (strcmp(str, "ZIP SERVER") == 0)
		return 0x24;
	else if (strcmp(str, "SWITCH BINARY") == 0)
		return 0x25;
	else if (strcmp(str, "SWITCH MULTILEVEL") == 0)
		return 0x26;
	else if (strcmp(str, "SWITCH ALL") == 0)
		return 0x27;
	else if (strcmp(str, "SWITCH TOGGLE BINARY") == 0)
		return 0x28;
	else if (strcmp(str, "SWITCH TOGGLE MULTILEVEL") == 0)
		return 0x29;
	else if (strcmp(str, "CHIMNEY FAN") == 0)
		return 0x2A;
	else if (strcmp(str, "SCENE ACTIVATION") == 0)
		return 0x2B;
	else if (strcmp(str, "SCENE ACTUATOR CONF") == 0)
		return 0x2C;
	else if (strcmp(str, "SCENE CONTROLLER CONF") == 0)
		return 0x2D;
	else if (strcmp(str, "ZIP CLIENT") == 0)
		return 0x2E;
	else if (strcmp(str, "ZIP ADV SERVICES") == 0)
		return 0x2F;
	else if (strcmp(str, "SENSOR BINARY") == 0)
		return 0x30;
	else if (strcmp(str, "SENSOR MULTILEVEL") == 0)
		return 0x31;
	else if (strcmp(str, "METER") == 0)
		return 0x32;
	else if (strcmp(str, "ZIP ADV SERVER") == 0)
		return 0x33;
	else if (strcmp(str, "COLOR") == 0)
		return 0x33;
	else if (strcmp(str, "ZIP ADV CLIENT") == 0)
		return 0x34;
	else if (strcmp(str, "METER PULSE") == 0)
		return 0x35;
	else if (strcmp(str, "THERMOSTAT HEATING") == 0)
		return 0x38;
	else if (strcmp(str, "THERMOSTAT MODE") == 0)
		return 0x40;
	else if (strcmp(str, "THERMOSTAT OPERATING STATE") == 0)
		return 0x42;
	else if (strcmp(str, "THERMOSTAT SETPOINT") == 0)
		return 0x43;
	else if (strcmp(str, "THERMOSTAT FAN MODE") == 0)
		return 0x44;
	else if (strcmp(str, "THERMOSTAT FAN STATE") == 0)
		return 0x45;
	else if (strcmp(str, "CLIMATE CONTROL SCHEDULE") == 0)
		return 0x46;
	else if (strcmp(str, "THERMOSTAT SETBACK") == 0)
		return 0x47;
	else if (strcmp(str, "DOOR LOCK LOGGING") == 0)
		return 0x4C;
	else if (strcmp(str, "SCHEDULE ENTRY LOCK") == 0)
		return 0x4E;
	else if (strcmp(str, "BASIC WINDOW COVERING") == 0)
		return 0x50;
	else if (strcmp(str, "MTP WINDOW COVERING") == 0)
		return 0x51;
	else if (strcmp(str, "MULTI INSTANCE") == 0)
		return 0x60;
	else if (strcmp(str, "DOOR LOCK") == 0)
		return 0x62;
	else if (strcmp(str, "USER CODE") == 0)
		return 0x63;
	else if (strcmp(str, "CONFIGURATION") == 0)
		return 0x70;
	else if (strcmp(str, "ALARM") == 0)
		return 0x71;
	else if (strcmp(str, "MANUFACTURER SPECIFIC") == 0)
		return 0x72;
	else if (strcmp(str, "POWERLEVEL") == 0)
		return 0x73;
	else if (strcmp(str, "PROTECTION") == 0)
		return 0x75;
	else if (strcmp(str, "LOCK") == 0)
		return 0x76;
	else if (strcmp(str, "NODE NAMING") == 0)
		return 0x77;
	else if (strcmp(str, "FIRMWARE UPDATE MD") == 0)
		return 0x7A;
	else if (strcmp(str, "GROUPING NAME") == 0)
		return 0x7B;
	else if (strcmp(str, "REMOTE ASSOCIATION ACTIVATE") == 0)
		return 0x7C;
	else if (strcmp(str, "REMOTE ASSOCIATION") == 0)
		return 0x7D;
	else if (strcmp(str, "BATTERY") == 0)
		return 0x80;
	else if (strcmp(str, "CLOCK") == 0)
		return 0x81;
	else if (strcmp(str, "HAIL") == 0)
		return 0x82;
	else if (strcmp(str, "WAKE UP") == 0)
		return 0x84;
	else if (strcmp(str, "ASSOCIATION") == 0)
		return 0x85;
	else if (strcmp(str, "VERSION") == 0)
		return 0x86;
	else if (strcmp(str, "INDICATOR") == 0)
		return 0x87;
	else if (strcmp(str, "PROPRIETARY") == 0)
		return 0x88;
	else if (strcmp(str, "LANGUAGE") == 0)
		return 0x89;
	else if (strcmp(str, "TIME") == 0)
		return 0x8A;
	else if (strcmp(str, "TIME PARAMETERS") == 0)
		return 0x8B;
	else if (strcmp(str, "GEOGRAPHIC LOCATION") == 0)
		return 0x8C;
	else if (strcmp(str, "COMPOSITE") == 0)
		return 0x8D;
	else if (strcmp(str, "MULTI INSTANCE ASSOCIATION") == 0)
		return 0x8E;
	else if (strcmp(str, "MULTI CMD") == 0)
		return 0x8F;
	else if (strcmp(str, "ENERGY PRODUCTION") == 0)
		return 0x90;
	else if (strcmp(str, "MANUFACTURER PROPRIETARY") == 0)
		return 0x91;
	else if (strcmp(str, "SCREEN MD") == 0)
		return 0x92;
	else if (strcmp(str, "SCREEN ATTRIBUTES") == 0)
		return 0x93;
	else if (strcmp(str, "SIMPLE AV CONTROL") == 0)
		return 0x94;
	else if (strcmp(str, "AV CONTENT DIRECTORY MD") == 0)
		return 0x95;
	else if (strcmp(str, "AV RENDERER STATUS") == 0)
		return 0x96;
	else if (strcmp(str, "AV CONTENT SEARCH MD") == 0)
		return 0x97;
	else if (strcmp(str, "SECURITY") == 0)
		return 0x98;
	else if (strcmp(str, "AV TAGGING MD") == 0)
		return 0x99;
	else if (strcmp(str, "IP CONFIGURATION") == 0)
		return 0x9A;
	else if (strcmp(str, "ASSOCIATION COMMAND CONFIGURATION") == 0)
		return 0x9B;
	else if (strcmp(str, "SENSOR ALARM") == 0)
		return 0x9C;
	else if (strcmp(str, "SILENCE ALARM") == 0)
		return 0x9D;
	else if (strcmp(str, "SENSOR CONFIGURATION") == 0)
		return 0x9E;
	else if (strcmp(str, "MARK") == 0)
		return 0xEF;
	else if (strcmp(str, "NON INTEROPERABLE") == 0)
		return 0xF0;
	else
		return 0xFF;
}
const char *controllerErrorStr (OpenZWave::Driver::ControllerError err)
{
  switch (err) {
  case OpenZWave::Driver::ControllerError_None:
    return "None";
  case OpenZWave::Driver::ControllerError_ButtonNotFound:
    return "Button Not Found";
  case OpenZWave::Driver::ControllerError_NodeNotFound:
    return "Node Not Found";
  case OpenZWave::Driver::ControllerError_NotBridge:
    return "Not a Bridge";
  case OpenZWave::Driver::ControllerError_NotPrimary:
    return "Not Primary Controller";
  case OpenZWave::Driver::ControllerError_IsPrimary:
    return "Is Primary Controller";
  case OpenZWave::Driver::ControllerError_NotSUC:
    return "Not Static Update Controller";
  case OpenZWave::Driver::ControllerError_NotSecondary:
    return "Not Secondary Controller";
  case OpenZWave::Driver::ControllerError_NotFound:
    return "Not Found";
  case OpenZWave::Driver::ControllerError_Busy:
    return "Controller Busy";
  case OpenZWave::Driver::ControllerError_Failed:
    return "Failed";
  case OpenZWave::Driver::ControllerError_Disabled:
    return "Disabled";
  case OpenZWave::Driver::ControllerError_Overflow:
    return "Overflow";
  default:
    return "Unknown error";
  }
}
#endif
