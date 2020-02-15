#pragma once
#include <string>

struct lua_State;

class CLuaTable
{
public:

	void Publish();
	
	// constructors
	CLuaTable(lua_State *lua_state, std::string Name, int NrCols, int NrRows);
	CLuaTable(lua_State *lua_state, std::string Name);

	void InitTable(lua_State* lua_state, std::string Name, int NrCols, int NrRows);

	// string labels
	void AddString(std::string Label, std::string Value);
	void AddBool(std::string Label, bool Value);
	void AddInteger(std::string Label, long long Value);
	void AddNumber(std::string Label, long double Value);

	// index labels
	void AddString(long long Index, std::string Value);
	void AddBool(long long Index, bool Value);
	void AddInteger(long long Index, long long Value);
	void AddNumber(long long Index, long double Value);

	// sub tables
	void OpenSubTableEntry(long long Index, int NrCols, int NrRows);
	void OpenSubTableEntry(std::string Name, int NrCols, int NrRows);
	void CloseSubTableEntry(void);

	~CLuaTable();

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
		_eLabelType label_type;
		_eValueType value_type;
		std::string label;
		long long index;
		int nrCols;
		int nrRows;
		bool bValue;
		long long iValue;
		long double dValue;
		std::string sValue;
	};

	lua_State *m_lua_state;
	std::string m_name;
	std::vector<_tEntry> m_luatable;
	int m_subtable_level;

	void PushRow(std::vector<_tEntry>::iterator table_entry);
};
