#include "stdafx.h"
#include "NefitEasy.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../webserver/Base64.h"

/*
We need HTTP server to access Nefit/Bosch XMPP backend over HTTP written by Robert Klep (https://github.com/robertklep/nefit-easy-http-server)

Install:
sudo npm un -g nefit-easy-http-server
sudo npm i -g nefit-easy-http-server

Start: (should be done automatically)

easy-server --host=0.0.0.0 --serial=XXXX --access-key=XXXX --password=XXXX

After this you should be able to connect to port 3000
*/

#ifdef _DEBUG
	//#define DEBUG_NefitEasyW
	//#define DEBUG_NefitEasyR
#endif

#ifdef DEBUG_NefitEasyW
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
#ifdef DEBUG_NefitEasyR
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

// Some convenience code, saves typing and duplication

template<typename T>
Json::Value JsonValue(const T& tValue)
{
	Json::Value jValue;
	jValue["value"] = tValue;
	return jValue;
}

class LogError
{
private:

	char szErrorMessage[1024];

public:

	LogError(const char* szRequestUrl)
	{
		sprintf(szErrorMessage, "NefitEasy: (request=%s): ", szRequestUrl);
	}
	
	bool operator()(const char* szErrorFormat, ...)
	{
		int iOffset = strlen(szErrorMessage);
		
		va_list args;
		va_start(args, szErrorFormat);
		vsnprintf(szErrorMessage + iOffset, 1024 - iOffset, szErrorFormat, args);
		va_end(args);
		
		_log.Log(LOG_ERROR, szErrorMessage);
		
		return false;
	}
	
	bool None()
	{
		return true;
	}
};

#define NEFITEASY_HTTP_BRIDGE "/bridge"
#define NEFITEASY_STATUS_URL "/ecus/rrc/uiStatus"
#define NEFITEASY_OUTDOORTEMP_URL "/system/sensors/temperatures/outdoor_t1"
#define NEFITEASY_FLOWTEMP_URL "/heatingCircuits/hc1/actualSupplyTemperature"
#define NEFITEASY_PRESSURE_URL "/system/appliance/systemPressure"
#define NEFITEASY_DISPLAYCODE_URL "/system/appliance/displaycode"
#define NEFITEASY_CAUSECODE_URL "/system/appliance/causecode"
#define NEFITEASY_GAS_URL "/ecus/rrc/recordings/yearTotal"
#define NEFITEASY_SET_TEMP_ROOM "/heatingCircuits/hc1/temperatureRoomManual"
#define NEFITEASY_SET_TEMP_OVERRIDE "/heatingCircuits/hc1/manualTempOverride/status"
#define NEFITEASY_SET_TEMP_OVERRIDE_TEMP "/heatingCircuits/hc1/manualTempOverride/temperature"
#define NEFITEASY_SET_USER_MODE "/heatingCircuits/hc1/usermode"
#define NEFITEASY_SET_HOT_WATER_MANUAL_MODE "/dhwCircuits/dhwA/dhwOperationManualMode"
#define NEFITEASY_SET_HOT_WATER_CLOCK_MODE "/dhwCircuits/dhwA/dhwOperationClockMode"
#define NEFITEASY_LOCATION_LATITUDE "/system/location/latitude"
#define NEFITEASY_LOCATION_LONGITUDE "/system/location/longitude"

// Magic numbers
const uint8_t nmagic_chat[] = {
	0x58, 0xf1, 0x8d, 0x70, 0xf6, 0x67, 0xc9, 0xc7,
	0x9e, 0xf7, 0xde, 0x43, 0x5b, 0xf0, 0xf9, 0xb1,
	0x55, 0x3b, 0xbb, 0x6e, 0x61, 0x81, 0x62, 0x12,
	0xab, 0x80, 0xe5, 0xb0, 0xd3, 0x51, 0xfb, 0xb1
};
const uint8_t nmagic_email[] = {
	0x52, 0xea, 0xfb, 0x7a, 0x84, 0xe9, 0x5c, 0x1d,
	0xbd, 0xb0, 0xff, 0xef, 0x1a, 0xa5, 0xc8, 0xd1,
	0xaa, 0xb8, 0x15, 0x8b, 0x52, 0x32, 0x93, 0x4f,
	0x15, 0x4a, 0x7c, 0xff, 0xee, 0x29, 0xb9, 0x23 
};
const uint8_t nmagic_alarm[] = { 
	0xb7, 0x69, 0x18, 0x67, 0x79, 0x9c, 0x11, 0xd5, 
	0xb8, 0x37, 0xf8, 0xa5, 0xe8, 0x6e, 0x81, 0xc8, 
	0xe6, 0xd2, 0xbb, 0xcc, 0x62, 0x4f, 0x15, 0x7a, 
	0xc4, 0xf0, 0x3d, 0x5d, 0x37, 0x01, 0xe1, 0x1e 
};

