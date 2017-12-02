class CdzVents
{
public:
	CdzVents(void);
	~CdzVents(void);
	const std::string GetVersion();
	// use int for reason until C++11 is used (forward declare enum)
	void ExportDomoticzDataToLua(lua_State *lua_state, const uint64_t deviceID, const uint64_t varID, const int reason);
	void SetGlobalVariables(lua_State *lua_state, const int reason);
	void LoadEvents();
	bool processLuaCommand(lua_State *lua_state, const std::string &filename, const int tIndex);
	void ProcessHttpResponse(lua_State *lua_state, const std::vector<std::string> &item, const std::string &sValue, const std::string &nValueWording);

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
	void IterateTable(lua_State *lua_state, const int tIndex, std::vector<_tLuaTableValues> &vLuaTable);

	std::string m_version;
};
