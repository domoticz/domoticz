class CdzVents
{
public:
	CdzVents(void);
	~CdzVents(void);
	const std::string GetVersion();
	void ExportDomoticzDataToLua(lua_State *lua_state, const uint64_t deviceID, const uint64_t varID, const uint8_t reason);
	void SetGlobalVariables(lua_State *lua_state, const uint8_t reason);

private:
	std::string m_version;
};
