#include "stdafx.h"
#include "SQLHelper.h"
#include <iostream>     /* standard I/O functions                         */
#include <iomanip>
#include "RFXtrx.h"
#include "RFXNames.h"
#include "localtime_r.h"
#include "Logger.h"
#include "mainworker.h"
#ifdef WITH_EXTERNAL_SQLITE
#include <sqlite3.h>
#else
#include "../sqlite/sqlite3.h"
#endif
#include "../hardware/hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../smtpclient/SMTPClient.h"
#include "WebServerHelper.h"
#include "../webserver/Base64.h"
#include "unzip.h"
#include <boost/lexical_cast.hpp>
#include "../notifications/NotificationHelper.h"
#include "IFTTT.h"

#ifndef WIN32
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
#else
	#include "../msbuild/WindowsHelper.h"
#endif
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define DB_VERSION 120

extern http::server::CWebServerHelper m_webservers;
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
"[CustomImage] INTEGER DEFAULT 0, "
"[Description] VARCHAR(200) DEFAULT '', "
"[Options] TEXT DEFAULT null);";

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
"[User] VARCHAR(100) DEFAULT (''), "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateSceneLog =
"CREATE TABLE IF NOT EXISTS [SceneLog] ("
"[SceneRowID] BIGINT(10) NOT NULL, "
"[nValue] INTEGER DEFAULT 0, "
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
"[Days] INTEGER NOT NULL, "
"[Month] INTEGER DEFAULT 0, "
"[MDay] INTEGER DEFAULT 0, "
"[Occurence] INTEGER DEFAULT 0);";

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
"[CustomMessage] VARCHAR(300) DEFAULT (''), "
"[ActiveSystems] VARCHAR(200) DEFAULT (''), "
"[Priority] INTEGER default 0, "
"[SendAlways] INTEGER default 0, "
"[LastSend] DATETIME DEFAULT 0);";

const char *sqlCreateHardware =
"CREATE TABLE IF NOT EXISTS [Hardware] ("
"[ID] INTEGER PRIMARY KEY, "
"[Name] VARCHAR(200) NOT NULL, "
"[Enabled] INTEGER DEFAULT 1, "
"[Type] INTEGER NOT NULL, "
"[Address] VARCHAR(200), "
"[Port] INTEGER, "
"[SerialPort] VARCHAR(50) DEFAULT (''), "
"[Username] VARCHAR(100), "
"[Password] VARCHAR(100), "
"[Extra] TEXT DEFAULT (''),"
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
"[Protocol] INTEGER DEFAULT 0, "
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
"[Favorite] INTEGER DEFAULT 0, \n"
"[Order] INTEGER BIGINT(10) default 0, \n"
"[nValue] INTEGER DEFAULT 0, \n"
"[SceneType] INTEGER DEFAULT 0, \n"
"[Protected] INTEGER DEFAULT 0, \n"
"[OnAction] VARCHAR(200) DEFAULT '', "
"[OffAction] VARCHAR(200) DEFAULT '', "
"[Description] VARCHAR(200) DEFAULT '', "
"[Activators] VARCHAR(200) DEFAULT '', "
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

const char *sqlCreateTimerPlans =
"CREATE TABLE IF NOT EXISTS [TimerPlans] ("
"[ID] INTEGER PRIMARY KEY, "
"[Name] VARCHAR(200) NOT NULL);";

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
"[Days] INTEGER NOT NULL, "
"[Month] INTEGER DEFAULT 0, "
"[MDay] INTEGER DEFAULT 0, "
"[Occurence] INTEGER DEFAULT 0);";

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
"[Days] INTEGER NOT NULL, "
"[Month] INTEGER DEFAULT 0, "
"[MDay] INTEGER DEFAULT 0, "
"[Occurence] INTEGER DEFAULT 0);";

const char *sqlCreateSharedDevices =
"CREATE TABLE IF NOT EXISTS [SharedDevices] ("
"[ID] INTEGER PRIMARY KEY,  "
"[SharedUserID] BIGINT NOT NULL, "
"[DeviceRowID] BIGINT NOT NULL);";

const char *sqlCreateEventMaster =
"CREATE TABLE IF NOT EXISTS [EventMaster] ("
"[ID] INTEGER PRIMARY KEY,  "
"[Name] VARCHAR(200) NOT NULL, "
"[Interpreter] VARCHAR(10) DEFAULT 'Blockly', "
"[Type] VARCHAR(10) DEFAULT 'All', "
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
	"[MacAddress] VARCHAR(50) DEFAULT Unknown, "
	"[Timeout] INTEGER DEFAULT 5);";

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

const char *sqlCreateHttpLink =
	"CREATE TABLE IF NOT EXISTS [HttpLink] ("
	"[ID] INTEGER PRIMARY KEY, "
	"[DeviceID]  BIGINT NOT NULL, "
	"[DelimitedValue] INTEGER DEFAULT 0, "
	"[TargetType] INTEGER DEFAULT 0, "
	"[TargetVariable] VARCHAR(100), "
	"[TargetDeviceID] INTEGER, "
	"[TargetProperty] VARCHAR(100), "
	"[Enabled] INTEGER DEFAULT 1, "
	"[IncludeUnit] INTEGER default 0); ";

const char *sqlCreatePushLink =
"CREATE TABLE IF NOT EXISTS [PushLink] ("
"[ID] INTEGER PRIMARY KEY, "
"[PushType] INTEGER, "
"[DeviceID]  BIGINT NOT NULL, "
"[DelimitedValue] INTEGER DEFAULT 0, "
"[TargetType] INTEGER DEFAULT 0, "
"[TargetVariable] VARCHAR(100), "
"[TargetDeviceID] INTEGER, "
"[TargetProperty] VARCHAR(100), "
"[Enabled] INTEGER DEFAULT 1, "
"[IncludeUnit] INTEGER default 0); ";

const char *sqlCreateGooglePubSubLink =
"CREATE TABLE IF NOT EXISTS [GooglePubSubLink] ("
"[ID] INTEGER PRIMARY KEY, "
"[DeviceID]  BIGINT NOT NULL, "
"[DelimitedValue] INTEGER DEFAULT 0, "
"[TargetType] INTEGER DEFAULT 0, "
"[TargetVariable] VARCHAR(100), "
"[TargetDeviceID] INTEGER, "
"[TargetProperty] VARCHAR(100), "
"[Enabled] INTEGER DEFAULT 1, "
"[IncludeUnit] INTEGER default 0); ";

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
	" [Name] VARCHAR(100) DEFAULT Unknown,"
	" [SketchName] VARCHAR(100) DEFAULT Unknown,"
	" [SketchVersion] VARCHAR(40) DEFAULT(1.0));";

const char *sqlCreateMySensorsVariables =
	"CREATE TABLE IF NOT EXISTS [MySensorsVars]("
	" [HardwareID] INTEGER NOT NULL,"
	" [NodeID] INTEGER NOT NULL,"
	" [ChildID] INTEGER NOT NULL,"
	" [VarID] INTEGER NOT NULL,"
	" [Value] VARCHAR(100) NOT NULL);";

const char *sqlCreateMySensorsChilds =
"CREATE TABLE IF NOT EXISTS [MySensorsChilds]("
" [HardwareID] INTEGER NOT NULL,"
" [NodeID] INTEGER NOT NULL,"
" [ChildID] INTEGER NOT NULL,"
" [Name] VARCHAR(100) DEFAULT '',"
" [Type] INTEGER NOT NULL,"
" [UseAck] INTEGER DEFAULT 0,"
" [AckTimeout] INTEGER DEFAULT 1200);";

const char *sqlCreateToonDevices =
	"CREATE TABLE IF NOT EXISTS [ToonDevices]("
	" [HardwareID] INTEGER NOT NULL,"
	" [UUID] VARCHAR(100) NOT NULL);";

const char *sqlCreateUserSessions =
	"CREATE TABLE IF NOT EXISTS [UserSessions]("
	" [SessionID] VARCHAR(100) NOT NULL,"
	" [Username] VARCHAR(100) NOT NULL,"
	" [AuthToken] VARCHAR(100) UNIQUE NOT NULL,"
	" [ExpirationDate] DATETIME NOT NULL,"
	" [RemoteHost] VARCHAR(50) NOT NULL,"
	" [LastUpdate] DATETIME DEFAULT(datetime('now', 'localtime')),"
	" PRIMARY KEY([SessionID]));";

const char *sqlCreateMobileDevices =
"CREATE TABLE IF NOT EXISTS [MobileDevices]("
"[ID] INTEGER PRIMARY KEY, "
"[Active] BOOLEAN DEFAULT false, "
"[Name] VARCHAR(100) DEFAULT '',"
"[DeviceType] VARCHAR(100) DEFAULT '',"
"[SenderID] TEXT NOT NULL,"
"[UUID] TEXT NOT NULL, "
"[LastUpdate] DATETIME DEFAULT(datetime('now', 'localtime'))"
");";

extern std::string szUserDataFolder;

