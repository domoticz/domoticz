#include "stdafx.h"
#include "SQLHelper.h"
#include <iostream>     /* standard I/O functions                         */
#include "RFXtrx.h"
#include "Helper.h"
#include "RFXNames.h"
#include "WindowsHelper.h"
#include "localtime_r.h"
#include "Logger.h"
#include "mainworker.h"
#include "../sqlite/sqlite3.h"
#include "../hardware/hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../smtpclient/SMTPClient.h"
#include "../webserver/Base64.h"
#include "mainstructs.h"

#define DB_VERSION 33

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
"[LastLevel] INTEGER DEFAULT 0, "
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
"[Date] DATE NOT NULL);";

const char *sqlCreateTempVars =
"CREATE TABLE IF NOT EXISTS [TempVars] ("
"[Key] VARCHAR(200) NOT NULL, "
"[nValue] INTEGER DEFAULT 0, "
"[sValue] VARCHAR(200));";

const char *sqlCreateTimers =
"CREATE TABLE IF NOT EXISTS [Timers] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] BOOLEAN DEFAULT true, "
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Time] TIME NOT NULL, "
"[Type] INTEGER NOT NULL, "
"[Cmd] INTEGER NOT NULL, "
"[Level] INTEGER DEFAULT 15, "
"[Hue] INTEGER DEFAULT 0, "
"[UseRandomness] INTEGER DEFAULT 0, "
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
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";


const char *sqlCreateNotifications =
"CREATE TABLE IF NOT EXISTS [Notifications] ("
"[ID] INTEGER PRIMARY KEY, "
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Params] VARCHAR(100), "
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
"[Mode5] CHAR DEFAULT 0);";

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
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateMeter_Calendar =
"CREATE TABLE IF NOT EXISTS [Meter_Calendar] ("
"[DeviceRowID] BIGINT NOT NULL, "
"[Value] BIGINT NOT NULL, "
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
"[VideoURL] VARCHAR(200) DEFAULT (''), "
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
"[Order] INTEGER BIGINT(10) DEFAULT 0);";

const char *sqlCreateDevicesToPlanStatusTrigger =
	"CREATE TRIGGER IF NOT EXISTS deviceplantatusupdate AFTER INSERT ON DeviceToPlansMap\n"
	"BEGIN\n"
	"	UPDATE DeviceToPlansMap SET [Order] = (SELECT MAX([Order]) FROM DeviceToPlansMap)+1 WHERE DeviceToPlansMap.ID = NEW.ID;\n"
	"END;\n";

const char *sqlCreatePlans =
"CREATE TABLE IF NOT EXISTS [Plans] ("
"[ID] INTEGER PRIMARY KEY, "
"[Order] INTEGER BIGINT(10) default 0, "
"[Name] VARCHAR(200) NOT NULL);";

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
"[Hue] INTEGER DEFAULT 0);";

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
"[Time] TIME NOT NULL, "
"[Type] INTEGER NOT NULL, "
"[Cmd] INTEGER NOT NULL, "
"[Level] INTEGER DEFAULT 15, "
"[Hue] INTEGER DEFAULT 0, "
"[UseRandomness] INTEGER DEFAULT 0, "
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

const char *sqlCreateLoad =
"CREATE TABLE IF NOT EXISTS [Load] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Load] FLOAT NOT NULL, "
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateLoad_Calendar =
"CREATE TABLE IF NOT EXISTS [Load_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Load_Min] FLOAT NOT NULL, "
"[Load_Max] FLOAT NOT NULL, "
"[Load_Avg] FLOAT DEFAULT 0, "
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


extern std::string szStartupFolder;

CSQLHelper::CSQLHelper(void)
{
	m_pMain=NULL;
	m_LastSwitchID="";
	m_LastSwitchRowID=0;
	m_dbase=NULL;
	m_demo_dbase=NULL;
	m_stoprequested=false;
	m_sensortimeoutcounter=0;

	m_windunit=WINDUNIT_MS;
	m_tempunit=TEMPUNIT_C;
	SetUnitsAndScale();

	SetDatabaseName("domoticz.db");
}

CSQLHelper::~CSQLHelper(void)
{
	if (m_background_task_thread!=NULL)
	{
		assert(m_background_task_thread);
		m_stoprequested = true;
		m_background_task_thread->join();
	}
	if (m_dbase!=NULL)
	{
		sqlite3_close(m_dbase);
		m_dbase=NULL;
	}
	if (m_demo_dbase!=NULL)
	{
		sqlite3_close(m_demo_dbase);
		m_demo_dbase=NULL;
	}
}

