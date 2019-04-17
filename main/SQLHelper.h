#pragma once

#include <string>
#include "RFXNames.h"
#include "../hardware/hardwaretypes.h"
#include "Helper.h"
#include "../httpclient/UrlEncode.h"
#include "../httpclient/HTTPClient.h"
#include "StoppableTask.h"

#define timer_resolution_hz 25

struct sqlite3;

enum _eWindUnit
{
	WINDUNIT_MS=0,
	WINDUNIT_KMH,
	WINDUNIT_MPH,
	WINDUNIT_Knots,
	WINDUNIT_Beaufort,
};

enum _eTempUnit
{
	TEMPUNIT_C=0,
	TEMPUNIT_F,
};

enum _eWeightUnit
{
    WEIGHTUNIT_KG,
    WEIGHTUNIT_LB,
};

enum _eUsrVariableType
{
	USERVARTYPE_INTEGER = 0,
	USERVARTYPE_FLOAT, //1
	USERVARTYPE_STRING, //2
	USERVARTYPE_DATE, //3 Date in format DD/MM/YYYY
	USERVARTYPE_TIME, //4 Time in 24 hr format HH:MM
};

enum _eTaskItemType
{
	TITEM_SWITCHCMD=0,
	TITEM_EXECUTE_SCRIPT,
	TITEM_EMAIL_CAMERA_SNAPSHOT,
	TITEM_SEND_EMAIL,
	TITEM_SWITCHCMD_EVENT,
	TITEM_SWITCHCMD_SCENE,
	TITEM_GETURL,
	TITEM_SEND_EMAIL_TO,
	TITEM_SET_VARIABLE,
	TITEM_SEND_SMS,
	TITEM_SEND_NOTIFICATION,
	TITEM_SET_SETPOINT,
	TITEM_SEND_IFTTT_TRIGGER,
	TITEM_UPDATEDEVICE,
	TITEM_CUSTOM_COMMAND,
};

struct _tTaskItem
{
	_eTaskItemType _ItemType;
	float _DelayTime;
	int _HardwareID;
	uint64_t _idx;
	std::string _ID;
	unsigned char _unit;
	unsigned char _devType;
	unsigned char _subType;
	unsigned char _signallevel;
	unsigned char _batterylevel;
	int _switchtype;
	int _nValue;
	std::string _sValue;
	std::string _command;
	std::string _sUntil;
	int _level;
	_tColor _Color;
	std::string _relatedEvent;
	timeval _DelayTimeBegin;

