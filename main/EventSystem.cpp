#include "stdafx.h"
#include "mainworker.h"
#include "RFXNames.h"
#include "EventSystem.h"
#include "Helper.h"
#include "SQLHelper.h"
#include "Logger.h"
#include "../hardware/hardwaretypes.h"
#include "../hardware/Kodi.h"
#include "../hardware/LogitechMediaServer.h"
#include <iostream>
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "localtime_r.h"
#include "SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef WIN32
#include "dirent_windows.h"
#else
#include <dirent.h>
#endif

extern "C" {
#ifdef WITH_EXTERNAL_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#else
#include "../lua/src/lua.h"
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
#endif
#ifdef ENABLE_PYTHON
#include <Python.h>
#endif
}

#ifdef ENABLE_PYTHON
#include <boost/python.hpp>
using namespace boost::python;
#endif

extern std::string szUserDataFolder;

CEventSystem::CEventSystem(void)
{
	m_stoprequested = false;
	m_bEnabled = true;
}


CEventSystem::~CEventSystem(void)
{
	StopEventSystem();
	/*
	if (m_pLUA!=NULL)
	{
	lua_close(m_pLUA);
	m_pLUA=NULL;
	}
	*/
}

void CEventSystem::StartEventSystem()
{
	StopEventSystem();
	if (!m_bEnabled)
		return;

	m_sql.GetPreferencesVar("SecStatus", m_SecStatus);

	LoadEvents();
	GetCurrentStates();

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEventSystem::Do_Work, this)));
}

void CEventSystem::StopEventSystem()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
	}
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
	boost::unique_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
	_log.Log(LOG_STATUS, "EventSystem: reset all events...");
	m_events.clear();

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, Name, Interpreter, Type, Status, XMLStatement FROM EventMaster WHERE Interpreter <> 'Blockly' AND Status > 0 ORDER BY ID");
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			_tEventItem eitem;
			std::stringstream s_str(sd[0]);
			s_str >> eitem.ID;
			eitem.Name = sd[1];
			eitem.Interpreter = sd[2];
			std::transform(sd[3].begin(), sd[3].end(), sd[3].begin(), ::tolower);
			eitem.Type = sd[3];
			eitem.EventStatus = atoi(sd[4].c_str());
			eitem.Actions = sd[5];
			eitem.SequenceNo = 0;
			m_events.push_back(eitem);

		}
	}

	result = m_sql.safe_query("SELECT EventRules.ID,EventMaster.Name,EventRules.Conditions,EventRules.Actions,EventMaster.Status,EventRules.SequenceNo,EventMaster.Interpreter,EventMaster.Type FROM EventRules INNER JOIN EventMaster ON EventRules.EMID=EventMaster.ID ORDER BY EventRules.ID");
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			_tEventItem eitem;
			std::stringstream s_str(sd[0]);
			s_str >> eitem.ID;
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
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "EventSystem: Events (re)loaded");
#endif
}

void CEventSystem::Do_Work()
{
	m_stoprequested = false;
	time_t lasttime = mytime(NULL);
	//bool bFirstTime = true;
	struct tm tmptime;
	struct tm ltime;


	localtime_r(&lasttime, &tmptime);
	int _LastMinute = tmptime.tm_min;

	_log.Log(LOG_STATUS, "EventSystem: Started");
	while (!m_stoprequested)
	{
		//sleep 500 milliseconds
		sleep_milliseconds(500);
		time_t atime = mytime(NULL);

		if (atime != lasttime)
		{
			lasttime = atime;

			localtime_r(&atime, &ltime);

			if (ltime.tm_sec % 12 == 0) {
				m_mainworker.HeartbeatUpdate("EventSystem");
			}
			if (ltime.tm_min != _LastMinute)//((ltime.tm_min != _LastMinute) || (bFirstTime))
			{
				_LastMinute = ltime.tm_min;
				//bFirstTime = false;
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
	if (sString.size() < 1)
		return;

	size_t tpos = sString.find('"');
	if (tpos==0) //strip first quote
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

struct _tHardwareListIntEV{
	std::string Name;
	bool Enabled;
};

void CEventSystem::GetCurrentStates()
{
	std::vector<std::vector<std::string> > result;

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	_log.Log(LOG_STATUS, "EventSystem: reset all device statuses...");
	m_devicestates.clear();

	result = m_sql.safe_query(
		"SELECT A.HardwareID, A.ID, A.Name, A.nValue, A.sValue, A.Type, A.SubType, A.SwitchType, A.LastUpdate, A.LastLevel, A.Options "
		"FROM DeviceStatus AS A, Hardware AS B "
		"WHERE (A.Used = '1') AND (B.ID == A.HardwareID) AND (B.Enabled == 1)");
	if (result.size()>0)
	{
		// Allocate all memory before filling
		m_devicestates.get_allocator().allocate(result.size());
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			_tDeviceStatus sitem;

			// Fix string capacity to avoid map entry resizing
			std::string l_deviceName;		l_deviceName.reserve(100);
			std::string l_sValue;			l_sValue.reserve(200);
			std::string l_nValueWording;	l_nValueWording.reserve(20);
			std::string l_lastUpdate;		l_lastUpdate.reserve(30);

			std::stringstream s_str(sd[1]);
			s_str >> sitem.ID;
			sitem.deviceName = l_deviceName.assign(sd[2]);

			sitem.nValue = atoi(sd[3].c_str());
			sitem.sValue = l_sValue.assign(sd[4]);
			sitem.devType = atoi(sd[5].c_str());
			sitem.subType = atoi(sd[6].c_str());
			sitem.switchtype = atoi(sd[7].c_str());
			_eSwitchType switchtype = (_eSwitchType)sitem.switchtype;
			std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sd[10].c_str());
			sitem.nValueWording = l_nValueWording.assign(nValueToWording(sitem.devType, sitem.subType, switchtype, (unsigned char)sitem.nValue, sitem.sValue, options));
			sitem.lastUpdate = l_lastUpdate.assign(sd[8]);
			sitem.lastLevel = atoi(sd[9].c_str());
			m_devicestates[sitem.ID] = sitem;
		}
	}
}

void CEventSystem::GetCurrentUserVariables()
{
	boost::unique_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);

	//_log.Log(LOG_STATUS, "EventSystem: reset all user variables...");
	m_uservariables.clear();

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,Value, ValueType, LastUpdate FROM UserVariables");
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			_tUserVariable uvitem;
			std::stringstream s_str(sd[0]);
			s_str >> uvitem.ID;
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
	result = m_sql.safe_query("SELECT ID, Name, nValue, SceneType, LastUpdate FROM Scenes");
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			_tScenesGroups sgitem;
			std::stringstream s_str(sd[0]);
			s_str >> sgitem.ID;
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

	std::stringstream szQuery;

	float EnergyDivider = 1000.0f;
	float GasDivider = 100.0f;
	float WaterDivider = 100.0f;
	int tValue;
	if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
	{
		EnergyDivider = float(tValue);
	}
	if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
	{
		GasDivider = float(tValue);
	}
	if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
	{
		WaterDivider = float(tValue);
	}

	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	//char szTmp[300];

	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
	{
		_tDeviceStatus sitem = iterator->second;
		std::vector<std::string> splitresults;
		StringSplit(sitem.sValue, ";", splitresults);

		float temp = 0;
		float chill = 0;
		unsigned char humidity = 0;
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

		bool isTemp = false;
		bool isDew = false;
		bool isHum = false;
		bool isBaro = false;
		bool isBaroFloat = false;
		bool isUtility = false;
		bool isWeather = false;
		bool isRain = false;
		bool isUV = false;
		bool isWindDir = false;
		bool isWindSpeed = false;
		bool isWindGust = false;

		szQuery.clear();
		szQuery.str("");

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
			humidity = (unsigned char)sitem.nValue;
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
			if (sitem.subType == sTypeTHBFloat)
			{
				barometer = static_cast<float>(atof(splitresults[3].c_str()));
				isBaroFloat = true;
			}
			else
			{
				barometer = static_cast<float>(atof(splitresults[3].c_str()));
			}
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
					windspeed = float(intSpeed) * 0.1f; //m/s
					isWindSpeed = true;
				}

				int intGust = atoi(splitresults[3].c_str());
				windgust = float(intGust) * 0.1f; //m/s
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
					chill = static_cast<float>(atof(splitresults[5].c_str()));
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
			if (!splitresults.empty())
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
				if ((sitem.subType == sTypeVisibility)
				 || (sitem.subType == sTypeSolarRadiation))
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
				if (sitem.subType == sTypeCounterIncremental)
				{
					//get value of today
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

					char szDate[40];
					sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

					std::vector<std::vector<std::string> > result2;
					result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
						sitem.ID, szDate);
					if (result2.size() > 0)
					{
						std::vector<std::string> sd2 = result2[0];

						unsigned long long total_min, total_max, total_real;

						std::stringstream s_str1(sd2[0]);
						s_str1 >> total_min;
						std::stringstream s_str2(sd2[1]);
						s_str2 >> total_max;
						total_real = total_max - total_min;

						char szTmp[100];
						sprintf(szTmp, "%llu", total_real);

						float musage = 0;
						_eMeterType metertype = (_eMeterType)sitem.switchtype;
						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
							musage = float(total_real) / EnergyDivider;
							sprintf(szTmp, "%.03f kWh", musage);
							break;
						case MTYPE_GAS:
							musage = float(total_real) / GasDivider;
							sprintf(szTmp, "%.02f m3", musage);
							break;
						case MTYPE_WATER:
							musage = float(total_real) / WaterDivider;
							sprintf(szTmp, "%.02f m3", musage);
							break;
						case MTYPE_COUNTER:
							sprintf(szTmp, "%llu", total_real);
							break;
						}
						utilityval = static_cast<float>(atof(szTmp));
						isUtility = true;
					}
				}
			}
		}
		break;
		case pTypeRAIN:
			if (splitresults.size() == 2)
			{
				//get lowest value of today
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

				char szDate[100];
				sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;

				szQuery.clear();
				szQuery.str("");
				if (sitem.subType != sTypeRAINWU)
				{
					result2 = m_sql.safe_query(
						"SELECT MIN(Total), MAX(Total) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
						sitem.ID, szDate);
				}
				else
				{
					result2 = m_sql.safe_query(
						"SELECT Total, Total FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1",
						sitem.ID, szDate);
				}
				if (result2.size()>0)
				{
					double total_real = 0;
					std::vector<std::string> sd2 = result2[0];
					if (sitem.subType != sTypeRAINWU)
					{
						float total_min = static_cast<float>(atof(sd2[0].c_str()));
						float total_max = static_cast<float>(atof(splitresults[1].c_str()));
						total_real = total_max - total_min;
					}
					else
					{
						total_real = atof(sd2[1].c_str());
					}
					rainmm = float(total_real);
					rainmmlasthour = static_cast<float>(atof(splitresults[0].c_str())) / 100.0f;
					isRain = true;
					weatherval = rainmmlasthour;
					isWeather = true;
				}
			}
			break;
		case pTypeP1Gas:
			{
				//get lowest value of today
				float GasDivider = 1000.0f;
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

				char szDate[40];
				sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;
				result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
					sitem.ID, szDate);
				if (result2.size()>0)
				{
					std::vector<std::string> sd2 = result2[0];

					unsigned long long total_min_gas, total_real_gas;
					unsigned long long gasactual;

					std::stringstream s_str1(sd2[0]);
					s_str1 >> total_min_gas;
					std::stringstream s_str2(sitem.sValue);
					s_str2 >> gasactual;
					total_real_gas = gasactual - total_min_gas;
					utilityval = float(total_real_gas) / GasDivider;
					isUtility = true;
				}
			}
			break;
		case pTypeRFXMeter:
			if (sitem.subType == sTypeRFXMeterCount)
			{
				//get value of today
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

				char szDate[40];
				sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

				std::vector<std::vector<std::string> > result2;
				result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
					sitem.ID, szDate);
				if (result2.size()>0)
				{
					std::vector<std::string> sd2 = result2[0];

					unsigned long long total_min, total_max, total_real;

					std::stringstream s_str1(sd2[0]);
					s_str1 >> total_min;
					std::stringstream s_str2(sd2[1]);
					s_str2 >> total_max;
					total_real = total_max - total_min;

					char szTmp[100];
					sprintf(szTmp, "%llu", total_real);

					float musage = 0;
					_eMeterType metertype = (_eMeterType)sitem.switchtype;
					switch (metertype)
					{
					case MTYPE_ENERGY:
					case MTYPE_ENERGY_GENERATED:
						musage = float(total_real) / EnergyDivider;
						sprintf(szTmp, "%.03f kWh", musage);
						break;
					case MTYPE_GAS:
						musage = float(total_real) / GasDivider;
						sprintf(szTmp, "%.02f m3", musage);
						break;
					case MTYPE_WATER:
						musage = float(total_real) / WaterDivider;
						sprintf(szTmp, "%.02f m3", musage);
						break;
					case MTYPE_COUNTER:
						sprintf(szTmp, "%llu", total_real);
						break;
					}
					utilityval = static_cast<float>(atof(szTmp));
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
	}
}

