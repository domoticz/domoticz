#include "stdafx.h"
#include <string>
#include "Deconz.h"
#include "../../main/Logger.h"
#include "../../main/EventsPythonDevice.h"
#include "../../main/localtime_r.h"
#include "../../json/reader.h"
#include "../../main/WebServer.h"
#include <platform/Log.h>
#include "../../main/mainworker.h"
#include "../../main/SQLHelper.h"

Deconz::Deconz(const int id, const std::string& ipAddress, const unsigned short port,
               const std::string& username, const int poll, const int options)
{
	(this)->m_HwdID = id;
	(this)->ipAddress = ipAddress;
	(this)->username = username;
	(this)->port = port;
	(this)->poll_interval = poll;

	if (poll_interval < 1)
	{
		poll_interval = DECONZ_DEFAULT_POLL_INTERVAL;
		_log.Log(LOG_STATUS, "%s: Using default poll interval of %d seconds.", DECONZ, poll_interval);
	}
	else
	{
		_log.Log(LOG_STATUS, "%s: Using poll interval of %d seconds.", DECONZ, poll_interval);
	}
}

Deconz::~Deconz()
{
}

void Deconz::Init()
{
}

bool Deconz::StartHardware()
{
	RequestStart();

	thread = std::make_shared<std::thread>(&Deconz::Do_Work, this);
	SetThreadName(thread->native_handle(), DECONZ);
	m_bIsStarted = true;
	sOnConnected(this);

	return (thread != nullptr);
}

bool Deconz::StopHardware()
{
	if (thread)
	{
		RequestStop();
		thread->join();
		thread.reset();
	}
	m_bIsStarted = false;
	return !m_bIsStarted;
}

void Deconz::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = poll_interval - 1;

	_log.Log(LOG_STATUS, "%s: Worker started...", DECONZ);

	while (!IsStopRequested(500))
	{
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % poll_interval == 0)
			{
				m_LastHeartbeat = mytime(nullptr);
				GetStates();
			}
		}
	}
	_log.Log(LOG_STATUS, "%s: Worker stopped...", DECONZ);
}

bool Deconz::WriteToHardware(const char* pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	int svalue = 0;
	int svalue2 = 0;
	int svalue3 = 0;
	std::string LCmd = "";
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
			float fvalue = (254.0f / 100.0f)*float(pSwitch->level);
			if (fvalue > 254.0f)
				fvalue = 254.0f;
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
		else if (pLed->command == Color_LedOn)
		{
			LCmd = "On";
			svalue = 254;
			SwitchLight(nodeID, LCmd, svalue);
			return true;
		}
		else if (pLed->command == Color_SetBrightnessLevel)
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
				float fvalue = (254.0f / 100.0f)*float(pLed->value);
				if (fvalue > 254.0f)
					fvalue = 254.0f;
				svalue = round(fvalue);
				SwitchLight(nodeID, LCmd, svalue);
			}
			return true;
		}
		else if (pLed->command == Color_SetColorToWhite)
		{
			LCmd = "Set White";
			SwitchLight(nodeID, LCmd, 0);
			return true;
		}
		else if (pLed->command == Color_SetColor)
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
				svalue2 = round(float(pLed->color.t)*(500.0f - 153.0f) / 255.0f + 153.0f);
			}
			else if (pLed->color.mode == ColorModeRGB)
			{
				float hsb[3];
				rgb2hsb(pLed->color.r, pLed->color.g, pLed->color.b, hsb);
				float cHue = (65535.0f)*hsb[0]; // Scale hue from 0..1 to 0..65535
				float cSat = (254.0f)*hsb[1];   // Scale saturation from 0..1 to 0..254
				LCmd = "Set Hue";
				svalue2 = round(cHue);
				svalue3 = round(cSat);
			}
			else {
				_log.Log(LOG_STATUS, "%s: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", DECONZ, pLed->color.mode);
			}
			float fvalue = (254.0f / 100.0f)*float(pLed->value);
			if (fvalue > 254.0f)
				fvalue = 254.0f;
			svalue = round(fvalue);
			SwitchLight(nodeID, LCmd, svalue, svalue2, svalue3);
			return true;
		}
	}
	return true;
}