	static _tTaskItem UpdateDevice(const float DelayTime, const uint64_t idx, const int nValue, const std::string &sValue, const int Protected, const bool bEventTrigger)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_UPDATEDEVICE;
		tItem._DelayTime = DelayTime;
		tItem._idx = idx;
		tItem._nValue = nValue;
		tItem._sValue = sValue;
		tItem._HardwareID = Protected;
		tItem._switchtype = bEventTrigger ? 1 : 0;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}

	static _tTaskItem SwitchLight(const float DelayTime, const uint64_t idx, const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const int switchtype, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SWITCHCMD;
		tItem._DelayTime=DelayTime;
		tItem._idx=idx;
		tItem._HardwareID=HardwareID;
		tItem._ID=ID;
		tItem._unit=unit;
		tItem._devType=devType;
		tItem._subType=subType;
		tItem._signallevel=signallevel;
		tItem._batterylevel=batterylevel;
		tItem._switchtype=switchtype;
		tItem._nValue=nValue;
		tItem._sValue=sValue;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem ExecuteScript(const float DelayTime, const std::string &ScriptPath, const std::string &ScriptParams)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_EXECUTE_SCRIPT;
		tItem._DelayTime=DelayTime;
		tItem._ID=ScriptPath;
		tItem._sValue=ScriptParams;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem EmailCameraSnapshot(const float DelayTime, const std::string &CamIdx, const std::string &Subject)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_EMAIL_CAMERA_SNAPSHOT;
		tItem._DelayTime=DelayTime;
		tItem._ID=CamIdx;
		tItem._sValue=Subject;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SendEmail(const float DelayTime, const std::string &Subject, const std::string &Body)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SEND_EMAIL;
		tItem._DelayTime=DelayTime;
		tItem._ID=Subject;
		tItem._sValue=Body;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SendEmailTo(const float DelayTime, const std::string &Subject, const std::string &Body, const std::string &To)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SEND_EMAIL_TO;
		tItem._DelayTime=DelayTime;
		tItem._ID=Subject;
		tItem._sValue=Body;
		tItem._command=To;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SendSMS(const float DelayTime, const std::string &Subject)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_SEND_SMS;
		tItem._DelayTime = DelayTime;
		tItem._ID = Subject;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SendIFTTTTrigger(const float DelayTime, const std::string &EventID, const std::string &Value1, const std::string &Value2, const std::string &Value3)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_SEND_IFTTT_TRIGGER;
		tItem._DelayTime = DelayTime;
		tItem._ID = EventID;
		tItem._command = Value1 + "!#" + Value2 + "!#" + Value3;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SwitchLightEvent(const float DelayTime, const uint64_t idx, const std::string &Command, const int Level, const _tColor Color, const std::string &eventName)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SWITCHCMD_EVENT;
		tItem._DelayTime=DelayTime;
		tItem._idx=idx;
		tItem._command= Command;
		tItem._level= Level;
		tItem._Color=Color;
		tItem._relatedEvent = eventName;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SwitchSceneEvent(const float DelayTime, const uint64_t idx, const std::string &Command, const std::string &eventName)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SWITCHCMD_SCENE;
		tItem._DelayTime=DelayTime;
		tItem._idx=idx;
		tItem._command= Command;
		tItem._relatedEvent = eventName;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem GetHTTPPage(const float DelayTime, const std::string &URL, const std::string &/*eventName*/)
	{
		return GetHTTPPage(DelayTime, URL, "", HTTPClient::HTTP_METHOD_GET, "", "");
	}
	static _tTaskItem GetHTTPPage(const float DelayTime, const std::string &URL, const std::string &extraHeaders, const HTTPClient::_eHTTPmethod method, const std::string &postData, const std::string &trigger)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_GETURL;
		tItem._DelayTime = DelayTime;
		tItem._sValue = URL;
		tItem._switchtype = method;
		tItem._relatedEvent = extraHeaders;
		tItem._command = postData;
		tItem._ID = trigger;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SetVariable(const float DelayTime, const uint64_t idx, const std::string &varvalue, const bool eventtrigger)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_SET_VARIABLE;
		tItem._DelayTime = DelayTime;
		tItem._idx = idx;
		tItem._sValue = varvalue;
		tItem._nValue = (eventtrigger==true)?1:0;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SendNotification(const float DelayTime, const std::string &Subject, const std::string &Body, const std::string &ExtraData, const int Priority, const std::string &Sound, const std::string &SubSystem)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_SEND_NOTIFICATION;
		tItem._DelayTime = DelayTime;
		tItem._idx = Priority;
		std::string tSubject((!Subject.empty()) ? Subject : " ");
		std::string tBody((!Body.empty()) ? Body : " ");
		std::string tExtraData((!ExtraData.empty()) ? ExtraData : " ");
		std::string tSound((!Sound.empty()) ? Sound : " ");
		std::string tSubSystem((!SubSystem.empty()) ? SubSystem : " ");
		tItem._command = tSubject + "!#" + tBody + "!#" + tExtraData + "!#" + tSound + "!#" + tSubSystem ;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem SetSetPoint(const float DelayTime, const uint64_t idx, const std::string &varvalue, const std::string &mode = std::string(), const std::string &until = std::string())
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_SET_SETPOINT;
		tItem._DelayTime = DelayTime;
		tItem._idx = idx;
		tItem._sValue = varvalue;
		tItem._command = mode;
		tItem._sUntil = until;

		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
	static _tTaskItem CustomCommand(const float DelayTime, const uint64_t idx, const std::string &cmdstr)
	{
		_tTaskItem tItem;
		tItem._ItemType = TITEM_CUSTOM_COMMAND;
		tItem._DelayTime = DelayTime;
		tItem._idx = idx;
		tItem._command = cmdstr;
		if (DelayTime)
			getclock(&tItem._DelayTimeBegin);
		return tItem;
	}
};

//row result for an sql query : string Vector
typedef   std::vector<std::string> TSqlRowQuery;

// result for an sql query : Vector of TSqlRowQuery
typedef   std::vector<TSqlRowQuery> TSqlQueryResult;

class CSQLHelper : public StoppableTask
{
public:
	CSQLHelper(void);
	~CSQLHelper(void);

	void SetDatabaseName(const std::string &DBName);

	bool OpenDatabase();
	void CloseDatabase();

	bool BackupDatabase(const std::string &OutputFile);
	bool RestoreDatabase(const std::string &dbase);

