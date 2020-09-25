#include "stdafx.h"
#include "mainworker.h"
#include "SQLHelper.h"
#include "localtime_r.h"
#include "../hardware/hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXNames.h"
#include "../main/WebServerHelper.h"
#include "../main/LuaTable.h"
#include "../main/json_helper.h"
#include "dzVents.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "../webserver/Base64.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

extern std::string szUserDataFolder, szWebRoot, szStartupFolder, szAppVersion;
extern http::server::CWebServerHelper m_webservers;

CdzVents CdzVents::m_dzvents;

CdzVents::CdzVents(void) :
	m_version("3.0.14")
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
	bool reasonNotification = false;
	std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
	for (itt = items.begin(); itt != items.end(); ++itt)
	{
		if (itt->reason == m_mainworker.m_eventsystem.REASON_URL)
			reasonURL = true;
		else if (itt->reason == m_mainworker.m_eventsystem.REASON_SECURITY)
			reasonSecurity = true;
		else if (itt->reason == m_mainworker.m_eventsystem.REASON_TIME)
			reasonTime = true;
		else if (itt->reason == m_mainworker.m_eventsystem.REASON_NOTIFICATION)
			reasonNotification = true;
	}

	ExportDomoticzDataToLua(lua_state, items);
	SetGlobalVariables(lua_state, reasonTime, secStatus);

	if (reasonURL)
		ProcessHttpResponse(lua_state, items);
	if (reasonSecurity)
		ProcessSecurity(lua_state, items);
	if (reasonNotification)
		ProcessNotification(lua_state, items);
}

void CdzVents::ProcessNotificationItem(CLuaTable &luaTable, int &index, const CEventSystem::_tEventQueue& item)
{
	std::string type, status;

	luaTable.OpenSubTableEntry(index, 1, 4);
	type = m_mainworker.m_notificationsystem.GetTypeString(item.nValue);
	status = m_mainworker.m_notificationsystem.GetStatusString(item.lastLevel);

	if (item.sValue.empty())
		luaTable.AddString("message", "");
	else
	{
		luaTable.AddString("message", "");
		luaTable.OpenSubTableEntry("data", 0, 0);
		if (item.nValue >= Notification::HW_TIMEOUT && item.nValue <= Notification::HW_THREAD_ENDED)
		{
			Json::Value eventdata;
			if (ParseJSon(item.sValue, eventdata))
			{
				luaTable.AddInteger("id", eventdata["m_HwdID"].asInt());
				luaTable.AddString("name", eventdata["m_Name"].asString());
			}
		}
		else if (item.nValue == Notification::DZ_BACKUP_DONE)
		{
			Json::Value eventdata;
			if(ParseJSon(item.sValue, eventdata))
			{
				type = type + eventdata["type"].asString();
				luaTable.AddNumber("duration", eventdata["duration"].asFloat());
				luaTable.AddString("location", eventdata["location"].asString());
			}
		}
		else if (item.nValue == Notification::DZ_CUSTOM)
		{
			Json::Value eventdata;
			if (ParseJSon(item.sValue, eventdata))
			{
				luaTable.AddString("name", eventdata["name"].asString());
				luaTable.AddString("data", eventdata["data"].asString());
			}
		}
		luaTable.CloseSubTableEntry();
	}
	luaTable.AddString("type", type);
	luaTable.AddString("status", status);
	luaTable.CloseSubTableEntry();
	index++;
}

