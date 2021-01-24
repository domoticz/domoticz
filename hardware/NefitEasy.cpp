#include "stdafx.h"
#include "NefitEasy.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
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

bool CheckId(const char* szUrlPath, const Json::Value& response)
{
	return (!response["id"].empty() && response["id"].asString() == szUrlPath);
}

CNefitEasy::CNefitEasy(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_lastgasusage = 0;
	m_bClockMode = false;

	Init();
}

void CNefitEasy::Init()
{
	m_lastgasusage = 0;
	m_bClockMode = false;
}

bool CNefitEasy::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CNefitEasy::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CNefitEasy::StopHardware()
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

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
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

bool CNefitEasy::WriteToHardware(const char *pdata, const unsigned char /*length*/)
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
		if (node_id == 2)
		{
			//Hot Water Switch
			SetHotWaterMode(bIsOn);
			return true;
		}
	}
	return false;
}

void CNefitEasy::SetUserMode(bool bSetUserModeClock)
{
	Json::Value root;
	root["value"] = (bSetUserModeClock == true) ? "clock" : "manual";

	std::stringstream szURL;
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	//ExtraHeaders.push_back("User-Agent: NefitEasy");
	ExtraHeaders.push_back("Content-Type: application/json");

	try
	{

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_USER_MODE;
		if (!HTTPClient::POST(szURL.str(), root.toStyledString(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error setting User Mode!");
			return;
		}
		GetStatusDetails();
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error setting User Mode!");
		return;
	}
}

void CNefitEasy::SetHotWaterMode(bool bTurnOn)
{
	Json::Value root;
	root["value"] = (bTurnOn == true) ? "on" : "off";

	std::stringstream szURL;
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	//ExtraHeaders.push_back("User-Agent: NefitEasy");
	ExtraHeaders.push_back("Content-Type: application/json");

	try
	{
		//szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << (m_bClockMode == true) ? NEFITEASY_SET_HOT_WATER_CLOCK_MODE : NEFITEASY_SET_HOT_WATER_MANUAL_MODE;
		//Set Both modes
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_HOT_WATER_CLOCK_MODE;
		if (!HTTPClient::POST(szURL.str(), root.toStyledString(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error setting User Mode!");
			return;
		}
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_HOT_WATER_MANUAL_MODE;
		if (!HTTPClient::POST(szURL.str(), root.toStyledString(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error setting User Mode!");
			return;
		}
		GetStatusDetails();
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error setting User Mode!");
		return;
	}
}

bool CNefitEasy::GetStatusDetails()
{
	std::string sResult;

	//Status
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_uistatus.json");
#else
	std::stringstream szURL;
	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_STATUS_URL;
	try
	{
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
		return false;
	}
#endif

#ifdef DEBUG_NefitEasyW
	SaveString2Disk(sResult, "E:\\nefit_uistatus_prop.json");
#endif
	Json::Value root;
	Json::Value root2;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		if (sResult.find("Error: REQUEST_TIMEOUT") != std::string::npos)
			_log.Log(LOG_ERROR, "NefitEasy: Request Timeout !");
		else
			_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (main)!");
		return false;
	}
	if(!CheckId(NEFITEASY_STATUS_URL, root))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid response received (main)");
		return false;
	}

	if (root["value"].empty())
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (main)");
		return false;
	}
	root2 = root["value"];
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
			std::string bstatus;
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
		SendSwitch(1, 1, -1, m_bClockMode, 0, "Clock Mode", m_Name);
	}
	if (!root2["DHW"].empty())
	{
		tmpstr = root2["DHW"].asString();
		bool bIsOn = (tmpstr != "off");
		SendSwitch(2, 1, -1, bIsOn, 0, "Hot Water", m_Name);
	}

	return true;
}

bool CNefitEasy::GetOutdoorTemp()
{
	std::string sResult;
	Json::Value root;
	bool ret;

	//Outdoor Temperature
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_outdoor.json");
#else
	std::stringstream szURL2;
	szURL2 << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_OUTDOORTEMP_URL;
	try
	{
		if (!HTTPClient::GET(szURL2.str(), sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
		return false;
	}
#endif

#ifdef DEBUG_NefitEasyW
	SaveString2Disk(sResult, "E:\\nefit_outdoor.json");
#endif
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (ODT)");
		return false;
	}
	if(!CheckId(NEFITEASY_OUTDOORTEMP_URL, root))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid response received! (ODT)");
		return false;
	}
	if (root["value"].empty() == true)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (ODT)");
		return false;
	}

	float temp = root["value"].asFloat();
	SendTempSensor(2, -1, temp, "Outside Temperature");
	return true;
}

bool CNefitEasy::GetFlowTemp()
{
	std::string sResult;
	Json::Value root;
	bool ret;

	//Flow Temperature
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_flow.json");
#else
	std::stringstream szURL2;
	szURL2 << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_FLOWTEMP_URL;
	try
	{
		if (!HTTPClient::GET(szURL2.str(), sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
		return false;
	}
#endif

#ifdef DEBUG_NefitEasyW
	SaveString2Disk(sResult, "E:\\nefit_flow.json");
#endif
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (FT)");
		return false;
	}
	if(!CheckId(NEFITEASY_FLOWTEMP_URL, root))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid response received (FT)");
		return false;
	}
	if (root["value"].empty() == true)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (FT)");
		return false;
	}

	float temp = root["value"].asFloat();
	SendTempSensor(3, -1, temp, "Flow Temperature");
	return true;
}

