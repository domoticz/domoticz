#include "stdafx.h"
#include "PhilipsHue.h"
#include "PhilipsHueSensors.h"
#include "PhilipsHueV2Sensors.h"
#include "../../main/Helper.h"
#include "../../main/Logger.h"
#include "../../main/RFXtrx.h"
#include "../../main/SQLHelper.h"
#include "../../main/mainworker.h"
#include "../../main/WebServer.h"
#include <libwebem/cWebem.h>
#include "../../httpclient/HTTPClient.h"
#include "../../main/json_helper.h"
#include "../hardwaretypes.h"
#include <curl/curl.h>

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

#define SAFE_TYPE_CHECK(obj, check, fieldname) \
    do { \
        if (!((obj).check())) { \
            _log.Log(LOG_DEBUG_INT, "PhilipsHue: JSON field '%s' has unexpected type", (fieldname)); \
        } \
    } while (0)

#ifdef _DEBUG
//#define DEBUG_PhilipsHue_R
//#define DEBUG_PhilipsHue_W
#endif

#ifdef DEBUG_PhilipsHue_W
extern void SaveString2Disk(std::string str, std::string filename);
#endif
#ifdef DEBUG_PhilipsHue_R
std::string ReadFile(std::string filename)
#endif

#ifdef DEBUG_PhilipsHue_W
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif

#ifdef DEBUG_PhilipsHue_R
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

CPhilipsHue::CPhilipsHue(const int ID, const std::string& IPAddress, const unsigned short Port, const std::string& Username, const int PollInterval, const int Options) :
	m_IPAddress(IPAddress),
	m_UserName(Username)
{
	m_HwdID = ID;
	m_Port = Port;
	m_poll_interval = PollInterval;
	if (m_poll_interval < 5)
	{
		m_poll_interval = HUE_DEFAULT_POLL_INTERVAL;
		Log(LOG_STATUS, "Poll interval too short. Using default interval of %d seconds.", m_poll_interval);
	}
	else
	{
		Log(LOG_STATUS, "Using poll interval of %d seconds.", m_poll_interval);
	}

	m_add_groups = (Options & HUE_NOT_ADD_GROUPS) != 0;
	m_add_scenes = (Options & HUE_NOT_ADD_SCENES) != 0;
}

bool CPhilipsHue::CheckIsV2Bridge()
{
	std::string url = "https://" + m_IPAddress + "/clip/v2/resource";
	CURL* curl = curl_easy_init();
	if (!curl)
		return false;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	CURLcode res = curl_easy_perform(curl);
	long httpCode = 0;
	if (res == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
	curl_easy_cleanup(curl);
	_log.Debug(DEBUG_HARDWARE, "PhilipsHue: CheckIsV2Bridge HTTP %ld from %s (403 = V2)", httpCode, url.c_str());
	return (httpCode == 403);
}

bool CPhilipsHue::CheckV1Bridge(int port)
{
	const std::string schema = (port == 443) ? "https" : "http";
	std::string url = schema + "://" + m_IPAddress + ":" + std::to_string(port) + "/api/";
	CURL* curl = curl_easy_init();
	if (!curl)
		return false;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	CURLcode res = curl_easy_perform(curl);
	long httpCode = 0;
	if (res == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
	curl_easy_cleanup(curl);
	_log.Debug(DEBUG_HARDWARE, "PhilipsHue: CheckV1Bridge port %d HTTP %ld (200/405 = V1)", port, httpCode);
	return (httpCode == 200 || httpCode == 405);
}

bool CPhilipsHue::Init()
{
	m_use_v2_sensors = false;
	m_html_schema = "http";

	if (CheckIsV2Bridge())
	{
		m_html_schema = "https";
		m_Port = 443;
		try {
			m_v2sensors = std::make_unique<CPhilipsHueV2Sensors>(m_html_schema, m_IPAddress, std::to_string(m_Port), m_UserName);
			m_use_v2_sensors = true;
			Log(LOG_STATUS, "V2 bridge detected, using V2 API.");
		}
		catch (const std::exception& e) {
			Log(LOG_ERROR, "Failed to create V2 sensors helper: %s", e.what());
			return false;
		}
		return true;
	}

	if (CheckV1Bridge(443))
	{
		m_html_schema = "https";
		m_Port = 443;
		Log(LOG_STATUS, "V1 bridge detected on port 443 (HTTPS).");
		return true;
	}

	if (CheckV1Bridge(80))
	{
		m_html_schema = "http";
		m_Port = 80;
		Log(LOG_STATUS, "V1 bridge detected on port 80 (HTTP).");
		return true;
	}

	Log(LOG_ERROR, "No Philips Hue bridge found at %s (tried V2 HTTPS/443, V1 HTTPS/443, V1 HTTP/80). Check IP address and network connectivity.", m_IPAddress.c_str());
	return false;
}

bool CPhilipsHue::StartHardware()
{
	RequestStart();

	if (!Init())
		return false;

	if (m_use_v2_sensors && m_v2sensors)
	{
		{
			std::lock_guard<std::mutex> lock(m_http_mutex);
			GetV2Sensors();
		}

		m_sse = std::make_unique<CPhilipsHueSSE>(
			m_IPAddress,
			std::to_string(m_Port),
			m_UserName,
			[this](const std::string& data) { OnSSEEvent(data); }
		);
		m_sse->Start();
	}

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CPhilipsHue::StopHardware()
{
	if (m_sse)
	{
		m_sse->Stop();
		m_sse.reset();
	}
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CPhilipsHue::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = m_poll_interval - 1;

	Log(LOG_STATUS, "Worker started...");

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
			if (m_use_v2_sensors && m_v2sensors && sec_counter % 600 == 0)
			{
				std::lock_guard<std::mutex> lock(m_http_mutex);
				GetV2Sensors();
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool CPhilipsHue::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	try
	{
		const tRBUF* pSen = reinterpret_cast<const tRBUF*>(pdata);

		unsigned char packettype = pSen->ICMND.packettype;

		int svalue = 0;
		int svalue2 = 0;
		int svalue3 = 0;
		std::string LCmd;
		int nodeID = 0;

		if (packettype == pTypeGeneralSwitch)
		{
			const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pSen);
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
				svalue = ground(fvalue);
			}
			SwitchLight(nodeID, LCmd, svalue);
		}
		else if (packettype == pTypeColorSwitch)
		{
			const _tColorSwitch* pLed = reinterpret_cast<const _tColorSwitch*>(pSen);
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
					svalue = ground(fvalue);
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
					svalue2 = ground(float(pLed->color.t) * (500.0F - 153.0F) / 255.0F + 153.0F);
				}
				else if (pLed->color.mode == ColorModeRGB)
				{
					//get the xy color
					double x = 0;
					double y = 0;
					RgbToXY(m_lightModels[nodeID], pLed->color.r, pLed->color.g, pLed->color.b, x, y);
					LCmd = "Set XY";
					svalue2 = ground(x * 1000);
					svalue3 = ground(y * 1000);

					/*
					float hsb[3];
					rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
					float cHue = (65535.0F) * hsb[0]; // Scale hue from 0..1 to 0..65535
					float cSat = (254.0F) * hsb[1];	  // Scale saturation from 0..1 to 0..254
					LCmd = "Set Hue";
					svalue2 = ground(cHue);
					svalue3 = ground(cSat);
					*/
				}
				else {
					Log(LOG_STATUS, "SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
				}
				float fvalue = (254.0F / 100.0F) * float(pLed->value);
				if (fvalue > 254.0F)
					fvalue = 254.0F;
				svalue = ground(fvalue);
				SwitchLight(nodeID, LCmd, svalue, svalue2, svalue3);
				return true;
			}
		}
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "WriteToHardware Error: %s", e.what());
		return false;
	}
	return true;
}

