#include "stdafx.h"
#include "SQLHelper.h"
#include <iostream>     /* standard I/O functions                         */
#include "RFXtrx.h"
#include "Helper.h"
#include "RFXNames.h"
#include "localtime_r.h"
#include "Logger.h"
#include "mainworker.h"
#include "../sqlite/sqlite3.h"
#include "../hardware/hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../smtpclient/SMTPClient.h"
#include "WebServer.h"
#include "../webserver/Base64.h"
#include "unzip.h"
#include "mainstructs.h"
#include <boost/lexical_cast.hpp>
#include "../notifications/NotificationHelper.h"

#ifndef WIN32
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
#else
	#include "../msbuild/WindowsHelper.h"
#endif

#define DB_VERSION 67

extern http::server::CWebServer m_webserver;
extern std::string szWWWFolder;

const char *sqlCreateDeviceStatus =
"CREATE TABLE IF NOT EXISTS [DeviceStatus] ("
"[ID] INTEGER PRIMARY KEY, "
"[HardwareID] INTEGER NOT NULL, "
"[DeviceID] VARCHAR(25) NOT NULL, "
"[Unit] INTEGER DEFAULT 0, "
"[Name] VARCHAR(100) DEFAULT Unknown, "
"[Used] INTEGER DEFAULT 0, "
"[Type] INTEGER NOT NULL, "
"[SubType] INTEGER NOT NULL, "
"[SwitchType] INTEGER DEFAULT 0, "
"[Favorite] INTEGER DEFAULT 0, "
"[SignalLevel] INTEGER DEFAULT 0, "
"[BatteryLevel] INTEGER DEFAULT 0, "
"[nValue] INTEGER DEFAULT 0, "
"[sValue] VARCHAR(200) DEFAULT null, "
"[LastUpdate] DATETIME DEFAULT (datetime('now','localtime')),"
"[Order] INTEGER BIGINT(10) default 0, "
"[AddjValue] FLOAT DEFAULT 0, "
"[AddjMulti] FLOAT DEFAULT 1, "
"[AddjValue2] FLOAT DEFAULT 0, "
"[AddjMulti2] FLOAT DEFAULT 1, "
"[StrParam1] VARCHAR(200) DEFAULT '', "
"[StrParam2] VARCHAR(200) DEFAULT '', "
"[LastLevel] INTEGER DEFAULT 0, "
"[Protected] INTEGER DEFAULT 0, "
"[CustomImage] INTEGER DEFAULT 0);";

const char *sqlCreateDeviceStatusTrigger =
"CREATE TRIGGER IF NOT EXISTS devicestatusupdate AFTER INSERT ON DeviceStatus\n"
"BEGIN\n"
"	UPDATE DeviceStatus SET [Order] = (SELECT MAX([Order]) FROM DeviceStatus)+1 WHERE DeviceStatus.ID = NEW.ID;\n"
"END;\n";

const char *sqlCreateEventActions =
"CREATE TABLE IF NOT EXISTS [EventActions] ("
"[ID] INTEGER PRIMARY KEY, "
"[ConditionID] INTEGER NOT NULL, "
"[ActionType] INTEGER NOT NULL, "
"[DeviceRowID] INTEGER DEFAULT 0, "
"[Param1] VARCHAR(120), "
"[Param2] VARCHAR(120), "
"[Param3] VARCHAR(120), "
"[Param4] VARCHAR(120), "
"[Param5] VARCHAR(120), "
"[Order] INTEGER BIGINT(10) default 0);"; 

const char *sqlCreateEventActionsTrigger =
"CREATE TRIGGER IF NOT EXISTS eventactionsstatusupdate AFTER INSERT ON EventActions\n"
"BEGIN\n"
"  UPDATE EventActions SET [Order] = (SELECT MAX([Order]) FROM EventActions)+1 WHERE EventActions.ID = NEW.ID;\n"
"END;\n";

const char *sqlCreateLightingLog =
"CREATE TABLE IF NOT EXISTS [LightingLog] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[nValue] INTEGER DEFAULT 0, "
"[sValue] VARCHAR(200), "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreatePreferences =
"CREATE TABLE IF NOT EXISTS [Preferences] ("
"[Key] VARCHAR(50) NOT NULL, "
"[nValue] INTEGER DEFAULT 0, "
"[sValue] VARCHAR(200));";

const char *sqlCreateRain =
"CREATE TABLE IF NOT EXISTS [Rain] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Total] FLOAT NOT NULL, "
"[Rate] INTEGER DEFAULT 0, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateRain_Calendar =
"CREATE TABLE IF NOT EXISTS [Rain_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Total] FLOAT NOT NULL, "
"[Rate] INTEGER DEFAULT 0, "
"[Date] DATE NOT NULL);";

const char *sqlCreateTemperature =
"CREATE TABLE IF NOT EXISTS [Temperature] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Temperature] FLOAT NOT NULL, "
"[Chill] FLOAT DEFAULT 0, "
"[Humidity] INTEGER DEFAULT 0, "
"[Barometer] INTEGER DEFAULT 0, "
"[DewPoint] FLOAT DEFAULT 0, "
"[SetPoint] FLOAT DEFAULT 0, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateTemperature_Calendar =
"CREATE TABLE IF NOT EXISTS [Temperature_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Temp_Min] FLOAT NOT NULL, "
"[Temp_Max] FLOAT NOT NULL, "
"[Temp_Avg] FLOAT DEFAULT 0, "
"[Chill_Min] FLOAT DEFAULT 0, "
"[Chill_Max] FLOAT, "
"[Humidity] INTEGER DEFAULT 0, "
"[Barometer] INTEGER DEFAULT 0, "
"[DewPoint] FLOAT DEFAULT 0, "
"[SetPoint_Min] FLOAT DEFAULT 0, "
"[SetPoint_Max] FLOAT DEFAULT 0, "
"[SetPoint_Avg] FLOAT DEFAULT 0, "
"[Date] DATE NOT NULL);";

const char *sqlCreateTimers =
"CREATE TABLE IF NOT EXISTS [Timers] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] BOOLEAN DEFAULT true, "
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Date] DATE DEFAULT 0, "
"[Time] TIME NOT NULL, "
"[Type] INTEGER NOT NULL, "
"[Cmd] INTEGER NOT NULL, "
"[Level] INTEGER DEFAULT 15, "
"[Hue] INTEGER DEFAULT 0, "
"[UseRandomness] INTEGER DEFAULT 0, "
"[TimerPlan] INTEGER DEFAULT 0, "
"[Days] INTEGER NOT NULL);";

const char *sqlCreateUV =
"CREATE TABLE IF NOT EXISTS [UV] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Level] FLOAT NOT NULL, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateUV_Calendar =
"CREATE TABLE IF NOT EXISTS [UV_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Level] FLOAT, "
"[Date] DATE NOT NULL);";

const char *sqlCreateWind =
"CREATE TABLE IF NOT EXISTS [Wind] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Direction] FLOAT NOT NULL, "
"[Speed] INTEGER NOT NULL, "
"[Gust] INTEGER NOT NULL, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateWind_Calendar =
"CREATE TABLE IF NOT EXISTS [Wind_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Direction] FLOAT NOT NULL, "
"[Speed_Min] INTEGER NOT NULL, "
"[Speed_Max] INTEGER NOT NULL, "
"[Gust_Min] INTEGER NOT NULL, "
"[Gust_Max] INTEGER NOT NULL, "
"[Date] DATE NOT NULL);";

const char *sqlCreateMultiMeter =
"CREATE TABLE IF NOT EXISTS [MultiMeter] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Value1] BIGINT NOT NULL, "
"[Value2] BIGINT DEFAULT 0, "
"[Value3] BIGINT DEFAULT 0, "
"[Value4] BIGINT DEFAULT 0, "
"[Value5] BIGINT DEFAULT 0, "
"[Value6] BIGINT DEFAULT 0, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateMultiMeter_Calendar =
"CREATE TABLE IF NOT EXISTS [MultiMeter_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Value1] BIGINT NOT NULL, "
"[Value2] BIGINT NOT NULL, "
"[Value3] BIGINT NOT NULL, "
"[Value4] BIGINT NOT NULL, "
"[Value5] BIGINT NOT NULL, "
"[Value6] BIGINT NOT NULL, "
"[Counter1] BIGINT DEFAULT 0, "
"[Counter2] BIGINT DEFAULT 0, "
"[Counter3] BIGINT DEFAULT 0, "
"[Counter4] BIGINT DEFAULT 0, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";


const char *sqlCreateNotifications =
"CREATE TABLE IF NOT EXISTS [Notifications] ("
"[ID] INTEGER PRIMARY KEY, "
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Params] VARCHAR(100), "
"[Priority] INTEGER default 0, "
"[LastSend] DATETIME DEFAULT 0);";

const char *sqlCreateHardware =
"CREATE TABLE IF NOT EXISTS [Hardware] ("
"[ID] INTEGER PRIMARY KEY, "
"[Name] VARCHAR(200) NOT NULL, "
"[Enabled] INTEGER DEFAULT 1, "
"[Type] INTEGER NOT NULL, "
"[Address] VARCHAR(200), "
"[Port] INTEGER, "
"[Username] VARCHAR(100), "
"[Password] VARCHAR(100), "
"[Mode1] CHAR DEFAULT 0, "
"[Mode2] CHAR DEFAULT 0, "
"[Mode3] CHAR DEFAULT 0, "
"[Mode4] CHAR DEFAULT 0, "
"[Mode5] CHAR DEFAULT 0, "
"[Mode6] CHAR DEFAULT 0, "
"[DataTimeout] INTEGER DEFAULT 0);";

const char *sqlCreateUsers =
"CREATE TABLE IF NOT EXISTS [Users] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] INTEGER NOT NULL DEFAULT 0, "
"[Username] VARCHAR(200) NOT NULL, "
"[Password] VARCHAR(200) NOT NULL, "
"[Rights] INTEGER DEFAULT 255, "
"[TabsEnabled] INTEGER DEFAULT 255, "
"[RemoteSharing] INTEGER DEFAULT 0);";

const char *sqlCreateMeter =
"CREATE TABLE IF NOT EXISTS [Meter] ("
"[DeviceRowID] BIGINT NOT NULL, "
"[Value] BIGINT NOT NULL, "
"[Usage] INTEGER DEFAULT 0, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateMeter_Calendar =
"CREATE TABLE IF NOT EXISTS [Meter_Calendar] ("
"[DeviceRowID] BIGINT NOT NULL, "
"[Value] BIGINT NOT NULL, "
"[Counter] BIGINT DEFAULT 0, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateLightSubDevices =
"CREATE TABLE IF NOT EXISTS [LightSubDevices] ("
"[ID] INTEGER PRIMARY KEY, "
"[DeviceRowID] INTEGER NOT NULL, "
"[ParentID] INTEGER NOT NULL);";

const char *sqlCreateCameras =
"CREATE TABLE IF NOT EXISTS [Cameras] ("
"[ID] INTEGER PRIMARY KEY, "
"[Name] VARCHAR(200) NOT NULL, "
"[Enabled] INTEGER DEFAULT 1, "
"[Address] VARCHAR(200), "
"[Port] INTEGER, "
"[Username] VARCHAR(100) DEFAULT (''), "
"[Password] VARCHAR(100) DEFAULT (''), "
"[ImageURL] VARCHAR(200) DEFAULT (''));";

const char *sqlCreateCamerasActiveDevices =
"CREATE TABLE IF NOT EXISTS [CamerasActiveDevices] ("
"[ID] INTEGER PRIMARY KEY, "
"[CameraRowID] INTEGER NOT NULL, "
"[DevSceneType] INTEGER NOT NULL, "
"[DevSceneRowID] INTEGER NOT NULL, "
"[DevSceneWhen] INTEGER NOT NULL, "
"[DevSceneDelay] INTEGER NOT NULL);";

const char *sqlCreatePlanMappings =
"CREATE TABLE IF NOT EXISTS [DeviceToPlansMap] ("
"[ID] INTEGER PRIMARY KEY, "
"[DeviceRowID] BIGINT NOT NULL, "
"[DevSceneType] INTEGER DEFAULT 0, "
"[PlanID] BIGINT NOT NULL, "
"[Order] INTEGER BIGINT(10) DEFAULT 0, "
"[XOffset] INTEGER default 0, "
"[YOffset] INTEGER default 0);";

const char *sqlCreateDevicesToPlanStatusTrigger =
	"CREATE TRIGGER IF NOT EXISTS deviceplantatusupdate AFTER INSERT ON DeviceToPlansMap\n"
	"BEGIN\n"
	"	UPDATE DeviceToPlansMap SET [Order] = (SELECT MAX([Order]) FROM DeviceToPlansMap)+1 WHERE DeviceToPlansMap.ID = NEW.ID;\n"
	"END;\n";

const char *sqlCreatePlans =
"CREATE TABLE IF NOT EXISTS [Plans] ("
"[ID] INTEGER PRIMARY KEY, "
"[Order] INTEGER BIGINT(10) default 0, "
"[Name] VARCHAR(200) NOT NULL, "
"[FloorplanID] INTEGER default 0, "
"[Area] VARCHAR(200) DEFAULT '');";

const char *sqlCreatePlanOrderTrigger =
	"CREATE TRIGGER IF NOT EXISTS planordertrigger AFTER INSERT ON Plans\n"
	"BEGIN\n"
	"	UPDATE Plans SET [Order] = (SELECT MAX([Order]) FROM Plans)+1 WHERE Plans.ID = NEW.ID;\n"
	"END;\n";

const char *sqlCreateScenes =
"CREATE TABLE IF NOT EXISTS [Scenes] (\n"
"[ID] INTEGER PRIMARY KEY, \n"
"[Name] VARCHAR(100) NOT NULL, \n"
"[HardwareID] INTEGER DEFAULT 0, \n"
"[DeviceID] VARCHAR(25), \n"
"[Unit] INTEGER DEFAULT 0, \n"
"[Type] INTEGER DEFAULT 0, \n"
"[SubType] INTEGER DEFAULT 0, \n"
"[Favorite] INTEGER DEFAULT 0, \n"
"[Order] INTEGER BIGINT(10) default 0, \n"
"[nValue] INTEGER DEFAULT 0, \n"
"[SceneType] INTEGER DEFAULT 0, \n"
"[ListenCmd] INTEGER DEFAULT 1, \n"
"[Protected] INTEGER DEFAULT 0, \n"
"[OnAction] VARCHAR(200) DEFAULT '', "
"[OffAction] VARCHAR(200) DEFAULT '', "
"[LastUpdate] DATETIME DEFAULT (datetime('now','localtime')));\n";

const char *sqlCreateScenesTrigger =
"CREATE TRIGGER IF NOT EXISTS scenesupdate AFTER INSERT ON Scenes\n"
"BEGIN\n"
"	UPDATE Scenes SET [Order] = (SELECT MAX([Order]) FROM Scenes)+1 WHERE Scenes.ID = NEW.ID;\n"
"END;\n";

const char *sqlCreateSceneDevices =
"CREATE TABLE IF NOT EXISTS [SceneDevices] ("
"[ID] INTEGER PRIMARY KEY, "
"[Order] INTEGER BIGINT(10) default 0, "
"[SceneRowID] BIGINT NOT NULL, "
"[DeviceRowID] BIGINT NOT NULL, "
"[Cmd] INTEGER DEFAULT 1, "
"[Level] INTEGER DEFAULT 100, "
"[Hue] INTEGER DEFAULT 0, "
"[OnDelay] INTEGER DEFAULT 0, "
"[OffDelay] INTEGER DEFAULT 0);";

const char *sqlCreateSceneDeviceTrigger =
	"CREATE TRIGGER IF NOT EXISTS scenedevicesupdate AFTER INSERT ON SceneDevices\n"
	"BEGIN\n"
	"	UPDATE SceneDevices SET [Order] = (SELECT MAX([Order]) FROM SceneDevices)+1 WHERE SceneDevices.ID = NEW.ID;\n"
	"END;\n";

const char *sqlCreateSceneTimers =
"CREATE TABLE IF NOT EXISTS [SceneTimers] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] BOOLEAN DEFAULT true, "
"[SceneRowID] BIGINT(10) NOT NULL, "
"[Date] DATE DEFAULT 0, "
"[Time] TIME NOT NULL, "
"[Type] INTEGER NOT NULL, "
"[Cmd] INTEGER NOT NULL, "
"[Level] INTEGER DEFAULT 15, "
"[Hue] INTEGER DEFAULT 0, "
"[UseRandomness] INTEGER DEFAULT 0, "
"[TimerPlan] INTEGER DEFAULT 0, "
"[Days] INTEGER NOT NULL);";

const char *sqlCreateSetpointTimers =
"CREATE TABLE IF NOT EXISTS [SetpointTimers] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] BOOLEAN DEFAULT true, "
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Date] DATE DEFAULT 0, "
"[Time] TIME NOT NULL, "
"[Type] INTEGER NOT NULL, "
"[Temperature] FLOAT DEFAULT 0, "
"[TimerPlan] INTEGER DEFAULT 0, "
"[Days] INTEGER NOT NULL);";

const char *sqlCreateSharedDevices =
"CREATE TABLE IF NOT EXISTS [SharedDevices] ("
"[ID] INTEGER PRIMARY KEY,  "
"[SharedUserID] BIGINT NOT NULL, "
"[DeviceRowID] BIGINT NOT NULL);";

const char *sqlCreateEventMaster =
"CREATE TABLE IF NOT EXISTS [EventMaster] ("
"[ID] INTEGER PRIMARY KEY,  "
"[Name] VARCHAR(200) NOT NULL, "
"[XMLStatement] TEXT NOT NULL, "
"[Status] INTEGER DEFAULT 0);";

const char *sqlCreateEventRules =
"CREATE TABLE IF NOT EXISTS [EventRules] ("
"[ID] INTEGER PRIMARY KEY, "
"[EMID] INTEGER, "
"[Conditions] TEXT NOT NULL, "
"[Actions] TEXT NOT NULL, "
"[SequenceNo] INTEGER NOT NULL, "
"FOREIGN KEY (EMID) REFERENCES EventMaster(ID));";

const char *sqlCreateZWaveNodes =
	"CREATE TABLE IF NOT EXISTS [ZWaveNodes] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[HardwareID] INTEGER NOT NULL, "
	"[HomeID] INTEGER NOT NULL, "
	"[NodeID] INTEGER NOT NULL, "
	"[Name] VARCHAR(100) DEFAULT Unknown, "
	"[ProductDescription] VARCHAR(100) DEFAULT Unknown, "
	"[PollTime] INTEGER DEFAULT 0);";

const char *sqlCreateWOLNodes =
	"CREATE TABLE IF NOT EXISTS [WOLNodes] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[HardwareID] INTEGER NOT NULL, "
	"[Name] VARCHAR(100) DEFAULT Unknown, "
	"[MacAddress] VARCHAR(50) DEFAULT Unknown);";

const char *sqlCreatePercentage =
"CREATE TABLE IF NOT EXISTS [Percentage] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Percentage] FLOAT NOT NULL, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreatePercentage_Calendar =
"CREATE TABLE IF NOT EXISTS [Percentage_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Percentage_Min] FLOAT NOT NULL, "
"[Percentage_Max] FLOAT NOT NULL, "
"[Percentage_Avg] FLOAT DEFAULT 0, "
"[Date] DATE NOT NULL);";

const char *sqlCreateFan =
"CREATE TABLE IF NOT EXISTS [Fan] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Speed] INTEGER NOT NULL, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateFan_Calendar =
"CREATE TABLE IF NOT EXISTS [Fan_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Speed_Min] INTEGER NOT NULL, "
"[Speed_Max] INTEGER NOT NULL, "
"[Speed_Avg] INTEGER DEFAULT 0, "
"[Date] DATE NOT NULL);";

const char *sqlCreateBackupLog =
"CREATE TABLE IF NOT EXISTS [BackupLog] ("
"[Key] VARCHAR(50) NOT NULL, "
"[nValue] INTEGER DEFAULT 0); ";

const char *sqlCreateEnoceanSensors =
	"CREATE TABLE IF NOT EXISTS [EnoceanSensors] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[HardwareID] INTEGER NOT NULL, "
	"[DeviceID] VARCHAR(25) NOT NULL, "
	"[Manufacturer] INTEGER NOT NULL, "
	"[Profile] INTEGER NOT NULL, "
	"[Type] INTEGER NOT NULL);";

const char *sqlCreateFibaroLink =
	"CREATE TABLE IF NOT EXISTS [FibaroLink] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[DeviceID]  BIGINT NOT NULL, "
	"[DelimitedValue] INTEGER DEFAULT 0, "
	"[TargetType] INTEGER DEFAULT 0, "
	"[TargetVariable] VARCHAR(100), "
	"[TargetDeviceID] INTEGER, "
	"[TargetProperty] VARCHAR(100), "
	"[Enabled] INTEGER DEFAULT 1, "
	"[IncludeUnit] INTEGER default 0); ";

const char *sqlCreateUserVariables =
	"CREATE TABLE IF NOT EXISTS [UserVariables] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[Name] VARCHAR(200), "
	"[ValueType] INT NOT NULL, "
	"[Value] VARCHAR(200), "
	"[LastUpdate] DATETIME DEFAULT(datetime('now', 'localtime')));";

const char *sqlCreateFloorplans =
	"CREATE TABLE IF NOT EXISTS [Floorplans] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[Name] VARCHAR(200) NOT NULL, "
	"[ImageFile] VARCHAR(100) NOT NULL, "
	"[ScaleFactor] FLOAT DEFAULT 1.0, "
	"[Order] INTEGER BIGINT(10) default 0);";

const char *sqlCreateFloorplanOrderTrigger =
	"CREATE TRIGGER IF NOT EXISTS floorplanordertrigger AFTER INSERT ON Floorplans\n"
	"BEGIN\n"
	"	UPDATE Floorplans SET [Order] = (SELECT MAX([Order]) FROM Floorplans)+1 WHERE Floorplans.ID = NEW.ID;\n"
	"END;\n";

const char *sqlCreateCustomImages =
	"CREATE TABLE IF NOT EXISTS [CustomImages]("
	"	[ID] INTEGER PRIMARY KEY, "
	"	[Base] VARCHAR(80) NOT NULL, "
	"	[Name] VARCHAR(80) NOT NULL, "
	"	[Description] VARCHAR(80) NOT NULL, "
	"	[IconSmall] BLOB, "
	"	[IconOn] BLOB, "
	"	[IconOff] BLOB);";

const char *sqlCreateMySensors =
	"CREATE TABLE IF NOT EXISTS [MySensors]("
	" [HardwareID] INTEGER NOT NULL,"
	" [ID] INTEGER NOT NULL,"
	" [SketchName] VARCHAR(100) DEFAULT Unknown,"
	" [SketchVersion] VARCHAR(40) DEFAULT(1.0));";

const char *sqlCreateMySensorsVariables =
"CREATE TABLE IF NOT EXISTS [MySensorsVars]("
" [HardwareID] INTEGER NOT NULL,"
" [NodeID] INTEGER NOT NULL,"
" [ChildID] INTEGER NOT NULL,"
" [VarID] INTEGER NOT NULL,"
" [Value] VARCHAR(100) NOT NULL);";

const char *sqlCreateToonDevices =
"CREATE TABLE IF NOT EXISTS [ToonDevices]("
" [HardwareID] INTEGER NOT NULL,"
" [UUID] VARCHAR(100) NOT NULL);";

extern std::string szStartupFolder;

