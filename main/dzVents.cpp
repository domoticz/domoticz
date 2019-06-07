#include "stdafx.h" 
#include "mainworker.h"
#include "SQLHelper.h"
#include "localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/WebServerHelper.h"
#include "dzVents.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "../webserver/Base64.h"

extern std::string szUserDataFolder, szWebRoot, szStartupFolder, szAppVersion;
extern http::server::CWebServerHelper m_webservers;

CdzVents CdzVents::m_dzvents;

CdzVents::CdzVents(void) :
	m_version("2.4.22")
{
	m_bdzVentsExist = false;
}

CdzVents::~CdzVents(void)
{
}

const std::string CdzVents::GetVersion()
{
	return m_version;
}

void CdzVents::EvaluateDzVents(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items, const int secStatus)
{
	// reroute print library to Domoticz logger
	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

	bool reasonTime = false;
	bool reasonURL = false;
	bool reasonSecurity = false;
	std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
	for (itt = items.begin(); itt != items.end(); ++itt)
	{
		if (itt->reason == m_mainworker.m_eventsystem.REASON_URL)
			reasonURL = true;

		if (itt->reason == m_mainworker.m_eventsystem.REASON_SECURITY)
			reasonSecurity = true;

		if (itt->reason == m_mainworker.m_eventsystem.REASON_TIME)
			reasonTime = true;
	}
	ExportDomoticzDataToLua(lua_state, items);
	SetGlobalVariables(lua_state, reasonTime, secStatus);

	if (reasonURL)
		ProcessHttpResponse(lua_state, items);

	if (reasonSecurity)
		ProcessSecurity(lua_state, items);
}

void CdzVents::ProcessSecurity(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items)
{
	int index = 1;
	int secstatus = 0;
	std::string secstatusw = "";
	lua_createtable(lua_state, 0, 0);
	std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
	for (itt = items.begin(); itt != items.end(); ++itt)
	{
		if (itt->reason == m_mainworker.m_eventsystem.REASON_SECURITY)
		{
			m_sql.GetPreferencesVar("SecStatus", secstatus);
			if (itt->nValue == 1)
				secstatusw = "Armed Home";
			else if (itt->nValue == 2)
				secstatusw = "Armed Away";
			else
				secstatusw = "Disarmed";

			lua_pushnumber(lua_state, (lua_Number)index);
			lua_pushstring(lua_state, secstatusw.c_str());
			lua_rawset(lua_state, -3);
			index++;
		}
	}
	lua_setglobal(lua_state, "securityupdates");
}

void CdzVents::ProcessHttpResponse(lua_State* lua_state, const std::vector<CEventSystem::_tEventQueue>& items)
{
	int index = 1;
	int statusCode = 0;
	
	std::string protocol;
	std::string statusText;
	
	
	lua_createtable(lua_state, 0, 0);
	std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
	for (itt = items.begin(); itt != items.end(); ++itt)
	{
		if (itt->reason == m_mainworker.m_eventsystem.REASON_URL)
		{
			lua_pushnumber(lua_state, (lua_Number)index);
			lua_createtable(lua_state, 0, 0);
			lua_pushstring(lua_state, "headers");
			lua_createtable(lua_state, (int)itt->vData.size() + 2, 0); // status is split into 3 parts
			if (!itt->vData.empty())
			{
				std::vector<std::string>::const_iterator itt2;
				for (itt2 = itt->vData.begin(); itt2 != itt->vData.end(); ++itt2)
				{
					std::string header = (*itt2);

					size_t pos = header.find(": ");
					if (pos != std::string::npos)
					{
						lua_pushstring(lua_state, header.substr(0, pos).c_str());
						lua_pushstring(lua_state, header.substr(pos + 2).c_str());
						lua_rawset(lua_state, -3);
					}
					else
					{
						if (header.find("HTTP/") == 0)
						{
							std::vector<std::string> results;
							StringSplit(header, " ", results);
							if (results.size() >= 2)
							{
								pos = header.find(results[0]);
								protocol = header.substr(0, pos + results[0].size());
								statusCode = atoi(results[1].c_str());
								if (results.size() >= 3)
								{
									statusText = header.substr(header.find(results[2]));
								}
								else
								{
									statusText = ((statusCode >= 200) && (statusCode <= 299)) ? "OK" : "No reason returned!";
								}
							}
						}
					}
				}
			}
			lua_rawset(lua_state, -3); //end of table

			lua_pushstring(lua_state, "protocol");
			lua_pushstring(lua_state, protocol.c_str());
			lua_rawset(lua_state, -3);
			lua_pushstring(lua_state, "statusText");
			lua_pushstring(lua_state, statusText.c_str());
			lua_rawset(lua_state, -3);
			lua_pushstring(lua_state, "statusCode");
			lua_pushnumber(lua_state, statusCode);
			lua_rawset(lua_state, -3);
			lua_pushstring(lua_state, "data");
			lua_pushstring(lua_state, itt->sValue.c_str());
			lua_rawset(lua_state, -3);
			lua_pushstring(lua_state, "callback");
			lua_pushstring(lua_state, itt->nValueWording.c_str());
			lua_rawset(lua_state, -3);
			lua_settable(lua_state, -3); // number entry
			index++;
		}
	}
	lua_setglobal(lua_state, "httpresponse");
}