// Various prefixes used by Bosch.
#define NEFITEASY_HOST_URL "wa2-mz36-qrmzh6.bosch.de"
#define NEFITEASY_HOST_PORT 5222
#define NEFITEASY_USER_AGENT "User-Agent: NefitEasy"
#define NEFITEASY_ACCESSKEY_PREFIX "Ct7ZR03b_"
#define NEFITEASY_RRCCONTACT_PREFIX "rrccontact_"
#define NEFITEASY_RRCGATEWAY_PREFIX "rrcgateway_"

CNefitEasy::CNefitEasy(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	m_bDoLogin = true;
	m_lastgasusage = 0;
	m_bClockMode = false;
/*
	// Generate some commonly used properties.
	m_ConnectionPassword = NEFITEASY_ACCESSKEY_PREFIX + m_AccessKey;
	std::string suffix = m_SerialNumber + "@" + NEFITEASY_HOST_URL;
	m_jid = NEFITEASY_RRCCONTACT_PREFIX + suffix;
	m_from = m_jid;
	m_to = NEFITEASY_RRCGATEWAY_PREFIX + suffix;

	if (ConnectToXMPP(NEFITEASY_HOST_URL, NEFITEASY_HOST_PORT))
	{
	}
*/
	Init();
}

CNefitEasy::~CNefitEasy(void)
{
	Logout();
}

bool CNefitEasy::ConnectToXMPP(const std::string &IPAddress, const int PortNumber)
{
	return false;
}

void CNefitEasy::Logout()
{
	m_bDoLogin = true;
}

void CNefitEasy::Init()
{
	m_lastgasusage = 0;
	m_bClockMode = false;
}

bool CNefitEasy::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CNefitEasy::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CNefitEasy::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);	//#define DEBUG_NefitEasyW
	//#define DEBUG_NefitEasyR

		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}


#define NEFIT_FAST_POLL_INTERVAL 30
#define NEFIT_SLOW_INTERVAL 300
#define NEFIT_GAS_INTERVAL 400

void CNefitEasy::Do_Work()
{
	int sec_counter = 0;
	bool bFirstTime = true;
	bool ret = true;

	int fast_pollint = NEFIT_FAST_POLL_INTERVAL;
	int slow_pollint = NEFIT_SLOW_INTERVAL;

	_log.Log(LOG_STATUS, "NefitEasy: Worker started...");

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if ((sec_counter % fast_pollint == 0) || (bFirstTime))
		{
			try
			{
				ret = GetStatusDetails();
				if (ret)
					ret = GetFlowTemp();
				if (ret)
					ret = GetDisplayCode();
				fast_pollint = (ret == true) ? NEFIT_FAST_POLL_INTERVAL : NEFIT_FAST_POLL_INTERVAL * 3;
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error getting/processing status result...");
			}
		}
		if ((sec_counter % slow_pollint == 0) || (bFirstTime))
		{
			try
			{
				ret = GetPressure();
				if (ret)
					ret = GetOutdoorTemp();
				slow_pollint = (ret == true) ? NEFIT_SLOW_INTERVAL : NEFIT_SLOW_INTERVAL * 2;
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error getting/processing pressure result...");
			}
		}
		if (sec_counter % NEFIT_GAS_INTERVAL == 0)
		{
			try
			{
				ret = GetGasUsage();
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error getting/processing gas result...");
			}
		}
		bFirstTime = false;
	}
	_log.Log(LOG_STATUS, "NefitEasy: Worker stopped...");
}

