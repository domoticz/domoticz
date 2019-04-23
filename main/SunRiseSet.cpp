#include "stdafx.h"
#include "SunRiseSet.h"

#include <math.h>
#include "localtime_r.h"
//#include <sys/timeb.h>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>


#ifndef PI
#define PI			3.1415926535897932384
#endif

/* A macro to compute the number of days elapsed since 2000 Jan 0.0 */
/* (which is equal to 1999 Dec 31, 0h UT)						   */

#define days_since_2000_Jan_0(y,m,d) \
	(367L*(y)-((7*((y)+(((m)+9)/12)))/4)+((275*(m))/9)+(d)-730530L)

/* Some conversion factors between radians and degrees */

#ifndef PI
 #define PI		3.1415926535897932384
#endif

#define RADEG	 ( 180.0 / PI )
#define DEGRAD	( PI / 180.0 )

/* The trigonometric functions in degrees */

#define sind(x)  sin((x)*DEGRAD)
#define cosd(x)  cos((x)*DEGRAD)
#define tand(x)  tan((x)*DEGRAD)

#define atand(x)	(RADEG*atan(x))
#define asind(x)	(RADEG*asin(x))
#define acosd(x)	(RADEG*acos(x))
#define atan2d(y,x) (RADEG*atan2(y,x))


/* Following are some macros around the "workhorse" function __daylen__ */
/* They mainly fill in the desired values for the reference altitude	*/
/* below the horizon, and also selects whether this altitude should	 */
/* refer to the Sun's center or its upper limb.						 */


/* This macro computes the length of the day, from sunrise to sunset. */
/* Sunrise/set is considered to occur when the Sun's upper limb is	*/
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).										*/
#define day_length(year,month,day,lon,lat)  \
		__daylen__(year, month, day, lon, lat, -35.0/60.0, 1)

/* This macro computes the length of the day, including civil twilight. */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
/* the horizon.														 */
#define day_civil_twilight_length(year,month,day,lon,lat)  \
		__daylen__(year, month, day, lon, lat, -6.0, 0)

/* This macro computes the length of the day, incl. nautical twilight.  */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees	*/
/* below the horizon.												   */
#define day_nautical_twilight_length(year,month,day,lon,lat)  \
		__daylen__(year, month, day, lon, lat, -12.0, 0)

/* This macro computes the length of the day, incl. astronomical twilight. */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
/* below the horizon.													  */
#define day_astronomical_twilight_length(year,month,day,lon,lat)  \
		__daylen__(year, month, day, lon, lat, -18.0, 0)

/* This macro computes times for sunrise/sunset.					  */
/* Sunrise/set is considered to occur when the Sun's upper limb is	*/
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).										*/
#define sun_rise_set(year,month,day,lon,lat,rise,set)  \
		__sunriset__( year, month, day, lon, lat, -35.0/60.0, 1, rise, set )

/* This macro computes the start and end times of civil twilight.	   */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
/* the horizon.														 */
#define civil_twilight(year,month,day,lon,lat,start,end)  \
		__sunriset__( year, month, day, lon, lat, -6.0, 0, start, end )

/* This macro computes the start and end times of nautical twilight.	*/
/* Nautical twilight starts/ends when the Sun's center is 12 degrees	*/
/* below the horizon.												   */
#define nautical_twilight(year,month,day,lon,lat,start,end)  \
		__sunriset__( year, month, day, lon, lat, -12.0, 0, start, end )

/* This macro computes the start and end times of astronomical twilight.   */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
/* below the horizon.													  */
#define astronomical_twilight(year,month,day,lon,lat,start,end)  \
		__sunriset__( year, month, day, lon, lat, -18.0, 0, start, end )

boost::posix_time::time_duration get_utc_offset() {
	using namespace boost::posix_time;

	// boost::date_time::c_local_adjustor uses the C-API to adjust a
	// moment given in utc to the same moment in the local time zone.
	typedef boost::date_time::c_local_adjustor<ptime> local_adj;

	const ptime utc_now = second_clock::universal_time();
	const ptime now = local_adj::utc_to_local(utc_now);

	return now - utc_now;
}