std::string hue_errorDescription(const Json::Value& root)
{
	if (root.isObject() && root.isMember("error"))
		return root["error"]["description"].asString();

	if (root.isArray() && !root.empty() &&
		root[0].isObject() && root[0].isMember("error"))
		return root[0]["error"]["description"].asString();

	return "Unknown!?"; // layout not recognised
}

bool CPhilipsHue::SwitchLight(const int nodeID, const std::string& LCmd, const int svalue, const int svalue2 /*= 0*/, const int svalue3 /*= 0*/)
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

	if (LCmd == "On")
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
	else if (LCmd == "Set XY")
	{
		sPostData << R"({"on": true, "xy": [)" << ((double)svalue2 / 1000) << ", " << ((double)svalue3 / 1000) << " ], \"bri\": " << svalue << "  }";
		setOn = true;
		setLevel = true;
		setHueSat = true;
		setMode = true;
		mode = HLMODE_HS;
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
		Log(LOG_ERROR, "Invalid light command received!");
		return false;
	}

	// Update cached state
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		_tHueLightState* pState = nullptr;

		if (nodeID < 1000)
		{
			//Light
			auto&& ittLight = m_lights.find(nodeID);
			if (ittLight != m_lights.end())
			{
				pState = &ittLight->second;
			}
		}
		else if (nodeID < 2000)
		{
			//Group
			auto&& ittGroup = m_groups.find(nodeID - 1000);
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
			if (setCt) pState->ct = int((float(svalue2) - 153.0) / (500.0 - 153.0));
			if (setMode) pState->mode = mode;
		}
	}

	std::stringstream sstr2;
	if (nodeID < 1000)
	{
		//Light
		sstr2 << m_html_schema << "://" << m_IPAddress
			<< ":" << m_Port
			<< "/api/" << m_UserName
			<< "/lights/" << nodeID << "/state";
	}
	else if (nodeID < 2000)
	{
		//Group
		sstr2 << m_html_schema << "://" << m_IPAddress
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
			Log(LOG_ERROR, "Scene not found!");
			return false;
		}
		sPostData.clear();
		sPostData.str("");
		sPostData << R"({"scene": ")" << result[0][0] << "\"}";
		sstr2 << m_html_schema << "://" << m_IPAddress
			<< ":" << m_Port
			<< "/api/" << m_UserName
			<< "/groups/0/action";
	}
	std::string sURL = sstr2.str();
	{
		std::lock_guard<std::mutex> lock(m_http_mutex);
		if (!HTTPClient::PUT(sURL, sPostData.str(), ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "Error sending command to Hue bridge (Switch Light/Scene)");
			return false;
		}
	}
#ifdef DEBUG_PhilipsHue_W
	SaveString2Disk(sResult, urlToFilename("PhilipsHue", sURL));
#endif

	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if (!ret)
	{
		Log(LOG_ERROR, "Error sending switch command. Invalid (json) data retuned.");
		return false;
	}

	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		Log(LOG_ERROR, "SwitchLight Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}

	return true;
}

std::string CPhilipsHue::RegisterUser(const std::string& IPAddress, const unsigned short Port, const std::string& username)
{
	std::string retStr = "Error;Unknown";
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");
	std::string sResult;
	std::string sPostData;

	//Providing own username is not allowed, so don't use it and only provide devicetype
	sPostData = R"({ "devicetype": "domoticz" })";

	std::stringstream sstr2;

	std::string html_schema;
	if (Port == 443)
		html_schema = "https";
	else
		html_schema = "http";

	sstr2 << html_schema << "://" << IPAddress
		<< ":" << Port
		<< "/api";
	std::string sURL = sstr2.str();

	if (!HTTPClient::POST(sURL, sPostData, ExtraHeaders, sResult))
	{
		retStr = "Error;Error connecting to Hue bridge:";
		return retStr;
	}
#ifdef DEBUG_PhilipsHue_W
	SaveString2Disk(sResult, urlToFilename("PhilipsHue", sURL));
#endif

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if (!ret)
	{
		retStr = "Error;Registration failed (Wrong IPAddress?)";
		return retStr;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		retStr = "Error;" + hue_errorDescription(root);
	 	return retStr;
	 }

	if (root.isArray() && root.size() > 0 && root[0].isObject() && root[0].isMember("success")) {
		std::string new_username = root[0]["success"]["username"].asString();
		retStr = "OK;" + new_username;
	} else {
		retStr = "Error;Unexpected response format (no success node)";
	}

	return retStr;
}

void CPhilipsHue::InsertUpdateLamp(const int NodeID, const _eHueLightType LType, const _tHueLightState tstate, const std::string& Name, const std::string& Options, const std::string& modelid, const bool AddMissingDevice)
{
	if (LType == HLTYPE_RGB_W || LType == HLTYPE_CW_WW || LType == HLTYPE_RGB_CW_WW)
	{
		char szID[10];
		char szSValue[20];
		if (NodeID == 1)
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
					Log(LOG_STATUS, "Updating Name of light '%s' from %s to %s", szID, curName.c_str(), Name.c_str());
					m_sql.UpdateDeviceName(sID, Name);
				}
			}
			if (sTypeOld != sType)
			{
				Log(LOG_STATUS, "Updating SubType of light '%s' from %u to %u", szID, sTypeOld, sType);
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
				color = _tColor(ground(iCt), ColorModeTemp);
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
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&lcmd, Name.c_str(), 255, m_Name.c_str());

		if (result.empty())
		{
			//Set SwitchType to STYPE_Dimmer
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (HardwareID == %d) AND (DeviceID == '%q')",
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
					Log(LOG_STATUS, "Updating Name of scene '%s' from %s to %s", szID, curName.c_str(), Name.c_str());
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
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&lcmd, Name.c_str(), 255, m_Name.c_str());

		if (result.empty())
		{
			//Set SwitchType to STYPE_PushOn
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (HardwareID == %d) AND (DeviceID == '%q')",
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
		//Log(LOG_STATUS, "HueBridge state change for DeviceID '%s': Level = %d", szID, tstate.level);

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
					Log(LOG_STATUS, "Updating Name of light/switch '%s' from %s to %s", szID, curName.c_str(), Name.c_str());
					m_sql.UpdateDeviceName(sID, Name);
				}
			}

			//Update state
			int nvalue = atoi(result[0][0].c_str());
		}
		else
		{
			Log(LOG_STATUS, "adding device '%s'", Name.c_str());
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
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&lcmd, Name.c_str(), 255, m_Name.c_str());

		if (result.empty())
		{
			//Set SwitchType
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (HardwareID == %d) AND (DeviceID == '%q')",
				int(LType == HLTYPE_DIM ? STYPE_Dimmer : STYPE_OnOff), m_HwdID, szID);
		}
	}
}

