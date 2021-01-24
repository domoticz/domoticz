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

#include "../tinyxpath/tinyxml.h"

#define round(a) ( int ) ( a + .5 )

// Plugwise Anna Thermostat
// Anna Sensors
// Anna Switches

#define sAnneBoilerState 8
#define sAnnaFlameState  9
#define sAnnaProximity   10
#define sAnnaPresets     11
#define sAnnaComfort     12

const std::string ANNA_VERSION       = "1.0.1";
const std::string ANNA_GET_STATUS    = "/core/appliances";
const std::string ANNA_LOCATION      = "/cache/domain_objects;class=Location";
const std::string ANNA_SET_LOCATION  = "/core/locations";
const std::string ANNA_LEVEL_NAMES   = "Off|Home|Away|Night|Vacation|Frost";
const std::string ANNA_LEVEL_ACTIONS = "00|10|20|30|40|50";



//#define _DEBUG // toggle for reading and writing local files
#ifdef _DEBUG
//#define DEBUG_AnnaThermostat
#define DEBUG_ANNA_APPLIANCE_READ  "/tmp/anna/appliances.xml"
#define DEBUG_ANNA_WRITE           "/tmp/anna/output.txt"
#define DEBUG_ANNA_LOCATION_READ   "/tmp/anna/location.xml"
#define DEBUG_ANNA_CRLF          " \r\n"
#endif

#ifdef DEBUG_AnnaThermostat
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "ab+");
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

CAnnaThermostat::CAnnaThermostat(const int ID, const std::string& IPAddress, const unsigned short usIPPort, const std::string& Username, const std::string& Password) :
	m_IPAddress(IPAddress),
	m_IPPort(usIPPort),
	m_UserName(CURLEncode::URLEncode(Username)),
	m_Password(CURLEncode::URLEncode(Password))
{
	m_HwdID = ID;
	Init();
}

void CAnnaThermostat::OnError(const std::exception &e)
{
	_log.Log(LOG_ERROR, "AnnaTherm: Error: %s", e.what());
}
void CAnnaThermostat::CAnnaThermostat::Init()
{
	m_ThermostatID = "";
	m_ProximityID = "";
	m_AnnaLocation.m_ALocationID = "";
	m_AnnaLocation.m_ALocationName = "";
	m_AnnaLocation.m_ALocationType = "";
}

bool CAnnaThermostat::StartHardware()
{

	RequestStart();
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CAnnaThermostat::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CAnnaThermostat::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

#define ANNA_POLL_INTERVAL 30

void CAnnaThermostat::Do_Work()
{
	bool bFirstTime = true;
	Log(LOG_STATUS, "AnnaTherm:Worker started...");
	int sec_counter = ANNA_POLL_INTERVAL - 5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(nullptr);
		}

		if ((sec_counter % ANNA_POLL_INTERVAL == 0) || (bFirstTime))
		{
			sec_counter = 0;
			if (bFirstTime)
			{
				FixUnit(); // Making sure the unit ID's are set to 1.  Req for future extensions
				InitialMessageMigrateCheck();
				bFirstTime = false;
			}
			GetMeterDetails();
		}

	}
	Log(LOG_STATUS, "AnnaTherm: Worker stopped...");
}

void CAnnaThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string& defaultname)
{
	_tThermostat thermos;
	thermos.subtype = sTypeThermSetpoint;
	thermos.id1 = 0;
	thermos.id2 = 0;
	thermos.id3 = 0;
	thermos.id4 = Idx;
	thermos.dunit = 1;
	thermos.temp = Temp;
	sDecodeRXMessage(this, (const unsigned char *)&thermos, defaultname.c_str(), 255, nullptr);
}

bool CAnnaThermostat::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	if (packettype == pTypeLighting2)
	{
		int node_id = pCmd->LIGHTING2.id4;
		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		if (node_id == sAnneBoilerState || node_id == sAnnaFlameState)
		{
			return false; // just return Error as these are not supposed to be switches
		}
		if (node_id == sAnnaProximity)
		{
			return AnnaToggleProximity(bIsOn);
		}
	}
	else if (packettype == pTypeGeneralSwitch)
	{
		const _tGeneralSwitch* xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		return AnnaSetPreset(xcmd->level);
	}
	return false;
}