bool SunRiseSet::GetSunRiseSet(const double latit, const double longit, _tSubRiseSetResults &result)
{
	time_t sekunnit;
	/** First get current time **/
	time(&sekunnit);
	struct tm ltime;
	localtime_r(&sekunnit,&ltime);
	// this is Y2K compliant algorithm
	int year = 1900 + ltime.tm_year;
	int month = ltime.tm_mon + 1;
	int day = ltime.tm_mday;
	return GetSunRiseSet(latit, longit, year,month,day,result);
}

bool SunRiseSet::GetSunRiseSet(const double latit, const double longit, const int year, const int month, const int day, _tSubRiseSetResults &result)
{
	result.latit=latit;
	result.longit=longit;
	result.year=year;
	result.month=month;
	result.day=day;

	boost::posix_time::time_duration uoffset=get_utc_offset();
	double timezone=(double)(uoffset.ticks()/3600000000LL);
	// Assuming we now got the diff in hours and minutes here. Do we?

	double daylen; //, civlen, nautlen, astrlen;
	double rise, set, civ_start, civ_end, naut_start, naut_end, astr_start, astr_end;
	int rs, civ, naut, astr;

	daylen = day_length(year,month,day,longit,latit);
	//civlen = day_civil_twilight_length(year,month,day,longit,latit);
	//nautlen = day_nautical_twilight_length(year,month,day,longit,latit);
	//astrlen = day_astronomical_twilight_length(year,month,day,longit,latit);

	double _tmpH;
	result.DaylengthMins = static_cast<int>(std::round(modf(daylen, &_tmpH)*60));
	result.DaylengthHours = static_cast<int>(_tmpH);

	rs = sun_rise_set(year, month, day, longit, latit, &rise, &set);
	civ = civil_twilight(year, month, day, longit, latit, &civ_start, &civ_end);
	naut = nautical_twilight(year, month, day, longit, latit, &naut_start, &naut_end);
	astr = astronomical_twilight(year, month, day, longit, latit, &astr_start, &astr_end);

	rise = UtcToLocal(rise, timezone);
	set = UtcToLocal(set, timezone);
	result.SunAtSouthMin = static_cast<int>(std::round(modf((rise+set)/2.0, &_tmpH)*60));
	result.SunAtSouthHour = static_cast<int>(_tmpH);

	switch(rs) {
		case 0:
			result.SunRiseMin = static_cast<int>(std::round(modf(rise, &_tmpH)*60));
			result.SunRiseHour = static_cast<int>(_tmpH);
			result.SunSetMin = static_cast<int>(std::round(modf(set, &_tmpH)*60));
			result.SunSetHour = static_cast<int>(_tmpH);
			//fix a possible rounding issue above
			if (result.SunRiseMin > 59)
			{
				result.SunRiseMin = 0;
				result.SunRiseHour = (result.SunRiseHour + 1) % 24;
			}
			if (result.SunSetMin > 59)
			{
				result.SunSetMin = 0;
				result.SunSetHour = (result.SunSetHour + 1) % 24;
			}
			break;
		case +1:
		case -1:
			// Sun below/above horizon in the space of 24 hours
			result.SunRiseMin = 0; // Which is actually not true, but it works like previous version.
			result.SunRiseHour = 0;
			result.SunSetMin = 0;
			result.SunSetHour = 0;
			break;
	}

	switch(civ) {
		case 0:
			civ_start = UtcToLocal(civ_start, timezone);
			civ_end = UtcToLocal(civ_end, timezone);
			result.CivilTwilightStartMin = static_cast<int>(std::round(modf(civ_start, &_tmpH)*60));
			result.CivilTwilightStartHour = static_cast<int>(_tmpH);
			result.CivilTwilightEndMin = static_cast<int>(std::round(modf(civ_end, &_tmpH)*60));
			result.CivilTwilightEndHour = static_cast<int>(_tmpH);
			break;
		case +1:
		case -1:
			// Never as bright/darker than civil twilight
			result.CivilTwilightStartMin = 0;
			result.CivilTwilightStartHour = 0;
			result.CivilTwilightEndMin = 0;
			result.CivilTwilightEndHour = 0;
			break;
	}

	switch(naut) {
		case 0:
			naut_start = UtcToLocal(naut_start, timezone);
			naut_end = UtcToLocal(naut_end, timezone);
			result.NauticalTwilightStartMin = static_cast<int>(std::round(modf(naut_start, &_tmpH)*60));
			result.NauticalTwilightStartHour = static_cast<int>(_tmpH);
			result.NauticalTwilightEndMin = static_cast<int>(std::round(modf(naut_end, &_tmpH)*60));
			result.NauticalTwilightEndHour = static_cast<int>(_tmpH);
			break;
		case +1:
		case -1:
			// Never as bright/darker than nautical twilight
			result.NauticalTwilightStartMin = 0;
			result.NauticalTwilightStartHour = 0;
			result.NauticalTwilightEndMin = 0;
			result.NauticalTwilightEndHour = 0;
			break;
	}

	switch(astr) {
		case 0:
			astr_start = UtcToLocal(astr_start, timezone);
			astr_end = UtcToLocal(astr_end, timezone);
			result.AstronomicalTwilightStartMin = static_cast<int>(std::round(modf(astr_start, &_tmpH)*60));
			result.AstronomicalTwilightStartHour = static_cast<int>(_tmpH);
			result.AstronomicalTwilightEndMin = static_cast<int>(std::round(modf(astr_end, &_tmpH)*60));
			result.AstronomicalTwilightEndHour = static_cast<int>(_tmpH);
			break;
		case +1:
		case -1:
			// Never as bright/darker than astronomical twilight
			result.AstronomicalTwilightStartMin = 0;
			result.AstronomicalTwilightStartHour = 0;
			result.AstronomicalTwilightEndMin = 0;
			result.AstronomicalTwilightEndHour = 0;
			break;
	}

	return true;
}
/* The "workhorse" function for sun rise/set times */

