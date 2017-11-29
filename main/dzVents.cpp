#include "stdafx.h"
#include "mainworker.h"
#include "SQLHelper.h"
#include "localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/WebServerHelper.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

extern std::string szUserDataFolder;
extern http::server::CWebServerHelper m_webservers;

CdzVents::CdzVents(void)
{
	m_version = "2.4.0";
}

CdzVents::~CdzVents(void)
{
}

const std::string CdzVents::GetVersion()
{
	return m_version;
}

float CdzVents::RandomTime(const int randomTime)
{
	srand((unsigned int)mytime(NULL));
	return ((float)(rand() / (RAND_MAX / randomTime)));
}

void CdzVents::ProcessHttpResponse(lua_State *lua_state, const std::vector<std::string> &vData, const std::string &sValue, const std::string &nValueWording)
{
	int statusCode;
	lua_createtable(lua_state, 0, 0);
	lua_pushstring(lua_state, "headers");
	lua_createtable(lua_state, (int)vData.size(), 0);
	if (vData.size() > 0)
	{
		std::vector<std::string>::const_iterator itt;
		for (itt = vData.begin(); itt != vData.end() - 1; itt++)
		{
			size_t pos = (*itt).find(": ");
			if (pos != std::string::npos)
			{
				lua_pushstring(lua_state, (*itt).substr(0, pos).c_str());
				lua_pushstring(lua_state, (*itt).substr(pos + 2).c_str());
				lua_rawset(lua_state, -3);
			}
		}
		// last item in vector is always the status code
		itt = vData.end() - 1;
		std::stringstream ss(*itt);
		ss >> statusCode;
	}
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "statusCode");
	lua_pushnumber(lua_state, (lua_Number)statusCode);
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "data");
	lua_pushstring(lua_state, sValue.c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "callback");
	lua_pushstring(lua_state, nValueWording.c_str());
	lua_rawset(lua_state, -3);
	lua_setglobal(lua_state, "httpresponse");
}

bool CdzVents::OpenURL(lua_State *lua_state, const std::map<int, _tLuaTableValues> &mLuaTable)
{
	float delayTime = 0;
	std::string URL, extraHeaders, method, postData, trigger;

	std::map<int, _tLuaTableValues>::const_iterator itt;
	for (itt = mLuaTable.begin(); itt != mLuaTable.end(); itt++)
	{
		if (itt->second.isTable && itt->second.sValue == "headers")
		{
			if (itt == mLuaTable.end())
				break;

			int tIndex = itt->second.tIndex;
			itt++;
			std::map<int, _tLuaTableValues>::const_iterator itt2;
			for (itt2 = itt; itt2 != mLuaTable.end(); itt2++)
			{
				if (itt2->second.tIndex != tIndex)
				{
					itt--;
					break;
				}
				extraHeaders += "!#" + itt2->second.name + ": " + itt2->second.sValue;
				if (itt != mLuaTable.end())
					itt++;
			}
		}
		else if (itt->second.type == TYPE_STRING)
		{
			if (itt->second.name == "URL")
				URL = itt->second.sValue;
			else if (itt->second.name == "method")
				method = itt->second.sValue;
			else if (itt->second.name == "postdata")
				postData = itt->second.sValue;
			else if (itt->second.name == "_trigger")
				trigger = itt->second.sValue;
		}
		else if (itt->second.type == TYPE_INTEGER)
		{
			if (itt->second.name == "_random")
				delayTime = RandomTime(itt->second.iValue);
			else if (itt->second.name == "_after")
				delayTime = static_cast<float>(itt->second.iValue);
		}
	}
	if (URL.empty())
	{
		_log.Log(LOG_ERROR, "dzVents: No URL supplied!");
		return false;
	}

	HTTPClient::_eHTTPmethod eMethod = HTTPClient::HTTP_METHOD_GET; // defaults to GET
	if (!method.empty() && method == "POST")
		eMethod = HTTPClient::HTTP_METHOD_POST;

	if (!postData.empty() && eMethod != HTTPClient::HTTP_METHOD_POST)
	{
		_log.Log(LOG_ERROR, "dzVents: You can only use postdata with method POST..");
		return false;
	}

	m_sql.AddTaskItem(_tTaskItem::GetHTTPPage(delayTime, URL, extraHeaders, eMethod, postData, trigger));
	return true;
}