bool Deconz::SwitchLight(const int nodeID, const std::string& LCmd, const int svalue, const int svalue2, const int svalue3)
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
	LightColorMode mode;

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
		sPostData << "{\"on\": true, \"bri\": " << svalue << " }";
		setOn = true;
		setLevel = true;
	}
	else if (LCmd == "Set White")
	{
		sPostData << "{\"on\": true, \"sat\": 0 , \"bri\": 255, \"hue\": 0 }";
		// Do state update next time the light is polled
	}
	else if (LCmd == "Set Hue")
	{
		sPostData << "{\"on\": true, \"sat\": " << svalue3 << ", \"hue\": " << svalue2 << ", \"bri\": " << svalue << "  }";
		setOn = true;
		setLevel = true;
		setHueSat = true;
		setMode = true;
		mode = LMODE_HS;
	}
	else if (LCmd == "Set CT")
	{
		sPostData << "{\"on\": true, \"ct\": " << svalue2 << ", \"bri\": " << svalue << "  }";
		setOn = true;
		setLevel = true;
		setCt = true;
		setMode = true;
		mode = LMODE_CT;
	}
	else
	{
		_log.Log(LOG_ERROR, "Philips Hue: Invalid light command received!");
		return false;
	}

	// Update cached state
	LightState *pState = NULL;

	if (nodeID < 1000)
	{
		//Light
		auto && ittLight = lights.find(nodeID);
		if (ittLight != lights.end())
		{
			pState = &ittLight->second.state;
		}
	}	
	if (pState)
	{
		if (setOn) pState->on = On;
		if (setLevel) pState->level = int((100.0f / 254.0f)*float(svalue));
		if (setHueSat) pState->hue = svalue2;
		if (setHueSat) pState->sat = svalue3;
		if (setCt) pState->ct = int((float(svalue2) - 153.0) / (500.0 - 153.0));
		if (setMode) pState->mode = mode;
	}

	std::stringstream sstr2;
	if (nodeID < 1000)
	{
		//Light
		sstr2 << "http://" << ipAddress
			<< ":" << port
			<< "/api/" << username
			<< "/lights/" << nodeID << "/state";
	}
	
	std::string sURL = sstr2.str();
	if (!HTTPClient::PUT(sURL, sPostData.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "%s: Error connecting to bridge (Switch Light/Scene), (Check IPAddress/Username)", DECONZ);
		return false;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "%s: Invalid data received (Switch Light/Scene), or invalid IPAddress/Username!", DECONZ);
		return false;
	}

	if (sResult.find("error") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "%s: Error received: %s", DECONZ, root[0]["error"]["description"].asString().c_str());
		return false;
	}
	return true;
}


void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}

bool Deconz::GetStates()
{
	std::string result;
	std::stringstream sstr2;

	sstr2 << "http://" << ipAddress
		<< ":" << port
		<< "/api/" << username;
	//Get Data
	std::string sURL = sstr2.str();
	std::vector<std::string> ExtraHeaders;
	if (!HTTPClient::GET(sURL, ExtraHeaders, result))
	{
		_log.Log(LOG_ERROR, "%s: Error getting states. Please check settings...", DECONZ);
		return false;
	}

	SaveString2Disk(result, "E:\\deconz.json");

	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(result, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "%s: Invalid data received. Please verify settings.", DECONZ);
		return false;
	}

	if (result.find("\"error\":") != std::string::npos)
	{
		//We had an error
		_log.Log(LOG_ERROR, "%s: Error received: %s", DECONZ, root[0]["error"]["description"].asString().c_str());
		return false;
	}

	GetLights(root);
	GetSensors(root);

	return true;
}

bool Deconz::GetLights(const Json::Value& root)
{
	if (root["lights"].empty())
	{
		return false;
	}

	for (auto iLight = root["lights"].begin(); iLight != root["lights"].end(); ++iLight)
	{
		Json::Value light = *iLight;
		if (light.isObject())
		{
			bool doSend = true;
			bool newLight = false;
			int lightId = atoi(iLight.key().asCString());
			DeconzLight dlight(light);

			/* Not needed for other than debug */ /*
			_log.Log(LOG_STATUS, "%s: Found light with id=%d, manufacturer=%s, modelid=%s", DECONZ, lightId,
			         dlight.manufacturer.c_str(), dlight.modelId.c_str());
			*/
			auto storedLight = lights.find(lightId);
			if (storedLight != lights.end())
			{
				if (DeconzLight::StatesSimilar(storedLight->second.state, dlight.state))
				{
					doSend = false;
				}
			}

			if (doSend)
			{
				lights[lightId] = dlight;
				InsertUpdateSwitch(lightId, dlight, "", true);
			}
		}
	}

	return true;
}

