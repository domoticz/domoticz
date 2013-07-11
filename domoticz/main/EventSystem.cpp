#include "stdafx.h"
#include "mainworker.h"
#include "RFXNames.h"
#include "EventSystem.h"
#include "Helper.h"
#include "SQLHelper.h"
#include "Logger.h"
#include <iostream>
#include <boost/lexical_cast.hpp>

extern std::string szStartupFolder;

std::string lua_Dir = szStartupFolder;

#ifdef WIN32
    #include "../main/dirent_windows.h"
#else
    #include <dirent.h>
#endif

extern "C" {
#include "../lua/src/lua.h"    
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
}

CEventSystem::CEventSystem(void)
{
	/*
    m_pLUA=luaL_newstate();
	if (m_pLUA!=NULL)
	{
		luaL_openlibs(m_pLUA);
	}
    */
	m_pMain=NULL;
	m_stoprequested=false;
}


CEventSystem::~CEventSystem(void)
{
	StopEventSystem();
    /*
	if (m_pLUA!=NULL)
	{
		lua_close(m_pLUA);
		m_pLUA=NULL;
	}
    */
}

void CEventSystem::StartEventSystem(MainWorker *pMainWorker)
{
	m_pMain=pMainWorker;

	LoadEvents();
    GetCurrentStates();

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
            
			if (sd.size()==4)
			{
				_tEventItem eitem;
				std::stringstream s_str( sd[0] );
				s_str >> eitem.ID;
				eitem.Name		= sd[1];
				eitem.Conditions	= sd[2];
				eitem.Actions		= sd[3];
				m_events.push_back(eitem);
			}
			else
			{
				m_pMain->m_sql.query("DROP TABLE IF EXISTS [Events]");
				return;
			}
        }
#ifdef _DEBUG
        _log.Log(LOG_NORM,"Events (re)loaded");
#endif
	}
}

void CEventSystem::Do_Work()
{
	m_stoprequested=false;

	time_t lasttime=time(NULL);

	while (!m_stoprequested)
	{
		//sleep 500 milliseconds
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
    
	EvaluateEvent("time");

}

void CEventSystem::GetCurrentStates()
{
    
    boost::lock_guard<boost::mutex> l(deviceStateMutex);
    m_devicestates.clear();
    
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Name,nValue,sValue, Type, SubType, SwitchType, LastUpdate FROM DeviceStatus WHERE (Used = '1')";
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
            sitem.deviceName = sd[1];
            std::stringstream nv_str( sd[2] );
			nv_str >> sitem.nValue;
            sitem.sValue	= sd[3];
			_eSwitchType switchtype=(_eSwitchType)atoi(sd[6].c_str());
            sitem.nValueWording = nValueToWording(atoi(sd[4].c_str()), atoi(sd[5].c_str()), switchtype, (unsigned char)sitem.nValue,sitem.sValue);
            sitem.lastUpdate = sd[7];
            m_devicestates[sitem.ID] = sitem;
        }
  	}
    
    
}

std::string CEventSystem::UpdateSingleState(unsigned long long ulDevID, std::string devname, const int nValue, const char* sValue, const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, std::string lastUpdate)
{
    
    boost::lock_guard<boost::mutex> l(deviceStateMutex);
    
    std::string nValueWording = nValueToWording(devType, subType, switchType, nValue, sValue);
    
    std::map<unsigned long long,_tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);
    if (itt != m_devicestates.end()) {
		//Update
        _tDeviceStatus replaceitem = itt->second;
        replaceitem.deviceName = devname;
        replaceitem.nValue = nValue;
        replaceitem.sValue = sValue;
        replaceitem.nValueWording = nValueWording;
        replaceitem.lastUpdate = lastUpdate;
        itt->second = replaceitem;
    }
    else {
		//Insert
        _tDeviceStatus newitem;
        newitem.ID = ulDevID;
        newitem.deviceName = devname;
        newitem.nValue = nValue;
        newitem.sValue = sValue;
        newitem.nValueWording = nValueWording;
        newitem.lastUpdate = lastUpdate;
        m_devicestates[newitem.ID] = newitem;
    }
    return nValueWording;
}


bool CEventSystem::ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string devname)
{
    // query to get switchtype & LastUpdate, can't seem to get it from SQLHelper? 
    std::vector<std::vector<std::string> > result;
    std::stringstream szQuery;
    szQuery << "SELECT ID, SwitchType, LastUpdate FROM DeviceStatus WHERE (Name == '" << devname << "')";
    result=m_pMain->m_sql.query(szQuery.str());
    if (result.size()>0) {
        std::vector<std::string> sd=result[0];
        _eSwitchType switchType=(_eSwitchType)atoi(sd[1].c_str());
       
        std::string nValueWording = UpdateSingleState(ulDevID, devname, nValue, sValue, devType, subType, switchType, sd[2]);
        EvaluateEvent("device", ulDevID, devname, nValue, sValue, nValueWording);
    }
    else {
        _log.Log(LOG_ERROR,"Could not determine switch type for event device %s",devname.c_str());
    }
    
	return true;
}