CSQLHelper::CSQLHelper(void)
{
	m_LastSwitchID="";
	m_LastSwitchRowID=0;
	m_dbase=NULL;
	m_stoprequested=false;
	m_sensortimeoutcounter=0;
	m_bAcceptNewHardware=true;
	m_bAllowWidgetOrdering=true;
	m_ActiveTimerPlan=0;
	m_windunit=WINDUNIT_MS;
	m_tempunit=TEMPUNIT_C;
	SetUnitsAndScale();
	m_bAcceptHardwareTimerActive=false;
	m_iAcceptHardwareTimerCounter=0;

	SetDatabaseName("domoticz.db");
}

CSQLHelper::~CSQLHelper(void)
{
	if (m_background_task_thread)
	{
		m_stoprequested = true;
		m_background_task_thread->join();
	}
	if (m_dbase!=NULL)
	{
		sqlite3_close(m_dbase);
		m_dbase=NULL;
	}
}

bool CSQLHelper::OpenDatabase()
{
	//Open Database
	int rc = sqlite3_open(m_dbase_name.c_str(), &m_dbase);
	if (rc)
	{
		_log.Log(LOG_ERROR,"Error opening SQLite3 database: %s", sqlite3_errmsg(m_dbase));
		sqlite3_close(m_dbase);
		return false;
	}
#ifndef WIN32
	//test, this could improve performance
	rc=sqlite3_exec(m_dbase, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);
	rc=sqlite3_exec(m_dbase, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
#else
	rc=sqlite3_exec(m_dbase, "PRAGMA journal_mode=DELETE", NULL, NULL, NULL);
#endif
    rc=sqlite3_exec(m_dbase, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
	bool bNewInstall=false;
	std::vector<std::vector<std::string> > result=query("SELECT name FROM sqlite_master WHERE type='table' AND name='DeviceStatus'");
	bNewInstall=(result.size()==0);
	int dbversion=0;
	if (!bNewInstall)
	{
		GetPreferencesVar("DB_Version", dbversion);
		//Pre-SQL Patches
	}

	//create database (if not exists)
	query(sqlCreateDeviceStatus);
	query(sqlCreateDeviceStatusTrigger);
	query(sqlCreateEventActions);
	query(sqlCreateEventActionsTrigger);
	query(sqlCreateLightingLog);
	query(sqlCreatePreferences);
	query(sqlCreateRain);
	query(sqlCreateRain_Calendar);
	query(sqlCreateTemperature);
	query(sqlCreateTemperature_Calendar);
	query(sqlCreateTimers);
	query(sqlCreateSetpointTimers);
	query(sqlCreateUV);
	query(sqlCreateUV_Calendar);
	query(sqlCreateWind);
	query(sqlCreateWind_Calendar);
	query(sqlCreateMeter);
	query(sqlCreateMeter_Calendar);
	query(sqlCreateMultiMeter);
	query(sqlCreateMultiMeter_Calendar);
	query(sqlCreateNotifications);
	query(sqlCreateHardware);
	query(sqlCreateUsers);
	query(sqlCreateLightSubDevices);
    query(sqlCreateCameras);
	query(sqlCreateCamerasActiveDevices);
    query(sqlCreatePlanMappings);
	query(sqlCreateDevicesToPlanStatusTrigger);
    query(sqlCreatePlans);
	query(sqlCreatePlanOrderTrigger);
	query(sqlCreateScenes);
	query(sqlCreateScenesTrigger);
	query(sqlCreateSceneDevices);
	query(sqlCreateSceneDeviceTrigger);
	query(sqlCreateSceneTimers);
	query(sqlCreateSharedDevices);
    query(sqlCreateEventMaster);
    query(sqlCreateEventRules);
	query(sqlCreateZWaveNodes);
	query(sqlCreateWOLNodes);
	query(sqlCreatePercentage);
	query(sqlCreatePercentage_Calendar);
	query(sqlCreateFan);
	query(sqlCreateFan_Calendar);
	query(sqlCreateBackupLog);
	query(sqlCreateEnoceanSensors);
	query(sqlCreateFibaroLink);
	query(sqlCreateUserVariables);
	query(sqlCreateFloorplans);
	query(sqlCreateFloorplanOrderTrigger);
	query(sqlCreateCustomImages);
	query(sqlCreateMySensors);
	query(sqlCreateMySensorsVariables);
	query(sqlCreateToonDevices);

	if ((!bNewInstall) && (dbversion < DB_VERSION))
	{
		//Post-SQL Patches
		if (dbversion < 2)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [Order] INTEGER BIGINT(10) default 0");
			query(sqlCreateDeviceStatusTrigger);
			CheckAndUpdateDeviceOrder();
		}
		if (dbversion < 3)
		{
			query("ALTER TABLE Hardware ADD COLUMN [Enabled] INTEGER default 1");
		}
		if (dbversion < 4)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjValue] FLOAT default 0");
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjMulti] FLOAT default 1");
		}
		if (dbversion < 5)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [Cmd] INTEGER default 1");
			query("ALTER TABLE SceneDevices ADD COLUMN [Level] INTEGER default 100");
		}
		if (dbversion < 6)
		{
			query("ALTER TABLE Cameras ADD COLUMN [ImageURL] VARCHAR(100)");
		}
		if (dbversion < 7)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjValue2] FLOAT default 0");
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjMulti2] FLOAT default 1");
		}
		if (dbversion < 8)
		{
			query("DROP TABLE IF EXISTS [Cameras]");
			query(sqlCreateCameras);
		}
		if (dbversion < 9) {
			query("UPDATE Notifications SET Params = 'S' WHERE Params = ''");
		}
		if (dbversion < 10)
		{
			//P1 Smart meter power change, need to delete all short logs from today
			char szDateStart[40];
			time_t now = mytime(NULL);
			struct tm tm1;
			localtime_r(&now, &tm1);
			struct tm ltime;
			ltime.tm_isdst = tm1.tm_isdst;
			ltime.tm_hour = 0;
			ltime.tm_min = 0;
			ltime.tm_sec = 0;
			ltime.tm_year = tm1.tm_year;
			ltime.tm_mon = tm1.tm_mon;
			ltime.tm_mday = tm1.tm_mday;

			sprintf(szDateStart, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

			char szTmp[200];

			std::vector<std::vector<std::string> > result;
			sprintf(szTmp, "SELECT ID FROM DeviceStatus WHERE (Type=%d)", pTypeP1Power);
			result = query(szTmp);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					sprintf(szTmp, "DELETE FROM MultiMeter WHERE (DeviceRowID='%s') AND (Date>='%s')", idx.c_str(), szDateStart);
					query(szTmp);
				}
			}
		}
		if (dbversion < 11)
		{
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;

			szQuery << "SELECT ID, Username, Password FROM Cameras ORDER BY ID";
			result = query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string camuser = base64_encode((const unsigned char*)sd[1].c_str(), sd[1].size());
					std::string campwd = base64_encode((const unsigned char*)sd[2].c_str(), sd[2].size());
					std::stringstream szQuery2;
					szQuery2 << "UPDATE Cameras SET Username='" << camuser << "', Password='" << campwd << "' WHERE (ID=='" << sd[0] << "')";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 12)
		{
			std::vector<std::vector<std::string> > result;
			result = query("SELECT t.RowID, u.RowID from MultiMeter_Calendar as t, MultiMeter_Calendar as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID])");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					++itt;
					std::vector<std::string> sd = *itt;
					std::stringstream szQuery2;
					szQuery2 << "DELETE FROM MultiMeter_Calendar WHERE (RowID=='" << sd[0] << "')";
					query(szQuery2.str());
				}
			}

		}
		if (dbversion < 13)
		{
			DeleteHardware("1001");
		}
		if (dbversion < 14)
		{
			query("ALTER TABLE Users ADD COLUMN [RemoteSharing] INTEGER default 0");
		}
		if (dbversion < 15)
		{
			query("DROP TABLE IF EXISTS [HardwareSharing]");
			query("ALTER TABLE DeviceStatus ADD COLUMN [LastLevel] INTEGER default 0");
		}
		if (dbversion < 16)
		{
			query("ALTER TABLE Events RENAME TO tmp_Events;");
			query("CREATE TABLE Events ([ID] INTEGER PRIMARY KEY, [Name] VARCHAR(200) NOT NULL, [XMLStatement] TEXT NOT NULL,[Conditions] TEXT, [Actions] TEXT);");
			query("INSERT INTO Events(Name, XMLStatement) SELECT Name, XMLStatement FROM tmp_Events;");
			query("DROP TABLE tmp_Events;");
		}
		if (dbversion < 17)
		{
			query("ALTER TABLE Events ADD COLUMN [Status] INTEGER default 0");
		}
		if (dbversion < 18)
		{
			query("ALTER TABLE Temperature ADD COLUMN [DewPoint] FLOAT default 0");
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [DewPoint] FLOAT default 0");
		}
		if (dbversion < 19)
		{
			query("ALTER TABLE Scenes ADD COLUMN [SceneType] INTEGER default 0");
		}

		if (dbversion < 20)
		{
			query("INSERT INTO EventMaster(Name, XMLStatement, Status) SELECT Name, XMLStatement, Status FROM Events;");
			query("INSERT INTO EventRules(EMID, Conditions, Actions, SequenceNo) SELECT EventMaster.ID, Events.Conditions, Events.Actions, 1 FROM Events INNER JOIN EventMaster ON EventMaster.Name = Events.Name;");
			query("DROP TABLE Events;");
		}
		if (dbversion < 21)
		{
			//increase Video/Image URL for camera's
			//create a backup
			query("ALTER TABLE Cameras RENAME TO tmp_Cameras");
			//Create the new table
			query(sqlCreateCameras);
			//Copy values from tmp_Cameras back into our new table
			query(
				"INSERT INTO Cameras([ID],[Name],[Enabled],[Address],[Port],[Username],[Password],[ImageURL])"
				"SELECT [ID],[Name],[Enabled],[Address],[Port],[Username],[Password],[ImageURL]"
				"FROM tmp_Cameras");
			//Drop the tmp_Cameras table
			query("DROP TABLE tmp_Cameras");
		}
		if (dbversion < 22)
		{
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [Order] INTEGER BIGINT(10) default 0");
			query(sqlCreateDevicesToPlanStatusTrigger);
		}
		if (dbversion < 23)
		{
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [Temp_Avg] FLOAT default 0");

			std::vector<std::vector<std::string> > result;
			result = query("SELECT RowID, (Temp_Max+Temp_Min)/2 FROM Temperature_Calendar");
			if (result.size() > 0)
			{
				char szTmp[100];
				sqlite3_exec(m_dbase, "BEGIN TRANSACTION;", NULL, NULL, NULL);
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					sprintf(szTmp, "UPDATE Temperature_Calendar SET Temp_Avg=%.1f WHERE RowID='%s'", atof(sd[1].c_str()), sd[0].c_str());
					query(szTmp);
				}
				sqlite3_exec(m_dbase, "END TRANSACTION;", NULL, NULL, NULL);
			}
		}
		if (dbversion < 24)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [Order] INTEGER BIGINT(10) default 0");
			query(sqlCreateSceneDeviceTrigger);
			CheckAndUpdateSceneDeviceOrder();
		}
		if (dbversion < 25)
		{
			query("DROP TABLE IF EXISTS [Plans]");
			query(sqlCreatePlans);
			query(sqlCreatePlanOrderTrigger);
		}
		if (dbversion < 26)
		{
			query("DROP TABLE IF EXISTS [DeviceToPlansMap]");
			query(sqlCreatePlanMappings);
			query(sqlCreateDevicesToPlanStatusTrigger);
		}
		if (dbversion < 27)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [CustomImage] INTEGER default 0");
		}
		if (dbversion < 28)
		{
			query("ALTER TABLE Timers ADD COLUMN [UseRandomness] INTEGER default 0");
			query("ALTER TABLE SceneTimers ADD COLUMN [UseRandomness] INTEGER default 0");
			query("UPDATE Timers SET [Type]=2, [UseRandomness]=1 WHERE ([Type]=5)");
			query("UPDATE SceneTimers SET [Type]=2, [UseRandomness]=1 WHERE ([Type]=5)");
			//"[] INTEGER DEFAULT 0, "
		}
		if (dbversion < 29)
		{
			query("ALTER TABLE Scenes ADD COLUMN [ListenCmd] INTEGER default 1");
		}
		if (dbversion < 30)
		{
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [DevSceneType] INTEGER default 0");
		}
		if (dbversion < 31)
		{
			query("ALTER TABLE Users ADD COLUMN [TabsEnabled] INTEGER default 255");
		}
		if (dbversion < 32)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [Hue] INTEGER default 0");
			query("ALTER TABLE SceneTimers ADD COLUMN [Hue] INTEGER default 0");
			query("ALTER TABLE Timers ADD COLUMN [Hue] INTEGER default 0");
		}
		if (dbversion < 33)
		{
			query("DROP TABLE IF EXISTS [Load]");
			query("DROP TABLE IF EXISTS [Load_Calendar]");
			query("DROP TABLE IF EXISTS [Fan]");
			query("DROP TABLE IF EXISTS [Fan_Calendar]");
			query(sqlCreatePercentage);
			query(sqlCreatePercentage_Calendar);
			query(sqlCreateFan);
			query(sqlCreateFan_Calendar);

			char szTmp[200];

			std::vector<std::vector<std::string> > result;
			result = query("SELECT ID FROM DeviceStatus WHERE (DeviceID LIKE 'WMI%')");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					sprintf(szTmp, "DELETE FROM Temperature WHERE (DeviceRowID='%s')", idx.c_str());
					query(szTmp);
					sprintf(szTmp, "DELETE FROM Temperature_Calendar WHERE (DeviceRowID='%s')", idx.c_str());
					query(szTmp);
				}
			}
			query("DELETE FROM DeviceStatus WHERE (DeviceID LIKE 'WMI%')");
		}
		if (dbversion < 34)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [StrParam1] VARCHAR(200) DEFAULT ''");
			query("ALTER TABLE DeviceStatus ADD COLUMN [StrParam2] VARCHAR(200) DEFAULT ''");
		}
		if (dbversion < 35)
		{
			query("ALTER TABLE Notifications ADD COLUMN [Priority] INTEGER default 0");
		}
		if (dbversion < 36)
		{
			query("ALTER TABLE Meter ADD COLUMN [Usage] INTEGER default 0");
		}
		if (dbversion < 37)
		{
			//move all load data from tables into the new percentage one
			query(
				"INSERT INTO Percentage([DeviceRowID],[Percentage],[Date])"
				"SELECT [DeviceRowID],[Load],[Date] FROM Load");
			query(
				"INSERT INTO Percentage_Calendar([DeviceRowID],[Percentage_Min],[Percentage_Max],[Percentage_Avg],[Date])"
				"SELECT [DeviceRowID],[Load_Min],[Load_Max],[Load_Avg],[Date] FROM Load_Calendar");
			query("DROP TABLE IF EXISTS [Load]");
			query("DROP TABLE IF EXISTS [Load_Calendar]");
		}
		if (dbversion < 38)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [Protected] INTEGER default 0");
		}
		if (dbversion < 39)
		{
			query("ALTER TABLE Scenes ADD COLUMN [Protected] INTEGER default 0");
		}
		if (dbversion < 40)
		{
			FixDaylightSaving();
		}
		if (dbversion < 41)
		{
			query("ALTER TABLE FibaroLink ADD COLUMN [IncludeUnit] INTEGER default 0");
		}
		if (dbversion < 42)
		{
			query("INSERT INTO Plans (Name) VALUES ('$Hidden Devices')");
		}
		if (dbversion < 43)
		{
			query("ALTER TABLE Scenes ADD COLUMN [OnAction] VARCHAR(200) DEFAULT ''");
			query("ALTER TABLE Scenes ADD COLUMN [OffAction] VARCHAR(200) DEFAULT ''");
		}
		if (dbversion < 44)
		{
			//Drop VideoURL
			//create a backup
			query("ALTER TABLE Cameras RENAME TO tmp_Cameras");
			//Create the new table
			query(sqlCreateCameras);
			//Copy values from tmp_Cameras back into our new table
			query(
				"INSERT INTO Cameras([ID],[Name],[Enabled],[Address],[Port],[Username],[Password],[ImageURL])"
				"SELECT [ID],[Name],[Enabled],[Address],[Port],[Username],[Password],[ImageURL]"
				"FROM tmp_Cameras");
			//Drop the tmp_Cameras table
			query("DROP TABLE tmp_Cameras");
		}
		if (dbversion < 45)
		{
			query("ALTER TABLE Timers ADD COLUMN [TimerPlan] INTEGER default 0");
			query("ALTER TABLE SceneTimers ADD COLUMN [TimerPlan] INTEGER default 0");
		}
		if (dbversion < 46)
		{
			query("ALTER TABLE Plans ADD COLUMN [FloorplanID] INTEGER default 0");
			query("ALTER TABLE Plans ADD COLUMN [Area] VARCHAR(200) DEFAULT ''");
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [XOffset] INTEGER default 0");
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [YOffset] INTEGER default 0");
		}
		if (dbversion < 47)
		{
			query("ALTER TABLE Hardware ADD COLUMN [DataTimeout] INTEGER default 0");
		}
		if (dbversion < 48)
		{
			char szTmp[200];
			sprintf(szTmp, "SELECT ID FROM DeviceStatus WHERE (Type=%d)", pTypeUsage);
			result = query(szTmp);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					sprintf(szTmp, "UPDATE Meter SET Value = Value * 10 WHERE (DeviceRowID='%s')", idx.c_str());
					query(szTmp);
					sprintf(szTmp, "UPDATE Meter_Calendar SET Value = Value * 10 WHERE (DeviceRowID='%s')", idx.c_str());
					query(szTmp);
					sprintf(szTmp, "UPDATE MultiMeter_Calendar SET Value1 = Value1 * 10, Value2 = Value2 * 10 WHERE (DeviceRowID='%s')", idx.c_str());
					query(szTmp);
				}
			}
			query("ALTER TABLE Hardware ADD COLUMN [DataTimeout] INTEGER default 0");
		}
		if (dbversion < 49)
		{
			query("ALTER TABLE Plans ADD COLUMN [FloorplanID] INTEGER default 0");
			query("ALTER TABLE Plans ADD COLUMN [Area] VARCHAR(200) DEFAULT ''");
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [XOffset] INTEGER default 0");
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [YOffset] INTEGER default 0");
		}
		if (dbversion < 50)
		{
			query("ALTER TABLE Timers ADD COLUMN [Date] DATE default 0");
			query("ALTER TABLE SceneTimers ADD COLUMN [Date] DATE default 0");
		}
		if (dbversion < 51)
		{
			query("ALTER TABLE Meter_Calendar ADD COLUMN [Counter] BIGINT default 0");
			query("ALTER TABLE MultiMeter_Calendar ADD COLUMN [Counter1] BIGINT default 0");
			query("ALTER TABLE MultiMeter_Calendar ADD COLUMN [Counter2] BIGINT default 0");
			query("ALTER TABLE MultiMeter_Calendar ADD COLUMN [Counter3] BIGINT default 0");
			query("ALTER TABLE MultiMeter_Calendar ADD COLUMN [Counter4] BIGINT default 0");
		}
		if (dbversion < 52)
		{
			//Move onboard system sensor (temperature) to the motherboard hardware
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			szQuery << "SELECT ID FROM Hardware WHERE (Type=='" << HTYPE_System << "') AND (Name=='Motherboard') LIMIT 1";
			result = m_sql.query(szQuery.str());
			if (!result.empty())
			{
				int hwId = atoi(result[0][0].c_str());
				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE DeviceStatus SET HardwareID=" << hwId << " WHERE (HardwareID=1000)";
				m_sql.query(szQuery.str());
			}
		}
		if (dbversion < 53)
		{
			query("ALTER TABLE Floorplans ADD COLUMN [ScaleFactor] Float default 1.0");
		}
		if (dbversion < 54)
		{
			query("ALTER TABLE Temperature ADD COLUMN [SetPoint] FLOAT default 0");
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [SetPoint_Min] FLOAT default 0");
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [SetPoint_Max] FLOAT default 0");
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [SetPoint_Avg] FLOAT default 0");
		}
		if (dbversion < 55)
		{
			query("DROP TABLE IF EXISTS [CustomImages]");
			query(sqlCreateCustomImages);
		}
		if (dbversion < 56)
		{
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result2;
			std::vector<std::vector<std::string> >::const_iterator itt;
			std::string pHash;
			szQuery2 << "SELECT sValue FROM Preferences WHERE (Key='WebPassword')";
			result2 = m_sql.query(szQuery2.str());
			if (result2.size() > 0)
			{
				std::string pwd = result2[0][0];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Preferences SET sValue='" << pHash << "' WHERE (Key='WebPassword')";
					m_sql.query(szQuery2.str());
				}
			}

			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT sValue FROM Preferences WHERE (Key='SecPassword')";
			result2 = m_sql.query(szQuery2.str());
			if (result2.size() > 0)
			{
				std::string pwd = result2[0][0];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Preferences SET sValue='" << pHash << "' WHERE (Key='SecPassword')";
					m_sql.query(szQuery2.str());
				}
			}

			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT sValue FROM Preferences WHERE (Key='ProtectionPassword')";
			result2 = m_sql.query(szQuery2.str());
			if (result2.size() > 0)
			{
				std::string pwd = result2[0][0];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Preferences SET sValue='" << pHash << "' WHERE (Key='ProtectionPassword')";
					m_sql.query(szQuery2.str());
				}
			}
			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT ID, Password FROM Users";
			result2 = m_sql.query(szQuery2.str());
			for (itt = result2.begin(); itt != result2.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				std::string pwd = sd[1];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Users SET Password='" << pHash << "' WHERE (ID=" << sd[0] << ")";
					m_sql.query(szQuery2.str());
				}
			}

			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT ID, Password FROM Hardware WHERE ([Type]==" << HTYPE_Domoticz << ")";
			result2 = m_sql.query(szQuery2.str());
			for (itt = result2.begin(); itt != result2.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				std::string pwd = sd[1];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(pwd);
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Hardware SET Password='" << pHash << "' WHERE (ID=" << sd[0] << ")";
					m_sql.query(szQuery2.str());
				}
			}
		}
		if (dbversion < 57)
		{
			//S0 Meter patch
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID, Mode1, Mode2, Mode3, Mode4 FROM HARDWARE WHERE([Type]==" << HTYPE_S0SmartMeter << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::stringstream szAddress;
					szAddress
						<< sd[1] << ";" << sd[2] << ";"
						<< sd[3] << ";" << sd[4] << ";"
						<< sd[1] << ";" << sd[2] << ";"
						<< sd[1] << ";" << sd[2] << ";"
						<< sd[1] << ";" << sd[2];
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Hardware SET Address='" << szAddress.str() << "', Mode1=0, Mode2=0, Mode3=0, Mode4=0 WHERE (ID=" << sd[0] << ")";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 58)
		{
			//Patch for new ZWave light sensor type
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			result = query("SELECT ID FROM Hardware WHERE ([Type] = 21) OR ([Type] = 21)");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE DeviceStatus SET SubType=" << sTypeZWaveSwitch << " WHERE ([Type]=" << pTypeLighting2 << ") AND (SubType=" << sTypeAC << ") AND (HardwareID=" << result[0][0] << ")";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 60)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [OnDelay] INTEGER default 0");
			query("ALTER TABLE SceneDevices ADD COLUMN [OffDelay] INTEGER default 0");
		}
		if (dbversion < 61)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [Description] VARCHAR(200) DEFAULT ''");
		}
		if (dbversion < 62)
		{
			//Fix for Teleinfo hardware, where devices where created with Hardware_ID=0
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			result = query("SELECT ID FROM Hardware WHERE ([Type] = 19)");
			if (result.size() > 0)
			{
				int TeleInfoHWID = atoi(result[0][0].c_str());
				szQuery2 << "UPDATE DeviceStatus SET HardwareID=" << TeleInfoHWID << " WHERE ([HardwareID]=0)";
				query(szQuery2.str());
			}
		}
		if (dbversion < 63)
		{
			query("DROP TABLE IF EXISTS [TempVars]");
		}
		if (dbversion < 64)
		{
			FixDaylightSaving();
		}
		if (dbversion < 65)
		{
			//Patch for Toon, reverse counters
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			std::vector<std::vector<std::string> > result2;
			std::vector<std::vector<std::string> > result3;
			std::vector<std::vector<std::string> >::const_iterator itt;
			std::vector<std::vector<std::string> >::const_iterator itt2;
			std::vector<std::vector<std::string> >::const_iterator itt3;
			szQuery << "SELECT ID FROM HARDWARE WHERE([Type]==" << HTYPE_TOONTHERMOSTAT << ")";
			result = query(szQuery.str());
			for (itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				int hwid = atoi(sd[0].c_str());

				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM DeviceStatus WHERE (Type=" << pTypeP1Power << ") AND (HardwareID=" << hwid << ")";
				result2 = query(szQuery.str());
				for (itt2 = result2.begin(); itt2 != result2.end(); ++itt2)
				{
					std::vector<std::string> sd = *itt2;

					//First the shortlog
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT ROWID, Value1, Value2, Value3, Value4, Value5, Value6 FROM MultiMeter WHERE (DeviceRowID==" << sd[0] << ")";
					result3 = m_sql.query(szQuery.str());
					for (itt3 = result3.begin(); itt3 != result3.end(); ++itt3)
					{
						std::vector<std::string> sd = *itt3;
						//value1 = powerusage1;
						//value2 = powerdeliv1;
						//value5 = powerusage2;
						//value6 = powerdeliv2;
						//value3 = usagecurrent;
						//value4 = delivcurrent;
						szQuery.clear();
						szQuery.str("");
						szQuery << "UPDATE MultiMeter SET Value1=" << sd[5] << ", Value2=" << sd[6] << ", Value5=" << sd[1] << ", Value6=" << sd[2] << " WHERE (ROWID=" << sd[0] << ")";
						query(szQuery.str());
					}
					//Next for the calendar
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT ROWID, Value1, Value2, Value3, Value4, Value5, Value6, Counter1, Counter2, Counter3, Counter4 FROM MultiMeter_Calendar WHERE (DeviceRowID==" << sd[0] << ")";
					result3 = m_sql.query(szQuery.str());
					for (itt3 = result3.begin(); itt3 != result3.end(); ++itt3)
					{
						std::vector<std::string> sd = *itt3;
						szQuery.clear();
						szQuery.str("");
						szQuery << "UPDATE MultiMeter_Calendar SET Value1=" << sd[5] << ", Value2=" << sd[6] << ", Value5=" << sd[1] << ", Value6=" << sd[2] << ", Counter1=" << sd[9] << ", Counter2=" << sd[10] << ", Counter3=" << sd[7] << ", Counter4=" << sd[8] << " WHERE (ROWID=" << sd[0] << ")";
						query(szQuery.str());
					}
				}
			}
		}
		if (dbversion < 66)
		{
			query("ALTER TABLE Hardware ADD COLUMN [Mode6] CHAR default 0");
		}
		if (dbversion < 67)
		{
			//Enable all notification systems
			UpdatePreferencesVar("NMAEnabled", 1);
			UpdatePreferencesVar("ProwlEnabled", 1);
			UpdatePreferencesVar("PushALotEnabled", 1);
			UpdatePreferencesVar("PushoverEnabled", 1);
			UpdatePreferencesVar("ClickatellEnabled", 1);
		}
	}
	else if (bNewInstall)
	{
		//place here actions that needs to be performed on new databases
		query("INSERT INTO Plans (Name) VALUES ('$Hidden Devices')");
	}
	UpdatePreferencesVar("DB_Version",DB_VERSION);

	//Make sure we have some default preferences
	int nValue;
	std::string sValue;
	if (!GetPreferencesVar("LightHistoryDays", nValue))
	{
		UpdatePreferencesVar("LightHistoryDays", 30);
	}
	if (!GetPreferencesVar("MeterDividerEnergy", nValue))
	{
		UpdatePreferencesVar("MeterDividerEnergy", 1000);
	}
	else if (nValue == 0)
	{
		//Sanity check!
		UpdatePreferencesVar("MeterDividerEnergy", 1000);
	}
	if (!GetPreferencesVar("MeterDividerGas", nValue))
	{
		UpdatePreferencesVar("MeterDividerGas", 100);
	}
	else if (nValue == 0)
	{
		//Sanity check!
		UpdatePreferencesVar("MeterDividerGas", 100);
	}
	if (!GetPreferencesVar("MeterDividerWater", nValue))
	{
		UpdatePreferencesVar("MeterDividerWater", 100);
	}
	else if (nValue == 0)
	{
		//Sanity check!
		UpdatePreferencesVar("MeterDividerWater", 100);
	}
	if (!GetPreferencesVar("RandomTimerFrame", nValue))
	{
		UpdatePreferencesVar("RandomTimerFrame", 15);
	}
	if (!GetPreferencesVar("ElectricVoltage", nValue))
	{
		UpdatePreferencesVar("ElectricVoltage", 230);
	}
	if (!GetPreferencesVar("CM113DisplayType", nValue))
	{
		UpdatePreferencesVar("CM113DisplayType", 0);
	}
	if (!GetPreferencesVar("5MinuteHistoryDays", nValue))
	{
		UpdatePreferencesVar("5MinuteHistoryDays", 1);
	}
	if (!GetPreferencesVar("SensorTimeout", nValue))
	{
		UpdatePreferencesVar("SensorTimeout", 60);
	}
	if (!GetPreferencesVar("SensorTimeoutNotification", nValue))
	{
		UpdatePreferencesVar("SensorTimeoutNotification", 0); //default disabled
	}
	
	if (!GetPreferencesVar("UseAutoUpdate", nValue))
	{
		UpdatePreferencesVar("UseAutoUpdate", 1);
	}

	if (!GetPreferencesVar("UseAutoBackup", nValue))
	{
		UpdatePreferencesVar("UseAutoBackup", 0);
	}

	if (GetPreferencesVar("Rego6XXType", nValue))
	{
        // The setting is no longer here. It has moved to the hardware table (Mode1)
        // Copy the setting so no data is lost - if the rego is used. (zero is the default 
        // so it's no point copying this value...)
        // THIS SETTING CAN BE REMOVED WHEN ALL CAN BE ASSUMED TO HAVE UPDATED (summer 2013?)
        if(nValue > 0)
        {
        	std::stringstream szQuery;

			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID,Mode1 FROM Hardware WHERE (Type=" << HTYPE_Rego6XX << ")";
        	result=query(szQuery.str());
        	if (result.size()>0)
            {
                if(atoi(result[0][1].c_str()) != nValue)
                {
					UpdateRFXCOMHardwareDetails(atoi(result[0][0].c_str()), nValue, 0, 0, 0, 0, 0);
                }
            }
		    UpdatePreferencesVar("Rego6XXType", 0); // Set to zero to avoid another copy
        }
	}
	//Costs for Energy/Gas and Water (See your provider, try to include tax and other stuff)
	if (!GetPreferencesVar("CostEnergy", nValue))
	{
		UpdatePreferencesVar("CostEnergy", 2149);
	}
	if (!GetPreferencesVar("CostEnergyT2", nValue))
	{
		GetPreferencesVar("CostEnergy", nValue);
		UpdatePreferencesVar("CostEnergyT2", nValue);
	}
	if (!GetPreferencesVar("CostGas", nValue))
	{
		UpdatePreferencesVar("CostGas", 6218);
	}
	if (!GetPreferencesVar("CostWater", nValue))
	{
		UpdatePreferencesVar("CostWater", 16473);
	}
	if (!GetPreferencesVar("UseEmailInNotifications", nValue))
	{
		UpdatePreferencesVar("UseEmailInNotifications", 1);
	}
	if (!GetPreferencesVar("EmailPort", nValue))
	{
		UpdatePreferencesVar("EmailPort", 25);
	}
	if (!GetPreferencesVar("EmailAsAttachment", nValue))
	{
		UpdatePreferencesVar("EmailAsAttachment", 0);
	}
	if (!GetPreferencesVar("DoorbellCommand", nValue))
	{
		UpdatePreferencesVar("DoorbellCommand", 0);
	}
	if (!GetPreferencesVar("SmartMeterType", nValue))	//0=meter has decimals, 1=meter does not have decimals, need this for the day graph
	{
		UpdatePreferencesVar("SmartMeterType", 0);
	}
	if (!GetPreferencesVar("EnableTabLights", nValue))
	{
		UpdatePreferencesVar("EnableTabLights", 1);
	}
	if (!GetPreferencesVar("EnableTabTemp", nValue))
	{
		UpdatePreferencesVar("EnableTabTemp", 1);
	}
	if (!GetPreferencesVar("EnableTabWeather", nValue))
	{
		UpdatePreferencesVar("EnableTabWeather", 1);
	}
	if (!GetPreferencesVar("EnableTabUtility", nValue))
	{
		UpdatePreferencesVar("EnableTabUtility", 1);
	}
	if (!GetPreferencesVar("EnableTabCustom", nValue))
	{
		UpdatePreferencesVar("EnableTabCustom", 1);
	}
	if (!GetPreferencesVar("EnableTabScenes", nValue))
	{
		UpdatePreferencesVar("EnableTabScenes", 1);
	}
	if (!GetPreferencesVar("EnableTabFloorplans", nValue))
	{
		UpdatePreferencesVar("EnableTabFloorplans", 0);
	}
	if (!GetPreferencesVar("NotificationSensorInterval", nValue))
	{
		UpdatePreferencesVar("NotificationSensorInterval", 12*60*60);
	}
	else
	{
		if (nValue<60)
			UpdatePreferencesVar("NotificationSensorInterval", 60);
	}
	if (!GetPreferencesVar("NotificationSwitchInterval", nValue))
	{
		UpdatePreferencesVar("NotificationSwitchInterval", 0);
	}
	if (!GetPreferencesVar("RemoteSharedPort", nValue))
	{
		UpdatePreferencesVar("RemoteSharedPort", 6144);
	}
	if (!GetPreferencesVar("Language", sValue))
	{
		UpdatePreferencesVar("Language", "en");
	}
	if (!GetPreferencesVar("DashboardType", nValue))
	{
		UpdatePreferencesVar("DashboardType", 0);
	}
	if (!GetPreferencesVar("MobileType", nValue))
	{
		UpdatePreferencesVar("MobileType", 0);
	}
	if (!GetPreferencesVar("WindUnit", nValue))
	{
		UpdatePreferencesVar("WindUnit", (int)WINDUNIT_MS);
	}
	else
	{
		m_windunit=(_eWindUnit)nValue;
	}
	if (!GetPreferencesVar("TempUnit", nValue))
	{
		UpdatePreferencesVar("TempUnit", (int)TEMPUNIT_C);
	}
	else
	{
		m_tempunit=(_eTempUnit)nValue;

	}
	SetUnitsAndScale();

	if (!GetPreferencesVar("SecStatus", nValue))
	{
		UpdatePreferencesVar("SecStatus", (int)SECSTATUS_DISARMED);
	}
	if (!GetPreferencesVar("SecOnDelay", nValue))
	{
		UpdatePreferencesVar("SecOnDelay", 30);
	}

	if (!GetPreferencesVar("AuthenticationMethod", nValue))
	{
		UpdatePreferencesVar("AuthenticationMethod", 0);//AUTH_LOGIN=0, AUTH_BASIC=1
	}
	if (!GetPreferencesVar("ReleaseChannel", nValue))
	{
		UpdatePreferencesVar("ReleaseChannel", 0);//Stable=0, Beta=1
	}
	if (!GetPreferencesVar("RaspCamParams", sValue))
	{
		UpdatePreferencesVar("RaspCamParams", "-w 800 -h 600 -t 1"); //width/height/time2wait
	}
	if (!GetPreferencesVar("UVCParams", sValue))
	{
		UpdatePreferencesVar("UVCParams", "-S80 -B128 -C128 -G80 -x800 -y600 -q100"); //width/height/time2wait
	}
	if (sValue == "- -S80 -B128 -C128 -G80 -x800 -y600 -q100")
	{
		UpdatePreferencesVar("UVCParams", "-S80 -B128 -C128 -G80 -x800 -y600 -q100"); //fix a bug
	}

	nValue = 1;
	if (!GetPreferencesVar("AcceptNewHardware", nValue))
	{
		UpdatePreferencesVar("AcceptNewHardware", 1);
		nValue=1;
	}
	m_bAcceptNewHardware=(nValue==1);
	if (!GetPreferencesVar("ZWavePollInterval", nValue))
	{
		UpdatePreferencesVar("ZWavePollInterval", 60);
	}
	if (!GetPreferencesVar("ZWaveEnableDebug", nValue))
	{
		UpdatePreferencesVar("ZWaveEnableDebug", 0);
	}
	if (!GetPreferencesVar("ZWaveNetworkKey", sValue))
	{
		UpdatePreferencesVar("ZWaveNetworkKey", "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10");
	}
	if (!GetPreferencesVar("ZWaveEnableNightlyNetworkHeal", nValue))
	{
		UpdatePreferencesVar("ZWaveEnableNightlyNetworkHeal", 0);
	}
	if (!GetPreferencesVar("BatteryLowNotification", nValue))
	{
		UpdatePreferencesVar("BatteryLowNotification", 0); //default disabled
	}
	nValue = 1;
	if (!GetPreferencesVar("AllowWidgetOrdering", nValue))
	{
		UpdatePreferencesVar("AllowWidgetOrdering", 1); //default enabled
		nValue=1;
	}
	m_bAllowWidgetOrdering=(nValue==1);
	nValue = 0;
	if (!GetPreferencesVar("ActiveTimerPlan", nValue))
	{
		UpdatePreferencesVar("ActiveTimerPlan", 0); //default
		nValue=0;
	}
	m_ActiveTimerPlan=nValue;
	if (!GetPreferencesVar("HideDisabledHardwareSensors", nValue))
	{
		UpdatePreferencesVar("HideDisabledHardwareSensors", 1);
	}
	nValue = 0;
	if (!GetPreferencesVar("DisableEventScriptSystem", nValue))
	{
		UpdatePreferencesVar("DisableEventScriptSystem", 0);
		nValue = 0;
	}
	m_bDisableEventSystem = (nValue==1);

	if (!GetPreferencesVar("WebTheme", sValue))
	{
		UpdatePreferencesVar("WebTheme", "default");
	}

	if (!GetPreferencesVar("FloorplanPopupDelay", nValue))
	{
		UpdatePreferencesVar("FloorplanPopupDelay", 750);
	}
	if (!GetPreferencesVar("FloorplanFullscreenMode", nValue))
	{
		UpdatePreferencesVar("FloorplanFullscreenMode", 0);
	}
	if (!GetPreferencesVar("FloorplanAnimateZoom", nValue))
	{
		UpdatePreferencesVar("FloorplanAnimateZoom", 1);
	}
	if (!GetPreferencesVar("FloorplanShowSensorValues", nValue))
	{
		UpdatePreferencesVar("FloorplanShowSensorValues", 1);
	}
	if (!GetPreferencesVar("FloorplanShowSwitchValues", nValue))
	{
		UpdatePreferencesVar("FloorplanShowSwitchValues", 0);
	}
	if (!GetPreferencesVar("FloorplanShowSceneNames", nValue))
	{
		UpdatePreferencesVar("FloorplanShowSceneNames", 1);
	}
	if (!GetPreferencesVar("FloorplanRoomColour", sValue))
	{
		UpdatePreferencesVar("FloorplanRoomColour", "Blue");
	}
	if (!GetPreferencesVar("FloorplanActiveOpacity", nValue))
	{
		UpdatePreferencesVar("FloorplanActiveOpacity", 25);
	}
	if (!GetPreferencesVar("FloorplanInactiveOpacity", nValue))
	{
		UpdatePreferencesVar("FloorplanInactiveOpacity", 5);
	}
	if (!GetPreferencesVar("TempHome", sValue))
	{
		UpdatePreferencesVar("TempHome", "20");
	}
	if (!GetPreferencesVar("TempAway", sValue))
	{
		UpdatePreferencesVar("TempAway", "15");
	}
	if (!GetPreferencesVar("TempComfort", sValue))
	{
		UpdatePreferencesVar("TempComfort", "22.0");
	}

	//Start background thread
	if (!StartThread())
		return false;
	return true;
}