CSQLHelper::CSQLHelper(void)
{
	m_LastSwitchRowID=0;
	m_dbase=NULL;
	m_stoprequested=false;
	m_sensortimeoutcounter=0;
	m_bAcceptNewHardware=true;
	m_bAllowWidgetOrdering=true;
	m_ActiveTimerPlan=0;
	m_windunit=WINDUNIT_MS;
	m_tempunit=TEMPUNIT_C;
	m_weightunit=WEIGHTUNIT_KG;
	SetUnitsAndScale();
	m_bAcceptHardwareTimerActive=false;
	m_iAcceptHardwareTimerCounter=0;
	m_bEnableEventSystem = true;
	m_bDisableDzVentsSystem = false;
	m_ShortLogInterval = 5;
	m_bPreviousAcceptNewHardware = false;

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
		OptimizeDatabase(m_dbase);
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
	sqlite3_exec(m_dbase, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);
	sqlite3_exec(m_dbase, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
#else
	sqlite3_exec(m_dbase, "PRAGMA journal_mode=DELETE", NULL, NULL, NULL);
#endif
    sqlite3_exec(m_dbase, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
	std::vector<std::vector<std::string> > result=query("SELECT name FROM sqlite_master WHERE type='table' AND name='DeviceStatus'");
	bool bNewInstall=(result.size()==0);
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
	query(sqlCreateSceneLog);
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
	query(sqlCreateTimerPlans);
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
	query(sqlCreateHttpLink);
	query(sqlCreatePushLink);
	query(sqlCreateGooglePubSubLink);
	query(sqlCreateUserVariables);
	query(sqlCreateFloorplans);
	query(sqlCreateFloorplanOrderTrigger);
	query(sqlCreateCustomImages);
	query(sqlCreateMySensors);
	query(sqlCreateMySensorsVariables);
	query(sqlCreateMySensorsChilds);
	query(sqlCreateToonDevices);
	query(sqlCreateUserSessions);
	query(sqlCreateMobileDevices);
	//Add indexes to log tables
	query("create index if not exists ds_hduts_idx    on DeviceStatus(HardwareID, DeviceID, Unit, Type, SubType);");
	query("create index if not exists f_id_idx        on Fan(DeviceRowID);");
	query("create index if not exists f_id_date_idx   on Fan(DeviceRowID, Date);");
	query("create index if not exists fc_id_idx       on Fan_Calendar(DeviceRowID);");
	query("create index if not exists fc_id_date_idx  on Fan_Calendar(DeviceRowID, Date);");
	query("create index if not exists ll_id_idx       on LightingLog(DeviceRowID);");
	query("create index if not exists ll_id_date_idx  on LightingLog(DeviceRowID, Date);");
	query("create index if not exists sl_id_idx       on SceneLog(SceneRowID);");
	query("create index if not exists sl_id_date_idx  on SceneLog(SceneRowID, Date);");
	query("create index if not exists m_id_idx        on Meter(DeviceRowID);");
	query("create index if not exists m_id_date_idx   on Meter(DeviceRowID, Date);");
	query("create index if not exists mc_id_idx       on Meter_Calendar(DeviceRowID);");
	query("create index if not exists mc_id_date_idx  on Meter_Calendar(DeviceRowID, Date);");
	query("create index if not exists mm_id_idx       on MultiMeter(DeviceRowID);");
	query("create index if not exists mm_id_date_idx  on MultiMeter(DeviceRowID, Date);");
	query("create index if not exists mmc_id_idx      on MultiMeter_Calendar(DeviceRowID);");
	query("create index if not exists mmc_id_date_idx on MultiMeter_Calendar(DeviceRowID, Date);");
	query("create index if not exists p_id_idx        on Percentage(DeviceRowID);");
	query("create index if not exists p_id_date_idx   on Percentage(DeviceRowID, Date);");
	query("create index if not exists pc_id_idx       on Percentage_Calendar(DeviceRowID);");
	query("create index if not exists pc_id_date_idx  on Percentage_Calendar(DeviceRowID, Date);");
	query("create index if not exists r_id_idx        on Rain(DeviceRowID);");
	query("create index if not exists r_id_date_idx   on Rain(DeviceRowID, Date);");
	query("create index if not exists rc_id_idx       on Rain_Calendar(DeviceRowID);");
	query("create index if not exists rc_id_date_idx  on Rain_Calendar(DeviceRowID, Date);");
	query("create index if not exists t_id_idx        on Temperature(DeviceRowID);");
	query("create index if not exists t_id_date_idx   on Temperature(DeviceRowID, Date);");
	query("create index if not exists tc_id_idx       on Temperature_Calendar(DeviceRowID);");
	query("create index if not exists tc_id_date_idx  on Temperature_Calendar(DeviceRowID, Date);");
	query("create index if not exists u_id_idx        on UV(DeviceRowID);");
	query("create index if not exists u_id_date_idx   on UV(DeviceRowID, Date);");
	query("create index if not exists uv_id_idx       on UV_Calendar(DeviceRowID);");
	query("create index if not exists uv_id_date_idx  on UV_Calendar(DeviceRowID, Date);");
	query("create index if not exists w_id_idx        on Wind(DeviceRowID);");
	query("create index if not exists w_id_date_idx   on Wind(DeviceRowID, Date);");
	query("create index if not exists wc_id_idx       on Wind_Calendar(DeviceRowID);");
	query("create index if not exists wc_id_date_idx  on Wind_Calendar(DeviceRowID, Date);");

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

			std::vector<std::vector<std::string> > result;
			result = safe_query("SELECT ID FROM DeviceStatus WHERE (Type=%d)", pTypeP1Power);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					safe_query("DELETE FROM MultiMeter WHERE (DeviceRowID='%q') AND (Date>='%q')", idx.c_str(), szDateStart);
				}
			}
		}
		if (dbversion < 11)
		{
			std::vector<std::vector<std::string> > result;

			result = safe_query("SELECT ID, Username, Password FROM Cameras ORDER BY ID");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string camuser = base64_encode((const unsigned char*)sd[1].c_str(), sd[1].size());
					std::string campwd = base64_encode((const unsigned char*)sd[2].c_str(), sd[2].size());
					safe_query("UPDATE Cameras SET Username='%q', Password='%q' WHERE (ID=='%q')",
						camuser.c_str(), campwd.c_str(), sd[0].c_str());
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
					safe_query("DELETE FROM MultiMeter_Calendar WHERE (RowID=='%q')", sd[0].c_str());
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
				sqlite3_exec(m_dbase, "BEGIN TRANSACTION;", NULL, NULL, NULL);
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					safe_query("UPDATE Temperature_Calendar SET Temp_Avg=%.1f WHERE RowID='%q'", atof(sd[1].c_str()), sd[0].c_str());
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

			std::vector<std::vector<std::string> > result;
			result = query("SELECT ID FROM DeviceStatus WHERE (DeviceID LIKE 'WMI%')");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					safe_query("DELETE FROM Temperature WHERE (DeviceRowID='%q')", idx.c_str());
					safe_query("DELETE FROM Temperature_Calendar WHERE (DeviceRowID='%q')", idx.c_str());
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
			result = safe_query("SELECT ID FROM DeviceStatus WHERE (Type=%d)", pTypeUsage);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					safe_query("UPDATE Meter SET Value = Value * 10 WHERE (DeviceRowID='%q')", idx.c_str());
					safe_query("UPDATE Meter_Calendar SET Value = Value * 10 WHERE (DeviceRowID='%q')", idx.c_str());
					safe_query("UPDATE MultiMeter_Calendar SET Value1 = Value1 * 10, Value2 = Value2 * 10 WHERE (DeviceRowID='%q')", idx.c_str());
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
			std::vector<std::vector<std::string> > result;
			result = safe_query("SELECT ID FROM Hardware WHERE (Type==%d) AND (Name=='Motherboard') LIMIT 1", HTYPE_System);
			if (!result.empty())
			{
				int hwId = atoi(result[0][0].c_str());
				safe_query("UPDATE DeviceStatus SET HardwareID=%d WHERE (HardwareID=1000)", hwId);
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
			result2 = query(szQuery2.str());
			if (result2.size() > 0)
			{
				std::string pwd = result2[0][0];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Preferences SET sValue='" << pHash << "' WHERE (Key='WebPassword')";
					query(szQuery2.str());
				}
			}

			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT sValue FROM Preferences WHERE (Key='SecPassword')";
			result2 = query(szQuery2.str());
			if (result2.size() > 0)
			{
				std::string pwd = result2[0][0];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Preferences SET sValue='" << pHash << "' WHERE (Key='SecPassword')";
					query(szQuery2.str());
				}
			}

			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT sValue FROM Preferences WHERE (Key='ProtectionPassword')";
			result2 = query(szQuery2.str());
			if (result2.size() > 0)
			{
				std::string pwd = result2[0][0];
				if (pwd.size() != 32)
				{
					pHash = GenerateMD5Hash(base64_decode(pwd));
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Preferences SET sValue='" << pHash << "' WHERE (Key='ProtectionPassword')";
					query(szQuery2.str());
				}
			}
			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT ID, Password FROM Users";
			result2 = query(szQuery2.str());
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
					query(szQuery2.str());
				}
			}

			szQuery2.clear();
			szQuery2.str("");
			szQuery2 << "SELECT ID, Password FROM Hardware WHERE ([Type]==" << HTYPE_Domoticz << ")";
			result2 = query(szQuery2.str());
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
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 57)
		{
			//S0 Meter patch
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID, Mode1, Mode2, Mode3, Mode4 FROM HARDWARE WHERE([Type]==" << HTYPE_S0SmartMeterUSB << ")";
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
			result = query("SELECT ID FROM Hardware WHERE ([Type] = 9) OR ([Type] = 21)");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					szQuery2.clear();
					szQuery2.str("");
					//#define sTypeZWaveSwitch 0xA1
					szQuery2 << "UPDATE DeviceStatus SET SubType=" << 0xA1 << " WHERE ([Type]=" << pTypeLighting2 << ") AND (SubType=" << sTypeAC << ") AND (HardwareID=" << result[0][0] << ")";
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
					result3 = query(szQuery.str());
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
					result3 = query(szQuery.str());
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
			UpdatePreferencesVar("PushsaferEnabled", 1);
			UpdatePreferencesVar("ClickatellEnabled", 1);
		}
		if (dbversion < 68)
		{
			query("ALTER TABLE Notifications ADD COLUMN [CustomMessage] VARCHAR(300) DEFAULT ('')");
			query("ALTER TABLE Notifications ADD COLUMN [ActiveSystems] VARCHAR(300) DEFAULT ('')");
		}
		if (dbversion < 69)
		{
			//Serial Port patch (using complete port paths now)
			query("ALTER TABLE Hardware ADD COLUMN [SerialPort] VARCHAR(50) DEFAULT ('')");

			//Convert all serial hardware to use the new column
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			std::vector<std::vector<std::string> >::const_iterator itt;
			szQuery << "SELECT ID,[Type],Port FROM Hardware";
			result = query(szQuery.str());
			if (!result.empty())
			{
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					int hwId = atoi(sd[0].c_str());
					_eHardwareTypes hwtype = (_eHardwareTypes)atoi(sd[1].c_str());
					size_t Port = (size_t)atoi(sd[2].c_str());

					if (IsSerialDevice(hwtype))
					{
						char szSerialPort[50];
#if defined WIN32
						sprintf(szSerialPort, "COM%d", (int)Port);
#else
						bool bUseDirectPath = false;
						std::vector<std::string> serialports = GetSerialPorts(bUseDirectPath);
						if (bUseDirectPath)
						{
							if (Port >= serialports.size())
							{
								_log.Log(LOG_ERROR, "Serial Port out of range!...");
								continue;
							}
							strcpy(szSerialPort, serialports[Port].c_str());
						}
						else
						{
							sprintf(szSerialPort, "/dev/ttyUSB%d", (int)Port);
						}
#endif
						safe_query("UPDATE Hardware SET Port=0, SerialPort='%q' WHERE (ID=%d)",
							szSerialPort, hwId);
					}
				}
			}
		}
		if (dbversion < 70)
		{
			query("ALTER TABLE [WOLNodes] ADD COLUMN [Timeout] INTEGER DEFAULT 5");
		}
		if (dbversion < 71)
		{
			//Dropping debug cleanup triggers
			query("DROP TRIGGER IF EXISTS onTemperatureDelete");
			query("DROP TRIGGER IF EXISTS onRainDelete");
			query("DROP TRIGGER IF EXISTS onWindDelete");
			query("DROP TRIGGER IF EXISTS onUVDelete");
			query("DROP TRIGGER IF EXISTS onMeterDelete");
			query("DROP TRIGGER IF EXISTS onMultiMeterDelete");
			query("DROP TRIGGER IF EXISTS onPercentageDelete");
			query("DROP TRIGGER IF EXISTS onFanDelete");
		}
		if (dbversion < 72)
		{
			query("ALTER TABLE [Notifications] ADD COLUMN [SendAlways] INTEGER DEFAULT 0");
		}
		if (dbversion < 73)
		{
			if (!DoesColumnExistsInTable("Description", "DeviceStatus"))
			{
				query("ALTER TABLE DeviceStatus ADD COLUMN [Description] VARCHAR(200) DEFAULT ''");
			}
			query("ALTER TABLE Scenes ADD COLUMN [Description] VARCHAR(200) DEFAULT ''");
		}
		if (dbversion < 74)
		{
			if (!DoesColumnExistsInTable("Description", "DeviceStatus"))
			{
				query("ALTER TABLE DeviceStatus ADD COLUMN [Description] VARCHAR(200) DEFAULT ''");
			}
		}
		if (dbversion < 75)
		{
			safe_query("UPDATE Hardware SET Username='%q', Password='%q' WHERE ([Type]=%d)",
				"Change_user_pass", "", HTYPE_THERMOSMART);
			if (!DoesColumnExistsInTable("Description", "DeviceStatus"))
			{
				query("ALTER TABLE DeviceStatus ADD COLUMN [Description] VARCHAR(200) DEFAULT ''");
			}
		}
		if (dbversion < 76)
		{
			if (!DoesColumnExistsInTable("Name", "MySensorsChilds"))
			{
				query("ALTER TABLE MySensorsChilds ADD COLUMN [Name] VARCHAR(100) DEFAULT ''");
			}
			if (!DoesColumnExistsInTable("UseAck", "MySensorsChilds"))
			{
				query("ALTER TABLE MySensorsChilds ADD COLUMN [UseAck] INTEGER DEFAULT 0");
			}
		}
		if (dbversion < 77)
		{
			//Simplify Scenes table, and add support for multiple activators
			query("ALTER TABLE Scenes ADD COLUMN [Activators] VARCHAR(200) DEFAULT ''");
			std::vector<std::vector<std::string> > result, result2;
			std::vector<std::vector<std::string> >::const_iterator itt;
			result = safe_query("SELECT ID, HardwareID, DeviceID, Unit, [Type], SubType, SceneType, ListenCmd FROM Scenes");
			if (!result.empty())
			{
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string Activator("");
					result2 = safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%q) AND (DeviceID=='%q') AND (Unit==%q) AND ([Type]==%q) AND (SubType==%q)",
						sd[1].c_str(), sd[2].c_str(), sd[3].c_str(), sd[4].c_str(), sd[5].c_str());
					if (!result2.empty())
					{
						Activator = result2[0][0];
						if (sd[6] == "0") { //Scene
							Activator += ":" + sd[7];
						}
					}
					safe_query("UPDATE Scenes SET Activators='%q' WHERE (ID==%q)", Activator.c_str(), sd[0].c_str());
				}
			}
			//create a backup
			query("ALTER TABLE Scenes RENAME TO tmp_Scenes");
			//Create the new table
			query(sqlCreateScenes);
			//Copy values from tmp_Scenes back into our new table
			query(
				"INSERT INTO Scenes ([ID],[Name],[Favorite],[Order],[nValue],[SceneType],[LastUpdate],[Protected],[OnAction],[OffAction],[Description],[Activators])"
				"SELECT [ID],[Name],[Favorite],[Order],[nValue],[SceneType],[LastUpdate],[Protected],[OnAction],[OffAction],[Description],[Activators] FROM tmp_Scenes");
			//Drop the tmp table
			query("DROP TABLE tmp_Scenes");
		}
		if (dbversion < 78)
		{
			//Patch for soil moisture to use large ID
			result = safe_query("SELECT ID, DeviceID FROM DeviceStatus WHERE (Type=%d) AND (SubType=%d)", pTypeGeneral, sTypeSoilMoisture);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string idx = sd[0];
					int lid = atoi(sd[1].c_str());
					char szTmp[20];
					sprintf(szTmp, "%08X", lid);
					safe_query("UPDATE DeviceStatus SET DeviceID='%q' WHERE (ID='%q')", szTmp, idx.c_str());
				}
			}
		}
		if (dbversion < 79)
		{
			//MQTT filename for ca file
			query("ALTER TABLE Hardware ADD COLUMN [Extra] VARCHAR(200) DEFAULT ('')");
		}
		if (dbversion < 81)
		{
			// MQTT set default mode
			std::stringstream szQuery2;
			szQuery2 << "UPDATE Hardware SET Mode1=1 WHERE  ([Type]==" << HTYPE_MQTT << " )";
			query(szQuery2.str());
		}
		if (dbversion < 82)
		{
			//pTypeEngery sensor to new kWh sensor
			std::vector<std::vector<std::string> > result2;
			std::vector<std::vector<std::string> >::const_iterator itt2	;
			result2 = safe_query("SELECT ID, DeviceID FROM DeviceStatus WHERE ([Type] = %d)", pTypeENERGY);
			for (itt2 = result2.begin(); itt2 != result2.end(); ++itt2)
			{
				std::vector<std::string> sd2 = *itt2;

				//Change type to new sensor, and update ID
				int oldID = atoi(sd2[1].c_str());
				char szTmp[20];
				sprintf(szTmp, "%08X", oldID);
				safe_query("UPDATE DeviceStatus SET [DeviceID]='%s', [Type]=%d, [SubType]=%d, [Unit]=1 WHERE (ID==%s)", szTmp, pTypeGeneral, sTypeKwh, sd2[0].c_str());

				//meter table
				safe_query("UPDATE Meter SET Value=Value/100, Usage=Usage*10 WHERE DeviceRowID=%s", sd2[0].c_str());
				//meter_calendar table
				safe_query("UPDATE Meter_Calendar SET Value=Value/100, Counter=Counter/100 WHERE (DeviceRowID==%s)", sd2[0].c_str());
			}
		}
		if (dbversion < 83)
		{
			//Add hardware monitor as normal hardware class (if not already added)
			std::vector<std::vector<std::string> > result;
			result = safe_query("SELECT ID FROM Hardware WHERE (Type==%d)", HTYPE_System);
			if (result.size() < 1)
			{
				m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) VALUES ('Motherboard',1, %d,'',1,'','',0,0,0,0,0,0)", HTYPE_System);
			}
		}
		if (dbversion < 85)
		{
			//MySensors, default use ACK for Childs
			safe_query("UPDATE MySensorsChilds SET[UseAck] = 1 WHERE(ChildID != 255)");
		}
		if (dbversion < 86)
		{
			//MySensors add Name field
			query("ALTER TABLE MySensors ADD COLUMN [Name] VARCHAR(100) DEFAULT ''");
			safe_query("UPDATE MySensors SET [Name] = [SketchName]");
		}
		if (dbversion < 87)
		{
			//MySensors change waterflow percentage sensor to a real waterflow sensor
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;
			std::vector<std::vector<std::string> >::const_iterator itt;
			szQuery << "SELECT HardwareID,NodeID,ChildID FROM MySensorsChilds WHERE ([Type]==" << 21 << ")";
			result = query(szQuery.str());
			for (itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				int hwid = atoi(sd[0].c_str());
				int nodeid = atoi(sd[1].c_str());
				//int childid = atoi(sd[2].c_str());

				szQuery.clear();
				szQuery.str("");

				char szID[20];
				sprintf(szID, "%08X", nodeid);

				szQuery << "UPDATE DeviceStatus SET SubType=" << sTypeWaterflow << " WHERE ([Type]=" << pTypeGeneral << ") AND (SubType=" << sTypePercentage << ") AND (HardwareID=" << hwid << ") AND (DeviceID='" << szID << "')";
				query(szQuery.str());
			}
		}
		if (dbversion < 88)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [Options] VARCHAR(1024) DEFAULT null");
		}
		if (dbversion < 89)
		{
			std::stringstream szQuery;
			szQuery << "UPDATE DeviceStatus SET [DeviceID]='0' || DeviceID WHERE ([Type]=" << pTypeGeneralSwitch << ") AND (SubType=" << sSwitchTypeSelector << ") AND length(DeviceID) = 7";
			query(szQuery.str());
		}
		if (dbversion < 90)
		{
			if (!DoesColumnExistsInTable("Interpreter", "EventMaster"))
			{
				query("ALTER TABLE EventMaster ADD COLUMN [Interpreter] VARCHAR(10) DEFAULT 'Blockly'");
			}
			if (!DoesColumnExistsInTable("Type", "EventMaster"))
			{
				query("ALTER TABLE EventMaster ADD COLUMN [Type] VARCHAR(10) DEFAULT 'All'");
			}
		}
		if (dbversion < 91)
		{
			//Add DomoticzInternal as normal hardware class (if not already added)
			int hwdID = -1;
			std::string securityPanelDeviceID = "148702"; // 0x00148702
			std::vector<std::vector<std::string> > result;
			result = safe_query("SELECT ID FROM Hardware WHERE (Type==%d)", HTYPE_DomoticzInternal);
			if (result.size() < 1) {
				m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) VALUES ('Domoticz Internal',1, %d,'',1,'','',0,0,0,0,0,0)", HTYPE_DomoticzInternal);
				result = safe_query("SELECT ID FROM Hardware WHERE (Type==%d)", HTYPE_DomoticzInternal);
			}
			if (result.size() > 0) {
				hwdID = atoi(result[0][0].c_str());
			}
			if (hwdID > 0) {
				// Update HardwareID for Security Panel device
				int oldHwdID = 1000;
				result = safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID='%q') AND (Type=%d) AND (SubType=%d)", oldHwdID, securityPanelDeviceID.c_str(), pTypeSecurity1, sTypeDomoticzSecurity);
				if (result.size() > 0)
				{
					m_sql.safe_query("UPDATE DeviceStatus SET HardwareID=%d WHERE (HardwareID==%d) AND (DeviceID='%q') AND (Type=%d) AND (SubType=%d)", hwdID, oldHwdID, securityPanelDeviceID.c_str(), pTypeSecurity1, sTypeDomoticzSecurity);
				}
				// Update Name for Security Panel device
				result = safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID='%q') AND (Type=%d) AND (SubType=%d) AND Name='Unknown'", hwdID, securityPanelDeviceID.c_str(), pTypeSecurity1, sTypeDomoticzSecurity);
				if (result.size() > 0)
				{
					m_sql.safe_query("UPDATE DeviceStatus SET Name='Domoticz Security Panel' WHERE (HardwareID==%d) AND (DeviceID='%q') AND (Type=%d) AND (SubType=%d) AND Name='Unknown'", hwdID, securityPanelDeviceID.c_str(), pTypeSecurity1, sTypeDomoticzSecurity);
				}
			}
		}
		if (dbversion < 92) {
			// Change DeviceStatus.Options datatype from VARCHAR(1024) to TEXT
			std::string tableName = "DeviceStatus";
			std::string fieldList = "[ID],[HardwareID],[DeviceID],[Unit],[Name],[Used],[Type],[SubType],[SwitchType],[Favorite],[SignalLevel],[BatteryLevel],[nValue],[sValue],[LastUpdate],[Order],[AddjValue],[AddjMulti],[AddjValue2],[AddjMulti2],[StrParam1],[StrParam2],[LastLevel],[Protected],[CustomImage],[Description],[Options]";
			std::stringstream szQuery;

			sqlite3_exec(m_dbase, "PRAGMA foreign_keys=off", NULL, NULL, NULL);
			sqlite3_exec(m_dbase, "BEGIN TRANSACTION", NULL, NULL, NULL);

			// Drop indexes and trigger
			safe_query("DROP TRIGGER IF EXISTS devicestatusupdate");
			// Save all table rows
			szQuery.clear();
			szQuery.str("");
			szQuery << "ALTER TABLE " << tableName << " RENAME TO _" << tableName << "_old";
			safe_query(szQuery.str().c_str());
			// Create new table
			safe_query(sqlCreateDeviceStatus);
			// Restore all table rows
			szQuery.clear();
			szQuery.str("");
			szQuery << "INSERT INTO " << tableName << " (" << fieldList << ") SELECT " << fieldList << " FROM _" << tableName << "_old";
			safe_query(szQuery.str().c_str());
			// Restore indexes and triggers
			safe_query(sqlCreateDeviceStatusTrigger);
			// Delete old table
			szQuery.clear();
			szQuery.str("");
			szQuery << "DROP TABLE IF EXISTS _" << tableName << "_old";
			safe_query(szQuery.str().c_str());

			sqlite3_exec(m_dbase, "END TRANSACTION", NULL, NULL, NULL);
			sqlite3_exec(m_dbase, "PRAGMA foreign_keys=on", NULL, NULL, NULL);
		}
		if (dbversion < 93)
		{
			if (!DoesColumnExistsInTable("Month", "Timers"))
			{
				query("ALTER TABLE Timers ADD COLUMN [Month] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("MDay", "Timers"))
			{
				query("ALTER TABLE Timers ADD COLUMN [MDay] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("Occurence", "Timers"))
			{
				query("ALTER TABLE Timers ADD COLUMN [Occurence] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("Month", "SceneTimers"))
			{
				query("ALTER TABLE SceneTimers ADD COLUMN [Month] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("MDay", "SceneTimers"))
			{
				query("ALTER TABLE SceneTimers ADD COLUMN [MDay] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("Occurence", "SceneTimers"))
			{
				query("ALTER TABLE SceneTimers ADD COLUMN [Occurence] INTEGER DEFAULT 0");
			}
		}
		if (dbversion < 94)
		{
			std::stringstream szQuery;
			szQuery << "UPDATE Timers SET [Type]=[Type]+2 WHERE ([Type]>" << TTYPE_FIXEDDATETIME << ")";
			query(szQuery.str());
			szQuery.clear();
			szQuery.str("");
			szQuery << "UPDATE SceneTimers SET [Type]=[Type]+2 WHERE ([Type]>" << TTYPE_FIXEDDATETIME << ")";
			query(szQuery.str());
		}
		if (dbversion < 95)
		{
			if (!DoesColumnExistsInTable("Month", "SetpointTimers"))
			{
				query("ALTER TABLE SetpointTimers ADD COLUMN [Month] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("MDay", "SetpointTimers"))
			{
				query("ALTER TABLE SetpointTimers ADD COLUMN [MDay] INTEGER DEFAULT 0");
			}
			if (!DoesColumnExistsInTable("Occurence", "SetpointTimers"))
			{
				query("ALTER TABLE SetpointTimers ADD COLUMN [Occurence] INTEGER DEFAULT 0");
			}
		}
		if (dbversion < 96)
		{
			if (!DoesColumnExistsInTable("Name", "MobileDevices"))
			{
				query("ALTER TABLE MobileDevices ADD COLUMN [Name] VARCHAR(100) DEFAULT ''");
			}
		}
		if (dbversion < 97)
		{
			//Patch for pTypeLighting2/sTypeZWaveSwitch to pTypeGeneralSwitch/sSwitchGeneralSwitch
			std::stringstream szQuery,szQuery2;
			std::vector<std::vector<std::string> > result, result2;
			std::vector<std::string> sd;
			result = query("SELECT ID FROM Hardware WHERE ([Type] = 9) OR ([Type] = 21)");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					sd = *itt;
					szQuery.clear();
					szQuery.str("");
					szQuery << "SELECT ID, DeviceID FROM DeviceStatus WHERE ([Type]=" << pTypeLighting2 << ") AND (SubType=" << 0xA1 << ") AND (HardwareID=" << sd[0] << ")";
					result2 = query(szQuery.str());
					if (result2.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt2;
						for (itt2 = result2.begin(); itt2 != result2.end(); ++itt2)
						{
							sd = *itt2;
							std::string ndeviceid = "0" + sd[1];
							szQuery2.clear();
							szQuery2.str("");
							//#define sTypeZWaveSwitch 0xA1
							szQuery2 << "UPDATE DeviceStatus SET DeviceID='" << ndeviceid << "', [Type]=" << pTypeGeneralSwitch << ", SubType=" << sSwitchGeneralSwitch << " WHERE (ID=" << sd[0] << ")";
							query(szQuery2.str());
						}
					}
				}
			}
		}
		if (dbversion < 98)
		{
			// Shorthen cookies validity to 30 days
			query("UPDATE UserSessions SET ExpirationDate = datetime(ExpirationDate, '-335 days')");
		}
		if (dbversion < 99)
		{
			//Convert depricated CounterType 'Time' to type Counter with options ValueQuantity='Time' & ValueUnits='Min'
			//Add options ValueQuantity='Count' to existing CounterType 'Counter'
			const unsigned char charNTYPE_TODAYTIME = 'm';
			std::stringstream szQuery, szQuery1, szQuery2, szQuery3;
			std::vector<std::vector<std::string> > result, result1;
			std::vector<std::string> sd;
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT ID FROM Hardware"
				" WHERE (([Type] = " << HTYPE_Dummy << ") OR ([Type] = " << HTYPE_YouLess << "))";
			result = query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					sd = *itt;
					szQuery1.clear();
					szQuery1.str("");
					szQuery1 << "SELECT ID, DeviceID, SwitchType FROM DeviceStatus"
						" WHERE ((([Type]=" << pTypeRFXMeter << ") AND (SubType=" << sTypeRFXMeterCount << "))"
						" OR (([Type]=" << pTypeGeneral << ") AND (SubType=" << sTypeCounterIncremental << "))"
						" OR ([Type]=" << pTypeYouLess << "))"
						" AND ((SwitchType=" << MTYPE_COUNTER << ") OR (SwitchType=" << MTYPE_TIME << "))"
						" AND (HardwareID=" << sd[0] << ")";
					result1 = query(szQuery1.str());
					if (result1.size() > 0)
					{
						std::vector<std::vector<std::string> >::const_iterator itt2;
						for (itt2 = result1.begin(); itt2 != result1.end(); ++itt2)
						{
							sd = *itt2;
							uint64_t devidx = atoi(sd[0].c_str());
							_eMeterType switchType = (_eMeterType)atoi(sd[2].c_str());

							if (switchType == MTYPE_COUNTER)
							{
								//Add options to existing SwitchType 'Counter'
								m_sql.SetDeviceOptions(devidx, m_sql.BuildDeviceOptions("ValueQuantity:Count;ValueUnits:", false));
							}
							else if (switchType == MTYPE_TIME)
							{
								//Set default options
								m_sql.SetDeviceOptions(devidx, m_sql.BuildDeviceOptions("ValueQuantity:Time;ValueUnits:Min", false));

								//Convert to Counter
								szQuery2.clear();
								szQuery2.str("");
								szQuery2 << "UPDATE DeviceStatus"
									" SET SwitchType=" << MTYPE_COUNTER << " WHERE (ID=" << devidx << ")";
								query(szQuery2.str());

								//Update notifications 'Time' -> 'Counter'
								szQuery3.clear();
								szQuery3.str("");
								szQuery3 << "UPDATE Notifications"
									" SET Params=REPLACE(Params, '" << charNTYPE_TODAYTIME  << ";', '" << Notification_Type_Desc(NTYPE_TODAYCOUNTER, 1) << ";')"
									" WHERE (DeviceRowID=" << devidx << ")";
								query(szQuery3.str());
							}
						}
					}
				}
			}
		}
		if (dbversion < 100)
		{
			//Convert temperature sensor type sTypeTEMP10 to sTypeTEMP5 for specified hardware classes
			std::stringstream szQuery, szQuery2;
			std::vector<std::vector<std::string> > result;
			std::vector<std::string> sd;
			szQuery << "SELECT ID FROM Hardware WHERE ( "
				<< "([Type] = " << HTYPE_OpenThermGateway << ") OR "
				<< "([Type] = " << HTYPE_OpenThermGatewayTCP << ") OR "
				<< "([Type] = " << HTYPE_DavisVantage << ") OR "
				<< "([Type] = " << HTYPE_System << ") OR "
				<< "([Type] = " << HTYPE_ICYTHERMOSTAT << ") OR "
				<< "([Type] = " << HTYPE_Meteostick << ") OR "
				<< "([Type] = " << HTYPE_PVOUTPUT_INPUT << ") OR "
				<< "([Type] = " << HTYPE_SBFSpot << ") OR "
				<< "([Type] = " << HTYPE_SolarEdgeTCP << ") OR "
				<< "([Type] = " << HTYPE_TE923 << ") OR "
				<< "([Type] = " << HTYPE_TOONTHERMOSTAT << ") OR "
				<< "([Type] = " << HTYPE_Wunderground << ") OR "
				<< "([Type] = " << HTYPE_DarkSky << ") OR "
				<< "([Type] = " << HTYPE_AccuWeather << ") OR "
				<< "([Type] = " << HTYPE_OpenZWave << ")"
				<< ")";
			result = query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					sd = *itt;
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE DeviceStatus SET SubType=" << sTypeTEMP5 << " WHERE ([Type]==" << pTypeTEMP << ") AND (SubType==" << sTypeTEMP10 << ") AND (HardwareID=" << sd[0] << ")";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 101)
		{
			//Convert TempHum sensor type sTypeTH2 to sTypeHUM1 for specified hardware classes
			std::stringstream szQuery, szQuery2;
			std::vector<std::vector<std::string> > result;
			std::vector<std::string> sd;
			szQuery << "SELECT ID FROM Hardware WHERE ( "
				<< "([Type] = " << HTYPE_DavisVantage << ") OR "
				<< "([Type] = " << HTYPE_TE923 << ") OR "
				<< "([Type] = " << HTYPE_OpenZWave << ")"
				<< ")";
			result = query(szQuery.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					sd = *itt;
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE DeviceStatus SET SubType=" << sTypeHUM1 << " WHERE ([Type]==" << pTypeHUM << ") AND (SubType==" << sTypeTH2 << ") AND (HardwareID=" << sd[0] << ")";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 102)
		{
			// Remove old indexes
			query("drop index if exists f_idx;");
			query("drop index if exists fc_idx;");
			query("drop index if exists l_idx;");
			query("drop index if exists s_idx;");
			query("drop index if exists m_idx;");
			query("drop index if exists mc_idx;");
			query("drop index if exists mm_idx;");
			query("drop index if exists mmc_idx;");
			query("drop index if exists p_idx;");
			query("drop index if exists pc_idx;");
			query("drop index if exists r_idx;");
			query("drop index if exists rc_idx;");
			query("drop index if exists t_idx;");
			query("drop index if exists tc_idx;");
			query("drop index if exists u_idx;");
			query("drop index if exists uv_idx;");
			query("drop index if exists w_idx;");
			query("drop index if exists wc_idx;");
			// Add new indexes
			query("create index if not exists ds_hduts_idx    on DeviceStatus(HardwareID, DeviceID, Unit, Type, SubType);");
			query("create index if not exists f_id_idx        on Fan(DeviceRowID);");
			query("create index if not exists f_id_date_idx   on Fan(DeviceRowID, Date);");
			query("create index if not exists fc_id_idx       on Fan_Calendar(DeviceRowID);");
			query("create index if not exists fc_id_date_idx  on Fan_Calendar(DeviceRowID, Date);");
			query("create index if not exists ll_id_idx       on LightingLog(DeviceRowID);");
			query("create index if not exists ll_id_date_idx  on LightingLog(DeviceRowID, Date);");
			query("create index if not exists sl_id_idx       on SceneLog(SceneRowID);");
			query("create index if not exists sl_id_date_idx  on SceneLog(SceneRowID, Date);");
			query("create index if not exists m_id_idx        on Meter(DeviceRowID);");
			query("create index if not exists m_id_date_idx   on Meter(DeviceRowID, Date);");
			query("create index if not exists mc_id_idx       on Meter_Calendar(DeviceRowID);");
			query("create index if not exists mc_id_date_idx  on Meter_Calendar(DeviceRowID, Date);");
			query("create index if not exists mm_id_idx       on MultiMeter(DeviceRowID);");
			query("create index if not exists mm_id_date_idx  on MultiMeter(DeviceRowID, Date);");
			query("create index if not exists mmc_id_idx      on MultiMeter_Calendar(DeviceRowID);");
			query("create index if not exists mmc_id_date_idx on MultiMeter_Calendar(DeviceRowID, Date);");
			query("create index if not exists p_id_idx        on Percentage(DeviceRowID);");
			query("create index if not exists p_id_date_idx   on Percentage(DeviceRowID, Date);");
			query("create index if not exists pc_id_idx       on Percentage_Calendar(DeviceRowID);");
			query("create index if not exists pc_id_date_idx  on Percentage_Calendar(DeviceRowID, Date);");
			query("create index if not exists r_id_idx        on Rain(DeviceRowID);");
			query("create index if not exists r_id_date_idx   on Rain(DeviceRowID, Date);");
			query("create index if not exists rc_id_idx       on Rain_Calendar(DeviceRowID);");
			query("create index if not exists rc_id_date_idx  on Rain_Calendar(DeviceRowID, Date);");
			query("create index if not exists t_id_idx        on Temperature(DeviceRowID);");
			query("create index if not exists t_id_date_idx   on Temperature(DeviceRowID, Date);");
			query("create index if not exists tc_id_idx       on Temperature_Calendar(DeviceRowID);");
			query("create index if not exists tc_id_date_idx  on Temperature_Calendar(DeviceRowID, Date);");
			query("create index if not exists u_id_idx        on UV(DeviceRowID);");
			query("create index if not exists u_id_date_idx   on UV(DeviceRowID, Date);");
			query("create index if not exists uv_id_idx       on UV_Calendar(DeviceRowID);");
			query("create index if not exists uv_id_date_idx  on UV_Calendar(DeviceRowID, Date);");
			query("create index if not exists w_id_idx        on Wind(DeviceRowID);");
			query("create index if not exists w_id_date_idx   on Wind(DeviceRowID, Date);");
			query("create index if not exists wc_id_idx       on Wind_Calendar(DeviceRowID);");
			query("create index if not exists wc_id_date_idx  on Wind_Calendar(DeviceRowID, Date);");
		}
		if (dbversion < 103)
		{
			FixDaylightSaving();
		}
		if (dbversion < 104)
		{
			//S0 Meter patch
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID, Address FROM Hardware WHERE([Type]==" << HTYPE_S0SmartMeterUSB << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Hardware SET Extra='" << sd[1] << "', Address='' WHERE (ID=" << sd[0] << ")";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 105)
		{
			if (!DoesColumnExistsInTable("AckTimeout", "MySensorsChilds"))
			{
				query("ALTER TABLE MySensorsChilds ADD COLUMN [AckTimeout] INTEGER DEFAULT 1200");
			}
		}
		if (dbversion < 106)
		{
			//Adjust Limited device id's to uppercase HEX
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID, DeviceID FROM DeviceStatus WHERE([Type]==" << pTypeLimitlessLights << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					szQuery2.clear();
					szQuery2.str("");
					uint32_t lID;
					std::stringstream s_strid;
					s_strid << std::hex << sd[1];
					s_strid >> lID;

					if (lID > 9)
					{
						char szTmp[10];
						sprintf(szTmp, "%08X", lID);
						szQuery2 << "UPDATE DeviceStatus SET DeviceID='" << szTmp << "' WHERE (ID=" << sd[0] << ")";
						query(szQuery2.str());
					}
				}
			}
		}
		if (dbversion < 107)
		{
			if (!DoesColumnExistsInTable("User", "LightingLog"))
			{
				query("ALTER TABLE LightingLog ADD COLUMN [User] VARCHAR(100) DEFAULT ('')");
			}
		}
		if (dbversion < 108)
		{
			//Fix possible HTTP notifier problem
			std::string sValue;
			GetPreferencesVar("HTTPPostContentType", sValue);
			if ((sValue.size()>100)||(sValue.empty()))
			{
				sValue = "application/json";
				std::string sencoded = base64_encode((const unsigned char*)sValue.c_str(), sValue.size());
				UpdatePreferencesVar("HTTPPostContentType", sencoded);
			}
		}
		if (dbversion < 109)
		{
			query("INSERT INTO TimerPlans (ID, Name) VALUES (0, 'default')");
			query("INSERT INTO TimerPlans (ID, Name) VALUES (1, 'Holiday')");
		}
		if (dbversion < 110)
		{
			query("ALTER TABLE Hardware RENAME TO tmp_Hardware;");
			query(sqlCreateHardware);
			query("INSERT INTO Hardware(ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout) SELECT ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM tmp_Hardware;");
			query("DROP TABLE tmp_Hardware;");

			result = safe_query("SELECT ID, Extra FROM Hardware WHERE Type=%d", HTYPE_HTTPPOLLER);
			if (result.size() > 0)
			{
				std::stringstream szQuery2;
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string id = sd[0];
					std::string extra = sd[1];
					std::string extraBase64=base64_encode((const unsigned char *)extra.c_str(), extra.length());
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE Hardware SET Mode1=0, Extra='%s' WHERE (ID=" << id << ")";
					safe_query(szQuery2.str().c_str(), extraBase64.c_str());
				}
			}
		}
		if (dbversion < 111)
		{
			//SolarEdge API, no need for Serial Number anymore
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID, Password FROM Hardware WHERE([Type]==" << HTYPE_SolarEdgeAPI << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					safe_query("UPDATE Hardware SET Username='%q', Password='' WHERE (ID=%s)", sd[1].c_str(), sd[0].c_str());
				}
			}
		}
		if (dbversion < 112)
		{
			//Fix for MySensor general switch with wrong subtype
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID FROM Hardware WHERE ([Type]==" << HTYPE_MySensorsTCP << ") OR ([Type]==" << HTYPE_MySensorsUSB << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					szQuery2.clear();
					szQuery2.str("");
					szQuery2 << "UPDATE DeviceStatus SET SubType=" << sSwitchTypeAC << " WHERE ([Type]==" << pTypeGeneralSwitch << ") AND (SubType==" << sTypeAC << ") AND (HardwareID=" << sd[0] << ")";
					query(szQuery2.str());
				}
			}
		}
		if (dbversion < 113)
		{
			//Patch for new 1-Wire subtypes
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID FROM Hardware WHERE([Type]==" << HTYPE_1WIRE << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					safe_query("UPDATE DeviceStatus SET SubType='%d' WHERE ([Type]==%d) AND (SubType==%d) AND (HardwareID=%s)",
						sTypeTEMP5, pTypeTEMP, sTypeTEMP10, sd[0].c_str());

					safe_query("UPDATE DeviceStatus SET SubType='%d' WHERE ([Type]==%d) AND (SubType==%d) AND (HardwareID=%s)",
						sTypeHUM1, pTypeHUM, sTypeHUM2, sd[0].c_str());
				}
			}
		}
		if (dbversion < 114)
		{
			//Set default values for new parameters in EcoDevices and Teleinfo EDF
			std::stringstream szQuery1, szQuery2;
			szQuery1 << "UPDATE Hardware SET Mode1 = 0, Mode2 = 60 WHERE Type =" << HTYPE_ECODEVICES;
			query(szQuery1.str());
			szQuery2 << "UPDATE Hardware SET Mode1 = 0, Mode2 = 0, Mode3 = 60 WHERE Type =" << HTYPE_TeleinfoMeter;
			query(szQuery2.str());
		}
		if (dbversion < 115)
		{
			//Patch for Evohome Web
			std::stringstream szQuery2;
			std::vector<std::vector<std::string> > result;
			szQuery2 << "SELECT ID, Name, Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware WHERE([Type]==" << HTYPE_EVOHOME_WEB << ")";
			result = query(szQuery2.str());
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					std::string lowerName="fitbit";
					for (uint8_t i = 0; i < 6; i++)
						lowerName[i] = sd[1][i] | 0x20;
					if (lowerName == "fitbit")
						safe_query("DELETE FROM Hardware WHERE ID=%s", sd[0].c_str());
					else
					{
						int newParams = (sd[3]=="0") ? 0 : 1;
						if (sd[4]=="1")
							newParams += 2;
						if (sd[5]=="1")
							newParams += 4;
						m_sql.safe_query("UPDATE Hardware SET Mode2=%d, Mode3=%s, Mode4=0, Mode5=0 WHERE ID=%s", newParams, sd[6].c_str(), sd[0].c_str());
						m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='' WHERE HardwareID=%s", sd[0].c_str());
					}
				}
			}
		}
		if (dbversion < 116)
		{
			//Patch for GCM/FCM
			safe_query("UPDATE MobileDevices SET Active=1");
			if (!DoesColumnExistsInTable("DeviceType", "MobileDevices"))
			{
				query("ALTER TABLE MobileDevices ADD COLUMN [DeviceType] VARCHAR(100) DEFAULT ('')");
			}
		}
		if (dbversion < 117)
		{
			//Add Protocol for Camera (HTTP/HTTPS/...)
			if (!DoesColumnExistsInTable("Protocol", "Cameras"))
			{
				query("ALTER TABLE Cameras ADD COLUMN [Protocol] INTEGER DEFAULT 0");
			}
		}
		if (dbversion < 118)
		{
			//Delete script that could crash domoticz (maybe the dev can have a look?)
			std::string sfile = szUserDataFolder + "scripts/python/script_device_PIRsmarter.py";
			std::remove(sfile.c_str());
		}
		if (dbversion < 119)
		{
			//change Disable Event System to Enable Event System
			int nValue = 0;
			if (GetPreferencesVar("DisableEventScriptSystem", nValue))
			{
				UpdatePreferencesVar("EnableEventScriptSystem", !nValue);
			}
			DeletePreferencesVar("DisableEventScriptSystem");
		}
		if (dbversion < 120)
		{
			// remove old dzVents dirs
			std::string dzv_Dir, dzv_scripts;
#ifdef WIN32
			dzv_Dir = szUserDataFolder + "scripts\\dzVents\\generated_scripts\\";
			dzv_scripts = szUserDataFolder + "scripts\\dzVents\\";
#else
			dzv_Dir = szUserDataFolder + "scripts/dzVents/generated_scripts/";
			dzv_scripts = szUserDataFolder + "scripts/dzVents/";
#endif
			const std::string
			dzv_rm_Dir1 = dzv_scripts + "runtime",
			dzv_rm_Dir2 = dzv_scripts + "documentation";

			if ((file_exist(dzv_rm_Dir1.c_str()) || file_exist(dzv_rm_Dir2.c_str())) &&
				!szUserDataFolder.empty())
			{
				std::string errorPath;
				if (int returncode = RemoveDir(dzv_rm_Dir1 + "|" + dzv_rm_Dir2, errorPath))
					_log.Log(LOG_ERROR, "EventSystem: (%d) Could not remove %s, please remove manually!", returncode, errorPath.c_str());
			}
		}
	}
	else if (bNewInstall)
	{
		//place here actions that needs to be performed on new databases
		query("INSERT INTO Plans (Name) VALUES ('$Hidden Devices')");
		// Add hardware for internal use
		m_sql.safe_query("INSERT INTO Hardware (Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6) VALUES ('Domoticz Internal',1, %d,'',1,'','',0,0,0,0,0,0)", HTYPE_DomoticzInternal);
	}
	UpdatePreferencesVar("DB_Version",DB_VERSION);

	//Make sure we have some default preferences
	int nValue=10;
	std::string sValue;
	if (!GetPreferencesVar("Title", sValue))
        {
                UpdatePreferencesVar("Title", "Domoticz");
        }
        if ((!GetPreferencesVar("LightHistoryDays", nValue)) || (nValue==0))
	{
		UpdatePreferencesVar("LightHistoryDays", 30);
	}
	if ((!GetPreferencesVar("MeterDividerEnergy", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("MeterDividerEnergy", 1000);
	}
	else if (nValue == 0)
	{
		//Sanity check!
		UpdatePreferencesVar("MeterDividerEnergy", 1000);
	}
	if ((!GetPreferencesVar("MeterDividerGas", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("MeterDividerGas", 100);
	}
	else if (nValue == 0)
	{
		//Sanity check!
		UpdatePreferencesVar("MeterDividerGas", 100);
	}
	if ((!GetPreferencesVar("MeterDividerWater", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("MeterDividerWater", 100);
	}
	else if (nValue == 0)
	{
		//Sanity check!
		UpdatePreferencesVar("MeterDividerWater", 100);
	}
	if ((!GetPreferencesVar("RandomTimerFrame", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("RandomTimerFrame", 15);
	}
	if ((!GetPreferencesVar("ElectricVoltage", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("ElectricVoltage", 230);
	}
	if (!GetPreferencesVar("CM113DisplayType", nValue))
	{
		UpdatePreferencesVar("CM113DisplayType", 0);
	}
	if ((!GetPreferencesVar("5MinuteHistoryDays", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("5MinuteHistoryDays", 1);
	}
	if ((!GetPreferencesVar("SensorTimeout", nValue)) || (nValue == 0))
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
	if ((!GetPreferencesVar("CostEnergy", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("CostEnergy", 2149);
	}
	if ((!GetPreferencesVar("CostEnergyT2", nValue)) || (nValue == 0))
	{
		GetPreferencesVar("CostEnergy", nValue);
		UpdatePreferencesVar("CostEnergyT2", nValue);
	}
	if ((!GetPreferencesVar("CostEnergyR1", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("CostEnergyR1", 800);
	}
	if ((!GetPreferencesVar("CostEnergyR2", nValue)) || (nValue == 0))
	{
		GetPreferencesVar("CostEnergyR1", nValue);
		UpdatePreferencesVar("CostEnergyR2", nValue);
	}

	if ((!GetPreferencesVar("CostGas", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("CostGas", 6218);
	}
	if ((!GetPreferencesVar("CostWater", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("CostWater", 16473);
	}
	if (!GetPreferencesVar("UseEmailInNotifications", nValue))
	{
		UpdatePreferencesVar("UseEmailInNotifications", 1);
	}
	if (!GetPreferencesVar("SendErrorNotifications", nValue))
	{
		UpdatePreferencesVar("SendErrorNotifications", 0);
	}
	if ((!GetPreferencesVar("EmailPort", nValue)) || (nValue == 0))
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
	if ((!GetPreferencesVar("RemoteSharedPort", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("RemoteSharedPort", 6144);
	}
	if ((!GetPreferencesVar("Language", sValue)) || (sValue.empty()))
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
	if (!GetPreferencesVar("WeightUnit", nValue))
	{
		UpdatePreferencesVar("WeightUnit", (int)WEIGHTUNIT_KG);
	}
	else
	{
		m_weightunit=(_eWeightUnit)nValue;

	}
	SetUnitsAndScale();

	if (!GetPreferencesVar("SecStatus", nValue))
	{
		UpdatePreferencesVar("SecStatus", 0);
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
	if ((!GetPreferencesVar("RaspCamParams", sValue)) || (sValue.empty()))
	{
		UpdatePreferencesVar("RaspCamParams", "-w 800 -h 600 -t 1"); //width/height/time2wait
	}
	if ((!GetPreferencesVar("UVCParams", sValue)) || (sValue.empty()))
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
	if ((!GetPreferencesVar("ZWavePollInterval", nValue)) || (nValue == 0))
	{
		UpdatePreferencesVar("ZWavePollInterval", 60);
	}
	if (!GetPreferencesVar("ZWaveEnableDebug", nValue))
	{
		UpdatePreferencesVar("ZWaveEnableDebug", 0);
	}
	if ((!GetPreferencesVar("ZWaveNetworkKey", sValue)) || (sValue.empty()))
	{
		sValue = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
		UpdatePreferencesVar("ZWaveNetworkKey", sValue);
	}
	//Double check network_key
	std::vector<std::string> splitresults;
	StringSplit(sValue, ",", splitresults);
	if (splitresults.size() != 16)
	{
		sValue = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
		UpdatePreferencesVar("ZWaveNetworkKey", sValue);
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
	if (!GetPreferencesVar("EnableEventScriptSystem", nValue))
	{
		UpdatePreferencesVar("EnableEventScriptSystem", 1);
		nValue = 1;
	}
	m_bEnableEventSystem = (nValue == 1);

	nValue = 0;
	if (!GetPreferencesVar("DisableDzVentsSystem", nValue))
	{
		UpdatePreferencesVar("DisableDzVentsSystem", 0);
		nValue = 0;
	}
	m_bDisableDzVentsSystem = (nValue == 1);

	if (!GetPreferencesVar("DzVentsLogLevel", nValue))
	{
		UpdatePreferencesVar("DzVentsLogLevel", 3);
	}

	nValue = 1;
	if (!GetPreferencesVar("LogEventScriptTrigger", nValue))
	{
		UpdatePreferencesVar("LogEventScriptTrigger", 1);
		nValue = 1;
	}
	m_bLogEventScriptTrigger = (nValue != 0);

	if ((!GetPreferencesVar("WebTheme", sValue)) || (sValue.empty()))
	{
		UpdatePreferencesVar("WebTheme", "default");
	}

	if ((!GetPreferencesVar("FloorplanPopupDelay", nValue)) || (nValue == 0))
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
	if ((!GetPreferencesVar("FloorplanRoomColour", sValue)) || (sValue.empty()))
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
	if ((!GetPreferencesVar("TempHome", sValue)) || (sValue.empty()))
	{
		UpdatePreferencesVar("TempHome", "20");
	}
	if ((!GetPreferencesVar("TempAway", sValue)) || (sValue.empty()))
	{
		UpdatePreferencesVar("TempAway", "15");
	}
	if ((!GetPreferencesVar("TempComfort", sValue)) || (sValue.empty()))
	{
		UpdatePreferencesVar("TempComfort", "22.0");
	}
	if ((!GetPreferencesVar("DegreeDaysBaseTemperature", sValue)) || (sValue.empty()))
	{
		UpdatePreferencesVar("DegreeDaysBaseTemperature", "18.0");
	}
	if ((!GetPreferencesVar("HTTPURL", sValue)) || (sValue.empty()))
	{
		sValue = "https://www.somegateway.com/pushurl.php?username=#FIELD1&password=#FIELD2&apikey=#FIELD3&from=#FIELD4&to=#TO&message=#MESSAGE";
		std::string sencoded = base64_encode((const unsigned char*)sValue.c_str(), sValue.size());
		UpdatePreferencesVar("HTTPURL", sencoded);
	}
	if ((!GetPreferencesVar("HTTPPostContentType", sValue)) || (sValue.empty()))
	{
		sValue = "application/json";
		std::string sencoded = base64_encode((const unsigned char*)sValue.c_str(), sValue.size());
		UpdatePreferencesVar("HTTPPostContentType", sencoded);
	}
	if (!GetPreferencesVar("ShowUpdateEffect", nValue))
	{
		UpdatePreferencesVar("ShowUpdateEffect", 0);
	}
	nValue = 5;
	if (!GetPreferencesVar("ShortLogInterval", nValue))
	{
		UpdatePreferencesVar("ShortLogInterval", nValue);
	}
	if (nValue < 1)
		nValue = 5;
	m_ShortLogInterval = nValue;

	if (!GetPreferencesVar("SendErrorsAsNotification", nValue))
	{
		UpdatePreferencesVar("SendErrorsAsNotification", 0);
		nValue = 0;
	}
	_log.ForwardErrorsToNotificationSystem(nValue != 0);

	if (!GetPreferencesVar("IFTTTEnabled", nValue))
	{
		UpdatePreferencesVar("IFTTTEnabled", 0);
	}
	if (!GetPreferencesVar("EmailEnabled", nValue))
	{
		UpdatePreferencesVar("EmailEnabled", 1);
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
	uint64_t ID;
	std::stringstream s_str(idx);
	s_str >> ID;

	return SwitchLightFromTasker(ID, switchcmd, atoi(level.c_str()), atoi(hue.c_str()));
}

bool CSQLHelper::SwitchLightFromTasker(uint64_t idx, const std::string &switchcmd, int level, int hue)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,AddjValue2,nValue,sValue,Name,Options FROM DeviceStatus WHERE (ID == %" PRIu64 ")", idx);
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
		sleep_milliseconds(static_cast<const long>(1000.0f / timer_resolution_hz));

		if (m_bAcceptHardwareTimerActive)
		{
			m_iAcceptHardwareTimerCounter -= static_cast<float>(1. / timer_resolution_hz);
			if (m_iAcceptHardwareTimerCounter <= (1.0f / timer_resolution_hz / 2))
			{
				m_bAcceptHardwareTimerActive = false;
				m_bAcceptNewHardware = m_bPreviousAcceptNewHardware;
				UpdatePreferencesVar("AcceptNewHardware", (m_bAcceptNewHardware == true) ? 1 : 0);
				if (!m_bAcceptNewHardware)
				{
					_log.Log(LOG_STATUS, "Receiving of new sensors disabled!...");
				}
			}
		}

		{ // additional scope for lock (accessing size should be within lock too)
			boost::lock_guard<boost::mutex> l(m_background_task_mutex);
			if (m_background_task_queue.size() > 0)
			{
				_items2do.clear();

				std::vector<_tTaskItem>::iterator itt = m_background_task_queue.begin();
				while (itt != m_background_task_queue.end())
				{
					if (itt->_DelayTime)
					{
						struct timeval tvDiff, DelayTimeEnd;
						getclock(&DelayTimeEnd);
						if (timeval_subtract(&tvDiff, &DelayTimeEnd, &itt->_DelayTimeBegin)) {
							tvDiff.tv_sec = 0;
							tvDiff.tv_usec = 0;
						}
						float diff = ((tvDiff.tv_usec / 1000000.0f) + tvDiff.tv_sec);
						if ((itt->_DelayTime) <= diff)
						{
							_items2do.push_back(*itt);
							itt = m_background_task_queue.erase(itt);
						}
						else
							++itt;
					}
					else
					{
						_items2do.push_back(*itt);
						itt = m_background_task_queue.erase(itt);
					}
				}
			}
		}

		if (_items2do.size() < 1) {
			continue;
		}

		std::vector<_tTaskItem>::iterator itt = _items2do.begin();
		while (itt != _items2do.end())
		{
			if (_log.isTraceEnabled())
				_log.Log(LOG_TRACE, "SQLH: Do Task ItemType:%d Cmd:%s Value:%s ", itt->_ItemType, itt->_command.c_str(), itt->_sValue.c_str());

			if (itt->_ItemType == TITEM_SWITCHCMD)
			{
				if (itt->_switchtype == STYPE_Motion)
				{
					std::string devname = "";
					switch (itt->_devType)
					{
					case pTypeLighting1:
					case pTypeLighting2:
					case pTypeLighting3:
					case pTypeLighting5:
					case pTypeLighting6:
					case pTypeLimitlessLights:
					case pTypeGeneralSwitch:
					case pTypeHomeConfort:
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
							UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(), devname, true);
							break;
						}
						break;
					case pTypeLighting4:
						//only update internally
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(), devname, true);
						break;
					default:
						//unknown hardware type, sensor will only be updated internally
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(), devname, true);
						break;
					}
				}
				else
				{
					if (itt->_devType == pTypeLighting4)
					{
						//only update internally
						std::string devname = "";
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(), devname, true);
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
				ShellExecute(NULL, "open", itt->_ID.c_str(), itt->_sValue.c_str(), NULL, SW_SHOWNORMAL);
#else
				std::string lscript = itt->_ID + " " + itt->_sValue;
				int ret = system(lscript.c_str());
				if (ret != 0)
				{
					_log.Log(LOG_ERROR, "Error executing script command (%s). returned: %d", itt->_ID.c_str(), ret);
				}
#endif
			}
			else if (itt->_ItemType == TITEM_EMAIL_CAMERA_SNAPSHOT)
			{
				m_mainworker.m_cameras.EmailCameraSnapshot(itt->_ID, itt->_sValue);
			}
			else if (itt->_ItemType == TITEM_GETURL)
			{
				std::string sResult;
				bool ret = HTTPClient::GET(itt->_sValue, sResult);
				if (!ret)
				{
					_log.Log(LOG_ERROR, "Error opening url: %s", itt->_sValue.c_str());
				}
			}
			else if ((itt->_ItemType == TITEM_SEND_EMAIL) || (itt->_ItemType == TITEM_SEND_EMAIL_TO))
			{
				int nValue;
				if (GetPreferencesVar("EmailEnabled", nValue))
				{
					if (nValue)
					{
						std::string sValue;
						if (GetPreferencesVar("EmailServer", sValue))
						{
							if (sValue != "")
							{
								std::string EmailFrom;
								std::string EmailTo;
								std::string EmailServer = sValue;
								int EmailPort = 25;
								std::string EmailUsername;
								std::string EmailPassword;
								GetPreferencesVar("EmailFrom", EmailFrom);
								if (itt->_ItemType != TITEM_SEND_EMAIL_TO)
								{
									GetPreferencesVar("EmailTo", EmailTo);
								}
								else
								{
									EmailTo = itt->_command;
								}
								GetPreferencesVar("EmailUsername", EmailUsername);
								GetPreferencesVar("EmailPassword", EmailPassword);

								GetPreferencesVar("EmailPort", EmailPort);

								SMTPClient sclient;
								sclient.SetFrom(CURLEncode::URLDecode(EmailFrom.c_str()));
								sclient.SetTo(CURLEncode::URLDecode(EmailTo.c_str()));
								sclient.SetCredentials(base64_decode(EmailUsername), base64_decode(EmailPassword));
								sclient.SetServer(CURLEncode::URLDecode(EmailServer.c_str()), EmailPort);
								sclient.SetSubject(CURLEncode::URLDecode(itt->_ID));
								sclient.SetHTMLBody(itt->_sValue);
								bool bRet = sclient.SendEmail();

								if (bRet)
									_log.Log(LOG_STATUS, "Notification sent (Email)");
								else
									_log.Log(LOG_ERROR, "Notification failed (Email)");
							}
						}
					}
				}
			}
			else if (itt->_ItemType == TITEM_SEND_SMS)
			{
				m_notifications.SendMessage(0, std::string(""), "clickatell", itt->_ID, itt->_ID, std::string(""), 1, std::string(""), false);
			}
			else if (itt->_ItemType == TITEM_SWITCHCMD_EVENT)
			{
				SwitchLightFromTasker(itt->_idx, itt->_command.c_str(), itt->_level, itt->_Hue);
			}

			else if (itt->_ItemType == TITEM_SWITCHCMD_SCENE)
			{
				m_mainworker.SwitchScene(itt->_idx, itt->_command.c_str());
			}
			else if (itt->_ItemType == TITEM_SET_VARIABLE)
			{
				std::vector<std::vector<std::string> > result;
				std::stringstream s_str;
				result = safe_query("SELECT Name, ValueType FROM UserVariables WHERE (ID == %" PRIu64 ")", itt->_idx);
				if (result.size() > 0)
				{
					std::vector<std::string> sd = result[0];
					s_str.clear();
					s_str.str("");
					s_str << itt->_idx;
					std::string updateResult = UpdateUserVariable(s_str.str(), sd[0], sd[1], itt->_sValue, (itt->_nValue == 0) ? false : true);
					if (updateResult != "OK") {
						_log.Log(LOG_ERROR, "Error updating variable %s: %s", sd[0].c_str(), updateResult.c_str());
					}
					else
					{
						_log.Log(LOG_STATUS, "Set UserVariable %s = %s", sd[0].c_str(), CURLEncode::URLDecode(itt->_sValue).c_str());
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "Variable not found!");
				}
			}
			else if (itt->_ItemType == TITEM_SET_SETPOINT)
			{
				std::stringstream sstr;
				sstr << itt->_idx;
				std::string idx = sstr.str();
				float fValue = (float)atof(itt->_sValue.c_str());
				m_mainworker.SetSetPoint(idx, fValue);
			}
			else if (itt->_ItemType == TITEM_SEND_NOTIFICATION)
			{
				std::vector<std::string> splitresults;
				StringSplit(itt->_command, "!#", splitresults);
				if (splitresults.size() == 5) {
					std::string subsystem = splitresults[4];
					if (subsystem.empty() || subsystem == " ") {
						subsystem = NOTIFYALL;
					}
					m_notifications.SendMessageEx(0, std::string(""), subsystem, splitresults[0], splitresults[1], splitresults[2], static_cast<int>(itt->_idx), splitresults[3], true);
				}
			}
			else if (itt->_ItemType == TITEM_SEND_IFTTT_TRIGGER)
			{
				std::vector<std::string> splitresults;
				StringSplit(itt->_command, "!#", splitresults);
				if (!splitresults.empty())
				{
					std::string sValue1, sValue2, sValue3;
					if (splitresults.size() > 0)
						sValue1 = splitresults[0];
					if (splitresults.size() > 1)
						sValue2 = splitresults[1];
					if (splitresults.size() > 2)
						sValue3 = splitresults[2];
					IFTTT::Send_IFTTT_Trigger(itt->_ID, sValue1, sValue2, sValue3);
				}
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

bool CSQLHelper::DoesColumnExistsInTable(const std::string &columnname, const std::string &tablename)
{
	if (!m_dbase)
	{
		_log.Log(LOG_ERROR, "Database not open!!...Check your user rights!..");
		return false;
	}
	bool columnExists = false;

	sqlite3_stmt *statement;
	std::string szQuery = "SELECT " + columnname + " FROM " + tablename;
	if (sqlite3_prepare_v2(m_dbase, szQuery.c_str(), -1, &statement, NULL) == SQLITE_OK)
	{
		columnExists = true;
		sqlite3_finalize(statement);
	}
	return columnExists;
}

void CSQLHelper::safe_exec_no_return(const char *fmt, ...)
{
	if (!m_dbase)
		return;

	va_list args;
	va_start(args, fmt);
	char *zQuery = sqlite3_vmprintf(fmt, args);
	va_end(args);
	if (!zQuery)
		return;
	sqlite3_exec(m_dbase, zQuery, NULL, NULL, NULL);
	sqlite3_free(zQuery);
}

std::vector<std::vector<std::string> > CSQLHelper::safe_query(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char *zQuery = sqlite3_vmprintf(fmt, args);
	va_end(args);
	if (!zQuery)
	{
		_log.Log(LOG_ERROR, "SQL: Out of memory, or invalid printf!....");
		std::vector<std::vector<std::string> > results;
		return results;
	}
	std::vector<std::vector<std::string> > results = query(zQuery);
	sqlite3_free(zQuery);
	return results;
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
		while(true)
		{
			int result = sqlite3_step(statement);
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

	if (_log.isTraceEnabled()) {
		_log.Log(LOG_TRACE, "SQLQ query : %s", szQuery.c_str());
		if (!_log.TestFilter("SQLR"))
			LogQueryResult(results);
	}

	std::string error = sqlite3_errmsg(m_dbase);
	if(error != "not an error")
		_log.Log(LOG_ERROR, "SQL Query(\"%s\") : %s", szQuery.c_str(), error.c_str());
	return results;
}

std::vector<std::vector<std::string> > CSQLHelper::safe_queryBlob(const char *fmt, ...)
{
	va_list args;
	std::vector<std::vector<std::string> > results;
	va_start(args, fmt);
	char *zQuery = sqlite3_vmprintf(fmt, args);
	va_end(args);
	if (!zQuery)
	{
		_log.Log(LOG_ERROR, "SQL: Out of memory, or invalid printf!....");
		std::vector<std::vector<std::string> > results;
		return results;
	}
	results = queryBlob(zQuery);
	sqlite3_free(zQuery);
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
		while (true)
		{
			int result = sqlite3_step(statement);
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
		_log.Log(LOG_ERROR, "SQL Query(\"%s\") : %s", szQuery.c_str(), error.c_str());
	return results;
}

uint64_t CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, std::string &devname, const bool bUseOnOffAction)
{
	return UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, "", devname, bUseOnOffAction);
}

uint64_t CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue, std::string &devname, const bool bUseOnOffAction)
{
	return UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, 0, sValue, devname, bUseOnOffAction);
}

uint64_t CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction)
{
	uint64_t devRowID=UpdateValueInt(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue,devname,bUseOnOffAction);
	if (devRowID == -1)
		return -1;

	if (!IsLightOrSwitch(devType, subType))
	{
		return devRowID;
	}

	//Get the ID of this device
	std::vector<std::vector<std::string> > result,result2;
	std::vector<std::vector<std::string> >::const_iterator itt,itt2;

	result=safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	if (result.size()==0)
		return devRowID; //should never happen, because it was previously inserted if non-existent

	std::string idx=result[0][0];

	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now,&ltime);

	//Check if this switch was a Sub/Slave device for other devices, if so adjust the state of those other devices
	result = safe_query("SELECT A.ParentID, B.Name, B.HardwareID, B.[Type], B.[SubType], B.Unit FROM LightSubDevices as A, DeviceStatus as B WHERE (A.DeviceRowID=='%q') AND (A.DeviceRowID!=A.ParentID) AND (B.[ID] == A.ParentID)", idx.c_str());
	if (result.size()>0)
	{
		//This is a sub/slave device for another main device
		//Set the Main Device state to the same state as this device
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			safe_query(
				"UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == '%q')",
				nValue,
				sValue,
				ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
				sd[0].c_str()
				);

			//Call the EventSystem for the main switch
			uint64_t ParentID = (uint64_t)atoll(sd[0].c_str());
			std::string ParentName = sd[1];
			int ParentHardwareID = atoi(sd[2].c_str());
			unsigned char ParentType = (unsigned char)atoi(sd[3].c_str());
			unsigned char ParentSubType = (unsigned char)atoi(sd[4].c_str());
			unsigned char ParentUnit = (unsigned char)atoi(sd[5].c_str());
			m_mainworker.m_eventsystem.ProcessDevice(ParentHardwareID, ParentID, ParentUnit, ParentType, ParentSubType, signallevel, batterylevel, nValue, sValue, ParentName, 0);

			//Set the status of all slave devices from this device (except the one we just received) to off
			//Check if this switch was a Sub/Slave device for other devices, if so adjust the state of those other devices
			result2 = safe_query(
				"SELECT a.DeviceRowID, b.Type FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='%q') AND (a.DeviceRowID!='%q') AND (b.ID == a.DeviceRowID) AND (a.DeviceRowID!=a.ParentID)",
				sd[0].c_str(),
				idx.c_str()
				);
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
					case pTypeThermostat4:
						newnValue = thermostat4_sOff;
						break;
					case pTypeRadiator1:
						newnValue = Radiator1_sNight;
						break;
					case pTypeGeneralSwitch:
						newnValue = gswitch_sOff;
						break;
					case pTypeHomeConfort:
						newnValue = HomeConfort_sOff;
						break;
					default:
						continue;
					}
					safe_query(
						"UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == '%q')",
						newnValue,
						"",
						ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
						sd[0].c_str()
						);
				}
			}
		}
	}

	//If this is a 'Main' device, and it has Sub/Slave devices,
	//set the status of the Sub/Slave devices to Off, as we might be out of sync then
	result = safe_query(
		"SELECT a.DeviceRowID, b.Type FROM LightSubDevices a, DeviceStatus b WHERE (a.ParentID=='%q') AND (b.ID == a.DeviceRowID) AND (a.DeviceRowID!=a.ParentID)",
		idx.c_str()
		);
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
			case pTypeThermostat4:
				newnValue = thermostat4_sOff;
				break;
			case pTypeRadiator1:
				newnValue = Radiator1_sNight;
				break;
			case pTypeGeneralSwitch:
				newnValue = gswitch_sOff;
				break;
			case pTypeHomeConfort:
				newnValue = HomeConfort_sOff;
				break;
			default:
				continue;
			}
			safe_query(
				"UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == '%q')",
				newnValue,
				"",
				ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
				sd[0].c_str()
				);
		}
	}
	return devRowID;
}

uint64_t CSQLHelper::UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname, const bool bUseOnOffAction)
{
	if (!m_dbase)
		return -1;

	uint64_t ulID=0;
	bool bDeviceUsed = false;
	bool bSameDeviceStatusValue = false;
	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT ID,Name, Used, SwitchType, nValue, sValue, LastUpdate, Options FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	if (result.size()==0)
	{
		//Insert

		if (!m_bAcceptNewHardware)
		{
			devname = "Ignored";
			return -1; //We do not allow new devices
		}

		devname="Unknown";
		safe_query(
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue) "
			"VALUES ('%d','%q','%d','%d','%d','%d','%d','%d','%q')",
			HardwareID,
			ID,unit,devType,subType,
			signallevel,batterylevel,
			nValue,sValue);

		//Get new ID
		result = safe_query(
			"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			HardwareID, ID, unit, devType, subType);
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
		std::string sOption=result[0][7];
		devname=result[0][1];
		bDeviceUsed= atoi(result[0][2].c_str())!=0;
		_eSwitchType stype = (_eSwitchType)atoi(result[0][3].c_str());
		int old_nValue = atoi(result[0][4].c_str());
		std::string old_sValue = result[0][5];
		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now,&ltime);
		//Commit: If Option 1: energy is computed as usage*time
		//Default is option 0, read from device
		if (sOption == "1" && devType == pTypeGeneral && subType == sTypeKwh)
		{
			std::vector<std::string> parts;
			struct tm ntime;
			double interval;
			float nEnergy;
			char sCompValue[100];
			std::string sLastUpdate = result[0][6];
			time_t lutime;
			ParseSQLdatetime(lutime, ntime, sLastUpdate, ltime.tm_isdst);

			interval = difftime(now,lutime);
			StringSplit(result[0][5].c_str(), ";", parts);
			nEnergy = static_cast<float>(strtof(parts[0].c_str(), NULL)*interval / 3600 + strtof(parts[1].c_str(), NULL)); //Rob: whats happening here... strtof ?
			StringSplit(sValue, ";", parts);
			sprintf(sCompValue, "%s;%.1f", parts[0].c_str(), nEnergy);
			sValue = sCompValue;
		}
	        //~ use different update queries based on the device type
	        if (devType == pTypeGeneral && subType == sTypeCounterIncremental)
        	{
			result = safe_query(
				"UPDATE DeviceStatus SET SignalLevel=%d, BatteryLevel=%d, nValue= nValue + %d, sValue= sValue + '%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' "
				"WHERE (ID = %" PRIu64 ")",
				signallevel,batterylevel,
				nValue,sValue,
				ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
				ulID);
	        }
		else
		{
			if (
				(stype == STYPE_DoorContact) ||
				(stype == STYPE_DoorLock) ||
				(stype == STYPE_Contact)
				)
			{
				//Check if we received the same state as before, if yes, don't do anything (only update)
				//This is specially handy for devices that send a keep-alive status every xx minutes
				//like professional alarm system equipment
				//.. we should make this an option of course
				bSameDeviceStatusValue = (
					(nValue == old_nValue) &&
					(sValue == old_sValue)
					);
			}

			result = safe_query(
				"UPDATE DeviceStatus SET SignalLevel=%d, BatteryLevel=%d, nValue=%d, sValue='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' "
				"WHERE (ID = %" PRIu64 ")",
				signallevel, batterylevel,
				nValue, sValue,
				ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
				ulID);
		}
	}

	if (bSameDeviceStatusValue)
		return ulID; //status has not changed, no need to process further

	switch (devType)
	{
	case pTypeRego6XXValue:
        if(subType != sTypeRego6XXStatus)
        {
            break;
        }
	case pTypeGeneral:
		if ((devType == pTypeGeneral) && (subType != sTypeTextStatus) && (subType != sTypeAlert))
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
	case pTypeFan:
	case pTypeRFY:
	case pTypeChime:
	case pTypeThermostat2:
	case pTypeThermostat3:
	case pTypeThermostat4:
	case pTypeRemote:
	case pTypeGeneralSwitch:
	case pTypeHomeConfort:
	case pTypeRadiator1:
		if ((devType == pTypeRadiator1) && (subType != sTypeSmartwaresSwitchRadiator))
			break;
		//Add Lighting log
		m_LastSwitchID=ID;
		m_LastSwitchRowID=ulID;
		result = safe_query(
			"INSERT INTO LightingLog (DeviceRowID, nValue, sValue) "
			"VALUES ('%" PRIu64 "', '%d', '%q')",
			ulID,
			nValue,sValue);

		if (!bDeviceUsed)
			return ulID;	//don't process further as the device is not used
		std::string lstatus="";

		result = safe_query(
			"SELECT Name,SwitchType,AddjValue,StrParam1,StrParam2,Options,LastLevel FROM DeviceStatus WHERE (ID = %" PRIu64 ")",
			ulID);
		if (result.size()>0)
		{
			bool bHaveGroupCmd = false;
			int maxDimLevel = 0;
			int llevel = 0;
			bool bHaveDimmer = false;
			std::vector<std::string> sd=result[0];
			std::string Name=sd[0];
			_eSwitchType switchtype=(_eSwitchType)atoi(sd[1].c_str());
			float AddjValue = static_cast<float>(atof(sd[2].c_str()));
			GetLightStatus(devType, subType, switchtype,nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

			bool bIsLightSwitchOn=IsLightSwitchOn(lstatus);
			std::string slevel = sd[6];

			if ((bIsLightSwitchOn) && (llevel != 0) && (llevel != 255) ||
				(switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted))
			{
				if (((switchtype == STYPE_BlindsPercentage) ||
					(switchtype == STYPE_BlindsPercentageInverted)) &&
					(nValue == light2_sOn))
				{
						llevel = 100;
				}
				//update level for device
				safe_query(
					"UPDATE DeviceStatus SET LastLevel='%d' WHERE (ID = %" PRIu64 ")",
					llevel,
					ulID);
				if (bUseOnOffAction)
				    slevel = boost::lexical_cast<std::string>(llevel);
			}

			if (bUseOnOffAction)
			{
				//Perform any On/Off actions
				std::string OnAction=sd[3];
				std::string OffAction=sd[4];
				std::string Options=sd[5];

				if(devType==pTypeEvohome)
				{
					bIsLightSwitchOn=true;//Force use of OnAction for all actions

				} else if (switchtype == STYPE_Selector) {
					bIsLightSwitchOn = (llevel > 0) ? true : false;
					OnAction = CURLEncode::URLDecode(GetSelectorSwitchLevelAction(BuildDeviceOptions(Options, true), llevel));
					OffAction = CURLEncode::URLDecode(GetSelectorSwitchLevelAction(BuildDeviceOptions(Options, true), 0));
				}
				if (bIsLightSwitchOn) {
					stdreplace(OnAction, "{level}", slevel);
					stdreplace(OnAction, "{status}", lstatus);
					stdreplace(OnAction, "{deviceid}", ID);
				} else {
					stdreplace(OffAction, "{level}", slevel);
					stdreplace(OffAction, "{status}", lstatus);
					stdreplace(OffAction, "{deviceid}", ID);
				}
				HandleOnOffAction(bIsLightSwitchOn, OnAction, OffAction);
			}

			//Check if we need to email a snapshot of a Camera
			std::string emailserver;
			int n2Value;
			if (GetPreferencesVar("EmailServer",n2Value,emailserver))
			{
				if (emailserver!="")
				{
					result=safe_query(
						"SELECT CameraRowID, DevSceneDelay FROM CamerasActiveDevices WHERE (DevSceneType==0) AND (DevSceneRowID==%" PRIu64 ") AND (DevSceneWhen==%d)",
						ulID,
						(bIsLightSwitchOn==true)?0:1
						);
					if (result.size()>0)
					{
						std::vector<std::vector<std::string> >::const_iterator ittCam;
						for (ittCam=result.begin(); ittCam!=result.end(); ++ittCam)
						{
							std::vector<std::string> sd=*ittCam;
							std::string camidx=sd[0];
							float delay= static_cast<float>(atof(sd[1].c_str()));
							std::string subject=Name + " Status: " + lstatus;
							AddTaskItem(_tTaskItem::EmailCameraSnapshot(delay+1,camidx,subject));
						}
					}
				}
			}

			if (m_bEnableEventSystem)
			{
				//Execute possible script
				std::string scriptname;
#ifdef WIN32
				scriptname = szUserDataFolder + "scripts\\domoticz_main.bat";
#else
				scriptname = szUserDataFolder + "scripts/domoticz_main";
#endif
				if (file_exist(scriptname.c_str()))
				{
					//Add parameters
					std::stringstream s_scriptparams;
					std::string nszUserDataFolder = szUserDataFolder;
					if (nszUserDataFolder == "")
						nszUserDataFolder = ".";
					s_scriptparams << nszUserDataFolder << " " << HardwareID << " " << ulID << " " << (bIsLightSwitchOn ? "On" : "Off") << " \"" << lstatus << "\"" << " \"" << devname << "\"";
					//add script to background worker
					boost::lock_guard<boost::mutex> l(m_background_task_mutex);
					m_background_task_queue.push_back(_tTaskItem::ExecuteScript(1, scriptname, s_scriptparams.str()));
				}
			}

			_eHardwareTypes HWtype= HTYPE_Domoticz; //just a value
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(HardwareID);
			if (pHardware != NULL)
				HWtype = pHardware->HwdType;

			//Check for notifications
			if (HWtype != HTYPE_LogitechMediaServer) // Skip notifications for LMS here; is handled by the LMS plug-in
			{
				if (switchtype == STYPE_Selector)
					m_notifications.CheckAndHandleSwitchNotification(ulID, devname, (bIsLightSwitchOn) ? NTYPE_SWITCH_ON : NTYPE_SWITCH_OFF, llevel);
				else
					m_notifications.CheckAndHandleSwitchNotification(ulID, devname, (bIsLightSwitchOn) ? NTYPE_SWITCH_ON : NTYPE_SWITCH_OFF);
			}
			if (bIsLightSwitchOn)
			{
				if ((int)AddjValue!=0) //Off Delay
				{
					bool bAdd2DelayQueue=false;
					int cmd=0;
					if (
						(switchtype == STYPE_OnOff) ||
						(switchtype == STYPE_Motion) ||
						(switchtype == STYPE_Dimmer) ||
						(switchtype == STYPE_PushOn) ||
						(switchtype == STYPE_DoorContact) ||
						(switchtype == STYPE_DoorLock) ||
						(switchtype == STYPE_Selector)
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
						case pTypeHomeConfort:
							cmd = HomeConfort_sOff;
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

	if (_log.isTraceEnabled()) _log.Log(LOG_TRACE,"SQLH UpdateValueInt %s HwID:%d  DevID:%s Type:%d  sType:%d nValue:%d sValue:%s ", devname.c_str(),HardwareID, ID, devType, subType, nValue, sValue );

	if (bDeviceUsed)
		m_mainworker.m_eventsystem.ProcessDevice(HardwareID, ulID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, 0);
	return ulID;
}

bool CSQLHelper::GetLastValue(const int HardwareID, const char* DeviceID, const unsigned char unit, const unsigned char devType, const unsigned char subType, int &nValue, std::string &sValue, struct tm &LastUpdateTime)
{
	bool result=false;
	std::vector<std::vector<std::string> > sqlresult;
	std::string sLastUpdate;
	//std::string sValue;
	//struct tm LastUpdateTime;
	sqlresult=safe_query(
		"SELECT nValue,sValue,LastUpdate FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d) order by LastUpdate desc limit 1",
		HardwareID,DeviceID,unit,devType,subType);

	if (sqlresult.size()!=0)
	{
		nValue=(int)atoi(sqlresult[0][0].c_str());
		sValue=sqlresult[0][1];
		sLastUpdate=sqlresult[0][2];

		time_t lutime;
		ParseSQLdatetime(lutime, LastUpdateTime, sLastUpdate);

		result=true;
	}

	return result;
}


void CSQLHelper::GetAddjustment(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti)
{
	AddjValue=0.0f;
	AddjMulti=1.0f;
	std::vector<std::vector<std::string> > result;
	result = safe_query(
		"SELECT AddjValue,AddjMulti FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
		HardwareID, ID, unit, devType, subType);
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
	result=safe_query(
		"SELECT SwitchType FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
		HardwareID, ID, unit, devType, subType);
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
	result=safe_query(
		"SELECT AddjValue2,AddjMulti2 FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
		HardwareID, ID, unit, devType, subType);
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
void CSQLHelper::UpdatePreferencesVar(const std::string &Key, const double Value)
{
	std::string sValue = boost::to_string(Value);
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

	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT ROWID FROM Preferences WHERE (Key='%q')",
		Key.c_str());
	if (result.size() == 0)
	{
		//Insert
		result = safe_query("INSERT INTO Preferences (Key, nValue, sValue) VALUES ('%q', %d,'%q')",
			Key.c_str(), nValue, sValue.c_str());
	}
	else
	{
		//Update
		result = safe_query("UPDATE Preferences SET Key='%q', nValue=%d, sValue='%q' WHERE (ROWID = '%q')",
			Key.c_str(), nValue, sValue.c_str(), result[0][0].c_str());
	}
}

bool CSQLHelper::GetPreferencesVar(const std::string &Key, std::string &sValue)
{
	if (!m_dbase)
		return false;


	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT sValue FROM Preferences WHERE (Key='%q')",
		Key.c_str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	sValue = sd[0];
	return true;
}

bool CSQLHelper::GetPreferencesVar(const std::string &Key, double &Value)
{

	std::string sValue;
	int nValue;
	Value = 0;
	bool res = GetPreferencesVar(Key, nValue, sValue);
	if (!res)
		return false;
	Value = atof(sValue.c_str());
	return true;
}
bool CSQLHelper::GetPreferencesVar(const std::string &Key, int &nValue, std::string &sValue)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT nValue, sValue FROM Preferences WHERE (Key='%q')",
		Key.c_str());
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
void CSQLHelper::DeletePreferencesVar(const std::string &Key)
{
  std::string sValue ;
	if (!m_dbase)
		return ;

  //if found, delete
  if ( GetPreferencesVar(Key,sValue)== true)
  {
	  TSqlQueryResult result;
	  result = safe_query("DELETE FROM Preferences WHERE (Key='%q')",Key.c_str());
  }
}



int CSQLHelper::GetLastBackupNo(const char *Key, int &nValue)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT nValue FROM BackupLog WHERE (Key='%q')",Key);
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

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ROWID FROM BackupLog WHERE (Key='%q')",Key);
	if (result.size()==0)
	{
		//Insert
		safe_query(
			"INSERT INTO BackupLog (Key, nValue) "
			"VALUES ('%q','%d')",
			Key,
			nValue);
	}
	else
	{
		//Update
		uint64_t ID = 0;
		std::stringstream s_str( result[0][0] );
		s_str >> ID;

		safe_query(
			"UPDATE BackupLog SET Key='%q', nValue=%d "
			"WHERE (ROWID = %" PRIu64 ")",
			Key,
			nValue,
			ID);
	}
}

void CSQLHelper::UpdateRFXCOMHardwareDetails(const int HardwareID, const int msg1, const int msg2, const int msg3, const int msg4, const int msg5, const int msg6)
{
	safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d, Mode3=%d, Mode4=%d, Mode5=%d, Mode6=%d WHERE (ID == %d)",
		msg1, msg2, msg3, msg4, msg5, msg6, HardwareID);
}

bool CSQLHelper::HasTimers(const uint64_t Idx)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	result=safe_query("SELECT COUNT(*) FROM Timers WHERE (DeviceRowID==%" PRIu64 ") AND (TimerPlan==%d)", Idx, m_ActiveTimerPlan);
	if (result.size() != 0)
	{
		std::vector<std::string> sd = result[0];
		int totaltimers = atoi(sd[0].c_str());
		if (totaltimers > 0)
			return true;
	}
	result = safe_query("SELECT COUNT(*) FROM SetpointTimers WHERE (DeviceRowID==%" PRIu64 ") AND (TimerPlan==%d)", Idx, m_ActiveTimerPlan);
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
	uint64_t idxll;
	s_str >> idxll;
	return HasTimers(idxll);
}

bool CSQLHelper::HasSceneTimers(const uint64_t Idx)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	result=safe_query("SELECT COUNT(*) FROM SceneTimers WHERE (SceneRowID==%" PRIu64 ") AND (TimerPlan==%d)", Idx, m_ActiveTimerPlan);
	if (result.size()==0)
		return false;
	std::vector<std::string> sd=result[0];
	int totaltimers=atoi(sd[0].c_str());
	return (totaltimers>0);
}

bool CSQLHelper::HasSceneTimers(const std::string &Idx)
{
	std::stringstream s_str( Idx );
	uint64_t idxll;
	s_str >> idxll;
	return HasSceneTimers(idxll);
}

void CSQLHelper::ScheduleShortlog()
{
#ifdef _DEBUG
	//return;
#endif
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
		_log.Log(LOG_ERROR, "Domoticz: Error running the shortlog schedule script!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
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
		CleanupLightSceneLog();
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "Domoticz: Error running the daily schedule script!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return;
	}
}

void CSQLHelper::UpdateTemperatureLog()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR (Type=%d AND SubType=%d) OR (Type=%d AND SubType=%d) OR (Type=%d AND SubType=%d))",
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
		pTypeRadiator1,
		pTypeGeneral,sTypeSystemTemp,
		pTypeThermostat,sTypeThermSetpoint,
		pTypeGeneral, sTypeBaro
		);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			if (dType != pTypeRadiator1)
			{
				//do not include sensors that have no reading within an hour (except for devices that do not provide feedback, like the smartware radiator)
				std::string sLastUpdate = sd[5];
				struct tm ntime;
				time_t checktime;
				ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

				if (difftime(now,checktime) >= SensorTimeOut * 60)
					continue;
			}

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
			case pTypeRadiator1:
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
			safe_query(
				"INSERT INTO Temperature (DeviceRowID, Temperature, Chill, Humidity, Barometer, DewPoint, SetPoint) "
				"VALUES ('%" PRIu64 "', '%.2f', '%.2f', '%d', '%d', '%.2f', '%.2f')",
				ID,
				temp,
				chill,
				humidity,
				barometer,
				dewpoint,
				setpoint
				);
		}
	}
}

void CSQLHelper::UpdateRainLog()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d)", pTypeRAIN);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			//unsigned char dType=atoi(sd[1].c_str());
			//unsigned char dSubType=atoi(sd[2].c_str());
			//int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

			if (difftime(now,checktime) >= SensorTimeOut * 60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<2)
				continue; //impossible

			int rate=atoi(splitresults[0].c_str());
			float total = static_cast<float>(atof(splitresults[1].c_str()));

			//insert record
			safe_query(
				"INSERT INTO Rain (DeviceRowID, Total, Rate) "
				"VALUES ('%" PRIu64 "', '%.2f', '%d')",
				ID,
				total,
				rate
				);
		}
	}
}

void CSQLHelper::UpdateWindLog()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ID,DeviceID, Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d)", pTypeWIND);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			unsigned short DeviceID;
			std::stringstream s_str2(sd[1]);
			s_str2 >> DeviceID;

			//unsigned char dType=atoi(sd[2].c_str());
			//unsigned char dSubType=atoi(sd[3].c_str());
			//int nValue=atoi(sd[4].c_str());
			std::string sValue=sd[5];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[6];
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

			if (difftime(now,checktime) >= SensorTimeOut * 60)
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
			safe_query(
				"INSERT INTO Wind (DeviceRowID, Direction, Speed, Gust) "
				"VALUES ('%" PRIu64 "', '%.2f', '%d', '%d')",
				ID,
				direction,
				speed,
				gust
				);
		}
	}
}

void CSQLHelper::UpdateUVLog()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d) OR (Type=%d AND SubType=%d)",
		pTypeUV,
		pTypeGeneral, sTypeUV
	);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			//unsigned char dType=atoi(sd[1].c_str());
			//unsigned char dSubType=atoi(sd[2].c_str());
			//int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

			if (difftime(now,checktime) >= SensorTimeOut * 60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			float level = static_cast<float>(atof(splitresults[0].c_str()));

			//insert record
			safe_query(
				"INSERT INTO UV (DeviceRowID, Level) "
				"VALUES ('%" PRIu64 "', '%g')",
				ID,
				level
				);
		}
	}
}

