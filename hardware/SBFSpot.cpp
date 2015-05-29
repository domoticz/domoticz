#include "stdafx.h"
#include "SBFSpot.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"

#define round(a) ( int ) ( a + .5 )

CSBFSpot::CSBFSpot(const int ID, const std::string SMAConfigFile)
{
	m_HwdID=ID;
	m_SBFConfigFile=SMAConfigFile;
	m_SBFDataPath="";
	m_stoprequested=false;
	Init();
}

CSBFSpot::~CSBFSpot(void)
{
}

void CSBFSpot::Init()
{
	m_SBFDataPath="";
	m_SBFPlantName="";
	m_SBFDateFormat="";
	m_SBFTimeFormat="";
	m_LastDateTime="";

	std::string tmpString;
	std::ifstream infile;
	std::string sLine;
	infile.open(m_SBFConfigFile.c_str());
	if (!infile.is_open())
	{
		_log.Log(LOG_ERROR,"SBFSpot: Could not open configuration file!");
		return;
	}
	while (!infile.eof())
	{
		getline(infile, sLine);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		sLine = stdstring_trim(sLine);
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
					m_SBFDataPath=tmpString;
				}
			}
			else if (sLine.find("Plantname=")==0)
			{
				m_SBFPlantName=sLine.substr(strlen("Plantname="));
			}
			else if (sLine.find("DateFormat=")==0)
			{
				m_SBFDateFormat=sLine.substr(strlen("DateFormat="));
			}
			else if (sLine.find("TimeFormat=")==0)
			{
				m_SBFTimeFormat=sLine.substr(strlen("TimeFormat="));
			}
		}
	}
	infile.close();
	if ((m_SBFDataPath.size()==0)||(m_SBFDateFormat.size()==0)||(m_SBFTimeFormat.size()==0))
	{
		_log.Log(LOG_ERROR,"SBFSpot: Could not find OutputPath in configuration file!");
	}
}

bool CSBFSpot::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSBFSpot::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CSBFSpot::StopHardware()
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

void CSBFSpot::Do_Work()
{
	int LastMinute=-1;

	_log.Log(LOG_STATUS,"SBFSpot: Worker started...");
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
		if (ltime.tm_sec % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
	}
	_log.Log(LOG_STATUS,"SBFSpot: Worker stopped...");
}

bool CSBFSpot::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

char *strftime_t (const char *format, const time_t rawtime)
{
	static char buffer[256];
	struct tm ltime;
	localtime_r(&rawtime,&ltime);
	strftime(buffer, sizeof(buffer), format, &ltime);
	return buffer;
}

void CSBFSpot::SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname)
{
	int Idx=(ID1 * 256) + ID2;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.ENERGY.packettype=pTypeENERGY;
	tsen.ENERGY.subtype=sTypeELEC2;
	tsen.ENERGY.packetlength=sizeof(tsen.ENERGY)-1;
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
		result=m_sql.query(szQuery.str());
	}
}

void CSBFSpot::SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname)
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

void CSBFSpot::SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname)
{
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp,"%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype=sTypeVoltage;
	gDevice.id=1;
	gDevice.floatval1=Volt;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
		result=m_sql.query(szQuery.str());
	}
}

void CSBFSpot::SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname)
{
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp,"%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
	{
		bDeviceExits=false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype=sTypePercentage;
	gDevice.id=1;
	gDevice.floatval1=Percentage;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypePercentage) << ")";
		result=m_sql.query(szQuery.str());

	}
}

