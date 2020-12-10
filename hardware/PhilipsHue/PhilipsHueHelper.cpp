#include "stdafx.h"
#include "PhilipsHue.h"
#include "../../main/Logger.h"
#include <cmath>

/*
 * Helper funtions to convert from Philips XY color to RGB
 * Based on: https://github.com/benknight/hue-python-rgb-converter/blob/master/rgbxy
 * And also: https://developers.meethue.com/documentation/color-conversions-rgb-xy (login needed)
 *           https://github.com/Q42Philips/hue-color-converter/blob/master/index.js
 */

namespace
{
	struct point
	{
		double x;
		double y;
		point()
		{
			x = 0.0;
			y = 0.0;
		}
		point(double x, double y)
		{
			this->x = x;
			this->y = y;
		}
	};

	const point colorPointsGamut_A[3] = { point(0.703, 0.296), point(0.214, 0.709), point(0.139, 0.081) };
	const point colorPointsGamut_B[3] = { point(0.674, 0.322), point(0.408, 0.517), point(0.168, 0.041) };
	const point colorPointsGamut_C[3] = { point(0.692, 0.308), point(0.17, 0.7), point(0.153, 0.048) };
	const point colorPointsGamut_Default[3] = { point(1.0, 0.0), point(0.0, 1.0), point(0.0, 0.0) };

	constexpr std::array<const char *, 10> GAMUT_A_BULBS_LIST{ "LLC001", "LLC005", "LLC006", "LLC007", "LLC010", "LLC011", "LLC012", "LLC014", "LLC013", "LST001" };
	constexpr std::array<const char *, 8> GAMUT_B_BULBS_LIST{ "LCT001", "LCT002", "LCT003", "LCT004", "LLM001", "LCT005", "LCT006", "LCT007" };
	constexpr std::array<const char *, 10> GAMUT_C_BULBS_LIST{ "LCT010", "LCT011", "LCT012", "LCT014", "LCT015", "LCT016", "LLC020", "LST002", "LCS001", "LCG002" };
	constexpr std::array<const char *, 7> MULTI_SOURCE_LUMINAIRES{ "HBL001", "HBL002", "HBL003", "HIL001", "HIL002", "HEL001", "HEL002" };

	const point *get_light_gamut(const std::string &modelid)
	{
		if (std::find(GAMUT_A_BULBS_LIST.begin(), GAMUT_A_BULBS_LIST.end(), modelid) != GAMUT_A_BULBS_LIST.end())
			return colorPointsGamut_A;
		if (std::find(GAMUT_B_BULBS_LIST.begin(), GAMUT_B_BULBS_LIST.end(), modelid) != GAMUT_B_BULBS_LIST.end())
			return colorPointsGamut_B;
		if (std::find(MULTI_SOURCE_LUMINAIRES.begin(), MULTI_SOURCE_LUMINAIRES.end(), modelid) != MULTI_SOURCE_LUMINAIRES.end())
			return colorPointsGamut_B;
		if (std::find(GAMUT_C_BULBS_LIST.begin(), GAMUT_C_BULBS_LIST.end(), modelid) != GAMUT_C_BULBS_LIST.end())
			return colorPointsGamut_C;
		return colorPointsGamut_Default;
	}

	double cross_product(point p1, point p2)
	{
		// Returns the cross product of two points
		// (This is really the determinant, not the cross product)
		return p1.x * p2.y - p1.y * p2.x;
	}

	bool check_point_in_lamps_reach(point p, const std::string &modelid)
	{
		const point *gamut = get_light_gamut(modelid);
		point Red = gamut[0];
		point Lime = gamut[1];
		point Blue = gamut[2];

		// Check if the provided XYPoint can be recreated by a Hue lamp
		point v1 = point(Lime.x - Red.x, Lime.y - Red.y);
		point v2 = point(Blue.x - Red.x, Blue.y - Red.y);

		point q = point(p.x - Red.x, p.y - Red.y);
		double s = cross_product(q, v2) / cross_product(v1, v2);
		double t = cross_product(v1, q) / cross_product(v1, v2);

		return (s >= 0.0) && (t >= 0.0) && (s + t <= 1.0);
	}

