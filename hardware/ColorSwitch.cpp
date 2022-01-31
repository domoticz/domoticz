#include "stdafx.h"
#include "../main/Helper.h"
#include "hardwaretypes.h"
#include "ColorSwitch.h"
#include "../main/json_helper.h"
#include <cmath>

_tColor::_tColor()
{
	//level = 0;
	t = r = g = b = cw = ww = 0;
	mode = ColorModeNone;
}

_tColor::_tColor(const Json::Value &json)
{
	fromJSON(json);
}

_tColor::_tColor(const std::string &sRaw) //explicit to avoid unintentional conversion of string to _tColor
{
	fromString(sRaw);
}

_tColor::_tColor(const uint8_t ir, const uint8_t ig, const uint8_t ib, const uint8_t icw, const uint8_t iww, ColorMode imode)
{
	mode = imode;
	//level=ilevel;
	r=ir;
	g=ig;
	b=ib;
	cw=icw;
	ww=iww;
	t = 0;
}

_tColor::_tColor(uint8_t x, ColorMode imode)
{
	t = r = g = b = cw = ww = 0;
	mode = imode;
	if (mode == ColorModeWhite)
	{
		ww = 0xff;
		cw = 0xff;
	}
	else if (mode == ColorModeTemp)
	{
		ww = x;
		cw = 255-x;
		t = x;
	}
}

std::string _tColor::getrgbwwhex() const
{
	char tmp[13];
	snprintf(tmp, sizeof(tmp), "%02x%02x%02x%02x%02x", r, g, b, cw, ww);
	return std::string(tmp);
}

void _tColor::fromJSON(const Json::Value &root)
{
	mode = ColorModeNone;
	try {
		int tmp = root.get("m", 0).asInt();
		if (tmp == ColorModeNone || tmp > ColorModeLast) return;
		mode = ColorMode(tmp);
		t = (uint8_t)root.get("t", 0).asInt();
		r = (uint8_t)root.get("r", 0).asInt();
		g = (uint8_t)root.get("g", 0).asInt();
		b = (uint8_t)root.get("b", 0).asInt();
		cw = (uint8_t)root.get("cw", 0).asInt();
		ww = (uint8_t)root.get("ww", 0).asInt();
		//level = root.get("l", 0).asInt();
	}
	catch (...) {
	}
}

void _tColor::fromString(const std::string &s)
{
	Json::Value root;
	mode = ColorModeNone;
	try {
		bool parsingSuccessful = ParseJSonStrict(s, root);
		if ( !parsingSuccessful )
		{
			mode = ColorModeNone;
			return;
		}
		fromJSON(root);
	}
	catch (...) {
	}
}

Json::Value _tColor::toJSONValue() const
{
	Json::Value root;

	// Return empty value if the color is not valid
	if (mode == ColorModeNone || mode > ColorModeLast) return root;

	root["m"] = mode;
	//root["l"] = level;
	root["t"] = t;
	root["r"] = r;
	root["g"] = g;
	root["b"] = b;
	root["cw"] = cw;
	root["ww"] = ww;

	return root;
}

std::string _tColor::toJSONString() const
{
	// Return the empty string if the color is not valid
	if (mode == ColorModeNone || mode > ColorModeLast) return "";

	Json::Value root = toJSONValue();

	return JSonToRawString(root);
}

std::string _tColor::toString() const
{
	char tmp[1024];
	// Return the empty string if the color is not valid
	if (mode == ColorModeNone || mode > ColorModeLast) return "{INVALID}";

	snprintf(tmp, sizeof(tmp), "{m: %u, RGB: %02x%02x%02x, CWWW: %02x%02x, CT: %u}", (unsigned int)mode, r, g, b, cw, ww, t);

	return std::string(tmp);
}