void CEventSystem::RemoveSingleState(int ulDevID)
{
	if (!m_bEnabled)
		return;

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	//_log.Log(LOG_STATUS,"EventSystem: deleted device %d",ulDevID);
	m_devicestates.erase(ulDevID);

}

void CEventSystem::WWWUpdateSingleState(const uint64_t ulDevID, const std::string &devname)
{
	if (!m_bEnabled)
		return;

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	std::map<uint64_t, _tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);
	if (itt != m_devicestates.end()) {
		//_log.Log(LOG_STATUS,"EventSystem: www update device %" PRIu64 "",ulDevID);

		// Fix string capacity to avoid map entry resizing
		std::string l_deviceName;		l_deviceName.reserve(100);		l_deviceName.assign(devname);

		_tDeviceStatus replaceitem = itt->second;
		replaceitem.deviceName = l_deviceName;
		itt->second = replaceitem;
	}
}

void CEventSystem::WWWUpdateSecurityState(int securityStatus)
{
	if (!m_bEnabled)
		return;

	m_sql.GetPreferencesVar("SecStatus", m_SecStatus);
	GetCurrentUserVariables();
	EvaluateEvent("security");
}

std::string CEventSystem::UpdateSingleState(const uint64_t ulDevID, const std::string &devname, const int nValue, const char* sValue, const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, const std::string &lastUpdate, const unsigned char lastLevel, const std::map<std::string, std::string> & options)
{
	std::string nValueWording = nValueToWording(devType, subType, switchType, nValue, sValue, options);

	// Fix string capacity to avoid map entry resizing
	std::string l_deviceName;		l_deviceName.reserve(100);		l_deviceName.assign(devname);
	std::string l_sValue;			l_sValue.reserve(200);			l_sValue.assign(sValue);
	std::string l_nValueWording;	l_nValueWording.reserve(20);	l_nValueWording.assign(nValueWording);
	std::string l_lastUpdate;		l_lastUpdate.reserve(30);		l_lastUpdate.assign(lastUpdate);

	boost::unique_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);

	std::map<uint64_t, _tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);
	if (itt != m_devicestates.end()) {
		//_log.Log(LOG_STATUS,"EventSystem: update device %" PRIu64 "",ulDevID);
		_tDeviceStatus replaceitem = itt->second;
		replaceitem.deviceName = l_deviceName;
		replaceitem.nValue = nValue;
		replaceitem.sValue = l_sValue;
		replaceitem.nValueWording = l_nValueWording;
		replaceitem.lastUpdate = l_lastUpdate;
		replaceitem.lastLevel = lastLevel;
		itt->second = replaceitem;
	} else {
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
		m_devicestates[newitem.ID] = newitem;
	}
	return nValueWording;
}


void CEventSystem::ProcessDevice(const int HardwareID, const uint64_t ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string &devname, const int varId)
{
	if (!m_bEnabled)
		return;

	// query to get switchtype & LastUpdate, can't seem to get it from SQLHelper?
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, SwitchType, LastUpdate, LastLevel, Options FROM DeviceStatus WHERE (Name == '%q')",
		devname.c_str());
	if (result.size()>0) {
		std::vector<std::string> sd = result[0];
		_eSwitchType switchType = (_eSwitchType)atoi(sd[1].c_str());
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(result[0][4].c_str());

		std::string nValueWording = UpdateSingleState(ulDevID, devname, nValue, sValue, devType, subType, switchType, sd[2], atoi(sd[3].c_str()), options);
		GetCurrentUserVariables();
		EvaluateEvent("device", ulDevID, devname, nValue, sValue, nValueWording, 0);
	}
	else {
		_log.Log(LOG_ERROR, "EventSystem: Could not determine switch type for event device %s", devname.c_str());
	}
}

void CEventSystem::ProcessMinute()
{
	GetCurrentUserVariables();
	GetCurrentScenesGroups();
	EvaluateEvent("time");
}

void CEventSystem::ProcessUserVariable(const uint64_t varId)
{
	if (!m_bEnabled)
		return;
	GetCurrentUserVariables();
	EvaluateEvent("uservariable", varId);
}

void CEventSystem::EvaluateEvent(const std::string &reason)
{
	EvaluateEvent(reason, 0, "", 0, "", "", 0);
}

void CEventSystem::EvaluateEvent(const std::string &reason, const uint64_t varId)
{
	EvaluateEvent(reason, 0, "", 0, "", "", varId);
}

