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

		double DaysUntilY2K;

		double Declination;

		int DaylengthHour;
		int DaylengthMin;

		int BeginCivilTwilightHour;
		int BeginCivilTwilightMin;

		int CivilTwilightHour;
		int CivilTwilightMin;

		int NoonHour;
		int NoonMin;

		double AltitudeNoonMax;
		unsigned char AltitudeNoonMaxSN;

		int SunRiseHour;
		int SunRiseMin;
		int SunSetHour;
		int SunSetMin;
	};
	static bool GetSunRiseSet(const double latit, const double longit, _tSubRiseSetResults &result);
	static bool GetSunRiseSet(const double latit, const double longit, const int year, const int month, const int day, _tSubRiseSetResults &result);
private:
	static double FNday (int y, int m, int d, float h);
	static double FNrange (double x);
	static double f0(double lat, double declin);
	static double f1(double lat, double declin);
	static double FNsun (double d);
	static void gethourmin(const double dhr, int &hour, int &min);
};

