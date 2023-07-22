#include "stdafx.h"
#include "AtagOne.h"
#include "../main/Helper.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/WebServerHelper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../main/json_helper.h"

extern http::server::CWebServerHelper m_webservers;

//Inspidred by https://github.com/kozmoz/atag-one-api

#define ATAGONE_URL_LOGIN "https://portal.atag-one.com/Account/Login"
#define ATAGONE_URL_DEVICE_HOME "https://portal.atag-one.com/Home/Index/{0}"
#define ATAGONE_URL_DIAGNOSTICS "https://portal.atag-one.com/Device/Diagnostics/{0}"
#define ATAGONE_URL_LATEST_REPORT "https://portal.atag-one.com/Device/LatestReport?deviceId={0}"
#define ATAGONE_URL_UPDATE_DEVICE_CONTROL "https://portal.atag-one.com/Home/UpdateDeviceControl/?deviceId={0}"
#define ATAGONE_URL_DEVICE_SET_SETPOINT "https://portal.atag-one.com/Home/DeviceSetSetpoint/{0}?temperature={1}"
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
	SetModes(Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
	Init();
}

void CAtagOne::SetModes(const int Mode1, const int /*Mode2*/, const int /*Mode3*/, const int /*Mode4*/, const int /*Mode5*/, const int /*Mode6*/)
{
	m_OutsideTemperatureIdx = Mode1;
}

void CAtagOne::Init()
{
	m_Thermostats.clear();
	m_bDoLogin = true;
}

bool CAtagOne::StartHardware()
{
	RequestStart();

	Init();

	m_LastMinute = -1;
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CAtagOne::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
    return true;
}

#define AtagOne_POLL_INTERVAL 60

void CAtagOne::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");
	int sec_counter = AtagOne_POLL_INTERVAL - 3;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % AtagOne_POLL_INTERVAL == 0)
		{
			//SendOutsideTemperature();
			GetMeterDetails();
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

void CAtagOne::GetMeterDetails()
{
	if (m_UserName.empty() || m_Password.empty())
		return;

	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}
	if (m_Thermostats.empty())
		return;

	//Can only process 1 thermostat at the momement because of poorly choosen sensor Node/Child id's
	int iActThermostat = 0;
	GetDeviceDetails(m_Thermostats[iActThermostat]);
}

bool CAtagOne::GetDeviceDetails(const std::string& ThermostatID)
{
	std::string sResult;
#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne_getdiag.txt");
#else

	std::string sURL;
/*
	sURL = ATAGONE_URL_DIAGNOSTICS;
	stdreplace(sURL, "{0}", CURLEncode::URLEncode(ThermostatID));
	if (!HTTPClient::GET(sURL, sResult))
	{
		Log(LOG_ERROR, "Error getting thermostat data!");
		m_bDoLogin = true;
		return false;
	}
#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_diagnostics.txt");
#endif
*/
	sURL = ATAGONE_URL_LATEST_REPORT;
	stdreplace(sURL, "{0}", CURLEncode::URLEncode(ThermostatID));

	if (!HTTPClient::GET(sURL, sResult))
	{
		Log(LOG_ERROR, "Error getting thermostat data!");
		m_bDoLogin = true;
		return false;
	}

#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_latest_report.txt");
#endif
#endif
	//Extract all values from the HTML page, and put them in a json array
	Json::Value root;
	std::string sret;
	sret = GetHTMLPageValue(sResult, "Kamertemperatuur|Room temperature|Raumtemperatur", true);
	if (sret.empty())
	{
		Log(LOG_ERROR, "Invalid/no data received (1)...");
		return false;
	}
	root["roomTemperature"] = static_cast<float>(atof(sret.c_str()));
	//root["deviceAlias"] = GetHTMLPageValue(sResult, "Apparaat alias|Device alias", false);
	//root["latestReportTime"] = GetHTMLPageValue(sResult, "Laatste rapportagetijd|Latest report time", false);
	//root["connectedTo"] = GetHTMLPageValue(sResult, "Verbonden met|Connected to", false);
	root["burningHours"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Branduren|Burning hours", true).c_str()));
	//root["boilerHeatingFor"] = GetHTMLPageValue(sResult, "Ketel in bedrijf voor|Boiler heating for", false);
	sret = GetHTMLPageValue(sResult, "Brander status|Flame status|Brennerstatus", false);
	root["flameStatus"] = ((sret == "Aan") || (sret == "On") || (sret == "An")) ? true : false;
	root["outsideTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Buitentemperatuur|Outside temperature|Au&#223;entemperatur", true).c_str()));
	root["dhwSetpoint"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Setpoint warmwater|DHW setpoint", true).c_str()));
	root["dhwWaterTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Warmwatertemperatuur|DHW water temperature|Warmwassertemperatur", true).c_str()));
	root["chSetpoint"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "Setpoint cv|CH setpoint", true).c_str()));
	root["chWaterTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "CV-aanvoertemperatuur|CH water temperature", true).c_str()));
	root["chWaterPressure"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "CV-waterdruk|CH water pressure|Anlagendruck", true).c_str()));
	root["chReturnTemperature"] = static_cast<float>(atof(GetHTMLPageValue(sResult, "CV retourtemperatuur|CH return temperature|HZ R&#252;cklauftemperatur", true).c_str()));

