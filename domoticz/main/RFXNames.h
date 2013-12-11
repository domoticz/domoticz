#pragma once

#include <string>

#define sTypeTH_LC_TC 0xA0   //La Cross Temp_Hum combined
#define sTypeTEMP_SYSTEM 0xA0  //Internal sensor

enum _eSwitchType
{
	STYPE_OnOff=0,			//0
	STYPE_Doorbell,			//1
	STYPE_Contact,			//2
	STYPE_Blinds,			//3
	STYPE_X10Siren,			//4
	STYPE_SMOKEDETECTOR,	//5
    STYPE_BlindsInverted,	//6
	STYPE_Dimmer,			//7
	STYPE_Motion,			//8
	STYPE_PushOn,			//9
	STYPE_PushOff,			//10
	STYPE_DoorLock,			//11
    STYPE_Dusk,             //12
	STYPE_END
};

enum _eMeterType
{
	MTYPE_ENERGY=0,
	MTYPE_GAS,
	MTYPE_WATER,
    MTYPE_COUNTER,
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
	HTYPE_P1SmartMeter,
	HTYPE_P1SmartMeterLAN,
	HTYPE_YouLess,
	HTYPE_TE923,
	HTYPE_Rego6XX,
	HTYPE_RazberryZWave,
	HTYPE_DavisVantage,
	HTYPE_VOLCRAFTCO20,
	HTYPE_1WIRE,
	HTYPE_RaspberryBMP085,
	HTYPE_Wunderground,
	HTYPE_Dummy,
	HTYPE_PiFace,
	HTYPE_S0SmartMeter,
	HTYPE_OpenThermGateway,
	HTYPE_TeleinfoMeter,
	HTYPE_OpenThermGatewayTCP,
	HTYPE_OpenZWave,
	HTYPE_LimitlessLights,
	HTYPE_System,
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
	NTYPE_SWITCH_ON,//7
	NTYPE_AMPERE1,
	NTYPE_AMPERE2,
	NTYPE_AMPERE3,
	NTYPE_ENERGYINSTANT,
	NTYPE_ENERGYTOTAL,
	NTYPE_TODAYENERGY,
	NTYPE_TODAYGAS,
	NTYPE_TODAYCOUNTER,
	NTYPE_SWITCH_OFF,//16
	NTYPE_PERCENTAGE,
	NTYPE_DEWPOINT,
	NTYPE_RPM,
};

enum _eShareRights
{
	SHARE_SENSORS=0,
	SHARE_ALL
};

enum _eEventActions
{
	EACTION_SWITCHLIGHT=0,
	EACTION_SENDNOTIFICATION,
	EACTION_SENDEMAIL,
	EACTION_EXECUTESCRIPT,
};

const char *RFX_Type_Desc(const unsigned char i, const unsigned char snum);
const char *RFX_Type_SubType_Desc(const unsigned char dType, const unsigned char sType);
unsigned char Get_Humidity_Level(const unsigned char hlevel);
const char *RFX_Humidity_Status_Desc(const unsigned char status);
const char *Switch_Type_Desc(const _eSwitchType sType);
const char *Meter_Type_Desc(const _eMeterType sType);
const char *RFX_Forecast_Desc(const unsigned char Forecast);
const char *RFX_WSForecast_Desc(const unsigned char Forecast);
const char *Timer_Type_Desc(int tType);
const char *Timer_Cmd_Desc(int tCmd);
const char *Hardware_Type_Desc(int hType);
const char *Security_Status_Desc(const unsigned char status);
const char *Notification_Type_Desc(const int nType, const unsigned char snum);
const char *Notification_Type_Label(const int nType);
const char *Get_Moisture_Desc(const int moisture);

void GetLightStatus(
	const unsigned char dType, 
	const unsigned char dSubType, 
	const unsigned char nValue, 
	const std::string &sValue, 
	std::string &lstatus, 
	int &llevel, 
	bool &bHaveDimmer,
	int &maxDimLevel,
	bool &bHaveGroupCmd);

bool GetLightCommand(
	const unsigned char dType, 
	const unsigned char dSubType, 
	const _eSwitchType switchtype,
	std::string switchcmd,
	unsigned char &cmd
	);

bool IsLightSwitchOn(const std::string &lstatus);
