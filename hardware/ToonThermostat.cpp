#include "stdafx.h"
#include "ToonThermostat.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"

#ifdef _DEBUG
//	#define DEBUG_ToonThermostat
//	#define DEBUG_ToonThermostatW
#endif

#define TOON_POLL_INTERVAL 300
#define TOON_POLL_INTERVAL_SHORT 30

#ifdef DEBUG_ToonThermostatW
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
#ifdef DEBUG_ToonThermostat
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

CToonThermostat::CToonThermostat(const int ID, const std::string &Username, const std::string &Password, const int &Agreement) :
m_UserName(Username),
m_Password(Password),
m_Agreement(Agreement)
{
	m_HwdID=ID;

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

	m_ClientID = "";
	m_ClientIDChecksum = "";
	m_lastSharedSendElectra = 0;
	m_lastSharedSendGas = 0;
	m_lastgasusage = 0;
	m_lastelectrausage = 0;
	m_lastelectradeliv = 0;

	m_LastUsage1 = 0;
	m_LastUsage2 = 0;
	m_OffsetUsage1 = 0;
	m_OffsetUsage2 = 0;
	m_LastDeliv1 = 0;
	m_LastDeliv2 = 0;
	m_OffsetDeliv1 = 0;
	m_OffsetDeliv2 = 0;

	m_bDoLogin = true;
	m_poll_counter = 5;
	m_retry_counter = 0;
}

void CToonThermostat::Init()
{
	m_ClientID = "";
	m_ClientIDChecksum = "";
	m_lastSharedSendElectra = 0;
	m_lastSharedSendGas = 0;
	m_lastgasusage = 0;
	m_lastelectrausage = 0;
	m_lastelectradeliv = 0;

	m_LastUsage1=0;
	m_LastUsage2=0;
	m_OffsetUsage1=0;
	m_OffsetUsage2=0;
	m_LastDeliv1 = 0;
	m_LastDeliv2 = 0;
	m_OffsetDeliv1 = 0;
	m_OffsetDeliv2 = 0;

	//Get Last meter counter values for Usage/Delivered
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID==1) AND ([Type]==%d) AND (SubType==%d)", m_HwdID, pTypeP1Power, sTypeP1Power);
	if (!result.empty())
	{
		unsigned long devID = (unsigned long)atol(result[0][0].c_str());
		result = m_sql.safe_query("SELECT MAX(Counter1), MAX(Counter2), MAX(Counter3), MAX(Counter4) FROM Multimeter_Calendar WHERE (DeviceRowID==%ld)", devID);
		if (!result.empty())
		{
			std::vector<std::string> sd = *result.begin();
			m_OffsetUsage1 = (unsigned long)atol(sd[0].c_str());
			m_OffsetDeliv1 = (unsigned long)atol(sd[1].c_str());
			m_OffsetUsage2 = (unsigned long)atol(sd[2].c_str());
			m_OffsetDeliv2 = (unsigned long)atol(sd[3].c_str());
		}
	}
	m_bDoLogin = true;
	m_poll_counter = 5;
	m_retry_counter = 0;
}

bool CToonThermostat::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CToonThermostat::StopHardware()
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

void CToonThermostat::Do_Work()
{
	_log.Log(LOG_STATUS,"ToonThermostat: Worker started...");
	int sec_counter = 1;
	while (!IsStopRequested(1000))
	{
		m_poll_counter--;
		sec_counter++;
		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
		if (m_poll_counter<=0)
		{
			m_poll_counter = TOON_POLL_INTERVAL;
			GetMeterDetails();
			if (m_retry_counter >= 3)
			{
				m_bDoLogin = true;
				m_retry_counter = 0;
				_log.Log(LOG_ERROR, "ToonThermostat: retrieveToonState request not successful, restarting..!");
			}
		}
	}
	Logout();

	_log.Log(LOG_STATUS,"ToonThermostat: Worker stopped...");
}

void CToonThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), 255, nullptr);
}

void CToonThermostat::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
		m_HwdID, pTypeLighting2, sTypeAC, szIdx);
	if (!result.empty())
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}

std::string CToonThermostat::GetRandom()
{
	return GenerateUUID();
}