bool CSQLHelper::StartThread()
{
	m_background_task_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSQLHelper::Do_Work, this)));

	return (m_background_task_thread!=NULL);
}

bool CSQLHelper::SwitchLightFromTasker(const std::string &idx, const std::string &switchcmd, const std::string &level, const std::string &hue)
{
	unsigned long long ID;
	std::stringstream s_str(idx);
	s_str >> ID;

	return SwitchLightFromTasker(ID, switchcmd, atoi(level.c_str()), atoi(hue.c_str()));
}

bool CSQLHelper::SwitchLightFromTasker(unsigned long long idx, const std::string &switchcmd, int level, int hue)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,AddjValue2 FROM DeviceStatus WHERE (ID == " << idx << ")";
	result = m_sql.query(szQuery.str());
	if (result.size()<1)
		return false;

	std::vector<std::string> sd = result[0];
	return m_mainworker.SwitchLightInt(sd, switchcmd, level, hue, false);
}

void CSQLHelper::Do_Work()
{
	std::vector<_tTaskItem> _items2do;

	while (!m_stoprequested)
	{
		//sleep 1 second
		sleep_seconds(1);

		if (m_bAcceptHardwareTimerActive)
		{
			m_iAcceptHardwareTimerCounter--;
			if (m_iAcceptHardwareTimerCounter <= 0)
			{
				m_bAcceptHardwareTimerActive = false;
				m_bAcceptNewHardware = m_bPreviousAcceptNewHardware;
				UpdatePreferencesVar("AcceptNewHardware", (m_bAcceptNewHardware==true)?1:0);
				if (!m_bAcceptNewHardware)
				{
					_log.Log(LOG_STATUS, "Receiving of new sensors disabled!...");
				}
			}
		}

		if (m_background_task_queue.size()>0)
		{
			_items2do.clear();
			boost::lock_guard<boost::mutex> l(m_background_task_mutex);

			std::vector<_tTaskItem>::iterator itt=m_background_task_queue.begin();
			while (itt!=m_background_task_queue.end())
			{
				itt->_DelayTime--;
				if (itt->_DelayTime<=0)
				{
					_items2do.push_back(*itt);
					itt=m_background_task_queue.erase(itt);
				}
				else
					++itt;
			}
		}
		if (_items2do.size()<1)
			continue;

		std::vector<_tTaskItem>::iterator itt=_items2do.begin();
		while (itt!=_items2do.end())
		{
			if (itt->_ItemType == TITEM_SWITCHCMD)
			{
				if (itt->_switchtype==STYPE_Motion)
				{
					std::string devname="";
					switch (itt->_devType)
					{
					case pTypeLighting1:
					case pTypeLighting2:
					case pTypeLighting3:
					case pTypeLighting5:
					case pTypeLighting6:
					case pTypeLimitlessLights:
					case pTypeGeneralSwitch:
						SwitchLightFromTasker(itt->_idx, "Off", 0, -1);
						break;
					case pTypeSecurity1:
						switch (itt->_subType)
						{
						case sTypeSecX10M:
							SwitchLightFromTasker(itt->_idx, "No Motion", 0, -1);
							break;
						default:
							//just update internally
							UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(),devname,true);
							break;
						}
						break;
					case pTypeLighting4:
						//only update internally
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(),devname,true);
						break;
					default:
						//unknown hardware type, sensor will only be updated internally
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(),devname,true);
						break;
					}
				}
				else
				{
					if (itt->_devType==pTypeLighting4)
					{
						//only update internally
						std::string devname="";
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(),devname,true);
					}
					else
						SwitchLightFromTasker(itt->_idx, "Off", 0, -1);
				}
			}
			else if (itt->_ItemType == TITEM_EXECUTE_SCRIPT)
			{
				//start script
				_log.Log(LOG_STATUS, "Executing script: %s", itt->_ID.c_str());
#ifdef WIN32
				ShellExecute(NULL,"open",itt->_ID.c_str(),itt->_sValue.c_str(),NULL,SW_SHOWNORMAL);
#else
				std::string lscript=itt->_ID + " " + itt->_sValue;
				int ret=system(lscript.c_str());
				if (ret != 0)
				{
					_log.Log(LOG_ERROR, "Error executing script command (%s). returned: %d",itt->_ID.c_str(), ret);
				}
#endif
			}
			else if (itt->_ItemType == TITEM_EMAIL_CAMERA_SNAPSHOT)
			{
				m_mainworker.m_cameras.EmailCameraSnapshot(itt->_ID,itt->_sValue);
			}
			else if (itt->_ItemType == TITEM_GETURL)
			{
				std::string sResult;
				bool ret=HTTPClient::GET(itt->_sValue,sResult);
				if (!ret)
				{
					_log.Log(LOG_ERROR,"Error opening url: %s",itt->_sValue.c_str());
				}
			}
			else if ((itt->_ItemType == TITEM_SEND_EMAIL)||(itt->_ItemType == TITEM_SEND_EMAIL_TO))
			{
				int nValue;
				std::string sValue;
				if (GetPreferencesVar("EmailServer",nValue,sValue))
				{
					if (sValue!="")
					{
						std::string EmailFrom;
						std::string EmailTo;
						std::string EmailServer=sValue;
						int EmailPort=25;
						std::string EmailUsername;
						std::string EmailPassword;
						GetPreferencesVar("EmailFrom",nValue,EmailFrom);
						if (itt->_ItemType != TITEM_SEND_EMAIL_TO)
						{
							GetPreferencesVar("EmailTo",nValue,EmailTo);
						}
						else
						{
							EmailTo=itt->_command;
						}
						GetPreferencesVar("EmailUsername",nValue,EmailUsername);
						GetPreferencesVar("EmailPassword",nValue,EmailPassword);
						GetPreferencesVar("EmailPort", EmailPort);

						SMTPClient sclient;
						sclient.SetFrom(CURLEncode::URLDecode(EmailFrom.c_str()));
						sclient.SetTo(CURLEncode::URLDecode(EmailTo.c_str()));
						sclient.SetCredentials(base64_decode(EmailUsername),base64_decode(EmailPassword));
						sclient.SetServer(CURLEncode::URLDecode(EmailServer.c_str()),EmailPort);
						sclient.SetSubject(CURLEncode::URLDecode(itt->_ID));
						sclient.SetHTMLBody(itt->_sValue);
						bool bRet=sclient.SendEmail();

						if (bRet)
							_log.Log(LOG_STATUS,"Notification sent (Email)");
						else
							_log.Log(LOG_ERROR,"Notification failed (Email)");

					}
				}
			}
            else if (itt->_ItemType == TITEM_SWITCHCMD_EVENT)
            {
				SwitchLightFromTasker(itt->_idx, itt->_command.c_str(), itt->_level, itt->_Hue);
            }

            else if (itt->_ItemType == TITEM_SWITCHCMD_SCENE)
            {
				m_mainworker.SwitchScene(itt->_idx, itt->_command.c_str());
            }
            
			++itt;
		}
		_items2do.clear();
	}
}

void CSQLHelper::SetDatabaseName(const std::string &DBName)
{
	m_dbase_name=DBName;
}

