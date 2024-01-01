#pragma once

#include <string>

enum _eSwitchType
{
	STYPE_OnOff = 0,
	STYPE_Doorbell = 1,
	STYPE_Contact = 2,
	STYPE_Blinds = 3,
	STYPE_X10Siren = 4,
	STYPE_SMOKEDETECTOR = 5,
	STYPE_Dimmer = 7,
	STYPE_Motion = 8,
	STYPE_PushOn = 9,
	STYPE_PushOff = 10,
	STYPE_DoorContact = 11,
	STYPE_Dusk = 12,
	STYPE_BlindsPercentage = 13,
	STYPE_VenetianBlindsUS = 14,
	STYPE_VenetianBlindsEU = 15,
	STYPE_Media = 17,
	STYPE_Selector = 18,
	STYPE_DoorLock = 19,
	STYPE_DoorLockInverted = 20,
	STYPE_BlindsPercentageWithStop = 21,

	STYPE_END = 23 //always set this to highest numer + 1
};

enum _eMeterType
{
	MTYPE_ENERGY = 0,		//0
	MTYPE_GAS,				//1
	MTYPE_WATER,			//2
	MTYPE_COUNTER,			//3
	MTYPE_ENERGY_GENERATED,	//4
	MTYPE_TIME,				//5
	MTYPE_END
};

enum _eTimerType
{
	TTYPE_BEFORESUNRISE = 0,
	TTYPE_AFTERSUNRISE,
	TTYPE_ONTIME,
	TTYPE_BEFORESUNSET,
	TTYPE_AFTERSUNSET,
	TTYPE_FIXEDDATETIME,
	TTYPE_DAYSODD,
	TTYPE_DAYSEVEN,
	TTYPE_WEEKSODD,
	TTYPE_WEEKSEVEN,
	TTYPE_MONTHLY,
	TTYPE_MONTHLY_WD,
	TTYPE_YEARLY,
	TTYPE_YEARLY_WD,
	TTYPE_BEFORESUNATSOUTH,
	TTYPE_AFTERSUNATSOUTH,
	TTYPE_BEFORECIVTWSTART,
	TTYPE_AFTERCIVTWSTART,
	TTYPE_BEFORECIVTWEND,
	TTYPE_AFTERCIVTWEND,
	TTYPE_BEFORENAUTTWSTART,
	TTYPE_AFTERNAUTTWSTART,
	TTYPE_BEFORENAUTTWEND,
	TTYPE_AFTERNAUTTWEND,
	TTYPE_BEFOREASTTWSTART,
	TTYPE_AFTERASTTWSTART,
	TTYPE_BEFOREASTTWEND,
	TTYPE_AFTERASTTWEND,
	TTYPE_END
};

enum _eTimerCommand
{
	TCMD_ON = 0,
	TCMD_OFF
};

enum _eMediaStatus
{
	MSTAT_OFF = 0,
	MSTAT_ON,
	MSTAT_PAUSED,
	MSTAT_STOPPED,
	MSTAT_VIDEO,
	MSTAT_AUDIO,
	MSTAT_PHOTO,
	MSTAT_PLAYING,
	MSTAT_DISCONNECTED,
	MSTAT_SLEEPING,
	MSTAT_UNKNOWN
};

