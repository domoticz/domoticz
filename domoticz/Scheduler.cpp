#include "stdafx.h"
#include "Scheduler.h"
#include "mainworker.h"

CScheduler::CScheduler(void)
{
	m_tSunRise=0;
	m_tSunSet=0;
	m_pMain=NULL;
	m_stoprequested=false;
}

CScheduler::~CScheduler(void)
{
}

void CScheduler::StartScheduler(MainWorker *pMainWorker)
{
	m_pMain=pMainWorker;
	ReloadSchedules();
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CScheduler::Do_Work, this)));
}

void CScheduler::StopScheduler()
{
	if (m_thread!=NULL)
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
	for (itt=m_scheduleitems.begin(); itt!=m_scheduleitems.end(); ++itt)
	{
		ret.push_back(*itt);
	}
	return ret;
}

void CScheduler::ReloadSchedules()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	m_scheduleitems.clear();

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	szQuery << "SELECT DeviceRowID, Time, Type, Cmd, Level, Days FROM Timers WHERE (Active == 1) ORDER BY ID";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			tScheduleItem titem;

			std::stringstream s_str( sd[0] );
			s_str >> titem.DevID;

			titem.startHour=(unsigned char)atoi(sd[1].substr(0,2).c_str());
			titem.startMin=(unsigned char)atoi(sd[1].substr(3,2).c_str());
			titem.startTime=0;

			titem.timerType=(_eTimerType)atoi(sd[2].c_str());
			titem.timerCmd=(_eTimerCommand)atoi(sd[3].c_str());
			titem.Level=(unsigned char)atoi(sd[4].c_str());
			titem.Days=atoi(sd[5].c_str());
			if (AdjustScheduleItem(&titem,false)==true)
				m_scheduleitems.push_back(titem);
		}
	}

}

void CScheduler::SetSunRiseSetTimers(std::string sSunRise, std::string sSunSet)
{
	time_t temptime;
	time_t atime=time(NULL);
	tm *ltime;
	ltime=localtime(&atime);
	if (!ltime)
		return;

	bool bReloadSchedules=false;
	{	//needed private scope for the lock
		boost::lock_guard<boost::mutex> l(m_mutex);
		unsigned char hour,min,sec;

		hour=atoi(sSunRise.substr(0,2).c_str());
		min=atoi(sSunRise.substr(3,2).c_str());
		sec=atoi(sSunRise.substr(6,2).c_str());

		ltime->tm_hour = hour;
		ltime->tm_min = min;
		ltime->tm_sec = sec;
		temptime = mktime(ltime);
		if (m_tSunRise!=temptime)
		{
			bReloadSchedules=true;
			m_tSunRise=temptime;
		}

		hour=atoi(sSunSet.substr(0,2).c_str());
		min=atoi(sSunSet.substr(3,2).c_str());
		sec=atoi(sSunSet.substr(6,2).c_str());

		ltime->tm_hour = hour;
		ltime->tm_min = min;
		ltime->tm_sec = sec;
		temptime = mktime(ltime);
		if (m_tSunSet!=temptime)
		{
			bReloadSchedules=true;
			m_tSunSet=temptime;
		}
	}
	if (bReloadSchedules)
		ReloadSchedules();
}