void CAnnaThermostat::SetSetpoint(const int /*idx*/, const float temp)
{
	std::stringstream szURL;
	if (!CheckLoginData())
		return;

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

	if (!HTTPClient::PUT(szURL.str(), sPostData.str(), ExtraHeaders, sResult, true))
	{
		Log(LOG_ERROR, "AnnaTherm: Error setting setpoint!");
		return;
	}
}

bool CAnnaThermostat::AnnaSetPreset(uint8_t level)
{
	std::stringstream szURL;

	if (!CheckLoginData())
		return false;

	if (m_AnnaLocation.m_ALocationID.empty())
		AnnaGetLocation();

	if (m_Password.empty())
	{
		szURL << "http://" << m_IPAddress << ":" << m_IPPort;
	}
	else
	{
		szURL << "http://" << m_UserName << ":" << m_Password << "@" << m_IPAddress << ":" << m_IPPort;
	}
	szURL << ANNA_SET_LOCATION;
	szURL << ";id=";
	szURL << m_AnnaLocation.m_ALocationID;

	std::stringstream sPostData;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	char szTemp[10];
	switch (level) {
	case 0:
		strcpy(szTemp, "none");
		break;
	case 10:
		strcpy(szTemp, "home");
		break;
	case 20:
		strcpy(szTemp, "away");
		break;
	case 30:
		strcpy(szTemp, "asleep");
		break;
	case 40:
		strcpy(szTemp, "vacation");
		break;
	case 50:
		strcpy(szTemp, "no_frost");
		break;
	default:
		Log(LOG_STATUS, "AnnaTherm: Invalid value for Preset %i .. Aborting Switch", level);
		return false;
	}
	Log(LOG_STATUS, "Switching Anna gateway preset to: %s", szTemp);
	sPostData << "<locations>";
	sPostData << "<location id=\"";
	sPostData << m_AnnaLocation.m_ALocationID;
	sPostData << "\"><name>";
	sPostData << m_AnnaLocation.m_ALocationName;
	sPostData << "</name>";
	sPostData << "<type>";
	sPostData << m_AnnaLocation.m_ALocationType;
	sPostData << "</type>";
	sPostData << "<preset>";
	sPostData << szTemp;
	sPostData << "</preset>";
	sPostData << "</location>";
	sPostData << "</locations>";
#ifdef DEBUG_AnnaThermostat
	SaveString2Disk("<-- ANNA - SetPReset -->/ \r\n", DEBUG_ANNA_WRITE);
	SaveString2Disk(szURL.str(), DEBUG_ANNA_WRITE);
	SaveString2Disk(DEBUG_ANNA_CRLF, DEBUG_ANNA_WRITE);
	SaveString2Disk(sPostData.str(), DEBUG_ANNA_WRITE);
	SaveString2Disk(DEBUG_ANNA_CRLF, DEBUG_ANNA_WRITE);

#else
	if (!HTTPClient::PUT(szURL.str(), sPostData.str(), ExtraHeaders, sResult, true))
	{
		Log(LOG_ERROR, "AnnaTherm: Error setting Preset State !");
		return false;
	}
#endif
	 return true;
}