void CdzVents::ProcessNotification(lua_State* lua_state, const std::vector<CEventSystem::_tEventQueue>& items)
{
	int index = 1;
	bool bHardware = false;
	bool bCustomEvent = false;
	CLuaTable luaTable(lua_state, "notification");

	luaTable.OpenSubTableEntry("domoticz", 0, 0);
	std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
	for (itt = items.begin(); itt != items.end(); itt++)
	{
		if (itt->reason == m_mainworker.m_eventsystem.REASON_NOTIFICATION)
		{
			switch (itt->nValue)
			{
			case Notification::DZ_START:
			case Notification::DZ_STOP:
			case Notification::DZ_BACKUP_DONE:
			case Notification::DZ_NOTIFICATION:
				ProcessNotificationItem(luaTable, index, *itt);
				break;
			case Notification::DZ_CUSTOM:
				bCustomEvent = true;
				break;
			default:
				bHardware = true;
				break;
			}
		}
	}

	luaTable.CloseSubTableEntry();

	luaTable.OpenSubTableEntry("hardware", 0, 0);
	if (bHardware)
	{
		for (itt = items.begin(); itt != items.end(); itt++)
		{
			switch (itt->nValue)
			{
			case Notification::HW_START:
			case Notification::HW_STOP:
			case Notification::HW_THREAD_ENDED:
			case Notification::HW_TIMEOUT:
				_log.Log(LOG_ERROR, "dzVents notification type: %s not yet supported", m_mainworker.m_notificationsystem.GetTypeString(itt->nValue).c_str());
				//ProcessNotificationItem(luaTable, index, *itt);
				break;
			default:
				break;
			}
		}
	}

	luaTable.CloseSubTableEntry();

	luaTable.OpenSubTableEntry("customevent", 0, 0);
	if (bCustomEvent)
	{
		for (itt = items.begin(); itt != items.end(); itt++)
		{
			switch (itt->nValue)
			{
			case Notification::DZ_CUSTOM:
				ProcessNotificationItem(luaTable, index, *itt);
				break;
			default:
				break;
			}
		}
	}

	luaTable.CloseSubTableEntry();
	luaTable.Publish();
}

void CdzVents::ProcessSecurity(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items)
{
	int index = 1;
	int secstatus = 0;
	std::string secstatusw = "";

	CLuaTable luaTable(lua_state, "securityupdates");

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

			luaTable.AddString(index, secstatusw);
			index++;
		}
	}

	luaTable.Publish();
}