bool CNefitEasy::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		//Light command
		int node_id = pCmd->LIGHTING2.id4;
		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		if (node_id == 1)
		{
			//User Mode Switch Clock/Manual
			SetUserMode(bIsOn);
			return true;
		}
		else if (node_id == 2)
		{
			//Hot Water Switch
			SetHotWaterMode(bIsOn);
			return true;
		}
	}
	return false;
}

std::string CNefitEasy::CmdUrl(const char* szUrlPath)
{
		std::stringstream szURL;
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << szUrlPath;
		return szURL.str();
}

bool CNefitEasy::HttpPOST(const char* szUrlPath, const Json::Value& jValue, const char* szErrorMessage)
{
	LogError error(szUrlPath);
	
	try 
	{
		std::vector<std::string> vExtraHeaders;
		//ExtraHeaders.push_back("User-Agent: NefitEasy");
		vExtraHeaders.push_back("Content-Type: application/json");
		std::string sResponse;

		if (!HTTPClient::POST(CmdUrl(szUrlPath), jValue.toStyledString(), vExtraHeaders, sResponse))
		{
			return error(szErrorMessage);
		}
	}
	catch(...)
	{
		return error(szErrorMessage);
	}
	
	return error.None();
}

bool CNefitEasy::HttpGET(const char* szUrlPath, const char* szDebugFilePath, Json::Value& jValue)
{
	LogError error(szUrlPath);
	
	try 
	{
		std::string sResponse;
#ifdef DEBUG_NefitEasyR
		sResponse = ReadFile(szDebugFilePath);
#else
		if(!HTTPClient::GET(CmdUrl(szUrlPath), sResponse))
		{
			return error("Error GETting http data");
		}
#endif
		Json::Reader jReader;
		if(!jReader.parse(sResponse, jValue))
		{
			if (sResponse.find("Error: REQUEST_TIMEOUT") != std::string::npos)
			{
				return error("Request timeout!");
			}
			else
			{
				return error("Invalid json data received, unable to parse");
			}
		}
		
		// This is the kicker... It can happen that you receive the response to another
		// request (e.g. receive gas usage response to a get status details request).
		// Why this happens I don't know, but we can safeguard against it because each
		// respons contains the type of request it is for in the 'id' key. 
		if(jValue["id"].empty())
		{
			return error("Invalid json response received: no 'id' key found!");
		}
		
		std::string sId(jValue["id"].asString());
		if(sId != szUrlPath)
		{
			return error("Invalid response received: %s requested, %s received", szUrlPath, sId.c_str());
		}
		
		if(jValue["value"].empty())
		{
			return error("Invalid data received: no 'value' key in response");
		}
		
#ifdef DEBUG_NefitEasyW
		SaveString2Disk(jValue.toStyledString(), szDebugFilePath);
#endif

		return error.None();
	}
	catch(...)	
	{
		return error("Error GETting http data");
	}
}

bool CNefitEasy::HttpGET(const char* szUrlPath, const char* szDebugFilePath, float& fResult)
{
	Json::Value jValue;
	bool bResult = HttpGET(szUrlPath, szDebugFilePath, jValue);
	if(bResult) 
	{
		fResult = jValue["value"].asFloat();
	}
	return bResult;
}

bool CNefitEasy::HttpGET(const char* szUrlPath, const char* szDebugFilePath,  std::string& sResult)
{
	Json::Value jValue;
	bool bResult = HttpGET(szUrlPath, szDebugFilePath, jValue);
	if(bResult) 
	{
		sResult = jValue["value"].asString();
	}
	return bResult;
}

void CNefitEasy::SetUserMode(bool bSetUserModeClock)
{
	if(HttpPOST(NEFITEASY_SET_USER_MODE, JsonValue(bSetUserModeClock?"clock":"manual"), "Error setting User Mode"))
	{
		GetStatusDetails();
	}
}

void CNefitEasy::SetHotWaterMode(bool bTurnOn)
{
	Json::Value jValue = JsonValue(bTurnOn ? "on" : "off");
	const char* szErrorMessage = "Error setting user mode";
	if(HttpPOST(NEFITEASY_SET_HOT_WATER_CLOCK_MODE, jValue, szErrorMessage)
	&& HttpPOST(NEFITEASY_SET_HOT_WATER_MANUAL_MODE, jValue, szErrorMessage))
	{
		GetStatusDetails();
	}
}

