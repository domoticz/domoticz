#pragma once

namespace Json
{
	class Value;
};

#define pTypeColorSwitch		0xF1
#define sTypeColor_RGB_W		0x01 // RGB + white, either RGB or white can be lit
#define sTypeColor_RGB			0x02 // RGB
#define sTypeColor_White		0x03 // Monochrome white
#define sTypeColor_RGB_CW_WW	0x04 // RGB + cold white + warm white, either RGB or white can be lit
#define sTypeColor_LivCol		0x05
#define sTypeColor_RGB_W_Z		0x06 // Like RGBW, but allows combining RGB and white
#define sTypeColor_RGB_CW_WW_Z	0x07 // Like RGBWW, but allows combining RGB and white
#define sTypeColor_CW_WW		0x08 // Cold white + Warm white

enum ColorMode {
	ColorModeNone = 0, // Illegal
	ColorModeWhite,    // White. Valid fields: none
	ColorModeTemp,     // White with color temperature. Valid fields: t
	ColorModeRGB,      // Color. Valid fields: r, g, b.
	                   // Note: The color will be normalized to full brightness in this mode before sent to HW
	                   // due to limitations in multiple HW types. Normalization is done automatically by Domoticz MQTT
	                   // and WebServer command handlers, and the brightness is adjusted accordingly.
	ColorModeCustom,   // Custom (color + white). Valid fields: r, g, b, cw, ww, depending on device capabilities
	ColorModeLast = ColorModeCustom,
};

struct _tColor {
	ColorMode mode;
	//uint8_t level; // Range:0..255, Master brightness (potential for future use)
	uint8_t t;     // Range:0..255, Color temperature (warm / cold ratio, 0 is coldest, 255 is warmest)
	uint8_t r;     // Range:0..255, Red level
	uint8_t g;     // Range:0..255, Green level
	uint8_t b;     // Range:0..255, Blue level
	uint8_t cw;    // Range:0..255, Cold white level
	uint8_t ww;    // Range:0..255, Warm white level (also used as level for monochrome white)

	_tColor();
	explicit _tColor(const Json::Value &json);
	explicit _tColor(const std::string &sRaw); //explicit to avoid unintentional conversion of string to _tColor
	explicit _tColor(const uint8_t ir, const uint8_t ig, const uint8_t ib, const uint8_t icw, const uint8_t iww, ColorMode imode);
	explicit _tColor(uint8_t x, ColorMode imode);
	std::string getrgbwwhex() const;
	void fromJSON(const Json::Value &root);
	void fromString(const std::string &s);
	std::string toJSONString() const;
	Json::Value toJSONValue() const;
	std::string toString() const;
};

const _tColor NoColor = _tColor();

struct _tColorSwitch {
	uint8_t len;
	uint8_t type;
	uint8_t subtype;
	uint32_t id;
	uint8_t dunit; //0=All, 1=Group1,2=Group2,3=Group3,4=Group4, 5=IboxLed
	uint8_t command;
	uint32_t value;  // Value of command
	_tColor color;   // Color

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar & cereal::make_nvp("len", len);
		ar & cereal::make_nvp("type", type);
		ar & cereal::make_nvp("subtype", subtype);
		ar & cereal::make_nvp("id", id);
		ar & cereal::make_nvp("dunit", dunit);
		ar & cereal::make_nvp("command", command);
		ar & cereal::make_nvp("value", value);
		ar & cereal::make_nvp("color", color);
	}

	_tColorSwitch()
	{
		id = 1;
		dunit = 1;
		len=sizeof(_tColorSwitch)-1;
		type=pTypeColorSwitch;
		subtype=sTypeColor_RGB_W;
		command=0;
		value=0;
		color=NoColor;
	}
};

#define Color_LedOff 0
#define Color_LedOn 1
#define Color_LedNight 2
#define Color_LedFull 3
#define Color_BrightnessUp 4
#define Color_BrightnessDown 5
#define Color_ColorTempUp 6
#define Color_ColorTempDown 7
#define Color_RGBDiscoNext 8
#define Color_RGBDiscoPrevious 9
#define Color_SetColor 10
#define Color_DiscoSpeedSlower 11
#define Color_DiscoSpeedFaster 12
#define Color_DiscoMode 13
#define Color_SetColorToWhite 14
#define Color_SetBrightnessLevel 15
#define Color_SetBrightUp 16
#define Color_SetBrightDown 17
#define Color_WarmWhiteIncrease 18
#define Color_CoolWhiteIncrease 19
#define Color_NightMode 20
#define Color_FullBrightness 21
#define Color_DiscoSpeedFasterLong 22 //exclude RGB
//#define Color_SetHEXColor 23
#define Color_DiscoMode_1 24
#define Color_DiscoMode_2 25
#define Color_DiscoMode_3 26
#define Color_DiscoMode_4 27
#define Color_DiscoMode_5 28
#define Color_DiscoMode_6 29
#define Color_DiscoMode_7 30
#define Color_DiscoMode_8 31
#define Color_DiscoMode_9 32
//#define Color_SetKelvinLevel 33
#define Color_DiscoSpeedMinimal 34
#define Color_DiscoSpeedMaximal 35