void _tColor::RgbFromXY(const double x, const double y, uint8_t &r8, uint8_t &g8, uint8_t &b8)
{
	double brightness = 1.0;

	/* Returns (r, g, b) for given x, y values.
	   Implementation of the instructions found on the Philips Hue iOS SDK docs: http://goo.gl/kWKXKl
	*/

	// Calculate XYZ values Convert using the following formulas:
	double Y = brightness;
	double X = (Y / y) * x;
	double Z = (Y / y) * (1 - x - y);

	// Convert to RGB using Wide RGB D65 conversion
	double r = X * 1.656492 - Y * 0.354851 - Z * 0.255038;
	double g = -X * 0.707196 + Y * 1.655397 + Z * 0.036152;
	double b = X * 0.051713 - Y * 0.121364 + Z * 1.011530;

	// Scale before gamma correction
	if (r > b && r > g && r > 1.0)
	{
		// red is too big
		g = g / r;
		b = b / r;
		r = 1.0;
	}
	else if (g > b && g > r && g > 1.0)
	{
		// green is too big
		r = r / g;
		b = b / g;
		g = 1.0;
	}
	else if (b > r && b > g && b > 1.0)
	{
		// blue is too big
		r = r / b;
		g = g / b;
		b = 1.0;
	}

	// Apply gamma correction
	r = r <= 0.0031308 ? 12.92 * r : (1.0 + 0.055) * pow(r, (1.0 / 2.4)) - 0.055;
	g = g <= 0.0031308 ? 12.92 * g : (1.0 + 0.055) * pow(g, (1.0 / 2.4)) - 0.055;
	b = b <= 0.0031308 ? 12.92 * b : (1.0 + 0.055) * pow(b, (1.0 / 2.4)) - 0.055;

	// Bring all negative components to zero
	r = std::max(0.0, r);
	g = std::max(0.0, g);
	b = std::max(0.0, b);

	// If one component is greater than 1, weight components by that value.
	// TODO: Rewrite with C++11 lambda functions:
	// max_component = max(r, g, b)
	//    if max_component > 1:
	//        r, g, b = map(lambda x: x / max_component, [r, g, b])
	if (r > b && r > g)
	{
		// red is biggest
		if (r > 1.0)
		{
			g = g / r;
			b = b / r;
			r = 1.0;
		}
	}
	else if (g > b && g > r)
	{
		// green is biggest
		if (g > 1.0)
		{
			r = r / g;
			b = b / g;
			g = 1.0;
		}
	}
	else if (b > r && b > g)
	{
		// blue is biggest
		if (b > 1.0)
		{
			r = r / b;
			g = g / b;
			b = 1.0;
		}
	}

	// Convert the RGB values to your color object The rgb values from the above formulas are between 0.0 and 1.0.
	r8 = uint8_t(r * 255.0);
	g8 = uint8_t(g * 255.0);
	b8 = uint8_t(b * 255.0);
}

void _tColor::XYFromRGB(const uint8_t r8, const uint8_t g8, const uint8_t b8, double& x, double& y, double& Y)
{
	//Input r8,g8,b8 0-255
	//Output Yxy (0 - 1.0)
	double red = double(r8) / 255.0;
	double green = double(g8) / 255.0;
	double blue = double(b8) / 255.0;

	// Apply gamma correction
	double varR = (red > 0.04045) ? pow((red + 0.055) / (1.0 + 0.055), 2.4) : (red / 12.92);
	double varG = (green > 0.04045) ? pow((green + 0.055) / (1.0 + 0.055), 2.4) : (green / 12.92);
	double varB = (blue > 0.04045) ? pow((blue + 0.055) / (1.0 + 0.055), 2.4) : (blue / 12.92);

	double tX = varR * 0.664511 + varG * 0.154324 + varB * 0.162028;
	double tY = varR * 0.283881 + varG * 0.668433 + varB * 0.047685;
	double tZ = varR * 0.000088 + varG * 0.072310 + varB * 0.986039;

	// XYZ to Yxy
	Y = tY;
	x = tX / (tX + tY + tZ);
	y = tY / (tX + tY + tZ);

	if (std::isnan(Y)) {
		Y = 0.0;
	}

	if (std::isnan(x)) {
		x = 0.0;
	}

	if (std::isnan(y)) {
		y = 0.0;
	}
}
