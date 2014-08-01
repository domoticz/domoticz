#include "stdafx.h"
#include "ToonThermostat.h"
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

//#define DEBUG_ToonThermostat

#ifdef DEBUG_ToonThermostat
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

CToonThermostat::CToonThermostat(const int ID, const std::string &Username, const std::string &Password)
{
	m_HwdID=ID;
	m_UserName=Username;
	m_Password=Password;
	m_ClientID = "";

	memset(&m_p1power, 0, sizeof(m_p1power));
	memset(&m_p1gas, 0, sizeof(m_p1gas));

	m_p1power.len = sizeof(P1Power)-1;
	m_p1power.type = pTypeP1Power;
	m_p1power.subtype = sTypeP1Power;

	m_p1gas.len = sizeof(P1Gas)-1;
	m_p1gas.type = pTypeP1Gas;
	m_p1gas.subtype = sTypeP1Gas;

	Init();
}

CToonThermostat::~CToonThermostat(void)
{
}

void CToonThermostat::Init()
{
	m_ClientIDChecksum = "";
	m_stoprequested = false;
	m_lastSharedSendElectra = 0;
	m_lastSharedSendGas = 0;
	m_lastgasusage = 0;
	m_lastelectrausage = 0;
}

bool CToonThermostat::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CToonThermostat::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CToonThermostat::StopHardware()
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

#define TOON_POLL_INTERVAL 1

void CToonThermostat::Do_Work()
{
	int LastMinute=-1;

	_log.Log(LOG_STATUS,"ToonThermostat: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		time_t atime=mytime(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);
		if (ltime.tm_min/TOON_POLL_INTERVAL!=LastMinute)
		{
			LastMinute=ltime.tm_min/TOON_POLL_INTERVAL;
			GetMeterDetails();
		}

		if (ltime.tm_sec % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
	}
	_log.Log(LOG_STATUS,"ToonThermostat: Worker stopped...");
}

void CToonThermostat::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CToonThermostat::SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=12;
	tsen.TEMP.id1=0;
	tsen.TEMP.id2=Idx;

	tsen.TEMP.tempsign=(Temp>=0)?0:1;
	int at10=round(abs(Temp*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
		result=m_sql.query(szQuery.str());
	}
}

void CToonThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	bool bDeviceExits=true;
	std::stringstream szQuery;

	char szID[10];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, 0, Idx);

	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tThermostat thermos;
	thermos.subtype=sTypeThermSetpoint;
	thermos.id1=0;
	thermos.id2=0;
	thermos.id3=0;
	thermos.id4=Idx;
	thermos.dunit=0;

	thermos.temp=Temp;

	sDecodeRXMessage(this, (const unsigned char *)&thermos);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
		result=m_sql.query(szQuery.str());
	}
}


std::string CToonThermostat::GetRandom()
{
	std::stringstream sstr;
	int rndval = rand();
	sstr << rndval;

	std::string ret;
	sstr >> ret;
	return ret;
}