static bool hue_http_get(const std::string& url, const std::vector<std::string>& ExtraHeaders, std::string& outBody)
{
#ifdef DEBUG_PhilipsHue_R
	outBody = ReadFile(urlToFilename("PhilipsHue", url));
#endif
	bool ret = HTTPClient::GET(url, ExtraHeaders, outBody);
#ifdef DEBUG_PhilipsHue_W
	SaveString2Disk(outBody, urlToFilename("PhilipsHue", url));
#endif
	return ret;
}

bool CPhilipsHue::GetStates()
{
	std::string sResult;
	std::stringstream sstr2;
	sstr2 << m_html_schema << "://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName;
	//Get Data
	std::string sURL = sstr2.str();
	std::vector<std::string> ExtraHeaders;
	{
		std::lock_guard<std::mutex> lock(m_http_mutex);
		if (!hue_http_get(sURL, ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "Error getting Light States, (Check IPAddress/Username)");
			return false;
		}
	}
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if (!ret) 
	{
		Log(LOG_ERROR, "Invalid data received! (login/json)");
		return false;
	}
	if (sResult.find("\"error\":") != std::string::npos)
	{
		//We had an error
		Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
		return false;
	}

	if (!root.isObject())
	{
		Log(LOG_ERROR, "Invalid data received! (unknown json payload)");
		return false;
	}

	try
	{
		if (!GetLights(root))
		{
			//Log(LOG_ERROR, "No Lights found!");
			return false;
		}
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Error processing Lights: %s", e.what());
	}
	try
	{
		GetGroups(root);
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Error processing Groups: %s", e.what());
	}
	try
	{
		GetScenes(root);
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Error processing Scenes: %s", e.what());
	}
	try
	{
		GetSensors(root);
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Error processing Sensors: %s", e.what());
	}
	if (m_use_v2_sensors && m_v2sensors && (!m_sse || !m_sse->IsConnected()))
	{
		try
		{
			std::lock_guard<std::mutex> lock(m_http_mutex);
			GetV2Sensors();
		}
		catch (const std::exception& e)
		{
			Log(LOG_ERROR, "Error processing V2 Sensors: %s", e.what());
		}
	}

	return true;
}

// NEW: deterministic conversion from v2 uuid/rid string to numeric node ID
int CPhilipsHue::NodeIDFromRid(const std::string& rid)
{
	// Stable non-cryptographic hash -> 32-bit int. Guaranteed deterministic across restarts.
	std::hash<std::string> hasher;
	uint32_t h = static_cast<uint32_t>(hasher(rid));
	// Keep it in a safe positive range; mask off sign bit
	const uint32_t safe = h & 0x7FFFFFFF;
	// Limit to a range so NodeIDs are not ridiculously large (optional)
	return static_cast<int>(safe % 2000000); // produces 0 .. 1_999_999
}

void CPhilipsHue::LightStateFromJSON(const Json::Value& lightstate, _tHueLightState& tlight, _eHueLightType& LType)
{
	if (!lightstate.isObject())
		return;

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

bool CPhilipsHue::GetLights(const Json::Value& root)
{
	if (root["lights"].empty())
		return false;

	for (auto iLight = root["lights"].begin(); iLight != root["lights"].end(); ++iLight)
	{
		Json::Value light = *iLight;
		if (!light.isObject())
			continue;
		int lID = atoi(iLight.key().asString().c_str());

		_tHueLightState tlight;
		_eHueLightType LType;
		bool bDoSend = true;
		try {
			if (!light["state"].empty())
			{
				LightStateFromJSON(light["state"], tlight, LType);
			}
		}
		catch (const std::exception& ex) {
			Log(LOG_ERROR, "PhilipsHue: exception parsing light %s state: %s. Skipping this light.", iLight.key().asString().c_str(), ex.what());
			continue;
		}
		catch (...) {
			Log(LOG_ERROR, "PhilipsHue: unknown exception parsing light %s state. Skipping this light.", iLight.key().asString().c_str());
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto myLight = m_lights.find(lID);
			if (myLight != m_lights.end())
			{
				if (StatesSimilar(myLight->second, tlight))
					bDoSend = false;
			}
		}
		if (bDoSend)
		{
			//Log(LOG_STATUS, "HueBridge state change: tbri = %d, level = %d", tbri, tlight.level);
			if (!light["modelid"].empty())
			{
				std::string modelid = light["modelid"].asString();
				{
					std::lock_guard<std::mutex> lock(m_mutex);
					m_lights[lID] = tlight;
					m_lightModels[lID] = modelid;
				}
				InsertUpdateLamp(lID, LType, tlight, light["name"].asString(), "", modelid, true);
			}
		}
	}
	return true;
}

bool CPhilipsHue::GetGroups(const Json::Value& root)
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

			if (!group["action"].empty())
			{
				try {
					LightStateFromJSON(group["action"], tstate, LType);
				}
				catch (const std::exception& e) {
					Log(LOG_ERROR, "Exception while parsing Hue group %d action: %s", gID, e.what());
					continue;
				}
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto myGroup = m_groups.find(gID);
				if (myGroup != m_groups.end())
				{
					if (StatesSimilar(myGroup->second.gstate, tstate))
						bDoSend = false;
				}
			}
			if (bDoSend)
			{
				std::string oname = (!group["name"].empty()) ? group["name"].asString() : "??";
				if (m_use_v2_sensors && m_v2sensors && (oname.empty() || oname == "??" || oname.rfind("Group", 0) == 0))
				{
					std::string v2name = m_v2sensors->GetRoomNameByV1GroupId(gID);
					if (!v2name.empty())
						oname = v2name;
				}
				std::string Name = "Group " + oname;
				{
					std::lock_guard<std::mutex> lock(m_mutex);
					m_groups[gID].gstate = tstate;
				}
				InsertUpdateLamp(1000 + gID, LType, tstate, Name, "", "", m_add_groups);
			}
		}
	}
	//Special Request for Group0 (All Lights)
	std::stringstream sstr2;
	sstr2 << m_html_schema << "://" << m_IPAddress
		<< ":" << m_Port
		<< "/api/" << m_UserName
		<< "/groups/0";
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	if (!hue_http_get(sstr2.str(), ExtraHeaders, sResult))
	{
		//No group all(0)
		return true;
	}
	Json::Value root2;
	bool ret = ParseJSon(sResult, root2);
	if ((!ret) || (!root2.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received, or invalid IPAddress/Username!");
		return false;
	}

	 if (sResult.find("\"error\":") != std::string::npos)
	 {
	 	//We had an error
		Log(LOG_ERROR, "Error received: %s", hue_errorDescription(root).c_str());
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

	if (!root2["action"].empty())
	{
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
	}

	int gID = 0;
	bool bDoSendGroup0 = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::map<int, _tHueGroup>::iterator myGroup = m_groups.find(gID);
		if (myGroup != m_groups.end())
		{
			if (!StatesSimilar(myGroup->second.gstate, tstate))
			{
				myGroup->second.gstate = tstate;
				bDoSendGroup0 = true;
			}
		}
	}
	if (bDoSendGroup0)
	{
		std::string Name = "Group All Lights";
		InsertUpdateLamp(1000 + gID, LType, tstate, Name, "", "", m_add_groups);
	}
	else {
		//Should we add Group 0? Seems we are not doing anything with Group 0
	}
	return true;
}