bool CdzVents::UpdateDevice(lua_State *lua_state, const std::map<int, _tLuaTableValues> &mLuaTable)
{
	std::string luaString, szIdx, nValue, sValue, Protected;
	uint64_t ulIdx;
	float delayTime = 0;
	bool bEventTrigger = false;

	std::map<int, _tLuaTableValues>::const_iterator itt;
	for (itt = mLuaTable.begin(); itt != mLuaTable.end(); itt++)
	{
		if (itt->second.type == TYPE_INTEGER)
		{
			std::stringstream ss;
			ss << itt->second.iValue;
			if (itt->second.name == "idx")
			{
				ulIdx = static_cast<uint64_t>(itt->second.iValue);
				szIdx = ss.str();
			}
			else if (itt->second.name == "nValue")
				nValue = ss.str();
			else if (itt->second.name == "protected")
				Protected = ss.str();
			else if (itt->second.name == "_random")
				delayTime = RandomTime(itt->second.iValue);
			else if (itt->second.name == "_after")
				delayTime = static_cast<float>(itt->second.iValue);
		}
		else if (itt->second.type == TYPE_STRING)
		{
			if (itt->second.name == "sValue")
				sValue = itt->second.sValue;
		}
		else if (itt->second.type == TYPE_BOOLEAN && itt->second.name == "_trigger")
				bEventTrigger = true;
	}
	if (szIdx.empty())
		return false;

	luaString = szIdx + "|";

	if (!nValue.empty())
		luaString += nValue;

	luaString += "|";
	if (!sValue.empty())
		luaString += sValue;

	luaString += "|";
	if (!Protected.empty())
		luaString += Protected;

	m_sql.AddTaskItem(_tTaskItem::UpdateDevice(delayTime, ulIdx, luaString, bEventTrigger));
	return true;
}

bool CdzVents::UpdateVariable(lua_State *lua_state, const std::map<int, _tLuaTableValues> &mLuaTable)
{
	std::string variableName, variableValue;
	float delayTime = 0;
	bool bEventTrigger = false;
	uint64_t idx;

	std::map<int, _tLuaTableValues>::const_iterator itt;
	for (itt = mLuaTable.begin(); itt != mLuaTable.end(); itt++)
	{
		if (itt->second.type == TYPE_INTEGER)
		{
			if (itt->second.name == "idx")
				idx = static_cast<uint64_t>(itt->second.iValue);
			else if (itt->second.name == "_random")
				delayTime = RandomTime(itt->second.iValue);
			else if (itt->second.name == "_after")
				delayTime = static_cast<float>(itt->second.iValue);
		}
		else if (itt->second.type == TYPE_STRING && itt->second.name == "value")
			variableValue = itt->second.sValue;

		else if (itt->second.type == TYPE_BOOLEAN && itt->second.name == "_trigger")
			bEventTrigger = true;
	}
	if (bEventTrigger)
		m_mainworker.m_eventsystem.SetEventTrigger(idx, m_mainworker.m_eventsystem.REASON_USERVARIABLE, delayTime);

	m_sql.AddTaskItem(_tTaskItem::SetVariable(delayTime, idx, variableValue, false));
	return true;
}

bool CdzVents::CancelItem(lua_State *lua_state, const std::map<int, _tLuaTableValues> &mLuaTable)
{
	uint64_t idx;
	int count = 0;
	std::string type;

	std::map<int, _tLuaTableValues>::const_iterator itt;
	for (itt = mLuaTable.begin(); itt != mLuaTable.end(); itt++)
	{
		if (itt->second.type == TYPE_INTEGER && itt->second.name == "idx")
			idx = static_cast<uint64_t>(itt->second.iValue);

		else if (itt->second.type == TYPE_STRING && itt->second.name == "type")
			type = itt->second.sValue;
	}
	_tTaskItem tItem;
	tItem._idx = idx;
	tItem._DelayTime = 0;
	if (type == "device")
		tItem._ItemType = TITEM_SWITCHCMD_EVENT;
	else if (type == "scene")
		tItem._ItemType = TITEM_SWITCHCMD_SCENE;
	else if (type == "variable")
		tItem._ItemType = TITEM_SET_VARIABLE;

	m_sql.AddTaskItem(tItem, true);
	return true;
}

bool CdzVents::processLuaCommand(lua_State *lua_state, const std::string &filename, const int tIndex)
{
	bool scriptTrue = false;
	std::string lCommand = std::string(lua_tostring(lua_state, -2));
	std::map<int, _tLuaTableValues> mLuaTable;
	IterateTable(lua_state, tIndex, 0, mLuaTable);
	if (mLuaTable.size() > 0)
	{
		if (lCommand == "OpenURL")
			scriptTrue = OpenURL(lua_state, mLuaTable);

		else if (lCommand == "UpdateDevice")
			scriptTrue = UpdateDevice(lua_state, mLuaTable);

		else if (lCommand == "Variable")
			scriptTrue = UpdateVariable(lua_state, mLuaTable);

		else if (lCommand == "Cancel")
			scriptTrue = CancelItem(lua_state, mLuaTable);
	}
	return scriptTrue;
}