int SunRiseSet::__sunriset__( int year, int month, int day, double lon, double lat,
                  double altit, int upper_limb, double *trise, double *tset )
/***************************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value IS critical in this function!            */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing rise/set     */
/*               times, and to zero when computing start/end of       */
/*               twilight.                                            */
/*        *rise = where to store the rise time                        */
/*        *set  = where to store the set  time                        */
/*                Both times are relative to the specified altitude,  */
/*                and thus this function can be used to compute       */
/*                various twilight times, as well as rise/set times   */
/* Return value:  0 = sun rises/sets this day, times stored at        */
/*                    *trise and *tset.                               */
/*               +1 = sun above the specified "horizon" 24 hours.     */
/*                    *trise set to time when the sun is at south,    */
/*                    minus 12 hours while *tset is set to the south  */
/*                    time plus 12 hours. "Day" length = 24 hours     */
/*               -1 = sun is below the specified "horizon" 24 hours   */
/*                    "Day" length = 0 hours, *trise and *tset are    */
/*                    both set to the time when the sun is at south.  */
/*                                                                    */
/**********************************************************************/
{
	double  d,  /* Days since 2000 Jan 0.0 (negative before) */
	sr,         /* Solar distance, astronomical units */
	sRA,        /* Sun's Right Ascension */
	sdec,       /* Sun's declination */
	sradius,    /* Sun's apparent radius */
	t,          /* Diurnal arc */
	tsouth,     /* Time when Sun is at south */
	sidtime;    /* Local sidereal time */

	int rc = 0; /* Return cde from function - usually 0 */

	/* Compute d of 12h local mean solar time */
	d = days_since_2000_Jan_0(year,month,day) + 0.5 - lon/360.0;

	/* Compute the local sidereal time of this moment */
	sidtime = revolution(GMST0(d) + 180.0 + lon);

	/* Compute Sun's RA, Decl and distance at this moment */
	sun_RA_dec( d, &sRA, &sdec, &sr );

	/* Compute time when Sun is at south - in hours UT */
	tsouth = 12.0 - rev180(sidtime - sRA)/15.0;

	/* Compute the Sun's apparent radius in degrees */
	sradius = 0.2666 / sr;

	/* Do correction to upper limb, if necessary */
	if ( upper_limb )
		altit -= sradius;

	/* Compute the diurnal arc that the Sun traverses to reach */
	/* the specified altitude altit: */
	{
		double cost;
		cost = ( sind(altit) - sind(lat) * sind(sdec) ) /
		( cosd(lat) * cosd(sdec) );
		if ( cost >= 1.0 )
		rc = -1, t = 0.0;       /* Sun always below altit */
		else if ( cost <= -1.0 )
		rc = +1, t = 12.0;      /* Sun always above altit */
		else
		t = acosd(cost)/15.0;   /* The diurnal arc, hours */
	}

	/* Store rise and set times - in hours UT */
	*trise = tsouth - t;
	*tset  = tsouth + t;

	return rc;
}	/* __sunriset__ */


