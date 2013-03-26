#include "stdafx.h"
#include "SQLHelper.h"
#include <iostream>     /* standard I/O functions                         */
#include "RFXtrx.h"
#include "Helper.h"
#include "RFXNames.h"
#include "WindowsHelper.h"
#include "localtime_r.h"
#include "Logger.h"
#include "../sqlite/sqlite3.h"
#include "../hardware/hardwaretypes.h"
#include "../httpclient/mynetwork.h"

#define DB_VERSION 8

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
"[AddjMulti2] FLOAT DEFAULT 1);";

const char *sqlCreateDeviceStatusTrigger =
"CREATE TRIGGER IF NOT EXISTS devicestatusupdate AFTER INSERT ON DeviceStatus\n"
"BEGIN\n"
"	UPDATE DeviceStatus SET [Order] = (SELECT MAX([Order]) FROM DeviceStatus)+1 WHERE DeviceStatus.ID = NEW.ID;\n"
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
"[sValue] CHARCHAR(200));";

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
"[Date] DATETIME DEFAULT (datetime('now','localtime')));";

const char *sqlCreateTemperature_Calendar =
"CREATE TABLE IF NOT EXISTS [Temperature_Calendar] ("
"[DeviceRowID] BIGINT(10) NOT NULL, "
"[Temp_Min] FLOAT NOT NULL, "
"[Temp_Max] FLOAT NOT NULL, "
"[Chill_Min] FLOAT DEFAULT 0, "
"[Chill_Max] FLOAT, "
"[Humidity] INTEGER DEFAULT 0, "
"[Barometer] INTEGER DEFAULT 0, "
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
"[Speed] INTERGER NOT NULL, "
"[Gust] INTERGER NOT NULL, "
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

const char *sqlCreateHardwareSharing =
"CREATE TABLE IF NOT EXISTS [HardwareSharing] ("
"[ID] INTEGER PRIMARY KEY, "
"[HardwareID] INTEGER NOT NULL, "
"[Port] INTEGER NOT NULL, "
"[Username] VARCHAR(100), "
"[Password] VARCHAR(100), "
"[Rights] INTEGER DEFAULT 0);";

const char *sqlCreateUsers =
"CREATE TABLE IF NOT EXISTS [Users] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] INTEGER NOT NULL DEFAULT 0, "
"[Username] VARCHAR(200) NOT NULL, "
"[Password] VARCHAR(200) NOT NULL, "
"[Rights] INTEGER DEFAULT 255);";

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
"[VideoURL] VARCHAR(100) DEFAULT (''), "
"[ImageURL] VARCHAR(100) DEFAULT (''));";

const char *sqlCreatePlanMappings =
"CREATE TABLE IF NOT EXISTS [DeviceToPlansMap] ("
"[ID] INTEGER PRIMARY KEY, "
"[DeviceRowID] BIGINT NOT NULL, "
"[PlanID] BIGINT NOT NULL, "
"[HPos] FLOAT NOT NULL, "
"[VPos] FLOAT NOT NULL, "
"[Name] VARCHAR(200) NOT NULL);";

const char *sqlCreatePlans =
"CREATE TABLE IF NOT EXISTS [Plans] ("
"[ID] INTEGER PRIMARY KEY, "
"[PlanOrder] BIGINT NOT NULL, "
"[Name] VARCHAR(200) NOT NULL);";

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
"[LastUpdate] DATETIME DEFAULT (datetime('now','localtime')));\n";

const char *sqlCreateScenesTrigger =
"CREATE TRIGGER IF NOT EXISTS scenesupdate AFTER INSERT ON Scenes\n"
"BEGIN\n"
"	UPDATE Scenes SET [Order] = (SELECT MAX([Order]) FROM Scenes)+1 WHERE Scenes.ID = NEW.ID;\n"
"END;\n";

const char *sqlCreateSceneDevices =
"CREATE TABLE IF NOT EXISTS [SceneDevices] ("
"[ID] INTEGER PRIMARY KEY, "
"[SceneRowID] BIGINT NOT NULL, "
"[DeviceRowID] BIGINT NOT NULL, "
"[Cmd] INTEGER DEFAULT 1, "
"[Level] INTEGER DEFAULT 100);";

const char *sqlCreateSceneTimers =
"CREATE TABLE IF NOT EXISTS [SceneTimers] ("
"[ID] INTEGER PRIMARY KEY, "
"[Active] BOOLEAN DEFAULT true, "
"[SceneRowID] BIGINT(10) NOT NULL, "
"[Time] TIME NOT NULL, "
"[Type] INTEGER NOT NULL, "
"[Cmd] INTEGER NOT NULL, "
"[Level] INTEGER DEFAULT 15, "
"[Days] INTEGER NOT NULL);";

CSQLHelper::CSQLHelper(void)
{
	m_LastSwitchID="";
	m_LastSwitchRowID=0;
	m_dbase=NULL;
	m_stoprequested=false;
	SetDatabaseName("domoticz.db");
}

CSQLHelper::~CSQLHelper(void)
{
	if (m_device_status_thread!=NULL)
	{
		assert(m_device_status_thread);
		m_stoprequested = true;
		m_device_status_thread->join();
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
	//test, this could improve performance
	//rc=sqlite3_exec(m_dbase, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);
	//rc=sqlite3_exec(m_dbase, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);

	rc=sqlite3_exec(m_dbase, "PRAGMA journal_mode=DELETE", NULL, NULL, NULL);

	bool bNewInstall=false;
	std::vector<std::vector<std::string> > result=query("SELECT name FROM sqlite_master WHERE type='table' AND name='DeviceStatus'");
	bNewInstall=(result.size()==0);

	//create database (if not exists)
	query(sqlCreateDeviceStatus);
	query(sqlCreateDeviceStatusTrigger);
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
	query(sqlCreateHardwareSharing);
	query(sqlCreateUsers);
	query(sqlCreateLightSubDevices);
    query(sqlCreateCameras);
    query(sqlCreatePlanMappings);
    query(sqlCreatePlans);
	query(sqlCreateScenes);
	query(sqlCreateScenesTrigger);
	query(sqlCreateSceneDevices);
	query(sqlCreateSceneTimers);

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
			query("DELETE FROM Cameras");
			query(sqlCreateCameras);
		}
	}
	UpdatePreferencesVar("DB_Version",DB_VERSION);

	//Make sure we have some default preferences
	int nValue;
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
		nValue=1;
	}
	Set5MinuteHistoryDays(nValue);
	if (!GetPreferencesVar("SensorTimeout", nValue))
	{
		UpdatePreferencesVar("SensorTimeout", 60);
	}

	//Start background thread
	if (!StartThread())
		return false;

	return true;
}

void CSQLHelper::Set5MinuteHistoryDays(const int Days)
{
	if (Days<1)
		m_5MinuteHistoryDays=1;
	else
		m_5MinuteHistoryDays=Days;
	UpdatePreferencesVar("5MinuteHistoryDays", Days);
}

bool CSQLHelper::StartThread()
{
	m_device_status_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSQLHelper::Do_Work, this)));

	return (m_device_status_thread!=NULL);
}

void CSQLHelper::Do_Work()
{
	while (!m_stoprequested)
	{
		//sleep 1 second
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		if (m_device_status_queue.size()<1)
			continue;
		boost::lock_guard<boost::mutex> l(m_device_status_mutex);

		std::vector<_tDeviceStatus>::iterator itt=m_device_status_queue.begin();
		while (itt!=m_device_status_queue.end())
		{
			itt->_DelayTime--;
			if (itt->_DelayTime<=0)
			{
				UpdateValueInt(itt->_HardwareID, itt->_ID.c_str(), itt->_unit, itt->_devType, itt->_subType, itt->_signallevel, itt->_batterylevel, itt->_nValue, itt->_sValue.c_str());
				itt=m_device_status_queue.erase(itt);
			}
			else
				itt++;
		}
	}
}

void CSQLHelper::SetDatabaseName(const std::string DBName)
{
	m_dbase_name=DBName;
}

std::vector<std::vector<std::string> > CSQLHelper::query(const std::string szQuery)
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

void CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue)
{
	UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, "");
}

void CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const char* sValue)
{
	UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, 0, sValue);
}

void CSQLHelper::UpdateValue(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue)
{
	UpdateValueInt(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue);

	bool bIsLightSwitch=false;
	switch (devType)
	{
	case pTypeLighting1:
	case pTypeLighting2:
	case pTypeLighting3:
	case pTypeLighting4:
	case pTypeLighting5:
	case pTypeLighting6:
	case pTypeSecurity1:
		bIsLightSwitch=true;
		break;
	}
	if (!bIsLightSwitch)
		return;

	//Get the ID of this device
	std::vector<std::vector<std::string> > result,result2;
	std::vector<std::vector<std::string> >::const_iterator itt,itt2;
	char szTmp[300];

	sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return; //should never happen, because it was previously inserted if non-existent

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
					case pTypeSecurity1:
						newnValue=sStatusNormal;
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
			case pTypeSecurity1:
				newnValue=sStatusNormal;
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

void CSQLHelper::UpdateValueInt(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue)
{
	if (!m_dbase)
		return;

	char szTmp[1000];

	unsigned long long ulID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
	{
		//Insert
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
			return;
		}
		std::stringstream s_str( result[0][0] );
		s_str >> ulID;
	}
	else
	{
		//Update
		std::stringstream s_str( result[0][0] );
		s_str >> ulID;

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
	case pTypeLighting1:
	case pTypeLighting2:
	case pTypeLighting3:
	case pTypeLighting4:
	case pTypeLighting5:
	case pTypeLighting6:
	case pTypeSecurity1:
		//Add Lighting log
		m_LastSwitchID=ID;
		m_LastSwitchRowID=ulID;
		sprintf(szTmp,
			"INSERT INTO LightingLog (DeviceRowID, nValue, sValue) "
			"VALUES (%llu, %d, '%s')",
			ulID,
			nValue,sValue);
		result=query(szTmp);

		//Check for notifications
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
			if (IsLightSwitchOn(lstatus)==true)
			{
				std::vector<_tNotification> notifications=GetNotifications(ulID);
				if (notifications.size()>0)
				{
					std::string msg=Name+" pressed";
					SendNotification("", m_urlencoder.URLEncode(msg));

					TouchNotification(notifications[0].ID);
				}
				if (AddjValue!=0)
				{
					bool bAdd2DelayQueue=false;
					int cmd=0;
					if (switchtype==STYPE_Motion)
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
						}
					}
	/* Smoke detectors are manually reset!
					else if (
						(devType==pTypeSecurity1)&&
						(subType==sTypeKD101)
						)
					{
						cmd=sStatusPanicOff;
						bAdd2DelayQueue=true;
					}
	*/
					if (bAdd2DelayQueue==true)
					{
						boost::lock_guard<boost::mutex> l(m_device_status_mutex);
						m_device_status_queue.push_back(_tDeviceStatus(AddjValue,HardwareID,ID,unit,devType,subType,signallevel,batterylevel,cmd,sValue));
					}
				}
			}
		}//end of check for notifications
	}
}