void CEventSystem::EvaluateEvent(const std::string &reason, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId)
{
	if (!m_bEnabled)
		return;
	boost::unique_lock<boost::shared_mutex> uservariablesMutexLock(m_uservariablesMutex);

	std::stringstream lua_DirT;

#ifdef WIN32
	lua_DirT << szUserDataFolder << "scripts\\lua\\";
#else
	lua_DirT << szUserDataFolder << "scripts/lua/";
#endif

	std::string lua_Dir = lua_DirT.str();

	DIR *lDir;
	struct dirent *ent;

	if ((lDir = opendir(lua_Dir.c_str())) != NULL)
	{
		while ((ent = readdir(lDir)) != NULL)
		{
			std::string filename = ent->d_name;
			if (dirent_is_file(lua_Dir, ent))
			{
				if ((filename.length() < 4) || (filename.compare(filename.length() - 4, 4, ".lua") != 0))
				{
					//_log.Log(LOG_STATUS,"EventSystem: ignore file not .lua: %s",filename.c_str());
				}
				else if (filename.find("_demo.lua") == std::string::npos) //skip demo lua files
				{
					if ((reason == "device") && (filename.find("_device_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename, "", DeviceID, devname, nValue, sValue, nValueWording, 0);
					}
					else if ((reason == "time") && (filename.find("_time_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename, "");
					}
					else if ((reason == "security") && (filename.find("_security_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename, "");
					}
					else if ((reason == "uservariable") && (filename.find("_variable_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename, "", varId);
					}
				}
			}
		}
		closedir(lDir);
	}
	else {
		_log.Log(LOG_ERROR, "EventSystem: Error accessing lua script directory %s", lua_Dir.c_str());
	}

#ifdef ENABLE_PYTHON
	try
	{
		std::stringstream python_DirT;
#ifdef WIN32
		python_DirT << szUserDataFolder << "scripts\\python\\";
#else
		python_DirT << szUserDataFolder << "scripts/python/";
#endif

		std::string python_Dir = python_DirT.str();

		if ((lDir = opendir(python_Dir.c_str())) != NULL)
		{
			while ((ent = readdir(lDir)) != NULL)
			{
				std::string filename = ent->d_name;
				if (dirent_is_file(python_Dir, ent))
				{
					if ((filename.length() < 4) || (filename.compare(filename.length() - 3, 3, ".py") != 0))
					{
						//_log.Log(LOG_STATUS,"EventSystem: ignore file not .lua: %s",filename.c_str());
					}
					else if (filename.find("_demo.py") == std::string::npos) //skip demo python files
					{
						if ((reason == "device") && (filename.find("_device_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename, "", DeviceID, devname, nValue, sValue, nValueWording, 0);
						}
						else if ((reason == "time") && (filename.find("_time_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename, "");
						}
						else if ((reason == "security") && (filename.find("_security_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename, "");
						}
						else if ((reason == "uservariable") && (filename.find("_variable_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename, "", varId);
						}
					}
				}
			}
			closedir(lDir);
		}
		else {
			_log.Log(LOG_ERROR, "EventSystem: Error accessing python script directory %s", python_Dir.c_str());
		}
	}
	catch (...)
	{
	}
#endif

	EvaluateBlockly(reason, DeviceID, devname, nValue, sValue, nValueWording, varId);

	// handle database held scripts
	try {
		boost::shared_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			bool eventInScope = ((it->Interpreter != "Blockly") && ((it->Type == "all") || (it->Type == reason)));
			bool eventActive = (it->EventStatus == 1);
			if (eventInScope && eventActive) {
				if (it->Interpreter == "Lua") {
					if (reason == "device")			EvaluateLua(reason, it->Name, it->Actions, DeviceID, devname, nValue, sValue, nValueWording, 0);
					if (reason == "time")			EvaluateLua(reason, it->Name, it->Actions);
					if (reason == "security")		EvaluateLua(reason, it->Name, it->Actions);
					if (reason == "uservariable")	EvaluateLua(reason, it->Name, it->Actions, varId);
				}
				if (it->Interpreter == "Python") {
#ifdef ENABLE_PYTHON
					if (reason == "device")			EvaluatePython(reason, it->Name, it->Actions, DeviceID, devname, nValue, sValue, nValueWording, 0);
					if (reason == "time")			EvaluatePython(reason, it->Name, it->Actions);
					if (reason == "security")		EvaluatePython(reason, it->Name, it->Actions);
					if (reason == "uservariable")	EvaluatePython(reason, it->Name, it->Actions, varId);
					//_log.Log(LOG_ERROR, "EventSystem: Error processing database scripts, Python not supported yet");
#else
					_log.Log(LOG_ERROR, "EventSystem: Error processing database scripts, Python not enabled");
#endif
				}
			}
		}
	}
	catch (...) {
		_log.Log(LOG_ERROR, "EventSystem: Exception processing database scripts");
	}
}

lua_State *CEventSystem::CreateBlocklyLuaState()
{
	lua_State *lua_state = luaL_newstate();
	if (lua_state == NULL)
		return NULL;

	lua_state = luaL_newstate();

	// load Lua libraries
	static const luaL_Reg lualibs[] =
	{
		{ "base", luaopen_base },
		{ "io", luaopen_io },
		{ "table", luaopen_table },
		{ "string", luaopen_string },
		{ "math", luaopen_math },
		{ NULL, NULL }
	};

	luaL_requiref(lua_state, "os", luaopen_os, 1);

	const luaL_Reg *lib = lualibs;
	for (; lib->func != NULL; lib++)
	{
		lib->func(lua_state);
		lua_settop(lua_state, 0);
	}

	// reroute print library to domoticz logger
	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
	lua_createtable(lua_state, (int)m_devicestates.size(), 0);

	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator) {
		_tDeviceStatus sitem = iterator->second;
		lua_pushnumber(lua_state, (lua_Number)sitem.ID);
		lua_pushstring(lua_state, sitem.nValueWording.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "device");
	devicestatesMutexLock.unlock();

	lua_createtable(lua_state, (int)m_uservariables.size(), 0);

	typedef std::map<uint64_t, _tUserVariable>::iterator it_var;
	for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); ++iterator) {
		_tUserVariable uvitem = iterator->second;
		if (uvitem.variableType == 0) {
			//Integer
			lua_pushnumber(lua_state, (lua_Number)uvitem.ID);
			lua_pushnumber(lua_state, atoi(uvitem.variableValue.c_str()));
			lua_rawset(lua_state, -3);
		}
		else if (uvitem.variableType == 1) {
			//Float
			lua_pushnumber(lua_state, (lua_Number)uvitem.ID);
			lua_pushnumber(lua_state, atof(uvitem.variableValue.c_str()));
			lua_rawset(lua_state, -3);
		}
		else {
			//String,Date,Time
			lua_pushnumber(lua_state, (lua_Number)uvitem.ID);
			lua_pushstring(lua_state, uvitem.variableValue.c_str());
			lua_rawset(lua_state, -3);
		}
	}
	lua_setglobal(lua_state, "variable");

	boost::lock_guard<boost::mutex> measurementStatesMutexLock(m_measurementStatesMutex);
	GetCurrentMeasurementStates();

	if (m_tempValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_tempValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_tempValuesByID.begin(); p != m_tempValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "temperaturedevice");
	}
	if (m_dewValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_dewValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_dewValuesByID.begin(); p != m_dewValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "dewpointdevice");
	}
	if (m_humValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_humValuesByID.size(), 0);
		std::map<uint64_t, int>::iterator p;
		for (p = m_humValuesByID.begin(); p != m_humValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "humiditydevice");
	}
	if (m_baroValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_baroValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_baroValuesByID.begin(); p != m_baroValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "barometerdevice");
	}
	if (m_utilityValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_utilityValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_utilityValuesByID.begin(); p != m_utilityValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "utilitydevice");
	}
	if (m_weatherValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_weatherValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_weatherValuesByID.begin(); p != m_weatherValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "weatherdevice");
	}
	if (m_rainValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_rainValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_rainValuesByID.begin(); p != m_rainValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "raindevice");
	}
	if (m_rainLastHourValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_rainLastHourValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_rainLastHourValuesByID.begin(); p != m_rainLastHourValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "rainlasthourdevice");
	}
	if (m_uvValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_uvValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_uvValuesByID.begin(); p != m_uvValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "uvdevice");
	}
	if (m_winddirValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_winddirValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_winddirValuesByID.begin(); p != m_winddirValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "winddirdevice");
	}
	if (m_windspeedValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_windspeedValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_windspeedValuesByID.begin(); p != m_windspeedValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "windspeeddevice");
	}
	if (m_windgustValuesByID.size() > 0) {
		lua_createtable(lua_state, (int)m_windgustValuesByID.size(), 0);
		std::map<uint64_t, float>::iterator p;
		for (p = m_windgustValuesByID.begin(); p != m_windgustValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "windgustdevice");
	}

	lua_pushnumber(lua_state, (lua_Number)m_SecStatus);
	lua_setglobal(lua_state, "securitystatus");

	return lua_state;
}

void CEventSystem::EvaluateBlockly(const std::string &reason, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId)
{
//#ifdef _DEBUG
	//    _log.Log(LOG_STATUS,"EventSystem: blockly %s trigger",reason.c_str());
//#endif

	lua_State *lua_state=NULL;

	if ((reason == "device") && (DeviceID >0)) {
		std::size_t found;
		boost::shared_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			bool eventInScope = ((it->Interpreter == "Blockly") && ((it->Type=="all")||(it->Type == reason)));
			bool eventActive = (it->EventStatus == 1);
			std::stringstream sstr;
			sstr << "device[" << DeviceID << "]";
			std::string conditions (it->Conditions);
			found = conditions.find(sstr.str());
			if ((eventInScope) && (eventActive) && (found != std::string::npos)) {
				// Replace Sunrise and sunset placeholder with actual time for query
				if (conditions.find("@Sunrise") != std::string::npos) {
					int intRise = getSunRiseSunSetMinutes("Sunrise");
					std::stringstream ssRise;
					ssRise << intRise;
					stdreplace(conditions, "@Sunrise", ssRise.str());
				}
				if (conditions.find("@Sunset") != std::string::npos) {
					int intSet = getSunRiseSunSetMinutes("Sunset");
					std::stringstream ssSet;
					ssSet << intSet;
					stdreplace(conditions, "@Sunset", ssSet.str());
				}

				std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + conditions + " then result = 1 end; return result";

				if (lua_state == NULL)
				{
					lua_state = CreateBlocklyLuaState();
					if (lua_state == NULL)
						return;
				}

				//_log.Log(LOG_STATUS,"EventSystem: ifc: %s",ifCondition.c_str());
				if (luaL_dostring(lua_state, ifCondition.c_str()))
				{
					_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
				}
				else
				{
					lua_Number ruleTrue = lua_tonumber(lua_state, -1);
					if (ruleTrue != 0)
					{
						if (m_sql.m_bLogEventScriptTrigger)
							_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
						parseBlocklyActions(it->Actions, it->Name, it->ID);
					}
				}
			}
		}
	}
	else if (reason == "security") {
		// security status change
		std::size_t found;
		boost::shared_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			bool eventInScope = ((it->Interpreter == "Blockly") && ((it->Type == "all") || (it->Type == reason)));
			bool eventActive = (it->EventStatus == 1);
			std::stringstream sstr;
			sstr << "securitystatus";
			std::string conditions (it->Conditions);
			found = conditions.find(sstr.str());

			if ((eventInScope) && (eventActive) && (found != std::string::npos)) {

				// Replace Sunrise and sunset placeholder with actual time for query
				if (conditions.find("@Sunrise") != std::string::npos) {
					int intRise = getSunRiseSunSetMinutes("Sunrise");
					std::stringstream ssRise;
					ssRise << intRise;
					stdreplace(conditions, "@Sunrise", ssRise.str());
				}
				if (conditions.find("@Sunset") != std::string::npos) {
					int intSet = getSunRiseSunSetMinutes("Sunset");
					std::stringstream ssSet;
					ssSet << intSet;
					stdreplace(conditions, "@Sunset", ssSet.str());
				}

				std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + conditions + " then result = 1 end; return result";

				if (lua_state == NULL)
				{
					lua_state = CreateBlocklyLuaState();
					if (lua_state == NULL)
						return;
				}

				//_log.Log(LOG_NORM,"ifc: %s",ifCondition.c_str());
				if (luaL_dostring(lua_state, ifCondition.c_str()))
				{
					_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
				}
				else {
					lua_Number ruleTrue = lua_tonumber(lua_state, -1);

					if (ruleTrue != 0)
					{
						if (m_sql.m_bLogEventScriptTrigger)
							_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
						parseBlocklyActions(it->Actions, it->Name, it->ID);
					}
				}
			}
		}
	}
	else if (reason == "time") {
		boost::shared_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			bool eventInScope = ((it->Interpreter == "Blockly") && ((it->Type == "all") || (it->Type == reason)));
			bool eventActive = (it->EventStatus == 1);
			if ((eventInScope) && (eventActive)) {
				// time rules will only run when time or date based critera are found
				std::string conditions (it->Conditions);
				if ((conditions.find("timeofday") != std::string::npos) || (conditions.find("weekday") != std::string::npos)) {

					// Replace Sunrise and sunset placeholder with actual time for query
					if (conditions.find("@Sunrise") != std::string::npos) {
						int intRise = getSunRiseSunSetMinutes("Sunrise");
						std::stringstream ssRise;
						ssRise << intRise;
						stdreplace(conditions, "@Sunrise", ssRise.str());
					}
					if (conditions.find("@Sunset") != std::string::npos) {
						int intSet = getSunRiseSunSetMinutes("Sunset");
						std::stringstream ssSet;
						ssSet << intSet;
						stdreplace(conditions, "@Sunset", ssSet.str());
					}

					std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + conditions + " then result = 1 end; return result";

					if (lua_state == NULL)
					{
						lua_state = CreateBlocklyLuaState();
						if (lua_state == NULL)
							return;
					}

					//_log.Log(LOG_NORM,"ifc: %s",ifCondition.c_str());
					if (luaL_dostring(lua_state, ifCondition.c_str()))
					{
						_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
					}
					else {
						lua_Number ruleTrue = lua_tonumber(lua_state, -1);
						if (ruleTrue != 0)
						{
							if (m_sql.m_bLogEventScriptTrigger)
								_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
							parseBlocklyActions(it->Actions, it->Name, it->ID);
						}
					}
				}
			}
		}
	}
	else if ((reason == "uservariable") && (varId >0)) {
		std::size_t found;
		boost::shared_lock<boost::shared_mutex> eventsMutexLock(m_eventsMutex);
		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			bool eventInScope = ((it->Interpreter == "Blockly") && ((it->Type == "all") || (it->Type == reason)));
			bool eventActive = (it->EventStatus == 1);
			std::stringstream sstr;
			sstr << "variable[" << varId << "]";
			std::string conditions (it->Conditions);
			found = conditions.find(sstr.str());

			if ((eventInScope) && (eventActive) && (found != std::string::npos)) {

				// Replace Sunrise and sunset placeholder with actual time for query
				if (conditions.find("@Sunrise") != std::string::npos) {
					int intRise = getSunRiseSunSetMinutes("Sunrise");
					std::stringstream ssRise;
					ssRise << intRise;
					stdreplace(conditions, "@Sunrise", ssRise.str());
				}
				if (conditions.find("@Sunset") != std::string::npos) {
					int intSet = getSunRiseSunSetMinutes("Sunset");
					std::stringstream ssSet;
					ssSet << intSet;
					stdreplace(conditions, "@Sunset", ssSet.str());
				}
				std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + conditions + " then result = 1 end; return result";

				if (lua_state == NULL)
				{
					lua_state = CreateBlocklyLuaState();
					if (lua_state == NULL)
						return;
				}

				//_log.Log(LOG_STATUS,"ifc: %s",ifCondition.c_str());
				if (luaL_dostring(lua_state, ifCondition.c_str()))
				{
					_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
				}
				else {
					lua_Number ruleTrue = lua_tonumber(lua_state, -1);

					if (ruleTrue != 0)
					{
						if (m_sql.m_bLogEventScriptTrigger)
							_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
						parseBlocklyActions(it->Actions, it->Name, it->ID);
					}
				}
			}
		}
	}

	if (lua_state != NULL)
	{
		lua_close(lua_state);
	}
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

	return ret;
}

std::string CEventSystem::ParseBlocklyString(const std::string &oString)
{
	std::string retString = oString;

	while (1)
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
		std::string part_right = retString.substr(pos2+2);
		part_middle = ProcessVariableArgument(part_middle);
		retString = part_left + part_middle + part_right;
	}

	return retString;
}