/* The "workhorse" function */


double SunRiseSet::__daylen__( int year, int month, int day, double lon, double lat, double altit, int upper_limb )
/**********************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value is not critical. Set it to the correct   */
/*       longitude if you're picky, otherwise set to to, say, 0.0     */
/*       The latitude however IS critical - be sure to get it correct */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing day length   */
/*               and to zero when computing day+twilight length.      */
/**********************************************************************/
{
	double  d,	/* Days since 2000 Jan 0.0 (negative before) */
	obl_ecl,	/* Obliquity (inclination) of Earth's axis */
	sr,			/* Solar distance, astronomical units */
	slon,		/* True solar longitude */
	sin_sdecl,	/* Sine of Sun's declination */
	cos_sdecl,	/* Cosine of Sun's declination */
	sradius,	/* Sun's apparent radius */
	t;			/* Diurnal arc */

	/* Compute d of 12h local mean solar time */
	d = days_since_2000_Jan_0(year,month,day) + 0.5 - lon/360.0;

	/* Compute obliquity of ecliptic (inclination of Earth's axis) */
	obl_ecl = 23.4393 - 3.563E-7 * d;

	/* Compute Sun's ecliptic longitude and distance */
	sunpos( d, &slon, &sr );

	/* Compute sine and cosine of Sun's declination */
	sin_sdecl = sind(obl_ecl) * sind(slon);
	cos_sdecl = sqrt( 1.0 - sin_sdecl * sin_sdecl );

	/* Compute the Sun's apparent radius, degrees */
	sradius = 0.2666 / sr;

	/* Do correction to upper limb, if necessary */
	if ( upper_limb )
	altit -= sradius;

	/* Compute the diurnal arc that the Sun traverses to reach */
	/* the specified altitude altit: */
	{
		double cost;
		cost = ( sind(altit) - sind(lat) * sin_sdecl ) /
				( cosd(lat) * cos_sdecl );
		if ( cost >= 1.0 )
			t = 0.0;	/* Sun always below altit */
		else if ( cost <= -1.0 )
			t = 24.0;	/* Sun always above altit */
		else t = (2.0/15.0) * acosd(cost); /* The diurnal arc, hours */
	}
	return t;
}	/* __daylen__ */


/* This function computes the Sun's position at any instant */

void SunRiseSet::sunpos( double d, double *lon, double *r )
/******************************************************/
/* Computes the Sun's ecliptic longitude and distance */
/* at an instant given in d, number of days since     */
/* 2000 Jan 0.0.  The Sun's ecliptic latitude is not  */
/* computed, since it's always very near 0.           */
/******************************************************/
{
	double	M,		/* Mean anomaly of the Sun */
			w,		/* Mean longitude of perihelion */
					/* Note: Sun's mean longitude = M + w */
			e,		/* Eccentricity of Earth's orbit */
			E,		/* Eccentric anomaly */
			x, y,	/* x, y coordinates in orbit */
			v;		/* True anomaly */

	/* Compute mean elements */
	M = revolution(356.0470 + 0.9856002585 * d);
	w = 282.9404 + 4.70935E-5 * d;
	e = 0.016709 - 1.151E-9 * d;

	/* Compute true longitude and radius vector */
	E = M + e * RADEG * sind(M) * ( 1.0 + e * cosd(M) );
	x = cosd(E) - e;
	y = sqrt( 1.0 - e*e ) * sind(E);
	*r = sqrt( x*x + y*y );              /* Solar distance */
	v = atan2d( y, x );                  /* True anomaly */
	*lon = v + w;                        /* True solar longitude */
	if ( *lon >= 360.0 )
	*lon -= 360.0;                   /* Make it 0..360 degrees */
}