bool CScheduler::AdjustScheduleItem(tScheduleItem *pItem, bool bForceAddDay)
{
	time_t rtime;
	time_t atime;
	tm *ltime;
	atime=time(NULL);
	ltime=localtime(&atime);
	ltime->tm_sec=0;
	rtime=mktime(ltime);	//time now without seconds

	if (pItem->timerType == TTYPE_ONTIME)
	{
		ltime->tm_hour=pItem->startHour;
		ltime->tm_min=pItem->startMin;
	}
	else if (pItem->timerType == TTYPE_BEFORESUNSET)
	{
		if (m_tSunSet==0)
			return false;
		ltime=localtime(&m_tSunSet);
		ltime->tm_hour-=pItem->startHour;
		ltime->tm_min-=pItem->startMin;
	}
	else if (pItem->timerType == TTYPE_AFTERSUNSET)
	{
		if (m_tSunSet==0)
			return false;
		ltime=localtime(&m_tSunSet);
		ltime->tm_hour+=pItem->startHour;
		ltime->tm_min+=pItem->startMin;
	}
	else if (pItem->timerType == TTYPE_BEFORESUNRISE)
	{
		if (m_tSunRise==0)
			return false;
		ltime=localtime(&m_tSunRise);
		ltime->tm_hour-=pItem->startHour;
		ltime->tm_min-=pItem->startMin;
	}
	else if (pItem->timerType == TTYPE_AFTERSUNRISE)
	{
		if (m_tSunRise==0)
			return false;
		ltime=localtime(&m_tSunRise);
		ltime->tm_hour+=pItem->startHour;
		ltime->tm_min+=pItem->startMin;
	}


	time_t itime=mktime(ltime);
	if ((itime<rtime)||(bForceAddDay))
	{
		//item is scheduled for next day
		ltime->tm_mday+=1;
		itime=mktime(ltime);
	}
	pItem->startTime=itime;
	return true;
}

void CScheduler::Do_Work()
{
	while (!m_stoprequested)
	{
		//sleep 1 second
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		CheckSchedules();
	}
	std::cout << "Scheduler stopped..." << std::endl;
}

void CScheduler::CheckSchedules()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	time_t atime=time(NULL);

	std::vector<tScheduleItem>::iterator itt;
	for (itt=m_scheduleitems.begin(); itt!=m_scheduleitems.end(); ++itt)
	{
		if (atime>itt->startTime)
		{
			struct tm *ltime;
			ltime=localtime(&itt->startTime);

			//check if we are on a valid day
			bool bOkToFire=false;
			if (itt->Days & 0x80)
			{
				//everyday
				bOkToFire=true; 
			}
			else if (itt->Days & 0x100)
			{
				//weekdays
				if ((ltime->tm_wday>0)&&(ltime->tm_wday<6))
					bOkToFire=true;
			}
			else if (itt->Days & 0x200)
			{
				//weekends
				if ((ltime->tm_wday==0)||(ltime->tm_wday==6))
					bOkToFire=true;
			}
			else
			{
				//custom days
				if ((itt->Days & 0x01)&&(ltime->tm_wday==1))
					bOkToFire=true;//Monday
				if ((itt->Days & 0x02)&&(ltime->tm_wday==2))
					bOkToFire=true;//Tuesday
				if ((itt->Days & 0x04)&&(ltime->tm_wday==3))
					bOkToFire=true;//Wednesday
				if ((itt->Days & 0x08)&&(ltime->tm_wday==4))
					bOkToFire=true;//Thursday
				if ((itt->Days & 0x10)&&(ltime->tm_wday==5))
					bOkToFire=true;//Friday
				if ((itt->Days & 0x20)&&(ltime->tm_wday==6))
					bOkToFire=true;//Saturday
				if ((itt->Days & 0x40)&&(ltime->tm_wday==0))
					bOkToFire=true;//Sunday
			}
			if (bOkToFire)
			{
				std::cout << "Schedule item started! DevID: " << itt->DevID << ", Time: " << asctime(ltime) << std::endl;
				std::string switchcmd="";
				if (itt->timerCmd == TCMD_ON)
					switchcmd="On";
				else if (itt->timerCmd == TCMD_OFF)
					switchcmd="Off";
				if (switchcmd=="")
				{
					std::cerr << "Unknown switch command in timer!!...." << std::endl;
				}
				else
				{
					if (!m_pMain->SwitchLight(itt->DevID,switchcmd,itt->Level))
					{
						std::cerr << "Error sending switch command, DevID: " << itt->DevID << ", Time: " << asctime(ltime) << std::endl;
					}
				}
				AdjustScheduleItem(&*itt,true);
			}
		}
	}
}
