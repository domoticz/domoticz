#include "stdafx.h"
#include "mainworker.h"
#include "RFXNames.h"
#include "EventSystem.h"
#include "dzVents.h"
#include "Helper.h"
#include "HTMLSanitizer.h"
#include "SQLHelper.h"
#include "Logger.h"
#include "../hardware/hardwaretypes.h"
#include "../hardware/Kodi.h"
#include "../hardware/LogitechMediaServer.h"
#include "../hardware/MySensorsBase.h"
#include <iostream>
#include "../httpclient/UrlEncode.h"
#include "localtime_r.h"
#include "SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "WebServer.h"
#include "../main/WebServerHelper.h"
#include "../webserver/cWebem.h"
#include "../main/json_helper.h"
#include "../main/NotificationSystem.h"
#include "../main/LuaTable.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

bool g_bUseEventTrigger = true;

extern time_t m_StartTime;
extern std::string szUserDataFolder, szStartupFolder;
extern http::server::CWebServerHelper m_webservers;

#ifdef ENABLE_PYTHON
#include "../hardware/plugins/Plugins.h"
#include "../hardware/plugins/PluginMessages.h"
#include "EventsPythonModule.h"
#include "EventsPythonDevice.h"
extern PyObject * PDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
#endif

// Helper table for Blockly and SQL name mapping
const std::string CEventSystem::m_szReason[] =
{
	"device",			// 0
	"scenegroup",			// 1
	"uservariable",			// 2
	"time",				// 3
	"security",			// 4
	"url",				// 5
	"notification"			// 6
};

// Security status
const std::string CEventSystem::m_szSecStatus[] =
{
	"Disarmed",		// 0
	"Armed Home",	// 1
	"Armed Away"	// 2
};

// This table specifies which JSON fields are passed to the LUA scripts.
// If new return fields are added in CWebServer::GetJSonDevices, they should
// be added to this table.
const CEventSystem::_tJsonMap CEventSystem::JsonMap[] = {
	{ "Barometer", "barometer", JTYPE_FLOAT },
	{ "CameraIndx", "cameraIdx", JTYPE_STRING },
	{ "Chill", "chill", JTYPE_FLOAT },
	{ "Color", "color", JTYPE_STRING },
	{ "Counter", "counter", JTYPE_STRING },
	{ "CounterDeliv", "counterDelivered", JTYPE_FLOAT },
	{ "CounterDelivToday", "counterDeliveredToday", JTYPE_STRING },
	{ "CounterToday", "counterToday", JTYPE_STRING },
	{ "Current", "current", JTYPE_FLOAT },
	{ "CustomImage", "customImage", JTYPE_INT },
	{ "DewPoint", "dewPoint", JTYPE_FLOAT },
	{ "Direction", "direction", JTYPE_FLOAT },
	{ "DirectionStr", "directionString", JTYPE_STRING },
	{ "Forecast", "forecast", JTYPE_INT },
	{ "ForecastStr", "forecastString", JTYPE_STRING },
	{ "HardwareName", "hardwareName", JTYPE_STRING },
	{ "HardwareType", "hardwareType", JTYPE_STRING },
	{ "HardwareTypeVal", "hardwareTypeValue", JTYPE_INT },
	{ "Humidity", "humidity", JTYPE_INT },
	{ "HumidityStatus", "humidityStatus", JTYPE_STRING },
	{ "Image", "Image", JTYPE_STRING },
	{ "InternalState", "internalState", JTYPE_STRING }, // door contact
	{ "LevelActions", "levelActions", JTYPE_STRING },
	{ "LevelInt", "levelVal", JTYPE_INT },
	{ "LevelNames", "levelNames", JTYPE_STRING },
	{ "LevelOffHidden", "levelOffHidden", JTYPE_BOOL },
	{ "MaxDimLevel", "maxDimLevel", JTYPE_INT },
	{ "Mode", "mode", JTYPE_INT }, // zwave thermostat
	{ "Modes", "modes", JTYPE_STRING },
	{ "Moisture", "moisture", JTYPE_STRING },
	{ "Pressure", "pressure", JTYPE_FLOAT },
	{ "Protected", "protected", JTYPE_BOOL },
	{ "Quality", "quality", JTYPE_STRING },
	{ "Radiation", "radiation", JTYPE_FLOAT },
	{ "Rain", "rain", JTYPE_FLOAT },
	{ "RainRate", "rainRate", JTYPE_FLOAT },
	{ "SensorType", "sensorType", JTYPE_INT },
	{ "SensorUnit", "sensorUnit", JTYPE_STRING },
	{ "SetPoint", "setPoint", JTYPE_FLOAT },
	{ "Speed", "speed", JTYPE_FLOAT },
	{ "Temp", "temperature", JTYPE_FLOAT },
	{ "TypeImg", "icon", JTYPE_STRING },
	{ "Unit", "unit", JTYPE_INT },
	{ "Until", "until", JTYPE_STRING }, // evohome zone/water
	{ "Usage", "usage", JTYPE_STRING },
	{ "UsedByCamera", "usedByCamera", JTYPE_BOOL },
	{ "UsageDeliv", "usageDelivered", JTYPE_STRING },
	{ "ValueQuantity", "valueQuantity", JTYPE_STRING },
	{ "ValueUnits", "valueUnits", JTYPE_STRING },
	{ "Visibility", "visibility", JTYPE_FLOAT },
	{ "Voltage", "voltage", JTYPE_FLOAT },
	{ nullptr, nullptr, JTYPE_STRING },
};

CEventSystem::CEventSystem()
{
	m_bEnabled = false;
}

CEventSystem::~CEventSystem()
{
	StopEventSystem();
}

void CEventSystem::StartEventSystem()
{
	StopEventSystem();
	m_mainworker.m_notificationsystem.Register(this);

	if (!m_bEnabled)
		return;

	RequestStart();
	m_TaskQueue.RequestStart();

	m_sql.GetPreferencesVar("SecStatus", m_SecStatus);

	LoadEvents();
	GetCurrentStates();
	GetCurrentScenesGroups();
	GetCurrentUserVariables();
#ifdef ENABLE_PYTHON
	Plugins::PythonEventsInitialize(szUserDataFolder);
#endif

	m_thread = std::make_shared<std::thread>(&CEventSystem::Do_Work, this);
	SetThreadName(m_thread->native_handle(), "EventSystem");
	m_eventqueuethread = std::make_shared<std::thread>(&CEventSystem::EventQueueThread, this);
	SetThreadName(m_eventqueuethread->native_handle(), "EventSystemQueue");
	m_szStartTime = TimeToString(&m_StartTime, TF_DateTime);
}

void CEventSystem::StopEventSystem()
{
	RequestStop();
	m_TaskQueue.RequestStop();
	m_mainworker.m_notificationsystem.Unregister(this);

	if (m_eventqueuethread)
	{
		UnlockEventQueueThread();
		m_eventqueuethread->join();
		m_eventqueuethread.reset();
	}
	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}

#ifdef ENABLE_PYTHON
	Plugins::PythonEventsStop();
#endif
}

void CEventSystem::SetEnabled(const bool bEnabled)
{
	m_bEnabled = bEnabled;
	if (!bEnabled)
	{
		//Remove from heartbeat system
		m_mainworker.HeartbeatRemove("EventSystem");
	}
}

void CEventSystem::LoadEvents()
{
	std::string dzv_Dir, s;
	CdzVents* dzvents = CdzVents::GetInstance();
	dzvents->m_bdzVentsExist = false;

#ifdef WIN32
	m_lua_Dir = szUserDataFolder + "scripts\\lua\\";
	dzv_Dir = szUserDataFolder + "scripts\\dzVents\\generated_scripts\\";
	dzvents->m_scriptsDir = szUserDataFolder + "scripts\\dzVents\\scripts\\";
	dzvents->m_runtimeDir = szStartupFolder + "dzVents\\runtime\\";
#else
	m_lua_Dir = szUserDataFolder + "scripts/lua/";
	dzv_Dir = szUserDataFolder + "scripts/dzVents/generated_scripts/";
	dzvents->m_scriptsDir = szUserDataFolder + "scripts/dzVents/scripts/";
	dzvents->m_runtimeDir = szStartupFolder + "dzVents/runtime/";
#endif

	boost::unique_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
	_log.Log(LOG_STATUS, "EventSystem: reset all events...");
	m_events.clear();

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT EventRules.ID, EventMaster.Name, EventRules.Conditions, EventRules.Actions, EventMaster.Status, "
		"EventRules.SequenceNo, EventMaster.Interpreter, EventMaster.Type FROM EventRules "
		"INNER JOIN EventMaster ON EventRules.EMID = EventMaster.ID ORDER BY EventRules.ID");
	if (!result.empty())
	{
		for (auto &sd : result)
		{
			_tEventItem eitem;
			eitem.ID = std::stoull(sd[0]);
			eitem.Name = sd[1] + "_" + sd[5];
			eitem.Interpreter = sd[6];
			std::transform(sd[7].begin(), sd[7].end(), sd[7].begin(), ::tolower);
			eitem.Type = sd[7];
			eitem.Conditions = sd[2];
			eitem.Actions = sd[3];
			eitem.EventStatus = atoi(sd[4].c_str());
			eitem.SequenceNo = atoi(sd[5].c_str());
			m_events.push_back(eitem);
		}
	}

	std::vector<std::string> FileEntries;
	std::string filename;

	// Remove dzVents DB files from disk
	DirectoryListing(FileEntries, dzv_Dir, false, true);
	for (const auto &file : FileEntries)
	{
		filename = dzv_Dir + file;
		if (filename.find("README.md") == std::string::npos)
			std::remove(filename.c_str());
	}

	result = m_sql.safe_query(
		"SELECT ID, Name, Interpreter, Type, Status, XMLStatement FROM EventMaster "
		"WHERE Interpreter <> 'Blockly' AND Status > 0 ORDER BY ID");

	if (!result.empty())
	{
		for (auto &sd : result)
		{
			CEventSystem::_tEventItem eitem;
			eitem.ID = std::stoull(sd[0]);
			eitem.Name = sd[1];
			eitem.Interpreter = sd[2];
			std::transform(sd[3].begin(), sd[3].end(), sd[3].begin(), ::tolower);
			eitem.Type = sd[3];
			eitem.EventStatus = atoi(sd[4].c_str());
			eitem.Actions = sd[5];
			eitem.SequenceNo = 0;
			m_events.push_back(eitem);

			// Write active dzVents scripts to disk.
			if ((eitem.Interpreter == "dzVents") && (eitem.EventStatus != 0))
			{
				s = dzv_Dir + eitem.Name + ".lua";
				_log.Log(LOG_STATUS, "dzVents: Write file: %s", s.c_str());
				FILE *fOut = fopen(s.c_str(), "wb+");
				if (fOut)
				{
					fwrite(eitem.Actions.c_str(), 1, eitem.Actions.size(), fOut);
					fclose(fOut);
					dzvents->m_bdzVentsExist = true;
				}
				else
				{
					_log.Log(LOG_ERROR, "EventSystem: problem writing file: %s",  s.c_str());
				}
			}
		}
	}
#ifdef _DEBUG
	_log.Log(LOG_STATUS, "EventSystem: Events (re)loaded");
#endif
}

void CEventSystem::Do_Work()
{
#ifdef ENABLE_PYTHON
#ifdef WIN32
	m_python_Dir = szUserDataFolder + "scripts\\python\\";
#else
	m_python_Dir = szUserDataFolder + "scripts/python/";
#endif
#endif
	time_t lasttime = mytime(nullptr);
	struct tm tmptime;
	struct tm ltime;

	localtime_r(&lasttime, &tmptime);
	int _LastMinute = tmptime.tm_min;

	_log.Log(LOG_STATUS, "EventSystem: Started");
	while (!IsStopRequested(500))
	{
		time_t atime = mytime(nullptr);

		if (atime != lasttime)
		{
			lasttime = atime;

			localtime_r(&atime, &ltime);

			if (ltime.tm_sec % 12 == 0) {
				m_mainworker.HeartbeatUpdate("EventSystem");
			}
			if (ltime.tm_min != _LastMinute)
			{
				_LastMinute = ltime.tm_min;
				ProcessMinute();
			}
		}
	}
	_log.Log(LOG_STATUS, "EventSystem: Stopped...");

}
/*
std::string utf8_to_string(const char *utf8str, const std::locale& loc)
{
// UTF-8 to wstring
std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
std::wstring wstr = wconv.from_bytes(utf8str);
// wstring to string
std::vector<char> buf(wstr.size());
std::use_facet<std::ctype<wchar_t>>(loc).narrow(wstr.data(), wstr.data() + wstr.size(), '?', buf.data());
return std::string(buf.data(), buf.size());
}
*/

void CEventSystem::StripQuotes(std::string &sString)
{
	if (sString.empty())
		return;

	size_t tpos = sString.find('"');
	if (tpos == 0) //strip first quote
		sString = sString.substr(1);

	if (!sString.empty())
	{
		if (sString[sString.size() - 1] == '"')
		{
			//strip last quote
			sString = sString.substr(0, sString.size() - 1);
		}
	}
}

std::string CEventSystem::SpaceToUnderscore(std::string sResult)
{
	std::replace(sResult.begin(), sResult.end(), ' ', '_');
	return sResult;
}

std::string CEventSystem::LowerCase(std::string sResult)
{
	std::transform(sResult.begin(), sResult.end(), sResult.begin(), ::tolower);
	return sResult;
}

void CEventSystem::UpdateJsonMap(_tDeviceStatus &item, const uint64_t ulDevID)
{
	item.JsonMapString.clear();
	item.JsonMapFloat.clear();
	item.JsonMapInt.clear();
	item.JsonMapBool.clear();

	Json::Value tempjson;
	m_webservers.GetJSonDevices(tempjson, "", "", "", std::to_string(ulDevID), "", "", true, false, false, 0, "");
	Json::ArrayIndex rsize = tempjson["result"].size();

	if (rsize > 0)
	{
		uint8_t index = 0;
		std::string l_JsonValueString;
		l_JsonValueString.reserve(50);

		while (JsonMap[index].szOriginal != nullptr)
		{
			if (tempjson["result"][0][JsonMap[index].szOriginal] != Json::Value::null)
			{
				std::string value = tempjson["result"][0][JsonMap[index].szOriginal].asString();

				switch (JsonMap[index].eType)
				{
				case JTYPE_STRING:
					item.JsonMapString[index] = l_JsonValueString.assign(value);
					break;
				case JTYPE_FLOAT:
					item.JsonMapFloat[index] = (float)atof(value.c_str());
					break;
				case JTYPE_INT:
					item.JsonMapInt[index] = atoi(value.c_str());
					break;
				case JTYPE_BOOL:
					if (value == "true")
						item.JsonMapBool[index] = true;
					else
						item.JsonMapBool[index] = false;
					break;
				default:
					item.JsonMapString[index] = l_JsonValueString.assign("unknown_type");
				}
			}
			index++;
		}
	}
}

void CEventSystem::GetCurrentStates()
{
	std::vector<std::vector<std::string> > result;

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	_log.Log(LOG_STATUS, "EventSystem: reset all device statuses...");
	m_devicestates.clear();

	result = m_sql.safe_query(
		"SELECT A.HardwareID, A.ID, A.Name, A.nValue, A.sValue, A.Type, A.SubType, A.SwitchType, A.LastUpdate, A.LastLevel, A.Options, A.Description, A.BatteryLevel, A.SignalLevel, A.Unit, A.DeviceID, A.Protected, A.AddjValue, A.AddjMulti, A.AddjValue2, A.AddjMulti2 "
		"FROM DeviceStatus AS A, Hardware AS B "
		"WHERE (A.Used = '1') AND (B.ID == A.HardwareID) AND (B.Enabled == 1)");
	if (!result.empty())
	{
		std::map<uint64_t, _tDeviceStatus> m_devicestates_temp;
		for (auto &sd : result)
		{
			_tDeviceStatus sitem;

			// Fix string capacity to avoid map entry resizing
			std::string l_deviceName;		l_deviceName.reserve(100);
			std::string l_sValue;			l_sValue.reserve(200);
			std::string l_nValueWording;	l_nValueWording.reserve(20);
			std::string l_lastUpdate;		l_lastUpdate.reserve(30);
			std::string l_description;		l_description.reserve(200);
			std::string l_deviceID;			l_deviceID.reserve(25);

			sitem.hardwareID = atoi(sd[0].c_str());
			sitem.ID = std::stoull(sd[1]);
			sitem.deviceName = l_deviceName.assign(sd[2]);

			sitem.devType = atoi(sd[5].c_str());
			sitem.subType = atoi(sd[6].c_str());

			if ((sitem.devType == pTypeGeneral) && (sitem.subType == sTypeCounterIncremental))
			{
				//special case for incremental counter, need to calculate the actual count value

				uint64_t total_min, total_max, total_real;
				std::vector<std::vector<std::string> > result2;

				result2 = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID=%" PRIu64 ")", sitem.ID);
				total_max = std::stoull(result2[0][0]);

				//get value of today
				std::string szDate = TimeToString(nullptr, TF_Date);
				result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", sitem.ID, szDate.c_str());
				if (!result2.empty())
				{
					total_min = std::stoull(result2[0][0]);
					total_real = total_max - total_min;

					sd[4] = std::to_string(total_real); //sitem.sValue = l_sValue.assign(sd[4]);
				}
			}

			sitem.nValue = atoi(sd[3].c_str());
			sitem.sValue = l_sValue.assign(sd[4]);

			sitem.switchtype = atoi(sd[7].c_str());
			_eSwitchType switchtype = (_eSwitchType)sitem.switchtype;
			std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sd[10]);
			sitem.nValueWording = l_nValueWording.assign(nValueToWording(sitem.devType, sitem.subType, switchtype, sitem.nValue, sitem.sValue, options));
			sitem.lastUpdate = l_lastUpdate.assign(sd[8]);
			sitem.lastLevel = atoi(sd[9].c_str());
			sitem.description = l_description.assign(sd[11]);
			sitem.batteryLevel = atoi(sd[12].c_str());
			sitem.signalLevel = atoi(sd[13].c_str());
			sitem.unit = atoi(sd[14].c_str());
			sitem.deviceID = l_deviceID.assign(sd[15]);
			sitem.protection = atoi(sd[16].c_str());
			sitem.AddjValue = std::stof(sd[17]);
			sitem.AddjMulti = std::stof(sd[18]);
			sitem.AddjValue2 = std::stof(sd[19]);
			sitem.AddjMulti2 = std::stof(sd[20]);

			if (!m_sql.m_bDisableDzVentsSystem)
			{
				UpdateJsonMap(sitem, sitem.ID);
			}
			m_devicestates_temp[sitem.ID] = sitem;
		}
		m_devicestates = m_devicestates_temp;
	}
}

