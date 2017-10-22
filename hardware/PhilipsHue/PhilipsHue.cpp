#include "stdafx.h"
#include "PhilipsHue.h"
#include "PhilipsHueSensors.h"
#include "../../main/Helper.h"
#include "../../main/Logger.h"
#include "../../main/localtime_r.h"
#include "../../main/RFXtrx.h"
#include "../../main/SQLHelper.h"
#include "../../main/mainworker.h"
#include "../../main/WebServer.h"
#include "../../webserver/cWebem.h"
#include "../../httpclient/HTTPClient.h"
#include "../../json/json.h"
#include "../hardwaretypes.h"
#include <boost/make_shared.hpp>

#define round(a) ( int ) ( a + .5 )

using namespace std;

#define HUE_DEFAULT_POLL_INTERVAL 10

//#define DEBUG_PhilipsHue

#ifdef DEBUG_PhilipsHue
void SaveString2Disk(string str, string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
string ReadFile(string filename)
{
	ifstream file;
	string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif

CPhilipsHue::CPhilipsHue(const int ID, const string &IPAddress, const unsigned short Port, const string &Username, const int PollInterval) :
m_IPAddress(IPAddress),
m_UserName(Username)
{
	m_HwdID=ID;
	m_Port = Port;
	m_poll_interval = PollInterval;
	m_stoprequested=false;

	// Catch uninitialised Mode1 entry.
	if (m_poll_interval < 1)
	{
		m_poll_interval = HUE_DEFAULT_POLL_INTERVAL;
		_log.Log(LOG_STATUS, "Philips Hue: Using default poll interval of %d secs.", m_poll_interval);
	}
	else
	{
		_log.Log(LOG_STATUS, "Philips Hue: Using poll interval of %d secs.", m_poll_interval);
	}

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
	m_thread = boost::make_shared<boost::thread>(boost::bind(&CPhilipsHue::Do_Work, this));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

bool CPhilipsHue::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}


void CPhilipsHue::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = m_poll_interval - 1;

	_log.Log(LOG_STATUS,"Philips Hue: Worker started...");

	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % m_poll_interval == 0)
			{
				m_LastHeartbeat = mytime(NULL);
				GetStates();
			}
		}
	}
	_log.Log(LOG_STATUS,"Philips Hue: Worker stopped...");
}

bool CPhilipsHue::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	int svalue = 0;
	string LCmd = "";
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
		//_log.Log(LOG_STATUS, "HueBridge state change: svalue = %d, psen-level = %d", svalue, pSen->LIGHTING2.level);
		SwitchLight(nodeID, LCmd, svalue);
	}
	else if (packettype == pTypeLimitlessLights)
	{
		_tLimitlessLights *pLed = (_tLimitlessLights*)pSen;
		nodeID = static_cast<int>(pLed->id);

		if (pLed->command == Limitless_LedOff)
		{
			LCmd = "Off";
			svalue = 0;
			SwitchLight(nodeID, LCmd, svalue);
			return true;
		}
		else if (pLed->command == Limitless_LedOn)
		{
			LCmd = "On";
			svalue = 255;
			SwitchLight(nodeID, LCmd, svalue);
			return true;
		}
		else if (pLed->command == Limitless_SetBrightnessLevel)
		{
			if (pLed->value == 0)
			{
				//Off
				LCmd = "Off";
				svalue = 0;
				SwitchLight(nodeID, LCmd, svalue);
			}
			else
			{
				LCmd = "Set Level";
				float fvalue = (255.0f / 100.0f)*float(pLed->value);
				if (fvalue > 255.0f)
					fvalue = 255.0f;
				svalue = round(fvalue);
				SwitchLight(nodeID, LCmd, svalue);
			}
			return true;
		}
		else if (pLed->command == Limitless_SetColorToWhite)
		{
			LCmd = "Set White";
			SwitchLight(nodeID, LCmd, 0);
			return true;
		}
		else if (pLed->command == Limitless_SetRGBColour)
		{
			float cHue = (65535.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
			LCmd = "Set Hue";
			svalue = round(cHue);
			SwitchLight(nodeID, LCmd, svalue);
			return true;
		}
	}
	return true;
}

