#include "stdafx.h"
#include "SMASpot.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_SMASpot

CSMASpot::CSMASpot(const int ID, const std::string SMAConfigFile)
{
	m_HwdID=ID;
	m_SMAConfigFile=SMAConfigFile;
	m_SMADataPath="";
	m_stoprequested=false;
	Init();
}

CSMASpot::~CSMASpot(void)
{
}

void CSMASpot::Init()
{
	m_SMADataPath="";
	m_SMAPlantName="";
	m_SMADateFormat="";
	m_SMATimeFormat="";
	m_LastDateTime="";

	std::string tmpString;
	std::ifstream infile;
	std::string sLine;
	infile.open(m_SMAConfigFile.c_str());
	if (!infile.is_open())
	{
		_log.Log(LOG_ERROR,"SMASpot: Could not open configuration file!");
		return;
	}
	while (!infile.eof())
	{
		getline(infile, sLine);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		if (sLine.size()!=0)
		{
			if (sLine.find("OutputPath=")==0)
			{
				tmpString=sLine.substr(strlen("OutputPath="));
				if (tmpString!="")
				{
					unsigned char lastchar=tmpString[tmpString.size()-1];
#ifdef WIN32
					if (lastchar!='\\')
						tmpString+='\\';
#else
					if (lastchar!='/')
						tmpString+='/';
#endif
					m_SMADataPath=tmpString;
				}
			}
			else if (sLine.find("Plantname=")==0)
			{
				m_SMAPlantName=sLine.substr(strlen("Plantname="));
			}
			else if (sLine.find("DateFormat=")==0)
			{
				m_SMADateFormat=sLine.substr(strlen("DateFormat="));
			}
			else if (sLine.find("TimeFormat=")==0)
			{
				m_SMATimeFormat=sLine.substr(strlen("TimeFormat="));
			}
		}
	}
	infile.close();
	if ((m_SMADataPath.size()==0)||(m_SMADateFormat.size()==0)||(m_SMATimeFormat.size()==0))
	{
		_log.Log(LOG_ERROR,"SMASpot: Could not find OutputPath in configuration file!");
	}
}

bool CSMASpot::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSMASpot::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CSMASpot::StopHardware()
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

#define SMA_POLL_INTERVAL 1

void CSMASpot::Do_Work()
{
	int LastMinute=-1;

	_log.Log(LOG_NORM,"SMASpot Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		time_t atime=mytime(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);
		if (((ltime.tm_min/SMA_POLL_INTERVAL!=LastMinute))&&(ltime.tm_sec>20))
		{
			LastMinute=ltime.tm_min/SMA_POLL_INTERVAL;
			GetMeterDetails();
		}
	}
	_log.Log(LOG_NORM,"SMASpot Worker stopped...");
}

void CSMASpot::WriteToHardware(const char *pdata, const unsigned char length)
{

}

char *strftime_t (const char *format, const time_t rawtime)
{
	static char buffer[256];
	struct tm ltime;
	localtime_r(&rawtime,&ltime);
	strftime(buffer, sizeof(buffer), format, &ltime);
	return buffer;
}

void CSMASpot::SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname)
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

void CSMASpot::SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp,"%08X", Idx);

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

void CSMASpot::SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname)
{
	if (m_pMainWorker==NULL)
		return;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp,"%08X", Idx);

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

bool CSMASpot::GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal)
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

void CSMASpot::GetMeterDetails()
{
	if (m_SMADataPath.size()==0)
		return;
	if (m_SMAPlantName.size()==0)
		return;

	time_t atime=time(NULL);
	char szLogFile[256];
	char szDateStr[50];
	strcpy(szDateStr,strftime_t("%Y%m%d", atime));
	sprintf(szLogFile, "%s%s-Spot-%s.csv", strftime_t(m_SMADataPath.c_str(), atime), m_SMAPlantName.c_str(),szDateStr);

	std::string szSeperator=";";
	bool bHaveVersion=false;
	std::string tmpString;
	std::ifstream infile;
	std::string szLastLine="";
	std::vector<std::string> results;
	std::string sLine;
	infile.open(szLogFile);
	if (!infile.is_open())
	{
		return;
	}
	while (!infile.eof())
	{
		getline(infile, sLine);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		if (sLine.size()!=0)
		{
			if (sLine.find("sep=")==0)
			{
				tmpString=sLine.substr(strlen("sep="));
				if (tmpString!="")
				{
					szSeperator=tmpString;
				}
			}
			else if (sLine.find("Version CSV1")==0)
			{
				bHaveVersion=true;
			}
			else if (bHaveVersion)
			{
				StringSplit(sLine,szSeperator,results);
				if (results.size()==30)
				{
					szLastLine=sLine;
				}
			}
		}
	}
	infile.close();

	if (szLastLine.size()==0)
		return;
	if (results[1].size()<1)
		return;
	if (results[28]!="OK")
		return;
	std::string szDate=results[0];
	if (szDate==m_LastDateTime)
		return;
	m_LastDateTime=szDate;
/*
	std::string szDateTimeFormat=m_SMADateFormat + " " + m_SMATimeFormat;
	struct tm aitime;
	strptime(szDate.c_str(), szDateTimeFormat.c_str(), &aitime);
	time_t t = mktime(&aitime);
*/
	std::string szKwhCounter=results[23];
	szKwhCounter=stdreplace(szKwhCounter,",",".");
	double kWhCounter=atof(szKwhCounter.c_str());
	std::string szPacActual=results[20];
	szPacActual=stdreplace(szPacActual,",",".");
	double Pac=atof(szPacActual.c_str());
	if (kWhCounter!=0)
	{
		double LastUsage=0;
		double LastTotal=0;
		if (GetMeter(0,1,LastUsage,LastTotal))
		{
			if (kWhCounter<LastTotal)
				return;
		}
		SendMeter(0,1, Pac/1000.0, kWhCounter, "SolarMain");
	}

	float voltage;
	tmpString=results[16];
	tmpString=stdreplace(tmpString,",",".");
	voltage=(float)atof(tmpString.c_str());
	SendVoltage(1,voltage,"Volt uac1");
	tmpString=results[17];
	tmpString=stdreplace(tmpString,",",".");
	voltage=(float)atof(tmpString.c_str());
	if (voltage!=0) {
		SendVoltage(2,voltage,"Volt uac2");
	}
	tmpString=results[18];
	tmpString=stdreplace(tmpString,",",".");
	voltage=(float)atof(tmpString.c_str());
	if (voltage!=0) {
		SendVoltage(3,voltage,"Volt uac3");
	}

	float percentage;
	tmpString=results[21];
	tmpString=stdreplace(tmpString,",",".");
	percentage=(float)atof(tmpString.c_str());
	SendPercentage(1,percentage,"Efficiency");
	tmpString=results[24];
	tmpString=stdreplace(tmpString,",",".");
	percentage=(float)atof(tmpString.c_str());
	SendPercentage(2,percentage,"Hz");
	tmpString=results[27];
	tmpString=stdreplace(tmpString,",",".");
	percentage=(float)atof(tmpString.c_str());
	SendPercentage(3,percentage,"BT_Signal");

}

