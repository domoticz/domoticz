#pragma once

#include <string>
#include <vector>

extern "C" {
#ifdef WITH_EXTERNAL_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#else
#include "../lua/src/lua.h"
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
#endif
}

#include "LuaCommon.h"

class CEventSystem : public CLuaCommon
{
	typedef struct lua_State lua_State;

	struct _tEventItem
	{
		uint64_t ID;
		std::string Name;
		std::string Interpreter;
		std::string Type;
		std::string Conditions;
		std::string Actions;
		int SequenceNo;
		int EventStatus;

	};

	struct _tActionParseResults
	{
		std::string sCommand;
		float fForSec;
		float fAfterSec;
		float fRandomSec;
		int iRepeat;
		float fRepeatSec;
	};

public:
	struct _tDeviceStatus
	{
		uint64_t ID;
		std::string deviceName;
		int nValue;
		std::string sValue;
		uint8_t devType;
		uint8_t subType;
		std::string nValueWording;
		std::string lastUpdate;
		uint8_t lastLevel;
		uint8_t switchtype;
	};

	struct _tUserVariable
	{
		uint64_t ID;
		std::string variableName;
		std::string variableValue;
		int variableType;
		std::string lastUpdate;
	};

	struct _tScenesGroups
	{
		uint64_t ID;
		std::string scenesgroupName;
		std::string scenesgroupValue;
		int scenesgroupType;
		std::string lastUpdate;
	};

	CEventSystem(void);
	~CEventSystem(void);

	void StartEventSystem();
	void StopEventSystem();

	void LoadEvents();
	void ProcessUserVariable(const uint64_t varId);
	void ProcessDevice(const int HardwareID, const uint64_t ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string &devname, const int varId);
	void RemoveSingleState(int ulDevID);
	void WWWUpdateSingleState(const uint64_t ulDevID, const std::string &devname);
	void WWWUpdateSecurityState(int securityStatus);
	void WWWGetItemStates(std::vector<_tDeviceStatus> &iStates);
	void SetEnabled(const bool bEnabled);
	void GetCurrentStates();

	void exportDeviceStatesToLua(lua_State *lua_state);

    bool PythonScheduleEvent(std::string ID, const std::string &Action, const std::string &eventName);

private:
	//lua_State	*m_pLUA;
	bool m_bEnabled;
	boost::shared_mutex m_devicestatesMutex;
	boost::shared_mutex m_eventsMutex;
	boost::shared_mutex m_uservariablesMutex;
	boost::shared_mutex m_scenesgroupsMutex;
	boost::mutex m_measurementStatesMutex;
	boost::mutex luaMutex;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	int m_SecStatus;


	//our thread
	void Do_Work();
	void ProcessMinute();
	void GetCurrentMeasurementStates();
	void GetCurrentUserVariables();
	void GetCurrentScenesGroups();
	std::string UpdateSingleState(const uint64_t ulDevID, const std::string &devname, const int nValue, const char* sValue, const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, const std::string &lastUpdate, const unsigned char lastLevel, const std::map<std::string, std::string> & options);
	void EvaluateEvent(const std::string &reason);
	void EvaluateEvent(const std::string &reason, const uint64_t varId);
	void EvaluateEvent(const std::string &reason, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId);
	void EvaluateBlockly(const std::string &reason, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId);
	bool parseBlocklyActions(const std::string &Actions, const std::string &eventName, const uint64_t eventID);
	std::string ProcessVariableArgument(const std::string &Argument);
#ifdef ENABLE_PYTHON
	void EvaluatePython(const std::string &reason, const std::string &filename, const std::string &PyString, const uint64_t varId);
	void EvaluatePython(const std::string &reason, const std::string &filename, const std::string &PyString);
	void EvaluatePython(const std::string &reason, const std::string &filename, const std::string &PyString, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId);
#endif
	void EvaluateLua(const std::string &reason, const std::string &filename, const std::string &LuaString, const uint64_t varId);
	void EvaluateLua(const std::string &reason, const std::string &filename, const std::string &LuaString);
	void EvaluateLua(const std::string &reason, const std::string &filename, const std::string &LuaString, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId);
	void luaThread(lua_State *lua_state, const std::string &filename);
	static void luaStop(lua_State *L, lua_Debug *ar);
	std::string nValueToWording(const uint8_t dType, const uint8_t dSubType, const _eSwitchType switchtype, const int nValue, const std::string &sValue, const std::map<std::string, std::string> & options);
	static int l_domoticz_print(lua_State* lua_state);
	void OpenURL(const std::string &URL);
	void WriteToLog(const std::string &devNameNoQuotes, const std::string &doWhat);
	bool ScheduleEvent(int deviceID, std::string Action, bool isScene, const std::string &eventName, int sceneType);
	bool ScheduleEvent(std::string ID, const std::string &Action, const std::string &eventName);
	void UpdateDevice(const std::string &DevParams);
	lua_State *CreateBlocklyLuaState();

	std::string ParseBlocklyString(const std::string &oString);
	void ParseActionString( const std::string &oAction_, _tActionParseResults &oResults_ );

	//std::string reciprocalAction (std::string Action);
	std::vector<_tEventItem> m_events;


	std::map<uint64_t, _tDeviceStatus> m_devicestates;
	std::map<uint64_t, _tUserVariable> m_uservariables;
	std::map<uint64_t, _tScenesGroups> m_scenesgroups;
	std::map<std::string, float> m_tempValuesByName;
	std::map<std::string, float> m_dewValuesByName;
	std::map<std::string, float> m_rainValuesByName;
	std::map<std::string, float> m_rainLastHourValuesByName;
	std::map<std::string, float> m_uvValuesByName;
	std::map<std::string, float> m_weatherValuesByName;
	std::map<std::string, int>	 m_humValuesByName;
	std::map<std::string, float> m_baroValuesByName;
	std::map<std::string, float> m_utilityValuesByName;
	std::map<std::string, float> m_winddirValuesByName;
	std::map<std::string, float> m_windspeedValuesByName;
	std::map<std::string, float> m_windgustValuesByName;
	std::map<std::string, int>	 m_zwaveAlarmValuesByName;

	std::map<uint64_t, float> m_tempValuesByID;
	std::map<uint64_t, float> m_dewValuesByID;
	std::map<uint64_t, float> m_rainValuesByID;
	std::map<uint64_t, float> m_rainLastHourValuesByID;
	std::map<uint64_t, float> m_uvValuesByID;
	std::map<uint64_t, float> m_weatherValuesByID;
	std::map<uint64_t, int>	m_humValuesByID;
	std::map<uint64_t, float> m_baroValuesByID;
	std::map<uint64_t, float> m_utilityValuesByID;
	std::map<uint64_t, float> m_winddirValuesByID;
	std::map<uint64_t, float> m_windspeedValuesByID;
	std::map<uint64_t, float> m_windgustValuesByID;
	std::map<uint64_t, int> m_zwaveAlarmValuesByID;

	void reportMissingDevice(const int deviceID, const std::string &EventName, const uint64_t eventID);
	int getSunRiseSunSetMinutes(const std::string &what);
	bool isEventscheduled(const std::string &eventName);
	bool iterateLuaTable(lua_State *lua_state, const int tIndex, const std::string &filename);
	bool processLuaCommand(lua_State *lua_state, const std::string &filename);
	void report_errors(lua_State *L, int status, std::string filename);
	unsigned char calculateDimLevel(int deviceID, int percentageLevel);
	void StripQuotes(std::string &sString);
};