bool CPhilipsHue::SwitchLight(const int nodeID, const string &LCmd, const int svalue)
{
	vector<string> ExtraHeaders;
	string sResult;
	stringstream sPostData;

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
	else if (LCmd == "Set Hex")
	{
		sPostData << "{\"on\": true, \"sat\": 255 , \"hue\": " << svalue << " }";
	}
	else
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid light command received!");
		return false;
	}

	stringstream sstr2;
	if (nodeID < 1000)
	{
		//Light
		sstr2 << "http://" << m_IPAddress
			<< ":" << m_Port
			<< "/api/" << m_UserName
			<< "/lights/" << nodeID << "/state";
	}
	else if (nodeID < 2000)
	{
		//Group
		sstr2 << "http://" << m_IPAddress
			<< ":" << m_Port
			<< "/api/" << m_UserName
			<< "/groups/" << nodeID - 1000 << "/action";
	}
	else
	{
		//Scene
		//Because Scenes does not have a unique numeric value (but a string as ID),
		//lookup the Options, and activate this scene

		vector<vector<string> > result;
		result = m_sql.safe_query("SELECT MacAddress FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, nodeID - 2000);
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "Philips Hue: Scene not found!");
			return false;
		}
		sPostData.clear();
		sPostData.str("");
		sPostData << "{\"scene\": \"" << result[0][0] << "\"}";
		sstr2 << "http://" << m_IPAddress
			<< ":" << m_Port
			<< "/api/" << m_UserName
			<< "/groups/0/action";
	}
	string sURL = sstr2.str();
	if (!HTTPClient::PUT(sURL, sPostData.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Error connecting to Hue bridge (Switch Light/Scene), (Check IPAddress/Username)");
		return false;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received (Switch Light/Scene), or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("error") != string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Philips Hue: Error received: %s", root[0]["error"]["description"].asString().c_str());
		return false;
	}
	return true;
}

string CPhilipsHue::RegisterUser(const string &IPAddress, const unsigned short Port, const string &username)
{
	string retStr = "Error;Unknown";
	vector<string> ExtraHeaders;
	string sResult;
	string sPostData;
	
	//Providing own username is not allowed, so don't use it and only provide devicetype
	sPostData = "{ \"devicetype\": \"domoticz\" }";

	stringstream sstr2;
	sstr2 << "http://" << IPAddress
		<< ":" << Port
		<< "/api";
	string sURL = sstr2.str();

	if (!HTTPClient::POST(sURL, sPostData, ExtraHeaders, sResult))
	{
		retStr = "Error;Error connecting to Hue bridge:";
		return retStr;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		retStr = "Error;Registration failed (Wrong IPAddress?)";
		return retStr;
	}

	if (sResult.find("error") != string::npos)
	{
		retStr = "Error;" + root[0]["error"]["description"].asString();
		return retStr;
	}

	string new_username = root[0]["success"]["username"].asString();
	retStr = "OK;" + new_username;
	return retStr;
}