bool CEventSystem::parseBlocklyActions(const std::string &Actions, const std::string &eventName, const uint64_t eventID)
{
	if (isEventscheduled(eventName))
	{
		//_log.Log(LOG_NORM,"Already scheduled this event, skipping");
		return false;
	}
	bool actionsDone = false;
	std::string csubstr;
	std::string tmpstr(Actions);
	size_t sPos=0, ePos;
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
		sPos = csubstr.find_first_of("[");
		ePos = csubstr.find_first_of("]");

		if ((sPos == std::string::npos) || (ePos == std::string::npos))
		{
			_log.Log(LOG_ERROR, "EventSystem: Malformed action sequence!");
			break;
		}

		size_t eQPos = csubstr.find_first_of("=");
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
				if (ScheduleEvent(deviceNo, doWhat, false, eventName, 0)) {
					actionsDone = true;
				}
			}
			else {
				reportMissingDevice(deviceNo, eventName, eventID);
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
				if (ScheduleEvent(deviceNo, doWhat, true, eventName, sceneType))
				{
					actionsDone = true;
				}
			}
			continue;
		}
		else if (deviceName.find("Variable:") == 0)
		{
			std::string variableNo = deviceName.substr(9);
			float afterTimerSeconds = 0;
			size_t aFind = doWhat.find(" AFTER ");
			if ((aFind > 0) && (aFind != std::string::npos)) {
				std::string delayString = doWhat.substr(aFind + 7);
				std::string newAction = doWhat.substr(0, aFind);
				afterTimerSeconds = static_cast<float>(atof(delayString.c_str()));
				doWhat = newAction;
				StripQuotes(doWhat);
			}
			doWhat = ProcessVariableArgument(doWhat);
			if (afterTimerSeconds < (1. / timer_resolution_hz / 2))
			{
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE (ID == '%q')", variableNo.c_str());
				if (result.size() > 0)
				{
					std::vector<std::string> sd = result[0];
					std::string updateResult = m_sql.UpdateUserVariable(variableNo, sd[0], sd[1], doWhat, false);
					if (updateResult != "OK") {
						_log.Log(LOG_ERROR, "EventSystem: Error updating variable %s: %s", sd[0].c_str(), updateResult.c_str());
					}
				}
			}
			else
			{
				float DelayTime = afterTimerSeconds;
				m_sql.AddTaskItem(_tTaskItem::SetVariable(DelayTime, (const uint64_t)atol(variableNo.c_str()), doWhat, false));
			}
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("SendCamera:") == 0)
		{
			if (!atoi(deviceName.substr(11).c_str()))
				continue;
			ScheduleEvent(deviceName, doWhat, eventName);
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("SetSetpoint:") == 0)
		{
			int idx = atoi(deviceName.substr(12).c_str());
			m_sql.AddTaskItem(_tTaskItem::SetSetPoint(0.5f, idx, doWhat));
			actionsDone = true;
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
				continue;;
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
				continue;;
			}
			doWhat = ParseBlocklyString(doWhat);
			m_sql.AddTaskItem(_tTaskItem::SendSMS(1, doWhat));
			actionsDone = true;
			continue;
		}
		else if (deviceName.find("OpenURL") != std::string::npos)
		{
			OpenURL(doWhat);
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
			std::string sParam = "";
			size_t tpos = sPath.find('$');
			if (tpos != std::string::npos)
			{
				sPath = sPath.substr(0, tpos);
				sParam = doWhat.substr(tpos + 1);
				sParam = ParseBlocklyString(sParam);
			}
#if !defined WIN32
			if (sPath.find("/") != 0)
				sPath = szUserDataFolder + "scripts/" + sPath;
#endif

			m_sql.AddTaskItem(_tTaskItem::ExecuteScript(0.2f, sPath, sParam));
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
			std::string subject, body, priority("0"), sound;
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
			m_sql.AddTaskItem(_tTaskItem::SendNotification(1, subject, body, std::string(""), atoi(priority.c_str()), sound));
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

void CEventSystem::ParseActionString( const std::string &oAction_, _tActionParseResults &oResults_ ) {
	int iLastTokenType = 0;

	std::vector<std::string> oSplitResults;
	StringSplit( oAction_, " ", oSplitResults );
	for( std::vector<std::string>::const_iterator oIterator = oSplitResults.begin(); oIterator != oSplitResults.end(); ++oIterator ) {
		std::string sToken = *oIterator;
		if ( sToken == "FOR" ) {
			iLastTokenType = 1;
		} else if ( sToken == "AFTER" ) {
			iLastTokenType = 2;
		} else if ( sToken == "RANDOM" ) {
			iLastTokenType = 3;
		} else if ( sToken == "REPEAT" ) {
			iLastTokenType = 4;
		} else if ( sToken == "INTERVAL" ) {
			iLastTokenType = 5;
		} else if ( sToken == "TURN" ) {
			iLastTokenType = 0;
		} else if ( sToken.find( "SECOND" ) != std::string::npos ) {
			switch( iLastTokenType ) {
				case 1: oResults_.fForSec /= 60.; break;
				case 3: oResults_.fRandomSec /= 60.; break;
			}
			iLastTokenType = 0;
		} else if ( sToken.find( "MINUTE" ) != std::string::npos ) {
			switch( iLastTokenType ) {
				case 2: oResults_.fAfterSec *= 60.; break;
				case 5: oResults_.fRepeatSec *= 60.; break;
			}
			iLastTokenType = 0;
		} else if ( sToken.find( "HOUR" ) != std::string::npos ) {
			switch( iLastTokenType ) {
				case 1: oResults_.fForSec *= 60.; break;
				case 2: oResults_.fAfterSec *= 3600.; break;
				case 3: oResults_.fRandomSec *= 60.; break;
				case 5: oResults_.fRepeatSec *= 3600.; break;
			}
			iLastTokenType = 0;
		} else {
			switch( iLastTokenType ) {
				case 0:
					if ( oResults_.sCommand.length() > 0 ) {
						oResults_.sCommand.append( " " );
					}
					oResults_.sCommand.append( sToken );
					break;
				case 1:
					oResults_.fForSec = 60.f * static_cast<float>(atof(sToken.c_str()));
					break;
				case 2:
					oResults_.fAfterSec = 1.f * static_cast<float>(atof(sToken.c_str()));
					break;
				case 3:
					oResults_.fRandomSec = 60.f * static_cast<float>(atof(sToken.c_str()));
					break;
				case 4:
					oResults_.iRepeat = atoi( sToken.c_str() );
					break;
				case 5:
					oResults_.fRepeatSec = 1.f * static_cast<float>(atof(sToken.c_str()));
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

void CEventSystem::EvaluatePython(const std::string &reason, const std::string &filename, const std::string &PyString, const uint64_t varId)
{
	EvaluatePython(reason, filename, PyString, 0, "", 0, "", "", varId);
}

void CEventSystem::EvaluatePython(const std::string &reason, const std::string &filename, const std::string &PyString)
{
	EvaluatePython(reason, filename, PyString, 0, "", 0, "", "", 0);
}


static int numargs=0;

/* Return the number of arguments of the application command line */
static PyObject*
PyDomoticz_log(PyObject *self, PyObject *args)
{
	char* msg;
	int type;
    if(!PyArg_ParseTuple(args, "is", &type, &msg))
        return NULL;
	_log.Log((_eLogLevel)type, msg);
	Py_INCREF(Py_None);
    return Py_None;

}

static PyMethodDef DomoticzMethods[] = {
    {"log", PyDomoticz_log, METH_VARARGS,  "log to Domoticz."},
    {NULL, NULL, 0, NULL}
};


// from https://gist.github.com/octavifs/5362297

template <class K, class V>
boost::python::dict toPythonDict(std::map<K, V> map) {
    typename std::map<K, V>::iterator iter;
	boost::python::dict dictionary;
	for (iter = map.begin(); iter != map.end(); ++iter) {
		dictionary[iter->first] = iter->second;
	}
	return dictionary;
}


// this should be filled in by the preprocessor
const char * Python_exe = "PYTHON_EXE";

void CEventSystem::EvaluatePython(const std::string &reason, const std::string &filename, const std::string &PyString, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId)
{
	//_log.Log(LOG_NORM, "EventSystem: Already scheduled this event, skipping");
	//_log.Log(LOG_STATUS, "EventSystem: script %s trigger, file: %s, deviceName: %s" , reason.c_str(), filename.c_str(), devname.c_str());

	std::stringstream python_DirT;

#ifdef WIN32
	python_DirT << szUserDataFolder << "scripts\\python\\";
#else
	python_DirT << szUserDataFolder << "scripts/python/";
#endif
	std::string python_Dir = python_DirT.str();
	if(!Py_IsInitialized()) {
		Py_SetProgramName((char*)Python_exe); // will this cast lead to problems ?
		Py_Initialize();
		Py_InitModule("domoticz_", DomoticzMethods);
		// TODO: may have a small memleak, remove references in destructor
		PyObject* sys = PyImport_ImportModule("sys");
		PyObject *path = PyObject_GetAttrString(sys, "path");
		PyList_Append(path, PyString_FromString(python_Dir.c_str()));

		bool (CEventSystem::*ScheduleEventMethod)(std::string ID, const std::string &Action, const std::string &eventName) = &CEventSystem::ScheduleEvent;
		class_<CEventSystem, boost::noncopyable>("Domoticz", no_init)
			.def("command", ScheduleEventMethod)
			;
	}

	FILE* PythonScriptFile = fopen(filename.c_str(), "r");
	object main_module = import("__main__");
	object main_namespace = dict(main_module.attr("__dict__")).copy();


	try {
		object domoticz_module = import("domoticz");
		object reloader = import("reloader");
		reloader.attr("_check_reload")();

		//object alldevices = dict();
		object devices = domoticz_module.attr("devices");
		object domoticz_namespace = domoticz_module.attr("__dict__");

		domoticz_namespace["event_system"] = ptr(this);

		main_namespace["changed_device_name"] = str(devname);
		domoticz_namespace["changed_device_name"] = str(devname);

		boost::shared_lock<boost::shared_mutex> devicestatesMutexLock1(m_devicestatesMutex);
		typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
		for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
		{
			_tDeviceStatus sitem = iterator->second;
			object deviceStatus = domoticz_module.attr("Device")(sitem.ID, sitem.deviceName, sitem.devType, sitem.subType, sitem.switchtype, sitem.nValue, sitem.nValueWording, sitem.sValue, sitem.lastUpdate);
			devices[sitem.deviceName] = deviceStatus;
		}
		main_namespace["domoticz"] = ptr(this);
		main_namespace["__file__"] = filename;

		if (reason == "device")
		{
			main_namespace["changed_device"] = devices[m_devicestates[DeviceID].deviceName];
			domoticz_namespace["changed_device"] = devices[m_devicestates[DeviceID].deviceName];
		}
		devicestatesMutexLock1.unlock();


		int intRise = getSunRiseSunSetMinutes("Sunrise");
		int intSet = getSunRiseSunSetMinutes("Sunset");

		time_t now = mytime(NULL);
		struct tm mtime;
		time_t tmidnight;
		getMidnight(tmidnight,mtime);
		int minutesSinceMidnight = (int)(difftime(now,tmidnight) / 60);
	
		bool dayTimeBool = false;
		bool nightTimeBool = false;
		if ((minutesSinceMidnight > intRise) && (minutesSinceMidnight < intSet)) {
			dayTimeBool = true;
		}
		else {
			nightTimeBool = true;
		}
		main_namespace["is_daytime"] = dayTimeBool;
		main_namespace["is_nighttime"] = nightTimeBool;
		main_namespace["sunrise_in_minutes"] = intRise;
		main_namespace["sunset_in_minutes"] = intSet;

		domoticz_namespace["is_daytime"] = dayTimeBool;
		domoticz_namespace["is_nighttime"] = nightTimeBool;
		domoticz_namespace["sunrise_in_minutes"] = intRise;
		domoticz_namespace["sunset_in_minutes"] = intSet;

		//main_namespace["timeofday"] = ... 		// not sure how to set this


		/*std::string secstatusw = "";
		m_sql.GetPreferencesVar("SecStatus", secstatus);
		if (secstatus == 1) {
			secstatusw = "Armed Home";
		}
		else if (secstatus == 2) {
			secstatusw = "Armed Away";
		}
		else {
			secstatusw = "Disarmed";
		}
		main_namespace["Security"] = secstatusw;*/

		// put variables in user_variables dict, but also in the namespace
		object user_variables = dict();
		{
			typedef std::map<uint64_t, _tUserVariable>::iterator it_var;
			for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); ++iterator) {
				_tUserVariable uvitem = iterator->second;
				//user_variables[uvitem.variableName] = uvitem;
				if (uvitem.variableType == 0) {
					//Integer
					main_namespace[uvitem.variableName] = atoi(uvitem.variableValue.c_str());
					user_variables[uvitem.variableName] = main_namespace[uvitem.variableName];
				}
				else if (uvitem.variableType == 1) {
					//Float
					main_namespace[uvitem.variableName] = atof(uvitem.variableValue.c_str());
					user_variables[uvitem.variableName] = main_namespace[uvitem.variableName];
				}
				else {
					//String,Date,Time
					main_namespace[uvitem.variableName] = uvitem.variableValue;
					user_variables[uvitem.variableName] = main_namespace[uvitem.variableName];
				}
			}
		}

		domoticz_namespace["user_variables"] = user_variables;
		main_namespace["user_variables"] = user_variables;
		main_namespace["otherdevices_temperature"] = toPythonDict(m_tempValuesByName);
		main_namespace["otherdevices_dewpoint"] = toPythonDict(m_dewValuesByName);
		main_namespace["otherdevices_barometer"] = toPythonDict(m_baroValuesByName);
		main_namespace["otherdevices_utility"] = toPythonDict(m_utilityValuesByName);
		main_namespace["otherdevices_rain"] = toPythonDict(m_rainValuesByName);
		main_namespace["otherdevices_rain_lasthour"] = toPythonDict(m_rainLastHourValuesByName);
		main_namespace["otherdevices_uv"] = toPythonDict(m_uvValuesByName);
		main_namespace["otherdevices_winddir"] = toPythonDict(m_winddirValuesByName);
		main_namespace["otherdevices_windspeed"] = toPythonDict(m_windspeedValuesByName);
		main_namespace["otherdevices_windgust"] = toPythonDict(m_windgustValuesByName);

		if(PyString.length() > 0)
			exec(str(PyString), main_namespace);
		else
			object ignored = exec_file(str(filename), main_namespace);
	} catch(...) {

		PyObject *exc,*val,*tb;
		PyErr_Fetch(&exc,&val,&tb);
		boost::python::handle<> hexc(exc), hval(boost::python::allow_null(val)), htb(boost::python::allow_null(tb));
		boost::python::object traceback(boost::python::import("traceback"));

		boost::python::object format_exception(traceback.attr("format_exception"));
		boost::python::object formatted_list = format_exception(hexc, hval, htb);
		boost::python::object formatted = boost::python::str("\n").join(formatted_list);

		object traceback_module = import("traceback");
		std::string formatted_str = extract<std::string>(formatted);
		//PyErr_Print();
		PyErr_Clear();
		if(PyString.length() > 0)
			_log.Log(LOG_ERROR, "in event %s: %s", filename.c_str(), formatted_str.c_str());
		else
			_log.Log(LOG_ERROR, "%s",formatted_str.c_str());
	}
	if (PythonScriptFile!=NULL)
		fclose(PythonScriptFile);
	//Py_Finalize();
}
#endif // ENABLE_PYTHON

void CEventSystem::exportDeviceStatesToLua(lua_State *lua_state)
{
	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock2(m_devicestatesMutex);
	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushstring(lua_state, sitem.nValueWording.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices");

	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushstring(lua_state, sitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_lastupdate");

	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushstring(lua_state, sitem.sValue.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_svalues");
	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushnumber(lua_state, (lua_Number)sitem.ID);
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_idx");
	devicestatesMutexLock2.unlock();
}

void CEventSystem::EvaluateLua(const std::string &reason, const std::string &filename, const std::string &LuaString, const uint64_t varId)
{
	EvaluateLua(reason, filename, LuaString, 0, "", 0, "", "", varId);
}

void CEventSystem::EvaluateLua(const std::string &reason, const std::string &filename, const std::string &LuaString)
{
	EvaluateLua(reason, filename, LuaString, 0, "", 0, "", "", 0);
}

void CEventSystem::EvaluateLua(const std::string &reason, const std::string &filename, const std::string &LuaString, const uint64_t DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const uint64_t varId)
{
	boost::lock_guard<boost::mutex> l(luaMutex);

	//if (isEventscheduled(filename))
	//{
	//	//_log.Log(LOG_NORM,"EventSystem: Already scheduled this event, skipping");
	//	return;
	//}

	lua_State *lua_state;
	lua_state = luaL_newstate();

	// load Lua libraries
	static const luaL_Reg lualibs[] =
	{
		{ "base", luaopen_base },
		{ "io", luaopen_io },
		{ "table", luaopen_table },
		{ "string", luaopen_string },
		{ "math", luaopen_math },
		{ NULL, NULL }
	};

	const luaL_Reg *lib = lualibs;
	for (; lib->func != NULL; lib++)
	{
		lib->func(lua_state);
		lua_settop(lua_state, 0);
	}

	// reroute print library to Domoticz logger
	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

#ifdef _DEBUG
	_log.Log(LOG_STATUS, "EventSystem: script %s trigger", reason.c_str());
#endif

	int intRise = getSunRiseSunSetMinutes("Sunrise");
	int intSet = getSunRiseSunSetMinutes("Sunset");
	time_t now = time(0);
	struct tm mtime;
	time_t tmidnight;
	getMidnight(tmidnight,mtime);
	int minutesSinceMidnight = (int)(difftime(now,tmidnight) / 60);

	bool dayTimeBool = false;
	bool nightTimeBool = false;
	if ((minutesSinceMidnight > intRise) && (minutesSinceMidnight < intSet)) {
		dayTimeBool = true;
	}
	else {
		nightTimeBool = true;
	}

	lua_createtable(lua_state, 4, 0);
	lua_pushstring(lua_state, "Daytime");
	lua_pushboolean(lua_state, dayTimeBool);
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "Nighttime");
	lua_pushboolean(lua_state, nightTimeBool);
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "SunriseInMinutes");
	lua_pushnumber(lua_state, intRise);
	lua_rawset(lua_state, -3);
	lua_pushstring(lua_state, "SunsetInMinutes");
	lua_pushnumber(lua_state, intSet);
	lua_rawset(lua_state, -3);
	lua_setglobal(lua_state, "timeofday");

	{
		boost::lock_guard<boost::mutex> measurementStatesMutexLock(m_measurementStatesMutex);
		GetCurrentMeasurementStates();

		float thisDeviceTemp = 0;
		float thisDeviceDew = 0;
		float thisDeviceRain = 0;
		float thisDeviceRainLastHour = 0;
		float thisDeviceUV = 0;
		unsigned char thisDeviceHum = 0;
		float thisDeviceBaro = 0;
		float thisDeviceUtility = 0;
		float thisDeviceWindDir = 0;
		float thisDeviceWindSpeed = 0;
		float thisDeviceWindGust = 0;
		float thisDeviceWeather = 0;

		if (m_tempValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_tempValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_tempValuesByName.begin(); p != m_tempValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceTemp = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_temperature");
		}
		if (m_dewValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_dewValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_dewValuesByName.begin(); p != m_dewValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceDew = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_dewpoint");
		}
		if (m_humValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_humValuesByName.size(), 0);
			std::map<std::string, int>::iterator p;
			for (p = m_humValuesByName.begin(); p != m_humValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceHum = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_humidity");
		}
		if (m_baroValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_baroValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_baroValuesByName.begin(); p != m_baroValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceBaro = (float)p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_barometer");
		}
		if (m_utilityValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_utilityValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_utilityValuesByName.begin(); p != m_utilityValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceUtility = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_utility");
		}
		if (m_rainValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_rainValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_rainValuesByName.begin(); p != m_rainValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceRain = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_rain");
		}
		if (m_rainLastHourValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_rainLastHourValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_rainLastHourValuesByName.begin(); p != m_rainLastHourValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceRainLastHour = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_rain_lasthour");
		}
		if (m_uvValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_uvValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_uvValuesByName.begin(); p != m_uvValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceUV = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_uv");
		}
		if (m_winddirValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_winddirValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_winddirValuesByName.begin(); p != m_winddirValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceWindDir = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_winddir");
		}
		if (m_windspeedValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_windspeedValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_windspeedValuesByName.begin(); p != m_windspeedValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceWindSpeed = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_windspeed");
		}
		if (m_windgustValuesByName.size()>0)
		{
			lua_createtable(lua_state, (int)m_windgustValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_windgustValuesByName.begin(); p != m_windgustValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceWindGust = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_windgust");
		}
		if (m_weatherValuesByName.size() > 0)
		{
			lua_createtable(lua_state, (int)m_weatherValuesByName.size(), 0);
			std::map<std::string, float>::iterator p;
			for (p = m_weatherValuesByName.begin(); p != m_weatherValuesByName.end(); ++p)
			{
				lua_pushstring(lua_state, p->first.c_str());
				lua_pushnumber(lua_state, (lua_Number)p->second);
				lua_rawset(lua_state, -3);
				if (p->first == devname) {
					thisDeviceWeather = p->second;
				}
			}
			lua_setglobal(lua_state, "otherdevices_weather");
		}

		if (reason == "device")
		{
			lua_createtable(lua_state, 1, 0);
			lua_pushstring(lua_state, devname.c_str());
			lua_pushstring(lua_state, nValueWording.c_str());
			lua_rawset(lua_state, -3);
			if (thisDeviceTemp != 0)
			{
				std::string tempName = devname;
				tempName += "_Temperature";
				lua_pushstring(lua_state, tempName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceTemp);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceDew != 0)
			{
				std::string tempName = devname;
				tempName += "_Dewpoint";
				lua_pushstring(lua_state, tempName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceDew);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceHum != 0) {
				std::string humName = devname;
				humName += "_Humidity";
				lua_pushstring(lua_state, humName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceHum);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceBaro != 0) {
				std::string baroName = devname;
				baroName += "_Barometer";
				lua_pushstring(lua_state, baroName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceBaro);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceUtility != 0) {
				std::string utilityName = devname;
				utilityName += "_Utility";
				lua_pushstring(lua_state, utilityName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceUtility);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceWeather != 0) {
				std::string weatherName = devname;
				weatherName += "_Weather";
				lua_pushstring(lua_state, weatherName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceWeather);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceRain != 0)
			{
				std::string tempName = devname;
				tempName += "_Rain";
				lua_pushstring(lua_state, tempName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceRain);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceRainLastHour != 0)
			{
				std::string tempName = devname;
				tempName += "_RainLastHour";
				lua_pushstring(lua_state, tempName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceRainLastHour);
				lua_rawset(lua_state, -3);
			}
			if (thisDeviceUV != 0)
			{
				std::string tempName = devname;
				tempName += "_UV";
				lua_pushstring(lua_state, tempName.c_str());
				lua_pushnumber(lua_state, (lua_Number)thisDeviceUV);
				lua_rawset(lua_state, -3);
			}
			lua_setglobal(lua_state, "devicechanged");


			// BEGIN OTO: populate changed info
			lua_createtable(lua_state, 3, 0);
			lua_pushstring(lua_state, "idx");
			lua_pushnumber(lua_state, (lua_Number)DeviceID);
			lua_rawset(lua_state, -3);

			lua_pushstring(lua_state, "svalue");
			lua_pushstring(lua_state, sValue);
			lua_rawset(lua_state, -3);
			
			lua_pushstring(lua_state, "nvalue");
			lua_pushnumber(lua_state, nValue);
			lua_rawset(lua_state, -3);

			/* USELESS, WE HAVE THE DEVICE INDEX
			// replace devicechanged => 
			lua_pushstring(lua_state, "name"); 
			lua_pushnumber(lua_state, nValue);
			lua_rawset(lua_state, -3);
			*/
			lua_setglobal(lua_state, "devicechanged_ext");
			// END OTO
		}
	}

	exportDeviceStatesToLua(lua_state);
	
	lua_createtable(lua_state, (int)m_uservariables.size(), 0);

	typedef std::map<uint64_t, _tUserVariable>::iterator it_var;
	for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); ++iterator) {
		_tUserVariable uvitem = iterator->second;
		if (uvitem.variableType == 0)  {
			//Integer
			lua_pushstring(lua_state, uvitem.variableName.c_str());
			lua_pushnumber(lua_state, atoi(uvitem.variableValue.c_str()));
			lua_rawset(lua_state, -3);
		}
		else if (uvitem.variableType == 1)  {
			//Float
			lua_pushstring(lua_state, uvitem.variableName.c_str());
			lua_pushnumber(lua_state, atof(uvitem.variableValue.c_str()));
			lua_rawset(lua_state, -3);
		}
		else {
			//String,Date,Time
			lua_pushstring(lua_state, uvitem.variableName.c_str());
			lua_pushstring(lua_state, uvitem.variableValue.c_str());
			lua_rawset(lua_state, -3);
		}
	}
	lua_setglobal(lua_state, "uservariables");

	lua_createtable(lua_state, (int)m_scenesgroups.size(), 0);
	typedef std::map<uint64_t, _tScenesGroups>::iterator it_scgr;
	for (it_scgr iterator = m_scenesgroups.begin(); iterator != m_scenesgroups.end(); ++iterator) {
		_tScenesGroups sgitem = iterator->second;
		lua_pushstring(lua_state, sgitem.scenesgroupName.c_str());
		lua_pushstring(lua_state, sgitem.scenesgroupValue.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_scenesgroups");

	lua_createtable(lua_state, (int)m_scenesgroups.size(), 0);
	typedef std::map<uint64_t, _tScenesGroups>::iterator it_scgr;
	for (it_scgr iterator = m_scenesgroups.begin(); iterator != m_scenesgroups.end(); ++iterator)
	{
		_tScenesGroups sgitem = iterator->second;
		lua_pushstring(lua_state, sgitem.scenesgroupName.c_str());
		lua_pushnumber(lua_state, (lua_Number)sgitem.ID);
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_scenesgroups_idx");

	lua_createtable(lua_state, (int)m_uservariables.size(), 0);

	typedef std::map<uint64_t, _tUserVariable>::iterator it_var;
	for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); ++iterator) {
		_tUserVariable uvitem = iterator->second;
		lua_pushstring(lua_state, uvitem.variableName.c_str());
		lua_pushstring(lua_state, uvitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "uservariables_lastupdate");

	if (reason == "uservariable") {
		if (varId > 0) {
			typedef std::map<uint64_t, _tUserVariable>::iterator it_cvar;
			for (it_cvar iterator = m_uservariables.begin(); iterator != m_uservariables.end(); ++iterator) {
				_tUserVariable uvitem = iterator->second;
				if (uvitem.ID == varId) {
					lua_createtable(lua_state, 1, 0);
					lua_pushstring(lua_state, uvitem.variableName.c_str());
					lua_pushstring(lua_state, uvitem.variableValue.c_str());
					lua_rawset(lua_state, -3);
					lua_setglobal(lua_state, "uservariablechanged");
				}
			}
		}
	}

	int secstatus = 0;
	std::string secstatusw = "";
	m_sql.GetPreferencesVar("SecStatus", secstatus);
	if (secstatus == 1) {
		secstatusw = "Armed Home";
	}
	else if (secstatus == 2) {
		secstatusw = "Armed Away";
	}
	else {
		secstatusw = "Disarmed";
	}

	lua_createtable(lua_state, 1, 0);
	lua_pushstring(lua_state, "Security");
	lua_pushstring(lua_state, secstatusw.c_str());
	lua_rawset(lua_state, -3);
	lua_setglobal(lua_state, "globalvariables");

	int status = 0;
	if (LuaString.length() == 0) {
		status = luaL_loadfile(lua_state, filename.c_str());
	}
	else {
		status = luaL_loadstring(lua_state, LuaString.c_str());
	}

	if (status == 0)
	{
		lua_sethook(lua_state, luaStop, LUA_MASKCOUNT, 10000000);
		//luaThread = boost::thread(&CEventSystem::luaThread, lua_state, filename);
		//boost::shared_ptr<boost::thread> luaThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEventSystem::luaThread, this, lua_state, filename)));
		boost::thread luaThread(boost::bind(&CEventSystem::luaThread, this, lua_state, filename));
		//m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEventSystem::Do_Work, this)));
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
		lua_sethook(L, NULL, 0, 0);
		luaL_error(L, "Lua script execution exceeds maximum number of lines");
		lua_close(L);
	}
}

bool CEventSystem::iterateLuaTable(lua_State *lua_state, const int tIndex, const std::string &filename)
{
	bool scriptTrue = false;

	lua_pushnil(lua_state); // first key
	while (lua_next(lua_state, tIndex) != 0)
	{
		if ((std::string(luaL_typename(lua_state, -2)) == "string") && (std::string(luaL_typename(lua_state, -1))) == "string")
		{
			scriptTrue = processLuaCommand(lua_state, filename);
		}
		else if ((std::string(luaL_typename(lua_state, -2)) == "number") && lua_istable(lua_state, -1))
		{
			scriptTrue = iterateLuaTable(lua_state, tIndex + 2, filename);
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
		std::string subject(" "), body(" "), priority("0"), sound;
		std::string extraData;
		std::vector<std::string> aParam;
		StringSplit(luaString, "#", aParam);
		subject = body = aParam[0];
		if (aParam.size() > 1) {
			body = aParam[1];
		}
		if (aParam.size() > 2) {
			priority = aParam[2];
		}
		if (aParam.size() > 3) {
			sound = aParam[3];
		}
		if (aParam.size() > 4) {
			extraData = "|Device=" + aParam[4];
		}
		m_sql.AddTaskItem(_tTaskItem::SendNotification(1, subject, body, extraData, atoi(priority.c_str()), sound));
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
	else if (lCommand == "OpenURL")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		OpenURL(luaString);
		scriptTrue = true;
	}
	else if (lCommand == "UpdateDevice")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		UpdateDevice(luaString);
		scriptTrue = true;
	}
	else if (lCommand.find("Variable:") == 0)
	{
		std::string variableName = lCommand.substr(9);
		std::string variableValue = lua_tostring(lua_state, -1);

		std::vector<std::vector<std::string> > result;

		float afterTimerSeconds = 0;
		size_t aFind = variableValue.find(" AFTER ");
		if ((aFind > 0) && (aFind != std::string::npos)) {
			std::string delayString = variableValue.substr(aFind + 7);
			std::string newAction = variableValue.substr(0, aFind);
			afterTimerSeconds = static_cast<float>(atof(delayString.c_str()));
			variableValue = newAction;
		}
		result = m_sql.safe_query("SELECT ID, ValueType FROM UserVariables WHERE (Name == '%q')", variableName.c_str());
		if (result.size() > 0)
		{
			std::vector<std::string> sd = result[0];

			variableValue = ProcessVariableArgument(variableValue);

			if (afterTimerSeconds < (1./timer_resolution_hz/2))
			{
				std::string updateResult = m_sql.UpdateUserVariable(sd[0], variableName, sd[1], variableValue, false);
				if (updateResult != "OK") {
					_log.Log(LOG_ERROR, "EventSystem: Error updating variable %s: %s", variableName.c_str(), updateResult.c_str());
				}
			}
			else
			{
				float DelayTime = afterTimerSeconds;
				uint64_t idx;
				std::stringstream sstr;
				sstr << sd[0];
				sstr >> idx;
				m_sql.AddTaskItem(_tTaskItem::SetVariable(DelayTime, idx, variableValue, false));
			}
			scriptTrue = true;
		}
	}
	else if (lCommand.find("SetSetPoint:") == 0)
	{
		std::string SetPointIdx = lCommand.substr(12);
		std::string SetPointValue = lua_tostring(lua_state, -1);

		int idx = atoi(SetPointIdx.c_str());
		m_sql.AddTaskItem(_tTaskItem::SetSetPoint(0.5f, idx, SetPointValue));
	}
	else
	{
		if (ScheduleEvent(lua_tostring(lua_state, -2), lua_tostring(lua_state, -1), filename)) {
			scriptTrue = true;
		}
	}

	return scriptTrue;

}


void CEventSystem::report_errors(lua_State *L, int status, std::string filename)
{
	if (status != 0) {
		_log.Log(LOG_ERROR, "EventSystem: in %s: %s", filename.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1); // remove error message
	}
}

void CEventSystem::UpdateDevice(const std::string &DevParams)
{
	std::vector<std::string> strarray;
	StringSplit(DevParams, "|", strarray);
	if (strarray.size() != 3)
		return; //Invalid!
	std::string idx = strarray[0];
	std::string nvalue = strarray[1];
	std::string svalue = strarray[2];
	//Get device parameters
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType, Name, SwitchType, LastLevel, Options FROM DeviceStatus WHERE (ID=='%q')",
		idx.c_str());
	if (result.size()>0)
	{
		std::string hid = result[0][0];
		std::string did = result[0][1];
		std::string dunit = result[0][2];
		std::string dtype = result[0][3];
		std::string dsubtype = result[0][4];
		std::string dname = result[0][5];
		_eSwitchType dswitchtype = (_eSwitchType)atoi(result[0][6].c_str());
		int dlastlevel = atoi(result[0][7].c_str());
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(result[0][8].c_str());

		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now, &ltime);

		char szLastUpdate[40];
		sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

		m_sql.safe_query("UPDATE DeviceStatus SET nValue='%q', sValue='%q', LastUpdate='%q' WHERE (ID = '%q')",
			nvalue.c_str(), svalue.c_str(), szLastUpdate, idx.c_str());


		uint64_t ulIdx = 0;
		std::stringstream s_str(idx);
		s_str >> ulIdx;

		int devType = atoi(dtype.c_str());
		int subType = atoi(dsubtype.c_str());

		UpdateSingleState(ulIdx, dname, atoi(nvalue.c_str()), svalue.c_str(), devType, subType, dswitchtype, szLastUpdate, dlastlevel, options);

		//Check if we need to log this event
		switch (devType)
		{
		case pTypeRego6XXValue:
			if (subType != sTypeRego6XXStatus)
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
		case pTypeFan:
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
		case pTypeThermostat4:
		case pTypeRemote:
		case pTypeGeneralSwitch:
		case pTypeHomeConfort:
		case pTypeRadiator1:
			if ((devType == pTypeRadiator1) && (subType != sTypeSmartwaresSwitchRadiator))
				break;
			//Add Lighting log
			m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue) VALUES ('%" PRIu64 "', '%d', '%q')", ulIdx, atoi(nvalue.c_str()), svalue.c_str());
			break;
		}

		//Check if it's a setpoint device, and if so, set the actual setpoint
		if (
			((devType == pTypeThermostat) && (subType == sTypeThermSetpoint)) ||
			(devType == pTypeRadiator1)
			)
		{
			_log.Log(LOG_NORM, "EventSystem: Sending SetPoint to device....");
			m_mainworker.SetSetPoint(idx, static_cast<float>(atof(svalue.c_str())));
		}
		else if ((devType == pTypeGeneral) && (subType == sTypeZWaveThermostatMode))
		{
			_log.Log(LOG_NORM, "EventSystem: Sending Thermostat Mode to device....");
			m_mainworker.SetZWaveThermostatMode(idx, atoi(nvalue.c_str()));
		}
		else if ((devType == pTypeGeneral) && (subType == sTypeZWaveThermostatFanMode))
		{
			_log.Log(LOG_NORM, "EventSystem: Sending Thermostat Fan Mode to device....");
			m_mainworker.SetZWaveThermostatFanMode(idx, atoi(nvalue.c_str()));
		}
	}
}

void CEventSystem::OpenURL(const std::string &URL)
{
	_log.Log(LOG_STATUS, "EventSystem: Fetching url...");
	m_sql.AddTaskItem(_tTaskItem::GetHTTPPage(0.2f, URL, "OpenURL"));
	// maybe do something with sResult in the future.
}

void CEventSystem::WriteToLog(const std::string &devNameNoQuotes, const std::string &doWhat)
{

	if (devNameNoQuotes == "WriteToLogText")
	{
		_log.Log(LOG_STATUS, "%s", doWhat.c_str());
	}
	else if (devNameNoQuotes == "WriteToLogUserVariable")
	{
		_log.Log(LOG_STATUS, "%s", m_uservariables[atoi(doWhat.c_str())].variableValue.c_str());
	}
	else if (devNameNoQuotes == "WriteToLogDeviceVariable")
	{
		boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
		_log.Log(LOG_STATUS, "%s", m_devicestates[atoi(doWhat.c_str())].sValue.c_str());
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

		std::string cAction = Action;
		float delay = 0;
		size_t aFind = Action.find(" AFTER ");
		if ((aFind > 0) && (aFind != std::string::npos)) {
			std::string delayString = Action.substr(aFind + 7);
			std::string newAction = Action.substr(0, aFind);
			delay = static_cast<float>(atof(delayString.c_str()));
			cAction = newAction;
		}
		StripQuotes(cAction);


		std::string subject = cAction;
		if (delay < (1./timer_resolution_hz/2))
		{
			m_mainworker.m_cameras.EmailCameraSnapshot(deviceName, subject);
		}
		else
			m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(delay, deviceName, subject));
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
	if (result.size()>0)
	{
		std::vector<std::string> sd = result[0];
		int idx = atoi(sd[0].c_str());
		return (ScheduleEvent(idx, Action, isScene, eventName, sceneType));
	}

	return false;
}


bool CEventSystem::ScheduleEvent(int deviceID, std::string Action, bool isScene, const std::string &eventName, int sceneType)
{
	boost::shared_lock<boost::shared_mutex> devicestatesMutexLock(m_devicestatesMutex);
	std::string previousState = m_devicestates[deviceID].nValueWording;
	unsigned char previousLevel = calculateDimLevel(deviceID, m_devicestates[deviceID].lastLevel);
	unsigned char level = 0;
	devicestatesMutexLock.unlock();

	struct _tActionParseResults oParseResults = { "", 0, 0, 0, 1, 0 };
	ParseActionString( Action, oParseResults );

	if ( oParseResults.sCommand.substr( 0, 9 ) == "Set Level" ) {
		level = calculateDimLevel( deviceID, atoi( oParseResults.sCommand.substr( 10 ).c_str() ) );
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 9);
	} else if ( oParseResults.sCommand.substr( 0, 10 ) == "Set Volume" ) {
		level = atoi( oParseResults.sCommand.substr( 11 ).c_str() );
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 10);
	} else if ( oParseResults.sCommand.substr( 0, 13 ) == "Play Playlist" ) {
		std::string	sParams = oParseResults.sCommand.substr( 14 );

		CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_Kodi);
		if (pBaseHardware != NULL)
		{
			CKodi			*pHardware = reinterpret_cast<CKodi*>(pBaseHardware);
			std::string		sPlayList = sParams;
			size_t			iLastSpace = sParams.find_last_of(' ', sParams.length());

			if (iLastSpace != std::string::npos)
			{
				sPlayList = sParams.substr(0, iLastSpace);
				level = atoi(sParams.substr(iLastSpace).c_str());
			}
			if (!pHardware->SetPlaylist(deviceID, sPlayList.c_str()))
			{
				pBaseHardware = NULL; // Kodi hardware exists, but the device for the event is not a Kodi
			}
		}

		if (pBaseHardware == NULL)  // if not handled try Logitech
		{
			pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_LogitechMediaServer);
			if (pBaseHardware == NULL) return false;
			CLogitechMediaServer *pHardware = reinterpret_cast<CLogitechMediaServer*>(pBaseHardware);

			int iPlaylistID = pHardware->GetPlaylistRefID(oParseResults.sCommand.substr( 14 ).c_str());
			if (iPlaylistID == 0) return false;

			level = iPlaylistID;
		}
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 13);
	} else if ( oParseResults.sCommand.substr( 0, 14 ) == "Play Favorites" ) {
		std::string	sParams = oParseResults.sCommand.substr( 15 );
		CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_Kodi);
		if (pBaseHardware != NULL)
		{
			CKodi			*pHardware = reinterpret_cast<CKodi*>(pBaseHardware);
			if (sParams.length() > 0)
			{
				level = atoi(sParams.c_str());
			}
		}
		oParseResults.sCommand = oParseResults.sCommand.substr(0, 14);
	} else if ( oParseResults.sCommand.substr( 0, 7 ) == "Execute" ) {
		std::string	sParams = oParseResults.sCommand.substr( 8 );
		CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByType(HTYPE_Kodi);
		if (pBaseHardware != NULL)
		{
			CKodi	*pHardware = reinterpret_cast<CKodi*>(pBaseHardware);
			pHardware->SetExecuteCommand(deviceID, sParams);
		}
	}

	if ( previousState.substr( 0, 9 ) == "Set Level" ) {
		previousState = previousState.substr(0, 9);
	} else if ( previousState.substr( 0, 10 ) == "Set Volume" ) {
		previousState = previousState.substr(0, 10);
	}

	int iDeviceDelay = 0;
	if ( ! isScene ) {
		// Get Device details, check for switch global OnDelay/OffDelay (stored in AddjValue2/AddjValue).
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query( "SELECT SwitchType, AddjValue2 FROM DeviceStatus WHERE (ID == %d)", deviceID );
		if ( result.size() < 1 ) {
			return false;
		}

		std::vector<std::string> sd = result[0];
		_eSwitchType switchtype = (_eSwitchType)atoi(sd[0].c_str());
		int iOnDelay = atoi(sd[1].c_str());

		bool bIsOn = IsLightSwitchOn( oParseResults.sCommand );
		if ( switchtype == STYPE_Selector ) {
			bIsOn = ( level > 0 ) ? true : false;
		}
		iDeviceDelay = bIsOn ? iOnDelay : 0;
	}


	float fPreviousRandomTime = 0;
	for ( int iIndex = 0; iIndex < abs(oParseResults.iRepeat); iIndex++ ) {
		_tTaskItem tItem;

		float fRandomTime = 0;
		if ( oParseResults.fRandomSec > (1./timer_resolution_hz/2) ) {
			srand( (unsigned int)mytime( NULL ) + iIndex );
			fRandomTime = (float)rand() / (float)( RAND_MAX / oParseResults.fRandomSec );
		}

		float fDelayTime = oParseResults.fAfterSec + fPreviousRandomTime + fRandomTime + iDeviceDelay + ( iIndex * oParseResults.fForSec ) + ( iIndex * oParseResults.fRepeatSec );
		fPreviousRandomTime = fRandomTime;

		if ( isScene ) {

			if (
				oParseResults.sCommand == "On"
				|| oParseResults.sCommand == "Off"
			) {
				tItem = _tTaskItem::SwitchSceneEvent( fDelayTime, deviceID, oParseResults.sCommand, eventName );
			} else if ( oParseResults.sCommand == "Active" ) {
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query( "UPDATE SceneTimers SET Active=1 WHERE (SceneRowID == %d)", deviceID );
				m_mainworker.m_scheduler.ReloadSchedules();
			} else if ( oParseResults.sCommand == "Inactive" ) {
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query( "UPDATE SceneTimers SET Active=0 WHERE (SceneRowID == %d)", deviceID );
				m_mainworker.m_scheduler.ReloadSchedules();
			}

		} else {
			tItem = _tTaskItem::SwitchLightEvent( fDelayTime, deviceID, oParseResults.sCommand, level, -1, eventName );
		}

		m_sql.AddTaskItem( tItem );
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "EventSystem: Scheduled %s after %0.2f.", tItem._command.c_str(), tItem._DelayTime );
#endif

		if (
			oParseResults.fForSec > (1./timer_resolution_hz/2)
			&& (
				oParseResults.iRepeat > 0
				|| iIndex < abs(oParseResults.iRepeat)-1
			)
		) {
			fDelayTime += oParseResults.fForSec;

			_tTaskItem tDelayedtItem;
			if ( isScene ) {
				if ( oParseResults.sCommand == "On" ) {
					tDelayedtItem = _tTaskItem::SwitchSceneEvent( fDelayTime, deviceID, "Off", eventName );
				} else if ( oParseResults.sCommand == "Off" ) {
					tDelayedtItem = _tTaskItem::SwitchSceneEvent( fDelayTime, deviceID, "On", eventName );
				}
			} else {
				tDelayedtItem = _tTaskItem::SwitchLightEvent( fDelayTime, deviceID, previousState, previousLevel, -1, eventName );
			}
			m_sql.AddTaskItem( tDelayedtItem );
#ifdef _DEBUG
			_log.Log(LOG_STATUS, "EventSystem: Scheduled %s after %0.2f.", tDelayedtItem._command.c_str(), tDelayedtItem._DelayTime );
#endif
		}

	}

	return true;
}



