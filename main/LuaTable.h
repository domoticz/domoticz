#pragma once
#include <string>

struct lua_State;

class CLuaTable
{
public:

	void Publish();
	
	// constructors
	CLuaTable(lua_State *lua_state, const std::string &Name, int NrCols, int NrRows);
	CLuaTable(lua_State *lua_state, const std::string &Name);

	void InitTable(lua_State *lua_state, const std::string &Name, int NrCols, int NrRows);

	// string labels
	void AddString(std::string Label, std::string Value);
	void AddBool(std::string Label, bool Value);
	void AddInteger(std::string Label, int64_t Value);
	void AddNumber(std::string Label, long double Value);

	// index labels
	void AddString(int64_t Index, std::string Value);
	void AddBool(int64_t Index, bool Value);
	void AddInteger(int64_t Index, int64_t Value);
	void AddNumber(int64_t Index, long double Value);

	// sub tables
	void OpenSubTableEntry(int64_t Index, int NrCols, int NrRows);
	void OpenSubTableEntry(std::string Name, int NrCols, int NrRows);
	void CloseSubTableEntry();

	~CLuaTable() = default;

      private:
	enum _eValueType
	{
		TYPE_STRING,
		TYPE_INTEGER,
		TYPE_BOOL,
		TYPE_NUMBER
	};

	enum _eLabelType
	{
		TYPE_TABLE,
		TYPE_SUBTABLE_OPEN_LABEL,
		TYPE_SUBTABLE_OPEN_INDEX,
		TYPE_SUBTABLE_CLOSE,
		TYPE_VALUE_LABEL,
		TYPE_VALUE_INDEX
	};

	struct _tEntry
	{
		_eLabelType label_type = TYPE_TABLE;
		_eValueType value_type = TYPE_STRING;
		std::string label;
		int64_t index = 0;
		int nrCols = 0;
		int nrRows = 0;
		bool bValue = false;
		int64_t iValue = 0;
		long double dValue = 0;
		std::string sValue;
	};

	lua_State *m_lua_state;
	std::string m_name;
	std::vector<_tEntry> m_luatable;
	int m_subtable_level;

	void PushRow(std::vector<_tEntry>::iterator table_entry);
};
