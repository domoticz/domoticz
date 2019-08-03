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
// Anna Sensors

// Anna Switches
                 
#define sAnneBoilerState 8				 
#define sAnnaFlameState  9
#define sAnnaProximity   10
#define sAnnaPresets     11
#define sAnnaComfort     12

const std::string ANNA_GET_STATUS   = "/core/appliances";
const std::string ANNA_LOCATION     = "/cache/domain_objects;class=Location";
const std::string ANNA_SET_LOCATION = "/core/locations";


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
	FILE *fOut = fopen(filename.c_str(), "ab+");
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

void CAnnaThermostat::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"AnnaTherm: Error: %s",e.what());
}
void CAnnaThermostat::CAnnaThermostat::Init()
{
	m_ThermostatID = "";
	m_ProximityID = "";
	m_AnnaLocation.m_ALocationID = "";
	m_AnnaLocation.m_ALocationName = "";
	m_AnnaLocation.m_ALocationType= ""; 
}

bool CAnnaThermostat::StartHardware()
{
	RequestStart();

	Init();

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CAnnaThermostat::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
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
    m_bIsStarted=false;
    return true;
}

#define ANNA_POLL_INTERVAL 30

void CAnnaThermostat::Do_Work()
{
	bool bFirstTime = true;
	Log(LOG_STATUS,"AnnaTherm:Worker started...");
	int sec_counter = ANNA_POLL_INTERVAL-5;
	while (!IsStopRequested(1000))
	{
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
	Log(LOG_STATUS,"AnnaTherm: Worker stopped...");
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
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	if (packettype == pTypeLighting2)
	{	
		int node_id = pCmd->LIGHTING2.id4;
		bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);
		if (node_id == sAnneBoilerState || node_id ==sAnnaFlameState )
		{
		    return false ; // just return Error as these are not supposed to be switches 
		}
		else if (node_id == sAnnaProximity)
		{
			return AnnaToggleProximity(bIsOn);
		}
	}	
	else if (packettype ==  pTypeGeneralSwitch)
	{
		const _tGeneralSwitch *xcmd = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int ID = xcmd->id;
		int level = xcmd->level;
		return AnnaSetPreset(level);
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
		return ;
	}
}
bool CAnnaThermostat::AnnaSetPreset( std::int8_t level)
{
	std::stringstream szURL;

	if(!CheckLoginData())
	    return false;

	if (m_AnnaLocation.m_ALocationID.size() == 0)
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
	switch(level) {
    case  0: 
			strcpy(szTemp,"none");
			break;
	case 10: 
			strcpy(szTemp,"home");
			break;
	case 20: 
			strcpy(szTemp,"away");			
			break;
	case 30: 
			strcpy(szTemp,"asleep");			
			break;
	case 40: 
			strcpy(szTemp,"vacation");			
			break;
    default: 
            strcpy(szTemp,"none");			
			break;
	}
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
    SaveString2Disk("<-- ANNA - SetPReset -->/ \r\n",DEBUG_ANNA_WRITE);
    SaveString2Disk(szURL.str(),DEBUG_ANNA_WRITE);
  	SaveString2Disk(DEBUG_ANNA_CRLF,DEBUG_ANNA_WRITE);
	SaveString2Disk(sPostData.str(),DEBUG_ANNA_WRITE);
	SaveString2Disk(DEBUG_ANNA_CRLF,DEBUG_ANNA_WRITE);

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

	if(!CheckLoginData())
	    return false;

	if (m_ProximityID.size() == 0)
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
		strcpy(szTemp,"on");
	}
	else
	{
		strcpy(szTemp,"off");
	}
	sPostData << "<toggle>";
	sPostData << "<state>";
	sPostData << szTemp;
	sPostData << "</state>";
	sPostData << "</toggle>";

