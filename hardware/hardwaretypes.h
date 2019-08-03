#pragma once

#include "../cereal/cereal.hpp"
//#include "../cereal/types/string.hpp"
//#include "../cereal/types/utility.hpp"
//#include "../cereal/types/memory.hpp"
//#include "../cereal/types/complex.hpp"
//#include "../cereal/types/base_class.hpp"
//#include "../cereal/types/array.hpp"
//#include "../cereal/types/vector.hpp"
//#include "../cereal/types/map.hpp"

#include "ColorSwitch.h"

#define sTypeDomoticzSecurity 0x83
#define sTypeSmartwaresSwitchRadiator 0x84

#define sTypeRAINWU 0x70	 //Weather Underground (Total rain reported, no counter)
#define sTypeRAINByRate 0x71	 //DarkSky for example (Only rate, no total, no counter) rate in mm/hour x 10000, so all decimals will fit

#define sTypeTHBFloat 0x10   //Weather Station
#define sTypeWINDNoTemp 0x30    //Weather Station
#define sTypeWINDNoTempNoChill 0x31    //Weather Station

#define wsbaroforcast_heavy_snow 0x01
#define wsbaroforcast_snow 0x01
#define wsbaroforcast_heavy_rain 0x02
#define wsbaroforcast_rain 0x03
#define wsbaroforcast_cloudy 0x04
#define wsbaroforcast_some_clouds 0x05
#define wsbaroforcast_sunny 0x06
#define wsbaroforcast_unknown 0x07
#define wsbaroforcast_unstable 0x08
#define wsbaroforcast_stable 0x09

#define bmpbaroforecast_stable			0x00
#define bmpbaroforecast_sunny			0x01
#define bmpbaroforecast_cloudy			0x02
#define bmpbaroforecast_unstable		0x03
#define bmpbaroforecast_thunderstorm	0x04
#define bmpbaroforecast_unknown			0x05
#define bmpbaroforecast_rain			0x06 //when forecast was cloudy and pressure is below 1010 we have 50%+ change of rain

#define pTypeThermostat			0xF2
#define sTypeThermSetpoint		0x01
#define sTypeThermTemperature	0x02


#define pTypeGeneral				0xF3
#define sTypeVisibility				0x01
#define sTypeSolarRadiation			0x02
#define sTypeSoilMoisture			0x03
#define sTypeLeafWetness			0x04
#define sTypeSystemTemp				0x05
#define sTypePercentage				0x06
#define sTypeFan					0x07
#define sTypeVoltage				0x08
#define sTypePressure				0x09
#define sTypeSetPoint				0x10
#define sTypeTemperature			0x11
#define sTypeZWaveClock				0x12
#define sTypeTextStatus				0x13
#define sTypeZWaveThermostatMode	0x14
#define sTypeZWaveThermostatFanMode	0x15
#define sTypeAlert					0x16
#define sTypeCurrent				0x17
#define sTypeSoundLevel				0x18
#define sTypeUV						0x19
#define sTypeBaro					0x1A
#define sTypeDistance				0x1B
#define sTypeCounterIncremental		0x1C
#define sTypeKwh					0x1D
#define sTypeWaterflow				0x1E
#define sTypeCustom					0x1F
#define sTypeZWaveAlarm				0x20
#define sTypeManagedCounter			0x21