void CEventSystem::EvaluateEvent(const std::string reason)
{
    EvaluateEvent(reason, 0, "", 0, "", "");
}

void CEventSystem::EvaluateEvent(const std::string reason, const unsigned long long DeviceID, const std::string devname, const int nValue, const char* sValue, std::string nValueWording)
{

    std::string lua_Dir = szStartupFolder;
    
    #ifdef WIN32
        lua_Dir.append("scripts\\lua\\");
    #else
        lua_Dir.append("scripts/lua/");
    #endif
    
    DIR *lDir;
    struct dirent *ent;
    
	if ((lDir = opendir(lua_Dir.c_str())) != NULL)
	{
     	while ((ent = readdir (lDir)) != NULL)
		{
			std::string filename = ent->d_name;
			if (ent->d_type==DT_REG) {
                if ((filename.find("_device_") != -1) && (reason == "device") && (filename.find("_demo.lua") == -1)) {
                    //_log.Log(LOG_NORM,"found device file: %s",filename.c_str());
                    EvaluateLua(reason,lua_Dir+filename, DeviceID, devname, nValue, sValue, nValueWording);
                }
                if (((filename.find("_time_") != -1)) && (reason == "time") && (filename.find("_demo.lua") == -1)) {
                    //_log.Log(LOG_NORM,"found time file: %s",filename.c_str());
                    EvaluateLua(reason,lua_Dir+filename);
                }
			}
		}
		closedir(lDir);
	}
    else {
        _log.Log(LOG_ERROR,"Error accessing lua script directory %s", lua_Dir.c_str());
    }
}

void CEventSystem::EvaluateLua(const std::string reason, const std::string filename)
{
    EvaluateLua(reason, filename, 0, "", 0, "", "");
}

void CEventSystem::EvaluateLua(const std::string reason, const std::string filename, const unsigned long long DeviceID, const std::string devname, const int nValue, const char* sValue, std::string nValueWording)
{

    lua_State *lua_state;
    lua_state = luaL_newstate();
  
    // load Lua libraries
    static const luaL_Reg lualibs[] =
    {
        {"base", luaopen_base},
        {"io", luaopen_io},
        {"table", luaopen_table},
        {"string", luaopen_string},
        {"math", luaopen_math},
        {NULL, NULL}
    };
 
    const luaL_Reg *lib = lualibs;
    for(; lib->func != NULL; lib++)
    {
        lib->func(lua_state);
        lua_settop(lua_state, 0);
    }
    
    // reroute print library to domoticz logger
    luaL_openlibs(lua_state);
    lua_pushcfunction(lua_state, l_domoticz_print);
    lua_setglobal(lua_state, "print");
    
#ifdef _DEBUG
    _log.Log(LOG_NORM,"EventSystem %s trigger",reason.c_str());
#endif
    
    if (reason == "device") {
        lua_createtable(lua_state, 1, 0);
        lua_pushstring( lua_state, devname.c_str() );
        lua_pushstring( lua_state, nValueWording.c_str() ); 
        lua_rawset( lua_state, -3 );
        lua_pushstring( lua_state, "svalues" );
        lua_pushstring( lua_state, sValue );
        lua_rawset( lua_state, -3 );
        lua_setglobal(lua_state, "devicechanged");
    }

    lua_createtable(lua_state, m_devicestates.size(), 0);
    typedef std::map<unsigned long long,_tDeviceStatus>::iterator it_type;
    for(it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++) {
        _tDeviceStatus sitem = iterator->second;
        lua_pushstring( lua_state, sitem.deviceName.c_str() );
        lua_pushstring( lua_state, sitem.nValueWording.c_str() );
        lua_rawset( lua_state, -3 );
    }
    lua_setglobal(lua_state, "otherdevices");

    lua_createtable(lua_state, m_devicestates.size(), 0);
    typedef std::map<unsigned long long,_tDeviceStatus>::iterator it_type;
    for(it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++) {
        _tDeviceStatus sitem = iterator->second;
        lua_pushstring( lua_state, sitem.deviceName.c_str() );
        lua_pushstring( lua_state, sitem.lastUpdate.c_str() );
        lua_rawset( lua_state, -3 );
    }
    lua_setglobal(lua_state, "otherdevices_lastupdate");
    
    lua_createtable(lua_state, m_devicestates.size(), 0);
    typedef std::map<unsigned long long,_tDeviceStatus>::iterator it_type;
    for(it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++) {
        _tDeviceStatus sitem = iterator->second;
        lua_pushstring( lua_state, sitem.deviceName.c_str() );
        lua_pushstring( lua_state, sitem.sValue.c_str() );
        lua_rawset( lua_state, -3 );
    }
    lua_setglobal(lua_state, "otherdevices_svalues");
    
    //TBD: blockly parse using luaL_dostring(lua_state, blockly conditions"); or lua_eval?
    
    int status = luaL_loadfile(lua_state, filename.c_str());
    
    int result = 0;
    if(status == LUA_OK)
    {
        result = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
    }
    else
    {
        _log.Log(LOG_ERROR,"Could not load lua script %s",filename.c_str());
    }
    
    report_errors(lua_state, result);
    
    bool scriptTrue = false;
    lua_getglobal(lua_state, "commandArray");
    if( lua_istable( lua_state, -1 ) ) {
        int tIndex=lua_gettop(lua_state);
        lua_pushnil(lua_state); // first key
        while (lua_next(lua_state, tIndex) != 0) {

            if ((std::string(luaL_typename(lua_state, -2))=="string") && (std::string(luaL_typename(lua_state, -1)))=="string") {
                scriptTrue = true;
                if (std::string(lua_tostring(lua_state, -2))== "SendNotification") {
                    std::string luaString = lua_tostring(lua_state, -1);
                    SendEventNotification(luaString.substr(0,luaString.find('#')), luaString.substr(luaString.find('#')+1));
                }
                else {
                    ScheduleEvent(lua_tostring(lua_state, -2),lua_tostring(lua_state, -1));
                }
            }
            else {
                _log.Log(LOG_ERROR,"commandArray should only return ['string']='Actionstring'");
            }
            // removes 'value'; keeps 'key' for next iteration
            lua_pop(lua_state, 1);
        }
    }
    else {
        _log.Log(LOG_ERROR,"Lua script did not return a commandArray");
    }
    
    if (scriptTrue) {
        _log.Log(LOG_NORM,"Event triggered: %s",filename.c_str());
    }
    
    lua_close(lua_state);

}