bool CdzVents::OpenURL(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	float delayTime = 0;
	std::string URL, extraHeaders, method, postData, trigger;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->isTable && itt->sValue == "headers" && itt != vLuaTable.end() - 1)
		{
			int tIndex = itt->tIndex;
			itt++;
			std::vector<_tLuaTableValues>::const_iterator itt2;
			for (itt2 = itt; itt2 != vLuaTable.end(); ++itt2)
			{
				if (itt2->tIndex != tIndex)
				{
					itt--;
					break;
				}
				extraHeaders += "!#" + itt2->name + ": " + itt2->sValue;
				if (itt != vLuaTable.end() - 1)
					itt++;
			}
		}
		else if (itt->type == TYPE_STRING)
		{
			if (itt->name == "URL")
				URL = itt->sValue;
			else if (itt->name == "method")
				method = itt->sValue;
			else if (itt->name == "postdata")
				postData = itt->sValue;
			else if (itt->name == "_trigger")
				trigger = itt->sValue;
		}
		else if (itt->type == TYPE_INTEGER)
		{
			if (itt->name == "_random")
				delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
			else if (itt->name == "_after")
				delayTime = static_cast<float>(itt->iValue);
		}
	}
	
	if (URL.empty())
	{
		_log.Log(LOG_ERROR, "dzVents: No URL supplied!");
		return false;
	}
	
	// Handle situation where WebLocalNetworks is not open without password for dzVents
	if (URL.find("127.0.0.1") != std::string::npos)
	{
		std::string allowedNetworks;
		int rnvalue = 0;
		m_sql.GetPreferencesVar("WebLocalNetworks",rnvalue, allowedNetworks);
		if (allowedNetworks.find("127.0.0.") == std::string::npos)
		{
			_log.Log(LOG_ERROR, "dzVents: local netWork not open for dzVents openURL call !");
			_log.Log(LOG_ERROR, "dzVents: check dzVents wiki (look for 'Using dzVents with Domoticz')");
			return false;
		}
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

bool CdzVents::UpdateDevice(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	bool bEventTrigger = false;
	int nValue = -1, Protected = -1;
	uint64_t idx = -1;
	float delayTime = 0;
	std::string sValue;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER)
		{
			if (itt->name == "idx")
				idx = static_cast<uint64_t>(itt->iValue);
			else if (itt->name == "nValue")
				nValue = itt->iValue;
			else if (itt->name == "protected")
				Protected = itt->iValue;
			else if (itt->name == "_random")
				delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
			else if (itt->name == "_after")
				delayTime = static_cast<float>(itt->iValue);
		}
		else if (itt->type == TYPE_STRING && itt->name == "sValue")
			sValue = itt->sValue;
		else if (itt->type == TYPE_BOOLEAN && itt->name == "_trigger")
			bEventTrigger = true;
	}
	if (idx == -1)
		return false;

	m_sql.AddTaskItem(_tTaskItem::UpdateDevice(delayTime, idx, nValue, sValue, Protected, bEventTrigger), false);
	return true;
}