bool CToonThermostat::Login()
{
	if (!m_ClientID.empty())
	{
		Logout();
	}
	m_ClientID = "";
	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << CURLEncode::URLEncode(m_Password);
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::string sURL = TOON_HOST + TOON_LOGIN_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"ToonThermostat: Error login!");
		return false;
	}

#ifdef DEBUG_ToonThermostatW
	char szFileName[MAX_PATH];
	static int lNum = 1;
	sprintf_s(szFileName, "E:\\toonlogin_%03d.txt", lNum++);
	SaveString2Disk(sResult, szFileName);
#endif

	Json::Value root;
	bool bRet = ParseJSon(sResult, root);
	if (!bRet)
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
	if (root["agreements"].size() < size_t(m_Agreement) + 1)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Agreement not found, did you setup your toon correctly?");
		return false;
	}

	agreementId = root["agreements"][m_Agreement]["agreementId"].asString();
	agreementIdChecksum = root["agreements"][m_Agreement]["agreementIdChecksum"].asString();

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
#ifdef DEBUG_ToonThermostatW
	char szFileName2[MAX_PATH];
	static int l2Num = 1;
	sprintf_s(szFileName2, "E:\\toonlogin_authstart_%03d.txt", l2Num++);
	SaveString2Disk(sResult, szFileName2);
#endif

	root.clear();
	bRet = ParseJSon(sResult, root);
	if (!bRet)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].asBool() == true)
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
	m_sql.safe_query("INSERT INTO ToonDevices (HardwareID, UUID) VALUES (%d, '%q')",
		m_HwdID, UUID.c_str());
	return GetUUIDIdx(UUID, idx);
}

bool CToonThermostat::GetUUIDIdx(const std::string &UUID, int &idx)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT [ROWID] FROM ToonDevices WHERE (HardwareID=%d) AND (UUID='%q')",
		m_HwdID, UUID.c_str());
	if (result.empty())
		return false;
	std::vector<std::string> sd = result[0];
	idx = atoi(sd[0].c_str());
	return true;
}

bool CToonThermostat::GetUUIDFromIdx(const int idx, std::string &UUID)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT [UUID] FROM ToonDevices WHERE (HardwareID=%d) AND (ROWID=%d)",
		m_HwdID, idx);
	if (result.empty())
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
	bool bRet = ParseJSon(sResult, root);
	if (!bRet)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
/*
	int Idx=0;
	if (GetUUIDIdx(UUID, Idx))
	{
		UpdateSwitch(Idx, SwitchState != 0, "");
	}
*/
	m_retry_counter = 0;
	m_poll_counter = TOON_POLL_INTERVAL_SHORT;

	return (root["success"].asBool() == true);
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
	bool bRet = ParseJSon(sResult, root);
	if (!bRet)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	if (root["success"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return false;
	}
	m_retry_counter = 0;
	m_poll_counter = TOON_POLL_INTERVAL_SHORT;
	return (root["success"].asBool() == true);
}

bool CToonThermostat::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (m_UserName.empty())
		return false;
	if (m_Password.empty())
		return false;

	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false; //later add RGB support, if someone can provide access

	int node_id = pCmd->LIGHTING2.id4;
	if ((node_id == 113) || (node_id == 114) || (node_id == 115))
		return false; //we can not turn on/off the internal status

	int state = 0;
	if (pCmd->LIGHTING2.cmnd == light2_sOn)
		state = 1;

	if (node_id == 254)
		return SwitchAll(state);

	std::string uuid;
	if (!GetUUIDFromIdx(node_id, uuid))
		return false;
	return SwitchLight(uuid, state);
}

double CToonThermostat::GetElectricOffset(const int idx, const double currentKwh)
{
	std::map<int, double>::const_iterator itt = m_OffsetElectricUsage.find(idx);
	if (itt == m_OffsetElectricUsage.end())
	{
		//First time, lets add it
		bool bExists = false;
		m_OffsetElectricUsage[idx] = GetKwhMeter(idx, 1, bExists);
		m_LastElectricCounter[idx] = currentKwh;
	}
	return m_OffsetElectricUsage[idx];
}

