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
#include "../../main/json_helper.h"
#include "../hardwaretypes.h"

#define round(a) ( int ) ( a + .5 )

#define HUE_DEFAULT_POLL_INTERVAL 10
#define HUE_NOT_ADD_GROUPS 0x01
#define HUE_NOT_ADD_SCENES 0x02

#define SensorTypeDaylight "Daylight"
#define SensorTypeZGPSwitch "ZGPSwitch"
#define SensorTypeZLLSwitch "ZLLSwitch"
#define SensorTypeZLLPresence "ZLLPresence"
#define SensorTypeZLLTemperature "ZLLTemperature"
#define SensorTypeZLLLightLevel "ZLLLightLevel"
#define SensorTypeGeofence "Geofence"

//#define DEBUG_PhilipsHue

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

CPhilipsHue::CPhilipsHue(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username, const int PollInterval, const int Options) :
m_IPAddress(IPAddress),
m_UserName(Username)
{
	m_HwdID=ID;
	m_Port = Port;
	m_poll_interval = PollInterval;
	m_add_groups = (Options & HUE_NOT_ADD_GROUPS) != 0;
	m_add_scenes = (Options & HUE_NOT_ADD_SCENES) != 0;

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

void CPhilipsHue::Init()
{
}

bool CPhilipsHue::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CPhilipsHue::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CPhilipsHue::StopHardware()
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


void CPhilipsHue::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = m_poll_interval - 1;

	_log.Log(LOG_STATUS,"Philips Hue: Worker started...");

	while (!IsStopRequested(500))
	{
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % m_poll_interval == 0)
			{
				m_LastHeartbeat = mytime(nullptr);
				GetStates();
			}
		}
	}
	_log.Log(LOG_STATUS,"Philips Hue: Worker stopped...");
}

bool CPhilipsHue::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	int svalue = 0;
	int svalue2 = 0;
	int svalue3 = 0;
	std::string LCmd;
	int nodeID = 0;

	if (packettype == pTypeGeneralSwitch)
	{
		const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pSen);
		//light command
		nodeID = static_cast<int>(pSwitch->id);
		if ((pSwitch->cmnd == gswitch_sOff) || (pSwitch->cmnd == gswitch_sGroupOff))
		{
			LCmd = "Off";
			svalue = 0;
		}
		else if ((pSwitch->cmnd == gswitch_sOn) || (pSwitch->cmnd == gswitch_sGroupOn))
		{
			LCmd = "On";
			svalue = 254;
		}
		else if (pSwitch->cmnd == gswitch_sSetLevel)
		{
			// From Philips Hue API documentation:
			// Brightness is a scale from 1 (the minimum the light is capable of) to 254 (the maximum). Note: a brightness of 1 is not off.
			LCmd = "Set Level";
			float fvalue = (254.0F / 100.0F) * float(pSwitch->level);
			if (fvalue > 254.0F)
				fvalue = 254.0F;
			svalue = round(fvalue);
		}
		SwitchLight(nodeID, LCmd, svalue);
	}
	else if (packettype == pTypeColorSwitch)
	{
		const _tColorSwitch *pLed = reinterpret_cast<const _tColorSwitch*>(pSen);
		nodeID = static_cast<int>(pLed->id);

		if (pLed->command == Color_LedOff)
		{
			LCmd = "Off";
			svalue = 0;
			SwitchLight(nodeID, LCmd, svalue);
			return true;
		}
		if (pLed->command == Color_LedOn)
		{
			LCmd = "On";
			svalue = 254;
			SwitchLight(nodeID, LCmd, svalue);
			return true;
		}
		if (pLed->command == Color_SetBrightnessLevel)
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
				// From Philips Hue API documentation:
				// Brightness is a scale from 1 (the minimum the light is capable of) to 254 (the maximum). Note: a brightness of 1 is not off.
				LCmd = "Set Level";
				float fvalue = (254.0F / 100.0F) * float(pLed->value);
				if (fvalue > 254.0F)
					fvalue = 254.0F;
				svalue = round(fvalue);
				SwitchLight(nodeID, LCmd, svalue);
			}
			return true;
		}
		if (pLed->command == Color_SetColorToWhite)
		{
			LCmd = "Set White";
			SwitchLight(nodeID, LCmd, 0);
			return true;
		}
		if (pLed->command == Color_SetColor)
		{
			// From Philips Hue API documentation:
			// bri: Brightness is a scale from 1 (the minimum the light is capable of) to 254 (the maximum). Note: a brightness of 1 is not off.
			// hue: The hue value is a wrapping value between 0 and 65535. Both 0 and 65535 are red, 25500 is green and 46920 is blue.
			// sat: Saturation of the light. 254 is the most saturated (colored) and 0 is the least saturated (white).
			// ct: The Mired Color temperature of the light. 2012 connected lights are capable of 153 (6500K) to 500 (2000K).
			if (pLed->value == 0)
			{
				//Off
				LCmd = "Off";
				svalue = 0;
				SwitchLight(nodeID, LCmd, svalue);
			}
			else if (pLed->color.mode == ColorModeWhite)
			{
				LCmd = "Set Hue";
				//TODO: Is this correct way to turn off RGB LED and turn on white LED?
				svalue2 = 0; // Hue
				svalue3 = 0; // sat
			}
			else if (pLed->color.mode == ColorModeTemp)
			{
				LCmd = "Set CT";
				svalue2 = round(float(pLed->color.t) * (500.0F - 153.0F) / 255.0F + 153.0F);
			}
			else if (pLed->color.mode == ColorModeRGB)
			{
				float hsb[3];
				rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
				float cHue = (65535.0F) * hsb[0]; // Scale hue from 0..1 to 0..65535
				float cSat = (254.0F) * hsb[1];	  // Scale saturation from 0..1 to 0..254
				LCmd = "Set Hue";
				svalue2 = round(cHue);
				svalue3 = round(cSat);
			}
			else{
				_log.Log(LOG_STATUS, "Philips Hue: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
			}
			float fvalue = (254.0F / 100.0F) * float(pLed->value);
			if (fvalue > 254.0F)
				fvalue = 254.0F;
			svalue = round(fvalue);
			SwitchLight(nodeID, LCmd, svalue, svalue2, svalue3);
			return true;
		}
	}
	return true;
}