bool Deconz::GetSensors(const Json::Value& root)
{
	if (root["sensors"].empty())
	{
		return false;
	}

	for (auto iSensor = root["sensors"].begin(); iSensor != root["sensors"].end(); ++iSensor)
	{
		Json::Value sensor = *iSensor;
		if (sensor.isObject())
		{
			bool doSend = true;
			bool newSensor = false;
			int sensorId = atoi(iSensor.key().asCString());
			DeconzSensor recSensor(sensor);

			/* Just for debug
			_log.Log(LOG_STATUS, "%s: Found sensor with id=%d, manufacturer=%s, modelid=%s", DECONZ, sensorId,
			         recSensor.manufacturer.c_str(), recSensor.modelId.c_str());
			*/

			DeconzSensor storedSensor;
			if (sensors.find(sensorId) != sensors.end())
			{
				storedSensor = sensors[sensorId];
			}
			else
			{
				newSensor = true;
			}

			// update internal table
			sensors.erase(sensorId);
			sensors.insert(std::make_pair(sensorId, recSensor));

			if (doSend)
			{
				int domoticzId = 3000 + sensorId;
				std::string deviceName = recSensor.type + " " + recSensor.name;

				if (recSensor.type == "ZHAPresence")
				{
					if ((storedSensor.state.presence != recSensor.state.presence) || newSensor)
					{
						_log.Log(LOG_STATUS, "%s: updating sensor %d from %s to %s", DECONZ, sensorId,
						         storedSensor.state.presence ? "true" : "false",
						         recSensor.state.presence ? "true" : "false");
						InsertUpdateSwitch(domoticzId, STYPE_Motion, recSensor.state.presence, deviceName,
						                   recSensor.config.battery);
					}
				} else if(recSensor.type == "ZHASwitch")
				{
					int bogus = recSensor.state.buttonevent;
				}
			} else
			{
				_log.Log(LOG_STATUS, "%s: not updating %s", DECONZ, recSensor.type.c_str());
			}
		}
	}

	return true;
}

void Deconz::InsertUpdateSwitch(const int NodeID, const _eSwitchType SType, const bool status, const std::string& Name,
                                const uint8_t BatteryLevel) const
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

	if (status)
	{
		xcmd.cmnd = gswitch_sOn;
	}
	else
	{
		xcmd.cmnd = gswitch_sOff;
	}

	//check if this switch is already in the database
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query(
		"SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) ", m_HwdID, ID,
		pTypeGeneralSwitch);
	if (result.empty())
	{
		_log.Log(LOG_STATUS, "%s: New Device Found (%s)", DECONZ, Name.c_str());
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, Name.c_str(), BatteryLevel);

		m_sql.safe_query(
			"UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '%q')",
			Name.c_str(), (SType), 0, m_HwdID, ID);
	}
	else
	{
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, Name.c_str(), BatteryLevel);
	}
}