void CPhilipsHue::InsertUpdateSwitch(const int NodeID, const _eHueLightType LType, const bool bIsOn, const int BrightnessLevel, const int Sat, const int Hue, const string &Name, const string &Options)
{
	if (LType == HLTYPE_RGBW)
	{
		char szID[10];
		char szSValue[20];
		if (NodeID==1)
			sprintf(szID, "%d", NodeID);
		else
			sprintf(szID, "%08x", (unsigned int)NodeID);
		sprintf(szSValue, "%d;%d", Sat, Hue);
		unsigned char unitcode = 1;
		int cmd = (bIsOn ? Limitless_LedOn : Limitless_LedOff);
		int nvalue = 0;
		bool tIsOn = !(bIsOn);
		
		//Get current nValue if exist
		vector<vector<string> > result;
		result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeLimitlessLights, sTypeLimitlessRGBW, szID);

		if (!result.empty())
		{
			//Already in the system
			//Update state
			nvalue = atoi(result[0][0].c_str());
			tIsOn = (nvalue != 0);					
		}
		
		if (bIsOn != tIsOn) //light was switched, send on or off
		{
			//Send as LimitlessLight
			_tLimitlessLights lcmd;
			lcmd.id = NodeID;
			lcmd.command = cmd;
			lcmd.value = BrightnessLevel;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, Name.c_str(), 255);
		}
		
		if (bIsOn && (BrightnessLevel != 100))
				cmd = Limitless_SetBrightnessLevel;

		if (!result.empty())
		{
			time_t now = time(0);
			struct tm ltime;
			localtime_r(&now, &ltime);

			char szLastUpdate[40];
			sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
			m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastLevel = %d, LastUpdate='%q' WHERE(HardwareID == %d) AND (DeviceID == '%q')",
			int(cmd), szSValue, BrightnessLevel, szLastUpdate, m_HwdID, szID);
		}
		else
		{
			//Set Name/Parameters
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, nValue=%d, sValue='%q', LastLevel=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
				Name.c_str(), int(STYPE_Dimmer), int(cmd), szSValue, BrightnessLevel, m_HwdID, szID);
		}
	}
	else if (LType == HLTYPE_SCENE)
	{
		char szID[10];
		sprintf(szID, "%08x", (unsigned int)NodeID);
		unsigned char unitcode = 1;
		int cmd = (bIsOn ? Limitless_LedOn : Limitless_LedOff);
		
		//Get current nValue if exist
		vector<vector<string> > result;
		result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeLimitlessLights, sTypeLimitlessRGBW, szID);
		if (!result.empty())
		{
			//Already in the system
			int nvalue = atoi(result[0][0].c_str());
			bool tIsOn = (nvalue != 0);
			if (bIsOn == tIsOn) //Check if the scene was switched
				return;
		}
		//Send as LimitlessLight
		_tLimitlessLights lcmd;
		lcmd.id = NodeID;
		lcmd.command = cmd;
		lcmd.value = 0;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, Name.c_str(), 255);
		
		if (result.empty())
		{
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, Options='%q' WHERE (HardwareID == %d) AND(DeviceID == '%q')",
				Name.c_str(), int(STYPE_PushOn), Options.c_str(), m_HwdID, szID);
		}
	}
	else
	{
		//Send as Lighting 2
		char szID[10];
		sprintf(szID, "%X%02X%02X%02X", 0, 0, 0, NodeID);
		unsigned char unitcode = 1;
		int cmd = (bIsOn ? light2_sOn : light2_sOff);
		int level = 0;
		int nvalue = 0;
		bool tIsOn = !(bIsOn);
		
		if (LType == HLTYPE_NORMAL)
			bIsOn ? level = 15 : level = 0;
		else
		{
			float flevel = (15.0f / 100.0f)*float(BrightnessLevel);
			level = round(flevel);
			if (level > 15)
				level = 15;
			if (level == 0)
				level += 1; //If brightnesslevel < 6, level = 0 even if light is on
		}
		char szLevel[20];
		sprintf(szLevel, "%d", level);

		//Check if we already exist
		vector<vector<string> > result;
		result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeLighting2, sTypeAC, szID);
		//_log.Log(LOG_STATUS, "HueBridge state change: Bri = %d, Level = %d", BrightnessLevel, level);

		if (!result.empty())
		{
			//Already in the system
			//Update state
			nvalue = atoi(result[0][0].c_str());
			tIsOn = (nvalue != 0);					
		}
		
		if (bIsOn != tIsOn) //light was switched, send on or off
		{
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
			lcmd.LIGHTING2.cmnd = cmd;
			lcmd.LIGHTING2.level = level;
			lcmd.LIGHTING2.filler = 0;
			lcmd.LIGHTING2.rssi = 12;
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd.LIGHTING2, Name.c_str(), 255);
		}
		
		if (bIsOn && (level != 15))
				cmd = light2_sSetLevel;
		
		if (!result.empty())
		{
			time_t now = time(0);
			struct tm ltime;
			localtime_r(&now, &ltime);

			char szLastUpdate[40];
			sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
			m_sql.safe_query("UPDATE DeviceStatus SET LastLevel=%d, nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')",
				BrightnessLevel, int(cmd), szLevel, szLastUpdate, m_HwdID, szID);
		}
		else
		{
			//Set Name/Parameters
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, LastLevel=%d, nValue=%d, sValue='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')",
				Name.c_str(), int(LType == HLTYPE_DIM ? STYPE_Dimmer : STYPE_OnOff), BrightnessLevel, int(cmd), szLevel, m_HwdID, szID);
		}
	}
}

