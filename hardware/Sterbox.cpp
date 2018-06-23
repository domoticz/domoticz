#include "stdafx.h"
#include "Sterbox.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

//http://sterbox.eu/index.php/sterbox

#define STERBOX_POLL_INTERVAL 8

CSterbox::CSterbox(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
m_szIPAddress(IPAddress),
m_Username(username),
m_Password(password)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	m_bOutputLog = false;
	Init();
}

CSterbox::~CSterbox(void)
{
}

void CSterbox::Init()
{
}

bool CSterbox::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSterbox::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "Sterbox: Started");
	return (m_thread!=NULL);
}

bool CSterbox::StopHardware()
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

void CSterbox::Do_Work()
{
	int sec_counter = STERBOX_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (sec_counter % STERBOX_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"Sterbox: Worker stopped...");
}

bool CSterbox::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if (packettype == pTypeLighting2)
	{
		//light command

		int Relay = pSen->LIGHTING2.unitcode;
		if (Relay > 20)
			return false;

		std::stringstream szURL;
		//std::stringstream szhex;
		//szhex << std::hex << Relay;
		//std::string strhex = szhex.str();
		//_log.Log(LOG_ERROR, "Sterbox: Size od szhex: %s", strhex.c_str());


		//if (m_Password.empty())
		//{
			szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
		//}
		//else
		//{
		//	szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
		//}

		szURL << "/x?x00" << std::uppercase << std::hex << Relay;

		if (pSen->LIGHTING2.cmnd == light2_sOff)
		{
			szURL << "=0";
		}
		else
		{
			szURL << "=0";
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			_log.Log(LOG_ERROR, "Sterbox: Protocol Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		std::string strhex = szURL.str();
		_log.Log(LOG_STATUS, "Sterbox: sending relay command to: %s", strhex.c_str());

		return true;
	}
	return false;
}

void CSterbox::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
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

void CSterbox::GetMeterDetails()
{
	std::string sResult;
	std::stringstream szURL;

	//if(m_Password.empty())
	//{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	//}
	//else
	//{
	//	szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	//}

	szURL << "/x.cgi"; 

	if (!HTTPClient::GET(szURL.str(),sResult))
	{
		_log.Log(LOG_ERROR,"Sterbox: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	std::vector<std::string> results;
	std::vector<std::string> outputs;
	std::vector<std::string> inputs;
	std::vector<std::string> analog;
	StringSplit(sResult, "<br>", results);
	if (results.size()<8)
	{
		_log.Log(LOG_ERROR,"Sterbox: Result 8 Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	//if (results[0] != "<body>")
	std::string tmpstr;
	std::string tmpstr2;
	std::string tmpinp;
	tmpstr = results[0];
	size_t ii;
	size_t jj;
	int pos1;

	if (m_Username.empty())
	{
		StringSplit("o,o,o,o,i,i,i,i", ",", inputs);
	}
	else
	{
		StringSplit(m_Username, ",", inputs);
		//_log.Log(LOG_ERROR,"Sterbox: Username : %s , IP: %s", m_Username.c_str(), m_szIPAddress.c_str());
	}

	for (ii = 1; ii < results.size(); ii++)
	{
		tmpstr = results[ii];
		if (tmpstr.find("OU") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("OU"));
			pos1 = tmpstr.find("=");
			if (pos1 != std::string::npos)
			{	
				tmpstr = tmpstr.substr(pos1+1);
				//_log.Log(LOG_ERROR,"Sterbox: OU Status: %s", tmpstr.c_str());
				StringSplit(tmpstr, ",", outputs);
				for (jj = 0; jj < inputs.size(); jj++) 
				{	
					tmpinp = inputs[jj];
					//if (( jj < 4 || jj > 7  ))
					pos1 = tmpinp.find("o");
					if (pos1 != std::string::npos)
					{
					int lValue = 0;
					//tmpstr = tmpstr.substr(pos1+1);
					tmpstr2 = outputs[jj];
					//_log.Log(LOG_ERROR,"Sterbox: OU Status: %s", tmpstr2.c_str());
					pos1 = tmpstr2.find("s");
					if (pos1 != std::string::npos)
					{
						lValue = 1;
					}
					else
					{
						lValue = 0;
					}
					std::stringstream sstr;
					sstr << "Relay " << jj;
					UpdateSwitch(1, jj, (lValue == 1) ? true : false, 100, sstr.str());
					}
				}
			}
		}
		else if (tmpstr.find("IN") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("IN"));
			pos1 = tmpstr.find("=");
			if (pos1 != std::string::npos)
			{	
				tmpstr = tmpstr.substr(pos1+1);
				//_log.Log(LOG_ERROR,"Sterbox: OU Status: %s", tmpstr.c_str());
				StringSplit(tmpstr, ",", outputs);
				for (jj = 0; jj < inputs.size(); jj++) 
				{	
					tmpinp = inputs[jj];
					//if (( jj > 3 && jj < 8  ))
					pos1 = tmpinp.find("i");
					if (pos1 != std::string::npos)
					{
					int lValue = 0;
					//tmpstr = tmpstr.substr(pos1+1);
					tmpstr2 = outputs[jj];
					//_log.Log(LOG_ERROR,"Sterbox: OU Status: %s", tmpstr2.c_str());
					pos1 = tmpstr2.find("S");
					if (pos1 != std::string::npos)
					{
						lValue = 1;
					}
					else
					{
						lValue = 0;
					}
					std::stringstream sstr;
					sstr << "Relay " << jj;
					UpdateSwitch(1, jj, (lValue == 1) ? true : false, 100, sstr.str());
					}
				}
			}
		}
		else if (tmpstr.find("AN") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("AN"));
			pos1 = tmpstr.find("=");
			if (pos1 != std::string::npos)
			{
				tmpstr = tmpstr.substr(pos1+1);
				StringSplit(tmpstr, ",", outputs);
				if (m_Password.empty())
				{
					StringSplit("t,t,t", ",", analog);
				}
				else
				{
					//StringSplit("t,t,t", ",", inputs);
					StringSplit(m_Password, ",", analog);
				//_log.Log(LOG_ERROR,"Sterbox: Pass : %s", m_Password.c_str());

				}
				for (jj = 0; jj < 3; jj++)
				{
					tmpstr2 = outputs[jj];
					tmpinp = analog[jj];
					tmpstr2 = tmpstr2.substr(1, 10);

					float lValue = (float)atof(tmpstr2.c_str());
					std::stringstream sstr;
					sstr << "Analog " << jj;
					pos1 = tmpinp.find("t");
					if (pos1 != std::string::npos)
					{
						SendTempSensor(jj,255,lValue, sstr.str());
					}
					pos1 = tmpinp.find("v");
					if (pos1 != std::string::npos)
					{
						SendVoltageSensor(0, jj, 255, lValue, sstr.str());
					}
					pos1 = tmpinp.find("l");
					if (pos1 != std::string::npos)
					{
						SendLuxSensor(0, jj, 255,lValue, sstr.str());
					}
					pos1 = tmpinp.find("h");
					if (pos1 != std::string::npos)
					{
						SendHumiditySensor(jj,255,int(lValue), sstr.str());
					}
	
					//SendTempSensor(jj,255,lValue, sstr.str());
					//_log.Log(LOG_ERROR,"Sterbox: OU Status: %s", tmpstr2.c_str());

				}
				//int Idx = atoi(tmpstr.substr(0, pos1).c_str());
				//tmpstr = tmpstr.substr(pos1 + 1);
				//pos1 = tmpstr.find("<");
				//if (pos1 != std::string::npos)
				//{
				//	int lValue = atoi(tmpstr.substr(0, pos1).c_str());
				//	float voltage = (float)(5.0f / 1023.0f)*lValue;
				//	if (voltage > 5.0f)
				//		voltage = 5.0f;
				//	std::stringstream sstr;
				//	sstr << "Voltage " << Idx;
				//	SendVoltageSensor(0, Idx, 255, voltage, sstr.str());
				//}
			}
		}
	}
}
