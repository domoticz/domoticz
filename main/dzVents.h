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

private:
	enum _eType
	{
		TYPE_UNKNOWN,	// 0
		TYPE_STRING,	// 1
		TYPE_INTEGER	// 2
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
	bool IterateTable(lua_State *lua_state, const int tIndex, int index, std::map<int, _tLuaTableValues> &mLuaTable);
	float RandomTime(const int randomTime);
	void OpenURL(const std::map<std::string, std::string> &URLdata, const std::map<std::string, std::string> &URLheaders);

	std::string m_version;
};