std::string CEventSystem::nValueToWording(const unsigned char dType, const unsigned char dSubType, const _eSwitchType switchtype, const unsigned char nValue, const std::string &sValue, const std::map<std::string, std::string> & options)
{

	std::string lstatus = "";
	int llevel = 0;
	bool bHaveDimmer = false;
	bool bHaveGroupCmd = false;
	int maxDimLevel = 0;
	GetLightStatus(dType, dSubType, switchtype,nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
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
	else if(switchtype == STYPE_Selector)
	{
		std::map<std::string, std::string> statuses;
		GetSelectorSwitchStatuses(options, statuses);
		std::stringstream sslevel;
		sslevel << llevel;
		lstatus = statuses[sslevel.str()];
	}
	else if ((switchtype == STYPE_Contact) || (switchtype == STYPE_DoorLock))
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
		else
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
		else
		{
			lstatus = "Closed";
		}
	}
	else if (switchtype == STYPE_Media)
	{
		lstatus = Media_Player_States((const _eMediaStatus)nValue);
	}
	else if (lstatus == "")
	{
		lstatus = sValue;
		//OJO if lstatus  is still empty we use nValue for lstatus. ss for conversion
        	if (lstatus == "")
        	{
			std::stringstream ss;
			ss << (unsigned int)nValue;
           		lstatus = ss.str();
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
			//std::string lstring=lua_tostring(lua_state, i);
			_log.Log(LOG_STATUS, "LUA: %s", lua_tostring(lua_state, i));
		}
		else
		{
			/* non strings? */
		}
	}
	return 0;
}

