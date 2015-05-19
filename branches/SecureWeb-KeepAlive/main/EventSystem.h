#pragma once

#include <string>
#include <vector>

extern "C" {
#include "../lua/src/lua.h"    
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
}

class CEventSystem
{
	typedef struct lua_State lua_State;

	struct _tEventItem
	{
		unsigned long long ID;
		std::string Name;
		std::string Conditions;
		std::string Actions;
		int SequenceNo;
		int EventStatus;

	};

public:
	struct _tDeviceStatus
	{
		unsigned long long ID;
		std::string deviceName;
		unsigned long long nValue;
		std::string sValue;
		unsigned char devType;
		unsigned char subType;
		std::string nValueWording;
		std::string lastUpdate;
		unsigned char lastLevel;
		unsigned char switchtype;
	};
	std::map<unsigned long long, _tDeviceStatus> m_devicestates;

	struct _tUserVariable
	{
		unsigned long long ID;
		std::string variableName;
		std::string variableValue;
		int variableType;
		std::string lastUpdate;
	};
	std::map<unsigned long long, _tUserVariable> m_uservariables;

	CEventSystem(void);
	~CEventSystem(void);

	void StartEventSystem();
	void StopEventSystem();

	void LoadEvents();
	void ProcessUserVariable(const unsigned long long varId);
	void ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string &devname, const int varId);
	void RemoveSingleState(int ulDevID);
	void WWWUpdateSingleState(const unsigned long long ulDevID, const std::string &devname);
	void WWWUpdateSecurityState(int securityStatus);
	void WWWGetItemStates(std::vector<_tDeviceStatus> &iStates);
	void SetEnabled(const bool bEnabled);
private:
	//lua_State	*m_pLUA;
	bool m_bEnabled;
	boost::mutex eventMutex;
	boost::mutex luaMutex;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	int m_SecStatus;


	//our thread
	void Do_Work();
	void ProcessMinute();
	void GetCurrentStates();
	void GetCurrentMeasurementStates();
	void GetCurrentUserVariables();
	std::string UpdateSingleState(const unsigned long long ulDevID, const std::string &devname, const int nValue, const char* sValue, const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, const std::string &lastUpdate, const unsigned char lastLevel);
	void EvaluateEvent(const std::string &reason);
	void EvaluateEvent(const std::string &reason, const unsigned long long varId);
	void EvaluateEvent(const std::string &reason, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId);
	void EvaluateBlockly(const std::string &reason, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId);
	bool parseBlocklyActions(const std::string &Actions, const std::string &eventName, const unsigned long long eventID);
#ifdef ENABLE_PYTHON
	void EvaluatePython(const std::string &reason, const std::string &filename, const unsigned long long varId);
	void EvaluatePython(const std::string &reason, const std::string &filename);
	void EvaluatePython(const std::string &reason, const std::string &filename, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId);
#endif
	void EvaluateLua(const std::string &reason, const std::string &filename, const unsigned long long varId);
	void EvaluateLua(const std::string &reason, const std::string &filename);
	void EvaluateLua(const std::string &reason, const std::string &filename, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId);
	void luaThread(lua_State *lua_state, const std::string &filename);
	static void luaStop(lua_State *L, lua_Debug *ar);
	std::string nValueToWording(const unsigned char dType, const unsigned char dSubType, const _eSwitchType switchtype, const unsigned char nValue, const std::string &sValue);
	static int l_domoticz_print(lua_State* lua_state);
	void SendEventNotification(const std::string &Subject, const std::string &Body, const std::string &ExtraData, const int Priority, const std::string &Sound);
	void OpenURL(const std::string &URL);
	void WriteToLog(const std::string &devNameNoQuotes, const std::string &doWhat);
	bool ScheduleEvent(int deviceID, std::string Action, bool isScene, const std::string &eventName, int sceneType);
	bool ScheduleEvent(std::string ID, const std::string &Action, const std::string &eventName);
	void UpdateDevice(const std::string &DevParams);
	//std::string reciprocalAction (std::string Action);
	std::vector<_tEventItem> m_events;
	

	std::map<std::string, float> m_tempValuesByName;
	std::map<std::string, float> m_dewValuesByName;
	std::map<std::string, float> m_rainValuesByName;
	std::map<std::string, float> m_rainLastHourValuesByName;
	std::map<std::string, float> m_uvValuesByName;
	std::map<std::string, unsigned char> m_humValuesByName;
	std::map<std::string, float> m_baroValuesByName;
	std::map<std::string, float> m_utilityValuesByName;
	std::map<std::string, float> m_winddirValuesByName;
	std::map<std::string, float> m_windspeedValuesByName;
	std::map<std::string, float> m_windgustValuesByName;

	std::map<unsigned long long, float> m_tempValuesByID;
	std::map<unsigned long long, float> m_dewValuesByID;
	std::map<unsigned long long, float> m_rainValuesByID;
	std::map<unsigned long long, float> m_rainLastHourValuesByID;
	std::map<unsigned long long, float> m_uvValuesByID;
	std::map<unsigned long long, unsigned char> m_humValuesByID;
	std::map<unsigned long long, float> m_baroValuesByID;
	std::map<unsigned long long, float> m_utilityValuesByID;
	std::map<unsigned long long, float> m_winddirValuesByID;
	std::map<unsigned long long, float> m_windspeedValuesByID;
	std::map<unsigned long long, float> m_windgustValuesByID;

	void reportMissingDevice(const int deviceID, const std::string &EventName, const unsigned long long eventID);
	int getSunRiseSunSetMinutes(const std::string &what);
	bool isEventscheduled(const std::string &eventName);
	bool iterateLuaTable(lua_State *lua_state, const int tIndex, const std::string &filename);
	bool processLuaCommand(lua_State *lua_state, const std::string &filename);
	void report_errors(lua_State *L, int status);
	unsigned char calculateDimLevel(int deviceID, int percentageLevel);

};