void Deconz::InsertUpdateSwitch(const int NodeID, const DeconzLight& light, const std::string& Options,
                                const bool AddMissingDevice)
{
	auto LType = light.lightType;
	auto tstate = light.state;
	auto name = light.name;
	auto modelid = light.modelId;

	if (LType == LTYPE_RGB_W || LType == LTYPE_CW_WW || LType == LTYPE_RGB_CW_WW)
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
		case LTYPE_CW_WW:
			sType = sTypeColor_CW_WW;
			break;
		case LTYPE_RGB_CW_WW:
			sType = sTypeColor_RGB_CW_WW;
			break;
		case LTYPE_RGB_W:
		default:
			sType = sTypeColor_RGB_W;
			break;
		}

		//Get current nValue if exist
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query(
			"SELECT nValue, LastLevel, Color, SubType, ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (DeviceID=='%q')",
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
			if (sTypeOld != sType)
			{
				_log.Log(LOG_STATUS, "Philips Hue: Updating SubType of light '%s' from %u to %u", szID, sTypeOld,
				         sType);
				m_sql.UpdateDeviceValue("SubType", (int)sType, sID);
			}
		}

		if (tstate.on && (tstate.level != 100))
			cmd = Color_SetBrightnessLevel;

		if (tstate.on && (tstate.mode != LMODE_NONE))
		{
			if (tstate.mode == LMODE_HS)
			{
				int r, g, b;

				//convert hue+sat to RGB
				float iHue = float(tstate.hue) * 360.0f / 65535.0f;
				float iSat = float(tstate.sat) / 254.0f;
				hsb2rgb(iHue, iSat, 1.0f, r, g, b, 255);

				color = _tColor(r, g, b, 0, 0, ColorModeRGB);
			}
			if (tstate.mode == LMODE_XY)
			{
				uint8_t r, g, b;

				//convert xy to RGB
				DeconzLight::RgbFromXY(tstate.x, tstate.y, 1.0, r, g, b);

				color = _tColor(r, g, b, 0, 0, ColorModeRGB);
			}
			if (tstate.mode == LMODE_CT)
			{
				float iCt = (float(tstate.ct) - 153.0f) / (500.0f - 153.0f) * 255.0f;
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
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, name.c_str(), 255);

		if (result.empty())
		{
			//Set SwitchType to STYPE_Dimmer
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
			                 int(STYPE_Dimmer), m_HwdID, szID);
		}
	}
		/* No drama!
		 *else if (LType == LTYPE_SCENE)
		{
			char szID[10];
			sprintf(szID, "%08X", (unsigned int)NodeID);
			unsigned char unitcode = 1;
			int cmd = (tstate.on ? Color_LedOn : Color_LedOff);
	
			//Get current nValue if exist
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
				m_HwdID, int(unitcode), pTypeColorSwitch, sTypeColor_RGB_W, szID);
	
			if (result.empty() && !AddMissingDevice)
				return;
	
			if (!result.empty())
			{
				//Already in the system
				int nvalue = atoi(result[0][0].c_str());
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
			m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, Name.c_str(), 255);
	
			if (result.empty())
			{
				//Set SwitchType to STYPE_PushOn
				m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
					int(STYPE_Dimmer), m_HwdID, szID);
			}
		} */
	else
	{
		//Send as GeneralSwitch
		char szID[10];
		sprintf(szID, "%08X", (unsigned int)NodeID);
		unsigned char unitcode = 1;
		int cmd = (tstate.on ? gswitch_sOn : gswitch_sOff);

		//Check if we already exist
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query(
			"SELECT nValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
			m_HwdID, int(unitcode), pTypeLighting2, sTypeAC, szID);
		//_log.Log(LOG_STATUS, "HueBridge state change for DeviceID '%s': Level = %d", szID, tstate.level);

		if (result.empty() && !AddMissingDevice)
			return;

		if (!result.empty())
		{
			//Already in the system
			//Update state
			int nvalue = atoi(result[0][0].c_str());
		}

		if (tstate.on && (tstate.level != 100))
			cmd = gswitch_sSetLevel;

		_tGeneralSwitch lcmd;
		lcmd.subtype = sSwitchGeneralSwitch;
		lcmd.id = NodeID;
		lcmd.unitcode = unitcode;
		lcmd.cmnd = cmd;
		lcmd.level = tstate.level;
		lcmd.seqnbr = 1;
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd, name.c_str(), 255);

		if (result.empty())
		{
			//Set SwitchType
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE(HardwareID == %d) AND (DeviceID == '%q')",
			                 int(LType == LTYPE_DIM ? STYPE_Dimmer : STYPE_OnOff), m_HwdID, szID);
		}
	}
}


std::string Deconz::RegisterUser(const std::string& IPAddress, const unsigned short Port,
                                 const std::string& username)
{
	std::string retStr = "Error;Unknown";
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/json");
	std::string sResult;
	std::string sPostData;

	//Providing own username is not allowed, so don't use it and only provide devicetype
	sPostData = "{ \"devicetype\": \"domoticz\" }";

	std::stringstream sstr2;
	sstr2 << "http://" << IPAddress
		<< ":" << Port
		<< "/api";
	std::string sURL = sstr2.str();

	if (!HTTPClient::POST(sURL, sPostData, ExtraHeaders, sResult))
	{
		retStr = "Error;Error connecting to deCONZ bridge:";
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

	if (sResult.find("error") != std::string::npos)
	{
		retStr = "Error;" + root[0]["error"]["description"].asString();
		return retStr;
	}

	std::string new_username = root[0]["success"]["username"].asString();
	retStr = "OK;" + new_username;
	return retStr;
}

//Webserver helpers
namespace http
{
	namespace server
	{
		void CWebServer::Cmd_DeconzRegister(WebEmSession& session, const request& req, Json::Value& root)
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
			if (
				(sipaddress == "") ||
				(sport == "")
			)
				return;

			std::string sresult = Deconz::RegisterUser(sipaddress, (unsigned short)atoi(sport.c_str()),
			                                           susername);
			std::vector<std::string> strarray;
			StringSplit(sresult, ";", strarray);
			if (strarray.size() != 2)
				return;

			if (strarray[0] == "Error")
			{
				root["statustext"] = strarray[1];
				return;
			}
			root["status"] = "OK";
			root["username"] = strarray[1];
		}
	}
}
