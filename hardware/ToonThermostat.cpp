#include "stdafx.h"
#include "ToonThermostat.h"
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

#ifdef _DEBUG
	//#define DEBUG_ToonThermostat
#endif

#ifdef DEBUG_ToonThermostat
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

const std::string TOON_HOST = "https://toonopafstand.eneco.nl";
//const std::string TOON_HOST = "https://toonopafstand-acc.quby.nl"; //needs URL encoding
const std::string TOON_LOGIN_PATH = "/toonMobileBackendWeb/client/login";
const std::string TOON_LOGOUT_PATH = "/toonMobileBackendWeb/client/auth/logout";
const std::string TOON_LOGINCHECK_PATH = "/toonMobileBackendWeb/client/checkIfLoggedIn";
const std::string TOON_AGREEMENT_PATH = "/toonMobileBackendWeb/client/auth/start";
const std::string TOON_UPDATE_PATH = "/toonMobileBackendWeb/client/auth/retrieveToonState";
const std::string TOON_CHANGE_SCHEME = "/toonMobileBackendWeb/client/auth/schemeState";
const std::string TOON_TEMPSET_PATH = "/toonMobileBackendWeb/client/auth/setPoint";
const std::string TOON_CHANGE_SCREEN_PATH = "/toonMobileBackendWeb/client/auth/kpi/changedScreen";
const std::string TOON_TAB_PRESSED_PATH = "/toonMobileBackendWeb/client/auth/kpi/tabPressed";
const std::string TOON_GET_ELECTRIC_GRAPH = "/toonMobileBackendWeb/client/auth/getElecGraphData";
const std::string TOON_GET_GAS_GRAPH = "/toonMobileBackendWeb/client/auth/getGasGraphData"; //?smartMeter=false
const std::string TOON_SWITCH_POWER = "/toonMobileBackendWeb/client/auth/smartplug/setTarget";
const std::string TOON_SWITCH_ALL = "/toonMobileBackendWeb/client/auth/smartplug/switchAll";

//enum _eProgramStates {
//	PROG_MANUAL = 0,
//	PROG_BASE,			//1
//	PROG_TEMPOVERRIDE,	//2
//	PROG_PROGOVERRIDE,	//3
//	PROG_HOLIDAY,		//4
//	PROG_MANUALHOLIDAY,	//5
//	PROG_AWAYNOW,		//6
//	PROG_DAYOFF,		//7
//	PROG_LOCKEDBASE		//8
//};

//enum _eActiveStates {
//	STATE_RELAX = 0,
//	STATE_ACTIVE,	//1
//	STATE_SLEEP,	//2
//	STATE_AWAY,		//3
//	STATE_HOLIDAY	//4
//};

CToonThermostat::CToonThermostat(const int ID, const std::string &Username, const std::string &Password)
{
	m_HwdID=ID;
	m_UserName=Username;
	m_Password=Password;
	m_ClientID = "";

	memset(&m_p1power, 0, sizeof(m_p1power));
	memset(&m_p1gas, 0, sizeof(m_p1gas));

	m_p1power.len = sizeof(P1Power)-1;
	m_p1power.type = pTypeP1Power;
	m_p1power.subtype = sTypeP1Power;
	m_p1power.ID = 1;

	m_p1gas.len = sizeof(P1Gas)-1;
	m_p1gas.type = pTypeP1Gas;
	m_p1gas.subtype = sTypeP1Gas;
	m_p1gas.ID = 1;

	Init();
}

CToonThermostat::~CToonThermostat(void)
{
}

void CToonThermostat::Init()
{
	m_ClientID = "";
	m_ClientIDChecksum = "";
	m_stoprequested = false;
	m_lastSharedSendElectra = 0;
	m_lastSharedSendGas = 0;
	m_lastgasusage = 0;
	m_lastelectrausage = 0;
	m_bDoLogin = true;
}

bool CToonThermostat::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CToonThermostat::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CToonThermostat::StopHardware()
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

#define TOON_POLL_INTERVAL 30