bool CPhilipsHue::SwitchLight(const int nodeID, const std::string &LCmd, const int svalue, const int svalue2 /*= 0*/, const int svalue3 /*= 0*/)
{
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");
	std::string sResult;
	std::stringstream sPostData;
	bool setOn = false;
	bool On = true;
	bool setLevel = false;
	bool setHueSat = false;
	bool setCt = false;
	bool setMode = false;
	_eHueColorMode mode;

	if (LCmd=="On")
	{
		sPostData << "{\"on\": true }";
		setOn = true;
	}
	else if (LCmd == "Off")
	{
		sPostData << "{\"on\": false }";
		setOn = true;
		On = false;
	}
	else if (LCmd == "Set Level")
	{
		sPostData << R"({"on": true, "bri": )" << svalue << " }";
		setOn = true;
		setLevel = true;
	}
	else if (LCmd == "Set White")
	{
		sPostData << R"({"on": true, "sat": 0 , "bri": 255, "hue": 0 })";
		// Do state update next time the light is polled
	}
	else if (LCmd == "Set Hue")
	{
		sPostData << R"({"on": true, "sat": )" << svalue3 << ", \"hue\": " << svalue2 << ", \"bri\": " << svalue << "  }";
		setOn = true;
		setLevel = true;
		setHueSat = true;
		setMode = true;
		mode = HLMODE_HS;
	}
	else if (LCmd == "Set CT")
	{
		sPostData << R"({"on": true, "ct": )" << svalue2 << ", \"bri\": " << svalue << "  }";
		setOn = true;
		setLevel = true;
		setCt = true;
		setMode = true;
		mode = HLMODE_CT;
	}
	else
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid light command received!");
		return false;
	}

	// Update cached state
	_tHueLightState *pState = nullptr;

	if (nodeID < 1000)
	{
		//Light
		auto && ittLight = m_lights.find(nodeID);
		if (ittLight != m_lights.end())
		{
			pState = &ittLight->second;
		}
	}
	else if (nodeID < 2000)
	{
		//Group
		auto && ittGroup = m_groups.find(nodeID - 1000);
		if (ittGroup != m_groups.end())
		{
			pState = &ittGroup->second.gstate;
		}
	}
	if (pState)
	{
		if (setOn) pState->on = On;
		if (setLevel)
			pState->level = int((100.0F / 254.0F) * float(svalue));
		if (setHueSat) pState->hue = svalue2;
		if (setHueSat) pState->sat = svalue3;
		if (setCt) pState->ct = int((float(svalue2)-153.0)/(500.0-153.0));
		if (setMode) pState->mode = mode;
	}

	std::stringstream sstr2;
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

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT MacAddress FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, nodeID - 2000);
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "Philips Hue: Scene not found!");
			return false;
		}
		sPostData.clear();
		sPostData.str("");
		sPostData << R"({"scene": ")" << result[0][0] << "\"}";
		sstr2 << "http://" << m_IPAddress
			<< ":" << m_Port
			<< "/api/" << m_UserName
			<< "/groups/0/action";
	}
	std::string sURL = sstr2.str();
	if (!HTTPClient::PUT(sURL, sPostData.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Error connecting to Hue bridge (Switch Light/Scene), (Check IPAddress/Username)");
		return false;
	}

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received (Switch Light/Scene), or invalid IPAddress/Username!");
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

std::string CPhilipsHue::RegisterUser(const std::string &IPAddress, const unsigned short Port, const std::string &username)
{
	std::string retStr = "Error;Unknown";
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");
	std::string sResult;
	std::string sPostData;

	//Providing own username is not allowed, so don't use it and only provide devicetype
	sPostData = R"({ "devicetype": "domoticz" })";

	std::stringstream sstr2;
	sstr2 << "http://" << IPAddress
		<< ":" << Port
		<< "/api";
	std::string sURL = sstr2.str();

	if (!HTTPClient::POST(sURL, sPostData, ExtraHeaders, sResult))
	{
		retStr = "Error;Error connecting to Hue bridge:";
		return retStr;
	}

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if (!ret)
	{
		retStr = "Error;Registration failed (Wrong IPAddress?)";
		return retStr;
	}

	if (sResult.find("error") != std::string::npos)
	{
		retStr = "Error;" + root[0]["error"]["description"].asString();
		return retStr;
	}

	std::string new_username = root[0]["success"]["username"].asString();
	retStr = "OK;" + new_username;
	return retStr;
}

void CPhilipsHue::InsertUpdateLamp(const int NodeID, const _eHueLightType LType, const _tHueLightState tstate, const std::string &Name, const std::string &Options, const std::string &modelid, const bool AddMissingDevice)
{
	if (LType == HLTYPE_RGB_W || LType == HLTYPE_CW_WW || LType == HLTYPE_RGB_CW_WW)
	{
		char szID[10];
		char szSValue[20];
		if (NodeID==1)
			sprintf(szID, "%d", NodeID);
		else
			sprintf(szID, "%08X", (unsigned int)NodeID);
		sprintf(szSValue, "%d;%d", tstate.sat, tstate.hue);
		unsigned char unitcode = 1;
		int cmd = (tstate.on ? Color_LedOn : Color_LedOff);
		_tColor color = NoColor;

		unsigned sType;
		switch (LType)
		{
			case HLTYPE_CW_WW:
				sType = sTypeColor_CW_WW;
				break;
			case HLTYPE_RGB_CW_WW:
				sType = sTypeColor_RGB_CW_WW;
				break;
			case HLTYPE_RGB_W:
			default:
				sType = sTypeColor_RGB_W;
				break;
		}

		//Get current nValue if exist
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT nValue, LastLevel, Color, SubType, ID, Used, Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeColorSwitch, szID);

		if (result.empty() && !AddMissingDevice)
			return;

		if (!result.empty())
		{
			//Already in the system
			//Update state
			int nvalue = atoi(result[0][0].c_str());
			unsigned sTypeOld = atoi(result[0][3].c_str());
			std::string sID = result[0][4];
			bool bUsed = std::stoi(result[0][5]) != 0;
			std::string curName = result[0][6];
			if (!bUsed)
			{
				if (curName != Name)
				{
					//Update device name
					_log.Log(LOG_STATUS, "Philips Hue: Updating Name of light '%s' from %s to %s", szID, curName.c_str(), Name.c_str());
					m_sql.UpdateDeviceName(sID, Name);
				}
			}
			if (sTypeOld != sType)
			{
				_log.Log(LOG_STATUS, "Philips Hue: Updating SubType of light '%s' from %u to %u", szID, sTypeOld, sType);
				m_sql.UpdateDeviceValue("SubType", (int)sType, sID);
			}
		}

		if (tstate.on && (tstate.level != 100))
			cmd = Color_SetBrightnessLevel;

		if (tstate.on && (tstate.mode != HLMODE_NONE))
		{
			if (tstate.mode == HLMODE_HS)
			{
				int r, g, b;

				//convert hue+sat to RGB
				float iHue = float(tstate.hue) * 360.0F / 65535.0F;
				float iSat = float(tstate.sat) / 254.0F;
				hsb2rgb(iHue, iSat, 1.0F, r, g, b, 255);

				color = _tColor(r, g, b, 0, 0, ColorModeRGB);
			}
			if (tstate.mode == HLMODE_XY)
			{
				uint8_t r, g, b;

				//convert xy to RGB
				RgbFromXY(tstate.x, tstate.y, 1.0, modelid, r, g, b);

				color = _tColor(r, g, b, 0, 0, ColorModeRGB);
			}
			if (tstate.mode == HLMODE_CT)
			{
				float iCt = (float(tstate.ct) - 153.0F) / (500.0F - 153.0F) * 255.0F;
				color = _tColor(round(iCt), ColorModeTemp);
			}
			cmd = Color_SetColor;
		}

		//Send as ColorSwitch
		_tColorSwitch lcmd;
		lcmd.id = NodeID;
		lcmd.command = cmd;
		lcmd.value = tstate.level;
		lcmd.color = color;
		lcmd.subtype = sType;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, Name.c_str(), 255, m_Name.c_str());

		if (result.empty())
		{
			//Set SwitchType to STYPE_Dimmer
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
				int(STYPE_Dimmer), m_HwdID, szID);
		}
	}
	else if (LType == HLTYPE_SCENE)
	{
		char szID[10];
		sprintf(szID, "%08X", (unsigned int)NodeID);
		unsigned char unitcode = 1;
		int cmd = (tstate.on ? Color_LedOn : Color_LedOff);

		//Get current nValue if exist
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT nValue, ID, Used, Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeColorSwitch, sTypeColor_RGB_W, szID);

		if (result.empty() && !AddMissingDevice)
			return;

		if (!result.empty())
		{
			//Already in the system
			int nvalue = atoi(result[0][0].c_str());
			std::string sID = result[0][1];
			bool bUsed = std::stoi(result[0][2]) != 0;
			std::string curName = result[0][3];
			if (!bUsed)
			{
				if (curName != Name)
				{
					//Update device name
					_log.Log(LOG_STATUS, "Philips Hue: Updating Name of scene '%s' from %s to %s", szID, curName.c_str(), Name.c_str());
					m_sql.UpdateDeviceName(sID, Name);
				}
			}

			bool tIsOn = (nvalue != 0);
			if (tstate.on == tIsOn) //Check if the scene was switched
				return;
		}
		//Send as ColorSwitch
		_tColorSwitch lcmd;
		lcmd.id = NodeID;
		lcmd.command = cmd;
		lcmd.value = 0;
		//lcmd.subtype = sType; // TODO: set type also for groups?
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, Name.c_str(), 255, m_Name.c_str());

		if (result.empty())
		{
			//Set SwitchType to STYPE_PushOn
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
				int(STYPE_Dimmer), m_HwdID, szID);
		}
	}
	else
	{
		//Send as GeneralSwitch
		char szID[10];
		sprintf(szID, "%08X", (unsigned int)NodeID);
		unsigned char unitcode = 1;
		int cmd = (tstate.on ? gswitch_sOn : gswitch_sOff);

		//Check if we already exist
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT nValue, LastLevel, ID, Used, Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeGeneralSwitch, sSwitchGeneralSwitch, szID);
		//_log.Log(LOG_STATUS, "HueBridge state change for DeviceID '%s': Level = %d", szID, tstate.level);

		if (result.empty() && !AddMissingDevice)
			return;

		if (!result.empty())
		{
			//Already in the system

			std::string sID = result[0][2];
			bool bUsed = std::stoi(result[0][3]) != 0;
			std::string curName = result[0][4];

			if (!bUsed)
			{
				if (curName != Name)
				{
					//Update device name
					_log.Log(LOG_STATUS, "Philips Hue: Updating Name of light/switch '%s' from %s to %s", szID, curName.c_str(), Name.c_str());
					m_sql.UpdateDeviceName(sID, Name);
				}
			}

			//Update state
			int nvalue = atoi(result[0][0].c_str());
		}
		else
 		{
 			_log.Log(LOG_STATUS, "Philips Hue: adding device '%s'", Name.c_str());
 		}

		//Change command to SetLevel for dimmer type switch
		if (LType == HLTYPE_DIM && tstate.on && (tstate.level != 100))
			cmd = gswitch_sSetLevel;

		_tGeneralSwitch lcmd;
		lcmd.subtype = sSwitchGeneralSwitch;
		lcmd.id = NodeID;
		lcmd.unitcode = unitcode;
		lcmd.cmnd = cmd;
		lcmd.level = tstate.level;
		lcmd.seqnbr = 1;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, Name.c_str(), 255, m_Name.c_str());

		if (result.empty())
		{
			//Set SwitchType
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
				int(LType == HLTYPE_DIM ? STYPE_Dimmer : STYPE_OnOff), m_HwdID, szID);
		}
	}
}