	//Returns DeviceRowID
	uint64_t UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, std::string &devname, const bool bUseOnOffAction=true);
	uint64_t UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue, std::string &devname, const bool bUseOnOffAction=true);
	uint64_t UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction=true);
	uint64_t UpdateValueLighting2GroupCmd(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction = true);
	uint64_t UpdateValueHomeConfortGroupCmd(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction = true);

	bool DoesDeviceExist(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType);

	uint64_t InsertDevice(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const int switchType, const int nValue, const char* sValue, const std::string &devname, const unsigned char signallevel = 12, const unsigned char batterylevel = 255, const int used = 0);

	bool GetLastValue(const int HardwareID, const char* DeviceID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &nvalue, std::string &sValue, struct tm &LastUpdateTime);

	void Lighting2GroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd);
	void HomeConfortGroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd);
	void GeneralSwitchGroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd);

	void GetAddjustment(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti);
	void GetAddjustment2(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti);

	void GetMeterType(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &meterType);

	void DeleteDataPoint(const char *ID, const std::string &Date);

	void UpdateRFXCOMHardwareDetails(const int HardwareID, const int msg1, const int msg2, const int msg3, const int msg4, const int msg5, const int msg6);

	void UpdatePreferencesVar(const std::string &Key, const std::string &sValue);
	void UpdatePreferencesVar(const std::string &Key, const int nValue);
	void UpdatePreferencesVar(const std::string &Key, const int nValue, const std::string &sValue);
	bool GetPreferencesVar(const std::string &Key, int &nValue, std::string &sValue);
	bool GetPreferencesVar(const std::string &Key, int &nValue);
	bool GetPreferencesVar(const std::string &Key, std::string &sValue);

	int GetLastBackupNo(const char *Key, int &nValue);
	void SetLastBackupNo(const char *Key, const int nValue);

	bool HasTimers(const uint64_t Idx);
	bool HasTimers(const std::string &Idx);
	bool HasSceneTimers(const uint64_t Idx);
	bool HasSceneTimers(const std::string &Idx);

	void CheckSceneStatus(const uint64_t Idx);
	void CheckSceneStatus(const std::string &Idx);
	void CheckSceneStatusWithDevice(const uint64_t DevIdx);
	void CheckSceneStatusWithDevice(const std::string &DevIdx);

	void ScheduleShortlog();
	void ScheduleDay();

	void ClearShortLog();
	void VacuumDatabase();
	void OptimizeDatabase(sqlite3 *dbase);

	void DeleteHardware(const std::string &idx);

	void DeleteCamera(const std::string &idx);

	void DeletePlan(const std::string &idx);

	void DeleteEvent(const std::string &idx);

	void DeleteDevices(const std::string &idx);

	void TransferDevice(const std::string &oldidx, const std::string &newidx);

	bool DoesSceneByNameExits(const std::string &SceneName);

	void AddTaskItem(const _tTaskItem &tItem, const bool cancelItem = false);

	void EventsGetTaskItems(std::vector<_tTaskItem> &currentTasks);

	void SetUnitsAndScale();

	void CheckDeviceTimeout();
	void CheckBatteryLow();

	bool HandleOnOffAction(const bool bIsOn, const std::string &OnAction, const std::string &OffAction);

	std::vector<std::vector<std::string> > safe_query(const char *fmt, ...);
	std::vector<std::vector<std::string> > safe_queryBlob(const char *fmt, ...);
	void safe_exec_no_return(const char *fmt, ...);
	bool safe_UpdateBlobInTableWithID(const std::string &Table, const std::string &Column, const std::string &sID, const std::string &BlobData);
	bool DoesColumnExistsInTable(const std::string &columnname, const std::string &tablename);

	bool AddUserVariable(const std::string &varname, const _eUsrVariableType eVartype, const std::string &varvalue, std::string &errorMessage);
	bool UpdateUserVariable(const std::string &idx, const std::string &varname, const _eUsrVariableType eVartype, const std::string &varvalue, const bool eventtrigger, std::string &errorMessage);
	void DeleteUserVariable(const std::string &idx);
	bool CheckUserVariable(const _eUsrVariableType eVartype, const std::string &varvalue, std::string &errorMessage);

	uint64_t CreateDevice(const int HardwareID, const int SensorType, const int SensorSubType, std::string &devname, const unsigned long nid, const std::string &soptions);

	void UpdateDeviceValue(const char * FieldName , std::string &Value , std::string &Idx );
	void UpdateDeviceValue(const char * FieldName , int Value , std::string &Idx )   ;
	void UpdateDeviceValue(const char * FieldName , float Value , std::string &Idx ) ;
	std::string GetDeviceValue(const char * FieldName , const char *Idx );

	bool GetPreferencesVar(const std::string &Key, double &Value);
	void UpdatePreferencesVar(const std::string &Key, const double Value);
	void DeletePreferencesVar(const std::string &Key);
	void AllowNewHardwareTimer(const int iTotMinutes);

	bool InsertCustomIconFromZip(const std::string &szZip, std::string &ErrorMessage);
	bool InsertCustomIconFromZipFile(const std::string & szZipFile, std::string & ErrorMessage);

	std::map<std::string, std::string> BuildDeviceOptions(const std::string & options, const bool decode = true);
	std::map<std::string, std::string> GetDeviceOptions(const std::string & idx);
	std::string FormatDeviceOptions(const std::map<std::string, std::string> & optionsMap);
	bool SetDeviceOptions(const uint64_t idx, const std::map<std::string, std::string> & options);

	float GetCounterDivider(const int metertype, const int dType, const float DefaultValue);
