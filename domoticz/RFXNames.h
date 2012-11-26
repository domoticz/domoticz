#pragma once

#include <string>

#define switchTypeOnOff 0
#define switchTypeDoorbell 1
#define switchTypeContact 2
#define switchTypeMax 2

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

const char *RFX_Type_Desc(const unsigned char i, const unsigned char snum);
const char *RFX_Type_SubType_Desc(const unsigned char dType, const unsigned char sType);
const char *RFX_Humidity_Status_Desc(const unsigned char status);
const char *Switch_Type_Desc(const unsigned char sType);
const char *RFX_Forecast_Desc(const unsigned char Forecast);
const char *Timer_Type_Desc(int tType);
const char *Timer_Cmd_Desc(int tCmd);
const char *Hardware_Type_Desc(int hType);
const char *Security_Status_Desc(const unsigned char status);

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
	const unsigned char switchtype,
	std::string switchcmd,
	unsigned char &cmd
	);
