#include "stdafx.h"
#include "Helper.h"
#include "Logger.h"
#include "RFXtrx.h"
#include "../main/LuaCommon.h"

extern "C" {
#ifdef WITH_EXTERNAL_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#else
#include "../lua/src/lua.h"
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
#endif
}

#include <xpath_processor.h>

#include "json_helper.h"
#include "SQLHelper.h"
#include "mainworker.h"
#include "../hardware/hardwaretypes.h"

extern std::string szUserDataFolder;

int CLuaCommon::l_domoticz_applyXPath(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs >= 2)
	{
		if (lua_isstring(lua_state, 1) && lua_isstring(lua_state, 2))
		{
			std::string buffer = lua_tostring(lua_state, 1);
			std::string xpath = lua_tostring(lua_state, 2);

			TiXmlDocument doc;
			doc.Parse(buffer.c_str(), 0, TIXML_ENCODING_UTF8);

			TiXmlElement* root = doc.RootElement();
			if (!root)
			{
				_log.Log(LOG_ERROR, "CLuaHandler (applyXPath from LUA) : Invalid data received!");
				return 0;
			}
			TinyXPath::xpath_processor processor(root, xpath.c_str());
#ifdef WITH_EXTERNAL_TINYXPATH
			TIXML_STRING xresult = processor.S_compute_xpath();
#else
			TiXmlString xresult = processor.S_compute_xpath();
#endif
			lua_pushstring(lua_state, xresult.c_str());
			return 1;
		}
		else
		{
			_log.Log(LOG_ERROR, "CLuaHandler (applyXPath from LUA) : Incorrect parameters type");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "CLuaHandler (applyXPath from LUA) : Not enough parameters");
	}
	return 0;
}

int CLuaCommon::l_domoticz_applyJsonPath(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs >= 2)
	{
		if (lua_isstring(lua_state, 1) && lua_isstring(lua_state, 2))
		{
			std::string buffer = lua_tostring(lua_state, 1);
			std::string jsonpath = lua_tostring(lua_state, 2);

			Json::Value root;
			bool bRet = ParseJSon(buffer, root);
			if (!bRet)
			{
				_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid Json data received");
				return 0;
			}

			// Grab optional arguments
			Json::PathArgument arg1;
			Json::PathArgument arg2;
			Json::PathArgument arg3;
			Json::PathArgument arg4;
			Json::PathArgument arg5;
			if (nargs >= 3)
			{
				if (lua_isstring(lua_state, 3))
				{
					arg1 = Json::PathArgument(lua_tostring(lua_state, 3));
				}
				else
				{
					_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #1 for domoticz_applyJsonPath");
					return 0;
				}
				if (nargs >= 4)
				{
					if (lua_isstring(lua_state, 4))
					{
						arg2 = Json::PathArgument(lua_tostring(lua_state, 4));
					}
					else
					{
						_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #2 for domoticz_applyJsonPath");
						return 0;
					}
					if (nargs >= 5)
					{
						if (lua_isstring(lua_state, 5))
						{
							arg3 = Json::PathArgument(lua_tostring(lua_state, 5));
						}
						else
						{
							_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #3 for domoticz_applyJsonPath");
							return 0;
						}
						if (nargs >= 6)
						{
							if (lua_isstring(lua_state, 6))
							{
								arg2 = Json::PathArgument(lua_tostring(lua_state, 6));
							}
							else
							{
								_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #4 for domoticz_applyJsonPath");
								return 0;
							}
							if (nargs >= 7)
							{
								if (lua_isstring(lua_state, 7))
								{
									arg5 = Json::PathArgument(lua_tostring(lua_state, 7));
								}
								else
								{
									_log.Log(LOG_ERROR, "WebServer (applyJsonPath from LUA) : Invalid extra argument #5 for domoticz_applyJsonPath");
									return 0;
								}
							}
						}
					}
				}
			}

			try
			{
				// Apply the JsonPath to the Json
				Json::Path path(jsonpath, arg1, arg2, arg3, arg4, arg5);
				Json::Value& node = path.make(root);

				// Check if some data has been found
				if (!node.isNull())
				{
					if (node.isDouble())
					{
						lua_pushnumber(lua_state, node.asDouble());
						return 1;
					}
					if (node.isInt())
					{
						lua_pushnumber(lua_state, (double)node.asInt());
						return 1;
					}
					if (node.isInt64())
					{
						lua_pushnumber(lua_state, (double)node.asInt64());
						return 1;
					}
					if (node.isString())
					{
						lua_pushstring(lua_state, node.asCString());
						return 1;
					}
					lua_pushnil(lua_state);
					return 1;
				}
			}
			catch (Json::LogicError& e)
			{
				_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : JsonError '%s'",e.what());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Incorrect parameters type");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Incorrect parameters count");
	}
	return 0;
}
