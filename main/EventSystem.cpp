#include "stdafx.h"
#include "mainworker.h"
#include "RFXNames.h"
#include "EventSystem.h"
#include "Helper.h"
#include "SQLHelper.h"
#include "Logger.h"
#include "../hardware/hardwaretypes.h"
#include <iostream>
#include "../httpclient/HTTPClient.h"
#include "localtime_r.h"
#include "SQLHelper.h"
#include "../notifications/NotificationHelper.h"

#ifdef WIN32
#include "dirent_windows.h"
#else
#include <dirent.h>
#endif

extern "C" {
#include "../lua/src/lua.h"    
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
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
	boost::lock_guard<boost::mutex> l(eventMutex);

	m_events.clear();

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT EventRules.ID,EventMaster.Name,EventRules.Conditions,EventRules.Actions,EventMaster.Status, EventRules.SequenceNo FROM EventRules INNER JOIN EventMaster ON EventRules.EMID=EventMaster.ID ORDER BY EventRules.ID";
	result = m_sql.query(szQuery.str());
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
			eitem.Conditions = sd[2];
			eitem.Actions = sd[3];
			eitem.EventStatus = atoi(sd[4].c_str());
			eitem.SequenceNo = atoi(sd[5].c_str());
			m_events.push_back(eitem);

		}
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "EventSystem: Events (re)loaded");
#endif
	}
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

void CEventSystem::GetCurrentStates()
{
	m_devicestates.clear();

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Name,nValue,sValue, Type, SubType, SwitchType, LastUpdate, LastLevel FROM DeviceStatus WHERE (Used = '1')";
	result = m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			_tDeviceStatus sitem;
			std::stringstream s_str(sd[0]);
			s_str >> sitem.ID;
			sitem.deviceName = sd[1];//utf8_to_string(sd[1].c_str(),std::locale(".1252"));

			std::stringstream nv_str(sd[2]);
			nv_str >> sitem.nValue;
			sitem.sValue = sd[3];
			sitem.devType = atoi(sd[4].c_str());
			sitem.subType = atoi(sd[5].c_str());
			sitem.switchtype = atoi(sd[6].c_str());
			_eSwitchType switchtype = (_eSwitchType)sitem.switchtype;
			sitem.nValueWording = nValueToWording(sitem.devType, sitem.subType, switchtype, (unsigned char)sitem.nValue, sitem.sValue);
			sitem.lastUpdate = sd[7];
			sitem.lastLevel = atoi(sd[8].c_str());
			m_devicestates[sitem.ID] = sitem;
		}
	}
}

void CEventSystem::GetCurrentUserVariables()
{
	m_uservariables.clear();

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Name,Value, ValueType, LastUpdate FROM UserVariables";
	result = m_sql.query(szQuery.str());
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

	//char szTmp[300];

	typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++)
	{
		_tDeviceStatus sitem = iterator->second;
		std::vector<std::string> splitresults;
		StringSplit(sitem.sValue, ";", splitresults);

		if (splitresults.size()==0)
			continue;

		float temp = 0;
		float chill = 0;
		unsigned char humidity = 0;
		float barometer = 0;
		float rainmm = 0;
		float rainmmlasthour = 0;
		float uv = 0;
		float dewpoint = 0;
		float utilityval = 0;
		float winddir = 0;
		float windspeed = 0;
		float windgust = 0;

		bool isTemp = false;
		bool isDew = false;
		bool isHum = false;
		bool isBaro = false;
		bool isBaroFloat = false;
		bool isUtility = false;
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
			temp = static_cast<float>(atof(splitresults[0].c_str()));
			isTemp = true;
			break;
		case pTypeThermostat:
			temp = static_cast<float>(atof(splitresults[0].c_str()));
			isTemp = true;
			break;
		case pTypeThermostat1:
			temp = static_cast<float>(atof(splitresults[0].c_str()));
			isTemp = true;
			break;
		case pTypeHUM:
			humidity = (unsigned char)sitem.nValue;
			isHum = true;
			break;
		case pTypeTEMP_HUM:
			temp = static_cast<float>(atof(splitresults[0].c_str()));
			humidity = atoi(splitresults[1].c_str());
			dewpoint = (float)CalculateDewPoint(temp, humidity);
			isTemp = true;
			isHum = true;
			isDew = true;
			break;
		case pTypeTEMP_HUM_BARO:
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
			temp = static_cast<float>(atof(splitresults[0].c_str()));
			barometer = static_cast<float>(atof(splitresults[1].c_str()));
			isTemp = true;
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
				temp = static_cast<float>(atof(splitresults[0].c_str()));
				isTemp = true;
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
			if (splitresults.size() == 2)
				utilityval = static_cast<float>(atof(splitresults[1].c_str()));
			else
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
			isUtility = true;
			break;
		case pTypePOWER:
			utilityval = static_cast<float>(atof(splitresults[0].c_str()));
			isUtility = true;
			break;
		case pTypeUsage:
			utilityval = static_cast<float>(atof(splitresults[0].c_str()));
			isUtility = true;
			break;
		case pTypeP1Power:
			utilityval = static_cast<float>(atof(splitresults[4].c_str()));
			isUtility = true;
			break;
		case pTypeLux:
			utilityval = static_cast<float>(atof(splitresults[0].c_str()));
			isUtility = true;
			break;
		case pTypeGeneral:
		{
			if (sitem.subType == sTypeVisibility)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			if (sitem.subType == sTypeDistance)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			else if (sitem.subType == sTypeSolarRadiation)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			else if (sitem.subType == sTypePercentage)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			else if (sitem.subType == sTypeVoltage)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			else if (sitem.subType == sTypeCurrent)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			else if (sitem.subType == sTypeSetPoint)
			{
				utilityval = static_cast<float>(atof(splitresults[0].c_str()));
				isUtility = true;
			}
			else if (sitem.subType == sTypeCounterIncremental)
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
				std::stringstream szQuery;
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sitem.ID << " AND Date>='" << szDate << "')";
				result2 = m_sql.query(szQuery.str());
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
					szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << sitem.ID << " AND Date>='" << szDate << "')";
				}
				else
				{
					szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << sitem.ID << " AND Date>='" << szDate << "') ORDER BY ROWID DESC LIMIT 1";
				}
				result2 = m_sql.query(szQuery.str());
				if (result2.size()>0)
				{
					double total_real = 0;
					float rate = 0;
					std::vector<std::string> sd2 = result2[0];
					if (sitem.subType != sTypeRAINWU)
					{
						float total_min = static_cast<float>(atof(sd2[0].c_str()));
						float total_max = static_cast<float>(atof(splitresults[1].c_str()));
						rate = static_cast<float>(atof(sd2[2].c_str()));
						if (sitem.subType == sTypeRAIN2)
							rate /= 100.0f;
						total_real = total_max - total_min;
					}
					else
					{
						rate = static_cast<float>(atof(sd2[2].c_str()));
						total_real = atof(sd2[1].c_str());
					}
					//total_real*=AddjMulti;
					rainmm = float(total_real);
					isRain = true;
					//Calculate Last Hour
					szQuery.clear();
					szQuery.str("");
					sprintf(szDate, "datetime('now', 'localtime', '-%d hour')", 1);
					if (sitem.subType != sTypeRAINWU)
					{
						szQuery << "SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=" << sitem.ID << " AND Date>=" << szDate << ")";
					}
					else
					{
						szQuery << "SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=" << sitem.ID << " AND Date>=" << szDate << ") ORDER BY ROWID DESC LIMIT 1";
					}
					rainmmlasthour = 0;
					result2 = m_sql.query(szQuery.str());
					if (result2.size()>0)
					{
						std::vector<std::string> sd2 = result2[0];
						float r_first = static_cast<float>(atof(sd2[0].c_str()));
						rainmmlasthour = static_cast<float>(atof(splitresults[1].c_str())) - r_first;
					}
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
				std::stringstream szQuery;
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value) FROM Meter WHERE (DeviceRowID=" << sitem.ID << " AND Date>='" << szDate << "')";
				result2 = m_sql.query(szQuery.str());
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
				std::stringstream szQuery;
				szQuery.clear();
				szQuery.str("");
				szQuery << "SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=" << sitem.ID << " AND Date>='" << szDate << "')";
				result2 = m_sql.query(szQuery.str());
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

	boost::lock_guard<boost::mutex> l(eventMutex);

	//_log.Log(LOG_STATUS,"EventSystem: deleted device %d",ulDevID);
	m_devicestates.erase(ulDevID);

}