void CEventSystem::GetCurrentUserVariables()
{
	boost::unique_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);

	//_log.Log(LOG_STATUS, "EventSystem: reset all user variables...");
	m_uservariables.clear();

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,Value, ValueType, LastUpdate FROM UserVariables");
	if (!result.empty())
	{
		for (const auto &sd : result)
		{
			_tUserVariable uvitem;
			uvitem.ID = std::stoull(sd[0]);
			uvitem.variableName = sd[1];
			uvitem.variableValue = sd[2];
			uvitem.variableType = atoi(sd[3].c_str());
			uvitem.lastUpdate = sd[4];
			m_uservariables[uvitem.ID] = uvitem;
		}
	}
}

void CEventSystem::GetCurrentScenesGroups()
{
	boost::unique_lock<boost::shared_mutex> scenesgroupsMutexLock(m_scenesgroupsMutex);

	m_scenesgroups.clear();

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, Name, nValue, SceneType, LastUpdate, Protected FROM Scenes");
	if (!result.empty())
	{
		for (const auto &sd : result)
		{
			std::vector<std::vector<std::string> > result2;

			_tScenesGroups sgitem;
			sgitem.ID = std::stoull(sd[0]);
			unsigned char nValue = atoi(sd[2].c_str());

			if (nValue == 0)
				sgitem.scenesgroupValue = "Off";
			else if (nValue == 1)
				sgitem.scenesgroupValue = "On";
			else
				sgitem.scenesgroupValue = "Mixed";
			sgitem.scenesgroupName = sd[1];
			sgitem.scenesgroupType = atoi(sd[3].c_str());
			sgitem.lastUpdate = sd[4];
			sgitem.protection = atoi(sd[5].c_str());
			result2 = m_sql.safe_query("SELECT DISTINCT A.DeviceRowID FROM SceneDevices AS A, DeviceStatus AS B WHERE (A.SceneRowID == %" PRIu64 ") AND (A.DeviceRowID == B.ID)", sgitem.ID);
			if (!result2.empty())
			{
				for (const auto &sd2 : result2)
				{
					sgitem.memberID.push_back(std::stoull(sd2[0]));
				}
			}
			m_scenesgroups[sgitem.ID] = sgitem;
		}
	}
}

void CEventSystem::GetCurrentMeasurementStates()
{
	m_tempValuesByName.clear();
	m_dewValuesByName.clear();
	m_humValuesByName.clear();
	m_baroValuesByName.clear();
	m_utilityValuesByName.clear();
	m_rainValuesByName.clear();
	m_rainLastHourValuesByName.clear();
	m_uvValuesByName.clear();
	m_weatherValuesByName.clear();
	m_winddirValuesByName.clear();
	m_windspeedValuesByName.clear();
	m_windgustValuesByName.clear();
	m_zwaveAlarmValuesByName.clear();

	m_tempValuesByID.clear();
	m_dewValuesByID.clear();
	m_humValuesByID.clear();
	m_baroValuesByID.clear();
	m_utilityValuesByID.clear();
	m_rainValuesByID.clear();
	m_rainLastHourValuesByID.clear();
	m_uvValuesByID.clear();
	m_weatherValuesByID.clear();
	m_winddirValuesByID.clear();
	m_windspeedValuesByID.clear();
	m_windgustValuesByID.clear();
	m_zwaveAlarmValuesByID.clear();

	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	//char szTmp[300];

	for (const auto &state : m_devicestates)
	{
		_tDeviceStatus sitem = state.second;
		std::vector<std::string> splitresults;
		StringSplit(sitem.sValue, ";", splitresults);

		if ((state.second.devType == pTypeGeneral) && (state.second.subType == sTypeCounterIncremental))
			splitresults.clear();

		float temp = 0;
		int humidity = 0;
		float barometer = 0;
		float rainmm = 0;
		float rainmmlasthour = 0;
		float uv = 0;
		float dewpoint = 0;
		float utilityval = 0;
		float weatherval = 0;
		float winddir = 0;
		float windspeed = 0;
		float windgust = 0;
		int alarmval = 0;

		bool isTemp = false;
		bool isDew = false;
		bool isHum = false;
		bool isBaro = false;
		bool isUtility = false;
		bool isWeather = false;
		bool isRain = false;
		bool isUV = false;
		bool isWindDir = false;
		bool isWindSpeed = false;
		bool isWindGust = false;
		bool isZWaveAlarm = false;

		switch (sitem.devType)
		{
		case pTypeRego6XXTemp:
		case pTypeTEMP:
			if (!splitresults.empty())
			{
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				isTemp = true;
			}
			break;
		case pTypeThermostat:
			if (sitem.subType == sTypeThermTemperature)
			{
				if (!splitresults.empty())
				{
					temp = static_cast<float>(atof(splitresults[0].c_str()));
					isTemp = true;
				}
			}
			else
			{
				if (!splitresults.empty())
				{
					utilityval = static_cast<float>(atof(splitresults[0].c_str()));
					isUtility = true;
				}
			}
			break;
		case pTypeThermostat1:
			if (!splitresults.empty())
			{
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				isTemp = true;
			}
			break;
		case pTypeHUM:
			humidity = sitem.nValue;
			isHum = true;
			break;
		case pTypeTEMP_HUM:
			if (splitresults.size() > 1)
			{
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				humidity = atoi(splitresults[1].c_str());
				dewpoint = (float)CalculateDewPoint(temp, humidity);
				isTemp = true;
				isHum = true;
				isDew = true;
			}
			break;
		case pTypeTEMP_HUM_BARO:
			if (splitresults.size() < 5) {
				_log.Log(LOG_ERROR, "EventSystem: TEMP_HUM_BARO missing values : ID=%" PRIu64 ", sValue=%s", sitem.ID, sitem.sValue.c_str());
				continue;
			}
			temp = static_cast<float>(atof(splitresults[0].c_str()));
			humidity = atoi(splitresults[1].c_str());
			barometer = static_cast<float>(atof(splitresults[3].c_str()));
			dewpoint = (float)CalculateDewPoint(temp, humidity);
			isTemp = true;
			isHum = true;
			isBaro = true;
			isDew = true;
			break;
		case pTypeTEMP_BARO:
			if (splitresults.size() > 1)
			{
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				barometer = static_cast<float>(atof(splitresults[1].c_str()));
				isTemp = true;
				isBaro = true;
			}
			break;
		case pTypeBARO:
			barometer = static_cast<float>(atof(splitresults[0].c_str()));
			isBaro = true;
			break;
		case pTypeRadiator1:
			if (sitem.subType == sTypeSmartwares)
			{
				utilityval = static_cast<float>(atof(sitem.sValue.c_str()));
				isUtility = true;
			}
			break;
		case pTypeUV:
			if (splitresults.size() == 2)
			{
				uv = static_cast<float>(atof(splitresults[0].c_str()));
				isUV = true;
				weatherval = uv;
				isWeather = true;

				if (sitem.subType == sTypeUV3)
				{
					temp = static_cast<float>(atof(splitresults[1].c_str()));
					isTemp = true;
				}
			}
			break;
		case pTypeWIND:
			if (splitresults.size() == 6)
			{
				winddir = static_cast<float>(atof(splitresults[0].c_str()));
				isWindDir = true;

				if (sitem.subType != sTypeWIND5)
				{
					int intSpeed = atoi(splitresults[2].c_str());
					windspeed = float(intSpeed) * 0.1F; // m/s
					isWindSpeed = true;
				}

				int intGust = atoi(splitresults[3].c_str());
				windgust = float(intGust) * 0.1F; // m/s
				isWindGust = true;
				if ((windgust == 0) && (windspeed != 0))
				{
					weatherval = windspeed;
					isWeather = true;
				}
				else
				{
					weatherval = windgust;
					isWeather = true;
				}
				if ((sitem.subType == sTypeWIND4) || (sitem.subType == sTypeWINDNoTemp))
				{
					temp = static_cast<float>(atof(splitresults[4].c_str()));
					//chill = static_cast<float>(atof(splitresults[5].c_str()));
					isTemp = true;
				}
			}
			break;
		case pTypeRFXSensor:
			if (sitem.subType == sTypeRFXSensorTemp)
			{
				if (!splitresults.empty())
				{
					temp = static_cast<float>(atof(splitresults[0].c_str()));
					isTemp = true;
				}
			}
			else if ((sitem.subType == sTypeRFXSensorVolt) || (sitem.subType == sTypeRFXSensorAD))
			{
				utilityval = static_cast<float>(atof(sitem.sValue.c_str()));
				isUtility = true;
			}
			break;
		case pTypeAirQuality:
			utilityval = (float)(sitem.nValue);
			isUtility = true;
			break;
		case pTypeENERGY:
			if (!splitresults.empty())
			{
				if (splitresults.size() == 2)
					utilityval = static_cast<float>(atof(splitresults[1].c_str()));
				else
					utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			break;
		case pTypePOWER:
			if (!splitresults.empty())
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			break;
		case pTypeUsage:
			if (!splitresults.empty())
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			break;
		case pTypeP1Power:
			if (splitresults.size() == 6)
			{
				utilityval = static_cast<float>(atof(splitresults[4].c_str()));
				isUtility = true;
			}
			break;
		case pTypeLux:
			if (!splitresults.empty())
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			break;
		case pTypeGeneral:
		{
			if (!splitresults.empty())
			{
				if ((sitem.subType == sTypeVisibility) || (sitem.subType == sTypeSolarRadiation))
				{
					utilityval = static_cast<float>(atof(splitresults[0].c_str()));
					isUtility = true;
					weatherval = utilityval;
					isWeather = true;
				}
				else if (sitem.subType == sTypeBaro)
				{
					barometer = static_cast<float>(atof(splitresults[0].c_str()));
					isBaro = true;
				}
				else if ((sitem.subType == sTypeAlert)
					|| (sitem.subType == sTypeDistance)
					|| (sitem.subType == sTypePercentage)
					|| (sitem.subType == sTypeWaterflow)
					|| (sitem.subType == sTypeCustom)
					|| (sitem.subType == sTypeVoltage)
					|| (sitem.subType == sTypeCurrent)
					|| (sitem.subType == sTypeSetPoint)
					|| (sitem.subType == sTypeKwh)
					|| (sitem.subType == sTypeSoundLevel)
					)
				{
					utilityval = static_cast<float>(atof(splitresults[0].c_str()));
					isUtility = true;
				}
			}
			else
			{
				if (sitem.subType == sTypeZWaveAlarm)
				{
					alarmval = sitem.nValue;
					isZWaveAlarm = true;
				}
				else if (sitem.subType == sTypeCounterIncremental)
				{
					const _eMeterType metertype = (const _eMeterType)sitem.switchtype;

					float divider = m_sql.GetCounterDivider(int(metertype), int(sitem.devType), float(sitem.AddjValue2));

					uint64_t total_min, total_max, total_real;
					std::vector<std::vector<std::string> > result2;

					result2 = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID=%" PRIu64 ")", sitem.ID);
					total_max = std::stoull(result2[0][0]);

					//get value of today
					std::string szDate = TimeToString(nullptr, TF_Date);
					result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
						sitem.ID, szDate.c_str());
					if (!result2.empty())
					{
						total_min = std::stoull(result2[0][0]);
						total_real = total_max - total_min;

						utilityval = float(total_real) / divider;
						isUtility = true;
					}
				}
				else if (sitem.subType == sTypeManagedCounter)
				{
					const _eMeterType metertype = (const _eMeterType)sitem.switchtype;

					float divider = m_sql.GetCounterDivider(int(metertype), int(sitem.devType), float(sitem.AddjValue2));

					if (splitresults.size() > 1) {
						float usage = std::stof(splitresults[1]);
						if (usage < 0.0) {
							usage = 0.0;
						}

						utilityval = usage / divider;
						isUtility = true;
					}
				}
			}
		}
		break;
		case pTypeRAIN:
			if (splitresults.size() == 2)
			{
				rainmm = 0;
				rainmmlasthour = static_cast<float>(atof(splitresults[0].c_str())) / 100.0F;
				isRain = true;
				weatherval = rainmmlasthour;
				isWeather = true;

				//Calculate the total rainfall of today

				std::string szDate = TimeToString(nullptr, TF_Date);
				std::vector<std::vector<std::string> > result2;

				if (sitem.subType == sTypeRAINWU || sitem.subType == sTypeRAINByRate)
				{
					result2 = m_sql.safe_query(
						"SELECT Total, Total FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
						sitem.ID, szDate.c_str());
				}
				else
				{
					result2 = m_sql.safe_query(
						"SELECT MIN(Total), MAX(Total) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
						sitem.ID, szDate.c_str());
				}
				if (!result2.empty())
				{
					double total_real = 0;
					std::vector<std::string> sd2 = result2[0];
					if (sitem.subType == sTypeRAINWU || sitem.subType == sTypeRAINByRate)
					{
						total_real = atof(sd2[1].c_str());
					}
					else
					{
						float total_min = static_cast<float>(atof(sd2[0].c_str()));
						float total_max = static_cast<float>(atof(splitresults[1].c_str()));
						total_real = total_max - total_min;
					}
					rainmm = float(total_real);
				}
			}
			break;
		case pTypeP1Gas:
		{
			float GasDivider = 1000.0F;
			//get lowest value of today
			std::string szDate = TimeToString(nullptr, TF_Date);
			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
				sitem.ID, szDate.c_str());
			if (!result2.empty())
			{
				std::vector<std::string> sd2 = result2[0];

				uint64_t total_min_gas, total_real_gas;
				uint64_t gasactual;

				total_min_gas = std::stoull(sd2[0]);
				gasactual = std::stoull(sitem.sValue);
				total_real_gas = gasactual - total_min_gas;
				utilityval = float(total_real_gas) / GasDivider;
				isUtility = true;
			}
		}
		break;
		case pTypeRFXMeter:
			if (sitem.subType == sTypeRFXMeterCount)
			{
				const _eMeterType metertype = (const _eMeterType)sitem.switchtype;
				float divider = m_sql.GetCounterDivider(int(metertype), int(sitem.devType), float(sitem.AddjValue2));

				//get value of today
				std::string szDate = TimeToString(nullptr, TF_Date);
				std::vector<std::vector<std::string> > result2;
				result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
					sitem.ID, szDate.c_str());
				if (!result2.empty())
				{
					std::vector<std::string> sd2 = result2[0];

					uint64_t total_min, total_max, total_real;

					total_min = std::stoull(sd2[0]);
					total_max = std::stoull(sd2[1]);
					total_real = total_max - total_min;

					utilityval = float(total_real) / divider;
					isUtility = true;
				}
			}
			break;
		default:
			//Unknown device
			continue;
		}

		if (isTemp) {
			m_tempValuesByName[sitem.deviceName] = temp;
			m_tempValuesByID[sitem.ID] = temp;
		}
		if (isDew) {
			m_dewValuesByName[sitem.deviceName] = dewpoint;
			m_dewValuesByID[sitem.ID] = dewpoint;
		}
		if (isHum) {
			m_humValuesByName[sitem.deviceName] = humidity;
			m_humValuesByID[sitem.ID] = humidity;
		}
		if (isBaro) {
			m_baroValuesByName[sitem.deviceName] = barometer;
			m_baroValuesByID[sitem.ID] = barometer;
		}
		if (isUtility)
		{
			m_utilityValuesByName[sitem.deviceName] = utilityval;
			m_utilityValuesByID[sitem.ID] = utilityval;
		}
		if (isRain) {
			m_rainValuesByName[sitem.deviceName] = rainmm;
			m_rainValuesByID[sitem.ID] = rainmm;
			m_rainLastHourValuesByName[sitem.deviceName] = rainmmlasthour;
			m_rainLastHourValuesByID[sitem.ID] = rainmmlasthour;
		}
		if (isWeather)
		{
			m_weatherValuesByName[sitem.deviceName] = weatherval;
			m_weatherValuesByID[sitem.ID] = weatherval;
		}
		if (isUV) {
			m_uvValuesByName[sitem.deviceName] = uv;
			m_uvValuesByID[sitem.ID] = uv;
		}
		if (isWindDir) {
			m_winddirValuesByName[sitem.deviceName] = winddir;
			m_winddirValuesByID[sitem.ID] = winddir;
		}
		if (isWindSpeed) {
			m_windspeedValuesByName[sitem.deviceName] = windspeed;
			m_windspeedValuesByID[sitem.ID] = windspeed;
		}
		if (isWindGust) {
			m_windgustValuesByName[sitem.deviceName] = windgust;
			m_windgustValuesByID[sitem.ID] = windgust;
		}
		if (isZWaveAlarm)
		{
			m_zwaveAlarmValuesByName[sitem.deviceName] = alarmval;
			m_zwaveAlarmValuesByID[sitem.ID] = alarmval;
		}
	}
}

void CEventSystem::RemoveSingleState(const uint64_t ulDevID, const _eReason reason)
{
	if (!m_bEnabled)
		return;

	if (reason == REASON_DEVICE)
	{
		boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
		m_devicestates.erase(ulDevID);
	}
	else if (reason == REASON_SCENEGROUP)
	{
		boost::unique_lock<boost::shared_mutex> scenesgroupsMutexLock(m_scenesgroupsMutex);
		m_scenesgroups.erase(ulDevID);
	}
}