bool CdzVents::TriggerIFTTT(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	std::string sID, sValue1, sValue2, sValue3 ;
	float delayTime = 1;
	int rnvalue = 0;

	m_sql.GetPreferencesVar("IFTTTEnabled", rnvalue);
	if (rnvalue == 0)
	{
		_log.Log(LOG_ERROR, "dzVents: IFTTT not enabled" );
		return false;
	}

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER )
		{
			if (itt->name == "_random")
				delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));

			else if (itt->name == "_after")
				delayTime = static_cast<float>(itt->iValue);

			else if (itt->name == "sID")
				 sID = std::to_string(itt->iValue);

			else if (itt->name == "sValue1")
				sValue1 = std::to_string(itt->iValue);

			else if (itt->name == "sValue2")
				sValue2 = std::to_string(itt->iValue);

			else if (itt->name == "sValue3")
				sValue2 = std::to_string(itt->iValue);
		}
		else if (itt->type == TYPE_STRING)
		{
			if (itt->name == "sID")
				sID = itt->sValue;

			else if (itt->name == "sValue1")
				sValue1 = itt->sValue;

			else if (itt->name == "sValue2")
				sValue2 = itt->sValue;

			else if (itt->name == "sValue3")
				sValue3 = itt->sValue;
		}
	}

	m_sql.AddTaskItem(_tTaskItem::SendIFTTTTrigger(delayTime, sID, sValue1, sValue2, sValue3));
	return true;
}

bool CdzVents::UpdateVariable(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	std::string variableValue;
	float delayTime = 0;
	bool bEventTrigger = false;
	uint64_t idx;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER)
		{
			if (itt->name == "idx")
				idx = static_cast<uint64_t>(itt->iValue);
			else if (itt->name == "_random")
				delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
			else if (itt->name == "_after")
				delayTime = static_cast<float>(itt->iValue);
		}
		else if (itt->type == TYPE_STRING && itt->name == "value")
			variableValue = itt->sValue;

		else if (itt->type == TYPE_BOOLEAN && itt->name == "_trigger")
			bEventTrigger = true;
	}
	if (bEventTrigger)
		m_mainworker.m_eventsystem.SetEventTrigger(idx, m_mainworker.m_eventsystem.REASON_USERVARIABLE, delayTime);

	m_sql.AddTaskItem(_tTaskItem::SetVariable(delayTime, idx, variableValue, false));
	return true;
}

bool CdzVents::CancelItem(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	uint64_t idx;
	int count = 0;
	std::string type;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER && itt->name == "idx")
			idx = static_cast<uint64_t>(itt->iValue);

		else if (itt->type == TYPE_STRING && itt->name == "type")
			type = itt->sValue;
	}
	_tTaskItem tItem;
	tItem._idx = idx;
	tItem._DelayTime = 0;
	if (type == "device")
	{
		tItem._ItemType = TITEM_SWITCHCMD_EVENT;
		m_sql.AddTaskItem(tItem, true);
		tItem._ItemType = TITEM_UPDATEDEVICE;
	}
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
	std::vector<_tLuaTableValues> vLuaTable;
	IterateTable(lua_state, tIndex, vLuaTable);
	if (vLuaTable.size() > 0)
	{
		if (lCommand == "OpenURL")
			scriptTrue = OpenURL(lua_state, vLuaTable);

		else if (lCommand == "UpdateDevice")
			scriptTrue = UpdateDevice(lua_state, vLuaTable);

		else if (lCommand == "Variable")
			scriptTrue = UpdateVariable(lua_state, vLuaTable);

		else if (lCommand == "Cancel")
			scriptTrue = CancelItem(lua_state, vLuaTable);
		
		else if (lCommand == "TriggerIFTTT")
			scriptTrue = TriggerIFTTT(lua_state, vLuaTable);
	}
	return scriptTrue;
}

void CdzVents::IterateTable(lua_State *lua_state, const int tIndex, std::vector<_tLuaTableValues> &vLuaTable)
{
	_tLuaTableValues item;
	item.isTable = false;

	lua_pushnil(lua_state);
	while (lua_next(lua_state, tIndex) != 0)
	{
		item.type = TYPE_UNKNOWN;
		item.isTable = false;
		item.tIndex = tIndex;
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
				item.iValue = static_cast<int>(lua_tonumber(lua_state, -2));
			}
			else
			{
				lua_pop(lua_state, 1);
				continue;
			}
			item.isTable = true;
			item.tIndex += 2;
			vLuaTable.push_back(item);
			IterateTable(lua_state, item.tIndex, vLuaTable);
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
			vLuaTable.push_back(item);

		lua_pop(lua_state, 1);
	}
}