void CEventSystem::WWWUpdateSingleState(const unsigned long long ulDevID, const std::string &devname)
{
	if (!m_bEnabled)
		return;

	boost::lock_guard<boost::mutex> l(eventMutex);
	std::map<unsigned long long, _tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);
	if (itt != m_devicestates.end()) {
		//Update
		_tDeviceStatus replaceitem = itt->second;
		replaceitem.deviceName = devname;
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

std::string CEventSystem::UpdateSingleState(const unsigned long long ulDevID, const std::string &devname, const int nValue, const char* sValue, const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, const std::string &lastUpdate, const unsigned char lastLevel)
{

	std::string nValueWording = nValueToWording(devType, subType, switchType, nValue, sValue);

	std::map<unsigned long long, _tDeviceStatus>::iterator itt = m_devicestates.find(ulDevID);
	if (itt != m_devicestates.end()) {
		//Update
		_tDeviceStatus replaceitem = itt->second;
		replaceitem.deviceName = devname;
		replaceitem.nValue = nValue;
		replaceitem.sValue = sValue;
		replaceitem.nValueWording = nValueWording;
		replaceitem.lastUpdate = lastUpdate;
		replaceitem.lastLevel = lastLevel;
		itt->second = replaceitem;
	}
	else {
		//Insert
		_tDeviceStatus newitem;
		newitem.ID = ulDevID;
		newitem.deviceName = devname;
		newitem.nValue = nValue;
		newitem.sValue = sValue;
		newitem.nValueWording = nValueWording;
		newitem.lastUpdate = lastUpdate;
		newitem.lastLevel = lastLevel;
		m_devicestates[newitem.ID] = newitem;
	}
	return nValueWording;
}


void CEventSystem::ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string &devname, const int varId)
{
	if (!m_bEnabled)
		return;
	boost::lock_guard<boost::mutex> l(eventMutex);

	// query to get switchtype & LastUpdate, can't seem to get it from SQLHelper?
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT ID, SwitchType, LastUpdate, LastLevel FROM DeviceStatus WHERE (Name == '" << devname << "')";
	result = m_sql.query(szQuery.str());
	if (result.size()>0) {
		std::vector<std::string> sd = result[0];
		_eSwitchType switchType = (_eSwitchType)atoi(sd[1].c_str());

		std::string nValueWording = UpdateSingleState(ulDevID, devname, nValue, sValue, devType, subType, switchType, sd[2], atoi(sd[3].c_str()));
		GetCurrentUserVariables();
		EvaluateEvent("device", ulDevID, devname, nValue, sValue, nValueWording, 0);
	}
	else {
		_log.Log(LOG_ERROR, "EventSystem: Could not determine switch type for event device %s", devname.c_str());
	}
}

void CEventSystem::ProcessMinute()
{
	boost::lock_guard<boost::mutex> l(eventMutex);
	GetCurrentUserVariables();
	EvaluateEvent("time");
}

void CEventSystem::ProcessUserVariable(const unsigned long long varId)
{
	if (!m_bEnabled)
		return;
	boost::lock_guard<boost::mutex> l(eventMutex);
	GetCurrentUserVariables();
	EvaluateEvent("uservariable", varId);
}