std::vector<std::vector<std::string> > CSQLHelper::query(const std::string &szQuery)
{
	if (!m_dbase)
	{
		_log.Log(LOG_ERROR,"Database not open!!...Check your user rights!..");
		std::vector<std::vector<std::string> > results;
		return results;
	}
	boost::lock_guard<boost::mutex> l(m_sqlQueryMutex);
	
	sqlite3_stmt *statement;
	std::vector<std::vector<std::string> > results;

	if(sqlite3_prepare_v2(m_dbase, szQuery.c_str(), -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		int result = 0;
		while(true)
		{
			result = sqlite3_step(statement);

			if(result == SQLITE_ROW)
			{
				std::vector<std::string> values;
				for(int col = 0; col < cols; col++)
				{
					char* value = (char*)sqlite3_column_text(statement, col);
					if ((value == 0)&&(col==0))
						break;
					else if (value == 0)
						values.push_back(std::string("")); //insert empty string
					else
						values.push_back(value);
				}
				if (values.size()>0)
					results.push_back(values);
			}
			else
			{
				break;  
			}
		}
		sqlite3_finalize(statement);
	}

	std::string error = sqlite3_errmsg(m_dbase);
	if(error != "not an error") 
		_log.Log(LOG_ERROR,"%s",error.c_str());
	return results; 
}

std::vector<std::vector<std::string> > CSQLHelper::queryBlob(const std::string &szQuery)
{
	if (!m_dbase)
	{
		_log.Log(LOG_ERROR, "Database not open!!...Check your user rights!..");
		std::vector<std::vector<std::string> > results;
		return results;
	}
	boost::lock_guard<boost::mutex> l(m_sqlQueryMutex);

	sqlite3_stmt *statement;
	std::vector<std::vector<std::string> > results;

	if (sqlite3_prepare_v2(m_dbase, szQuery.c_str(), -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		int result = 0;
		while (true)
		{
			result = sqlite3_step(statement);

			if (result == SQLITE_ROW)
			{
				std::vector<std::string> values;
				for (int col = 0; col < cols; col++)
				{
					int blobSize = sqlite3_column_bytes(statement, col);
					char* value = (char*)sqlite3_column_blob(statement, col);
					if ((blobSize == 0) && (col == 0))
						break;
					else if (value == 0)
						values.push_back(std::string("")); //insert empty string
					else
						values.push_back(std::string(value,value+blobSize));
				}
				if (values.size()>0)
					results.push_back(values);
			}
			else
			{
				break;
			}
		}
		sqlite3_finalize(statement);
	}

	std::string error = sqlite3_errmsg(m_dbase);
	if (error != "not an error")
		_log.Log(LOG_ERROR, "%s", error.c_str());
	return results;
}

unsigned long long CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, std::string &devname, const bool bUseOnOffAction)
{
	return UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, "", devname, bUseOnOffAction);
}

unsigned long long CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue, std::string &devname, const bool bUseOnOffAction)
{
	return UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, 0, sValue, devname, bUseOnOffAction);
}

unsigned long long CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction)
{
	unsigned long long devRowID=UpdateValueInt(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue,devname,bUseOnOffAction);
	if (devRowID == -1)
		return -1;

	bool bIsLightSwitch=false;
	switch (devType)
	{
	case pTypeLighting1:
	case pTypeLighting2:
	case pTypeLighting3:
	case pTypeLighting4:
	case pTypeLighting5:
	case pTypeLighting6:
	case pTypeLimitlessLights:
	case pTypeSecurity1:
	case pTypeSecurity2:
	case pTypeCurtain:
	case pTypeBlinds:
	case pTypeRFY:
	case pTypeThermostat2:
	case pTypeThermostat3:
	case pTypeRemote:
	case pTypeGeneralSwitch:
		bIsLightSwitch = true;
		break;
	case pTypeRadiator1:
		bIsLightSwitch = (subType == sTypeSmartwaresSwitchRadiator);
		break;
	}
	if (!bIsLightSwitch)
		return devRowID;

	//Get the ID of this device
	std::vector<std::vector<std::string> > result,result2;
	std::vector<std::vector<std::string> >::const_iterator itt,itt2;
	char szTmp[300];

	sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return devRowID; //should never happen, because it was previously inserted if non-existent

	std::string idx=result[0][0];

	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now,&ltime);

	//Check if this switch was a Sub/Slave device for other devices, if so adjust the state of those other devices
	sprintf(szTmp,"SELECT ParentID FROM LightSubDevices WHERE (DeviceRowID=='%s') AND (DeviceRowID!=ParentID)",
		idx.c_str()
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		//This is a sub/slave device for another main device
		//Set the Main Device state to the same state as this device
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			sprintf(szTmp,
				"UPDATE DeviceStatus SET nValue=%d, sValue='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == '%s')",
				nValue,
				sValue,
				ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
				sd[0].c_str()
				);
			query(szTmp);

			//------
			//Should call eventsystem for the main switch here
			//m_mainworker.m_eventsystem.ProcessDevice(HardwareID, ulID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, 0);
			//------

			//Set the status of all slave devices from this device (except the one we just received) to off
			//Check if this switch was a Sub/Slave device for other devices, if so adjust the state of those other devices
			sprintf(szTmp,"SELECT a.DeviceRowID, b.Type FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='%s') AND (a.DeviceRowID!='%s') AND (b.ID == a.DeviceRowID) AND (a.DeviceRowID!=a.ParentID)",
				sd[0].c_str(),
				idx.c_str()
				);
			result2=query(szTmp);
			if (result2.size()>0)
			{
				for (itt2=result2.begin(); itt2!=result2.end(); ++itt2)
				{
					std::vector<std::string> sd=*itt2;
					int oDevType=atoi(sd[1].c_str());
					int newnValue=0;
					switch (oDevType)
					{
					case pTypeLighting1:
						newnValue=light1_sOff;
						break;
					case pTypeLighting2:
						newnValue=light2_sOff;
						break;
					case pTypeLighting3:
						newnValue=light3_sOff;
						break;
					case pTypeLighting4:
						newnValue=0;//light4_sOff;
						break;
					case pTypeLighting5:
						newnValue=light5_sOff;
						break;
					case pTypeLighting6:
						newnValue=light6_sOff;
						break;
					case pTypeLimitlessLights:
						newnValue=Limitless_LedOff;
						break;
					case pTypeSecurity1:
						newnValue=sStatusNormal;
						break;
					case pTypeSecurity2:
						newnValue = 0;// sStatusNormal;
						break;
					case pTypeCurtain:
						newnValue=curtain_sOpen;
						break;
					case pTypeBlinds:
						newnValue=blinds_sOpen;
						break;
					case pTypeRFY:
						newnValue=rfy_sUp;
						break;
					case pTypeThermostat2:
						newnValue = thermostat2_sOff;
						break;
					case pTypeThermostat3:
						newnValue=thermostat3_sOff;
						break;
					case pTypeRadiator1:
						newnValue = Radiator1_sNight;
						break;
					case pTypeGeneralSwitch:
						newnValue = gswitch_sOff;
						break;
					default:
						continue;
					}
					sprintf(szTmp,
						"UPDATE DeviceStatus SET nValue=%d, sValue='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == '%s')",
						newnValue,
						"",
						ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
						sd[0].c_str()
						);
					query(szTmp);
				}
			}
		}
	}

	//If this is a 'Main' device, and it has Sub/Slave devices,
	//set the status of the Sub/Slave devices to Off, as we might be out of sync then
	sprintf(szTmp,"SELECT a.DeviceRowID, b.Type FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='%s') AND (b.ID == a.DeviceRowID) AND (a.DeviceRowID!=a.ParentID)",
		idx.c_str()
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		//set the status to off
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			int oDevType=atoi(sd[1].c_str());
			int newnValue=0;
			switch (oDevType)
			{
			case pTypeLighting1:
				newnValue=light1_sOff;
				break;
			case pTypeLighting2:
				newnValue=light2_sOff;
				break;
			case pTypeLighting3:
				newnValue=light3_sOff;
				break;
			case pTypeLighting4:
				newnValue=0;//light4_sOff;
				break;
			case pTypeLighting5:
				newnValue=light5_sOff;
				break;
			case pTypeLighting6:
				newnValue=light6_sOff;
				break;
			case pTypeLimitlessLights:
				newnValue=Limitless_LedOff;
				break;
			case pTypeSecurity1:
				newnValue=sStatusNormal;
				break;
			case pTypeSecurity2:
				newnValue = 0;// sStatusNormal;
				break;
			case pTypeCurtain:
				newnValue=curtain_sOpen;
				break;
			case pTypeBlinds:
				newnValue=blinds_sOpen;
				break;
			case pTypeRFY:
				newnValue=rfy_sUp;
				break;
			case pTypeThermostat2:
				newnValue = thermostat2_sOff;
				break;
			case pTypeThermostat3:
				newnValue=thermostat3_sOff;
				break;
			case pTypeRadiator1:
				newnValue = Radiator1_sNight;
				break;
			case pTypeGeneralSwitch:
				newnValue = gswitch_sOff;
				break;
			default:
				continue;
			}
			sprintf(szTmp,
				"UPDATE DeviceStatus SET nValue=%d, sValue='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == '%s')",
				newnValue,
				"",
				ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
				sd[0].c_str()
				);
			query(szTmp);
		}
	}
	return devRowID;
}

unsigned long long CSQLHelper::UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction)
{
	if (!m_dbase)
		return -1;

	char szTmp[400];

	unsigned long long ulID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
	{
		//Insert

		if (!m_bAcceptNewHardware)
		{
			devname = "Ignored";
			return -1; //We do not allow new devices
		}

		devname="Unknown";
		sprintf(szTmp,
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue) "
			"VALUES ('%d','%s','%d','%d','%d','%d','%d','%d','%s')",
			HardwareID,
			ID,unit,devType,subType,
			signallevel,batterylevel,
			nValue,sValue);
		result=query(szTmp);

		//Get new ID
		sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
		result=query(szTmp);
		if (result.size()==0)
		{
			_log.Log(LOG_ERROR,"Serious database error, problem getting ID from DeviceStatus!");
			return -1;
		}
		std::stringstream s_str( result[0][0] );
		s_str >> ulID;
	}
	else
	{
		//Update
		std::stringstream s_str( result[0][0] );
		s_str >> ulID;

		devname=result[0][1];

		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now,&ltime);

		sprintf(szTmp,
			"UPDATE DeviceStatus SET SignalLevel=%d, BatteryLevel=%d, nValue=%d, sValue='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' "
			"WHERE (ID = %llu)",
			signallevel,batterylevel,
			nValue,sValue,
			ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
			ulID);
		result = query(szTmp);
	}

	switch (devType)
	{
	case pTypeRego6XXValue:
        if(subType != sTypeRego6XXStatus)
        {
            break;
        }
	case pTypeRadiator1:
		if (subType != sTypeSmartwaresSwitchRadiator)
		{
			break;
		}
	case pTypeLighting1:
	case pTypeLighting2:
	case pTypeLighting3:
	case pTypeLighting4:
	case pTypeLighting5:
	case pTypeLighting6:
	case pTypeLimitlessLights:
	case pTypeSecurity1:
	case pTypeSecurity2:
	case pTypeEvohome:
	case pTypeEvohomeRelay:
	case pTypeCurtain:
	case pTypeBlinds:
	case pTypeRFY:
	case pTypeChime:
	case pTypeThermostat2:
	case pTypeThermostat3:
	case pTypeRemote:
	case pTypeGeneralSwitch:
		//Add Lighting log
		m_LastSwitchID=ID;
		m_LastSwitchRowID=ulID;
		sprintf(szTmp,
			"INSERT INTO LightingLog (DeviceRowID, nValue, sValue) "
			"VALUES ('%llu', '%d', '%s')",
			ulID,
			nValue,sValue);
		result=query(szTmp);

		std::string lstatus="";
		int llevel=0;
		bool bHaveDimmer=false;
		bool bHaveGroupCmd=false;
		int maxDimLevel=0;

		sprintf(szTmp,
			"SELECT Name,SwitchType,AddjValue,StrParam1,StrParam2 FROM DeviceStatus WHERE (ID = %llu)",
			ulID);
		result = query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];
			std::string Name=sd[0];
			_eSwitchType switchtype=(_eSwitchType)atoi(sd[1].c_str());
			int AddjValue=(int)atof(sd[2].c_str());
			GetLightStatus(devType, subType, switchtype,nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

			bool bIsLightSwitchOn=IsLightSwitchOn(lstatus);

			if (((bIsLightSwitchOn) && (llevel != 0) && (llevel != 255)) || (switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted))
			{
				//update level for device
				sprintf(szTmp,
					"UPDATE DeviceStatus SET LastLevel='%d' WHERE (ID = %llu)",
					llevel,
					ulID);
				query(szTmp);

			}

			if (bUseOnOffAction)
			{
				//Perform any On/Off actions
				std::string OnAction=sd[3];
				std::string OffAction=sd[4];

				if(devType==pTypeEvohome)//would this be ok to extend as a general purpose feature?
				{
					stdreplace(OnAction, "{deviceid}", ID);
					stdreplace(OnAction, "{status}", lstatus);
					//boost::replace_all(OnAction, ID);//future expansion
					//boost::replace_all(OnAction, "{status}", lstatus);
					bIsLightSwitchOn=true;//Force use of OnAction for all actions
				}
				
				HandleOnOffAction(bIsLightSwitchOn,OnAction,OffAction);
			}

			//Check if we need to email a snapshot of a Camera
			std::string emailserver;
			int n2Value;
			if (GetPreferencesVar("EmailServer",n2Value,emailserver))
			{
				if (emailserver!="")
				{
					sprintf(szTmp,
						"SELECT CameraRowID, DevSceneDelay FROM CamerasActiveDevices WHERE (DevSceneType==0) AND (DevSceneRowID==%llu) AND (DevSceneWhen==%d)",
						ulID,
						(bIsLightSwitchOn==true)?0:1
						);
					result=query(szTmp);
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator ittCam;
						for (ittCam=result.begin(); ittCam!=result.end(); ++ittCam)
						{
							std::vector<std::string> sd=*ittCam;
							std::string camidx=sd[0];
							int delay=atoi(sd[1].c_str());
							std::string subject=Name + " Status: " + lstatus;
							AddTaskItem(_tTaskItem::EmailCameraSnapshot(delay+1,camidx,subject));
						}
					}
				}
			}

			if (!m_bDisableEventSystem)
			{
				//Execute possible script
				std::string scriptname;
#ifdef WIN32
				scriptname = szStartupFolder + "scripts\\domoticz_main.bat";
#else
				scriptname = szStartupFolder + "scripts/domoticz_main";
#endif
				if (file_exist(scriptname.c_str()))
				{
					//Add parameters
					std::stringstream s_scriptparams;
					std::string nszStartupFolder = szStartupFolder;
					if (nszStartupFolder == "")
						nszStartupFolder = ".";
					s_scriptparams << nszStartupFolder << " " << HardwareID << " " << ulID << " " << (bIsLightSwitchOn ? "On" : "Off") << " \"" << lstatus << "\"" << " \"" << devname << "\"";
					//add script to background worker				
					boost::lock_guard<boost::mutex> l(m_background_task_mutex);
					m_background_task_queue.push_back(_tTaskItem::ExecuteScript(1, scriptname, s_scriptparams.str()));
				}
			}

			//Check for notifications
			if (bIsLightSwitchOn)
				CheckAndHandleSwitchNotification(HardwareID,ID,unit,devType,subType,NTYPE_SWITCH_ON);
			else
				CheckAndHandleSwitchNotification(HardwareID,ID,unit,devType,subType,NTYPE_SWITCH_OFF);
			if (bIsLightSwitchOn)
			{
				if (AddjValue!=0) //Off Delay
				{
					bool bAdd2DelayQueue=false;
					int cmd=0;
					if (
						(switchtype==STYPE_OnOff)||
						(switchtype==STYPE_Motion)||
						(switchtype == STYPE_Dimmer) ||
						(switchtype == STYPE_PushOn) ||
						(switchtype==STYPE_DoorLock)
						)
					{
						switch (devType)
						{
						case pTypeLighting1:
							cmd=light1_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLighting2:
							cmd=light2_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLighting3:
							cmd=light3_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLighting4:
							cmd=light2_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLighting5:
							cmd=light5_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLighting6:
							cmd=light6_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeRemote:
							cmd=light2_sOff;
							break;
						case pTypeLimitlessLights:
							cmd=Limitless_LedOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeRFY:
							cmd = rfy_sStop;
							bAdd2DelayQueue = true;
							break;
						case pTypeRadiator1:
							cmd = Radiator1_sNight;
							bAdd2DelayQueue = true;
							break;
						case pTypeGeneralSwitch:
							cmd = gswitch_sOff;
							bAdd2DelayQueue = true;
							break;
						}
					}
	/* Smoke detectors are manually reset!
					else if (
						(devType==pTypeSecurity1)&&
						((subType==sTypeKD101)||(subType==sTypeSA30))
						)
					{
						cmd=sStatusPanicOff;
						bAdd2DelayQueue=true;
					}
	*/
					if (bAdd2DelayQueue==true)
					{
						boost::lock_guard<boost::mutex> l(m_background_task_mutex);
						_tTaskItem tItem=_tTaskItem::SwitchLight(AddjValue,ulID,HardwareID,ID,unit,devType,subType,switchtype,signallevel,batterylevel,cmd,sValue);
						//Remove all instances with this device from the queue first
						//otherwise command will be send twice, and first one will be to soon as it is currently counting
						std::vector<_tTaskItem>::iterator itt=m_background_task_queue.begin();
						while (itt!=m_background_task_queue.end())
						{
							if (
								(itt->_ItemType==TITEM_SWITCHCMD)&&
								(itt->_idx==ulID)&&
								(itt->_HardwareID == HardwareID)&&
								(itt->_nValue == cmd)
								)
							{
								itt=m_background_task_queue.erase(itt);
							}
							else
								++itt;
						}
						//finally add it to the queue
						m_background_task_queue.push_back(tItem);
					}
				}
			}
		}//end of check for notifications
		
		//Check Scene Status
		CheckSceneStatusWithDevice(ulID);
		break;
	}
	m_mainworker.m_eventsystem.ProcessDevice(HardwareID, ulID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, 0);
	return ulID;
}

bool CSQLHelper::GetLastValue(const int HardwareID, const char* DeviceID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &nValue, std::string &sValue, struct tm &LastUpdateTime)
{
    bool result=false;
    std::vector<std::vector<std::string> > sqlresult;
	char szTmp[400];
	std::string sLastUpdate;
	//std::string sValue;
	//struct tm LastUpdateTime;
	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);
		
	sprintf(szTmp,"SELECT nValue,sValue,LastUpdate FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID=%s AND Unit=%d AND Type=%d AND SubType=%d) order by LastUpdate desc limit 1",HardwareID,DeviceID,unit,devType,subType);
	sqlresult=query(szTmp);
	
	if (sqlresult.size()!=0)
	{
		nValue=(int)atoi(sqlresult[0][0].c_str());
		sValue=sqlresult[0][1];
	    sLastUpdate=sqlresult[0][2];
		
		LastUpdateTime.tm_isdst=tm1.tm_isdst;
		LastUpdateTime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
		LastUpdateTime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
		LastUpdateTime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
		LastUpdateTime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
		LastUpdateTime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
		LastUpdateTime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
	
		result=true;
	}
	
	return result;
}