void CToonThermostat::Do_Work()
{
	int sec_counter = TOON_POLL_INTERVAL-5;
	_log.Log(LOG_STATUS,"ToonThermostat: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
		if (sec_counter % TOON_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"ToonThermostat: Worker stopped...");
}

void CToonThermostat::SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=12;
	tsen.TEMP.id1=0;
	tsen.TEMP.id2=Idx;

	tsen.TEMP.tempsign=(Temp>=0)?0:1;
	int at10=round(abs(Temp*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
		result=m_sql.query(szQuery.str());
	}
}

void CToonThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
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

void CToonThermostat::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Type==" << pTypeLighting2 << ") AND (SubType==" << sTypeAC << ") AND (DeviceID=='" << szIdx << "')";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	else
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
			return;
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = 1;
	int level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (Type==" << pTypeLighting2 << ") AND (SubType==" << sTypeAC << ") AND (DeviceID=='" << szIdx << "')";
		result = m_sql.query(szQuery.str());
	}
}


std::string CToonThermostat::GetRandom()
{
	//5BA37E41-B5C8-4C19-AA81-9EA82430D7EA
	char szTmp[100];
	sprintf(szTmp,"%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
		// 32 bits for "time_low"
		rand() % 0xFFFF, rand() % 0xFFFF,

		// 16 bits for "time_mid"
		rand() % 0xFFFF,

		// 16 bits for "time_hi_and_version",
		// four most significant bits holds version number 4
		rand() % 0xFFFF | 0x4000,

		// 16 bits, 8 bits for "clk_seq_hi_res",
		// 8 bits for "clk_seq_low",
		// two most significant bits holds zero and one for variant DCE1.1
		rand() % 0xFFFF | 0x8000,

		// 48 bits for "node"
		rand() % 0xFFFF, rand() % 0xFFFF, rand() % 0xFFFF
		);
	std::string ret=szTmp;
	return ret;
}