bool CPhilipsHue::GetStates()
{
	string sResult;

#ifdef DEBUG_PhilipsHue
	sResult= ReadFile("E:\\philipshue.json");
#else
	stringstream sstr2;
	sstr2 << "http://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName;
	//Get Data
	string sURL = sstr2.str();
	vector<string> ExtraHeaders;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Error getting Light States, (Check IPAddress/Username)");
		return false;
	}
#endif
#ifdef DEBUG_PhilipsHue2
	SaveString2Disk(sResult, "E:\\philipshue.json");
#endif

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("\"error\":") != string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Philips Hue: Error received: %s", root[0]["error"]["description"].asString().c_str());
		return false;
	}


	if (!GetLights(root))
	{
		//_log.Log(LOG_ERROR, "Philips Hue: No Lights found!");
		return false;
	}

	GetGroups(root);
	GetScenes(root);
	GetSensors(root);

	return true;
}

bool CPhilipsHue::GetLights(const Json::Value &root)
{
	if (root["lights"].empty())
		return false;

	for (Json::Value::const_iterator iLight = root["lights"].begin(); iLight != root["lights"].end(); ++iLight)
	{
		Json::Value light = *iLight;
		if (light.isObject())
		{
			string szLID = iLight.key().asString();
			int lID = atoi(szLID.c_str());
			_tHueLight tlight;
			tlight.level = 0;
			tlight.sat = 0; // Philips 0- 254, should be corrected to 0 - 255 ?
			tlight.hue = 0; // Philips 0 - 65535, should be converted to 0 - 255 ?
			tlight.on = light["state"]["on"].asBool();
			bool bDoSend = true;
			_eHueLightType LType = HLTYPE_NORMAL;
	
			if (!light["state"]["bri"].empty())
			{
				//Lamp with brightness control
				LType = HLTYPE_DIM;
				int tbri = light["state"]["bri"].asInt();
				if ((tbri != 0) && (tbri != 255))
					tbri += 1; //hue reports 255 as 254
				tlight.level = int((100.0f / 255.0f)*float(tbri));
			}
			if ((!light["state"]["sat"].empty()) && (!light["state"]["hue"].empty()))
			{
				//Lamp with hue/sat control
				LType = HLTYPE_RGBW;
				tlight.sat = light["state"]["sat"].asInt();
				tlight.hue = light["state"]["hue"].asInt();
			}
			if (m_lights.find(lID) != m_lights.end())
			{
				_tHueLight alight = m_lights[lID];
				if (
					(alight.on == tlight.on) &&
					(alight.level == tlight.level) &&
					(alight.sat == tlight.sat) &&
					(alight.hue == tlight.hue)
					)
				{
					bDoSend = false;
				}
			}
			m_lights[lID] = tlight;
			if (bDoSend)
			{
				//_log.Log(LOG_STATUS, "HueBridge state change: tbri = %d, level = %d", tbri, tlight.level);
				InsertUpdateSwitch(lID, LType, tlight.on, tlight.level, tlight.sat, tlight.hue, light["name"].asString(), "");
			}
		}
	}
	return true;
}