bool CAnnaThermostat::AnnaToggleProximity(bool bToggle)
{
	std::stringstream szURL;

	if (!CheckLoginData())
		return false;

	if (m_ProximityID.empty())
		GetMeterDetails();

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
	szURL << m_ProximityID;
	szURL << "/toggle;type=proximity_sensor_state";

	std::stringstream sPostData;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	char szTemp[10];
	if (bToggle == true)
	{
		strcpy(szTemp, "on");
	}
	else
	{
		strcpy(szTemp, "off");
	}
	sPostData << "<toggle>";
	sPostData << "<state>";
	sPostData << szTemp;
	sPostData << "</state>";
	sPostData << "</toggle>";

#ifdef DEBUG_AnnaThermostat
	SaveString2Disk("<-- ANNA - TogglePRoximitySensor-->", DEBUG_ANNA_WRITE);
	SaveString2Disk(DEBUG_ANNA_CRLF, DEBUG_ANNA_WRITE);
	SaveString2Disk(szURL.str(), DEBUG_ANNA_WRITE);
	SaveString2Disk(DEBUG_ANNA_CRLF, DEBUG_ANNA_WRITE);
	SaveString2Disk(sPostData.str(), DEBUG_ANNA_WRITE);
#else
	if (!HTTPClient::PUT(szURL.str(), sPostData.str(), ExtraHeaders, sResult, true))
	{
		Log(LOG_ERROR, "AnnaTherm: Error setting toggle Proximity !");
		return false;
	}
#endif
	return true;
}
bool CAnnaThermostat::SetAway(const bool /*bIsAway*/)
{
	return false;
}

void CAnnaThermostat::SetProgramState(const int /*newState*/)
{
	if (!CheckLoginData())
		return;
}

std::string GetElementChildValue(TiXmlElement* pElement, const char* szChildName)
{
	std::string ret;
	TiXmlNode* pNode = pElement->FirstChild(szChildName);
	if (!pNode)
		return ret;
	pNode = pNode->FirstChild();
	if (!pNode)
		return ret;
	ret = pNode->Value();
	return ret;
}

std::string GetPeriodMeasurement(TiXmlElement* pElement)
{
	std::string ret;
	TiXmlNode* pNode = pElement->FirstChild("period");
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

	std::string sResult;
	if (!CheckLoginData())
		return;

#ifdef DEBUG_AnnaThermostat
	sResult = ReadFile(DEBUG_ANNA_APPLIANCE_READ);
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
		Log(LOG_ERROR, "AnnaTherm: Error getting current state!");
		return;
	}