bool CSBFSpot::GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal)
{
	int Idx=(ID1 * 256) + ID2;
	bool bDeviceExits=true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name, sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(Idx) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result=m_sql.query(szQuery.str());
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

void CSBFSpot::ImportOldMonthData()
{
	//check if this device exists in the database, if not exit
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(1) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		//Lets create the sensor, and try again
		SendMeter(0, 1, 0, 0, "SolarMain");
		szQuery.clear();
		szQuery.str("");
		szQuery << "SELECT ID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID==" << int(1) << ") AND (Type==" << int(pTypeENERGY) << ") AND (Subtype==" << int(sTypeELEC2) << ")";
		result = m_sql.query(szQuery.str());
		if (result.size() < 1)
		{
			return;
		}
	}
	unsigned long long ulID;
	std::stringstream s_str(result[0][0]);
	s_str >> ulID;

	//Try actual year, and previous year
	time_t atime = time(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int totyearsback = 2;

	int startYear = ltime.tm_year + 1900 - totyearsback;
	for (int iYear = startYear; iYear != startYear + 1 + totyearsback; iYear++)
	{
		for (int iMonth = 1; iMonth != 12+1; iMonth++)
		{
			ImportOldMonthData(ulID,iYear, iMonth);
		}
	}
}

void CSBFSpot::ImportOldMonthData(const unsigned long long DevID, const int Year, const int Month)
{
	if (m_SBFDataPath.size() == 0)
		return;
	if (m_SBFPlantName.size() == 0)
		return;

	char szLogFile[256];
	std::string tmpPath = m_SBFDataPath;
	std::stringstream sstr;
	sstr << Year;
	tmpPath=stdreplace(tmpPath, "%Y", sstr.str());
	sprintf(szLogFile, "%s%s-%04d%02d.csv", tmpPath.c_str(), m_SBFPlantName.c_str(),Year, Month);

	std::ifstream infile;
	infile.open(szLogFile);
	if (!infile.is_open())
	{
		return;
	}

	std::string tmpString;
	std::string szSeperator = "";
	std::vector<std::string> results;
	std::string sLine;
	bool bHaveVersion = false;
	bool bHaveDefinition = false;
	size_t dayPos = std::string::npos;
	size_t monthPos = std::string::npos;
	size_t yearPos = std::string::npos;

	bool bIsSMAWebExport = false;

	while (!infile.eof())
	{
		getline(infile, sLine);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		if (sLine.size() != 0)
		{
			if (bIsSMAWebExport)
			{
				int day = atoi(sLine.substr(dayPos, 2).c_str());
				int month = atoi(sLine.substr(monthPos, 2).c_str());
				int year = atoi(sLine.substr(yearPos, 4).c_str());

				std::string szKwhCounter = sLine.substr(18);
				size_t pPos = szKwhCounter.find('.');
				if (pPos==std::string::npos)
					continue;
				szKwhCounter = szKwhCounter.substr(0, pPos);
				pPos = szKwhCounter.find(',');
				if (pPos == std::string::npos)
					szKwhCounter = "0," + szKwhCounter;
				szKwhCounter = stdreplace(szKwhCounter, ",", ".");
				double kWhCounter = atof(szKwhCounter.c_str()) * 100000;
				unsigned long long ulCounter = (unsigned long long)kWhCounter;

				//check if this day record does not exists in the database, and insert it
				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;

				char szDate[40];
				sprintf(szDate, "%04d-%02d-%02d", year, month, day);

				szQuery << "SELECT Value FROM Meter_Calendar WHERE (DeviceRowID==" << DevID << ") AND (Date=='" << szDate << "')";
				result = m_sql.query(szQuery.str());
				if (result.size() == 0)
				{
					//Insert value into our database
					szQuery.clear();
					szQuery.str("");
					szQuery << "INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) VALUES ('" << DevID << "', '" << ulCounter << "', '" << szDate << "')";
					result = m_sql.query(szQuery.str());
				}

			}
			else if (sLine.find("Time,Energy (Wh)") == 0)
			{
				dayPos = 0;
				monthPos = 3;
				yearPos = 6;
				bIsSMAWebExport = true;
				continue;
			}
			else if (sLine.find("sep=") == 0)
			{
				tmpString = sLine.substr(strlen("sep="));
				if (tmpString != "")
				{
					szSeperator = tmpString;
				}
			}
			else if (sLine.find("Version CSV1") == 0)
			{
				bHaveVersion = true;
			}
			else if (bHaveVersion)
			{
				StringSplit(sLine, szSeperator, results);
				if (results.size() == 3)
				{
					if (!bHaveDefinition)
					{
						if (results[1] == "kWh")
						{
							dayPos = results[0].find("dd");
							if (dayPos == std::string::npos)
							{
								infile.close();
								return;
							}
							monthPos = results[0].find("MM");
							yearPos = results[0].find("yyyy");
							bHaveDefinition = true;
						}
					}
					else
					{
						int day = atoi(results[0].substr(dayPos, 2).c_str());
						int month = atoi(results[0].substr(monthPos, 2).c_str());
						int year = atoi(results[0].substr(yearPos, 4).c_str());

						std::string szKwhCounter = results[2];
						szKwhCounter = stdreplace(szKwhCounter, ",", ".");
						double kWhCounter = atof(szKwhCounter.c_str()) * 100000;
						unsigned long long ulCounter = (unsigned long long)kWhCounter;

						//check if this day record does not exists in the database, and insert it
						std::stringstream szQuery;
						std::vector<std::vector<std::string> > result;

						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", year, month, day);

						szQuery << "SELECT Value FROM Meter_Calendar WHERE (DeviceRowID==" << DevID << ") AND (Date=='" << szDate << "')";
						result = m_sql.query(szQuery.str());
						if (result.size() == 0)
						{
							//Insert value into our database
							szQuery.clear();
							szQuery.str("");
							szQuery << "INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) VALUES ('" << DevID << "', '" << ulCounter << "', '" << szDate << "')";
							result = m_sql.query(szQuery.str());
						}
					}
				}
			}
		}
	}
	infile.close();
}

void CSBFSpot::GetMeterDetails()
{
	if (m_SBFDataPath.size() == 0)
	{
		_log.Log(LOG_ERROR, "SBFSpot: Data path empty!");
		return;
	}
	if (m_SBFPlantName.size() == 0)
	{
		_log.Log(LOG_ERROR, "SBFSpot: Plant name empty!");
		return;
	}

	time_t atime=time(NULL);
	char szLogFile[256];
	char szDateStr[50];
	strcpy(szDateStr,strftime_t("%Y%m%d", atime));
	sprintf(szLogFile, "%s%s-Spot-%s.csv", strftime_t(m_SBFDataPath.c_str(), atime), m_SBFPlantName.c_str(),szDateStr);

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
		_log.Log(LOG_ERROR, "SBFSpot: Could not open spot file: %s", szLogFile);
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
				if (results.size()>=30)
				{
					szLastLine=sLine;
				}
			}
		}
	}
	infile.close();

	if (szLastLine.size() == 0)
	{
		_log.Log(LOG_ERROR, "SBFSpot: No data record found in spot file!");
		return;
	}
	if (results[1].size() < 1)
	{
		_log.Log(LOG_ERROR, "SBFSpot: No data record found in spot file!");
		return;
	}
	if ((results[28] != "OK") && (results[28] != "Ok"))
	{
		_log.Log(LOG_ERROR, "SBFSpot: Invalid field [28] should be OK!");
		return;
	}
	std::string szDate=results[0];
	if (szDate == m_LastDateTime)
	{
		return;
	}
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
			if (kWhCounter < (int)(LastTotal*100)/100)
			{
				_log.Log(LOG_ERROR, "SBFSpot: Actual KwH counter (%f) less then last Counter (%f)!",kWhCounter,LastTotal);
				return;
			}
		}
		SendMeter(0,1, Pac/1000.0, kWhCounter, "SolarMain");
	}

	float voltage;
	tmpString=results[16];
	tmpString=stdreplace(tmpString,",",".");
	voltage = static_cast<float>(atof(tmpString.c_str()));
	SendVoltage(1,voltage,"Volt uac1");
	tmpString=results[17];
	tmpString=stdreplace(tmpString,",",".");
	voltage = static_cast<float>(atof(tmpString.c_str()));
	if (voltage!=0) {
		SendVoltage(2,voltage,"Volt uac2");
	}
	tmpString=results[18];
	tmpString=stdreplace(tmpString,",",".");
	voltage = static_cast<float>(atof(tmpString.c_str()));
	if (voltage!=0) {
		SendVoltage(3,voltage,"Volt uac3");
	}

	float percentage;
	tmpString=results[21];
	tmpString=stdreplace(tmpString,",",".");
	percentage = static_cast<float>(atof(tmpString.c_str()));
	SendPercentage(1,percentage,"Efficiency");
	tmpString=results[24];
	tmpString=stdreplace(tmpString,",",".");
	percentage = static_cast<float>(atof(tmpString.c_str()));
	SendPercentage(2,percentage,"Hz");
	tmpString=results[27];
	tmpString=stdreplace(tmpString,",",".");
	percentage = static_cast<float>(atof(tmpString.c_str()));
	SendPercentage(3,percentage,"BT_Signal");

	if (results.size()>=31)
	{
		tmpString=results[30];
		tmpString=stdreplace(tmpString,",",".");
		float temperature = static_cast<float>(atof(tmpString.c_str()));
		SendTempSensor(1,temperature,"Temperature");
	}
}

