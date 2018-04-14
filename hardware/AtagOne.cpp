#include "stdafx.h"
#include "AtagOne.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/WebServerHelper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

extern http::server::CWebServerHelper m_webservers;

//Inspidred by https://github.com/kozmoz/atag-one-api

#define ATAGONE_URL_LOGIN "https://portal.atag-one.com/Account/Login"
#define ATAGONE_URL_DEVICE_HOME "https://portal.atag-one.com/Home/Index/{0}"
#define ATAGONE_URL_DIAGNOSTICS "https://portal.atag-one.com/Device/LatestReport"
#define ATAGONE_URL_UPDATE_DEVICE_CONTROL "https://portal.atag-one.com/Home/UpdateDeviceControl/?deviceId={0}"
#define ATAGONE_URL_DEVICE_SET_SETPOINT "https://portal.atag-one.com/Home/DeviceSetSetpoint"
#define ATAGONE_URL_AUTOMATICMODE_CONTROL "https://portal.atag-one.com/Home/AutomaticMode/?deviceId={0}"
#define ATAGONE_URL_DEVICE_CONTROL "https://portal.atag-one.com/Home/DeviceControl/{0}"

#define ATAGONE_TEMPERATURE_MIN 4
#define ATAGONE_TEMPERATURE_MAX 27

#ifdef _DEBUG
	//#define DEBUG_AtagOneThermostat
#endif

#ifdef DEBUG_AtagOneThermostat
void SaveString2Disk(std::string str, std::string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_AtagOneThermostat_read
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	std::string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif

CAtagOne::CAtagOne(const int ID, const std::string &Username, const std::string &Password, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6):
m_UserName(Username),
m_Password(Password)
{
	stdstring_trim(m_UserName);
	stdstring_trim(m_Password);
	m_HwdID=ID;
	m_OutsideTemperatureIdx = 0; //use build in
	m_LastMinute = -1;
	m_ThermostatID = "";
	SetModes(Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
	Init();
}

CAtagOne::~CAtagOne(void)
{
}

void CAtagOne::SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6)
{
	m_OutsideTemperatureIdx = Mode1;
}

void CAtagOne::Init()
{
	m_ThermostatID = "";
	m_bDoLogin = true;
	m_stoprequested = false;
}

bool CAtagOne::StartHardware()
{
	Init();
	m_LastMinute = -1;
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CAtagOne::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CAtagOne::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

std::string GetFirstDeviceID(const std::string &shtml)
{
	std::string sResult = shtml;
	// Evsdd - Updated string due to webpage change
	// Original format: <tr onclick="javascript:changeDeviceAndRedirect('/Home/Index/{0}','6808-1401-3109_15-30-001-544');">
	// New format: "/Home/Index/6808-1401-3109_15-30-001-544"
	size_t tpos = sResult.find("/Home/Index");
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(tpos);
	tpos = sResult.find("x/");
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(tpos + 2);
	tpos = sResult.find("\"");
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(0, tpos);
	return sResult;
}

std::string CAtagOne::GetRequestVerificationToken(const std::string &url)
{
	std::string sResult;
#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne_requesttoken.txt");
#else
	std::string sURL = url;
	stdreplace(sURL,"{0}", m_ThermostatID);

	if (!HTTPClient::GET(sURL, sResult))
	{
		_log.Log(LOG_ERROR, "AtagOne: Error requesting token!");
		return "";
	}
#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_requesttoken.txt");
#endif
#endif
	// <input name="__RequestVerificationToken" type="hidden" value="lFVlMZlt2-YJKAwZWS_K_p3gsQWjZOvBNBZ3lM8io_nFGFL0oRsj4YwQUdqGfyrEqGwEUPmm0FgKH1lPWdk257tuTWQ1" /> 
	size_t tpos = sResult.find("__RequestVerificationToken");
	if (tpos == std::string::npos)
	{
		tpos = sResult.find("changeDeviceAndRedirect");
		if (tpos != std::string::npos)
		{
			m_ThermostatID = GetFirstDeviceID(sResult);
		}
		return "";
	}
	sResult = sResult.substr(tpos);
	tpos = sResult.find("value=\"");
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(tpos+7);
	tpos = sResult.find("\"");
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(0,tpos);
	return sResult;
}

bool CAtagOne::Login()
{
	if (!m_ThermostatID.empty())
	{
		Logout();
	}
	if (m_UserName.empty())
		return false;
	m_ThermostatID = "";

	std::string sResult;

	// We need a session (cookie) and a verification token, get them first.
	std::string requestVerificationToken = GetRequestVerificationToken(ATAGONE_URL_LOGIN);
	if (requestVerificationToken.empty())
	{
		if (!m_ThermostatID.empty())
		{
			m_bDoLogin = false;
			return true;
		}
		_log.Log(LOG_ERROR, "AtagOne: Error login!");
		return false;
	}

#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne1.txt");
#else
	std::stringstream sstr;
	sstr
		<< "__RequestVerificationToken=" << requestVerificationToken
		<< "&Email=" << m_UserName
		<< "&Password=" << m_Password
		<< "&RememberMe=false";
	std::string szPostdata = sstr.str();
	std::vector<std::string> ExtraHeaders;

	//# 1. Login
	std::string sURL;
	sURL = ATAGONE_URL_LOGIN;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "AtagOne: Error login!");
		return false;
	}

#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne1.txt");
#endif
#endif
	//# 2. Extract DeviceID
	// <tr onclick="javascript:changeDeviceAndRedirect('/Home/Index/{0}','6808-1401-3109_15-30-001-544');">
	m_ThermostatID = GetFirstDeviceID(sResult);
	if (m_ThermostatID.empty())
	{
		_log.Log(LOG_ERROR, "AtagOne: Error getting device_id!");
		return false;
	}
	m_bDoLogin = false;
	return true;
}