bool CPhilipsHue::GetStates()
{
	std::string sResult;

#ifdef DEBUG_PhilipsHue
	sResult= ReadFile("E:\\philipshue.json");
#else
	std::stringstream sstr2;
	sstr2 << "http://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName;
	//Get Data
	std::string sURL = sstr2.str();
	std::vector<std::string> ExtraHeaders;
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

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("\"error\":") != std::string::npos)
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

void CPhilipsHue::LightStateFromJSON(const Json::Value &lightstate, _tHueLightState &tlight, _eHueLightType &LType)
{
	if (lightstate.isObject())
	{
		tlight.level = 0;          // Brightness of the light. This is a scale from the minimum brightness the light is capable of, 1,
		                           // to the maximum capable brightness, 254.
		tlight.sat = 0;            // Saturation of the light. 254 is the most saturated (colored) and 0 is the least saturated (white).
		tlight.hue = 0;            // Hue of the light. This is a wrapping value between 0 and 65535.
		tlight.ct = 0;             // The Mired Color temperature of the light. 2012 connected lights are capable of 153 (6500K) to 500 (2000K).
		tlight.mode = HLMODE_NONE; // Indicates the color mode in which the light is working, this is the last command type it received. Values
		                           // are "hs" for Hue and Saturation, "xy" for XY and "ct" for Color Temperature.
		tlight.x = 0.0;            // The x and y coordinates of a color in CIE color space.
		tlight.y = 0.0;            // The first entry is the x coordinate and the second entry is the y coordinate. Both x and y must be between 0 and 1.
		                           // If the specified coordinates are not in the CIE color space, the closest color to the coordinates will be chosen.
		tlight.on = false;

		bool hasBri = false;
		bool hasHueSat = false;
		bool hasTemp = false;

		LType = HLTYPE_NORMAL;

		if (!lightstate["on"].empty())
		{
			tlight.on = lightstate["on"].asBool();
		}
		if (!lightstate["colormode"].empty())
		{
			std::string sMode = lightstate["colormode"].asString();
			if (sMode == "hs") tlight.mode = HLMODE_HS;
			if (sMode == "xy") tlight.mode = HLMODE_XY;
			if (sMode == "ct") tlight.mode = HLMODE_CT;
		}
		if (!lightstate["bri"].empty())
		{
			//Lamp with brightness control
			hasBri = true;
			int tbri = lightstate["bri"].asInt();
			// Clamp to conform to HUE API
			tbri = std::max(1, tbri);
			tbri = std::min(254, tbri);
			tlight.level = int(std::ceil((100.0F / 254.0F) * float(tbri)));
		}
		if (!lightstate["sat"].empty())
		{
			//Lamp with color control
			hasHueSat = true;
			tlight.sat = lightstate["sat"].asInt();
			// Clamp to conform to HUE API
			tlight.sat = std::max(0, tlight.sat);
			tlight.sat = std::min(254, tlight.sat);
		}
		if (!lightstate["hue"].empty())
		{
			//Lamp with color control
			hasHueSat = true;
			tlight.hue = lightstate["hue"].asInt();
			// Clamp to conform to HUE API
			tlight.hue = std::max(0, tlight.hue);
			tlight.hue = std::min(65535, tlight.hue);
		}
		if (!lightstate["ct"].empty())
		{
			//Lamp with color temperature control
			hasTemp = true;
			tlight.ct = lightstate["ct"].asInt();
			// Clamp to conform to HUE API
			tlight.ct = std::max(153, tlight.ct);
			tlight.ct = std::min(500, tlight.ct);
		}
		if (!lightstate["xy"].empty())
		{
			//Lamp with color control
			hasHueSat = true;
			tlight.x = lightstate["xy"][0].asDouble();
			tlight.y = lightstate["xy"][1].asDouble();
		}

		if (hasBri) LType = HLTYPE_DIM;
		if (hasBri && hasHueSat && !hasTemp) LType = HLTYPE_RGB_W;
		if (hasBri && !hasHueSat && hasTemp) LType = HLTYPE_CW_WW;
		if (hasBri && hasHueSat && hasTemp) LType = HLTYPE_RGB_CW_WW;
	}
}

