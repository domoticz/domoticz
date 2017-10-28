#include "stdafx.h"
#include "RFXNames.h"
#include "RFXtrx.h"
#include "../hardware/hardwaretypes.h"
#include "../hardware/EvohomeBase.h"
#include "Helper.h"
#include <boost/algorithm/string.hpp>
#include "Logger.h"

typedef struct _STR_TABLE_SINGLE {
	unsigned long    id;
	const char   *str1;
	const char   *str2;
} STR_TABLE_SINGLE;

typedef struct _STR_TABLE_ID1_ID2 {
	unsigned long    id1;
	unsigned long    id2;
	const char   *str1;
} STR_TABLE_ID1_ID2;

const char *findTableIDSingle1 (const STR_TABLE_SINGLE *t, const unsigned long id)
{
	while (t->str1) {
		if (t->id == id)
			return t->str1;
		t++;
	}
	return "Unknown";
}

const char *findTableIDSingle2 (const STR_TABLE_SINGLE *t, const unsigned long id)
{
	while (t->str2) {
		if (t->id == id)
			return t->str2;
		t++;
	}
	return "Unknown";
}

const char *findTableID1ID2 (const _STR_TABLE_ID1_ID2 *t, const unsigned long id1, const unsigned long id2)
{
	while (t->str1) {
		if ( (t->id1 == id1) && (t->id2 == id2) )
			return t->str1;
		t++;
	}
	return "Unknown";
}

const char *RFX_Humidity_Status_Desc(const unsigned char status)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ humstat_normal, "Normal" },
		{ humstat_comfort, "Comfortable" },
		{ humstat_dry, "Dry" },
		{ humstat_wet, "Wet" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, status);
}

unsigned char Get_Humidity_Level(const unsigned char hlevel)
{
	if (hlevel<25)
		return humstat_dry;
	if (hlevel>60)
		return humstat_wet;
	if ((hlevel>=25)&&(hlevel<=60))
		return humstat_comfort;
	return humstat_normal;
}

const char *Security_Status_Desc(const unsigned char status)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ sStatusNormal, "Normal" },
		{ sStatusNormalDelayed, "Normal Delayed" },
		{ sStatusAlarm, "Alarm" },
		{ sStatusAlarmDelayed, "Alarm Delayed" },
		{ sStatusMotion, "Motion" },
		{ sStatusNoMotion, "No Motion" },
		{ sStatusPanic, "Panic" },
		{ sStatusPanicOff, "Panic End" },
		{ sStatusArmAway, "Arm Away" },
		{ sStatusArmAwayDelayed, "Arm Away Delayed" },
		{ sStatusArmHome, "Arm Home" },
		{ sStatusArmHomeDelayed, "Arm Home Delayed" },
		{ sStatusDisarm, "Disarm" },
		{ sStatusLightOff, "Light Off" },
		{ sStatusLightOn, "Light On" },
		{ sStatusLight2Off, "Light 2 Off" },
		{ sStatusLight2On, "Light 2 On" },
		{ sStatusDark, "Dark detected" },
		{ sStatusLight, "Light Detected" },
		{ sStatusBatLow, "Battery low MS10 or XX18 sensor" },
		{ sStatusPairKD101, "Pair KD101" },
		{ sStatusNormalTamper, "Normal + Tamper" },
		{ sStatusNormalDelayedTamper, "Normal Delayed + Tamper" },
		{ sStatusAlarmTamper, "Alarm + Tamper" },
		{ sStatusAlarmDelayedTamper, "Alarm Delayed + Tamper" },
		{ sStatusMotionTamper, "Motion + Tamper" },
		{ sStatusNoMotionTamper, "No Motion + Tamper" },
		{ 0, NULL }
	};
	return findTableIDSingle1 (Table, status);
}

const char *Timer_Type_Desc(const int tType)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ TTYPE_BEFORESUNRISE, "Before Sunrise" },
		{ TTYPE_AFTERSUNRISE, "After Sunrise" },
		{ TTYPE_ONTIME, "On Time" },
		{ TTYPE_BEFORESUNSET, "Before Sunset" },
		{ TTYPE_AFTERSUNSET, "After Sunset" },
		{ TTYPE_FIXEDDATETIME, "Fixed Date/Time" },
		{ TTYPE_DAYSODD, "Odd Day Numbers" },
		{ TTYPE_DAYSEVEN, "Even Day Numbers" },
		{ TTYPE_WEEKSODD, "Odd Week Numbers"},
		{ TTYPE_WEEKSEVEN, "Even Week Numbers" },
		{ TTYPE_MONTHLY, "Monthly" },
		{ TTYPE_MONTHLY_WD, "Monthly (Weekday)" },
		{ TTYPE_YEARLY, "Yearly" },
		{ TTYPE_YEARLY_WD, "Yearly (Weekday)" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, tType);
}

const char *Timer_Cmd_Desc(const int tType)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ TCMD_ON, "On" },
		{ TCMD_OFF, "Off" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, tType);
}

const char *Hardware_Type_Desc(int hType)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ HTYPE_RFXtrx315, "RFXCOM - RFXtrx315 USB 315MHz Transceiver" },
		{ HTYPE_RFXtrx433, "RFXCOM - RFXtrx433 USB 433.92MHz Transceiver" },
		{ HTYPE_RFXLAN, "RFXCOM - RFXtrx shared over LAN interface" },
		{ HTYPE_Domoticz, "Domoticz - Remote Server" },
		{ HTYPE_P1SmartMeter, "P1 Smart Meter USB" },
		{ HTYPE_P1SmartMeterLAN, "P1 Smart Meter with LAN interface" },
		{ HTYPE_YouLess, "YouLess Meter with LAN interface" },
		{ HTYPE_WINDDELEN, "Winddelen" },
		{ HTYPE_TE923, "TE923 USB Compatible Weather Station" },
		{ HTYPE_Rego6XX, "Rego 6XX USB/serial interface" },
		{ HTYPE_RazberryZWave, "Razberry Z-Wave via LAN interface (Deprecated)" },
		{ HTYPE_DavisVantage, "Davis Vantage Weather Station USB" },
		{ HTYPE_VOLCRAFTCO20, "Volcraft CO-20 USB air quality sensor" },
		{ HTYPE_1WIRE, "1-Wire (System)" },
		{ HTYPE_RaspberryBMP085, "I2C sensor BMP085/180 Temp+Baro" },
		{ HTYPE_Wunderground, "Weather Underground" },
		{ HTYPE_DarkSky, "DarkSky (Weather Lookup)" },
		{ HTYPE_Dummy, "Dummy (Does nothing, use for virtual switches only)" },
		{ HTYPE_Tellstick, "Tellstick" },
		{ HTYPE_PiFace, "PiFace - Raspberry Pi IO expansion board" },
		{ HTYPE_S0SmartMeterUSB, "S0 Meter USB" },
		{ HTYPE_OpenThermGateway, "OpenTherm Gateway USB" },
		{ HTYPE_TeleinfoMeter, "Teleinfo EDF" },
		{ HTYPE_OpenThermGatewayTCP, "OpenTherm Gateway with LAN interface" },
		{ HTYPE_OpenZWave, "OpenZWave USB" },
		{ HTYPE_LimitlessLights, "Limitless/AppLamp/Mi Light with LAN/WiFi interface" },
		{ HTYPE_System, "Motherboard sensors" },
		{ HTYPE_EnOceanESP2, "EnOcean USB (ESP2)" },
		{ HTYPE_SolarEdgeTCP, "SolarEdge via LAN interface" },
		{ HTYPE_SBFSpot, "SBFSpot (SMA)" },
		{ HTYPE_ICYTHERMOSTAT, "ICY Thermostat" },
		{ HTYPE_WOL, "Wake-on-LAN" },
		{ HTYPE_PVOUTPUT_INPUT, "PVOutput (Input)" },
		{ HTYPE_EnOceanESP3, "EnOcean USB (ESP3)" },
		{ HTYPE_RaspberryGPIO, "Raspberry's GPIO port" },
		{ HTYPE_Meteostick, "Meteostick USB" },
		{ HTYPE_TOONTHERMOSTAT, "Toon Thermostat" },
		{ HTYPE_ECODEVICES, "Eco Devices" },
		{ HTYPE_HARMONY_HUB, "Logitech Harmony Hub" },
		{ HTYPE_Mochad, "Mochad CM15Pro/CM19A bridge with LAN interface" },
		{ HTYPE_Philips_Hue, "Philips Hue Bridge" },
		{ HTYPE_EVOHOME_SERIAL, "Evohome USB (for HGI/S80)" },
		{ HTYPE_EVOHOME_SCRIPT, "Evohome via script" },
		{ HTYPE_EVOHOME_WEB, "Evohome via Web API" },
		{ HTYPE_EVOHOME_TCP, "Evohome via LAN (remote HGI/S80)" },
		{ HTYPE_MySensorsUSB, "MySensors Gateway USB" },
		{ HTYPE_MySensorsTCP, "MySensors Gateway with LAN interface" },
		{ HTYPE_MySensorsMQTT, "MySensors Gateway with MQTT interface" },
		{ HTYPE_MQTT, "MQTT Client Gateway with LAN interface" },
		{ HTYPE_FRITZBOX, "Fritzbox Callmonitor via LAN interface" },
		{ HTYPE_ETH8020, "ETH8020 Relay board with LAN interface" },
		{ HTYPE_RFLINKUSB, "RFLink Gateway USB" },
		{ HTYPE_KMTronicUSB, "KMTronic Gateway USB" },
		{ HTYPE_KMTronicTCP, "KMTronic Gateway with LAN interface" },
		{ HTYPE_SOLARMAXTCP, "SolarMax via LAN interface" },
		{ HTYPE_KMTronic433, "KMTronic 433MHz Gateway USB" },
		{ HTYPE_Pinger, "System Alive Checker (Ping)" },
		{ HTYPE_NEST, "Nest Thermostat/Protect" },
		{ HTYPE_THERMOSMART, "Thermosmart Thermostat" },
		{ HTYPE_Netatmo, "Netatmo" },
		{ HTYPE_Kodi, "Kodi Media Server" },
		{ HTYPE_PanasonicTV, "PanasonicTV" },
		{ HTYPE_ANNATHERMOSTAT, "Plugwise Anna Thermostat via LAN interface" },
		{ HTYPE_SatelIntegra, "Satel Integra via LAN interface" },
		{ HTYPE_LogitechMediaServer, "Logitech Media Server" },
		{ HTYPE_RFXtrx868, "RFXCOM - RFXtrx868 USB 868MHz Transceiver" },
		{ HTYPE_RFLINKTCP, "RFLink Gateway with LAN interface" },
		{ HTYPE_Comm5TCP, "Comm5 MA-5XXX with LAN interface" },
		{ HTYPE_Comm5Serial, "Comm5 MA-4XXX/MI-XXXX Serial/USB interface" },
		{ HTYPE_SolarEdgeAPI , "SolarEdge via Web API" },
		{ HTYPE_CurrentCostMeter, "CurrentCost Meter USB" },
		{ HTYPE_CurrentCostMeterLAN, "CurrentCost Meter with LAN interface" },
		{ HTYPE_DomoticzInternal, "Domoticz Internal interface" },
		{ HTYPE_NefitEastLAN, "Nefit Easy HTTP server over LAN interface" },
		{ HTYPE_OpenWebNetTCP, "MyHome OpenWebNet with LAN interface" },
		{ HTYPE_RaspberryHTU21D, "I2C sensor HTU21D(F)/SI702x Humidity+Temp" },
		{ HTYPE_AtagOne, "Atag One Thermostat" },
		{ HTYPE_Sterbox, "Sterbox v2-3 PLC with LAN interface" },
		{ HTYPE_HTTPPOLLER, "HTTP/HTTPS poller" },
		{ HTYPE_RAVEn, "Rainforest RAVEn USB" },
		{ HTYPE_S0SmartMeterTCP, "S0 Meter with LAN interface" },
		{ HTYPE_DenkoviSmartdenLan, "Denkovi Smartden with LAN interface" },
		{ HTYPE_AccuWeather, "AccuWeather (Weather Lookup)" },
		{ HTYPE_BleBox, "BleBox devices" },
		{ HTYPE_Ec3kMeterTCP, "Energy Count 3000/ NETBSEM4/ La Crosse RT-10 LAN" },
		{ HTYPE_OpenWeatherMap, "Open Weather Map" },
		{ HTYPE_GoodweAPI, "Goodwe solar inverter via Web API" },
		{ HTYPE_RaspberryTSL2561, "I2C sensor TSL2561 Illuminance" },
		{ HTYPE_Daikin, "Daikin Airconditioning with LAN (HTTP) interface" },
		{ HTYPE_HEOS, "HEOS by DENON" },
		{ HTYPE_MultiFun, "MultiFun LAN" },
		{ HTYPE_ZIBLUEUSB, "ZiBlue RFPlayer USB" },
		{ HTYPE_ZIBLUETCP, "ZiBlue RFPlayer with LAN interface" },
		{ HTYPE_Yeelight, "YeeLight LED" },
		{ HTYPE_RaspberryPCF8574, "I2C sensor PIO 8bit expander PCF8574 or PCF8574A" },
		{ HTYPE_PythonPlugin, "Python Plugin System" },
		{ HTYPE_XiaomiGateway, "Xiaomi Gateway" },
		{ HTYPE_RaspberryBME280, "I2C sensor BME280 Temp+Hum+Baro" },
		{ HTYPE_Arilux, "Arilux AL-LC0x" },
		{ HTYPE_OpenWebNetUSB, "MyHome OpenWebNet USB" },
		{ HTYPE_IntergasInComfortLAN2RF, "Intergas InComfort LAN2RF Gateway" },
		{ HTYPE_RelayNet, "Relay-Net 8 channel LAN Relay and binary Input module" },
		{ HTYPE_KMTronicUDP, "KMTronic Gateway with LAN/UDP interface" },
		{ HTYPE_SysfsGpio, "Generic sysfs GPIO" },
		{ HTYPE_Rtl433, "Rtl433 RTL-SDR receiver" },
		{ HTYPE_OnkyoAVTCP, "Onkyo AV Receiver (LAN)" },
		{ HTYPE_DenkoviSmartdenIPInOut, "Denkovi Smartden IP In with LAN interface" },
		{ 0, NULL, NULL }
	};
	return findTableIDSingle1 (Table, hType);
}

