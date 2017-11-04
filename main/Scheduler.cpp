#include "stdafx.h"
#include <iostream>
#include "Scheduler.h"
#include "localtime_r.h"
#include "Logger.h"
#include "Helper.h"
#include "SQLHelper.h"
#include "mainworker.h"
#include "WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

CScheduler::CScheduler(void)
{
	m_tSunRise = 0;
	m_tSunSet = 0;
	m_stoprequested = false;
	srand((int)mytime(NULL));
}

CScheduler::~CScheduler(void)
{
}

void CScheduler::StartScheduler()
{
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CScheduler::Do_Work, this)));
}

void CScheduler::StopScheduler()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
	}
}

std::vector<tScheduleItem> CScheduler::GetScheduleItems()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	std::vector<tScheduleItem> ret;

	std::vector<tScheduleItem>::iterator itt;
	for (itt = m_scheduleitems.begin(); itt != m_scheduleitems.end(); ++itt)
	{
		ret.push_back(*itt);
	}
	return ret;
}

void CScheduler::ReloadSchedules()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	m_scheduleitems.clear();

	std::vector<std::vector<std::string> > result;

	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	//Add Device Timers
	result = m_sql.safe_query(
		"SELECT T1.DeviceRowID, T1.Time, T1.Type, T1.Cmd, T1.Level, T1.Days, T2.Name,"
		" T2.Used, T1.UseRandomness, T1.Hue, T1.[Date], T1.MDay, T1.Month, T1.Occurence, T1.ID"
		" FROM Timers as T1, DeviceStatus as T2"
		" WHERE ((T1.Active == 1) AND (T1.TimerPlan == %d) AND (T2.ID == T1.DeviceRowID))"
		" ORDER BY T1.ID",
		m_sql.m_ActiveTimerPlan);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			bool bDUsed = (atoi(sd[7].c_str()) != 0);

			if (bDUsed == true)
			{
				tScheduleItem titem;

				titem.bEnabled = true;
				titem.bIsScene = false;
				titem.bIsThermostat = false;

				_eTimerType timerType = (_eTimerType)atoi(sd[2].c_str());

				{
					std::stringstream s_str(sd[0]);
					s_str >> titem.RowID; }
				{
					std::stringstream s_str(sd[14]);
					s_str >> titem.TimerID; }

				titem.startHour = (unsigned char)atoi(sd[1].substr(0, 2).c_str());
				titem.startMin = (unsigned char)atoi(sd[1].substr(3, 2).c_str());
				titem.startTime = 0;
				titem.timerType = timerType;
				titem.timerCmd = (_eTimerCommand)atoi(sd[3].c_str());
				titem.Level = (unsigned char)atoi(sd[4].c_str());
				titem.bUseRandomness = (atoi(sd[8].c_str()) != 0);
				titem.Hue = atoi(sd[9].c_str());
				titem.MDay = 0;
				titem.Month = 0;
				titem.Occurence = 0;

				if (timerType == TTYPE_FIXEDDATETIME)
				{
					std::string sdate = sd[10];
					if (sdate.size() != 10)
						continue; //invalid
					titem.startYear = (unsigned short)atoi(sdate.substr(0, 4).c_str());
					titem.startMonth = (unsigned char)atoi(sdate.substr(5, 2).c_str());
					titem.startDay = (unsigned char)atoi(sdate.substr(8, 2).c_str());
				}
				else if (timerType == TTYPE_MONTHLY)
				{
					std::string smday = sd[11];
					if (smday == "0")
						continue; //invalid
					titem.MDay = atoi(smday.c_str());
				}
				else if (timerType == TTYPE_MONTHLY_WD)
				{
					std::string socc = sd[13];
					if (socc == "0")
						continue; //invalid
					titem.Occurence = atoi(socc.c_str());
				}
				else if (timerType == TTYPE_YEARLY)
				{
					std::string smday = sd[11];
					std::string smonth = sd[12];
					if ((smday == "0") || (smonth == "0"))
						continue; //invalid
					titem.MDay = atoi(smday.c_str());
					titem.Month = atoi(smonth.c_str());
				}
				else if (timerType == TTYPE_YEARLY_WD)
				{
					std::string smonth = sd[12];
					std::string socc = sd[13];
					if ((smonth == "0") || (socc == "0"))
						continue; //invalid
					titem.Month = atoi(smonth.c_str());
					titem.Occurence = atoi(socc.c_str());
				}

				if ((titem.timerCmd == TCMD_ON) && (titem.Level == 0))
				{
					titem.Level = 100;
				}
				titem.Days = atoi(sd[5].c_str());
				titem.DeviceName = sd[6];
				if (AdjustScheduleItem(&titem, false) == true)
					m_scheduleitems.push_back(titem);
			}
			else
			{
				//not used? delete it
				m_sql.safe_query("DELETE FROM Timers WHERE (DeviceRowID == '%q')",
					sd[0].c_str());
			}
		}
	}

	//Add Scene Timers
	result = m_sql.safe_query(
		"SELECT T1.SceneRowID, T1.Time, T1.Type, T1.Cmd, T1.Level, T1.Days, T2.Name,"
		" T1.UseRandomness, T1.[Date], T1.MDay, T1.Month, T1.Occurence, T1.ID"
		" FROM SceneTimers as T1, Scenes as T2"
		" WHERE ((T1.Active == 1) AND (T1.TimerPlan == %d) AND (T2.ID == T1.SceneRowID))"
		" ORDER BY T1.ID",
		m_sql.m_ActiveTimerPlan);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			tScheduleItem titem;

			titem.bEnabled = true;
			titem.bIsScene = true;
			titem.bIsThermostat = false;
			titem.MDay = 0;
			titem.Month = 0;
			titem.Occurence = 0;

			{
				std::stringstream s_str(sd[0]);
				s_str >> titem.RowID; }
			{
				std::stringstream s_str(sd[12]);
				s_str >> titem.TimerID; }

			_eTimerType timerType = (_eTimerType)atoi(sd[2].c_str());

			if (timerType == TTYPE_FIXEDDATETIME)
			{
				std::string sdate = sd[8];
				if (sdate.size() != 10)
					continue; //invalid
				titem.startYear = (unsigned short)atoi(sdate.substr(0, 4).c_str());
				titem.startMonth = (unsigned char)atoi(sdate.substr(5, 2).c_str());
				titem.startDay = (unsigned char)atoi(sdate.substr(8, 2).c_str());
			}
			else if (timerType == TTYPE_MONTHLY)
			{
				std::string smday = sd[9];
				if (smday == "0")
					continue; //invalid
				titem.MDay = atoi(smday.c_str());
			}
			else if (timerType == TTYPE_MONTHLY_WD)
			{
				std::string socc = sd[11];
				if (socc == "0")
					continue; //invalid
				titem.Occurence = atoi(socc.c_str());
			}
			else if (timerType == TTYPE_YEARLY)
			{
				std::string smday = sd[9];
				std::string smonth = sd[10];
				if ((smday == "0") || (smonth == "0"))
					continue; //invalid
				titem.MDay = atoi(smday.c_str());
				titem.Month = atoi(smonth.c_str());
			}
			else if (timerType == TTYPE_YEARLY_WD)
			{
				std::string smonth = sd[10];
				std::string socc = sd[11];
				if ((smonth == "0") || (socc == "0"))
					continue; //invalid
				titem.Month = atoi(smonth.c_str());
				titem.Occurence = atoi(socc.c_str());
			}

			titem.startHour = (unsigned char)atoi(sd[1].substr(0, 2).c_str());
			titem.startMin = (unsigned char)atoi(sd[1].substr(3, 2).c_str());
			titem.startTime = 0;
			titem.timerType = timerType;
			titem.timerCmd = (_eTimerCommand)atoi(sd[3].c_str());
			titem.Level = (unsigned char)atoi(sd[4].c_str());
			titem.bUseRandomness = (atoi(sd[7].c_str()) != 0);
			if ((titem.timerCmd == TCMD_ON) && (titem.Level == 0))
			{
				titem.Level = 100;
			}
			titem.Days = atoi(sd[5].c_str());
			titem.DeviceName = sd[6];
			if (AdjustScheduleItem(&titem, false) == true)
				m_scheduleitems.push_back(titem);
		}
	}

	//Add Setpoint Timers
	result = m_sql.safe_query(
		"SELECT T1.DeviceRowID, T1.Time, T1.Type, T1.Temperature, T1.Days, T2.Name,"
		" T1.[Date], T1.MDay, T1.Month, T1.Occurence, T1.ID"
		" FROM SetpointTimers as T1, DeviceStatus as T2"
		" WHERE ((T1.Active == 1) AND (T1.TimerPlan == %d) AND (T2.ID == T1.DeviceRowID))"
		" ORDER BY T1.ID",
		m_sql.m_ActiveTimerPlan);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			tScheduleItem titem;

			titem.bEnabled = true;
			titem.bIsScene = false;
			titem.bIsThermostat = true;
			titem.MDay = 0;
			titem.Month = 0;
			titem.Occurence = 0;

			{
				std::stringstream s_str(sd[0]);
				s_str >> titem.RowID; }
			{
				std::stringstream s_str(sd[10]);
				s_str >> titem.TimerID; }

			_eTimerType timerType = (_eTimerType)atoi(sd[2].c_str());

			if (timerType == TTYPE_FIXEDDATETIME)
			{
				std::string sdate = sd[6];
				if (sdate.size() != 10)
					continue; //invalid
				titem.startYear = (unsigned short)atoi(sdate.substr(0, 4).c_str());
				titem.startMonth = (unsigned char)atoi(sdate.substr(5, 2).c_str());
				titem.startDay = (unsigned char)atoi(sdate.substr(8, 2).c_str());
			}
			else if (timerType == TTYPE_MONTHLY)
			{
				std::string smday = sd[7];
				if (smday == "0")
					continue; //invalid
				titem.MDay = atoi(smday.c_str());
			}
			else if (timerType == TTYPE_MONTHLY_WD)
			{
				std::string socc = sd[9];
				if (socc == "0")
					continue; //invalid
				titem.Occurence = atoi(socc.c_str());
			}
			else if (timerType == TTYPE_YEARLY)
			{
				std::string smday = sd[7];
				std::string smonth = sd[8];
				if ((smday == "0") || (smonth == "0"))
					continue; //invalid
				titem.MDay = atoi(smday.c_str());
				titem.Month = atoi(smonth.c_str());
			}
			else if (timerType == TTYPE_YEARLY_WD)
			{
				std::string smonth = sd[8];
				std::string socc = sd[9];
				if ((smonth == "0") || (socc == "0"))
					continue; //invalid
				titem.Month = atoi(smonth.c_str());
				titem.Occurence = atoi(socc.c_str());
			}

			titem.startHour = (unsigned char)atoi(sd[1].substr(0, 2).c_str());
			titem.startMin = (unsigned char)atoi(sd[1].substr(3, 2).c_str());
			titem.startTime = 0;
			titem.timerType = timerType;
			titem.timerCmd = TCMD_ON;
			titem.Temperature = static_cast<float>(atof(sd[3].c_str()));
			titem.Level = 100;
			titem.bUseRandomness = false;
			titem.Days = atoi(sd[4].c_str());
			titem.DeviceName = sd[5];
			if (AdjustScheduleItem(&titem, false) == true)
				m_scheduleitems.push_back(titem);
		}
	}
}