enum _eHardwareTypes {
	HTYPE_RFXtrx315 = 0,		//0
	HTYPE_RFXtrx433,			//1
	HTYPE_RFXLAN,				//2
	HTYPE_Domoticz,				//3
	HTYPE_P1SmartMeter,			//4
	HTYPE_P1SmartMeterLAN,		//5
	HTYPE_YouLess,				//6
	HTYPE_TE923,				//7
	HTYPE_Rego6XX,				//8
	HTYPE_TTN_MQTT,				//9
	HTYPE_DavisVantage,			//10
	HTYPE_VOLCRAFTCO20,			//11
	HTYPE_1WIRE,				//12
	HTYPE_RaspberryBMP085,		//13
	HTYPE_Wunderground,			//14
	HTYPE_Dummy,				//15
	HTYPE_PiFace,				//16
	HTYPE_S0SmartMeterUSB,		//17
	HTYPE_OpenThermGateway,		//18
	HTYPE_TeleinfoMeter,		//19
	HTYPE_OpenThermGatewayTCP,	//20
	HTYPE_FREETOUSE,			//21
	HTYPE_LimitlessLights,		//22
	HTYPE_System,				//23
	HTYPE_EnOceanESP2,			//24
	HTYPE_DarkSky,				//25
	HTYPE_SolarEdgeTCP,			//26
	HTYPE_SBFSpot,				//27
	HTYPE_ICYTHERMOSTAT,		//28
	HTYPE_WOL,					//29
	HTYPE_PVOUTPUT_INPUT,		//30
	HTYPE_EnOceanESP3,			//31
	HTYPE_RaspberryGPIO,		//32
	HTYPE_Meteostick,			//33
	HTYPE_TOONTHERMOSTAT,		//34
	HTYPE_ECODEVICES,			//35
	HTYPE_HARMONY_HUB,			//36
	HTYPE_Mochad,				//37
	HTYPE_Philips_Hue,			//38
	HTYPE_EVOHOME_SERIAL,		//39
	HTYPE_EVOHOME_SCRIPT,		//40
	HTYPE_MySensorsUSB,			//41
	HTYPE_MySensorsTCP,			//42
	HTYPE_MQTT,					//43
	HTYPE_FRITZBOX,				//44
	HTYPE_ETH8020,				//45
	HTYPE_RFLINKUSB,			//46
	HTYPE_KMTronicUSB,			//47
	HTYPE_KMTronicTCP,			//48
	HTYPE_SOLARMAXTCP,			//49
	HTYPE_KMTronic433,			//50
	HTYPE_Pinger,				//51
	HTYPE_NEST,					//52
	HTYPE_THERMOSMART,			//53
	HTYPE_Netatmo,				//54
	HTYPE_Kodi,					//55
	HTYPE_ANNATHERMOSTAT,		//56
	HTYPE_WINDDELEN,			//57
	HTYPE_SatelIntegra,			//58
	HTYPE_Tellstick,			//59
	HTYPE_LogitechMediaServer,	//60
	HTYPE_RFXtrx868,			//61
	HTYPE_RFLINKTCP,			//62
	HTYPE_Comm5TCP,				//63
	HTYPE_SolarEdgeAPI,			//64
	HTYPE_CurrentCostMeter,		//65
	HTYPE_CurrentCostMeterLAN,	//66
	HTYPE_DomoticzInternal,		//67
	HTYPE_NefitEastLAN,			//68
	HTYPE_PanasonicTV,			//69
	HTYPE_OpenWebNetTCP,		//70
	HTYPE_RaspberryHTU21D,		//71
	HTYPE_AtagOne,				//72
	HTYPE_Sterbox,				//73
	HTYPE_HTTPPOLLER,			//74
	HTYPE_EVOHOME_WEB,			//75
	HTYPE_RAVEn,	    		//76
	HTYPE_S0SmartMeterTCP,		//77
	HTYPE_BuienRadar,			//78
	HTYPE_AccuWeather,			//79
	HTYPE_Comm5Serial,          //80
	HTYPE_Ec3kMeterTCP,			//81
	HTYPE_BleBox,          		//82
	HTYPE_OpenWeatherMap,  		//83
	HTYPE_AlfenEveCharger,		//84
	HTYPE_RaspberryTSL2561,		//85
	HTYPE_Daikin,				//86
	HTYPE_HEOS,					//87
	HTYPE_MultiFun,				//88
	HTYPE_ZIBLUEUSB,			//89
	HTYPE_ZIBLUETCP,			//90
	HTYPE_Yeelight,				//91
	HTYPE_MySensorsMQTT,		//92
	HTYPE_RaspberryPCF8574,		//93
	HTYPE_PythonPlugin,			//94
	HTYPE_XiaomiGateway,		//95
	HTYPE_RaspberryBME280,		//96
	HTYPE_Arilux,				//97
	HTYPE_OpenWebNetUSB,		//98
	HTYPE_IntergasInComfortLAN2RF,	//99
	HTYPE_RelayNet,				//100
	HTYPE_KMTronicUDP,			//101
	HTYPE_SysfsGpio,			//102
	HTYPE_Rtl433,				//103
	HTYPE_OnkyoAVTCP,			//104
	HTYPE_EneverPriceFeeds,		//105
	HTYPE_EVOHOME_TCP,			//106
	HTYPE_USBtinGateway,		//107
	HTYPE_EnphaseAPI,			//108
	HTYPE_RaspberryMCP23017,	//109
	HTYPE_eHouseTCP,			//110
	HTYPE_Comm5SMTCP,			//111
	HTYPE_Nest_OAuthAPI,		//112
	HTYPE_EcoCompteur,			//113
	HTYPE_Honeywell,			//114
	HTYPE_Tado,					//115
	HTYPE_DenkoviHTTPDevices,	//116
	HTYPE_DenkoviUSBDevices,	//117
	HTYPE_DenkoviTCPDevices,	//118
	HTYPE_OctoPrint,			//119
	HTYPE_Tesla,                //120
	HTYPE_Meteorologisk,        //121
	HTYPE_Mercedes,				//122
	HTYPE_AirconWithMe,         //123
	HTYPE_TeleinfoMeterTCP,		//124
	HTYPE_MQTTAutoDiscovery,	//125
	HTYPE_RFLINKMQTT,			//126
	HTYPE_VisualCrossing,		//127
	HTYPE_MitsubishiWF,			//128
	HTYPE_END
};