void CSQLHelper::SetMainWorker(MainWorker *pWorker)
{
	m_pMain=pWorker;
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
	query(sqlCreateTempVars);
	query(sqlCreateTimers);
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
	query(sqlCreateLoad);
	query(sqlCreateLoad_Calendar);
	query(sqlCreateFan);
	query(sqlCreateFan_Calendar);

	int dbversion=0;
	GetPreferencesVar("DB_Version", dbversion);
	if ((!bNewInstall)&&(dbversion<DB_VERSION))
	{
		//upgrade
		if (dbversion<2)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [Order] INTEGER BIGINT(10) default 0");
			query(sqlCreateDeviceStatusTrigger);
			CheckAndUpdateDeviceOrder();
		}
		if (dbversion<3)
		{
			query("ALTER TABLE Hardware ADD COLUMN [Enabled] INTEGER default 1");
		}
		if (dbversion<4)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjValue] FLOAT default 0");
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjMulti] FLOAT default 1");
		}
		if (dbversion<5)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [Cmd] INTEGER default 1");
			query("ALTER TABLE SceneDevices ADD COLUMN [Level] INTEGER default 100");
		}
		if (dbversion<6)
		{
			query("ALTER TABLE Cameras ADD COLUMN [VideoURL] VARCHAR(100)");
			query("ALTER TABLE Cameras ADD COLUMN [ImageURL] VARCHAR(100)");
		}
		if (dbversion<7)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjValue2] FLOAT default 0");
			query("ALTER TABLE DeviceStatus ADD COLUMN [AddjMulti2] FLOAT default 1");
		}
		if (dbversion<8)
		{
			query("DROP TABLE IF EXISTS [Cameras]");
			query(sqlCreateCameras);
		}
		if (dbversion<9) {
			query("UPDATE Notifications SET Params = 'S' WHERE Params = ''");
		}
		if (dbversion<10)
		{
			//P1 Smart meter power change, need to delete all short logs from today
			char szDateStart[40];
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

			sprintf(szDateStart,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);

			char szTmp[200];

			std::vector<std::vector<std::string> > result;
			sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (Type=%d)",pTypeP1Power);
			result=query(szTmp);
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;
					std::string idx=sd[0];
					sprintf(szTmp,"DELETE FROM MultiMeter WHERE (DeviceRowID='%s') AND (Date>='%s')",idx.c_str(),szDateStart);
					query(szTmp);
				}
			}
		}
		if (dbversion<11)
		{
			std::stringstream szQuery;
			std::vector<std::vector<std::string> > result;

			szQuery << "SELECT ID, Username, Password FROM Cameras ORDER BY ID";
			result=m_pMain->m_sql.query(szQuery.str());
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;
					std::string camuser=base64_encode((const unsigned char*)sd[1].c_str(),sd[1].size());
					std::string campwd=base64_encode((const unsigned char*)sd[2].c_str(),sd[2].size());
					std::stringstream szQuery2;
					szQuery2 << "UPDATE Cameras SET Username='" << camuser << "', Password='" << campwd << "' WHERE (ID=='" << sd[0] << "')";
					m_pMain->m_sql.query(szQuery2.str());
				}
			}
		}
		if (dbversion<12)
		{
			std::vector<std::vector<std::string> > result;
			result=m_pMain->m_sql.query("SELECT t.RowID, u.RowID from MultiMeter_Calendar as t, MultiMeter_Calendar as u WHERE (t.[Date] == u.[Date]) AND (t.[DeviceRowID] == u.[DeviceRowID]) AND (t.[RowID] != u.[RowID])");
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					++itt;
					std::vector<std::string> sd=*itt;
					std::stringstream szQuery2;
					szQuery2 << "DELETE FROM MultiMeter_Calendar WHERE (RowID=='" << sd[0] << "')";
					m_pMain->m_sql.query(szQuery2.str());
				}
			}
			
		}
		if (dbversion<13)
		{
			DeleteHardware("1001");
		}
		if (dbversion<14)
		{
			query("ALTER TABLE Users ADD COLUMN [RemoteSharing] INTEGER default 0");
		}
		if (dbversion<15)
		{
			query("DROP TABLE IF EXISTS [HardwareSharing]");
			query("ALTER TABLE DeviceStatus ADD COLUMN [LastLevel] INTEGER default 0");
		}
		if (dbversion<16)
		{
            query("ALTER TABLE Events RENAME TO tmp_Events;");
            query("CREATE TABLE Events ([ID] INTEGER PRIMARY KEY, [Name] VARCHAR(200) NOT NULL, [XMLStatement] TEXT NOT NULL,[Conditions] TEXT, [Actions] TEXT);");
            query("INSERT INTO Events(Name, XMLStatement) SELECT Name, XMLStatement FROM tmp_Events;");
            query("DROP TABLE tmp_Events;");
     	}
		if (dbversion<17)
		{
            query("ALTER TABLE Events ADD COLUMN [Status] INTEGER default 0");
     	}
		if (dbversion<18)
		{
			query("ALTER TABLE Temperature ADD COLUMN [DewPoint] FLOAT default 0");
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [DewPoint] FLOAT default 0");
		}
		if (dbversion<19)
		{
			query("ALTER TABLE Scenes ADD COLUMN [SceneType] INTEGER default 0");
		}
        
		if (dbversion<20)
		{
			query("INSERT INTO EventMaster(Name, XMLStatement, Status) SELECT Name, XMLStatement, Status FROM Events;");
			query("INSERT INTO EventRules(EMID, Conditions, Actions, SequenceNo) SELECT EventMaster.ID, Events.Conditions, Events.Actions, 1 FROM Events INNER JOIN EventMaster ON EventMaster.Name = Events.Name;");
            query("DROP TABLE Events;");
		}
		if (dbversion<21)
		{
			//increase Video/Image URL for camera's
			//create a backup
			query("ALTER TABLE Cameras RENAME TO tmp_Cameras");
			//Create the new table
			query(sqlCreateCameras);
			//Copy values from tmp_Cameras back into our new table
			query(
				"INSERT INTO Cameras([ID],[Name],[Enabled],[Address],[Port],[Username],[Password],[VideoURL],[ImageURL])"
				"SELECT [ID],[Name],[Enabled],[Address],[Port],[Username],[Password],[VideoURL],[ImageURL]"
				"FROM tmp_Cameras");
			//Drop the tmp_Cameras table
			query("DROP TABLE tmp_Cameras");
		}
		if (dbversion<22)
		{
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [Order] INTEGER BIGINT(10) default 0");
			query(sqlCreateDevicesToPlanStatusTrigger);
		}
		if (dbversion<23)
		{
			query("ALTER TABLE Temperature_Calendar ADD COLUMN [Temp_Avg] FLOAT default 0");

			std::vector<std::vector<std::string> > result;
			result=query("SELECT RowID, (Temp_Max+Temp_Min)/2 FROM Temperature_Calendar");
			if (result.size()>0)
			{
				char szTmp[100];
				sqlite3_exec(m_dbase, "BEGIN TRANSACTION;", NULL, NULL, NULL);
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;
					sprintf(szTmp,"UPDATE Temperature_Calendar SET Temp_Avg=%.1f WHERE RowID='%s'",atof(sd[1].c_str()),sd[0].c_str());
					query(szTmp);
				}
				sqlite3_exec(m_dbase, "END TRANSACTION;", NULL, NULL, NULL);
			}
		}
		if (dbversion<24)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [Order] INTEGER BIGINT(10) default 0");
			query(sqlCreateSceneDeviceTrigger);
			CheckAndUpdateSceneDeviceOrder();
		}
		if (dbversion<25)
		{
			query("DROP TABLE IF EXISTS [Plans]");
			query(sqlCreatePlans);
			query(sqlCreatePlanOrderTrigger);
		}
		if (dbversion<26)
		{
			query("DROP TABLE IF EXISTS [DeviceToPlansMap]");
			query(sqlCreatePlanMappings);
			query(sqlCreateDevicesToPlanStatusTrigger);
		}
		if (dbversion<27)
		{
			query("ALTER TABLE DeviceStatus ADD COLUMN [CustomImage] INTEGER default 0");
		}
		if (dbversion<28)
		{
			query("ALTER TABLE Timers ADD COLUMN [UseRandomness] INTEGER default 0");
			query("ALTER TABLE SceneTimers ADD COLUMN [UseRandomness] INTEGER default 0");
			query("UPDATE Timers SET [Type]=2, [UseRandomness]=1 WHERE ([Type]=5)");
			query("UPDATE SceneTimers SET [Type]=2, [UseRandomness]=1 WHERE ([Type]=5)");
			//"[] INTEGER DEFAULT 0, "
		}
		if (dbversion<29)
		{
			query("ALTER TABLE Scenes ADD COLUMN [ListenCmd] INTEGER default 1");
		}
		if (dbversion<30)
		{
			query("ALTER TABLE DeviceToPlansMap ADD COLUMN [DevSceneType] INTEGER default 0");
		}
		if (dbversion<31)
		{
			query("ALTER TABLE Users ADD COLUMN [TabsEnabled] INTEGER default 255");
		}
		if (dbversion<32)
		{
			query("ALTER TABLE SceneDevices ADD COLUMN [Hue] INTEGER default 0");
			query("ALTER TABLE SceneTimers ADD COLUMN [Hue] INTEGER default 0");
			query("ALTER TABLE Timers ADD COLUMN [Hue] INTEGER default 0");
		}
		if (dbversion<33)
		{
			query("DROP TABLE IF EXISTS [Load]");
			query("DROP TABLE IF EXISTS [Load_Calendar]");
			query("DROP TABLE IF EXISTS [Fan]");
			query("DROP TABLE IF EXISTS [Fan_Calendar]");
			query(sqlCreateLoad);
			query(sqlCreateLoad_Calendar);
			query(sqlCreateFan);
			query(sqlCreateFan_Calendar);

			char szTmp[200];

			std::vector<std::vector<std::string> > result;
			result=query("SELECT ID FROM DeviceStatus WHERE (DeviceID LIKE 'WMI%')");
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt=result.begin(); itt!=result.end(); ++itt)
				{
					std::vector<std::string> sd=*itt;
					std::string idx=sd[0];
					sprintf(szTmp,"DELETE FROM Temperature WHERE (DeviceRowID='%s')",idx.c_str());
					query(szTmp);
					sprintf(szTmp,"DELETE FROM Temperature_Calendar WHERE (DeviceRowID='%s')",idx.c_str());
					query(szTmp);
				}
			}
			query("DELETE FROM DeviceStatus WHERE (DeviceID LIKE 'WMI%')");
		}
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
	if (!GetPreferencesVar("MeterDividerGas", nValue))
	{
		UpdatePreferencesVar("MeterDividerGas", 100);
	}
	if (!GetPreferencesVar("MeterDividerWater", nValue))
	{
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
		UpdatePreferencesVar("SensorTimeout", 3600);
	}
	if (!GetPreferencesVar("SensorTimeoutNotification", nValue))
	{
		UpdatePreferencesVar("SensorTimeoutNotification", 0); //default disabled
	}
	if (!GetPreferencesVar("SensorBatteryLowtNotification", nValue))
	{
		UpdatePreferencesVar("SensorBatteryLowtNotification", 0); //default disabled
	}
	
	if (!GetPreferencesVar("UseAutoUpdate", nValue))
	{
		UpdatePreferencesVar("UseAutoUpdate", 1);
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
        	result=m_pMain->m_sql.query(szQuery.str());
        	if (result.size()>0)
            {
                if(atoi(result[0][1].c_str()) != nValue)
                {
                    UpdateRFXCOMHardwareDetails(atoi(result[0][0].c_str()), nValue, 0, 0, 0, 0);
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
	if (!GetPreferencesVar("EnableTabScenes", nValue))
	{
		UpdatePreferencesVar("EnableTabScenes", 1);
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
	if (!GetPreferencesVar("SecStatus", nValue))
	{
		UpdatePreferencesVar("SecStatus", (int)SECSTATUS_DISARMED);
	}
	SetUnitsAndScale();

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
		UpdatePreferencesVar("RaspCamParams", "-w 800 -h 600 -t 0"); //width/height/time2wait
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

void CSQLHelper::Do_Work()
{
	std::vector<_tTaskItem> _items2do;

	while (!m_stoprequested)
	{
		//sleep 1 second
		sleep_seconds(1);

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
					case pTypeLighting4:
					case pTypeLighting5:
					case pTypeLighting6:
					case pTypeLimitlessLights:
						if (m_pMain)
							m_pMain->SwitchLight(itt->_idx,"Off",0,-1);
						break;
					case pTypeSecurity1:
						switch (itt->_subType)
						{
						case sTypeSecX10M:
							if (m_pMain)
								m_pMain->SwitchLight(itt->_idx,"No Motion",0,-1);
							break;
						default:
							//just update internally
							UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(),devname);
							break;
						}
						break;
					default:
						//unknown hardware type, sensor will only be updated internally
						UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str(),devname);
						break;
					}
				}
				else
				{
					if (m_pMain)
						m_pMain->SwitchLight(itt->_idx,"Off",0,-1);
				}
			}
			else if (itt->_ItemType == TITEM_EXECUTE_SCRIPT)
			{
				//start script
#ifdef WIN32
				ShellExecute(NULL,"open",itt->_ID.c_str(),itt->_sValue.c_str(),NULL,SW_SHOWNORMAL);
#else
				std::string lscript=itt->_ID + " " + itt->_sValue;
				system(lscript.c_str());
#endif
			}
			else if (itt->_ItemType == TITEM_EMAIL_CAMERA_SNAPSHOT)
			{
				m_pMain->m_cameras.EmailCameraSnapshot(itt->_ID,itt->_sValue);
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
			else if (itt->_ItemType == TITEM_SEND_EMAIL)
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
						GetPreferencesVar("EmailTo",nValue,EmailTo);
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
							_log.Log(LOG_NORM,"Notification send (Email)");
						else
							_log.Log(LOG_ERROR,"Notification failed (Email)");

					}
				}
			}
            else if (itt->_ItemType == TITEM_SWITCHCMD_EVENT)
            {
                if (m_pMain)
                    m_pMain->SwitchLight(itt->_idx,itt->_command.c_str(),itt->_level, itt->_Hue);
            }

            else if (itt->_ItemType == TITEM_SWITCHCMD_SCENE)
            {
                if (m_pMain)
                    m_pMain->SwitchScene(itt->_idx,itt->_command.c_str());
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
					if (value == 0)
						break;
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

unsigned long long CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, std::string &devname)
{
	return UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, "", devname);
}