void CScheduler::SetSunRiseSetTimers(const std::string &sSunRise, const std::string &sSunSet)
{
	bool bReloadSchedules = false;
	{	//needed private scope for the lock
		boost::lock_guard<boost::mutex> l(m_mutex);
		int hour, min, sec;

		time_t temptime;
		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);

		hour = atoi(sSunRise.substr(0, 2).c_str());
		min = atoi(sSunRise.substr(3, 2).c_str());
		sec = atoi(sSunRise.substr(6, 2).c_str());

		struct tm tm1;
		constructTime(temptime,tm1,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,hour,min,sec,ltime.tm_isdst);
		if ((m_tSunRise != temptime) && (temptime != 0))
		{
			if (m_tSunRise == 0)
				bReloadSchedules = true;
			m_tSunRise = temptime;
		}

		hour = atoi(sSunSet.substr(0, 2).c_str());
		min = atoi(sSunSet.substr(3, 2).c_str());
		sec = atoi(sSunSet.substr(6, 2).c_str());

		constructTime(temptime,tm1,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,hour,min,sec,ltime.tm_isdst);
		if ((m_tSunSet != temptime) && (temptime != 0))
		{
			if (m_tSunSet == 0)
				bReloadSchedules = true;
			m_tSunSet = temptime;
		}
	}
	if (bReloadSchedules)
		ReloadSchedules();
}