#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne_gettargetsetpoint.txt");
#else
	// We have to do an extra call to get the target temperature.
	sURL = ATAGONE_URL_UPDATE_DEVICE_CONTROL;
	stdreplace(sURL, "{0}", CURLEncode::URLEncode(ThermostatID));
	if (!HTTPClient::GET(sURL, sResult))
	{
		Log(LOG_ERROR, "Error getting target setpoint data!");
		m_bDoLogin = true;
		return false;
	}
#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_gettargetsetpoint.txt");
#endif
#endif
	Json::Value root2;
	bool ret = ParseJSon(sResult, root2);
	if ((!ret) || (!root2.isObject()))
	{
		Log(LOG_ERROR, "Invalid/no data received (2)...");
		return false;
	}
	if (root2["targetTemp"].empty())
	{
		Log(LOG_ERROR, "Invalid/no data received (3)...");
		return false;
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
		SendSwitch(1, 1, 255, bIsScheduleMode, 0, "Thermostat Schedule Mode", m_Name);
	}
	if (!root["flameStatus"].empty())
	{
		SendSwitch(2, 1, 255, root["flameStatus"].asBool(), 0, "Flame Status", m_Name);
	}
	return true;
}

bool CAtagOne::GetThermostats(const std::string &shtml)
{
	m_Thermostats.clear();

	// Evsdd - Updated string due to webpage change
	// Original format: <tr onclick="javascript:changeDeviceAndRedirect('/Home/Index/{0}','6808-1401-3109_15-30-001-544');">
	// New format: "/Home/Index/6808-1401-3109_15-30-001-544"
	std::string sResult = shtml;
	while (1 == 1)
	{
		size_t tpos = sResult.find("/Home/Index");
		if (tpos == std::string::npos)
			return (m_Thermostats.size() != 0);
		sResult = sResult.substr(tpos);
		tpos = sResult.find("x/");
		if (tpos == std::string::npos)
			return (m_Thermostats.size() != 0);

		sResult = sResult.substr(tpos + 2);

		std::string tstring = sResult;
		tpos = tstring.find('"');
		if (tpos == std::string::npos)
			return (m_Thermostats.size() != 0);
		tstring = tstring.substr(0, tpos);

		std::vector<std::string> strarray;
		StringSplit(tstring, ",", strarray);
		if (strarray.size() == 2)
		{
			tstring = strarray.at(1);
		}
		stdreplace(tstring, "'", "");
		size_t pos = tstring.find(')');
		if (pos != std::string::npos)
			tstring = tstring.substr(0, pos);
		m_Thermostats.push_back(tstring);
	}
	return (m_Thermostats.size() != 0);
}

std::string CAtagOne::GetRequestVerificationToken(const std::string &url, const std::string& ThermostatID)
{
	std::string sResult;
#ifdef DEBUG_AtagOneThermostat_read
	sResult = ReadFile("E:\\AtagOne_requesttoken.txt");
#else
	std::string sURL = url;
	stdreplace(sURL,"{0}", ThermostatID);

	if (!HTTPClient::GET(sURL, sResult))
	{
		Log(LOG_ERROR, "Error requesting token!");
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
			GetThermostats(sResult);
		}
		return "";
	}
	sResult = sResult.substr(tpos);
	tpos = sResult.find("value=\"");
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(tpos+7);
	tpos = sResult.find('"');
	if (tpos == std::string::npos)
		return "";
	sResult = sResult.substr(0,tpos);
	return sResult;
}