void CEventSystem::EvaluateEvent(const std::string &reason)
{
	EvaluateEvent(reason, 0, "", 0, "", "", 0);
}

void CEventSystem::EvaluateEvent(const std::string &reason, const unsigned long long varId)
{
	EvaluateEvent(reason, 0, "", 0, "", "", varId);
}

void CEventSystem::EvaluateEvent(const std::string &reason, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId)
{
	if (!m_bEnabled)
		return;
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
			if (ent->d_type == DT_REG)
			{
				if ((filename.length() < 4) || (filename.compare(filename.length() - 4, 4, ".lua") != 0))
				{
					//_log.Log(LOG_STATUS,"EventSystem: ignore file not .lua: %s",filename.c_str());
				}
				else if (filename.find("_demo.lua") == std::string::npos) //skip demo lua files
				{
					if ((reason == "device") && (filename.find("_device_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename, DeviceID, devname, nValue, sValue, nValueWording, 0);
					}
					else if ((reason == "time") && (filename.find("_time_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename);
					}
					else if ((reason == "security") && (filename.find("_security_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename);
					}
					else if ((reason == "uservariable") && (filename.find("_variable_") != std::string::npos))
					{
						EvaluateLua(reason, lua_Dir + filename, varId);
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
				if (ent->d_type == DT_REG)
				{
					if ((filename.length() < 4) || (filename.compare(filename.length() - 3, 3, ".py") != 0))
					{
						//_log.Log(LOG_STATUS,"EventSystem: ignore file not .lua: %s",filename.c_str());
					}
					else if (filename.find("_demo.py") == std::string::npos) //skip demo python files
					{
						if ((reason == "device") && (filename.find("_device_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename, DeviceID, devname, nValue, sValue, nValueWording, 0);
						}
						else if ((reason == "time") && (filename.find("_time_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename);
						}
						else if ((reason == "security") && (filename.find("_security_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename);
						}
						else if ((reason == "uservariable") && (filename.find("_variable_") != std::string::npos))
						{
							EvaluatePython(reason, python_Dir + filename, varId);
						}
					}
				}
			}
			closedir(lDir);
		}
		else {
			_log.Log(LOG_ERROR, "EventSystem: Error accessing python script directory %s", lua_Dir.c_str());
		}
	}
	catch (...)
	{
	}
#endif

	EvaluateBlockly(reason, DeviceID, devname, nValue, sValue, nValueWording, varId);
}

void CEventSystem::EvaluateBlockly(const std::string &reason, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId)
{

	//#ifdef _DEBUG
	//    _log.Log(LOG_STATUS,"EventSystem: blockly %s trigger",reason.c_str());
	//#endif

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

	lua_createtable(lua_state, (int)m_devicestates.size(), 0);

	typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++) {
		_tDeviceStatus sitem = iterator->second;
		lua_pushnumber(lua_state, (lua_Number)sitem.ID);
		lua_pushstring(lua_state, sitem.nValueWording.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "device");

	lua_createtable(lua_state, (int)m_uservariables.size(), 0);

	typedef std::map<unsigned long long, _tUserVariable>::iterator it_var;
	for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); iterator++) {
		_tUserVariable uvitem = iterator->second;
		if (uvitem.variableType == 0)  {
			//Integer
			lua_pushnumber(lua_state, (lua_Number)uvitem.ID);
			lua_pushnumber(lua_state, atoi(uvitem.variableValue.c_str()));
			lua_rawset(lua_state, -3);
		}
		else if (uvitem.variableType == 1)  {
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

	GetCurrentMeasurementStates();

	if (m_tempValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_tempValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_tempValuesByID.begin(); p != m_tempValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "temperaturedevice");
	}
	if (m_dewValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_dewValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_dewValuesByID.begin(); p != m_dewValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "dewpointdevice");
	}
	if (m_humValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_humValuesByID.size(), 0);
		std::map<unsigned long long, unsigned char>::iterator p;
		for (p = m_humValuesByID.begin(); p != m_humValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "humiditydevice");
	}
	if (m_baroValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_baroValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_baroValuesByID.begin(); p != m_baroValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "barometerdevice");
	}
	if (m_utilityValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_utilityValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_utilityValuesByID.begin(); p != m_utilityValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "utilitydevice");
	}
	if (m_rainValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_rainValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_rainValuesByID.begin(); p != m_rainValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "raindevice");
	}
	if (m_rainLastHourValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_rainLastHourValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_rainLastHourValuesByID.begin(); p != m_rainLastHourValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "rainlasthourdevice");
	}
	if (m_uvValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_uvValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_uvValuesByID.begin(); p != m_uvValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "uvdevice");
	}
	if (m_winddirValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_winddirValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_winddirValuesByID.begin(); p != m_winddirValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "winddirdevice");
	}
	if (m_windspeedValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_windspeedValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
		for (p = m_windspeedValuesByID.begin(); p != m_windspeedValuesByID.end(); ++p)
		{
			lua_pushnumber(lua_state, (lua_Number)p->first);
			lua_pushnumber(lua_state, (lua_Number)p->second);
			lua_rawset(lua_state, -3);
		}
		lua_setglobal(lua_state, "windspeeddevice");
	}
	if (m_windgustValuesByID.size()>0) {
		lua_createtable(lua_state, (int)m_windgustValuesByID.size(), 0);
		std::map<unsigned long long, float>::iterator p;
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

	if ((reason == "device") && (DeviceID >0)) {

		std::size_t found;
		bool eventActive = false;

		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			eventActive = false;
			std::stringstream sstr;
			sstr << "device[" << DeviceID << "]";
			found = it->Conditions.find(sstr.str());
			if (it->EventStatus == 1) { eventActive = true; };

			if ((eventActive) && (found != std::string::npos)) {

				// Replace Sunrise and sunset placeholder with actual time for query
				if (it->Conditions.find("@Sunrise") != std::string::npos) {
					int intRise = getSunRiseSunSetMinutes("Sunrise");
					std::stringstream ssRise;
					ssRise << intRise;
					it->Conditions = stdreplace(it->Conditions, "@Sunrise", ssRise.str());
				}
				if (it->Conditions.find("@Sunset") != std::string::npos) {
					int intSet = getSunRiseSunSetMinutes("Sunset");
					std::stringstream ssSet;
					ssSet << intSet;
					it->Conditions = stdreplace(it->Conditions, "@Sunset", ssSet.str());
				}

				std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + it->Conditions + " then result = 1 end; return result";
				//_log.Log(LOG_STATUS,"EventSystem: ifc: %s",ifCondition.c_str());
				if (luaL_dostring(lua_state, ifCondition.c_str()))
				{
					_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
				}
				else {
					lua_Number ruleTrue = lua_tonumber(lua_state, -1);

					if (ruleTrue != 0)
					{
						if (parseBlocklyActions(it->Actions, it->Name, it->ID))
						{
							_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
						}
					}
				}
			}
		}
	}
	else if (reason == "security") {
		// security status change
		std::size_t found;
		bool eventActive = false;

		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			eventActive = false;
			std::stringstream sstr;
			sstr << "securitystatus";
			found = it->Conditions.find(sstr.str());
			if (it->EventStatus == 1) { eventActive = true; };

			if ((eventActive) && (found != std::string::npos)) {

				// Replace Sunrise and sunset placeholder with actual time for query
				if (it->Conditions.find("@Sunrise") != std::string::npos) {
					int intRise = getSunRiseSunSetMinutes("Sunrise");
					std::stringstream ssRise;
					ssRise << intRise;
					it->Conditions = stdreplace(it->Conditions, "@Sunrise", ssRise.str());
				}
				if (it->Conditions.find("@Sunset") != std::string::npos) {
					int intSet = getSunRiseSunSetMinutes("Sunset");
					std::stringstream ssSet;
					ssSet << intSet;
					it->Conditions = stdreplace(it->Conditions, "@Sunset", ssSet.str());
				}

				std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + it->Conditions + " then result = 1 end; return result";
				//_log.Log(LOG_NORM,"ifc: %s",ifCondition.c_str());
				if (luaL_dostring(lua_state, ifCondition.c_str()))
				{
					_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
				}
				else {
					lua_Number ruleTrue = lua_tonumber(lua_state, -1);

					if (ruleTrue != 0)
					{
						if (parseBlocklyActions(it->Actions, it->Name, it->ID))
						{
							_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
						}
					}
				}
			}
		}
	}

	else if (reason == "time") {

		bool eventActive = false;

		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			eventActive = false;
			if (it->EventStatus == 1) { eventActive = true; };
			if (eventActive) {
				// time rules will only run when time or date based critera are found
				if ((it->Conditions.find("timeofday") != std::string::npos) || (it->Conditions.find("weekday") != std::string::npos)) {

					// Replace Sunrise and sunset placeholder with actual time for query
					if (it->Conditions.find("@Sunrise") != std::string::npos) {
						int intRise = getSunRiseSunSetMinutes("Sunrise");
						std::stringstream ssRise;
						ssRise << intRise;
						it->Conditions = stdreplace(it->Conditions, "@Sunrise", ssRise.str());
					}
					if (it->Conditions.find("@Sunset") != std::string::npos) {
						int intSet = getSunRiseSunSetMinutes("Sunset");
						std::stringstream ssSet;
						ssSet << intSet;
						it->Conditions = stdreplace(it->Conditions, "@Sunset", ssSet.str());
					}

					std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + it->Conditions + " then result = 1 end; return result";
					//_log.Log(LOG_NORM,"ifc: %s",ifCondition.c_str());
					if (luaL_dostring(lua_state, ifCondition.c_str()))
					{
						_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
					}
					else {
						lua_Number ruleTrue = lua_tonumber(lua_state, -1);
						if (ruleTrue != 0)
						{
							if (parseBlocklyActions(it->Actions, it->Name, it->ID))
							{
								_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
							}
						}
					}
				}
			}
		}
	}

	if ((reason == "uservariable") && (varId >0)) {

		std::size_t found;
		bool eventActive = false;

		std::vector<_tEventItem>::iterator it;
		for (it = m_events.begin(); it != m_events.end(); ++it) {
			eventActive = false;
			std::stringstream sstr;
			sstr << "variable[" << varId << "]";
			found = it->Conditions.find(sstr.str());
			if (it->EventStatus == 1) { eventActive = true; };

			if ((eventActive) && (found != std::string::npos)) {

				// Replace Sunrise and sunset placeholder with actual time for query
				if (it->Conditions.find("@Sunrise") != std::string::npos) {
					int intRise = getSunRiseSunSetMinutes("Sunrise");
					std::stringstream ssRise;
					ssRise << intRise;
					it->Conditions = stdreplace(it->Conditions, "@Sunrise", ssRise.str());
				}
				if (it->Conditions.find("@Sunset") != std::string::npos) {
					int intSet = getSunRiseSunSetMinutes("Sunset");
					std::stringstream ssSet;
					ssSet << intSet;
					it->Conditions = stdreplace(it->Conditions, "@Sunset", ssSet.str());
				}

				std::string ifCondition = "result = 0; weekday = os.date('*t')['wday']; timeofday = ((os.date('*t')['hour']*60)+os.date('*t')['min']); if " + it->Conditions + " then result = 1 end; return result";
				//_log.Log(LOG_STATUS,"ifc: %s",ifCondition.c_str());
				if (luaL_dostring(lua_state, ifCondition.c_str()))
				{
					_log.Log(LOG_ERROR, "EventSystem: Lua script error (Blockly), Name: %s => %s", it->Name.c_str(), lua_tostring(lua_state, -1));
				}
				else {
					lua_Number ruleTrue = lua_tonumber(lua_state, -1);

					if (ruleTrue != 0)
					{
						if (parseBlocklyActions(it->Actions, it->Name, it->ID))
						{
							_log.Log(LOG_NORM, "EventSystem: Event triggered: %s", it->Name.c_str());
						}
					}
				}
			}
		}
	}

	lua_close(lua_state);
}


bool CEventSystem::parseBlocklyActions(const std::string &Actions, const std::string &eventName, const unsigned long long eventID)
{
	if (isEventscheduled(eventName))
	{
		//_log.Log(LOG_NORM,"Already scheduled this event, skipping");
		return false;
	}

	std::istringstream ss(Actions);
	std::string csubstr;
	bool actionsDone = false;
	while (!ss.eof()) {
		getline(ss, csubstr, ',');
		if ((csubstr.find_first_of("=") == std::string::npos) || (csubstr.find_first_of("[") == std::string::npos) || (csubstr.find_first_of("]") == std::string::npos)) {
			_log.Log(LOG_ERROR, "EventSystem: Malformed action sequence!");
			break;
		}
		size_t eQPos = csubstr.find_first_of("=") + 1;
		std::string doWhat = csubstr.substr(eQPos);
		doWhat = doWhat.substr(1, doWhat.size() - 2);
		size_t sPos = csubstr.find_first_of("[") + 1;
		size_t ePos = csubstr.find_first_of("]");

		size_t sDiff = ePos - sPos;
		if (sDiff>0) {
			int sceneType = 0;
			std::string deviceName = csubstr.substr(sPos, sDiff);
			std::string variableNo = "0";
			bool isScene = false;
			bool isVariable = false;
			if ((deviceName.find("Scene:") == 0) || (deviceName.find("Group:") == 0))
			{
				isScene = true;
				sceneType = 1;
				if (deviceName.find("Group:") == 0) {
					sceneType = 2;
				}
				deviceName = deviceName.substr(6);
			}
			else if (deviceName.find("Variable:") == 0)
			{
				isVariable = true;
				variableNo = deviceName.substr(9);
				deviceName = "0";
			}

			int deviceNo = atoi(deviceName.c_str());
			if (deviceNo && !isScene && !isVariable) {
				if (m_devicestates.count(deviceNo)) {
					if (ScheduleEvent(deviceNo, doWhat, isScene, eventName, sceneType)) {
						actionsDone = true;
					}
				}
				else {
					reportMissingDevice(deviceNo, eventName, eventID);
				}
			}
			else if (deviceNo && isScene) {
				if (ScheduleEvent(deviceNo, doWhat, isScene, eventName, sceneType)) {
					actionsDone = true;
				}
			}
			else if (isVariable)
			{
				int afterTimerSeconds = 0;
				size_t aFind = doWhat.find(" AFTER ");
				if ((aFind > 0) && (aFind != std::string::npos)) {
					std::string delayString = doWhat.substr(aFind + 7);
					std::string newAction = doWhat.substr(0, aFind - 1);
					afterTimerSeconds = atoi(delayString.c_str());
					doWhat = newAction;
				}
				if (afterTimerSeconds == 0)
				{
					std::vector<std::vector<std::string> > result;
					char szTmp[300];
					sprintf(szTmp, "SELECT Name, ValueType FROM UserVariables WHERE (ID == '%s')", variableNo.c_str());
					result = m_sql.query(szTmp);
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
					int DelayTime = 1 + afterTimerSeconds;
					_tTaskItem tItem;
					tItem = _tTaskItem::SetVariable(DelayTime, (const unsigned long long)atol(variableNo.c_str()), doWhat, false);
					m_sql.AddTaskItem(tItem);
				}
			}
			else {
				std::string devNameNoQuotes = deviceName.substr(1, deviceName.size() - 2);
				if (devNameNoQuotes == "SendNotification") {
					std::string subject(""), body(""), priority("0"), sound("");
					std::vector<std::string> aParam;
					StringSplit(doWhat, "#", aParam);
					subject = body = aParam[0];
					if (aParam.size() > 1)
					{
						body = aParam[1];
					}
					if (aParam.size() == 3)
					{
						priority = aParam[2];
					}
					else if (aParam.size() == 4)
					{
						priority = aParam[2];
						sound = aParam[3];
					}
					SendEventNotification(subject, body, std::string(""), atoi(priority.c_str()), sound);
					actionsDone = true;
				}
				else if (devNameNoQuotes == "SendEmail") {
					std::string subject(""), body(""), to("");
					std::vector<std::string> aParam;
					StringSplit(doWhat, "#", aParam);
					if (aParam.size() !=3 )
					{
						//Invalid
						_log.Log(LOG_ERROR, "EventSystem: SendEmail, not enough parameters!");
						return false;
					}
					subject = aParam[0];
					body = aParam[1];
					body = stdreplace(body, "\\n", "<br>");
					to = aParam[2];
					m_sql.AddTaskItem(_tTaskItem::SendEmailTo(1, subject, body, to));
					actionsDone = true;
				}
				else if (devNameNoQuotes == "OpenURL") {
					OpenURL(doWhat);
					actionsDone = true;
				}
				else if (devNameNoQuotes.find("WriteToLog") == 0) {
					WriteToLog(devNameNoQuotes,doWhat);
					actionsDone = true;
				}
			}

		}
	}
	return actionsDone;
}

#ifdef ENABLE_PYTHON

void CEventSystem::EvaluatePython(const std::string &reason, const std::string &filename, const unsigned long long varId)
{
	EvaluatePython(reason, filename, 0, "", 0, "", "", varId);
}

void CEventSystem::EvaluatePython(const std::string &reason, const std::string &filename)
{
	EvaluatePython(reason, filename, 0, "", 0, "", "", 0);
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

void CEventSystem::EvaluatePython(const std::string &reason, const std::string &filename, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId)
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
		class_<CEventSystem, boost::noncopyable >("Domoticz", no_init)
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

		typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
		for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++)
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
		
		
		int intRise = getSunRiseSunSetMinutes("Sunrise");
		int intSet = getSunRiseSunSetMinutes("Sunset");
		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now, &ltime);
		int minutesSinceMidnight = (ltime.tm_hour * 60) + ltime.tm_min;
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
		typedef std::map<unsigned long long, _tUserVariable>::iterator it_var;
		for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); iterator++) {
			_tUserVariable uvitem = iterator->second;
			//user_variables[uvitem.variableName] = uvitem;
			if (uvitem.variableType == 0)  {
				//Integer
				main_namespace[uvitem.variableName] = atoi(uvitem.variableValue.c_str());
				user_variables[uvitem.variableName] = main_namespace[uvitem.variableName];
			}
			else if (uvitem.variableType == 1)  {
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
		_log.Log(LOG_ERROR, "%s",formatted_str.c_str());
	}
	fclose(PythonScriptFile);
	//Py_Finalize();
}
#endif // ENABLE_PYTHON

