#pragma once

#include <string>
#include <boost/thread/shared_mutex.hpp>

#include "../httpclient/HTTPClient.h"

#include "LuaCommon.h"
#include "concurrent_queue.h"
#include "StoppableTask.h"
#include "NotificationObserver.h"

class CEventSystem : public CLuaCommon, StoppableTask, CNotificationObserver
{
	friend class CdzVents;
	friend class CLuaHandler;
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
		float fForSec = 0;
		float fAfterSec = 0;
		float fRandomSec = 0;
		int iRepeat = 1;
		float fRepeatSec = 0;
		bool bEventTrigger = false;
	};
public:
	enum _eReason
	{
		REASON_DEVICE,			// 0
		REASON_SCENEGROUP,		// 1
		REASON_USERVARIABLE,		// 2
		REASON_TIME,			// 3
		REASON_SECURITY,		// 4
		REASON_URL,			// 5
		REASON_NOTIFICATION,		// 6
		REASON_SHELLCOMMAND		// 7
	};

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
		std::string description;
		std::string deviceID;
		int batteryLevel;
		int protection;
		int signalLevel;
		int unit;
		int hardwareID;
		float AddjValue;
		float AddjMulti;
		float AddjValue2;
		float AddjMulti2;
		uint8_t customImage;
		std::string image;
		std::map<uint8_t, int> JsonMapInt;
		std::map<uint8_t, float> JsonMapFloat;
		std::map<uint8_t, bool> JsonMapBool;
		std::map<uint8_t, std::string> JsonMapString;
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
		int protection;
		std::string lastUpdate;
		std::vector<uint64_t> memberID;
	};

	struct _tHardwareListInt {
		std::string Name;
		int HardwareTypeVal;
		std::string HardwareType;
		bool Enabled;
	} tHardwareList;

	CEventSystem();
	~CEventSystem();

	void StartEventSystem();
	void StopEventSystem();

	void LoadEvents();
	void ProcessDevice(int HardwareID, uint64_t ulDevID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
			   const char *sValue);
	void UpdateBatteryLevel(uint64_t ulDevID, unsigned char batteryLevel);

	void RemoveSingleState(uint64_t ulDevID, _eReason reason);
	void WWWUpdateSingleState(uint64_t ulDevID, const std::string &devname, _eReason reason);
	void WWWUpdateSecurityState(int securityStatus);
	void WWWGetItemStates(std::vector<_tDeviceStatus> &iStates);
	void SetEnabled(bool bEnabled);
	void GetCurrentStates();
	void GetCurrentScenesGroups();
	void GetCurrentUserVariables();
	bool UpdateSceneGroup(uint64_t ulDevID, int nValue, const std::string &lastUpdate);
	void UpdateUserVariable(uint64_t ulDevID, const std::string &varValue, const std::string &lastUpdate);
	bool PythonScheduleEvent(const std::string &ID, const std::string &Action, const std::string &eventName);
	bool GetEventTrigger(uint64_t ulDevID, _eReason reason, bool bEventTrigger);
	void SetEventTrigger(uint64_t ulDevID, _eReason reason, float fDelayTime);
	bool CustomCommand(uint64_t idx, const std::string &sCommand);

	void TriggerURL(const std::string &result, const std::vector<std::string> &headerData, const std::string &callback);
	void TriggerShellCommand(const std::string &result, const std::string &scriptstderr, const std::string &callback, int exitcode, bool timeoutOccurred);