enum _eNotificationTypes
{
	NTYPE_TEMPERATURE = 0,
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
	NTYPE_SETPOINT,
	NTYPE_VIDEO,
	NTYPE_AUDIO,
	NTYPE_PHOTO,
	NTYPE_PAUSED,
	NTYPE_STOPPED,
	NTYPE_PLAYING,
	NTYPE_VALUE,
	NTYPE_LASTUPDATE,
	NTYPE_SLEEPING
};

const char* RFX_Type_Desc(unsigned char i, unsigned char snum);
const char* RFX_Type_SubType_Desc(unsigned char dType, unsigned char sType);
unsigned char Get_Humidity_Level(unsigned char hlevel);
const char* RFX_Humidity_Status_Desc(unsigned char status);
const char* Switch_Type_Desc(_eSwitchType sType);
const char* Meter_Type_Desc(_eMeterType sType);
const char* RFX_Forecast_Desc(unsigned char Forecast);
const char* RFX_WSForecast_Desc(unsigned char Forecast);
const char* BMP_Forecast_Desc(unsigned char Forecast);
const char* Timer_Type_Desc(int tType);
const char* Timer_Cmd_Desc(int tCmd);
const char* Hardware_Type_Desc(int hType);
const char* Hardware_Short_Desc(int hType);
const char* Security_Status_Desc(unsigned char status);
const char* Notification_Type_Desc(int nType, unsigned char snum);
const char* Notification_Type_Label(int nType);
const char* Get_Moisture_Desc(int moisture);
const char* Get_Alert_Desc(int level);
const char* Media_Player_States(_eMediaStatus Status);

void GetLightStatus(unsigned char dType, unsigned char dSubType, _eSwitchType switchtype, unsigned char nValue, const std::string& sValue, std::string& lstatus, int& llevel, bool& bHaveDimmer,
	int& maxDimLevel, bool& bHaveGroupCmd);

int  GetSelectorSwitchLevel(const std::map<std::string, std::string>& options, const std::string& levelName);
std::string GetSelectorSwitchLevelAction(const std::map<std::string, std::string>& options, int level);
void GetSelectorSwitchStatuses(const std::map<std::string, std::string>& options, std::map<std::string, std::string>& statuses);

bool GetLightCommand(unsigned char dType, unsigned char dSubType, _eSwitchType switchtype, std::string switchcmd, unsigned char& cmd, const std::map<std::string, std::string>& options);

bool IsLightSwitchOn(const std::string& lstatus);

bool IsSerialDevice(_eHardwareTypes htype);
bool IsNetworkDevice(_eHardwareTypes htype);
void ConvertToGeneralSwitchType(std::string& devid, int& dtype, int& subtype);
