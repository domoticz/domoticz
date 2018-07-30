#include "stdafx.h"
#include "AnnaThermostat.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"

#include <tinyxml.h>

#define round(a) ( int ) ( a + .5 )

// Plugwise Anna Thermostat

const std::string ANNA_GET_STATUS = "/core/appliances";

#ifdef _DEBUG
	//#define DEBUG_AnnaThermostat
#endif

#ifdef DEBUG_AnnaThermostat
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

CAnnaThermostat::CAnnaThermostat(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password) :
m_IPAddress(IPAddress),
m_IPPort(usIPPort),
m_UserName(CURLEncode::URLEncode(Username)),
m_Password(CURLEncode::URLEncode(Password))
{
	m_HwdID=ID;
	Init();
	GetMeterDetails();
}

CAnnaThermostat::~CAnnaThermostat(void)
{
}

void CAnnaThermostat::Init()
{
	m_ThermostatID = "";
	m_stoprequested = false;
}

bool CAnnaThermostat::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CAnnaThermostat::Do_Work, this);
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CAnnaThermostat::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
    return true;
}

#define ANNA_POLL_INTERVAL 30

void CAnnaThermostat::Do_Work()
{
	bool bFirstTime = true;
	_log.Log(LOG_STATUS,"AnnaThermostat: Worker started...");
	int sec_counter = ANNA_POLL_INTERVAL-5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if ((sec_counter % ANNA_POLL_INTERVAL == 0) || (bFirstTime))
		{
			sec_counter = 0;
			bFirstTime = false;
			GetMeterDetails();
		}

	}
	_log.Log(LOG_STATUS,"AnnaThermostat: Worker stopped...");
}

void CAnnaThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
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

bool CAnnaThermostat::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;

	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
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

void CAnnaThermostat::SetSetpoint(const int /*idx*/, const float temp)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	if (m_ThermostatID.size() == 0)
		GetMeterDetails();

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_IPAddress << ":" << m_IPPort;
	}
	else
	{
		szURL << "http://" << m_UserName << ":" << m_Password << "@" << m_IPAddress << ":" << m_IPPort;
	}
	szURL << ANNA_GET_STATUS;
	szURL << ";id=";
	szURL << m_ThermostatID;
	szURL << "/thermostat";

	std::stringstream sPostData;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	char szTemp[10];
	sprintf(szTemp, "%.1f", temp);

	sPostData << "<thermostat>";
	sPostData << "<setpoint>";
	sPostData << szTemp;
	sPostData << "</setpoint>";
	sPostData << "</thermostat>";

	if (!HTTPClient::PUT(szURL.str(), sPostData.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "AnnaThermostat: Error getting current state!");
		return;
	}

}

bool CAnnaThermostat::SetAway(const bool /*bIsAway*/)
{
	return false;
}

void CAnnaThermostat::SetProgramState(const int /*newState*/)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;
}

std::string GetElementChildValue(TiXmlElement *pElement, const char *szChildName)
{
	std::string ret;
	TiXmlNode *pNode = pElement->FirstChild(szChildName);
	if (!pNode)
		return ret;
	pNode = pNode->FirstChild();
	if (!pNode)
		return ret;
	ret = pNode->Value();
	return ret;
}

std::string GetPeriodMeasurement(TiXmlElement *pElement)
{
	std::string ret;
	TiXmlNode *pNode = pElement->FirstChild("period");
	if (!pNode)
		return ret;
	pNode = pNode->FirstChild("measurement");
	if (!pNode)
		return ret;
	pNode = pNode->FirstChild();
	if (!pNode)
		return ret;
	ret = pNode->Value();
	return ret;
}

void CAnnaThermostat::GetMeterDetails()
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;
	std::string sResult;
#ifdef DEBUG_AnnaThermostat
	sResult = ReadFile("E:\\appliances.xml");
#else
	//Get Data
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_IPAddress << ":" << m_IPPort;
	}
	else
	{
		szURL << "http://" << m_UserName << ":" << m_Password << "@" << m_IPAddress << ":" << m_IPPort;
	}
	szURL << ANNA_GET_STATUS;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "AnnaThermostat: Error getting current state!");
		return;
	}
#endif
	if (sResult.empty())
	{
		_log.Log(LOG_ERROR, "AnnaThermostat: Invalid data received!");
		return;
	}

	stdreplace(sResult, "\r\n", "");

	TiXmlDocument doc;
	if (doc.Parse(sResult.c_str()))
	{
		_log.Log(LOG_ERROR, "AnnaThermostat: Invalid data received!");
		return;
	}

	TiXmlElement *pRoot;
	TiXmlElement *pAppliance, *pElem;
	TiXmlAttribute *pAttribute;
	std::string sname, tmpstr;

	pRoot = doc.FirstChildElement("appliances");
	if (!pRoot)
	{
		_log.Log(LOG_ERROR, "AnnaThermostat: Invalid data received!");
		return;
	}
	pAppliance = pRoot->FirstChildElement("appliance");
	while (pAppliance)
	{
		TiXmlHandle hAppliance = TiXmlHandle(pAppliance);

		pElem = pAppliance->FirstChildElement("name");
		if (pElem == NULL)
		{
			_log.Log(LOG_ERROR, "AnnaThermostat: Invalid data received!");
			return;
		}
		std::string ApplianceName=pElem->GetText();

		if ((m_ThermostatID.empty()) && (ApplianceName == "Anna"))
		{
			pAttribute = pAppliance->FirstAttribute();
			if (pAttribute != NULL)
			{
				std::string aName = pAttribute->Name();
				if (aName == "id")
				{
					m_ThermostatID = pAttribute->Value();
				}
			}
		}

		//Handle point_log
		pElem = hAppliance.FirstChild("logs").FirstChild().Element();
		if (!pElem)
		{
			_log.Log(LOG_ERROR, "AnnaThermostat: Invalid data received!");
			return;
		}
		TiXmlHandle hLogs = TiXmlHandle(pElem);
		pElem = hAppliance.FirstChild("logs").Child("point_log", 0).ToElement();
		if (!pElem)
		{
			_log.Log(LOG_ERROR, "AnnaThermostat: Invalid data received!");
			return;
		}
		for (pElem; pElem; pElem = pElem->NextSiblingElement())
		{
			sname = GetElementChildValue(pElem, "type");
			if (sname == "temperature")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendTempSensor(1, 255, temperature, sname);
				}
			}
			else if (sname == "illuminance")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float illuminance = (float)atof(tmpstr.c_str());
					SendLuxSensor(2, 1, 255, illuminance, sname);
				}
			}
			else if (sname == "thermostat")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendSetPointSensor(3, temperature, sname);
				}
			}
			/*
			else if (sname == "intended_boiler_temperature")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendTempSensor(4, 255, temperature, sname);
				}
			}
			*/
			else if (sname == "return_water_temperature")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendTempSensor(5, 255, temperature, sname);
				}
			}
			else if (sname == "boiler_temperature")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendTempSensor(6, 255, temperature, sname);
				}
			}
			else if (sname == "maximum_boiler_temperature")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendTempSensor(7, 255, temperature, sname);
				}
			}
		}

		pAppliance = pAppliance->NextSiblingElement("appliance");
	}
	return;
}