bool CPhilipsHue::GetScenes(const Json::Value& root)
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
						Log(LOG_ERROR, "Problem adding new Scene!!");
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

static int ParseV1NumericId(const std::string& id_v1)
{
	if (id_v1.empty())
		return -1;
	auto pos = id_v1.rfind('/');
	if (pos == std::string::npos || pos + 1 >= id_v1.size())
		return -1;
	try { return std::stoi(id_v1.substr(pos + 1)); }
	catch (...) { return -1; }
}

void CPhilipsHue::LogV2MigrationWarning(const std::string& ownerRid, int v1NodeID, int hashNodeID, const std::string& friendlyName)
{
	if (v1NodeID == hashNodeID)
		return;
	if (m_v2_migration_warned.find(ownerRid) != m_v2_migration_warned.end())
		return;
	Log(LOG_STATUS, "PhilipsHue: Sensor '%s' now uses V1-compatible ID %d (was %d). Old device can be removed from Settings > Devices.",
		friendlyName.c_str(), v1NodeID, hashNodeID);
	m_v2_migration_warned.insert(ownerRid);
}

bool CPhilipsHue::GetV2Sensors()
{
	// call v2 sensors UpdateAll() and map to Domoticz devices (change-aware)
	if (!m_use_v2_sensors || !m_v2sensors)
		return false;
	
	try
	{
		if (m_v2sensors->UpdateAll())
		{
			// Build map device id -> device data for name lookup
			std::map<std::string, HueV2Device> deviceById;
			for (const auto& d : m_v2sensors->GetDevices())
				deviceById[d.id] = d;

			// Map contact resources to Domoticz switches (one device per owner/device), but only when changed
			for (const auto& c : m_v2sensors->GetContacts())
			{
				const std::string ownerRid = c.owner_rid;
				if (ownerRid.empty())
					continue;

				// Name lookup
				std::string friendlyName;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					friendlyName = dit->second.name + " (Contact)";
				else
					friendlyName = "Hue V2 Contact";

				// Compute Domoticz node for device (owner-based)
				int nodeID = NodeIDFromRid(ownerRid);
				int domoticzNodeID = nodeID + 3000;

				// Check previous state: skip if state and changed timestamp are identical to last poll
				bool doUpdate = true;
				auto itPrevState = m_v2_contact_state.find(ownerRid);
				auto itPrevChanged = m_v2_contact_changed.find(ownerRid);
				if (itPrevState != m_v2_contact_state.end() && itPrevChanged != m_v2_contact_changed.end())
				{
					if (itPrevState->second == c.state && itPrevChanged->second == c.changed)
						doUpdate = false;
				}

				if (!doUpdate)
					continue;

				// contact_report.state: "contact" => closed (not alarmed). "no_contact" => open
				bool isOpen = (c.state == "no_contact");

				// Update/create Domoticz device and record state
				InsertUpdateSwitch(domoticzNodeID, 1, STYPE_DoorContact, isOpen, friendlyName, 0);

				// Store last-seen state and changed timestamp
				m_v2_contact_state[ownerRid] = c.state;
				m_v2_contact_changed[ownerRid] = c.changed;
			}

			// Map tamper resources similarly, only update when changed
			for (const auto& t : m_v2sensors->GetTampers())
			{
				const std::string ownerRid = t.owner_rid;
				if (ownerRid.empty())
					continue;

				std::string friendlyName;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					friendlyName = dit->second.name + " (Tamper)";
				else
					friendlyName = "Hue V2 Tamper";

				int nodeID = NodeIDFromRid(ownerRid);
				int domoticzNodeID = nodeID + 4000;

				// If there is no tamper report changed timestamp and no state, treat as no change
				bool doUpdate = true;
				auto itPrevState = m_v2_tamper_state.find(ownerRid);
				auto itPrevChanged = m_v2_tamper_changed.find(ownerRid);
				if (itPrevState != m_v2_tamper_state.end() && itPrevChanged != m_v2_tamper_changed.end())
				{
					if (itPrevState->second == t.state && itPrevChanged->second == t.changed)
						doUpdate = false;
				}

				if (!doUpdate)
					continue;

				bool isTampered = (t.state == "tampered");

				InsertUpdateSwitch(domoticzNodeID, 1, STYPE_OnOff, isTampered, friendlyName, 0);

				m_v2_tamper_state[ownerRid] = t.state;
				m_v2_tamper_changed[ownerRid] = t.changed;
			}

			// Map device_power to battery updates; only update existing devices and only when battery level changed
			// Note: m_v2_battery_level is always accessed under m_http_mutex (held by all callers of GetV2Sensors
			// and by OnSSEEvent before dispatching SSE handlers), so no additional lock is needed here.
			for (const auto& p : m_v2sensors->GetDevicePowers())
			{
				const std::string ownerRid = p.owner_rid;
				if (ownerRid.empty())
					continue;

				// Only update battery for devices discovered in v2 device list
				auto dit = deviceById.find(ownerRid);
				if (dit == deviceById.end())
					continue;

				// Determine numeric batteryLevel: prefer explicit numeric; otherwise map battery_state heuristically.
				int batteryLevel = p.battery_level; // parsed (-1 if missing)
				if (batteryLevel < 0)
				{
					if (p.battery_state == "full" || p.battery_state == "normal")
						batteryLevel = 100;
					else if (p.battery_state == "high")
						batteryLevel = 90;
					else if (p.battery_state == "low")
						batteryLevel = 20;
					else if (p.battery_state == "critical")
						batteryLevel = 5;
					else
						batteryLevel = 255; // unknown
				}
				if (batteryLevel > 100 && batteryLevel != 255)
					batteryLevel = 100;
				if (batteryLevel == 255)
					continue;
				m_v2_battery_level[ownerRid] = batteryLevel;

				// Determine the Domoticz node IDs (owner-based)
				int baseNode = NodeIDFromRid(ownerRid);
				int domoticzNodeID_contact = baseNode + 3000;
				int domoticzNodeID_tamper = baseNode + 4000;

				// Only update contact device if it exists; update only when battery changed
				char szID[16];
				std::vector<std::vector<std::string>> result;

				snprintf(szID, sizeof(szID), "%08X", domoticzNodeID_contact);
				result = m_sql.safe_query("SELECT ID, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d)",
					m_HwdID, szID, 1);
				if (!result.empty())
				{
					int deviceRowId = atoi(result[0][0].c_str());
					int prevBattery = atoi(result[0][1].c_str());
					if (prevBattery != batteryLevel)
					{
						// preserve current nValue for push
						int nValue = 0;
						auto res2 = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE ID=%d", deviceRowId);
						if (!res2.empty()) nValue = atoi(res2[0][0].c_str());
						bool contactIsOn = (nValue != 0);
						std::string contactName = (dit != deviceById.end() && !dit->second.name.empty()) ? (dit->second.name + " (Contact)") : "Hue V2 Contact";

						InsertUpdateSwitch(domoticzNodeID_contact, 1, STYPE_DoorContact, contactIsOn, contactName, (uint8_t)batteryLevel);
						m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d, LastUpdate=datetime('now','localtime') WHERE ID=%d", batteryLevel, deviceRowId);
						m_v2_battery_level[ownerRid] = batteryLevel;
					}
				}

				// Tamper device (same approach)
				snprintf(szID, sizeof(szID), "%08X", domoticzNodeID_tamper);
				result = m_sql.safe_query("SELECT ID, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d)",
					m_HwdID, szID, 1);
				if (!result.empty())
				{
					int deviceRowId = atoi(result[0][0].c_str());
					int prevBattery = atoi(result[0][1].c_str());
					if (prevBattery != batteryLevel)
					{
						int nValue = 0;
						auto res2 = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE ID=%d", deviceRowId);
						if (!res2.empty()) nValue = atoi(res2[0][0].c_str());
						bool tamperIsOn = (nValue != 0);
						std::string tamperName = (dit != deviceById.end() && !dit->second.name.empty()) ? (dit->second.name + " (Tamper)") : "Hue V2 Tamper";

						InsertUpdateSwitch(domoticzNodeID_tamper, 1, STYPE_OnOff, tamperIsOn, tamperName, (uint8_t)batteryLevel);
						m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d, LastUpdate=datetime('now','localtime') WHERE ID=%d", batteryLevel, deviceRowId);
						m_v2_battery_level[ownerRid] = batteryLevel;
					}
				}
			}

			for (const auto& mo : m_v2sensors->GetMotions())
			{
				const std::string& ownerRid = mo.owner_rid;
				if (ownerRid.empty())
					continue;

				int v1Num = ParseV1NumericId(mo.id_v1);
				int nodeID;
				if (v1Num >= 0)
					nodeID = v1Num + 3000;
				else
					nodeID = NodeIDFromRid(ownerRid) + 3000;

				std::string name;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					name = dit->second.name + " (Motion)";
				else
					name = "Hue Motion";

				if (v1Num >= 0)
					LogV2MigrationWarning(ownerRid, nodeID, NodeIDFromRid(ownerRid) + 3000, name);

				uint8_t motBattery = GetBatteryForOwner(ownerRid);
				InsertUpdateSwitch(nodeID, 1, STYPE_Motion, mo.motion, name, motBattery);
			}

			for (const auto& te : m_v2sensors->GetTemperatures())
			{
				const std::string& ownerRid = te.owner_rid;
				if (ownerRid.empty())
					continue;

				int v1Num = ParseV1NumericId(te.id_v1);
				int nodeID;
				if (v1Num >= 0)
					nodeID = v1Num + 3000;
				else
					nodeID = NodeIDFromRid(ownerRid) + 3000;

				std::string name;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					name = dit->second.name + " (Temp)";
				else
					name = "Hue Temperature";

				if (v1Num >= 0)
					LogV2MigrationWarning(ownerRid, nodeID, NodeIDFromRid(ownerRid) + 3000, name);

				uint8_t tempBattery = GetBatteryForOwner(ownerRid);
				SendTempSensor(nodeID, tempBattery, te.temperature, name);
			}

			for (const auto& ll : m_v2sensors->GetLightLevels())
			{
				const std::string& ownerRid = ll.owner_rid;
				if (ownerRid.empty())
					continue;

				int v1Num = ParseV1NumericId(ll.id_v1);
				int nodeID;
				if (v1Num >= 0)
					nodeID = v1Num + 3000;
				else
					nodeID = NodeIDFromRid(ownerRid) + 3000;

				if (v1Num >= 0)
				{
					std::string friendlyName;
					auto wdit = deviceById.find(ownerRid);
					if (wdit != deviceById.end() && !wdit->second.name.empty())
						friendlyName = wdit->second.name;
					else
						friendlyName = ownerRid;
					LogV2MigrationWarning(ownerRid, nodeID, NodeIDFromRid(ownerRid) + 3000, friendlyName);
				}

				std::string darkName;
				std::string luxName;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
				{
					darkName = dit->second.name + " (Dark)";
					luxName = dit->second.name + " Lux";
				}
				else
				{
					darkName = "Hue LightLevel";
					luxName = "Hue LightLevel Lux";
				}

				uint8_t llBattery = GetBatteryForOwner(ownerRid);
				InsertUpdateSwitch(nodeID, 1, STYPE_Dusk, ll.dark, darkName, llBattery);

				double lux = 0.00001;
				if (ll.lightlevel != 0)
				{
					double convertedLightLevel = (double)(ll.lightlevel - 1) / 10000.0;
					lux = pow(10.0, convertedLightLevel);
				}
				SendLuxSensor(nodeID, 0, llBattery, (const float)lux, luxName);
			}

			for (const auto& gm : m_v2sensors->GetGroupedMotions())
			{
				const std::string& ownerRid = gm.owner_rid;
				if (ownerRid.empty())
					continue;

				int nodeID = NodeIDFromRid(ownerRid) + 5000;

				std::string name;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					name = dit->second.name + " (Group Motion)";
				else
				{
					for (const auto& room : m_v2sensors->GetRooms())
					{
						if (room.id == ownerRid)
						{
							name = room.name + " (Group Motion)";
							break;
						}
					}
					if (name.empty())
						name = "Hue Group Motion";
				}

				uint8_t motBattery = GetBatteryForOwner(ownerRid);
				InsertUpdateSwitch(nodeID, 1, STYPE_Motion, gm.motion, name, motBattery);
			}

			for (const auto& gl : m_v2sensors->GetGroupedLightLevels())
			{
				const std::string& ownerRid = gl.owner_rid;
				if (ownerRid.empty())
					continue;

				int nodeID = NodeIDFromRid(ownerRid) + 6000;

				std::string darkName;
				std::string luxName;
				for (const auto& room : m_v2sensors->GetRooms())
				{
					if (room.id == ownerRid)
					{
						darkName = room.name + " Group (Dark)";
						luxName = room.name + " Group Lux";
						break;
					}
				}
				if (darkName.empty())
				{
					darkName = "Hue Group LightLevel";
					luxName = "Hue Group LightLevel Lux";
				}

				uint8_t llBattery = GetBatteryForOwner(ownerRid);
				InsertUpdateSwitch(nodeID, 1, STYPE_Dusk, gl.dark, darkName, llBattery);

				double lux = 0.00001;
				if (gl.lightlevel != 0)
				{
					double convertedLightLevel = (double)(gl.lightlevel - 1) / 10000.0;
					lux = pow(10.0, convertedLightLevel);
				}
				SendLuxSensor(nodeID, 0, llBattery, (const float)lux, luxName);
			}

			for (const auto& cm : m_v2sensors->GetCameraMotions())
			{
				const std::string& ownerRid = cm.owner_rid;
				if (ownerRid.empty())
					continue;

				int nodeID = NodeIDFromRid(ownerRid) + 7000;

				std::string name;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					name = dit->second.name + " (Camera Motion)";
				else
					name = "Hue Camera Motion";

				uint8_t motBattery = GetBatteryForOwner(ownerRid);
				InsertUpdateSwitch(nodeID, 1, STYPE_Motion, cm.motion, name, motBattery);
			}

			for (const auto& sam : m_v2sensors->GetSecurityAreaMotions())
			{
				const std::string& ownerRid = sam.owner_rid;
				if (ownerRid.empty())
					continue;

				int nodeID = NodeIDFromRid(ownerRid) + 9000;

				std::string name;
				auto dit = deviceById.find(ownerRid);
				if (dit != deviceById.end() && !dit->second.name.empty())
					name = dit->second.name + " (Security Motion)";
				else
					name = "Hue Security Motion";

				uint8_t motBattery = GetBatteryForOwner(ownerRid);
				InsertUpdateSwitch(nodeID, 1, STYPE_Motion, sam.motion, name, motBattery);
			}
		}
		else
		{
			_log.Debug(DEBUG_HARDWARE, "PhilipsHue: v2 sensors fetch returned no data or failed.");
		}
	}
	catch (const std::exception& e)
	{
		_log.Log(LOG_ERROR, "PhilipsHue: exception in v2 sensor handling: %s", e.what());
	}
	
	return true;
}

