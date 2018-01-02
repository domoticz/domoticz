class CdzVents
{
public:
	CdzVents(void);
	~CdzVents(void);
	const std::string GetVersion();
	// use int for reason until C++11 is used (forward declare enum)
	void ExportDomoticzDataToLua(lua_State *lua_state, const uint64_t deviceID, const uint64_t varID, const int reason);
	void SetGlobalVariables(lua_State *lua_state, const int reason);

private:
	std::string m_version;
};