unsigned long long CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue, std::string &devname)
{
	return UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, 0, sValue,devname);
}

unsigned long long CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname)
{
	if (!NeedToUpdateHardwareDevice(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue,devname))
		return -1;
	unsigned long long devRowID=UpdateValueInt(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue,devname);

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
	case pTypeBlinds:
		bIsLightSwitch=true;
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
					case pTypeBlinds:
						newnValue=blinds_sOpen;
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
			case pTypeBlinds:
				newnValue=blinds_sOpen;
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

unsigned long long CSQLHelper::UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname)
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
		devname="Unknown";
		sprintf(szTmp,
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue) "
			"VALUES (%d,'%s',%d,%d,%d,%d,%d,%d,'%s')",
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
	case pTypeLighting1:
	case pTypeLighting2:
	case pTypeLighting3:
	case pTypeLighting4:
	case pTypeLighting5:
	case pTypeLighting6:
	case pTypeLimitlessLights:
	case pTypeSecurity1:
	case pTypeBlinds:
	case pTypeChime:
		//Add Lighting log
		m_LastSwitchID=ID;
		m_LastSwitchRowID=ulID;
		sprintf(szTmp,
			"INSERT INTO LightingLog (DeviceRowID, nValue, sValue) "
			"VALUES (%llu, %d, '%s')",
			ulID,
			nValue,sValue);
		result=query(szTmp);

		std::string lstatus="";
		int llevel=0;
		bool bHaveDimmer=false;
		bool bHaveGroupCmd=false;
		int maxDimLevel=0;

		sprintf(szTmp,
			"SELECT Name,SwitchType,AddjValue FROM DeviceStatus WHERE (ID = %llu)",
			ulID);
		result = query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];
			std::string Name=sd[0];
			_eSwitchType switchtype=(_eSwitchType)atoi(sd[1].c_str());
			int AddjValue=(int)atof(sd[2].c_str());
			GetLightStatus(devType,subType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);

			bool bIsLightSwitchOn=IsLightSwitchOn(lstatus);

			if ((bIsLightSwitchOn)&&(llevel!=0))
			{
				//update level for device
				sprintf(szTmp,
					"UPDATE DeviceStatus SET LastLevel='%d' WHERE (ID = %llu)",
					llevel,
					ulID);
				query(szTmp);

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
				std::string nszStartupFolder=szStartupFolder;
				if (nszStartupFolder=="")
					nszStartupFolder=".";
				s_scriptparams << nszStartupFolder << " " << HardwareID << " " << ulID << " " << (bIsLightSwitchOn?"On":"Off") << " \"" << lstatus << "\"" << " \"" << devname << "\"";
				//add script to background worker				
				boost::lock_guard<boost::mutex> l(m_background_task_mutex);
				m_background_task_queue.push_back(_tTaskItem::ExecuteScript(1,scriptname,s_scriptparams.str()));
			}

			//Check for notifications
			if (bIsLightSwitchOn)
				CheckAndHandleSwitchNotification(HardwareID,ID,unit,devType,subType,NTYPE_SWITCH_ON);
			else
				CheckAndHandleSwitchNotification(HardwareID,ID,unit,devType,subType,NTYPE_SWITCH_OFF);
			if (bIsLightSwitchOn)
			{
				if (AddjValue!=0)
				{
					bool bAdd2DelayQueue=false;
					int cmd=0;
					if (
						(switchtype==STYPE_OnOff)||
						(switchtype==STYPE_Motion)||
						(switchtype==STYPE_PushOn)||
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
						case pTypeLighting5:
							cmd=light5_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLighting6:
							cmd=light6_sOff;
							bAdd2DelayQueue=true;
							break;
						case pTypeLimitlessLights:
							cmd=Limitless_LedOff;
							bAdd2DelayQueue=true;
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
								(itt->_HardwareID==HardwareID)
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
	m_pMain->m_eventsystem.ProcessDevice(HardwareID,ulID,unit,devType,subType,signallevel,batterylevel,nValue,sValue,devname);
	return ulID;
}

//Special case for Z-Wave, as we receive the same update as we send it
bool CSQLHelper::NeedToUpdateHardwareDevice(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, std::string &devname)
{
	if (devType!=pTypeLighting2)
		return true;

	CDomoticzHardwareBase *pHardware=m_pMain->GetHardware(HardwareID);
	if (!pHardware)
		return true;
	if (pHardware->HwdType!=HTYPE_RazberryZWave)
		return true;

	//Check if nValue and sValue are the same, if true, then just update the date
	std::vector<std::vector<std::string> > result;
	char szTmp[400];
	sprintf(szTmp,"SELECT ID, nValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()<1)
		return true; //new device
	std::string idx=result[0][0];
	int oldnValue=atoi(result[0][1].c_str());
	bool bIsSame=(oldnValue==nValue);
	if (!bIsSame)
		return true;

	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now,&ltime);

	unsigned long long ulID;
	std::stringstream s_str( idx );
	s_str >> ulID;

	sprintf(szTmp,
		"UPDATE DeviceStatus SET sValue='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID = %llu)",
		sValue,
		ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		ulID);
	result = query(szTmp);

	return false; 
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
		AddjValue=(float)atof(result[0][0].c_str());
		AddjMulti=(float)atof(result[0][1].c_str());
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
		AddjValue=(float)atof(result[0][0].c_str());
		AddjMulti=(float)atof(result[0][1].c_str());
	}
}

bool CSQLHelper::SendNotification(const std::string &EventID, const std::string &Message)
{
	int nValue;
	std::string sValue;
	std::string sResult;

#if defined WIN32
	//Make a system tray message
	ShowSystemTrayNotification(Message.c_str());
#endif

	//check if prowl enabled
	if (GetPreferencesVar("ProwlAPI",nValue,sValue))
	{
		if (sValue!="")
		{
			//send message to Prowl
			std::stringstream sUrl;
			sUrl << "http://api.prowlapp.com/publicapi/add?apikey=" << sValue << "&application=Domoticz&event=" << Message << "&description=" << Message << "&priority=0";
			if (!HTTPClient::GET(sUrl.str(),sResult))
			{
				_log.Log(LOG_ERROR,"Error sending Prowl Notification!");
			}
			else
			{
				_log.Log(LOG_NORM,"Notification send (Prowl)");
			}
		}
	}
	//check if NMA enabled
	if (GetPreferencesVar("NMAAPI",nValue,sValue))
	{
		if (sValue!="")
		{
			//send message to Prowl
			std::stringstream sUrl;
			sUrl << "http://www.notifymyandroid.com/publicapi/notify?apikey=" << sValue << "&application=Domoticz&event=" << Message << "&priority=0&description=" << Message;
			if (!HTTPClient::GET(sUrl.str(),sResult))
			{
				_log.Log(LOG_ERROR,"Error sending NMA Notification!");
			}
			else
			{
				_log.Log(LOG_NORM,"Notification send (NMA)");
			}
		}
	}
	//check if Email enabled
	if (GetPreferencesVar("UseEmailInNotifications", nValue))
	{
		if (nValue==1)
		{
			if (GetPreferencesVar("EmailServer",nValue,sValue))
			{
				if (sValue!="")
				{
					std::string szBody;
					szBody=
						"<html>\n"
						"<body>\n"
						"<b>" + CURLEncode::URLDecode(Message) + "</b>\n"
						"</body>\n"
						"</html>\n";

					AddTaskItem(_tTaskItem::SendEmail(1,CURLEncode::URLDecode(Message),szBody));
				}
			}
		}
	}
	return true;
}

bool CSQLHelper::SendNotificationEx(const std::string &Subject, const std::string &Body)
{
	int nValue;
	std::string sValue;
	char szURL[300];
	std::string sResult;

#if defined WIN32
	//Make a system tray message
	ShowSystemTrayNotification(Subject.c_str());
#endif

	std::string notimessage=Body;
	notimessage=stdreplace(notimessage,"<br>"," ");

	CURLEncode uencode;
	//check if prowl enabled
	if (GetPreferencesVar("ProwlAPI",nValue,sValue))
	{
		if (sValue!="")
		{
			//send message to Prowl
			sValue=stdstring_trim(sValue);
			sprintf(szURL,"http://api.prowlapp.com/publicapi/add?apikey=%s&application=Domoticz&event=%s&description=%s&priority=0",
				sValue.c_str(),uencode.URLEncode(Subject).c_str(),uencode.URLEncode(notimessage).c_str());
			if (!HTTPClient::GET(szURL,sResult))
			{
				_log.Log(LOG_ERROR,"Error sending Prowl Notification!");
			}
			else
			{
				_log.Log(LOG_NORM,"Notification send (Prowl)");
			}
		}
	}
	//check if NMA enabled
	if (GetPreferencesVar("NMAAPI",nValue,sValue))
	{
		if (sValue!="")
		{
			//send message to Prowl
			sValue=stdstring_trim(sValue);
			sprintf(szURL,"http://www.notifymyandroid.com/publicapi/notify?apikey=%s&application=Domoticz&event=%s&priority=0&description=%s",
				sValue.c_str(),uencode.URLEncode(Subject).c_str(),uencode.URLEncode(notimessage).c_str());
			if (!HTTPClient::GET(szURL,sResult))
			{
				_log.Log(LOG_ERROR,"Error sending NMA Notification!");
			}
			else
			{
				_log.Log(LOG_NORM,"Notification send (NMA)");
			}
		}
	}
	//check if Email enabled
	if (GetPreferencesVar("UseEmailInNotifications", nValue))
	{
		if (nValue==1)
		{
			if (GetPreferencesVar("EmailServer",nValue,sValue))
			{
				if (sValue!="")
				{
					std::string szBody;
					szBody=
						"<html>\n"
						"<body>\n"
						"<b>" + CURLEncode::URLDecode(Body) + "</b>\n"
						"</body>\n"
						"</html>\n";

					AddTaskItem(_tTaskItem::SendEmail(1,CURLEncode::URLDecode(Subject),szBody));
				}
			}
		}
	}
	return true;
}

void CSQLHelper::UpdatePreferencesVar(const char *Key, const char* sValue)
{
	UpdatePreferencesVar(Key, 0, sValue);
}

void CSQLHelper::UpdatePreferencesVar(const char *Key, const int nValue)
{
	UpdatePreferencesVar(Key, nValue, "");
}

void CSQLHelper::UpdatePreferencesVar(const char *Key, const int nValue, const char* sValue)
{
	if (!m_dbase)
		return;

	char szTmp[600];

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ROWID FROM Preferences WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()==0)
	{
		//Insert
		sprintf(szTmp,
			"INSERT INTO Preferences (Key, nValue, sValue) "
			"VALUES ('%s',%d,'%s')",
			Key,
			nValue,sValue);
		result=query(szTmp);
	}
	else
	{
		//Update
		std::stringstream s_str( result[0][0] );
		s_str >> ID;

		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now,&ltime);

		sprintf(szTmp,
			"UPDATE Preferences SET Key='%s', nValue=%d, sValue='%s' "
			"WHERE (ROWID = %llu)",
			Key,
			nValue,sValue,
			ID);
		result = query(szTmp);
	}
}

