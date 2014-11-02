#include "stdafx.h"
#include "localtime_r.h"

time_t m_lasttime=0;

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

time_t mytime(time_t * _Time)
{
	time_t acttime=time(_Time);
	if (acttime<m_lasttime)
		return m_lasttime;
	m_lasttime=acttime;
	return acttime;
}