void CSQLHelper::UpdateMeter()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> > result2;

	result=safe_query(
		"SELECT ID,Name,HardwareID,DeviceID,Unit,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE ("
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
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypePressure
		"(Type=%d AND SubType=%d) OR " //pTypeGeneral,sTypeCounterIncremental
		"(Type=%d AND SubType=%d)"     //pTypeGeneral,sTypeKwh
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
		pTypeGeneral, sTypePressure,
		pTypeGeneral, sTypeCounterIncremental,
		pTypeGeneral, sTypeKwh
		);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			char szTmp[200];
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			std::string devname = sd[1];
			int hardwareID= atoi(sd[2].c_str());
			std::string DeviceID=sd[3];
			unsigned char Unit = atoi(sd[4].c_str());

			unsigned char dType=atoi(sd[5].c_str());
			unsigned char dSubType=atoi(sd[6].c_str());
			int nValue=atoi(sd[7].c_str());
			std::string sValue=sd[8];
			std::string sLastUpdate=sd[9];

			std::string susage="0";

			//do not include sensors that have no reading within an hour
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);


			//Check for timeout, if timeout then dont add value
			if (dType!=pTypeP1Gas)
			{
				if (difftime(now,checktime) >= SensorTimeOut * 60)
					continue;
			}
			else
			{
				//P1 Gas meter transmits results every 1 a 2 hours
				if (difftime(now,checktime) >= 3 * 3600)
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
				m_notifications.CheckAndHandleNotification(ID, devname, dType, dSubType, NTYPE_USAGE, (float)nValue);
			}
			else if ((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))
			{
				sprintf(szTmp,"%d",nValue);
				sValue=szTmp;
			}
			else if ((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
			{
				double fValue = atof(sValue.c_str())*10.0f;
				sprintf(szTmp, "%.0f", fValue);
				sValue = szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel))
			{
				double fValue = atof(sValue.c_str())*10.0f;
				sprintf(szTmp, "%.0f", fValue);
				sValue = szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeKwh))
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size() < 2)
					continue;

				double fValue = atof(splitresults[0].c_str())*10.0f;
				sprintf(szTmp, "%.0f", fValue);
				susage = szTmp;

				fValue = atof(splitresults[1].c_str());
				sprintf(szTmp, "%.0f", fValue);
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
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if (dType==pTypeRFXSensor)
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if ((dType==pTypeGeneral) && (dSubType == sTypeCounterIncremental))
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if ((dType==pTypeGeneral)&&(dSubType==sTypeVoltage))
			{
				double fValue=atof(sValue.c_str())*1000.0f;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
			{
				double fValue = atof(sValue.c_str())*1000.0f;
				sprintf(szTmp, "%.0f", fValue);
				sValue = szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypePressure))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if (dType == pTypeUsage)
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}

			long long MeterValue;
			std::stringstream s_str2( sValue );
			s_str2 >> MeterValue;

			long long MeterUsage;
			std::stringstream s_str3( susage );
			s_str3 >> MeterUsage;

			//insert record
			safe_query(
				"INSERT INTO Meter (DeviceRowID, Value, [Usage]) "
				"VALUES ('%" PRIu64 "', '%lld', '%lld')",
				ID,
				MeterValue,
				MeterUsage
				);
		}
	}
}