bool CScheduler::AdjustScheduleItem(tScheduleItem *pItem, bool bForceAddDay)
{
	time_t atime = mytime(NULL);
	time_t rtime = atime;
	struct tm ltime;
	localtime_r(&atime, &ltime);
	int isdst = ltime.tm_isdst;
	struct tm tm1;
	tm1.tm_isdst = -1;

	unsigned long HourMinuteOffset = (pItem->startHour * 3600) + (pItem->startMin * 60);

	int nRandomTimerFrame = 15;
	m_sql.GetPreferencesVar("RandomTimerFrame", nRandomTimerFrame);
	if (nRandomTimerFrame == 0)
		nRandomTimerFrame = 15;
	int roffset = 0;
	if (pItem->bUseRandomness)
	{
		if ((pItem->timerType == TTYPE_BEFORESUNRISE) ||
			(pItem->timerType == TTYPE_AFTERSUNRISE) ||
			(pItem->timerType == TTYPE_BEFORESUNSET) ||
			(pItem->timerType == TTYPE_AFTERSUNSET))
			roffset = rand() % (nRandomTimerFrame);
		else
			roffset = rand() % (nRandomTimerFrame * 2) - nRandomTimerFrame;
	}

	if ((pItem->timerType == TTYPE_ONTIME) ||
		(pItem->timerType == TTYPE_DAYSODD) ||
		(pItem->timerType == TTYPE_DAYSEVEN) ||
		(pItem->timerType == TTYPE_WEEKSODD) ||
		(pItem->timerType == TTYPE_WEEKSEVEN))
	{
		constructTime(rtime,tm1,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,pItem->startHour,pItem->startMin,roffset*60,isdst);
	}
	else if (pItem->timerType == TTYPE_FIXEDDATETIME)
	{
		constructTime(rtime,tm1,pItem->startYear,pItem->startMonth,pItem->startDay,pItem->startHour,pItem->startMin,roffset*60,isdst);
		if (rtime < atime)
			return false; //past date/time
		pItem->startTime = rtime;
		return true;
	}
	else if (pItem->timerType == TTYPE_BEFORESUNSET)
	{
		if (m_tSunSet == 0)
			return false;
		rtime = m_tSunSet - HourMinuteOffset - (roffset * 60);
	}
	else if (pItem->timerType == TTYPE_AFTERSUNSET)
	{
		if (m_tSunSet == 0)
			return false;
		rtime = m_tSunSet + HourMinuteOffset + (roffset * 60);
	}
	else if (pItem->timerType == TTYPE_BEFORESUNRISE)
	{
		if (m_tSunRise == 0)
			return false;
		rtime = m_tSunRise - HourMinuteOffset - (roffset * 60);
	}
	else if (pItem->timerType == TTYPE_AFTERSUNRISE)
	{
		if (m_tSunRise == 0)
			return false;
		rtime = m_tSunRise + HourMinuteOffset + (roffset * 60);
	}
	else if (pItem->timerType == TTYPE_MONTHLY)
	{
		constructTime(rtime,tm1,ltime.tm_year+1900,ltime.tm_mon+1,pItem->MDay,pItem->startHour,pItem->startMin,0,isdst);

		while ( (rtime < atime) || (tm1.tm_mday != pItem->MDay) ) // past date/time OR mday exceeds max days in month
		{
			ltime.tm_mon++;
			constructTime(rtime,tm1,ltime.tm_year+1900,ltime.tm_mon+1,pItem->MDay,pItem->startHour,pItem->startMin,0,isdst);
		}

		rtime += roffset * 60; // add randomness
		pItem->startTime = rtime;
		return true;
	}
	else if (pItem->timerType == TTYPE_MONTHLY_WD)
	{
		//pItem->Days: mon=1 .. sat=32, sun=64
		//convert to : sun=0, mon=1 .. sat=6
		int daynum = (int)log2(pItem->Days) + 1;
		if (daynum == 7) daynum = 0;

		boost::gregorian::nth_day_of_the_week_in_month::week_num Occurence = static_cast<boost::gregorian::nth_day_of_the_week_in_month::week_num>(pItem->Occurence);
		boost::gregorian::greg_weekday::weekday_enum Day = static_cast<boost::gregorian::greg_weekday::weekday_enum>(daynum);
		boost::gregorian::months_of_year Month = static_cast<boost::gregorian::months_of_year>(ltime.tm_mon + 1);

		typedef boost::gregorian::nth_day_of_the_week_in_month nth_dow;
		nth_dow ndm(Occurence, Day, Month);
		boost::gregorian::date d = ndm.get_date(ltime.tm_year + 1900);

		constructTime(rtime,tm1,ltime.tm_year+1900,ltime.tm_mon+1,d.day(),pItem->startHour,pItem->startMin,0,isdst);

		if (rtime < atime) //past date/time
		{
			//schedule for next month
			ltime.tm_mon++;
			if (ltime.tm_mon == 12) // fix for roll over to next year
			{
				ltime.tm_mon = 0;
				ltime.tm_year++;
			}
			Month = static_cast<boost::gregorian::months_of_year>(ltime.tm_mon + 1);
			nth_dow ndm(Occurence, Day, Month);
			boost::gregorian::date d = ndm.get_date(ltime.tm_year + 1900);

			constructTime(rtime,tm1,ltime.tm_year+1900,ltime.tm_mon,d.day(),pItem->startHour,pItem->startMin,0,isdst);
		}

		rtime += roffset * 60; // add randomness
		pItem->startTime = rtime;
		return true;
	}
	else if (pItem->timerType == TTYPE_YEARLY)
	{
		constructTime(rtime,tm1,ltime.tm_year+1900,pItem->Month,pItem->MDay,pItem->startHour,pItem->startMin,0,isdst);

		while ( (rtime < atime) || (tm1.tm_mday != pItem->MDay) ) // past date/time OR mday exceeds max days in month
		{
			//schedule for next year
			ltime.tm_year++;
			constructTime(rtime,tm1,ltime.tm_year+1900,pItem->Month,pItem->MDay,pItem->startHour,pItem->startMin,0,isdst);
		}

		rtime += roffset * 60; // add randomness
		pItem->startTime = rtime;
		return true;
	}
	else if (pItem->timerType == TTYPE_YEARLY_WD)
	{
		//pItem->Days: mon=1 .. sat=32, sun=64
		//convert to : sun=0, mon=1 .. sat=6
		int daynum = (int)log2(pItem->Days) + 1;
		if (daynum == 7) daynum = 0;

		boost::gregorian::nth_day_of_the_week_in_month::week_num Occurence = static_cast<boost::gregorian::nth_day_of_the_week_in_month::week_num>(pItem->Occurence);
		boost::gregorian::greg_weekday::weekday_enum Day = static_cast<boost::gregorian::greg_weekday::weekday_enum>(daynum);
		boost::gregorian::months_of_year Month = static_cast<boost::gregorian::months_of_year>(ltime.tm_mon + 1);

		typedef boost::gregorian::nth_day_of_the_week_in_month nth_dow;
		nth_dow ndm(Occurence, Day, Month);
		boost::gregorian::date d = ndm.get_date(ltime.tm_year + 1900);

		constructTime(rtime, tm1, ltime.tm_year + 1900, pItem->Month, d.day(), pItem->startHour, pItem->startMin, 0, isdst);

		if (rtime < atime) //past date/time
		{
			//schedule for next year
			ltime.tm_year++;
			constructTime(rtime, tm1, ltime.tm_year + 1900, pItem->Month, d.day(), pItem->startHour, pItem->startMin, 0, isdst);
		}

		rtime += roffset * 60; // add randomness
		pItem->startTime = rtime;
		return true;
	}
	else
		return false; //unknown timer type

	// Adjust timer by 1 day if item is scheduled for next day or we are in the past
        while (bForceAddDay || (rtime < atime + 60) )
        {
                if (tm1.tm_isdst == -1) // rtime was loaded from sunset/sunrise values; need to initialize tm1
		{
                        localtime_r(&rtime, &tm1);

			//FIXME: because we are referencing the wrong date for sunset/sunrise values (i.e. today)
			//	 we actually need to add 24 hours rather than a day and NOT correct for DST change
			isdst = -1;
	                tm1.tm_mday--;
			tm1.tm_hour += 24;
			// end of FIXME block
		}
                tm1.tm_mday++;
                struct tm tm2;
                constructTime(rtime, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec, isdst);
                bForceAddDay = false;
        }

	pItem->startTime = rtime;
	return true;
}

void CScheduler::Do_Work()
{
	while (!m_stoprequested)
	{
		//sleep 1 second
		sleep_seconds(1);

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);


		if (ltime.tm_sec % 12 == 0) {
			m_mainworker.HeartbeatUpdate("Scheduler");
		}

		CheckSchedules();

		if (ltime.tm_sec == 0) {
			DeleteExpiredTimers();
		}
	}
	_log.Log(LOG_STATUS, "Scheduler stopped...");
}