bool CToonThermostat::Login()
{
	if (m_ClientID != "")
	{
		Logout();
	}
	m_ClientID = "";
	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::string sURL = TOON_HOST + TOON_LOGIN_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"ToonThermostat: Error login!");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}

	if (root["clientId"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_ClientID = root["clientId"].asString();
	if (root["clientIdChecksum"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_ClientIDChecksum = root["clientIdChecksum"].asString();

	std::string agreementId;
	std::string agreementIdChecksum;

	if (root["agreements"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	if (root["agreements"].size() < 1)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: No agreements found, did you setup your toon correctly?");
		return false;
	}
	agreementId = root["agreements"][0]["agreementId"].asString();
	agreementIdChecksum = root["agreements"][0]["agreementIdChecksum"].asString();

	std::stringstream sstr2;
	sstr2 << "clientId=" << m_ClientID 
		 << "&clientIdChecksum=" << m_ClientIDChecksum
		 << "&agreementId=" << agreementId
		 << "&agreementIdChecksum=" << agreementIdChecksum
		 << "&random=" << GetRandom();
	szPostdata = sstr2.str();
	sResult = "";

	sURL = TOON_HOST + TOON_AGREEMENT_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error login!");
		return false;
	}
	root.clear();
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"] == true)
	{
		m_bDoLogin = false;
		return true;
	}

	return false;
}

void CToonThermostat::Logout()
{
	if (m_bDoLogin)
		return; //we are not logged in
	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	std::stringstream sstr2;
	sstr2 << "clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();

	std::string sURL = TOON_HOST + TOON_LOGOUT_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error Logout!");
	}
	m_ClientID = "";
	m_ClientIDChecksum = "";
	m_bDoLogin = true;
}

bool CToonThermostat::AddUUID(const std::string &UUID, int &idx)
{
	std::stringstream sstr;

	sstr << "INSERT INTO ToonDevices (HardwareID, UUID) VALUES (" << m_HwdID << ", '" << UUID << "')";
	m_sql.query(sstr.str());
	return GetUUIDIdx(UUID, idx);
}

bool CToonThermostat::GetUUIDIdx(const std::string &UUID, int &idx)
{
	std::stringstream sstr;

	std::vector<std::vector<std::string> > result;
	sstr << "SELECT [ROWID] FROM ToonDevices WHERE (HardwareID=" << m_HwdID << ") AND (UUID='" << UUID << "')";
	result = m_sql.query(sstr.str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	idx = atoi(sd[0].c_str());
	return true;
}

bool CToonThermostat::GetUUIDFromIdx(const int idx, std::string &UUID)
{
	std::stringstream sstr;

	std::vector<std::vector<std::string> > result;
	sstr << "SELECT [UUID] FROM ToonDevices WHERE (HardwareID=" << m_HwdID << ") AND (ROWID=" << idx << ")";
	result = m_sql.query(sstr.str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	UUID = sd[0];
	return true;
}

bool CToonThermostat::SwitchLight(const std::string &UUID, const int SwitchState)
{
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sstr;

	sstr << "?clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&devUuid=" << UUID
		<< "&state=" << SwitchState
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr.str();
	std::string sURL = TOON_HOST + TOON_SWITCH_POWER + szPostdata;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error setting switch state!");
		m_bDoLogin = true;
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	return (root["success"] == true);
}

bool CToonThermostat::SwitchAll(const int SwitchState)
{
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sstr;

	sstr << "?clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&state=" << SwitchState
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr.str();
	std::string sURL = TOON_HOST + TOON_SWITCH_ALL + szPostdata;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error setting switch state!");
		m_bDoLogin = true;
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	return (root["success"] == true);
}

bool CToonThermostat::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;

	tRBUF *pCmd = (tRBUF *)pdata;
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false; //later add RGB support, if someone can provide access

	int node_id = pCmd->LIGHTING2.id4;
	if ((node_id == 113) || (node_id == 114) || (node_id == 115))
		return false; //we can not turn on/off the internal status

	int state = 0;
	int light_command = pCmd->LIGHTING2.cmnd;
	if (pCmd->LIGHTING2.cmnd == light2_sOn)
		state = 1;


	if (node_id == 254)
		return SwitchAll(state);

	std::string uuid;
	if (!GetUUIDFromIdx(node_id, uuid))
		return false;
	return SwitchLight(uuid, state);
}

void CToonThermostat::GetMeterDetails()
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

	std::stringstream sstr2;
	sstr2 << "?clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();
	//Get Data
	std::string sURL = TOON_HOST + TOON_UPDATE_PATH + szPostdata;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error getting current state!");
		m_bDoLogin = true;
		return;
	}
	time_t atime = mytime(NULL);

#ifdef DEBUG_ToonThermostat
	char szFileName[MAX_PATH];
	static int sNum = 1;
	sprintf_s(szFileName, "E:\\toonresult_%03d.txt", sNum++);
	SaveString2Disk(sResult, szFileName);
#endif

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		m_bDoLogin = true;
		return;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: ToonState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	if (root["success"] == false)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: ToonState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}

	//ZWave Devices
	if (root["deviceStatusInfo"].empty() == false)
	{
		if (root["deviceStatusInfo"]["device"].empty() == false)
		{
			int totDevices = root["deviceStatusInfo"]["device"].size();
			for (int ii = 0; ii < totDevices; ii++)
			{
				std::string deviceName = root["deviceStatusInfo"]["device"][ii]["name"].asString();
				std::string uuid = root["deviceStatusInfo"]["device"][ii]["devUUID"].asString();
				int state = root["deviceStatusInfo"]["device"][ii]["currentState"].asInt();

				int Idx;
				if (!GetUUIDIdx(uuid, Idx))
				{
					if (!AddUUID(uuid, Idx))
					{
						_log.Log(LOG_ERROR, "ToonThermostat: Error adding UUID to database?! Uuid=%s", uuid.c_str());
						return;
					}
				}
				UpdateSwitch(Idx, state != 0, deviceName);
			}
		}
	}

	//thermostatInfo
	if (root["thermostatInfo"].empty() == false)
	{
		float currentTemp = root["thermostatInfo"]["currentTemp"].asFloat() / 100.0f;
		float currentSetpoint = root["thermostatInfo"]["currentSetpoint"].asFloat() / 100.0f;
		SendSetPointSensor(1, currentSetpoint, "Room Setpoint");
		SendTempSensor(1, currentTemp, "Room Temperature");

		//int programState = root["thermostatInfo"]["programState"].asInt();
		//int activeState = root["thermostatInfo"]["activeState"].asInt();

		if (root["thermostatInfo"]["burnerInfo"].empty() == false)
		{
			//burnerinfo
			//0=off
			//1=heating
			//2=hot water
			//3=pre-heating
			int burnerInfo = 0;

			if (root["thermostatInfo"]["burnerInfo"].isString())
			{
				burnerInfo = atoi(root["thermostatInfo"]["burnerInfo"].asString().c_str());
			}
			else if (root["thermostatInfo"]["burnerInfo"].isInt())
			{
				burnerInfo = root["thermostatInfo"]["burnerInfo"].asInt();
			}
			if (burnerInfo == 1)
			{
				UpdateSwitch(113, true, "HeatingOn");
				UpdateSwitch(114, false, "TapwaterOn");
				UpdateSwitch(115, false, "PreheatOn");
			}
			else if (burnerInfo == 2)
			{
				UpdateSwitch(113, false, "HeatingOn");
				UpdateSwitch(114, true, "TapwaterOn");
				UpdateSwitch(115, false, "PreheatOn");
			}
			else if (burnerInfo == 3)
			{
				UpdateSwitch(113, false, "HeatingOn");
				UpdateSwitch(114, false, "TapwaterOn");
				UpdateSwitch(115, true, "PreheatOn");
			}
			else
			{
				UpdateSwitch(113, false, "HeatingOn");
				UpdateSwitch(114, false, "TapwaterOn");
				UpdateSwitch(115, false, "PreheatOn");
			}
		}
	}

	if (root["gasUsage"].empty() == false)
	{
		m_p1gas.gasusage = (unsigned long)(root["gasUsage"]["meterReading"].asFloat());
	}

	if (root["powerUsage"].empty() == false)
	{
		m_p1power.powerusage1 = (unsigned long)(root["powerUsage"]["meterReadingLow"].asFloat());
		m_p1power.powerusage2 = (unsigned long)(root["powerUsage"]["meterReading"].asFloat());

		if (root["powerUsage"]["meterReadingProdu"].empty() == false)
		{
			m_p1power.powerdeliv1 = (unsigned long)(root["powerUsage"]["meterReadingLowProdu"].asFloat());
			m_p1power.powerdeliv2 = (unsigned long)(root["powerUsage"]["meterReadingProdu"].asFloat());
		}

		m_p1power.usagecurrent = (unsigned long)(root["powerUsage"]["value"].asFloat());	//Watt
	}

	//Send Electra if value changed, or at least every 5 minutes
	if (
		(m_p1power.usagecurrent != m_lastelectrausage) ||
		(atime - m_lastSharedSendElectra >= 300)
		)
	{
		if ((m_p1power.powerusage1 != 0) || (m_p1power.powerusage2 != 0) || (m_p1power.powerdeliv1 != 0) || (m_p1power.powerdeliv2 != 0))
		{
			m_lastSharedSendElectra = atime;
			m_lastelectrausage = m_p1power.usagecurrent;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power);
		}
	}
	
	//Send GAS if the value changed, or at least every 5 minutes
	if (
		(m_p1gas.gasusage != m_lastgasusage) ||
		(atime - m_lastSharedSendGas >= 300)
		)
	{
		if (m_p1gas.gasusage != 0)
		{
			m_lastSharedSendGas = atime;
			m_lastgasusage = m_p1gas.gasusage;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1gas);
		}
	}
}