void CEventSystem::reportMissingDevice(const int deviceID, const std::string &eventName, const uint64_t eventID)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (ID == %d)", deviceID);
	if (!result.empty())
	{
		_log.Log(LOG_ERROR, "EventSystem: Device ('%s', ID=%d) used in event '%s' not found, make sure that it's hardware is not disabled!", result[0][0].c_str(), deviceID, eventName.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR, "EventSystem: Device no. '%d' used in event '%s' no longer exists, disabling event!", deviceID, eventName.c_str());


		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT EventMaster.ID FROM EventMaster INNER JOIN EventRules ON EventRules.EMID=EventMaster.ID WHERE (EventRules.ID == '%" PRIu64 "')",
			eventID);
		if (result.size() > 0)
		{

			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
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
	typedef std::map<uint64_t, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); ++iterator)
	{
		iStates.push_back(iterator->second);
	}
}

int CEventSystem::getSunRiseSunSetMinutes(const std::string &what)
{
	std::vector<std::string> strarray;
	std::vector<std::string> sunRisearray;
	std::vector<std::string> sunSetarray;

	if (!m_mainworker.m_LastSunriseSet.empty())
	{
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
		StringSplit(strarray[0], ":", sunRisearray);
		StringSplit(strarray[1], ":", sunSetarray);

		int sunRiseInMinutes = (atoi(sunRisearray[0].c_str()) * 60) + atoi(sunRisearray[1].c_str());
		int sunSetInMinutes = (atoi(sunSetarray[0].c_str()) * 60) + atoi(sunSetarray[1].c_str());

		if (what == "Sunrise") {
			return sunRiseInMinutes;
		}
		else {
			return sunSetInMinutes;
		}

	}

	return 0;
}

