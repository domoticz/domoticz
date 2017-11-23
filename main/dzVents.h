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
	float StringToFloatRandom(const std::string &randomString);
	void OpenURL(const std::map<std::string, std::string> &URLdata, const std::map<std::string, std::string> &URLheaders);

	std::string m_version;
};
