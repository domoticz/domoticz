#include "stdafx.h"
#include "Deconz.h"
#include "../../json/value.h"
#include <math.h>

struct point;

DeconzLight::DeconzLight(Json::Value light): DeconzNode(light)
{
	LightStateFromJSON(light["state"], state, lightType);
}

void DeconzLight::LightStateFromJSON(const Json::Value& ls, LightState& state, LightType& lightType)
{
	if (ls.isObject())
	{
		state.bri = 0;
		state.sat = 0;
		state.hue = 0;
		state.ct = 0;
		state.x = 0.0;
		state.y = 0.0;
		state.reachable = false;
		state.on = false;
		state.mode = LMODE_NONE;

		bool hasBri = false;
		bool hasHueSat = false;
		bool hasTemp = false;

		lightType = LTYPE_NORMAL;

		if (!ls["on"].empty())
		{			
			state.on = ls["on"].asBool();
		}
		if(!ls["colormode"].empty())
		{
			auto mode = ls["colormode"].asString();
			if (mode == "hs") state.mode = LMODE_HS;
			if (mode == "xy") state.mode = LMODE_XY;
			if (mode == "ct") state.mode = LMODE_CT;
		}
		if(!ls["bri"].empty())
		{
			hasBri = true;
			int bri = ls["bri"].asInt();
			state.bri = bri;
			state.level = int(ceil((100.0 / 254.0)*float(bri)));
		}
		if(!ls["sat"].empty())
		{
			hasHueSat = true;
			state.sat = ls["sat"].asInt();
		}
		if(!ls["hue"].empty())
		{
			hasHueSat = true;
			state.hue = ls["hue"].asInt();
		}
		if(!ls["ct"].empty())
		{
			hasTemp = true;
			state.ct = ls["ct"].asInt();
		}
		if(!ls["xy"].empty())
		{
			hasHueSat = true;
			state.x = ls["xy"][0].asDouble();
			state.y = ls["xy"][1].asDouble();
		}

		if (hasBri) lightType = LTYPE_DIM;
		if (hasBri && hasHueSat && !hasTemp) lightType = LTYPE_RGB_W;
		if (hasBri && !hasHueSat && hasTemp) lightType = LTYPE_CW_WW;
		if (hasBri && hasHueSat && hasTemp) lightType = LTYPE_RGB_CW_WW;
	}
}

DeconzLight::DeconzLight():DeconzNode(), hasColor(false), state()
{
	state.mode = LMODE_NONE;
	lightType = LTYPE_NORMAL;
}


bool DeconzLight::StatesSimilar(const LightState& s1, const LightState& s2)
{
	bool res = false;
	if (s1.on == s2.on && s1.mode==s2.mode && abs(s1.bri - s2.bri) <= 1)
	{
		switch (s1.mode) 
		{ 
			case LMODE_NONE:
				res = true;
			break;
		case LMODE_HS:			
			res = abs(s1.hue - s2.hue < 655 && abs(s1.sat - s2.sat) <= 3);
			break;
		case LMODE_CT:
			res = abs(s1.ct - s2.ct) <= 3;
			break;

		case LMODE_XY:
			res = abs(s1.x - s2.x) < 0.01 &&
				abs(s1.y - s2.y) < 0.01;
			break;		
		}
	}

	return res;
}

void DeconzLight::RgbFromXY(const double x, const double y, const double bri, uint8_t& r8, uint8_t& g8, uint8_t& b8)
{
	/* Raped from Philips Hue implementation (thanks? **buuurrp** ) */
	
	// Calculate XYZ values Convert using the following formulas:
	double Y = bri;
	double X = (Y / y) * x;
	double Z = (Y / y) * (1 - x - y);

	// Convert to RGB using Wide RGB D65 conversion
	double r = X * 1.656492 - Y * 0.354851 - Z * 0.255038;
	double g = -X * 0.707196 + Y * 1.655397 + Z * 0.036152;
	double b = X * 0.051713 - Y * 0.121364 + Z * 1.011530;

	// Scale before gamma correction
	if (r > b && r > g && r > 1.0) {
		// red is too big
		g = g / r;
		b = b / r;
		r = 1.0;
	}
	else if (g > b && g > r && g > 1.0) {
		// green is too big
		r = r / g;
		b = b / g;
		g = 1.0;
	}
	else if (b > r && b > g && b > 1.0) {
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
	//max_component = max(r, g, b)
	//    if max_component > 1:
	//        r, g, b = map(lambda x: x / max_component, [r, g, b])
	if (r > b && r > g) {
		// red is biggest
		if (r > 1.0) {
			g = g / r;
			b = b / r;
			r = 1.0;
		}
	}
	else if (g > b && g > r) {
		// green is biggest
		if (g > 1.0) {
			r = r / g;
			b = b / g;
			g = 1.0;
		}
	}
	else if (b > r && b > g) {
		// blue is biggest
		if (b > 1.0) {
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