void SunRiseSet::sun_RA_dec( double d, double *RA, double *dec, double *r )
/******************************************************/
/* Computes the Sun's equatorial coordinates RA, Decl */
/* and also its distance, at an instant given in d,   */
/* the number of days since 2000 Jan 0.0.             */
/******************************************************/
{
	double lon, obl_ecl, x, y, z;

	/* Compute Sun's ecliptical coordinates */
	sunpos( d, &lon, r );

	/* Compute ecliptic rectangular coordinates (z=0) */
	x = *r * cosd(lon);
	y = *r * sind(lon);

	/* Compute obliquity of ecliptic (inclination of Earth's axis) */
	obl_ecl = 23.4393 - 3.563E-7 * d;

	/* Convert to equatorial rectangular coordinates - x is unchanged */
	z = y * sind(obl_ecl);
	y = y * cosd(obl_ecl);

	/* Convert to spherical coordinates */
	*RA = atan2d( y, x );
	*dec = atan2d( z, sqrt(x*x + y*y) );

}	/* sun_RA_dec */


/******************************************************************/
/* This function reduces any angle to within the first revolution */
/* by subtracting or adding even multiples of 360.0 until the     */
/* result is >= 0.0 and < 360.0                                   */
/******************************************************************/

#define INV360    ( 1.0 / 360.0 )

double SunRiseSet::revolution( double x )
/*****************************************/
/* Reduce angle to within 0..360 degrees */
/*****************************************/
{
	return( x - 360.0 * floor( x * INV360 ) );
}	/* revolution */

double SunRiseSet::UtcToLocal(double time, double tz)
/*********************************************/
/* Add Time Zone difference to time          */
/*********************************************/
{
	time += tz;
	return((time > 24.0) ? time-24.0 : (time < 0.0) ? time+24.0 : time);
}

double SunRiseSet::rev180( double x )
/*********************************************/
/* Reduce angle to within +180..+180 degrees */
/*********************************************/
{
	return( x - 360.0 * floor( x * INV360 + 0.5 ) );
}	/* revolution */

/*******************************************************************/
/* This function computes GMST0, the Greenwich Mean Sidereal Time  */
/* at 0h UT (i.e. the sidereal time at the Greenwhich meridian at  */
/* 0h UT).  GMST is then the sidereal time at Greenwich at any     */
/* time of the day.  I've generalized GMST0 as well, and define it */
/* as:  GMST0 = GMST - UT  --  this allows GMST0 to be computed at */
/* other times than 0h UT as well.  While this sounds somewhat     */
/* contradictory, it is very practical:  instead of computing      */
/* GMST like:                                                      */
/*                                                                 */
/*  GMST = (GMST0) + UT * (366.2422/365.2422)                      */
/*                                                                 */
/* where (GMST0) is the GMST last time UT was 0 hours, one simply  */
/* computes:                                                       */
/*                                                                 */
/*  GMST = GMST0 + UT                                              */
/*                                                                 */
/* where GMST0 is the GMST "at 0h UT" but at the current moment!   */
/* Defined in this way, GMST0 will increase with about 4 min a     */
/* day.  It also happens that GMST0 (in degrees, 1 hr = 15 degr)   */
/* is equal to the Sun's mean longitude plus/minus 180 degrees!    */
/* (if we neglect aberration, which amounts to 20 seconds of arc   */
/* or 1.33 seconds of time)                                        */
/*                                                                 */
/*******************************************************************/

double SunRiseSet::GMST0(double d)
{
	double sidtim0;
	/* Sidtime at 0h UT = L (Sun's mean longitude) + 180.0 degr  */
	/* L = M + w, as defined in sunpos().  Since I'm too lazy to */
	/* add these numbers, I'll let the C compiler do it for me.  */
	/* Any decent C compiler will add the constants at compile   */
	/* time, imposing no runtime or code overhead.               */
	sidtim0 = revolution( ( 180.0 + 356.0470 + 282.9404 ) +
			( 0.9856002585 + 4.70935E-5 ) * d );
	return sidtim0;
}	/* GMST0 */

/*
 * Part of this code was provided by Paul Schlyter
 * Here is the original copyright information
 * Written as DAYLEN.C, 1989-08-16
 * Modified to SUNRISET.C, 1992-12-01
 * (c) Paul Schlyter, 1989, 1992
 * Released to the public domain by Paul Schlyter, December 1992
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