const char *Switch_Type_Desc(const _eSwitchType sType)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ STYPE_OnOff, "On/Off" },
		{ STYPE_Doorbell, "Doorbell" },
		{ STYPE_Contact, "Contact" },
		{ STYPE_Blinds, "Blinds" },
		{ STYPE_X10Siren, "X10 Siren" },
		{ STYPE_SMOKEDETECTOR, "Smoke Detector" },
		{ STYPE_BlindsInverted, "Blinds Inverted" },
		{ STYPE_Dimmer, "Dimmer" },
		{ STYPE_Motion, "Motion Sensor" },
		{ STYPE_PushOn, "Push On Button" },
		{ STYPE_PushOff, "Push Off Button" },
		{ STYPE_DoorContact, "Door Contact" },
		{ STYPE_Dusk, "Dusk Sensor" },
		{ STYPE_BlindsPercentage, "Blinds Percentage" },
		{ STYPE_VenetianBlindsUS, "Venetian Blinds US" },
		{ STYPE_VenetianBlindsEU, "Venetian Blinds EU" },
		{ STYPE_BlindsPercentageInverted, "Blinds Percentage Inverted" },
		{ STYPE_Media, "Media Player" },
		{ STYPE_Selector, "Selector" },
		{ STYPE_DoorLock, "Door Lock" },
		{ 0, NULL, NULL }
	};
	return findTableIDSingle1 (Table, sType);
}

const char *Meter_Type_Desc(const _eMeterType sType)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ MTYPE_ENERGY, "Energy" },
		{ MTYPE_GAS, "Gas" },
		{ MTYPE_WATER, "Water" },
		{ MTYPE_COUNTER, "Counter" },
		{ MTYPE_ENERGY_GENERATED, "Energy Generated" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, sType);
}

const char *Notification_Type_Desc(const int nType, const unsigned char snum)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ NTYPE_TEMPERATURE, "Temperature","T" },
		{ NTYPE_HUMIDITY, "Humidity","H" },
		{ NTYPE_RAIN, "Rain","R" },
		{ NTYPE_UV, "UV","U" },
		{ NTYPE_WIND, "Wind","W" },
		{ NTYPE_USAGE, "Usage","M" },
		{ NTYPE_BARO, "Baro","B" },
		{ NTYPE_SWITCH_ON, "Switch On", "S" },
		{ NTYPE_AMPERE1, "Ampere 1", "1" },
		{ NTYPE_AMPERE2, "Ampere 2", "2" },
		{ NTYPE_AMPERE3, "Ampere 3", "3" },
		{ NTYPE_ENERGYINSTANT, "Instant", "I" },
		{ NTYPE_TODAYENERGY, "Today", "E" },
		{ NTYPE_TODAYGAS, "Today", "G" },
		{ NTYPE_TODAYCOUNTER, "Today", "C" },
		{ NTYPE_SWITCH_OFF, "Switch Off", "O" },
		{ NTYPE_PERCENTAGE, "Percentage", "P" },
		{ NTYPE_RPM, "RPM", "Z" },
		{ NTYPE_DEWPOINT, "Dew Point", "D" },
		{ NTYPE_SETPOINT, "Set Point", "N" },
		{ NTYPE_VIDEO, "Play Video", "V" },
		{ NTYPE_AUDIO, "Play Audio", "A" },
		{ NTYPE_PHOTO, "View Photo", "X" },
		{ NTYPE_PAUSED, "Pause Stream", "Y" },
		{ NTYPE_STOPPED, "Stop Stream", "Q" },
		{ NTYPE_PLAYING, "Play Stream", "a" },
		{ NTYPE_VALUE, "Value", "F" },
		{ NTYPE_LASTUPDATE, "Last Update", "J"},
		{  0,NULL,NULL }
	};
	if (snum==0)
		return findTableIDSingle1 (Table, nType);
	else
		return findTableIDSingle2 (Table, nType);
}

const char *Notification_Type_Label(const int nType)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ NTYPE_TEMPERATURE, "degrees" },
		{ NTYPE_HUMIDITY, "%%" },
		{ NTYPE_RAIN, "mm" },
		{ NTYPE_UV, "UVI" },
		{ NTYPE_WIND, "m/s" },
		{ NTYPE_USAGE, "" },
		{ NTYPE_BARO, "hPa" },
		{ NTYPE_SWITCH_ON, "" },
		{ NTYPE_AMPERE1, "Ampere"},
		{ NTYPE_AMPERE2, "Ampere"},
		{ NTYPE_AMPERE3, "Ampere" },
		{ NTYPE_ENERGYINSTANT, "Watt" },
		{ NTYPE_TODAYENERGY, "kWh" },
		{ NTYPE_TODAYGAS, "m3" },
		{ NTYPE_TODAYCOUNTER, "cnt" },
		{ NTYPE_SWITCH_OFF, "On" },
		{ NTYPE_PERCENTAGE, "%%" },
		{ NTYPE_RPM, "RPM" },
		{ NTYPE_DEWPOINT, "degrees" },
		{ NTYPE_SETPOINT, "degrees" },
		{ NTYPE_VIDEO, "" },
		{ NTYPE_AUDIO, "" },
		{ NTYPE_PHOTO, "" },
		{ NTYPE_PAUSED, "" },
		{ NTYPE_STOPPED, "" },
		{ NTYPE_PLAYING, "" },
		{ NTYPE_VALUE, "" },
		{ NTYPE_LASTUPDATE, "minutes" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, nType);
}

const char *RFX_Forecast_Desc(const unsigned char Forecast)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ baroForecastNoInfo, "No Info" },
		{ baroForecastSunny, "Sunny" },
		{ baroForecastPartlyCloudy, "Partly Cloudy" },
		{ baroForecastCloudy, "Cloudy" },
		{ baroForecastRain, "Rain" },
		{  0,NULL,NULL }
	};
	return findTableIDSingle1 (Table, Forecast);
}

const char *RFX_WSForecast_Desc(const unsigned char Forecast)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ wsbaroforcast_heavy_snow,"Heavy Snow" },
		{ wsbaroforcast_snow, "Snow" },
		{ wsbaroforcast_heavy_rain, "Heavy Rain" },
		{ wsbaroforcast_rain, "Rain" },
		{ wsbaroforcast_cloudy, "Cloudy" },
		{ wsbaroforcast_some_clouds, "Some Clouds" },
		{ wsbaroforcast_sunny, "Sunny" },
		{ wsbaroforcast_unknown, "Unknown" },
		{ wsbaroforcast_unstable, "Unstable" },
		{ wsbaroforcast_stable, "Stable" },
		{ 0, NULL, NULL }
	};
	return findTableIDSingle1 (Table, Forecast);
}

const char *BMP_Forecast_Desc(const unsigned char Forecast)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ bmpbaroforecast_stable, "Stable" },
		{ bmpbaroforecast_sunny, "Sunny" },
		{ bmpbaroforecast_cloudy, "Cloudy" },
		{ bmpbaroforecast_unstable, "Unstable" },
		{ bmpbaroforecast_thunderstorm, "Thunderstorm" },
		{ bmpbaroforecast_unknown, "Unknown" },
		{ bmpbaroforecast_rain, "Cloudy/Rain" },
		{ 0, NULL, NULL }
	};
	return findTableIDSingle1(Table, Forecast);
}

const char *RFX_Type_Desc(const unsigned char i, const unsigned char snum)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ pTypeInterfaceControl, "Interface Control", "unknown" },
		{ pTypeInterfaceMessage, "Interface Message", "unknown" },
		{ pTypeRecXmitMessage, "Receiver/Transmitter Message", "unknown" },
		{ pTypeUndecoded, "Undecoded RF Message", "unknown" },
		{ pTypeLighting1, "Lighting 1" , "lightbulb", },
		{ pTypeLighting2, "Lighting 2" , "lightbulb", },
		{ pTypeLighting3, "Lighting 3" , "lightbulb", },
		{ pTypeLighting4, "Lighting 4" , "lightbulb", },
		{ pTypeLighting5, "Lighting 5" , "lightbulb", },
		{ pTypeLighting6, "Lighting 6" , "lightbulb", },
		{ pTypeHomeConfort, "Home Confort" , "lightbulb" },
		{ pTypeLimitlessLights, "Lighting Limitless/Applamp" , "lightbulb" },
		{ pTypeCurtain, "Curtain" , "blinds" },
		{ pTypeBlinds, "Blinds" , "blinds" },
		{ pTypeSecurity1, "Security", "security" },
		{ pTypeSecurity2, "Security", "security" },
		{ pTypeCamera, "Camera", "unknown" },
		{ pTypeRemote, "Remote & IR" , "unknown" },
		{ pTypeThermostat1, "Thermostat 1" , "temperature" },
		{ pTypeThermostat2, "Thermostat 2" , "temperature" },
		{ pTypeThermostat3, "Thermostat 3" , "temperature" },
		{ pTypeThermostat4, "Thermostat 4" , "temperature" },
		{ pTypeRadiator1, "Radiator 1", "temperature" },
		{ pTypeTEMP, "Temp", "temperature" },
		{ pTypeHUM, "Humidity" , "temperature" },
		{ pTypeTEMP_HUM, "Temp + Humidity" , "temperature" },
		{ pTypeBARO, "Barometric" , "temperature" },
		{ pTypeTEMP_HUM_BARO, "Temp + Humidity + Baro" , "temperature" },
		{ pTypeRAIN, "Rain" , "rain" },
		{ pTypeWIND, "Wind" , "wind" },
		{ pTypeUV, "UV" , "uv" },
		{ pTypeDT, "Date/Time" , "unknown" },
		{ pTypeCURRENT, "Current" , "current" },
		{ pTypeENERGY, "Energy" , "current" },
		{ pTypeCURRENTENERGY, "Current/Energy" , "current" },
		{ pTypeGAS, "Gas" , "counter" },
		{ pTypeWATER, "Water" , "counter" },
		{ pTypeWEIGHT, "Weight" , "scale" },
		{ pTypeRFXSensor, "RFXSensor" , "unknown" },
		{ pTypeRFXMeter, "RFXMeter" , "counter" },
		{ pTypeP1Power, "P1 Smart Meter" , "counter" },
		{ pTypeP1Gas, "P1 Smart Meter" , "counter" },
		{ pTypeYouLess, "YouLess Meter", "counter" },
		{ pTypeFS20, "FS20" , "unknown" },
		{ pTypeRego6XXTemp, "Temp" , "temperature" },
		{ pTypeRego6XXValue, "Value" , "utility" },
		{ pTypeAirQuality, "Air Quality" , "air" },
		{ pTypeUsage, "Usage" , "current" },
		{ pTypeTEMP_BARO, "Temp + Baro" , "temperature" },
		{ pTypeLux, "Lux" , "lux" },
		{ pTypeGeneral, "General" , "General" },
		{ pTypeThermostat, "Thermostat" , "thermostat" },
		{ pTypeTEMP_RAIN, "Temp + Rain" , "Temp + Rain" },
		{ pTypeChime, "Chime" , "doorbell" },
		{ pTypeFan, "Fan" , "fan" },
		{ pTypeBBQ, "BBQ Meter", "bbq" },
		{ pTypePOWER, "Power" , "current" },
		{ pTypeRFY, "RFY" , "blinds" },
		{ pTypeEvohome, "Heating" , "evohome" },
		{ pTypeEvohomeZone, "Heating" , "evohome" },
		{ pTypeEvohomeWater, "Heating" , "evohome" },
		{ pTypeEvohomeRelay, "Heating" , "evohome" },
		{ pTypeGeneralSwitch, "Light/Switch", "lightbulb" },
		{ 0, NULL, NULL }
	};
	if (snum==1)
		return findTableIDSingle1 (Table, i);

	return findTableIDSingle2 (Table, i);
}

