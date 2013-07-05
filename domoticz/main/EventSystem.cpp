#include "stdafx.h"
#include "EventSystem.h"
#include "mainworker.h"
#include "localtime_r.h"
#include "Logger.h"

extern "C" {
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
}

CEventSystem::CEventSystem(void)
{
	m_pLUA=luaL_newstate();
	if (m_pLUA!=NULL)
	{
		luaL_openlibs(m_pLUA);
	}
	m_pMain=NULL;
	m_stoprequested=false;
}


CEventSystem::~CEventSystem(void)
{
	StopEventSystem();
	if (m_pLUA!=NULL)
	{
		lua_close(m_pLUA);
		m_pLUA=NULL;
	}
}

void CEventSystem::StartEventSystem(MainWorker *pMainWorker)
{
	m_pMain=pMainWorker;

	LoadEvents();

	m_secondcounter=(58*2);

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEventSystem::Do_Work, this)));
}

void CEventSystem::StopEventSystem()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
}

void CEventSystem::LoadEvents()
{
	boost::lock_guard<boost::mutex> l(eventMutex);
	m_events.clear();
/*
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Var1,Var2 FROM Database ORDER BY ID";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
		}
	}
*/

	//test
	_tEventItem eitem;
	eitem.ID=1234;
	eitem.szRules="IfThenElse";

	m_events.push_back(eitem);

	//test2
	eitem.ID=5678;
	eitem.szRules="If>Or<Set";

	m_events.push_back(eitem);
}

void CEventSystem::Do_Work()
{
	m_stoprequested=false;

	time_t lasttime=time(NULL);

	while (!m_stoprequested)
	{
		//sleep 1 second
		boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		m_secondcounter++;
		if (m_secondcounter==60*2)
		{
			m_secondcounter=0;
			ProcessMinute();
		}
	}
	_log.Log(LOG_NORM,"EventSystem stopped...");

}

void CEventSystem::ProcessMinute()
{
	boost::lock_guard<boost::mutex> l(eventMutex);

	std::vector<_tEventItem>::iterator itt;
	for (itt=m_events.begin(); itt!=m_events.end(); ++itt)
	{
		if (itt->ID==1234)
		{

		}
	}
}

bool CEventSystem::ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string devname)
{
	boost::lock_guard<boost::mutex> l(eventMutex);

	//do something

	return true;
}