void CSQLHelper::GetAddjustment(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti)
{
	AddjValue=0.0f;
	AddjMulti=1.0f;
	std::vector<std::vector<std::string> > result;
	char szTmp[400];
	sprintf(szTmp,"SELECT AddjValue,AddjMulti FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()!=0)
	{
		AddjValue = static_cast<float>(atof(result[0][0].c_str()));
		AddjMulti = static_cast<float>(atof(result[0][1].c_str()));
	}
}

void CSQLHelper::GetMeterType(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &meterType)
{
	meterType=0;
	std::vector<std::vector<std::string> > result;
	char szTmp[400];
	sprintf(szTmp,"SELECT SwitchType FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()!=0)
	{
		meterType=atoi(result[0][0].c_str());
	}
}

void CSQLHelper::GetAddjustment2(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti)
{
	AddjValue=0.0f;
	AddjMulti=1.0f;
	std::vector<std::vector<std::string> > result;
	char szTmp[400];
	sprintf(szTmp,"SELECT AddjValue2,AddjMulti2 FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()!=0)
	{
		AddjValue = static_cast<float>(atof(result[0][0].c_str()));
		AddjMulti = static_cast<float>(atof(result[0][1].c_str()));
	}
}

void CSQLHelper::UpdatePreferencesVar(const std::string &Key, const std::string &sValue)
{
	UpdatePreferencesVar(Key, 0, sValue);
}

void CSQLHelper::UpdatePreferencesVar(const std::string &Key, const int nValue)
{
	UpdatePreferencesVar(Key, nValue, "");
}

void CSQLHelper::UpdatePreferencesVar(const std::string &Key, const int nValue, const std::string &sValue)
{
	if (!m_dbase)
		return;

	std::stringstream sstr;

	std::vector<std::vector<std::string> > result;
	sstr << "SELECT ROWID FROM Preferences WHERE (Key='" << Key << "')";
	result = query(sstr.str());
	if (result.size() == 0)
	{
		//Insert
		sstr.clear();
		sstr.str("");
		sstr << "INSERT INTO Preferences (Key, nValue, sValue) VALUES ('" << Key << "','" << nValue << "','" << sValue << "')";
		result = query(sstr.str());
	}
	else
	{
		//Update
		sstr.clear();
		sstr.str("");
		sstr << "UPDATE Preferences SET Key='" << Key << "', nValue=" << nValue << ", sValue='" << sValue << "' WHERE (ROWID = " << result[0][0] << ")";
		result = query(sstr.str());
	}
}

bool CSQLHelper::GetPreferencesVar(const std::string &Key, std::string &sValue)
{
	if (!m_dbase)
		return false;

	std::stringstream sstr;

	std::vector<std::vector<std::string> > result;
	sstr << "SELECT sValue FROM Preferences WHERE (Key='" << Key << "')";
	result = query(sstr.str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	sValue = sd[0];
	return true;
}

bool CSQLHelper::GetPreferencesVar(const std::string &Key, int &nValue, std::string &sValue)
{
	if (!m_dbase)
		return false;

	std::stringstream sstr;

	std::vector<std::vector<std::string> > result;
	sstr << "SELECT nValue, sValue FROM Preferences WHERE (Key='" << Key << "')";
	result = query(sstr.str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	nValue = atoi(sd[0].c_str());
	sValue = sd[1];
	return true;
}

bool CSQLHelper::GetPreferencesVar(const std::string &Key, int &nValue)
{
	std::string sValue;
	return GetPreferencesVar(Key, nValue, sValue);
}

int CSQLHelper::GetLastBackupNo(const char *Key, int &nValue)
{
	if (!m_dbase)
		return false;

	char szTmp[200];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT nValue FROM BackupLog WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()<1)
		return -1;
	std::vector<std::string> sd=result[0];
	nValue=atoi(sd[0].c_str());
	return nValue;
}

void CSQLHelper::SetLastBackupNo(const char *Key, const int nValue)
{
	if (!m_dbase)
		return;

	char szTmp[600];

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ROWID FROM BackupLog WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()==0)
	{
		//Insert
		sprintf(szTmp,
			"INSERT INTO BackupLog (Key, nValue) "
			"VALUES ('%s','%d')",
			Key,
			nValue);
		result=query(szTmp);
	}
	else
	{
		//Update
		std::stringstream s_str( result[0][0] );
		s_str >> ID;

		sprintf(szTmp,
			"UPDATE BackupLog SET Key='%s', nValue=%d "
			"WHERE (ROWID = %llu)",
			Key,
			nValue,
			ID);
		result = query(szTmp);
	}
}

void CSQLHelper::UpdateRFXCOMHardwareDetails(const int HardwareID, const int msg1, const int msg2, const int msg3, const int msg4, const int msg5, const int msg6)
{
	std::stringstream szQuery;
	szQuery << "UPDATE Hardware SET Mode1=" << msg1 << ", Mode2=" << msg2 << ", Mode3=" << msg3 << ", Mode4=" << msg4 << ", Mode5=" << msg5 << ", Mode6=" << msg6 << " WHERE (ID == " << HardwareID << ")";
	query(szQuery.str());
}

bool CSQLHelper::CheckAndHandleTempHumidityNotification(
	const int HardwareID, 
	const std::string &ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const float temp, 
	const int humidity, 
	const bool bHaveTemp, 
	const bool bHaveHumidity)
{
	if (!m_dbase)
		return false;

	char szTmp[600];

	unsigned long long ulID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::stringstream s_str( result[0][0] );
	s_str >> ulID;
	std::string devicename=result[0][1];

	std::vector<_tNotification> notifications=GetNotifications(ulID);
	if (notifications.size()==0)
		return false;

	time_t atime=mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval=12*3600;
	GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime-=nNotificationInterval;

	std::string msg="";

	std::string signtemp=Notification_Type_Desc(NTYPE_TEMPERATURE,1);
	std::string signhum=Notification_Type_Desc(NTYPE_HUMIDITY,1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt=notifications.begin(); itt!=notifications.end(); ++itt)
	{
		if (atime>=itt->LastSend)
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size()<3)
				continue; //impossible
			std::string ntype=splitresults[0];
			bool bWhenIsGreater = (splitresults[1]==">");
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));
			if (m_tempunit==TEMPUNIT_F)
			{
				//Convert to Celsius
				svalue=(svalue/1.8f)-32.0f;
			}

			bool bSendNotification=false;

			if ((ntype==signtemp)&&(bHaveTemp))
			{
				//temperature
				if (bWhenIsGreater)
				{
					if (temp>svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s temperature is %.1f degrees", devicename.c_str(), temp);
						msg=szTmp;
					}
				}
				else
				{
					if (temp<svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s temperature is %.1f degrees", devicename.c_str(), temp);
						msg=szTmp;
					}
				}
			}
			else if ((ntype==signhum)&&(bHaveHumidity))
			{
				//humanity
				if (bWhenIsGreater)
				{
					if (humidity>svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Humidity is %d %%", devicename.c_str(), humidity);
						msg=szTmp;
					}
				}
				else
				{
					if (humidity<svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Humidity is %d %%", devicename.c_str(), humidity);
						msg=szTmp;
					}
				}
			}
			if (bSendNotification)
			{
				m_notifications.SendMessageEx(NOTIFYALL, std::string(""), msg, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CSQLHelper::CheckAndHandleDewPointNotification(
	const int HardwareID, 
	const std::string &ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const float temp,
	const float dewpoint)
{
	if (!m_dbase)
		return false;

	char szTmp[600];

	unsigned long long ulID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::stringstream s_str( result[0][0] );
	s_str >> ulID;
	std::string devicename=result[0][1];

	std::vector<_tNotification> notifications=GetNotifications(ulID);
	if (notifications.size()==0)
		return false;

	time_t atime=mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval=12*3600;
	GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime-=nNotificationInterval;

	std::string msg="";

	std::string signdewpoint=Notification_Type_Desc(NTYPE_DEWPOINT,1);

	std::vector<_tNotification>::const_iterator itt;
	for (itt=notifications.begin(); itt!=notifications.end(); ++itt)
	{
		if (atime>=itt->LastSend)
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible
			std::string ntype=splitresults[0];

			bool bSendNotification=false;

			if (ntype==signdewpoint)
			{
				//dewpoint
				if (temp<=dewpoint)
				{
					bSendNotification=true;
					sprintf(szTmp,"%s Dew Point reached (%.1f degrees)", devicename.c_str(), temp);
					msg=szTmp;
				}
			}
			if (bSendNotification)
			{
				m_notifications.SendMessageEx(NOTIFYALL, std::string(""), msg, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CSQLHelper::CheckAndHandleAmpere123Notification(
	const int HardwareID, 
	const std::string &ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const float Ampere1,
	const float Ampere2,
	const float Ampere3)
{
	if (!m_dbase)
		return false;

	char szTmp[600];

	unsigned long long ulID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::stringstream s_str( result[0][0] );
	s_str >> ulID;
	std::string devicename=result[0][1];

	std::vector<_tNotification> notifications=GetNotifications(ulID);
	if (notifications.size()==0)
		return false;

	time_t atime=mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval=12*3600;
	GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime-=nNotificationInterval;

	std::string msg="";

	std::string signamp1=Notification_Type_Desc(NTYPE_AMPERE1,1);
	std::string signamp2=Notification_Type_Desc(NTYPE_AMPERE2,2);
	std::string signamp3=Notification_Type_Desc(NTYPE_AMPERE3,3);

	std::vector<_tNotification>::const_iterator itt;
	for (itt=notifications.begin(); itt!=notifications.end(); ++itt)
	{
		if (atime>=itt->LastSend)
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size()<3)
				continue; //impossible
			std::string ntype=splitresults[0];
			bool bWhenIsGreater = (splitresults[1]==">");
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));

			bool bSendNotification=false;

			if (ntype==signamp1)
			{
				//Ampere1
				if (bWhenIsGreater)
				{
					if (Ampere1>svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Ampere1 is %.1f Ampere", devicename.c_str(), Ampere1);
						msg=szTmp;
					}
				}
				else
				{
					if (Ampere1<svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Ampere1 is %.1f Ampere", devicename.c_str(), Ampere1);
						msg=szTmp;
					}
				}
			}
			else if (ntype==signamp2)
			{
				//Ampere2
				if (bWhenIsGreater)
				{
					if (Ampere2>svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Ampere2 is %.1f Ampere", devicename.c_str(), Ampere2);
						msg=szTmp;
					}
				}
				else
				{
					if (Ampere2<svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Ampere2 is %.1f Ampere", devicename.c_str(), Ampere2);
						msg=szTmp;
					}
				}
			}
			else if (ntype==signamp3)
			{
				//Ampere3
				if (bWhenIsGreater)
				{
					if (Ampere3>svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Ampere3 is %.1f Ampere", devicename.c_str(), Ampere3);
						msg=szTmp;
					}
				}
				else
				{
					if (Ampere3<svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s Ampere3 is %.1f Ampere", devicename.c_str(), Ampere3);
						msg=szTmp;
					}
				}
			}
			if (bSendNotification)
			{
				m_notifications.SendMessageEx(NOTIFYALL, std::string(""), msg, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CSQLHelper::CheckAndHandleNotification(
	const int HardwareID, 
	const std::string &ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const _eNotificationTypes ntype, 
	const float mvalue)
{
	if (!m_dbase)
		return false;

	char szTmp[600];

	unsigned long long ulID=0;
	double intpart;
	std::string pvalue;
	if (modf (mvalue, &intpart)==0)
		sprintf(szTmp,"%.0f",mvalue);
	else
		sprintf(szTmp,"%.1f",mvalue);
	pvalue=szTmp;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID, Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::stringstream s_str( result[0][0] );
	s_str >> ulID;
	std::string devicename=result[0][1];

	std::vector<_tNotification> notifications=GetNotifications(ulID);
	if (notifications.size()==0)
		return false;

	time_t atime=mytime(NULL);

	//check if not sent 12 hours ago, and if applicable

	int nNotificationInterval=12*3600;
	GetPreferencesVar("NotificationSensorInterval", nNotificationInterval);
	atime-=nNotificationInterval;

	std::string msg="";

	std::string ltype=Notification_Type_Desc(ntype,0);
	std::string nsign=Notification_Type_Desc(ntype,1);
	std::string label=Notification_Type_Label(ntype);

	std::vector<_tNotification>::const_iterator itt;
	for (itt=notifications.begin(); itt!=notifications.end(); ++itt)
	{
		if (atime>=itt->LastSend)
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size()<3)
				continue; //impossible
			std::string ntype=splitresults[0];
			bool bWhenIsGreater = (splitresults[1]==">");
			float svalue = static_cast<float>(atof(splitresults[2].c_str()));

			bool bSendNotification=false;

			if (ntype==nsign)
			{
				if (bWhenIsGreater)
				{
					if (mvalue>svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s %s is %s %s", 
							devicename.c_str(),
							ltype.c_str(),
							pvalue.c_str(),
							label.c_str()
							);
						msg=szTmp;
					}
				}
				else
				{
					if (mvalue<svalue)
					{
						bSendNotification=true;
						sprintf(szTmp,"%s %s is %s %s", 
							devicename.c_str(),
							ltype.c_str(),
							pvalue.c_str(),
							label.c_str()
							);
						msg=szTmp;
					}
				}
			}
			if (bSendNotification)
			{
				m_notifications.SendMessageEx(NOTIFYALL, std::string(""), msg, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CSQLHelper::CheckAndHandleSwitchNotification(
	const int HardwareID, 
	const std::string &ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const _eNotificationTypes ntype)
{
	if (!m_dbase)
		return false;

	char szTmp[600];

	unsigned long long ulID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID, Name, SwitchType FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::stringstream s_str( result[0][0] );
	s_str >> ulID;
	std::string devicename=result[0][1];
	_eSwitchType switchtype=(_eSwitchType)atoi(result[0][2].c_str());

	std::vector<_tNotification> notifications=GetNotifications(ulID);
	if (notifications.size()==0)
		return false;

	std::string msg="";

	std::string ltype=Notification_Type_Desc(ntype,1);

	time_t atime=mytime(NULL);
	int nNotificationInterval=0;
	GetPreferencesVar("NotificationSwitchInterval", nNotificationInterval);
	atime-=nNotificationInterval;

	std::vector<_tNotification>::const_iterator itt;
	for (itt=notifications.begin(); itt!=notifications.end(); ++itt)
	{
		if (atime>=itt->LastSend)
		{
			std::vector<std::string> splitresults;
			StringSplit(itt->Params, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible
			std::string atype=splitresults[0];

			bool bSendNotification=false;

			if (atype==ltype)
			{
				bSendNotification=true;
				msg=devicename;
				if (ntype==NTYPE_SWITCH_ON)
				{
					switch (switchtype)
					{
					case STYPE_Doorbell:
						msg+=" pressed";
						break;
					case STYPE_Contact:
					case STYPE_DoorLock:
						msg+=" Open";
						break;
					case STYPE_Motion:
						msg+=" movement detected";
						break;
					case STYPE_SMOKEDETECTOR:
						msg+=" ALARM/FIRE !";
						break;
					default:
						msg+=" >> ON";
						break;
					}
				 
				}
				else {
					switch (switchtype)
					{
					case STYPE_DoorLock:
					case STYPE_Contact:
						msg+=" Closed";
						break;
					default:
						msg+=" >> OFF";
						break;
					}
				}
			}
			if (bSendNotification)
			{
				m_notifications.SendMessageEx(NOTIFYALL, std::string(""), msg, itt->Priority, std::string(""), true);
				TouchNotification(itt->ID);
			}
		}
	}
	return true;
}

bool CSQLHelper::CheckAndHandleRainNotification(
	const int HardwareID, 
	const std::string &ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const _eNotificationTypes ntype, 
	const float mvalue)
{
	if (!m_dbase)
		return false;

	char szTmp[600];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,AddjValue,AddjMulti FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::string devidx=result[0][0];
	double AddjValue=atof(result[0][1].c_str());
	double AddjMulti=atof(result[0][2].c_str());

	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	if (subType!=sTypeRAINWU)
	{
		std::stringstream szQuery;
		szQuery << "SELECT MIN(Total) FROM Rain WHERE (DeviceRowID=" << devidx << " AND Date>='" << szDateEnd << "')";
		result=query(szQuery.str());
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float total_min = static_cast<float>(atof(sd[0].c_str()));
			float total_max=mvalue;
			double total_real=total_max-total_min;
			total_real*=AddjMulti;

			CheckAndHandleNotification(HardwareID, ID, unit, devType, subType, NTYPE_RAIN, (float)total_real);
		}
	}
	else
	{
		//value is already total rain
		double total_real=mvalue;
		total_real*=AddjMulti;
		CheckAndHandleNotification(HardwareID, ID, unit, devType, subType, NTYPE_RAIN, (float)total_real);
	}
	return false;
}

void CSQLHelper::TouchNotification(const unsigned long long ID)
{
	char szTmp[300];
	char szDate[100];
	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime,&ltime);
	sprintf(szDate,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);

	//Set LastSend date
	sprintf(szTmp,
		"UPDATE Notifications SET LastSend='%s' WHERE (ID = %llu)",szDate,ID);
	query(szTmp);
}

bool CSQLHelper::AddNotification(const std::string &DevIdx, const std::string &Param, const int Priority)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;

	//First check for duplicate, because we do not want this
	szQuery << "SELECT ROWID FROM Notifications WHERE (DeviceRowID==" << DevIdx << ") AND (Params=='" << Param << "')";
	result=query(szQuery.str());
	if (result.size()>0)
		return false;//already there!

	szQuery.clear();
	szQuery.str("");
	szQuery << "INSERT INTO Notifications (DeviceRowID, Params, Priority) VALUES ('" << DevIdx << "','" << Param << "','" << Priority << "')";
	result=query(szQuery.str());
	return true;
}

bool CSQLHelper::RemoveDeviceNotifications(const std::string &DevIdx)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result=query(szQuery.str());
	return true;
}

bool CSQLHelper::RemoveNotification(const std::string &ID)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM Notifications WHERE (ID==" << ID << ")";
	result=query(szQuery.str());
	return true;
}


std::vector<_tNotification> CSQLHelper::GetNotifications(const unsigned long long DevIdx)
{
	std::vector<_tNotification> ret;
	if (!m_dbase)
		return ret;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID, Params, Priority, LastSend FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result=query(szQuery.str());
	if (result.size()==0)
		return ret;

	time_t mtime=mytime(NULL);
	struct tm atime;
	localtime_r(&mtime,&atime);

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;

		_tNotification notification;
		std::stringstream s_str( sd[0] );
		s_str >> notification.ID;

		notification.Params=sd[1];
		notification.Priority=atoi(sd[2].c_str());

		std::string stime=sd[3];
		if (stime=="0")
		{
			notification.LastSend=0;
		}
		else
		{
			struct tm ntime;
			ntime.tm_isdst=atime.tm_isdst;
			ntime.tm_year=atoi(stime.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(stime.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(stime.substr(8,2).c_str());
			ntime.tm_hour=atoi(stime.substr(11,2).c_str());
			ntime.tm_min=atoi(stime.substr(14,2).c_str());
			ntime.tm_sec=atoi(stime.substr(17,2).c_str());
			notification.LastSend=mktime(&ntime);
		}

		ret.push_back(notification);
	}
	return ret;
}

std::vector<_tNotification> CSQLHelper::GetNotifications(const std::string &DevIdx)
{
	std::stringstream s_str( DevIdx );
	unsigned long long idxll;
	s_str >> idxll;
	return GetNotifications(idxll);
}

bool CSQLHelper::HasNotifications(const unsigned long long DevIdx)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery << "SELECT COUNT(*) FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result=query(szQuery.str());
	if (result.size()==0)
		return false;
	std::vector<std::string> sd=result[0];
	int totnotifications=atoi(sd[0].c_str());
	return (totnotifications>0);
}
bool CSQLHelper::HasNotifications(const std::string &DevIdx)
{
	std::stringstream s_str( DevIdx );
	unsigned long long idxll;
	s_str >> idxll;
	return HasNotifications(idxll);
}

bool CSQLHelper::HasTimers(const unsigned long long Idx)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery << "SELECT COUNT(*) FROM Timers WHERE (DeviceRowID==" << Idx << ") AND (TimerPlan==" << m_ActiveTimerPlan << ")";
	result=query(szQuery.str());
	if (result.size() != 0)
	{
		std::vector<std::string> sd = result[0];
		int totaltimers = atoi(sd[0].c_str());
		if (totaltimers > 0)
			return true;
	}
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT COUNT(*) FROM SetpointTimers WHERE (DeviceRowID==" << Idx << ") AND (TimerPlan==" << m_ActiveTimerPlan << ")";
	result = query(szQuery.str());
	if (result.size() != 0)
	{
		std::vector<std::string> sd = result[0];
		int totaltimers = atoi(sd[0].c_str());
		return (totaltimers > 0);
	}
	return false;
}

bool CSQLHelper::HasTimers(const std::string &Idx)
{
	std::stringstream s_str( Idx );
	unsigned long long idxll;
	s_str >> idxll;
	return HasTimers(idxll);
}

bool CSQLHelper::HasSceneTimers(const unsigned long long Idx)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery << "SELECT COUNT(*) FROM SceneTimers WHERE (SceneRowID==" << Idx << ") AND (TimerPlan==" << m_ActiveTimerPlan << ")";
	result=query(szQuery.str());
	if (result.size()==0)
		return false;
	std::vector<std::string> sd=result[0];
	int totaltimers=atoi(sd[0].c_str());
	return (totaltimers>0);
}

bool CSQLHelper::HasSceneTimers(const std::string &Idx)
{
	std::stringstream s_str( Idx );
	unsigned long long idxll;
	s_str >> idxll;
	return HasSceneTimers(idxll);
}

void CSQLHelper::Schedule5Minute()
{
	if (!m_dbase)
		return;

	try
	{
		//Force WAL flush
		sqlite3_wal_checkpoint(m_dbase, NULL);

		UpdateTemperatureLog();
		UpdateRainLog();
		UpdateWindLog();
		UpdateUVLog();
		UpdateMeter();
		UpdateMultiMeter();
		UpdatePercentageLog();
		UpdateFanLog();
		//Removing the line below could cause a very large database,
		//and slow(large) data transfer (specially when working remote!!)
		CleanupShortLog();
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "Domoticz: Error running the 5 minute schedule script!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return;
	}
}

void CSQLHelper::ScheduleDay()
{
	if (!m_dbase)
		return;

	try
	{
		//Force WAL flush
		sqlite3_wal_checkpoint(m_dbase, NULL);

		AddCalendarTemperature();
		AddCalendarUpdateRain();
		AddCalendarUpdateUV();
		AddCalendarUpdateWind();
		AddCalendarUpdateMeter();
		AddCalendarUpdateMultiMeter();
		AddCalendarUpdatePercentage();
		AddCalendarUpdateFan();
		CleanupLightLog();
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "Domoticz: Error running the daily minute schedule script!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return;
	}
}

void CSQLHelper::UpdateTemperatureLog()
{
	char szTmp[600];

	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR (Type=%d AND SubType=%d) OR (Type=%d AND SubType=%d) OR (Type=%d AND SubType=%d))",
		pTypeTEMP,
		pTypeHUM,
		pTypeTEMP_HUM,
		pTypeTEMP_HUM_BARO,
		pTypeTEMP_BARO,
		pTypeUV,
		pTypeWIND,
		pTypeThermostat1,
		pTypeRFXSensor,
		pTypeRego6XXTemp,
		pTypeEvohomeZone,
		pTypeEvohomeWater,
		pTypeGeneral,sTypeSystemTemp,
		pTypeThermostat,sTypeThermSetpoint,
		pTypeGeneral, sTypeBaro
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			float temp=0;
			float chill=0;
			unsigned char humidity=0;
			int barometer=0;
			float dewpoint=0;
			float setpoint=0;

			switch (dType)
			{
			case pTypeRego6XXTemp:
			case pTypeTEMP:
			case pTypeThermostat:
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				break;
			case pTypeThermostat1:
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				break;
			case pTypeEvohomeWater:
				if (splitresults.size()>=2)
				{
					temp=static_cast<float>(atof(splitresults[0].c_str()));
					setpoint=static_cast<float>((splitresults[1]=="On")?60:0);
					//FIXME hack setpoint just on or off...may throw graph out so maybe pick sensible on off values? 
					//(if the actual hw set point was retrievable should use that otherwise some config option)
					//actually if we plot the average it should give us an idea of how often hw has been switched on
					//more meaningful if it was plotted against the zone valve & boiler relay i guess (actual time hw heated)
				}
				break;
			case pTypeEvohomeZone:
				if (splitresults.size()>=2)
				{
					temp=static_cast<float>(atof(splitresults[0].c_str()));
					setpoint=static_cast<float>(atof(splitresults[1].c_str()));
				}
				break;
			case pTypeHUM:
				humidity=nValue;
				break;
			case pTypeTEMP_HUM:
				if (splitresults.size()>=2)
				{
					temp = static_cast<float>(atof(splitresults[0].c_str()));
					humidity=atoi(splitresults[1].c_str());
					dewpoint=(float)CalculateDewPoint(temp,humidity);
				}
				break;
			case pTypeTEMP_HUM_BARO:
				if (splitresults.size()==5)
				{
					temp = static_cast<float>(atof(splitresults[0].c_str()));
					humidity=atoi(splitresults[1].c_str());
					if (dSubType==sTypeTHBFloat)
						barometer=int(atof(splitresults[3].c_str())*10.0f);
					else
						barometer=atoi(splitresults[3].c_str());
					dewpoint=(float)CalculateDewPoint(temp,humidity);
				}
				break;
			case pTypeTEMP_BARO:
				if (splitresults.size()>=2)
				{
					temp = static_cast<float>(atof(splitresults[0].c_str()));
					barometer=int(atof(splitresults[1].c_str())*10.0f);
				}
				break;
			case pTypeUV:
				if (dSubType!=sTypeUV3)
					continue;
				if (splitresults.size()>=2)
				{
					temp = static_cast<float>(atof(splitresults[1].c_str()));
				}
				break;
			case pTypeWIND:
				if ((dSubType!=sTypeWIND4)&&(dSubType!=sTypeWINDNoTemp))
					continue;
				if (splitresults.size()>=6)
				{
					temp = static_cast<float>(atof(splitresults[4].c_str()));
					chill = static_cast<float>(atof(splitresults[5].c_str()));
				}
				break;
			case pTypeRFXSensor:
				if (dSubType!=sTypeRFXSensorTemp)
					continue;
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				break;
			case pTypeGeneral:
				if (dSubType == sTypeSystemTemp)
				{
					temp = static_cast<float>(atof(splitresults[0].c_str()));
				}
				else if (dSubType == sTypeBaro)
				{
					if (splitresults.size() != 2)
						continue;
					barometer = int(atof(splitresults[0].c_str())*10.0f);
				}
				break;
			}
			//insert record
			sprintf(szTmp,
				"INSERT INTO Temperature (DeviceRowID, Temperature, Chill, Humidity, Barometer, DewPoint, SetPoint) "
				"VALUES ('%llu', '%.2f', '%.2f', '%d', '%d', '%.2f', '%.2f')",
				ID,
				temp,
				chill,
				humidity,
				barometer,
				dewpoint,
				setpoint
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdateRainLog()
{
	char szTmp[600];
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d)",
		pTypeRAIN
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<2)
				continue; //impossible

			float total=0;
			int rate=0;

			rate=atoi(splitresults[0].c_str());
			total = static_cast<float>(atof(splitresults[1].c_str()));

			//insert record
			sprintf(szTmp,
				"INSERT INTO Rain (DeviceRowID, Total, Rate) "
				"VALUES ('%llu', '%.2f', '%d')",
				ID,
				total,
				rate
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdateWindLog()
{
	char szTmp[600];
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,DeviceID, Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d)", pTypeWIND);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			unsigned short DeviceID;
			std::stringstream s_str2(sd[1]);
			s_str2 >> DeviceID;

			unsigned char dType=atoi(sd[2].c_str());
			unsigned char dSubType=atoi(sd[3].c_str());
			int nValue=atoi(sd[4].c_str());
			std::string sValue=sd[5];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[6];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<4)
				continue; //impossible

			float direction = static_cast<float>(atof(splitresults[0].c_str()));

			int speed = atoi(splitresults[2].c_str());
			int gust = atoi(splitresults[3].c_str());

			std::map<unsigned short, _tWindCalculationStruct>::iterator itt = m_mainworker.m_wind_calculator.find(DeviceID);
			if (itt != m_mainworker.m_wind_calculator.end())
			{
				int speed_max, gust_max, speed_min, gust_min;
				itt->second.GetMMSpeedGust(speed_min, speed_max, gust_min, gust_max);
				if (speed_max != -1)
					speed = speed_max;
				if (gust_max != -1)
					gust = gust_max;
			}


			//insert record
			sprintf(szTmp,
				"INSERT INTO Wind (DeviceRowID, Direction, Speed, Gust) "
				"VALUES ('%llu', '%.2f', '%d', '%d')",
				ID,
				direction,
				speed,
				gust
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdateUVLog()
{
	char szTmp[600];
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d)", pTypeUV);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			float level = static_cast<float>(atof(splitresults[0].c_str()));

			//insert record
			sprintf(szTmp,
				"INSERT INTO UV (DeviceRowID, Level) "
				"VALUES ('%llu', '%.1f')",
				ID,
				level
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdateMeter()
{
	char szTmp[700];
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	char szDateToday[100];
	sprintf(szDateToday,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> > result2;

	sprintf(szTmp,
		"SELECT ID,HardwareID,DeviceID,Unit,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE ("
		"Type=%d OR " //pTypeRFXMeter
		"Type=%d OR " //pTypeP1Gas
		"Type=%d OR " //pTypeYouLess
		"Type=%d OR " //pTypeENERGY
		"Type=%d OR " //pTypePOWER
		"Type=%d OR " //pTypeAirQuality
		"Type=%d OR " //pTypeUsage
		"Type=%d OR " //pTypeLux
		"Type=%d OR " //pTypeWEIGHT
		"(Type=%d AND SubType=%d) OR " //pTypeRego6XXValue,sTypeRego6XXCounter
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypeVisibility
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypeSolarRadiation
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypeSoilMoisture
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypeLeafWetness
		"(Type=%d AND SubType=%d) OR " //pTypeRFXSensor,sTypeRFXSensorAD
		"(Type=%d AND SubType=%d) OR" //pTypeRFXSensor,sTypeRFXSensorVolt
		"(Type=%d AND SubType=%d) OR"  //pTypeGeneral,sTypeVoltage
		"(Type=%d AND SubType=%d) OR"  //pTypeGeneral,sTypeCurrent
		"(Type=%d AND SubType=%d) OR"  //pTypeGeneral,sTypeSoundLevel
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypeDistance
		"(Type=%d AND SubType=%d)"	  //pTypeGeneral,sTypePressure
		")",
		pTypeRFXMeter,
		pTypeP1Gas,
		pTypeYouLess,
		pTypeENERGY,
		pTypePOWER,
		pTypeAirQuality,
		pTypeUsage,
		pTypeLux,
		pTypeWEIGHT,
		pTypeRego6XXValue,sTypeRego6XXCounter,
		pTypeGeneral,sTypeVisibility,
		pTypeGeneral,sTypeSolarRadiation,
		pTypeGeneral,sTypeSoilMoisture,
		pTypeGeneral,sTypeLeafWetness,
		pTypeRFXSensor,sTypeRFXSensorAD,
		pTypeRFXSensor,sTypeRFXSensorVolt,
		pTypeGeneral, sTypeVoltage,
		pTypeGeneral, sTypeCurrent,
		pTypeGeneral, sTypeSoundLevel,
		pTypeGeneral, sTypeDistance,
		pTypeGeneral, sTypePressure
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			int hardwareID= atoi(sd[1].c_str());
			std::string DeviceID=sd[2];
			unsigned char Unit = atoi(sd[3].c_str());

			unsigned char dType=atoi(sd[4].c_str());
			unsigned char dSubType=atoi(sd[5].c_str());
			int nValue=atoi(sd[6].c_str());
			std::string sValue=sd[7];
			std::string sLastUpdate=sd[8];

			std::string susage="0";

			//do not include sensors that have no reading within an hour
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);

			if (dType!=pTypeP1Gas)
			{
				if (now-checktime>=SensorTimeOut*60)
					continue;
			}
			else
			{
				//P1 Gas meter transmits results every 1 a 2 hours
				if (now-checktime>=3*3600)
					continue;
			}
			if (dType==pTypeYouLess)
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size()<2)
					continue;
				sValue=splitresults[0];
				susage = splitresults[1];
			}
			else if (dType==pTypeENERGY)
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size()<2)
					continue;
				susage=splitresults[0];
				double fValue=atof(splitresults[1].c_str())*100;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if (dType==pTypePOWER)
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size()<2)
					continue;
				susage=splitresults[0];
				double fValue=atof(splitresults[1].c_str())*100;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if (dType==pTypeAirQuality)
			{
				sprintf(szTmp,"%d",nValue);
				sValue=szTmp;
				CheckAndHandleNotification(hardwareID, DeviceID, Unit, dType, dSubType, NTYPE_USAGE, (float)nValue);
			}
			else if ((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))
			{
				sprintf(szTmp,"%d",nValue);
				sValue=szTmp;
			}
			else if ((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
			{
				double fValue = atof(sValue.c_str())*10.0f;
				sprintf(szTmp, "%d", int(fValue));
				sValue = szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))
			{
				double fValue = atof(sValue.c_str())*10.0f;
				sprintf(szTmp, "%d", int(fValue));
				sValue = szTmp;
			}
			else if (dType == pTypeLux)
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if (dType==pTypeWEIGHT)
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}
			else if (dType==pTypeRFXSensor)
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}
			else if ((dType==pTypeGeneral)&&(dSubType==sTypeVoltage))
			{
				double fValue=atof(sValue.c_str())*1000.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
			{
				double fValue = atof(sValue.c_str())*1000.0f;
				sprintf(szTmp, "%d", int(fValue));
				sValue = szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypePressure))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}
			else if (dType == pTypeUsage)
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
			}

			unsigned long long MeterValue;
			std::stringstream s_str2( sValue );
			s_str2 >> MeterValue;

			unsigned long long MeterUsage;
			std::stringstream s_str3( susage );
			s_str3 >> MeterUsage;

			//insert record
			sprintf(szTmp,
				"INSERT INTO Meter (DeviceRowID, Value, [Usage]) "
				"VALUES ('%llu', '%llu', '%llu')",
				ID,
				MeterValue,
				MeterUsage
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdateMultiMeter()
{
	char szTmp[600];
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d)",
		pTypeP1Power,
		pTypeCURRENT,
		pTypeCURRENTENERGY
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;
			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);

			unsigned long long value1=0;
			unsigned long long value2=0;
			unsigned long long value3=0;
			unsigned long long value4=0;
			unsigned long long value5=0;
			unsigned long long value6=0;

			if (dType==pTypeP1Power)
			{
				if (splitresults.size()!=6)
					continue; //impossible
				unsigned long long powerusage1;
				unsigned long long powerusage2;
				unsigned long long powerdeliv1;
				unsigned long long powerdeliv2;
				unsigned long long usagecurrent;
				unsigned long long delivcurrent;

				std::stringstream s_powerusage1(splitresults[0]);
				std::stringstream s_powerusage2(splitresults[1]);
				std::stringstream s_powerdeliv1(splitresults[2]);
				std::stringstream s_powerdeliv2(splitresults[3]);
				std::stringstream s_usagecurrent(splitresults[4]);
				std::stringstream s_delivcurrent(splitresults[5]);

				s_powerusage1 >> powerusage1;
				s_powerusage2 >> powerusage2;
				s_powerdeliv1 >> powerdeliv1;
				s_powerdeliv2 >> powerdeliv2;
				s_usagecurrent >> usagecurrent;
				s_delivcurrent >> delivcurrent;

				value1=powerusage1;
				value2=powerdeliv1;
				value5=powerusage2;
				value6=powerdeliv2;
				value3=usagecurrent;
				value4=delivcurrent;
			}
			else if ((dType==pTypeCURRENT)&&(dSubType==sTypeELEC1))
			{
				if (splitresults.size()!=3)
					continue; //impossible

				value1=(unsigned long)(atof(splitresults[0].c_str())*10.0f);
				value2=(unsigned long)(atof(splitresults[1].c_str())*10.0f);
				value3=(unsigned long)(atof(splitresults[2].c_str())*10.0f);
			}
			else if ((dType==pTypeCURRENTENERGY)&&(dSubType==sTypeELEC4))
			{
				if (splitresults.size()!=4)
					continue; //impossible

				value1=(unsigned long)(atof(splitresults[0].c_str())*10.0f);
				value2=(unsigned long)(atof(splitresults[1].c_str())*10.0f);
				value3=(unsigned long)(atof(splitresults[2].c_str())*10.0f);
				value4=(unsigned long)(atof(splitresults[3].c_str())*1000.0f);
			}
			else
				continue;//don't know you (yet)

			//insert record
			sprintf(szTmp,
				"INSERT INTO MultiMeter (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6) "
				"VALUES ('%llu', '%llu', '%llu', '%llu', '%llu', '%llu', '%llu')",
				ID,
				value1,
				value2,
				value3,
				value4,
				value5,
				value6
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdatePercentageLog()
{
	char szTmp[600];

	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d AND SubType=%d)",
		pTypeGeneral,sTypePercentage
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			float percentage = static_cast<float>(atof(sValue.c_str()));

			//insert record
			sprintf(szTmp,
				"INSERT INTO Percentage (DeviceRowID, Percentage) "
				"VALUES ('%llu', '%.2f')",
				ID,
				percentage
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}

void CSQLHelper::UpdateFanLog()
{
	char szTmp[600];

	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d AND SubType=%d)",
		pTypeGeneral,sTypeFan
		);
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			ntime.tm_isdst=tm1.tm_isdst;
			ntime.tm_year=atoi(sLastUpdate.substr(0,4).c_str())-1900;
			ntime.tm_mon=atoi(sLastUpdate.substr(5,2).c_str())-1;
			ntime.tm_mday=atoi(sLastUpdate.substr(8,2).c_str());
			ntime.tm_hour=atoi(sLastUpdate.substr(11,2).c_str());
			ntime.tm_min=atoi(sLastUpdate.substr(14,2).c_str());
			ntime.tm_sec=atoi(sLastUpdate.substr(17,2).c_str());
			time_t checktime=mktime(&ntime);
			if (now-checktime>=SensorTimeOut*60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			int speed= (int)atoi(sValue.c_str());

			//insert record
			sprintf(szTmp,
				"INSERT INTO Fan (DeviceRowID, Speed) "
				"VALUES ('%llu', '%d')",
				ID,
				speed
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
}


void CSQLHelper::AddCalendarTemperature()
{
	char szTmp[600];

	//Get All temperature devices in the Temperature Table
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Temperature ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		sprintf(szTmp,"SELECT MIN(Temperature), MAX(Temperature), AVG(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer), MIN(DewPoint), MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) FROM Temperature WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float temp_min = static_cast<float>(atof(sd[0].c_str()));
			float temp_max = static_cast<float>(atof(sd[1].c_str()));
			float temp_avg = static_cast<float>(atof(sd[2].c_str()));
			float chill_min = static_cast<float>(atof(sd[3].c_str()));
			float chill_max = static_cast<float>(atof(sd[4].c_str()));
			int humidity=atoi(sd[5].c_str());
			int barometer=atoi(sd[6].c_str());
			float dewpoint = static_cast<float>(atof(sd[7].c_str()));
			float setpoint_min=static_cast<float>(atof(sd[8].c_str()));
			float setpoint_max=static_cast<float>(atof(sd[9].c_str()));
			float setpoint_avg=static_cast<float>(atof(sd[10].c_str()));
			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Temperature_Calendar (DeviceRowID, Temp_Min, Temp_Max, Temp_Avg, Chill_Min, Chill_Max, Humidity, Barometer, DewPoint, SetPoint_Min, SetPoint_Max, SetPoint_Avg, Date) "
				"VALUES ('%llu', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%d', '%d', '%.2f', '%.2f', '%.2f', '%.2f', '%s')",
				ID,
				temp_min,
				temp_max,
				temp_avg,
				chill_min,
				chill_max,
				humidity,
				barometer,
				dewpoint,
				setpoint_min,
				setpoint_max,
				setpoint_avg,
				szDateStart
				);
			result=query(szTmp);

		}
	}
}

void CSQLHelper::AddCalendarUpdateRain()
{
	char szTmp[600];

	//Get All UV devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Rain ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		//Get Device Information
		sprintf(szTmp,"SELECT SubType FROM DeviceStatus WHERE (ID='%llu')",ID);
		result=query(szTmp);
		if (result.size()<1)
			continue;
		std::vector<std::string> sd=result[0];

		unsigned char subType=atoi(sd[0].c_str());

		if (subType!=sTypeRAINWU)
		{
			sprintf(szTmp,"SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
				ID,
				szDateStart,
				szDateEnd
				);
		}
		else
		{
			sprintf(szTmp,"SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s') ORDER BY ROWID DESC LIMIT 1",
				ID,
				szDateStart,
				szDateEnd
				);
		}

		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float total_min = static_cast<float>(atof(sd[0].c_str()));
			float total_max = static_cast<float>(atof(sd[1].c_str()));
			int rate=atoi(sd[2].c_str());

			float total_real=0;
			if (subType!=sTypeRAINWU)
			{
				total_real=total_max-total_min;
			}
			else
			{
				total_real=total_max;
			}
			

			if (total_real<1000)
			{
				//insert into calendar table
				sprintf(szTmp,
					"INSERT INTO Rain_Calendar (DeviceRowID, Total, Rate, Date) "
					"VALUES ('%llu', '%.2f', '%d', '%s')",
					ID,
					total_real,
					rate,
					szDateStart
					);
				result=query(szTmp);
			}
		}
	}
}

