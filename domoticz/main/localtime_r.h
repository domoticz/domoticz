#pragma once

#include <time.h>

#ifndef localtime_r
	struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif

//Time helper, to make sure time does not go away (for systems without a RTC)

time_t mytime(time_t * _Time);