void CEventSystem::WWWUpdateSingleState(const uint64_t ulDevID, const std::string &devname, const _eReason reason)
{
	if (!m_bEnabled)
		return;

	std::string l_deviceName;		l_deviceName.reserve(100);		l_deviceName.assign(devname);

	if (reason == REASON_DEVICE)
	{
		boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

		auto itt = m_devicestates.find(ulDevID);
		if (itt != m_devicestates.end())
		{
			_tDeviceStatus replaceitem = itt->second;
			replaceitem.deviceName = l_deviceName;
			itt->second = replaceitem;
		}
	}
	else if (reason == REASON_SCENEGROUP)
	{
		boost::unique_lock<boost::shared_mutex> scenesgroupsMutexLock(m_scenesgroupsMutex);
		auto itt = m_scenesgroups.find(ulDevID);
		if (itt != m_scenesgroups.end())
		{
			_tScenesGroups replaceitem = itt->second;
			replaceitem.scenesgroupName = l_deviceName;
			itt->second = replaceitem;
		}
	}
}

void CEventSystem::WWWUpdateSecurityState(int securityStatus)
{
	if (!m_bEnabled)
		return;

	m_sql.GetPreferencesVar("SecStatus", m_SecStatus);
	_tEventQueue item;
	item.reason = REASON_SECURITY;
	item.id = 0;
	item.nValue = m_SecStatus;
	m_eventqueue.push(item);
}

bool CEventSystem::GetEventTrigger(const uint64_t ulDevID, const _eReason reason, const bool bEventTrigger)
{
	boost::unique_lock<boost::shared_mutex> eventtriggerMutexLock(m_eventtriggerMutex);
	if (!m_eventtrigger.empty())
	{
		time_t atime = mytime(nullptr);
		for (auto itt = m_eventtrigger.begin(); itt != m_eventtrigger.end();)
		{
			if (itt->ID == ulDevID && itt->reason == reason)
			{
				if (atime >= itt->timestamp)
				{
					m_eventtrigger.erase(itt);
					return !bEventTrigger;
				}
				itt = m_eventtrigger.erase(itt);
			}
			else
				itt++;
		}
	}
	return bEventTrigger;
}

bool CEventSystem::Update(const Notification::_eType type, const Notification::_eStatus status, const std::string &eventdata)
{
	if (!m_bEnabled)
		return false;
	_tEventQueue item;
	item.reason = REASON_NOTIFICATION;
	item.nValue = static_cast<int>(type);
	item.sValue = eventdata;
	item.lastLevel = static_cast<uint8_t>(status);
	if (type != Notification::DZ_STOP)
		m_eventqueue.push(item);
	else // blocking call on application shutdown
	{
		std::vector<_tEventQueue> items;
		items.push_back(item);
		EvaluateEvent(items);
	}
	return true;
}

void CEventSystem::TriggerURL(const std::string &result, const std::vector<std::string> &headerData, const std::string &callback)
{
	_tEventQueue item;
	item.reason = REASON_URL;
	item.id = 0;
	item.sValue = result;
	item.nValueWording = callback;
	item.vData = headerData;
	m_eventqueue.push(item);
}

void CEventSystem::TriggerShellCommand(const std::string &result, const std::string &scriptstderr, const std::string &callback, int exitcode, bool timeoutOccurred)
{
	_tEventQueue item;
	item.reason = REASON_SHELLCOMMAND;
	item.id = 0;
	item.sValue = result;
	item.nValue = exitcode;
	item.nValueWording = callback;
	item.errorText = scriptstderr;
	item.timeoutOccurred = timeoutOccurred;
	m_eventqueue.push(item);
}

void CEventSystem::SetEventTrigger(const uint64_t ulDevID, const _eReason reason, const float fDelayTime)
{
	if (!m_bEnabled)
		return;

	boost::unique_lock<boost::shared_mutex> eventtriggerMutexLock(m_eventtriggerMutex);
	if (!m_eventtrigger.empty())
	{
		time_t atime = mytime(nullptr) + static_cast<int>(fDelayTime);
		for (auto itt = m_eventtrigger.begin(); itt != m_eventtrigger.end();)
		{
			if (itt->ID == ulDevID && itt->reason == reason && itt->timestamp >= atime) // cancel later or equal queued items
				itt = m_eventtrigger.erase(itt);
			else
				itt++;
		}
	}
	_tEventTrigger item;
	item.ID = ulDevID;
	item.reason = reason;
	item.timestamp = mytime(nullptr) + static_cast<int>(fDelayTime);
	m_eventtrigger.push_back(item);
}

bool CEventSystem::UpdateSceneGroup(const uint64_t ulDevID, const int nValue, const std::string &lastUpdate)
{
	if (!m_bEnabled)
		return true; // seems counterintuitive, but prevents device triggers being queued

	bool bEventTrigger = true;
	boost::unique_lock<boost::shared_mutex> scenesgroupsMutexLock(m_scenesgroupsMutex);
	std::map<uint64_t, _tScenesGroups>::iterator itt = m_scenesgroups.find(ulDevID);
	if (itt != m_scenesgroups.end())
	{
		_tScenesGroups replaceitem = itt->second;
		if (nValue == 0)
			replaceitem.scenesgroupValue = "Off";
		else if (nValue == 1)
			replaceitem.scenesgroupValue = "On";
		else
			replaceitem.scenesgroupValue = "Mixed";
		bEventTrigger = GetEventTrigger(ulDevID, REASON_SCENEGROUP, true);
		if (bEventTrigger)
		{
			_tEventQueue item;
			item.nValueWording = replaceitem.scenesgroupValue;
			item.reason = REASON_SCENEGROUP;
			item.id = ulDevID;
			item.nValue = nValue;
			item.devname = replaceitem.scenesgroupName;
			item.sValue = replaceitem.scenesgroupValue;
			item.lastUpdate = itt->second.lastUpdate;
			m_eventqueue.push(item);
		}
		replaceitem.lastUpdate = lastUpdate;
		itt->second = replaceitem;
	}
	return bEventTrigger;
}

void CEventSystem::UpdateUserVariable(const uint64_t ulDevID, const std::string &varValue, const std::string &lastUpdate)
{
	if (!m_bEnabled)
		return;

	boost::unique_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);

	std::map<uint64_t, _tUserVariable>::iterator itt = m_uservariables.find(ulDevID);
	if (itt == m_uservariables.end())
		return; //not found

	_tUserVariable replaceitem = itt->second;

	replaceitem.variableValue = varValue;

	if (GetEventTrigger(ulDevID, REASON_USERVARIABLE, false))
	{
		_tEventQueue item;
		item.reason = REASON_USERVARIABLE;
		item.id = ulDevID;
		item.sValue = varValue;
		item.lastUpdate = itt->second.lastUpdate;
		m_eventqueue.push(item);
	}
	replaceitem.lastUpdate = lastUpdate;
	itt->second = replaceitem;
}

void CEventSystem::UpdateBatteryLevel(const uint64_t ulDevID, const unsigned char batteryLevel)
{
	if (!m_bEnabled)
		return;

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
	std::map<uint64_t, _tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);

	if (itt != m_devicestates.end())
	{
		_tDeviceStatus replaceitem = itt->second;
		replaceitem.batteryLevel = batteryLevel;
		itt->second = replaceitem;
	}
}


std::string CEventSystem::UpdateSingleState(
	const uint64_t ulDevID, 
	const std::string &devname, 
	const int nValue, const std::string& sValue,
	const unsigned char devType, const unsigned char subType, 
	const _eSwitchType switchType, 
	const std::string &lastUpdate, 
	const unsigned char lastLevel, 
	const unsigned char batteryLevel,
	const std::map<std::string, std::string> & options
)
{
	std::string nValueWording = nValueToWording(devType, subType, switchType, nValue, sValue, options);

	// Fix string capacity to avoid map entry resizing
	std::string l_deviceName;		l_deviceName.reserve(100);		l_deviceName.assign(devname);
	std::string l_sValue;			l_sValue.reserve(200);			l_sValue.assign(sValue);
	std::string l_nValueWording;	l_nValueWording.reserve(20);	l_nValueWording.assign(nValueWording);
	std::string l_lastUpdate;		l_lastUpdate.reserve(30);		l_lastUpdate.assign(lastUpdate);

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	std::map<uint64_t, _tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);

	if (itt != m_devicestates.end())
	{
		//_log.Log(LOG_STATUS,"EventSystem: update device %" PRIu64 "",ulDevID);
		_tDeviceStatus replaceitem = itt->second;
		replaceitem.deviceName = l_deviceName;
		//replaceitem.batteryLevel = batteryLevel;
		if (nValue != -1)
			replaceitem.nValue = nValue;
		if (!sValue.empty())
			replaceitem.sValue = l_sValue;
		if (!l_nValueWording.empty() || l_nValueWording != "-1")
			replaceitem.nValueWording = l_nValueWording;
		if (!lastUpdate.empty())
			replaceitem.lastUpdate = l_lastUpdate;
		if (lastLevel != 255)
			replaceitem.lastLevel = lastLevel;

		if (!m_sql.m_bDisableDzVentsSystem)
		{
			UpdateJsonMap(replaceitem, ulDevID);
		}
		itt->second = replaceitem;
	}
	else
	{
		//_log.Log(LOG_STATUS,"EventSystem: insert device %" PRIu64 "",ulDevID);
		_tDeviceStatus newitem;
		newitem.devType = devType;
		newitem.subType = subType;
		newitem.switchtype = switchType;
		newitem.ID = ulDevID;
		newitem.deviceName = l_deviceName;
		newitem.nValue = nValue;
		newitem.sValue = l_sValue;
		newitem.nValueWording = l_nValueWording;
		newitem.lastUpdate = l_lastUpdate;
		newitem.lastLevel = lastLevel;
		//newitem.batteryLevel = batteryLevel;

		if (!m_sql.m_bDisableDzVentsSystem)
		{
			UpdateJsonMap(newitem, ulDevID);
		}
		m_devicestates[newitem.ID] = newitem;
	}
	return nValueWording;
}

void CEventSystem::UnlockEventQueueThread()
{
	// Push dummy message to unlock queue
	_tEventQueue item;
	item.id = -1;
	m_eventqueue.push(item);
}

void CEventSystem::EventQueueThread()
{
	_log.Log(LOG_STATUS, "EventSystem: Queue thread started...");
	_tEventQueue item;
	std::vector<_tEventQueue> items;

	while (!m_TaskQueue.IsStopRequested(0))
	{
		bool hasPopped = m_eventqueue.timed_wait_and_pop<std::chrono::duration<int> >(item, std::chrono::duration<int>(5)); // timeout after 5 sec
		if (!hasPopped)
			continue;

		if (m_TaskQueue.IsStopRequested(0))
			break;
#ifdef _DEBUG
		//_log.Log(LOG_STATUS, "EventSystem: \n reason => %d\n id => %" PRIu64 "\n devname => %s\n nValue => %d\n sValue => %s\n nValueWording => %s\n lastUpdate => %s\n lastLevel => %d\n",
			//item.reason, item.id, item.devname.c_str(), item.nValue, item.sValue.c_str(), item.nValueWording.c_str(), item.lastUpdate.c_str(), item.lastLevel);
#endif
		for (const auto &i : items)
		{
			if (i.id == item.id && i.reason <= REASON_SCENEGROUP && i.reason == item.reason)
			{
				EvaluateEvent(items);
				items.clear();
				break;
			}
		}
		items.push_back(item);
		if (!m_eventqueue.empty())
			continue;

		EvaluateEvent(items);
		items.clear();
	}
	m_eventqueue.clear();

	_log.Log(LOG_STATUS, "EventSystem: Queue thread stopped...");
}

void CEventSystem::ProcessDevice(
	const int HardwareID, 
	const uint64_t ulDevID, 
	const unsigned char unit, 
	const unsigned char devType, 
	const unsigned char subType, 
	const unsigned char signallevel, 
	const unsigned char batterylevel, 
	const int nValue, 
	const char* sValue, 
	const std::string &devname)
{
	if (!m_bEnabled)
		return;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT SwitchType, LastUpdate, LastLevel, Options FROM DeviceStatus WHERE (Name == '%q')", devname.c_str());
	if (result.empty())
	{
		//inpossible as we just updated it
		_log.Log(LOG_ERROR, "EventSystem: Could not find device in system: ((ID=%" PRIu64 ": %s)", ulDevID, devname.c_str());
		return; 
	}

	std::vector<std::string> sd = result[0];

	_eSwitchType switchType = (_eSwitchType)std::stoi(sd[0]);
	std::string lastUpdate = sd[1];
	uint8_t lastLevel = (uint8_t)std::stoi(sd[2]);
	std::string dev_options = sd[3];

	std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(dev_options);

	std::string osValue = sValue;

	if ((devType == pTypeGeneral) && (subType == sTypeCounterIncremental))
	{
		//special case for incremental counter, need to calculate the actual count value

		//get value of today
		uint64_t total_max = std::stoull(osValue);

		std::string szDate = TimeToString(nullptr, TF_Date);
		std::vector<std::vector<std::string> > result2;
		result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", ulDevID, szDate.c_str());
		if (!result2.empty())
		{
			uint64_t total_min = std::stoull(result2[0][0]);
			uint64_t total_real = total_max - total_min;

			osValue = std::to_string(total_real); //sitem.sValue = l_sValue.assign(dev_options);
		}
	}

	if (g_bUseEventTrigger && GetEventTrigger(ulDevID, REASON_DEVICE, true))
	{
		_tEventQueue item;
		item.reason = REASON_DEVICE;
		item.id = ulDevID;
		item.devname = devname;
		item.nValue = nValue;
		item.sValue = osValue;
		item.nValueWording = UpdateSingleState(ulDevID, devname, nValue, osValue, devType, subType, switchType, "", 255, batterylevel, options);
		boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
		auto itt = m_devicestates.find(ulDevID);
		if (itt != m_devicestates.end())
		{
			item.lastLevel = itt->second.lastLevel;
			item.lastUpdate = itt->second.lastUpdate;
			if (!m_sql.m_bDisableDzVentsSystem)
			{
				item.JsonMapString = itt->second.JsonMapString;
				item.JsonMapInt = itt->second.JsonMapInt;
				item.JsonMapFloat = itt->second.JsonMapFloat;
				item.JsonMapBool = itt->second.JsonMapBool;
			}
			_tDeviceStatus replaceitem = itt->second;
			replaceitem.lastUpdate = lastUpdate;
			replaceitem.lastLevel = lastLevel;
			itt->second = replaceitem;
		}
		m_eventqueue.push(item);
	}
	else
		UpdateSingleState(ulDevID, devname, nValue, osValue, devType, subType, switchType, lastUpdate, lastLevel, batterylevel, options);
}

void CEventSystem::ProcessMinute()
{
	_tEventQueue item;
	item.reason = REASON_TIME;
	item.id = 0;
	m_eventqueue.push(item);
}

void CEventSystem::EvaluateEvent(const std::vector<_tEventQueue> &items)
{
	if (!m_bEnabled)
		return;

	std::vector<std::string> FileEntries;
	std::string filename;
#ifdef ENABLE_PYTHON
	std::vector<std::string> FileEntriesPython;
	DirectoryListing(FileEntriesPython, m_python_Dir, false, true);
#endif

	if (!m_sql.m_bDisableDzVentsSystem)
	{
		CdzVents* dzvents = CdzVents::GetInstance();
		if (dzvents->m_bdzVentsExist)
			EvaluateLua(items, dzvents->m_runtimeDir + "dzVents.lua", "");
		else
		{
			DirectoryListing(FileEntries, dzvents->m_scriptsDir, false, true);
			for (const auto &filename : FileEntries)
			{
				if (filename.length() > 4 &&
					filename.compare(filename.length() - 4, 4, ".lua") == 0)
				{
					EvaluateLua(items, dzvents->m_runtimeDir + "dzVents.lua", "");
					break;
				}
			}
			FileEntries.clear();
		}
	}

	bool bDeviceFileFound = false;
	DirectoryListing(FileEntries, m_lua_Dir, false, true);
	for (const auto &item : items)
	{
		for (const auto &filename : FileEntries)
		{
			if (filename.length() > 4 &&
				filename.compare(filename.length() - 4, 4, ".lua") == 0 &&
				filename.find("_demo.lua") == std::string::npos)
			{
				if (item.reason == REASON_DEVICE && filename.find("_device_") != std::string::npos)
				{
					bDeviceFileFound = false;
					boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
					for (const auto &state : m_devicestates)
					{
						std::string deviceName = SpaceToUnderscore(LowerCase(state.second.deviceName));
						if (filename.find("_device_" + deviceName + ".lua") != std::string::npos)
						{
							bDeviceFileFound = true;
							if (deviceName == SpaceToUnderscore(LowerCase(item.devname)))
							{
								devicestatesMutexLock.unlock();
								EvaluateLua(item, m_lua_Dir + filename, "");
								break;
							}
						}
					}
					if (!bDeviceFileFound)
					{
						devicestatesMutexLock.unlock();
						EvaluateLua(item, m_lua_Dir + filename, "");
					}
				}
				else if ((item.reason == REASON_TIME && filename.find("_time_") != std::string::npos)
					 || (item.reason == REASON_SECURITY && filename.find("_security_") != std::string::npos)
					 || (item.reason == REASON_NOTIFICATION && filename.find("_notification_") != std::string::npos)
					 || (item.reason == REASON_USERVARIABLE && filename.find("_variable_") != std::string::npos))
				{
					EvaluateLua(item, m_lua_Dir + filename, "");
				}
			}
			// else _log.Log(LOG_STATUS,"EventSystem: ignore file not .lua or is demo file: %s", filename.c_str());
		}

#ifdef ENABLE_PYTHON
		boost::unique_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);
		try
		{
			for (const auto &filename : FileEntriesPython)
			{
				if (filename.length() > 3 &&
					filename.compare(filename.length() - 3, 3, ".py") == 0 &&
					filename.find("_demo.py") == std::string::npos)
				{
					if ((item.reason == REASON_DEVICE && filename.find("_device_") != std::string::npos)
					    || (item.reason == REASON_TIME && filename.find("_time_") != std::string::npos)
					    || (item.reason == REASON_SECURITY && filename.find("_security_") != std::string::npos)
					    || (item.reason == REASON_USERVARIABLE && filename.find("_variable_") != std::string::npos))
					{
						EvaluatePython(item, m_python_Dir + filename, "");
					}
				}
				// else _log.Log(LOG_STATUS,"EventSystem: ignore file not .py or is demo file: %s", filename.c_str());
			}
		}
		catch (...)
		{
		}
		uservariablesMutexLock.unlock();

		// Notify plugin system of security events if a plugin owns a Security Panel
		if (item.reason == REASON_SECURITY)
		{
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query(
				"SELECT DeviceStatus.HardwareID, DeviceStatus.ID, DeviceStatus.Unit FROM DeviceStatus INNER JOIN Hardware ON DeviceStatus.HardwareID=Hardware.ID WHERE (DeviceStatus.Type=%d AND DeviceStatus.SubType=%d  AND Hardware.Type=%d)",
				pTypeSecurity1, sTypeDomoticzSecurity, HTYPE_PythonPlugin);

			if (!result.empty())
			{
				std::vector<std::string> sd = result[0];
				Plugins::CPlugin* pPlugin = (Plugins::CPlugin*)m_mainworker.GetHardware(atoi(sd[0].c_str()));
				if (pPlugin)
					pPlugin->MessagePlugin(new Plugins::onSecurityEventCallback(
						pPlugin, atoi(sd[2].c_str()), item.nValue, m_szSecStatus[item.nValue]));
			}
		}
#endif
		EvaluateDatabaseEvents(item);
	}
}

