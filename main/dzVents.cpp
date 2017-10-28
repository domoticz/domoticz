#include "stdafx.h"
#include "mainworker.h"
#include "SQLHelper.h"
#include "localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/WebServerHelper.h"

extern std::string szUserDataFolder;
extern http::server::CWebServerHelper m_webservers;

CdzVents::CdzVents(void)
{
	m_version = "2.3.0";
}

CdzVents::~CdzVents(void)
{
}

const std::string CdzVents::GetVersion()
{
	return m_version;
}

void CdzVents::SetGlobalVariables(lua_State *lua_state, const int reason)
{
	std::stringstream lua_DirT;

	lua_DirT << szUserDataFolder <<
#ifdef WIN32
	"scripts\\dzVents\\";
#else
	"scripts/dzVents/";
#endif

	lua_pushstring(lua_state, "script_path");
	lua_pushstring(lua_state, lua_DirT.str().c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "script_reason");
	lua_pushstring(lua_state, m_mainworker.m_eventsystem.m_szReason[reason].c_str());
	lua_rawset(lua_state, -3);

	char szTmp[10];
	sprintf(szTmp, "%.02f", 1.23f);
	sprintf(szTmp, "%c", szTmp[1]);
	lua_pushstring(lua_state, "radix_separator");
	lua_pushstring(lua_state, szTmp);
	lua_rawset(lua_state, -3);

	sprintf(szTmp, "%.02f", 1234.56f);
	lua_pushstring(lua_state, "group_separator");
	if (szTmp[1] == '2')
	{
		lua_pushstring(lua_state, "");
	}
	else
	{
		sprintf(szTmp, "%c", szTmp[1]);
		lua_pushstring(lua_state, szTmp);
	}
	lua_rawset(lua_state, -3);

	int rnvalue = 0;
	m_sql.GetPreferencesVar("DzVentsLogLevel", rnvalue);
	lua_pushstring(lua_state, "dzVents_log_level");
	lua_pushnumber(lua_state, (lua_Number)rnvalue);
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "domoticz_listening_port");
	lua_pushstring(lua_state, m_webservers.our_listener_port.c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "domoticz_start_time");
	lua_pushstring(lua_state, m_mainworker.m_eventsystem.m_szStartTime.c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "currentTime");
	lua_pushstring(lua_state, TimeToString(NULL, TF_DateTimeMs).c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "systemUptime");
	lua_pushnumber(lua_state, (lua_Number)SystemUptime());
	lua_rawset(lua_state, -3);
}