void CSQLHelper::UpdateMultiMeter()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d)",
		pTypeP1Power,
		pTypeCURRENT,
		pTypeCURRENTENERGY
		);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			unsigned char dType=atoi(sd[1].c_str());
			unsigned char dSubType=atoi(sd[2].c_str());
			//int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

			if (difftime(now,checktime) >= SensorTimeOut * 60)
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
				value4=(unsigned long long)(atof(splitresults[3].c_str())*1000.0f);
			}
			else
				continue;//don't know you (yet)

			//insert record
			safe_query(
				"INSERT INTO MultiMeter (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6) "
				"VALUES ('%" PRIu64 "', '%llu', '%llu', '%llu', '%llu', '%llu', '%llu')",
				ID,
				value1,
				value2,
				value3,
				value4,
				value5,
				value6
				);
		}
	}
}

void CSQLHelper::UpdatePercentageLog()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d AND SubType=%d) OR (Type=%d AND SubType=%d) OR (Type=%d AND SubType=%d)",
		pTypeGeneral, sTypePercentage,
		pTypeGeneral, sTypeWaterflow,
		pTypeGeneral, sTypeCustom
		);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			//unsigned char dType=atoi(sd[1].c_str());
			//unsigned char dSubType=atoi(sd[2].c_str());
			//int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

			if (difftime(now,checktime) >= SensorTimeOut * 60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			float percentage = static_cast<float>(atof(sValue.c_str()));

			//insert record
			safe_query(
				"INSERT INTO Percentage (DeviceRowID, Percentage) "
				"VALUES ('%" PRIu64 "', '%g')",
				ID,
				percentage
				);
		}
	}
}