bool CNefitEasy::GetStatusDetails()
{
	Json::Value jValue;
	if(!HttpGET(NEFITEASY_STATUS_URL, "E:\\nefit_uistatus.json", jValue))
	{
		return false;
	}

	Json::Value root2 = jValue["value"];
	if (root2["TOT"].empty())
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (main)");
		return false;
	}
	/*
	Status output:
	ARS -> '?' string
	BAI -> 'boiler indicator' ('CH' = 'central heating', 'HW' = 'hot water', 'No' = 'off' ) string
	BBE -> 'boiler block' bool
	BLE -> 'boiler lock' bool
	BMR -> 'boiler maintainance' bool
	CPM -> 'clock program' string (auto)
	CSP -> 'current switchpoint' string float
	CTD -> 'current time/date' string
	CTR -> 'control' string (room)
	DAS -> '?' on/off
	DHW -> 'Domestic Hot Water' on/off
	ESI -> 'powersave active' on/off
	FPA -> 'fireplace mode active, continue heating even when maximum setpoint temperature reached' string
	HED_DB -> '?'
	HED_DEV -> 'hed device at home' bool
	HED_EN -> 'hed enabled' bool
	HMD -> 'holiday mode' on/off
	IHS -> 'in house status' string (ok)
	IHT -> 'in house temp' string float
	MMT -> 'temp manual setpoint' string float
	PMR -> '?' bool
	RS -> '?' on/off
	TAS -> '?' on/off
	TOD -> 'temp override duration' string integer?
	TOR -> 'temp override' on/off
	TOT -> 'temp override temp setpoint' string float
	TSP -> 'temp setpoint' string float
	UMD -> 'user mode' string (clock)
	*/
	std::string tmpstr;
	if (!root2["TSP"].empty())
	{
		tmpstr = root2["TSP"].asString();
		if (tmpstr != "null")
		{
			float temp = static_cast<float>(atof(tmpstr.c_str()));
			SendSetPointSensor(1, 1, 1, temp, "Setpoint");
		}
	}
	if (!root2["IHT"].empty())
	{
		tmpstr = root2["IHT"].asString();
		if (tmpstr != "null")
		{
			float temp = static_cast<float>(atof(tmpstr.c_str()));
			SendTempSensor(1, -1, temp, "Room Temperature");
		}
	}
	if (!root2["BAI"].empty())
	{
		tmpstr = root2["BAI"].asString();
		if (tmpstr != "null")
		{
			std::string bstatus = "";
			if (tmpstr == "CH")
				bstatus = "central heating";
			else if (tmpstr == "HW")
				bstatus = "hot water";
			else if (tmpstr == "No")
				bstatus = "off";
			if (!bstatus.empty())
			{
				if (m_LastBoilerStatus != bstatus)
				{
					m_LastBoilerStatus = bstatus;
					SendTextSensor(1, 2, -1, bstatus, "Boiler Status");
				}
			}
		}
	}
	if (!root2["UMD"].empty())
	{
		tmpstr = root2["UMD"].asString();
		m_bClockMode = (tmpstr == "clock");
		SendSwitch(1, 1, -1, m_bClockMode, 0, "Clock Mode");
	}
	if (!root2["DHW"].empty())
	{
		tmpstr = root2["DHW"].asString();
		bool bIsOn = (tmpstr != "off");
		SendSwitch(2, 1, -1, bIsOn, 0, "Hot Water");
	}

	return true;
}

bool CNefitEasy::GetOutdoorTemp()
{
	float fTemp;
	if(!HttpGET(NEFITEASY_OUTDOORTEMP_URL, "E:\\nefit_outdoor.json", fTemp))
	{
		return false;
	}
	
	SendTempSensor(2, -1, fTemp, "Outside Temperature");
	return true;
}

bool CNefitEasy::GetFlowTemp()
{
	float fTemp;
	if(!HttpGET(NEFITEASY_FLOWTEMP_URL, "E:\\nefit_flow.json", fTemp))
	{
		return false;
	}
	
	SendTempSensor(3, -1, fTemp, "Flow Temperature");
	return true;
}