// Note:
// Some groups have only White lights,
// We whould find a way to have these working as normal lights instead of RGBW
bool CPhilipsHue::GetGroups(const Json::Value &root)
{
	//Groups (0=All)

	if (root["groups"].empty())
		return false;

	_eHueLightType LType;

	for (Json::Value::const_iterator iGroup = root["groups"].begin(); iGroup != root["groups"].end(); ++iGroup)
	{
		Json::Value group = *iGroup;
		if (group.isObject())
		{
			string szGID = iGroup.key().asString();
			int gID = atoi(szGID.c_str());
			_tHueLight tstate;
			tstate.on = false;
			tstate.level = 0;
			tstate.sat = 0;
			tstate.hue = 0;

			if (!group["action"]["on"].empty())
				tstate.on = group["action"]["on"].asBool();
			if (!group["action"]["bri"].empty())
			{
				int tbri = group["action"]["bri"].asInt();
				if ((tbri != 0) && (tbri != 255))
					tbri += 1; //hue reports 255 as 254
				tstate.level = int((100.0f / 255.0f)*float(tbri));
			}

			LType = HLTYPE_RGBW;// HLTYPE_NORMAL;

			if (!group["action"]["sat"].empty())
			{
				tstate.sat = group["action"]["sat"].asInt();
				//LType = HLTYPE_RGBW;
			}
			if (!group["action"]["hue"].empty())
			{
				tstate.hue = group["action"]["hue"].asInt();
				//LType = HLTYPE_RGBW;
			}
			
			bool bDoSend = true;
			if (m_groups.find(gID) != m_groups.end())
			{
				_tHueGroup agroup = m_groups[gID];
				if (
					(agroup.gstate.on == tstate.on) &&
					(agroup.gstate.level == tstate.level) &&
					(agroup.gstate.sat == tstate.sat) &&
					(agroup.gstate.hue == tstate.hue)
					)
				{
					bDoSend = false;
				}
			}
			m_groups[gID].gstate = tstate;
			if (bDoSend)
			{
				string Name = "Group " + group["name"].asString();
				InsertUpdateSwitch(1000 + gID, LType, tstate.on, tstate.level, tstate.sat, tstate.hue, Name, "");
			}
		}
	}
	//Special Request for Group0 (All Lights)
	stringstream sstr2;
	sstr2 << "http://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName
		<< "/groups/0";
	string sResult;
	vector<string> ExtraHeaders;
	if (!HTTPClient::GET(sstr2.str(), ExtraHeaders, sResult))
	{
		//No group all(0)
		return true;
	}
	Json::Reader jReader;
	Json::Value root2;
	bool ret = jReader.parse(sResult, root2);
	if ((!ret) || (!root2.isObject()))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("\"error\":") != string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Philips Hue: Error received: %s", root2[0]["error"]["description"].asString().c_str());
		return false;
	}

	if (sResult.find("lights") == string::npos)
	{
		return false;
	}
	_tHueLight tstate;
	tstate.on = false;
	tstate.level = 0;
	tstate.sat = 0;
	tstate.hue = 0;

	LType = HLTYPE_RGBW;// HLTYPE_NORMAL;

	if (!root2["action"]["on"].empty())
		tstate.on = root2["action"]["on"].asBool();
	if (!root2["action"]["bri"].empty())
	{
		int tbri = root2["action"]["bri"].asInt();
		if ((tbri != 0) && (tbri < 3))
			tbri = 3;
		tstate.level = int((100.0f / 254.0f)*float(tbri));
	}
	if (!root2["action"]["sat"].empty())
	{
		tstate.sat = root2["action"]["sat"].asInt();
		//LType = HLTYPE_RGBW;
	}
	if (!root2["action"]["hue"].empty())
	{
		tstate.hue = root2["action"]["hue"].asInt();
		//LType = HLTYPE_RGBW;
	}

	bool bDoSend = true;
	int gID = 0;
	if (m_groups.find(gID) != m_groups.end())
	{
		_tHueGroup agroup = m_groups[gID];
		if (
			(agroup.gstate.on == tstate.on) &&
			(agroup.gstate.level == tstate.level) &&
			(agroup.gstate.sat == tstate.sat) &&
			(agroup.gstate.hue == tstate.hue)
			)
		{
			bDoSend = false;
		}
	}
	m_groups[gID].gstate = tstate;
	if (bDoSend)
	{
		string Name = "Group All Lights";
		InsertUpdateSwitch(1000 + gID, LType, tstate.on, tstate.level, tstate.sat, tstate.hue, Name,"");
	}
	return true;
}