bool CdzVents::IterateTable(lua_State *lua_state, const int tIndex, int index, std::map<int, _tLuaTableValues> &mLuaTable)
{
	_tLuaTableValues item;
	item.isTable = false;
	static int indexCount;
	indexCount = index;

	lua_pushnil(lua_state);
	while (lua_next(lua_state, tIndex) != 0)
	{
		item.type = TYPE_UNKNOWN;
		item.isTable = false;
		item.tIndex = tIndex;
		indexCount++;
		if (lua_istable(lua_state, -1))
		{
			if (std::string(luaL_typename(lua_state, -2)) == "string")
			{
				item.type = TYPE_STRING;
				item.sValue = std::string(lua_tostring(lua_state, -2));
			}
			else if (std::string(luaL_typename(lua_state, -2)) == "number")
			{
				item.type = TYPE_INTEGER;
				item.iValue = lua_tonumber(lua_state, -2);
			}
			else
				continue;
			item.isTable = true;
			item.tIndex += 2;
			mLuaTable.insert(std::pair<int, _tLuaTableValues> (indexCount, item));
			IterateTable(lua_state, item.tIndex, indexCount, mLuaTable);
		}
		else if (std::string(luaL_typename(lua_state, -1)) == "string")
		{
			item.type = TYPE_STRING;
			item.name = std::string(lua_tostring(lua_state, -2));
			item.sValue = std::string(lua_tostring(lua_state, -1));
		}
		else if (std::string(luaL_typename(lua_state, -1)) == "number")
		{
			item.type = TYPE_INTEGER;
			item.iValue = lua_tointeger(lua_state, -1);
			item.name = std::string(lua_tostring(lua_state, -2));
		}
		else if (std::string(luaL_typename(lua_state, -1)) == "boolean")
		{
			item.type = TYPE_BOOLEAN;
			item.iValue = lua_toboolean(lua_state, -1);
			item.name = std::string(lua_tostring(lua_state, -2));
		}
		if (!item.isTable && item.type != TYPE_UNKNOWN)
			mLuaTable.insert(std::pair<int, _tLuaTableValues> (indexCount, item));
		lua_pop(lua_state, 1);
	}
}

void CdzVents::LoadEvents()
{
	m_bdzVentsExist = false;
	std::string dzv_Dir, s;
#ifdef WIN32
	dzv_Dir = szUserDataFolder + "scripts\\dzVents\\generated_scripts\\";
	m_scriptsDir = szUserDataFolder + "scripts\\dzVents\\scripts\\";
	m_runtimeDir = szUserDataFolder + "dzVents\\runtime\\";
#else
	dzv_Dir = szUserDataFolder + "scripts/dzVents/generated_scripts/";
	m_scriptsDir = szUserDataFolder + "scripts/dzVents/scripts/";
	m_runtimeDir = szUserDataFolder + "dzVents/runtime/";
#endif

	// Remove dzVents DB files from disk
	std::vector<std::string> FileEntries;
	std::vector<std::string>::const_iterator itt;
	std::string filename;
	DirectoryListing(FileEntries, dzv_Dir, false, true);
	for (itt = FileEntries.begin(); itt != FileEntries.end(); ++itt)
	{
		filename = dzv_Dir + *itt;
		if (filename.find("README.md") == std::string::npos)
			std::remove(filename.c_str());
	}

	if (m_sql.m_bDisableDzVentsSystem)
		return;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, Name, Interpreter, Type, Status, XMLStatement FROM EventMaster WHERE Interpreter <> 'Blockly' AND Status > 0 ORDER BY ID");
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			CEventSystem::_tEventItem eitem;
			std::stringstream s_str(sd[0]);
			s_str >> eitem.ID;
			eitem.Name = sd[1];
			eitem.Interpreter = sd[2];
			std::transform(sd[3].begin(), sd[3].end(), sd[3].begin(), ::tolower);
			eitem.Type = sd[3];
			eitem.EventStatus = atoi(sd[4].c_str());
			eitem.Actions = sd[5];
			eitem.SequenceNo = 0;
			m_mainworker.m_eventsystem.m_events.push_back(eitem);

			// Write active dzVents scripts to disk.
			if ((eitem.Interpreter == "dzVents") && (eitem.EventStatus != 0))
			{
				s = dzv_Dir + eitem.Name.c_str() + ".lua";
				_log.Log(LOG_STATUS, "dzVents: Write file: %s",s.c_str());
				FILE *fOut = fopen(s.c_str(), "wb+");
				if (fOut)
				{
					fwrite(eitem.Actions.c_str(), 1, eitem.Actions.size(), fOut);
					fclose(fOut);
				}
				m_bdzVentsExist = true;
			}
		}
	}
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
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "deviceIDs");
		lua_createtable(lua_state, 0, 0);
		std::vector<uint64_t>::const_iterator itt2;
		if (sgitem.memberID.size() > 0)
		{
			int index = 1;
			for (itt2 = sgitem.memberID.begin(); itt2 != sgitem.memberID.end(); itt2++)
			{
				lua_pushnumber(lua_state, (lua_Number)index);
				lua_pushnumber(lua_state, (lua_Number)*itt2);
				lua_rawset(lua_state, -3);
				index++;
			}
		}

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
