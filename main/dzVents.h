#pragma once
#include "EventSystem.h"

class CdzVents
{
public:
	CdzVents(void);
	~CdzVents(void);
	static CdzVents* GetInstance() { return &m_dzvents; }
	const std::string GetVersion();
	void LoadEvents();
	bool processLuaCommand(lua_State *lua_state, const std::string &filename, const int tIndex);
	void EvaluateDzVents(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items, const int secStatus);

	std::string m_scriptsDir, m_runtimeDir;
	bool m_bdzVentsExist;

private:

	enum _eType
	{
		TYPE_UNKNOWN,	// 0
		TYPE_STRING,	// 1
		TYPE_INTEGER,	// 2
		TYPE_BOOLEAN    // 3
	};
	struct _tLuaTableValues
	{
		_eType type;
		bool isTable;
		int tIndex;
		int iValue;
		std::string name;
		std::string sValue;
	};

	float RandomTime(const int randomTime);
	bool OpenURL(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable);
	bool UpdateDevice(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable);
	bool UpdateVariable(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable);
	bool CancelItem(lua_State *lua_state, const std::vector<_tLuaTableValues> &vLuaTable);
	void ExportDomoticzDataToLua(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items);
	void IterateTable(lua_State *lua_state, const int tIndex, std::vector<_tLuaTableValues> &vLuaTable);
	void SetGlobalVariables(lua_State *lua_state, const bool reasonTime, const int secStatus);
	void ProcessHttpResponse(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items);
	void ProcessSecurity(lua_State *lua_state, const std::vector<CEventSystem::_tEventQueue> &items);

	static int l_domoticz_print(lua_State* lua_state);
	static CdzVents m_dzvents;
	std::string m_version;
};