bool CNefitEasy::GetPressure()
{
	std::string sResult;
	Json::Value root;
	bool ret;

	//Status
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_pressure.json");
#else
	std::stringstream szURL;
	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_PRESSURE_URL;
	try
	{
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
		return false;
	}
#endif

#ifdef DEBUG_NefitEasyW
	SaveString2Disk(sResult, "E:\\nefit_uipres.json");
#endif
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (Press)");
		return false;
	}
	if(!CheckId(NEFITEASY_PRESSURE_URL, root))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid response received! (Press)");
		return false;
	}
	if (root["value"].empty())
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (Press)");
		return false;
	}
	float pressure = root["value"].asFloat();
	SendPressureSensor(1, 1, -1, pressure, "Pressure");
	return true;
}

bool CNefitEasy::GetDisplayCode()
{
	std::string sResult;
	Json::Value root;
	bool ret;

#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_displaycode.json");
#else
	std::stringstream szURL3;
	szURL3 << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_DISPLAYCODE_URL;
	try
	{
		if (!HTTPClient::GET(szURL3.str(), sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
		return false;
	}
#endif

#ifdef DEBUG_NefitEasyW
	SaveString2Disk(sResult, "E:\\nefit_displaycode.json");
#endif
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (DP)");
		return false;
	}
	if(!CheckId(NEFITEASY_DISPLAYCODE_URL, root))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid response received! (DP)");
		return false;
	}
	if (root["value"].empty() == true)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (DP)");
		return false;
	}

	std::string dcode = root["value"].asString();
	std::string display_code = "Unknown";
	if (dcode == "-H")
		display_code = "central heating active";
	else if (dcode == "=H")
		display_code = "hot water active";
	else if (dcode == "0C")
		display_code = "system starting";
	else if (dcode == "0L")
		display_code = "system starting";
	else if (dcode == "0U")
		display_code = "system starting";
	else if (dcode == "0E")
		display_code = "system waiting";
	else if (dcode == "0H")
		display_code = "system standby";
	else if (dcode == "0A")
		display_code = "system waiting (boiler cannot transfer heat to central heating)";
	else if (dcode == "0Y")
		display_code = "system waiting (boiler cannot transfer heat to central heating)";
	//else if (dcode == "0E")
	//display_code = "system waiting (boiler cannot transfer heat to central heating)";
	else if (dcode == "2E")
		display_code = "boiler water pressure too low";
	else if (dcode == "H07")
		display_code = "boiler water pressure too low";
	else if (dcode == "2F")
		display_code = "sensors measured abnormal temperature";
	else if (dcode == "2L")
		display_code = "sensors measured abnormal temperature";
	else if (dcode == "2P")
		display_code = "sensors measured abnormal temperature";
	else if (dcode == "2U")
		display_code = "sensors measured abnormal temperature";
	else if (dcode == "4F")
		display_code = "sensors measured abnormal temperature";
	else if (dcode == "4L")
		display_code = "sensors measured abnormal temperature";
	else if (dcode == "6A")
		display_code = "burner doesn't ignite";
	else if (dcode == "6C")
		display_code = "burner doesn't ignite";
	else if (dcode == "rE")
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
	std::string sResult;
	Json::Value root;
	bool ret;

	//Status
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_yearTotal.json");
#else
	std::stringstream szURL;
	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_GAS_URL;
	try
	{
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error getting http data!");
		return false;
	}
#endif

#ifdef DEBUG_NefitEasyW
	SaveString2Disk(sResult, "E:\\nefit_yearTotal.json");
#endif
	ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (Gas)");
		return false;
	}
	if(!CheckId(NEFITEASY_GAS_URL, root))
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid response received! (Gas)");
		return false;
	}
	if (root["value"].empty())
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (Gas)");
		return false;
	}
	float yeargas = root["value"].asFloat();
	//convert from kWh to m3
	yeargas *= (0.12307692F * 1000.0F);
	uint32_t gusage = (uint32_t)yeargas;
	if (gusage < m_lastgasusage)
	{

	}
	m_p1gas.gasusage = gusage;
	sDecodeRXMessage(this, (const unsigned char *)&m_p1gas, "Gas", 255, nullptr);
	return true;
}

void CNefitEasy::SetSetpoint(const int /*idx*/, const float temp)
{
	Json::Value root;
	root["value"] = temp;
	std::stringstream szURL;
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	//ExtraHeaders.push_back("User-Agent: NefitEasy");
	ExtraHeaders.push_back("Content-Type: application/json");

	try
	{

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_TEMP_ROOM;
		if (!HTTPClient::POST(szURL.str(), root.toStyledString(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "NefitEasy: Error setting setpoint!");
			return;
		}
		szURL.clear();
		szURL.str("");
		if (temp != 0)
		{
			Json::Value root2;
			root2["value"] = "on";
			szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_TEMP_OVERRIDE;
			if (!HTTPClient::POST(szURL.str(), root2.toStyledString(), ExtraHeaders, sResult))
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error setting setpoint!");
				return;
			}
			szURL.clear();
			szURL.str("");
			szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_TEMP_OVERRIDE_TEMP;
			if (!HTTPClient::POST(szURL.str(), root.toStyledString(), ExtraHeaders, sResult))
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error setting setpoint!");
				return;
			}
		}
		else
		{
			Json::Value root2;
			root2["value"] = "off";
			szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_SET_TEMP_OVERRIDE;
			if (!HTTPClient::POST(szURL.str(), root2.toStyledString(), ExtraHeaders, sResult))
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error setting setpoint!");
				return;
			}
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Error setting Setpoint!");
		return;
	}
	GetStatusDetails();
}
