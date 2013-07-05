#include "stdafx.h"
#include "EventSystem.h"
#include "mainworker.h"
#include "localtime_r.h"
#include "Logger.h"
#include <iostream>
#include <ctime>
#include <boost/algorithm/string.hpp>

extern "C" {
#include "../lua/src/lua.h"    
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

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Name,Conditions,Actions FROM Events ORDER BY ID";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
            
			_tEventItem eitem;
			std::stringstream s_str( sd[0] );
			s_str >> eitem.ID;
            eitem.Name		= sd[1];
            eitem.Conditions	= sd[2];
			eitem.Actions		= sd[3];
            m_events.push_back(eitem);
        }
        _log.Log(LOG_NORM,"Events (re)loaded");
	}
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
    
	EvaluateEvent("Time", 0);

}

void CEventSystem::GetCurrentStates()
{
    
    boost::lock_guard<boost::mutex> l(deviceStateMutex);
    m_devicestates.clear();
    
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,nValue,sValue FROM DeviceStatus WHERE (Used = '1') ORDER BY ID DESC";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
            std::vector<std::string> sd=*itt;
            _tDeviceStatus sitem;
			std::stringstream s_str( sd[0] );
			s_str >> sitem.ID;
            std::stringstream nv_str( sd[1] );
			nv_str >> sitem.nValue;
            sitem.sValue	= sd[2];
            m_devicestates.push_back(sitem);
        }
        //_log.Log(LOG_NORM,"Events: device states (re)loaded");
	}
    
    
}


bool CEventSystem::ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string devname)
{
    EvaluateEvent("Device", ulDevID);
    
	return true;
}

void CEventSystem::EvaluateEvent(const std::string reason, const unsigned long long DeviceID)
{

    time_t theTime = time(NULL);
    aTime = localtime(&theTime);
    char timeBuffer[100];

    sprintf(timeBuffer, "%d:%d", aTime->tm_hour, aTime->tm_min);
    
    if (reason=="Time") {_log.Log(LOG_NORM,"EventSystem %s trigger: %s",reason.c_str(),timeBuffer);};
    if (reason=="Device") {_log.Log(LOG_NORM,"EventSystem %s trigger: deviceno: %d",reason.c_str(),DeviceID);};
   
    GetCurrentStates();
    
    // pass conditions and actions over to lua here
    // trigger custom lua scripts here
    

}