//General Switch
#define pTypeGeneralSwitch			0xF4
#define sSwitchTypeX10				0x00
#define sSwitchTypeARC				0x01
#define sSwitchTypeAB400D			0x02
#define sSwitchTypeWaveman			0x03
#define sSwitchTypeEMW200			0x04
#define sSwitchTypeIMPULS			0x05
#define sSwitchTypeRisingSun		0x06
#define sSwitchTypePhilips			0x07
#define sSwitchTypeEnergenie		0x08
#define sSwitchTypeEnergenie5		0x09
#define sSwitchTypeGDR2				0x0A
#define sSwitchTypeAC				0x0B
#define sSwitchTypeHEU				0x0C
#define sSwitchTypeANSLUT			0x0D
#define sSwitchTypeKambrook			0x0E
#define sSwitchTypeKoppla			0x0F
#define sSwitchTypePT2262			0x10
#define sSwitchTypeLightwaveRF		0x11
#define sSwitchTypeEMW100			0x12
#define sSwitchTypeBBSB				0x13
#define sSwitchTypeMDREMOTE			0x14
#define sSwitchTypeRSL				0x15
#define sSwitchTypeLivolo			0x16
#define sSwitchTypeTRC02			0x17
#define sSwitchTypeAoke				0x18
#define sSwitchTypeTRC02_2			0x19
#define sSwitchTypeEurodomest		0x1A
#define sSwitchTypeLivoloAppliance	0x1B
#define sSwitchTypeBlyss			0x1C
#define sSwitchTypeByronSX			0x1D
#define sSwitchTypeByronMP001		0x1E
#define sSwitchTypeSelectPlus		0x1F
#define sSwitchTypeSelectPlus3		0x20
#define sSwitchTypeFA20				0x21
#define sSwitchTypeChuango			0x22
#define sSwitchTypePlieger			0x23
#define sSwitchTypeSilvercrest		0x24
#define sSwitchTypeMertik			0x25
#define sSwitchTypeHomeConfort		0x26
#define sSwitchTypePowerfix			0x27
#define sSwitchTypeTriState			0x28
#define sSwitchTypeDeltronic		0x29
#define sSwitchTypeFA500			0x30
#define sSwitchTypeHT12E			0x31
#define sSwitchTypeEV1527			0x32
#define sSwitchTypeElmes			0x33
#define sSwitchTypeAster			0x34
#define sSwitchTypeSartano			0x35
#define sSwitchTypeEurope			0x36
#define sSwitchTypeAvidsen			0x37
#define sSwitchTypeBofu				0x38
#define sSwitchTypeBrel				0x39
#define sSwitchTypeRTS   			0x3a
#define sSwitchTypeElroDB			0x3b
#define sSwitchTypeDooya			0x3c
#define sSwitchTypeUnitec			0x3d
#define sSwitchTypeSelector			0x3e
#define sSwitchTypeMaclean			0x3f
#define sSwitchTypeR546				0x40
#define sSwitchTypeDiya				0x41
#define sSwitchTypeX10secu			0x42
#define sSwitchTypeAtlantic			0x43
#define sSwitchTypeSilvercrestDB	0x44
#define sSwitchTypeMedionDB			0x45
#define sSwitchTypeVMC				0x46
#define sSwitchTypeKeeloq			0x47
#define sSwitchCustomSwitch			0x48
#define sSwitchGeneralSwitch		0x49
#define sSwitchTypeKoch				0x4a
#define sSwitchTypeKingpin			0x4b
#define sSwitchTypeFunkbus			0x4c
#define sSwitchTypeNice				0x4d
#define sSwitchTypeForest			0x4e
#define sSwitchBlindsT1				0x4f
#define sSwitchMC145026				0x50
#define sSwitchLobeco				0x51
#define sSwitchFriedland			0x52
#define sSwitchBFT					0x53
#define sSwitchNovatys				0x54
#define sSwitchHalemeier			0x55
#define sSwitchGaposa				0x56
#define sSwitchMiLightv1			0x57
#define sSwitchMiLightv2			0x58
#define sSwitchHT6P20				0x59
#define sSwitchTypeDoitrand			0x5a
#define sSwitchTypeWarema			0x5b
#define sSwitchTypeAnsluta			0x5c
#define sSwitchTypeLivcol			0x5d
#define sSwitchTypeBosch			0x5e
#define sSwitchTypeNingbo			0x5f
#define sSwitchTypeDitec			0x60
#define sSwitchTypeSteffen			0x61
#define sSwitchTypeAlectoSA			0x62
#define sSwitchTypeGPIOset			0x63
#define sSwitchLightT1				0x64
#define sSwitchTypeKonigSec			0x65
#define sSwitchTypeRM174RF			0x66
#define sSwitchTypeLiwin			0x67
#define sSwitchAuxiliaryT1			0x68
#define sSwitchBlindsT2				0x69
#define sSwitchLightT2				0x70
#define sSwitchContactT1			0x71
#define sSwitchTypeYW_Secu			0x6a
#define sSwitchTypeMertik_GV60		0x6b
#define sSwitchTypeNingbo64			0x6c
#define sSwitchTypeX2D				0x6d
#define sSwitchTypeHRCMotor			0x6e
#define sSwitchTypeVelleman			0x6f
#define sSwitchTypeRFCustom			0x72
#define sSwitchTypeYW_Sensor		0x73
#define sSwitchTypeLegrandcad		0x74
#define sSwitchTypeSysfsGpio		0x75
#define sSwitchTypeHager			0x76
#define sSwitchTypeFaber			0x77
#define sSwitchTypeDrayton			0x78
#define sSwitchTypeV2Phoenix		0x79

