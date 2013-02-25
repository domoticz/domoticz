#include "stdafx.h"
#include <iostream>
#include "Scheduler.h"
#include "mainworker.h"
#include "localtime_r.h"
#include "Logger.h"

CScheduler::CScheduler(void)
{
	m_tSunRise=0;
	m_tSunSet=0;
	m_pMain=NULL;
	m_stoprequested=false;
	srand((int)time(NULL));
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
	std::vector<std::vector<std::string> > result2;

	szQuery << "SELECT DeviceRowID, Time, Type, Cmd, Level, Days FROM Timers WHERE (Active == 1) ORDER BY ID";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT Used FROM DeviceStatus WHERE (ID == " << sd[0] << ")";
			result2=m_pMain->m_sql.query(szQuery.str());
			if (result2.size()>0)
			{
				std::string sIsUsed = result2[0][0];
				if (atoi(sIsUsed.c_str())==1)
				{
					tScheduleItem titem;

					std::stringstream s_str( sd[0] );


					s_str >> titem.DevID;

					titem.startHour=(unsigned char)atoi(sd[1].substr(0,2).c_str());
					titem.startMin=(unsigned char)atoi(sd[1].substr(3,2).c_str());
					titem.startTime=0;

					titem.timerType=(_eTimerType)atoi(sd[2].c_str());
					titem.timerCmd=(_eTimerCommand)atoi(sd[3].c_str());
					titem.Level=(unsigned char)atoi(sd[4].c_str());

					if ((titem.timerCmd==TCMD_ON)&&(titem.Level==0))
					{
						titem.Level=100;
					}

					titem.Days=atoi(sd[5].c_str());
					if (AdjustScheduleItem(&titem)==true)
						m_scheduleitems.push_back(titem);
				}
				else
				{
					//not used? delete it
					szQuery.clear();
					szQuery.str("");
					szQuery << "DELETE FROM Timers WHERE (DeviceRowID == " << sd[0] << ")";
					m_pMain->m_sql.query(szQuery.str());
				}
			}
		}
	}

}

void CScheduler::SetSunRiseSetTimers(std::string sSunRise, std::string sSunSet)
{
	bool bReloadSchedules=false;
	{	//needed private scope for the lock
		boost::lock_guard<boost::mutex> l(m_mutex);
		unsigned char hour,min,sec;

		time_t temptime;
		time_t atime=time(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);

		hour=atoi(sSunRise.substr(0,2).c_str());
		min=atoi(sSunRise.substr(3,2).c_str());
		sec=atoi(sSunRise.substr(6,2).c_str());

		ltime.tm_hour = hour;
		ltime.tm_min = min;
		ltime.tm_sec = sec;
		temptime = mktime(&ltime);
		if (temptime<atime)
			temptime+=(24*3600);
		if ((m_tSunRise!=temptime)&&(temptime!=0))
		{
			bReloadSchedules=true;
			m_tSunRise=temptime;
		}
		if (m_tSunRise<atime)
			m_tSunRise+=(24*3600);

		hour=atoi(sSunSet.substr(0,2).c_str());
		min=atoi(sSunSet.substr(3,2).c_str());
		sec=atoi(sSunSet.substr(6,2).c_str());

		ltime.tm_hour = hour;
		ltime.tm_min = min;
		ltime.tm_sec = sec;
		temptime = mktime(&ltime);
		if (temptime<atime)
			temptime+=(24*3600);
		if ((m_tSunSet!=temptime)&&(temptime!=0))
		{
			bReloadSchedules=true;
			m_tSunSet=temptime;
		}
		if (m_tSunSet<atime)
			m_tSunSet+=(24*3600);
	}
	if (bReloadSchedules)
		ReloadSchedules();
}