lua_State *CEventSystem::CreateBlocklyLuaState()
{
	lua_State *lua_state = luaL_newstate();
	if (lua_state == nullptr)
		return nullptr;

	// load Lua libraries
	static const luaL_Reg lualibs[] = {
		{ "base", luaopen_base },     { "io", luaopen_io },	{ "table", luaopen_table },
		{ "string", luaopen_string }, { "math", luaopen_math }, { nullptr, nullptr },
	};

	luaL_requiref(lua_state, "os", luaopen_os, 1);

	const luaL_Reg *lib = lualibs;
	for (; lib->func != nullptr; lib++)
	{
		lib->func(lua_state);
		lua_settop(lua_state, 0);
	}

	// reroute print library to domoticz logger
	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
	
	CLuaTable luaTable(lua_state, "device", (int)m_devicestates.size(), 0);

	for (const auto &state : m_devicestates)
	{
		_tDeviceStatus sitem = state.second;
		luaTable.AddString(sitem.ID, sitem. nValueWording);
	}
	luaTable.Publish();
	devicestatesMutexLock.unlock();

	boost::shared_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);
	
	luaTable.InitTable(lua_state, "variable", (int)m_uservariables.size(), 0);

	for (const auto &variable : m_uservariables)
	{
		_tUserVariable uvitem = variable.second;
		if (uvitem.variableType == 0) {
			//Integer
			luaTable.AddInteger(uvitem.ID, atoi(uvitem.variableValue.c_str()));
		}
		else if (uvitem.variableType == 1) {
			//Float
			luaTable.AddNumber(uvitem.ID, atof(uvitem.variableValue.c_str()));
		}
		else {
			//String,Date,Time
			luaTable.AddString(uvitem.ID, uvitem.variableValue);
		}
	}
	luaTable.Publish();
	uservariablesMutexLock.unlock();

	std::lock_guard<std::mutex> measurementStatesMutexLock(m_measurementStatesMutex);
	GetCurrentMeasurementStates();

	if (!m_tempValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "temperaturedevice", (int)m_tempValuesByID.size(), 0);
		for (const auto &temp : m_tempValuesByID)
			luaTable.AddNumber(temp.first, temp.second);
		luaTable.Publish();
	}
	if (!m_dewValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "dewpointdevice", (int)m_dewValuesByID.size(), 0);
		for (const auto &dew : m_dewValuesByID)
		{
			luaTable.AddNumber(dew.first, dew.second);
		}
		luaTable.Publish();
	}
	if (!m_humValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "humiditydevice", (int)m_humValuesByID.size(), 0);
		for (const auto &hum : m_humValuesByID)
		{
			luaTable.AddNumber(hum.first, hum.second);
		}
		luaTable.Publish();
	}
	if (!m_baroValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "barometerdevice", (int)m_baroValuesByID.size(), 0);
		for (const auto &baro : m_baroValuesByID)
		{
			luaTable.AddNumber(baro.first, baro.second);
		}
		luaTable.Publish();
	}
	if (!m_utilityValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "utilitydevice", (int)m_utilityValuesByID.size(), 0);
		for (const auto &utility : m_utilityValuesByID)
		{
			luaTable.AddNumber(utility.first, utility.second);
		}
		luaTable.Publish();
	}
	if (!m_weatherValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "weatherdevice", (int)m_weatherValuesByID.size(), 0);
		for (const auto &weather : m_weatherValuesByID)
		{
			luaTable.AddNumber(weather.first, weather.second);
		}
		luaTable.Publish();
	}
	if (!m_rainValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "raindevice", (int)m_rainValuesByID.size(), 0);
		for (const auto &rain : m_rainValuesByID)
		{
			luaTable.AddNumber(rain.first, rain.second);
		}
		luaTable.Publish();
	}
	if (!m_rainLastHourValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "rainlasthourdevice", (int)m_rainLastHourValuesByID.size(), 0);
		for (const auto &rainlh : m_rainLastHourValuesByID)
		{
			luaTable.AddNumber(rainlh.first, rainlh.second);
		}
		luaTable.Publish();
	}
	if (!m_uvValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "uvdevice", (int)m_uvValuesByID.size(), 0);
		for (const auto &uv : m_uvValuesByID)
		{
			luaTable.AddNumber(uv.first, uv.second);
		}
		luaTable.Publish();
	}
	if (!m_winddirValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "winddirdevice", (int)m_winddirValuesByID.size(), 0);
		for (const auto &winddir : m_winddirValuesByID)
		{
			luaTable.AddNumber(winddir.first, winddir.second);
		}
		luaTable.Publish();
	}
	if (!m_windspeedValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "windspeeddevice", (int)m_windspeedValuesByID.size(), 0);
		for (const auto &windspeed : m_windspeedValuesByID)
		{
			luaTable.AddNumber(windspeed.first, windspeed.second);
		}
		luaTable.Publish();
	}
	if (!m_windgustValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "windgustdevice", (int)m_windgustValuesByID.size(), 0);
		for (const auto &windgust : m_windgustValuesByID)
		{
			luaTable.AddNumber(windgust.first, windgust.second);
		}
		luaTable.Publish();
	}
	if (!m_zwaveAlarmValuesByID.empty())
	{
		luaTable.InitTable(lua_state, "zwavealarms", (int)m_zwaveAlarmValuesByID.size(), 0);
		for (const auto &alarm : m_zwaveAlarmValuesByID)
		{
			luaTable.AddNumber(alarm.first, alarm.second);
		}
		luaTable.Publish();
	}

	lua_pushnumber(lua_state, (lua_Number)m_SecStatus);
	lua_setglobal(lua_state, "securitystatus");

	return lua_state;
}

lua_State *CEventSystem::ParseBlocklyLua(lua_State *lua_state, const _tEventItem &item)
{
	std::string conditions = item.Conditions;
	// Replace Sunrise and sunset placeholder with actual time for query
	if (conditions.find("@Sunrise") != std::string::npos)
	{
		stdreplace(conditions, "@Sunrise", std::to_string(getSunRiseSunSetMinutes("Sunrise")));
	}
	if (conditions.find("@Sunset") != std::string::npos)
	{
		stdreplace(conditions, "@Sunset", std::to_string(getSunRiseSunSetMinutes("Sunset")));
	}

	std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + conditions + " then result = 1 end; return result";

	if (lua_state == nullptr)
	{
		lua_state = CreateBlocklyLuaState();
		if (lua_state == nullptr)
			return nullptr;
	}

	//_log.Log(LOG_STATUS,"EventSystem: ifc: %s",ifCondition.c_str());
	if (luaL_dostring(lua_state, ifCondition.c_str()))
	{
		_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", item.Name.c_str(), lua_tostring(lua_state, -1));
	}
	else
	{
		lua_Number ruleTrue = lua_tonumber(lua_state, -1);
		if (ruleTrue != 0)
		{
			if (m_sql.m_bLogEventScriptTrigger)
				_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", item.Name.c_str());
			parseBlocklyActions(item);
		}
	}
	return lua_state;
}

void CEventSystem::EvaluateDatabaseEvents(const _tEventQueue &item)
{
	lua_State *lua_state = nullptr;

	boost::shared_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
	try
	{
		for (const auto &event : m_events)
		{
			bool eventInScope = ((event.Type == "all") || (event.Type == m_szReason[item.reason]));
			bool eventActive = (event.EventStatus == 1);

			if (eventInScope && eventActive)
			{
				if (event.Interpreter == "Blockly")
				{
					std::size_t found = std::string::npos;
					if ((item.reason == REASON_DEVICE) && (item.id > 0))
					{
						std::stringstream sstr;
						sstr << "[" << item.id << "]";
						found = event.Conditions.find(sstr.str());
					}
					else if (item.reason == REASON_SECURITY)
					{
						// security status change
						std::stringstream sstr;
						sstr << "securitystatus";
						found = event.Conditions.find(sstr.str());
					}
					else if (item.reason == REASON_TIME)
					{
						// time rules will only run when time or date based criteria are found
						found = event.Conditions.find("timeofday");
						if (found == std::string::npos)
							found = event.Conditions.find("weekday");
					}
					else if ((item.reason == REASON_USERVARIABLE) && (item.id > 0))
					{
						std::stringstream sstr;
						sstr << "variable[" << item.id << "]";
						found = event.Conditions.find(sstr.str());
					}

					if (found != std::string::npos)
						lua_state = ParseBlocklyLua(lua_state, event);
				}
				else if (event.Interpreter == "Lua")
					EvaluateLua(item, event.Name, event.Actions);

				else if (event.Interpreter == "Python")
				{
#ifdef ENABLE_PYTHON
					boost::unique_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);
					EvaluatePython(item, event.Name, event.Actions);
#else
					_log.Log(LOG_ERROR, "EventSystem: Error processing database scripts, Python not enabled");
#endif
				}
			}
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "EventSystem: Exception processing database scripts");
	}

	if (lua_state != nullptr)
		lua_close(lua_state);
}

static inline long long GetIndexFromDevice(std::string devline)
{
	size_t fpos = devline.find('[');
	if (fpos == std::string::npos)
		return -1;
	devline = devline.substr(fpos + 1);
	fpos = devline.find(']');
	if (fpos == std::string::npos)
		return -1;
	devline = devline.substr(0, fpos);
	return atoll(devline.c_str());
}