//Switch commands
#define gswitch_sOff				0x00
#define gswitch_sOn					0x01
#define gswitch_sSetLevel			0x02
#define gswitch_sGroupOff			0x03
#define gswitch_sGroupOn			0x04
#define gswitch_sSetGroupLevel		0x05
#define gswitch_sDim				0x06
#define gswitch_sBright				0x07
#define gswitch_sSound0				0x08
#define gswitch_sSound1				0x09
#define gswitch_sSound2				0x0A
#define gswitch_sSound3				0x0B
#define gswitch_sSound4				0x0C
#define gswitch_sSound5				0x0D
#define gswitch_sSound6				0x0E
#define gswitch_sSound7				0x0F
#define gswitch_sSound8				0x10
#define gswitch_sStop				0x11
#define gswitch_sProgram			0x12
#define gswitch_sPause				0x13
#define gswitch_sPlay				0x14
#define gswitch_sSetVolume			0x15
#define gswitch_sPlayPlaylist		0x16
#define gswitch_sPlayFavorites		0x17
#define gswitch_sExecute			0x18
#define gswitch_sColor				0x19
#define gswitch_sDiscop				0x1a
#define gswitch_sDiscom				0x1b

//--------------

#define pTypeLux		0xF6
#define sTypeLux		0x01

#define pTypeTEMP_BARO	0xF7
#define sTypeBMP085		0x01

#define pTypeUsage		0xF8
#define sTypeElectric	0x01

#define pTypeAirQuality	0xF9
#define sTypeVoltcraft	0x01

#define pTypeP1Power	0xFA
#define sTypeP1Power	0x01
#define mModeP1Norm		0x00
#define mModeP1Double	0x01

#define pTypeP1Gas		0xFB
#define sTypeP1Gas		0x02

#define pTypeYouLess	0xFC
#define sTypeYouLess	0x01

#define pTypeRego6XXTemp    0xFD
#define sTypeRego6XXTemp    0x01

#define pTypeRego6XXValue    0xFE
#define sTypeRego6XXStatus   0x02
#define sTypeRego6XXCounter  0x03

//RFY2 (protocol v2)
#define sTypeRFY2			0xFE

//types for evohome
#define pTypeEvohome 0x45
#define sTypeEvohome 0x00 //Controller

#define pTypeEvohomeZone 0x46 //Seems easier to define a new type here
#define sTypeEvohomeZone 0x00 //Actual temp zone

#define pTypeEvohomeWater 0x47 //Seems easier to define a new type here
#define sTypeEvohomeWater 0x00 //Hot water (Ideally this would just be a zone but for whatever reason evohome treats this differently)

#define pTypeEvohomeRelay 0x44 //Relay
#define sTypeEvohomeRelay 0x00

//#define sTypeEvohomeOutside 0x30 //If connected
//#define sTypeEvohomeStatus 0x40 //Not sure if we can do this in 1 sensor would be for things like zone valve status, boiler relay status (maybe OT values too) and comms errors (maybe seperature sensor or switch for each is easiest)

typedef struct _tThermostat {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	uint8_t	id3;
	uint8_t	id4;
	uint8_t dunit;
	uint8_t battery_level;
	float temp;
	float temp1;
	float temp2;
	float temp3;
	uint8_t utemp1;
	uint8_t utemp2;
	uint8_t utemp3;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("dunit", dunit);
		ar & cereal::make_nvp("battery_level", battery_level);
		ar & cereal::make_nvp("temp", temp);
		ar & cereal::make_nvp("temp1", temp1);
		ar & cereal::make_nvp("temp2", temp2);
		ar & cereal::make_nvp("temp3", temp3);
		ar & cereal::make_nvp("utemp1", utemp1);
		ar & cereal::make_nvp("utemp2", utemp2);
		ar & cereal::make_nvp("utemp3", utemp3);
	}

	_tThermostat()
	{
		len = sizeof(_tThermostat) - 1;
		type = pTypeThermostat;
		subtype = sTypeThermTemperature;
		battery_level = 255;
		id1 = 1;
		id2 = 0;
		id3 = 0;
		id4 = 0;
		dunit = 0;
		temp = 0;
		temp1 = 0;
		temp2 = 0;
		temp3 = 0;
		utemp1 = 0;
		utemp2 = 0;
		utemp3 = 0;
	}
} tThermostat;