#endif
	if (sResult.empty())
	{
		Log(LOG_ERROR, "AnnaTherm: No or invalid data received!");
		return;
	}
	TiXmlDocument doc;
	if (doc.Parse(sResult.c_str(), nullptr, TIXML_ENCODING_UTF8) && doc.Error())
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot parse XML");
		return;
	}

	TiXmlElement* pRoot;
	TiXmlElement* pAppliance, * pElem;
	TiXmlAttribute* pAttribute;
	std::string sname, tmpstr;

	pRoot = doc.FirstChildElement("appliances");
	if (!pRoot)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find appliances in XML");
		return;
	}
	pAppliance = pRoot->FirstChildElement("appliance");
	while (pAppliance)
	{
		TiXmlHandle hAppliance = TiXmlHandle(pAppliance);

		pElem = pAppliance->FirstChildElement("name");
		if (pElem == nullptr)
		{
			Log(LOG_ERROR, "AnnaTherm: Cannot find appliance attributes");
			return;
		}
		std::string ApplianceName = pElem->GetText();

		if ((m_ThermostatID.empty()) && ((ApplianceName == "Anna") || (ApplianceName == "Adam")))
		{
			pAttribute = pAppliance->FirstAttribute();
			if (pAttribute != nullptr)
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
			Log(LOG_ERROR, "AnnaTherm: Cannot find logs in XML");
			return;
		}
		//TiXmlHandle hLogs = TiXmlHandle(pElem);
		pElem = hAppliance.FirstChild("logs").Child("point_log", 0).ToElement();
		if (!pElem)
		{
			Log(LOG_ERROR, "AnnaTherm: No log points found in XML");
			return;
		}
		for (pElem; pElem; pElem = pElem->NextSiblingElement())
		{

			sname = GetElementChildValue(pElem, "type");
			//tmpstr = GetPeriodMeasurement(pElem);
			//Log (LOG_NORM,"%s : %s ", sname.c_str(), tmpstr.c_str());
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
			else if (sname == "intended_boiler_temperature")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					float temperature = (float)atof(tmpstr.c_str());
					SendTempSensor(4, 255, temperature, sname);
				}
			}
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
			else if (sname == "boiler_state")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					if (strcmp(tmpstr.c_str(), "on") == 0)
					{
						SendSwitch(sAnneBoilerState, 1, 255, true, 0, sname, m_Name);
					}
					else
					{
						SendSwitch(sAnneBoilerState, 1, 255, false, 0, sname, m_Name);
					}
				}
			}
			else if (sname == "flame_state")
			{
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					if (strcmp(tmpstr.c_str(), "on") == 0)
					{

						SendSwitch(sAnnaFlameState, 1, 255, true, 0, sname, m_Name);
					}
					else
					{
						SendSwitch(sAnnaFlameState, 1, 255, false, 0, sname, m_Name);
					}
				}
			}
			else if (sname == "proximity_sensor_state")
			{
				bool bSwitch = false;
				if (m_ProximityID.empty())
				{
					pAttribute = pAppliance->FirstAttribute();
					if (pAttribute != nullptr)
					{
						std::string aName = pAttribute->Name();
						if (aName == "id")
						{
							m_ProximityID = pAttribute->Value();
						}
					}
				}
				if (m_ProximityID.empty())
				{
					Log(LOG_ERROR, "AnnaTherm: Error getting ProximityID !");
					return;
				}
				tmpstr = GetPeriodMeasurement(pElem);
				if (!tmpstr.empty())
				{
					if (strcmp(tmpstr.c_str(), "on") == 0)
					{
						bSwitch = true;
					}
					else
					{
						bSwitch = false;
					}
					SendSwitch(sAnnaProximity, 1, 255, bSwitch, 0, sname, m_Name);
				}
			}
			else if (sname == "preset_state")
			{
				char sPreset[4];
				if (m_AnnaLocation.m_ALocationID.empty())
				{
					if (!AnnaGetLocation()) {
						Log(LOG_ERROR, "AnnaTherm: Error getting Location Information !");
						return;
					}
				}
				tmpstr = GetPeriodMeasurement(pElem);
				if (strcmp(tmpstr.c_str(), "none") == 0)
				{
					strncpy(sPreset, "00", sizeof(sPreset));
				}
				else if (strcmp(tmpstr.c_str(), "home") == 0)
				{
					strncpy(sPreset, "10", sizeof(sPreset));
				}
				else if (strcmp(tmpstr.c_str(), "away") == 0)
				{
					strncpy(sPreset, "20", sizeof(sPreset));
				}
				else if (strcmp(tmpstr.c_str(), "asleep") == 0)
				{
					strncpy(sPreset, "30", sizeof(sPreset));
				}
				else if (strcmp(tmpstr.c_str(), "vacation") == 0)
				{
					strncpy(sPreset, "40", sizeof(sPreset));
				}
				else if (strcmp(tmpstr.c_str(), "no_frost") == 0)
				{
					strncpy(sPreset, "50", sizeof(sPreset));
				}
				else strncpy(sPreset, "50", sizeof(sPreset));

				std::string PresetName = "Anna Preset";
				SendSelectorSwitch(sAnnaPresets, 1, sPreset, PresetName, 16, false, ANNA_LEVEL_NAMES, ANNA_LEVEL_ACTIONS, true, m_Name);
			}
		}
		pAppliance = pAppliance->NextSiblingElement("appliance");
	}
}

// Checks if the Username and password are filled in
bool CAnnaThermostat::CheckLoginData()
{
	if (m_UserName.empty())
		return false;
	if (m_Password.empty())
		return false;
	return true;
}