const char *RFX_Type_SubType_Desc(const unsigned char dType, const unsigned char sType)
{
	static const STR_TABLE_ID1_ID2	Table[] =
	{
		{ pTypeTEMP, sTypeTEMP1, "THR128/138, THC138" },
		{ pTypeTEMP, sTypeTEMP2, "THC238/268, THN132, THWR288, THRN122, THN122, AW129/131" },
		{ pTypeTEMP, sTypeTEMP3, "THWR800" },
		{ pTypeTEMP, sTypeTEMP4, "RTHN318" },
		{ pTypeTEMP, sTypeTEMP5, "LaCrosse TX3" },
		{ pTypeTEMP, sTypeTEMP6, "TS15C" },
		{ pTypeTEMP, sTypeTEMP7, "Viking 02811/02813, Proove TSS330" },
		{ pTypeTEMP, sTypeTEMP8, "LaCrosse WS2300" },
		{ pTypeTEMP, sTypeTEMP9, "RUBiCSON" },
		{ pTypeTEMP, sTypeTEMP10, "TFA 30.3133" },
		{ pTypeTEMP, sTypeTEMP11, "WT0122 Pool sensor" },
		{ pTypeTEMP, sTypeTEMP_SYSTEM, "System" },

		{ pTypeHUM, sTypeHUM1, "LaCrosse TX3" },
		{ pTypeHUM, sTypeHUM2, "LaCrosse WS2300" },

		{ pTypeTEMP_HUM, sTypeTH1, "THGN122/123, THGN132, THGR122/228/238/268" },
		{ pTypeTEMP_HUM, sTypeTH2, "THGR810, THGN800" },
		{ pTypeTEMP_HUM, sTypeTH3, "RTGR328" },
		{ pTypeTEMP_HUM, sTypeTH4, "THGR328" },
		{ pTypeTEMP_HUM, sTypeTH5, "WTGR800" },
		{ pTypeTEMP_HUM, sTypeTH6, "THGR918, THGRN228, THGN500" },
		{ pTypeTEMP_HUM, sTypeTH7, "Cresta, TFA TS34C" },
		{ pTypeTEMP_HUM, sTypeTH8, "WT450H" },
		{ pTypeTEMP_HUM, sTypeTH9, "Viking 02035, 02038, TSS320" },
		{ pTypeTEMP_HUM, sTypeTH10, "Rubicson/IW008T/TX95" },
		{ pTypeTEMP_HUM, sTypeTH11, "Oregon EW109" },
		{ pTypeTEMP_HUM, sTypeTH12, "Imagintronix" },
		{ pTypeTEMP_HUM, sTypeTH13, "Alecto WS1700" },
		{ pTypeTEMP_HUM, sTypeTH14, "Alecto" },
		{ pTypeTEMP_HUM, sTypeTH_LC_TC, "LaCrosse TX3" },


		{ pTypeTEMP_HUM_BARO, sTypeTHB1, "THB1 - BTHR918, BTHGN129" },
		{ pTypeTEMP_HUM_BARO, sTypeTHB2, "THB2 - BTHR918N, BTHR968" },
		{ pTypeTEMP_HUM_BARO, sTypeTHBFloat, "Weather Station" },

		{ pTypeRAIN, sTypeRAIN1, "RGR126/682/918/928" },
		{ pTypeRAIN, sTypeRAIN2, "PCR800" },
		{ pTypeRAIN, sTypeRAIN3, "TFA" },
		{ pTypeRAIN, sTypeRAIN4, "UPM RG700" },
		{ pTypeRAIN, sTypeRAIN5, "LaCrosse WS2300" },
		{ pTypeRAIN, sTypeRAIN6, "LaCrosse TX5" },
		{ pTypeRAIN, sTypeRAINWU, "WWW" },

		{ pTypeWIND, sTypeWIND1, "WTGR800" },
		{ pTypeWIND, sTypeWIND2, "WGR800" },
		{ pTypeWIND, sTypeWIND3, "STR918/928, WGR918" },
		{ pTypeWIND, sTypeWIND4, "TFA" },
		{ pTypeWIND, sTypeWIND5, "UPM WDS500" },
		{ pTypeWIND, sTypeWIND6, "LaCrosse WS2300" },
		{ pTypeWIND, sTypeWIND7, "Alecto WS4500" },
		{ pTypeWIND, sTypeWINDNoTemp, "Weather Station" },

		{ pTypeUV, sTypeUV1, "UVN128,UV138" },
		{ pTypeUV, sTypeUV2, "UVN800" },
		{ pTypeUV, sTypeUV3, "TFA" },

		{ pTypeLighting1, sTypeX10, "X10" },
		{ pTypeLighting1, sTypeARC, "ARC" },
		{ pTypeLighting1, sTypeAB400D, "ELRO AB400" },
		{ pTypeLighting1, sTypeWaveman, "Waveman" },
		{ pTypeLighting1, sTypeEMW200, "EMW200" },
		{ pTypeLighting1, sTypeIMPULS, "Impuls" },
		{ pTypeLighting1, sTypeRisingSun, "RisingSun" },
		{ pTypeLighting1, sTypePhilips, "Philips" },
		{ pTypeLighting1, sTypeEnergenie, "Energenie" },
		{ pTypeLighting1, sTypeEnergenie5, "Energenie 5-gang" },
		{ pTypeLighting1, sTypeGDR2, "COCO GDR2" },
		{ pTypeLighting1, sTypeHQ, "HQ COCO-20" },

		{ pTypeLighting2, sTypeAC, "AC" },
		{ pTypeLighting2, sTypeHEU, "HomeEasy EU" },
		{ pTypeLighting2, sTypeANSLUT, "Anslut" },

		{ pTypeLighting3, sTypeKoppla, "Ikea Koppla" },

		{ pTypeLighting4, sTypePT2262, "PT2262" },

		{ pTypeLighting5, sTypeLightwaveRF, "LightwaveRF" },
		{ pTypeLighting5, sTypeEMW100, "EMW100" },
		{ pTypeLighting5, sTypeBBSB, "BBSB new" },
		{ pTypeLighting5, sTypeMDREMOTE, "MDRemote" },
		{ pTypeLighting5, sTypeRSL, "Conrad RSL" },
		{ pTypeLighting5, sTypeLivolo, "Livolo" },
		{ pTypeLighting5, sTypeTRC02, "TRC02 (RGB)" },
		{ pTypeLighting5, sTypeTRC02_2, "TRC02_2 (RGB)" },
		{ pTypeLighting5, sTypeAoke, "Aoke" },
		{ pTypeLighting5, sTypeEurodomest, "Eurodomest" },
		{ pTypeLighting5, sTypeLivoloAppliance, "Livolo Appliance" },
		{ pTypeLighting5, sTypeRGB432W, "RGB432W" },
		{ pTypeLighting5, sTypeMDREMOTE107, "MDRemote 107" },
		{ pTypeLighting5, sTypeLegrandCAD, "Legrand CAD" },
		{ pTypeLighting5, sTypeAvantek, "Avantek" },
		{ pTypeLighting5, sTypeIT, "Intertek,FA500,PROmax" },
		{ pTypeLighting5, sTypeMDREMOTE108, "MDRemote 108" },
		{ pTypeLighting5, sTypeKangtai, "Kangtai / Cotech" },

		{ pTypeLighting6, sTypeBlyss, "Blyss" },

		{ pTypeHomeConfort, sTypeHomeConfortTEL010 , "TEL-010" },

		{ pTypeCurtain, sTypeHarrison, "Harrison" },

		{ pTypeBlinds, sTypeBlindsT0, "RollerTrol, Hasta new" },
		{ pTypeBlinds, sTypeBlindsT1, "Hasta old" },
		{ pTypeBlinds, sTypeBlindsT2, "A-OK RF01" },
		{ pTypeBlinds, sTypeBlindsT3, "A-OK AC114" },
		{ pTypeBlinds, sTypeBlindsT4, "RAEX" },
		{ pTypeBlinds, sTypeBlindsT5, "Media Mount" },
		{ pTypeBlinds, sTypeBlindsT6, "DC106" },
		{ pTypeBlinds, sTypeBlindsT7, "Forest" },
		{ pTypeBlinds, sTypeBlindsT8, "Chamberlain CS4330CN" },
		{ pTypeBlinds, sTypeBlindsT9, "Sunpery" },
		{ pTypeBlinds, sTypeBlindsT10, "Dolat DLM-1" },
		{ pTypeBlinds, sTypeBlindsT11, "ASP" },
		{ pTypeBlinds, sTypeBlindsT12, "Confexx" },
		{ pTypeBlinds, sTypeBlindsT13, "Screenline" },

		{ pTypeSecurity1, sTypeSecX10, "X10 security" },
		{ pTypeSecurity1, sTypeSecX10M, "X10 security motion" },
		{ pTypeSecurity1, sTypeSecX10R, "X10 security remote" },
		{ pTypeSecurity1, sTypeKD101, "KD101 smoke detector" },
		{ pTypeSecurity1, sTypePowercodeSensor, "Visonic PowerCode sensor - primary contact" },
		{ pTypeSecurity1, sTypePowercodeMotion, "Visonic PowerCode motion" },
		{ pTypeSecurity1, sTypeCodesecure, "Visonic CodeSecure" },
		{ pTypeSecurity1, sTypePowercodeAux, "Visonic PowerCode sensor - auxiliary contact" },
		{ pTypeSecurity1, sTypeMeiantech, "Meiantech/Atlantic/Aidebao" },
		{ pTypeSecurity1, sTypeSA30, "Alecto SA30 smoke detector" },
		{ pTypeSecurity1, sTypeDomoticzSecurity, "Security Panel" },

		{ pTypeSecurity2, sTypeSec2Classic, "KeeLoq" },

		{ pTypeCamera, sTypeNinja, "Meiantech" },

		{ pTypeRemote, sTypeATI, "ATI Remote Wonder" },
		{ pTypeRemote, sTypeATIplus, "ATI Remote Wonder Plus" },
		{ pTypeRemote, sTypeMedion, "Medion Remote" },
		{ pTypeRemote, sTypePCremote, "PC Remote" },
		{ pTypeRemote, sTypeATIrw2, "ATI Remote Wonder II" },

		{ pTypeThermostat1, sTypeDigimax, "Digimax" },
		{ pTypeThermostat1, sTypeDigimaxShort, "Digimax with short format" },

		{ pTypeThermostat2, sTypeHE105, "HE105" },
		{ pTypeThermostat2, sTypeRTS10, "RTS10" },

		{ pTypeThermostat3, sTypeMertikG6RH4T1, "Mertik G6R-H4T1" },
		{ pTypeThermostat3, sTypeMertikG6RH4TB, "Mertik G6R-H4TB" },
		{ pTypeThermostat3, sTypeMertikG6RH4TD, "Mertik G6R-H4TD" },
		{ pTypeThermostat3, sTypeMertikG6RH4S, "Mertik G6R-H4S" },

		{ pTypeThermostat4, sTypeMCZ1, "MCZ 1 fan model" },
		{ pTypeThermostat4, sTypeMCZ2, "MCZ 2 fan model" },
		{ pTypeThermostat4, sTypeMCZ3, "MCZ 3 fan model" },

		{ pTypeRadiator1, sTypeSmartwares, "Smartwares" },
		{ pTypeRadiator1, sTypeSmartwaresSwitchRadiator, "Smartwares Mode" },

		{ pTypeDT, sTypeDT1, "RTGR328N" },

		{ pTypeCURRENT, sTypeELEC1, "CM113, Electrisave" },

		{ pTypeENERGY, sTypeELEC2, "CM119 / CM160" },
		{ pTypeENERGY, sTypeELEC3, "CM180" },

		{ pTypeCURRENTENERGY, sTypeELEC4, "CM180i" },

		{ pTypeWEIGHT, sTypeWEIGHT1, "BWR102" },
		{ pTypeWEIGHT, sTypeWEIGHT2, "GR101" },

		{ pTypeRFXSensor, sTypeRFXSensorTemp, "Temperature" },
		{ pTypeRFXSensor, sTypeRFXSensorAD, "A/D" },
		{ pTypeRFXSensor, sTypeRFXSensorVolt, "Voltage" },

		{ pTypeRFXMeter, sTypeRFXMeterCount, "RFXMeter counter" },

		{ pTypeP1Power, sTypeP1Power, "Energy" },
		{ pTypeP1Gas, sTypeP1Gas, "Gas" },

		{ pTypeYouLess, sTypeYouLess, "YouLess counter" },

		{ pTypeRego6XXTemp, sTypeRego6XXTemp, "Rego 6XX" },
		{ pTypeRego6XXValue, sTypeRego6XXStatus, "Rego 6XX" },
		{ pTypeRego6XXValue, sTypeRego6XXCounter, "Rego 6XX" },

		{ pTypeAirQuality, sTypeVoltcraft, "Voltcraft CO-20" },

		{ pTypeUsage, sTypeElectric, "Electric" },

		{ pTypeTEMP_BARO, sTypeBMP085, "BMP085 I2C" },

		{ pTypeLux, sTypeLux, "Lux" },

		{ pTypeGeneral, sTypeVisibility, "Visibility" },
		{ pTypeGeneral, sTypeSolarRadiation, "Solar Radiation" },
		{ pTypeGeneral, sTypeSoilMoisture, "Soil Moisture" },
		{ pTypeGeneral, sTypeLeafWetness, "Leaf Wetness" },
		{ pTypeGeneral, sTypeSystemTemp, "System temperature" },
		{ pTypeGeneral, sTypePercentage, "Percentage" },
		{ pTypeGeneral, sTypeFan, "Fan" },
		{ pTypeGeneral, sTypeVoltage, "Voltage" },
		{ pTypeGeneral, sTypeCurrent, "Current" },
		{ pTypeGeneral, sTypePressure, "Pressure" },
		{ pTypeGeneral, sTypeBaro, "Barometer" },
		{ pTypeGeneral, sTypeSetPoint, "Setpoint" },
		{ pTypeGeneral, sTypeTemperature, "Temperature" },
		{ pTypeGeneral, sTypeZWaveClock, "Thermostat Clock" },
		{ pTypeGeneral, sTypeTextStatus, "Text" },
		{ pTypeGeneral, sTypeZWaveThermostatMode, "Thermostat Mode" },
		{ pTypeGeneral, sTypeZWaveThermostatFanMode, "Thermostat Fan Mode" },
		{ pTypeGeneral, sTypeAlert, "Alert" },
		{ pTypeGeneral, sTypeSoundLevel, "Sound Level" },
		{ pTypeGeneral, sTypeUV, "UV" },
		{ pTypeGeneral, sTypeDistance, "Distance" },
		{ pTypeGeneral, sTypeCounterIncremental, "Counter Incremental" },
		{ pTypeGeneral, sTypeKwh, "kWh" },
		{ pTypeGeneral, sTypeWaterflow, "Waterflow" },
		{ pTypeGeneral, sTypeCustom, "Custom Sensor" },
		{ pTypeGeneral, sTypeZWaveAlarm, "Alarm" },

		{ pTypeThermostat, sTypeThermSetpoint, "SetPoint" },
		{ pTypeThermostat, sTypeThermTemperature, "Temperature" },

		{ pTypeChime, sTypeByronSX, "ByronSX" },
		{ pTypeChime, sTypeByronMP001, "Byron MP001" },
		{ pTypeChime, sTypeSelectPlus, "SelectPlus" },
		{ pTypeChime, sTypeSelectPlus3, "SelectPlus3" },
		{ pTypeChime, sTypeEnvivo, "Envivo" },

		{ pTypeFan, sTypeSiemensSF01 , "Siemens SF01" },
		{ pTypeFan, sTypeItho , "Itho CVE RFT" },
		{ pTypeFan, sTypeLucciAir, "Lucci Air" },

		{ pTypeTEMP_RAIN, sTypeTR1, "Alecto WS1200" },

		{ pTypeBBQ, sTypeBBQ1, "Maverick ET-732" },

		{ pTypePOWER, sTypeELEC5, "Revolt" },

		{ pTypeLimitlessLights, sTypeLimitlessRGBW, "RGBW" },
		{ pTypeLimitlessLights, sTypeLimitlessRGB, "RGB" },
		{ pTypeLimitlessLights, sTypeLimitlessWhite, "White" },
		{ pTypeLimitlessLights, sTypeLimitlessRGBWW, "RGBWW" },

		{ pTypeRFY, sTypeRFY, "RFY" },
		{ pTypeRFY, sTypeRFY2, "RFY2" },
		{ pTypeRFY, sTypeRFYext, "RFY-Ext" },
		{ pTypeRFY, sTypeASA, "ASA" },

		{ pTypeEvohome, sTypeEvohome, "Evohome" },
		{ pTypeEvohomeZone, sTypeEvohomeZone, "Zone" },
		{ pTypeEvohomeWater, sTypeEvohomeWater, "Hot Water" },
		{ pTypeEvohomeRelay, sTypeEvohomeRelay, "Relay" },

		{ pTypeGeneralSwitch, sSwitchTypeX10, "X10" },
		{ pTypeGeneralSwitch, sSwitchTypeARC, "ARC" },
		{ pTypeGeneralSwitch, sSwitchTypeAB400D, "ELRO AB400" },
		{ pTypeGeneralSwitch, sSwitchTypeWaveman, "Waveman" },
		{ pTypeGeneralSwitch, sSwitchTypeEMW200, "EMW200" },
		{ pTypeGeneralSwitch, sSwitchTypeIMPULS, "Impuls" },
		{ pTypeGeneralSwitch, sSwitchTypeRisingSun, "RisingSun" },
		{ pTypeGeneralSwitch, sSwitchTypePhilips, "Philips" },
		{ pTypeGeneralSwitch, sSwitchTypeEnergenie, "Energenie" },
		{ pTypeGeneralSwitch, sSwitchTypeEnergenie5, "Energenie 5-gang" },
		{ pTypeGeneralSwitch, sSwitchTypeGDR2, "COCO GDR2" },
		{ pTypeGeneralSwitch, sSwitchTypeAC, "AC" },
		{ pTypeGeneralSwitch, sSwitchTypeHEU, "HomeEasy EU" },
		{ pTypeGeneralSwitch, sSwitchTypeANSLUT, "Anslut" },
		{ pTypeGeneralSwitch, sSwitchTypeKoppla, "Ikea Koppla" },
		{ pTypeGeneralSwitch, sSwitchTypePT2262, "PT2262" },
		{ pTypeGeneralSwitch, sSwitchTypeLightwaveRF, "LightwaveRF" },
		{ pTypeGeneralSwitch, sSwitchTypeEMW100, "EMW100" },
		{ pTypeGeneralSwitch, sSwitchTypeBBSB, "BBSB new" },
		{ pTypeGeneralSwitch, sSwitchTypeMDREMOTE, "MDRemote" },
		{ pTypeGeneralSwitch, sSwitchTypeRSL, "Conrad RSL" },
		{ pTypeGeneralSwitch, sSwitchTypeLivolo, "Livolo" },
		{ pTypeGeneralSwitch, sSwitchTypeTRC02, "TRC02 (RGB)" },
		{ pTypeGeneralSwitch, sSwitchTypeTRC02_2, "TRC02_2 (RGB)" },
		{ pTypeGeneralSwitch, sSwitchTypeAoke, "Aoke" },
		{ pTypeGeneralSwitch, sSwitchTypeEurodomest, "Eurodomest" },
		{ pTypeGeneralSwitch, sSwitchTypeLivoloAppliance, "Livolo Appliance" },
		{ pTypeGeneralSwitch, sSwitchTypeBlyss, "Blyss" },
		{ pTypeGeneralSwitch, sSwitchTypeByronSX, "ByronSX" },
		{ pTypeGeneralSwitch, sSwitchTypeByronMP001, "Byron MP001" },
		{ pTypeGeneralSwitch, sSwitchTypeSelectPlus, "SelectPlus" },
		{ pTypeGeneralSwitch, sSwitchTypeSelectPlus3, "SelectPlus3" },
		{ pTypeGeneralSwitch, sSwitchTypeFA20, "FA20RF" },
		{ pTypeGeneralSwitch, sSwitchTypeChuango, "Chuango" },
		{ pTypeGeneralSwitch, sSwitchTypePlieger, "Plieger" },
		{ pTypeGeneralSwitch, sSwitchTypeSilvercrest, "SilverCrest" },
		{ pTypeGeneralSwitch, sSwitchTypeMertik, "Mertik" },
		{ pTypeGeneralSwitch, sSwitchTypeHomeConfort, "HomeConfort" },
		{ pTypeGeneralSwitch, sSwitchTypePowerfix, "Powerfix" },
		{ pTypeGeneralSwitch, sSwitchTypeTriState, "TriState" },
		{ pTypeGeneralSwitch, sSwitchTypeDeltronic, "Deltronic" },
		{ pTypeGeneralSwitch, sSwitchTypeFA500, "FA500" },
		{ pTypeGeneralSwitch, sSwitchTypeHT12E, "HT12E" },
		{ pTypeGeneralSwitch, sSwitchTypeEV1527, "EV1527" },
		{ pTypeGeneralSwitch, sSwitchTypeElmes, "Elmes" },
		{ pTypeGeneralSwitch, sSwitchTypeAster, "Aster" },
		{ pTypeGeneralSwitch, sSwitchTypeSartano, "Sartano" },
		{ pTypeGeneralSwitch, sSwitchTypeEurope, "Europe" },
		{ pTypeGeneralSwitch, sSwitchTypeAvidsen, "Avidsen" },
		{ pTypeGeneralSwitch, sSwitchTypeBofu, "BofuMotor" },
		{ pTypeGeneralSwitch, sSwitchTypeBrel, "BrelMotor" },
		{ pTypeGeneralSwitch, sSwitchTypeRTS, "RTS" },
		{ pTypeGeneralSwitch, sSwitchTypeElroDB, "ElroDB" },
		{ pTypeGeneralSwitch, sSwitchTypeDooya, "Dooya" },
		{ pTypeGeneralSwitch, sSwitchTypeUnitec, "Unitec" },
		{ pTypeGeneralSwitch, sSwitchTypeSelector, "Selector Switch" },
		{ pTypeGeneralSwitch, sSwitchTypeMaclean, "Maclean" },
		{ pTypeGeneralSwitch, sSwitchTypeR546, "R546" },
		{ pTypeGeneralSwitch, sSwitchTypeDiya, "Diya" },
		{ pTypeGeneralSwitch, sSwitchTypeX10secu, "X10Secure" },
		{ pTypeGeneralSwitch, sSwitchTypeAtlantic, "Atlantic" },
		{ pTypeGeneralSwitch, sSwitchTypeSilvercrestDB, "SilvercrestDB" },
		{ pTypeGeneralSwitch, sSwitchTypeMedionDB, "MedionDB" },
		{ pTypeGeneralSwitch, sSwitchTypeVMC, "VMC" },
		{ pTypeGeneralSwitch, sSwitchTypeKeeloq, "Keeloq" },
		{ pTypeGeneralSwitch, sSwitchCustomSwitch, "CustomSwitch" },
		{ pTypeGeneralSwitch, sSwitchGeneralSwitch, "Switch" },
		{ pTypeGeneralSwitch, sSwitchTypeKoch, "Koch" },
		{ pTypeGeneralSwitch, sSwitchTypeKingpin, "Kingpin" },
		{ pTypeGeneralSwitch, sSwitchTypeFunkbus, "Funkbus" },
		{ pTypeGeneralSwitch, sSwitchTypeNice, "Nice" },
		{ pTypeGeneralSwitch, sSwitchTypeForest, "Forest" },
		{ pTypeGeneralSwitch, sSwitchBlindsT1, "Legrand MyHome Blind Bus" },
		{ pTypeGeneralSwitch, sSwitchLightT1, "Legrand MyHome Light Bus" },
		{ pTypeGeneralSwitch, sSwitchAuxiliaryT1, "Legrand MyHome Auxiliary Bus" },
		{ pTypeGeneralSwitch, sSwitchContactT1, "Legrand MyHome DryContact/IRdetec" },
		{ pTypeGeneralSwitch, sSwitchMC145026, "MC145026" },
		{ pTypeGeneralSwitch, sSwitchLobeco, "Lobeco" },
		{ pTypeGeneralSwitch, sSwitchFriedland, "Friedland" },
		{ pTypeGeneralSwitch, sSwitchBFT, "BFT" },
		{ pTypeGeneralSwitch, sSwitchNovatys, "Novatys" },
		{ pTypeGeneralSwitch, sSwitchHalemeier, "Halemeier" },
		{ pTypeGeneralSwitch, sSwitchGaposa, "Gaposa" },
		{ pTypeGeneralSwitch, sSwitchMiLightv1, "MiLightv1" },
		{ pTypeGeneralSwitch, sSwitchMiLightv2, "MiLightv2" },
		{ pTypeGeneralSwitch, sSwitchHT6P20, "HT6P20" },
		{ pTypeGeneralSwitch, sSwitchTypeDoitrand, "Doitrand" },
		{ pTypeGeneralSwitch, sSwitchTypeWarema, "Warema" },
		{ pTypeGeneralSwitch, sSwitchTypeAnsluta, "Ansluta" },
		{ pTypeGeneralSwitch, sSwitchTypeLivcol, "Livcol" },
		{ pTypeGeneralSwitch, sSwitchTypeBosch, "Bosch" },
		{ pTypeGeneralSwitch, sSwitchTypeNingbo, "Ningbo" },
		{ pTypeGeneralSwitch, sSwitchTypeDitec, "Ditec" },
		{ pTypeGeneralSwitch, sSwitchTypeSteffen, "Steffen" },
		{ pTypeGeneralSwitch, sSwitchTypeAlectoSA, "AlectoSA" },
		{ pTypeGeneralSwitch, sSwitchTypeGPIOset, "GPIOset" },
		{ pTypeGeneralSwitch, sSwitchTypeKonigSec, "KonigSec" },
		{ pTypeGeneralSwitch, sSwitchTypeRM174RF, "RM174RF" },
		{ pTypeGeneralSwitch, sSwitchTypeLiwin, "Liwin" },
		{ pTypeGeneralSwitch, sSwitchBlindsT2, "Legrand MyHome Blind Zigbee" },
		{ pTypeGeneralSwitch, sSwitchLightT2, "Legrand MyHome Light Zigbee" },
		{ pTypeGeneralSwitch, sSwitchTypeYW_Secu, "YW_Secu" },
		{ pTypeGeneralSwitch, sSwitchTypeMertik_GV60, "Mertik_GV60" },
		{ pTypeGeneralSwitch, sSwitchTypeNingbo64, "Ningbo64"},
		{ pTypeGeneralSwitch, sSwitchTypeX2D, "X2D" },
		{ pTypeGeneralSwitch, sSwitchTypeHRCMotor, "HRCMotor" },
		{ pTypeGeneralSwitch, sSwitchTypeVelleman, "Velleman" },
		{ pTypeGeneralSwitch, sSwitchTypeRFCustom, "RFCustom" },
		{ pTypeGeneralSwitch, sSwitchTypeYW_Sensor, "YW_Sensor" },
		{ pTypeGeneralSwitch, sSwitchTypeLegrandcad, "LEGRANDCAD" },
		{ pTypeGeneralSwitch, sSwitchTypeSysfsGpio, "SysfsGpio" },
		{ pTypeGeneralSwitch, sSwitchTypeHager, "Hager" },
		{ pTypeGeneralSwitch, sSwitchTypeFaber, "Faber" },
		{ pTypeGeneralSwitch, sSwitchTypeDrayton, "Drayton" },
		{ pTypeGeneralSwitch, sSwitchTypeV2Phoenix, "V2Phoenix" },
		{  0,0,NULL }
	};
	return findTableID1ID2(Table, dType, sType);
}