void CScheduler::CheckSchedules()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	std::vector<tScheduleItem>::iterator itt;
	for (itt = m_scheduleitems.begin(); itt != m_scheduleitems.end(); ++itt)
	{
		if ((itt->bEnabled) && (atime > itt->startTime))
		{
			//check if we are on a valid day
			bool bOkToFire = false;
			if (itt->timerType == TTYPE_FIXEDDATETIME)
			{
				bOkToFire = true;
			}
			else if (itt->timerType == TTYPE_DAYSODD)
			{
				bOkToFire = (ltime.tm_mday % 2 != 0);
			}
			else if (itt->timerType == TTYPE_DAYSEVEN)
			{
				bOkToFire = (ltime.tm_mday % 2 == 0);
			}
			else
			{
				if (itt->Days & 0x80)
				{
					//everyday
					bOkToFire = true;
				}
				else if (itt->Days & 0x100)
				{
					//weekdays
					if ((ltime.tm_wday > 0) && (ltime.tm_wday < 6))
						bOkToFire = true;
				}
				else if (itt->Days & 0x200)
				{
					//weekends
					if ((ltime.tm_wday == 0) || (ltime.tm_wday == 6))
						bOkToFire = true;
				}
				else
				{
					//custom days
					if ((itt->Days & 0x01) && (ltime.tm_wday == 1))
						bOkToFire = true;//Monday
					if ((itt->Days & 0x02) && (ltime.tm_wday == 2))
						bOkToFire = true;//Tuesday
					if ((itt->Days & 0x04) && (ltime.tm_wday == 3))
						bOkToFire = true;//Wednesday
					if ((itt->Days & 0x08) && (ltime.tm_wday == 4))
						bOkToFire = true;//Thursday
					if ((itt->Days & 0x10) && (ltime.tm_wday == 5))
						bOkToFire = true;//Friday
					if ((itt->Days & 0x20) && (ltime.tm_wday == 6))
						bOkToFire = true;//Saturday
					if ((itt->Days & 0x40) && (ltime.tm_wday == 0))
						bOkToFire = true;//Sunday
				}
				if (bOkToFire)
				{
					if ((itt->timerType == TTYPE_WEEKSODD) ||
						(itt->timerType == TTYPE_WEEKSEVEN))
					{
						struct tm timeinfo;
						localtime_r(&itt->startTime, &timeinfo);

						boost::gregorian::date d = boost::gregorian::date(
							timeinfo.tm_year + 1900,
							timeinfo.tm_mon + 1,
							timeinfo.tm_mday);
						int w = d.week_number();

						if (itt->timerType == TTYPE_WEEKSODD)
							bOkToFire = (w % 2 != 0);
						else
							bOkToFire = (w % 2 == 0);
					}
				}
			}
			if (bOkToFire)
			{
				char ltimeBuf[30];
				strftime(ltimeBuf, sizeof(ltimeBuf), "%Y-%m-%d %H:%M:%S", &ltime);

				if (itt->bIsScene == true)
					_log.Log(LOG_STATUS, "Schedule item started! Name: %s, Type: %s, SceneID: %" PRIu64 ", Time: %s", itt->DeviceName.c_str(), Timer_Type_Desc(itt->timerType), itt->RowID, ltimeBuf);
				else if (itt->bIsThermostat == true)
					_log.Log(LOG_STATUS, "Schedule item started! Name: %s, Type: %s, ThermostatID: %" PRIu64 ", Time: %s", itt->DeviceName.c_str(), Timer_Type_Desc(itt->timerType), itt->RowID, ltimeBuf);
				else
					_log.Log(LOG_STATUS, "Schedule item started! Name: %s, Type: %s, DevID: %" PRIu64 ", Time: %s", itt->DeviceName.c_str(), Timer_Type_Desc(itt->timerType), itt->RowID, ltimeBuf);
				std::string switchcmd = "";
				if (itt->timerCmd == TCMD_ON)
					switchcmd = "On";
				else if (itt->timerCmd == TCMD_OFF)
					switchcmd = "Off";
				if (switchcmd == "")
				{
					_log.Log(LOG_ERROR, "Unknown switch command in timer!!....");
				}
				else
				{
					if (itt->bIsScene == true)
					{
/*
						if (
							(itt->timerType == TTYPE_BEFORESUNRISE) ||
							(itt->timerType == TTYPE_AFTERSUNRISE) ||
							(itt->timerType == TTYPE_BEFORESUNSET) ||
							(itt->timerType == TTYPE_AFTERSUNSET)
							)
						{

						}
*/
						if (!m_mainworker.SwitchScene(itt->RowID, switchcmd))
						{
							_log.Log(LOG_ERROR, "Error switching Scene command, SceneID: %" PRIu64 ", Time: %s", itt->RowID, ltimeBuf);
						}
					}
					else if (itt->bIsThermostat == true)
					{
						std::stringstream sstr;
						sstr << itt->RowID;
						if (!m_mainworker.SetSetPoint(sstr.str(), itt->Temperature))
						{
							_log.Log(LOG_ERROR, "Error setting thermostat setpoint, ThermostatID: %" PRIu64 ", Time: %s", itt->RowID, ltimeBuf);
						}
					}
					else
					{
						//Get SwitchType
						std::vector<std::vector<std::string> > result;
						result = m_sql.safe_query("SELECT Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == %" PRIu64 ")",
							itt->RowID);
						if (result.size() > 0)
						{
							std::vector<std::string> sd = result[0];

							unsigned char dType = atoi(sd[0].c_str());
							unsigned char dSubType = atoi(sd[1].c_str());
							_eSwitchType switchtype = (_eSwitchType)atoi(sd[2].c_str());
							std::string lstatus = "";
							int llevel = 0;
							bool bHaveDimmer = false;
							bool bHaveGroupCmd = false;
							int maxDimLevel = 0;

							GetLightStatus(dType, dSubType, switchtype, 0, "", lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
							int ilevel = maxDimLevel;
							if ((switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted))
							{
								if (itt->timerCmd == TCMD_ON)
								{
									switchcmd = "Set Level";
									float fLevel = (maxDimLevel / 100.0f)*itt->Level;
									if (fLevel > 100)
										fLevel = 100;
									ilevel = int(fLevel) + 1;
								}
								else if (itt->timerCmd == TCMD_OFF)
									ilevel = 0;
							}
							else if ((switchtype == STYPE_Dimmer) && (maxDimLevel != 0))
							{
								if (itt->timerCmd == TCMD_ON)
								{
									switchcmd = "Set Level";
									float fLevel = (maxDimLevel / 100.0f)*itt->Level;
									if (fLevel > 100)
										fLevel = 100;
									ilevel = int(fLevel);
								}
							} else if (switchtype == STYPE_Selector) {
								if (itt->timerCmd == TCMD_ON) {
									switchcmd = "Set Level";
									ilevel = itt->Level;
								} else if (itt->timerCmd == TCMD_OFF) {
									ilevel = 0; // force level to a valid value for Selector
								}
							}
							if (!m_mainworker.SwitchLight(itt->RowID, switchcmd, ilevel, itt->Hue,false,0))
							{
								_log.Log(LOG_ERROR, "Error sending switch command, DevID: %" PRIu64 ", Time: %s", itt->RowID, ltimeBuf);
							}
						}
					}
				}
			}
			if (!AdjustScheduleItem(&*itt, true))
			{
				//something is wrong, probably no sunset/rise
				if (itt->timerType != TTYPE_FIXEDDATETIME)
				{
					itt->startTime += atime + (24 * 3600);
				}
				else {
					//Disable timer
					itt->bEnabled = false;
				}
			}
		}
	}
}

