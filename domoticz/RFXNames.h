#pragma once

#include <string>

enum _eSwitchType
{
	STYPE_OnOff=0,
	STYPE_Doorbell,
	STYPE_Contact,
	STYPE_Blinds,
	STYPE_X10Siren,
	STYPE_SMOKEDETECTOR,
	STYPE_END
};

enum _eMeterType
{
	MTYPE_ENERGY=0,
	MTYPE_GAS,
	MTYPE_WATER,
	MTYPE_END
};

enum _eTimerType
{
	TTYPE_BEFORESUNRISE=0,
	TTYPE_AFTERSUNRISE,
	TTYPE_ONTIME,
	TTYPE_BEFORESUNSET,
	TTYPE_AFTERSUNSET,
	TTYPE_END
};

enum _eTimerCommand
{
	TCMD_ON=0,
	TCMD_OFF
};

enum _eHardwareTypes {
	HTYPE_RFXtrx315=0,
	HTYPE_RFXtrx433,
	HTYPE_RFXLAN,
	HTYPE_Domoticz,
	HTYPE_END
};

enum _eNotificationTypes
{
	NTYPE_TEMPERATURE=0,
	NTYPE_HUMIDITY,
	NTYPE_RAIN,
	NTYPE_UV,
	NTYPE_WIND,
	NTYPE_USAGE,
	NTYPE_BARO,
	NTYPE_SWITCH,
	NTYPE_AMPERE1,
	NTYPE_AMPERE2,
	NTYPE_AMPERE3,
	NTYPE_ENERGYINSTANT,
	NTYPE_ENERGYTOTAL,
};

enum _eShareRights
{
	SHARE_SENSORS=0,
	SHARE_ALL
};

const char *RFX_Type_Desc(const unsigned char i, const unsigned char snum);
const char *RFX_Type_SubType_Desc(const unsigned char dType, const unsigned char sType);
const char *RFX_Humidity_Status_Desc(const unsigned char status);
const char *Switch_Type_Desc(const _eSwitchType sType);
const char *Meter_Type_Desc(const _eMeterType sType);
const char *RFX_Forecast_Desc(const unsigned char Forecast);
const char *Timer_Type_Desc(int tType);
const char *Timer_Cmd_Desc(int tCmd);
const char *Hardware_Type_Desc(int hType);
const char *Security_Status_Desc(const unsigned char status);
const char *Notification_Type_Desc(const int nType, const unsigned char snum);
const char *Notification_Type_Label(const int nType);

void GetLightStatus(
	const unsigned char dType, 
	const unsigned char dSubType, 
	const unsigned char nValue, 
	const std::string sValue, 
	std::string &lstatus, 
	int &llevel, 
	bool &bHaveDimmer,
	bool &bHaveGroupCmd);

bool GetLightCommand(
	const unsigned char dType, 
	const unsigned char dSubType, 
	const _eSwitchType switchtype,
	std::string switchcmd,
	unsigned char &cmd
	);