const char *Media_Player_States(const _eMediaStatus Status)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ MSTAT_OFF, "Off" },
		{ MSTAT_ON, "On" },
		{ MSTAT_PAUSED, "Paused" },
		{ MSTAT_STOPPED, "Stopped" },
		{ MSTAT_VIDEO, "Video" },
		{ MSTAT_AUDIO, "Audio" },
		{ MSTAT_PHOTO, "Photo" },
		{ MSTAT_PLAYING, "Playing" },
		{ MSTAT_DISCONNECTED, "Disconnected" },
		{ MSTAT_SLEEPING, "Sleeping" },
		{ MSTAT_UNKNOWN, "Unknown" },
		{ 0, NULL, NULL }
	};
	return findTableIDSingle1(Table, Status);
}

const char *ZWave_Clock_Days(const unsigned char Day)
{
	static const STR_TABLE_SINGLE	Table[] =
	{
		{ 0, "Monday" },
		{ 1, "Tuesday" },
		{ 2, "Wednesday" },
		{ 3, "Thursday" },
		{ 4, "Friday" },
		{ 5, "Saturday" },
		{ 6, "Sunday" },
		{ 0, NULL, NULL }
	};
	return findTableIDSingle1(Table, Day);
}
/*
const char *ZWave_Thermostat_Modes[] =
{
	"Off",
	"Heat",
	"Cool",
	"Auto",
	"Aux Heat",
	"Resume",
	"Fan Only",
	"Furnace",
	"Dry Air",
	"Moist Air",
	"Auto Changeover",
	"Heat Econ",
	"Cool Econ",
	"Away",
	"Unknown",
	NULL
};
*/
const char *ZWave_Thermostat_Fan_Modes[] =
{
	"Auto Low",
	"On Low",
	"Auto High",
	"On High",
	"Unknown 4",
	"Unknown 5",
	"Circulate",
	"Unknown",
	NULL
};

int Lookup_ZWave_Thermostat_Modes(const std::vector<std::string> &Modes, const std::string &sMode)
{
	int ii = 0;
	std::vector<std::string>::const_iterator itt;
	for (itt = Modes.begin(); itt != Modes.end(); ++itt)
	{
		if (*itt == sMode)
			return ii;
		ii++;
	}
	return -1;
}

int Lookup_ZWave_Thermostat_Fan_Modes(const std::string &sMode)
{
	int ii = 0;
	while (ZWave_Thermostat_Fan_Modes[ii]!=NULL)
	{
		if (ZWave_Thermostat_Fan_Modes[ii] == sMode)
		{
			return ii;
		}
		ii++;
	}
	return -1;
}