void CdzVents::ProcessHttpResponse(lua_State* lua_state, const std::vector<CEventSystem::_tEventQueue>& items)
{
	int index = 1;
	int statusCode = 0;

	std::string protocol;
	std::string statusText;

	CLuaTable luaTable(lua_state, "httpresponse");

	std::vector<CEventSystem::_tEventQueue>::const_iterator itt;
	for (itt = items.begin(); itt != items.end(); ++itt)
	{
		if (itt->reason == m_mainworker.m_eventsystem.REASON_URL)
		{
			luaTable.OpenSubTableEntry(index, 0, 0);
			luaTable.OpenSubTableEntry("headers", (int)itt->vData.size() + 2, 0); // status is split into 3 parts
			if (!itt->vData.empty())
			{
				std::vector<std::string>::const_iterator itt2;
				for (itt2 = itt->vData.begin(); itt2 != itt->vData.end(); ++itt2)
				{
					std::string header = (*itt2);
					size_t pos = header.find(": ");
					if (pos != std::string::npos)
						luaTable.AddString(header.substr(0, pos), header.substr(pos + 2));
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
			luaTable.CloseSubTableEntry();

			luaTable.AddString("protocol", protocol);
			luaTable.AddString("statusText", statusText);
			luaTable.AddInteger("statusCode", statusCode);
			luaTable.AddString("data", itt->sValue);
			luaTable.AddString("callback", itt->nValueWording);
			luaTable.CloseSubTableEntry(); // index entry
			index++;
		}
	}

	luaTable.Publish();
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
		else if (( itt->type == TYPE_INTEGER) && (itt->name == "_random"))
			delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
		else if (( itt->type == TYPE_FLOAT) && (itt->name == "_after"))
			delayTime = itt->fValue;
	}

	if (URL.empty())
	{
		_log.Log(LOG_ERROR, "dzVents: No URL supplied!");
		return false;
	}

	// Handle situation where WebLocalNetworks is not open without password for dzVents
	if ( URL.find("127.0.0") != std::string::npos || URL.find("::") != std::string::npos || URL.find("localhost") != std::string::npos )
	{
		std::string allowedNetworks;
		int rnvalue = 0;
		m_sql.GetPreferencesVar("WebLocalNetworks",rnvalue, allowedNetworks);
		if ( ( allowedNetworks.find("127.0.0.") == std::string::npos ) && ( allowedNetworks.find("::") == std::string::npos ) )
		{
			_log.Log(LOG_ERROR, "dzVents: local netWork not open for dzVents openURL call !");
			_log.Log(LOG_ERROR, "dzVents: check dzVents wiki (look for 'Using dzVents with Domoticz')");
			return false;
		}
	}

	HTTPClient::_eHTTPmethod eMethod = HTTPClient::HTTP_METHOD_GET; // defaults to GET
	if (!method.empty())
	{
		if (method == "GET")
			eMethod = HTTPClient::HTTP_METHOD_GET;
		else if (method == "POST")
			eMethod = HTTPClient::HTTP_METHOD_POST;
		else if (method == "PUT")
			eMethod = HTTPClient::HTTP_METHOD_PUT;
		else if (method == "DELETE")
			eMethod = HTTPClient::HTTP_METHOD_DELETE;
		else
		{
			_log.Log(LOG_ERROR, "dzVents: Invalid HTTP method '%s'", method.c_str());
			return false;
		}
	}

	if (!postData.empty() && eMethod == HTTPClient::HTTP_METHOD_GET)
	{
		_log.Log(LOG_ERROR, "dzVents: You cannot use postdata with method GET.");
		return false;
	}

	m_sql.AddTaskItem(_tTaskItem::GetHTTPPage(delayTime, URL, extraHeaders, eMethod, postData, trigger));
	return true;
}

bool CdzVents::TriggerCustomEvent(lua_State* lua_state, const std::vector<_tLuaTableValues>& vLuaTable)
{
	float delayTime = 0;
	std::string name;
	std::string sValue;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if ((itt->type == TYPE_STRING) && (itt->name == "name"))
			name = itt->sValue;
		else if ((itt->type == TYPE_STRING) && (itt->name == "data"))
			sValue = itt->sValue;
		else if (( itt->type == TYPE_INTEGER) && (itt->name == "_random"))
			delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
		else if (( itt->type == TYPE_FLOAT) && (itt->name == "_after"))
			delayTime = itt->fValue;
	}

	if (name.empty())
		return false;

	m_sql.AddTaskItem(_tTaskItem::CustomEvent(delayTime, name, sValue));

	return true;
}
bool CdzVents::UpdateDevice(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	bool bEventTrigger = false;
	int nValue = -1, Protected = -1;
	int idx = -1;
	float delayTime = 0;
	std::string sValue;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER)
		{
			if (itt->name == "idx")
				idx = itt->iValue;
			else if (itt->name == "nValue")
				nValue = itt->iValue;
			else if (itt->name == "protected")
				Protected = itt->iValue;
			else if (itt->name == "_random")
				delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
		}

		else if (( itt->type == TYPE_FLOAT) && (itt->name == "_after"))
			delayTime = itt->fValue;
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
			else if (itt->name == "sID")
				 sID = std::to_string(itt->iValue);
			else if (itt->name == "sValue1")
				sValue1 = std::to_string(itt->iValue);
			else if (itt->name == "sValue2")
				sValue2 = std::to_string(itt->iValue);
			else if (itt->name == "sValue3")
				sValue2 = std::to_string(itt->iValue);
		}

		else if (( itt->type == TYPE_FLOAT) && (itt->name == "_after"))
			delayTime = itt->fValue;
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
	int idx = 0;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER)
		{
			if (itt->name == "idx")
				idx = itt->iValue;
			else if (itt->name == "_random")
				delayTime = static_cast<float>(GenerateRandomNumber(itt->iValue));
		}

		else if (( itt->type == TYPE_FLOAT) && (itt->name == "_after"))
			delayTime = itt->fValue;
		else if (itt->type == TYPE_STRING && itt->name == "value")
			variableValue = itt->sValue;
		else if (itt->type == TYPE_BOOLEAN && itt->name == "_trigger")
			bEventTrigger = true;
	}

	if (idx == 0)
		return false;
	if (bEventTrigger)
		m_mainworker.m_eventsystem.SetEventTrigger(idx, m_mainworker.m_eventsystem.REASON_USERVARIABLE, delayTime);
	m_sql.AddTaskItem(_tTaskItem::SetVariable(delayTime, idx, variableValue, false));
	return true;
}