bool CPhilipsHue::GetSensors(const Json::Value& root)
{
	if (root["sensors"].empty())
		return false;

	for (auto iSensor = root["sensors"].begin(); iSensor != root["sensors"].end(); ++iSensor)
	{
		Json::Value sensor = *iSensor;
		if (!sensor.isObject())
			continue;
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
					double convertedLightLevel = (double)(current_sensor.m_state.m_lightlevel - 1) / 10000.0;
					lux = pow(10.0, convertedLightLevel);
				}
				SendLuxSensor(sID, 0, current_sensor.m_config.m_battery, (const float)lux, current_sensor.m_type + " Lux " + current_sensor.m_name);
			}
		}
		else
		{
			//Log(LOG_STATUS, "Ignoring Philips Hue CLIP Sensors: (%s)", device_name.c_str());
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
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&xcmd, Name.c_str(), BatteryLevel, m_Name.c_str());
	if (result.empty())
	{
		//Log(LOG_STATUS, "Philips Hue Switch: New Device Found (%s)", Name.c_str());
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE (HardwareID == %d) AND (DeviceID == '%08X') AND (Unit == '%d')", Name.c_str(), (STYPE_Selector), 0, m_HwdID, NodeID, xcmd.unitcode);
		return true;
	}
	return false;
}