std::string CEventSystem::ProcessVariableArgument(const std::string &Argument)
{
	std::string ret = Argument;
	long long dindex = GetIndexFromDevice(Argument);
	if (dindex == -1)
		return ret;

	if (Argument.find("temperaturedevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_tempValuesByID.find(dindex);
		if (itt != m_tempValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("dewpointdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_dewValuesByID.find(dindex);
		if (itt != m_dewValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("humiditydevice") == 0)
	{
		std::map<uint64_t, int>::const_iterator itt = m_humValuesByID.find(dindex);
		if (itt != m_humValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("barometerdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_baroValuesByID.find(dindex);
		if (itt != m_baroValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("utilitydevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_utilityValuesByID.find(dindex);
		if (itt != m_utilityValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("weatherdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_weatherValuesByID.find(dindex);
		if (itt != m_weatherValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("raindevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_rainValuesByID.find(dindex);
		if (itt != m_rainValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("rainlasthourdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_rainLastHourValuesByID.find(dindex);
		if (itt != m_rainLastHourValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("uvdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_uvValuesByID.find(dindex);
		if (itt != m_uvValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("winddirdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_winddirValuesByID.find(dindex);
		if (itt != m_winddirValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("windspeeddevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_windspeedValuesByID.find(dindex);
		if (itt != m_windspeedValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("windgustdevice") == 0)
	{
		std::map<uint64_t, float>::const_iterator itt = m_windgustValuesByID.find(dindex);
		if (itt != m_windgustValuesByID.end())
		{
			std::stringstream sstr;
			sstr << itt->second;
			return sstr.str();
		}
	}
	else if (Argument.find("variable") == 0)
	{
		std::map<uint64_t, _tUserVariable>::const_iterator itt = m_uservariables.find(dindex);
		if (itt != m_uservariables.end())
		{
			return itt->second.variableValue;
		}
	}
	else if (Argument.find("zwavealarms") == 0)
	{
		std::map<uint64_t, int>::const_iterator itt = m_zwaveAlarmValuesByID.find(dindex);
		if (itt != m_zwaveAlarmValuesByID.end())
		{
			std::stringstream sstr;
			sstr << (int)itt->second;
			return sstr.str();
		}
	}

	return ret;
}

std::string CEventSystem::ParseBlocklyString(const std::string &oString)
{
	std::string retString = oString;

	while (true)
	{
		size_t pos1, pos2;
		pos1 = retString.find("{{");
		if (pos1 == std::string::npos)
			return retString;
		pos2 = retString.find("}}");
		if (pos2 == std::string::npos)
			return retString;
		std::string part_left = retString.substr(0, pos1);
		std::string part_middle = retString.substr(pos1 + 2, pos2 - pos1 - 2);
		std::string part_right = retString.substr(pos2 + 2);
		part_middle = ProcessVariableArgument(part_middle);
		retString = part_left + part_middle + part_right;
	}

	return retString;
}

bool CEventSystem::parseBlocklyActions(const _tEventItem &item)
{
	if (isEventscheduled(item.Name))
	{
		//_log.Log(LOG_NORM,"Already scheduled this event, skipping");
		return false;
	}
	bool actionsDone = false;
	std::string csubstr;
	std::string tmpstr(item.Actions);
	size_t sPos = 0, ePos;
	do
	{
		ePos = tmpstr.find(",commandArray[");
		if (ePos != std::string::npos)
		{
			csubstr = tmpstr.substr(0, ePos);
			tmpstr = tmpstr.substr(ePos + 1);
		}
		else
		{
			csubstr = tmpstr;
			tmpstr.clear();
		}
		sPos = csubstr.find_first_of('[');
		ePos = csubstr.find_first_of(']');

		if ((sPos == std::string::npos) || (ePos == std::string::npos))
		{
			_log.Log(LOG_ERROR, "EventSystem: Malformed action sequence!");
			break;
		}

		size_t eQPos = csubstr.find_first_of('=');
		if (eQPos == std::string::npos) {
			_log.Log(LOG_ERROR, "EventSystem: Malformed action sequence!");
			break;
		}
		std::string doWhat = csubstr.substr(eQPos + 1);
		StripQuotes(doWhat);

		std::string deviceName = csubstr.substr(sPos + 1, ePos - sPos - 1);
		if (deviceName.empty())
		{
			_log.Log(LOG_ERROR, "EventSystem: Malformed action sequence!");
			break;
		}

		int deviceNo = atoi(deviceName.c_str());
		if (deviceNo)
		{
			boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
			if (m_devicestates.count(deviceNo)) {
				devicestatesMutexLock.unlock(); // Unlock to avoid recursive lock (because the ScheduleEvent function locks again)
				if (ScheduleEvent(deviceNo, doWhat, false, item.Name, 0)) {
					actionsDone = true;
				}
			}
			else {
				reportMissingDevice(deviceNo, item);
			}
			continue;
		}

		if ((deviceName.find("Scene:") == 0) || (deviceName.find("Group:") == 0))
		{
			int sceneType = 1;
			if (deviceName.find("Group:") == 0)
			{
				sceneType = 2;
			}
			deviceNo = atoi(deviceName.substr(6).c_str());
			if (deviceNo)
			{
				if (ScheduleEvent(deviceNo, doWhat, true, item.Name, sceneType))
				{
					actionsDone = true;
				}
			}
			continue;
		}
		if (deviceName.find("Variable:") == 0)
		{
			std::string variableNo = deviceName.substr(9);
			_tActionParseResults parseResult;
			parseResult.fAfterSec = 0;
			ParseActionString(doWhat, parseResult);
			StripQuotes(parseResult.sCommand);

			doWhat = ProcessVariableArgument(parseResult.sCommand);
			if (parseResult.fAfterSec < (1. / timer_resolution_hz / 2))
			{
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE (ID == '%q')", variableNo.c_str());
				if (!result.empty())
				{
					std::vector<std::string> sd = result[0];
					std::string errorMessage;
					if (!m_sql.UpdateUserVariable(variableNo, sd[0], (const _eUsrVariableType)atoi(sd[1].c_str()), doWhat, false, errorMessage))
					{
						_log.Log(LOG_ERROR, "EventSystem: Error updating variable %s: %s", sd[0].c_str(), errorMessage.c_str());
					}
				}
			}
			else
				m_sql.AddTaskItem(_tTaskItem::SetVariable(parseResult.fAfterSec, (const uint64_t)atol(variableNo.c_str()), doWhat, false));

			actionsDone = true;
			continue;
		}
		if (deviceName.find("Text:") == 0)
		{
			std::string variableName = deviceName.substr(5);
			_tActionParseResults parseResult;
			parseResult.fAfterSec = 0;
			ParseActionString(doWhat, parseResult);
			StripQuotes(parseResult.sCommand);

			if (parseResult.fAfterSec < (1. / timer_resolution_hz / 2))
				m_mainworker.UpdateDevice(std::stoi(variableName), 0, parseResult.sCommand, "EventSystem/" + item.Name,
					12, 255, false);
			else
				m_sql.AddTaskItem(_tTaskItem::UpdateDevice(parseResult.fAfterSec, std::stoull(variableName), 0, parseResult.sCommand, false, false, item.Name));

			actionsDone = true;
		}
		else if (deviceName.find("SendCamera:") == 0)
		{
			if (!atoi(deviceName.substr(11).c_str()))
				continue;
			ScheduleEvent(deviceName, doWhat, item.Name);
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("SetSetpoint:") == 0)
		{
			int idx = atoi(deviceName.substr(12).c_str());
			std::string temp, mode, until;
			std::vector<std::string> aParam;
			StringSplit(doWhat, "#", aParam);
			switch (aParam.size()) {
			case 3:
				until = ParseBlocklyString(aParam[2]);
			case 2:
				mode = ParseBlocklyString(aParam[1]);
			case 1:
				temp = ParseBlocklyString(aParam[0]);
				m_sql.AddTaskItem(_tTaskItem::SetSetPoint(0.5F, idx, temp, mode, until));
				actionsDone = true;
				break;

			default:
				//Invalid
				_log.Log(LOG_ERROR, "EventSystem: SetPoint, not enough parameters!");
				break;
			}
			continue;
		}
		else if (deviceName.find("SendEmail") != std::string::npos)
		{
			std::string subject, body, to;
			std::vector<std::string> aParam;
			StringSplit(doWhat, "#", aParam);
			if (aParam.size() != 3)
			{
				//Invalid
				_log.Log(LOG_ERROR, "EventSystem: SendEmail, not enough parameters!");
				continue;
			}
			subject = ParseBlocklyString(aParam[0]);
			body = ParseBlocklyString(aParam[1]);
			stdreplace(body, "\\n", "<br>");
			to = aParam[2];
			m_sql.AddTaskItem(_tTaskItem::SendEmailTo(1, subject, body, to));
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("SendSMS") != std::string::npos)
		{
			if (doWhat.empty())
			{
				//Invalid
				_log.Log(LOG_ERROR, "EventSystem: SendSMS, not enough parameters!");
				continue;
			}
			doWhat = ParseBlocklyString(doWhat);
			m_sql.AddTaskItem(_tTaskItem::SendSMS(1, doWhat));
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("TriggerIFTTT") != std::string::npos)
		{
			std::vector<std::string> aParam;
			StringSplit(doWhat, "#", aParam);
			if ((aParam.empty()) || (aParam.size() > 4))
			{
				//Invalid
				_log.Log(LOG_ERROR, "EventSystem: TriggerIFTTT, not enough parameters!");
				continue;
			}
			std::string sID = ParseBlocklyString(aParam[0]);
			std::string sValue1, sValue2, sValue3;
			if (aParam.size() > 1)
				sValue1 = ParseBlocklyString(aParam[1]);
			if (aParam.size() > 2)
				sValue2 = ParseBlocklyString(aParam[2]);
			if (aParam.size() > 3)
				sValue3 = ParseBlocklyString(aParam[3]);
			m_sql.AddTaskItem(_tTaskItem::SendIFTTTTrigger(1, sID, sValue1, sValue2, sValue3));
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("OpenURL") != std::string::npos)
		{
			_tActionParseResults parseResult;
			parseResult.fAfterSec = 0.2F;
			ParseActionString(doWhat, parseResult);
			OpenURL(parseResult.fAfterSec, parseResult.sCommand);
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("StartScript") != std::string::npos)
		{
			if (doWhat.empty())
			{
				//Invalid
				_log.Log(LOG_ERROR, "EventSystem: StartScript, not enough parameters!");
				break;
			}
			std::string sPath = doWhat;
			std::string sParam;
			size_t tpos = sPath.find('$');
			if (tpos != std::string::npos)
			{
				sPath = sPath.substr(0, tpos);
				sParam = doWhat.substr(tpos + 1);
				sParam = ParseBlocklyString(sParam);
			}
#if !defined WIN32
			if (sPath.find('/') != 0)
				sPath = szUserDataFolder + "scripts/" + sPath;
#endif

			m_sql.AddTaskItem(_tTaskItem::ExecuteScript(0.2F, sPath, sParam));
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("WriteToLog") != std::string::npos)
		{
			std::string devNameNoQuotes = deviceName.substr(1, deviceName.size() - 2);
			WriteToLog(devNameNoQuotes, doWhat);
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("SendNotification") != std::string::npos)
		{
			std::string subject, body, priority("0"), sound, subsystem;
			std::vector<std::string> aParam;
			StringSplit(doWhat, "#", aParam);
			subject = body = aParam[0];
			if (aParam.size() > 1)
			{
				body = aParam[1];
			}

			StripQuotes(subject);
			StripQuotes(body);

			subject = ParseBlocklyString(ProcessVariableArgument(subject));
			body = ParseBlocklyString(ProcessVariableArgument(body));

			if (aParam.size() == 3)
			{
				priority = aParam[2];
			}
			else if (aParam.size() == 4)
			{
				priority = aParam[2];
				sound = aParam[3];
			}
			else if (aParam.size() == 5)
			{
				priority = aParam[2];
				sound = aParam[3];
				subsystem = aParam[4];
			}
			m_sql.AddTaskItem(_tTaskItem::SendNotification(0, subject, body, std::string(""), atoi(priority.c_str()), sound, subsystem));
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("CustomCommand:") == 0)
		{
			int idx = atoi(deviceName.substr(14).c_str());
			_tActionParseResults parseResult;
			parseResult.fAfterSec = 0;
			ParseActionString(doWhat, parseResult);
			m_sql.AddTaskItem(_tTaskItem::CustomCommand(parseResult.fAfterSec, idx, doWhat));
			actionsDone = true;
			continue;
		}
		else
		{
			_log.Log(LOG_ERROR, "EventSystem: Unknown action sequence! (%s)", csubstr.c_str());
			break;
		}
	} while ((sPos = tmpstr.find("commandArray[")) == 0);
	return actionsDone;
}

void CEventSystem::ParseActionString(const std::string &oAction_, _tActionParseResults &oResults_) {
	int iLastTokenType = 0;

	std::vector<std::string> oSplitResults;
	StringSplit(oAction_, " ", oSplitResults);
	for (const auto &sToken : oSplitResults)
	{
		if (sToken == "FOR") {
			iLastTokenType = 1;
		}
		else if (sToken == "AFTER")
		{
			iLastTokenType = 2;
		}
		else if (sToken == "RANDOM")
		{
			iLastTokenType = 3;
		}
		else if (sToken == "REPEAT")
		{
			iLastTokenType = 4;
		}
		else if (sToken == "INTERVAL")
		{
			iLastTokenType = 5;
		}
		else if (sToken == "TURN")
		{
			iLastTokenType = 0;
		}
		else if (sToken == "TRIGGER")
		{
			iLastTokenType = 0;
		}
		else if (sToken == "NOTRIGGER")
			oResults_.bEventTrigger = false;

		else if (sToken.find("SECOND") != std::string::npos) {
			switch (iLastTokenType) {
			case 1: oResults_.fForSec /= 60.; break;
			case 3: oResults_.fRandomSec /= 60.; break;
			}
			iLastTokenType = 0;
		}
		else if (sToken.find("MINUTE") != std::string::npos) {
			switch (iLastTokenType) {
			case 2: oResults_.fAfterSec *= 60.; break;
			case 5: oResults_.fRepeatSec *= 60.; break;
			}
			iLastTokenType = 0;
		}
		else if (sToken.find("HOUR") != std::string::npos) {
			switch (iLastTokenType) {
			case 1: oResults_.fForSec *= 60.; break;
			case 2: oResults_.fAfterSec *= 3600.; break;
			case 3: oResults_.fRandomSec *= 60.; break;
			case 5: oResults_.fRepeatSec *= 3600.; break;
			}
			iLastTokenType = 0;
		}
		else {
			switch (iLastTokenType) {
			case 0:
				if (oResults_.sCommand.length() > 0) {
					oResults_.sCommand.append(" ");
				}
				oResults_.sCommand.append(sToken);
				break;
			case 1:
				oResults_.fForSec = 60.F * static_cast<float>(atof(sToken.c_str()));
				break;
			case 2:
				oResults_.fAfterSec = 1.F * static_cast<float>(atof(sToken.c_str()));
				break;
			case 3:
				oResults_.fRandomSec = 60.F * static_cast<float>(atof(sToken.c_str()));
				break;
			case 4:
				oResults_.iRepeat = atoi(sToken.c_str());
				break;
			case 5:
				oResults_.fRepeatSec = 1.F * static_cast<float>(atof(sToken.c_str()));
				break;
			}
		}
	}
#ifdef _DEBUG
	_log.Log(
		LOG_NORM, "Command=%s, FOR=%.2f, AFTER=%.2f, RANDOM=%.2f, REPEAT=%d INTERVAL %.2f",
		oResults_.sCommand.c_str(),
		oResults_.fForSec,
		oResults_.fAfterSec,
		oResults_.fRandomSec,
		oResults_.iRepeat,
		oResults_.fRepeatSec
	);
#endif
}

#ifdef ENABLE_PYTHON

// Python EventModule helper functions
bool CEventSystem::PythonScheduleEvent(const std::string &ID, const std::string &Action, const std::string &eventName)
{
	if (ID.find("Variable:") == 0) {
		std::string variableName = ID.substr(9);

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ID, ValueType FROM UserVariables WHERE (Name == '%q')", variableName.c_str());
		if (result.size() != 1) {
			_log.Log(LOG_ERROR, "EventSystem: Unknown variable %s", variableName.c_str());
			return false;
		}
		std::vector<std::string> sd = result[0];

		std::string doWhat = std::string(Action);
		_tActionParseResults parseResult;
		parseResult.fAfterSec = 0;
		ParseActionString(doWhat, parseResult);

		doWhat = ProcessVariableArgument(parseResult.sCommand);

		uint64_t idx = atol(sd[0].c_str());
		m_sql.AddTaskItem(_tTaskItem::SetVariable(parseResult.fAfterSec, idx, doWhat, false));

		return true;
	}
	if (ID.find("SetSetpoint:") == 0)
	{
		int idx = atoi(ID.substr(12).c_str());
		std::string doWhat = std::string(Action);
		std::string temp, mode, until;
		std::vector<std::string> aParam;
		StringSplit(doWhat, "#", aParam);
		switch (aParam.size()) {
		case 3:
			until = aParam[2];
		case 2:
			mode = aParam[1];
		case 1:
			temp = aParam[0];
			m_sql.AddTaskItem(_tTaskItem::SetSetPoint(0.5F, idx, temp, mode, until));
			break;

		default:
			//Invalid
			_log.Log(LOG_ERROR, "EventSystem: SetPoint, not enough parameters!");
			return false;
		}

		return true;
	}
	if (ID.find("CustomCommand:") == 0)
	{
		int idx = atoi(ID.substr(14).c_str());
		std::string doWhat = std::string(Action);
		_tActionParseResults parseResult;
		parseResult.fAfterSec = 0;
		ParseActionString(doWhat, parseResult);
		m_sql.AddTaskItem(_tTaskItem::CustomCommand(parseResult.fAfterSec, idx, doWhat));
		return true;
	}
	return ScheduleEvent(ID, Action, eventName);
}

void CEventSystem::EvaluatePython(const _tEventQueue &item, const std::string &filename, const std::string &PyString)
{
	//_log.Log(LOG_NORM, "EventSystem: Already scheduled this event, skipping");
	// _log.Log(LOG_STATUS, "EventSystem: script %s trigger, file: %s, script: %s, deviceName: %s" , reason.c_str(), filename.c_str(), PyString.c_str(), devname.c_str());

	Plugins::PythonEventsProcessPython(m_szReason[item.reason], filename, PyString, item.id, m_devicestates, m_uservariables, getSunRiseSunSetMinutes("Sunrise"),
		getSunRiseSunSetMinutes("Sunset"));

	//Py_Finalize();
}

#endif // ENABLE_PYTHON

void CEventSystem::ExportDeviceStatesToLua(lua_State *lua_state, const _tEventQueue &item)
{
	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	CLuaTable luaTable(lua_state, "otherdevices", (int)m_devicestates.size(), 0);
	for (const auto &state : m_devicestates)
	{
		luaTable.AddString(state.second.deviceName, (state.first == item.id && item.reason == REASON_DEVICE)
								    ? item.nValueWording
								    : state.second.nValueWording);
	}
	luaTable.Publish();

	luaTable.InitTable(lua_state, "otherdevices_lastupdate", (int)m_devicestates.size(), 0);
	for (const auto &state : m_devicestates)
	{
		luaTable.AddString(state.second.deviceName,
				   (state.first == item.id && item.reason == REASON_DEVICE) ? item.lastUpdate : state.second.lastUpdate);
	}
	luaTable.Publish();

	luaTable.InitTable(lua_state, "otherdevices_svalues", (int)m_devicestates.size(), 0);
	for (const auto &state : m_devicestates)
	{
		luaTable.AddString(state.second.deviceName,
				   (state.first == item.id && item.reason == REASON_DEVICE) ? item.sValue : state.second.sValue);
	}
	luaTable.Publish();

	luaTable.InitTable(lua_state, "otherdevices_idx", (int)m_devicestates.size(), 0);
	for (const auto &state : m_devicestates)
	{
		luaTable.AddInteger(state.second.deviceName, state.second.ID);
	}
	luaTable.Publish();

	luaTable.InitTable(lua_state, "otherdevices_lastlevel", (int)m_devicestates.size(), 0);
	for (const auto &state : m_devicestates)
	{
		luaTable.AddNumber(state.second.deviceName,
				   (state.first == item.id && item.reason == REASON_DEVICE) ? item.lastLevel : state.second.lastLevel);
	}
	luaTable.Publish();
}

void CEventSystem::EvaluateLuaClassic(lua_State *lua_state, const _tEventQueue &item, const int secStatus)
{
	// reroute print library to Domoticz logger
	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

	{
		std::lock_guard<std::mutex> measurementStatesMutexLock(m_measurementStatesMutex);
		GetCurrentMeasurementStates();

		float thisDeviceTemp = 0;
		float thisDeviceDew = 0;
		float thisDeviceRain = 0;
		float thisDeviceRainLastHour = 0;
		float thisDeviceUV = 0;
		unsigned char thisDeviceHum = 0;
		float thisDeviceBaro = 0;
		float thisDeviceUtility = 0;
		//float thisDeviceWindDir = 0;
		//float thisDeviceWindSpeed = 0;
		//float thisDeviceWindGust = 0;
		float thisDeviceWeather = 0;
		int thisZwaveAlarm = 0;

		if (!m_tempValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_temperature", (int)m_tempValuesByName.size(), 0);
			for (const auto &temp : m_tempValuesByName)
			{
				luaTable.AddNumber(temp.first, temp.second);
				if (temp.first == item.devname)
				{
					thisDeviceTemp = temp.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_dewValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_dewpoint", (int)m_dewValuesByName.size(), 0);
			for (const auto &dew : m_dewValuesByName)
			{
				luaTable.AddNumber(dew.first, dew.second);
				if (dew.first == item.devname)
				{
					thisDeviceDew = dew.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_humValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_humidity", (int)m_humValuesByName.size(), 0);
			for (const auto &hum : m_humValuesByName)
			{
				luaTable.AddNumber(hum.first, hum.second);
				if (hum.first == item.devname)
				{
					thisDeviceHum = hum.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_baroValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_barometer", (int)m_baroValuesByName.size(), 0);
			for (const auto &baro : m_baroValuesByName)
			{
				luaTable.AddNumber(baro.first, baro.second);
				if (baro.first == item.devname)
				{
					thisDeviceBaro = (float)baro.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_utilityValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_utility", (int)m_utilityValuesByName.size(), 0);
			for (const auto &utility : m_utilityValuesByName)
			{
				luaTable.AddNumber(utility.first, utility.second);
				if (utility.first == item.devname)
				{
					thisDeviceUtility = utility.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_rainValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_rain", (int)m_rainValuesByName.size(), 0);
			for (const auto &rain : m_rainValuesByName)
			{
				luaTable.AddNumber(rain.first, rain.second);
				if (rain.first == item.devname)
				{
					thisDeviceRain = rain.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_rainLastHourValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_rain_lasthour", (int)m_rainLastHourValuesByName.size(), 0);
			for (const auto &rainlh : m_rainLastHourValuesByName)
			{
				luaTable.AddNumber(rainlh.first, rainlh.second);
				if (rainlh.first == item.devname)
				{
					thisDeviceRainLastHour = rainlh.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_uvValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_uv", (int)m_uvValuesByName.size(), 0);
			for (const auto &uv : m_uvValuesByName)
			{
				luaTable.AddNumber(uv.first, uv.second);
				if (uv.first == item.devname)
				{
					thisDeviceUV = uv.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_winddirValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_winddir", (int)m_winddirValuesByName.size(), 0);
			for (const auto &winddir : m_winddirValuesByName)
			{
				luaTable.AddNumber(winddir.first, winddir.second);
				// if (winddir.first == item.devname) {
				// thisDeviceWindDir = winddir.second;
				//}
			}
			luaTable.Publish();
		}
		if (!m_windspeedValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_windspeed", (int)m_windspeedValuesByName.size(), 0);
			for (const auto &windspeed : m_windspeedValuesByName)
			{
				luaTable.AddNumber(windspeed.first, windspeed.second);
				// if (windspeed.first == item.devname) {
				// thisDeviceWindSpeed = windspeed.second;
				//}
			}
			luaTable.Publish();
		}
		if (!m_windgustValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_windgust", (int)m_windgustValuesByName.size(), 0);
			for (const auto &windgust : m_windgustValuesByName)
			{
				luaTable.AddNumber(windgust.first, windgust.second);
				// if (windgust.first == item.devname) {
				// thisDeviceWindGust = windgust.second;
				//}
			}
			luaTable.Publish();
		}
		if (!m_weatherValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_weather", (int)m_weatherValuesByName.size(), 0);
			for (const auto &weather : m_weatherValuesByName)
			{
				luaTable.AddNumber(weather.first, weather.second);
				if (weather.first == item.devname)
				{
					thisDeviceWeather = weather.second;
				}
			}
			luaTable.Publish();
		}
		if (!m_zwaveAlarmValuesByName.empty())
		{
			CLuaTable luaTable(lua_state, "otherdevices_zwavealarms", (int)m_zwaveAlarmValuesByName.size(), 0);
			for (const auto &alarm : m_zwaveAlarmValuesByName)
			{
				luaTable.AddNumber(alarm.first, alarm.second);
				if (alarm.first == item.devname)
				{
					thisZwaveAlarm = alarm.second;
				}
			}
			luaTable.Publish();
		}

		if (item.reason == REASON_DEVICE)
		{
			CLuaTable luaTable(lua_state, "devicechanged", 1, 0);
			luaTable.AddString(item.devname, item.nValueWording);
			if (thisDeviceTemp != 0)
			{
				std::string tempName = item.devname;
				tempName += "_Temperature";
				luaTable.AddNumber(tempName, thisDeviceTemp);
			}
			if (thisDeviceDew != 0)
			{
				std::string tempName = item.devname;
				tempName += "_Dewpoint";
				luaTable.AddNumber(tempName, thisDeviceDew);
			}
			if (thisDeviceHum != 0) {
				std::string humName = item.devname;
				humName += "_Humidity";
				luaTable.AddNumber(humName, thisDeviceHum);
			}
			if (thisDeviceBaro != 0) {
				std::string baroName = item.devname;
				baroName += "_Barometer";
				luaTable.AddNumber(baroName, thisDeviceBaro);
			}
			if (thisDeviceUtility != 0) {
				std::string utilityName = item.devname;
				utilityName += "_Utility";
				luaTable.AddNumber(utilityName, thisDeviceUtility);
			}
			if (thisDeviceWeather != 0) {
				std::string weatherName = item.devname;
				weatherName += "_Weather";
				luaTable.AddNumber(weatherName, thisDeviceWeather);
			}
			if (thisDeviceRain != 0)
			{
				std::string tempName = item.devname;
				tempName += "_Rain";
				luaTable.AddNumber(tempName, thisDeviceRain);
			}
			if (thisDeviceRainLastHour != 0)
			{
				std::string tempName = item.devname;
				tempName += "_RainLastHour";
				luaTable.AddNumber(tempName, thisDeviceRainLastHour);
			}
			if (thisDeviceUV != 0)
			{
				std::string tempName = item.devname;
				tempName += "_UV";
				luaTable.AddNumber(tempName, thisDeviceUV);
			}
			if (thisZwaveAlarm != 0) {
				std::string alarmName = item.devname;
				alarmName += "_ZWaveAlarm";
				luaTable.AddNumber(alarmName, thisZwaveAlarm);
			}
			luaTable.Publish();

			// BEGIN OTO: populate changed info
			luaTable.InitTable(lua_state, "devicechanged_ext", 3, 0);
			luaTable.AddInteger("idx", item.id);
			luaTable.AddString("svalue", item.sValue);
			luaTable.AddInteger("nvalue", item.nValue);

			/* USELESS, WE HAVE THE DEVICE INDEX
			// replace devicechanged =>
			luaTable.AddInteger("name", nValue);
			*/
			luaTable.Publish();
			// END OTO
		}
	}

	ExportDeviceStatesToLua(lua_state, item);

	boost::shared_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);

	CLuaTable luaTable(lua_state, "uservariables", (int)m_uservariables.size(), 0);

	for (const auto &uservar : m_uservariables)
	{
		_tUserVariable uvitem = uservar.second;
		if (uvitem.variableType == 0) {
			//Integer
			luaTable.AddInteger(uvitem.variableName, atoi(uvitem.variableValue.c_str()));
		}
		else if (uvitem.variableType == 1) {
			//Float
			luaTable.AddNumber(uvitem.variableName, atof(uvitem.variableValue.c_str()));
		}
		else {
			//String,Date,Time
			luaTable.AddString(uvitem.variableName, uvitem.variableValue);
		}
	}
	luaTable.Publish();

	luaTable.InitTable(lua_state, "uservariables_lastupdate", (int)m_uservariables.size(), 0);

	for (const auto &uservar : m_uservariables)
	{
		_tUserVariable uvitem = uservar.second;
		luaTable.AddString(uvitem.variableName, uvitem.lastUpdate);
	}
	luaTable.Publish();

	if (item.reason == REASON_USERVARIABLE) {
		if (item.id > 0) {
			for (const auto &uservar : m_uservariables)
			{
				_tUserVariable uvitem = uservar.second;
				if (uvitem.ID == item.id) {
					luaTable.InitTable(lua_state, "uservariablechanged", 1, 0);
					luaTable.AddString(uvitem.variableName, uvitem.variableValue);
					luaTable.Publish();
				}
			}
		}
	}
	uservariablesMutexLock.unlock();

	boost::shared_lock<boost::shared_mutex> scenesgroupsMutexLock(m_scenesgroupsMutex);
	luaTable.InitTable(lua_state, "otherdevices_scenesgroups", (int)m_scenesgroups.size(), 0);
	for (const auto &group : m_scenesgroups)
	{
		_tScenesGroups sgitem = group.second;
		luaTable.AddString(sgitem.scenesgroupName, sgitem.scenesgroupValue);
	}
	luaTable.Publish();

	luaTable.InitTable(lua_state, "otherdevices_scenesgroups_idx", (int)m_scenesgroups.size(), 0);
	for (const auto &group : m_scenesgroups)
	{
		_tScenesGroups sgitem = group.second;
		luaTable.AddInteger(sgitem.scenesgroupName, sgitem.ID);
	}
	luaTable.Publish();
	scenesgroupsMutexLock.unlock();

	luaTable.InitTable(lua_state, "globalvariables", 0, 0);
	luaTable.AddString("Security", m_szSecStatus[secStatus]);
	luaTable.Publish();

	if (item.reason == REASON_NOTIFICATION)
	{
		luaTable.InitTable(lua_state, "notification", 0, 0);
		luaTable.AddInteger("id", item.id);
		luaTable.AddString("type", m_mainworker.m_notificationsystem.GetTypeString(item.nValue));
		luaTable.AddString("status", m_mainworker.m_notificationsystem.GetStatusString(item.lastLevel));
		luaTable.AddString("message", item.sValue);
		luaTable.Publish();
	}
}

void CEventSystem::EvaluateLua(const _tEventQueue &item, const std::string &filename, const std::string &LuaString)
{
	std::vector<_tEventQueue> items;
	items.push_back(item);
	EvaluateLua(items, filename, LuaString);
}

void CEventSystem::EvaluateLua(const std::vector<_tEventQueue> &items, const std::string &filename, const std::string &LuaString)
{
	std::lock_guard<std::mutex> l(luaMutex);

	lua_State *lua_state;
	lua_state = luaL_newstate();

	// load Lua libraries
	static const luaL_Reg lualibs[] = {
		{ "base", luaopen_base },     { "io", luaopen_io },	{ "table", luaopen_table },
		{ "string", luaopen_string }, { "math", luaopen_math }, { nullptr, nullptr },
	};

	const luaL_Reg *lib = lualibs;
	for (; lib->func != nullptr; lib++)
	{
		lib->func(lua_state);
		lua_settop(lua_state, 0);
	}

	lua_pushcfunction(lua_state, l_domoticz_applyJsonPath);
	lua_setglobal(lua_state, "domoticz_applyJsonPath");

	lua_pushcfunction(lua_state, l_domoticz_applyXPath);
	lua_setglobal(lua_state, "domoticz_applyXPath");

#ifdef _DEBUG
	_log.Log(LOG_STATUS, "EventSystem: script %s trigger (%s)", m_szReason[items[0].reason].c_str(), filename.c_str());
#endif

	int sunTimers[10];
	if (m_mainworker.m_SunRiseSetMins.size() == 10)
	{
		for (int i = 0; i < 10; i++)
			sunTimers[i] = m_mainworker.m_SunRiseSetMins[i];
	}

	// Do not correct for DST change - we only need this to compare with intRise and intSet which aren't as well
	time_t now = mytime(nullptr);
	struct tm ltime;
	localtime_r(&now, &ltime);
	int minutesSinceMidnight = (ltime.tm_hour * 60) + ltime.tm_min;

	bool dayTimeBool = false;
	bool nightTimeBool = false;
	if (sunTimers[0] == 0 && sunTimers[1] == 0) {
		if (sunTimers[9] == 0)
			nightTimeBool = true; // Sun below horizon in the space of 24 hours
		else
			dayTimeBool = true; // Sun above horizon in the space of 24 hours
	}
	else if ((minutesSinceMidnight >= sunTimers[0]) && (minutesSinceMidnight < sunTimers[1])) {
		dayTimeBool = true;
	}
	else {
		nightTimeBool = true;
	}

	bool civilDaytTime = false;
	bool civilNightTime = false;
	if ((minutesSinceMidnight >= sunTimers[3]) && (minutesSinceMidnight < sunTimers[4])) {
		civilDaytTime = true;
	}
	else {
		civilNightTime = true;
	}

	CLuaTable luaTable(lua_state, "timeofday", 4, 0);
	luaTable.AddBool("Daytime", dayTimeBool);
	luaTable.AddBool("Nighttime", nightTimeBool);
	luaTable.AddBool("Civildaytime", civilDaytTime);
	luaTable.AddBool("Civilnighttime", civilNightTime);
	luaTable.AddInteger("SunriseInMinutes", sunTimers[0]);
	luaTable.AddInteger("SunsetInMinutes", sunTimers[1]);
	luaTable.AddInteger("SunAtSouthInMinutes", sunTimers[2]);
	luaTable.AddInteger("CivTwilightStartInMinutes", sunTimers[3]);
	luaTable.AddInteger("CivTwilightEndInMinutes", sunTimers[4]);
	luaTable.AddInteger("NautTwilightStartInMinutes", sunTimers[5]);
	luaTable.AddInteger("NautTwilightEndInMinutes", sunTimers[6]);
	luaTable.AddInteger("AstrTwilightStartInMinutes", sunTimers[7]);
	luaTable.AddInteger("AstrTwilightEndInMinutes", sunTimers[8]);
	luaTable.AddInteger("DayLengthInMinutes", sunTimers[9]);
	luaTable.Publish();

	int secstatus = 0;
	m_sql.GetPreferencesVar("SecStatus", secstatus);
	CdzVents* dzvents = CdzVents::GetInstance();
	if (!m_sql.m_bDisableDzVentsSystem && filename == dzvents->m_runtimeDir + "dzVents.lua")
		dzvents->EvaluateDzVents(lua_state, items, secstatus);
	else
		EvaluateLuaClassic(lua_state, items[0], secstatus);

	int status = 0;
	if (LuaString.length() == 0)
		status = luaL_loadfile(lua_state, filename.c_str());
	else
		status = luaL_loadstring(lua_state, LuaString.c_str());

	if (status == 0)
	{
		lua_sethook(lua_state, luaStop, LUA_MASKCOUNT, 10000000);

		boost::thread luaThread(boost::bind(&CEventSystem::luaThread, this, lua_state, filename));
		SetThreadName(luaThread.native_handle(), "luaThread");

		if (!luaThread.timed_join(boost::posix_time::seconds(10)))
		{
			_log.Log(LOG_ERROR, "EventSystem: Warning!, lua script %s has been running for more than 10 seconds", filename.c_str());
		}
		else
		{
			//_log.Log(LOG_ERROR, "EventSystem: lua script completed");
		}
	}
	else
	{
		report_errors(lua_state, status, filename);
		lua_close(lua_state);
		return;
	}

	/*
	if (status == 0)
	{
		lua_sethook(lua_state, luaStop, LUA_MASKCOUNT, 10000000);
		status = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
	}

	report_errors(lua_state, status);

	bool scriptTrue = false;
	lua_getglobal(lua_state, "commandArray");
	if (lua_istable(lua_state, -1))
	{
		int tIndex = lua_gettop(lua_state);
		scriptTrue = iterateLuaTable(lua_state, tIndex, filename);
	}
	else
	{
		if (status == 0)
		{
			_log.Log(LOG_ERROR, "EventSystem: Lua script did not return a commandArray");
		}
	}

	if (scriptTrue)
	{
		_log.Log(LOG_STATUS, "EventSystem: Script event triggered: %s", filename.c_str());
	}
	lua_close(lua_state);
	*/
}

void CEventSystem::luaThread(lua_State *lua_state, const std::string &filename)
{
	int status;
	status = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
	report_errors(lua_state, status, filename);

	bool scriptTrue = false;
	lua_getglobal(lua_state, "commandArray");
	if (lua_istable(lua_state, -1))
	{
		int tIndex = lua_gettop(lua_state);
		scriptTrue = iterateLuaTable(lua_state, tIndex, filename);
	}
	else
	{
		if (status == 0)
		{
			_log.Log(LOG_ERROR, "EventSystem: Lua script %s did not return a commandArray", filename.c_str());
		}
	}

	if (scriptTrue)
	{
		if (m_sql.m_bLogEventScriptTrigger)
			_log.Log(LOG_STATUS, "EventSystem: Script event triggered: %s", filename.c_str());
	}

	lua_close(lua_state);

}

void CEventSystem::luaStop(lua_State *L, lua_Debug *ar)
{
	if (ar->event == LUA_HOOKCOUNT)
	{
		(void)ar;  /* unused arg. */
		lua_sethook(L, nullptr, 0, 0);
		luaL_error(L, "Lua script execution exceeds maximum number of lines");
		lua_close(L);
	}
}

bool CEventSystem::iterateLuaTable(lua_State *lua_state, const int tIndex, const std::string &filename)
{
	CdzVents* dzvents = CdzVents::GetInstance();
	bool scriptTrue = false;
	lua_pushnil(lua_state); // first key
	while (lua_next(lua_state, tIndex) != 0)
	{
		if ((std::string(luaL_typename(lua_state, -2)) == "string") && (std::string(luaL_typename(lua_state, -1)) == "string"))
		{
			scriptTrue = processLuaCommand(lua_state, filename);
		}
		else if ((std::string(luaL_typename(lua_state, -2)) == "number") && lua_istable(lua_state, -1))
		{
			scriptTrue = iterateLuaTable(lua_state, tIndex + 2, filename);
		}
		else if (!m_sql.m_bDisableDzVentsSystem && lua_istable(lua_state, -1) && std::string(luaL_typename(lua_state, -2)) == "string")
		{
			scriptTrue = dzvents->processLuaCommand(lua_state, filename, tIndex);
		}
		else
		{
			_log.Log(LOG_ERROR, "EventSystem: commandArray in script %s should only return ['string']='actionstring' or [integer]={['string']='actionstring'}", filename.c_str());
		}
		// removes 'value'; keeps 'key' for next iteration
		lua_pop(lua_state, 1);
	}

	return scriptTrue;

}

bool CEventSystem::processLuaCommand(lua_State *lua_state, const std::string &filename)
{
	bool scriptTrue = false;
	std::string lCommand = std::string(lua_tostring(lua_state, -2));
	if (lCommand == "SendNotification") {
		std::string luaString = lua_tostring(lua_state, -1);
		std::string subject, body, priority("0"), sound, subsystem;
		std::string extraData;
		std::vector<std::string> aParam;
		StringSplit(luaString, "#", aParam);
		subject = body = aParam[0];
		if (aParam.size() > 1) {
			if (!aParam[1].empty())
				body = aParam[1];
		}
		if (aParam.size() > 2) {
			priority = aParam[2];
		}
		if (aParam.size() > 3) {
			sound = aParam[3];
		}
		if (aParam.size() > 4) {
			if (aParam[4].find("midx_") != std::string::npos) {
				extraData = aParam[4];
			}
			else {
				extraData = "|Device=" + aParam[4];
			}
		}
		if (aParam.size() > 5) {
			subsystem = aParam[5];
		}

		m_sql.AddTaskItem(_tTaskItem::SendNotification(0, subject, body, extraData, atoi(priority.c_str()), sound, subsystem));
		scriptTrue = true;
	}
	else if (lCommand == "SendEmail") {
		std::string luaString = lua_tostring(lua_state, -1);
		std::string subject, body, to;
		std::vector<std::string> aParam;
		StringSplit(luaString, "#", aParam);
		if (aParam.size() != 3)
		{
			//Invalid
			_log.Log(LOG_ERROR, "EventSystem: SendEmail, not enough parameters!");
			return false;
		}
		subject = aParam[0];
		body = aParam[1];
		stdreplace(body, "\\n", "<br>");
		to = aParam[2];
		m_sql.AddTaskItem(_tTaskItem::SendEmailTo(1, subject, body, to));
		scriptTrue = true;
	}
	else if (lCommand == "SendSMS") {
		std::string luaString = lua_tostring(lua_state, -1);
		if (luaString.empty())
		{
			//Invalid
			_log.Log(LOG_ERROR, "EventSystem: SendSMS, not enough parameters!");
			return false;
		}
		m_sql.AddTaskItem(_tTaskItem::SendSMS(1, luaString));
		scriptTrue = true;
	}
	else if (lCommand == "TriggerIFTTT")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		std::vector<std::string> aParam;
		StringSplit(luaString, "#", aParam);
		if ((aParam.empty()) || (aParam.size() > 4))
		{
			//Invalid
			_log.Log(LOG_ERROR, "EventSystem: TriggerIFTTT, not enough parameters!");
			return false;
		}
		std::string sID = ParseBlocklyString(aParam[0]);
		std::string sValue1, sValue2, sValue3;
		if (aParam.size() > 1)
			sValue1 = ParseBlocklyString(aParam[1]);
		if (aParam.size() > 2)
			sValue2 = ParseBlocklyString(aParam[2]);
		if (aParam.size() > 3)
			sValue3 = ParseBlocklyString(aParam[3]);
		m_sql.AddTaskItem(_tTaskItem::SendIFTTTTrigger(1, sID, sValue1, sValue2, sValue3));
		scriptTrue = true;
	}
	else if (lCommand == "OpenURL")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		_tActionParseResults parseResult;
		parseResult.fAfterSec = 0.2F;
		ParseActionString(luaString, parseResult);
		OpenURL(parseResult.fAfterSec, parseResult.sCommand);
		scriptTrue = true;
	}
	else if (lCommand == "UpdateDevice")
	{
		std::vector<std::string> strarray;
		StringSplit(lua_tostring(lua_state, -1), "|", strarray);
		if (strarray.size() < 2 || strarray.size() > 4)
		{
			_log.Log(LOG_ERROR, "EventSystem: UpdateDevice, invalid parameters!");
			return false;
		}
		int nValue = 0;
		std::string sValue;
		int idx = std::stoi(strarray[0]);
		if (strarray.size() > 1 && !strarray[1].empty())
			nValue = atoi(strarray[1].c_str());
		if (strarray.size() > 2 && !strarray[2].empty())
			sValue = strarray[2];
		//if (strarray.size() > 3 && !strarray[3].empty())
			//Protected = atoi(strarray[3].c_str()); //GizMoCuz: this should not be able to be changed via events!

		m_mainworker.UpdateDevice(idx, nValue, sValue, "EventSystem/" + filename, 12, 255, false);
		scriptTrue = true;
	}
	else if (lCommand.find("Variable:") == 0)
	{
		std::string variableName = lCommand.substr(9);
		std::string variableValue = lua_tostring(lua_state, -1);

		std::vector<std::vector<std::string> > result;

		_tActionParseResults parseResult;
		parseResult.fAfterSec = 0;
		ParseActionString(variableValue, parseResult);

		result = m_sql.safe_query("SELECT ID, ValueType FROM UserVariables WHERE (Name == '%q')", variableName.c_str());
		if (!result.empty())
		{
			std::vector<std::string> sd = result[0];
			variableValue = ProcessVariableArgument(parseResult.sCommand);

			if (parseResult.fAfterSec < (1. / timer_resolution_hz / 2))
			{
				std::string errorMessage;
				if (!m_sql.UpdateUserVariable(sd[0], variableName, (const _eUsrVariableType)atoi(sd[1].c_str()), variableValue, false, errorMessage))
				{
					_log.Log(LOG_ERROR, "EventSystem: Error updating variable %s: %s", variableName.c_str(), errorMessage.c_str());
				}
			}
			else
			{
				uint64_t idx = std::stoull(sd[0]);
				m_sql.AddTaskItem(_tTaskItem::SetVariable(parseResult.fAfterSec, idx, variableValue, false));
			}
			scriptTrue = true;
		}
	}
	else if (lCommand.find("SetSetPoint:") == 0)
	{
		std::string SetPointIdx = lCommand.substr(12);
		std::string luaString = lua_tostring(lua_state, -1);
		std::string temp, mode, until;
		std::vector<std::string> aParam;
		int idx;
		StringSplit(luaString, "#", aParam);
		switch (aParam.size()) {
		case 3:
			until = aParam[2];
		case 2:
			mode = aParam[1];
		case 1:
			idx = atoi(SetPointIdx.c_str());
			temp = aParam[0];
			m_sql.AddTaskItem(_tTaskItem::SetSetPoint(0.5F, idx, temp, mode, until));
			break;

		default:
			//Invalid
			_log.Log(LOG_ERROR, "EventSystem: SetPoint, incorrect parameters!");
			return false;
		}
	}
	else if (lCommand.find("CustomCommand:") == 0)
	{
		int idx = atoi(lCommand.substr(14).c_str());
		std::string luaString = lua_tostring(lua_state, -1);
		_tActionParseResults parseResult;
		parseResult.fAfterSec = 0;
		ParseActionString(luaString, parseResult);
		m_sql.AddTaskItem(_tTaskItem::CustomCommand(parseResult.fAfterSec, idx, luaString));
	}
	else
	{
		if (ScheduleEvent(lua_tostring(lua_state, -2), lua_tostring(lua_state, -1), filename)) {
			scriptTrue = true;
		}
	}
	return scriptTrue;
}

void CEventSystem::report_errors(lua_State *L, int status, const std::string &filename)
{
	if (status != 0) {
		_log.Log(LOG_ERROR, "EventSystem: in %s: %s", filename.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1); // remove error message
	}
}

bool CEventSystem::CustomCommand(const uint64_t idx, const std::string &sCommand)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT H.ID FROM DeviceStatus DS, Hardware H WHERE (DS.ID=='%u') AND (DS.HardwareID == H.ID)", idx);
	if (result.size() != 1)
		return false;

	int HardwareID = atoi(result[0][0].c_str());
	CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(HardwareID);
	if (!pHardware)
		return false;

	return pHardware->CustomCommand(idx, sCommand);
}

void CEventSystem::OpenURL(const float delay, const std::string &URL)
{
	if (m_sql.m_bEnableEventSystemFullURLLog)
	{
		if (!delay)
			_log.Log(LOG_STATUS, "EventSystem: Fetching URL %s...", URL.c_str());
		else
			_log.Log(LOG_STATUS, "EventSystem: Fetching URL %s after %.1f seconds...", URL.c_str(), delay);
	}
	else
	{
		if (!delay)
			_log.Log(LOG_STATUS, "EventSystem: Opening a URL...");
		else
			_log.Log(LOG_STATUS, "EventSystem: Opening a URL after %.1f seconds...", delay);
	}

	m_sql.AddTaskItem(_tTaskItem::GetHTTPPage(delay, URL, "OpenURL"));
	// maybe do something with sResult in the future.
}

void CEventSystem::WriteToLog(const std::string &devNameNoQuotes, const std::string &doWhat)
{
	if (devNameNoQuotes == "WriteToLogText")
	{
		_log.Log(LOG_STATUS, "%s", ParseBlocklyString(doWhat).c_str());
	}
	else if (devNameNoQuotes == "WriteToLogUserVariable")
	{
		_log.Log(LOG_STATUS, "%s", m_uservariables[atoi(doWhat.c_str())].variableValue.c_str());
	}
	else if (devNameNoQuotes == "WriteToLogDeviceVariable")
	{
		boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
		int devIdx = atoi(doWhat.c_str());
		if (m_devicestates[devIdx].devType == pTypeHUM)
		{
			//nValue devices
			_log.Log(LOG_STATUS, "%d", m_devicestates[devIdx].nValue);
		}
		else
		{
			_log.Log(LOG_STATUS, "%s", m_devicestates[devIdx].sValue.c_str());
		}
	}
	else if (devNameNoQuotes == "WriteToLogSwitch")
	{
		boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
		_log.Log(LOG_STATUS, "%s", m_devicestates[atoi(doWhat.c_str())].nValueWording.c_str());
	}
}

bool CEventSystem::ScheduleEvent(std::string deviceName, const std::string &Action, const std::string &eventName)
{
	std::vector<std::vector<std::string> > result;
	bool isScene = false;
	int sceneType = 0;

	if ((deviceName.find("Scene:") == 0) || (deviceName.find("Group:") == 0))
	{
		isScene = true;
		sceneType = 1;
		if (deviceName.find("Group:") == 0) {
			sceneType = 2;
		}
		deviceName = deviceName.substr(6);
	}
	else if (deviceName.find("SendCamera:") == 0)
	{
		deviceName = deviceName.substr(11);
		result = m_sql.safe_query("SELECT Name FROM Cameras WHERE (ID == '%q')", deviceName.c_str());
		if (result.empty())
			return false;

		_tActionParseResults parseResult;
		parseResult.fAfterSec = 0;
		ParseActionString(Action, parseResult);
		StripQuotes(parseResult.sCommand);

		std::string subject = parseResult.sCommand;
		if (parseResult.fAfterSec < (1. / timer_resolution_hz / 2))
		{
			m_mainworker.m_cameras.EmailCameraSnapshot(deviceName, subject);
		}
		else
			m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(parseResult.fAfterSec, deviceName, subject));
		return true;
	}

	if (isScene) {
		result = m_sql.safe_query("SELECT ID FROM Scenes WHERE (Name == '%q')",
			deviceName.c_str());
	}
	else {
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (Name == '%q')",
			deviceName.c_str());
	}
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];
		int idx = atoi(sd[0].c_str());
		return (ScheduleEvent(idx, Action, isScene, eventName, sceneType));
	}

	return false;
}

bool CEventSystem::ScheduleEvent(int deviceID, const std::string &Action, bool isScene, const std::string &eventName, int sceneType)
{
	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
	std::string previousState = m_devicestates[deviceID].nValueWording;
	int previousLevel = calculateDimLevel(deviceID, m_devicestates[deviceID].lastLevel);
	int level = 0;
	devicestatesMutexLock.unlock();

	_tActionParseResults oParseResults;
	oParseResults.bEventTrigger = true;

	ParseActionString(Action, oParseResults);

	if (oParseResults.sCommand.substr(0, 9) == "Set Level") {
		level = calculateDimLevel(deviceID, atoi(oParseResults.sCommand.substr(10).c_str()));
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 9);
	}
	else if (oParseResults.sCommand.substr(0, 10) == "Set Volume") {
		level = atoi(oParseResults.sCommand.substr(11).c_str());
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 10);
	}
	else if (oParseResults.sCommand.substr(0, 13) == "Play Playlist") {
		std::string	sParams = oParseResults.sCommand.substr(14);

		CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_Kodi);
		if (pBaseHardware != nullptr)
		{
			CKodi			*pHardware = reinterpret_cast<CKodi*>(pBaseHardware);
			std::string		sPlayList = sParams;
			size_t			iLastSpace = sParams.find_last_of(' ', sParams.length());

			if (iLastSpace != std::string::npos)
			{
				sPlayList = sParams.substr(0, iLastSpace);
				level = atoi(sParams.substr(iLastSpace).c_str());
			}
			if (!pHardware->SetPlaylist(deviceID, sPlayList))
			{
				pBaseHardware = nullptr; // Kodi hardware exists, but the device for the event is not a Kodi
			}
		}

		if (pBaseHardware == nullptr) // if not handled try Logitech
		{
			pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_LogitechMediaServer);
			if (pBaseHardware == nullptr)
				return false;
			CLogitechMediaServer *pHardware = reinterpret_cast<CLogitechMediaServer*>(pBaseHardware);

			int iPlaylistID = pHardware->GetPlaylistRefID(oParseResults.sCommand.substr(14));
			if (iPlaylistID == 0) return false;

			level = iPlaylistID;
		}
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 13);
	}
	else if (oParseResults.sCommand.substr(0, 14) == "Play Favorites") {
		std::string	sParams = oParseResults.sCommand.substr(15);
		CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_Kodi);
		if (pBaseHardware != nullptr)
		{
			//CKodi			*pHardware = reinterpret_cast<CKodi*>(pBaseHardware);
			if (sParams.length() > 0)
			{
				level = atoi(sParams.c_str());
			}
		}
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 14);
	}
	else if (oParseResults.sCommand.substr(0, 7) == "Execute") {
		std::string	sParams = oParseResults.sCommand.substr(8);
		CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_Kodi);
		if (pBaseHardware != nullptr)
		{
			CKodi	*pHardware = reinterpret_cast<CKodi*>(pBaseHardware);
			pHardware->SetExecuteCommand(deviceID, sParams);
		}
	}

	if (previousState.substr(0, 9) == "Set Level" || previousState.substr(0, 5) == "Level") {
		previousState = "Set Level";
	}
	else if (previousState.substr(0, 10) == "Set Volume") {
		previousState = previousState.substr(0, 10);
	}

	int iDeviceDelay = 0;
	if (!isScene) {
		// Get Device details, check for switch global OnDelay/OffDelay (stored in AddjValue2/AddjValue).
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT SwitchType, AddjValue2 FROM DeviceStatus WHERE (ID == %d)", deviceID);
		if (result.empty())
		{
			return false;
		}

		std::vector<std::string> sd = result[0];
		_eSwitchType switchtype = (_eSwitchType)atoi(sd[0].c_str());
		int iOnDelay = atoi(sd[1].c_str());

		bool bIsOn = IsLightSwitchOn(oParseResults.sCommand);
		if (switchtype == STYPE_Selector) {
			bIsOn = (level > 0) ? true : false;
		}
		iDeviceDelay = bIsOn ? iOnDelay : 0;
	}

	float fPreviousRandomTime = 0;
	for (int iIndex = 0; iIndex < abs(oParseResults.iRepeat); iIndex++) {
		_tTaskItem tItem;

		float fRandomTime = 0;
		if (oParseResults.fRandomSec > (1. / timer_resolution_hz / 2))
			fRandomTime = static_cast<float>(GenerateRandomNumber(static_cast<int>(oParseResults.fRandomSec)));

		float fDelayTime = oParseResults.fAfterSec + fPreviousRandomTime + fRandomTime + iDeviceDelay + (iIndex * oParseResults.fForSec) + (iIndex * oParseResults.fRepeatSec);
		fPreviousRandomTime = fRandomTime;

		if (!oParseResults.bEventTrigger)
			SetEventTrigger(deviceID, (!isScene ? REASON_DEVICE : REASON_SCENEGROUP), fDelayTime);

		if (isScene) {

			if (
				oParseResults.sCommand == "On"
				|| oParseResults.sCommand == "Off"
				) {
				tItem = _tTaskItem::SwitchSceneEvent(fDelayTime, deviceID, oParseResults.sCommand, eventName, "EventSystem/" + eventName);
			}
			else if (oParseResults.sCommand == "Active") {
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("UPDATE SceneTimers SET Active=1 WHERE (SceneRowID == %d)", deviceID);
				m_mainworker.m_scheduler.ReloadSchedules();
			}
			else if (oParseResults.sCommand == "Inactive") {
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("UPDATE SceneTimers SET Active=0 WHERE (SceneRowID == %d)", deviceID);
				m_mainworker.m_scheduler.ReloadSchedules();
			}

		}
		else {
			tItem = _tTaskItem::SwitchLightEvent(fDelayTime, deviceID, oParseResults.sCommand, level, NoColor, eventName, "EventSystem/" + eventName);
		}
		m_sql.AddTaskItem(tItem);
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "EventSystem: Scheduled %s after %0.2f.", tItem._command.c_str(), tItem._DelayTime);
#endif

		if (
			oParseResults.fForSec > (1. / timer_resolution_hz / 2)
			&& (
				oParseResults.iRepeat > 0
				|| iIndex < abs(oParseResults.iRepeat) - 1
				)
			) {
			fDelayTime += oParseResults.fForSec;

			if (!oParseResults.bEventTrigger)
				SetEventTrigger(deviceID, (!isScene ? REASON_DEVICE : REASON_SCENEGROUP), fDelayTime);

			_tTaskItem tDelayedtItem;
			if (isScene) {
				if (oParseResults.sCommand == "On") {
					tDelayedtItem = _tTaskItem::SwitchSceneEvent(fDelayTime, deviceID, "Off", eventName, "EventSystem/" + eventName);
				}
				else if (oParseResults.sCommand == "Off") {
					tDelayedtItem = _tTaskItem::SwitchSceneEvent(fDelayTime, deviceID, "On", eventName, "EventSystem/" + eventName);
				}
			}
			else {
				tDelayedtItem = _tTaskItem::SwitchLightEvent(fDelayTime, deviceID, previousState, previousLevel, NoColor, eventName, "EventSystem/" + eventName);
			}
			m_sql.AddTaskItem(tDelayedtItem);
#ifdef _DEBUG
			_log.Log(LOG_STATUS, "EventSystem: Scheduled %s after %0.2f.", tDelayedtItem._command.c_str(), tDelayedtItem._DelayTime);
#endif
		}
	}
	return true;
}


std::string CEventSystem::nValueToWording(const uint8_t dType, const uint8_t dSubType, const _eSwitchType switchtype, const int nValue, const std::string &sValue, const std::map<std::string, std::string> & options)
{

	std::string lstatus;
	int llevel = 0;
	bool bHaveDimmer = false;
	bool bHaveGroupCmd = false;
	int maxDimLevel = 0;
	GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
	/*
		if (lstatus.find("Set Level") == 0)
		{
			lstatus = "Set Level";
		}
	*/
	if (switchtype == STYPE_Dimmer)
	{
		// use default lstatus
	}
	else if (((dType == pTypeGeneral) && (dSubType == sTypeCounterIncremental)) ||
		(dType == pTypeRFXMeter))
	{
		lstatus = sValue;
	}
	else if (dType == pTypeHUM)
	{
		lstatus = std::to_string(nValue);
	}
	else if (switchtype == STYPE_Selector)
	{
		std::map<std::string, std::string> statuses;
		GetSelectorSwitchStatuses(options, statuses);
		lstatus = statuses[std::to_string(llevel)];
	}
	else if ((switchtype == STYPE_Contact) || (switchtype == STYPE_DoorContact))
	{
		bool bIsOn = IsLightSwitchOn(lstatus);
		if (bIsOn)
		{
			lstatus = "Open";
		}
		else if (lstatus == "Off")
		{
			lstatus = "Closed";
		}
	}
	else if (switchtype == STYPE_DoorLock)
	{
		bool bIsOn = IsLightSwitchOn(lstatus);
		if (bIsOn)
		{
			lstatus = "Locked";
		}
		else if (lstatus == "Off")
		{
			lstatus = "Unlocked";
		}
	}
	else if (switchtype == STYPE_DoorLockInverted)
	{
		bool bIsOn = IsLightSwitchOn(lstatus);
		if (bIsOn)
		{
			lstatus = "Unlocked";
		}
		else if (lstatus == "Off")
		{
			lstatus = "Locked";
		}
	}
	else if (switchtype == STYPE_Blinds)
	{
		if (lstatus == "On")
		{
			lstatus = "Closed";
		}
		else if (lstatus == "Stop")
		{
			lstatus = "Stopped";
		}
		else
		{
			lstatus = "Open";
		}
	}
	else if (switchtype == STYPE_BlindsInverted)
	{
		if (lstatus == "Off")
		{
			lstatus = "Closed";
		}
		else if (lstatus == "Stop")
		{
			lstatus = "Stopped";
		}
		else
		{
			lstatus = "Open";
		}
	}
	else if (switchtype == STYPE_BlindsPercentage)
	{
		if (lstatus == "On")
		{
			lstatus = "Closed";
		}
		else if (lstatus == "Off")
		{
			lstatus = "Open";
		}
	}
	else if (switchtype == STYPE_BlindsPercentageInverted)
	{
		if (lstatus == "On")
		{
			lstatus = "Open";
		}
		else if (lstatus == "Off")
		{
			lstatus = "Closed";
		}
	}
	else if (switchtype == STYPE_Media)
	{
		lstatus = Media_Player_States((const _eMediaStatus)nValue);
	}
	else if (lstatus.empty())
	{
		lstatus = sValue;
		//OJO if lstatus  is still empty we use nValue for lstatus. ss for conversion
		if (lstatus.empty())
		{
			lstatus = std::to_string(nValue);
		}
	}
	return lstatus;
}

int CEventSystem::l_domoticz_print(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);

	for (int i = 1; i <= nargs; i++)
	{
		if (lua_isstring(lua_state, i))
		{
			std::string lstring = lua_tostring(lua_state, i);
			if (lstring.find("Error: ") != std::string::npos)
			{
				_log.Log(LOG_ERROR, "LUA: %s", lstring.c_str());
			}
			else
			{
				_log.Log(LOG_STATUS, "LUA: %s", lstring.c_str());
			}
		}
		else
		{
			/* non strings? */
		}
	}
	return 0;
}

void CEventSystem::reportMissingDevice(const int deviceID, const _tEventItem &item)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID == %d)", deviceID);
	if (!result.empty())
	{
		_log.Log(LOG_ERROR, "EventSystem: Device ('%s', ID=%d) used in event '%s' not found, make sure that it's hardware is not disabled!", result[0][0].c_str(), deviceID, item.Name.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR, "EventSystem: Device no. '%d' used in event '%s' no longer exists, disabling event!", deviceID, item.Name.c_str());

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT EventMaster.ID FROM EventMaster INNER JOIN EventRules ON EventRules.EMID=EventMaster.ID WHERE (EventRules.ID == '%" PRIu64 "')",
			item.ID);
		if (!result.empty())
		{
			for (const auto &sd : result)
			{
				int eventStatus = 2;
				m_sql.safe_query("UPDATE EventMaster SET Status = %d WHERE (ID == '%q')",
					eventStatus, sd[0].c_str());
			}
		}
	}
}

void CEventSystem::WWWGetItemStates(std::vector<_tDeviceStatus> &iStates)
{
	if (!m_bEnabled)
		return;

	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	iStates.clear();
	std::transform(m_devicestates.begin(), m_devicestates.end(), std::back_inserter(iStates),
		       [](const std::pair<const uint64_t, CEventSystem::_tDeviceStatus> &m) { return m.second; });
}

int CEventSystem::getSunRiseSunSetMinutes(const std::string &what)
{
	if (!m_mainworker.m_SunRiseSetMins.empty())
	{
		int ordinalNum = 1; // Defaults to Sunset to keep compatibility with previous code
		if (what == "Sunrise") ordinalNum = 0;
		else if (what == "Sunset") ordinalNum = 1; // For clarity
		else if (what == "SunAtSouth") ordinalNum = 2;
		else if (what == "CivTwilightStart") ordinalNum = 3;
		else if (what == "CivTwilightEnd") ordinalNum = 4;
		else if (what == "NautTwilightStart") ordinalNum = 5;
		else if (what == "NautTwilightEnd") ordinalNum = 6;
		else if (what == "AstrTwilightStart") ordinalNum = 7;
		else if (what == "AstrTwilightEnd") ordinalNum = 8;
		else if (what == "DayLength") ordinalNum = 9;
		return m_mainworker.m_SunRiseSetMins[ordinalNum];
	}
	return 0;
}

bool CEventSystem::isEventscheduled(const std::string &eventName)
{
	std::vector<_tTaskItem> currentTasks;

	m_sql.EventsGetTaskItems(currentTasks);
	if (currentTasks.empty())
		return false;

	return std::any_of(currentTasks.begin(), currentTasks.end(),
			   [&](const _tTaskItem &currentTask) { return currentTask._relatedEvent == eventName; });
}

int CEventSystem::calculateDimLevel(int deviceID, int percentageLevel)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Type, SubType, SwitchType, Options FROM DeviceStatus WHERE (ID == %d)", deviceID);
	int iLevel = 0;
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];

		unsigned char dType = atoi(sd[0].c_str());
		unsigned char dSubType = atoi(sd[1].c_str());
		_eSwitchType switchtype = (_eSwitchType)atoi(sd[2].c_str());
		std::string lstatus;
		int llevel = 0;
		bool bHaveDimmer = false;
		bool bHaveGroupCmd = false;
		int maxDimLevel = 0;

		GetLightStatus(dType, dSubType, switchtype, 0, "", lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
		iLevel = maxDimLevel;

		if (maxDimLevel != 0)
		{
			if ((switchtype == STYPE_Dimmer) || (switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted))
			{
				float fLevel = (maxDimLevel / 100.0F) * percentageLevel;
				if (fLevel > 100)
					fLevel = 100;
				iLevel = int(fLevel);
			}
			else if (switchtype == STYPE_Selector)
			{
				// llevel cannot be get without sValue so level is getting from percentageLevel
				iLevel = percentageLevel;
				int maxLevel = 100;
				if (!sd[3].empty())
				{
					std::map<std::string, std::string> statuses;
					GetSelectorSwitchStatuses(m_sql.BuildDeviceOptions(sd[3]), statuses);
					maxLevel = (statuses.size() - 1) * 10;
				}
				if (iLevel > maxLevel)
					iLevel = maxLevel;
				else if (iLevel < 0)
					iLevel = 0;
			}
		}
	}
	return iLevel;
}

//Webserver helpers
namespace http {
	namespace server {
		struct _tSortedEventsInt
		{
			std::string ID;
			std::string eventstatus;
		};

		void CWebServer::EventCreate(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			Json::Value root;
			root["title"] = "CreateEvent";
			root["status"] = "ERR";

			redirect_uri = root.toStyledString();
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string eventname = HTMLSanitizer::Sanitize(CURLEncode::URLDecode(request::findValue(&req, "name")));
			if (eventname.empty())
				return;

			std::string interpreter = CURLEncode::URLDecode(request::findValue(&req, "interpreter"));
			if (interpreter.empty())
				return;

			std::string eventtype = CURLEncode::URLDecode(request::findValue(&req, "eventtype"));
			if (eventtype.empty())
				return;

			std::string eventxml = CURLEncode::URLDecode(request::findValue(&req, "xml"));
			if (eventxml.empty())
				return;

			std::string eventactive = CURLEncode::URLDecode(request::findValue(&req, "eventstatus"));
			if (eventactive.empty())
				return;

			std::string eventid = CURLEncode::URLDecode(request::findValue(&req, "eventid"));

			std::string eventlogic = CURLEncode::URLDecode(request::findValue(&req, "logicarray"));
			if ((interpreter == "Blockly") && (eventlogic.empty()))
				return;

			int eventStatus = atoi(eventactive.c_str());

			bool parsingSuccessful = eventxml.length() > 0;
			Json::Value jsonRoot;
			if (interpreter == "Blockly") {
				parsingSuccessful = ParseJSon(eventlogic, jsonRoot);
			}

			if (!parsingSuccessful)
			{
				_log.Log(LOG_ERROR, "Webserver event parser: Invalid data received!");
			}
			else {
				if (eventid.empty())
				{
					std::vector<std::vector<std::string> > result;
					m_sql.safe_query("INSERT INTO EventMaster (Name, Interpreter, Type, XMLStatement, Status) VALUES ('%q','%q','%q','%q','%d')",
						eventname.c_str(), interpreter.c_str(), eventtype.c_str(), eventxml.c_str(), eventStatus);
					result = m_sql.safe_query("SELECT ID FROM EventMaster WHERE (Name == '%q')",
						eventname.c_str());
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];
						eventid = sd[0];
					}
				}
				else {
					m_sql.safe_query("UPDATE EventMaster SET Name='%q', Interpreter='%q', Type='%q', XMLStatement ='%q', Status ='%d' WHERE (ID == '%q')",
						eventname.c_str(), interpreter.c_str(), eventtype.c_str(), eventxml.c_str(), eventStatus, eventid.c_str());
					m_sql.safe_query("DELETE FROM EventRules WHERE (EMID == '%q')",
						eventid.c_str());
				}

				if (eventid.empty())
				{
					//eventid should now never be empty!
					_log.Log(LOG_ERROR, "Error writing event actions to database!");
				}
				else {
					std::string sNewEditorTheme = CURLEncode::URLDecode(request::findValue(&req, "editortheme"));
					std::string sOldEditorTheme = "ace/theme/xcode";
					m_sql.GetPreferencesVar("ScriptEditorTheme", sOldEditorTheme);
					if (sNewEditorTheme.length() && (sNewEditorTheme != sOldEditorTheme))
					{
						m_sql.UpdatePreferencesVar("ScriptEditorTheme", sNewEditorTheme);
					}

					if (interpreter == "Blockly") {
						const Json::Value array = jsonRoot["eventlogic"];
						for (int index = 0; index < (int)array.size(); ++index)
						{
							std::string conditions = array[index].get("conditions", "").asString();
							std::string actions = array[index].get("actions", "").asString();

							if (
								(actions.find("SendNotification") != std::string::npos) ||
								(actions.find("SendEmail") != std::string::npos) ||
								(actions.find("SendSMS") != std::string::npos) ||
								(actions.find("TriggerIFTTT") != std::string::npos)
								)
							{
								stdreplace(actions, "$", "#");
							}
							int sequenceNo = index + 1;
							m_sql.safe_query("INSERT INTO EventRules (EMID, Conditions, Actions, SequenceNo) VALUES ('%q','%q','%q','%d')",
								eventid.c_str(), conditions.c_str(), actions.c_str(), sequenceNo);
						}
					}

					m_mainworker.m_eventsystem.LoadEvents();
					root["status"] = "OK";
				}
			}
			redirect_uri = root.toStyledString();
		}

		void CWebServer::RType_Events(WebEmSession & session, const request& req, Json::Value &root)
		{
			//root["status"]="OK";
			root["title"] = "Events";

			std::string cparam = request::findValue(&req, "param");
			if (cparam.empty())
			{
				cparam = request::findValue(&req, "dparam");
				if (cparam.empty())
				{
					return;
				}
			}

			std::vector<std::vector<std::string> > result;

			if (cparam == "list")
			{
				root["title"] = "ListEvents";
				root["status"] = "OK";
#ifdef ENABLE_PYTHON
				root["interpreters"] = "Blockly:Lua:dzVents:Python";
#else
				root["interpreters"] = "Blockly:Lua:dzVents";
#endif

				std::map<std::string, _tSortedEventsInt> _levents;
				result = m_sql.safe_query("SELECT ID, Name, XMLStatement, Status FROM EventMaster ORDER BY ID ASC");
				if (!result.empty())
				{
					for (const auto &sd : result)
					{
						std::string ID = sd[0];
						std::string Name = sd[1];
						std::string eventStatus = sd[3];
						_tSortedEventsInt eitem;
						eitem.ID = ID;
						eitem.eventstatus = eventStatus;
						if (_levents.find(Name) != _levents.end())
						{
							//Duplicate event name, add the ID
							std::stringstream szQuery;
							szQuery << Name << " (" << ID << ")";
							Name = szQuery.str();
						}
						_levents[Name] = eitem;
					}
					//return a sorted event list
					int ii = 0;
					for (const auto &event : _levents)
					{
						root["result"][ii]["name"] = event.first;
						root["result"][ii]["id"] = event.second.ID;
						root["result"][ii]["eventstatus"] = event.second.eventstatus;
						ii++;
					}
				}
			}
			else if (cparam == "new")
			{
				root["title"] = "NewEvent";

				std::string interpreter = request::findValue(&req, "interpreter");
				if (interpreter.empty())
					return;

				std::string eventType = request::findValue(&req, "eventtype");
				if (eventType.empty())
					return;

				std::stringstream template_file;
#ifdef WIN32
				template_file << szUserDataFolder << "scripts\\templates\\" << eventType << "." << interpreter;
#else
				template_file << szUserDataFolder << "scripts/templates/" << eventType << "." << interpreter;
#endif
				std::ifstream file;
				std::stringstream template_content;
				file.open(template_file.str().c_str(), std::ifstream::in);
				if (file.is_open())
				{
					template_content << file.rdbuf();
					file.close();
				}
				root["template"] = template_content.str();
				root["status"] = "OK";
			}
			else if (cparam == "load")
			{
				root["title"] = "LoadEvent";

				std::string idx = request::findValue(&req, "event");
				if (idx.empty())
					return;

				int ii = 0;

				std::string sEditorTheme = "ace/theme/xcode";
				m_sql.GetPreferencesVar("ScriptEditorTheme", sEditorTheme);
				root["editortheme"] = sEditorTheme;

				result = m_sql.safe_query("SELECT ID, Name, XMLStatement, Status, Interpreter, Type FROM EventMaster WHERE (ID=='%q')",
					idx.c_str());
				if (!result.empty())
				{
					for (const auto &sd : result)
					{
						std::string ID = sd[0];
						std::string Name = sd[1];
						std::string XMLStatement = sd[2];
						std::string eventStatus = sd[3];
						//int Status=atoi(sd[3].c_str());

						root["result"][ii]["id"] = ID;
						root["result"][ii]["name"] = Name;
						root["result"][ii]["xmlstatement"] = XMLStatement;
						root["result"][ii]["eventstatus"] = eventStatus;
						root["result"][ii]["interpreter"] = sd[4];
						root["result"][ii]["type"] = sd[5];
						ii++;
					}
					root["status"] = "OK";
				}
			}
			else if (cparam == "updatestatus")
			{
				std::string eventactive = request::findValue(&req, "eventstatus");
				if (eventactive.empty())
					return;

				std::string eventid = request::findValue(&req, "eventid");
				if (eventid.empty())
					return;

				m_sql.safe_query("UPDATE EventMaster SET Status ='%d' WHERE (ID == '%q')", atoi(eventactive.c_str()), eventid.c_str());
				m_mainworker.m_eventsystem.LoadEvents();
				root["status"] = "OK";

			}
			else if (cparam == "create")
			{

				root["title"] = "AddEvent";

				std::string eventname = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
				if (eventname.empty())
					return;

				std::string interpreter = request::findValue(&req, "interpreter");
				if (interpreter.empty())
					return;

				std::string eventtype = request::findValue(&req, "eventtype");
				if (eventtype.empty())
					return;

				std::string eventxml = request::findValue(&req, "xml");
				if (eventxml.empty())
					return;

				std::string eventactive = request::findValue(&req, "eventstatus");
				if (eventactive.empty())
					return;

				std::string eventid = request::findValue(&req, "eventid");

				std::string eventlogic = request::findValue(&req, "logicarray");
				if ((interpreter == "Blockly") && (eventlogic.empty()))
					return;

				int eventStatus = atoi(eventactive.c_str());

				bool parsingSuccessful = eventxml.length() > 0;
				Json::Value jsonRoot;
				if (interpreter == "Blockly") {
					parsingSuccessful = ParseJSon(eventlogic, jsonRoot);
				}

				if (!parsingSuccessful)
				{
					_log.Log(LOG_ERROR, "Webserver event parser: Invalid data received!");
				}
				else {
					if (eventid.empty())
					{
						m_sql.safe_query("INSERT INTO EventMaster (Name, Interpreter, Type, XMLStatement, Status) VALUES ('%q','%q','%q','%q','%d')",
							eventname.c_str(), interpreter.c_str(), eventtype.c_str(), eventxml.c_str(), eventStatus);
						result = m_sql.safe_query("SELECT ID FROM EventMaster WHERE (Name == '%q')",
							eventname.c_str());
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];
							eventid = sd[0];
						}
					}
					else {
						m_sql.safe_query("UPDATE EventMaster SET Name='%q', Interpreter='%q', Type='%q', XMLStatement ='%q', Status ='%d' WHERE (ID == '%q')",
							eventname.c_str(), interpreter.c_str(), eventtype.c_str(), eventxml.c_str(), eventStatus, eventid.c_str());
						m_sql.safe_query("DELETE FROM EventRules WHERE (EMID == '%q')",
							eventid.c_str());
					}

					if (eventid.empty())
					{
						//eventid should now never be empty!
						_log.Log(LOG_ERROR, "Error writing event actions to database!");
					}
					else {
						std::string sNewEditorTheme = request::findValue(&req, "editortheme");
						std::string sOldEditorTheme = "ace/theme/xcode";
						m_sql.GetPreferencesVar("ScriptEditorTheme", sOldEditorTheme);
						if (sNewEditorTheme.length() && (sNewEditorTheme != sOldEditorTheme))
						{
							m_sql.UpdatePreferencesVar("ScriptEditorTheme", sNewEditorTheme);
						}

						if (interpreter == "Blockly") {
							const Json::Value array = jsonRoot["eventlogic"];
							for (int index = 0; index < (int)array.size(); ++index)
							{
								std::string conditions = array[index].get("conditions", "").asString();
								std::string actions = array[index].get("actions", "").asString();

								if (
									(actions.find("SendNotification") != std::string::npos) ||
									(actions.find("SendEmail") != std::string::npos) ||
									(actions.find("SendSMS") != std::string::npos) ||
									(actions.find("TriggerIFTTT") != std::string::npos)
									)
								{
									stdreplace(actions, "$", "#");
								}
								int sequenceNo = index + 1;
								m_sql.safe_query("INSERT INTO EventRules (EMID, Conditions, Actions, SequenceNo) VALUES ('%q','%q','%q','%d')",
									eventid.c_str(), conditions.c_str(), actions.c_str(), sequenceNo);
							}
						}

						m_mainworker.m_eventsystem.LoadEvents();
						root["status"] = "OK";
					}
				}
			}
			else if (cparam == "delete")
			{
				root["title"] = "DeleteEvent";
				std::string idx = request::findValue(&req, "event");
				if (idx.empty())
					return;
				m_sql.DeleteEvent(idx);
				m_mainworker.m_eventsystem.LoadEvents();
				root["status"] = "OK";
			}
			else if (cparam == "currentstates")
			{
				std::vector<CEventSystem::_tDeviceStatus> devStates;
				m_mainworker.m_eventsystem.WWWGetItemStates(devStates);
				if (devStates.empty())
					return;

				int ii = 0;
				for (const auto &state : devStates)
				{
					root["title"] = "Current States";
					root["result"][ii]["id"] = (Json::Value::UInt64)state.ID;
					root["result"][ii]["name"] = state.deviceName;
					root["result"][ii]["value"] = state.nValueWording;
					std::stringstream sstr;
					sstr << state.nValue;
					if (!state.sValue.empty())
						sstr << "/" << state.sValue;
					root["result"][ii]["values"] = sstr.str();
					root["result"][ii]["lastupdate"] = state.lastUpdate;
					ii++;
				}
				root["status"] = "OK";
			}
		}
	} // namespace server
} // namespace http