void CSQLHelper::UpdateFanLog()
{
	time_t now = mytime(NULL);
	if (now==0)
		return;
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d AND SubType=%d)",
		pTypeGeneral,sTypeFan
		);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			uint64_t ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;

			//unsigned char dType=atoi(sd[1].c_str());
			//unsigned char dSubType=atoi(sd[2].c_str());
			//int nValue=atoi(sd[3].c_str());
			std::string sValue=sd[4];

			//do not include sensors that have no reading within an hour
			std::string sLastUpdate=sd[5];
			struct tm ntime;
			time_t checktime;
			ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

			if (difftime(now,checktime) >= SensorTimeOut * 60)
				continue;

			std::vector<std::string> splitresults;
			StringSplit(sValue, ";", splitresults);
			if (splitresults.size()<1)
				continue; //impossible

			int speed= (int)atoi(sValue.c_str());

			//insert record
			safe_query(
				"INSERT INTO Fan (DeviceRowID, Speed) "
				"VALUES ('%" PRIu64 "', '%d')",
				ID,
				speed
				);
		}
	}
}


void CSQLHelper::AddCalendarTemperature()
{
	//Get All temperature devices in the Temperature Table
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM Temperature ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		result=safe_query("SELECT MIN(Temperature), MAX(Temperature), AVG(Temperature), MIN(Chill), MAX(Chill), AVG(Humidity), AVG(Barometer), MIN(DewPoint), MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) FROM Temperature WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
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
			result=safe_query(
				"INSERT INTO Temperature_Calendar (DeviceRowID, Temp_Min, Temp_Max, Temp_Avg, Chill_Min, Chill_Max, Humidity, Barometer, DewPoint, SetPoint_Min, SetPoint_Max, SetPoint_Avg, Date) "
				"VALUES ('%" PRIu64 "', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%d', '%d', '%.2f', '%.2f', '%.2f', '%.2f', '%q')",
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
		}
	}
}