bool CAtagOne::Login()
{
	Init();
	if (m_UserName.empty())
		return false;

	std::string sResult;

	// We need a session (cookie) and a verification token, get them first.
	std::string requestVerificationToken = GetRequestVerificationToken(ATAGONE_URL_LOGIN, "");
	if (requestVerificationToken.empty())
	{
		if (!m_Thermostats.empty())
		{
			m_bDoLogin = false;
			return true;
		}
		Log(LOG_ERROR, "Error login!");
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
		Log(LOG_ERROR, "Error login!");
		return false;
	}

#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne1_login.txt");
#endif
#endif
	//# 2. Extract DeviceID
	// <tr onclick="javascript:changeDeviceAndRedirect('/Home/Index/{0}','6808-1401-3109_15-30-001-544');">
	GetThermostats(sResult);
	if (m_Thermostats.empty())
	{
		Log(LOG_ERROR, "Error getting device_id!");
		return false;
	}

	m_bDoLogin = false;
	return true;
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

	Json::ArrayIndex rsize = tempjson["result"].size();
	if (rsize < 1)
		return false;

	bool bHaveTimeout = tempjson["result"][0]["HaveTimeout"].asBool();
	if (bHaveTimeout)
		return false;
	tvalue = tempjson["result"][0]["Temp"].asFloat();
	return true;
}

bool CAtagOne::WriteToHardware(const char *pdata, const unsigned char /*length*/)
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

std::string CAtagOne::GetHTMLPageValue(const std::string &hpage, const std::string &svalueLng, const bool asFloat)
{
	std::vector<std::string > labels_;
	StringSplit(svalueLng, "|", labels_);

	// HTML structure of values in page.
	//     <label class="col-xs-6 control-label">Apparaat alias</label>
	//     <div class="col-xs-6">
	//         <p class="form-control-static">CV-ketel</p>
	//     </div>
	for (const auto &label : labels_)
	{
		std::string sresult = hpage;
		std::string sstring = ">" + label + "</label>";
		size_t tpos = sresult.find(sstring);
		if (tpos == std::string::npos)
		{
			//Sometimes we see a space behind the label!??
			sstring = ">" + label + " </label>";
			tpos = sresult.find(sstring);
			if (tpos == std::string::npos)
			{
				continue;
			}
		}
		sresult = sresult.substr(tpos + sstring.size());
		tpos = sresult.find("<p");
		if (tpos == std::string::npos)
			continue;
		sresult = sresult.substr(tpos+2);
		tpos = sresult.find('>');
		if (tpos == std::string::npos)
			continue;
		sresult = sresult.substr(tpos + 1);
		tpos = sresult.find('<');
		if (tpos == std::string::npos)
			continue;
		sresult = sresult.substr(0,tpos);
		stdstring_trim(sresult);

		tpos = sresult.find("&#");
		if (tpos != std::string::npos)
		{
			sresult = sresult.substr(0, tpos);
		}

		stdreplace(sresult, "\r\n", "");

		if (asFloat)
			stdreplace(sresult, ",", ".");
		return sresult;
	}
	return "";
}

void CAtagOne::SetSetpoint(const int idx, const float temp)
{
	if (idx != 1)
	{
		Log(LOG_ERROR, "Currently only Room Temperature Setpoint allowed!");
		return;
	}

	int iActThermostat = 0; //(idx-1)
	std::string szThermostat = m_Thermostats[iActThermostat];

	int rtemp = int(temp * 2.0F);
	float dtemp = float(rtemp) / 2.0F;
	if (
		(dtemp<ATAGONE_TEMPERATURE_MIN) ||
		(dtemp>ATAGONE_TEMPERATURE_MAX)
		)
	{
		Log(LOG_ERROR, "Temperature should be between %d and %d", ATAGONE_TEMPERATURE_MIN, ATAGONE_TEMPERATURE_MAX);
		return;
	}
	char szTemp[20];
	sprintf(szTemp, "%.1f", dtemp);
	std::string sTemp = szTemp;

	// Get updated request verification token first.
	std::string  requestVerificationToken = GetRequestVerificationToken(ATAGONE_URL_DEVICE_HOME, szThermostat);

	// https://portal.atag-one.com/Home/DeviceSetSetpoint/6808-1401-3109_15-30-001-544?temperature=18.5
	std::string sURL;
	sURL = ATAGONE_URL_DEVICE_SET_SETPOINT;
	stdreplace(sURL, "{0}", CURLEncode::URLEncode(szThermostat));
	stdreplace(sURL, "{1}", sTemp);

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
		Log(LOG_ERROR, "Error setting Setpoint!");
		return;
	}
#ifdef DEBUG_AtagOneThermostat
	SaveString2Disk(sResult, "E:\\AtagOne_setsetpoint.txt");
#endif
	SendSetPointSensor(0,0, (const uint8_t)idx, dtemp, "");
}

void CAtagOne::SetPauseStatus(const bool /*bIsPause*/)
{
}

void CAtagOne::SendOutsideTemperature()
{
	float temp;
	if (!GetOutsideTemperatureFromDomoticz(temp))
		return;
	SetOutsideTemp(temp);
}

void CAtagOne::SetOutsideTemp(const float /*temp*/)
{
}

