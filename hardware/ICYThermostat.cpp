#include "stdafx.h"
#include "ICYThermostat.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_ICYThermostat

#define ICY_LOGIN_URL "https://portal.icy.nl/login"
#define ICY_DATA_URL "https://portal.icy.nl/data"

#define ENI_LOGIN_URL "https://eniportal.icy.nl/api/login" //https://eniportal.icy.nl/#/user/login"
#define ENI_DATA_URL "https://eniportal.icy.nl/api/data" //https://eniportal.icy.nl/#/user/data" // /api/data

#define SEC_LOGIN_URL "https://secportal.icy.nl/api/login" //https://secportal.icy.nl/#/user/login"
#define SEC_DATA_URL "https://secportal.icy.nl/api/data" //https://secportal.icy.nl/#/user/data" // /api/data


CICYThermostat::CICYThermostat(const int ID, const std::string &Username, const std::string &Password) :
m_UserName(Username),
m_Password(Password)
{
	m_HwdID=ID;
	m_companymode = CMODE_UNKNOWN;
	Init();
}

CICYThermostat::~CICYThermostat(void)
{
}

void CICYThermostat::Init()
{
	m_SerialNumber="";
	m_Token="";
}

bool CICYThermostat::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CICYThermostat::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CICYThermostat::StopHardware()
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

#define ICY_POLL_INTERVAL 60

void CICYThermostat::Do_Work()
{
	int sec_counter = ICY_POLL_INTERVAL-5;
	_log.Log(LOG_STATUS,"ICYThermostat: Worker started...");
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % ICY_POLL_INTERVAL ==0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"ICYThermostat: Worker stopped...");
}

bool CICYThermostat::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void CICYThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), 255);
}

bool CICYThermostat::GetSerialAndToken()
{
	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::string sURL = "";

	if ((m_companymode == CMODE_UNKNOWN) || (m_companymode == CMODE_PORTAL))
		sURL = ICY_LOGIN_URL;
	else if (m_companymode == CMODE_ENI)
		sURL = ENI_LOGIN_URL;
	else
		sURL = SEC_LOGIN_URL;

	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error login!");
		return false;
	}
	if (sResult.find("BadLogin") != std::string::npos)
	{
		if (m_companymode == CMODE_UNKNOWN)
		{
			//Try ENI mode
			sURL = ENI_LOGIN_URL;
			sResult = "";
			if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
			{
				_log.Log(LOG_ERROR, "ICYThermostat: Error login!");
				return false;
			}
			if (sResult.find("BadLogin") != std::string::npos)
			{
				if (m_companymode == CMODE_UNKNOWN)
				{
					//Try SEC mode
					sURL = SEC_LOGIN_URL;
					sResult = "";
					if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
					{
						_log.Log(LOG_ERROR, "ICYThermostat: Error login!");
						return false;
					}
					if (sResult.find("BadLogin") != std::string::npos)
					{
						_log.Log(LOG_ERROR, "ICYThermostat: Error login! (Check username/password)");
						return false;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "ICYThermostat: Error login! (Check username/password)");
					return false;
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "ICYThermostat: Error login! (Check username/password)");
			return false;
		}
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "ICYThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	if (root["serialthermostat1"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ICYThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_SerialNumber = root["serialthermostat1"].asString();
	if (root["token"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ICYThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_Token = root["token"].asString();

	if (m_companymode == CMODE_UNKNOWN)
	{
		if (sURL == ICY_LOGIN_URL)
			m_companymode = CMODE_PORTAL;
		else if (sURL == ENI_LOGIN_URL)
			m_companymode = CMODE_ENI;
		else
			m_companymode = CMODE_SEC;
	}


	return true;
}

void CICYThermostat::GetMeterDetails()
{
	if (m_UserName.size()==0)
		return;
	if (m_Password.size()==0)
		return;
	if (!GetSerialAndToken())
		return;

	std::string sResult;

	//Get Data
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Session-token:"+m_Token);

	std::string sURL = "";

	if (m_companymode == CMODE_PORTAL)
		sURL = ICY_DATA_URL;
	else if (m_companymode == CMODE_ENI)
		sURL = ENI_DATA_URL;
	else
		sURL = SEC_DATA_URL;

	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error getting data!");
		return;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "ICYThermostat: Invalid data received!");
		return;
	}
	if (root["temperature1"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ICYThermostat: Invalid data received!");
		return;
	}
	SendSetPointSensor(1, root["temperature1"].asFloat(), "Room Setpoint");
	if (root["temperature2"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ICYThermostat: Invalid data received!");
		return;
	}
	SendTempSensor(1, 255, root["temperature2"].asFloat(), "Room Temperature");
}

void CICYThermostat::SetSetpoint(const int idx, const float temp)
{
	if (idx==1)
	{
		//Room Set Point
		if (!GetSerialAndToken())
			return;

		char szTemp[20];
		sprintf(szTemp,"%.1f",temp);
		std::stringstream sstr;
		sstr << "uid=" << m_SerialNumber << "&temperature1=" << szTemp;
		std::string szPostdata=sstr.str();

		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back("Session-token:"+m_Token);
		std::string sResult;

		std::string sURL = "";
		if (m_companymode == CMODE_PORTAL)
			sURL = ICY_DATA_URL;
		else if (m_companymode == CMODE_ENI)
			sURL = ENI_DATA_URL;
		else
			sURL = SEC_DATA_URL;

		if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult)) {
			_log.Log(LOG_ERROR,"ICYThermostat: Error setting SetPoint temperature!");
		}
		else {
			_log.Debug(DEBUG_HARDWARE, "ICYThermostat: Setting Room SetPoint to: %.1f", temp);
		}
	}
}