bool CSQLHelper::GetPreferencesVar(const char *Key, std::string &sValue)
{
	if (!m_dbase)
		return false;

	char szTmp[200];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT sValue FROM Preferences WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()<1)
		return false;
	std::vector<std::string> sd=result[0];
	sValue=sd[0];
	return true;
}

bool CSQLHelper::GetPreferencesVar(const char *Key, int &nValue, std::string &sValue)
{
	if (!m_dbase)
		return false;

	char szTmp[200];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT nValue, sValue FROM Preferences WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()<1)
		return false;
	std::vector<std::string> sd=result[0];
	nValue=atoi(sd[0].c_str());
	sValue=sd[1];
	return true;
}

bool CSQLHelper::GetPreferencesVar(const char *Key, int &nValue)
{
	std::string sValue;
	return GetPreferencesVar(Key, nValue, sValue);
}

void CSQLHelper::UpdateRFXCOMHardwareDetails(const int HardwareID, const int msg1, const int msg2, const int msg3, const int msg4, const int msg5)
{
	std::stringstream szQuery;
	szQuery << "UPDATE Hardware SET Mode1=" << msg1 << ", Mode2=" << msg2 << ", Mode3=" << msg3 <<", Mode4=" << msg4 <<", Mode5=" << msg5 << " WHERE (ID == " << HardwareID << ")";
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

	//check if not send 12 hours ago, and if applicable

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
			float svalue=(float)atof(splitresults[2].c_str());

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
				SendNotification("", m_urlencoder.URLEncode(msg));
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

	//check if not send 12 hours ago, and if applicable

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
				SendNotification("", m_urlencoder.URLEncode(msg));
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

	//check if not send 12 hours ago, and if applicable

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
			float svalue=(float)atof(splitresults[2].c_str());

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
				SendNotification("", m_urlencoder.URLEncode(msg));
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

	//check if not send 12 hours ago, and if applicable

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
			float svalue=(float)atof(splitresults[2].c_str());

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
				SendNotification("", m_urlencoder.URLEncode(msg));
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
				SendNotification("", m_urlencoder.URLEncode(msg));
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

			float total_min=(float)atof(sd[0].c_str());
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

bool CSQLHelper::AddNotification(const std::string &DevIdx, const std::string &Param)
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
	szQuery << "INSERT INTO Notifications (DeviceRowID, Params) VALUES (" << DevIdx << ",'" << Param << "')";
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
	szQuery << "SELECT ID, Params, LastSend FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
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

		std::string stime=sd[2];
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
	szQuery.clear();
	szQuery.str("");
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
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT COUNT(*) FROM Timers WHERE (DeviceRowID==" << Idx << ")";
	result=query(szQuery.str());
	if (result.size()==0)
		return false;
	std::vector<std::string> sd=result[0];
	int totaltimers=atoi(sd[0].c_str());
	return (totaltimers>0);
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
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT COUNT(*) FROM SceneTimers WHERE (SceneRowID==" << Idx << ")";
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

void CSQLHelper::UpdateTempVar(const char *Key, const char* sValue)
{
	if (!m_dbase)
		return;

	char szTmp[600];

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ROWID FROM TempVars WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()==0)
	{
		//Insert
		sprintf(szTmp,
			"INSERT INTO TempVars (Key, sValue) VALUES ('%s','%s')",
			Key,sValue);
		result=query(szTmp);
	}
	else
	{
		//Update
		std::stringstream s_str( result[0][0] );
		s_str >> ID;
		sprintf(szTmp,"UPDATE TempVars SET sValue='%s' WHERE (ROWID = %llu)",sValue,ID);
		result = query(szTmp);
	}
}

