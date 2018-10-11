#pragma once
#include "DeconzNode.h"


enum LightType
{
	LTYPE_NORMAL,
	LTYPE_DIM,
	LTYPE_RGB_W,
	LTYPE_CW_WW,
	LTYPE_RGB_CW_WW
};

enum LightColorMode
{
	LMODE_NONE,
	LMODE_HS,
	LMODE_CT,
	LMODE_XY
};

struct LightState
{
	bool on;
	LightColorMode mode;
	int bri;
	int hue;
	int sat;
	int ct;
	bool reachable;
	double x;
	double y;
	int level;	
};

class DeconzLight : public DeconzNode
{
public:
	DeconzLight();
	DeconzLight(Json::Value light);
	static void LightStateFromJSON(const Json::Value &lightState, LightState &state, LightType& lightType);
	static bool StatesSimilar(const LightState& s1, const LightState &s2);
	static void RgbFromXY(const double x, const double y, const double bri, uint8_t &r8, uint8_t &g8, uint8_t &b8);

	bool hasColor;
	LightState state;
	LightType lightType;
};