void GetLightStatus(
		const unsigned char dType,
		const unsigned char dSubType,
		const _eSwitchType switchtype,
		const unsigned char nValue,
		const std::string &sValue,
		std::string &lstatus,
		int &llevel,
		bool &bHaveDimmer,
		int &maxDimLevel,
		bool &bHaveGroupCmd)
{
	bHaveDimmer=false;
	maxDimLevel=0;
	bHaveGroupCmd=false;
	lstatus="";

	char szTmp[80];
	switch (dType)
	{
	case pTypeLighting1:
		switch (dSubType)
		{
		case sTypeX10:
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light1_sOff:
				lstatus="Off";
				break;
			case light1_sOn:
				lstatus="On";
				break;
			case light1_sDim:
				lstatus="Dim";
				break;
			case light1_sBright:
				lstatus="Bright";
				break;
			case light1_sAllOn:
				lstatus="All On";
				break;
			case light1_sAllOff:
				lstatus="All Off";
				break;
			}
			break;
		case sTypeARC:
		case sTypePhilips:
		case sTypeEnergenie:
		case sTypeEnergenie5:
		case sTypeGDR2:
		case sTypeHQ:
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light1_sOff:
				lstatus="Off";
				break;
			case light1_sOn:
				lstatus="On";
				break;
			case light1_sAllOn:
				lstatus="All On";
				break;
			case light1_sAllOff:
				lstatus="All Off";
				break;
			case light1_sChime:
				lstatus="Chime";
				break;
			}
			break;
		case sTypeAB400D:
		case sTypeWaveman:
		case sTypeEMW200:
		case sTypeIMPULS:
		case sTypeRisingSun:
			switch (nValue)
			{
			case light1_sOff:
				lstatus="Off";
				break;
			case light1_sOn:
				lstatus="On";
				break;
			}
			break;
		}
		break;
	case pTypeLighting2:
		// Determine max dim level based on switch type
		maxDimLevel = 15;

		if (switchtype != STYPE_Media) {
			// Calculate % that the light is currently on, taking the maxdimlevel into account.
			llevel=(int)float((100.0f/float(maxDimLevel))*atof(sValue.c_str()));
		}

		// Fill in other parameters
		switch (dSubType)
		{
		case sTypeAC:
		case sTypeHEU:
		case sTypeANSLUT:
		case sTypeKambrook:
			bHaveDimmer=true;
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light2_sOff:
				lstatus="Off";
				break;
			case light2_sOn:
				lstatus="On";
				break;
			case light2_sSetLevel:
				sprintf(szTmp,"Set Level: %d %%", llevel);
				if (sValue!="0")
					lstatus=szTmp;
				else
					lstatus="Off";
				break;
			case light2_sGroupOff:
				lstatus="Group Off";
				break;
			case light2_sGroupOn:
				lstatus="Group On";
				break;
			case light2_sSetGroupLevel:
				sprintf(szTmp,"Set Group Level: %d %%", atoi(sValue.c_str()));
				if (sValue!="0")
					lstatus=szTmp;
				else
					lstatus="Off";
				break;
			}
			break;
		}
		break;
	case pTypeLighting4:
		switch (nValue)
		{
		case light2_sOff:
			lstatus="Off";
			break;
		case light2_sOn:
			lstatus="On";
			break;
		}
		break;
	case pTypeLighting5:
		if (dSubType==sTypeLivolo)
			llevel=int((100.0f/7.0f)*atof(sValue.c_str()));
		else
			llevel=int((100.0f/31.0f)*atof(sValue.c_str()));
		switch (dSubType)
		{
		case sTypeLightwaveRF:
			bHaveGroupCmd=true;
			bHaveDimmer=true;
			maxDimLevel = 32;
			llevel = (int)float((100.0f / float(maxDimLevel))*atof(sValue.c_str()));
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sGroupOff:
				lstatus="Group Off";
				break;
			case light5_sMood1:
				lstatus="Group Mood 1";
				break;
			case light5_sMood2:
				lstatus="Group Mood 2";
				break;
			case light5_sMood3:
				lstatus="Group Mood 3";
				break;
			case light5_sMood4:
				lstatus="Group Mood 4";
				break;
			case light5_sMood5:
				lstatus="Group Mood 5";
				break;
			case light5_sUnlock:
				lstatus="Unlock";
				break;
			case light5_sLock:
				lstatus="Lock";
				break;
			case light5_sAllLock:
				lstatus="All lock";
				break;
			case light5_sClose:
				lstatus="Close inline relay";
				break;
			case light5_sStop:
				lstatus="Stop inline relay";
				break;
			case light5_sOpen:
				lstatus="Open inline relay";
				break;
			case light5_sSetLevel:
				sprintf(szTmp,"Set Level: %d %%" ,llevel);
				if (sValue!="0")
					lstatus=szTmp;
				else
					lstatus="Off";
				break;
			}
			break;
		case sTypeEMW100:
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sLearn:
				lstatus="Learn";
				break;
			}
			break;
		case sTypeBBSB:
		case sTypeRSL:
		case sTypeKangtai:
			bHaveGroupCmd=true;
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sGroupOff:
				lstatus="Group Off";
				break;
			case light5_sGroupOn:
				lstatus="Group On";
				break;
			}
			break;
		case sTypeLivolo:
			bHaveGroupCmd=true;
			bHaveDimmer=true;
			maxDimLevel=7;
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			case light5_sLivoloGang2Toggle:
				lstatus="Set Level";
				break;
			case light5_sLivoloGang3Toggle:
				lstatus="Set Level";
				break;
			}
			break;
		case sTypeLivoloAppliance:
			bHaveGroupCmd = true;
			switch (nValue)
			{
			case light5_sLivoloAllOff:
				lstatus = "Off";
				break;
			case light5_sLivoloGang1Toggle:
				lstatus = "On";
				break;
			}
			break;
		case sTypeRGB432W:
			switch (nValue)
			{
			case light5_sRGBoff:
				lstatus = "Off";
				break;
			case light5_sRGBon:
				lstatus = "On";
				break;
			case light5_sRGBbright:
				lstatus = "Bright";
				break;
			case light5_sRGBdim:
				lstatus = "Dim";
				break;
			case light5_sRGBcolorplus:
				lstatus = "Color Plus";
				break;
			case light5_sRGBcolormin:
				lstatus = "Color Min";
				break;
			}
			break;
		case sTypeTRC02:
		case sTypeTRC02_2:
			bHaveGroupCmd = true;
			bHaveDimmer=true;
			maxDimLevel=7;
			switch (nValue)
			{
			case light5_sOff:
				lstatus="Off";
				break;
			case light5_sOn:
				lstatus="On";
				break;
			}
			break;
		case sTypeAoke:
			switch (nValue)
			{
			case light5_sOff:
				lstatus = "Off";
				break;
			case light5_sOn:
				lstatus = "On";
				break;
			}
			break;
		case sTypeEurodomest:
			switch (nValue)
			{
			case light5_sOff:
				lstatus = "Off";
				break;
			case light5_sOn:
				lstatus = "On";
				break;
			case light5_sGroupOff:
				lstatus = "Group Off";
				break;
			case light5_sGroupOn:
				lstatus = "Group On";
				break;
			}
			break;
		case sTypeIT:
			maxDimLevel = 9;
			llevel = (int)float((100.0f / float(maxDimLevel))*atof(sValue.c_str()));
			switch (nValue)
			{
			case light5_sOff:
				lstatus = "Off";
				break;
			case light5_sOn:
				lstatus = "On";
				break;
			case light5_sGroupOff:
				lstatus = "Group Off";
				break;
			case light5_sGroupOn:
				lstatus = "Group On";
				break;
			case light5_sSetLevel:
				sprintf(szTmp, "Set Level: %d %%", llevel);
				if (sValue != "0")
					lstatus = szTmp;
				else
					lstatus = "Off";
				break;
			}
			break;
		}
		break;
	case pTypeLighting6:
		bHaveGroupCmd=true;
		switch (dSubType)
		{
		case sTypeBlyss:
			switch (nValue)
			{
			case light6_sOff:
				lstatus="Off";
				break;
			case light6_sOn:
				lstatus="On";
				break;
			case light6_sGroupOff:
				lstatus="Group Off";
				break;
			case light6_sGroupOn:
				lstatus="Group On";
				break;
			}
		}
		break;
	case pTypeHomeConfort:
		switch (dSubType)
		{
		case sTypeHomeConfortTEL010:
			bHaveGroupCmd = true;
			switch (nValue)
			{
			case HomeConfort_sOff:
				lstatus = "Off";
				break;
			case HomeConfort_sOn:
				lstatus = "On";
				break;
			case HomeConfort_sGroupOn:
				lstatus = "Group On";
				break;
			case HomeConfort_sGroupOff:
				lstatus = "Group Off";
				break;
			}
			break;
		}
		break;
	case pTypeGeneralSwitch:
		maxDimLevel = 100;

		// Calculate % that the light is currently on, taking the maxdimlevel into account.
		llevel = (int)float((100.0f / float(maxDimLevel))*atof(sValue.c_str()));

		// Fill in other parameters
		switch (dSubType)
		{
		case sTypeAC:
		case sTypeHEU:
		case sTypeANSLUT:
		case sSwitchTypeSelector:
		case sSwitchGeneralSwitch:
			bHaveDimmer = true;
			bHaveGroupCmd = true;
			break;
		}
		switch (nValue)
		{
		case gswitch_sOff:
			lstatus = "Off";
			break;
		case gswitch_sOn:
			lstatus = "On";
			break;
		case gswitch_sSetLevel:
			sprintf(szTmp, "Set Level: %d %%", llevel);
			if (sValue != "0")
				lstatus = szTmp;
			else
				lstatus = "Off";
			break;
		case gswitch_sGroupOff:
			lstatus = "Group Off";
			break;
		case gswitch_sGroupOn:
			lstatus = "Group On";
			break;
		case gswitch_sSetGroupLevel:
			sprintf(szTmp, "Set Group Level: %d %%", atoi(sValue.c_str()));
			if (sValue != "0")
				lstatus = szTmp;
			else
				lstatus = "Group Off";
			break;
		case gswitch_sDim:
			lstatus = "Dim";
			break;
		case gswitch_sBright:
			lstatus = "Bright";
			break;
		case gswitch_sSound0:
			lstatus = "Sound 0";
			break;
		case gswitch_sSound1:
			lstatus = "Sound 1";
			break;
		case gswitch_sSound2:
			lstatus = "Sound 2";
			break;
		case gswitch_sSound3:
			lstatus = "Sound 3";
			break;
		case gswitch_sSound4:
			lstatus = "Sound 4";
			break;
		case gswitch_sSound5:
			lstatus = "Sound 5";
			break;
		case gswitch_sSound6:
			lstatus = "Sound 6";
			break;
		case gswitch_sSound7:
			lstatus = "Sound 7";
			break;
		case gswitch_sSound8:
			lstatus = "Sound 8";
			break;
		case gswitch_sStop:
			lstatus = "Stop";
			break;
		case gswitch_sProgram:
			lstatus = "Program";
			break;
		}
		break;
	case pTypeLimitlessLights:
		bHaveDimmer=true;
		maxDimLevel=100;
		switch (nValue)
		{
		case Limitless_LedOff:
			lstatus="Off";
			break;
		case Limitless_LedOn:
			lstatus="On";
			break;
		case Limitless_SetBrightnessLevel:
			lstatus="Set Level";
			break;
		case Limitless_SetKelvinLevel:
			lstatus="Set Kelvin Level";
			break;
		case Limitless_SetColorToWhite:
			lstatus="Set to White";
			break;
		case Limitless_NightMode:
			lstatus="NightMode";
			break;
		case Limitless_DiscoMode_1:
			lstatus="Disco Mode 1";
			break;
		case Limitless_DiscoMode_2:
			lstatus="Disco Mode 2";
			break;
		case Limitless_DiscoMode_3:
			lstatus="Disco Mode 3";
			break;
		case Limitless_DiscoMode_4:
			lstatus="Disco Mode 4";
			break;
		case Limitless_DiscoMode_5:
			lstatus="Disco Mode 5";
			break;
		case Limitless_DiscoMode_6:
			lstatus="Disco Mode 6";
			break;
		case Limitless_DiscoMode_7:
			lstatus="Disco Mode 7";
			break;
		case Limitless_DiscoMode_8:
			lstatus="Disco Mode 8";
			break;
		case Limitless_DiscoMode_9:
			lstatus="Disco Mode 9";
			break;
		}
		break;
	case pTypeSecurity1:
		llevel=0;
		switch (nValue)
		{
		case sStatusNormal:
			lstatus="Normal";
			break;
		case sStatusNormalDelayed:
			lstatus="Normal Delayed";
			break;
		case sStatusAlarm:
			lstatus="Alarm";
			break;
		case sStatusAlarmDelayed:
			lstatus="Alarm Delayed";
			break;
		case sStatusMotion:
			lstatus="Motion";
			break;
		case sStatusNoMotion:
			lstatus="No Motion";
			break;
		case sStatusPanic:
			lstatus="Panic";
			break;
		case sStatusPanicOff:
			lstatus="Panic End";
			break;
		case sStatusArmAway:
			lstatus="Arm Away";
			break;
		case sStatusArmAwayDelayed:
			lstatus="Arm Away Delayed";
			break;
		case sStatusArmHome:
			lstatus="Arm Home";
			break;
		case sStatusArmHomeDelayed:
			lstatus="Arm Home Delayed";
			break;
		case sStatusDisarm:
			lstatus="Disarm";
			break;
		case sStatusLightOff:
			lstatus="Light Off";
			break;
		case sStatusLightOn:
			lstatus="Light On";
			break;
		case sStatusLight2Off:
			lstatus="Light 2 Off";
			break;
		case sStatusLight2On:
			lstatus="Light 2 On";
			break;
		case sStatusDark:
			lstatus="Dark detected";
			break;
		case sStatusLight:
			lstatus="Light Detected";
			break;
		case sStatusBatLow:
			lstatus="Battery low MS10 or XX18 sensor";
			break;
		case sStatusPairKD101:
			lstatus="Pair KD101";
			break;
		case sStatusNormalTamper:
			lstatus="Normal + Tamper";
			break;
		case sStatusNormalDelayedTamper:
			lstatus="Normal Delayed + Tamper";
			break;
		case sStatusAlarmTamper:
			lstatus="Alarm + Tamper";
			break;
		case sStatusAlarmDelayedTamper:
			lstatus="Alarm Delayed + Tamper";
			break;
		case sStatusMotionTamper:
			lstatus="Motion + Tamper";
			break;
		case sStatusNoMotionTamper:
			lstatus="No Motion + Tamper";
			break;
		}
		break;
	case pTypeSecurity2:
		llevel = 0;
		switch (nValue)
		{
		case 0:
			lstatus = "Normal";
			break;
		}
		break;
	case pTypeRego6XXValue:
		switch (nValue)
		{
		case 0:
			lstatus="Off";
			break;
		case 1:
			lstatus="On";
			break;
		}
		break;
	case pTypeCurtain:
		switch (nValue)
		{
		case curtain_sOpen:
			lstatus="Off";
			break;
		case curtain_sClose:
			lstatus="On";
			break;
		case curtain_sStop:
			lstatus="Stop";
			break;
		}
		break;
	case pTypeBlinds:
		switch (nValue)
		{
		case blinds_sOpen:
			if (dSubType == sTypeBlindsT10)
				lstatus = "On";
			else
				lstatus = "Off";
			break;
		case blinds_sClose:
			if (dSubType == sTypeBlindsT10)
				lstatus = "Off";
			else
				lstatus = "On";
			break;
		case blinds_sStop:
			lstatus="Stop";
			break;
		case blinds_sConfirm:
			lstatus="Confirm";
			break;
		case blinds_sLimit:
			lstatus="Limit";
			break;
		case blinds_slowerLimit:
			lstatus="Lower Limit";
			break;
		case blinds_sDeleteLimits:
			lstatus="Delete Limits";
			break;
		case blinds_sChangeDirection:
			lstatus="Change Direction";
			break;
		case blinds_sLeft:
			lstatus="Left";
			break;
		case blinds_sRight:
			lstatus="Right";
			break;
		}
		break;
	case pTypeRFY:
		if (switchtype == STYPE_OnOff)
		{
			switch (nValue)
			{
			case rfy_sUp:
				lstatus = "On";
				break;
			case rfy_sDown:
				lstatus = "Off";
				break;
			case rfy_sStop:
				lstatus = "Stop";
				break;
			}
		}
		else
		{
			switch (nValue)
			{
			case rfy_sUp:
				lstatus = "Off";
				break;
			case rfy_sDown:
				lstatus = "On";
				break;
			case rfy_sStop:
				lstatus = "Stop";
				break;
			case rfy_s05SecUp:
				if (switchtype == STYPE_VenetianBlindsUS)
				{
					lstatus = "Off";
				}
				break;
			case rfy_s2SecUp:
				if (switchtype == STYPE_VenetianBlindsEU)
				{
					lstatus = "Off";
				}
				break;
			case rfy_s05SecDown:
				if (switchtype == STYPE_VenetianBlindsUS)
				{
					lstatus = "On";
				}
				break;
			case rfy_s2SecDown:
				if (switchtype == STYPE_VenetianBlindsEU)
				{
					lstatus = "On";
				}
				break;
			case rfy_sEnableSunWind:
				lstatus = "SunWindChange";
				break;
			case rfy_sDisableSun:
				lstatus = "SunWindChange";
				break;
			}
		}
		break;
	case pTypeChime:
		lstatus="On";
		break;
	case pTypeRemote:
		lstatus="On";
		break;
	case pTypeThermostat2:
		switch (nValue)
		{
		case thermostat2_sOff:
			lstatus = "Off";
			break;
		case thermostat2_sOn:
			lstatus = "On";
			break;
		}
		break;
	case pTypeThermostat3:
		switch (nValue)
		{
		case thermostat3_sOff:
			lstatus="Off";
			break;
		case thermostat3_sOn:
			lstatus="On";
			break;
		case thermostat3_sUp:
			lstatus="Up";
			break;
		case thermostat3_sDown:
			lstatus="Down";
			break;
		case thermostat3_sRunUp:
			if (dSubType==sTypeMertikG6RH4T1)
				lstatus="Run Up";
			else
				lstatus="2nd Off";
			break;
		case thermostat3_sRunDown:
			if (dSubType==sTypeMertikG6RH4T1)
				lstatus="Run Down";
			else
				lstatus="2nd On";
			break;
		case thermostat3_sStop:
			lstatus="Stop";
			break;
		}
		break;
	case pTypeThermostat4:
		switch (nValue)
		{
		case thermostat4_sOff:
			lstatus = "Off";
			break;
		case thermostat4_sManual:
			lstatus = "Manual";
			break;
		case thermostat4_sAuto:
			lstatus = "Auto";
			break;
		case thermostat4_sEco:
			lstatus = "Eco";
			break;
		}
		break;
	case pTypeRadiator1:
		switch (nValue)
		{
		case Radiator1_sNight:
			lstatus = "Off";
			break;
		case Radiator1_sDay:
			lstatus = "On";
			break;
		case Radiator1_sSetTemp:
			lstatus = "On";
			break;
		}
		break;
	case pTypeEvohome:
		llevel=0;
		lstatus=CEvohomeBase::GetWebAPIModeName(nValue);
		break;
	case pTypeEvohomeRelay:
		bHaveDimmer=true;
		maxDimLevel=200;
		llevel=int(0.5f*atof(sValue.c_str()));
		switch (nValue)
		{
		case light1_sOff:
			lstatus="Off";
			break;
		case light1_sOn:
			lstatus="On";
			break;
		}
		break;
	case pTypeFan:
		switch (dSubType)
		{
			case sTypeSiemensSF01:
			{
				switch (nValue)
				{
					case fan_sTimer:
						lstatus = "timer";
						break;
					case fan_sPlus:
						lstatus = "+";
						break;
					case fan_sMin:
						lstatus = "-";
						break;
					case fan_sLearn:
						lstatus = "learn";
						break;
					case fan_sConfirm:
						lstatus = "confirm";
						break;
					case fan_sLight:
						lstatus = "light";
						break;
				}
			}
			break;
			case sTypeItho:
			{
				switch (nValue)
				{
					case fan_Itho1:
						lstatus = "1";
						break;
					case fan_Itho2:
						lstatus = "2";
						break;
					case fan_Itho3:
						lstatus = "3";
						break;
					case fan_IthoTimer:
						lstatus = "timer";
						break;
					case fan_IthoNotAtHome:
						lstatus = "notathome";
						break;
					case fan_IthoLearn:
						lstatus = "learn";
						break;
					case fan_IthoEraseAll:
						lstatus = "eraseall";
						break;
				}
			}
			break;
			case sTypeLucciAir:
			{
				switch (nValue)
				{
					case fan_LucciHi:
						lstatus = "hi";
						break;
					case fan_LucciMed:
						lstatus = "med";
						break;
					case fan_LucciLow:
						lstatus = "low";
						break;
					case fan_LucciOff:
						lstatus = "off";
						break;
					case fan_LucciLight:
						lstatus = "light";
						break;
				}
			}
			break;
		}
		break;
	}
	if (_log.isTraceEnabled()) _log.Log(LOG_TRACE,"RFXN : GetLightStatus Typ:%2d STyp:%2d nVal:%d sVal:%-4s llvl:%2d isDim:%d maxDim:%2d GrpCmd:%d lstat:%s",
		dType,dSubType,nValue,sValue.c_str(),llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd,lstatus.c_str());
}

