#pragma once

struct lua_State;
struct lua_Debug;

#include "LuaCommon.h"

class CLuaHandler : public CLuaCommon
{
public:
	explicit CLuaHandler(int hwdID = 0);

	bool executeLuaScript(const std::string &script, const std::string &content);
	bool executeLuaScript(const std::string &script, const std::string &content, std::vector<std::string>& allParameters);

private:
	void luaThread(lua_State *lua_state, const std::string &filename);
	static void luaStop(lua_State *L, lua_Debug *ar);
	void report_errors(lua_State *L, int status);

	static int l_domoticz_print(lua_State* lua_state);
	static int l_domoticz_updateDevice(lua_State* lua_state);

	int m_HwdID;
};

