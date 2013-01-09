#pragma once

#include <time.h>

#ifndef localtime_r
	struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif