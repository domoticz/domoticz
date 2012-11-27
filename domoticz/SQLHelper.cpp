#include "stdafx.h"
#include "SQLHelper.h"
#include "sqlite3.h"
#include <iostream>     /* standard I/O functions                         */
#include "RFXtrx.h"
#include "Helper.h"
#include "RFXNames.h"
#include "mynetwork.h"

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
"[LastUpdate] DATETIME DEFAULT (datetime('now','localtime')));";

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
"[Rights] INTEGER DEFAULT 255);";

CSQLHelper::CSQLHelper(void)
{
	m_LastSwitchID="";
	m_LastSwitchRowID=0;
	//Open Database
	int rc = sqlite3_open("domoticz.db", &m_dbase);
	if (rc)
	{
		std::cerr << "Error opening SQLite3 database: " << sqlite3_errmsg(m_dbase) << std::endl << std::endl;
		sqlite3_close(m_dbase);
	}
	else
	{
		//create database (if not exists)
		query(sqlCreateDeviceStatus);
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
		query(sqlCreateNotifications);
		query(sqlCreateHardware);
		query(sqlCreateUsers);
	}
}

CSQLHelper::~CSQLHelper(void)
{
	if (m_dbase!=NULL)
	{
		sqlite3_close(m_dbase);
		m_dbase=NULL;
	}
}

std::vector<std::vector<std::string> > CSQLHelper::query(const std::string szQuery)
{
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
					{
						break;
					}
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
		std::cout << error << std::endl;
	return results; 
}

void CSQLHelper::UpdateValue(const int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue)
{
	UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, "");
}

void CSQLHelper::UpdateValue(const int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, const char* sValue)
{
	UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, 0, sValue);
}

void CSQLHelper::UpdateValue(const int HardwareID, const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue, const char* sValue)
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
			std::cerr << "Serious database error, getting ID from DeviceStatus!" << std::endl;
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
		struct tm *ltime=localtime(&now);

		sprintf(szTmp,
			"UPDATE DeviceStatus SET SignalLevel=%d, BatteryLevel=%d, nValue=%d, sValue='%s', LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' "
			"WHERE (ID = %llu)",
			signallevel,batterylevel,
			nValue,sValue,
			ltime->tm_year+1900,ltime->tm_mon+1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec,
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
		GetLightStatus(devType,subType,nValue,sValue,lstatus,llevel,bHaveDimmer,bHaveGroupCmd);
		if ((lstatus=="On")||(lstatus=="Group On")||(lstatus=="All On"))
		{
			std::string notifyparams;
			if (GetNotification(ulID,notifyparams))
			{
				sprintf(szTmp,
					"SELECT Name FROM DeviceStatus WHERE (ID = %llu)",
					ulID);
				result = query(szTmp);
				if (result.size()>0)
				{
					std::vector<std::string> sd=result[0];
					std::string msg=sd[0]+" pressed";
					SendNotification("", m_urlencoder.URLEncode(msg));

					TouchNotification(ulID);
				}
			}
		}
	}
}

void CSQLHelper::TouchNotification(unsigned long long ID)
{
	char szTmp[300];
	char szDate[100];
	time_t atime = time(NULL);
	struct tm* tm = localtime(&atime);
	sprintf(szDate,"%04d-%02d-%02d %02d:%02d:%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);

	//Set LastSend date
	sprintf(szTmp,
		"UPDATE Notifications SET LastSend='%s' WHERE (DeviceRowID = %llu)",szDate,ID);
	query(szTmp);
}

bool CSQLHelper::SendNotification(const std::string EventID, const std::string Message)
{
	int nValue;
	std::string sValue;
	char szURL[300];
	unsigned char *pData=NULL;
	unsigned long ulLength=0;
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
		}
	}
	return true;
}

void CSQLHelper::UpdatePreferencesVar(const char *Key, const char* sValue)
{
	UpdatePreferencesVar(Key, 0, sValue);
}

void CSQLHelper::UpdatePreferencesVar(const char *Key, int nValue)
{
	UpdatePreferencesVar(Key, nValue, "");
}

void CSQLHelper::UpdatePreferencesVar(const char *Key, int nValue, const char* sValue)
{
	if (!m_dbase)
		return;

	char szTmp[1000];

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID FROM Preferences WHERE (Key='%s')",Key);
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
		struct tm *ltime=localtime(&now);

		sprintf(szTmp,
			"UPDATE Preferences SET Key='%s', nValue=%d, sValue='%s' "
			"WHERE (ID = %llu)",
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

	char szTmp[1000];

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

bool CSQLHelper::AddEditNotification(const std::string Idx, const std::string Param)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID FROM Notifications WHERE (DeviceRowID==" << Idx << ")";
	result=query(szQuery.str());
	if (result.size()==0)
	{
		//Insert
		szQuery.clear();
		szQuery.str("");
		szQuery << "INSERT INTO Notifications (DeviceRowID, Params) VALUES (" << Idx << ",'" << Param << "')";
		result=query(szQuery.str());
	}
	else
	{
		//Update
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE Notifications SET Params='" << Param << "' WHERE (DeviceRowID==" << Idx << ")";
		result = query(szQuery.str());
	}
	return true;
}

bool CSQLHelper::RemoveNotification(const std::string Idx)
{
	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM Notifications WHERE (DeviceRowID==" << Idx << ")";
	result=query(szQuery.str());
	return true;
}

bool CSQLHelper::GetNotification(const unsigned long long Idx, std::string &Param)
{
	if (!m_dbase)
		return false;

	std::vector<std::vector<std::string> > result;

	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT Params FROM Notifications WHERE (DeviceRowID==" << Idx << ")";
	result=query(szQuery.str());
	if (result.size()==0)
		return false;
	std::vector<std::string> sd=result[0];
	Param=sd[0];
	return true;
}

bool CSQLHelper::GetNotification(const std::string Idx, std::string &Param)
{
	std::stringstream s_str( Idx );
	unsigned long long idxll;
	s_str >> idxll;
	return GetNotification(idxll, Param);
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

void CSQLHelper::UpdateTempVar(const char *Key, const char* sValue)
{
	if (!m_dbase)
		return;

	char szTmp[1000];

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID FROM TempVars WHERE (Key='%s')",Key);
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
		sprintf(szTmp,"UPDATE TempVars SET sValue='%s' WHERE (ID = %llu)",sValue,ID);
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
}

void CSQLHelper::ScheduleDay()
{
	if (!m_dbase)
		return;

	AddCalendarTemperature();
	AddCalendarUpdateRain();
	AddCalendarUpdateUV();
	AddCalendarUpdateWind();
}

void CSQLHelper::UpdateTemperatureLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm* tm1 = localtime(&now);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue FROM DeviceStatus WHERE (Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d OR Type=%d)",
		pTypeTEMP,
		pTypeHUM,
		pTypeTEMP_HUM,
		pTypeTEMP_HUM_BARO,
		pTypeUV,
		pTypeWIND,
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
				barometer=atoi(splitresults[3].c_str());
				break;
			case pTypeUV:
				if (dSubType!=sTypeUV3)
					continue;
				temp=(float)atof(splitresults[1].c_str());
				break;
			case pTypeWIND:
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
	//truncate the temperature table (remove items older then 24 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=tm1->tm_hour;
	ltime.tm_min=tm1->tm_min;
	ltime.tm_sec=tm1->tm_sec;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;
	//subtract one day
	ltime.tm_mday -= 1;
	time_t daybefore = mktime(&ltime);
	struct tm* tm2 = localtime(&daybefore);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday,tm2->tm_hour,tm2->tm_min);

	sprintf(szTmp,"DELETE FROM Temperature WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);

}

void CSQLHelper::UpdateRainLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm* tm1 = localtime(&now);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue FROM DeviceStatus WHERE (Type=%d)",
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
	//truncate the rain table (remove items older then 24 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=tm1->tm_hour;
	ltime.tm_min=tm1->tm_min;
	ltime.tm_sec=tm1->tm_sec;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;
	//subtract one day
	ltime.tm_mday -= 1;
	time_t daybefore = mktime(&ltime);
	struct tm* tm2 = localtime(&daybefore);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday,tm2->tm_hour,tm2->tm_min);

	sprintf(szTmp,"DELETE FROM Rain WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::UpdateWindLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm* tm1 = localtime(&now);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue FROM DeviceStatus WHERE (Type=%d)", pTypeWIND);
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
	//truncate the wind table (remove items older then 24 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=tm1->tm_hour;
	ltime.tm_min=tm1->tm_min;
	ltime.tm_sec=tm1->tm_sec;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;
	//subtract one day
	ltime.tm_mday -= 1;
	time_t daybefore = mktime(&ltime);
	struct tm* tm2 = localtime(&daybefore);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday,tm2->tm_hour,tm2->tm_min);

	sprintf(szTmp,"DELETE FROM Wind WHERE (Date<'%s')",
		szDateEnd
		);
	result=query(szTmp);
}

