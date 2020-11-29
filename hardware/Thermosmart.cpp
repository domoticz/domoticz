#include "stdafx.h"
#include "Thermosmart.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/WebServerHelper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../main/json_helper.h"

#define round(a) ( int ) ( a + .5 )

const std::string THERMOSMART_LOGIN_PATH = "https://api.thermosmart.com/login";
const std::string THERMOSMART_AUTHORISE_PATH = "https://api.thermosmart.com/oauth2/authorize?response_type=code&client_id=client123&redirect_uri=http://clientapp.com/done";
const std::string THERMOSMART_DECISION_PATH = "https://api.thermosmart.com/oauth2/authorize/decision";
const std::string THERMOSMART_TOKEN_PATH = "https://username:password@api.thermosmart.com/oauth2/token";
const std::string THERMOSMART_ACCESS_PATH = "https://api.thermosmart.com/thermostat/[TID]?access_token=[access_token]";
const std::string THERMOSMART_SETPOINT_PATH = "https://api.thermosmart.com/thermostat/[TID]?access_token=[access_token]";
const std::string THERMOSMART_SET_PAUZE = "https://api.thermosmart.com/thermostat/[TID]/pause?access_token=[access_token]";

extern http::server::CWebServerHelper m_webservers;

#ifdef _DEBUG
	//#define DEBUG_ThermosmartThermostat_read
#endif

#ifdef DEBUG_ThermosmartThermostat
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
#ifdef DEBUG_ThermosmartThermostat_read
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

CThermosmart::CThermosmart(const int ID, const std::string &Username, const std::string &Password, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6)
{
	if ((Password == "secret")|| (Password.empty()))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Please update your username/password!...");
	}
	else
	{
		m_UserName = Username;
		m_Password = Password;
		stdstring_trim(m_UserName);
		stdstring_trim(m_Password);
	}
	m_HwdID=ID;
	m_OutsideTemperatureIdx = 0; //use build in
	m_LastMinute = -1;
	SetModes(Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
	Init();
}

void CThermosmart::SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6)
{
	m_OutsideTemperatureIdx = Mode1;
}

void CThermosmart::Init()
{
	m_AccessToken = "";
	m_ThermostatID = "";
	m_bDoLogin = true;
}

bool CThermosmart::StartHardware()
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

bool CThermosmart::StopHardware()
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

#define THERMOSMART_POLL_INTERVAL 30

void CThermosmart::Do_Work()
{
	_log.Log(LOG_STATUS,"Thermosmart: Worker started...");
	int sec_counter = THERMOSMART_POLL_INTERVAL-5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % THERMOSMART_POLL_INTERVAL == 0)
		{
			SendOutsideTemperature();
			GetMeterDetails();
		}
	}
	Logout();

	_log.Log(LOG_STATUS,"Thermosmart: Worker stopped...");
}

bool CThermosmart::GetOutsideTemperatureFromDomoticz(float &tvalue)
{
	if (m_OutsideTemperatureIdx == 0)
		return false;
	Json::Value tempjson;
	m_webservers.GetJSonDevices(tempjson, "", "temp", "ID", std::to_string(m_OutsideTemperatureIdx), "", "", true, false, false, 0, "");

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

void CThermosmart::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;
	thermos.temp=Temp;
	sDecodeRXMessage(this, (const unsigned char *)&thermos, "Setpoint", 255, nullptr);
}

bool CThermosmart::Login()
{
	if (!m_AccessToken.empty())
	{
		Logout();
	}
	if (m_UserName.empty())
		return false;
	m_AccessToken = "";
	m_ThermostatID = "";

	std::string sURL;
	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	//# 1. Login

	sURL = THERMOSMART_LOGIN_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR,"Thermosmart: Error login!");
			return false;
		}

#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart1.txt");
#endif

	//# 2. Get Authorize Dialog
	sURL = THERMOSMART_AUTHORISE_PATH;
	stdreplace(sURL, "client123", "api-rob-b130d8f5123bf24b");
	ExtraHeaders.clear();
	if (!HTTPClient::GET(sURL, sResult))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!");
		return false;
	}

#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart2.txt");
#endif

	size_t tpos = sResult.find("value=");
	if (tpos == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!, check username/password");
		return false;
	}
	sResult = sResult.substr(tpos + 7);
	tpos = sResult.find("\">");
	if (tpos == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!, check username/password");
		return false;
	}
	std::string TID = sResult.substr(0, tpos);

	//# 3. Authorize  (read out transaction_id from the HTML form received in the previous step). transaction_id prevents from XSRF attacks.
	szPostdata = "transaction_id=" + TID;
	ExtraHeaders.clear();
	sURL = THERMOSMART_DECISION_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult, false))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!, check username/password");
		return false;
	}