bool CPhilipsHue::GetLights(const Json::Value &root)
{
	if (root["lights"].empty())
		return false;

	for (auto iLight = root["lights"].begin(); iLight != root["lights"].end(); ++iLight)
	{
		Json::Value light = *iLight;
		if (light.isObject())
		{
			int lID = atoi(iLight.key().asString().c_str());

			_tHueLightState tlight;
			_eHueLightType LType;
			bool bDoSend = true;
			LightStateFromJSON(light["state"], tlight, LType);

			auto myLight = m_lights.find(lID);
			if (myLight != m_lights.end())
			{
				if (StatesSimilar(myLight->second, tlight))
					bDoSend = false;
			}
			if (bDoSend)
			{
				//_log.Log(LOG_STATUS, "HueBridge state change: tbri = %d, level = %d", tbri, tlight.level);
				m_lights[lID] = tlight;
				std::string modelid = light["modelid"].asString();
				InsertUpdateLamp(lID, LType, tlight, light["name"].asString(), "", modelid, true);
			}
		}
	}
	return true;
}

bool CPhilipsHue::GetGroups(const Json::Value &root)
{
	//Groups (0=All)

	if (root["groups"].empty())
		return false;

	for (auto iGroup = root["groups"].begin(); iGroup != root["groups"].end(); ++iGroup)
	{
		Json::Value group = *iGroup;
		if (group.isObject())
		{
			int gID = atoi(iGroup.key().asString().c_str());

			_tHueLightState tstate;
			_eHueLightType LType;
			bool bDoSend = true;
			LightStateFromJSON(group["action"], tstate, LType); //TODO: Verify there is no crash with "bad" key

			auto myGroup = m_groups.find(gID);
			if (myGroup != m_groups.end())
			{
				if (StatesSimilar(myGroup->second.gstate, tstate))
					bDoSend = false;
			}
			if (bDoSend)
			{
				m_groups[gID].gstate = tstate;
				std::string Name = "Group " + group["name"].asString();
				InsertUpdateLamp(1000 + gID, LType, tstate, Name, "", "", m_add_groups);
			}
		}
	}
	//Special Request for Group0 (All Lights)
	std::stringstream sstr2;
	sstr2 << "http://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName
		<< "/groups/0";
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	if (!HTTPClient::GET(sstr2.str(), ExtraHeaders, sResult))
	{
		//No group all(0)
		return true;
	}
	Json::Value root2;
	bool ret = ParseJSon(sResult, root2);
	if ((!ret) || (!root2.isObject()))
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "Philips Hue: Error received: %s", root2[0]["error"]["description"].asString().c_str());
		return false;
	}

	if (sResult.find("lights") == std::string::npos)
	{
		return false;
	}
	_tHueLightState tstate;
	tstate.on = false;
	tstate.level = 0;
	tstate.hue = 0;
	tstate.sat = 0;
	tstate.ct = 0;
	tstate.x = 0.0;
	tstate.y = 0.0;
	tstate.mode = HLMODE_NONE;

	_eHueLightType LType = HLTYPE_RGB_W;// HLTYPE_NORMAL;

	if (!root2["action"]["on"].empty())
	{
		tstate.on = root2["action"]["on"].asBool();
		if (tstate.on) tstate.level = 100; // Set default full brightness for non dimmable group
	}
	if (!root2["action"]["bri"].empty())
	{
		int tbri = root2["action"]["bri"].asInt();
		if ((tbri != 0) && (tbri < 3))
			tbri = 3;
		tstate.level = int((100.0F / 254.0F) * float(tbri));
	}
	if (!root2["action"]["sat"].empty())
	{
		tstate.sat = root2["action"]["sat"].asInt();
		//LType = HLTYPE_RGB_W;
	}
	if (!root2["action"]["hue"].empty())
	{
		tstate.hue = root2["action"]["hue"].asInt();
		//LType = HLTYPE_RGB_W;
	}

	int gID = 0;
	std::map<int, _tHueGroup>::iterator myGroup = m_groups.find(gID);
	if (myGroup != m_groups.end())
	{
		if (!StatesSimilar(myGroup->second.gstate, tstate))
		{
			myGroup->second.gstate = tstate;
			std::string Name = "Group All Lights";
			InsertUpdateLamp(1000 + gID, LType, tstate, Name, "", "", m_add_groups);
		}
	}
	return true;
}

