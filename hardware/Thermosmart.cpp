#include "stdafx.h"
#include "Thermosmart.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

#define round(a) ( int ) ( a + .5 )

const std::string THERMOSMART_LOGIN_PATH = "https://api.thermosmart.com/login";
const std::string THERMOSMART_AUTHORISE_PATH = "https://api.thermosmart.com/oauth2/authorize?response_type=code&client_id=client123&redirect_uri=http://clientapp.com/done";
const std::string THERMOSMART_DECISION_PATH = "https://api.thermosmart.com/oauth2/authorize/decision";
const std::string THERMOSMART_TOKEN_PATH = "https://api.thermosmart.com/oauth2/token";
const std::string THERMOSMART_ACCESS_PATH = "https://api.thermosmart.com/thermostat";

#ifdef _DEBUG
	#define DEBUG_ThermosmartThermostat
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

CThermosmart::CThermosmart(const int ID, const std::string &Username, const std::string &Password)
{
	m_HwdID=ID;
	m_UserName=Username;
	m_Password=Password;
	Init();
}

CThermosmart::~CThermosmart(void)
{
}

void CThermosmart::Init()
{
	m_AccessToken = "";
	m_ThermostatID = "";
	m_stoprequested = false;
	m_bDoLogin = true;
}

bool CThermosmart::StartHardware()
{
	Init();
	m_LastMinute = -1;
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CThermosmart::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CThermosmart::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
	if (!m_bDoLogin)
		Logout();
    return true;
}

#define THERMOSMART_POLL_INTERVAL 30

void CThermosmart::Do_Work()
{
	_log.Log(LOG_STATUS,"Thermosmart: Worker started...");
	int sec_counter = THERMOSMART_POLL_INTERVAL-5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (sec_counter % THERMOSMART_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"Thermosmart: Worker stopped...");
}

void CThermosmart::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	bool bDeviceExits=true;
	std::stringstream szQuery;

	char szID[10];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, 0, Idx);

	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
		result=m_sql.query(szQuery.str());
	}
}


bool CThermosmart::Login()
{
	if (!m_AccessToken.empty())
	{
		Logout();
	}
	m_AccessToken = "";
	m_ThermostatID = "";

	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::string sURL = THERMOSMART_LOGIN_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"Thermosmart: Error login!");
		return false;
	}
#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart1.txt");
#endif

	sURL = THERMOSMART_AUTHORISE_PATH;
	sURL = stdreplace(sURL, "client123", "3de470ce1db15f92");
	ExtraHeaders.clear();
	if (!HTTPClient::GET(sURL, sResult))
	{
		_log.Log(LOG_ERROR, "Thermosmart: Error login!");
		return false;
	}
#ifdef DEBUG_ThermosmartThermostat
	SaveString2Disk(sResult, "E:\\thermosmart2.txt");
#endif

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
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;

	tRBUF *pCmd = (tRBUF *)pdata;
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false; //later add RGB support, if someone can provide access

	int node_id = pCmd->LIGHTING2.id4;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);

	return false;
}

void CThermosmart::GetMeterDetails()
{
	if (m_UserName.size()==0)
		return;
	if (m_Password.size()==0)
		return;
	std::string sResult;
	if (m_bDoLogin)
	{
		if (!Login())
		return;
	}
}

void CThermosmart::SetSetpoint(const int idx, const float temp)
{
}
