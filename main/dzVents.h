class CdzVents
{
public:
	CdzVents(void);
	~CdzVents(void);
	const std::string GetVersion();
	void ExportDomoticzDataToLua(lua_State *lua_state, const uint64_t deviceID, const uint64_t varID, const std::string &reason);
	void SetGlobalVariables(lua_State *lua_state, const std::string &reason);

private:
	std::string m_version;
};
