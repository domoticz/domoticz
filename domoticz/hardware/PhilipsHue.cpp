#include "stdafx.h"
#include "PhilipsHue.h"
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

#define DEBUG_PhilipsHue

#ifdef DEBUG_PhilipsHue
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

CPhilipsHue::CPhilipsHue(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username)
{
	m_HwdID=ID;
	m_IPAddress= IPAddress;
	m_Port = Port;
	m_UserName = Username;
	m_stoprequested=false;
	m_bGetInitialLights = false;
	Init();
}

CPhilipsHue::~CPhilipsHue(void)
{
}

void CPhilipsHue::Init()
{
}

bool CPhilipsHue::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPhilipsHue::Do_Work, this)));
	m_bIsStarted=true;
	m_bGetInitialLights = true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CPhilipsHue::StopHardware()
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

#define HUE_POLL_INTERVAL 10

void CPhilipsHue::Do_Work()
{
	int LastSecond=-1;

	_log.Log(LOG_STATUS,"Philips Hue: Worker started...");
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		time_t atime=mytime(NULL);
		int tsec = atime%HUE_POLL_INTERVAL;
		if (((tsec == 0) && ((atime%60) != LastSecond)) || (LastSecond == -1))
		{
			LastSecond = (atime % 60);
			if (m_bGetInitialLights)
			{
				m_bGetInitialLights = false;
				GetLightStates();
			}
			mytime(&m_LastHeartbeat);

		}
	}
	_log.Log(LOG_STATUS,"Philips Hue: Worker stopped...");
}

void CPhilipsHue::SendSwitchIfNotExists(const int NodeID, const _eHueLightType LType, const int BrightnessLevel, const std::string &Name)
{
	if (LType == HLTYPE_RGBW)
	{
		char szID[10];
		sprintf(szID, "%08x", (unsigned int)NodeID);
		std::string ID = szID;
		unsigned char unitcode = 1;

		//Check if we already exist
		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << int(unitcode) << ") AND (Type==" << pTypeLimitlessLights << ") AND (SubType==" << sTypeLimitlessRGBW << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
		if (result.size() > 0)
			return; //Already in the system

		//Send as LimitlessLight
		_tLimitlessLights lcmd;
		lcmd.id = NodeID;
		lcmd.command = Limitless_LedOff;
		lcmd.value = 0;
		sDecodeRXMessage(this, (const unsigned char *)&lcmd);

		//Set Name
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << Name << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
	}
	else
	{
		char szID[10];
		sprintf(szID, "%X%02X%02X%02X", 0, 0, 0, NodeID);
		std::string ID = szID;
		unsigned char unitcode = 1;

		//Check if we already exist
		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (Unit==" << int(unitcode) << ") AND (Type==" << pTypeLighting2 << ") AND (SubType==" << sTypeAC << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
		if (result.size() > 0)
			return; //Already in the system

		//Send as Lighting 2

		tRBUF lcmd;
		memset(&lcmd, 0, sizeof(RBUF));
		lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
		lcmd.LIGHTING2.packettype = pTypeLighting2;
		lcmd.LIGHTING2.subtype = sTypeAC;
		lcmd.LIGHTING2.seqnbr = 1;
		lcmd.LIGHTING2.id1 = 0;
		lcmd.LIGHTING2.id2 = 0;
		lcmd.LIGHTING2.id3 = 0;
		lcmd.LIGHTING2.id4 = NodeID;
		lcmd.LIGHTING2.unitcode = unitcode;
		int level = 15;
		if (LType == HLTYPE_NORMAL)
		{
			//simple on/off device
			if (BrightnessLevel == 0)
			{
				level = 0;
				lcmd.LIGHTING2.cmnd = light2_sOff;
			}
			else
			{
				level = 15;
				lcmd.LIGHTING2.cmnd = light2_sOn;
			}
		}
		else
		{
			//dimmer able device
			if (BrightnessLevel == 0)
				level = 0;
			if (BrightnessLevel == 255)
				level = 15;
			else
			{
				float flevel = (15.0f / 100.0f)*float(BrightnessLevel);
				level = round(flevel);
				if (level > 15)
					level = 15;
			}
			if (level == 0)
				lcmd.LIGHTING2.cmnd = light2_sOff;
			else if (level == 15)
				lcmd.LIGHTING2.cmnd = light2_sOn;
			else
				lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		}
		lcmd.LIGHTING2.level = level;
		lcmd.LIGHTING2.filler = 0;
		lcmd.LIGHTING2.rssi = 12;

		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

		//Set Name
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << Name << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << ID << "')";
		result = m_sql.query(szQuery.str());
	}
}


bool CPhilipsHue::GetLightStates()
{
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::stringstream sstr2;
	sstr2 << "http://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName;
	//Get Data
	std::string sURL = sstr2.str();
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"Philips Hue: Error getting Light States, (Check IPAddress/Username)");
		return false;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("error") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Philips Hue: Error received: %s", root[0]["error"]["description"].asString().c_str());
		return false;
	}

	if (sResult.find("lights")==std::string::npos)
	{
		return false;
	}

	int totLights = root["lights"].size();
	char szNode[10];
	for (int ii = 0; ii < totLights; ii++)
	{
		sprintf(szNode, "%d", ii + 1);
		std::string ltype = root["lights"][szNode]["type"].asString();
		if (
			(ltype == "Dimmable plug-in unit")||
			(ltype == "Dimmable light")
			)
		{
			//Normal light (with dim option)
			int BrightnessLevel = 0;
			bool bIsOn = root["lights"][szNode]["state"]["on"].asBool();
			if (bIsOn)
			{
				int tbri = root["lights"][szNode]["state"]["bri"].asInt();
				BrightnessLevel = int((100.0f / 255.0f)*float(tbri));
			}
			SendSwitchIfNotExists(ii + 1, HLTYPE_DIM, BrightnessLevel, root["lights"][szNode]["name"].asString());
		}
		else if (
			(ltype == "Extended color light")||
			(ltype == "Color light")
			)
		{
			//RGBW type
			int BrightnessLevel = 0;
			bool bIsOn = root["lights"][szNode]["state"]["on"].asBool();
			if (bIsOn)
			{
				int tbri = root["lights"][szNode]["state"]["bri"].asInt();
				BrightnessLevel = int((100.0f / 255.0f)*float(tbri));
			}
			SendSwitchIfNotExists(ii + 1, HLTYPE_RGBW, BrightnessLevel, root["lights"][szNode]["name"].asString());
		}
	}

	return true;
}

