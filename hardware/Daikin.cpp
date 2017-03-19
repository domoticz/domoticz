#include "stdafx.h"
#include "Daikin.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

#define Daikin_POLL_INTERVAL 300

#ifdef _DEBUG
	//#define DEBUG_DaikinR
	//#define DEBUG_DaikinW
#endif

#ifdef DEBUG_DaikinW
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
#ifdef DEBUG_DaikinR
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

CDaikin::CDaikin(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
m_szIPAddress(IPAddress),
m_Username(CURLEncode::URLEncode(username)),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	m_bOutputLog = false;
	Init();
}

CDaikin::~CDaikin(void)
{
}

void CDaikin::Init()
{
}

bool CDaikin::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDaikin::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "Daikin: Started");
	return (m_thread!=NULL);
}

bool CDaikin::StopHardware()
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

void CDaikin::Do_Work()
{
	int sec_counter = Daikin_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (sec_counter % Daikin_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"Daikin: Worker stopped...");
}

bool CDaikin::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void CDaikin::SetSetpoint(const int idx, const float temp)
{
/*
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "api.php";

	//{pow: "0", mode: "2", stemp: "M", shum: "0", f_rate: "A", f_dir: "0"}

	var temp = minimize_opt(control_response);
	temp.stemp = (parseInt(control_response.stemp) + inc).toString();
	send_control(temp);

	std::string sPostData;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	if (!HTTPClient::POST(szURL.str(), sPostData, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error setting current state!");
		return;
	}

#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\Daikin_set_setpoint.txt");
#endif
	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}
*/
}

void CDaikin::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, SubUnit);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
		{
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel == level)
				return;
		}
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
	lcmd.LIGHTING2.unitcode = SubUnit;
	if (!bOn)
	{
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

void CDaikin::GetMeterDetails()
{
	GetControlInfo();
	GetSensorInfo();
}

void CDaikin::GetControlInfo()
{
	std::string sResult;
#ifdef DEBUG_DaikinR
	sResult = ReadFile("E:\\Daikin_get_control_info.txt");
#else
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/aircon/get_control_info";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\Daikin_get_control_info.txt");
#endif
#endif
	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 8)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid data received");
		return;
	}
	std::vector<std::string>::const_iterator itt;
	for (itt = results.begin(); itt != results.end(); ++itt)
	{
		std::string sVar = *itt;
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "mode")
		{
			//0-1-7 auto
			//2 dehum
			//3 cooling
			//4 heating
			//6 fan
		}
		else if (results2[0] == "stemp")
		{
			//Target Temperature
			SendSetPointSensor(1, 1, 1, static_cast<float>(atof(results2[1].c_str())), "Target Temperature");
		}

	}
	//UpdateSwitch(1, Idx, (lValue == 1) ? true : false, 100, sstr.str());

}

void CDaikin::GetSensorInfo()
{
	std::string sResult;
#ifdef DEBUG_DaikinR
	sResult = ReadFile("E:\\Daikin_get_sensor_info.txt");
#else
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/aircon/get_sensor_info";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\Daikin_get_sensor_info.txt");
#endif
#endif
	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 6)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid data received");
		return;
	}
	float htemp = -1;
	int hhum = -1;
	std::vector<std::string>::const_iterator itt;
	for (itt = results.begin(); itt != results.end(); ++itt)
	{
		std::string sVar = *itt;
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "htemp")
		{
			htemp = static_cast<float>(atof(results2[1].c_str()));
		}
		else if (results2[0] == "hhum")
		{
			if (results2[1]!="-")
				hhum = static_cast<int>(atoi(results2[1].c_str()));
		}
		else if (results2[0] == "otemp")
		{
			SendTempSensor(11, -1, static_cast<float>(atof(results2[1].c_str())), "Outside Temperature");
		}
		if (htemp != -1)
		{
			if (hhum != -1)
				SendTempHumSensor(10, -1, htemp, hhum, "Home Temp+Hum");
			else
				SendTempSensor(10, -1, htemp, "Home Temperature");
		}
	}
}

