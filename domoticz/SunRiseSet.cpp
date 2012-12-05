#include "stdafx.h"
#include "SunRiseSet.h"

#include <math.h>
#include <time.h>
#include <sys/timeb.h>

#include <boost/date_time/c_local_time_adjustor.hpp>

#ifndef PI
#define PI			3.1415926535897932384
#endif


#define tpi			(2 * PI)
#define degs		(180.0/PI)
#define rads		(PI/180.0)

double L,g;
#define SunDia		0.53  // Sun radius degrees
#define AirRefr		(34.0/60.0) // atmospheric refraction degrees //

// Get the days to J2000
// h is UT in decimal hours
// FNday only works between 1901 to 2099 - see Meeus chapter 7

double SunRiseSet::FNday (int y, int m, int d, float h)
{
	long int luku = - 7 * (y + (m + 9)/12)/4 + 275*m/9 + d;

	// Typecasting needed for TClite on PC DOS at least, to avoid product overflow
	luku+= (long int)y*367;

	return (double)luku - 730531.5 + h/24.0;
};

// the function below returns an angle in the range
// 0 to 2*pi

double SunRiseSet::FNrange (double x)
{
	double b = x / tpi;
	double a = tpi * (b - (long)(b));
	if (a < 0) a = tpi + a;
	return a;
};

// Calculating the hour angle
double SunRiseSet::f0(double lat, double declin)
{
	double fo,dfo;
	// Correction: different sign at S HS
	dfo = rads*(0.5*SunDia + AirRefr); if (lat < 0.0) dfo = -dfo;
	fo = tan(declin + dfo) * tan(lat*rads);

	if (fo > 0.99999) fo=1.0; // to avoid overflow //
	fo = asin(fo) + PI/2.0;
	return fo;
};

// Calculating the hour angle for twilight times
//
double SunRiseSet::f1(double lat, double declin)
{

	double fi,df1;
	// Correction: different sign at S HS
	df1 = rads * 6.0; if (lat < 0.0) df1 = -df1;
	fi = tan(declin + df1) * tan(lat*rads);

	if (fi > 0.99999) fi=1.0; // to avoid overflow //
	fi = asin(fi) + PI/2.0;
	return fi;
};

// Find the ecliptic longitude of the Sun
double SunRiseSet::FNsun (double d)
{
	// mean longitude of the Sun
	L = FNrange(280.461 * rads + .9856474 * rads * d);

	// mean anomaly of the Sun
	g = FNrange(357.528 * rads + .9856003 * rads * d);

	// Ecliptic longitude of the Sun
	return FNrange(L + 1.915 * rads * sin(g) + .02 * rads * sin(2 * g));
};

// Display decimal hours in hours and minutes
void SunRiseSet::gethourmin(const double dhr, int &hour, int &min)
{
	hour=(int) dhr;
	min = (int)((dhr - (double) hour)*60);
};

bool SunRiseSet::GetSunRiseSet(const double latit, const double longit, _tSubRiseSetResults &result)
{
	time_t sekunnit;
	struct tm *ltime;
	/** First get current time **/
	time(&sekunnit);
	ltime=localtime(&sekunnit);
	// this is Y2K compliant algorithm
	int year = 1900 + ltime->tm_year;
	int month = ltime->tm_mon + 1;
	int day = ltime->tm_mday;
	return GetSunRiseSet(latit, longit, year,month,day,result);
}

boost::posix_time::time_duration get_utc_offset() {
	using namespace boost::posix_time;

	// boost::date_time::c_local_adjustor uses the C-API to adjust a
	// moment given in utc to the same moment in the local time zone.
	typedef boost::date_time::c_local_adjustor<ptime> local_adj;

	const ptime utc_now = second_clock::universal_time();
	const ptime now = local_adj::utc_to_local(utc_now);

	return now - utc_now;
}

bool SunRiseSet::GetSunRiseSet(const double latit, const double longit, const int year, const int month, const int day, _tSubRiseSetResults &result)
{
	result.latit=latit;
	result.longit=longit;
	result.year=year;
	result.month=month;
	result.day=day;

	double daylen;

	double d = FNday(year, month, day, 12);

	// Use FNsun to find the ecliptic longitude of the
	// Sun

	double lambda = FNsun(d);

	// Obliquity of the ecliptic

	double obliq = 23.439 * rads - .0000004 * rads * d;

	// Find the RA and DEC of the Sun

	double alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
	double delta = asin(sin(obliq) * sin(lambda));


	// Find the Equation of Time in minutes
	// Correction suggested by David Smith

	double LL = L - alpha;
	if (L < PI) LL += tpi;
	double equation = 1440.0 * (1.0 - LL / tpi);


	double ha = f0(latit,delta);

	double hb = f1(latit,delta);
	double twx = hb - ha;   // length of twilight in radians
	twx = 12.0*twx/PI;      // length of twilight in degrees

	// Conversion of angle to hours and minutes //
	daylen = degs * ha / 7.5;
	if (daylen<0.0001) {daylen = 0.0;}
	// arctic winter   //

	boost::posix_time::time_duration uoffset=get_utc_offset();

	double timezone=(double)(uoffset.ticks()/3600000000L);

	double riset = 12.0 - 12.0 * ha/PI + timezone - longit/15.0 + equation/60.0;
	double settm = 12.0 + 12.0 * ha/PI + timezone - longit/15.0 + equation/60.0;
	double noont = riset + 12.0 * ha/PI;
	double altmax = 90.0 + delta * degs - latit;
	// Correction suggested by David Smith
	// to express as degrees from the N horizon

	if (delta * degs > latit ) altmax = 90.0 + latit - delta * degs;

	double twam = riset - twx;      // morning twilight begin
	double twpm = settm + twx;      // evening twilight end

	if (riset > 24.0) riset-= 24.0;
	if (settm > 24.0) settm-= 24.0;
	if (riset < 0.0) riset+= 24.0;
	if (settm < 0.0) settm+= 24.0;
	if (noont > 24.0) noont-= 24.0;
	if (noont < 0.0) noont+= 24.0;
	if (twam > 24.0) twam-= 24.0;
	if (twam < 0.0) twam+= 24.0;
	if (twpm > 24.0) twpm-= 24.0;
	if (twpm < 0.0) twpm+= 24.0;

	result.Declination = delta * degs;

	gethourmin(daylen,result.DaylengthHour,result.DaylengthMin);

	gethourmin(twam,result.BeginCivilTwilightHour,result.BeginCivilTwilightMin);

	gethourmin(riset,result.SunRiseHour,result.SunRiseMin);
	
	gethourmin(noont,result.NoonHour,result.NoonMin);

	result.AltitudeNoonMax = altmax;
	result.AltitudeNoonMaxSN = (latit>= delta * degs ? 'S' : 'N');

	gethourmin(settm,result.SunSetHour,result.SunSetMin);
	
	gethourmin(twpm,result.CivilTwilightHour,result.CivilTwilightMin);

	result.DaysUntilY2K=-d;

	return true;
}