void CSQLHelper::AddCalendarUpdateRain()
{
	//Get All UV devices
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM Rain ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		//Get Device Information
		result=safe_query("SELECT SubType FROM DeviceStatus WHERE (ID='%" PRIu64 "')",ID);
		if (result.size()<1)
			continue;
		std::vector<std::string> sd=result[0];

		unsigned char subType=atoi(sd[0].c_str());

		if (subType!=sTypeRAINWU)
		{
			result=safe_query("SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
				ID,
				szDateStart,
				szDateEnd
				);
		}
		else
		{
			result=safe_query("SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q') ORDER BY ROWID DESC LIMIT 1",
				ID,
				szDateStart,
				szDateEnd
				);
		}

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
				result=safe_query(
					"INSERT INTO Rain_Calendar (DeviceRowID, Total, Rate, Date) "
					"VALUES ('%" PRIu64 "', '%.2f', '%d', '%q')",
					ID,
					total_real,
					rate,
					szDateStart
					);
			}
		}
	}
}

void CSQLHelper::AddCalendarUpdateMeter()
{
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
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM Meter ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		//Get Device Information
		result=safe_query("SELECT Name, HardwareID, DeviceID, Unit, Type, SubType, SwitchType FROM DeviceStatus WHERE (ID='%" PRIu64 "')",ID);
		if (result.size()<1)
			continue;
		std::vector<std::string> sd=result[0];
		std::string devname = sd[0];
		//int hardwareID= atoi(sd[1].c_str());
		//std::string DeviceID=sd[2];
		//unsigned char Unit = atoi(sd[3].c_str());
		unsigned char devType=atoi(sd[4].c_str());
		unsigned char subType=atoi(sd[5].c_str());
		_eSwitchType switchtype=(_eSwitchType) atoi(sd[6].c_str());
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


		result=safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			double total_min=(double)atof(sd[0].c_str());
			double total_max = (double)atof(sd[1].c_str());
			double avg_value = (double)atof(sd[2].c_str());

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
				result=safe_query(
					"INSERT INTO Meter_Calendar (DeviceRowID, Value, Counter, Date) "
					"VALUES ('%" PRIu64 "', '%.2f', '%.2f', '%q')",
					ID,
					total_real,
					counter,
					szDateStart
					);

				//Check for Notification
				musage=0;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					musage=float(total_real)/EnergyDivider;
					if (musage!=0)
						m_notifications.CheckAndHandleNotification(ID, devname, devType, subType, NTYPE_TODAYENERGY, musage);
					break;
				case MTYPE_GAS:
					musage=float(total_real)/tGasDivider;
					if (musage!=0)
						m_notifications.CheckAndHandleNotification(ID, devname, devType, subType, NTYPE_TODAYGAS, musage);
					break;
				case MTYPE_WATER:
					musage=float(total_real)/WaterDivider;
					if (musage!=0)
						m_notifications.CheckAndHandleNotification(ID, devname, devType, subType, NTYPE_TODAYGAS, musage);
					break;
				case MTYPE_COUNTER:
					musage=float(total_real);
					if (musage!=0)
						m_notifications.CheckAndHandleNotification(ID, devname, devType, subType, NTYPE_TODAYCOUNTER, musage);
					break;
				default:
					//Unhandled
					musage = 0;
					break;
				}
			}
			else
			{
				//AirQuality/Usage Meter/Moisture/RFXSensor/Voltage/Lux/SoundLevel insert into MultiMeter_Calendar table
				result=safe_query(
					"INSERT INTO MultiMeter_Calendar (DeviceRowID, Value1,Value2,Value3,Value4,Value5,Value6, Date) "
					"VALUES ('%" PRIu64 "', '%.2f','%.2f','%.2f','%.2f','%.2f','%.2f', '%q')",
					ID,
					total_min,total_max, avg_value,0.0f,0.0f,0.0f,
					szDateStart
					);
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
				result = safe_query("SELECT Value FROM Meter WHERE (DeviceRowID='%" PRIu64 "') ORDER BY ROWID DESC LIMIT 1", ID);
				if (result.size() > 0)
				{
					std::vector<std::string> sd = result[0];
					//Insert the last (max) counter value into the meter table to get the "today" value correct.
					result = safe_query(
						"INSERT INTO Meter (DeviceRowID, Value, Date) "
						"VALUES ('%" PRIu64 "', '%q', '%q')",
						ID,
						sd[0].c_str(),
						szDateEnd
					);
				}
			}
		}
		else
		{
			//no new meter result received in last day
			//insert into calendar table
			result=safe_query(
				"INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) "
				"VALUES ('%" PRIu64 "', '%.2f', '%q')",
				ID,
				0.0f,
				szDateStart
				);
		}
	}
}

