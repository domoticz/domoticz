#include "stdafx.h"
#include "Helper.h"
#include "Logger.h"
#include "RFXtrx.h"
#include "LuaHandler.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "../tinyxpath/xpath_processor.h"

#include <json/json.h>
#include "SQLHelper.h"
#include "mainworker.h"
#include "../hardware/hardwaretypes.h"
#include <boost/thread.hpp>

extern std::string szUserDataFolder;

int CLuaHandler::l_domoticz_updateDevice(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs >= 3 && nargs <= 5)
	{
		// Supported format ares :
		// - deviceId (integer), svalue (string), nvalue (string), [rssi(integer)], [battery(integer)]
		// - deviceId (integer), svalue (string), nvalue (integer), [rssi(integer)], [battery(integer)]
		if (lua_isnumber(lua_state, 1) && (lua_isstring(lua_state, 2) || lua_isnumber(lua_state, 2)) && lua_isstring(lua_state, 3))
		{
			// Extract the parameters from the lua 'updateDevice' function
			int ideviceId = (int)lua_tointeger(lua_state, 1);
			std::string nvalue = lua_tostring(lua_state, 2);
			std::string svalue = lua_tostring(lua_state, 3);
			if (((lua_isstring(lua_state, 3) && nvalue.empty()) && svalue.empty()))
			{
				_log.Log(LOG_ERROR, "CLuaHandler (updateDevice from LUA) : nvalue and svalue are empty ");
				return 0;
			}

			// Parse
			int invalue = (!nvalue.empty()) ? atoi(nvalue.c_str()) : 0;
			int signallevel = 12;
			if (nargs >= 4 && lua_isnumber(lua_state, 4))
			{
				signallevel = (int)lua_tointeger(lua_state, 4);
			}
			int batterylevel = 255;
			if (nargs == 5 && lua_isnumber(lua_state, 5))
			{
				batterylevel = (int)lua_tointeger(lua_state, 5);
			}
			_log.Log(LOG_NORM, "CLuaHandler (updateDevice from LUA) : idx=%d nvalue=%s svalue=%s invalue=%d signallevel=%d batterylevel=%d", ideviceId, nvalue.c_str(), svalue.c_str(), invalue, signallevel, batterylevel);

			m_mainworker.UpdateDevice(ideviceId, invalue, svalue, "EventSystem", signallevel, batterylevel);
		}
		else
		{
			_log.Log(LOG_ERROR, "CLuaHandler (updateDevice from LUA) : Incorrect parameters type");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "CLuaHandler (updateDevice from LUA) : Not enough parameters");
	}
	return 0;
}

int CLuaHandler::l_domoticz_print(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);

	for (int i = 1; i <= nargs; i++)
	{
		if (lua_isstring(lua_state, i))
		{
			//std::string lstring=lua_tostring(lua_state, i);
			_log.Log(LOG_NORM, "CLuaHandler: udevices: %s", lua_tostring(lua_state, i));
		}
		else
		{
			/* non strings? */
		}
	}
	return 0;
}

CLuaHandler::CLuaHandler(int hwdID)
{
	m_HwdID = hwdID;
}

void CLuaHandler::luaThread(lua_State *lua_state, const std::string &filename)
{
	int status;

	status = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
	report_errors(lua_state, status);
	lua_close(lua_state);
}

void CLuaHandler::luaStop(lua_State *L, lua_Debug *ar)
{
	if (ar->event == LUA_HOOKCOUNT)
	{
		(void)ar;  /* unused arg. */
		lua_sethook(L, nullptr, 0, 0);
		luaL_error(L, "LuaHandler: Lua script execution exceeds maximum number of lines");
		lua_close(L);
	}
}

void CLuaHandler::report_errors(lua_State *L, int status)
{
	if (status != 0) {
		_log.Log(LOG_ERROR, "CLuaHandler: %s", lua_tostring(L, -1));
		lua_pop(L, 1); // remove error message
	}
}

bool CLuaHandler::executeLuaScript(const std::string &script, const std::string &content)
{
	std::vector<std::string> allParameters;
	return executeLuaScript(script, content, allParameters);
}

bool CLuaHandler::executeLuaScript(const std::string &script, const std::string &content, std::vector<std::string>& allParameters)
{
	std::stringstream lua_DirT;
#ifdef WIN32
	lua_DirT << szUserDataFolder << "scripts\\lua_parsers\\";
#else
	lua_DirT << szUserDataFolder << "scripts/lua_parsers/";
#endif
	std::string lua_Dir = lua_DirT.str();

	lua_State *lua_state;
	lua_state = luaL_newstate();

	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

	lua_pushcfunction(lua_state, l_domoticz_updateDevice);
	lua_setglobal(lua_state, "domoticz_updateDevice");

	lua_pushcfunction(lua_state, l_domoticz_applyJsonPath);
	lua_setglobal(lua_state, "domoticz_applyJsonPath");

	lua_pushcfunction(lua_state, l_domoticz_applyXPath);
	lua_setglobal(lua_state, "domoticz_applyXPath");

	lua_pushinteger(lua_state, m_HwdID);
	lua_setglobal(lua_state, "hwdId");

	lua_createtable(lua_state, 1, 0);
	lua_pushstring(lua_state, "content");
	lua_pushstring(lua_state, content.c_str());
	lua_rawset(lua_state, -3);
	lua_setglobal(lua_state, "request");

	CEventSystem::_tEventQueue item;
	item.id = 0;
	m_mainworker.m_eventsystem.ExportDeviceStatesToLua(lua_state, item);

	// Push all url parameters as a map indexed by the parameter name
	// Each entry will be uri[<param name>] = <param value>
	int totParameters = (int)allParameters.size();
	lua_createtable(lua_state, totParameters, 0);
	for (int i = 0; i < totParameters; i++)
	{
		std::vector<std::string> parameterCouple;
		StringSplit(allParameters[i], "=", parameterCouple);
		if (parameterCouple.size() == 2) {
			// Add an url parameter after 'url' decoding it
			lua_pushstring(lua_state, CURLEncode::URLDecode(parameterCouple[0]).c_str());
			lua_pushstring(lua_state, CURLEncode::URLDecode(parameterCouple[1]).c_str());
			lua_rawset(lua_state, -3);
		}
	}
	lua_setglobal(lua_state, "uri");

	std::string fullfilename = lua_Dir + script;
	int status = luaL_loadfile(lua_state, fullfilename.c_str());
	if (status == 0)
	{
		lua_sethook(lua_state, luaStop, LUA_MASKCOUNT, 10000000);
		boost::thread aluaThread([this, lua_state, fullfilename] { luaThread(lua_state, fullfilename); });
		SetThreadName(aluaThread.native_handle(), "aluaThread");
		aluaThread.timed_join(boost::posix_time::seconds(10));
		return true;
	}
	report_errors(lua_state, status);
	lua_close(lua_state);
	return false;
}