void CPhilipsHue::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pSen = (tRBUF*)pdata;

	unsigned char packettype = pSen->ICMND.packettype;
	unsigned char subtype = pSen->ICMND.subtype;

	int svalue = 0;
	std::string LCmd = "";
	int nodeID = 0;

	if (packettype == pTypeLighting2)
	{
		//light command
		nodeID = pSen->LIGHTING2.id4;
		if ((pSen->LIGHTING2.cmnd == light2_sOff) || (pSen->LIGHTING2.cmnd == light2_sGroupOff))
		{
			LCmd = "Off";
			svalue = 0;
		}
		else if ((pSen->LIGHTING2.cmnd == light2_sOn) || (pSen->LIGHTING2.cmnd == light2_sGroupOn))
		{
			LCmd = "On";
			svalue = 255;
		}
		else
		{
			LCmd = "Set Level";
			float fvalue = (255.0f / 15.0f)*float(pSen->LIGHTING2.level);
			if (fvalue > 255.0f)
				fvalue = 255.0f;
			svalue = round(fvalue);
		}
		SwitchLight(nodeID, LCmd, svalue);
	}
	else if (packettype == pTypeLimitlessLights)
	{
		_tLimitlessLights *pLed = (_tLimitlessLights*)pSen;
		nodeID = (int)pLed->id;

		if (pLed->command == Limitless_LedOff)
		{
			LCmd = "Off";
			svalue = 0;
			SwitchLight(nodeID, LCmd, svalue);
			return;
		}
		else if (pLed->command == Limitless_LedOn)
		{
			LCmd = "On";
			svalue = 255;
			SwitchLight(nodeID, LCmd, svalue);
			return;
		}
		else if (pLed->command == Limitless_SetBrightnessLevel)
		{
			LCmd = "Set Level";
			float fvalue = (255.0f / 100.0f)*float(pLed->value);
			if (fvalue > 255.0f)
				fvalue = 255.0f;
			svalue = round(fvalue);
			SwitchLight(nodeID, LCmd, svalue);
			return;
		}
		else if (pLed->command == Limitless_SetColorToWhite)
		{
			LCmd = "Set White";
			SwitchLight(nodeID, LCmd, 0);
			return;
		}
		else if (pLed->command == Limitless_SetRGBColour)
		{
			float cHue = (65535.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
			LCmd = "Set Hue";
			svalue = round(cHue);
			SwitchLight(nodeID, LCmd, svalue);
			return;
		}
	}
}

bool CPhilipsHue::SwitchLight(const int nodeID, const std::string &LCmd, const int svalue)
{
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sPostData;

	std::stringstream sstr2;
	sstr2 << "http://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName
		<< "/lights/" << nodeID << "/state";

	if (LCmd=="On")
	{
		sPostData << "{\"on\": true }";
	}
	else if (LCmd == "Off")
	{
		sPostData << "{\"on\": false }";
	}
	else if (LCmd == "Set Level")
	{
		sPostData << "{\"on\": true, \"bri\": " << svalue << " }";
	}
	else if (LCmd == "Set White")
	{
		sPostData << "{\"on\": true, \"sat\": 0 , \"bri\": 255, \"hue\": 0 }";
	}
	else if (LCmd == "Set Hue")
	{
		sPostData << "{\"on\": true, \"sat\": 255 , \"hue\": " << svalue << " }";
	}
	else
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid light command received!");
		return false;
	}

	std::string sURL = sstr2.str();
	if (!HTTPClient::PUT(sURL, sPostData.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Error getting Light States, (Check IPAddress/Username)");
		return false;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("error") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Philips Hue: Error received: %s", root[0]["error"]["description"].asString().c_str());
		return false;
	}
	return true;
}