void CToonThermostat::GetMeterDetails()
{
	if (m_UserName.empty())
		return;
	if (m_Password.empty())
		return;
	std::string sResult;
	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}
	std::vector<std::string> ExtraHeaders;
	Json::Value root;

	bool bIsValid = false;
	std::stringstream sstr2;
	sstr2 << "?clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();
	//Get Data

#ifdef DEBUG_ToonThermostat
	sResult = ReadFile("E:\\toonresult_001.txt");
#else
	std::string sURL = TOON_HOST + TOON_UPDATE_PATH + szPostdata;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error getting current state!");
		m_bDoLogin = true;
		return;
	}
#endif
#ifdef DEBUG_ToonThermostatW
	char szFileName[MAX_PATH];
	static int sNum = 1;
	sprintf_s(szFileName, "E:\\toonresult_%03d.txt", sNum++);
	SaveString2Disk(sResult, szFileName);
#endif

	bool bRet = ParseJSon(sResult, root);
	if (!bRet)
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
	if (root["success"].asBool() == false)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: ToonState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}

	//check if we have all required data fields, if not retry with a shorter interval
	if (root["thermostatInfo"].empty() == false)
	{
		if (root["powerUsage"].empty() == false)
		{
			if (root["gasUsage"].empty() == false)
			{
				bIsValid = true;
			}
		}
	}
	if (!bIsValid)
	{
		m_retry_counter++;
		m_poll_counter = TOON_POLL_INTERVAL_SHORT;
		return;
	}
	m_retry_counter = 0;

	ParseThermostatData(root);
	ParsePowerUsage(root);
	ParseGasUsage(root);
	ParseDeviceStatusData(root);
}

bool CToonThermostat::ParsePowerUsage(const Json::Value &root)
{
	if (root["powerUsage"].empty())
		return false;

	time_t atime = mytime(nullptr);

	unsigned long powerusage1 = (unsigned long)(root["powerUsage"]["meterReadingLow"].asFloat());
	unsigned long powerusage2 = (unsigned long)(root["powerUsage"]["meterReading"].asFloat());

	if ((powerusage1 == 0) && (powerusage2 == 0))
	{
		//New firmware does not provide meter readings anymore
		if (root["powerUsage"]["dayUsage"].empty() == false)
		{
			unsigned long usage1 = (unsigned long)(root["powerUsage"]["dayUsage"].asFloat());
			unsigned long usage2 = (unsigned long)(root["powerUsage"]["dayLowUsage"].asFloat());
			if (usage1 < m_LastUsage1)
			{
				m_OffsetUsage1 += m_LastUsage1;
			}
			if (usage2 < m_LastUsage2)
			{
				m_OffsetUsage2 += m_LastUsage2;
			}
			m_p1power.powerusage1 = m_OffsetUsage1 + usage1;
			m_p1power.powerusage2 = m_OffsetUsage2 + usage2;
			m_LastUsage1 = usage1;
			m_LastUsage2 = usage2;
		}
	}
	else
	{
		m_p1power.powerusage1 = powerusage1;
		m_p1power.powerusage2 = powerusage2;
	}

	if (root["powerUsage"]["meterReadingProdu"].empty() == false)
	{
		unsigned long powerdeliv1 = (unsigned long)(root["powerUsage"]["meterReadingLowProdu"].asFloat());
		unsigned long powerdeliv2 = (unsigned long)(root["powerUsage"]["meterReadingProdu"].asFloat());

		if ((powerdeliv1 != 0) || (powerdeliv2 != 0))
		{
			m_p1power.powerdeliv1 = powerdeliv1;
			m_p1power.powerdeliv2 = powerdeliv2;
		}
		else
		{
			//Have not received an example from a user that has produced with the new firmware
			//for now ignoring
		}
	}

	m_p1power.usagecurrent = (unsigned long)(root["powerUsage"]["value"].asFloat());	//Watt
	m_p1power.delivcurrent = (unsigned long)(root["powerUsage"]["valueProduced"].asFloat());	//Watt

	if (root["powerUsage"]["valueSolar"].empty() == false)
	{
		float valueSolar = (float)(root["powerUsage"]["valueSolar"].asFloat());
		if (valueSolar != 0)
		{
			SendWattMeter(1, 1, 255, valueSolar, "Solar");
		}
	}

	//Send Electra if value changed, or at least every 5 minutes
	if (
		(m_p1power.usagecurrent != m_lastelectrausage) ||
		(m_p1power.delivcurrent != m_lastelectradeliv) ||
		(difftime(atime,m_lastSharedSendElectra) >= 300)
		)
	{
		if ((m_p1power.powerusage1 != 0) || (m_p1power.powerusage2 != 0) || (m_p1power.powerdeliv1 != 0) || (m_p1power.powerdeliv2 != 0))
		{
			m_lastSharedSendElectra = atime;
			m_lastelectrausage = m_p1power.usagecurrent;
			m_lastelectradeliv = m_p1power.delivcurrent;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power, nullptr, 255, nullptr);
		}
	}
	return true;
}