#ifdef DEBUG_AnnaThermostat
    SaveString2Disk("<-- ANNA - TogglePRoximitySensor-->",DEBUG_ANNA_WRITE);
	SaveString2Disk(DEBUG_ANNN_CRLF,DEBUG_ANNA_WRITE);
	SaveString2Disk(szURL.str(),DEBUG_ANNA_WRITE);	
	SaveString2Disk(DEBUG_ANNN_CRLF,DEBUG_ANNA_WRITE);
	SaveString2Disk(sPostData.str(),DEBUG_ANNA_WRITE);
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
	if(!CheckLoginData())
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

	std::string sResult;
    if(!CheckLoginData())
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
	if (doc.Parse(sResult.c_str(), 0, TIXML_ENCODING_UTF8) && doc.Error())
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot parse XML");
		return;
	}

	TiXmlElement *pRoot;
	TiXmlElement *pAppliance, *pElem;
	TiXmlAttribute *pAttribute;
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
		if (pElem == NULL)
		{
			Log(LOG_ERROR, "AnnaTherm: Cannot find appliance attributes");
			return;
		}
		std::string ApplianceName=pElem->GetText();

		if ((m_ThermostatID.empty()) && ((ApplianceName == "Anna") || (ApplianceName == "Adam")))
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
			Log(LOG_ERROR, "AnnaTherm: Cannot find logs in XML");
			return;
		}
		TiXmlHandle hLogs = TiXmlHandle(pElem);
		pElem = hAppliance.FirstChild("logs").Child("point_log", 0).ToElement();
		if (!pElem)
		{
			Log(LOG_ERROR, "AnnaTherm: No log points found in XML");
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
                    if(strcmp(tmpstr.c_str(), "on") == 0)
                    {
                        SendSwitch(sAnneBoilerState, 1, 255, true, 0, sname);
                    }
                    else
                    {
                        SendSwitch(sAnneBoilerState, 1, 255, false, 0, sname);
                    }
                }
            }
            else if (sname == "flame_state")
            {
                tmpstr = GetPeriodMeasurement(pElem);
                if (!tmpstr.empty())
                {
                    if(strcmp(tmpstr.c_str(), "on") == 0)
                    {

                        SendSwitch(sAnnaFlameState, 1, 255, true, 0, sname);
                    }
                    else
                    {
                        SendSwitch(sAnnaFlameState, 1, 255, false, 0, sname);
                    }
				//make device ID
				const int NodeID = sAnnaFlameState;
				unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
				unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
				unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
				unsigned char ID4 = (unsigned char)NodeID & 0xFF;

				char szIdx[10];
				sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
                //m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (HardwareID==%d) AND (DeviceID=='%q')", 5, m_HwdID,szIdx);
                }

            }
			else if (sname == "proximity_sensor_state")
			{
				bool bSwitch = false;
				if (m_ProximityID.empty())
				{
					pAttribute = pAppliance->FirstAttribute();
					if (pAttribute != NULL)
					{
						std::string aName = pAttribute->Name();
						if (aName == "id")
						{
							m_ProximityID= pAttribute->Value();
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
                    if(strcmp(tmpstr.c_str(), "on") == 0)
                    {
					    bSwitch = true;
                    }
                    else
                    {
					   bSwitch = false;
                    }
			     	SendSwitch(sAnnaProximity, 0, 255, bSwitch,0, sname);
					//m_sql.safe_query("UPDATE DeviceStatus SET CustomImage=%d WHERE (HardwareID==%d) AND (DeviceID=='%08X')", 12, m_HwdID,sAnnaProximity);
				}			
			}			
            else if (sname == "preset_state")
			{
				char sPreset[4];
				if (m_AnnaLocation.m_ALocationID.empty())
				{    
					if(!AnnaGetLocation()){
						Log(LOG_ERROR, "AnnaTherm: Error getting Location Information !");
						return;
					}	 
				}
				tmpstr = GetPeriodMeasurement(pElem);
                if(strcmp(tmpstr.c_str(), "none") == 0)
                {
				    strncpy ( sPreset,  "00", sizeof(sPreset) );
                }
                else if (strcmp(tmpstr.c_str(), "home") == 0)
                {
					   strncpy ( sPreset,  "10", sizeof(sPreset) );
                }
				else if (strcmp(tmpstr.c_str(), "away") == 0)
                {
					   strncpy ( sPreset,  "20", sizeof(sPreset) );
                }
				else if (strcmp(tmpstr.c_str(), "asleep") == 0)
                {
					   strncpy ( sPreset,  "30", sizeof(sPreset) );
                }
				else if (strcmp(tmpstr.c_str(), "vacation") == 0)
                {
					   strncpy ( sPreset,  "40", sizeof(sPreset) );
                }
				else if (strcmp(tmpstr.c_str(), "no_frost") == 0)
				{
					strncpy ( sPreset,  "00", sizeof(sPreset) );
				}	
				int customImage = 16;//Frost
				std::string options_str;
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%08X')", m_HwdID,sAnnaPresets);
				if (result.empty()) //Switch is not yet in the system so create it
				{   
					std::string options_str = "";
					options_str = m_sql.FormatDeviceOptions(m_sql.BuildDeviceOptions( "SelectorStyle:0;LevelNames:Off|Home|Away|Night|Vacation;LevelOffHidden:true;LevelActions:00|10|20|30|40", false));
					m_sql.safe_query(
				"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Options) "
				"VALUES (%d, '%08X', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q')", m_HwdID, sAnnaPresets, 0, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector,0, "Anna Presets", sPreset, customImage, options_str.c_str());
				}
				else
				{
					result = m_sql.safe_query("UPDATE DeviceStatus SET sValue = '%q' where (HardwareID==%d) AND (DeviceID=='%08X')", sPreset, m_HwdID,sAnnaPresets);	
				}								
			}			
		}
		pAppliance = pAppliance->NextSiblingElement("appliance");
	}
	return;
}

// Checks if the USername and password are filled in
bool CAnnaThermostat::CheckLoginData()
{
	if (m_UserName.size() == 0)
		return false;
	if (m_Password.size() == 0)
		return false;
	return true;	
}

// Fetched the location information (id,name and type)from the  first location given by the Anna/Adam Gateway					
 bool CAnnaThermostat::AnnaGetLocation()
 {	 
	std::string sResult;
    if(!CheckLoginData())
	    return false ;
    
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
	if (doc.Parse(sResult.c_str(), 0, TIXML_ENCODING_UTF8) && doc.Error())
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot parse XML");
		return false;
	}

	TiXmlElement *pRoot;
	TiXmlElement *pLocation, *pElem;
	TiXmlAttribute *pAttribute;
	std::string sname, tmpstr;

	pRoot = doc.FirstChildElement("domain_objects");
	if (!pRoot)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find domain info in XML");
		return false;
	}
	pLocation = pRoot->FirstChildElement("location");
	TiXmlHandle hAppliance = TiXmlHandle(pLocation);
	if (m_AnnaLocation.m_ALocationID.empty()) 
	{
		pAttribute = pLocation->FirstAttribute();
		if (pAttribute != NULL)
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
	if (pElem == NULL)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find Location name");
		return false;
	}
	m_AnnaLocation.m_ALocationName=pElem->GetText();
	pElem = pLocation->FirstChildElement("type");
	if (pElem == NULL)
	{
		Log(LOG_ERROR, "AnnaTherm: Cannot find Location type");
		return false;
	}	m_AnnaLocation.m_ALocationType=pElem->GetText();
	return true;
}