public:
	std::string m_LastSwitchID;	//for learning command
	uint64_t m_LastSwitchRowID;
	_eWindUnit	m_windunit;
	std::string	m_windsign;
	float		m_windscale;
	_eTempUnit	m_tempunit;
	_eWeightUnit m_weightunit;
	std::string	m_tempsign;
	std::string	m_weightsign;
	float		m_tempscale;
	float		m_weightscale;
	bool		m_bAcceptNewHardware;
	bool		m_bAllowWidgetOrdering;
	int			m_ActiveTimerPlan;
	bool		m_bEnableEventSystem;
	int			m_ShortLogInterval;
	bool		m_bLogEventScriptTrigger;
	bool		m_bDisableDzVentsSystem;
private:
	std::mutex		m_sqlQueryMutex;
	sqlite3			*m_dbase;
	std::string		m_dbase_name;
	unsigned char	m_sensortimeoutcounter;
	std::map<uint64_t, int> m_timeoutlastsend;
	std::map<uint64_t, int> m_batterylowlastsend;
	bool			m_bAcceptHardwareTimerActive;
	float			m_iAcceptHardwareTimerCounter;
	bool			m_bPreviousAcceptNewHardware;

	std::vector<_tTaskItem> m_background_task_queue;
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_background_task_mutex;
	bool StartThread();
	void StopThread();
	void Do_Work();

	bool SwitchLightFromTasker(const std::string &idx, const std::string &switchcmd, const std::string &level, const std::string &color);
	bool SwitchLightFromTasker(uint64_t idx, const std::string &switchcmd, int level, _tColor color);

	void FixDaylightSavingTableSimple(const std::string &TableName);
	void FixDaylightSaving();

	//Returns DeviceRowID
	uint64_t UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction);

	bool UpdateCalendarMeter(
		const int HardwareID,
		const char* DeviceID,
		const unsigned char unit,
		const unsigned char devType,
		const unsigned char subType,
		const bool shortLog,
		const long long MeterValue,
		const long long MeterUsage,
		const char* date);

	void CheckAndUpdateDeviceOrder();
	void CheckAndUpdateSceneDeviceOrder();

	void CleanupLightSceneLog();

	void UpdateTemperatureLog();
	void UpdateRainLog();
	void UpdateWindLog();
	void UpdateUVLog();
	void UpdateMeter();
	void UpdateMultiMeter();
	void UpdatePercentageLog();
	void UpdateFanLog();
	void AddCalendarTemperature();
	void AddCalendarUpdateRain();
	void AddCalendarUpdateWind();
	void AddCalendarUpdateUV();
	void AddCalendarUpdateMeter();
	void AddCalendarUpdateMultiMeter();
	void AddCalendarUpdatePercentage();
	void AddCalendarUpdateFan();
	void CleanupShortLog();
	bool CheckDate(const std::string &sDate, int &d, int &m, int &y);
	bool CheckDateSQL(const std::string &sDate);
	bool CheckDateTimeSQL(const std::string &sDateTime);
	bool CheckTime(const std::string &sTime);

	std::vector<std::vector<std::string> > query(const std::string &szQuery);
	std::vector<std::vector<std::string> > queryBlob(const std::string &szQuery);
};

extern CSQLHelper m_sql;
