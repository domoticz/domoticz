#include "stdafx.h"
#include "localtime_r.h"

#ifndef localtime_r
struct tm *localtime_r(const time_t *timep, struct tm *result)
{
#ifdef localtime_s
	localtime_s(timep, result);
#else
	struct tm *s = localtime (timep);
	if (s == NULL)
		return NULL;
	*result = *s;
#endif
	return result;
}
#endif