void CAtagOne::Logout()
{
	if (m_bDoLogin)
		return; //we are not logged in
	m_ThermostatID = "";
	m_bDoLogin = true;
}


#define AtagOne_POLL_INTERVAL 60

void CAtagOne::Do_Work()
{
	_log.Log(LOG_STATUS,"AtagOne: Worker started...");
	int sec_counter = AtagOne_POLL_INTERVAL-5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (sec_counter % AtagOne_POLL_INTERVAL == 0)
		{
			//SendOutsideTemperature();
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"AtagOne: Worker stopped...");
}

bool CAtagOne::GetOutsideTemperatureFromDomoticz(float &tvalue)
{
	if (m_OutsideTemperatureIdx == 0)
		return false;
	Json::Value tempjson;
	std::stringstream sstr;
	sstr << m_OutsideTemperatureIdx;
	m_webservers.GetJSonDevices(tempjson, "", "temp", "ID", sstr.str(), "", "", true, false, false, 0, "");

	size_t tsize = tempjson.size();
	if (tsize < 1)
		return false;

	Json::Value::const_iterator itt;
	Json::ArrayIndex rsize = tempjson["result"].size();
	if (rsize < 1)
		return false;

	bool bHaveTimeout = tempjson["result"][0]["HaveTimeout"].asBool();
	if (bHaveTimeout)
		return false;
	tvalue = tempjson["result"][0]["Temp"].asFloat();
	return true;
}

bool CAtagOne::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		//Light command

		int node_id = pCmd->LIGHTING2.id4;
		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		if (node_id == 1)
		{
			//Pause Switch
			SetPauseStatus(bIsOn);
			return true;
		}
	}
	return false;
}

static std::string GetHTMLPageValue(const std::string &hpage, const std::string &svalueLng1, const std::string &svalueLng2, const bool asFloat)
{
	std::vector<std::string > m_labels;
	if (!svalueLng1.empty())
		m_labels.push_back(svalueLng1);
	if (!svalueLng2.empty())
		m_labels.push_back(svalueLng2);
	std::vector<std::string >::const_iterator itt;
	// HTML structure of values in page.
	//     <label class="col-xs-6 control-label">Apparaat alias</label>
	//     <div class="col-xs-6">
	//         <p class="form-control-static">CV-ketel</p>
	//     </div> 
	for (itt = m_labels.begin(); itt != m_labels.end(); ++itt)
	{
		std::string sresult = hpage;
		std::string sstring = ">" + *itt + "</label>";
		size_t tpos = sresult.find(sstring);
		if (tpos==std::string::npos)
			continue;
		sresult = sresult.substr(tpos + sstring.size());
		tpos = sresult.find("<p");
		if (tpos == std::string::npos)
			continue;
		sresult = sresult.substr(tpos+2);
		tpos = sresult.find(">");
		if (tpos == std::string::npos)
			continue;
		sresult = sresult.substr(tpos + 1);
		tpos = sresult.find("<");
		if (tpos == std::string::npos)
			continue;
		sresult = sresult.substr(0,tpos);
		stdstring_trim(sresult);

		if (asFloat)
			stdreplace(sresult, ",", ".");
		return sresult;
	}
	return "";
}