/**
 * Returns a map associating a level value to its name.
 */
void GetSelectorSwitchStatuses(const std::map<std::string, std::string> & options, std::map<std::string, std::string> & statuses) {
	std::map< std::string, std::string >::const_iterator itt = options.find("LevelNames");
	if (itt != options.end()) {
		//_log.Log(LOG_STATUS, "DEBUG : Get selector switch statuses...");
		std::string sOptions = itt->second;
		std::vector<std::string> strarray;
		boost::split(strarray, sOptions, boost::is_any_of("|"), boost::token_compress_off);
		std::vector<std::string>::iterator itt;
		int i = 0;
		std::stringstream ss;
		for (itt = strarray.begin(); (itt != strarray.end()) && (i <= 100); ++itt) {
			ss.clear(); ss.str(""); ss << i;
			std::string level(ss.str());
			std::string levelName = *itt;
			//_log.Log(LOG_STATUS, "DEBUG : Get selector status '%s' for level %s", levelName.c_str(), level.c_str());
			statuses.insert(std::pair<std::string, std::string>(level.c_str(), levelName.c_str()));
			i += 10;
		}
	}
}

/**
 * Returns the level value associated to a name.
 */
int GetSelectorSwitchLevel(const std::map<std::string, std::string> & options, const std::string & levelName) {
	int level = -1; // not found
	std::map< std::string, std::string >::const_iterator itt = options.find("LevelNames");
	if (itt != options.end()) {
		//_log.Log(LOG_STATUS, "DEBUG : Get selector switch level...");
		std::string sOptions = itt->second;
		std::vector<std::string> strarray;
		boost::split(strarray, sOptions, boost::is_any_of("|"), boost::token_compress_off);
		std::vector<std::string>::iterator itt;
		int i = 0;
		for (itt = strarray.begin(); (itt != strarray.end()) && (i <= 100); ++itt) {
			if (*itt == levelName) {
				level = i;
				break;
			}
			i += 10;
		}
	}
	return level;
}

/**
 * Returns the action associated with a level
 */
std::string GetSelectorSwitchLevelAction(const std::map<std::string, std::string> & options, const int level) {
	std::string action = ""; // not found
	std::map< std::string, std::string >::const_iterator itt = options.find("LevelActions");
	if (itt != options.end()) {
		//_log.Log(LOG_STATUS, "DEBUG : Get selector switch level action...");
		std::string sOptions = itt->second;
		std::vector<std::string> strarray;
		boost::split(strarray, sOptions, boost::is_any_of("|"), boost::token_compress_off);
		std::vector<std::string>::iterator itt;
		int i = 0;
		for (itt = strarray.begin(); (itt != strarray.end()) && (i <= 100); ++itt) {
			if (i == level) {
				action = *itt;
				break;
			}
			i += 10;
		}
	}
	return action;
}