void CPhilipsHue::InsertUpdateSwitch(const int NodeID, const uint8_t Unitcode, const _eSwitchType SType, const bool bIsOn, const std::string& Name, const uint8_t BatteryLevel)
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
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&xcmd, Name.c_str(), BatteryLevel, m_Name.c_str());
	if (result.empty())
	{
		//Log(LOG_STATUS, "Philips Hue Switch: New Device Found (%s)", Name.c_str());
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE (HardwareID == %d) AND (DeviceID == '%08X') AND (Unit == '%d')", Name.c_str(), (SType), 0, m_HwdID, NodeID, xcmd.unitcode);
	}
}

void CPhilipsHue::SetSwitchOptions(const int NodeID, const uint8_t Unitcode, const std::map<std::string, std::string>& options)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X') AND (Unit == '%d')", m_HwdID, NodeID, Unitcode);
	if (!result.empty())
	{
		int sIdx = std::stoi(result[0][0]);
		m_sql.SetDeviceOptions(sIdx, options);
	}
}


uint8_t CPhilipsHue::GetBatteryForOwner(const std::string& ownerRid) const
{
	auto bit = m_v2_battery_level.find(ownerRid);
	if (bit != m_v2_battery_level.end())
		return (uint8_t)bit->second;
	return 255;
}

std::string CPhilipsHue::GetV2DeviceName(const std::string& ownerRid) const
{
	if (!m_v2sensors)
		return "";
	for (const auto& d : m_v2sensors->GetDevices())
	{
		if (d.id == ownerRid)
			return d.name;
	}
	return "";
}

void CPhilipsHue::OnSSEEvent(const std::string& jsonData)
{
	Json::Value events;
	if (!ParseJSon(jsonData, events) || !events.isArray())
		return;

	std::lock_guard<std::mutex> lock(m_mutex);
	std::lock_guard<std::mutex> httpLock(m_http_mutex);

	for (const auto& event : events)
	{
		if (!event.isObject())
			continue;
		std::string eventType = event["type"].asString();
		if (eventType != "update" && eventType != "add")
			continue;

		const auto& dataArr = event["data"];
		if (!dataArr.isArray())
			continue;

		for (const auto& resource : dataArr)
		{
			if (!resource.isObject())
				continue;
			std::string rtype = resource["type"].asString();
			DispatchSSEResource(rtype, resource);
		}
	}
}

void CPhilipsHue::DispatchSSEResource(const std::string& rtype, const Json::Value& resource)
{
	if (rtype == "contact")
		HandleSSEContact(resource);
	else if (rtype == "tamper")
		HandleSSETamper(resource);
	else if (rtype == "motion")
		HandleSSEMotion(resource);
	else if (rtype == "temperature")
		HandleSSETemperature(resource);
	else if (rtype == "light_level")
		HandleSSELightLevel(resource);
	else if (rtype == "grouped_motion")
		HandleSSEGroupedMotion(resource);
	else if (rtype == "grouped_light_level")
		HandleSSEGroupedLightLevel(resource);
	else if (rtype == "camera_motion")
		HandleSSECameraMotion(resource);
	else if (rtype == "security_area_motion")
		HandleSSESecurityAreaMotion(resource);
	else if (rtype == "bell_button")
		HandleSSEBellButton(resource);
	else if (rtype == "button")
		HandleSSEButton(resource);
	else if (rtype == "device_power")
		HandleSSEDevicePower(resource);
}

void CPhilipsHue::HandleSSEContact(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("contact_report"))
		return;

	const auto& report = r["contact_report"];
	std::string state;
	std::string changed;
	if (report.isMember("state"))
		state = report["state"].asString();
	if (report.isMember("changed"))
		changed = report["changed"].asString();

	if (state.empty() || changed.empty())
		return;

	auto itCState = m_v2_contact_state.find(ownerRid);
	auto itCChanged = m_v2_contact_changed.find(ownerRid);
	if (itCState != m_v2_contact_state.end() && itCChanged != m_v2_contact_changed.end())
		if (itCState->second == state && itCChanged->second == changed)
			return;

	int nodeID = NodeIDFromRid(ownerRid) + 3000;
	bool isOpen = (state == "no_contact");

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Contact)") : "Hue V2 Contact";

	InsertUpdateSwitch(nodeID, 1, STYPE_DoorContact, isOpen, name, 0);

	m_v2_contact_state[ownerRid] = state;
	m_v2_contact_changed[ownerRid] = changed;
}

void CPhilipsHue::HandleSSETamper(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("tamper_reports"))
		return;

	const auto& reports = r["tamper_reports"];
	if (!reports.isArray() || reports.empty())
		return;

	const auto& report = reports[0];
	std::string state;
	std::string changed;
	if (report.isMember("state"))
		state = report["state"].asString();
	if (report.isMember("changed"))
		changed = report["changed"].asString();

	if (state.empty() || changed.empty())
		return;

	auto itTState = m_v2_tamper_state.find(ownerRid);
	auto itTChanged = m_v2_tamper_changed.find(ownerRid);
	if (itTState != m_v2_tamper_state.end() && itTChanged != m_v2_tamper_changed.end())
		if (itTState->second == state && itTChanged->second == changed)
			return;

	int nodeID = NodeIDFromRid(ownerRid) + 4000;
	bool isTampered = (state == "tampered");

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Tamper)") : "Hue V2 Tamper";

	InsertUpdateSwitch(nodeID, 1, STYPE_OnOff, isTampered, name, 0);

	m_v2_tamper_state[ownerRid] = state;
	m_v2_tamper_changed[ownerRid] = changed;
}

