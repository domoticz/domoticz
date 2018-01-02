#pragma once

#define sTypeDomoticzSecurity 0x83
#define sTypeSmartwaresSwitchRadiator 0x84

#define sTypeRAINWU 0x70	 //Weather Underground (Total rain reported, no counter)

#define sTypeTHBFloat 0x10   //Weather Station
#define sTypeWINDNoTemp 0x30    //Weather Station

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

#define pTypeLimitlessLights	0xF1
#define sTypeLimitlessRGBW		0x01
#define sTypeLimitlessRGB		0x02
#define sTypeLimitlessWhite		0x03
#define sTypeLimitlessRGBWW		0x04

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

	_tThermostat()
	{
		len=sizeof(_tThermostat)-1;
		type=pTypeThermostat;
		subtype=sTypeThermTemperature;
		battery_level=255;
		id1=1;
		id2=0;
		id3=0;
		id4=0;
		dunit=0;
		temp=0;
		temp1=0;
		temp2=0;
		temp3=0;
		utemp1=0;
		utemp2=0;
		utemp3=0;
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

	_tTempBaro()
	{
		len=sizeof(_tTempBaro)-1;
		type=pTypeTEMP_BARO;
		subtype=sTypeBMP085;
		id1=1;
		temp=0;
		baro=0;
		altitude=0;
		forecast=0;
	}
} _tTempBaro;

typedef struct _tAirQualityMeter {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint8_t	id1;
	uint8_t	id2;
	int32_t airquality;
	_tAirQualityMeter()
	{
		len=sizeof(_tAirQualityMeter)-1;
		type=pTypeAirQuality;
		subtype=sTypeVoltcraft;
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

	_tUsageMeter()
	{
		len=sizeof(_tUsageMeter)-1;
		type=pTypeUsage;
		subtype=sTypeElectric;
		id1=0;
		id2=0;
		id3=0;
		id4=0;
		dunit=0;
		fusage=0;
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

	_tLightMeter()
	{
		len=sizeof(_tLightMeter)-1;
		type=pTypeLux;
		subtype=sTypeLux;
		id1=0;
		id2=0;
		id3=0;
		id4=0;
		battery_level=255;
		dunit=0;
		fLux=0;
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
	_tGeneralDevice()
	{
		len=sizeof(_tGeneralDevice)-1;
		type=pTypeGeneral;
		subtype=sTypeVisibility;
		id=0;
		floatval1=0;
		floatval2=0;
		intval1=0;
		intval2=0;
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
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint32_t powerusage1;
	uint32_t powerusage2;
	uint32_t powerdeliv1;
	uint32_t powerdeliv2;
	uint32_t usagecurrent;
	uint32_t delivcurrent;
	int32_t ID;
	_tP1Power()
	{
		len = sizeof(_tP1Power) - 1;
		type = pTypeP1Power;
		subtype = sTypeP1Power;
		ID = 1;
		powerusage1 = 0;
		powerusage2 = 0;
		powerdeliv1 = 0;
		powerdeliv2 = 0;
		usagecurrent = 0;
		delivcurrent = 0;
	}
} P1Power;

typedef struct _tP1Gas {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint32_t gasusage;
	int32_t ID;
	_tP1Gas()
	{
		len = sizeof(_tP1Gas) - 1;
		type = pTypeP1Gas;
		subtype = sTypeP1Gas;
		ID = 1;
		gasusage = 0;
	}
} P1Gas;

typedef struct _tLimitlessLights {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint32_t id;
	uint8_t dunit; //0=All, 1=Group1,2=Group2,3=Group3,4=Group4, 5=IboxLed
	uint8_t command;
	uint8_t value;
	_tLimitlessLights()
	{
		id = 1;
		dunit = 1;
		len=sizeof(_tLimitlessLights)-1;
		type=pTypeLimitlessLights;
		subtype=sTypeLimitlessRGBW;
		command=0;
		value=0;
	}
} _tLimitlessLights;

#define Limitless_LedOff 0
#define Limitless_LedOn 1
#define Limitless_LedNight 2
#define Limitless_LedFull 3
#define Limitless_BrightnessUp 4
#define Limitless_BrightnessDown 5
#define Limitless_ColorTempUp 6
#define Limitless_ColorTempDown 7
#define Limitless_RGBDiscoNext 8
#define Limitless_RGBDiscoPrevious 9
#define Limitless_SetRGBColour 10
#define Limitless_DiscoSpeedSlower 11
#define Limitless_DiscoSpeedFaster 12
#define Limitless_DiscoMode 13
#define Limitless_SetColorToWhite 14
#define Limitless_SetBrightnessLevel 15
#define Limitless_SetBrightUp 16
#define Limitless_SetBrightDown 17
#define Limitless_WarmWhiteIncrease 18
#define Limitless_CoolWhiteIncrease 19
#define Limitless_NightMode 20
#define Limitless_FullBrightness 21
#define Limitless_DiscoSpeedFasterLong 22 //exclude RGB
#define Limitless_SetHEXColour 23
#define Limitless_DiscoMode_1 24
#define Limitless_DiscoMode_2 25
#define Limitless_DiscoMode_3 26
#define Limitless_DiscoMode_4 27
#define Limitless_DiscoMode_5 28
#define Limitless_DiscoMode_6 29
#define Limitless_DiscoMode_7 30
#define Limitless_DiscoMode_8 31
#define Limitless_DiscoMode_9 32
#define Limitless_SetKelvinLevel 33
#define Limitless_DiscoSpeedMinimal 34
#define Limitless_DiscoSpeedMaximal 35


typedef union tREVOBUF {
	struct _tEVOHOME1 {
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
	} EVOHOME1;

	struct _tEVOHOME2 {
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
	} EVOHOME2;

	struct _tEVOHOME3 {
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
	} EVOHOME3;
} REVOBUF;