void CSQLHelper::GetAddjustment(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti)
{
	AddjValue=0.0f;
	AddjMulti=1.0f;
	std::vector<std::vector<std::string> > result;
	char szTmp[1000];
	sprintf(szTmp,"SELECT AddjValue,AddjMulti FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()!=0)
	{
		AddjValue=(float)atof(result[0][0].c_str());
		AddjMulti=(float)atof(result[0][1].c_str());
	}
}

void CSQLHelper::GetAddjustment2(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, float &AddjValue, float &AddjMulti)
{
	AddjValue=0.0f;
	AddjMulti=1.0f;
	std::vector<std::vector<std::string> > result;
	char szTmp[1000];
	sprintf(szTmp,"SELECT AddjValue2,AddjMulti2 FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result=query(szTmp);
	if (result.size()!=0)
	{
		AddjValue=(float)atof(result[0][0].c_str());
		AddjMulti=(float)atof(result[0][1].c_str());
	}
}

bool CSQLHelper::SendNotification(const std::string EventID, const std::string Message)
{
	int nValue;
	std::string sValue;
	char szURL[300];
	unsigned char *pData=NULL;
	unsigned long ulLength=0;

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
			sprintf(szURL,"http://api.prowlapp.com/publicapi/add?apikey=%s&priority=0&application=Domoticz&event=%s&description=%s",
				sValue.c_str(),EventID.c_str(),Message.c_str());
			I_HTTPRequest * r = NewHTTPRequest( szURL );
			if (r!=NULL)
			{
				if (r->readDataInVecBuffer())
				{
					r->getBuffer(pData, ulLength);
				}
				r->dispose();
			}
			_log.Log(LOG_NORM,"Notification send (Prowl)");
		}
	}
	//check if NMA enabled
	if (GetPreferencesVar("NMAAPI",nValue,sValue))
	{
		if (sValue!="")
		{
			//send message to Prowl
			sprintf(szURL,"http://www.notifymyandroid.com/publicapi/notify?apikey=%s&priority=0&application=Domoticz&event=%s&description=%s",
				sValue.c_str(),EventID.c_str(),Message.c_str());
			I_HTTPRequest * r = NewHTTPRequest( szURL );
			if (r!=NULL)
			{
				if (r->readDataInVecBuffer())
				{
					r->getBuffer(pData, ulLength);
				}
				r->dispose();
			}
			_log.Log(LOG_NORM,"Notification send (NMA)");
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

	char szTmp[1000];

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
	const std::string ID, 
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

	char szTmp[1000];

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

	time_t atime=time(NULL);

	//check if not send 12 hours ago, and if applicable

	atime-=(12*3600);
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

bool CSQLHelper::CheckAndHandleAmpere123Notification(
	const int HardwareID, 
	const std::string ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const float Ampere1,
	const float Ampere2,
	const float Ampere3)
{
	if (!m_dbase)
		return false;

	char szTmp[1000];

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

	time_t atime=time(NULL);

	//check if not send 12 hours ago, and if applicable

	atime-=(12*3600);
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
	const std::string ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const _eNotificationTypes ntype, 
	const float mvalue)
{
	if (!m_dbase)
		return false;

	char szTmp[1000];

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

	time_t atime=time(NULL);

	//check if not send 12 hours ago, and if applicable

	atime-=(12*3600);
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

bool CSQLHelper::CheckAndHandleRainNotification(
	const int HardwareID, 
	const std::string ID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const _eNotificationTypes ntype, 
	const float mvalue)
{
	if (!m_dbase)
		return false;

	char szTmp[1000];

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,AddjValue,AddjMulti FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID.c_str(), unit, devType, subType);
	result=query(szTmp);
	if (result.size()==0)
		return false;
	std::string devidx=result[0][0];
	double AddjValue=atof(result[0][1].c_str());
	double AddjMulti=atof(result[0][2].c_str());

	char szDateEnd[40];

	time_t now = time(NULL);
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
	return false;
}

void CSQLHelper::TouchNotification(const unsigned long long ID)
{
	char szTmp[300];
	char szDate[100];
	time_t atime = time(NULL);
	struct tm ltime;
	localtime_r(&atime,&ltime);
	sprintf(szDate,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);

	//Set LastSend date
	sprintf(szTmp,
		"UPDATE Notifications SET LastSend='%s' WHERE (ID = %llu)",szDate,ID);
	query(szTmp);
}

bool CSQLHelper::AddNotification(const std::string DevIdx, const std::string Param)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "INSERT INTO Notifications (DeviceRowID, Params) VALUES (" << DevIdx << ",'" << Param << "')";
	result=query(szQuery.str());
	return true;
}

bool CSQLHelper::UpdateNotification(const std::string ID, const std::string Param)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	//Update
	szQuery.clear();
	szQuery.str("");
	szQuery << "UPDATE Notifications SET Params='" << Param << "' WHERE (ID==" << ID << ")";
	result = query(szQuery.str());
	return true;
}


bool CSQLHelper::RemoveDeviceNotifications(const std::string DevIdx)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM Notifications WHERE (DeviceRowID==" << DevIdx << ")";
	result=query(szQuery.str());
	return true;
}

bool CSQLHelper::RemoveNotification(const std::string ID)
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

	time_t mtime=time(NULL);
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

std::vector<_tNotification> CSQLHelper::GetNotifications(const std::string DevIdx)
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
bool CSQLHelper::HasNotifications(const std::string DevIdx)
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

bool CSQLHelper::HasTimers(const std::string Idx)
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

bool CSQLHelper::HasSceneTimers(const std::string Idx)
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

	char szTmp[1000];

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
	CleanupLightLog();
}

void CSQLHelper::UpdateTemperatureLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d)",
		pTypeTEMP,
		pTypeHUM,
		pTypeTEMP_HUM,
		pTypeTEMP_HUM_BARO,
		pTypeUV,
		pTypeWIND,
		pTypeThermostat1,
		pTypeRFXSensor
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
			unsigned char nValue=atoi(sd[3].c_str());
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

			switch (dType)
			{
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
				break;
			case pTypeTEMP_HUM_BARO:
				temp=(float)atof(splitresults[0].c_str());
				humidity=atoi(splitresults[1].c_str());
				if (dSubType==sTypeTHBFloat)
					barometer=int(atof(splitresults[3].c_str())*10.0f);
				else
					barometer=atoi(splitresults[3].c_str());
				break;
			case pTypeUV:
				if (dSubType!=sTypeUV3)
					continue;
				temp=(float)atof(splitresults[1].c_str());
				break;
			case pTypeWIND:
				if (dSubType!=sTypeWIND4)
					continue;
				temp=(float)atof(splitresults[4].c_str());
				chill=(float)atof(splitresults[5].c_str());
				break;
			case pTypeRFXSensor:
				if (dSubType!=sTypeRFXSensorTemp)
					continue;
				temp=(float)atof(splitresults[0].c_str());
				break;
			}
			//insert record
			sprintf(szTmp,
				"INSERT INTO Temperature (DeviceRowID, Temperature, Chill, Humidity, Barometer) "
				"VALUES (%llu, %.2f, %.2f, %d, %d)",
				ID,
				temp,
				chill,
				humidity,
				barometer
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);

		}
	}
	//truncate the temperature table (remove items older then 48 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract xx days
	ltime.tm_mday -= m_5MinuteHistoryDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);

	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	sprintf(szTmp,"DELETE FROM Temperature WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);

}