bool CNefitEasy::GetPressure()
{
	float fPressure;
	if(!HttpGET(NEFITEASY_PRESSURE_URL, "E:\\nefit_pressure.json", fPressure))
	{
		return false;
	}
	
	SendPressureSensor(1, 1, -1, fPressure, "Pressure");
	return true;
}

bool CNefitEasy::GetDisplayCode()
{
	std::string sDisplayCode;
	if(!HttpGET(NEFITEASY_DISPLAYCODE_URL, "E:\\nefit_displaycode.json", sDisplayCode))
	{
		return false;
	}
	std::string display_code = "Unknown";
	if (sDisplayCode == "-H")
		display_code = "central heating active";
	else if (sDisplayCode == "=H")
		display_code = "hot water active";
	else if (sDisplayCode == "0C")
		display_code = "system starting";
	else if (sDisplayCode == "0L")
		display_code = "system starting";
	else if (sDisplayCode == "0U")
		display_code = "system starting";
	else if (sDisplayCode == "0E")
		display_code = "system waiting";
	else if (sDisplayCode == "0H")
		display_code = "system standby";
	else if (sDisplayCode == "0A")
		display_code = "system waiting (boiler cannot transfer heat to central heating)";
	else if (sDisplayCode == "0Y")
		display_code = "system waiting (boiler cannot transfer heat to central heating)";
	//else if (sDisplayCode == "0E")
	//display_code = "system waiting (boiler cannot transfer heat to central heating)";
	else if (sDisplayCode == "2E")
		display_code = "boiler water pressure too low";
	else if (sDisplayCode == "H07")
		display_code = "boiler water pressure too low";
	else if (sDisplayCode == "2F")
		display_code = "sensors measured abnormal temperature";
	else if (sDisplayCode == "2L")
		display_code = "sensors measured abnormal temperature";
	else if (sDisplayCode == "2P")
		display_code = "sensors measured abnormal temperature";
	else if (sDisplayCode == "2U")
		display_code = "sensors measured abnormal temperature";
	else if (sDisplayCode == "4F")
		display_code = "sensors measured abnormal temperature";
	else if (sDisplayCode == "4L")
		display_code = "sensors measured abnormal temperature";
	else if (sDisplayCode == "6A")
		display_code = "burner doesn't ignite";
	else if (sDisplayCode == "6C")
		display_code = "burner doesn't ignite";
	else if (sDisplayCode == "rE")
		display_code = "system restarting";

	if (m_LastDisplayCode != display_code)
	{
		m_LastDisplayCode = display_code;
		SendTextSensor(1, 1, -1, display_code, "Display Code");
	}
	return true;
}

bool CNefitEasy::GetGasUsage()
{
	float fYearGas;
	if(!HttpGET(NEFITEASY_GAS_URL, "E:\\nefit_yearTotal.json", fYearGas))
	{
		return false;
	}

	//convert from kWh to m3
	fYearGas *= (0.12307692f*1000.0f);
	
	m_p1gas.gasusage = (uint32_t)fYearGas;
	sDecodeRXMessage(this, (const unsigned char *)&m_p1gas, "Gas", 255);
	return true;
}

void CNefitEasy::SetSetpoint(const int idx, const float temp)
{
	const char* szErrorMessage = "Error setting setpoint";
	if(!HttpPOST(NEFITEASY_SET_TEMP_ROOM, JsonValue(temp), szErrorMessage))
	{
		return;
	}
	
	if(temp)
	{
		if(!HttpPOST(NEFITEASY_SET_TEMP_OVERRIDE, JsonValue("on"), szErrorMessage))
		{
			return;
		}
		
		if(!HttpPOST(NEFITEASY_SET_TEMP_OVERRIDE_TEMP, JsonValue(temp), szErrorMessage))
		{
			return;
		}
	}
	else 
	{
		if(!HttpPOST(NEFITEASY_SET_TEMP_OVERRIDE, JsonValue("off"), szErrorMessage))
		{
			return;
		}
	}
	
	GetStatusDetails();
}
