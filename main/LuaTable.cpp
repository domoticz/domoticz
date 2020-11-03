#include "stdafx.h"
#include "LuaTable.h"
#include "../main/Logger.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

CLuaTable::CLuaTable(lua_State *lua_state, std::string Name)
{
	InitTable(lua_state, Name, 0, 0);
}

CLuaTable::CLuaTable(lua_State *lua_state, std::string Name, int NrCols, int NrRows)
{
	InitTable(lua_state, Name, NrCols, NrRows);
}

CLuaTable::~CLuaTable(void)
{
}

void CLuaTable::InitTable(lua_State* lua_state, std::string Name, int NrCols, int NrRows)
{
	m_lua_state = lua_state;
	m_name = Name;
	m_subtable_level = 0;
	m_luatable.clear();

	_tEntry tableEntry;
	tableEntry.label_type = TYPE_TABLE;
	tableEntry.label = Name;
	tableEntry.nrCols = NrCols;
	tableEntry.nrRows = NrRows;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddString(std::string Label, std::string Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_LABEL;
	tableEntry.value_type = TYPE_STRING;
	tableEntry.label = Label;
	tableEntry.sValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddBool(std::string Label, bool Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_LABEL;
	tableEntry.value_type = TYPE_BOOL;
	tableEntry.label = Label;
	tableEntry.bValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddNumber(std::string Label, long double Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_LABEL;
	tableEntry.value_type = TYPE_NUMBER;
	tableEntry.label = Label;
	tableEntry.dValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddInteger(std::string Label, long long Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_LABEL;
	tableEntry.value_type = TYPE_INTEGER;
	tableEntry.label = Label;
	tableEntry.iValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddInteger(long long Index, long long Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_INDEX;
	tableEntry.value_type = TYPE_INTEGER;
	//tableEntry.label = std::to_string(Index);
	tableEntry.index = Index;
	tableEntry.iValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddString(long long Index, std::string Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_INDEX;
	tableEntry.value_type = TYPE_STRING;
	//tableEntry.label = std::to_string(Index);
	tableEntry.index = Index;
	tableEntry.sValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddNumber(long long Index, long double Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_INDEX;
	tableEntry.value_type = TYPE_NUMBER;
	//tableEntry.label = std::to_string(Index);
	tableEntry.index = Index;
	tableEntry.dValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::AddBool(long long Index, bool Value)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_VALUE_INDEX;
	tableEntry.value_type = TYPE_BOOL;
	//tableEntry.label = std::to_string(Index);
	tableEntry.index = Index;
	tableEntry.bValue = Value;
	m_luatable.push_back(tableEntry);
}

void CLuaTable::OpenSubTableEntry(long long Index, int NrCols, int NrRows)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_SUBTABLE_OPEN_INDEX;
	//tableEntry.label = std::to_string(Index);
	tableEntry.index = Index;
	tableEntry.nrCols = NrCols;
	tableEntry.nrRows = NrRows;
	m_luatable.push_back(tableEntry);
	m_subtable_level++;
}

void CLuaTable::OpenSubTableEntry(std::string Name, int NrCols, int NrRows)
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_SUBTABLE_OPEN_LABEL;
	tableEntry.label = Name;
	tableEntry.nrCols = NrCols;
	tableEntry.nrRows = NrRows;
	m_luatable.push_back(tableEntry);
	m_subtable_level++;
}

void CLuaTable::CloseSubTableEntry()
{
	_tEntry tableEntry;
	tableEntry.label_type = TYPE_SUBTABLE_CLOSE;
	m_luatable.push_back(tableEntry);
	m_subtable_level--;
}

void CLuaTable::PushRow(std::vector<_tEntry>::iterator itt)
{
	if (itt->label_type == TYPE_VALUE_INDEX)
		lua_pushinteger(m_lua_state, (lua_Integer)itt->index);
	else
		lua_pushstring(m_lua_state, itt->label.c_str());

	switch (itt->value_type)
	{
	case TYPE_BOOL:
		lua_pushboolean(m_lua_state, itt->bValue);
		break;
	case TYPE_INTEGER:
		lua_pushinteger(m_lua_state, (lua_Integer)itt->iValue);
		break;
	case TYPE_NUMBER:
		lua_pushnumber(m_lua_state, (lua_Number)itt->dValue);
		break;
	case TYPE_STRING:
		lua_pushstring(m_lua_state, itt->sValue.c_str());
		break;
	default:
		_log.Log(LOG_ERROR, "Unsupported value type in LuaTable!");
	}
	lua_rawset(m_lua_state, -3);
}

void CLuaTable::Publish()
{
	if ((m_subtable_level == 0) && (m_luatable.size() > 0))
	{
		for (auto itt = m_luatable.begin(); itt != m_luatable.end(); itt++)
		{
			switch (itt->label_type)
			{
			case TYPE_TABLE:
				lua_createtable(m_lua_state, itt->nrCols, itt->nrRows);
				break;
			case TYPE_SUBTABLE_OPEN_LABEL:
				lua_pushstring(m_lua_state, itt->label.c_str());
				lua_createtable(m_lua_state, itt->nrCols, itt->nrRows);
				break;
			case TYPE_SUBTABLE_OPEN_INDEX:
				lua_pushinteger(m_lua_state, (lua_Integer)itt->index);
				lua_createtable(m_lua_state, itt->nrCols, itt->nrRows);
				break;
			case TYPE_SUBTABLE_CLOSE:
				lua_settable(m_lua_state, -3);
				break;
			case TYPE_VALUE_LABEL:
			case TYPE_VALUE_INDEX:
				PushRow(itt);
				break;
			default:
				_log.Log(LOG_ERROR, "Unsupported label type in LuaTable!");
			}
		}
		lua_setglobal(m_lua_state, m_name.c_str());
		m_luatable.clear();
	}
	else
		_log.Log(LOG_ERROR, "Lua table %s is not published. Not all sub tables are closed!", m_name.c_str());	
}