bool CEventSystem::isEventscheduled(const std::string &eventName)
{
	bool foundIt = false;
	std::vector<_tTaskItem> currentTasks;
	m_sql.EventsGetTaskItems(currentTasks);
	if (currentTasks.size() == 0) {
		return foundIt;
	}
	else {
		for (std::vector<_tTaskItem>::iterator it = currentTasks.begin(); it != currentTasks.end(); ++it)
		{
			if (it->_relatedEvent == eventName) {
				foundIt = true;
			}
		}
	}
	return foundIt;
}


unsigned char CEventSystem::calculateDimLevel(int deviceID, int percentageLevel)
{

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == %d)",
		deviceID);
	unsigned char ilevel = 0;
	if (result.size()>0)
	{
		std::vector<std::string> sd = result[0];

		unsigned char dType = atoi(sd[0].c_str());
		unsigned char dSubType = atoi(sd[1].c_str());
		_eSwitchType switchtype = (_eSwitchType)atoi(sd[2].c_str());
		std::string lstatus = "";
		int llevel = 0;
		bool bHaveDimmer = false;
		bool bHaveGroupCmd = false;
		int maxDimLevel = 0;

		GetLightStatus(dType, dSubType, switchtype,0, "", lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
		ilevel = maxDimLevel;

		if (maxDimLevel != 0)
		{
			if ((switchtype == STYPE_Dimmer) || (switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted))
			{
				float fLevel = (maxDimLevel / 100.0f)*percentageLevel;
				if (fLevel > 100)
					fLevel = 100;
				ilevel = int(fLevel);
				if (ilevel > 0) { ilevel++; }
			} else if (switchtype == STYPE_Selector) {
				// llevel cannot be get without sValue so level is getting from percentageLevel
				ilevel = percentageLevel;
				if (ilevel > 100) {
					ilevel = 100;
				} else if (ilevel < 0) {
					ilevel = 0;
				}
			}
		}
	}
	return ilevel;
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

			std::string eventname = CURLEncode::URLDecode(request::findValue(&req, "name"));
			if (eventname == "")
				return;

			std::string interpreter = CURLEncode::URLDecode(request::findValue(&req, "interpreter"));
			if (interpreter == "")
				return;

			std::string eventtype = CURLEncode::URLDecode(request::findValue(&req, "eventtype"));
			if (eventtype == "")
				return;

			std::string eventxml = CURLEncode::URLDecode(request::findValue(&req, "xml"));
			if (eventxml == "")
				return;

			std::string eventactive = CURLEncode::URLDecode(request::findValue(&req, "eventstatus"));
			if (eventactive == "")
				return;

			std::string eventid = CURLEncode::URLDecode(request::findValue(&req, "eventid"));


			std::string eventlogic = CURLEncode::URLDecode(request::findValue(&req, "logicarray"));
			if ((interpreter == "Blockly") && (eventlogic == ""))
				return;

			int eventStatus = atoi(eventactive.c_str());

			bool parsingSuccessful = eventxml.length() > 0;
			Json::Value jsonRoot;
			if (interpreter == "Blockly") {
				Json::Reader reader;
				std::stringstream ssel(eventlogic);
				parsingSuccessful = reader.parse(ssel, jsonRoot);
			}

			if (!parsingSuccessful)
			{
				_log.Log(LOG_ERROR, "Webserver event parser: Invalid data received!");
			}
			else {
				if (eventid == "") {
					std::vector<std::vector<std::string> > result;
					m_sql.safe_query("INSERT INTO EventMaster (Name, Interpreter, Type, XMLStatement, Status) VALUES ('%q','%q','%q','%q','%d')",
						eventname.c_str(), interpreter.c_str(), eventtype.c_str(), eventxml.c_str(), eventStatus);
					result = m_sql.safe_query("SELECT ID FROM EventMaster WHERE (Name == '%q')",
						eventname.c_str());
					if (result.size() > 0)
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

				if (eventid == "")
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

							if ((actions.find("SendNotification") != std::string::npos) || (actions.find("SendEmail") != std::string::npos) || (actions.find("SendSMS") != std::string::npos))
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
			if (cparam == "")
			{
				cparam = request::findValue(&req, "dparam");
				if (cparam == "")
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
				root["interpreters"] = "Blockly:Lua:Python";
#else
				root["interpreters"] = "Blockly:Lua";
#endif

				std::map<std::string, _tSortedEventsInt> _levents;
				result = m_sql.safe_query("SELECT ID, Name, XMLStatement, Status FROM EventMaster ORDER BY ID ASC");
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
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
					std::map<std::string, _tSortedEventsInt>::const_iterator itt2;
					int ii = 0;
					for (itt2 = _levents.begin(); itt2 != _levents.end(); ++itt2)
					{
						root["result"][ii]["name"] = itt2->first;
						root["result"][ii]["id"] = itt2->second.ID;
						root["result"][ii]["eventstatus"] = itt2->second.eventstatus;
						ii++;
					}
				}
			}
			else if (cparam == "new")
			{
				root["title"] = "NewEvent";

				std::string interpreter = request::findValue(&req, "interpreter");
				if (interpreter == "")
					return;

				std::string eventType = request::findValue(&req, "eventtype");
				if (eventType == "")
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
				if (idx == "")
					return;

				int ii = 0;

				std::string sEditorTheme = "ace/theme/xcode";
				m_sql.GetPreferencesVar("ScriptEditorTheme", sEditorTheme);
				root["editortheme"] = sEditorTheme;

				result = m_sql.safe_query("SELECT ID, Name, XMLStatement, Status, Interpreter, Type FROM EventMaster WHERE (ID=='%q')",
					idx.c_str());
				if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
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
			else if (cparam == "create")
			{

				root["title"] = "AddEvent";

				std::string eventname = request::findValue(&req, "name");
				if (eventname == "")
					return;

				std::string interpreter = request::findValue(&req, "interpreter");
				if (interpreter == "")
					return;

				std::string eventtype = request::findValue(&req, "eventtype");
				if (eventtype == "")
					return;

				std::string eventxml = request::findValue(&req, "xml");
				if (eventxml == "")
					return;

				std::string eventactive = request::findValue(&req, "eventstatus");
				if (eventactive == "")
					return;

				std::string eventid = request::findValue(&req, "eventid");


				std::string eventlogic = request::findValue(&req, "logicarray");
				if ((interpreter == "Blockly") && (eventlogic == ""))
					return;

				int eventStatus = atoi(eventactive.c_str());

				bool parsingSuccessful = eventxml.length() > 0;
				Json::Value jsonRoot;
				if (interpreter == "Blockly") {
					Json::Reader reader;
					std::stringstream ssel(eventlogic);
					parsingSuccessful = reader.parse(ssel, jsonRoot);
				}

				if (!parsingSuccessful)
				{
					_log.Log(LOG_ERROR, "Webserver event parser: Invalid data received!");
				}
				else {
					if (eventid == "") {
						m_sql.safe_query("INSERT INTO EventMaster (Name, Interpreter, Type, XMLStatement, Status) VALUES ('%q','%q','%q','%q','%d')",
							eventname.c_str(), interpreter.c_str(), eventtype.c_str(), eventxml.c_str(), eventStatus);
						result = m_sql.safe_query("SELECT ID FROM EventMaster WHERE (Name == '%q')",
							eventname.c_str());
						if (result.size() > 0)
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

					if (eventid == "")
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

								if ((actions.find("SendNotification") != std::string::npos) || (actions.find("SendEmail") != std::string::npos) || (actions.find("SendSMS") != std::string::npos))
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
				if (idx == "")
					return;
				m_sql.DeleteEvent(idx);
				m_mainworker.m_eventsystem.LoadEvents();
				root["status"] = "OK";
			}
			else if (cparam == "currentstates")
			{
				std::vector<CEventSystem::_tDeviceStatus> devStates;
				m_mainworker.m_eventsystem.WWWGetItemStates(devStates);
				if (devStates.size() == 0)
					return;

				int ii = 0;
				std::vector<CEventSystem::_tDeviceStatus>::iterator itt;
				for (itt = devStates.begin(); itt != devStates.end(); ++itt)
				{
					root["title"] = "Current States";
					root["result"][ii]["id"] = itt->ID;
					root["result"][ii]["name"] = itt->deviceName;
					root["result"][ii]["value"] = itt->nValueWording;
					root["result"][ii]["svalues"] = itt->sValue;
					root["result"][ii]["lastupdate"] = itt->lastUpdate;
					ii++;
				}
				root["status"] = "OK";
			}
		}
	}
}
