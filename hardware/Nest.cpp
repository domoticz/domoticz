#include "stdafx.h"
#include "Nest.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

#define round(a) ( int ) ( a + .5 )

const std::string NEST_LOGIN_PATH = "https://home.nest.com/user/login";
const std::string NEST_GET_STATUS = "/v2/mobile/user.";
const std::string NEST_SET_SHARED = "/v2/put/shared.";
const std::string NEST_SET_STRUCTURE = "/v2/put/structure.";
const std::string NEST_SET_DEVICE = "/v2/put/device.";

#ifdef _DEBUG
	//#define DEBUG_NextThermostatR
	//#define DEBUG_NextThermostatW
#endif

#ifdef DEBUG_NextThermostatW
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
#ifdef DEBUG_NextThermostatR
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

CNest::CNest(const int ID, const std::string &Username, const std::string &Password) :
m_UserName(CURLEncode::URLEncode(Username)),
m_Password(CURLEncode::URLEncode(Password))
{
	m_HwdID=ID;
	Init();
}

CNest::~CNest(void)
{
}

void CNest::Init()
{
	m_AccessToken = "";
	m_UserID = "";
	m_stoprequested = false;
	m_bDoLogin = true;
}

bool CNest::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CNest::Do_Work, this);
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CNest::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
	if (!m_bDoLogin)
		Logout();
    return true;
}

#define NEST_POLL_INTERVAL 30

void CNest::Do_Work()
{
	_log.Log(LOG_STATUS,"Nest: Worker started...");
	int sec_counter = NEST_POLL_INTERVAL-5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % NEST_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}

	}
	_log.Log(LOG_STATUS,"Nest: Worker stopped...");
}

void CNest::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
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


// Creates and updates switch used to log Heating and/or Colling.
void CNest::UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (SubType==%d) AND (DeviceID=='%q')",
		m_HwdID, pTypeLighting2, sTypeAC, szIdx);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
			return;
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
	lcmd.LIGHTING2.unitcode = 1;
	int level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

bool CNest::Login()
{
	if (!m_AccessToken.empty())
	{
		Logout();
	}
	m_AccessToken = "";
	m_UserID = "";

	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	std::string sResult;

	std::string sURL = NEST_LOGIN_PATH;
	if (!HTTPClient::POST(sURL, szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"Nest: Error login!");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Nest: Invalid data received, or invalid username/password!");
		return false;
	}
	if (root["urls"].empty())
	{
		_log.Log(LOG_ERROR, "Nest: Invalid data received, or invalid username/password!");
		return false;
	}
	if (root["urls"]["transport_url"].empty())
	{
		_log.Log(LOG_ERROR, "Nest: Invalid data received, or invalid username/password!");
		return false;
	}
	m_TransportURL = root["urls"]["transport_url"].asString();

	if (root["access_token"].empty())
	{
		_log.Log(LOG_ERROR, "Nest: Invalid data received, or invalid username/password!");
		return false;
	}
	m_AccessToken = root["access_token"].asString();

	if (root["userid"].empty())
	{
		_log.Log(LOG_ERROR, "Nest: Invalid data received, or invalid username/password!");
		return false;
	}
	m_UserID = root["userid"].asString();

	m_bDoLogin = false;
	return true;
}

void CNest::Logout()
{
	if (m_bDoLogin)
		return; //we are not logged in
	m_AccessToken = "";
	m_UserID = "";
	m_bDoLogin = true;
}


bool CNest::WriteToHardware(const char *pdata, const unsigned char length)
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

	if (node_id % 3 == 0)
	{
		//Away
		return SetAway(node_id, bIsOn);
	}

	if (node_id % 4 == 0)
	{
		//Manual Eco Mode
		return SetManualEcoMode(node_id, bIsOn);
	}

	return false;
}