bool CSQLHelper::GetTempVar(const char *Key, int &nValue, std::string &sValue)
{
	if (!m_dbase)
		return false;

	char szTmp[100];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT nValue,sValue FROM TempVars WHERE (Key='%s')",Key);
	result=query(szTmp);
	if (result.size()<1)
		return false;
	std::vector<std::string> sd=result[0];
	nValue=atoi(sd[0].c_str());
	sValue=sd[1];
	return true;
}

void CSQLHelper::Schedule5Minute()
{
	if (!m_dbase)
		return;

	UpdateTemperatureLog();
	UpdateRainLog();
	UpdateWindLog();
	UpdateUVLog();
	UpdateMeter();
	UpdateMultiMeter();
	UpdateLoadLog();
	UpdateFanLog();
	//Removing the line below could cause a very large database,
	//and slow(large) data transfer (specially when working remote!!)
	CleanupShortLog();
}

void CSQLHelper::ScheduleDay()
{
	if (!m_dbase)
		return;

	AddCalendarTemperature();
	AddCalendarUpdateRain();
	AddCalendarUpdateUV();
	AddCalendarUpdateWind();
	AddCalendarUpdateMeter();
	AddCalendarUpdateMultiMeter();
	AddCalendarUpdateLoad();
	AddCalendarUpdateFan();
	CleanupLightLog();
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
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR (Type=%d AND SubType=%d))",
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
		pTypeGeneral,
		sTypeSystemTemp
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

			switch (dType)
			{
			case pTypeRego6XXTemp:
			case pTypeTEMP:
				temp=(float)atof(splitresults[0].c_str());
				break;
			case pTypeThermostat1:
				temp=(float)atof(splitresults[0].c_str());
				break;
			case pTypeHUM:
				humidity=nValue;
				break;
			case pTypeTEMP_HUM:
				temp=(float)atof(splitresults[0].c_str());
				humidity=atoi(splitresults[1].c_str());
				dewpoint=(float)CalculateDewPoint(temp,humidity);
				break;
			case pTypeTEMP_HUM_BARO:
				temp=(float)atof(splitresults[0].c_str());
				humidity=atoi(splitresults[1].c_str());
				if (dSubType==sTypeTHBFloat)
					barometer=int(atof(splitresults[3].c_str())*10.0f);
				else
					barometer=atoi(splitresults[3].c_str());
				dewpoint=(float)CalculateDewPoint(temp,humidity);
				break;
			case pTypeTEMP_BARO:
				temp=(float)atof(splitresults[0].c_str());
				barometer=int(atof(splitresults[1].c_str())*10.0f);
				break;
			case pTypeUV:
				if (dSubType!=sTypeUV3)
					continue;
				temp=(float)atof(splitresults[1].c_str());
				break;
			case pTypeWIND:
				if ((dSubType!=sTypeWIND4)&&(dSubType!=sTypeWINDNoTemp))
					continue;
				temp=(float)atof(splitresults[4].c_str());
				chill=(float)atof(splitresults[5].c_str());
				break;
			case pTypeRFXSensor:
				if (dSubType!=sTypeRFXSensorTemp)
					continue;
				temp=(float)atof(splitresults[0].c_str());
				break;
			case pTypeGeneral:
				if (dSubType==sTypeSystemTemp)
					temp=(float)atof(splitresults[0].c_str());
				break;
			}
			//insert record
			sprintf(szTmp,
				"INSERT INTO Temperature (DeviceRowID, Temperature, Chill, Humidity, Barometer, DewPoint) "
				"VALUES (%llu, %.2f, %.2f, %d, %d, %.2f)",
				ID,
				temp,
				chill,
				humidity,
				barometer,
				dewpoint
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
			if (splitresults.size()<1)
				continue; //impossible

			float total=0;
			int rate=0;

			rate=atoi(splitresults[0].c_str());
			total=(float)atof(splitresults[1].c_str());

			//insert record
			sprintf(szTmp,
				"INSERT INTO Rain (DeviceRowID, Total, Rate) "
				"VALUES (%llu, %.2f, %d)",
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
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d)", pTypeWIND);
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

			float direction=(float)atof(splitresults[0].c_str());
			int speed=atoi(splitresults[2].c_str());
			int gust=atoi(splitresults[3].c_str());

			//insert record
			sprintf(szTmp,
				"INSERT INTO Wind (DeviceRowID, Direction, Speed, Gust) "
				"VALUES (%llu, %.2f, %d, %d)",
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

			float level=(float)atof(splitresults[0].c_str());

			//insert record
			sprintf(szTmp,
				"INSERT INTO UV (DeviceRowID, Level) "
				"VALUES (%llu, %.1f)",
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
		"(Type=%d AND SubType=%d)" //pTypeRFXSensor,sTypeRFXSensorVolt
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
		pTypeRFXSensor,sTypeRFXSensorVolt
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

			bool bSkipSameValue=true;

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
			}
			else if ((dType==pTypeENERGY)||(dType==pTypePOWER))
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size()<2)
					continue;
				double fValue=atof(splitresults[1].c_str())*100;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}
			else if (dType==pTypeAirQuality)
			{
				sprintf(szTmp,"%d",nValue);
				sValue=szTmp;
				CheckAndHandleNotification(hardwareID, DeviceID, Unit, dType, dSubType, NTYPE_USAGE, (float)nValue);
				bSkipSameValue=false;
			}
			else if ((dType==pTypeGeneral)&&((dSubType==sTypeSoilMoisture)||(dSubType==sTypeLeafWetness)))
			{
				sprintf(szTmp,"%d",nValue);
				sValue=szTmp;
				bSkipSameValue=false;
			}
			else if ((dType==pTypeGeneral)&&(dSubType==sTypeVisibility))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
				bSkipSameValue=false;
			}
			else if ((dType==pTypeGeneral)&&(dSubType==sTypeSolarRadiation))
			{
				double fValue=atof(sValue.c_str())*10.0f;
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
				bSkipSameValue=false;
			}
			else if (dType==pTypeLux)
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
				bSkipSameValue=false;
			}
			else if (dType==pTypeWEIGHT)
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%.1f",fValue);
				sValue=szTmp;
				bSkipSameValue=false;
			}
			else if (dType==pTypeRFXSensor)
			{
				double fValue=atof(sValue.c_str());
				sprintf(szTmp,"%d",int(fValue));
				sValue=szTmp;
				bSkipSameValue=false;
			}

			unsigned long long MeterValue;
			std::stringstream s_str2( sValue );
			s_str2 >> MeterValue;

			if (bSkipSameValue)
			{
				//if last value == actual value, then do not insert it
				sprintf(szTmp,"SELECT Value FROM Meter WHERE (DeviceRowID=%llu) AND (Date>='%s') ORDER BY ROWID DESC LIMIT 1",ID,szDateToday);
				result2=query(szTmp);
				if (result2.size()>0)
				{
					std::vector<std::string> sd2=result2[0];
					std::string sValueLast=sd2[0];
					if (sValueLast==sValue)
						continue; //skip same value
				}
			}

			//insert record
			sprintf(szTmp,
				"INSERT INTO Meter (DeviceRowID, Value) "
				"VALUES (%llu, %llu)",
				ID,
				MeterValue
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
				"VALUES (%llu, %llu, %llu, %llu, %llu, %llu, %llu)",
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

void CSQLHelper::UpdateLoadLog()
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
		pTypeGeneral,sTypeSystemLoad
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

			float load= (float)atof(sValue.c_str());

			//insert record
			sprintf(szTmp,
				"INSERT INTO Load (DeviceRowID, Load) "
				"VALUES (%llu, %.2f)",
				ID,
				load
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
		pTypeGeneral,sTypeSystemFan
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
				"VALUES (%llu, %d)",
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

		sprintf(szTmp,"SELECT MIN(Temperature), MAX(Temperature), AVG(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer), MIN(DewPoint) FROM Temperature WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float temp_min=(float)atof(sd[0].c_str());
			float temp_max=(float)atof(sd[1].c_str());
			float temp_avg=(float)atof(sd[2].c_str());
			float chill_min=(float)atof(sd[3].c_str());
			float chill_max=(float)atof(sd[4].c_str());
			int humidity=atoi(sd[5].c_str());
			int barometer=atoi(sd[6].c_str());
			float dewpoint=(float)atof(sd[7].c_str());
			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Temperature_Calendar (DeviceRowID, Temp_Min, Temp_Max, Temp_Avg, Chill_Min, Chill_Max, Humidity, Barometer, DewPoint, Date) "
				"VALUES (%llu, %.2f, %.2f, %.2f, %.2f, %.2f, %d, %d, %.2f, '%s')",
				ID,
				temp_min,
				temp_max,
				temp_avg,
				chill_min,
				chill_max,
				humidity,
				barometer,
				dewpoint,
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

			float total_min=(float)atof(sd[0].c_str());
			float total_max=(float)atof(sd[1].c_str());
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
					"VALUES (%llu, %.2f, %d, '%s')",
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
				(!((devType==pTypeGeneral)&&(subType==sTypeSolarRadiation)))&&
				(!((devType==pTypeGeneral)&&(subType==sTypeSoilMoisture)))&&
				(!((devType==pTypeGeneral)&&(subType==sTypeLeafWetness)))&&
				(devType!=pTypeLux)&&
				(devType!=pTypeWEIGHT)&&
				(devType!=pTypeUsage)
				)
			{
				double total_real=total_max-total_min;

				//insert into calendar table
				sprintf(szTmp,
					"INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) "
					"VALUES (%llu, %.2f, '%s')",
					ID,
					total_real,
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
				//AirQuality/Usage Meter/Moisture/RFXSensor insert into MultiMeter_Calendar table
				sprintf(szTmp,
					"INSERT INTO MultiMeter_Calendar (DeviceRowID, Value1,Value2,Value3,Value4,Value5,Value6, Date) "
					"VALUES (%llu, %.2f,%.2f,%.2f,%.2f,%.2f,%.2f, '%s')",
					ID,
					total_min,total_max,0.0f,0.0f,0.0f,0.0f,
					szDateStart
					);
				result=query(szTmp);
			}
			if (
				(devType!=pTypeAirQuality)&&
				(devType!=pTypeRFXSensor)&&
				((devType!=pTypeGeneral)&&(subType!=sTypeVisibility))&&
				((devType!=pTypeGeneral)&&(subType!=sTypeSolarRadiation))&&
				((devType!=pTypeGeneral)&&(subType!=sTypeSoilMoisture))&&
				((devType!=pTypeGeneral)&&(subType!=sTypeLeafWetness))&&
				(devType!=pTypeLux)&&
				(devType!=pTypeWEIGHT)
				)
			{
				//Insert the last (max) counter value into the meter table to get the "today" value correct.
				sprintf(szTmp,
					"INSERT INTO Meter (DeviceRowID, Value, Date) "
					"VALUES (%llu, %s, '%s')",
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
				"VALUES (%llu, %.2f, '%s')",
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

			if (devType==pTypeP1Power)
			{
				for (int ii=0; ii<6; ii++)
				{
					float total_min=(float)atof(sd[(ii*2)+0].c_str());
					float total_max=(float)atof(sd[(ii*2)+1].c_str());
					total_real[ii]=total_max-total_min;
				}
			}
			else
			{
				for (int ii=0; ii<6; ii++)
				{
					float fvalue=(float)atof(sd[ii].c_str());
					total_real[ii]=fvalue;
				}
			}

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO MultiMeter_Calendar (DeviceRowID, Value1, Value2, Value3, Value4, Value5, Value6, Date) "
				"VALUES (%llu, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, '%s')",
				ID,
				total_real[0],
				total_real[1],
				total_real[2],
				total_real[3],
				total_real[4],
				total_real[5],
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

			float Direction=(float)atof(sd[0].c_str());
			int speed_min=atoi(sd[1].c_str());
			int speed_max=atoi(sd[2].c_str());
			int gust_min=atoi(sd[3].c_str());
			int gust_max=atoi(sd[4].c_str());

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Wind_Calendar (DeviceRowID, Direction, Speed_Min, Speed_Max, Gust_Min, Gust_Max, Date) "
				"VALUES (%llu, %.2f, %d, %d, %d, %d, '%s')",
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

			float level=(float)atof(sd[0].c_str());

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO UV_Calendar (DeviceRowID, Level, Date) "
				"VALUES (%llu, %.2f, '%s')",
				ID,
				level,
				szDateStart
				);
			result=query(szTmp);
		}
	}
}

void CSQLHelper::AddCalendarUpdateLoad()
{
	char szTmp[600];

	//Get All load devices in the Load Table
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Load ORDER BY DeviceRowID");
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

		sprintf(szTmp,"SELECT MIN(Load), MAX(Load), AVG(Load) FROM Load WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float load_min=(float)atof(sd[0].c_str());
			float load_max=(float)atof(sd[1].c_str());
			float load_avg=(float)atof(sd[2].c_str());
			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Load_Calendar (DeviceRowID, Load_Min, Load_Max, Load_Avg, Date) "
				"VALUES (%llu, %.2f, %.2f, %.2f,'%s')",
				ID,
				load_min,
				load_max,
				load_avg,
				szDateStart
				);
			result=query(szTmp);

		}
	}
}