typedef struct _tTempBaro {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	float temp;
	float baro;
	float altitude;
	uint8_t forecast;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("temp", temp);
		ar & cereal::make_nvp("baro", baro);
		ar & cereal::make_nvp("altitude", altitude);
		ar & cereal::make_nvp("forecast", forecast);
	}

	_tTempBaro()
	{
		len = sizeof(_tTempBaro) - 1;
		type = pTypeTEMP_BARO;
		subtype = sTypeBMP085;
		id1 = 1;
		temp = 0;
		baro = 0;
		altitude = 0;
		forecast = 0;
	}
} _tTempBaro;

typedef struct _tAirQualityMeter {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	int32_t airquality;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("airquality", airquality);
	}

	_tAirQualityMeter()
	{
		len = sizeof(_tAirQualityMeter) - 1;
		type = pTypeAirQuality;
		subtype = sTypeVoltcraft;
		id1 = 0;
		id2 = 1;
		airquality = 0;
	}
} AirQualityMeter;

typedef struct _tUsageMeter {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	uint8_t	id3;
	uint8_t	id4;
	uint8_t dunit;
	float fusage;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("dunit", dunit);
		ar & cereal::make_nvp("fusage", fusage);
	}

	_tUsageMeter()
	{
		len = sizeof(_tUsageMeter) - 1;
		type = pTypeUsage;
		subtype = sTypeElectric;
		id1 = 0;
		id2 = 0;
		id3 = 0;
		id4 = 0;
		dunit = 0;
		fusage = 0;
	}
} UsageMeter;

typedef struct _tLightMeter {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	uint8_t	id3;
	uint8_t	id4;
	uint8_t dunit;
	uint8_t battery_level;
	float fLux;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("id4", id4);
		ar & cereal::make_nvp("dunit", dunit);
		ar & cereal::make_nvp("battery_level", battery_level);
		ar & cereal::make_nvp("fLux", fLux);
	}

	_tLightMeter()
	{
		len = sizeof(_tLightMeter) - 1;
		type = pTypeLux;
		subtype = sTypeLux;
		id1 = 0;
		id2 = 0;
		id3 = 0;
		id4 = 0;
		battery_level = 255;
		dunit = 0;
		fLux = 0;
	}
} LightMeter;

typedef struct _tGeneralDevice {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t id;
	float floatval1;
	float floatval2;
	int32_t intval1;
	int32_t intval2;
	char text[64];

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id", id);
		ar & cereal::make_nvp("floatval1", floatval1);
		ar & cereal::make_nvp("floatval2", floatval2);
		ar & cereal::make_nvp("intval1", intval1);
		ar & cereal::make_nvp("intval2", intval2);
		ar & cereal::make_nvp("text", text);
	}

	_tGeneralDevice()
	{
		len = sizeof(_tGeneralDevice) - 1;
		type = pTypeGeneral;
		subtype = sTypeVisibility;
		id = 0;
		floatval1 = 0;
		floatval2 = 0;
		intval1 = 0;
		intval2 = 0;
		text[0] = 0;
	}
} GeneralDevice;