void CToonThermostat::SetSetpoint(const int idx, const float temp)
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

	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	if (idx==1)
	{
		//Room Set Point

		char szTemp[20];
		sprintf(szTemp,"%d",int(temp*100.0f));
		std::string sTemp = szTemp;

		std::stringstream sstr2;
		sstr2 << "?clientId=" << m_ClientID
			<< "&clientIdChecksum=" << m_ClientIDChecksum
			<< "&value=" << sTemp
			<< "&random=" << GetRandom();
		std::string szPostdata = sstr2.str();

		std::string sURL = TOON_HOST + TOON_TEMPSET_PATH + szPostdata;
		if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "ToonThermostat: Error setting setpoint!");
			m_bDoLogin = true;
			return;
		}

		Json::Value root;
		Json::Reader jReader;
		if (!jReader.parse(sResult, root))
		{
			_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
			m_bDoLogin = true;
			return;
		}
		if (root["success"].empty() == true)
		{
			_log.Log(LOG_ERROR, "ToonThermostat: setPoint request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
		if (root["success"] == false)
		{
			_log.Log(LOG_ERROR, "ToonThermostat: setPoint request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
	}
}

void CToonThermostat::SetProgramState(const int newState)
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

	std::stringstream sstr2;
	sstr2 << "?clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&state=2"
		<< "&temperatureState=" << newState
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();

	std::string sURL = TOON_HOST + TOON_CHANGE_SCHEME + szPostdata;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error setting Program State!");
		return;
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: setProgramState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: setProgramState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	if (root["success"] == false)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: setProgramState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
}