bool CToonThermostat::ParseGasUsage(const Json::Value &root)
{
	if (root["gasUsage"].empty())
		return false;
	time_t atime = mytime(nullptr);

	m_p1gas.gasusage = (unsigned long)(root["gasUsage"]["meterReading"].asFloat());

	//Send GAS if the value changed, or at least every 5 minutes
	if (
		(m_p1gas.gasusage != m_lastgasusage) ||
		(difftime(atime,m_lastSharedSendGas) >= 300)
		)
	{
		if (m_p1gas.gasusage != 0)
		{
			m_lastSharedSendGas = atime;
			m_lastgasusage = m_p1gas.gasusage;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1gas, nullptr, 255, nullptr);
		}
	}
	return true;
}

bool CToonThermostat::ParseDeviceStatusData(const Json::Value &root)
{
	//ZWave Devices
	if (root["deviceStatusInfo"].empty())
		return false;

	if (root["deviceStatusInfo"]["device"].empty())
		return false;

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
				return false;
			}
		}
		UpdateSwitch(Idx, state != 0, deviceName);

		if (root["deviceStatusInfo"]["device"][ii]["currentUsage"].empty() == false)
		{
			double currentUsage = root["deviceStatusInfo"]["device"][ii]["currentUsage"].asDouble();
			double DayCounter = root["deviceStatusInfo"]["device"][ii]["dayUsage"].asDouble();

			//double ElecOffset = GetElectricOffset(Idx, DayCounter);
			double OldDayCounter = m_LastElectricCounter[Idx];
			if (DayCounter < OldDayCounter)
			{
				//daily counter went to zero
				m_OffsetElectricUsage[Idx] += OldDayCounter;
			}
			m_LastElectricCounter[Idx] = DayCounter;
			SendKwhMeterOldWay(Idx, 1, 255, currentUsage / 1000.0, (m_OffsetElectricUsage[Idx] + m_LastElectricCounter[Idx]) / 1000.0, deviceName);
		}
	}
	return true;
}

bool CToonThermostat::ParseThermostatData(const Json::Value &root)
{
	//thermostatInfo
	if (root["thermostatInfo"].empty())
		return false;

	float currentTemp = root["thermostatInfo"]["currentTemp"].asFloat() / 100.0F;
	float currentSetpoint = root["thermostatInfo"]["currentSetpoint"].asFloat() / 100.0F;
	SendSetPointSensor(1, currentSetpoint, "Room Setpoint");
	SendTempSensor(1, 255, currentTemp, "Room Temperature");

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
	return true;
}

void CToonThermostat::SetSetpoint(const int idx, const float temp)
{
	if (m_UserName.empty())
		return;
	if (m_Password.empty())
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
		sprintf(szTemp, "%d", int(temp * 100.0F));
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
		bool bRet = ParseJSon(sResult, root);
		if (!bRet)
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
		if (root["success"].asBool() == false)
		{
			_log.Log(LOG_ERROR, "ToonThermostat: setPoint request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
		SendSetPointSensor(idx, temp, "Room Setpoint");
		m_retry_counter = 0;
		m_poll_counter = TOON_POLL_INTERVAL_SHORT;
	}
}

void CToonThermostat::SetProgramState(const int newState)
{
	if (m_UserName.empty())
		return;
	if (m_Password.empty())
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
	bool bRet = ParseJSon(sResult, root);
	if (!bRet)
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
	if (root["success"].asBool() == false)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: setProgramState request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}
	m_retry_counter = 0;
	m_poll_counter = TOON_POLL_INTERVAL_SHORT;
}
