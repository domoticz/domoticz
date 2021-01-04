#pragma once

struct lua_State;
struct lua_Debug;

class CLuaCommon
{
public:
  CLuaCommon() = default;
  ;

protected:
	static int l_domoticz_applyJsonPath(lua_State* lua_state);
	static int l_domoticz_applyXPath(lua_State* lua_state);
};