bool CPhilipsHue::GetScenes(const Json::Value &root)
{
	if (root["scenes"].empty())
		return false;

	for (auto iScene = root["scenes"].begin(); iScene != root["scenes"].end(); ++iScene)
	{
		Json::Value scene = *iScene;
		if (scene.isObject())
		{
			_tHueScene hscene;
			hscene.id = iScene.key().asString();
			hscene.name = scene["name"].asString();
			hscene.lastupdated = scene["lastupdated"].asString();
			if (hscene.lastupdated.empty())
				continue; //old scene/legacy scene

			//Strip some info
			size_t tpos = hscene.name.find(" from ");
			if (tpos != std::string::npos)
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
				std::vector<std::vector<std::string> > result;
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
				std::string Name = "Scene " + hscene.name;
				_tHueLightState tstate;
				tstate.on = false;
				tstate.level = 100;
				tstate.hue = 0;
				tstate.sat = 0;
				tstate.ct = 0;
				tstate.x = 0.0;
				tstate.y = 0.0;
				tstate.mode = HLMODE_NONE;
				InsertUpdateLamp(2000 + sID, HLTYPE_SCENE, tstate, Name, hscene.id, "", m_add_scenes);
			}
		}
	}
	return true;
}

bool CPhilipsHue::GetSensors(const Json::Value &root)
{
	if (root["sensors"].empty())
		return false;

	for (auto iSensor = root["sensors"].begin(); iSensor != root["sensors"].end(); ++iSensor)
	{
		Json::Value sensor = *iSensor;
		if (sensor.isObject())
		{
			bool bNewSensor = false;
			int sID = atoi(iSensor.key().asString().c_str());

			CPHSensor current_sensor(sensor);
			CPHSensor previous_sensor;
			// Check if sensor exists and whether last update is changed
			if (m_sensors.find(sID) != m_sensors.end())
			{
				previous_sensor = m_sensors[sID];
				if (previous_sensor.m_state == current_sensor.m_state)
				{
					//Nothing changed
					continue;
				}
			}
			else
			{
				// New sensor found, always update it's value
				previous_sensor = CPHSensor();
				bNewSensor = true;
			}
			m_sensors[sID] = current_sensor;
			uint8_t unitcode = 1;

			sID += 3000;
			std::string device_name = current_sensor.m_type + " " + current_sensor.m_name;
			if (current_sensor.m_type == SensorTypeDaylight)
			{
			}
			else if (
				(current_sensor.m_type == SensorTypeZGPSwitch)
					|| (current_sensor.m_type == SensorTypeZLLSwitch))
			{
				int32_t selectorLevel = current_sensor.m_state.GetSelectorLevel(previous_sensor.m_state);
				if (selectorLevel >= 0)
				{
					if (InsertUpdateSelectorSwitch(sID, unitcode, selectorLevel, device_name, current_sensor.m_config.m_battery))
					{
						//New switch. Set levels and options for selector
						SetSwitchOptions(sID, unitcode, current_sensor.m_state.GetButtonOptions());
					}
				}
			}
			else if (current_sensor.m_type == SensorTypeZLLPresence)
			{
				if ((previous_sensor.m_state.m_presence != current_sensor.m_state.m_presence)
					|| (bNewSensor))
				{
					InsertUpdateSwitch(sID, unitcode, STYPE_Motion, current_sensor.m_state.m_presence, device_name, current_sensor.m_config.m_battery);
				}
			}
			else if (current_sensor.m_type == SensorTypeGeofence)
			{
				if ((previous_sensor.m_state.m_presence != current_sensor.m_state.m_presence)
					|| (bNewSensor))
				{
					InsertUpdateSwitch(sID, unitcode, STYPE_Motion, current_sensor.m_state.m_presence, device_name, current_sensor.m_config.m_battery);
				}
			}
			else if (current_sensor.m_type == SensorTypeZLLTemperature)
			{
				if ((previous_sensor.m_state.m_temperature != current_sensor.m_state.m_temperature)
					|| (bNewSensor))
				{
					SendTempSensor(sID, current_sensor.m_config.m_battery, float(current_sensor.m_state.m_temperature / 100.0F), device_name);
				}
			}
			else if (current_sensor.m_type == SensorTypeZLLLightLevel)
			{
				if ((previous_sensor.m_state.m_dark != current_sensor.m_state.m_dark)
					|| (bNewSensor))
				{
					InsertUpdateSwitch(sID, unitcode, STYPE_Dusk, current_sensor.m_state.m_dark, device_name, current_sensor.m_config.m_battery);
				}

				if ((previous_sensor.m_state.m_lightlevel != current_sensor.m_state.m_lightlevel)
					|| (bNewSensor))
				{
					double lux = 0.00001;
					if (current_sensor.m_state.m_lightlevel != 0)
					{
						float convertedLightLevel = float((current_sensor.m_state.m_lightlevel - 1) / 10000.00F);
						lux = pow(10, convertedLightLevel);
					}
					SendLuxSensor(sID, 0, current_sensor.m_config.m_battery, (const float)lux, current_sensor.m_type + " Lux " + current_sensor.m_name);
				}
			}
			else
			{
				//_log.Log(LOG_STATUS, "Ignoring Philips Hue CLIP Sensors: (%s)", device_name.c_str());
			}
		}
	}

	return true;
}