void CSQLHelper::AddCalendarUpdateMultiMeter()
{
	float EnergyDivider=1000.0f;
	int tValue;
	if (GetPreferencesVar("MeterDividerEnergy", tValue))
	{
		EnergyDivider=float(tValue);
	}

	//Get All meter devices
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM MultiMeter ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		//Get Device Information
		result=safe_query("SELECT Name, HardwareID, DeviceID, Unit, Type, SubType, SwitchType FROM DeviceStatus WHERE (ID='%" PRIu64 "')",ID);
		if (result.size()<1)
			continue;
		std::vector<std::string> sd=result[0];

		std::string devname = sd[0];
		//int hardwareID= atoi(sd[1].c_str());
		//std::string DeviceID=sd[2];
		//unsigned char Unit = atoi(sd[3].c_str());
		unsigned char devType=atoi(sd[4].c_str());
		unsigned char subType=atoi(sd[5].c_str());
		//_eSwitchType switchtype=(_eSwitchType) atoi(sd[6].c_str());
		//_eMeterType metertype=(_eMeterType)switchtype;

		result=safe_query(
			"SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2), MIN(Value3), MAX(Value3), MIN(Value4), MAX(Value4), MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM MultiMeter WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
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
			result=safe_query(
				"INSERT INTO MultiMeter_Calendar (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Counter1, Counter2, Counter3, Counter4, Date) "
				"VALUES ('%" PRIu64 "', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%q')",
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

			//Check for Notification
			if (devType==pTypeP1Power)
			{
				float musage=(total_real[0]+total_real[2])/EnergyDivider;
				m_notifications.CheckAndHandleNotification(ID, devname, devType, subType, NTYPE_TODAYENERGY, musage);
			}
/*
			//Insert the last (max) counter values into the table to get the "today" value correct.
			sprintf(szTmp,
				"INSERT INTO MultiMeter (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Date) "
				"VALUES (%" PRIu64 ", %s, %s, %s, %s, %s, %s, '%s')",
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
	//Get All Wind devices
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM Wind ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		result=safe_query("SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float Direction = static_cast<float>(atof(sd[0].c_str()));
			int speed_min=atoi(sd[1].c_str());
			int speed_max=atoi(sd[2].c_str());
			int gust_min=atoi(sd[3].c_str());
			int gust_max=atoi(sd[4].c_str());

			//insert into calendar table
			result=safe_query(
				"INSERT INTO Wind_Calendar (DeviceRowID, Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date) "
				"VALUES ('%" PRIu64 "', '%.2f', '%d', '%d', '%d', '%d', '%q')",
				ID,
				Direction,
				speed_min,
				speed_max,
				gust_min,
				gust_max,
				szDateStart
				);
		}
	}
}

void CSQLHelper::AddCalendarUpdateUV()
{
	//Get All UV devices
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM UV ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		result=safe_query("SELECT MAX(Level) FROM UV WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float level = static_cast<float>(atof(sd[0].c_str()));

			//insert into calendar table
			result=safe_query(
				"INSERT INTO UV_Calendar (DeviceRowID, Level, Date) "
				"VALUES ('%" PRIu64 "', '%g', '%q')",
				ID,
				level,
				szDateStart
				);
		}
	}
}

void CSQLHelper::AddCalendarUpdatePercentage()
{
	//Get All Percentage devices in the Percentage Table
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM Percentage ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		result=safe_query("SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage) FROM Percentage WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float percentage_min = static_cast<float>(atof(sd[0].c_str()));
			float percentage_max = static_cast<float>(atof(sd[1].c_str()));
			float percentage_avg = static_cast<float>(atof(sd[2].c_str()));
			//insert into calendar table
			result=safe_query(
				"INSERT INTO Percentage_Calendar (DeviceRowID, Percentage_Min, Percentage_Max, Percentage_Avg, Date) "
				"VALUES ('%" PRIu64 "', '%g', '%g', '%g','%q')",
				ID,
				percentage_min,
				percentage_max,
				percentage_avg,
				szDateStart
				);
		}
	}
}


void CSQLHelper::AddCalendarUpdateFan()
{
	//Get All FAN devices in the Fan Table
	std::vector<std::vector<std::string> > resultdevices;
	resultdevices=safe_query("SELECT DISTINCT(DeviceRowID) FROM Fan ORDER BY DeviceRowID");
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	time_t yesterday;
	struct tm tm2;
	getNoon(yesterday,tm2,ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday-1); // we only want the date
	sprintf(szDateStart,"%04d-%02d-%02d",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday);

	std::vector<std::vector<std::string> > result;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=resultdevices.begin(); itt!=resultdevices.end(); ++itt)
	{
		std::vector<std::string> sddev=*itt;
		uint64_t ID;
		std::stringstream s_str( sddev[0] );
		s_str >> ID;

		result=safe_query("SELECT MIN(Speed), MAX(Speed), AVG(Speed) FROM Fan WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q' AND Date<'%q')",
			ID,
			szDateStart,
			szDateEnd
			);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			int speed_min=(int)atoi(sd[0].c_str());
			int speed_max=(int)atoi(sd[1].c_str());
			int speed_avg=(int)atoi(sd[2].c_str());
			//insert into calendar table
			result=safe_query(
				"INSERT INTO Fan_Calendar (DeviceRowID, Speed_Min, Speed_Max, Speed_Avg, Date) "
				"VALUES ('%" PRIu64 "', '%d', '%d', '%d','%q')",
				ID,
				speed_min,
				speed_max,
				speed_avg,
				szDateStart
				);
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
#if 0
		char szDateStr[40];
		time_t clear_time = mytime(NULL) - (n5MinuteHistoryDays * 24 * 3600);
		struct tm ltime;
		localtime_r(&clear_time, &ltime);
		sprintf(szDateStr, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
		_log.Log(LOG_STATUS, "Cleaning up shortlog older than %s", szDateStr);
#endif

		char szQuery[250];
		std::string szQueryFilter = "strftime('%s',datetime('now','localtime')) - strftime('%s',Date) > (SELECT p.nValue * 86400 From Preferences AS p WHERE p.Key='5MinuteHistoryDays')";

		sprintf(szQuery, "DELETE FROM Temperature WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM Rain WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM Wind WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM UV WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM Meter WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM MultiMeter WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM Percentage WHERE %s", szQueryFilter.c_str());
		query(szQuery);

		sprintf(szQuery, "DELETE FROM Fan WHERE %s", szQueryFilter.c_str());
		query(szQuery);
	}
}

void CSQLHelper::ClearShortLog()
{
	query("DELETE FROM Temperature");
	query("DELETE FROM Rain");
	query("DELETE FROM Wind");
	query("DELETE FROM UV");
	query("DELETE FROM Meter");
	query("DELETE FROM MultiMeter");
	query("DELETE FROM Percentage");
	query("DELETE FROM Fan");
	VacuumDatabase();
}

void CSQLHelper::VacuumDatabase()
{
	query("VACUUM");
}

void CSQLHelper::OptimizeDatabase(sqlite3 *dbase)
{
	if (dbase == NULL)
		return;
	sqlite3_exec(dbase, "PRAGMA optimize;", NULL, NULL, NULL);
}

void CSQLHelper::DeleteHardware(const std::string &idx)
{
	safe_query("DELETE FROM Hardware WHERE (ID == '%q')",idx.c_str());

	//and now delete all records in the DeviceStatus table itself
	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID == '%q')",idx.c_str());
	if (result.size()>0)
	{
		std::string devs2delete = "";
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			if (!devs2delete.empty())
				devs2delete += ";";
			devs2delete += sd[0];
		}
		DeleteDevices(devs2delete);
	}
	//also delete all records in other tables
	safe_query("DELETE FROM ZWaveNodes WHERE (HardwareID== '%q')",idx.c_str());
	safe_query("DELETE FROM EnoceanSensors WHERE (HardwareID== '%q')", idx.c_str());
	safe_query("DELETE FROM MySensors WHERE (HardwareID== '%q')", idx.c_str());
	safe_query("DELETE FROM WOLNodes WHERE (HardwareID == '%q')",idx.c_str());
}

void CSQLHelper::DeleteCamera(const std::string &idx)
{
	safe_query("DELETE FROM Cameras WHERE (ID == '%q')",idx.c_str());
	safe_query("DELETE FROM CamerasActiveDevices WHERE (CameraRowID == '%q')",idx.c_str());
}

void CSQLHelper::DeletePlan(const std::string &idx)
{
	safe_query("DELETE FROM Plans WHERE (ID == '%q')",idx.c_str());
}

void CSQLHelper::DeleteEvent(const std::string &idx)
{
	safe_query("DELETE FROM EventRules WHERE (EMID == '%q')",idx.c_str());
	safe_query("DELETE FROM EventMaster WHERE (ID == '%q')",idx.c_str());
}

//Argument, one or multiple devices separated by a semicolumn (;)
void CSQLHelper::DeleteDevices(const std::string &idx)
{
	std::vector<std::string> _idx;
	StringSplit(idx, ";", _idx);
	if (!_idx.empty())
	{
		//Avoid mutex deadlock here
		boost::lock_guard<boost::mutex> l(m_sqlQueryMutex);
		std::vector<std::string>::const_iterator itt;

		char* errorMessage;
		sqlite3_exec(m_dbase, "BEGIN TRANSACTION", NULL, NULL, &errorMessage);

		for (itt = _idx.begin(); itt != _idx.end(); ++itt)
		{
			safe_exec_no_return("DELETE FROM LightingLog WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM LightSubDevices WHERE (ParentID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM LightSubDevices WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Notifications WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Rain WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Rain_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Temperature WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Temperature_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Timers WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM SetpointTimers WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM UV WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM UV_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Wind WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Wind_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Meter WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Meter_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM MultiMeter WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM MultiMeter_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Percentage WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM Percentage_Calendar WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM SceneDevices WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM DeviceToPlansMap WHERE (DeviceRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM CamerasActiveDevices WHERE (DevSceneType==0) AND (DevSceneRowID == '%q')", (*itt).c_str());
			safe_exec_no_return("DELETE FROM SharedDevices WHERE (DeviceRowID== '%q')", (*itt).c_str());
			//notify eventsystem device is no longer present
			std::stringstream sstridx(*itt);
			uint64_t ullidx;
			sstridx >> ullidx;
			m_mainworker.m_eventsystem.RemoveSingleState(ullidx, m_mainworker.m_eventsystem.REASON_DEVICE);
			//and now delete all records in the DeviceStatus table itself
			safe_exec_no_return("DELETE FROM DeviceStatus WHERE (ID == '%q')", (*itt).c_str());
		}
		sqlite3_exec(m_dbase, "COMMIT TRANSACTION", NULL, NULL, &errorMessage);
	}
	else
		return;

	m_notifications.ReloadNotifications();
}

void CSQLHelper::TransferDevice(const std::string &idx, const std::string &newidx)
{
	std::vector<std::vector<std::string> > result;

	safe_query("UPDATE LightingLog SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());
	safe_query("UPDATE LightSubDevices SET ParentID='%q' WHERE (ParentID == '%q')",newidx.c_str(),idx.c_str());
	safe_query("UPDATE LightSubDevices SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());
	safe_query("UPDATE Notifications SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());
	safe_query("UPDATE DeviceToPlansMap SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')", newidx.c_str(), idx.c_str());
	safe_query("UPDATE SharedDevices SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')", newidx.c_str(), idx.c_str());

	//Rain
	result=safe_query("SELECT Date FROM Rain WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Rain SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Rain SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	result=safe_query("SELECT Date FROM Rain_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Rain_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Rain_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	//Temperature
	result=safe_query("SELECT Date FROM Temperature WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Temperature SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Temperature SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	result=safe_query("SELECT Date FROM Temperature_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Temperature_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Temperature_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	safe_query("UPDATE Timers SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	//UV
	result=safe_query("SELECT Date FROM UV WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE UV SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE UV SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	result=safe_query("SELECT Date FROM UV_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE UV_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE UV_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	//Wind
	result=safe_query("SELECT Date FROM Wind WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Wind SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Wind SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	result=safe_query("SELECT Date FROM Wind_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Wind_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Wind_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	//Meter
	result=safe_query("SELECT Date FROM Meter WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Meter SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Meter SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	result=safe_query("SELECT Date FROM Meter_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE Meter_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE Meter_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	//Multimeter
	result=safe_query("SELECT Date FROM MultiMeter WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE MultiMeter SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE MultiMeter SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	result=safe_query("SELECT Date FROM MultiMeter_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1",newidx.c_str());
	if (result.size()>0)
		safe_query("UPDATE MultiMeter_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')",newidx.c_str(),idx.c_str(),result[0][0].c_str());
	else
		safe_query("UPDATE MultiMeter_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')",newidx.c_str(),idx.c_str());

	//Percentage
	result = safe_query("SELECT Date FROM Percentage WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1", newidx.c_str());
	if (result.size() > 0)
		safe_query("UPDATE Percentage SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')", newidx.c_str(), idx.c_str(), result[0][0].c_str());
	else
		safe_query("UPDATE Percentage SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')", newidx.c_str(), idx.c_str());

	result = safe_query("SELECT Date FROM Percentage_Calendar WHERE (DeviceRowID == '%q') ORDER BY Date ASC LIMIT 1", newidx.c_str());
	if (result.size() > 0)
		safe_query("UPDATE Percentage_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q') AND (Date<'%q')", newidx.c_str(), idx.c_str(), result[0][0].c_str());
	else
		safe_query("UPDATE Percentage_Calendar SET DeviceRowID='%q' WHERE (DeviceRowID == '%q')", newidx.c_str(), idx.c_str());
}

void CSQLHelper::CheckAndUpdateDeviceOrder()
{
	std::vector<std::vector<std::string> > result;

	//Get All ID's where Order=0
	result=safe_query("SELECT ROWID FROM DeviceStatus WHERE ([Order]==0)");
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			safe_query("UPDATE DeviceStatus SET [Order] = (SELECT MAX([Order]) FROM DeviceStatus)+1 WHERE (ROWID == '%q')", sd[0].c_str());
		}
	}
}

void CSQLHelper::CheckAndUpdateSceneDeviceOrder()
{
	std::vector<std::vector<std::string> > result;

	//Get All ID's where Order=0
	result=safe_query("SELECT ROWID FROM SceneDevices WHERE ([Order]==0)");
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			safe_query("UPDATE SceneDevices SET [Order] = (SELECT MAX([Order]) FROM SceneDevices)+1 WHERE (ROWID == '%q')", sd[0].c_str());
		}
	}
}

void CSQLHelper::CleanupLightSceneLog()
{
	//cleanup the lighting log
	int nMaxDays=30;
	GetPreferencesVar("LightHistoryDays", nMaxDays);

	char szDateEnd[40];
	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	time_t daybefore;
	struct tm tm2;
	constructTime(daybefore,tm2,tm1.tm_year+1900,tm1.tm_mon+1,tm1.tm_mday-nMaxDays,tm1.tm_hour,tm1.tm_min,0,tm1.tm_isdst);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);


	safe_query("DELETE FROM LightingLog WHERE (Date<'%q')", szDateEnd);
	safe_query("DELETE FROM SceneLog WHERE (Date<'%q')", szDateEnd);
}

bool CSQLHelper::DoesSceneByNameExits(const std::string &SceneName)
{
	std::vector<std::vector<std::string> > result;

	//Get All ID's where Order=0
	result=safe_query("SELECT ID FROM Scenes WHERE (Name=='%q')", SceneName.c_str());
	return (result.size()>0);
}

void CSQLHelper::CheckSceneStatusWithDevice(const std::string &DevIdx)
{
	std::stringstream s_str( DevIdx );
	uint64_t idxll;
	s_str >> idxll;
	return CheckSceneStatusWithDevice(idxll);
}

void CSQLHelper::CheckSceneStatusWithDevice(const uint64_t DevIdx)
{
	std::vector<std::vector<std::string> > result;

	result=safe_query("SELECT SceneRowID FROM SceneDevices WHERE (DeviceRowID == %" PRIu64 ")", DevIdx);
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
	uint64_t idxll;
	s_str >> idxll;
	return CheckSceneStatus(idxll);
}

void CSQLHelper::CheckSceneStatus(const uint64_t Idx)
{
	std::vector<std::vector<std::string> > result;

	result=safe_query("SELECT nValue FROM Scenes WHERE (ID == %" PRIu64 ")", Idx);
	if (result.size()<1)
		return; //not found

	unsigned char orgValue=(unsigned char)atoi(result[0][0].c_str());
	unsigned char newValue=orgValue;

	result=safe_query("SELECT a.ID, a.DeviceID, a.Unit, a.Type, a.SubType, a.SwitchType, a.nValue, a.sValue FROM DeviceStatus AS a, SceneDevices as b WHERE (a.ID == b.DeviceRowID) AND (b.SceneRowID == %" PRIu64 ")",
		Idx);
	if (result.size()<1)
		return; //no devices in scene

	std::vector<std::vector<std::string> >::const_iterator itt;

	std::vector<bool> _DeviceStatusResults;

	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		int nValue=atoi(sd[6].c_str());
		std::string sValue=sd[7];
		//unsigned char Unit=atoi(sd[2].c_str());
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
		safe_query("UPDATE Scenes SET nValue=%d WHERE (ID == %" PRIu64 ")",
			int(newValue), Idx);
	}
}

void CSQLHelper::DeleteDataPoint(const char *ID, const std::string &Date)
{
	std::vector<std::vector<std::string> > result;
	result=safe_query("SELECT Type,SubType FROM DeviceStatus WHERE (ID==%q)",ID);
	if (result.size()<1)
		return;
	//std::vector<std::string> sd=result[0];
	//unsigned char dType=atoi(sd[0].c_str());
	//unsigned char dSubType=atoi(sd[1].c_str());

	if (Date.find(':')!=std::string::npos)
	{
		char szDateEnd[100];

		time_t now = mytime(NULL);
		struct tm tLastUpdate;
		localtime_r(&now, &tLastUpdate);

		time_t cEndTime;
		ParseSQLdatetime(cEndTime, tLastUpdate, Date, tLastUpdate.tm_isdst);
		tLastUpdate.tm_min += 2;
		cEndTime = mktime(&tLastUpdate);

//GB3:	ToDo: Database should know the difference between Summer and Winter time,
//	or we'll be deleting both entries if the DataPoint is inside a DST jump

		sprintf(szDateEnd, "%04d-%02d-%02d %02d:%02d:%02d", tLastUpdate.tm_year + 1900, tLastUpdate.tm_mon + 1, tLastUpdate.tm_mday, tLastUpdate.tm_hour, tLastUpdate.tm_min, tLastUpdate.tm_sec);
		//Short log
		safe_query("DELETE FROM Rain WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM Wind WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM UV WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM Temperature WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM Meter WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM MultiMeter WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM Percentage WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
		safe_query("DELETE FROM Fan WHERE (DeviceRowID=='%q') AND (Date>='%q') AND (Date<='%q')",ID,Date.c_str(),szDateEnd);
	}
	else
	{
		//Day/Month/Year
		safe_query("DELETE FROM Rain_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM Wind_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM UV_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM Temperature_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM Meter_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM MultiMeter_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM Percentage_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
		safe_query("DELETE FROM Fan_Calendar WHERE (DeviceRowID=='%q') AND (Date=='%q')",ID,Date.c_str());
	}
}

void CSQLHelper::AddTaskItem(const _tTaskItem &tItem)
{
	boost::lock_guard<boost::mutex> l(m_background_task_mutex);

	// Check if an event for the same device is already in queue, and if so, replace it
	if (_log.isTraceEnabled())
	   _log.Log(LOG_TRACE, "SQLH AddTask: Request to add task: idx=%" PRIu64 ", DelayTime=%f, Command='%s', Level=%d, Hue=%d, RelatedEvent='%s'", tItem._idx, tItem._DelayTime, tItem._command.c_str(), tItem._level, tItem._Hue, tItem._relatedEvent.c_str());
	// Remove any previous task linked to the same device

	if (
		(tItem._ItemType == TITEM_SWITCHCMD_EVENT) ||
		(tItem._ItemType == TITEM_SWITCHCMD_SCENE)
		)
	{
		std::vector<_tTaskItem>::iterator itt = m_background_task_queue.begin();
		while (itt != m_background_task_queue.end())
		{
			if (_log.isTraceEnabled())
				 _log.Log(LOG_TRACE, "SQLH AddTask: Comparing with item in queue: idx=%llu, DelayTime=%d, Command='%s', Level=%d, Hue=%d, RelatedEvent='%s'", itt->_idx, itt->_DelayTime, itt->_command.c_str(), itt->_level, itt->_Hue, itt->_relatedEvent.c_str());
			if (itt->_idx == tItem._idx && itt->_ItemType == tItem._ItemType)
			{
				float iDelayDiff = tItem._DelayTime - itt->_DelayTime;
				if (iDelayDiff < (1./timer_resolution_hz/2))
				{
					if (_log.isTraceEnabled())
						 _log.Log(LOG_TRACE, "SQLH AddTask: => Already present. Cancelling previous task item");
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
	_log.Log(LOG_STATUS, "Restore Database: Starting...");
	//write file to disk
	std::string fpath;
#ifdef WIN32
	size_t bpos=m_dbase_name.rfind('\\');
#else
	size_t bpos=m_dbase_name.rfind('/');
#endif
	if (bpos!=std::string::npos)
		fpath=m_dbase_name.substr(0,bpos+1);
#ifdef WIN32
	std::string outputfile=fpath+"restore.db";
#else
	std::string outputfile = "/tmp/restore.db";
#endif
	std::ofstream outfile;
	outfile.open(outputfile.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (!outfile.is_open())
	{
		_log.Log(LOG_ERROR, "Restore Database: Could not open backup file for writing!");
		return false;
	}
	outfile << dbase;
	outfile.flush();
	outfile.close();
	//check if we can open the database (check if valid)
	sqlite3 *dbase_restore=NULL;
	int rc = sqlite3_open(outputfile.c_str(), &dbase_restore);
	if (rc)
	{
		_log.Log(LOG_ERROR,"Restore Database: Could not open SQLite3 database: %s", sqlite3_errmsg(dbase_restore));
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
		_log.Log(LOG_ERROR, "Restore Database: Seems this is not our database, or it is corrupted!");
		sqlite3_close(dbase_restore);
		return false;
	}
	OptimizeDatabase(dbase_restore);
	sqlite3_close(dbase_restore);
	//we have a valid database!
	std::remove(outputfile.c_str());
	//stop database
	sqlite3_close(m_dbase);
	m_dbase=NULL;
	std::ofstream outfile2;
	outfile2.open(m_dbase_name.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (!outfile2.is_open())
	{
		_log.Log(LOG_ERROR, "Restore Database: Could not open backup file for writing!");
		return false;
	}
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
			_log.Log(LOG_ERROR, "Restore Database: Could not set database ownership (chown returned an error!)");
		}
	}
#endif
	if (!OpenDatabase())
	{
		_log.Log(LOG_ERROR, "Restore Database: Error opening new database!");
		return false;
	}
	//Cleanup the database
	VacuumDatabase();
	_log.Log(LOG_STATUS, "Restore Database: Succeeded!");
	return true;
}

bool CSQLHelper::BackupDatabase(const std::string &OutputFile)
{
	if (!m_dbase)
		return false; //database not open!

	//First cleanup the database
	OptimizeDatabase(m_dbase);
	VacuumDatabase();

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

uint64_t CSQLHelper::UpdateValueLighting2GroupCmd(const int HardwareID, const char* ID, const unsigned char unit,
	const unsigned char devType, const unsigned char subType,
	const unsigned char signallevel, const unsigned char batterylevel,
	const int nValue, const char* sValue,
	std::string &devname,
	const bool bUseOnOffAction)
{
	// We only have to update all others units within the ID group. If the current unit does not have the same value,
	// it will be updated too. The reason we choose the UpdateValue is the propagation of the change to all units involved, including LogUpdate.

	uint64_t devRowIndex = -1;
	typedef std::vector<std::vector<std::string> > VectorVectorString;

	VectorVectorString result = safe_query("SELECT Unit FROM DeviceStatus WHERE ((DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (nValue!=%d))",
		ID,
		pTypeLighting2,
		subType,
		nValue);

	for (VectorVectorString::const_iterator itt = result.begin(); itt != result.end(); ++itt)
	{
		unsigned char theUnit = atoi((*itt)[0].c_str()); // get the unit value
		devRowIndex = UpdateValue(HardwareID, ID, theUnit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction);
	}
	return devRowIndex;
}

void CSQLHelper::Lighting2GroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd)
{
	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now, &ltime);

	safe_query("UPDATE DeviceStatus SET nValue='%d', sValue='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (DeviceID=='%q') And (Type==%d) And (SubType==%d) And (nValue!=%d)",
		GroupCmd,
		"OFF",
		ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		ID.c_str(),
		pTypeLighting2,
		subType,
		GroupCmd);
}

uint64_t CSQLHelper::UpdateValueHomeConfortGroupCmd(const int HardwareID, const char* ID, const unsigned char unit,
	const unsigned char devType, const unsigned char subType,
	const unsigned char signallevel, const unsigned char batterylevel,
	const int nValue, const char* sValue,
	std::string &devname,
	const bool bUseOnOffAction)
{
	// We only have to update all others units within the ID group. If the current unit does not have the same value,
	// it will be updated too. The reason we choose the UpdateValue is the propagation of the change to all units involved, including LogUpdate.

	uint64_t devRowIndex = -1;
	typedef std::vector<std::vector<std::string> > VectorVectorString;

	VectorVectorString result = safe_query("SELECT Unit FROM DeviceStatus WHERE ((DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (nValue!=%d))",
		ID,
		pTypeHomeConfort,
		subType,
		nValue);

	for (VectorVectorString::const_iterator itt = result.begin(); itt != result.end(); ++itt)
	{
		unsigned char theUnit = atoi((*itt)[0].c_str()); // get the unit value
		devRowIndex = UpdateValue(HardwareID, ID, theUnit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction);
	}
	return devRowIndex;
}

void CSQLHelper::HomeConfortGroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd)
{
	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now, &ltime);

	safe_query("UPDATE DeviceStatus SET nValue='%s', sValue='%q', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (DeviceID=='%q') And (Type==%d) And (SubType==%d) And (nValue!=%d)",
		GroupCmd,
		"OFF",
		ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		ID.c_str(),
		pTypeHomeConfort,
		subType,
		GroupCmd);
}

void CSQLHelper::GeneralSwitchGroupCmd(const std::string &ID, const unsigned char subType, const unsigned char GroupCmd)
{
	safe_query("UPDATE DeviceStatus SET nValue = %d WHERE (DeviceID=='%q') And (Type==%d) And (SubType==%d)", GroupCmd, ID.c_str(), pTypeGeneralSwitch, subType);
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
	else if (m_windunit == WINDUNIT_Beaufort)
	{
		m_windsign = "bf";
		m_windscale = 1;
	}

	//Temp
	if (m_tempunit==TEMPUNIT_C)
	{
		m_tempsign="C";
		m_tempscale=1.0f;
	}
    else if (m_tempunit==TEMPUNIT_F)
	{
		m_tempsign="F";
		m_tempscale=1.0f; // *1.8 + 32
	}

    if(m_weightunit == WEIGHTUNIT_KG)
    {
        m_weightsign="kg";
        m_weightscale=1.0f;
    }
    else if(m_weightunit == WEIGHTUNIT_LB)
    {
        m_weightsign="lb";
        m_weightscale=2.20462f;
    }
}

bool CSQLHelper::HandleOnOffAction(const bool bIsOn, const std::string &OnAction, const std::string &OffAction)
{
	if (_log.isTraceEnabled())
	{
		if (bIsOn)
			_log.Log(LOG_TRACE, "SQLH HandleOnOffAction: OnAction:%s", OnAction.c_str());
		else
			_log.Log(LOG_TRACE, "SQLH HandleOnOffAction: OffAction:%s", OffAction.c_str());
	}

	if (bIsOn)
	{
		if (OnAction.empty())
			return true;

		if ((OnAction.find("http://") != std::string::npos) || (OnAction.find("https://") != std::string::npos))
		{
			AddTaskItem(_tTaskItem::GetHTTPPage(0.2f, OnAction, "SwitchActionOn"));
		}
		else if (OnAction.find("script://")!=std::string::npos)
		{
			//Execute possible script
			std::string scriptname = OnAction.substr(9);
#if !defined WIN32
			if (scriptname.find("/") != 0)
				scriptname = szUserDataFolder + "scripts/" + scriptname;
#endif
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
				AddTaskItem(_tTaskItem::ExecuteScript(0.2f,scriptname,scriptparams));
			}
			else
				_log.Log(LOG_ERROR, "SQLHelper: Error script not found '%s'", scriptname.c_str());
		}
		return true;
	}

	//Off action
	if (OffAction.empty())
		return true;

	if ((OffAction.find("http://") != std::string::npos) || (OffAction.find("https://") != std::string::npos))
	{
		AddTaskItem(_tTaskItem::GetHTTPPage(0.2f, OffAction, "SwitchActionOff"));
	}
	else if (OffAction.find("script://") != std::string::npos)
	{
		//Execute possible script
		std::string scriptname = OffAction.substr(9);
#if !defined WIN32
		if (scriptname.find("/") != 0)
			scriptname = szUserDataFolder + "scripts/" + scriptname;
#endif
		std::string scriptparams = "";
		int pindex = scriptname.find(' ');
		if (pindex != std::string::npos)
		{
			scriptparams = scriptname.substr(pindex + 1);
			scriptname = scriptname.substr(0, pindex);
		}
		if (file_exist(scriptname.c_str()))
		{
			AddTaskItem(_tTaskItem::ExecuteScript(0.2f, scriptname, scriptparams));
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

	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT ID,Name, BatteryLevel FROM DeviceStatus WHERE (Used!=0 AND BatteryLevel<%d AND BatteryLevel!=255)", iBatteryLowLevel);
	if (result.size() < 1)
		return;

	time_t now = mytime(NULL);
	struct tm stoday;
	localtime_r(&now, &stoday);

	uint64_t ulID;
	std::vector<std::vector<std::string> >::const_iterator itt;

	//check if last batterylow_notification is not sent today and if true, send notification
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;
		std::stringstream s_str(sd[0]);
		s_str >> ulID;
		bool bDoSend = true;
		std::map<uint64_t, int>::const_iterator sitt;
		sitt = m_batterylowlastsend.find(ulID);
		if (sitt != m_batterylowlastsend.end())
		{
			bDoSend = (stoday.tm_mday != sitt->second);
		}
		if (bDoSend)
		{
			char szTmp[300];
			int batlevel = atoi(sd[2].c_str());
			if (batlevel==0)
				sprintf(szTmp, "Battery Low: %s (Level: Low)", sd[1].c_str());
			else
				sprintf(szTmp, "Battery Low: %s (Level: %d %%)", sd[1].c_str(), batlevel);
			m_notifications.SendMessageEx(0, std::string(""), NOTIFYALL, szTmp, szTmp, std::string(""), 1, std::string(""), true);
			m_batterylowlastsend[ulID] = stoday.tm_mday;
		}
	}
}

//Executed every hour
void CSQLHelper::CheckDeviceTimeout()
{
	int TimeoutCheckInterval=1;
	GetPreferencesVar("SensorTimeoutNotification", TimeoutCheckInterval);

	if (TimeoutCheckInterval==0)
		return;
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
	result = safe_query(
		"SELECT ID, Name, LastUpdate FROM DeviceStatus WHERE (Used!=0 AND LastUpdate<='%04d-%02d-%02d %02d:%02d:%02d' "
		"AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d "
		"AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d) "
		"ORDER BY Name",
		ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		pTypeLighting1,
		pTypeLighting2,
		pTypeLighting3,
		pTypeLighting4,
		pTypeLighting5,
		pTypeLighting6,
		pTypeFan,
		pTypeRadiator1,
		pTypeLimitlessLights,
		pTypeSecurity1,
		pTypeCurtain,
		pTypeBlinds,
		pTypeRFY,
		pTypeChime,
		pTypeThermostat2,
		pTypeThermostat3,
		pTypeThermostat4,
		pTypeRemote,
		pTypeGeneralSwitch,
		pTypeHomeConfort
		);
	if (result.size()<1)
		return;

	uint64_t ulID;
	std::vector<std::vector<std::string> >::const_iterator itt;

	//check if last timeout_notification is not sent today and if true, send notification
	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		std::stringstream s_str( sd[0] );
		s_str >> ulID;
		bool bDoSend=true;
		std::map<uint64_t,int>::const_iterator sitt;
		sitt=m_timeoutlastsend.find(ulID);
		if (sitt!=m_timeoutlastsend.end())
		{
			bDoSend=(stoday.tm_mday!=sitt->second);
		}
		if (bDoSend)
		{
			char szTmp[300];
			sprintf(szTmp,"Sensor Timeout: %s, Last Received: %s",sd[1].c_str(),sd[2].c_str());
			m_notifications.SendMessageEx(0, std::string(""), NOTIFYALL, szTmp, szTmp, std::string(""), 1, std::string(""), true);
			m_timeoutlastsend[ulID]=stoday.tm_mday;
		}
	}
}

void CSQLHelper::FixDaylightSavingTableSimple(const std::string &TableName)
{
	std::vector<std::vector<std::string> > result;

	result=safe_query("SELECT t.RowID, u.RowID, t.Date FROM %s as t, %s as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID]) ORDER BY t.[RowID]",
		TableName.c_str(),
		TableName.c_str());
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
				std::vector<std::vector<std::string> > result2;
				result2=safe_query("SELECT date('%q','+1 day')",
					szDate.c_str());

				std::string szDateNew=result2[0][0];

				//Check if Date+1 exists, if yes, remove current double value
				result2=safe_query("SELECT RowID FROM %s WHERE (Date='%q') AND (RowID=='%q')",
					TableName.c_str(), szDateNew.c_str(), sd[1].c_str());
				if (result2.size()>0)
				{
					//Delete row
					safe_query("DELETE FROM %s WHERE (RowID=='%q')", TableName.c_str(), sd[1].c_str());
				}
				else
				{
					//Update date
					safe_query("UPDATE %s SET Date='%q' WHERE (RowID=='%q')", TableName.c_str(), szDateNew.c_str(), sd[1].c_str());
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

	result=safe_query("SELECT t.RowID, u.RowID, t.Value, u.Value, t.Date from Meter_Calendar as t, Meter_Calendar as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID]) ORDER BY t.[RowID]");
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
				std::vector<std::vector<std::string> > result2;
				result2=safe_query("SELECT date('%q','+1 day')", szDate.c_str());

				std::string szDateNew=result2[0][0];

				//Check if Date+1 exists, if yes, remove current double value
				result2=safe_query("SELECT RowID FROM Meter_Calendar WHERE (Date='%q') AND (RowID=='%q')", szDateNew.c_str(), sd1[1].c_str());
				if (result2.size()>0)
				{
					//Delete Row
					safe_query("DELETE FROM Meter_Calendar WHERE (RowID=='%q')", sd1[1].c_str());
				}
				else
				{
					//Update row with new Date
					safe_query("UPDATE Meter_Calendar SET Date='%q', Value=%llu WHERE (RowID=='%q')", szDateNew.c_str(), ValueDest, sd1[1].c_str());
				}
			}
		}
	}

	//Last (but not least) MultiMeter_Calendar
	result=safe_query("SELECT t.RowID, u.RowID, t.Value1, t.Value2, t.Value3, t.Value4, t.Value5, t.Value6, u.Value1, u.Value2, u.Value3, u.Value4, u.Value5, u.Value6, t.Date from MultiMeter_Calendar as t, MultiMeter_Calendar as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID]) ORDER BY t.[RowID]");
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
				std::vector<std::vector<std::string> > result2;
				result2=safe_query("SELECT date('%q','+1 day')", szDate.c_str());

				std::string szDateNew=result2[0][0];

				//Check if Date+1 exists, if yes, remove current double value
				result2=safe_query("SELECT RowID FROM MultiMeter_Calendar WHERE (Date='%q') AND (RowID=='%q')", szDateNew.c_str(), sd1[1].c_str());
				if (result2.size()>0)
				{
					//Delete Row
					safe_query("DELETE FROM MultiMeter_Calendar WHERE (RowID=='%q')", sd1[1].c_str());
				}
				else
				{
					//Update row with new Date
					safe_query("UPDATE MultiMeter_Calendar SET Date='%q', Value1=%llu, Value2=%llu, Value3=%llu, Value4=%llu, Value5=%llu, Value6=%llu WHERE (RowID=='%q')",
						szDateNew.c_str(), ValueDest1, ValueDest2, ValueDest3, ValueDest4, ValueDest5, ValueDest6, sd1[1].c_str());
				}
			}
		}
	}

}

std::string CSQLHelper::DeleteUserVariable(const std::string &idx)
{
	safe_query("DELETE FROM UserVariables WHERE (ID=='%q')", idx.c_str());
	if (m_bEnableEventSystem)
	{
		m_mainworker.m_eventsystem.GetCurrentUserVariables();
	}

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
	safe_query("INSERT INTO UserVariables (Name,ValueType,Value) VALUES ('%q','%d','%q')",
		varname.c_str(),
		typei,
		CURLEncode::URLDecode(varvalue.c_str()).c_str()
		);

	if (m_bEnableEventSystem)
	{
		m_mainworker.m_eventsystem.GetCurrentUserVariables();
		result = safe_query("SELECT ID, LastUpdate FROM UserVariables WHERE (Name == '%q')",
			varname.c_str()
		);
		if (result.size()>0)
		{
			std::vector<std::string> sd = result[0];
			std::stringstream vId_str(sd[0]);
			uint64_t vId;
			vId_str >> vId;
			m_mainworker.m_eventsystem.SetEventTrigger(vId, m_mainworker.m_eventsystem.REASON_USERVARIABLE, 0);
			m_mainworker.m_eventsystem.UpdateUserVariable(vId, "", "", -1, sd[1]);
		}
	}
	return "OK";
}

std::string CSQLHelper::UpdateUserVariable(const std::string &idx, const std::string &varname, const std::string &vartype, const std::string &varvalue, const bool eventtrigger)
{
	int typei = atoi(vartype.c_str());
	std::string formatError = CheckUserVariable(typei, varvalue);
	if (formatError != "OK")
		return formatError;

	/*
	std::vector<std::vector<std::string> > result;
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
	std::string szLastUpdate = TimeToString(NULL, TF_DateTime);
	std::string szVarValue = CURLEncode::URLDecode(varvalue.c_str());
	safe_query(
		"UPDATE UserVariables SET Name='%q', ValueType='%d', Value='%q', LastUpdate='%q' WHERE (ID == '%q')",
		varname.c_str(),
		typei,
		szVarValue.c_str(),
		szLastUpdate.c_str(),
		idx.c_str()
		);
	if (m_bEnableEventSystem)
	{
		std::stringstream vId_str(idx);
		uint64_t vId;
		vId_str >> vId;
		if (eventtrigger)
			m_mainworker.m_eventsystem.SetEventTrigger(vId, m_mainworker.m_eventsystem.REASON_USERVARIABLE, 0);
		m_mainworker.m_eventsystem.UpdateUserVariable(vId, varname, szVarValue, typei, szLastUpdate);
	}
	return "OK";
}

bool CSQLHelper::SetUserVariable(const uint64_t idx, const std::string &varvalue, const bool eventtrigger)
{
	std::string szLastUpdate = TimeToString(NULL, TF_DateTime);
	std::string szVarValue = CURLEncode::URLDecode(varvalue.c_str());
	safe_query(
		"UPDATE UserVariables SET Value='%q', LastUpdate='%q' WHERE (ID == %" PRIu64 ")",
		szVarValue.c_str(),
		szLastUpdate.c_str(),
		idx
		);
	if (m_bEnableEventSystem)
	{
		if (eventtrigger)
			m_mainworker.m_eventsystem.SetEventTrigger(idx, m_mainworker.m_eventsystem.REASON_USERVARIABLE, 0);
		m_mainworker.m_eventsystem.UpdateUserVariable(idx, "", szVarValue, 0, szLastUpdate);
	}
	return true;
}

std::string CSQLHelper::CheckUserVariableName(const std::string &varname)
{
	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT Name FROM UserVariables WHERE (Name=='%q')",
		varname.c_str());
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
	return safe_query("SELECT ID,Name,ValueType,Value,LastUpdate FROM UserVariables");
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
		struct tm norm;
		localtime_r(&when, &norm);

		return (norm.tm_mday == d    &&
			norm.tm_mon == m - 1 &&
			norm.tm_year == y - 1900);
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
	m_iAcceptHardwareTimerCounter = iTotMinutes * 60.0f;
	if (m_bAcceptHardwareTimerActive == false)
	{
		m_bPreviousAcceptNewHardware = m_bAcceptNewHardware;
	}
	m_bAcceptNewHardware = true;
	m_bAcceptHardwareTimerActive = true;
	_log.Log(LOG_STATUS, "New sensors allowed for %d minutes...", iTotMinutes);
}

std::string CSQLHelper::GetDeviceValue(const char * FieldName , const char *Idx )
{
	TSqlQueryResult result = safe_query("SELECT %s from DeviceStatus WHERE (ID == %s )",FieldName, Idx );
  if (result.size()>0)
	  return  result[0][0];
  else
	  return  "";
}

void CSQLHelper::UpdateDeviceValue(const char * FieldName , std::string &Value , std::string &Idx )
{
	safe_query("UPDATE DeviceStatus SET %s='%s' , LastUpdate='%s' WHERE (ID == %s )", FieldName, Value.c_str(), TimeToString(NULL, TF_DateTime).c_str(), Idx.c_str());
}
void CSQLHelper::UpdateDeviceValue(const char * FieldName , int Value , std::string &Idx )
{
	safe_query("UPDATE DeviceStatus SET %s=%d , LastUpdate='%s' WHERE (ID == %s )", FieldName, Value, TimeToString(NULL, TF_DateTime).c_str(),Idx.c_str());
}
void CSQLHelper::UpdateDeviceValue(const char * FieldName , float Value , std::string &Idx )
{
	safe_query("UPDATE DeviceStatus SET %s=%4.2f , LastUpdate='%s' WHERE (ID == %s )", FieldName, Value, TimeToString(NULL, TF_DateTime).c_str(),Idx.c_str());
}

//return temperature value from Svalue : is code temperature;humidity;???
float CSQLHelper::getTemperatureFromSValue(const char * sValue)
{
	std::vector<std::string> splitresults;
	StringSplit(sValue, ";", splitresults);
	if (splitresults.size()<1)
      return 0;
    else
      return (float)atof(splitresults[0].c_str());
}
void LogRow (TSqlRowQuery * row)
{
		std::string Row;
		for (unsigned int j=0;j<(*row).size();j++)
			Row = Row+(*row)[j]+";";
    _log.Log(LOG_TRACE,"SQLR result: %s",Row.c_str());
}
void CSQLHelper::LogQueryResult (TSqlQueryResult &result)
{
	for (unsigned int i=0;i<result.size();i++)
	{
		LogRow( &result[i] );
	}
}
bool CSQLHelper::InsertCustomIconFromZip(const std::string &szZip, std::string &ErrorMessage)
{
	//write file to disk
#ifdef WIN32
	std::string outputfile = "custom_icons.zip";
#else
	std::string outputfile = "/tmp/custom_icons.zip";
#endif
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

	return InsertCustomIconFromZipFile(outputfile, ErrorMessage);
}

bool CSQLHelper::InsertCustomIconFromZipFile(const std::string &szZipFile, std::string &ErrorMessage)
{
	clx::basic_unzip<char> in(szZipFile);
	if (!in.is_open())
	{
		ErrorMessage = "Error opening zip file";
		return false;
	}

	int iTotalAdded = 0;

	for (clx::unzip::iterator pos = in.begin(); pos != in.end(); ++pos) {
		//_log.Log(LOG_STATUS, "unzip: %s", pos->path().c_str());
		std::string fpath = pos->path();

		//Skip strange folders
		if (fpath.find("__MACOSX") != std::string::npos)
			continue;

		int ipos = fpath.find("icons.txt");
		if (ipos != std::string::npos)
		{
			std::string rpath;
			if (ipos > 0)
				rpath = fpath.substr(0, ipos);

			uLong fsize;
			unsigned char *pFBuf = (unsigned char *)(pos).Extract(fsize, 1);
			if (pFBuf == NULL)
			{
				ErrorMessage = "Could not extract icons.txt";
				return false;
			}
			pFBuf[fsize] = 0; //null terminate

			std::string _defFile = std::string(pFBuf, pFBuf + fsize);
			free(pFBuf);

			std::vector<std::string> _Lines;
			StringSplit(_defFile, "\n", _Lines);
			std::vector<std::string>::const_iterator itt;

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
					std::vector<std::vector<std::string> > result;
					result = safe_query("SELECT ID FROM CustomImages WHERE Base='%q'", IconBase.c_str());
					bool bIsDuplicate = (result.size() > 0);
					int RowID = 0;
					if (bIsDuplicate)
					{
						RowID = atoi(result[0][0].c_str());
					}

					//Locate the files in the zip, if not present back out
					std::string IconFile16 = IconBase + ".png";
					std::string IconFile48On = IconBase + "48_On.png";
					std::string IconFile48Off = IconBase + "48_Off.png";

					std::map<std::string, std::string> _dbImageFiles;
					_dbImageFiles["IconSmall"] = IconFile16;
					_dbImageFiles["IconOn"] = IconFile48On;
					_dbImageFiles["IconOff"] = IconFile48Off;

					//Check if all icons are there
					std::map<std::string, std::string>::const_iterator iItt;
					for (iItt = _dbImageFiles.begin(); iItt != _dbImageFiles.end(); ++iItt)
					{
						//std::string TableField = iItt->first;
						std::string IconFile = rpath + iItt->second;
						if (in.find(IconFile) == in.end())
						{
							ErrorMessage = "Icon File: " + IconFile + " is not present";
							if (iTotalAdded > 0)
							{
								m_webservers.ReloadCustomSwitchIcons();
							}
							return false;
						}
					}

					//All good, now lets add it to the database
					if (!bIsDuplicate)
					{
						safe_query("INSERT INTO CustomImages (Base,Name, Description) VALUES ('%q', '%q', '%q')",
							IconBase.c_str(), IconName.c_str(), IconDesc.c_str());

						//Get our Database ROWID
						result = safe_query("SELECT ID FROM CustomImages WHERE Base='%q'", IconBase.c_str());
						if (result.size() == 0)
						{
							ErrorMessage = "Error adding new row to database!";
							if (iTotalAdded > 0)
							{
								m_webservers.ReloadCustomSwitchIcons();
							}
							return false;
						}
						RowID = atoi(result[0][0].c_str());
					}
					else
					{
						//Update
						safe_query("UPDATE CustomImages SET Name='%q', Description='%q' WHERE ID=%d",
							IconName.c_str(), IconDesc.c_str(), RowID);

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
						std::string IconFile = rpath + iItt->second;

						sqlite3_stmt *stmt = NULL;
						char *zQuery = sqlite3_mprintf("UPDATE CustomImages SET %s = ? WHERE ID=%d", TableField.c_str(), RowID);
						if (!zQuery)
						{
							_log.Log(LOG_ERROR, "SQL: Out of memory, or invalid printf!....");
							return false;
						}
						int rc = sqlite3_prepare_v2(m_dbase, zQuery, -1, &stmt, NULL);
						sqlite3_free(zQuery);
						if (rc != SQLITE_OK) {
							ErrorMessage = "Problem inserting icon into database! " + std::string(sqlite3_errmsg(m_dbase));
							if (iTotalAdded > 0)
							{
								m_webservers.ReloadCustomSwitchIcons();
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
								m_webservers.ReloadCustomSwitchIcons();
							}
							return false;
						}
						rc = sqlite3_bind_blob(stmt, 1, pFBuf, fsize, SQLITE_STATIC);
						if (rc != SQLITE_OK) {
							ErrorMessage = "Problem inserting icon into database! " + std::string(sqlite3_errmsg(m_dbase));
							free(pFBuf);
							if (iTotalAdded > 0)
							{
								m_webservers.ReloadCustomSwitchIcons();
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
									m_webservers.ReloadCustomSwitchIcons();
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

		}
	}

	if (iTotalAdded == 0)
	{
		//definition file not found
		ErrorMessage = "No Icon definition file not found";
		return false;
	}

	m_webservers.ReloadCustomSwitchIcons();
	return true;
}

std::map<std::string, std::string> CSQLHelper::BuildDeviceOptions(const std::string & options, const bool decode) {
	std::map<std::string, std::string> optionsMap;
	if (!options.empty()) {
		//_log.Log(LOG_STATUS, "DEBUG : Build device options from '%s'...", options.c_str());
		std::vector<std::string> optionsArray;
		StringSplit(options, ";", optionsArray);
		std::vector<std::string>::iterator itt;
		for (itt=optionsArray.begin(); itt!=optionsArray.end(); ++itt) {
			if (*itt == "") {
				continue;
			}
			std::vector<std::string> optionArray;
			StringSplit(*itt, ":", optionArray);
			if (optionArray.size() == 2) {
				std::string optionName = optionArray[0].c_str();
				std::string optionValue = decode ? base64_decode(optionArray[1].c_str()) : optionArray[1].c_str();
				//_log.Log(LOG_STATUS, "DEBUG : Build device option ['%s': '%s'] => ['%s': '%s']", optionArray[0].c_str(), optionArray[1].c_str(), optionName.c_str(), optionValue.c_str());
				optionsMap.insert(std::pair<std::string, std::string>(optionName, optionValue));
			}
		}
	}
	//_log.Log(LOG_STATUS, "DEBUG : Build %d device(s) option(s)", optionsMap.size());
	return optionsMap;
}

std::map<std::string, std::string> CSQLHelper::GetDeviceOptions(const std::string & idx) {
	std::map<std::string, std::string> optionsMap;

	if (idx.empty()) {
		_log.Log(LOG_ERROR, "Cannot set options on device %s", idx.c_str());
		return optionsMap;
	}

	uint64_t ulID;
	std::stringstream s_str(idx);
	s_str >> ulID;
	std::vector<std::vector<std::string> > result;
	result = safe_query("SELECT Options FROM DeviceStatus WHERE (ID==%" PRIu64 ")", ulID);
	if (result.size() > 0) {
		std::vector<std::string> sd = result[0];
		optionsMap = BuildDeviceOptions(sd[0].c_str());
	}
	return optionsMap;
}

bool CSQLHelper::SetDeviceOptions(const uint64_t idx, const std::map<std::string, std::string> & optionsMap) {
	if (idx < 1) {
		_log.Log(LOG_ERROR, "Cannot set options on device %" PRIu64 "", idx);
		return false;
	}

	std::string options;
	int count = optionsMap.size();
	if (count > 0) {
		int i = 0;
		std::stringstream ssoptions;
		std::map<std::string, std::string>::const_iterator itt;
		for (itt = optionsMap.begin(); itt != optionsMap.end(); ++itt)
		{
			i++;
			//_log.Log(LOG_STATUS, "DEBUG : Reading device option ['%s', '%s']", itt->first.c_str(), itt->second.c_str());
			std::string optionName = itt->first.c_str();
			std::string optionValue = base64_encode((const unsigned char*)itt->second.c_str(), itt->second.size());
			ssoptions << optionName << ":" << optionValue;
			if (i < count) {
				ssoptions << ";";
			}
		}
		options.assign(ssoptions.str());
	}
	if (options.empty() && (count > 0)) {
		_log.Log(LOG_ERROR, "Cannot parse options for device %" PRIu64 "", idx);
		return false;
	}
	if (options.empty()) {
		//_log.Log(LOG_STATUS, "DEBUG : removing options on device %" PRIu64 "", idx);
		safe_query("UPDATE DeviceStatus SET Options = null WHERE (ID==%" PRIu64 ")", idx);
	} else {
		//_log.Log(LOG_STATUS, "DEBUG : setting options '%s' on device %" PRIu64 "", options.c_str(), idx);
		safe_query("UPDATE DeviceStatus SET Options = '%q' WHERE (ID==%" PRIu64 ")", options.c_str(), idx);
	}
	return true;
}