void CEventSystem::EvaluateLua(const std::string &reason, const std::string &filename, const unsigned long long varId)
{
	EvaluateLua(reason, filename, 0, "", 0, "", "", varId);
}

void CEventSystem::EvaluateLua(const std::string &reason, const std::string &filename)
{
	EvaluateLua(reason, filename, 0, "", 0, "", "", 0);
}

void CEventSystem::EvaluateLua(const std::string &reason, const std::string &filename, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording, const unsigned long long varId)
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
	struct tm ltime;
	localtime_r(&now, &ltime);
	int minutesSinceMidnight = (ltime.tm_hour * 60) + ltime.tm_min;
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
		std::map<std::string, unsigned char>::iterator p;
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
	}

	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushstring(lua_state, sitem.nValueWording.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices");

	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushstring(lua_state, sitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_lastupdate");

	lua_createtable(lua_state, (int)m_devicestates.size(), 0);
	typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++)
	{
		_tDeviceStatus sitem = iterator->second;
		lua_pushstring(lua_state, sitem.deviceName.c_str());
		lua_pushstring(lua_state, sitem.sValue.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "otherdevices_svalues");

	lua_createtable(lua_state, (int)m_uservariables.size(), 0);

	typedef std::map<unsigned long long, _tUserVariable>::iterator it_var;
	for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); iterator++) {
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

	lua_createtable(lua_state, (int)m_uservariables.size(), 0);

	typedef std::map<unsigned long long, _tUserVariable>::iterator it_var;
	for (it_var iterator = m_uservariables.begin(); iterator != m_uservariables.end(); iterator++) {
		_tUserVariable uvitem = iterator->second;
		lua_pushstring(lua_state, uvitem.variableName.c_str());
		lua_pushstring(lua_state, uvitem.lastUpdate.c_str());
		lua_rawset(lua_state, -3);
	}
	lua_setglobal(lua_state, "uservariables_lastupdate");

	if (reason == "uservariable") {
		if (varId > 0) {
			typedef std::map<unsigned long long, _tUserVariable>::iterator it_cvar;
			for (it_cvar iterator = m_uservariables.begin(); iterator != m_uservariables.end(); iterator++) {
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


	int status = luaL_loadfile(lua_state, filename.c_str());

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
		report_errors(lua_state, status);
		lua_close(lua_state);
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
			_log.Log(LOG_ERROR, "EventSystem: commandArray should only return ['string']='actionstring' or [integer]={['string']='actionstring'}");
		}
		// removes 'value'; keeps 'key' for next iteration
		lua_pop(lua_state, 1);
	}

	return scriptTrue;

}

bool CEventSystem::processLuaCommand(lua_State *lua_state, const std::string &filename)
{
	bool scriptTrue = false;
	if (std::string(lua_tostring(lua_state, -2)) == "SendNotification")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		std::string subject(""), body(""), priority("0"), sound("");
		std::vector<std::string> aParam;
		StringSplit(luaString, "#", aParam);
		subject = body = aParam[0];
		if (aParam.size() > 1)
		{
			body = aParam[1];
		}
		if (aParam.size() == 3)
		{
			priority = aParam[2];
		}
		else if (aParam.size() == 4)
		{
			priority = aParam[2];
			sound = aParam[3];
		}
		SendEventNotification(subject, body, std::string(""), atoi(priority.c_str()), sound);
		scriptTrue = true;
	}
	else if (std::string(lua_tostring(lua_state, -2)) == "SendEmail") {
		std::string luaString = lua_tostring(lua_state, -1);
		std::string subject(""), body(""), to("");
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
		body = stdreplace(body, "\\n", "<br>");
		to = aParam[2];
		m_sql.AddTaskItem(_tTaskItem::SendEmailTo(1, subject, body, to));
		scriptTrue = true;
	}
	else if (std::string(lua_tostring(lua_state, -2)) == "OpenURL")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		OpenURL(luaString);
		scriptTrue = true;
	}
	else if (std::string(lua_tostring(lua_state, -2)) == "UpdateDevice")
	{
		std::string luaString = lua_tostring(lua_state, -1);
		UpdateDevice(luaString);
		scriptTrue = true;
	}
	else if (std::string(lua_tostring(lua_state, -2)).find("Variable:") == 0)
	{
		std::string variableName = std::string(lua_tostring(lua_state, -2)).substr(9);
		std::string variableValue = lua_tostring(lua_state, -1);

		std::vector<std::vector<std::string> > result;
		char szTmp[300];

		int afterTimerSeconds = 0;
		size_t aFind = variableValue.find(" AFTER ");
		if ((aFind > 0) && (aFind != std::string::npos)) {
			std::string delayString = variableValue.substr(aFind + 7);
			std::string newAction = variableValue.substr(0, aFind);
			afterTimerSeconds = atoi(delayString.c_str());
			variableValue = newAction;
		}
		sprintf(szTmp, "SELECT ID, ValueType FROM UserVariables WHERE (Name == '%s')", variableName.c_str());
		result = m_sql.query(szTmp);
		if (result.size() > 0)
		{
			std::vector<std::string> sd = result[0];
			if (afterTimerSeconds == 0)
			{
				std::string updateResult = m_sql.UpdateUserVariable(sd[0], variableName, sd[1], variableValue, false);
				if (updateResult != "OK") {
					_log.Log(LOG_ERROR, "EventSystem: Error updating variable %s: %s", variableName.c_str(), updateResult.c_str());
				}
			}
			else
			{
				int DelayTime = 1 + afterTimerSeconds;
				unsigned long long idx;
				std::stringstream sstr;
				sstr << sd[0];
				sstr >> idx;
				_tTaskItem tItem;
				tItem = _tTaskItem::SetVariable(DelayTime, idx, variableValue, false);
				m_sql.AddTaskItem(tItem);
			}
			scriptTrue = true;
		}
	}
	else
	{
		if (ScheduleEvent(lua_tostring(lua_state, -2), lua_tostring(lua_state, -1), filename)) {
			scriptTrue = true;
		}
	}

	return scriptTrue;

}