bool CdzVents::CancelItem(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable)
{
	int idx = 0;
	std::string type;

	std::vector<_tLuaTableValues>::const_iterator itt;
	for (itt = vLuaTable.begin(); itt != vLuaTable.end(); ++itt)
	{
		if (itt->type == TYPE_INTEGER && itt->name == "idx")
			idx = itt->iValue;
		else if (itt->type == TYPE_STRING && itt->name == "type")
			type = itt->sValue;
	}

	if (idx == 0)
		return false;

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
		else if (lCommand == "CustomEvent")
			scriptTrue = TriggerCustomEvent(lua_state, vLuaTable);
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
			item.name = std::string(lua_tostring(lua_state, -2));
			if (item.name == "_after")
			{
				item.type = TYPE_FLOAT;
				item.fValue = static_cast<float>(lua_tonumber(lua_state, -1));
			}
			else
			{
				item.type = TYPE_INTEGER;
				item.iValue = static_cast<int>(lua_tonumber(lua_state, -1));
			}
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

	CLuaTable luaTable(lua_state, "globalvariables");
	luaTable.AddString("Security", m_mainworker.m_eventsystem.m_szSecStatus[secStatus]);
	luaTable.AddString("script_path", lua_DirT.str());
	luaTable.AddString("runtime_path", runtime_DirT.str());
	luaTable.AddBool("isTimeEvent", reasonTime);

	char szTmp[10];
	sprintf(szTmp, "%.02f", 1.23f);
	luaTable.AddString("radix_separator", std::string(1,szTmp[1]));

	sprintf(szTmp, "%.02f", 1234.56f);
	if (szTmp[1] == '2')
		luaTable.AddString("group_separator", "");
	else
		luaTable.AddString("group_separator", std::string(1,szTmp[1]));

	int rnvalue = 0;
	m_sql.GetPreferencesVar("DzVentsLogLevel", rnvalue);
	luaTable.AddInteger("dzVents_log_level", rnvalue);

	std::string sTitle;
	m_sql.GetPreferencesVar("Title", sTitle);
	luaTable.AddString("domoticz_title", sTitle);

	// Only when location entered in the settings
	// Add to table
	bool locationSet = false;
	std::string location;
	std::vector<std::string> strarray;
	if (m_sql.GetPreferencesVar("Location", rnvalue, location))
	{
		StringSplit(location, ";", strarray);
		if (strarray.size() == 2)
		{
			locationSet = true;
			// Only when webLocalNetworks has local network defined
			// Add latitude / longitude to table
			std::string allowedNetworks;
			rnvalue = 0;
			m_sql.GetPreferencesVar("WebLocalNetworks",rnvalue, allowedNetworks);
			if ( ( allowedNetworks.find("127.0.0.") != std::string::npos) || (allowedNetworks.find("::") != std::string::npos) )
			{
				luaTable.AddString("latitude", strarray[0]);
				luaTable.AddString("longitude", strarray[1]);
			}
		}
	}

	luaTable.AddBool("locationSet", locationSet);
	luaTable.AddString("domoticz_listening_port", m_webservers.our_listener_port);
	luaTable.AddString("domoticz_webroot", szWebRoot);
	luaTable.AddString("domoticz_start_time", m_mainworker.m_eventsystem.m_szStartTime);
	luaTable.AddString("currentTime", TimeToString(NULL, TF_DateTimeMs));
	luaTable.AddInteger("systemUptime", SystemUptime());
	luaTable.AddString("domoticz_version", szAppVersion);
	luaTable.AddString("dzVents_version", GetVersion());

	luaTable.Publish();
}