// Fetched the location information (id,name and type)from the  first location given by the Anna/Adam Gateway
bool CAnnaThermostat::AnnaGetLocation()
{
	std::string sResult;
	if (!CheckLoginData())
		return false;

#ifdef DEBUG_AnnaThermostat
	sResult = ReadFile(DEBUG_ANNA_LOCATION_READ);
#else
	//Get Location  Data
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_IPAddress << ":" << m_IPPort;
	}
	else
	{
		szURL << "http://" << m_UserName << ":" << m_Password << "@" << m_IPAddress << ":" << m_IPPort;
	}
	szURL << ANNA_LOCATION;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		Log(LOG_ERROR, "AnnaTherm: Error getting location Info !");
		return false;
	}
#endif
	if (sResult.empty())
	{
		Log(LOG_ERROR, "AnnaTherm: No or invalid location data received!");
		return false;
	}
	TiXmlDocument doc;
	if (doc.Parse(sResult.c_str(), nullptr, TIXML_ENCODING_UTF8) && doc.Error())
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot parse XML");
		return false;
	}

	TiXmlElement* pRoot;
	TiXmlElement* pLocation, * pElem;
	TiXmlAttribute* pAttribute;
	std::string sname, tmpstr;

	pRoot = doc.FirstChildElement("domain_objects");
	if (!pRoot)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find domain info in XML");
		return false;
	}
	pLocation = pRoot->FirstChildElement("location");
	//TiXmlHandle hAppliance = TiXmlHandle(pLocation);
	if (m_AnnaLocation.m_ALocationID.empty())
	{
		pAttribute = pLocation->FirstAttribute();
		if (pAttribute != nullptr)
		{
			std::string aName = pAttribute->Name();
			if (aName == "id")
			{
				m_AnnaLocation.m_ALocationID = pAttribute->Value();
			}
			else
			{
				Log(LOG_ERROR, "AnnaTherm: Cannot get location id ");
				return false;
			}
		}
	}
	pElem = pLocation->FirstChildElement("name");
	if (pElem == nullptr)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find Location name");
		return false;
	}
	m_AnnaLocation.m_ALocationName = pElem->GetText();
	pElem = pLocation->FirstChildElement("type");
	if (pElem == nullptr)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find Location type");
		return false;
	}
	m_AnnaLocation.m_ALocationType = pElem->GetText();
	return true;
}


bool CAnnaThermostat::InitialMessageMigrateCheck()
{
	bool retval = false;
	// Showing we are starting Annatherm and
	// notify users in the log how to use the event system once per startup
	Log(LOG_NORM, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	Log(LOG_NORM, "Plugwise Plugin Version: %s", ANNA_VERSION.c_str() );
	Log(LOG_NORM, "Notice: To change the preset by way of scripts/events, please use the following percentages levels:");
	Log(LOG_NORM, "10%% for Home, 20%% for Away, 30%% for Night, 40%% for Vacation & 50%% for Frost (Lua use 'Set Level: 20')");
	Log(LOG_NORM, "Any other value will be refused! ");
	Log(LOG_NORM, "To Detect changes in scripts use 'Off, Home, Away, Night, Vacation, Frost'");
	Log(LOG_NORM, "Adjusting the Temp manually on the ANNA or via App, forces the ANNA to set Scenes to Off!!");
	Log(LOG_NORM, "KNOWN ISSUE: The Gateway will not send an correct update if the previous scene is choosen again!");
	Log(LOG_NORM, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

	if( MigrateSelectorSwitch(sAnnaPresets, 1, ANNA_LEVEL_NAMES, ANNA_LEVEL_ACTIONS,true)== 1)
	{
		Log(LOG_STATUS, "Scene Selector switch updated to Version: %s ! ", ANNA_VERSION.c_str());
		retval = true;
	}
	return retval;
}
void CAnnaThermostat::FixUnit()
{   // This function will make sure that from now on  that the Preset switch in the Plugwise plugin will be using unit 0
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit == '%d')", m_HwdID,  0);
	if (result.empty())
		return;  // switch doen not exist yet
	
	for (const auto &it : result)
	{
		m_sql.safe_query("UPDATE DeviceStatus SET unit = 1 WHERE (ID==%s)", it.at(0).c_str());
	}
}
