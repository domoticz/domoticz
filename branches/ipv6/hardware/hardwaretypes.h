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
#define sTypeSwitch					0x19
#define sTypeBaro					0x1A
#define sTypeDistance				0x1B
#define sTypeCounterIncremental		0x1C

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

//Z-Wave
//#define pTypeENERGY 0x5A
#define sTypeZWaveUsage 0xA0
#define sTypeZWaveSwitch 0xA1

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
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	unsigned char dunit;
	unsigned char battery_level;
	float temp;
	float temp1;
	float temp2;
	float temp3;
	unsigned char utemp1;
	unsigned char utemp2;
	unsigned char utemp3;

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
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	BYTE	id1;
	float temp;
	float baro;
	float altitude;
	unsigned char forecast;

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
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	BYTE	id1;
	BYTE	id2;
	int airquality;
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
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	unsigned char dunit;
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
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	BYTE	id1;
	BYTE	id2;
	BYTE	id3;
	BYTE	id4;
	unsigned char dunit;
	unsigned char battery_level;
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
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned char id;
	float floatval1;
	float floatval2;
	int intval1;
	int intval2;
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
	}
} GeneralDevice;

typedef struct _tGeneralSwitch {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	int			  id;
	int			  unitcode;
	unsigned char cmnd;
	unsigned char level;
	unsigned char rssi;
	unsigned char battery_level;
	unsigned char seqnbr;
	int			  reserved_int;
	float		  reserved_float;
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
	}
} GeneralSwitch;

typedef struct _tP1Power {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned long powerusage1;
	unsigned long powerusage2;
	unsigned long powerdeliv1;
	unsigned long powerdeliv2;
	unsigned long usagecurrent;
	unsigned long delivcurrent;
	int ID;
	_tP1Power()
	{
		len = sizeof(_tP1Power) - 1;
		type = pTypeP1Power;
		subtype = sTypeP1Power;
		ID = 1;
	}
} P1Power;

typedef struct _tP1Gas {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned long gasusage;
	int ID;
	_tP1Gas()
	{
		len = sizeof(_tP1Gas) - 1;
		type = pTypeP1Gas;
		subtype = sTypeP1Gas;
		ID = 1;
	}
} P1Gas;

typedef struct _tLimitlessLights {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned long id;
	unsigned char dunit; //0=All, 1=Group1,2=Group2,3=Group3,4=Group4
	unsigned char command;
	unsigned char value;
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

typedef union tREVOBUF {
	struct _tEVOHOME1 {
		unsigned char len;
		unsigned char type;
		unsigned char subtype;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
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
		unsigned char len;
		unsigned char type;
		unsigned char subtype;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
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
		unsigned char len;
		unsigned char type;
		unsigned char subtype;
		BYTE	id1;
		BYTE	id2;
		BYTE	id3;
		uint8_t	devno;
		uint8_t	demand;
	} EVOHOME3;
} REVOBUF;