typedef struct _tGeneralSwitch {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	int32_t id;
	int32_t unitcode;
	uint8_t cmnd;
	uint8_t level;
	uint8_t rssi;
	uint8_t battery_level;
	uint8_t seqnbr;
	int32_t reserved_int;
	float	reserved_float;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id", id);
		ar & cereal::make_nvp("unitcode", unitcode);
		ar & cereal::make_nvp("cmnd", cmnd);
		ar & cereal::make_nvp("level", level);
		ar & cereal::make_nvp("rssi", rssi);
		ar & cereal::make_nvp("battery_level", battery_level);
		ar & cereal::make_nvp("seqnbr", seqnbr);
		ar & cereal::make_nvp("reserved_int", reserved_int);
		ar & cereal::make_nvp("reserved_float", reserved_float);
	}

	_tGeneralSwitch()
	{
		len = sizeof(_tGeneralSwitch) - 1;
		type = pTypeGeneralSwitch;
		subtype = sSwitchTypeAC;
		id = 1;
		unitcode = 1;
		seqnbr = 0;
		reserved_int = 0;
		reserved_float = 0;
		level = 0;
		battery_level = 255;
		rssi = 12;
		cmnd = 0;
	}
} GeneralSwitch;

typedef struct _tP1Power {
	uint8_t len = sizeof(_tP1Power) - 1;
	uint8_t type = pTypeP1Power;
	uint8_t subtype = sTypeP1Power;
	uint32_t powerusage1 = 0;
	uint32_t powerusage2 = 0;
	uint32_t powerdeliv1 = 0;
	uint32_t powerdeliv2 = 0;
	uint32_t usagecurrent = 0;
	uint32_t delivcurrent = 0;
	int32_t ID = 1;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("powerusage1", powerusage1);
		ar & cereal::make_nvp("powerusage2", powerusage2);
		ar & cereal::make_nvp("powerdeliv1", powerdeliv1);
		ar & cereal::make_nvp("powerdeliv2", powerdeliv2);
		ar & cereal::make_nvp("usagecurrent", usagecurrent);
		ar & cereal::make_nvp("delivcurrent", delivcurrent);
		ar & cereal::make_nvp("ID", ID);
	}
} P1Power;

typedef struct _tP1Gas {
	uint8_t len = sizeof(_tP1Gas) - 1;
	uint8_t type = pTypeP1Gas;
	uint8_t subtype = sTypeP1Gas;
	uint32_t gasusage = 0;
	int32_t ID = 1;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("gasusage", gasusage);
		ar & cereal::make_nvp("ID", ID);
	}
} P1Gas;

typedef struct _tEVOHOME1 {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	uint8_t	id3;
	uint8_t	status;
	uint8_t	mode;
	uint16_t year;
	uint8_t	month;
	uint8_t	day;
	uint8_t	hrs;
	uint8_t	mins;
	uint8_t	action;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("status", status);
		ar & cereal::make_nvp("mode", mode);
		ar & cereal::make_nvp("year", year);
		ar & cereal::make_nvp("month", month);
		ar & cereal::make_nvp("day", day);
		ar & cereal::make_nvp("hrs", hrs);
		ar & cereal::make_nvp("mins", mins);
		ar & cereal::make_nvp("action", action);
	}
} EVOHOME1;

typedef struct _tEVOHOME2 {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	uint8_t	id3;
	uint8_t	zone;
	uint8_t	updatetype;
	int16_t	temperature;
	uint8_t	mode;
	uint8_t	controllermode;
	uint16_t year;
	uint8_t	month;
	uint8_t	day;
	uint8_t	hrs;
	uint8_t	mins;
	uint8_t	battery_level;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("zone", zone);
		ar & cereal::make_nvp("updatetype", updatetype);
		ar & cereal::make_nvp("temperature", temperature);
		ar & cereal::make_nvp("mode", mode);
		ar & cereal::make_nvp("controllermode", controllermode);
		ar & cereal::make_nvp("year", year);
		ar & cereal::make_nvp("month", month);
		ar & cereal::make_nvp("day", day);
		ar & cereal::make_nvp("hrs", hrs);
		ar & cereal::make_nvp("mins", mins);
		ar & cereal::make_nvp("battery_level", battery_level);
	}
} EVOHOME2;

typedef struct _tEVOHOME3 {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	uint8_t	id3;
	uint8_t	devno;
	uint8_t	demand;
	uint8_t	updatetype;
	uint8_t	battery_level;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id1", id1);
		ar & cereal::make_nvp("id2", id2);
		ar & cereal::make_nvp("id3", id3);
		ar & cereal::make_nvp("devno", devno);
		ar & cereal::make_nvp("demand", demand);
		ar & cereal::make_nvp("updatetype", updatetype);
		ar & cereal::make_nvp("battery_level", battery_level);
	}
} EVOHOME3;