void CPhilipsHue::HandleSSEMotion(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("motion") || !r["motion"].isObject())
		return;

	const auto& motObj = r["motion"];
	bool motion = false;
	if (motObj.isMember("motion"))
		motion = motObj["motion"].asBool();

	std::string id_v1;
	if (r.isMember("id_v1"))
		id_v1 = r["id_v1"].asString();

	int v1Num = ParseV1NumericId(id_v1);
	int nodeID;
	if (v1Num >= 0)
		nodeID = v1Num + 3000;
	else
		nodeID = NodeIDFromRid(ownerRid) + 3000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Motion)") : "Hue Motion";

	uint8_t motBattery = GetBatteryForOwner(ownerRid);
	InsertUpdateSwitch(nodeID, 1, STYPE_Motion, motion, name, motBattery);
}

void CPhilipsHue::HandleSSETemperature(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("temperature"))
		return;

	const auto& tempObj = r["temperature"];
	if (!tempObj.isMember("temperature"))
		return;

	float temperature = tempObj["temperature"].asFloat();

	std::string id_v1;
	if (r.isMember("id_v1"))
		id_v1 = r["id_v1"].asString();

	int v1Num = ParseV1NumericId(id_v1);
	int nodeID;
	if (v1Num >= 0)
		nodeID = v1Num + 3000;
	else
		nodeID = NodeIDFromRid(ownerRid) + 3000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Temp)") : "Hue Temperature";

	uint8_t tempBattery = GetBatteryForOwner(ownerRid);
	SendTempSensor(nodeID, tempBattery, temperature, name);
}

void CPhilipsHue::HandleSSELightLevel(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("light"))
		return;

	const auto& lightObj = r["light"];
	if (!lightObj.isMember("light_level"))
		return;

	int lightlevel = lightObj["light_level"].asInt();
	bool dark = false;
	if (lightObj.isMember("dark"))
		dark = lightObj["dark"].asBool();

	std::string id_v1;
	if (r.isMember("id_v1"))
		id_v1 = r["id_v1"].asString();

	int v1Num = ParseV1NumericId(id_v1);
	int nodeID;
	if (v1Num >= 0)
		nodeID = v1Num + 3000;
	else
		nodeID = NodeIDFromRid(ownerRid) + 3000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string darkName;
	std::string luxName;
	if (!deviceName.empty())
	{
		darkName = deviceName + " (Dark)";
		luxName = deviceName + " Lux";
	}
	else
	{
		darkName = "Hue LightLevel";
		luxName = "Hue LightLevel Lux";
	}

	uint8_t llBattery = GetBatteryForOwner(ownerRid);
	InsertUpdateSwitch(nodeID, 1, STYPE_Dusk, dark, darkName, llBattery);

	double lux = 0.00001;
	if (lightlevel != 0)
	{
		double convertedLightLevel = (double)(lightlevel - 1) / 10000.0;
		lux = pow(10.0, convertedLightLevel);
	}
	SendLuxSensor(nodeID, 0, llBattery, (const float)lux, luxName);
}

void CPhilipsHue::HandleSSEGroupedMotion(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("motion") || !r["motion"].isObject())
		return;

	const auto& motObj = r["motion"];
	bool motion = false;
	if (motObj.isMember("motion"))
		motion = motObj["motion"].asBool();

	int nodeID = NodeIDFromRid(ownerRid) + 5000;

	std::string name;
	for (const auto& room : m_v2sensors->GetRooms())
	{
		if (room.id == ownerRid)
		{
			name = room.name + " (Group Motion)";
			break;
		}
	}
	if (name.empty())
		name = "Hue Group Motion";

	uint8_t motBattery = GetBatteryForOwner(ownerRid);
	InsertUpdateSwitch(nodeID, 1, STYPE_Motion, motion, name, motBattery);
}

void CPhilipsHue::HandleSSEGroupedLightLevel(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("light"))
		return;

	const auto& lightObj = r["light"];
	if (!lightObj.isMember("light_level"))
		return;

	int lightlevel = lightObj["light_level"].asInt();
	bool dark = false;
	if (lightObj.isMember("dark"))
		dark = lightObj["dark"].asBool();

	int nodeID = NodeIDFromRid(ownerRid) + 6000;

	std::string darkName;
	std::string luxName;
	for (const auto& room : m_v2sensors->GetRooms())
	{
		if (room.id == ownerRid)
		{
			darkName = room.name + " Group (Dark)";
			luxName = room.name + " Group Lux";
			break;
		}
	}
	if (darkName.empty())
	{
		darkName = "Hue Group LightLevel";
		luxName = "Hue Group LightLevel Lux";
	}

	uint8_t llBattery = GetBatteryForOwner(ownerRid);

	InsertUpdateSwitch(nodeID, 1, STYPE_Dusk, dark, darkName, llBattery);

	double lux = 0.00001;
	if (lightlevel != 0)
	{
		double convertedLightLevel = (double)(lightlevel - 1) / 10000.0;
		lux = pow(10.0, convertedLightLevel);
	}
	SendLuxSensor(nodeID, 0, llBattery, (const float)lux, luxName);
}

void CPhilipsHue::HandleSSECameraMotion(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("motion") || !r["motion"].isObject())
		return;

	const auto& motObj = r["motion"];
	bool motion = false;
	if (motObj.isMember("motion"))
		motion = motObj["motion"].asBool();

	int nodeID = NodeIDFromRid(ownerRid) + 7000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Camera Motion)") : "Hue Camera Motion";

	uint8_t motBattery = GetBatteryForOwner(ownerRid);
	InsertUpdateSwitch(nodeID, 1, STYPE_Motion, motion, name, motBattery);
}

void CPhilipsHue::HandleSSESecurityAreaMotion(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("motion") || !r["motion"].isObject())
		return;

	const auto& motObj = r["motion"];
	bool motion = false;
	if (motObj.isMember("motion"))
		motion = motObj["motion"].asBool();

	int nodeID = NodeIDFromRid(ownerRid) + 9000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Security Motion)") : "Hue Security Motion";

	uint8_t motBattery = GetBatteryForOwner(ownerRid);
	InsertUpdateSwitch(nodeID, 1, STYPE_Motion, motion, name, motBattery);
}

void CPhilipsHue::HandleSSEBellButton(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("bell") || !r["bell"].isObject())
		return;

	const auto& bellObj = r["bell"];
	std::string event;
	if (bellObj.isMember("bell_report") && bellObj["bell_report"].isObject())
	{
		if (bellObj["bell_report"].isMember("event"))
			event = bellObj["bell_report"]["event"].asString();
	}
	if (event.empty() && bellObj.isMember("last_event"))
		event = bellObj["last_event"].asString();

	if (event != "ring")
		return;

	int nodeID = NodeIDFromRid(ownerRid) + 10000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? (deviceName + " (Bell)") : "Hue Bell";

	uint8_t battery = GetBatteryForOwner(ownerRid);
	InsertUpdateSwitch(nodeID, 1, STYPE_Doorbell, true, name, battery);
}