void CdzVents::ExportHardwareData(CLuaTable &luaTable, int& index, const std::vector<CEventSystem::_tEventQueue>& items)
{
	;// to be implemented when hardware notification support is added
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

	CLuaTable luaTable(lua_state, "domoticzData");

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

		ParseSQLdatetime(checktime, ntime, sitem.lastUpdate, tm1.tm_isdst);
		bool timed_out = (now - checktime >= SensorTimeOut * 60);

		luaTable.OpenSubTableEntry(index, 1, 12);

		luaTable.AddString("name", sitem.deviceName);
		luaTable.AddBool("protected", (sitem.protection == 1) );
		luaTable.AddInteger("id", sitem.ID);
		luaTable.AddString("baseType","device");
		luaTable.AddString("deviceType", dev_type);
		luaTable.AddString("subType", sub_type);
		luaTable.AddString("switchType", Switch_Type_Desc((_eSwitchType)sitem.switchtype));
		luaTable.AddInteger("switchTypeValue", sitem.switchtype);
		luaTable.AddString("lastUpdate", sitem.lastUpdate);
		luaTable.AddInteger("lastLevel", sitem.lastLevel);
		luaTable.AddBool("changed", triggerDevice);
		luaTable.AddBool("timedOut", timed_out);

		//get all svalues separate
		std::vector<std::string> strarray;
		StringSplit(sitem.sValue, ";", strarray);

		luaTable.OpenSubTableEntry("rawData", 0, 0);
		for (uint8_t i = 0; i < strarray.size(); i++)
			luaTable.AddString(i + 1, strarray[i]);

		luaTable.CloseSubTableEntry();

		luaTable.AddString("deviceID", sitem.deviceID);
		luaTable.AddString("description", sitem.description);
		luaTable.AddInteger("batteryLevel", sitem.batteryLevel);
		luaTable.AddInteger("signalLevel", sitem.signalLevel);

		luaTable.OpenSubTableEntry("data", 0, 0);
		luaTable.AddString("_state", sitem.nValueWording);
		luaTable.AddInteger("_nValue", sitem.nValue);
		luaTable.AddInteger("hardwareID", sitem.hardwareID);

		// Lux does not have it's own field yet.
		if (sitem.devType == pTypeLux && sitem.subType == sTypeLux)
		{
			int lux = 0;
			if (strarray.size() > 0)
				lux = atoi(strarray[0].c_str());
			luaTable.AddNumber("lux", lux);
		}

		if (sitem.devType == pTypeGeneral && sitem.subType == sTypeKwh)
		{
			long double value = 0.0f;
			if (strarray.size() > 1)
				value = atof(strarray[1].c_str());
			luaTable.AddNumber("whTotal", value);
			value = 0.0f;
			if (strarray.size() > 0)
				value = atof(strarray[0].c_str());
			luaTable.AddNumber("whActual", value);
		}

		// Now see if we have additional fields from the JSON data
		if (sitem.JsonMapString.size() > 0)
		{
			std::map<uint8_t, std::string>::const_iterator itt;
			for (itt = sitem.JsonMapString.begin(); itt != sitem.JsonMapString.end(); ++itt)
			{
				if (strcmp(m_mainworker.m_eventsystem.JsonMap[itt->first].szOriginal, "LevelNames") == 0 ||
					strcmp(m_mainworker.m_eventsystem.JsonMap[itt->first].szOriginal, "LevelActions") == 0)
					luaTable.AddString(
						m_mainworker.m_eventsystem.JsonMap[itt->first].szNew,
						base64_decode(itt->second));
				else
					luaTable.AddString(
						m_mainworker.m_eventsystem.JsonMap[itt->first].szNew,
						itt->second);
			}
		}

		if (sitem.JsonMapFloat.size() > 0)
		{
			std::map<uint8_t, float>::const_iterator itt;
			for (itt = sitem.JsonMapFloat.begin(); itt != sitem.JsonMapFloat.end(); ++itt)
				luaTable.AddNumber(m_mainworker.m_eventsystem.JsonMap[itt->first].szNew, itt->second);
		}

		if (sitem.JsonMapInt.size() > 0)
		{
			std::map<uint8_t, int>::const_iterator itt;
			for (itt = sitem.JsonMapInt.begin(); itt != sitem.JsonMapInt.end(); ++itt)
				luaTable.AddInteger(m_mainworker.m_eventsystem.JsonMap[itt->first].szNew, itt->second);
		}

		if (sitem.JsonMapBool.size() > 0)
		{
			std::map<uint8_t, bool>::const_iterator itt;
			for (itt = sitem.JsonMapBool.begin(); itt != sitem.JsonMapBool.end(); ++itt)
				luaTable.AddBool(m_mainworker.m_eventsystem.JsonMap[itt->first].szNew, itt->second);
		}

		luaTable.CloseSubTableEntry();
		luaTable.CloseSubTableEntry();
		index++;
	}

	devicestatesMutexLock.unlock();

	// Now do the scenes and groups.
	const char *description = "";
	boost::shared_lock<boost::shared_mutex> scenesgroupsMutexLock(m_mainworker.m_eventsystem.m_scenesgroupsMutex);

	std::map<uint64_t, CEventSystem::_tScenesGroups>::const_iterator ittScenes;
	std::vector<std::vector<std::string> > result;

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

		result = m_sql.safe_query("SELECT Description FROM Scenes WHERE (ID=='%d')", sgitem.ID);
		if (result.empty())
			description = "";
		else
			description = result[0][0].c_str();

		luaTable.OpenSubTableEntry(index, 1, 7);

		luaTable.AddString("name", sgitem.scenesgroupName);
		luaTable.AddInteger("id", sgitem.ID);
		luaTable.AddString("description", description);
		luaTable.AddString("baseType", (sgitem.scenesgroupType == 0) ? "scene" : "group");
		luaTable.AddBool("protected", (lua_Number)sgitem.protection == 1);
		luaTable.AddString("lastUpdate", sgitem.lastUpdate);
		luaTable.AddBool("changed", triggerScene);

		luaTable.OpenSubTableEntry("data", 0, 0);

		luaTable.AddString("_state", sgitem.scenesgroupValue);

		luaTable.CloseSubTableEntry();

		luaTable.OpenSubTableEntry("deviceIDs", 0, 0);
		std::vector<uint64_t>::const_iterator itt2;
		if (sgitem.memberID.size() > 0)
		{
			int index = 1;
			for (itt2 = sgitem.memberID.begin(); itt2 != sgitem.memberID.end(); ++itt2)
			{
				luaTable.AddInteger(index, *itt2);
				index++;
			}
		}

		luaTable.CloseSubTableEntry(); // device table
		luaTable.CloseSubTableEntry(); // end entry
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

		luaTable.OpenSubTableEntry(index, 1, 5);

		luaTable.AddString("name", uvitem.variableName);
		luaTable.AddInteger("id", uvitem.ID);
		luaTable.AddString("baseType", "uservariable");
		luaTable.AddString("lastUpdate", uvitem.lastUpdate);
		luaTable.AddBool("changed", triggerVar);

		luaTable.OpenSubTableEntry("data", 0, 0);

		if (uvitem.variableType == 0)
		{
			luaTable.AddInteger("value", atoi(uvitem.variableValue.c_str()));
			vtype = "integer";
		}
		else if (uvitem.variableType == 1)
		{
			//Float
			luaTable.AddNumber("value", atof(uvitem.variableValue.c_str()));
			vtype = "float";
		}
		else
		{
			//String,Date,Time
			luaTable.AddString("value", uvitem.variableValue);
			if (uvitem.variableType == 2)
				vtype = "string";
			else if (uvitem.variableType == 3)
				vtype = "date";
			else if (uvitem.variableType == 4)
				vtype = "time";
			else
				vtype = "unknown";
		}

		luaTable.CloseSubTableEntry(); // data table

		luaTable.AddString("variableType", vtype);

		luaTable.CloseSubTableEntry(); // end entry

		index++;
	}

	// Now do the cameras.
	result = m_sql.safe_query("SELECT ID, Name FROM Cameras where enabled = '1' ORDER BY ID ASC");
	if (!result.empty())
	{
		for (const auto & itt : result)
		{
			std::vector<std::string> sd = itt;

			luaTable.OpenSubTableEntry(index, 1, 3);
			luaTable.AddString("name", sd[1]);
			luaTable.AddInteger("id", atoi(sd[0].c_str()));
			luaTable.AddString("baseType", "camera");
			luaTable.CloseSubTableEntry(); // end entry

			index++;
		}
	}

	// Now do the Hardware.
	result = m_sql.safe_query("SELECT ID, Name, type FROM Hardware where enabled = '1' ORDER BY ID ASC");
	int HardwareTypeVal;

	if (!result.empty())
	{
		for (const auto & itt : result)
		{
			std::vector<std::string> sd = itt;

			HardwareTypeVal = atoi(sd[2].c_str());

			luaTable.OpenSubTableEntry(index, 1, 6);
			luaTable.AddString("name", sd[1]);
			luaTable.AddInteger("id", atoi(sd[0].c_str()));
			luaTable.AddInteger("typeValue", HardwareTypeVal);
			luaTable.AddString("baseType", "hardware");
			if (HardwareTypeVal != HTYPE_PythonPlugin)
			{
				luaTable.AddString("typeName",Hardware_Type_Desc(HardwareTypeVal));
				luaTable.AddBool("isPythonPlugin", false);
			}
			else
			{
				luaTable.AddString("typeName", "Python plugin");
				luaTable.AddBool("isPythonPlugin", true);
			}
			luaTable.CloseSubTableEntry(); // end entry

			index++;
		}
	}

	ExportHardwareData(luaTable, index, items);

	luaTable.Publish();
}
