#pragma once

#include <time.h>

#ifndef localtime_r
	struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif

//Time helper, to make sure time does not go away (for systems without a RTC)

time_t mytime(time_t * _Time);

// DST safe SQL datetime string parser
bool ParseSQLdatetime(time_t &time, struct tm &result, const std::string &szSQLdate);
bool ParseSQLdatetime(time_t &time, struct tm &result, const std::string &szSQLdate, int isdst);


// DST safe datetime constructors
bool getMidnight(time_t &time, struct tm &result);
bool getMidnight(time_t &time, struct tm &result, int year, int month, int day);

bool getNoon(time_t &time, struct tm &result);
bool getNoon(time_t &time, struct tm &result, int year, int month, int day);

bool constructTime(time_t &time, struct tm &result, const int year, const int month, const int day, const int hour, const int minute, const int second);
bool constructTime(time_t &time, struct tm &result, const int year, const int month, const int day, const int hour, const int minute, const int second, int isdst);