bool CPhilipsHue::GetScenes(const Json::Value &root)
{
	if (root["scenes"].empty())
		return false;

	for (Json::Value::const_iterator iScene = root["scenes"].begin(); iScene != root["scenes"].end(); ++iScene)
	{
		Json::Value scene = *iScene;
		if (scene.isObject())
		{
			_tHueScene hscene;
			hscene.id = iScene.key().asString();;
			hscene.name = scene["name"].asString();
			hscene.lastupdated = scene["lastupdated"].asString();
			if (hscene.lastupdated.empty())
				continue; //old scene/legacy scene
			
			//Strip some info
			size_t tpos = hscene.name.find(" from ");
			if (tpos != string::npos)
			{
				hscene.name = hscene.name.substr(0, tpos);
			}

			bool bDoSend = true;
			if (m_scenes.find(hscene.id) != m_scenes.end())
			{
				_tHueScene ascene = m_scenes[hscene.id];
				if (ascene.lastupdated == hscene.lastupdated)
				{
					bDoSend = false;
				}
			}
			m_scenes[hscene.id] = hscene;

			if (bDoSend)
			{
				int sID = -1;
				vector<vector<string> > result;
				result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, hscene.id.c_str());
				if (!result.empty())
				{
					//existing scene
					sID = atoi(result[0][0].c_str());
				}
				else
				{
					//New scene
					m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (%d, '%q', '%q')", m_HwdID, hscene.name.c_str(), hscene.id.c_str());
					result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, hscene.id.c_str());
					if (result.empty())
					{
						_log.Log(LOG_ERROR, "Philips Hue: Problem adding new Scene!!");
						return false;
					}
					sID = atoi(result[0][0].c_str());
				}
				string Name = "Scene " + hscene.name;
				InsertUpdateSwitch(2000 + sID, HLTYPE_SCENE, false, 100, 0, 0, Name, hscene.id);
			}
		}
	}
	return true;
}

bool CPhilipsHue::GetSensors(const Json::Value &root)
{
	if (root["sensors"].empty())
		return false;

	for (Json::Value::const_iterator iSensor = root["sensors"].begin(); iSensor != root["sensors"].end(); ++iSensor)
	{
		Json::Value sensor = *iSensor;
		if (sensor.isObject())
		{
			bool bDoSend = true;
			bool bNewSensor = false;
			int sID = atoi(iSensor.key().asString().c_str());

			CPHSensor hsensor(sensor);
			CPHSensor lsensor;
			// Check if sensor exists and whether last update is changed
			if (m_sensors.find(sID) != m_sensors.end())
			{
				lsensor = m_sensors[sID];
				if (lsensor.m_state.m_lastupdated == hsensor.m_state.m_lastupdated)
				{
					bDoSend = false;
				}
			}
			else
			{
				// New sensor found, always update it's value
				bNewSensor = true;
			}
			m_sensors.erase(sID);
			m_sensors.insert(make_pair(sID, hsensor));

			if (bDoSend)
			{
				sID += 3000;
				string device_name = hsensor.m_type + " " + hsensor.m_name;
				if (hsensor.m_type == SensorTypeDaylight)
				{
				}
				else if ((hsensor.m_type == SensorTypeZGPSwitch)
					|| (hsensor.m_type == SensorTypeZLLSwitch))
				{
				}
				else if (hsensor.m_type == SensorTypeZLLPresence)
				{
					if ((lsensor.m_state.m_presence != hsensor.m_state.m_presence)
						|| (bNewSensor))
					{
						InsertUpdateSwitch(sID, STYPE_Motion, hsensor.m_state.m_presence, device_name, hsensor.m_config.m_battery);
					}
				}
				else if (hsensor.m_type == SensorTypeZLLTemperature)
				{
					if ((lsensor.m_state.m_temperature != hsensor.m_state.m_temperature)
						|| (bNewSensor))
					{
						SendTempSensor(sID, hsensor.m_config.m_battery, float(hsensor.m_state.m_temperature / 100.0f), device_name);
					}
				}
				else if (hsensor.m_type == SensorTypeZLLLightLevel)
				{
					if ((lsensor.m_state.m_dark != hsensor.m_state.m_dark)
						|| (bNewSensor))
					{
						InsertUpdateSwitch(sID, STYPE_Dusk, hsensor.m_state.m_dark, device_name, hsensor.m_config.m_battery);
					}

					if ((lsensor.m_state.m_lightlevel != hsensor.m_state.m_lightlevel)
						|| (bNewSensor))
					{
						double lux = 0.00001;
						if (hsensor.m_state.m_lightlevel != 0)
						{
							float convertedLightLevel = float((hsensor.m_state.m_lightlevel - 1) / 10000.00f);
							lux = pow(10, convertedLightLevel);
						}
						SendLuxSensor(sID, 0, hsensor.m_config.m_battery, (const float)lux, hsensor.m_type + " Lux " + hsensor.m_name);
					}
				}
				else
				{
					//_log.Log(LOG_STATUS, "Ignoring Philips Hue CLIP Sensors: (%s)", device_name.c_str());
				}
			}
			
		}
	}

	return true;
}