int CdzVents::l_domoticz_print(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);

	for (int i = 1; i <= nargs; i++)
	{
		if (lua_isstring(lua_state, i))
		{
			std::string lstring = lua_tostring(lua_state, i);
			if (lstring.find("Error: ") != std::string::npos)
			{
				_log.Log(LOG_ERROR, "dzVents: %s", lstring.c_str());
			}
			else
			{
				_log.Log(LOG_STATUS, "dzVents: %s", lstring.c_str());
			}
		}
		else
		{
			/* non strings? */
		}
	}
	return 0;
}

void CdzVents::SetGlobalVariables(lua_State *lua_state, const bool reasonTime, const int secStatus)
{
	std::stringstream lua_DirT, runtime_DirT;

	lua_DirT << szUserDataFolder <<
#ifdef WIN32
	"scripts\\dzVents\\";
#else
	"scripts/dzVents/";
#endif

	runtime_DirT << szStartupFolder <<
#ifdef WIN32
	"dzVents\\runtime\\";
#else
	"dzVents/runtime/";
#endif

	lua_createtable(lua_state, 0, 0);
	lua_pushstring(lua_state, "Security");
	lua_pushstring(lua_state, m_mainworker.m_eventsystem.m_szSecStatus[secStatus].c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "script_path");
	lua_pushstring(lua_state, lua_DirT.str().c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "runtime_path");
	lua_pushstring(lua_state, runtime_DirT.str().c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "isTimeEvent");
	lua_pushboolean(lua_state, reasonTime);
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

	std::string sTitle;
	m_sql.GetPreferencesVar("Title", sTitle);
	lua_pushstring(lua_state, "domoticz_title");
	lua_pushstring(lua_state, sTitle.c_str());
	lua_rawset(lua_state, -3);
	
	// Only when webLocalNetworks has local network defined
	// Add latitude / longitude to table
	std::string allowedNetworks;
	rnvalue = 0;
	m_sql.GetPreferencesVar("WebLocalNetworks",rnvalue, allowedNetworks);
	if (allowedNetworks.find("127.0.0.") != std::string::npos)
	{
		std::string location;
		std::vector<std::string> strarray;
		if (m_sql.GetPreferencesVar("Location", rnvalue, location))
			StringSplit(location, ";", strarray);
		if (strarray.size() == 2)
		{
			// Only when location entered in the settings
			// Add to table
			std::string Latitude = strarray[0];
			std::string Longitude = strarray[1];
			lua_pushstring(lua_state, "longitude");
			lua_pushstring(lua_state, Longitude.c_str());
			lua_rawset(lua_state, -3);
			lua_pushstring(lua_state, "latitude");
			lua_pushstring(lua_state, Latitude.c_str());
			lua_rawset(lua_state, -3);
		}
	}

	lua_pushstring(lua_state, "domoticz_listening_port");
	lua_pushstring(lua_state, m_webservers.our_listener_port.c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "domoticz_webroot");
	lua_pushstring(lua_state, szWebRoot.c_str());
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
	lua_pushstring(lua_state, "domoticz_version");
	lua_pushstring(lua_state, szAppVersion.c_str());
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "dzVents_version");
	lua_pushstring(lua_state,(GetVersion().c_str()));
	lua_rawset(lua_state, -3);
	lua_setglobal(lua_state, "globalvariables");
}