void CEventSystem::SendEventNotification(const std::string Subject, const std::string Body)
{
    m_pMain->m_sql.SendNotificationEx(Subject,Body);
}

void CEventSystem::ScheduleEvent(std::string deviceName, std::string Action)
{
    
    unsigned char _level = 0;
    if (Action.substr(0,9) == "Set Level") {
        _level = atoi(Action.substr(11).c_str());
        Action = Action.substr(0,9);
    }
#ifdef _DEBUG
    _log.Log(LOG_NORM,"Setting device %s to state %s", deviceName.c_str(),Action.c_str());
#endif
    
    std::vector<std::vector<std::string> > result;
    std::stringstream szQuery;
    szQuery << "SELECT ID FROM DeviceStatus WHERE (Name == '" << deviceName << "')";
    result=m_pMain->m_sql.query(szQuery.str());
    if (result.size()>0) {
        std::vector<std::string> sd=result[0];
        int DelayTime=1;
        int idx = atoi(sd[0].c_str());
        
        _tTaskItem tItem=_tTaskItem::SwitchLightEvent(DelayTime,idx,Action,_level);
        
        m_pMain->m_sql.AddTaskItem(tItem);
        
    }
    else {
        _log.Log(LOG_ERROR,"Could not find device '%s' mentioned in script", deviceName.c_str());
    }
}

std::string CEventSystem::nValueToWording (const unsigned char dType, const unsigned char dSubType, const _eSwitchType switchtype, const unsigned char nValue,const std::string sValue)
{
    
    std::string lstatus="";
    int llevel=0;
    bool bHaveDimmer=false;
    bool bHaveGroupCmd=false;
    int maxDimLevel=0;
    
    GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
    
    
    if (switchtype==STYPE_Dimmer)
    {
        //?
    }
    else if (switchtype==STYPE_Contact)
    {
        if (lstatus=="On") {
            lstatus="Open";
        } else {
            lstatus="Closed";
        }
    }
    else if ((switchtype==STYPE_Blinds) || (switchtype==STYPE_BlindsInverted))
    {
        if (lstatus=="On") {
            lstatus="Closed";
        } else {
            lstatus="Open";
        }
    }
    
    return lstatus;
    
}


int CEventSystem::l_domoticz_print(lua_State* lua_state) {
    int nargs = lua_gettop(lua_state);
    
    for (int i=1; i <= nargs; i++) {
        if (lua_isstring(lua_state, i)) {
            _log.Log(LOG_NORM,"LUA: %s",lua_tostring(lua_state, i));
        }
        else {
            /* non strings? */
        }
    }
    
    return 0;
}

void CEventSystem::report_errors(lua_State *lua_state, int status)
{
    if ( status!=0 ) {
        _log.Log(LOG_ERROR,"Lua parse error: %s", lua_tostring(lua_state, -1));
        lua_pop(lua_state, 1); // remove error message
    }
}
  
    
    