void CEventSystem::report_errors(lua_State *L, int status)
{
	if (status != 0) {
		_log.Log(LOG_ERROR, "EventSystem: %s", lua_tostring(L, -1));
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
	std::stringstream szQuery;
	szQuery << "SELECT HardwareID, DeviceID, Unit, Type, SubType, Name, SwitchType, LastLevel FROM DeviceStatus WHERE (ID==" << idx << ")";
	result = m_sql.query(szQuery.str());
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

		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now, &ltime);

		char szLastUpdate[40];
		sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET nValue=" << nvalue << ", sValue='" << svalue << "', LastUpdate='" << szLastUpdate << "' WHERE (ID = " << idx << ")";
		result = m_sql.query(szQuery.str());


		unsigned long long ulIdx = 0;
		std::stringstream s_str(idx);
		s_str >> ulIdx;

		int idtype = atoi(dtype.c_str());
		int idsubtype = atoi(dsubtype.c_str());

		UpdateSingleState(ulIdx, dname, atoi(nvalue.c_str()), svalue.c_str(), idtype, idsubtype, dswitchtype, szLastUpdate, dlastlevel);

		//Check if it's a setpoint device, and if so, set the actual setpoint

		if (
			((idtype == pTypeThermostat) && (idsubtype == sTypeThermSetpoint)) ||
			(idtype == pTypeRadiator1)
			)
		{
			_log.Log(LOG_NORM, "EventSystem: Sending SetPoint to device....");
			m_mainworker.SetSetPoint(idx, static_cast<float>(atof(svalue.c_str())));
		}
		else if ((idtype == pTypeGeneral) && (idsubtype == sTypeZWaveThermostatMode))
		{
			_log.Log(LOG_NORM, "EventSystem: Sending Thermostat Mode to device....");
			m_mainworker.SetZWaveThermostatMode(idx, atoi(nvalue.c_str()));
		}
		else if ((idtype == pTypeGeneral) && (idsubtype == sTypeZWaveThermostatFanMode))
		{
			_log.Log(LOG_NORM, "EventSystem: Sending Thermostat Fan Mode to device....");
			m_mainworker.SetZWaveThermostatFanMode(idx, atoi(nvalue.c_str()));
		}
	}
}

