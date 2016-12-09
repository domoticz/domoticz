#include "stdafx.h"
#include "Regulator.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../webserver/Base64.h"
#include "../main/WebServer.h"
#include "../main/LuaHandler.h"
#include "../main/WebServerHelper.h"
#include "../main/mainworker.h"

#include "RegDevice.h"

extern http::server::CWebServerHelper m_webservers;


CRegulator::CRegulator(const int ID, 
	const unsigned short refresh) :
m_refresh(refresh)
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_dHysteresis = 0.5;
}

CRegulator::~CRegulator(void)
{
}

bool CRegulator::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

bool CRegulator::StartHardware()
{
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRegulator::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CRegulator::StopHardware()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CRegulator::Do_Work()
{
	int polling_interval_sec = m_refresh;
	_log.Log(LOG_STATUS, "Regulator: Worker started...");

	// load the devices to manage
	ReadDatabase();

	// initial run
	Process();

	int sec_counter = polling_interval_sec - 1;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % polling_interval_sec == 0)
		{
			Process();
		}
	}

	_log.Log(LOG_STATUS,"Regulator: Worker stopped...");
}


bool CRegulator::GetSwitchState(std::string sSwitchDevID, bool &bvalue)
{
	Json::Value tempjson;
	std::stringstream sstr;
	m_webservers.GetJSonDevices(tempjson, "", "light", "ID", sSwitchDevID.c_str(), "", "", true, false, false, 0, "");

	size_t tsize = tempjson.size();
	if (tsize < 1) return false;

	Json::Value::const_iterator itt;
	Json::ArrayIndex rsize = tempjson["result"].size();
	if (rsize < 1) return false;

	bvalue = tempjson["result"][0]["Status"] == "On";
	return bvalue;
}

bool CRegulator::GetTemperature(std::string sTempDevID, float &fvalue, time_t& lastUpdate)
{
	Json::Value tempjson;
	std::stringstream sstr;
	m_webservers.GetJSonDevices(tempjson, "", "temp", "ID", sTempDevID.c_str(), "", "", true, false, false, 0, "");

	size_t tsize = tempjson.size();
	if (tsize < 1) return false;

	Json::Value::const_iterator itt;
	Json::ArrayIndex rsize = tempjson["result"].size();
	if (rsize < 1) return false;

	fvalue = tempjson["result"][0]["Temp"].asFloat();
	return true;
}

bool CRegulator::GetSetpoint(std::string sSetpointDevID, float &fvalue)
{
	Json::Value tempjson;
	std::stringstream sstr;
	m_webservers.GetJSonDevices(tempjson, "", "utility", "ID", sSetpointDevID.c_str(), "", "", true, false, false, 0, "");

	size_t tsize = tempjson.size();
	if (tsize < 1) return false;

	Json::Value::const_iterator itt;
	Json::ArrayIndex rsize = tempjson["result"].size();
	if (rsize < 1) return false;

	std::string v = tempjson["result"][0]["SetPoint"].asString();
	fvalue = (float)atof(v.c_str());
	return true;
}

void CRegulator::ReadDatabase()
{
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT R.TemperatureDeviceID,R.SwitchDeviceID,R.SetpointDeviceID "
		"FROM Regulators as R "
		"WHERE (R.RegulatorHardwareID=='%d')",m_HwdID);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			CRegDevice item;
			std::vector<std::string> sd = *itt;

			item.m_iTempDevID = atoi(sd[0].c_str());
			item.m_iSwitchDevID = atoi(sd[1].c_str());
			item.m_iSetpointDevID = atoi(sd[2].c_str());
			m_regulatedDevices.push_back(item);
		}
	}

}

void CRegulator::Process()
{
	std::vector<CRegDevice>::const_iterator itt;
	for (itt = m_regulatedDevices.begin(); itt != m_regulatedDevices.end(); ++itt)
	{
		CRegDevice reg = *itt;
		float fSetpoint = 0.0;
		float fCurrentTemp = 0.0;
		//bool bSwitchIsOn = false;
		time_t lastUpdTime;
		std::string setpointid = boost::to_string(reg.m_iSetpointDevID);
		std::string temperid = boost::to_string(reg.m_iTempDevID);
		std::string switchid = boost::to_string(reg.m_iSwitchDevID);
		std::stringstream s_str(switchid);
		unsigned long long SWID;
		s_str >> SWID;

		// get value for setpoint, temperature and switch state
		bool bGetSetpoint = GetSetpoint(setpointid, fSetpoint);
		bool bGetTemp = GetTemperature(temperid, fCurrentTemp, lastUpdTime);
		//bool bGetSwitch = GetSwitchState(switchid, bSwitchIsOn);

		if (!bGetSetpoint || !bGetTemp) // could not read temperature or setpoint value
		{
			// turn the switch off
			_log.Log(LOG_ERROR, "Regulator: could not read temperature sensor or setpoint value - turning switch OFF");
			bool bSuccess = m_mainworker.SwitchLight(SWID, "Off", 0, 0, false, 0);
		}
		else // setpoint & temp are known
		{
			// compare setpoint & temperature
			bool bTurnItOn = false;
			if (fCurrentTemp < fSetpoint + m_dHysteresis) bTurnItOn = true;
			else bTurnItOn = false;
			
			if (bTurnItOn)
			{
				//_log.Log(LOG_NORM, "Regulator: turning switch ON");
				bool bSuccess = m_mainworker.SwitchLight(SWID, "On", 0, 0, true/*only on change*/, 0);
			}
			else
			{
				//_log.Log(LOG_NORM, "Regulator: turning switch OFF");
				bool bSuccess = m_mainworker.SwitchLight(SWID, "Off", 0, 0, true/*only on change*/, 0);
			}
		}
	}//for each association

}