void CSQLHelper::UpdateRainLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
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
			unsigned char nValue=atoi(sd[3].c_str());
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
	//truncate the rain table (remove items older then 48 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract xx days
	ltime.tm_mday -= m_5MinuteHistoryDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	sprintf(szTmp,"DELETE FROM Rain WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::UpdateWindLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
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
			unsigned char nValue=atoi(sd[3].c_str());
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
	//truncate the wind table (remove items older then 48 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract xx days
	ltime.tm_mday -= m_5MinuteHistoryDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	sprintf(szTmp,"DELETE FROM Wind WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::UpdateUVLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
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
			unsigned char nValue=atoi(sd[3].c_str());
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
	//truncate the uv table (remove items older then 48 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract xx days
	ltime.tm_mday -= m_5MinuteHistoryDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	sprintf(szTmp,"DELETE FROM UV WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::UpdateMeter()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> > result2;

	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d OR Type=%d)",
		pTypeRFXMeter,
		pTypeP1Gas,
		pTypeYouLess,
		pTypeENERGY
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
			unsigned char nValue=atoi(sd[3].c_str());
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
			else if (dType==pTypeENERGY)
			{
				std::vector<std::string> splitresults;
				StringSplit(sValue, ";", splitresults);
				if (splitresults.size()<2)
					continue;
				double fValue=atof(splitresults[1].c_str())*100;
				sprintf(szTmp,"%.0f",fValue);
				sValue=szTmp;
			}

			unsigned long long MeterValue;
			std::stringstream s_str2( sValue );
			s_str2 >> MeterValue;

			//if last value == actual value, then do not insert it
			sprintf(szTmp,"SELECT Value FROM Meter WHERE (DeviceRowID=%llu) ORDER BY ROWID DESC LIMIT 1",ID);
			result2=query(szTmp);
			if (result2.size()>0)
			{
				std::vector<std::string> sd2=result2[0];
				std::string sValueLast=sd2[0];
				if (sValueLast==sValue)
					continue; //skip same value
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
	//truncate the Meter table (remove items older then 48 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract xx days
	ltime.tm_mday -= m_5MinuteHistoryDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	sprintf(szTmp,"DELETE FROM Meter WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::UpdateMultiMeter()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm tm1;
	localtime_r(&now,&tm1);

	int SensorTimeOut=60;
	GetPreferencesVar("SensorTimeout", SensorTimeOut);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue,LastUpdate FROM DeviceStatus WHERE (Type=%d OR Type=%d)",
		pTypeP1Power,
		pTypeCURRENT
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
			unsigned char nValue=atoi(sd[3].c_str());
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

				value1=powerusage1+powerusage2;
				value2=powerdeliv1+powerdeliv2;
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
			else
				continue;//don't know you (yet)

			//insert record
			sprintf(szTmp,
				"INSERT INTO MultiMeter (DeviceRowID, Value1, Value2, Value3, Value4) "
				"VALUES (%llu, %llu, %llu, %llu, %llu)",
				ID,
				value1,
				value2,
				value3,
				value4
				);
			std::vector<std::vector<std::string> > result2;
			result2=query(szTmp);
		}
	}
	//truncate the MultiMeter table (remove items older then 48 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1.tm_isdst;
	ltime.tm_hour=tm1.tm_hour;
	ltime.tm_min=tm1.tm_min;
	ltime.tm_sec=tm1.tm_sec;
	ltime.tm_year=tm1.tm_year;
	ltime.tm_mon=tm1.tm_mon;
	ltime.tm_mday=tm1.tm_mday;
	//subtract xx days
	ltime.tm_mday -= m_5MinuteHistoryDays;
	time_t daybefore = mktime(&ltime);
	struct tm tm2;
	localtime_r(&daybefore,&tm2);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2.tm_year+1900,tm2.tm_mon+1,tm2.tm_mday,tm2.tm_hour,tm2.tm_min);

	sprintf(szTmp,"DELETE FROM MultiMeter WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::AddCalendarTemperature()
{
	char szTmp[1000];

	//Get All temperature devices in the Temperature Table
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Temperature ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = time(NULL);
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

		sprintf(szTmp,"SELECT MIN(Temperature), MAX(Temperature), MIN(Chill), MAX(Chill), MAX(Humidity), MAX(Barometer) FROM Temperature WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
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
			float chill_min=(float)atof(sd[2].c_str());
			float chill_max=(float)atof(sd[3].c_str());
			int humidity=atoi(sd[4].c_str());
			int barometer=atoi(sd[5].c_str());

			//insert into calendar table
			sprintf(szTmp,
				"INSERT INTO Temperature_Calendar (DeviceRowID, Temp_Min, Temp_Max, Chill_Min, Chill_Max, Humidity, Barometer, Date) "
				"VALUES (%llu, %.2f, %.2f, %.2f, %.2f, %d, %d, '%s')",
				ID,
				temp_min,
				temp_max,
				chill_min,
				chill_max,
				humidity,
				barometer,
				szDateStart
				);
			result=query(szTmp);

		}
	}
}

void CSQLHelper::AddCalendarUpdateRain()
{
	char szTmp[1000];

	//Get All UV devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Rain ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = time(NULL);
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

		sprintf(szTmp,"SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float total_min=(float)atof(sd[0].c_str());
			float total_max=(float)atof(sd[1].c_str());
			int rate=atoi(sd[2].c_str());

			float total_real=total_max-total_min;

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

void CSQLHelper::AddCalendarUpdateMeter()
{
	char szTmp[1000];

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

	time_t now = time(NULL);
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

	std::vector<std::vector<std::string> > result,result2;

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


		sprintf(szTmp,"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%llu' AND Date>='%s' AND Date<'%s')",
			ID,
			szDateStart,
			szDateEnd
			);
		result=query(szTmp);
		if (result.size()>0)
		{
			std::vector<std::string> sd=result[0];

			float total_min=(float)atof(sd[0].c_str());
			float total_max=(float)atof(sd[1].c_str());

			float total_real=total_max-total_min;

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
			}
		}
		else
		{
			//no new meter result received in last hour
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
	char szTmp[1000];

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

	time_t now = time(NULL);
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
		}
	}
}

void CSQLHelper::AddCalendarUpdateWind()
{
	char szTmp[1000];

	//Get All Wind devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM Wind ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = time(NULL);
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
	char szTmp[1000];

	//Get All UV devices
	std::vector<std::vector<std::string> > resultdevices;
	strcpy(szTmp,"SELECT DISTINCT(DeviceRowID) FROM UV ORDER BY DeviceRowID");
	resultdevices=query(szTmp);
	if (resultdevices.size()<1)
		return; //nothing to do

	char szDateStart[40];
	char szDateEnd[40];

	time_t now = time(NULL);
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

void CSQLHelper::DeleteHardware(const std::string idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[1000];
	sprintf(szTmp,"DELETE FROM Hardware WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
	sprintf(szTmp,"DELETE FROM HardwareSharing WHERE (HardwareID == %s)",idx.c_str());
	query(szTmp);
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

void CSQLHelper::DeleteCamera(const std::string idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[1000];
	sprintf(szTmp,"DELETE FROM Cameras WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
}

void CSQLHelper::DeletePlan(const std::string idx)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[1000];
	sprintf(szTmp,"DELETE FROM Plans WHERE (ID == %s)",idx.c_str());
	result=query(szTmp);
}

void CSQLHelper::DeleteDevice(const std::string idx)
{
	char szTmp[1000];
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
	//and now delete all records in the DeviceStatus table itself
	sprintf(szTmp,"DELETE FROM DeviceStatus WHERE (ID == %s)",idx.c_str());
	query(szTmp);
}

void CSQLHelper::TransferDevice(const std::string idx, const std::string newidx)
{
	char szTmp[1000];
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

void CSQLHelper::CleanupLightLog()
{
	//cleanup the lighting log
	int nMaxDays=30;
	GetPreferencesVar("LightHistoryDays", nMaxDays);

	char szDateEnd[40];
	time_t now = time(NULL);
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

bool CSQLHelper::DoesSceneByNameExits(const std::string SceneName)
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