void CEventSystem::SendEventNotification(const std::string &Subject, const std::string &Body, const std::string &ExtraData, const int Priority, const std::string &Sound)
{
	m_notifications.SendMessageEx(NOTIFYALL, Subject, Body, ExtraData, Priority, Sound, true);
}

void CEventSystem::OpenURL(const std::string &URL)
{
	_log.Log(LOG_STATUS, "EventSystem: Fetching url...");
	_tTaskItem tItem;
	tItem = _tTaskItem::GetHTTPPage(1, URL, "OpenURL");
	m_sql.AddTaskItem(tItem);
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
		_log.Log(LOG_STATUS, "%s", m_devicestates[atoi(doWhat.c_str())].sValue.c_str());
	}
	else if (devNameNoQuotes == "WriteToLogSwitch")
	{
		_log.Log(LOG_STATUS, "%s", m_devicestates[atoi(doWhat.c_str())].nValueWording.c_str());
	}
}

bool CEventSystem::ScheduleEvent(std::string deviceName, const std::string &Action, const std::string &eventName)
{
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

	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;

	if (isScene) {
		szQuery << "SELECT ID FROM Scenes WHERE (Name == '" << deviceName << "')";
	}
	else {
		szQuery << "SELECT ID FROM DeviceStatus WHERE (Name == '" << deviceName << "')";
	}
	result = m_sql.query(szQuery.str());
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
	std::string previousState = m_devicestates[deviceID].nValueWording;
	unsigned char previousLevel = calculateDimLevel(deviceID, m_devicestates[deviceID].lastLevel);

	int suspendTimer = 0;
	int randomTimer = 0;
	int afterTimerSeconds = 0;

	size_t aFind = Action.find(" FOR ");
	if ((aFind > 0) && (aFind != std::string::npos)) {
		std::string delayString = Action.substr(aFind + 5);
		std::string newAction = Action.substr(0, aFind);
		suspendTimer = atoi(delayString.c_str());
		if (suspendTimer > 0)
		{
			Action = newAction;
		}
	}
	size_t rFind = Action.find(" RANDOM ");
	if ((rFind > 0) && (rFind != std::string::npos))
	{
		std::string delayString = Action.substr(rFind + 8);
		std::string newAction = Action.substr(0, rFind);
		randomTimer = atoi(delayString.c_str());
		if (randomTimer > 0)
		{
			Action = newAction;
		}
	}
	aFind = Action.find(" AFTER ");
	if ((aFind > 0) && (aFind != std::string::npos)) {
		std::string delayString = Action.substr(aFind + 7);
		std::string newAction = Action.substr(0, aFind);
		afterTimerSeconds = atoi(delayString.c_str());
		Action = newAction;
	}

	unsigned char _level = 0;
	if (Action.find("Set Level") == 0)
	{
		_level = calculateDimLevel(deviceID, atoi(Action.substr(10).c_str()));
		Action = Action.substr(0, 9);
	}


	int DelayTime = 1;

	if (randomTimer > 0) {
		int rTime;
		srand((unsigned int)mytime(NULL));
		rTime = rand() % randomTimer + 1;
		DelayTime = (rTime * 60) + 5; //prevent it from running again immediately the next minute if blockly script doesn't handle that
		//alreadyScheduled = isEventscheduled(deviceID, randomTimer, isScene);
	}
	if (afterTimerSeconds > 0)
	{
		DelayTime += afterTimerSeconds;
	}

	_tTaskItem tItem;

	if (isScene) {
		//Scenes
		if ((Action == "On") || (Action == "Off")) {
			tItem = _tTaskItem::SwitchSceneEvent(DelayTime, deviceID, Action, eventName);
		}
		else if (Action == "Active") {
			char szTmp[200];
			std::vector<std::vector<std::string> > result;
			sprintf(szTmp, "UPDATE SceneTimers SET Active=1 WHERE (SceneRowID == %d)", deviceID);
			result = m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}
		else if (Action == "Inactive") {
			char szTmp[200];
			std::vector<std::vector<std::string> > result;
			sprintf(szTmp, "UPDATE SceneTimers SET Active=0 WHERE (SceneRowID == %d)", deviceID);
			result = m_sql.query(szTmp);
			m_mainworker.m_scheduler.ReloadSchedules();
		}
	}
	else {
		//Lights/Switches
		bool bIsOn = IsLightSwitchOn(Action);
		if (bIsOn)
		{
			//Get Device details, check for switch global OnDelay (stored in AddjValue2)
			std::vector<std::vector<std::string> > result;
			std::stringstream szQuery;
			szQuery << "SELECT AddjValue2 FROM DeviceStatus WHERE (ID == " << deviceID << ")";
			result = m_sql.query(szQuery.str());
			if (result.size() < 1)
				return false;

			std::vector<std::string> sd = result[0];

			int iOnDelay = atoi(sd[0].c_str());
			DelayTime += iOnDelay;
		}

		tItem = _tTaskItem::SwitchLightEvent(DelayTime, deviceID, Action, _level, -1, eventName);
	}
	m_sql.AddTaskItem(tItem);

	if (suspendTimer > 0)
	{
		DelayTime = (suspendTimer * 60) + 5; //prevent it from running again immediately the next minute if blockly script doesn't handle that
		_tTaskItem delayedtItem;
		if (isScene) {
			if (Action == "On") {
				delayedtItem = _tTaskItem::SwitchSceneEvent(DelayTime, deviceID, "Off", eventName);
			}
			else {
				delayedtItem = _tTaskItem::SwitchSceneEvent(DelayTime, deviceID, "On", eventName);
			}
		}
		else {
			delayedtItem = _tTaskItem::SwitchLightEvent(DelayTime, deviceID, previousState, previousLevel, -1, eventName);
		}
		m_sql.AddTaskItem(delayedtItem);
	}

	return true;
}