bool CToonThermostat::GetSerialAndToken()
{
	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	if (!HTTPClient::POST("https://toonopafstand.eneco.nl/toonMobileBackendWeb/client/login", szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR,"ToonThermostat: Error login!");
		return false;
	}

#ifdef DEBUG_ToonThermostat
	SaveString2Disk(sResult, "E:\\toon-login.txt");
#endif

	if (sResult.find("BadLogin") != std::string::npos)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error login! (Check username/password)");
		return false;
	}

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}



	if (root["clientId"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_ClientID = root["clientId"].asString();
	if (root["clientIdChecksum"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	m_ClientIDChecksum = root["clientIdChecksum"].asString();

	std::string agreementId;
	std::string agreementIdChecksum;

	if (root["agreements"].empty() == true)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received, or invalid username/password!");
		return false;
	}
	agreementId = root["agreements"][0]["agreementId"].asString();
	agreementIdChecksum = root["agreements"][0]["agreementIdChecksum"].asString();

	std::stringstream sstr2;
	sstr2 << "clientId=" << m_ClientID 
		 << "&clientIdChecksum=" << m_ClientIDChecksum
		 << "&agreementId=" << agreementId
		 << "&agreementIdChecksum=" << agreementIdChecksum
		 << "&random=" << GetRandom();
	szPostdata = sstr2.str();
	sResult = "";
	if (!HTTPClient::POST("https://toonopafstand.eneco.nl/toonMobileBackendWeb/client/auth/start", szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error login!");
		return false;
	}
#ifdef DEBUG_ToonThermostat
	SaveString2Disk(sResult, "E:\\toon-start.txt");
#endif

	return true;
}

void CToonThermostat::Logout()
{
	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	std::stringstream sstr2;
	sstr2 << "clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();

	if (!HTTPClient::POST("https://toonopafstand.eneco.nl/toonMobileBackendWeb/client/auth/logout", szPostdata, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error login!");
	}
#ifdef DEBUG_ToonThermostat
	SaveString2Disk(sResult, "E:\\toon-logout.txt");
#endif

}

void CToonThermostat::GetMeterDetails()
{
	if (m_UserName.size()==0)
		return;
	if (m_Password.size()==0)
		return;
	if (!GetSerialAndToken())
		return;

	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	std::stringstream sstr2;
	sstr2 << "clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();
	//Get Data
	std::string sURL = "https://toonopafstand.eneco.nl/toonMobileBackendWeb/client/auth/retrieveToonState?" + szPostdata;
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error getting current state!");
		return;
	}
#ifdef DEBUG_ToonThermostat
	SaveString2Disk(sResult, "E:\\toon-retreivetoonstate.txt");
#endif

	Logout();

	time_t atime = mytime(NULL);

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Invalid data received!");
		return;
	}

	//thermostatInfo
	if (root["thermostatInfo"].empty() == false)
	{
		float currentTemp = root["thermostatInfo"]["currentTemp"].asFloat() / 100.0f;
		float currentSetpoint = root["thermostatInfo"]["currentSetpoint"].asFloat() / 100.0f;
		SendSetPointSensor(1, currentSetpoint, "Room Setpoint");
		SendTempSensor(1, currentTemp, "Room Temperature");
	}

	if (root["gasUsage"].empty() == false)
	{
		m_p1gas.gasusage = (unsigned long)(root["gasUsage"]["meterReading"].asFloat());

		if (
			(m_p1gas.gasusage != m_lastgasusage) ||
			(atime - m_lastSharedSendGas >= 300)
			)
		{
			//only update gas when there is a new value, or 5 minutes are passed
			m_lastSharedSendGas = atime;
			m_lastgasusage = m_p1gas.gasusage;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1gas);//decode message
		}
	}

	if (root["powerUsage"].empty() == false)
	{
		m_p1power.powerusage1 = (unsigned long)(root["powerUsage"]["meterReading"].asFloat());
		m_p1power.powerusage2 = (unsigned long)(root["powerUsage"]["meterReadingLow"].asFloat());
		m_p1power.usagecurrent = (unsigned long)(root["powerUsage"]["value"].asFloat());	//Watt
		sDecodeRXMessage(this, (const unsigned char *)&m_p1power);//decode message
	}
}

void CToonThermostat::SetSetpoint(const int idx, const float temp)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	if (idx==1)
	{
		//Room Set Point
		if (!GetSerialAndToken())
			return;

		char szTemp[20];
		sprintf(szTemp,"%d",int(temp*100.0f));
		std::string sTemp = szTemp;

		std::stringstream sstr2;
		sstr2 << "clientId=" << m_ClientID
			<< "&clientIdChecksum=" << m_ClientIDChecksum
			<< "&value=" << sTemp
			<< "&random=" << GetRandom();
		std::string szPostdata = sstr2.str();

		std::string sURL = "https://toonopafstand.eneco.nl/toonMobileBackendWeb/client/auth/setPoint?" + szPostdata;

		if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "ToonThermostat: Error setting setpoint!");
			return;
		}
		Logout();
#ifdef DEBUG_ToonThermostat
		SaveString2Disk(sResult, "E:\\toon-SetSetpoint.txt");
#endif

	}
}

void CToonThermostat::SetProgramState(const std::string &targetState)
{
	if (m_UserName.size() == 0)
		return;
	if (m_Password.size() == 0)
		return;

	std::string sResult;
	std::vector<std::string> ExtraHeaders;

	if (!GetSerialAndToken())
		return;

	std::stringstream sstr2;
	sstr2 << "clientId=" << m_ClientID
		<< "&clientIdChecksum=" << m_ClientIDChecksum
		<< "&state=2"
		<< "&temperatureState=" << targetState
		<< "&random=" << GetRandom();
	std::string szPostdata = sstr2.str();

	if (!HTTPClient::POST("https://toonopafstand.eneco.nl/toonMobileBackendWeb/client/auth/schemeState", szPostdata,ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "ToonThermostat: Error setting setpoint!");
		return;
	}
	Logout();
#ifdef DEBUG_ToonThermostat
	SaveString2Disk(sResult, "E:\\toon-schemestate.txt");
#endif
}