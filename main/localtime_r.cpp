#include "stdafx.h"
#include "localtime_r.h"

time_t m_lasttime=time(NULL);
boost::mutex TimeMutex_;

#ifdef __APPLE__
	#define localtime_r
#endif

#ifndef localtime_r
struct tm *localtime_r(const time_t *timep, struct tm *result)
{
#ifdef localtime_s
	localtime_s(timep, result);
#else
	boost::mutex::scoped_lock lock(TimeMutex_);
	struct tm *s = localtime(timep);
	if (s == NULL)
		return NULL;
	memcpy(result, s, sizeof(struct tm));
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