bool GetLightCommand(
	const unsigned char dType,
	const unsigned char dSubType,
	_eSwitchType switchtype,
	std::string switchcmd,
	unsigned char &cmd,
	const std::map<std::string, std::string> & options
	)
{
	if (switchtype==STYPE_Contact)
		return false;	//we can not (or will not) switch this type

	switch (dType)
	{
	case pTypeLighting1:
		if (switchtype==STYPE_Doorbell)
		{
			if (dSubType==sTypeARC)
			{
				if ((switchcmd=="On")||(switchcmd=="Group On")||(switchcmd=="Chime"))
				{
					cmd=light1_sChime;
					return true;
				}
			}
			else
			{
				//not sure yet, maybe more devices need the above chime command
				if ((switchcmd=="On")||(switchcmd=="Group On"))
				{
					cmd=light1_sAllOn;
					return true;
				}
			}
			//no other combinations for the door switch
			return false;
		}
		else if (switchtype==STYPE_X10Siren)
		{
			if ((switchcmd=="On")||(switchcmd=="All On"))
			{
				cmd=light1_sAllOn;
				return true;
			}
			else if ((switchcmd=="Off")||(switchcmd=="All Off"))
			{
				cmd=light1_sAllOff;
				return true;
			}
			return false;
		}
		if (switchcmd=="Off")
		{
			cmd=light1_sOff;
			return true;
		} else if (switchcmd=="On") {
			cmd=light1_sOn;
			return true;
		} else if (switchcmd=="Dim") {
			cmd=light1_sDim;
			return true;
		} else if (switchcmd=="Bright") {
			cmd=light1_sBright;
			return true;
		} else if (switchcmd=="All On") {
			cmd=light1_sAllOn;
			return true;
		} else if (switchcmd=="All Off") {
			cmd=light1_sAllOff;
			return true;
		} else if (switchcmd=="Chime") {
			cmd=light1_sChime;
			return true;
		} else
			return false;
		break;
	case pTypeLighting2:
		if (switchtype==STYPE_Doorbell)
		{
			if ((switchcmd=="On")||(switchcmd=="Group On"))
			{
				cmd=light2_sGroupOn;
				return true;
			}
			//no other combinations for the door switch
			return false;
		}
		else if (switchtype==STYPE_X10Siren)
		{
			if ((switchcmd=="On")||(switchcmd=="Group On"))
			{
				cmd=light2_sGroupOn;
				return true;
			}
			else if ((switchcmd=="Off")||(switchcmd=="Group Off"))
			{
				cmd=light2_sGroupOff;
				return true;
			}
			return false;
		}
		if (switchcmd=="Off")
		{
			cmd=light2_sOff;
			return true;
		}
		else if (switchcmd=="On")
		{
			cmd=light2_sOn;
			return true;
		}
		else if (switchcmd=="Set Level")
		{
			cmd=light2_sSetLevel;
			return true;
		}
		else if (switchcmd=="Group Off")
		{
			cmd=light2_sGroupOff;
			return true;
		}
		else if (switchcmd=="Group On")
		{
			cmd=light2_sGroupOn;
			return true;
		}
		else if (switchcmd=="Set Group Level")
		{
			cmd=light2_sSetGroupLevel;
			return true;
		}
		else if (switchcmd == "Stop")
		{
			cmd = gswitch_sStop;
			return true;
		}
		else if ((switchcmd == "Paused") || (switchcmd == "Pause"))
		{
			cmd = gswitch_sPause;
			return true;
		}
		else if ((switchcmd == "Playing") || (switchcmd == "Play"))
		{
			cmd = gswitch_sPlay;
			return true;
		}
		else if (switchcmd == "Play Playlist")
		{
			cmd = gswitch_sPlayPlaylist;
			return true;
		}
		else if (switchcmd == "Play Favorites")
		{
			cmd = gswitch_sPlayFavorites;
			return true;
		}
		else if (switchcmd == "Set Volume")
		{
			cmd = gswitch_sSetVolume;
			return true;
		}
		else if (switchcmd == "Execute")
		{
			cmd = gswitch_sExecute;
			return true;
		}
		else
			return false;
		break;
	case pTypeLighting4:
		cmd=light2_sOn;
		return true;
	case pTypeLighting5:
		if (dSubType==sTypeLivolo)
		{
			if (switchcmd=="Set Level")
			{
				cmd=light5_sLivoloGang2Toggle;
				return true;
			}
		}
		else if ((dSubType == sTypeTRC02) || (dSubType == sTypeTRC02_2))
		{
			if (switchcmd=="Set Color")
			{
				cmd=light5_sRGBcolormin+1; //color set is light5_sRGBcolormin+1 till 255?
				return true;
			}
		}
		else if ((dSubType != sTypeLightwaveRF) && (dSubType != sTypeIT))
		{
			//Only LightwaveRF/IT devices have a set-level
			if (switchcmd == "Set Level")
				switchcmd = "On";
		}
		else if ((dSubType == sTypeLightwaveRF) && (switchtype == STYPE_VenetianBlindsEU))
		{
			// The LightwaveRF inline relay has to be controlled by Venetian blinds logic as it has a stop setting
			if (switchcmd == "On")
				switchcmd = "Close inline relay";
			else if (switchcmd == "Off")
				switchcmd = "Open inline relay";
			else if (switchcmd == "Stop")
				switchcmd = "Stop inline relay";
		}

 		if (switchtype==STYPE_Doorbell)
 		{
			if ((switchcmd=="On")||(switchcmd=="Group On"))
			{
				cmd=light5_sGroupOn;
				return true;
			}
			//no other combinations for the door switch
			return false;
		}
		else if (switchtype==STYPE_X10Siren)
		{
			if ((switchcmd=="On")||(switchcmd=="Group On"))
			{
				cmd=light5_sGroupOn;
				return true;
			}
			else if ((switchcmd=="Off")||(switchcmd=="Group Off"))
			{
				cmd=light5_sGroupOff;
				return true;
			}
			return false;
		}
		if (switchcmd=="Off")
		{
			cmd=light5_sOff;
			return true;
		}
		else if (switchcmd=="On")
		{
			cmd=light5_sOn;
			return true;
		}
		else if (switchcmd=="Set Level")
		{
			cmd=light5_sSetLevel;
			return true;
		}
		else if (switchcmd=="Group Off")
		{
			cmd=light5_sGroupOff;
			return true;
		}
		else if (switchcmd=="Group On")
 		{
 			cmd=light5_sGroupOn;
			return true;
		}
		else if (switchcmd=="Close inline relay")
		{
			cmd=light5_sClose;
			return true;
		}
		else if (switchcmd=="Stop inline relay")
		{
			cmd=light5_sStop;
			return true;
		}
		else if (switchcmd=="Open inline relay")
		{
			cmd=light5_sOpen;
 			return true;
		}
		else if (switchcmd == "Group Mood 1")
		{
			cmd = light5_sMood1;
			return true;
		}
		else if (switchcmd == "Group Mood 2")
		{
			cmd = light5_sMood2;
			return true;
		}
		else if (switchcmd == "Group Mood 3")
		{
			cmd = light5_sMood3;
			return true;
		}
		else if (switchcmd == "Group Mood 4")
		{
			cmd = light5_sMood4;
			return true;
		}
		else if (switchcmd == "Group Mood 5")
		{
			cmd = light5_sMood5;
			return true;
		}
		else
			return false;
		break;
	case pTypeLighting6:
		if (switchcmd=="Off")
		{
			cmd=light6_sOff;
			return true;
		}
		else if (switchcmd=="On")
		{
			cmd=light6_sOn;
			return true;
		}
		else if (switchcmd=="Group Off")
		{
			cmd=light6_sGroupOff;
			return true;
		}
		else if (switchcmd=="Group On")
		{
			cmd=light6_sGroupOn;
			return true;
		}
		else
			return false;
		break;
	case pTypeHomeConfort:
		if (switchtype == STYPE_Doorbell)
		{
			if ((switchcmd == "On") || (switchcmd == "Group On"))
			{
				cmd = HomeConfort_sGroupOn;
				return true;
			}
			//no other combinations for the door switch
			return false;
		}
		if (switchcmd == "Off")
		{
			cmd = HomeConfort_sOff;
			return true;
		}
		else if (switchcmd == "On")
		{
			cmd = HomeConfort_sOn;
			return true;
		}
		else if (switchcmd == "Group Off")
		{
			cmd = HomeConfort_sGroupOff;
			return true;
		}
		else if (switchcmd == "Group On")
		{
			cmd = HomeConfort_sGroupOn;
			return true;
		}
		else
			return false;
		break;
	case pTypeGeneralSwitch:
		if (switchtype == STYPE_Doorbell)
		{
			if ((switchcmd == "On") || (switchcmd == "Group On"))
			{
				cmd = gswitch_sGroupOn;
				return true;
			}
			//no other combinations for the door switch
			return false;
		}
		else if (switchtype == STYPE_X10Siren)
		{
			if ((switchcmd == "On") || (switchcmd == "Group On"))
			{
				cmd = gswitch_sGroupOn;
				return true;
			}
			else if ((switchcmd == "Off") || (switchcmd == "Group Off"))
			{
				cmd = gswitch_sGroupOff;
				return true;
			}
			return false;
		}
		else if (switchtype == STYPE_Selector) {
			if ((switchcmd == "Paused") ||
				(switchcmd == "Pause") ||
				(switchcmd == "Playing") ||
				(switchcmd == "Play") ||
				(switchcmd == "Play Playlist") ||
				(switchcmd == "Play Favorites") ||
				(switchcmd == "Set Volume")) {
				// Not a managed command
				return false;
			}
			int level = GetSelectorSwitchLevel(options, switchcmd);
			if (level > 0) { // not Off but a level name
							 // switchcmd cannot be a level name
				return false;
			}
		}

		if (switchcmd == "Off")
		{
			cmd = gswitch_sOff;
			return true;
		}
		if (switchcmd == "On")
		{
			cmd = gswitch_sOn;
			return true;
		}
		if (switchcmd == "Set Level")
		{
			cmd = gswitch_sSetLevel;
			return true;
		}
		if (switchcmd == "Group Off")
		{
			cmd = gswitch_sGroupOff;
			return true;
		}
		if (switchcmd == "Group On")
		{
			cmd = gswitch_sGroupOn;
			return true;
		}
		if (switchcmd == "Set Group Level")
		{
			cmd = gswitch_sSetGroupLevel;
			return true;
		}
		if (switchcmd == "Dim")
		{
			cmd = gswitch_sDim;
			return true;
		}
		if (switchcmd == "Bright")
		{
			cmd = gswitch_sBright;
			return true;
		}
		if (switchcmd == "Sound 0")
		{
			cmd = gswitch_sSound0;
			return true;
		}
		if (switchcmd == "Sound 1")
		{
			cmd = gswitch_sSound1;
			return true;
		}
		if (switchcmd == "Sound 2")
		{
			cmd = gswitch_sSound2;
			return true;
		}
		if (switchcmd == "Sound 3")
		{
			cmd = gswitch_sSound3;
			return true;
		}
		if (switchcmd == "Sound 4")
		{
			cmd = gswitch_sSound4;
			return true;
		}
		if (switchcmd == "Sound 5")
		{
			cmd = gswitch_sSound5;
			return true;
		}
		if (switchcmd == "Sound 6")
		{
			cmd = gswitch_sSound6;
			return true;
		}
		if (switchcmd == "Sound 7")
		{
			cmd = gswitch_sSound7;
			return true;
		}
		if (switchcmd == "Sound 8")
		{
			cmd = gswitch_sSound8;
			return true;
		}
		if (switchcmd == "Stop")
		{
			cmd = gswitch_sStop;
			return true;
		}
		if (switchcmd == "Program")
		{
			cmd = gswitch_sProgram;
			return true;
		}
		break;
	case pTypeLimitlessLights:
		if (switchcmd=="Off")
		{
			cmd=Limitless_LedOff;
			return true;
		}
		else if (switchcmd=="On")
		{
			cmd=Limitless_LedOn;
			return true;
		}
		else if (switchcmd=="Set Color")
		{
			cmd=Limitless_SetRGBColour;
			return true;
		}
		else if (
			(switchcmd=="Set Brightness")||
			(switchcmd=="Set Level")
			)
		{
			cmd=Limitless_SetBrightnessLevel;
			return true;
		}
		else if (switchcmd=="Set Kelvin Level")
		{
			cmd=Limitless_SetKelvinLevel;
			return true;
		}
		else if (switchcmd == "Set White")
		{
			cmd = Limitless_SetColorToWhite;
			return true;
		}
		else if (switchcmd == "Set Full")
		{
			cmd=Limitless_SetColorToWhite;
			return true;
		}
		else if (switchcmd == "Set Night")
		{
			cmd = Limitless_NightMode;
			return true;
		}
		else if (switchcmd == "Bright Up")
		{
			cmd = Limitless_SetBrightUp;
			return true;
		}
		else if (switchcmd == "Bright Down")
		{
			cmd = Limitless_SetBrightDown;
			return true;
		}
		else if (switchcmd == "Disco Mode")
		{
			cmd = Limitless_DiscoMode;
			return true;
		}
		else if (switchcmd == "Disco Mode 1")
		{
			cmd = Limitless_DiscoMode_1;
			return true;
		}
		else if (switchcmd == "Disco Mode 2")
		{
			cmd = Limitless_DiscoMode_2;
			return true;
		}
		else if (switchcmd == "Disco Mode 3")
		{
			cmd = Limitless_DiscoMode_3;
			return true;
		}
		else if (switchcmd == "Disco Mode 4")
		{
			cmd = Limitless_DiscoMode_4;
			return true;
		}
		else if (switchcmd == "Disco Mode 5")
		{
			cmd = Limitless_DiscoMode_5;
			return true;
		}
		else if (switchcmd == "Disco Mode 6")
		{
			cmd = Limitless_DiscoMode_6;
			return true;
		}
		else if (switchcmd == "Disco Mode 7")
		{
			cmd = Limitless_DiscoMode_7;
			return true;
		}
		else if (switchcmd == "Disco Mode 8")
		{
			cmd = Limitless_DiscoMode_8;
			return true;
		}
		else if (switchcmd == "Disco Mode 9")
		{
			cmd = Limitless_DiscoMode_9;
			return true;
		}
		else if (switchcmd == "Disco Up")
		{
			cmd = Limitless_RGBDiscoNext;
			return true;
		}
		else if (switchcmd == "Disco Down")
		{
			cmd = Limitless_RGBDiscoPrevious;
			return true;
		}
		else if (switchcmd == "Speed Up")
		{
			cmd = Limitless_DiscoSpeedFaster;
			return true;
		}
		else if (switchcmd == "Speed Up Long")
		{
			cmd = Limitless_DiscoSpeedFasterLong;
			return true;
		}
		else if (switchcmd == "Speed Down")
		{
			cmd = Limitless_DiscoSpeedSlower;
			return true;
		}
		else if (switchcmd == "Speed Minimal")
		{
			cmd = Limitless_DiscoSpeedMinimal;
			return true;
		}
		else if (switchcmd == "Speed Maximal")
		{
			cmd = Limitless_DiscoSpeedMaximal;
			return true;
		}
		else if (switchcmd == "Warmer")
		{
			cmd = Limitless_WarmWhiteIncrease;
			return true;
		}
		else if (switchcmd == "Cooler")
		{
			cmd = Limitless_CoolWhiteIncrease;
			return true;
		}
		else
			return false;
		break;
	case pTypeSecurity1:
		if (
			(dSubType==sTypeKD101)||
			(dSubType==sTypeSA30)
			)
		{
			if ((switchcmd=="On")||(switchcmd=="All On"))
			{
				cmd=sStatusPanic;
				return true;
			}
			else if (switchcmd == "Off")
			{
				cmd = sStatusNormal;
				return true;
			}
		}
		else if (dSubType==sTypeSecX10M)
		{
			if (switchcmd=="Motion")
			{
				cmd=sStatusMotion;
				return true;
			}
			else if (switchcmd=="No Motion")
			{
				cmd=sStatusNoMotion;
				return true;
			}
		}
		else if ((dSubType==sTypeSecX10R)||(dSubType==sTypeMeiantech))
		{
			if (switchcmd=="On")
			{
				cmd=sStatusArmAway;
				return true;
			}
			else if (switchcmd=="Off")
			{
				cmd=sStatusDisarm;
				return true;
			}
			else if (switchcmd=="Arm Home")
			{
				cmd=sStatusArmHome;
				return true;
			}
			else if (switchcmd=="Arm Away")
			{
				cmd=sStatusArmAway;
				return true;
			}
			else if (switchcmd == "Panic")
			{
				cmd = sStatusPanic;
				return true;
			}
			else if (switchcmd == "Disarm")
			{
				cmd = sStatusDisarm;
				return true;
			}
		}
		else if (dSubType==sTypeSecX10)
		{
			if (switchcmd=="Normal")
			{
				cmd=sStatusNormal;
				return true;
			}
			else if (switchcmd=="Alarm")
			{
				cmd=sStatusAlarm;
				return true;
			}
			else if (switchcmd=="Normal Delayed")
			{
				cmd=sStatusNormalDelayed;
				return true;
			}
			else if (switchcmd=="Alarm Delayed")
			{
				cmd=sStatusAlarmDelayed;
				return true;
			}
			else if (switchcmd == "Arm Home")
			{
				cmd = sStatusArmHome;
				return true;
			}
			else if (switchcmd == "Arm Home Delayed")
			{
				cmd = sStatusArmHomeDelayed;
				return true;
			}
			else if (switchcmd == "Arm Away")
			{
				cmd = sStatusArmAway;
				return true;
			}
			else if (switchcmd == "Arm Away Delayed")
			{
				cmd = sStatusArmAwayDelayed;
				return true;
			}
			else if (switchcmd == "Panic")
			{
				cmd = sStatusPanic;
				return true;
			}
			else if (switchcmd == "Disarm")
			{
				cmd = sStatusDisarm;
				return true;
			}
			else if (switchcmd == "Light On")
			{
				cmd = sStatusLightOn;
				return true;
			}
			else if (switchcmd == "Light Off")
			{
				cmd = sStatusLightOff;
				return true;
			}
			else if (switchcmd == "Light 2 On")
			{
				cmd = sStatusLight2On;
				return true;
			}
			else if (switchcmd == "Light 2 Off")
			{
				cmd = sStatusLight2Off;
				return true;
			}
		}
		break;
	case pTypeSecurity2:
		cmd = 0;
		return true;
		break;
	case pTypeCurtain:
		{
			if (switchcmd=="On")
			{
				cmd=curtain_sClose;
			}
			else if (switchcmd=="Off")
			{
				cmd=curtain_sOpen;
			}
			else
			{
				cmd=curtain_sStop;
			}
			return true;
		}
		break;
	case pTypeBlinds:
		{
			if (switchcmd=="On")
			{
				if (dSubType == sTypeBlindsT10)
					cmd = blinds_sOpen;
				else
					cmd = blinds_sClose;
			}
			else if (switchcmd=="Off")
			{
				if (dSubType == sTypeBlindsT10)
					cmd = blinds_sClose;
				else
					cmd = blinds_sOpen;
			}
			else
			{
				cmd=blinds_sStop;
			}
			return true;
		}
		break;
	case pTypeRFY:
		{
			if (switchtype == STYPE_OnOff)
			{
				/*
				Venetian Blind in US mode:
				-up / down(transmit < 0.5 seconds) : open or close
				-up / down(transmit > 2seconds) : change angle

				Venetian Blind in Europe mode :
				-up / down(transmit < 0.5 seconds) : change angle
				-up / down(transmit > 2seconds) : open or close
				*/
				if (switchcmd == "On")
				{
					cmd = rfy_sUp;
				}
				else if (switchcmd == "Off")
				{
					cmd = rfy_sDown;
				}
			}
			else
			{
				/*
				Venetian Blind in US mode:
				-up / down(transmit < 0.5 seconds) : open or close
				-up / down(transmit > 2seconds) : change angle

				Venetian Blind in Europe mode :
				-up / down(transmit < 0.5 seconds) : change angle
				-up / down(transmit > 2seconds) : open or close
				*/
				if (switchcmd == "On")
				{
					if (switchtype == STYPE_VenetianBlindsUS)
					{
						cmd = rfy_s05SecDown;
					}
					else if (switchtype == STYPE_VenetianBlindsEU)
					{
						cmd = rfy_s2SecDown;
					}
					else
					{
						cmd = rfy_sDown;
					}
				}
				else if (switchcmd == "Off")
				{
					if (switchtype == STYPE_VenetianBlindsUS)
					{
						cmd = rfy_s05SecUp;
					}
					else if (switchtype == STYPE_VenetianBlindsEU)
					{
						cmd = rfy_s2SecUp;
					}
					else
					{
						cmd = rfy_sUp;
					}
				}
				else if (switchcmd == "Stop")
				{
					cmd = rfy_sStop;
				}
				else if (switchcmd == "Up")
				{
					cmd = rfy_sUp;
				}
				else if (switchcmd == "UpStop")
				{
					cmd = rfy_sUpStop;
				}
				else if (switchcmd == "Down")
				{
					cmd = rfy_sDown;
				}
				else if (switchcmd == "DownStop")
				{
					cmd = rfy_sDownStop;
				}
				else if (switchcmd == "UpDown")
				{
					cmd = rfy_sUpDown;
				}
				else if (switchcmd == "ListRemotes")
				{
					cmd = rfy_sListRemotes;
				}
				else if (switchcmd == "Program")
				{
					cmd = rfy_sProgram;
				}
				else if (switchcmd == "Program2Seconds")
				{
					cmd = rfy_s2SecProgram;
				}
				else if (switchcmd == "Program7Seconds")
				{
					cmd = rfy_s7SecProgram;
				}
				else if (switchcmd == "Stop2Seconds")
				{
					cmd = rfy_s2SecStop;
				}
				else if (switchcmd == "Stop5Seconds")
				{
					cmd = rfy_s5SecStop;
				}
				else if (switchcmd == "UpDown5Seconds")
				{
					cmd = rfy_s5SecUpDown;
				}
				else if (switchcmd == "EraseThis") //from the RFXtrx
				{
					cmd = rfy_sEraseThis;
				}
				else if (switchcmd == "EraseAll") //from the RFXtrx
				{
					cmd = rfy_sEraseAll;
				}
				else if (switchcmd == "Up05Seconds")
				{
					cmd = rfy_s05SecUp;
				}
				else if (switchcmd == "Down05Seconds")
				{
					cmd = rfy_s05SecDown;
				}
				else if (switchcmd == "Up2Seconds")
				{
					cmd = rfy_s2SecUp;
				}
				else if (switchcmd == "Down2Seconds")
				{
					cmd = rfy_s2SecDown;
				}
				else if (switchcmd == "EnableSunWind")
				{
					cmd = rfy_sEnableSunWind;
				}
				else if (switchcmd == "DisableSunWind")
				{
					cmd = rfy_sDisableSun;
				}
				else
				{
					cmd = rfy_sStop;
				}
			}
			return true;
		}
		break;
	case pTypeRemote:
		cmd=light2_sOn;
		break;
	case pTypeThermostat2:
		{
			if (switchcmd == "On")
			{
				cmd = thermostat2_sOn;
			}
			else if (switchcmd == "Off")
			{
				cmd = thermostat2_sOff;
			}
			else
			{
				cmd = thermostat2_sOff;
			}
			return true;
		}
		break;
	case pTypeThermostat3:
		{
			if (switchcmd=="On")
			{
				cmd=thermostat3_sOn;
			}
			else if (switchcmd=="Off")
			{
				cmd=thermostat3_sOff;
			}
			else if (switchcmd == "Up")
			{
				cmd = thermostat3_sUp;
			}
			else if (switchcmd == "Down")
			{
				cmd = thermostat3_sDown;
			}
			else if (switchcmd == "Run Up")
			{
				cmd = thermostat3_sRunUp;
			}
			else if (switchcmd == "Run Down")
			{
				cmd = thermostat3_sRunDown;
			}
			else if (switchcmd == "Stop")
			{
				cmd = thermostat3_sStop;
			}
			else
			{
				cmd=thermostat3_sOff;
			}
			return true;
		}
		break;
	case pTypeThermostat4:
	{
		if (switchcmd == "Off")
		{
			cmd = thermostat4_sOff;
		}
		else if (switchcmd == "Manual")
		{
			cmd = thermostat4_sManual;
		}
		else if (switchcmd == "Auto")
		{
			cmd = thermostat4_sAuto;
		}
		else if (switchcmd == "Eco")
		{
			cmd = thermostat4_sEco;
		}
		else
		{
			cmd = thermostat3_sOff;
		}
		return true;
	}
	break;
	case pTypeRadiator1:
	{
		if (switchcmd == "On")
		{
			cmd = Radiator1_sDay;
		}
		else if (switchcmd == "Off")
		{
			cmd = Radiator1_sNight;
		}
		else
		{
			cmd = Radiator1_sNight;
		}
		return true;
	}
	case pTypeFan:
	{
		switch (dSubType)
		{
			case sTypeSiemensSF01:
			{
				if (switchcmd == "timer")
				{
					cmd = fan_sTimer;
				}
				else if (switchcmd == "+")
				{
					cmd = fan_sPlus;
				}
				else if (switchcmd == "-")
				{
					cmd = fan_sMin;
				}
				else if (switchcmd == "learn")
				{
					cmd = fan_sLearn;
				}
				else if (switchcmd == "confirm")
				{
					cmd = fan_sConfirm;
				}
				else if (switchcmd == "light")
				{
					cmd = fan_sLight;
				}
			}
			break;
			case sTypeItho:
			{
				if ((switchcmd == "1") || (switchcmd == "On") || (switchcmd == "Off"))
				{
					cmd = fan_Itho1;
				}
				else if (switchcmd == "2")
				{
					cmd = fan_Itho2;
				}
				else if (switchcmd == "3")
				{
					cmd = fan_Itho3;
				}
				else if (switchcmd == "timer")
				{
					cmd = fan_IthoTimer;
				}
				else if (switchcmd == "notathome")
				{
					cmd = fan_IthoNotAtHome;
				}
				else if (switchcmd == "learn")
				{
					cmd = fan_IthoLearn;
				}
				else if (switchcmd == "eraseall")
				{
					cmd = fan_IthoEraseAll;
				}
			}
			break;
			case sTypeLucciAir:
			{
				if (switchcmd == "hi")
				{
					cmd = fan_LucciHi;
				}
				else if (switchcmd == "med")
				{
					cmd = fan_LucciMed;
				}
				else if (switchcmd == "low")
				{
					cmd = fan_LucciLow;
				}
				else if (switchcmd == "off")
				{
					cmd = fan_LucciOff;
				}
				else if (switchcmd == "light")
				{
					cmd = fan_LucciLight;
				}
			}
			break;
		}
		return true;
	}
	break;
	}
	//unknown command
	return false;
}

