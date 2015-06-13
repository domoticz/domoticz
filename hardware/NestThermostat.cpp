#include "stdafx.h"
#include "NestThermostat.h"
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

const std::string NEST_LOGIN_PATH = "https://home.nest.com/user/login";
const std::string NEST_GET_STATUS = "/v2/mobile/user.";
const std::string NEST_SET_SHARED = "/v2/put/shared.";
const std::string NEST_SET_STRUCTURE = "/v2/put/structure.";

#ifdef _DEBUG
	//#define DEBUG_NextThermostat
#endif

#ifdef DEBUG_NextThermostat
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

CNestThermostat::CNestThermostat(const int ID, const std::string &Username, const std::string &Password)
{
	m_HwdID=ID;
	m_UserName=Username;
	m_Password=Password;
	m_AccessToken = "";
	m_UserID = "";
	Init();
}

CNestThermostat::~CNestThermostat(void)
{
}

void CNestThermostat::Init()
{
	m_AccessToken = "";
	m_UserID = "";
	m_stoprequested = false;
	m_bDoLogin = true;
}

bool CNestThermostat::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CNestThermostat::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CNestThermostat::StopHardware()
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

#define NEST_POLL_INTERVAL 30

void CNestThermostat::Do_Work()
{
	_log.Log(LOG_STATUS,"NestThermostat: Worker started...");
	int sec_counter = NEST_POLL_INTERVAL-5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % NEST_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}

	}
	_log.Log(LOG_STATUS,"NestThermostat: Worker stopped...");
}

void CNestThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
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


bool CNestThermostat::Login()
{
	if (!m_AccessToken.empty())
	{
		Logout();
	}
	m_AccessToken = "";
	m_UserID = "";

	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	std::string sResult;

	std::string sURL = NEST_LOGIN_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"NestThermostat: Error login!");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "NestThermostat: Invalid data received, or invalid username/password!");
		return false;
	}

	if (root["urls"].empty())
	{
		_log.Log(LOG_ERROR, "NestThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	if (root["urls"]["transport_url"].empty())
	{
		_log.Log(LOG_ERROR, "NestThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_TransportURL = root["urls"]["transport_url"].asString();

	if (root["access_token"].empty())
	{
		_log.Log(LOG_ERROR, "NestThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_AccessToken = root["access_token"].asString();
	
	if (root["userid"].empty())
	{
		_log.Log(LOG_ERROR, "NestThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_UserID = root["userid"].asString();

	m_bDoLogin = false;
	return true;
}

void CNestThermostat::Logout()
{
	if (m_bDoLogin)
		return; //we are not logged in
	m_AccessToken = "";
	m_UserID = "";
	m_bDoLogin = true;
}


bool CNestThermostat::WriteToHardware(const char *pdata, const unsigned char length)
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

	if (node_id == 3)
	{
		//Away
		return SetAway(bIsOn);
	}

	return false;
}

void CNestThermostat::GetMeterDetails()
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
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-user-id:" + m_UserID);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	//Get Data
	std::string sURL = m_TransportURL + NEST_GET_STATUS + m_UserID;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestThermostat: Error getting current state!");
		m_bDoLogin = true;
		return;
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "NestThermostat: Invalid data received!");
		m_bDoLogin = true;
		return;
	}
	if (root["shared"].empty() == true)
	{
		_log.Log(LOG_ERROR, "NestThermostat: request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	if (root["shared"].size()<1)
	{
		_log.Log(LOG_ERROR, "NestThermostat: request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}

	Json::Value::Members members = root["shared"].getMemberNames();
	if (members.size() < 1)
	{
		_log.Log(LOG_ERROR, "NestThermostat: request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	//Get Serial
	m_Serial = *members.begin();

	//Get Structure
	members = root["structure"].getMemberNames();
	if (members.size() < 1)
	{
		_log.Log(LOG_ERROR, "NestThermostat: request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	m_StructureID = *members.begin();

	Json::Value vShared = *root["shared"].begin();

	//Setpoint
	if (!vShared["target_temperature"].empty())
	{
		float currentSetpoint = vShared["target_temperature"].asFloat();
		SendSetPointSensor(1, currentSetpoint, "Room Setpoint");
	}
	//Room Temperature/Humidity
	if (!vShared["current_temperature"].empty())
	{
		float currentTemp = vShared["current_temperature"].asFloat();
		int Humidity = root["device"][m_Serial]["current_humidity"].asInt();
		SendTempHumSensor(2, 255, currentTemp, Humidity, "Room TempHum");
	}

	//Away
	Json::Value vStructure = *root["structure"].begin();
	if (!vStructure["away"].empty())
	{
		bool bIsAway = vStructure["away"].asBool();
		SendSwitch(3, 1, 255, bIsAway, 0, "Away");
	}


}

void CNestThermostat::SetSetpoint(const int idx, const float temp)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	if (m_bDoLogin == true)
	{
		if (!Login())
			return;
	}
	if (m_Serial.size() == 0)
		GetMeterDetails();

	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	std::stringstream sstr;
	Json::Value root;
	root["target_change_pending"] = true;
	root["target_temperature"] = temp;

	std::string sResult;

	std::string sURL = m_TransportURL + NEST_SET_SHARED + m_Serial;
	if (!HTTPClient::POST(sURL, root.toStyledString(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestThermostat: Error setting setpoint!");
		m_bDoLogin = true;
		return;
	}
}

bool CNestThermostat::SetAway(const bool bIsAway)
{
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;

	if (m_bDoLogin == true)
	{
		if (!Login())
			return false;
	}
	if (m_StructureID.size() == 0)
		GetMeterDetails();

	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	std::stringstream sstr;
	Json::Value root;
	root["away"] = bIsAway;
	root["away_timestamp"] = (int)mytime(NULL);
	root["away_setter"] = 0;

	std::string sResult;

	std::string sURL = m_TransportURL + NEST_SET_STRUCTURE + m_StructureID;
	if (!HTTPClient::POST(sURL, root.toStyledString(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NestThermostat: Error setting away mode!");
		m_bDoLogin = true;
		return false;
	}
	return true;
}

void CNestThermostat::SetProgramState(const int newState)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}
}