void CSQLHelper::AddCalendarUpdateMeter()
{
	char szTmp[600];

	float EnergyDivider=1000.0f;
	float GasDivider=100.0f;
	float WaterDivider=100.0f;
	float musage=0;
	int tValue;
	if (GetPreferencesVar("MeterDividerEnergy", tValue))
	{
		EnergyDivider=float(tValue);
	}
	if (GetPreferencesVar("MeterDividerGas", tValue))
	{
		GasDivider=float(tValue);
	}
	if (GetPreferencesVar("MeterDividerWater", tValue))
	{
		WaterDivider=float(tValue);
	}

	//Get All Meter devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Meter ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);

	//Subtract one day
	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		//Get Device Information
		sprintf(szTmp,"SELECT HardwareID, DeviceID, Unit, Type, SubType, SwitchType FROM DeviceStatus WHERE (ID='%llu')",ID);
		result=query(szTmp);
		if (result.size()<1)
			continue;
		std::vector<std::string> sd=result[0];

		int hardwareID= atoi(sd[0].c_str());
		std::string DeviceID=sd[1];
		unsigned char Unit = atoi(sd[2].c_str());
		unsigned char devType=atoi(sd[3].c_str());
		unsigned char subType=atoi(sd[4].c_str());
		_eSwitchType switchtype=(_eSwitchType) atoi(sd[5].c_str());
		_eMeterType metertype=(_eMeterType)switchtype;

		float tGasDivider=GasDivider;

		if (devType==pTypeP1Power)
		{
			metertype=MTYPE_ENERGY;
		}
		else if (devType==pTypeP1Gas)
		{
			metertype=MTYPE_GAS;
			tGasDivider=1000.0f;
		}
        else if ((devType==pTypeRego6XXValue) && (subType==sTypeRego6XXCounter))
		{
			metertype=MTYPE_COUNTER;
		}


		sprintf(szTmp,"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			double total_min=(double)atof(sd[0].c_str());
			double total_max=(double)atof(sd[1].c_str());

			if (
				(devType!=pTypeAirQuality)&&
				(devType!=pTypeRFXSensor)&&
				(!((devType==pTypeGeneral)&&(subType==sTypeVisibility)))&&
				(!((devType == pTypeGeneral) && (subType == sTypeDistance))) &&
				(!((devType == pTypeGeneral) && (subType == sTypeSolarRadiation))) &&
				(!((devType==pTypeGeneral)&&(subType==sTypeSoilMoisture)))&&
				(!((devType==pTypeGeneral)&&(subType==sTypeLeafWetness)))&&
				(!((devType == pTypeGeneral) && (subType == sTypeVoltage))) &&
				(!((devType == pTypeGeneral) && (subType == sTypeCurrent))) &&
				(!((devType == pTypeGeneral) && (subType == sTypePressure))) &&
				(!((devType == pTypeGeneral) && (subType == sTypeSoundLevel))) &&
				(devType != pTypeLux) &&
				(devType!=pTypeWEIGHT)&&
				(devType!=pTypeUsage)
				)
			{
				double total_real=total_max-total_min;
				double counter = total_max;

				//insert into calendar table
				sprintf(szTmp,
					"INSERT INTO Meter_Calendar (DeviceRowID, Value, Counter, Date) "
					"VALUES ('%llu', '%.2f', '%.2f', '%s')",
					ID,
					total_real,
					counter,
					szDateStart
					);
				result=query(szTmp);

				//Check for Notification
				musage=0;
				switch (metertype)
				{
				case MTYPE_ENERGY:
					musage=float(total_real)/EnergyDivider;
					if (musage!=0)
						CheckAndHandleNotification(hardwareID, DeviceID, Unit, devType, subType, NTYPE_TODAYENERGY, musage);
					break;
				case MTYPE_GAS:
					musage=float(total_real)/tGasDivider;
					if (musage!=0)
						CheckAndHandleNotification(hardwareID, DeviceID, Unit, devType, subType, NTYPE_TODAYGAS, musage);
					break;
				case MTYPE_WATER:
					musage=float(total_real)/WaterDivider;
					if (musage!=0)
						CheckAndHandleNotification(hardwareID, DeviceID, Unit, devType, subType, NTYPE_TODAYGAS, musage);
					break;
				case MTYPE_COUNTER:
					musage=float(total_real);
					if (musage!=0)
						CheckAndHandleNotification(hardwareID, DeviceID, Unit, devType, subType, NTYPE_TODAYCOUNTER, musage);
					break;
				}
			}
			else
			{
				//AirQuality/Usage Meter/Moisture/RFXSensor/Voltage/Lux/SoundLevel insert into MultiMeter_Calendar table
				sprintf(szTmp,
					"INSERT INTO MultiMeter_Calendar (DeviceRowID, Value1,Value2,Value3,Value4,Value5,Value6, Date) "
					"VALUES ('%llu', '%.2f','%.2f','%.2f','%.2f','%.2f','%.2f', '%s')",
					ID,
					total_min,total_max,0.0f,0.0f,0.0f,0.0f,
					szDateStart
					);
				result=query(szTmp);
			}
			if (
				(devType!=pTypeAirQuality)&&
				(devType!=pTypeRFXSensor)&&
				((devType != pTypeGeneral) && (subType != sTypeVisibility)) &&
				((devType != pTypeGeneral) && (subType != sTypeDistance)) &&
				((devType != pTypeGeneral) && (subType != sTypeSolarRadiation)) &&
				((devType != pTypeGeneral) && (subType != sTypeVoltage)) &&
				((devType != pTypeGeneral) && (subType != sTypeCurrent)) &&
				((devType != pTypeGeneral) && (subType != sTypePressure)) &&
				((devType != pTypeGeneral) && (subType != sTypeSoilMoisture)) &&
				((devType != pTypeGeneral) && (subType != sTypeLeafWetness)) &&
				((devType != pTypeGeneral) && (subType != sTypeSoundLevel)) &&
				(devType != pTypeLux) &&
				(devType!=pTypeWEIGHT)
				)
			{
				//Insert the last (max) counter value into the meter table to get the "today" value correct.
				sprintf(szTmp,
					"INSERT INTO Meter (DeviceRowID, Value, Date) "
					"VALUES ('%llu', '%s', '%s')",
					ID,
					sd[1].c_str(),
					szDateEnd
					);
				result=query(szTmp);

			}
		}
		else
		{
			//no new meter result received in last day
			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) "
				"VALUES ('%llu', '%.2f', '%s')",
				ID,
				0.0f,
				szDateStart
				);
			result=query(szTmp);
		}
	}
}

void CSQLHelper::AddCalendarUpdateMultiMeter()
{
	char szTmp[600];

	float EnergyDivider=1000.0f;
	float GasDivider=100.0f;
	float WaterDivider=100.0f;
	float musage=0;
	int tValue;
	if (GetPreferencesVar("MeterDividerEnergy", tValue))
	{
		EnergyDivider=float(tValue);
	}
	if (GetPreferencesVar("MeterDividerGas", tValue))
	{
		GasDivider=float(tValue);
	}
	if (GetPreferencesVar("MeterDividerWater", tValue))
	{
		WaterDivider=float(tValue);
	}

	//Get All meter devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM MultiMeter ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);

	//Subtract one day
	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		//Get Device Information
		sprintf(szTmp,"SELECT HardwareID, DeviceID, Unit, Type, SubType, SwitchType FROM DeviceStatus WHERE (ID='%llu')",ID);
		result=query(szTmp);
		if (result.size()<1)
			continue;
		std::vector<std::string> sd=result[0];

		int hardwareID= atoi(sd[0].c_str());
		std::string DeviceID=sd[1];
		unsigned char Unit = atoi(sd[2].c_str());
		unsigned char devType=atoi(sd[3].c_str());
		unsigned char subType=atoi(sd[4].c_str());
		_eSwitchType switchtype=(_eSwitchType) atoi(sd[5].c_str());
		_eMeterType metertype=(_eMeterType)switchtype;

		sprintf(szTmp,"SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2), MIN(Value3), MAX(Value3), MIN(Value4), MAX(Value4), MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float total_real[6];
			float counter1 = 0;
			float counter2 = 0;
			float counter3 = 0;
			float counter4 = 0;

			if (devType==pTypeP1Power)
			{
				for (int ii=0; ii<6; ii++)
				{
					float total_min = static_cast<float>(atof(sd[(ii * 2) + 0].c_str()));
					float total_max = static_cast<float>(atof(sd[(ii * 2) + 1].c_str()));
					total_real[ii]=total_max-total_min;
				}
				counter1 = static_cast<float>(atof(sd[1].c_str()));
				counter2 = static_cast<float>(atof(sd[3].c_str()));
				counter3 = static_cast<float>(atof(sd[9].c_str()));
				counter4 = static_cast<float>(atof(sd[11].c_str()));
			}
			else
			{
				for (int ii=0; ii<6; ii++)
				{
					float fvalue = static_cast<float>(atof(sd[ii].c_str()));
					total_real[ii]=fvalue;
				}
			}

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO MultiMeter_Calendar (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Counter1, Counter2, Counter3, Counter4, Date) "
				"VALUES ('%llu', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%s')",
				ID,
				total_real[0],
				total_real[1],
				total_real[2],
				total_real[3],
				total_real[4],
				total_real[5],
				counter1,
				counter2,
				counter3,
				counter4,
				szDateStart
				);
			result=query(szTmp);

			//Check for Notification
			if (devType==pTypeP1Power)
			{
				float musage=(total_real[0]+total_real[1])/EnergyDivider;
				CheckAndHandleNotification(hardwareID, DeviceID, Unit, devType, subType, NTYPE_TODAYENERGY, musage);
			}
/*
			//Insert the last (max) counter values into the table to get the "today" value correct.
			sprintf(szTmp,
				"INSERT INTO MultiMeter (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Date) "
				"VALUES (%llu, %s, %s, %s, %s, %s, %s, '%s')",
				ID,
				sd[0].c_str(),
				sd[1].c_str(),
				sd[2].c_str(),
				sd[3].c_str(),
				sd[4].c_str(),
				sd[5].c_str(),
				szDateEnd
				);
				result=query(szTmp);
*/
		}
	}
}