void CPhilipsHue::HandleSSEButton(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("button") || !r["button"].isObject())
		return;

	const auto& btnObj = r["button"];
	std::string event;
	if (btnObj.isMember("button_report") && btnObj["button_report"].isObject())
	{
		if (btnObj["button_report"].isMember("event"))
			event = btnObj["button_report"]["event"].asString();
	}
	if (event.empty() && btnObj.isMember("last_event"))
		event = btnObj["last_event"].asString();

	if (event == "initial_press" || event == "repeat" || event == "long_press" || event.empty())
		return;

	int control_id = 0;
	if (r.isMember("metadata") && r["metadata"].isMember("control_id"))
		control_id = r["metadata"]["control_id"].asInt();

	if (!m_v2sensors)
		return;

	const auto& allButtons = m_v2sensors->GetButtons();
	int button_count = 0;
	bool any_long_press = false;
	std::string base_id_v1;
	int lowest_ctrl = INT_MAX;

	for (const auto& btn : allButtons)
	{
		if (btn.owner_rid != ownerRid)
			continue;
		button_count++;
		if (btn.supports_long_press)
			any_long_press = true;
		if (btn.control_id < lowest_ctrl && !btn.id_v1.empty())
		{
			lowest_ctrl = btn.control_id;
			base_id_v1 = btn.id_v1;
		}
	}

	if (button_count == 0)
		button_count = 1;

	int v1Num = ParseV1NumericId(base_id_v1);
	int nodeID = (v1Num >= 0) ? (v1Num + 3000) : (NodeIDFromRid(ownerRid) + 8000);

	int button_nr = (control_id > 0) ? (control_id - 1) : 0;
	if (button_nr > 255)
	{
		Log(LOG_ERROR, "PhilipsHue: button control_id out of range: %d", control_id);
		return;
	}
	int selectorLevel = button_nr * 10;
	if (any_long_press)
		selectorLevel *= 2;
	selectorLevel += 10;
	if (event == "long_release")
		selectorLevel += 10;

	std::string deviceName = GetV2DeviceName(ownerRid);
	std::string name = !deviceName.empty() ? deviceName : "Hue Switch";

	uint8_t battery = GetBatteryForOwner(ownerRid);

	if (selectorLevel > 255) selectorLevel = 255;
	bool isNew = InsertUpdateSelectorSwitch(nodeID, 1, (uint8_t)selectorLevel, name, battery);
	if (isNew)
	{
		std::map<std::string, std::string> options;
		options["SelectorStyle"] = "1";
		options["LevelOffHidden"] = "true";
		options["LevelNames"] = "Off";
		for (int b = 0; b < button_count; b++)
		{
			if (any_long_press)
			{
				options["LevelNames"] += "|Button " + std::to_string(b + 1) + " short";
				options["LevelNames"] += "|Button " + std::to_string(b + 1) + " long";
			}
			else
			{
				options["LevelNames"] += "|Button " + std::to_string(b + 1);
			}
		}
		SetSwitchOptions(nodeID, 1, options);
	}
}

void CPhilipsHue::HandleSSEDevicePower(const Json::Value& r)
{
	std::string ownerRid;
	if (r.isMember("owner") && r["owner"].isMember("rid"))
		ownerRid = r["owner"]["rid"].asString();
	if (ownerRid.empty())
		return;

	if (!r.isMember("power_state"))
		return;

	const auto& powerObj = r["power_state"];

	int batteryLevel = -1;
	if (powerObj.isMember("battery_level"))
		batteryLevel = powerObj["battery_level"].asInt();

	if (batteryLevel < 0 && powerObj.isMember("battery_state"))
	{
		const std::string bstate = powerObj["battery_state"].asString();
		if (bstate == "full" || bstate == "normal")
			batteryLevel = 100;
		else if (bstate == "high")
			batteryLevel = 90;
		else if (bstate == "low")
			batteryLevel = 20;
		else if (bstate == "critical")
			batteryLevel = 5;
		else
			batteryLevel = 255;
	}

	if (batteryLevel < 0 || batteryLevel == 255)
		return;
	if (batteryLevel > 100)
		batteryLevel = 100;

	m_v2_battery_level[ownerRid] = batteryLevel;
	int baseNode = NodeIDFromRid(ownerRid);
	int domoticzNodeID_contact = baseNode + 3000;
	int domoticzNodeID_tamper = baseNode + 4000;

	std::string deviceName = GetV2DeviceName(ownerRid);
	char szID[16];
	std::vector<std::vector<std::string>> result;

	snprintf(szID, sizeof(szID), "%08X", domoticzNodeID_contact);
	result = m_sql.safe_query("SELECT ID, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d)",
		m_HwdID, szID, 1);
	if (!result.empty())
	{
		int deviceRowId = atoi(result[0][0].c_str());
		int prevBattery = atoi(result[0][1].c_str());
		if (prevBattery != batteryLevel)
		{
			int nValue = 0;
			auto res2 = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE ID=%d", deviceRowId);
			if (!res2.empty())
				nValue = atoi(res2[0][0].c_str());
			bool contactIsOn = (nValue != 0);
			std::string contactName = !deviceName.empty() ? (deviceName + " (Contact)") : "Hue V2 Contact";
			InsertUpdateSwitch(domoticzNodeID_contact, 1, STYPE_DoorContact, contactIsOn, contactName, (uint8_t)batteryLevel);
			m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d, LastUpdate=datetime('now','localtime') WHERE ID=%d", batteryLevel, deviceRowId);
			m_v2_battery_level[ownerRid] = batteryLevel;
		}
	}

	snprintf(szID, sizeof(szID), "%08X", domoticzNodeID_tamper);
	result = m_sql.safe_query("SELECT ID, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d)",
		m_HwdID, szID, 1);
	if (!result.empty())
	{
		int deviceRowId = atoi(result[0][0].c_str());
		int prevBattery = atoi(result[0][1].c_str());
		if (prevBattery != batteryLevel)
		{
			int nValue = 0;
			auto res2 = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE ID=%d", deviceRowId);
			if (!res2.empty())
				nValue = atoi(res2[0][0].c_str());
			bool tamperIsOn = (nValue != 0);
			std::string tamperName = !deviceName.empty() ? (deviceName + " (Tamper)") : "Hue V2 Tamper";
			InsertUpdateSwitch(domoticzNodeID_tamper, 1, STYPE_OnOff, tamperIsOn, tamperName, (uint8_t)batteryLevel);
			m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d, LastUpdate=datetime('now','localtime') WHERE ID=%d", batteryLevel, deviceRowId);
			m_v2_battery_level[ownerRid] = batteryLevel;
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_PhilipsHueRegister(WebEmSession& session, const request& req, Json::Value& root)
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
		void CWebServer::Cmd_PhilipsHueGetGroups(WebEmSession& session, const request& req, Json::Value& root)
		{

		}
		void CWebServer::Cmd_PhilipsHueAddGroup(WebEmSession& session, const request& req, Json::Value& root)
		{

		}
		void CWebServer::Cmd_PhilipsHueDeleteGroup(WebEmSession& session, const request& req, Json::Value& root)
		{

		}
		void CWebServer::Cmd_PhilipsHueGroupAddLight(WebEmSession& session, const request& req, Json::Value& root)
		{

		}
		void CWebServer::Cmd_PhilipsHueGroupRemoveLight(WebEmSession& session, const request& req, Json::Value& root)
		{

		}
	} // namespace server
} // namespace http
