#pragma once

#include <vector>
#include <string>
#include "RFXNames.h"
#include "../httpclient/UrlEncode.h"
#include <map>

struct sqlite3;

class MainWorker;

struct _tNotification
{
	unsigned long long ID;
	std::string Params;
	time_t LastSend;
};

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

enum _eTaskItemType
{
	TITEM_SWITCHCMD=0,
	TITEM_EXECUTE_SCRIPT,
	TITEM_EMAIL_CAMERA_SNAPSHOT,
	TITEM_SEND_EMAIL,
    TITEM_SWITCHCMD_EVENT,
	TITEM_SWITCHCMD_SCENE,
	TITEM_GETURL,
};

struct _tTaskItem
{
	_eTaskItemType _ItemType;
	int _DelayTime;
	int _HardwareID;
	unsigned long long _idx;
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
    unsigned char _level;
	int _Hue;
    std::string _relatedEvent;
    
	_tTaskItem()
	{

	}

	static _tTaskItem SwitchLight(const int DelayTime, const unsigned long long idx, const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const int switchtype, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue)
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
		return tItem;
	}
	static _tTaskItem ExecuteScript(const int DelayTime, const std::string &ScriptPath, const std::string &ScriptParams)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_EXECUTE_SCRIPT;
		tItem._DelayTime=DelayTime;
		tItem._ID=ScriptPath;
		tItem._sValue=ScriptParams;
		return tItem;
	}
	static _tTaskItem EmailCameraSnapshot(const int DelayTime, const std::string &CamIdx, const std::string &Subject)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_EMAIL_CAMERA_SNAPSHOT;
		tItem._DelayTime=DelayTime;
		tItem._ID=CamIdx;
		tItem._sValue=Subject;
		return tItem;
	}
	static _tTaskItem SendEmail(const int DelayTime, const std::string &Subject, const std::string &Body)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SEND_EMAIL;
		tItem._DelayTime=DelayTime;
		tItem._ID=Subject;
		tItem._sValue=Body;
		return tItem;
	}
    static _tTaskItem SwitchLightEvent(const int DelayTime, const unsigned long long idx, const std::string &Command, const unsigned char Level, const int Hue, const std::string &eventName)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SWITCHCMD_EVENT;
		tItem._DelayTime=DelayTime;
		tItem._idx=idx;
        tItem._command= Command;
        tItem._level= Level;
		tItem._Hue=Hue;
        tItem._relatedEvent = eventName;

		return tItem;
	}
    static _tTaskItem SwitchSceneEvent(const int DelayTime, const unsigned long long idx, const std::string &Command, const std::string &eventName)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_SWITCHCMD_SCENE;
		tItem._DelayTime=DelayTime;
		tItem._idx=idx;
        tItem._command= Command;
        tItem._relatedEvent = eventName;
        
		return tItem;
	}
	static _tTaskItem GetHTTPPage(const int DelayTime, const std::string &URL, const std::string &eventName)
	{
		_tTaskItem tItem;
		tItem._ItemType=TITEM_GETURL;
		tItem._DelayTime=DelayTime;
		tItem._sValue= URL;
		tItem._relatedEvent = eventName;

		return tItem;
	}
};

class CSQLHelper
{
public:
	CSQLHelper(void);
	~CSQLHelper(void);

	void SetMainWorker(MainWorker *pWorker);
	bool OpenDatabase();
	void SetDatabaseName(const std::string &DBName);

	bool BackupDatabase(const std::string &OutputFile);