bool CScheduler::AdjustScheduleItem(tScheduleItem *pItem)
{
	time_t atime=time(NULL);
	time_t rtime=atime;
	struct tm ltime;
	localtime_r(&atime,&ltime);
	ltime.tm_sec=0;

	time_t sunset=m_tSunSet;
	if (sunset<atime)
		sunset+=(24*3600);
	time_t sunrise=m_tSunRise;
	if (sunrise<atime)
		sunrise+=(24*3600);

	unsigned long HourMinuteOffset=(pItem->startHour*3600)+(pItem->startMin*60);

	int nRandomTimerFrame=15;
	m_pMain->m_sql.GetPreferencesVar("RandomTimerFrame", nRandomTimerFrame);
	int roffset=rand() % (nRandomTimerFrame*2)-nRandomTimerFrame;

	if (pItem->timerType == TTYPE_ONTIME)
	{
		ltime.tm_hour=pItem->startHour;
		ltime.tm_min=pItem->startMin;
		rtime=mktime(&ltime);
	}
	else if (pItem->timerType == TTYPE_ONTIMERANDOM)
	{
		ltime.tm_hour=pItem->startHour;
		ltime.tm_min=pItem->startMin;
		rtime=mktime(&ltime)+(roffset*60);
	}
	else if (pItem->timerType == TTYPE_BEFORESUNSET)
	{
		if (m_tSunSet==0)
			return false;
		rtime=sunset-HourMinuteOffset;
	}
	else if (pItem->timerType == TTYPE_AFTERSUNSET)
	{
		if (m_tSunSet==0)
			return false;
		rtime=sunset+HourMinuteOffset;
	}
	else if (pItem->timerType == TTYPE_BEFORESUNRISE)
	{
		if (m_tSunRise==0)
			return false;
		rtime=sunrise-HourMinuteOffset;
	}
	else if (pItem->timerType == TTYPE_AFTERSUNRISE)
	{
		if (m_tSunRise==0)
			return false;
		rtime=sunrise+HourMinuteOffset;
	}
	else
		return false; //unknown timer type

	//Adjust timer by 1 day if we are in the past
	while (rtime<atime+60)
	{
		rtime+=(24*3600);
	}

	pItem->startTime=rtime;
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
	_log.Log(LOG_NORM,"Scheduler stopped...");
}

void CScheduler::CheckSchedules()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	time_t atime=time(NULL);
	struct tm ltime;
	localtime_r(&atime,&ltime);

	std::vector<tScheduleItem>::iterator itt;
	for (itt=m_scheduleitems.begin(); itt!=m_scheduleitems.end(); ++itt)
	{
		if (atime>itt->startTime)
		{

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
				if ((ltime.tm_wday>0)&&(ltime.tm_wday<6))
					bOkToFire=true;
			}
			else if (itt->Days & 0x200)
			{
				//weekends
				if ((ltime.tm_wday==0)||(ltime.tm_wday==6))
					bOkToFire=true;
			}
			else
			{
				//custom days
				if ((itt->Days & 0x01)&&(ltime.tm_wday==1))
					bOkToFire=true;//Monday
				if ((itt->Days & 0x02)&&(ltime.tm_wday==2))
					bOkToFire=true;//Tuesday
				if ((itt->Days & 0x04)&&(ltime.tm_wday==3))
					bOkToFire=true;//Wednesday
				if ((itt->Days & 0x08)&&(ltime.tm_wday==4))
					bOkToFire=true;//Thursday
				if ((itt->Days & 0x10)&&(ltime.tm_wday==5))
					bOkToFire=true;//Friday
				if ((itt->Days & 0x20)&&(ltime.tm_wday==6))
					bOkToFire=true;//Saturday
				if ((itt->Days & 0x40)&&(ltime.tm_wday==0))
					bOkToFire=true;//Sunday
			}
			if (bOkToFire)
			{
				_log.Log(LOG_NORM,"Schedule item started! Type: %s, DevID: %llu, Time: %s", Timer_Type_Desc(itt->timerType), itt->DevID, asctime(&ltime));
				std::string switchcmd="";
				if (itt->timerCmd == TCMD_ON)
					switchcmd="On";
				else if (itt->timerCmd == TCMD_OFF)
					switchcmd="Off";
				if (switchcmd=="")
				{
					_log.Log(LOG_ERROR,"Unknown switch command in timer!!....");
				}
				else
				{
					//Get SwitchType
					std::vector<std::vector<std::string> > result;
					std::stringstream szQuery;
					szQuery << "SELECT Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == " << itt->DevID << ")";
					result=m_pMain->m_sql.query(szQuery.str());
					if (result.size()>0)
					{
						std::vector<std::string> sd=result[0];

						unsigned char dType=atoi(sd[0].c_str());
						unsigned char dSubType=atoi(sd[1].c_str());
						_eSwitchType switchtype=(_eSwitchType) atoi(sd[2].c_str());
						std::string lstatus="";
						int llevel=0;
						bool bHaveDimmer=false;
						bool bHaveGroupCmd=false;
						int maxDimLevel=0;

						GetLightStatus(dType,dSubType,0,"",lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
						int ilevel=100;
						if ((switchtype == STYPE_Dimmer)&&(maxDimLevel!=0))
						{
							if (itt->timerCmd == TCMD_ON)
							{
								switchcmd="Set Level";
								float fLevel=(maxDimLevel/100.0f)*itt->Level;
								if (fLevel>100)
									fLevel=100;
								ilevel=int(fLevel);
							}
						}
						if (!m_pMain->SwitchLight(itt->DevID,switchcmd,ilevel))
						{
							_log.Log(LOG_ERROR,"Error sending switch command, DevID: %llu, Time: %s", itt->DevID, asctime(&ltime));
						}
					}
				}
				if (!AdjustScheduleItem(&*itt))
				{
					//something is wrong, probably no sunset/rise
					itt->startTime+=atime+(24*3600);
				}
			}
		}
	}
}