void CAtagOne::GetMeterDetails()
{
	if (m_UserName.empty() || m_Password.empty() )
		return;

	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}

	std::string sResult;
#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne_getdiag.txt");
#else
	std::string sURL = std::string(ATAGONE_URL_DIAGNOSTICS) + "?deviceId=" + CURLEncode::URLEncode(m_ThermostatID);
	if (!HTTPClient::GET(sURL, sResult))
	{
		_log.Log(LOG_ERROR, "AtagOne: Error getting thermostat data!");
		m_bDoLogin = true;
		return;
	}

#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_getdiag.txt");
#endif
#endif
	//Extract all values from the HTML page, and put them in a json array
	Json::Value root;
	std::string sret;
	sret = GetHTMLPageValue(sResult, "Kamertemperatuur", "Room temperature", true);
	if (sret.empty())
	{
		_log.Log(LOG_ERROR, "AtagOne: Invalid/no data received...");
		return;
	}
	root["roomTemperature"] = static_cast<float>(atof(sret.c_str()));
	root["deviceAlias"] = GetHTMLPageValue(sResult, "Apparaat alias", "Device alias", false);
	root["latestReportTime"] = GetHTMLPageValue(sResult, "Laatste rapportagetijd", "Latest report time", false);
	root["connectedTo"] = GetHTMLPageValue(sResult, "Verbonden met", "Connected to", false);
	root["burningHours"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Branduren", "Burning hours", true).c_str()));
	root["boilerHeatingFor"] = GetHTMLPageValue(sResult, "Ketel in bedrijf voor", "Boiler heating for", false);
	sret= GetHTMLPageValue(sResult, "Brander status", "Flame status", false);
	root["flameStatus"] = ((sret == "Aan") || (sret == "On")) ? true : false;
	root["outsideTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Buitentemperatuur", "Outside temperature", true).c_str()));
	root["dhwSetpoint"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Setpoint warmwater", "DHW setpoint", true).c_str()));
	root["dhwWaterTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Warmwatertemperatuur", "DHW water temperature", true).c_str()));
	root["chSetpoint"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Setpoint cv", "CH setpoint", true).c_str()));
	root["chWaterTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "CV-aanvoertemperatuur", "CH water temperature", true).c_str()));
	root["chWaterPressure"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "CV-waterdruk", "CH water pressure", true).c_str()));
	root["chReturnTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "CV retourtemperatuur", "CH return temperature", true).c_str()));

#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne_gettargetsetpoint.txt");
#else
	// We have to do an extra call to get the target temperature.
	sURL = ATAGONE_URL_UPDATE_DEVICE_CONTROL;
	stdreplace(sURL, "{0}", CURLEncode::URLEncode(m_ThermostatID));
	if (!HTTPClient::GET(sURL, sResult))
	{
		_log.Log(LOG_ERROR, "AtagOne: Error getting target setpoint data!");
		m_bDoLogin = true;
		return;
	}
#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_gettargetsetpoint.txt");
#endif
#endif
	Json::Value root2;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root2);
	if ((!ret) || (!root2.isObject()))
	{
		_log.Log(LOG_ERROR, "AtagOne: Invalid/no data received...");
		return;
	}
	if (root2["targetTemp"].empty())
	{
		_log.Log(LOG_ERROR, "AtagOne: Invalid/no data received...");
		return;
	}
	root["targetTemperature"] = static_cast<float>(atof(root2["targetTemp"].asString().c_str()));
	root["currentMode"] = root2["currentMode"].asString();
	root["vacationPlanned"] = root2["vacationPlanned"].asBool();

	//Handle the Values
	float temperature;
	temperature = (float)root["targetTemperature"].asFloat();
	SendSetPointSensor(0, 0, 1, temperature, "Room Setpoint");

	temperature = (float)root["roomTemperature"].asFloat();
	SendTempSensor(2, 255, temperature, "room Temperature");

	if (!root["outsideTemperature"].empty())
	{
		temperature = (float)root["outsideTemperature"].asFloat();
		SendTempSensor(3, 255, temperature, "outside Temperature");
	}

	//DHW
	if (!root["dhwSetpoint"].empty())
	{
		temperature = (float)root["dhwSetpoint"].asFloat();
		SendSetPointSensor(0, 0, 2, temperature, "DHW Setpoint");
	}
	if (!root["dhwWaterTemperature"].empty())
	{
		temperature = (float)root["dhwWaterTemperature"].asFloat();
		SendTempSensor(4, 255, temperature, "DHW Temperature");
	}
	//CH
	if (!root["chSetpoint"].empty())
	{
		temperature = (float)root["chSetpoint"].asFloat();
		SendSetPointSensor(0, 0, 3, temperature, "CH Setpoint");
	}
	if (!root["chWaterTemperature"].empty())
	{
		temperature = (float)root["chWaterTemperature"].asFloat();
		SendTempSensor(5, 255, temperature, "CH Temperature");
	}
	if (!root["chWaterPressure"].empty())
	{
		float pressure = (float)root["chWaterPressure"].asFloat();
		SendPressureSensor(1, 1, 255, pressure, "Pressure");
	}
	if (!root["chReturnTemperature"].empty())
	{
		temperature = (float)root["chReturnTemperature"].asFloat();
		SendTempSensor(6, 255, temperature, "CH Return Temperature");
	}

	if (!root["currentMode"].empty())
	{
		std::string actSource = root["currentMode"].asString();
		bool bIsScheduleMode = (actSource == "schedule_active");
		SendSwitch(1, 1, 255, bIsScheduleMode, 0, "Thermostat Schedule Mode");
	}
	if (!root["flameStatus"].empty())
	{
		SendSwitch(2, 1, 255, root["flameStatus"].asBool(), 0, "Flame Status");
	}
	
}