	//Returns DeviceRowID
	unsigned long long UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, std::string &devname);
	unsigned long long UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue, std::string &devname);
	unsigned long long UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname);

	
	bool GetLastValue(const int HardwareID, const char* DeviceID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &nvalue, std::string &sValue, struct tm &LastUpdateTime);
	
	void Lighting2GroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd);

	bool NeedToUpdateHardwareDevice(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname);

	void GetAddjustment(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti);
	void GetAddjustment2(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti);

	void GetMeterType(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &meterType);

	void DeleteDataPoint(const char *ID, const char *Date);

	void UpdateRFXCOMHardwareDetails(const int HardwareID, const int msg1, const int msg2, const int msg3, const int msg4, const int msg5);

	void UpdateTempVar(const char *Key, const char* sValue);
	bool GetTempVar(const char *Key, int &nValue, std::string &sValue);

	void UpdatePreferencesVar(const char *Key, const char* sValue);
	void UpdatePreferencesVar(const char *Key, const int nValue);
	void UpdatePreferencesVar(const char *Key, const int nValue, const char* sValue);
	bool GetPreferencesVar(const char *Key, int &nValue, std::string &sValue);
	bool GetPreferencesVar(const char *Key, int &nValue);
	bool GetPreferencesVar(const char *Key, std::string &sValue);

	//notification functions
	bool AddNotification(const std::string &DevIdx, const std::string &Param);
	bool RemoveDeviceNotifications(const std::string &DevIdx);
	bool RemoveNotification(const std::string &ID);
	std::vector<_tNotification> GetNotifications(const unsigned long long DevIdx);
	std::vector<_tNotification> GetNotifications(const std::string &DevIdx);
	void TouchNotification(const unsigned long long ID);
	bool HasNotifications(const unsigned long long DevIdx);
	bool HasNotifications(const std::string &DevIdx);

	bool CheckAndHandleTempHumidityNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const float temp, 
		const int humidity, 
		const bool bHaveTemp, 
		const bool bHaveHumidity);
	bool CheckAndHandleDewPointNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const float temp,
		const float dewpoint);
	bool CheckAndHandleNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleSwitchNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype);
	bool CheckAndHandleRainNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleTotalNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleUsageNotification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const _eNotificationTypes ntype, 
		const float mvalue);
	bool CheckAndHandleAmpere123Notification(
		const int HardwareID, 
		const std::string &ID, 
		const unsigned char unit, 
		const unsigned char devType, 
		const unsigned char subType, 
		const float Ampere1,
		const float Ampere2,
		const float Ampere3
		);

	bool HasTimers(const unsigned long long Idx);
	bool HasTimers(const std::string &Idx);
	bool HasSceneTimers(const unsigned long long Idx);
	bool HasSceneTimers(const std::string &Idx);

	void CheckSceneStatus(const unsigned long long Idx);
	void CheckSceneStatus(const std::string &Idx);
	void CheckSceneStatusWithDevice(const unsigned long long DevIdx);
	void CheckSceneStatusWithDevice(const std::string &DevIdx);

	bool SendNotification(const std::string &EventID, const std::string &Message);
	bool SendNotificationEx(const std::string &Subject, const std::string &Body);

	void Schedule5Minute();
	void ScheduleDay();

	void DeleteHardware(const std::string &idx);
    
    void DeleteCamera(const std::string &idx);

    void DeletePlan(const std::string &idx);

    void DeleteEvent(const std::string &idx);
    
	void DeleteDevice(const std::string &idx);

	void TransferDevice(const std::string &oldidx, const std::string &newidx);

	bool DoesSceneByNameExits(const std::string &SceneName);

	void AddTaskItem(const _tTaskItem &tItem);
    
    void EventsGetTaskItems(std::vector<_tTaskItem> &currentTasks);
   
	void SetUnitsAndScale();

	void CheckDeviceTimeout();

	std::vector<std::vector<std::string> > query(const std::string &szQuery);
public:
	std::string m_LastSwitchID;	//for learning command
	unsigned long long m_LastSwitchRowID;
	_eWindUnit	m_windunit;
	std::string	m_windsign;
	float		m_windscale;
	_eTempUnit	m_tempunit;
	std::string	m_tempsign;
	float		m_tempscale;
private:
	boost::mutex m_sqlQueryMutex;
	CURLEncode m_urlencoder;
	sqlite3 *m_dbase;
	sqlite3 *m_demo_dbase;
	MainWorker *m_pMain;
	std::string m_dbase_name;
	unsigned char m_sensortimeoutcounter;
	std::map<unsigned long long,int> m_timeoutlastsend;

	std::vector<_tTaskItem> m_background_task_queue;
	boost::shared_ptr<boost::thread> m_background_task_thread;
	boost::mutex m_background_task_mutex;
	bool m_stoprequested;
	bool StartThread();
	void Do_Work();

	//Returns DeviceRowID
	unsigned long long UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname);

	void CheckAndUpdateDeviceOrder();
	void CheckAndUpdateSceneDeviceOrder();

	void CleanupLightLog();

	void UpdateTemperatureLog();
	void UpdateRainLog();
	void UpdateWindLog();
	void UpdateUVLog();
	void UpdateMeter();
	void UpdateMultiMeter();
	void AddCalendarTemperature();
	void AddCalendarUpdateRain();
	void AddCalendarUpdateWind();
	void AddCalendarUpdateUV();
	void AddCalendarUpdateMeter();
	void AddCalendarUpdateMultiMeter();
	void CleanupShortLog();
};