void CNest::UpdateSmokeSensor(const unsigned char Idx, const bool bOn, const std::string &defaultname)
{
	bool bDeviceExits = true;
	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, Idx, 0);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szIdx);
	if (result.empty())
	{
		bDeviceExits = false;
	}
	else
	{
		//check if we have a change, if not only update the LastUpdate field
		bool bNoChange = false;
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			bNoChange = true;
		else if ((bOn && (nvalue != 0)))
			bNoChange = true;
		if (bNoChange)
		{
			time_t now = time(0);
			struct tm ltime;
			localtime_r(&now, &ltime);

			char szLastUpdate[40];
			sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

			m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%q' WHERE(HardwareID == %d) AND (DeviceID == '%q')",
				szLastUpdate, m_HwdID, szIdx);
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
	lcmd.LIGHTING2.id3 = Idx;
	lcmd.LIGHTING2.id4 = 0;
	lcmd.LIGHTING2.unitcode = 1;
	int level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;

	if (!bDeviceExits)
	{
		m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
		//Assign default name for device
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')", defaultname.c_str(), m_HwdID, szIdx);
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szIdx);
		if (!result.empty())
		{
			m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID=='%q')", STYPE_SMOKEDETECTOR, result[0][0].c_str());
		}
	}
	else
		sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}


void CNest::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_NextThermostatR
	sResult = ReadFile("E:\\nest.json");
#else
	if (m_UserName.size()==0)
		return;
	if (m_Password.size()==0)
		return;
	if (m_bDoLogin)
	{
		if (!Login())
		return;
	}
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-user-id:" + m_UserID);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	//Get Data
	std::string sURL = m_TransportURL + NEST_GET_STATUS + m_UserID;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "Nest: Error getting current state!");
		m_bDoLogin = true;
		return;
	}
#endif

#ifdef DEBUG_NextThermostatW
	SaveString2Disk(sResult, "E:\\nest.json");