	point get_closest_point_to_line(point A, point B, point P)
	{
		// Find the closest point on a line. This point will be reproducible by a Hue lamp.
		point AP = point(P.x - A.x, P.y - A.y);
		point AB = point(B.x - A.x, B.y - A.y);
		double ab2 = AB.x * AB.x + AB.y * AB.y;
		double ap_ab = AP.x * AB.x + AP.y * AB.y;
		double t = ap_ab / ab2;

		if (t < 0.0)
			t = 0.0;
		else if (t > 1.0)
			t = 1.0;

		return { A.x + AB.x * t, A.y + AB.y * t };
	}

	double get_distance_between_two_points(point p1, point p2)
	{
		// Returns the distance between two points.
		double dx = p1.x - p2.x;
		double dy = p1.y - p2.y;
		return sqrt(dx * dx + dy * dy);
	}

	point get_closest_point_to_point(point xy_point, const std::string &modelid)
	{
		const point *gamut = get_light_gamut(modelid);
		point Red = gamut[0];
		point Lime = gamut[1];
		point Blue = gamut[2];

		// Color is unreproducible, find the closest point on each line in the CIE 1931 'triangle'.
		point pAB = get_closest_point_to_line(Red, Lime, xy_point);
		point pAC = get_closest_point_to_line(Blue, Red, xy_point);
		point pBC = get_closest_point_to_line(Lime, Blue, xy_point);

		// Get the distances per point and see which point is closer to our Point.
		double dAB = get_distance_between_two_points(xy_point, pAB);
		double dAC = get_distance_between_two_points(xy_point, pAC);
		double dBC = get_distance_between_two_points(xy_point, pBC);

		double lowest = dAB;
		point closest_point = pAB;

		if (dAC < lowest)
		{
			lowest = dAC;
			closest_point = pAC;
		}

		if (dBC < lowest)
		{
			lowest = dBC;
			closest_point = pBC;
		}

		// Change the xy value to a value which is within the reach of the lamp.
		double cx = closest_point.x;
		double cy = closest_point.y;

		return { cx, cy };
	}
} // namespace

void CPhilipsHue::RgbFromXY(const double x, const double y, const double bri, const std::string &modelid, uint8_t &r8, uint8_t &g8, uint8_t &b8)
{
	/* Returns (r, g, b) for given x, y values.
	   Implementation of the instructions found on the Philips Hue iOS SDK docs: http://goo.gl/kWKXKl

	   Check if the xy value is within the color gamut of the lamp.
	   If not continue with step 2, otherwise step 3.
	   We do this to calculate the most accurate color the given light can actually do.*/

	point xy_point = point(x, y);

	// TODO: Maybe thsi can be skipped when converting xy-point reported to a light to rgb - why would a light report an unreachable point?
	if (!check_point_in_lamps_reach(xy_point, modelid)) {
		// Calculate the closest point on the color gamut triangle and use that as xy value
		xy_point = get_closest_point_to_point(xy_point, modelid);
	}

	// Calculate XYZ values Convert using the following formulas:
	double Y = bri;
	double X = (Y / xy_point.y) * xy_point.x;
	double Z = (Y / xy_point.y) * (1 - xy_point.x - xy_point.y);

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
	r = std::max(0.0,r);
	g = std::max(0.0,g);
	b = std::max(0.0,b);

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

bool CPhilipsHue::StatesSimilar(const _tHueLightState &s1, const _tHueLightState &s2)
{
	bool res = false;
	if (s1.on == s2.on &&
	    s1.mode == s2.mode &&
	    abs(s1.level - s2.level) <= 1)
	{
		switch (s1.mode)
		{
			case HLMODE_NONE:
				res = true;
				break;
			case HLMODE_CT:
				res = abs(s1.ct - s2.ct) <= 3; // 3 is 1% of 255
				break;
			case HLMODE_HS:
			{
				uint16_t h1 = (uint16_t) s1.hue;
				uint16_t h2 = (uint16_t) s2.hue;
				res = abs(int16_t(h1-h2)) < 655 && // 655 is 1% of 65535, the range of hue
					  abs(s1.sat - s2.sat) <= 3;   // 3 is 1% of 255, the range of sat
				  break;
			}
			case HLMODE_XY:
				res = std::abs(s1.x - s2.x) < 0.01 && // 655 is 1% of 65535, the range of hue
					  std::abs(s1.y - s2.y) < 0.01;   // 3 is 1% of 255, the range of sat
				break;
		}
	}
	return res;
}