void CSQLHelper::UpdateUVLog()
{
	char szTmp[1000];
	time_t now = time(NULL);
	struct tm* tm1 = localtime(&now);

	unsigned long long ID=0;

	std::vector<std::vector<std::string> > result;
	sprintf(szTmp,"SELECT ID,Type,SubType,nValue,sValue FROM DeviceStatus WHERE (Type=%d)", pTypeUV);
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
	//truncate the uv table (remove items older then 24 hours)
	char szDateEnd[40];
	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=tm1->tm_hour;
	ltime.tm_min=tm1->tm_min;
	ltime.tm_sec=tm1->tm_sec;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;
	//subtract one day
	ltime.tm_mday -= 1;
	time_t daybefore = mktime(&ltime);
	struct tm* tm2 = localtime(&daybefore);
	sprintf(szDateEnd,"%04d-%02d-%02d %02d:%02d:00",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday,tm2->tm_hour,tm2->tm_min);

	sprintf(szTmp,"DELETE FROM UV WHERE (Date<'%s')",
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
	struct tm* tm1 = localtime(&now);

	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm* tm2 = localtime(&later);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);

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
	struct tm* tm1 = localtime(&now);

	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm* tm2 = localtime(&later);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);

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
	struct tm* tm1 = localtime(&now);

	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm* tm2 = localtime(&later);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);

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
	struct tm* tm1 = localtime(&now);

	struct tm ltime;
	ltime.tm_isdst=tm1->tm_isdst;
	ltime.tm_hour=0;
	ltime.tm_min=0;
	ltime.tm_sec=0;
	ltime.tm_year=tm1->tm_year;
	ltime.tm_mon=tm1->tm_mon;
	ltime.tm_mday=tm1->tm_mday;

	sprintf(szDateEnd,"%04d-%02d-%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday);

	//Subtract one day

	ltime.tm_mday -= 1;
	time_t later = mktime(&ltime);
	struct tm* tm2 = localtime(&later);
	sprintf(szDateStart,"%04d-%02d-%02d",tm2->tm_year+1900,tm2->tm_mon+1,tm2->tm_mday);

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
	//also delete all records in other tables

	sprintf(szTmp,"SELECT ID FROM DeviceStatus WHERE (HardwareID == %s)",idx.c_str());
	result=query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			sprintf(szTmp,"DELETE FROM LightingLog WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Notifications WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Rain WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Rain_Calendar WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Temperature WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Temperature_Calendar WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Timers WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM UV WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM UV_Calendar WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Wind WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
			sprintf(szTmp,"DELETE FROM Wind_Calendar WHERE (DeviceRowID == %s)",sd[0].c_str());
			query(szTmp);
		}
	}
	//and now delete all records in the DeviceStatus table itself
	sprintf(szTmp,"DELETE FROM DeviceStatus WHERE (HardwareID == %s)",idx.c_str());
	result=query(szTmp);
}