void CSQLHelper::AddCalendarUpdateWind()
{
	char szTmp[600];

	//Get All Wind devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Wind ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		sprintf(szTmp,"SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float Direction = static_cast<float>(atof(sd[0].c_str()));
			int speed_min=atoi(sd[1].c_str());
			int speed_max=atoi(sd[2].c_str());
			int gust_min=atoi(sd[3].c_str());
			int gust_max=atoi(sd[4].c_str());

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Wind_Calendar (DeviceRowID, Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date) "
				"VALUES ('%llu', '%.2f', '%d', '%d', '%d', '%d', '%s')",
				ID,
				Direction,
				speed_min,
				speed_max,
				gust_min,
				gust_max,
				szDateStart
				);
			result=query(szTmp);
		}
	}
}

void CSQLHelper::AddCalendarUpdateUV()
{
	char szTmp[600];

	//Get All UV devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM UV ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		sprintf(szTmp,"SELECT MAX(Level) FROM UV WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float level = static_cast<float>(atof(sd[0].c_str()));

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO UV_Calendar (DeviceRowID, Level, Date) "
				"VALUES ('%llu', '%.2f', '%s')",
				ID,
				level,
				szDateStart
				);
			result=query(szTmp);
		}
	}
}

void CSQLHelper::AddCalendarUpdatePercentage()
{
	char szTmp[600];

	//Get All Percentage devices in the Percentage Table
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Percentage ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		sprintf(szTmp,"SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage) FROM Percentage WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float percentage_min = static_cast<float>(atof(sd[0].c_str()));
			float percentage_max = static_cast<float>(atof(sd[1].c_str()));
			float percentage_avg = static_cast<float>(atof(sd[2].c_str()));
			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Percentage_Calendar (DeviceRowID, Percentage_Min, Percentage_Max, Percentage_Avg, Date) "
				"VALUES ('%llu', '%.2f', '%.2f', '%.2f','%s')",
				ID,
				percentage_min,
				percentage_max,
				percentage_avg,
				szDateStart
				);
			result=query(szTmp);

		}
	}
}


void CSQLHelper::AddCalendarUpdateFan()
{
	char szTmp[600];

	//Get All FAN devices in the Fan Table
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Fan ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm tm2;
	localtime_r(&later,&tm2);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		unsigned long long ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		sprintf(szTmp,"SELECT MIN(Speed), MAX(Speed), AVG(Speed) FROM Fan WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			int speed_min=(int)atoi(sd[0].c_str());
			int speed_max=(int)atoi(sd[1].c_str());
			int speed_avg=(int)atoi(sd[2].c_str());
			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Fan_Calendar (DeviceRowID, Speed_Min, Speed_Max, Speed_Avg, Date) "
				"VALUES ('%llu', '%d', '%d', '%d','%s')",
				ID,
				speed_min,
				speed_max,
				speed_avg,
				szDateStart
				);
			result=query(szTmp);

		}
	}
}


void CSQLHelper::CleanupShortLog()
{
	int n5MinuteHistoryDays=1;
	if(GetPreferencesVar("5MinuteHistoryDays", n5MinuteHistoryDays))
    {
        // If the history days is zero then all data in the short logs is deleted!
        if(n5MinuteHistoryDays == 0)
        {
            _log.Log(LOG_ERROR,"CleanupShortLog(): MinuteHistoryDays is zero!");
            return;
        }

		char szDateStr[40];
		char szTmp[200];

	    sprintf(szDateStr,"datetime('now','-%d day', 'localtime')",n5MinuteHistoryDays);

	    sprintf(szTmp,"DELETE FROM Temperature WHERE (Date<%s)",szDateStr);
	    query(szTmp);

	    sprintf(szTmp,"DELETE FROM Rain WHERE (Date<%s)",szDateStr);
	    query(szTmp);

	    sprintf(szTmp,"DELETE FROM Wind WHERE (Date<%s)",szDateStr);
	    query(szTmp);

	    sprintf(szTmp,"DELETE FROM UV WHERE (Date<%s)",szDateStr);
	    query(szTmp);

	    sprintf(szTmp,"DELETE FROM Meter WHERE (Date<%s)",szDateStr);
	    query(szTmp);

	    sprintf(szTmp,"DELETE FROM MultiMeter WHERE (Date<%s)",szDateStr);
	    query(szTmp);

		sprintf(szTmp,"DELETE FROM Percentage WHERE (Date<%s)",szDateStr);
		query(szTmp);
	
		sprintf(szTmp,"DELETE FROM Fan WHERE (Date<%s)",szDateStr);
		query(szTmp);
	}
}

void CSQLHelper::DeleteHardware(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[200];
	sprintf(szTmp,"DELETE FROM Hardware WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
	//also delete all records in other tables

	sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (HardwareID == %s)",idx.c_str());
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			DeleteDevice(sd[0]);
		}
	}
	//and now delete all records in the DeviceStatus table itself
	sprintf(szTmp,"DELETE FROM DeviceStatus WHERE (HardwareID == %s)",idx.c_str());
	result=query(szTmp);
	sprintf(szTmp,"DELETE FROM ZWaveNodes WHERE (HardwareID== %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp, "DELETE FROM EnoceanSensors WHERE (HardwareID== %s)", idx.c_str());
	query(szTmp);
	sprintf(szTmp, "DELETE FROM MySensors WHERE (HardwareID== %s)", idx.c_str());
	query(szTmp);
}

void CSQLHelper::DeleteCamera(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[200];
	sprintf(szTmp,"DELETE FROM Cameras WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
	sprintf(szTmp,"DELETE FROM CamerasActiveDevices WHERE (CameraRowID == %s)",idx.c_str());
	result=query(szTmp);
}

void CSQLHelper::DeletePlan(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[200];
	sprintf(szTmp,"DELETE FROM Plans WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
}

void CSQLHelper::DeleteEvent(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[200];
	sprintf(szTmp,"DELETE FROM EventRules WHERE (EMID == %s)",idx.c_str());
	result=query(szTmp);
	sprintf(szTmp,"DELETE FROM EventMaster WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
}

void CSQLHelper::DeleteDevice(const std::string &idx)
{
	char szTmp[200];
	sprintf(szTmp,"DELETE FROM LightingLog WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM LightSubDevices WHERE (ParentID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM LightSubDevices WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Notifications WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Rain WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Rain_Calendar WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Temperature WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Temperature_Calendar WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Timers WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp, "DELETE FROM SetpointTimers WHERE (DeviceRowID == %s)", idx.c_str());
	query(szTmp);
	sprintf(szTmp, "DELETE FROM UV WHERE (DeviceRowID == %s)", idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM UV_Calendar WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Wind WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Wind_Calendar WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Meter WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM Meter_Calendar WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM MultiMeter WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM MultiMeter_Calendar WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM SceneDevices WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM DeviceToPlansMap WHERE (DeviceRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM CamerasActiveDevices WHERE (DevSceneType==0) AND (DevSceneRowID == %s)",idx.c_str());
	query(szTmp);
	sprintf(szTmp,"DELETE FROM SharedDevices WHERE (DeviceRowID== %s)",idx.c_str());
	query(szTmp);


	//and now delete all records in the DeviceStatus table itself
	sprintf(szTmp,"DELETE FROM DeviceStatus WHERE (ID == %s)",idx.c_str());
    //notify eventsystem device is no longer present
	m_mainworker.m_eventsystem.RemoveSingleState(atoi(idx.c_str()));
    
	query(szTmp);
}

void CSQLHelper::TransferDevice(const std::string &idx, const std::string &newidx)
{
	char szTmp[400];
	std::vector<std::vector<std::string> > result;

	sprintf(szTmp,"UPDATE LightingLog SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);
	sprintf(szTmp,"UPDATE LightSubDevices SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);
	sprintf(szTmp,"UPDATE LightSubDevices SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);
	sprintf(szTmp,"UPDATE Notifications SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Rain WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Rain SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Rain SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Rain_Calendar WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Rain_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Rain_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Temperature WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Temperature SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Temperature SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);


	sprintf(szTmp,"SELECT Date FROM Temperature_Calendar WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Temperature_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Temperature_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"UPDATE Timers SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM UV WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE UV SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE UV SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM UV_Calendar WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE UV_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE UV_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Wind WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Wind SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Wind SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Wind_Calendar WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Wind_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Wind_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Meter WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Meter SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Meter SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM Meter_Calendar WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE Meter_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE Meter_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM MultiMeter WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE MultiMeter SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE MultiMeter SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);

	sprintf(szTmp,"SELECT Date FROM MultiMeter_Calendar WHERE (DeviceRowID == '%s') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	result=query(szTmp);
	if (result.size()>0)
		sprintf(szTmp,"UPDATE MultiMeter_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s') AND (Date<'%s')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		sprintf(szTmp,"UPDATE MultiMeter_Calendar SET DeviceRowID=%s WHERE (DeviceRowID == '%s')",newidx.c_str(),idx.c_str());
	query(szTmp);
}

void CSQLHelper::CheckAndUpdateDeviceOrder()
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	//Get All ID's where Order=0
	szQuery << "SELECT ROWID FROM DeviceStatus WHERE ([Order]==0)";
	result=query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE DeviceStatus SET [Order] = (SELECT MAX([Order]) FROM DeviceStatus)+1 WHERE (ROWID == " << sd[0] << ")";
			query(szQuery.str());
		}
	}
}

void CSQLHelper::CheckAndUpdateSceneDeviceOrder()
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	//Get All ID's where Order=0
	szQuery << "SELECT ROWID FROM SceneDevices WHERE ([Order]==0)";
	result=query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE SceneDevices SET [Order] = (SELECT MAX([Order]) FROM SceneDevices)+1 WHERE (ROWID == " << sd[0] << ")";
			query(szQuery.str());
		}
	}
}

void CSQLHelper::CleanupLightLog()
{
	//cleanup the lighting log
	int nMaxDays=30;
	GetPreferencesVar("LightHistoryDays", nMaxDays);

	char szDateEnd[40];
	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract one day
	ltime.tm_mday -= nMaxDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	char szTmp[100];
	sprintf(szTmp,"DELETE FROM LightingLog WHERE (Date<'%s')",
		szDateEnd
		);
	query(szTmp);
}

bool CSQLHelper::DoesSceneByNameExits(const std::string &SceneName)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	//Get All ID's where Order=0
	szQuery << "SELECT ID FROM Scenes WHERE (Name=='" << SceneName << "')";
	result=query(szQuery.str());
	return (result.size()>0);
}

void CSQLHelper::CheckSceneStatusWithDevice(const std::string &DevIdx)
{
	std::stringstream s_str( DevIdx );
	unsigned long long idxll;
	s_str >> idxll;
	return CheckSceneStatusWithDevice(idxll);
}

void CSQLHelper::CheckSceneStatusWithDevice(const unsigned long long DevIdx)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery << "SELECT SceneRowID FROM SceneDevices WHERE (DeviceRowID == " << DevIdx << ")";
	result=query(szQuery.str());
	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		CheckSceneStatus(sd[0]);
	}
}

void CSQLHelper::CheckSceneStatus(const std::string &Idx)
{
	std::stringstream s_str( Idx );
	unsigned long long idxll;
	s_str >> idxll;
	return CheckSceneStatus(idxll);
}

void CSQLHelper::CheckSceneStatus(const unsigned long long Idx)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery << "SELECT nValue FROM Scenes WHERE (ID == " << Idx << ")";
	result=query(szQuery.str());
	if (result.size()<1)
		return; //not found

	unsigned char orgValue=(unsigned char)atoi(result[0][0].c_str());
	unsigned char newValue=orgValue;

	szQuery.clear();
	szQuery.str("");

	szQuery << "SELECT a.ID, a.DeviceID, a.Unit, a.Type, a.SubType, a.SwitchType, a.nValue, a.sValue FROM DeviceStatus AS a, SceneDevices as b WHERE (a.ID == b.DeviceRowID) AND (b.SceneRowID == " << Idx << ")";
	result=query(szQuery.str());
	if (result.size()<1)
		return; //no devices in scene

	std::vector<std::vector<std::string> >::const_iterator itt;

	std::vector<bool> _DeviceStatusResults;

	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		int nValue=atoi(sd[6].c_str());
		std::string sValue=sd[7];
		unsigned char Unit=atoi(sd[2].c_str());
		unsigned char dType=atoi(sd[3].c_str());
		unsigned char dSubType=atoi(sd[4].c_str());
		_eSwitchType switchtype=(_eSwitchType)atoi(sd[5].c_str());

		std::string lstatus="";
		int llevel=0;
		bool bHaveDimmer=false;
		bool bHaveGroupCmd=false;
		int maxDimLevel=0;

		GetLightStatus(dType, dSubType, switchtype,nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
		_DeviceStatusResults.push_back(IsLightSwitchOn(lstatus));
	}

	//Check if all on/off
	int totOn=0;
	int totOff=0;

	std::vector<bool>::const_iterator itt2;
	for (itt2=_DeviceStatusResults.begin(); itt2!=_DeviceStatusResults.end(); ++itt2)
	{
		if (*itt2==true)
			totOn++;
		else
			totOff++;
	}
	if (totOn==_DeviceStatusResults.size())
	{
		//All are on
		newValue=1;
	}
	else if (totOff==_DeviceStatusResults.size())
	{
		//All are Off
		newValue=0;
	}
	else
	{
		//Some are on, some are off
		newValue=2;
	}
	if (newValue!=orgValue)
	{
		//Set new Scene status
		szQuery.clear();
		szQuery.str("");

		szQuery << "UPDATE Scenes SET nValue=" << int(newValue) << " WHERE (ID == " << Idx << ")";
		result=query(szQuery.str());
	}
}

void CSQLHelper::DeleteDataPoint(const char *ID, const std::string &Date)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[200];
	sprintf(szTmp,"SELECT Type,SubType FROM DeviceStatus WHERE (ID==%s)",ID);
	result=query(szTmp);
	if (result.size()<1)
		return;
	//std::vector<std::string> sd=result[0];
	//unsigned char dType=atoi(sd[0].c_str());
	//unsigned char dSubType=atoi(sd[1].c_str());

	if (Date.find(':')!=std::string::npos)
	{
		char szDateEnd[100];
		sprintf(szDateEnd,"datetime('%s','+2 Minute', 'localtime')",Date.c_str());
		//Short log
		sprintf(szTmp,"DELETE FROM Rain WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Wind WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM UV WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Temperature WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Meter WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM MultiMeter WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Percentage WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);

		sprintf(szTmp,"DELETE FROM Fan WHERE (DeviceRowID==%s) AND (Date>='%s') AND (Date<=%s)",ID,Date.c_str(),szDateEnd);
		result=query(szTmp);
	}
	else
	{
		sprintf(szTmp,"DELETE FROM Rain_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Wind_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM UV_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Temperature_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Meter_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM MultiMeter_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Percentage_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
		sprintf(szTmp,"DELETE FROM Fan_Calendar WHERE (DeviceRowID==%s) AND (Date=='%s')",ID,Date.c_str());
		result=query(szTmp);
	}
}

void CSQLHelper::AddTaskItem(const _tTaskItem &tItem)
{
	boost::lock_guard<boost::mutex> l(m_background_task_mutex);
	
	// Check if an event for the same device is already in queue, and if so, replace it
	// _log.Log(LOG_NORM, "Request to add task: idx=%llu, DelayTime=%d, Command='%s', Level=%d, Hue=%d, RelatedEvent='%s'", tItem._idx, tItem._DelayTime, tItem._command.c_str(), tItem._level, tItem._Hue, tItem._relatedEvent.c_str());
	// Remove any previous task linked to the same device

	if (
		(tItem._ItemType == TITEM_SWITCHCMD_EVENT) ||
		(tItem._ItemType == TITEM_SWITCHCMD_SCENE)
		)
	{
		std::vector<_tTaskItem>::iterator itt = m_background_task_queue.begin();
		while (itt != m_background_task_queue.end())
		{
			// _log.Log(LOG_NORM, "Comparing with item in queue: idx=%llu, DelayTime=%d, Command='%s', Level=%d, Hue=%d, RelatedEvent='%s'", itt->_idx, itt->_DelayTime, itt->_command.c_str(), itt->_level, itt->_Hue, itt->_relatedEvent.c_str());
			if (itt->_idx == tItem._idx)
			{
				int iDelayDiff = tItem._DelayTime - itt->_DelayTime;
				if (iDelayDiff < 3)
				{
					// _log.Log(LOG_NORM, "=> Already present. Cancelling previous task item");
					itt = m_background_task_queue.erase(itt);
				}
				else
					++itt;
			}
			else
				++itt;
		}
	}
	// _log.Log(LOG_NORM, "=> Adding new task item");  
	m_background_task_queue.push_back(tItem);
}

void CSQLHelper::EventsGetTaskItems(std::vector<_tTaskItem> &currentTasks)
{
	boost::lock_guard<boost::mutex> l(m_background_task_mutex);
    
	currentTasks.clear();
    
    for(std::vector<_tTaskItem>::iterator it = m_background_task_queue.begin(); it != m_background_task_queue.end(); ++it)
    {
		currentTasks.push_back(*it);
	}
}

bool CSQLHelper::RestoreDatabase(const std::string &dbase)
{
	//write file to disk
	std::string fpath("");
#ifdef WIN32
	size_t bpos=m_dbase_name.rfind('\\');
#else
	size_t bpos=m_dbase_name.rfind('/');
#endif
	if (bpos!=std::string::npos)
		fpath=m_dbase_name.substr(0,bpos+1);
	std::string outputfile=fpath+"restore.db";
	std::ofstream outfile;
	outfile.open(outputfile.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (!outfile.is_open())
		return false;
	outfile << dbase;
	outfile.flush();
	outfile.close();
	//check if we can open the database (check if valid)
	sqlite3 *dbase_restore=NULL;
	int rc = sqlite3_open(outputfile.c_str(), &dbase_restore);
	if (rc)
	{
		_log.Log(LOG_ERROR,"Error opening SQLite3 database: %s", sqlite3_errmsg(dbase_restore));
		sqlite3_close(dbase_restore);
		return false;
	}
	if (dbase_restore==NULL)
		return false;
	//could still be not valid
	std::stringstream ss;
	ss << "SELECT sValue FROM Preferences WHERE (Key='DB_Version')";
	sqlite3_stmt *statement;
	if(sqlite3_prepare_v2(dbase_restore, ss.str().c_str(), -1, &statement, 0) != SQLITE_OK)
	{
		sqlite3_close(dbase_restore);
		return false;
	}
	sqlite3_close(dbase_restore);
	//we have a valid database!
	std::remove(outputfile.c_str());
	//stop database
	sqlite3_close(m_dbase);
	m_dbase=NULL;
	std::ofstream outfile2;
	outfile2.open(m_dbase_name.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (!outfile2.is_open())
		return false;
	outfile2 << dbase;
	outfile2.flush();
	outfile2.close();
	//change ownership
#ifndef WIN32
	struct stat info;
	if (stat(m_dbase_name.c_str(), &info)==0)
	{
		struct passwd *pw = getpwuid(info.st_uid);
		int ret=chown(m_dbase_name.c_str(),pw->pw_uid,pw->pw_gid);
		if (ret!=0)
		{
			_log.Log(LOG_ERROR, "Error setting database ownership (chown returned an error!)");
		}
	}
#endif
	return OpenDatabase();
}

bool CSQLHelper::BackupDatabase(const std::string &OutputFile)
{
	if (!m_dbase)
		return false; //database not open!

	boost::lock_guard<boost::mutex> l(m_sqlQueryMutex);

	int rc;                     // Function return code
	sqlite3 *pFile;             // Database connection opened on zFilename
	sqlite3_backup *pBackup;    // Backup handle used to copy data

	// Open the database file identified by zFilename.
	rc = sqlite3_open(OutputFile.c_str(), &pFile);
	if( rc!=SQLITE_OK )
		return false;

	// Open the sqlite3_backup object used to accomplish the transfer
    pBackup = sqlite3_backup_init(pFile, "main", m_dbase, "main");
    if( pBackup )
	{
      // Each iteration of this loop copies 5 database pages from database
      // pDb to the backup database.
      do {
        rc = sqlite3_backup_step(pBackup, 5);
        //xProgress(  sqlite3_backup_remaining(pBackup), sqlite3_backup_pagecount(pBackup) );
        //if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ){
          //sqlite3_sleep(250);
        //}
      } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

      /* Release resources allocated by backup_init(). */
      sqlite3_backup_finish(pBackup);
    }
    rc = sqlite3_errcode(pFile);
	// Close the database connection opened on database file zFilename
	// and return the result of this function.
	sqlite3_close(pFile);
	return ( rc==SQLITE_OK );
}

void CSQLHelper::Lighting2GroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd)
{
	char szTmp[100];
	sprintf(szTmp,"UPDATE DeviceStatus SET nValue = %d WHERE (DeviceID=='%s') And (Type==%d) And (SubType==%d)",GroupCmd,ID.c_str(),pTypeLighting2,subType);
	query(szTmp);
}

void CSQLHelper::GeneralSwitchGroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd)
{
	char szTmp[100];
	sprintf(szTmp, "UPDATE DeviceStatus SET nValue = %d WHERE (DeviceID=='%s') And (Type==%d) And (SubType==%d)", GroupCmd, ID.c_str(), pTypeGeneralSwitch, subType);
	query(szTmp);
}

void CSQLHelper::SetUnitsAndScale()
{
	//Wind
	if (m_windunit==WINDUNIT_MS)
	{
		m_windsign="m/s";
		m_windscale=0.1f;
	}
	else if (m_windunit==WINDUNIT_KMH)
	{
		m_windsign="km/h";
		m_windscale=0.36f;
	}
	else if (m_windunit==WINDUNIT_MPH)
	{
		m_windsign="mph";
		m_windscale=0.223693629205f;
	}
	else if (m_windunit==WINDUNIT_Knots)
	{
		m_windsign="kn";
		m_windscale=0.1943844492457398f;
	}

	//Temp
	if (m_tempunit==TEMPUNIT_C)
	{
		m_tempsign="C";
		m_tempscale=1.0f;
	}
	if (m_tempunit==TEMPUNIT_F)
	{
		m_tempsign="F";
		m_tempscale=1.0f; //*1.8 + 32
	}
}

bool CSQLHelper::HandleOnOffAction(const bool bIsOn, const std::string &OnAction, const std::string &OffAction)
{
	if (bIsOn)
	{
		if (OnAction.size()<1)
			return true;
		if (OnAction.find("http://")!=std::string::npos)
		{
			_tTaskItem tItem;
			tItem=_tTaskItem::GetHTTPPage(1,OnAction,"SwitchActionOn");
			AddTaskItem(tItem);
		}
		else if (OnAction.find("script://")!=std::string::npos)
		{
			//Execute possible script
			std::string scriptname = "";
			if (OnAction.find("script:///") != std::string::npos)
				scriptname = OnAction.substr(9);
			else
				scriptname = OnAction.substr(8);
			std::string scriptparams="";
			//Add parameters
			int pindex=scriptname.find(' ');
			if (pindex!=std::string::npos)
			{
				scriptparams=scriptname.substr(pindex+1);
				scriptname=scriptname.substr(0,pindex);
			}
			if (file_exist(scriptname.c_str()))
			{
				AddTaskItem(_tTaskItem::ExecuteScript(1,scriptname,scriptparams));
			}
		}
	}
	else
	{
		if (OffAction.size()<1)
			return true;
		if (OffAction.find("http://")!=std::string::npos)
		{
			_tTaskItem tItem;
			tItem=_tTaskItem::GetHTTPPage(1,OffAction,"SwitchActionOff");
			AddTaskItem(tItem);
		}
		else if (OffAction.find("script://")!=std::string::npos)
		{
			//Execute possible script
			std::string scriptname = "";
			if (OffAction.find("script:///") != std::string::npos)
				scriptname=OffAction.substr(9);
			else
				scriptname = OffAction.substr(8);
			std::string scriptparams="";
			int pindex=scriptname.find(' ');
			if (pindex!=std::string::npos)
			{
				scriptparams=scriptname.substr(pindex+1);
				scriptname=scriptname.substr(0,pindex);
			}
			if (file_exist(scriptname.c_str()))
			{
				AddTaskItem(_tTaskItem::ExecuteScript(1,scriptname,scriptparams));
			}
		}
	}
	return true;
}

//Executed every hour
void CSQLHelper::CheckBatteryLow()
{
	int iBatteryLowLevel=0;
	GetPreferencesVar("BatteryLowNotification", iBatteryLowLevel);
	if (iBatteryLowLevel==0)
		return;//disabled

}

//Executed every hour
void CSQLHelper::CheckDeviceTimeout()
{
	int TimeoutCheckInterval=1;
	GetPreferencesVar("SensorTimeoutNotification", TimeoutCheckInterval);
	if (TimeoutCheckInterval==0)
		return;//disabled

	m_sensortimeoutcounter+=1;
	if (m_sensortimeoutcounter<TimeoutCheckInterval)
		return;
	m_sensortimeoutcounter=0;

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);
	time_t now = mytime(NULL);
	struct tm stoday;
	localtime_r(&now,&stoday);
	now-=(SensorTimeOut*60);
	struct tm ltime;
	localtime_r(&now,&ltime);

	std::vector<std::vector<std::string> > result;
	char szTmp[300];
	sprintf(szTmp,
		"SELECT ID,Name,LastUpdate FROM DeviceStatus WHERE (Used!=0 AND LastUpdate<='%04d-%02d-%02d %02d:%02d:%02d' AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d) ORDER BY Name",
		ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		pTypeLighting1,
		pTypeLighting2,
		pTypeLighting3,
		pTypeLighting4,
		pTypeLighting5,
		pTypeLighting6,
		pTypeRadiator1,
		pTypeLimitlessLights,
		pTypeSecurity1,
		pTypeCurtain,
		pTypeBlinds,
		pTypeRFY,
		pTypeChime,
		pTypeThermostat2,
		pTypeThermostat3,
		pTypeRemote,
		pTypeGeneralSwitch
		);
	result=query(szTmp);
	if (result.size()<1)
		return;

	unsigned long long ulID;
	std::vector<std::vector<std::string> >::const_iterator itt;

	//check if last timeout_notification is not sent today and if true, send notification
	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		std::stringstream s_str( sd[0] );
		s_str >> ulID;
		bool bDoSend=true;
		std::map<unsigned long long,int>::const_iterator sitt;
		sitt=m_timeoutlastsend.find(ulID);
		if (sitt!=m_timeoutlastsend.end())
		{
			bDoSend=(stoday.tm_mday!=sitt->second);
		}
		if (bDoSend)
		{
			sprintf(szTmp,"Sensor Timeout: %s, Last Received: %s",sd[1].c_str(),sd[2].c_str());
			m_notifications.SendMessageEx(NOTIFYALL, std::string(""), szTmp, 1, std::string(""), true);
			m_timeoutlastsend[ulID]=stoday.tm_mday;
		}
	}
}

void CSQLHelper::FixDaylightSavingTableSimple(const std::string &TableName)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery << "SELECT t.RowID, u.RowID, t.Date from " << TableName << " as t, " << TableName << " as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID]) ORDER BY t.[RowID]";
	result=query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		std::stringstream sstr;
		unsigned long ID1;
		unsigned long ID2;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			sstr.clear();
			sstr.str("");
			sstr << sd[0];
			sstr >> ID1;
			sstr.clear();
			sstr.str("");
			sstr << sd[1];
			sstr >> ID2;
			if (ID2>ID1)
			{
				std::string szDate=sd[2];
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT date('" << szDate << "','+1 day')";
				std::vector<std::vector<std::string> > result2;
				result2=query(szQuery.str());

				std::string szDateNew=result2[0][0];

				//Check if Date+1 exists, if yes, remove current double value
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT RowID FROM " << TableName << " WHERE (Date='" << szDateNew << "') AND (RowID==" << sd[1] << ")";
				result2=query(szQuery.str());
				szQuery.clear();
				szQuery.str("");
				if (result2.size()>0)
				{
					//Delete row
					szQuery << "DELETE FROM " << TableName << " WHERE (RowID==" << sd[1] << ")";
					query(szQuery.str());
				}
				else
				{
					//Update date
					szQuery << "UPDATE " << TableName << " SET Date='" << szDateNew << "' WHERE (RowID==" << sd[1] << ")";
					query(szQuery.str());
				}
			}
		}
	}
}

