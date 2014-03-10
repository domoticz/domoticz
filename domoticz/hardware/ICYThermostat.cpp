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

	_log.Log(LOG_NORM,"ICYThermostat Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		time_t atime=mytime(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);
		if (((ltime.tm_min/ICY_POLL_INTERVAL!=LastMinute))&&(ltime.tm_sec>20))
		{
			LastMinute=ltime.tm_min/ICY_POLL_INTERVAL;
			GetMeterDetails();
		}
	}
	_log.Log(LOG_NORM,"ICYThermostat Worker stopped...");
}

void CICYThermostat::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CICYThermostat::SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	int Idx=(ID1 * 256) + ID2;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.ENERGY.packettype=pTypeENERGY;
	tsen.ENERGY.subtype=sTypeELEC2;
	tsen.ENERGY.id1=ID1;
	tsen.ENERGY.id2=ID2;
	tsen.ENERGY.count=1;
	tsen.ENERGY.rssi=12;

	tsen.ENERGY.battery_level=9;

	unsigned long long instant=(unsigned long long)(musage*1000.0);
	tsen.ENERGY.instant1=(unsigned char)(instant/0x1000000);
	instant-=tsen.ENERGY.instant1*0x1000000;
	tsen.ENERGY.instant2=(unsigned char)(instant/0x10000);
	instant-=tsen.ENERGY.instant2*0x10000;
	tsen.ENERGY.instant3=(unsigned char)(instant/0x100);
	instant-=tsen.ENERGY.instant3*0x100;
	tsen.ENERGY.instant4=(unsigned char)(instant);

	double total=(mtotal*1000.0)*223.666;
	tsen.ENERGY.total1=(unsigned char)(total/0x10000000000ULL);
	total-=tsen.ENERGY.total1*0x10000000000ULL;
	tsen.ENERGY.total2=(unsigned char)(total/0x100000000ULL);
	total-=tsen.ENERGY.total2*0x100000000ULL;
	tsen.ENERGY.total3=(unsigned char)(total/0x1000000);
	total-=tsen.ENERGY.total3*0x1000000;
	tsen.ENERGY.total4=(unsigned char)(total/0x10000);
	total-=tsen.ENERGY.total4*0x10000;
	tsen.ENERGY.total5=(unsigned char)(total/0x100);
	total-=tsen.ENERGY.total5*0x100;
	tsen.ENERGY.total6=(unsigned char)(total);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
		result=m_pMainWorker->m_sql.query(szQuery.str());
	}
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

void CICYThermostat::SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp,"%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype=sTypeVoltage;
	gDevice.id=1;
	gDevice.floatval1=Volt;
	gDevice.intval1=(int)Idx;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
		result=m_pMainWorker->m_sql.query(szQuery.str());

	}
}

void CICYThermostat::SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp,"%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype=sTypePercentage;
	gDevice.id=1;
	gDevice.floatval1=Percentage;
	gDevice.intval1=(int)Idx;
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
		result=m_pMainWorker->m_sql.query(szQuery.str());

	}
}

bool CICYThermostat::GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal)
{
	if (m_pMainWorker==NULL)
		return false;
	int Idx=(ID1 * 256) + ID2;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name, sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		return false;
	}
	std::vector<std::string> splitresult;
	StringSplit(result[0][1],";",splitresult);
	if (splitresult.size()!=2)
		return false;
	musage=atof(splitresult[0].c_str());
	mtotal=atof(splitresult[1].c_str())/1000.0;
	return true;
}

void CICYThermostat::GetMeterDetails()
{
	if (m_UserName.size()==0)
		return;
	if (m_Password.size()==0)
		return;

}