void CdzVents::ExportDomoticzDataToLua(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items)
{
	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_mainworker.m_eventsystem.m_devicestatesMutex);
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

		bool triggerDevice = false;
		std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
		for (itt = items.begin(); itt != items.end(); ++itt)
		{
			if (sitem.ID == itt->id && itt->reason == m_mainworker.m_eventsystem.REASON_DEVICE)
			{
				triggerDevice = true;
				sitem.lastUpdate = itt->lastUpdate;
				sitem.lastLevel = itt->lastLevel;
				sitem.sValue = itt->sValue;
				sitem.nValueWording = itt->nValueWording;
				sitem.nValue = itt->nValue;
				if (itt->JsonMapString.size() > 0)
					sitem.JsonMapString = itt->JsonMapString;
				if (itt->JsonMapFloat.size() > 0)
					sitem.JsonMapFloat = itt->JsonMapFloat;
				if (itt->JsonMapInt.size() > 0)
					sitem.JsonMapInt = itt->JsonMapInt;
				if (itt->JsonMapBool.size() > 0)
					sitem.JsonMapBool = itt->JsonMapBool;
			}
		}

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
		lua_pushboolean(lua_state, triggerDevice);
		lua_rawset(lua_state, -3);
		lua_pushstring(lua_state, "timedOut");
		lua_pushboolean(lua_state, timed_out);
		lua_rawset(lua_state, -3);

		//get all svalues separate
		std::vector<std::string> strarray;
		StringSplit(sitem.sValue, ";", strarray);

		lua_pushstring(lua_state, "rawData");
		lua_createtable(lua_state, 0, 0);

		for (uint8_t i = 0; i < strarray.size(); i++)
		{
			lua_pushnumber(lua_state, (lua_Number)i + 1);
			lua_pushstring(lua_state, strarray[i].c_str());
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
				if (strcmp(m_mainworker.m_eventsystem.JsonMap[itt->first].szOriginal, "LevelNames") == 0 ||
					strcmp(m_mainworker.m_eventsystem.JsonMap[itt->first].szOriginal, "LevelActions") == 0)
					lua_pushstring(lua_state, base64_decode(itt->second).c_str());
				else
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
	devicestatesMutexLock.unlock();

	// Now do the scenes and groups.
	const char *description = "";
	boost::shared_lock<boost::shared_mutex> scenesgroupsMutexLock(m_mainworker.m_eventsystem.m_scenesgroupsMutex);

	std::map<uint64_t, CEventSystem::_tScenesGroups>::const_iterator ittScenes;
	for (ittScenes = m_mainworker.m_eventsystem.m_scenesgroups.begin(); ittScenes != m_mainworker.m_eventsystem.m_scenesgroups.end(); ++ittScenes)
	{
		CEventSystem::_tScenesGroups sgitem = ittScenes->second;
		bool triggerScene = false;
		std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
		for (itt = items.begin(); itt != items.end(); ++itt)
		{
			if (sgitem.ID == itt->id && itt->reason == m_mainworker.m_eventsystem.REASON_SCENEGROUP)
			{
				triggerScene = true;
				sgitem.lastUpdate = itt->lastUpdate;
				sgitem.scenesgroupValue = itt->sValue;
			}
		}

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT Description FROM Scenes WHERE (ID=='%d')", sgitem.ID);
		if (result.empty())
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
		lua_pushstring(lua_state, (sgitem.scenesgroupType == 0) ? "scene" : "group");
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "lastUpdate");
		lua_pushstring(lua_state, sgitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);

		lua_pushstring(lua_state, "changed");
		lua_pushboolean(lua_state, triggerScene);
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
			for (itt2 = sgitem.memberID.begin(); itt2 != sgitem.memberID.end(); ++itt2)
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
		bool triggerVar = false;
		std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
		for (itt = items.begin(); itt != items.end(); ++itt)
		{
			if (uvitem.ID == itt->id && itt->reason == m_mainworker.m_eventsystem.REASON_USERVARIABLE)
			{
				triggerVar = true;
				uvitem.lastUpdate = itt->lastUpdate;
				uvitem.variableValue = itt->sValue;
			}
		}

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
		lua_pushboolean(lua_state, triggerVar);
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
				vtype = "string";
			else if (uvitem.variableType == 3)
				vtype = "date";
			else if (uvitem.variableType == 4)
				vtype = "time";
			else
				vtype = "unknown";
		}
		lua_rawset(lua_state, -3);

		lua_settable(lua_state, -3); // data table

		lua_pushstring(lua_state, "variableType");
		lua_pushstring(lua_state, vtype.c_str());
		lua_rawset(lua_state, -3);

		lua_settable(lua_state, -3); // end entry

		index++;
	}

	// Now do the cameras.
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, Name FROM Cameras where enabled = '1' ORDER BY ID ASC");
	if (!result.empty())
	{
		for (const auto & itt : result)
		{
			std::vector<std::string> sd = itt;
			lua_pushnumber(lua_state, (lua_Number)index);
			lua_createtable(lua_state, 1, 3);

			lua_pushstring(lua_state, "name");
			lua_pushstring(lua_state, sd[1].c_str());
			lua_rawset(lua_state, -3);

			lua_pushstring(lua_state, "id");
			lua_pushnumber(lua_state, atoi(sd[0].c_str()));
			lua_rawset(lua_state, -3);

			lua_pushstring(lua_state, "baseType");
			lua_pushstring(lua_state, "camera");
			lua_rawset(lua_state, -3);

			lua_settable(lua_state, -3); // end entry

			index++;
		}
	}

	lua_setglobal(lua_state, "domoticzData");
}