void CSQLHelper::FixDaylightSaving()
{
	//First the easy tables
	FixDaylightSavingTableSimple("Fan_Calendar");
	FixDaylightSavingTableSimple("Percentage_Calendar");
	FixDaylightSavingTableSimple("Rain_Calendar");
	FixDaylightSavingTableSimple("Temperature_Calendar");
	FixDaylightSavingTableSimple("UV_Calendar");
	FixDaylightSavingTableSimple("Wind_Calendar");

	//Meter_Calendar
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	szQuery << "SELECT t.RowID, u.RowID, t.Value, u.Value, t.Date from Meter_Calendar as t, Meter_Calendar as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID]) ORDER BY t.[RowID]";
	result=query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		std::stringstream sstr;
		unsigned long ID1;
		unsigned long ID2;
		unsigned long long Value1;
		unsigned long long Value2;
		unsigned long long ValueDest;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd1=*itt;

			sstr.clear();
			sstr.str("");
			sstr << sd1[0];
			sstr >> ID1;
			sstr.clear();
			sstr.str("");
			sstr << sd1[1];
			sstr >> ID2;
			sstr.clear();
			sstr.str("");
			sstr << sd1[2];
			sstr >> Value1;
			sstr.clear();
			sstr.str("");
			sstr << sd1[3];
			sstr >> Value2;
			if (ID2>ID1)
			{
				if (Value2>Value1)
					ValueDest=Value2-Value1;
				else
					ValueDest=Value2;

				std::string szDate=sd1[4];
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT date('" << szDate << "','+1 day')";
				std::vector<std::vector<std::string> > result2;
				result2=query(szQuery.str());

				std::string szDateNew=result2[0][0];

				//Check if Date+1 exists, if yes, remove current double value
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT RowID FROM Meter_Calendar WHERE (Date='" << szDateNew << "') AND (RowID==" << sd1[1] << ")";
				result2=query(szQuery.str());
				szQuery.clear();
				szQuery.str("");
				if (result2.size()>0)
				{
					//Delete Row
					szQuery << "DELETE FROM Meter_Calendar WHERE (RowID==" << sd1[1] << ")";
					query(szQuery.str());
				}
				else
				{
					//Update row with new Date
					szQuery << "UPDATE Meter_Calendar SET Date='" << szDateNew << "', Value=" << ValueDest << " WHERE (RowID==" << sd1[1] << ")";
					query(szQuery.str());
				}
			}
		}
	}

	//Last (but not least) MultiMeter_Calendar
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT t.RowID, u.RowID, t.Value1, t.Value2, t.Value3, t.Value4, t.Value5, t.Value6, u.Value1, u.Value2, u.Value3, u.Value4, u.Value5, u.Value6, t.Date from MultiMeter_Calendar as t, MultiMeter_Calendar as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID]) ORDER BY t.[RowID]";
	result=query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		std::stringstream sstr;
		unsigned long ID1;
		unsigned long ID2;
		unsigned long long tValue1;
		unsigned long long tValue2;
		unsigned long long tValue3;
		unsigned long long tValue4;
		unsigned long long tValue5;
		unsigned long long tValue6;

		unsigned long long uValue1;
		unsigned long long uValue2;
		unsigned long long uValue3;
		unsigned long long uValue4;
		unsigned long long uValue5;
		unsigned long long uValue6;

		unsigned long long ValueDest1;
		unsigned long long ValueDest2;
		unsigned long long ValueDest3;
		unsigned long long ValueDest4;
		unsigned long long ValueDest5;
		unsigned long long ValueDest6;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd1=*itt;

			sstr.clear();
			sstr.str("");
			sstr << sd1[0];
			sstr >> ID1;
			sstr.clear();
			sstr.str("");
			sstr << sd1[1];
			sstr >> ID2;

			sstr.clear();
			sstr.str("");
			sstr << sd1[2];
			sstr >> tValue1;
			sstr.clear();
			sstr.str("");
			sstr << sd1[3];
			sstr >> tValue2;
			sstr.clear();
			sstr.str("");
			sstr << sd1[4];
			sstr >> tValue3;
			sstr.clear();
			sstr.str("");
			sstr << sd1[5];
			sstr >> tValue4;
			sstr.clear();
			sstr.str("");
			sstr << sd1[6];
			sstr >> tValue5;
			sstr.clear();
			sstr.str("");
			sstr << sd1[7];
			sstr >> tValue6;

			sstr.clear();
			sstr.str("");
			sstr << sd1[8];
			sstr >> uValue1;
			sstr.clear();
			sstr.str("");
			sstr << sd1[9];
			sstr >> uValue2;
			sstr.clear();
			sstr.str("");
			sstr << sd1[10];
			sstr >> uValue3;
			sstr.clear();
			sstr.str("");
			sstr << sd1[11];
			sstr >> uValue4;
			sstr.clear();
			sstr.str("");
			sstr << sd1[12];
			sstr >> uValue5;
			sstr.clear();
			sstr.str("");
			sstr << sd1[13];
			sstr >> uValue6;

			if (ID2>ID1)
			{
				if (uValue1>tValue1)
					ValueDest1=uValue1-tValue1;
				else
					ValueDest1=uValue1;
				if (uValue2>tValue2)
					ValueDest2=uValue2-tValue2;
				else
					ValueDest2=uValue2;
				if (uValue3>tValue3)
					ValueDest3=uValue3-tValue3;
				else
					ValueDest3=uValue3;
				if (uValue4>tValue4)
					ValueDest4=uValue4-tValue4;
				else
					ValueDest4=uValue4;
				if (uValue5>tValue5)
					ValueDest5=uValue5-tValue5;
				else
					ValueDest5=uValue5;
				if (uValue6>tValue6)
					ValueDest6=uValue6-tValue6;
				else
					ValueDest6=uValue6;

				std::string szDate=sd1[14];
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT date('" << szDate << "','+1 day')";
				std::vector<std::vector<std::string> > result2;
				result2=query(szQuery.str());

				std::string szDateNew=result2[0][0];

				//Check if Date+1 exists, if yes, remove current double value
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT RowID FROM MultiMeter_Calendar WHERE (Date='" << szDateNew << "') AND (RowID==" << sd1[1] << ")";
				result2=query(szQuery.str());
				szQuery.clear();
				szQuery.str("");
				if (result2.size()>0)
				{
					//Delete Row
					szQuery << "DELETE FROM MultiMeter_Calendar WHERE (RowID==" << sd1[1] << ")";
					query(szQuery.str());
				}
				else
				{
					//Update row with new Date
					szQuery << "UPDATE MultiMeter_Calendar SET Date='" << szDateNew << "', Value1=" << ValueDest1 << ", Value2=" << ValueDest2 << ", Value3=" << ValueDest3 << ", Value4=" << ValueDest4 << ", Value5=" << ValueDest5 << ", Value6=" << ValueDest6 << " WHERE (RowID==" << sd1[1] << ")";
					query(szQuery.str());
				}
			}
		}
	}

}

std::string CSQLHelper::DeleteUserVariable(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[300];
	sprintf(szTmp, "DELETE FROM UserVariables WHERE (ID==%s)", idx.c_str());
	result = query(szTmp);

	return "OK";

}
std::string CSQLHelper::SaveUserVariable(const std::string &varname, const std::string &vartype, const std::string &varvalue)
{
	int typei = atoi(vartype.c_str());
	std::string dupeName = CheckUserVariableName(varname);
	if (dupeName != "OK")
		return dupeName;

	std::string formatError = CheckUserVariable(typei, varvalue);
	if (formatError != "OK")
		return formatError;

	std::vector<std::vector<std::string> > result;
	char szTmp[300];
	sprintf(szTmp, "INSERT INTO UserVariables (Name,ValueType,Value) VALUES ('%s','%d','%s')",
		varname.c_str(),
		typei,
		varvalue.c_str()
		);
	result = query(szTmp);

	sprintf(szTmp, "SELECT ID FROM UserVariables WHERE (Name == '%s')",
		varname.c_str()
	);
	result = query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::string> sd = result[0];
		std::stringstream vId_str(sd[0]);
		unsigned long long vId;
		vId_str >> vId;
		m_mainworker.m_eventsystem.ProcessUserVariable(vId);
	}

	
	return "OK";

}
std::string CSQLHelper::UpdateUserVariable(const std::string &idx, const std::string &varname, const std::string &vartype, const std::string &varvalue, const bool eventtrigger)
{
	int typei = atoi(vartype.c_str());
	std::string formatError = CheckUserVariable(typei, varvalue);
	if (formatError != "OK")
		return formatError;

	std::vector<std::vector<std::string> > result;
	char szTmp[300];

	/*
	sprintf(szTmp, "SELECT Value FROM UserVariables WHERE (Name == '%s')",
		varname.c_str()
		);
	result = query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::string> sd = result[0];
		if (varvalue == sd[0])
			return "New value same as current, not updating";
	}
	*/
	
	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	sprintf(szTmp,
		"UPDATE UserVariables SET Name='%s', ValueType='%d', Value='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == %s)",
		varname.c_str(),
		typei,
		varvalue.c_str(),
		ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		idx.c_str()
		);

	result = query(szTmp);
	if (eventtrigger) {
		std::stringstream vId_str(idx);
		unsigned long long vId;
		vId_str >> vId;
		m_mainworker.m_eventsystem.ProcessUserVariable(vId);
	}
	return "OK";

}

std::string CSQLHelper::CheckUserVariableName(const std::string &varname)
{
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM UserVariables WHERE (Name=='" << varname << "')";
	result = query(szQuery.str());
	if (result.size() > 0)
	{
		return "Variable name already exists!";
	}
	return "OK";
}


std::string CSQLHelper::CheckUserVariable(const int vartype, const std::string &varvalue)
{

	if (varvalue.size() > 200) {
		return "String exceeds maximum size";
	}
	if (vartype == 0) {
		//integer
		std::istringstream iss(varvalue);
		int i;
		iss >> std::noskipws >> i;
		if (!(iss.eof() && !iss.fail()))
		{
			return "Not a valid integer";
		}
	}
	else if (vartype == 1) {
		//float 
		std::istringstream iss(varvalue);
		float f;
		iss >> std::noskipws >> f; 
		if (!(iss.eof() && !iss.fail()))
		{
			return "Not a valid float";
		}
	}
	else if (vartype == 3) {
		//date  
		int d, m, y;
		if (!CheckDate(varvalue, d, m, y))
		{
			return "Not a valid date notation (DD/MM/YYYY)";
		}
	}
	else if (vartype == 4) {
		//time
		if (!CheckTime(varvalue))
			return "Not a valid time notation (HH:MM)";
	}
	else if (vartype == 5) {
		return "OK";
	}
	return "OK";
}


std::vector<std::vector<std::string> > CSQLHelper::GetUserVariables()
{
	std::stringstream szQuery;
	szQuery << "SELECT ID,Name,ValueType,Value,LastUpdate FROM UserVariables";
	return query(szQuery.str());

}

bool CSQLHelper::CheckDate(const std::string &sDate, int& d, int& m, int& y)
{
	std::istringstream is(sDate);
	char delimiter;
	if (is >> d >> delimiter >> m >> delimiter >> y) {
		struct tm t = { 0 };
		t.tm_mday = d;
		t.tm_mon = m - 1;
		t.tm_year = y - 1900;
		t.tm_isdst = -1;

		time_t when = mktime(&t);
		const struct tm *norm = localtime(&when);
		if (norm == NULL)
			return false;

		return (norm->tm_mday == d    &&
			norm->tm_mon == m - 1 &&
			norm->tm_year == y - 1900);
	}
	return false;
}

bool CSQLHelper::CheckTime(const std::string &sTime)
{
	
	int iSemiColon = sTime.find(':');
	if ((iSemiColon == std::string::npos) || (iSemiColon < 1) || (iSemiColon > 2) || (iSemiColon == sTime.length()-1)) return false;
	if ((sTime.length() < 3) || (sTime.length() > 5)) return false;
	if (atoi(sTime.substr(0, iSemiColon).c_str()) >= 24) return false;
	if (atoi(sTime.substr(iSemiColon + 1).c_str()) >= 60) return false;
	return true;
}

void CSQLHelper::AllowNewHardwareTimer(const int iTotMinutes)
{
	m_iAcceptHardwareTimerCounter = iTotMinutes * 60;
	if (m_bAcceptHardwareTimerActive == false)
	{
		m_bPreviousAcceptNewHardware = m_bAcceptNewHardware;
	}
	m_bAcceptNewHardware = true;
	m_bAcceptHardwareTimerActive = true;
	_log.Log(LOG_STATUS, "New sensors allowed for %d minutes...", iTotMinutes);
}

bool CSQLHelper::InsertCustomIconFromZip(const std::string &szZip, std::string &ErrorMessage)
{
	//write file to disk
	std::string outputfile = "custom_icons.zip";
	std::ofstream outfile;
	outfile.open(outputfile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!outfile.is_open())
	{
		ErrorMessage = "Error writing zip to disk";
		return false;
	}
	outfile << szZip;
	outfile.flush();
	outfile.close();

	clx::basic_unzip<char> in(outputfile);
	if (!in.is_open())
	{
		ErrorMessage = "Error opening zip file";
		return false;
	}
	clx::basic_unzip<char>::iterator fitt = in.find("icons.txt");
	if (fitt == in.end())
	{
		//definition file not found
		ErrorMessage = "Icon definition file not found";
		return false;
	}

	uLong fsize;
	unsigned char *pFBuf = (unsigned char *)(fitt).Extract(fsize,1);
	if (pFBuf == NULL)
	{
		ErrorMessage = "Could not extract icons.txt";
		return false;
	}
	pFBuf[fsize] = 0; //null terminate

	std::string _defFile = std::string(pFBuf, pFBuf+fsize);
	free(pFBuf);

	std::vector<std::string> _Lines;
	StringSplit(_defFile, "\n", _Lines);
	std::vector<std::string>::const_iterator itt;

	int iTotalAdded = 0;

	for (itt = _Lines.begin(); itt != _Lines.end(); ++itt)
	{
		std::string sLine = (*itt);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		std::vector<std::string> splitresult;
		StringSplit(sLine, ";", splitresult);
		if (splitresult.size() == 3)
		{
			std::string IconBase = splitresult[0];
			std::string IconName = splitresult[1];
			std::string IconDesc = splitresult[2];

			

			//Check if this Icon(Name) does not exist in the database already
			std::stringstream szQuery;
			szQuery << "SELECT ID FROM CustomImages WHERE Base='" << IconBase << "'";
			std::vector<std::vector<std::string> > result = query(szQuery.str());
			bool bIsDuplicate = (result.size()>0);
			int RowID = 0;
			if (bIsDuplicate)
			{
				RowID=atoi(result[0][0].c_str());
			}
			
			//Locate the files in the zip, if not present back out
			std::string IconFile16 = IconBase + ".png";
			std::string IconFile48On = IconBase + "48_On.png";
			std::string IconFile48Off = IconBase + "48_Off.png";

			std::map<std::string, std::string> _dbImageFiles;
			_dbImageFiles["IconSmall"] = IconFile16;
			_dbImageFiles["IconOn"] = IconFile48On;
			_dbImageFiles["IconOff"] = IconFile48Off;

			std::map<std::string, std::string>::const_iterator iItt;

			for (iItt = _dbImageFiles.begin(); iItt != _dbImageFiles.end(); ++iItt)
			{
				std::string TableField = iItt->first;
				std::string IconFile = iItt->second;
				if (in.find(IconFile) == in.end())
				{
					ErrorMessage = "Icon File: " + IconFile + " is not present";
					if (iTotalAdded>0)
					{
						m_webserver.ReloadCustomSwitchIcons();
					}
					return false;
				}
			}

			//All good, now lets add it to the database
			szQuery.clear();
			szQuery.str("");
			if (!bIsDuplicate)
			{
				szQuery << "INSERT INTO CustomImages (Base,Name, Description) VALUES ('" << IconBase << "', '" << IconName << "', '" << IconDesc << "')";
				result = query(szQuery.str());

				//Get our Database ROWID
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT ID FROM CustomImages WHERE Base='" << IconBase << "'";
				result = query(szQuery.str());
				if (result.size() == 0)
				{
					ErrorMessage = "Error adding new row to database!";
					if (iTotalAdded > 0)
					{
						m_webserver.ReloadCustomSwitchIcons();
					}
					return false;
				}
				RowID = atoi(result[0][0].c_str());
			}
			else
			{
				//Update
				szQuery << "UPDATE CustomImages SET Name='" << IconName << "', Description='" << IconDesc << "' WHERE ID=" << RowID;
				result = query(szQuery.str());

				//Delete from disk, so it will be updated when we exit this function
				std::string IconFile16 = szWWWFolder + "/images/" + IconBase + ".png";
				std::string IconFile48On = szWWWFolder + "/images/" + IconBase + "48_On.png";
				std::string IconFile48Off = szWWWFolder + "/images/" + IconBase + "48_Off.png";
				std::remove(IconFile16.c_str());
				std::remove(IconFile48On.c_str());
				std::remove(IconFile48Off.c_str());
			}

			//Insert the Icons

			for (iItt = _dbImageFiles.begin(); iItt != _dbImageFiles.end(); ++iItt)
			{
				std::string TableField = iItt->first;
				std::string IconFile = iItt->second;

				szQuery.clear();
				szQuery.str("");
				szQuery << "UPDATE CustomImages SET " << TableField << " = ? WHERE ID=" << RowID;

				sqlite3_stmt *stmt = NULL;
				int rc = sqlite3_prepare_v2(m_dbase, szQuery.str().c_str(), -1, &stmt, NULL);
				if (rc != SQLITE_OK) {
					ErrorMessage = "Problem inserting icon into database! " + std::string(sqlite3_errmsg(m_dbase));
					if (iTotalAdded > 0)
					{
						m_webserver.ReloadCustomSwitchIcons();
					}
					return false;
				}
				// SQLITE_STATIC because the statement is finalized
				// before the buffer is freed:
				pFBuf = (unsigned char *)in.find(IconFile).Extract(fsize);
				if (pFBuf == NULL)
				{
					ErrorMessage = "Could not extract File: " + IconFile16;
					if (iTotalAdded > 0)
					{
						m_webserver.ReloadCustomSwitchIcons();
					}
					return false;
				}
				rc = sqlite3_bind_blob(stmt, 1, pFBuf, fsize, SQLITE_STATIC);
				if (rc != SQLITE_OK) {
					ErrorMessage = "Problem inserting icon into database! " + std::string(sqlite3_errmsg(m_dbase));
					free(pFBuf);
					if (iTotalAdded > 0)
					{
						m_webserver.ReloadCustomSwitchIcons();
					}
					return false;
				}
				else {
					rc = sqlite3_step(stmt);
					if (rc != SQLITE_DONE)
					{
						free(pFBuf);
						ErrorMessage = "Problem inserting icon into database! " + std::string(sqlite3_errmsg(m_dbase));
						if (iTotalAdded > 0)
						{
							m_webserver.ReloadCustomSwitchIcons();
						}
						return false;
					}
				}
				sqlite3_finalize(stmt);
				free(pFBuf);
				iTotalAdded++;
			}
		}
	}
	m_webserver.ReloadCustomSwitchIcons();
	return true;
}