private:
	enum _eJsonType
	{
		JTYPE_STRING = 0,	// 0
		JTYPE_FLOAT,		// 1
		JTYPE_INT,			// 2
		JTYPE_BOOL			// 3
	};

	struct _tJsonMap
	{
		const char* szOriginal;
		const char* szNew;
		_eJsonType eType;
	};

	struct _tEventTrigger
	{
		uint64_t ID;
		_eReason reason;
		time_t timestamp;
	};

	struct _tEventQueue
	{
		_eReason reason;
		uint64_t id;
		std::string devname;
		int nValue;
		std::string sValue;
		std::string nValueWording;
		std::string lastUpdate;
		std::string errorText;
		bool timeoutOccurred;
		uint8_t lastLevel;
		std::vector<std::string> vData;
		std::map<uint8_t, int> JsonMapInt;
		std::map<uint8_t, float> JsonMapFloat;
		std::map<uint8_t, bool> JsonMapBool;
		std::map<uint8_t, std::string> JsonMapString;
		queue_element_trigger* trigger = nullptr;
	};
	concurrent_queue<_tEventQueue> m_eventqueue;

	std::vector<_tEventTrigger> m_eventtrigger;
	bool m_bEnabled;
	boost::shared_mutex m_devicestatesMutex;
	boost::shared_mutex m_eventsMutex;
	boost::shared_mutex m_uservariablesMutex;
	boost::shared_mutex m_scenesgroupsMutex;
	boost::shared_mutex m_eventtriggerMutex;
	std::mutex m_measurementStatesMutex;
	std::mutex luaMutex;
	std::shared_ptr<std::thread> m_thread;
	std::shared_ptr<std::thread> m_eventqueuethread;
	StoppableTask m_TaskQueue;
	int m_SecStatus;
	std::string m_lua_Dir;
	std::string m_szStartTime;

	static const std::string m_szReason[], m_szSecStatus[];
	static const _tJsonMap JsonMap[];

	//our thread
	void Do_Work();
	void ProcessMinute();
	void GetCurrentMeasurementStates();
	std::string UpdateSingleState(uint64_t ulDevID, const std::string &devname, int nValue, const std::string &sValue, unsigned char devType, unsigned char subType, _eSwitchType switchType,
				      const std::string &lastUpdate, unsigned char lastLevel, unsigned char batteryLevel, const std::map<std::string, std::string> &options);
	void EvaluateEvent(const std::vector<_tEventQueue> &items);
	void EvaluateDatabaseEvents(const _tEventQueue &item);
	lua_State *ParseBlocklyLua(lua_State *lua_state, const _tEventItem &item);
	bool parseBlocklyActions(const _tEventItem &item);
	std::string ProcessVariableArgument(const std::string &Argument);
#ifdef ENABLE_PYTHON
	std::string m_python_Dir;
	void EvaluatePython(const _tEventQueue &item, const std::string &filename, const std::string &PyString);
#endif
	void EvaluateLua(const _tEventQueue &item, const std::string &filename, const std::string &LuaString);
	void EvaluateLua(const std::vector<_tEventQueue> &items, const std::string &filename, const std::string &LuaString);
	void luaThread(lua_State *lua_state, const std::string &filename);
	static void luaStop(lua_State *L, lua_Debug *ar);
	std::string nValueToWording(uint8_t dType, uint8_t dSubType, _eSwitchType switchtype, int nValue, const std::string &sValue, const std::map<std::string, std::string> &options);
	static int l_domoticz_print(lua_State* lua_state);
	void OpenURL(float delay, const std::string &URL);
	void WriteToLog(const std::string &devNameNoQuotes, const std::string &doWhat);
	bool ScheduleEvent(int deviceID, const std::string &Action, bool isScene, const std::string &eventName, int sceneType);
	bool ScheduleEvent(std::string ID, const std::string &Action, const std::string &eventName);
	lua_State *CreateBlocklyLuaState();

	std::string ParseBlocklyString(const std::string &oString);
	void ParseActionString( const std::string &oAction_, _tActionParseResults &oResults_ );
	void UpdateJsonMap(_tDeviceStatus &item, uint64_t ulDevID);
	void EventQueueThread();
	void UnlockEventQueueThread();
	void ExportDeviceStatesToLua(lua_State *lua_state, const _tEventQueue &item);
	void EvaluateLuaClassic(lua_State *lua_state, const _tEventQueue &item, int secStatus);

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

	void reportMissingDevice(int deviceID, const _tEventItem &item);
	int getSunRiseSunSetMinutes(const std::string &what);
	bool isEventscheduled(const std::string &eventName);
	bool iterateLuaTable(lua_State *lua_state, int tIndex, const std::string &filename);
	bool processLuaCommand(lua_State *lua_state, const std::string &filename);
	void report_errors(lua_State *L, int status, const std::string &filename);
	int calculateDimLevel(int deviceID, int percentageLevel);
	void StripQuotes(std::string &sString);
	std::string SpaceToUnderscore(std::string sResult);
	std::string LowerCase(std::string sResult);

	bool Update(Notification::_eType type, Notification::_eStatus status, const std::string &eventdata) override;
};