void CdzVents::ExportDomoticzDataToLua(lua_State *lua_state, const uint64_t deviceID, const uint64_t varID, const int reason)
{
	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock3(m_mainworker.m_eventsystem.m_devicestatesMutex);
	int index = 1;
	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now, &tm1);
	struct tm tLastUpdate;
	localtime_r(&now, &tLastUpdate);
	int SensorTimeOut = 60;
	m_sql.GetPreferencesVar("SensorTimeout", SensorTimeOut);

	struct tm ntime;
	time_t checktime;

	lua_createtable(lua_state, 0, 0);

	// First export all the devices.
	std::map<uint64_t, CEventSystem::_tDeviceStatus>::iterator iterator;
	for (iterator = m_mainworker.m_eventsystem.m_devicestates.begin(); iterator != m_mainworker.m_eventsystem.m_devicestates.end(); ++iterator)
	{
		CEventSystem::_tDeviceStatus sitem = iterator->second;
		const char *dev_type = RFX_Type_Desc(sitem.devType, 1);
		const char *sub_type = RFX_Type_SubType_Desc(sitem.devType, sitem.subType);

		//_log.Log(LOG_STATUS, "Getting device with id: %s", rowid.c_str());

		ParseSQLdatetime(checktime, ntime, sitem.lastUpdate, tm1.tm_isdst);
		bool timed_out = (now - checktime >= SensorTimeOut * 60);

		lua_pushnumber(lua_state, (lua_Number)index);

		lua_createtable(lua_state, 1, 11);

		lua_pushstring(lua_state, "name");
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "id");
		lua_pushnumber(lua_state, (lua_Number)sitem.ID);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "baseType");
		lua_pushstring(lua_state, "device");
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "deviceType");
		lua_pushstring(lua_state, dev_type);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "subType");
		lua_pushstring(lua_state, sub_type);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "switchType");
		lua_pushstring(lua_state, Switch_Type_Desc((_eSwitchType)sitem.switchtype));
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "switchTypeValue");
		lua_pushnumber(lua_state, (lua_Number)sitem.switchtype);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "lastUpdate");
		lua_pushstring(lua_state, sitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "lastLevel");
		lua_pushnumber(lua_state, (lua_Number)sitem.lastLevel);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "changed");
		if (sitem.ID == deviceID && reason == m_mainworker.m_eventsystem.REASON_DEVICE)
			lua_pushboolean(lua_state, true);
		else
			lua_pushboolean(lua_state, false);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "timedOut");
		if (timed_out == true)
			lua_pushboolean(lua_state, true);
		else
			lua_pushboolean(lua_state, false);
		lua_rawset(lua_state, -3);

		//get all svalues separate
		std::vector<std::string> strarray;
		StringSplit(sitem.sValue, ";", strarray);

		lua_pushstring(lua_state, "rawData");
		lua_createtable(lua_state, 0, 0);

		for (uint8_t index2 = 0; index2 < strarray.size(); index2++)
		{
			lua_pushnumber(lua_state, (lua_Number)index2 + 1);
			lua_pushstring(lua_state, strarray[index2].c_str());
			lua_rawset(lua_state, -3);
		}
		lua_settable(lua_state, -3); // rawData table

		lua_pushstring(lua_state, "deviceID");
		lua_pushstring(lua_state, sitem.deviceID.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "description");
		lua_pushstring(lua_state, sitem.description.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "batteryLevel");
		lua_pushnumber(lua_state, (lua_Number)sitem.batteryLevel);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "signalLevel");
		lua_pushnumber(lua_state, (lua_Number)sitem.signalLevel);
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "data");
		lua_createtable(lua_state, 0, 0);

		lua_pushstring(lua_state, "_state");
		lua_pushstring(lua_state, sitem.nValueWording.c_str());
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "_nValue");
		lua_pushnumber(lua_state, (lua_Number)sitem.nValue);
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "hardwareID");
		lua_pushnumber(lua_state, (lua_Number)sitem.hardwareID);
		lua_rawset(lua_state, -3);

		// Lux does not have it's own field yet.
		if (sitem.devType == pTypeLux && sitem.subType == sTypeLux)
		{
			lua_pushstring(lua_state, "lux");
			if (strarray.size() > 0)
				lua_pushnumber(lua_state, (lua_Number)atoi(strarray[0].c_str()));
			else
				lua_pushnumber(lua_state, (lua_Number)0);
			lua_rawset(lua_state, -3);
		}

		if (sitem.devType == pTypeGeneral && sitem.subType == sTypeKwh)
		{
			lua_pushstring(lua_state, "whTotal");
			if (strarray.size() > 1)
				lua_pushnumber(lua_state, atof(strarray[1].c_str()));
			else
				lua_pushnumber(lua_state, 0.0f);
			lua_rawset(lua_state, -3);
			lua_pushstring(lua_state, "whActual");
			if (strarray.size() > 0)
				lua_pushnumber(lua_state, atof(strarray[0].c_str()));
			else
				lua_pushnumber(lua_state, 0.0f);
			lua_rawset(lua_state, -3);
		}

		// Now see if we have additional fields from the JSON data
		if (sitem.JsonMapString.size() > 0)
		{
			std::map<uint8_t, std::string>::const_iterator itt;
			for (itt = sitem.JsonMapString.begin(); itt != sitem.JsonMapString.end(); ++itt)
			{
				lua_pushstring(lua_state, m_mainworker.m_eventsystem.JsonMap[itt->first].szNew);
				lua_pushstring(lua_state, itt->second.c_str());
				lua_rawset(lua_state, -3);
			}
		}

		if (sitem.JsonMapFloat.size() > 0)
		{
			std::map<uint8_t, float>::const_iterator itt;
			for (itt = sitem.JsonMapFloat.begin(); itt != sitem.JsonMapFloat.end(); ++itt)
			{
				lua_pushstring(lua_state, m_mainworker.m_eventsystem.JsonMap[itt->first].szNew);
				lua_pushnumber(lua_state, itt->second);
				lua_rawset(lua_state, -3);
			}
		}

		if (sitem.JsonMapInt.size() > 0)
		{
			std::map<uint8_t, int>::const_iterator itt;
			for (itt = sitem.JsonMapInt.begin(); itt != sitem.JsonMapInt.end(); ++itt)
			{
				lua_pushstring(lua_state, m_mainworker.m_eventsystem.JsonMap[itt->first].szNew);
				lua_pushnumber(lua_state, itt->second);
				lua_rawset(lua_state, -3);
			}
		}

		if (sitem.JsonMapBool.size() > 0)
		{
			std::map<uint8_t, bool>::const_iterator itt;
			for (itt = sitem.JsonMapBool.begin(); itt != sitem.JsonMapBool.end(); ++itt)
			{
				lua_pushstring(lua_state, m_mainworker.m_eventsystem.JsonMap[itt->first].szNew);
				lua_pushboolean(lua_state, itt->second);
				lua_rawset(lua_state, -3);
			}
		}

		lua_settable(lua_state, -3); // data table
		lua_settable(lua_state, -3); // device entry
		index++;
	}
	devicestatesMutexLock3.unlock();

	// Now do the scenes and groups.
	const char *description = "";
	boost::shared_lock<boost::shared_mutex> scenesgroupsMutexLock(m_mainworker.m_eventsystem.m_scenesgroupsMutex);

	std::map<uint64_t, CEventSystem::_tScenesGroups>::const_iterator ittScenes;
	for (ittScenes = m_mainworker.m_eventsystem.m_scenesgroups.begin(); ittScenes != m_mainworker.m_eventsystem.m_scenesgroups.end(); ++ittScenes)
	{
		CEventSystem::_tScenesGroups sgitem = ittScenes->second;

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT Description FROM Scenes WHERE (ID=='%d')", sgitem.ID);
		if (result.size() == 0)
			description = "";
		else
			description = result[0][0].c_str();

		lua_pushnumber(lua_state, (lua_Number)index);

		lua_createtable(lua_state, 1, 6);

		lua_pushstring(lua_state, "name");
		lua_pushstring(lua_state, sgitem.scenesgroupName.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "id");
		lua_pushnumber(lua_state, (lua_Number)sgitem.ID);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "description");
		lua_pushstring(lua_state, description);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "baseType");
		if (sgitem.scenesgroupType == 0)
			lua_pushstring(lua_state, "scene");
		else
			lua_pushstring(lua_state, "group");
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "lastUpdate");
		lua_pushstring(lua_state, sgitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "changed");
		if (sgitem.ID == deviceID && reason == m_mainworker.m_eventsystem.REASON_SCENEGROUP)
			lua_pushboolean(lua_state, true);
		else
			lua_pushboolean(lua_state, false);
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "data");
		lua_createtable(lua_state, 0, 0);

		lua_pushstring(lua_state, "_state");
		lua_pushstring(lua_state, sgitem.scenesgroupValue.c_str());
		lua_rawset(lua_state, -3);

		lua_settable(lua_state, -3); // data table
		lua_settable(lua_state, -3); // end entry
		index++;
	}
	scenesgroupsMutexLock.unlock();

	std::string vtype;

	// Now do the user variables.
	boost::shared_lock<boost::shared_mutex> uservariablesMutexLock(m_mainworker.m_eventsystem.m_uservariablesMutex);
	std::map<uint64_t, CEventSystem::_tUserVariable>::const_iterator it_var;
	for (it_var = m_mainworker.m_eventsystem.m_uservariables.begin(); it_var != m_mainworker.m_eventsystem.m_uservariables.end(); ++it_var)
	{
		CEventSystem::_tUserVariable uvitem = it_var->second;

		lua_pushnumber(lua_state, (lua_Number)index);

		lua_createtable(lua_state, 1, 5);

		lua_pushstring(lua_state, "name");
		lua_pushstring(lua_state, uvitem.variableName.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "id");
		lua_pushnumber(lua_state, (lua_Number)uvitem.ID);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "baseType");
		lua_pushstring(lua_state, "uservariable");
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "lastUpdate");
		lua_pushstring(lua_state, uvitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "changed");
		if (uvitem.ID == varID)
		{
			lua_pushboolean(lua_state, true);
		}
		else
		{
			lua_pushboolean(lua_state, false);
		}
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "data");
		lua_createtable(lua_state, 0, 0);

		lua_pushstring(lua_state, "value");
		if (uvitem.variableType == 0)
		{
			//Integer
			lua_pushnumber(lua_state, atoi(uvitem.variableValue.c_str()));
			vtype = "integer";
		}
		else if (uvitem.variableType == 1)
		{
			//Float
			lua_pushnumber(lua_state, atof(uvitem.variableValue.c_str()));
			vtype = "float";
		}
		else
		{
			//String,Date,Time
			lua_pushstring(lua_state, uvitem.variableValue.c_str());
			if (uvitem.variableType == 2)
			{
				vtype = "string";
			}
			else if (uvitem.variableType == 3)
			{
				vtype = "date";
			}
			else if (uvitem.variableType == 4)
			{
				vtype = "time";
			}
			else
			{
				vtype = "unknown";
			}
		}
		lua_rawset(lua_state, -3);

		lua_settable(lua_state, -3); // data table

		lua_pushstring(lua_state, "variableType");
		lua_pushstring(lua_state, vtype.c_str());
		lua_rawset(lua_state, -3);

		lua_settable(lua_state, -3); // end entry

		index++;
	}

	lua_setglobal(lua_state, "domoticzData");
}