bool CPhilipsHue::InsertUpdateSelectorSwitch(const int NodeID, const uint8_t Unitcode, const uint8_t selectorLevel, const std::string& Name, const uint8_t BatteryLevel)
{
	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = NodeID;
	xcmd.unitcode = 1;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchTypeSelector;
	xcmd.battery_level = BatteryLevel;
	xcmd.level = selectorLevel;
	xcmd.rssi = 12;
	xcmd.cmnd = gswitch_sSetLevel;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, NodeID, xcmd.unitcode);
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, Name.c_str(), BatteryLevel, m_Name.c_str());
	if (result.empty())
	{
		//_log.Log(LOG_STATUS, "Philips Hue Switch: New Device Found (%s)", Name.c_str());
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%08X') AND (Unit == '%d')", Name.c_str(), (STYPE_Selector), 0, m_HwdID, NodeID, xcmd.unitcode);
		return true;
	}
	return false;
}

void CPhilipsHue::InsertUpdateSwitch(const int NodeID, const uint8_t Unitcode, const _eSwitchType SType, const bool bIsOn, const std::string &Name, const uint8_t BatteryLevel)
{
	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = NodeID;
	xcmd.unitcode = Unitcode;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch;
	xcmd.battery_level = BatteryLevel;
	xcmd.rssi = 12;

	if (bIsOn) 
		xcmd.cmnd = gswitch_sOn;
	else 
		xcmd.cmnd = gswitch_sOff;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, NodeID, xcmd.unitcode);
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, Name.c_str(), BatteryLevel, m_Name.c_str());
	if (result.empty())
	{
		//_log.Log(LOG_STATUS, "Philips Hue Switch: New Device Found (%s)", Name.c_str());
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%08X') AND (Unit == '%d')", Name.c_str(), (SType), 0, m_HwdID, NodeID, xcmd.unitcode);
	}
}

void CPhilipsHue::SetSwitchOptions(const int NodeID, const uint8_t Unitcode, const std::map<std::string, std::string> &options)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, NodeID, Unitcode);
	if (!result.empty())
	{
		int sIdx = std::stoi(result[0][0]);
		m_sql.SetDeviceOptions(sIdx, options);
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

			std::string sipaddress = request::findValue(&req, "ipaddress");
			std::string sport = request::findValue(&req, "port");
			std::string susername = request::findValue(&req, "username");
			if ((sipaddress.empty()) || (sport.empty()))
				return;

			std::string sresult = CPhilipsHue::RegisterUser(sipaddress, (unsigned short)atoi(sport.c_str()), susername);
			std::vector<std::string> strarray;
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
	} // namespace server
} // namespace http
