#pragma once

#define sTypeTHBFloat 0x10   //Weather Station
#define sTypeWINDNoTemp 0x30    //Weather Station

#define wsbaroforcast_heavy_snow 0x01
#define wsbaroforcast_snow 0x01
#define wsbaroforcast_heavy_rain 0x02
#define wsbaroforcast_rain 0x03
#define wsbaroforcast_cloudy 0x04
#define wsbaroforcast_some_clouds 0x05
#define wsbaroforcast_sunny 0x06

#define pTypeAirQuality	0xF9
#define sTypeVoltcraft	0x01

#define pTypeP1Power	0xFA
#define sTypeP1Power	0x01

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


typedef struct _tAirQualityMeter {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	std::string ID;
	int airquality;
} AirQualityMeter;