std::string CEventSystem::nValueToWording(const unsigned char dType, const unsigned char dSubType, const _eSwitchType switchtype, const unsigned char nValue, const std::string &sValue)
{

	std::string lstatus = "";
	int llevel = 0;
	bool bHaveDimmer = false;
	bool bHaveGroupCmd = false;
	int maxDimLevel = 0;

	GetLightStatus(dType, dSubType, switchtype,nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

	if (lstatus.find("Set Level") == 0)
	{
		lstatus = "Set Level";
	}

	if (switchtype == STYPE_Dimmer)
	{
		//?
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
			_log.Log(LOG_NORM, "LUA: %s", lua_tostring(lua_state, i));
		}
		else
		{
			/* non strings? */
		}
	}
	return 0;
}

void CEventSystem::reportMissingDevice(const int deviceID, const std::string &eventName, const unsigned long long eventID)
{
	_log.Log(LOG_ERROR, "EventSystem: Device no. '%d' used in event '%s' no longer exists, disabling event!", deviceID, eventName.c_str());


	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT EventMaster.ID FROM EventMaster INNER JOIN EventRules ON EventRules.EMID=EventMaster.ID WHERE (EventRules.ID == '" << eventID << "')";
	result = m_sql.query(szQuery.str());
	if (result.size()>0)
	{

		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			std::stringstream szRemQuery;
			int eventStatus = 2;
			szRemQuery << "UPDATE EventMaster SET Status ='" << eventStatus << "' WHERE (ID == '" << sd[0] << "')";
			m_sql.query(szQuery.str());
		}
	}
}

void CEventSystem::WWWGetItemStates(std::vector<_tDeviceStatus> &iStates)
{
	if (!m_bEnabled)
		return;

	boost::lock_guard<boost::mutex> l(eventMutex);

	iStates.clear();
	typedef std::map<unsigned long long, _tDeviceStatus>::iterator it_type;
	for (it_type iterator = m_devicestates.begin(); iterator != m_devicestates.end(); iterator++)
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
	std::stringstream szQuery;
	szQuery << "SELECT Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == " << deviceID << ")";
	result = m_sql.query(szQuery.str());
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
			}
		}
	}
	return ilevel;

}
