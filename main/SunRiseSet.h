#pragma once

class SunRiseSet
{
public:
	struct _tSubRiseSetResults
	{
		double latit;
		double longit;
		int year;
		int month;
		int day;

		int SunRiseHour;
		int SunRiseMin;
		int SunSetHour;
		int SunSetMin;

		int DaylengthHours;
		int DaylengthMins;

		int SunAtSouthHour;
		int SunAtSouthMin;

		int CivilTwilightStartHour;
		int CivilTwilightStartMin;
		int CivilTwilightEndHour;
		int CivilTwilightEndMin;
		int NauticalTwilightStartHour;
		int NauticalTwilightStartMin;
		int NauticalTwilightEndHour;
		int NauticalTwilightEndMin;
		int AstronomicalTwilightStartHour;
		int AstronomicalTwilightStartMin;
		int AstronomicalTwilightEndHour;
		int AstronomicalTwilightEndMin;

	};
	static bool GetSunRiseSet(const double latit, const double longit, _tSubRiseSetResults &result);
	static bool GetSunRiseSet(const double latit, const double longit, const int year, const int month, const int day, _tSubRiseSetResults &result);
private:
	static double UtcToLocal(double time, double tz);
	static double __daylen__( int year, int month, int day, double lon, double lat, double altit, int upper_limb);
	static int __sunriset__(int year, int month, int day, double lon, double lat, double altit, int upper_limb, double *rise, double *set);
	static void sunpos(double d, double *lon, double *r);
	static void sun_RA_dec(double d, double *RA, double *dec, double *r);
	static double revolution(double x);
	static double rev180(double x);
	static double GMST0(double d);
};