#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart3.txt");
#endif

	tpos = sResult.find("code=");
	if (tpos == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!, check username/password");
		return false;
	}
	std::string CODE = sResult.substr(tpos + 5);

	//# 4. Exchange authorization code for Access token (read out the code from the previous response)
	szPostdata = "grant_type=authorization_code&code=" + CODE + "&redirect_uri=http://clientapp.com/done";
	sURL = THERMOSMART_TOKEN_PATH;

	stdreplace(sURL, "username", "api-rob-b130d8f5123bf24b");
	stdreplace(sURL, "password", "c1d91661eef0bc4fa2ac67fd");

	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult, false))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!, check username/password");
		return false;
	}

#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart4.txt");
#endif

	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Invalid/no data received...");
		return false;
	}

	if (root["access_token"].empty()||root["thermostat"].empty())
	{
		_log.Log(LOG_ERROR, "Thermosmart: No access granted, check username/password...");
		return false;
	}

	m_AccessToken = root["access_token"].asString();
	m_ThermostatID = root["thermostat"].asString();

	_log.Log(LOG_STATUS, "Thermosmart: Login successfull!...");

	m_bDoLogin = false;
	return true;
}

void CThermosmart::Logout()
{
	if (m_bDoLogin)
		return; //we are not logged in
	m_AccessToken = "";
	m_ThermostatID = "";
	m_bDoLogin = true;
}


bool CThermosmart::WriteToHardware(const char *pdata, const unsigned char length)
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

void CThermosmart::GetMeterDetails()
{
	if (m_UserName.empty() || m_Password.empty() )
		return;
	std::string sResult;
#ifdef DEBUG_ThermosmartThermostat_read
	sResult = ReadFile("E:\\thermosmart_getdata.txt");
#else
	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}
	std::string sURL = THERMOSMART_ACCESS_PATH;
	stdreplace(sURL, "[TID]", m_ThermostatID);
	stdreplace(sURL, "[access_token]", m_AccessToken);
	if (!HTTPClient::GET(sURL, sResult))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error getting thermostat data!");
		m_bDoLogin = true;
		return;
	}

#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart_getdata.txt");
#endif
#endif
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Invalid/no data received...");
		m_bDoLogin = true;
		return;
	}

	if (root["target_temperature"].empty() || root["room_temperature"].empty())
	{
		_log.Log(LOG_ERROR, "Thermosmart: Invalid/no data received...");
		m_bDoLogin = true;
		return;
	}

	float temperature;
	temperature = (float)root["target_temperature"].asFloat();
	SendSetPointSensor(1, temperature, "target temperature");

	temperature = (float)root["room_temperature"].asFloat();
	SendTempSensor(2, 255, temperature, "room temperature");

	if (!root["outside_temperature"].empty())
	{
		temperature = (float)root["outside_temperature"].asFloat();
		SendTempSensor(3, 255, temperature, "outside temperature");
	}
	if (!root["source"].empty())
	{
		std::string actSource = root["source"].asString();
		bool bPauzeOn = (actSource == "pause");
		SendSwitch(1, 1, 255, bPauzeOn, 0, "Thermostat Pause", m_Name);
	}
}

void CThermosmart::SetSetpoint(const int idx, const float temp)
{
	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}

	char szTemp[20];
	sprintf(szTemp, "%.1f", temp);
	std::string sTemp = szTemp;

	std::string szPostdata = "target_temperature=" + sTemp;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::string sURL = THERMOSMART_SETPOINT_PATH;
	stdreplace(sURL, "[TID]", m_ThermostatID);
	stdreplace(sURL, "[access_token]", m_AccessToken);
	if (!HTTPClient::PUT(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error setting thermostat data!");
		m_bDoLogin = true;
		return;
	}
	SendSetPointSensor(1, temp, "target temperature");
}

void CThermosmart::SetPauseStatus(const bool bIsPause)
{
	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}

	std::string szPostdata = "{\"pause\":";
	szPostdata += (bIsPause) ? "true" : "false";
	szPostdata += "}";

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");
	std::string sResult;

	std::string sURL = THERMOSMART_SET_PAUZE;
	stdreplace(sURL, "[TID]", m_ThermostatID);
	stdreplace(sURL, "[access_token]", m_AccessToken);

	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error setting Pause status!");
		m_bDoLogin = true;
		return;
	}
}

void CThermosmart::SendOutsideTemperature()
{
	float temp;
	if (!GetOutsideTemperatureFromDomoticz(temp))
		return;
	SetOutsideTemp(temp);
}

void CThermosmart::SetOutsideTemp(const float temp)
{
	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}

	char szTemp[20];
	sprintf(szTemp, "%.1f", temp);
	std::string sTemp = szTemp;

	std::string szPostdata = "outside_temperature=" + sTemp;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::string sURL = THERMOSMART_SETPOINT_PATH;
	stdreplace(sURL, "[TID]", m_ThermostatID);
	stdreplace(sURL, "[access_token]", m_AccessToken);
	if (!HTTPClient::PUT(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error setting thermostat data!");
		m_bDoLogin = true;
		return;
	}
}