void CAtagOne::SetSetpoint(const int idx, const float temp)
{
	if (idx != 1)
	{
		_log.Log(LOG_ERROR, "AtagOne: Currently only Room Temperature Setpoint allowed!");
		return;
	}

	int rtemp = int(temp*2.0f);
	float dtemp = float(rtemp) / 2.0f;
	if (
		(dtemp<ATAGONE_TEMPERATURE_MIN) ||
		(dtemp>ATAGONE_TEMPERATURE_MAX)
		)
	{
		_log.Log(LOG_ERROR, "AtagOne: Temperature should be between %d and %d", ATAGONE_TEMPERATURE_MIN, ATAGONE_TEMPERATURE_MAX);
		return;
	}
	char szTemp[20];
	sprintf(szTemp, "%.1f", dtemp);
	std::string sTemp = szTemp;

	// Get updated request verification token first.
	std::string  requestVerificationToken = GetRequestVerificationToken(ATAGONE_URL_DEVICE_HOME);

	// https://portal.atag-one.com/Home/DeviceSetSetpoint/6808-1401-3109_15-30-001-544?temperature=18.5
	std::string sURL = std::string(ATAGONE_URL_DEVICE_SET_SETPOINT) + "/" + m_ThermostatID + "?temperature=" + sTemp;

	std::stringstream sstr;
	if (!requestVerificationToken.empty())
	{
		sstr << "__RequestVerificationToken=" << requestVerificationToken;
	}
	std::string szPostdata = sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "AtagOne: Error setting Setpoint!");
		return;
	}
#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_setsetpoint.txt");
#endif
	SendSetPointSensor(0,0,idx, dtemp, "");
}

void CAtagOne::SetPauseStatus(const bool bIsPause)
{
}

void CAtagOne::SendOutsideTemperature()
{
	float temp;
	if (!GetOutsideTemperatureFromDomoticz(temp))
		return;
	SetOutsideTemp(temp);
}

void CAtagOne::SetOutsideTemp(const float temp)
{
}