bool IsLightSwitchOn(const std::string &lstatus)
{
	return (
		(lstatus=="On")||
		(lstatus=="Group On")||
		(lstatus=="All On")||
		(lstatus=="Chime")||
		(lstatus=="Motion")||
		(lstatus=="Alarm")||
		(lstatus=="Panic")||
		(lstatus=="Light On")||
		(lstatus=="Light 2 On")||
		(lstatus=="Open inline relay")||
		(lstatus.find("Set Level")!=std::string::npos)||
		(lstatus.find("Set Group Level")!=std::string::npos)||
		(lstatus == "1") || //fan itho
		(lstatus == "2") ||
		(lstatus == "3")
		);
}

const char *Get_Moisture_Desc(const int moisture)
{
		if (moisture<10)
			return "saturated";
		else if (moisture<20)
			return "adequately wet";
		else if (moisture<60)
			return "irrigation advise";
		else if (moisture<100)
			return "irrigation";
		else
			return "dangerously dry";
}

const char *Get_Alert_Desc(const int level)
{
		if (level == 0)
			return "undefined";
		else if (level == 1)
			return "normal";
		else if (level == 2)
			return "warning";
		else if (level == 3)
			return "alert";
		else if (level == 4)
			return "alarm";
		else
			return "unknown level";
}

bool IsSerialDevice(const _eHardwareTypes htype)
{
	switch (htype) {
	case HTYPE_RFXtrx315:
	case HTYPE_RFXtrx433:
	case HTYPE_RFXtrx868:
	case HTYPE_P1SmartMeter:
	case HTYPE_Rego6XX:
	case HTYPE_DavisVantage:
	case HTYPE_S0SmartMeterUSB:
	case HTYPE_OpenThermGateway:
	case HTYPE_TeleinfoMeter:
	case HTYPE_OpenZWave:
	case HTYPE_EnOceanESP2:
	case HTYPE_EnOceanESP3:
	case HTYPE_Meteostick:
	case HTYPE_MySensorsUSB:
	case HTYPE_RFLINKUSB:
	case HTYPE_ZIBLUEUSB:
	case HTYPE_KMTronicUSB:
	case HTYPE_KMTronic433:
	case HTYPE_CurrentCostMeter:
	case HTYPE_RAVEn:
	case HTYPE_Comm5Serial:
		return true;
	default:
		return false;
	}
}

void ConvertToGeneralSwitchType(std::string &devid, int &dtype, int &subtype)
{
	if (dtype == pTypeLighting1) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeIMPULS) subtype = sSwitchTypeTriState;
		if (subtype == sTypeAB400D) subtype = sSwitchTypeAB400D;
		if (subtype == sTypeIMPULS) subtype = sSwitchTypeTriState;
		std::stringstream s_strid;
		s_strid << std::hex << atoi(devid.c_str());
		devid = s_strid.str();
		devid = "000000" + devid;
	}
	else if (dtype == pTypeLighting2) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeAC) subtype = sSwitchTypeAC;
		if (subtype == sTypeHEU) { subtype = sSwitchTypeHEU; devid = "7" + devid; }
		if (subtype == sTypeKambrook) subtype = sSwitchTypeKambrook;
		devid = "0" + devid;
	}
	else if (dtype == pTypeLighting3) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeKoppla) subtype = sSwitchTypeKoppla;
	}
	else if (dtype == pTypeLighting4) {
		dtype = pTypeGeneralSwitch;
		subtype = sSwitchTypeTriState;
	}
	else if (dtype == pTypeLighting5) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeEMW100) { subtype = sSwitchTypeEMW100; devid = "00" + devid; }
		if (subtype == sTypeLivolo) { subtype = sSwitchTypeLivolo; devid = "00" + devid; }
		if (subtype == sTypeLightwaveRF) { subtype = sSwitchTypeLightwaveRF; devid = "00" + devid; }
		if (subtype == sTypeLivoloAppliance) { subtype = sSwitchTypeLivoloAppliance; devid = "00" + devid; }
		if (subtype == sTypeEurodomest) subtype = sSwitchTypeEurodomest;
	}
	else if (dtype == pTypeLighting6) {
		dtype = pTypeGeneralSwitch;
		subtype = sSwitchTypeBlyss;
	}
	else if (dtype == pTypeChime) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeByronSX) subtype = sSwitchTypeByronSX;
		if (subtype == sTypeSelectPlus) subtype = sSwitchTypeSelectPlus;
		if (subtype == sTypeSelectPlus3) subtype = sSwitchTypeSelectPlus3;
		if (subtype == sTypeByronMP001) subtype = sSwitchTypeByronMP001;
	}
	else if (dtype == pTypeSecurity1) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeSecX10) subtype = sSwitchTypeX10secu;
		if (subtype == sTypeSecX10M) subtype = sSwitchTypeX10secu;
		if (subtype == sTypeSecX10R) subtype = sSwitchTypeX10secu;
	}
	else if (dtype == pTypeHomeConfort) {
		dtype = pTypeGeneralSwitch;
		subtype = sSwitchTypeHomeConfort;
	}
	else if (dtype == pTypeBlinds) {
		dtype = pTypeGeneralSwitch;
		if (subtype == sTypeBlindsT5) subtype = sSwitchTypeBofu;
		else if (subtype == sTypeBlindsT6) subtype = sSwitchTypeBrel;
		else if (subtype == sTypeBlindsT7) subtype = sSwitchTypeDooya;
		else if (subtype == sTypeBlindsT8) subtype = sSwitchTypeBofu;
		else if (subtype == sTypeBlindsT9) subtype = sSwitchTypeBrel;
		else if (subtype == sTypeBlindsT10) subtype = sSwitchTypeDooya;
		std::stringstream s_strid;
		s_strid << std::hex << strtoul(devid.c_str(), NULL, 16);
		unsigned long deviceid = 0;
		s_strid >> deviceid;
		deviceid = (unsigned long)((deviceid & 0xffffff00) >> 8);
		char szTmp[20];
		sprintf(szTmp, "%lx", deviceid);
		//_log.Log(LOG_ERROR, "RFLink: deviceid: %x", deviceid);
		devid = szTmp;
	}
	else if (dtype == pTypeRFY) {
		dtype = pTypeGeneralSwitch;
		subtype = sSwitchTypeRTS;
	}
}