void CScheduler::DeleteExpiredTimers()
{
	char szDate[20];
	char szTime[20];
	time_t now = mytime(NULL);
	struct tm tmnow;
	localtime_r(&now, &tmnow);
	sprintf(szDate, "%04d-%02d-%02d", tmnow.tm_year + 1900, tmnow.tm_mon + 1, tmnow.tm_mday);
	sprintf(szTime, "%02d:%02d", tmnow.tm_hour, tmnow.tm_min);
	int iExpiredTimers = 0;

	std::vector<std::vector<std::string> > result;
	// Check Timers
	result = m_sql.safe_query("SELECT ID FROM Timers WHERE (Type == %i AND ((Date < '%q') OR (Date == '%q' AND Time < '%q')))",
		TTYPE_FIXEDDATETIME,
		szDate,
		szDate,
		szTime
		);
	if (result.size() > 0) {
		m_sql.safe_query("DELETE FROM Timers WHERE (Type == %i AND ((Date < '%q') OR (Date == '%q' AND Time < '%q')))",
			TTYPE_FIXEDDATETIME,
			szDate,
			szDate,
			szTime
			);
		iExpiredTimers += result.size();
	}

	// Check SceneTimers
	result = m_sql.safe_query("SELECT ID FROM SceneTimers WHERE (Type == %i AND ((Date < '%q') OR (Date == '%q' AND Time < '%q')))",
		TTYPE_FIXEDDATETIME,
		szDate,
		szDate,
		szTime
		);
	if (result.size() > 0) {
		m_sql.safe_query("DELETE FROM SceneTimers WHERE (Type == %i AND ((Date < '%q') OR (Date == '%q' AND Time < '%q')))",
			TTYPE_FIXEDDATETIME,
			szDate,
			szDate,
			szTime
			);
		iExpiredTimers += result.size();
	}

	if (iExpiredTimers > 0) {
		_log.Log(LOG_STATUS, "Purged %i expired (scene)timer(s)", iExpiredTimers);

		ReloadSchedules();
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_Schedules(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string rfilter = request::findValue(&req, "filter");

			root["status"] = "OK";
			root["title"] = "Schedules";

			std::vector<std::vector<std::string> > tot_result;
			std::vector<std::vector<std::string> > tmp_result;

			//Retrieve active schedules
			std::vector<tScheduleItem> schedules = m_mainworker.m_scheduler.GetScheduleItems();

			//Add Timers
			if ((rfilter == "all") || (rfilter == "") || (rfilter == "device")) {
				tmp_result = m_sql.safe_query(
					"SELECT t.ID, t.Active, d.[Name], t.DeviceRowID AS ID, 0 AS IsScene, 0 AS IsThermostat,"
					" t.[Date], t.Time, t.Type, t.Cmd, t.Level, t.Hue, 0 AS Temperature, t.Days,"
					" t.UseRandomness, t.MDay, t.Month, t.Occurence"
					" FROM Timers AS t, DeviceStatus AS d"
					" WHERE (d.ID == t.DeviceRowID) AND (t.TimerPlan==%d)"
					" ORDER BY d.[Name], t.Time",
					m_sql.m_ActiveTimerPlan);
				if (tmp_result.size() > 0)
					tot_result.insert(tot_result.end(), tmp_result.begin(), tmp_result.end());
			}

			//Add Scene-Timers
			if ((rfilter == "all") || (rfilter == "") || (rfilter == "scene")) {
				tmp_result = m_sql.safe_query(
					"SELECT t.ID, t.Active, s.[Name], t.SceneRowID AS ID, 1 AS IsScene, 0 AS IsThermostat"
					", t.[Date], t.Time, t.Type, t.Cmd, t.Level, t.Hue, 0 AS Temperature, t.Days,"
					" t.UseRandomness, t.MDay, t.Month, t.Occurence"
					" FROM SceneTimers AS t, Scenes AS s"
					" WHERE (s.ID == t.SceneRowID) AND (t.TimerPlan==%d)"
					" ORDER BY s.[Name], t.Time",
					m_sql.m_ActiveTimerPlan);
				if (tmp_result.size() > 0)
					tot_result.insert(tot_result.end(), tmp_result.begin(), tmp_result.end());
			}

			//Add Setpoint-Timers
			if ((rfilter == "all") || (rfilter == "") || (rfilter == "thermostat")) {
				tmp_result = m_sql.safe_query(
					"SELECT t.ID, t.Active, d.[Name], t.DeviceRowID AS ID, 0 AS IsScene, 1 AS IsThermostat,"
					" t.[Date], t.Time, t.Type, 0 AS Cmd, 0 AS Level, 0 AS Hue, t.Temperature, t.Days,"
					" 0 AS UseRandomness, t.MDay, t.Month, t.Occurence"
					" FROM SetpointTimers AS t, DeviceStatus AS d"
					" WHERE (d.ID == t.DeviceRowID) AND (t.TimerPlan==%d)"
					" ORDER BY d.[Name], t.Time",
					m_sql.m_ActiveTimerPlan);
				if (tmp_result.size() > 0)
					tot_result.insert(tot_result.end(), tmp_result.begin(), tmp_result.end());
			}

			if (tot_result.size() > 0)
			{
				int ii = 0;
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = tot_result.begin(); itt != tot_result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					int iTimerIdx = atoi(sd[0].c_str());
					bool bActive = atoi(sd[1].c_str()) != 0;
					bool bIsScene = atoi(sd[4].c_str()) != 0;
					bool bIsThermostat = atoi(sd[5].c_str()) != 0;
					int iDays = atoi(sd[13].c_str());
					char ltimeBuf[30] = "";

					if (bActive)
					{
						//Find Timer in active schedules and retrieve ScheduleDate
						tScheduleItem sitem;
						sitem.TimerID = iTimerIdx;
						sitem.bIsScene = bIsScene;
						sitem.bIsThermostat = bIsThermostat;
						std::vector<tScheduleItem>::iterator it = std::find(schedules.begin(), schedules.end(), sitem);
						if (it != schedules.end()) {
							struct tm timeinfo;
							localtime_r(&it->startTime, &timeinfo);
							strftime(ltimeBuf, sizeof(ltimeBuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
						}
					}

					unsigned char iLevel = atoi(sd[10].c_str());
					if (iLevel == 0)
						iLevel = 100;

					int iTimerType = atoi(sd[8].c_str());
					std::string sdate = sd[6];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						char szTmp[30];
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["TimerID"] = iTimerIdx;
					root["result"][ii]["Active"] = bActive ? "true" : "false";
					root["result"][ii]["Type"] = bIsScene ? "Scene" : "Device";
					root["result"][ii]["IsThermostat"] = bIsThermostat ? "true" : "false";
					root["result"][ii]["DevName"] = sd[2];
					root["result"][ii]["DeviceRowID"] = atoi(sd[3].c_str());
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[7];
					root["result"][ii]["TimerType"] = iTimerType;
					root["result"][ii]["TimerTypeStr"] = Timer_Type_Desc(iTimerType);
					root["result"][ii]["Days"] = iDays;
					root["result"][ii]["MDay"] = atoi(sd[15].c_str());
					root["result"][ii]["Month"] = atoi(sd[16].c_str());
					root["result"][ii]["Occurence"] = atoi(sd[17].c_str());
					root["result"][ii]["ScheduleDate"] = ltimeBuf;
					if (bIsThermostat)
					{
						root["result"][ii]["Temperature"] = sd[12];
					}
					else
					{
						root["result"][ii]["TimerCmd"] = atoi(sd[9].c_str());
						root["result"][ii]["Level"] = iLevel;
						root["result"][ii]["Hue"] = atoi(sd[11].c_str());
						root["result"][ii]["Randomness"] = (atoi(sd[14].c_str()) != 0) ? "true" : "false";
					}
					ii++;
				}
			}
		}
		void CWebServer::RType_Timers(WebEmSession & session, const request& req, Json::Value &root)
		{
			uint64_t idx = 0;
			if (request::findValue(&req, "idx") != "")
			{
				std::stringstream s_str(request::findValue(&req, "idx"));
				s_str >> idx;
			}
			if (idx == 0)
				return;
			root["status"] = "OK";
			root["title"] = "Timers";
			char szTmp[50];

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Active, [Date], Time, Type, Cmd, Level, Hue, Days, UseRandomness, MDay, Month, Occurence FROM Timers WHERE (DeviceRowID==%" PRIu64 ") AND (TimerPlan==%d) ORDER BY ID",
				idx, m_sql.m_ActiveTimerPlan);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned char iLevel = atoi(sd[6].c_str());
					if (iLevel == 0)
						iLevel = 100;

					int iTimerType = atoi(sd[4].c_str());
					std::string sdate = sd[2];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Active"] = (atoi(sd[1].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[3].substr(0, 5);
					root["result"][ii]["Type"] = iTimerType;
					root["result"][ii]["Cmd"] = atoi(sd[5].c_str());
					root["result"][ii]["Level"] = iLevel;
					root["result"][ii]["Hue"] = atoi(sd[7].c_str());
					root["result"][ii]["Days"] = atoi(sd[8].c_str());
					root["result"][ii]["Randomness"] = (atoi(sd[9].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["MDay"] = atoi(sd[10].c_str());
					root["result"][ii]["Month"] = atoi(sd[11].c_str());
					root["result"][ii]["Occurence"] = atoi(sd[12].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_SetActiveTimerPlan(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			int rnOldvalue = 0;
			int rnvalue = 0;
			m_sql.GetPreferencesVar("ActiveTimerPlan", rnOldvalue);
			rnvalue = atoi(request::findValue(&req, "ActiveTimerPlan").c_str());
			if (rnOldvalue != rnvalue)
			{
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM TimerPlans WHERE (ID == %d)", rnvalue);
				if (result.empty())
					return; //timerplan not found!
				_log.Log(LOG_STATUS, "Scheduler Timerplan changed (%d - %s)", rnvalue, result[0][0].c_str());
				m_sql.UpdatePreferencesVar("ActiveTimerPlan", rnvalue);
				m_sql.m_ActiveTimerPlan = rnvalue;
				m_mainworker.m_scheduler.ReloadSchedules();
			}

			root["status"] = "OK";
			root["title"] = "SetActiveTimerPlan";
		}

		void CWebServer::Cmd_AddTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string active = request::findValue(&req, "active");
			std::string stimertype = request::findValue(&req, "timertype");
			std::string sdate = request::findValue(&req, "date");
			std::string shour = request::findValue(&req, "hour");
			std::string smin = request::findValue(&req, "min");
			std::string randomness = request::findValue(&req, "randomness");
			std::string scmd = request::findValue(&req, "command");
			std::string sdays = request::findValue(&req, "days");
			std::string slevel = request::findValue(&req, "level");	//in percentage
			std::string shue = request::findValue(&req, "hue");
			std::string smday = "0";
			std::string smonth = "0";
			std::string soccurence = "0";
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;
			unsigned char iTimerType = atoi(stimertype.c_str());

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}
			else if (iTimerType == TTYPE_MONTHLY)
			{
				smday = request::findValue(&req, "mday");
				if (smday == "") return;
			}
			else if (iTimerType == TTYPE_MONTHLY_WD)
			{
				soccurence = request::findValue(&req, "occurence");
				if (soccurence == "") return;
			}
			else if (iTimerType == TTYPE_YEARLY)
			{
				smday = request::findValue(&req, "mday");
				smonth = request::findValue(&req, "month");
				if ((smday == "") || (smonth == "")) return;
			}
			else if (iTimerType == TTYPE_YEARLY_WD)
			{
				smonth = request::findValue(&req, "month");
				soccurence = request::findValue(&req, "occurence");
				if ((smonth == "") || (soccurence == "")) return;
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			int hue = atoi(shue.c_str());
			int mday = atoi(smday.c_str());
			int month = atoi(smonth.c_str());
			int occurence = atoi(soccurence.c_str());
			root["status"] = "OK";
			root["title"] = "AddTimer";
			m_sql.safe_query(
				"INSERT INTO Timers (Active, DeviceRowID, [Date], Time, Type, UseRandomness, Cmd, Level, Hue, Days, MDay, Month, Occurence, TimerPlan) VALUES (%d,'%q','%04d-%02d-%02d','%02d:%02d',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",
				(active == "true") ? 1 : 0,
				idx.c_str(),
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				hue,
				days,
				mday,
				month,
				occurence,
				m_sql.m_ActiveTimerPlan
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_UpdateTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string active = request::findValue(&req, "active");
			std::string stimertype = request::findValue(&req, "timertype");
			std::string sdate = request::findValue(&req, "date");
			std::string shour = request::findValue(&req, "hour");
			std::string smin = request::findValue(&req, "min");
			std::string randomness = request::findValue(&req, "randomness");
			std::string scmd = request::findValue(&req, "command");
			std::string sdays = request::findValue(&req, "days");
			std::string slevel = request::findValue(&req, "level");	//in percentage
			std::string shue = request::findValue(&req, "hue");
			std::string smday = "0";
			std::string smonth = "0";
			std::string soccurence = "0";
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;

			unsigned char iTimerType = atoi(stimertype.c_str());
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}
			else if (iTimerType == TTYPE_MONTHLY)
			{
				smday = request::findValue(&req, "mday");
				if (smday == "") return;
			}
			else if (iTimerType == TTYPE_MONTHLY_WD)
			{
				soccurence = request::findValue(&req, "occurence");
				if (soccurence == "") return;
			}
			else if (iTimerType == TTYPE_YEARLY)
			{
				smday = request::findValue(&req, "mday");
				smonth = request::findValue(&req, "month");
				if ((smday == "") || (smonth == "")) return;
			}
			else if (iTimerType == TTYPE_YEARLY_WD)
			{
				smonth = request::findValue(&req, "month");
				soccurence = request::findValue(&req, "occurence");
				if ((smonth == "") || (soccurence == "")) return;
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			int hue = atoi(shue.c_str());
			int mday = atoi(smday.c_str());
			int month = atoi(smonth.c_str());
			int occurence = atoi(soccurence.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateTimer";
			m_sql.safe_query(
				"UPDATE Timers SET Active=%d, [Date]='%04d-%02d-%02d', Time='%02d:%02d', Type=%d, UseRandomness=%d, Cmd=%d, Level=%d, Hue=%d, Days=%d, MDay=%d, Month=%d, Occurence=%d WHERE (ID == '%q')",
				(active == "true") ? 1 : 0,
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				hue,
				days,
				mday,
				month,
				occurence,
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DeleteTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteTimer";
			m_sql.safe_query(
				"DELETE FROM Timers WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_EnableTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "EnableTimer";
			m_sql.safe_query(
				"UPDATE Timers SET Active=1 WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DisableTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DisableTimer";
			m_sql.safe_query(
				"UPDATE Timers SET Active=0 WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_ClearTimers(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearTimer";
			m_sql.safe_query(
				"DELETE FROM Timers WHERE (DeviceRowID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::RType_SetpointTimers(WebEmSession & session, const request& req, Json::Value &root)
		{
			uint64_t idx = 0;
			if (request::findValue(&req, "idx") != "")
			{
				std::stringstream s_str(request::findValue(&req, "idx"));
				s_str >> idx;
			}
			if (idx == 0)
				return;
			root["status"] = "OK";
			root["title"] = "SetpointTimers";
			char szTmp[50];

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Active, [Date], Time, Type, Temperature, Days, MDay, Month, Occurence FROM SetpointTimers WHERE (DeviceRowID=%" PRIu64 ") AND (TimerPlan==%d) ORDER BY ID",
				idx, m_sql.m_ActiveTimerPlan);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					int iTimerType = atoi(sd[4].c_str());
					std::string sdate = sd[2];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Active"] = (atoi(sd[1].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[3].substr(0, 5);
					root["result"][ii]["Type"] = iTimerType;
					root["result"][ii]["Temperature"] = atof(sd[5].c_str());
					root["result"][ii]["Days"] = atoi(sd[6].c_str());
					root["result"][ii]["MDay"] = atoi(sd[7].c_str());
					root["result"][ii]["Month"] = atoi(sd[8].c_str());
					root["result"][ii]["Occurence"] = atoi(sd[9].c_str());
					ii++;
				}
			}
		}
		void CWebServer::Cmd_AddSetpointTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string active = request::findValue(&req, "active");
			std::string stimertype = request::findValue(&req, "timertype");
			std::string sdate = request::findValue(&req, "date");
			std::string shour = request::findValue(&req, "hour");
			std::string smin = request::findValue(&req, "min");
			std::string stvalue = request::findValue(&req, "tvalue");
			std::string sdays = request::findValue(&req, "days");
			std::string smday = "0";
			std::string smonth = "0";
			std::string soccurence = "0";
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(stvalue == "") ||
				(sdays == "")
				)
				return;
			unsigned char iTimerType = atoi(stimertype.c_str());

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}
			else if (iTimerType == TTYPE_MONTHLY)
			{
				smday = request::findValue(&req, "mday");
				if (smday == "") return;
			}
			else if (iTimerType == TTYPE_MONTHLY_WD)
			{
				soccurence = request::findValue(&req, "occurence");
				if (soccurence == "") return;
			}
			else if (iTimerType == TTYPE_YEARLY)
			{
				smday = request::findValue(&req, "mday");
				smonth = request::findValue(&req, "month");
				if ((smday == "") || (smonth == "")) return;
			}
			else if (iTimerType == TTYPE_YEARLY_WD)
			{
				smonth = request::findValue(&req, "month");
				soccurence = request::findValue(&req, "occurence");
				if ((smonth == "") || (soccurence == "")) return;
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			int days = atoi(sdays.c_str());
			float temperature = static_cast<float>(atof(stvalue.c_str()));
			int mday = atoi(smday.c_str());
			int month = atoi(smonth.c_str());
			int occurence = atoi(soccurence.c_str());
			root["status"] = "OK";
			root["title"] = "AddSetpointTimer";
			m_sql.safe_query(
				"INSERT INTO SetpointTimers (Active, DeviceRowID, [Date], Time, Type, Temperature, Days, MDay, Month, Occurence, TimerPlan) VALUES (%d,'%q','%04d-%02d-%02d','%02d:%02d',%d,%.1f,%d,%d,%d,%d,%d)",
				(active == "true") ? 1 : 0,
				idx.c_str(),
				Year, Month, Day,
				hour, min,
				iTimerType,
				temperature,
				days,
				mday,
				month,
				occurence,
				m_sql.m_ActiveTimerPlan
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_UpdateSetpointTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string active = request::findValue(&req, "active");
			std::string stimertype = request::findValue(&req, "timertype");
			std::string sdate = request::findValue(&req, "date");
			std::string shour = request::findValue(&req, "hour");
			std::string smin = request::findValue(&req, "min");
			std::string stvalue = request::findValue(&req, "tvalue");
			std::string sdays = request::findValue(&req, "days");
			std::string smday = "0";
			std::string smonth = "0";
			std::string soccurence = "0";
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(stvalue == "") ||
				(sdays == "")
				)
				return;

			unsigned char iTimerType = atoi(stimertype.c_str());
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}
			else if (iTimerType == TTYPE_MONTHLY)
			{
				smday = request::findValue(&req, "mday");
				if (smday == "") return;
			}
			else if (iTimerType == TTYPE_MONTHLY_WD)
			{
				soccurence = request::findValue(&req, "occurence");
				if (soccurence == "") return;
			}
			else if (iTimerType == TTYPE_YEARLY)
			{
				smday = request::findValue(&req, "mday");
				smonth = request::findValue(&req, "month");
				if ((smday == "") || (smonth == "")) return;
			}
			else if (iTimerType == TTYPE_YEARLY_WD)
			{
				smonth = request::findValue(&req, "month");
				soccurence = request::findValue(&req, "occurence");
				if ((smonth == "") || (soccurence == "")) return;
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			int days = atoi(sdays.c_str());
			float tempvalue = static_cast<float>(atof(stvalue.c_str()));
			int mday = atoi(smday.c_str());
			int month = atoi(smonth.c_str());
			int occurence = atoi(soccurence.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateSetpointTimer";
			m_sql.safe_query(
				"UPDATE SetpointTimers SET Active=%d, [Date]='%04d-%02d-%02d', Time='%02d:%02d', Type=%d, Temperature=%.1f, Days=%d, MDay=%d, Month=%d, Occurence=%d WHERE (ID == '%q')",
				(active == "true") ? 1 : 0,
				Year, Month, Day,
				hour, min,
				iTimerType,
				tempvalue,
				days,
				mday,
				month,
				occurence,
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DeleteSetpointTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteSetpointTimer";
			m_sql.safe_query(
				"DELETE FROM SetpointTimers WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_EnableSetpointTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "EnableSetpointTimer";
			m_sql.safe_query(
				"UPDATE SetpointTimers SET Active=1 WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DisableSetpointTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DisableSetpointTimer";
			m_sql.safe_query(
				"UPDATE SetpointTimers SET Active=0 WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_ClearSetpointTimers(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearSetpointTimers";
			m_sql.safe_query(
				"DELETE FROM SetpointTimers WHERE (DeviceRowID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::RType_SceneTimers(WebEmSession & session, const request& req, Json::Value &root)
		{
			uint64_t idx = 0;
			if (request::findValue(&req, "idx") != "")
			{
				std::stringstream s_str(request::findValue(&req, "idx"));
				s_str >> idx;
			}
			if (idx == 0)
				return;
			root["status"] = "OK";
			root["title"] = "SceneTimers";

			char szTmp[40];

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Active, [Date], Time, Type, Cmd, Level, Hue, Days, UseRandomness, MDay, Month, Occurence FROM SceneTimers WHERE (SceneRowID==%" PRIu64 ") AND (TimerPlan==%d) ORDER BY ID",
				idx, m_sql.m_ActiveTimerPlan);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned char iLevel = atoi(sd[6].c_str());
					if (iLevel == 0)
						iLevel = 100;

					int iTimerType = atoi(sd[4].c_str());
					std::string sdate = sd[2];
					if ((iTimerType == TTYPE_FIXEDDATETIME) && (sdate.size() == 10))
					{
						int Year = atoi(sdate.substr(0, 4).c_str());
						int Month = atoi(sdate.substr(5, 2).c_str());
						int Day = atoi(sdate.substr(8, 2).c_str());
						sprintf(szTmp, "%02d-%02d-%04d", Month, Day, Year);
						sdate = szTmp;
					}
					else
						sdate = "";

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Active"] = (atoi(sd[1].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["Date"] = sdate;
					root["result"][ii]["Time"] = sd[3].substr(0, 5);
					root["result"][ii]["Type"] = iTimerType;
					root["result"][ii]["Cmd"] = atoi(sd[5].c_str());
					root["result"][ii]["Level"] = iLevel;
					root["result"][ii]["Hue"] = atoi(sd[7].c_str());
					root["result"][ii]["Days"] = atoi(sd[8].c_str());
					root["result"][ii]["Randomness"] = (atoi(sd[9].c_str()) == 0) ? "false" : "true";
					root["result"][ii]["MDay"] = atoi(sd[10].c_str());
					root["result"][ii]["Month"] = atoi(sd[11].c_str());
					root["result"][ii]["Occurence"] = atoi(sd[12].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_AddSceneTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string active = request::findValue(&req, "active");
			std::string stimertype = request::findValue(&req, "timertype");
			std::string sdate = request::findValue(&req, "date");
			std::string shour = request::findValue(&req, "hour");
			std::string smin = request::findValue(&req, "min");
			std::string randomness = request::findValue(&req, "randomness");
			std::string scmd = request::findValue(&req, "command");
			std::string sdays = request::findValue(&req, "days");
			std::string slevel = request::findValue(&req, "level");	//in percentage
			std::string smday = "0";
			std::string smonth = "0";
			std::string soccurence = "0";
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;
			unsigned char iTimerType = atoi(stimertype.c_str());

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}
			else if (iTimerType == TTYPE_MONTHLY)
			{
				smday = request::findValue(&req, "mday");
				if (smday == "") return;
			}
			else if (iTimerType == TTYPE_MONTHLY_WD)
			{
				soccurence = request::findValue(&req, "occurence");
				if (soccurence == "") return;
			}
			else if (iTimerType == TTYPE_YEARLY)
			{
				smday = request::findValue(&req, "mday");
				smonth = request::findValue(&req, "month");
				if ((smday == "") || (smonth == "")) return;
			}
			else if (iTimerType == TTYPE_YEARLY_WD)
			{
				smonth = request::findValue(&req, "month");
				soccurence = request::findValue(&req, "occurence");
				if ((smonth == "") || (soccurence == "")) return;
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			int mday = atoi(smday.c_str());
			int month = atoi(smonth.c_str());
			int occurence = atoi(soccurence.c_str());
			root["status"] = "OK";
			root["title"] = "AddSceneTimer";
			m_sql.safe_query(
				"INSERT INTO SceneTimers (Active, SceneRowID, [Date], Time, Type, UseRandomness, Cmd, Level, Days, MDay, Month, Occurence, TimerPlan) VALUES (%d,'%q','%04d-%02d-%02d','%02d:%02d',%d,%d,%d,%d,%d,%d,%d,%d,%d)",
				(active == "true") ? 1 : 0,
				idx.c_str(),
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				days,
				mday,
				month,
				occurence,
				m_sql.m_ActiveTimerPlan
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_UpdateSceneTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string active = request::findValue(&req, "active");
			std::string stimertype = request::findValue(&req, "timertype");
			std::string sdate = request::findValue(&req, "date");
			std::string shour = request::findValue(&req, "hour");
			std::string smin = request::findValue(&req, "min");
			std::string randomness = request::findValue(&req, "randomness");
			std::string scmd = request::findValue(&req, "command");
			std::string sdays = request::findValue(&req, "days");
			std::string slevel = request::findValue(&req, "level");	//in percentage
			std::string smday = "0";
			std::string smonth = "0";
			std::string soccurence = "0";
			if (
				(idx == "") ||
				(active == "") ||
				(stimertype == "") ||
				(shour == "") ||
				(smin == "") ||
				(randomness == "") ||
				(scmd == "") ||
				(sdays == "")
				)
				return;

			unsigned char iTimerType = atoi(stimertype.c_str());

			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			int Year = tm1.tm_year + 1900;
			int Month = tm1.tm_mon + 1;
			int Day = tm1.tm_mday;

			if (iTimerType == TTYPE_FIXEDDATETIME)
			{
				if (sdate.size() == 10)
				{
					Month = atoi(sdate.substr(0, 2).c_str());
					Day = atoi(sdate.substr(3, 2).c_str());
					Year = atoi(sdate.substr(6, 4).c_str());
				}
			}
			else if (iTimerType == TTYPE_MONTHLY)
			{
				smday = request::findValue(&req, "mday");
				if (smday == "") return;
			}
			else if (iTimerType == TTYPE_MONTHLY_WD)
			{
				soccurence = request::findValue(&req, "occurence");
				if (soccurence == "") return;
			}
			else if (iTimerType == TTYPE_YEARLY)
			{
				smday = request::findValue(&req, "mday");
				smonth = request::findValue(&req, "month");
				if ((smday == "") || (smonth == "")) return;
			}
			else if (iTimerType == TTYPE_YEARLY_WD)
			{
				smonth = request::findValue(&req, "month");
				soccurence = request::findValue(&req, "occurence");
				if ((smonth == "") || (soccurence == "")) return;
			}

			unsigned char hour = atoi(shour.c_str());
			unsigned char min = atoi(smin.c_str());
			unsigned char icmd = atoi(scmd.c_str());
			int days = atoi(sdays.c_str());
			unsigned char level = atoi(slevel.c_str());
			int mday = atoi(smday.c_str());
			int month = atoi(smonth.c_str());
			int occurence = atoi(soccurence.c_str());
			root["status"] = "OK";
			root["title"] = "UpdateSceneTimer";
			m_sql.safe_query(
				"UPDATE SceneTimers SET Active=%d, [Date]='%04d-%02d-%02d', Time='%02d:%02d', Type=%d, UseRandomness=%d, Cmd=%d, Level=%d, Days=%d, MDay=%d, Month=%d, Occurence=%d WHERE (ID == '%q')",
				(active == "true") ? 1 : 0,
				Year, Month, Day,
				hour, min,
				iTimerType,
				(randomness == "true") ? 1 : 0,
				icmd,
				level,
				days,
				mday,
				month,
				occurence,
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DeleteSceneTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DeleteSceneTimer";
			m_sql.safe_query(
				"DELETE FROM SceneTimers WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_EnableSceneTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "EnableSceneTimer";
			m_sql.safe_query(
				"UPDATE SceneTimers SET Active=1 WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_DisableSceneTimer(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "DisableSceneTimer";
			m_sql.safe_query(
				"UPDATE SceneTimers SET Active=0 WHERE (ID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}

		void CWebServer::Cmd_ClearSceneTimers(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			root["status"] = "OK";
			root["title"] = "ClearSceneTimer";
			m_sql.safe_query(
				"DELETE FROM SceneTimers WHERE (SceneRowID == '%q')",
				idx.c_str()
				);
			m_mainworker.m_scheduler.ReloadSchedules();
		}
	}
}
