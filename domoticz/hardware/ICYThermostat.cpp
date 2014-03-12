#include "stdafx.h"
#include "ICYThermostat.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../httpclient/HTTPClient.h"

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_ICYThermostat

CICYThermostat::CICYThermostat(const int ID, const std::string Username, const std::string Password)
{
	m_HwdID=ID;
	m_UserName=Username;
	m_Password=Password;
	m_stoprequested=false;
	Init();
}

CICYThermostat::~CICYThermostat(void)
{
}

void CICYThermostat::Init()
{
	m_SerialNumber="";
	m_Token="";
}

bool CICYThermostat::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CICYThermostat::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CICYThermostat::StopHardware()
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

#define ICY_POLL_INTERVAL 1

void CICYThermostat::Do_Work()
{
	int LastMinute=-1;

	_log.Log(LOG_NORM,"ICYThermostat: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		time_t atime=mytime(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);
		if (ltime.tm_min/ICY_POLL_INTERVAL!=LastMinute)
		{
			LastMinute=ltime.tm_min/ICY_POLL_INTERVAL;
			GetMeterDetails();
		}
	}
	_log.Log(LOG_NORM,"ICYThermostat: Worker stopped...");
}

void CICYThermostat::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CICYThermostat::SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeTEMP) << ") AND (Subtype==" << int(sTypeTEMP10) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
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
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
}

void CICYThermostat::SendSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;

	char szID[10];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, 0, Idx);

	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	result=m_pMainWorker->m_sql.query(szQuery.str());
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
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
}

bool CICYThermostat::GetSerialAndToken()
{
	std::stringstream sstr;
	sstr << "username=" << m_UserName << "&password=" << m_Password;
	std::string szPostdata=sstr.str();
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	if (!HTTPClient::POST("https://portal.icy.nl/login",szPostdata,ExtraHeaders,sResult))
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error login!");
		return false;
	}
	std::vector<std::string> splitresult;
	StringSplit(sResult,",",splitresult);
	if (splitresult.size()!=13)
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error login!");
		return false;
	}

	//Serial (9)
	std::vector<std::string> splitresult2;
	StringSplit(splitresult[9],":",splitresult2);
	if (splitresult2.size()!=2)
		return false;
	std::string szSerial=splitresult2[1].substr(1,splitresult2[1].size()-2);

	//Token (10)
	StringSplit(splitresult[10],":",splitresult2);
	if (splitresult2.size()!=2)
		return false;
	std::string szToken=splitresult2[1].substr(1,splitresult2[1].size()-2);

	if ((szSerial.size()==0)||(szToken.size()==0))
		return false;
	m_SerialNumber=szSerial;
	m_Token=szToken;
	return true;
}

void CICYThermostat::GetMeterDetails()
{
	if (m_UserName.size()==0)
		return;
	if (m_Password.size()==0)
		return;
	if (!GetSerialAndToken())
		return;
	//Get Data
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Session-token:"+m_Token);
	std::string sResult;

	if (!HTTPClient::GET("https://portal.icy.nl/data",ExtraHeaders,sResult))
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error getting data!");
		return;
	}
	std::vector<std::string> splitresult;
	StringSplit(sResult,",",splitresult);
	if (splitresult.size()<5)
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error getting data!");
		return;
	}

	std::string tmpString;
	std::string tmpLabel;
	float floatval;

	//Set Point (4)
	std::vector<std::string> splitresult2;
	StringSplit(splitresult[4],":",splitresult2);
	if (splitresult2.size()!=2)
	{
		_log.Log(LOG_ERROR,"ICYThermostat: Error getting data!");
		return;
	}
	tmpLabel=splitresult2[0].substr(1,splitresult2[0].size()-2);
	if (tmpLabel=="temperature1")
	{
		if (splitresult2[1].find('"')!=std::string::npos)
			tmpString=splitresult2[1].substr(1,splitresult2[1].size()-2);
		else
			tmpString=splitresult2[1];
		tmpString=stdreplace(tmpString,",",".");
		floatval=(float)atof(tmpString.c_str());
		SendSetPointSensor(1,floatval,"Room Setpoint");
	}

	//Room Temperature (5)
	StringSplit(splitresult[5],":",splitresult2);
	if (splitresult2.size()!=2)
		return;
	tmpLabel=splitresult2[0].substr(1,splitresult2[0].size()-2);
	if (tmpLabel=="temperature2")
	{
		if (splitresult2[1].find('"')!=std::string::npos)
			tmpString=splitresult2[1].substr(1,splitresult2[1].size()-2);
		else
			tmpString=splitresult2[1];
		tmpString=stdreplace(tmpString,",",".");
		floatval=(float)atof(tmpString.c_str());
		SendTempSensor(1,floatval,"Room Temperature");
	}
}

void CICYThermostat::SetSetpoint(const int idx, const float temp)
{
	if (idx==1)
	{
		//Room Set Point
		if (!GetSerialAndToken())
			return;

		char szTemp[20];
		sprintf(szTemp,"%.1f",temp);
		std::stringstream sstr;
		sstr << "uid=" << m_SerialNumber << "&temperature1=" << szTemp;
		std::string szPostdata=sstr.str();

		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back("Session-token:"+m_Token);
		std::string sResult;

		if (!HTTPClient::POST("https://portal.icy.nl/data",szPostdata,ExtraHeaders,sResult)) {
			_log.Log(LOG_ERROR,"ICYThermostat: Error setting SetPoint temperature!");
		}
		else {
			_log.Log(LOG_NORM,"ICYThermostat: Setting Room SetPoint to: %.1f",temp);
		}
	}
}