void CSQLHelper::AddCalendarUpdateFan()
{
	char szTmp[600];

	//Get All load devices in the Load Table
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
				"INSERT INTO Load_Calendar (DeviceRowID, Speed_Min, Speed_Max, Speed_Avg, Date) "
				"VALUES (%llu, %d, %d, %d,'%s')",
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
	sprintf(szTmp,"DELETE FROM UV WHERE (DeviceRowID == %s)",idx.c_str());
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
    m_pMain->m_eventsystem.RemoveSingleState(atoi(idx.c_str()));
    
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
	szQuery.clear();
	szQuery.str("");
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
	szQuery.clear();
	szQuery.str("");
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
	szQuery.clear();
	szQuery.str("");
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

		GetLightStatus(dType,dSubType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
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
	}
}

void CSQLHelper::AddTaskItem(const _tTaskItem &tItem)
{
	boost::lock_guard<boost::mutex> l(m_background_task_mutex);
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
		m_tempsign="° C";
		m_tempscale=1.0f;
	}
	if (m_tempunit==TEMPUNIT_F)
	{
		m_tempsign="° F";
		m_tempscale=1.0f; //*1.8 + 32
	}
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
		"SELECT ID,Name,LastUpdate FROM DeviceStatus WHERE (Used!=0 AND LastUpdate<='%04d-%02d-%02d %02d:%02d:%02d' AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d AND Type!=%d) ORDER BY Name",
		ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		pTypeLighting1,
		pTypeLighting2,
		pTypeLighting3,
		pTypeLighting4,
		pTypeLighting5,
		pTypeLighting6,
		pTypeLimitlessLights,
		pTypeSecurity1,
		pTypeBlinds,
		pTypeChime
		);
	result=query(szTmp);
	if (result.size()<1)
		return;

	unsigned long long ulID;
	std::vector<std::vector<std::string> >::const_iterator itt;

	//check if last timeout_notification is not send today and if true, send notification
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
			SendNotification("", m_urlencoder.URLEncode(szTmp));
			m_timeoutlastsend[ulID]=stoday.tm_mday;
		}
	}
}
