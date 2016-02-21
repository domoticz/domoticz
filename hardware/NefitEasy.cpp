#include "stdafx.h"
#include "NefitEasy.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"

/*
We need HTTP server to access Nefit/Bosch XMPP backend over HTTP written by Robert Klep (https://github.com/robertklep/nefit-easy-http-server)

Install:
sudo npm un -g nefit-easy-http-server
sudo npm i -g nefit-easy-http-server

Start: (should be done automatically)

easy-server --host=0.0.0.0 --serial=XXXX --access-key=XXXX --password=XXXX

After this you should be able to connect to port 3000
*/

//#define DEBUG_NefitEasyR
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
#define NEFITEASY_OUTDOOR_URL "/system/sensors/temperatures/outdoor_t1"
#define NEFITEASY_PRESSURE_URL "/system/appliance/systemPressure"
#define NEFITEASY_DISPLAYCODE_URL "/system/appliance/displaycode"
#define NEFITEASY_CAUSECODE_URL "/system/appliance/causecode"
#define NEFITEASY_SET_TEMP_ROOM "/heatingCircuits/hc1/temperatureRoomManual"
#define NEFITEASY_SET_TEMP_OVERRIDE "/heatingCircuits/hc1/manualTempOverride/status"
#define NEFITEASY_SET_TEMP_OVERRIDE_TEMP "/heatingCircuits/hc1/manualTempOverride/temperature"
#define NEFITEASY_LOCATION_LATITUDE "/system/location/latitude"
#define NEFITEASY_LOCATION_LONGITUDE "/system/location/longitude"

CNefitEasy::CNefitEasy(const int ID, const std::string &IPAddress, const unsigned short usIPPort):
m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	Init();
}

CNefitEasy::~CNefitEasy(void)
{
}

void CNefitEasy::Init()
{
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
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

#define NEFIT_STATUS_POLL_INTERVAL 30
#define NEFIT_PRESSURE_POLL_INTERVAL 300

void CNefitEasy::Do_Work()
{
	int sec_counter = 0;
	bool bFirstTime = true;
	int nstat_pollint = NEFIT_STATUS_POLL_INTERVAL;
	int npres_pollint = NEFIT_PRESSURE_POLL_INTERVAL;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if ((sec_counter % nstat_pollint == 0) || (bFirstTime))
		{
			try
			{
				nstat_pollint = (GetStatusDetails() == true) ? NEFIT_STATUS_POLL_INTERVAL : NEFIT_STATUS_POLL_INTERVAL * 3;
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error getting/processing status result...");
			}
		}
		if ((sec_counter % npres_pollint == 0) || (bFirstTime))
		{
			try
			{
				nstat_pollint = NEFIT_STATUS_POLL_INTERVAL;
				npres_pollint = (GetPressureDetails() == true) ? NEFIT_PRESSURE_POLL_INTERVAL : NEFIT_PRESSURE_POLL_INTERVAL * 2;
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "NefitEasy: Error getting/processing pressure result...");
			}
		}
		bFirstTime = false;
	}
	_log.Log(LOG_STATUS, "NefitEasy Worker stopped...");
}

bool CNefitEasy::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
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
		bool bret;
		bret = HTTPClient::GET(szURL.str(), sResult);
		if (!bret)
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
	SaveString2Disk(sResult, "E:\\nefit_uistatus.json");
#endif
	Json::Value root;
	Json::Value root2;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (main)!");
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
DHW -> '?Domestic Hot Water' on/off
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

	//Outdoor Temperature
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_outdoor.json");
#else
	std::stringstream szURL2;
	szURL2 << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_OUTDOOR_URL;
	try
	{
		bool bret;
		bret = HTTPClient::GET(szURL2.str(), sResult);
		if (!bret)
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
	ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (ODT)");
		return false;
	}
	if (root["value"].empty() == true)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received (ODT)");
		return false;
	}

	float temp = root["value"].asFloat();
	SendTempSensor(2, -1, temp, "Outside Temperature");

	//Display Code
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_displaycode.json");
#else
	std::stringstream szURL3;
	szURL3 << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_DISPLAYCODE_URL;
	try
	{
		bool bret;
		bret = HTTPClient::GET(szURL3.str(), sResult);
		if (!bret)
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
	ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (DP)");
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

bool CNefitEasy::GetPressureDetails()
{
	std::string sResult;

	//Status
#ifdef DEBUG_NefitEasyR
	sResult = ReadFile("E:\\nefit_pressure.json");
#else
	std::stringstream szURL;
	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << NEFITEASY_HTTP_BRIDGE << NEFITEASY_PRESSURE_URL;
	try
	{
		bool bret;
		bret = HTTPClient::GET(szURL.str(), sResult);
		if (!bret)
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
	SaveString2Disk(sResult, "E:\\nefit_uistatus.json");
#endif
	Json::Value root;
	Json::Value root2;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "NefitEasy: Invalid data received! (Press)");
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

void CNefitEasy::SetSetpoint(const int idx, const float temp)
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