void CPhilipsHue::InsertUpdateSwitch(const int NodeID, const _eSwitchType SType, const bool status, const string &Name, uint8_t BatteryLevel)
{
	int sID = NodeID;
	char ID[40];
	sprintf(ID, "%08X", sID);

	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = sID;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch;
	xcmd.battery_level = BatteryLevel;
	xcmd.rssi = 12;

	if (status) {
		xcmd.cmnd = gswitch_sOn;
	}
	else {
		xcmd.cmnd = gswitch_sOff;
	}

	//check if this switch is already in the database
	vector<vector<string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) ", m_HwdID, ID, pTypeGeneralSwitch);
	if (result.size() < 1)
	{
		//_log.Log(LOG_STATUS, "Philips Hue Switch: New Device Found (%s)", Name.c_str());
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, Name.c_str(), BatteryLevel);

		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%q')", Name.c_str(), (SType), 0, m_HwdID, ID);
	}
	else 
	{
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, Name.c_str(), BatteryLevel);
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_PhilipsHueRegister(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			root["title"] = "RegisterOnHue";

			string sipaddress = request::findValue(&req, "ipaddress");
			string sport = request::findValue(&req, "port");
			string susername = request::findValue(&req, "username");
			if (
				(sipaddress == "") ||
				(sport == "")
				)
				return;

			string sresult = CPhilipsHue::RegisterUser(sipaddress, (unsigned short)atoi(sport.c_str()), susername);
			vector<string> strarray;
			StringSplit(sresult, ";", strarray);
			if (strarray.size() != 2)
				return;

			if (strarray[0] == "Error") {
				root["statustext"] = strarray[1];
				return;
			}
			root["status"] = "OK";
			root["username"] = strarray[1];
		}
		void CWebServer::Cmd_PhilipsHueGetGroups(WebEmSession & session, const request& req, Json::Value &root)
		{

		}
		void CWebServer::Cmd_PhilipsHueAddGroup(WebEmSession & session, const request& req, Json::Value &root)
		{

		}
		void CWebServer::Cmd_PhilipsHueDeleteGroup(WebEmSession & session, const request& req, Json::Value &root)
		{

		}
		void CWebServer::Cmd_PhilipsHueGroupAddLight(WebEmSession & session, const request& req, Json::Value &root)
		{

		}
		void CWebServer::Cmd_PhilipsHueGroupRemoveLight(WebEmSession & session, const request& req, Json::Value &root)
		{

		}
	}
}