#endif

	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Nest: Invalid data received!");
		m_bDoLogin = true;
		return;
	}
	bool bHaveShared = !root["shared"].empty();
	bool bHaveTopaz = !root["topaz"].empty();

	if ((!bHaveShared) && (!bHaveTopaz))
	{
		_log.Log(LOG_ERROR, "Nest: request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}

	//Protect
	if (bHaveTopaz)
	{
		if (root["topaz"].size() < 1)
		{
			_log.Log(LOG_ERROR, "Nest: request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
		Json::Value::Members members = root["topaz"].getMemberNames();
		if (members.size() < 1)
		{
			_log.Log(LOG_ERROR, "Nest: request not successful, restarting..!");
			m_bDoLogin = true;
			return;
		}
		int SwitchIndex = 1;
		for (Json::Value::iterator itDevice = root["topaz"].begin(); itDevice != root["topaz"].end(); ++itDevice)
		{
			Json::Value device = *itDevice;
			std::string devstring = itDevice.key().asString();
			if (device["where_id"].empty())
				continue;
			std::string whereid = device["where_id"].asString();
			//lookup name
			std::string devName = devstring;
			if (!root["where"].empty())
			{
				for (Json::Value::iterator itWhere = root["where"].begin(); itWhere != root["where"].end(); ++itWhere)
				{
					Json::Value iwhere = *itWhere;
					if (!iwhere["wheres"].empty())
					{
						for (Json::Value::iterator itWhereNest = iwhere["wheres"].begin(); itWhereNest != iwhere["wheres"].end(); ++itWhereNest)
						{
							Json::Value iwhereItt = *itWhereNest;
							if (!iwhereItt["where_id"].empty())
							{
								std::string tmpWhereid = iwhereItt["where_id"].asString();
								if (tmpWhereid == whereid)
								{
									devName = iwhereItt["name"].asString();
									break;
								}
							}
						}
					}

				}
			}
			bool bIAlarm = false;
			bool bBool;
			if (!device["component_speaker_test_passed"].empty())
			{
				bBool = device["component_speaker_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_smoke_test_passed"].empty())
			{
				bBool = device["component_smoke_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_heat_test_passed"].empty())
			{
				bBool = device["component_heat_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_buzzer_test_passed"].empty())
			{
				bBool = device["component_buzzer_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_us_test_passed"].empty())
			{
				bBool = device["component_us_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_temp_test_passed"].empty())
			{
				bBool = device["component_temp_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_wifi_test_passed"].empty())
			{
				bBool = device["component_wifi_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_als_test_passed"].empty())
			{
				bBool = device["component_als_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_co_test_passed"].empty())
			{
				bBool = device["component_co_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			if (!device["component_hum_test_passed"].empty())
			{
				bBool = device["component_hum_test_passed"].asBool();
				if (!bBool)
					bIAlarm = true;
			}
			UpdateSmokeSensor(SwitchIndex, bIAlarm, devName);
			SwitchIndex++;
		}
	}

	//Thermostat
	if (!bHaveShared)
		return;
	if (root["shared"].size()<1)
	{
		if (bHaveTopaz)
			return;
		_log.Log(LOG_ERROR, "Nest: request not successful, restarting..!");
		m_bDoLogin = true;
		return;
	}

	size_t iThermostat = 0;
	for (Json::Value::iterator ittStructure = root["structure"].begin(); ittStructure != root["structure"].end(); ++ittStructure)
	{
		Json::Value nstructure = *ittStructure;
		if (!nstructure.isObject())
			continue;
		std::string StructureID = ittStructure.key().asString();
		std::string StructureName = nstructure["name"].asString();

		for (Json::Value::iterator ittDevice = nstructure["devices"].begin(); ittDevice != nstructure["devices"].end(); ++ittDevice)
		{
			std::string devID = (*ittDevice).asString();
			if (devID.find("device.")==std::string::npos)
				continue;
			std::string Serial = devID.substr(7);
			if (root["device"].empty())
				continue;
			if (root["device"][Serial].empty())
				continue; //not found !?
			if (root["shared"][Serial].empty())
				continue; //Nothing shared?


			Json::Value ndevice = root["device"][Serial];
			if (!ndevice.isObject())
				continue;

			std::string Name = "Thermostat";
			if (!ndevice["where_id"].empty())
			{
				//Lookup our 'where' (for the Name of the thermostat)
				std::string where_id = ndevice["where_id"].asString();

				if (!root["where"].empty())
				{
					if (!root["where"][StructureID].empty())
					{
						for (Json::Value::iterator ittWheres = root["where"][StructureID]["wheres"].begin(); ittWheres != root["where"][StructureID]["wheres"].end(); ++ittWheres)
						{
							Json::Value nwheres = *ittWheres;
							if (nwheres["where_id"] == where_id)
							{
								Name = StructureName + " " + nwheres["name"].asString();
								break;
							}
						}
					}
				}
			}

			_tNestThemostat ntherm;
			ntherm.Serial = Serial;
			ntherm.StructureID = StructureID;
			ntherm.Name = Name;
			m_thermostats[iThermostat] = ntherm;

			Json::Value nshared = root["shared"][Serial];

			//Setpoint
			if (!nshared["target_temperature"].empty())
			{
				float currentSetpoint = nshared["target_temperature"].asFloat();
				SendSetPointSensor((const unsigned char)(iThermostat * 3) + 1, currentSetpoint, Name + " Setpoint");
			}
			//Room Temperature/Humidity
			if (!nshared["current_temperature"].empty())
			{
				float currentTemp = nshared["current_temperature"].asFloat();
				int Humidity = root["device"][Serial]["current_humidity"].asInt();
				SendTempHumSensor((iThermostat * 3) + 2, 255, currentTemp, Humidity, Name + " TempHum");
			}

			// Check if thermostat is currently Heating
			if (nshared["can_heat"].asBool() && !nshared["hvac_heater_state"].empty())
			{
				bool bIsHeating = nshared["hvac_heater_state"].asBool();
				UpdateSwitch((unsigned char)(113 + (iThermostat * 3)), bIsHeating, Name + " HeatingOn");
			}

			// Check if thermostat is currently Cooling
			if (nshared["can_cool"].asBool() && !nshared["hvac_ac_state"].empty())
			{
				bool bIsCooling = nshared["hvac_ac_state"].asBool();
				UpdateSwitch((unsigned char)(114 + (iThermostat * 3)), bIsCooling, Name + " CoolingOn");
			}

			//Away
			if (!nstructure["away"].empty())
			{
				bool bIsAway = nstructure["away"].asBool();
				SendSwitch((iThermostat * 3) + 3, 1, 255, bIsAway, 0, Name + " Away");
			}

			//Manual Eco mode
			if (!ndevice["eco"]["mode"].empty())
			{
				std::string sCurrentHvacMode = ndevice["eco"]["mode"].asString();
				bool bIsManualEcoMode = (sCurrentHvacMode == "manual-eco");
				SendSwitch((iThermostat * 3) + 4, 1, 255, bIsManualEcoMode, 0, Name + " Manual Eco Mode");
			}

			iThermostat++;
		}
	}
}

void CNest::SetSetpoint(const int idx, const float temp)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	if (m_bDoLogin == true)
	{
		if (!Login())
			return;
	}
	size_t iThermostat = (idx - 1) / 3;
	if (iThermostat > m_thermostats.size())
		return;

	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	float tempDest = temp;
	unsigned char tSign = m_sql.m_tempsign[0];
	if (tSign == 'F')
	{
		tempDest = static_cast<float>(ConvertToCelsius(tempDest));
	}

	Json::Value root;
	root["target_change_pending"] = true;
	root["target_temperature"] = tempDest;

	std::string sResult;

	std::string sURL = m_TransportURL + NEST_SET_SHARED + m_thermostats[iThermostat].Serial;
	if (!HTTPClient::POST(sURL, root.toStyledString(), ExtraHeaders, sResult, true, true))
	{
		_log.Log(LOG_ERROR, "Nest: Error setting setpoint!");
		m_bDoLogin = true;
		return;
	}
	GetMeterDetails();
}

bool CNest::SetAway(const unsigned char Idx, const bool bIsAway)
{
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;

	if (m_bDoLogin == true)
	{
		if (!Login())
			return false;
	}

	size_t iThermostat = (Idx - 3) / 3;
	if (iThermostat > m_thermostats.size())
		return false;

	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	Json::Value root;
	root["away"] = bIsAway;
	root["away_timestamp"] = (int)mytime(NULL);
	root["away_setter"] = 0;

	std::string sResult;

	std::string sURL = m_TransportURL + NEST_SET_STRUCTURE + m_thermostats[iThermostat].StructureID;
	if (!HTTPClient::POST(sURL, root.toStyledString(), ExtraHeaders, sResult, true, true))
	{
		_log.Log(LOG_ERROR, "Nest: Error setting away mode!");
		m_bDoLogin = true;
		return false;
	}
	return true;
}

bool CNest::SetManualEcoMode(const unsigned char Idx, const bool bIsManualEcoMode)
{
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;

	if (m_bDoLogin == true)
	{
		if (!Login())
			return false;
	}

	size_t iThermostat = (Idx - 4) / 3;
	if (iThermostat > m_thermostats.size())
		return false;

	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("user-agent:Nest/1.1.0.10 CFNetwork/548.0.4");
	ExtraHeaders.push_back("Authorization:Basic " + m_AccessToken);
	ExtraHeaders.push_back("X-nl-protocol-version:1");

	Json::Value root;
	Json::Value eco;

	eco["mode"] = (bIsManualEcoMode ? "manual-eco" : "schedule");
	root["eco"] = eco;

	std::string sResult;

	// If thermostat information has not yet been read we can't do anything so let's fail.
	if (m_thermostats[iThermostat].Serial.empty())
		return false;

	std::string sURL = m_TransportURL + NEST_SET_DEVICE + m_thermostats[iThermostat].Serial;
	if (!HTTPClient::POST(sURL, root.toStyledString(), ExtraHeaders, sResult, true, true))
	{
		_log.Log(LOG_ERROR, "Nest: Error setting manual eco mode!");
		m_bDoLogin = true;
		return false;
	}
	return true;
}

void CNest::SetProgramState(const int newState)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	if (m_bDoLogin)
	{
		if (!Login())
			return;
	}
}
