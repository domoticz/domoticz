#include "stdafx.h"
#include "SBFSpot.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>
#include "hardwaretypes.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define round(a) ( int ) ( a + .5 )

CSBFSpot::CSBFSpot(const int ID, const std::string &SMAConfigFile)
{
	std::vector<std::string> results;

	m_HwdID=ID;
#ifdef WIN32
	StringSplit(SMAConfigFile, ";", results);
#else
	StringSplit(SMAConfigFile, ":", results);
#endif
	m_SBFConfigFile = results[0];

	if (results.size() > 1)
		m_SBFInverter = results[1];
	m_SBFDataPath="";
	Init();
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
		if (!sLine.empty())
		{
			if (sLine.find("OutputPath=")==0)
			{
				tmpString=sLine.substr(strlen("OutputPath="));
				if (!tmpString.empty())
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
	if ((m_SBFDataPath.empty()) || (m_SBFDateFormat.empty()) || (m_SBFTimeFormat.empty()))
	{
		_log.Log(LOG_ERROR,"SBFSpot: Could not find OutputPath in configuration file!");
	}
}

bool CSBFSpot::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CSBFSpot::StopHardware()
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

#define SMA_POLL_INTERVAL 1

void CSBFSpot::Do_Work()
{
	int LastMinute=-1;

	_log.Log(LOG_STATUS,"SBFSpot: Worker started...");
	while (!IsStopRequested(1000))
	{
		time_t atime = mytime(nullptr);
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

	sDecodeRXMessage(this, (const unsigned char *)&tsen.ENERGY, defaultname.c_str(), 255, nullptr);
}

bool CSBFSpot::GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal)
{
	int Idx=(ID1 * 256) + ID2;
	std::vector<std::vector<std::string> > result;
	result=m_sql.safe_query("SELECT Name, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID==%d) AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, int(Idx), int(pTypeENERGY), int(sTypeELEC2));
	if (result.empty())
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
	_log.Log(LOG_STATUS, "SBFSpot Import Old Month Data: Start");
	//check if this device exists in the database, if not exit
	bool bDeviceExits = true;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, "00000001", int(pTypeGeneral), int(sTypeKwh));
	if (result.empty())
	{
		//Lets create the sensor, and try again
		SendMeter(0, 1, 0, 0, "SolarMain");
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
			m_HwdID, "00000001", int(pTypeGeneral), int(sTypeKwh));
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "SBFSpot Import Old Month Data: FAILED - Cannot find sensor in database");
			return;
		}
	}
	uint64_t ulID = std::stoull(result[0][0]);

	//Try actual year, and previous year
	time_t atime = time(nullptr);
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
	_log.Log(LOG_STATUS, "SBFSpot Import Old Month Data: Complete");
}

void CSBFSpot::ImportOldMonthData(const uint64_t DevID, const int Year, const int Month)
{
	if (m_SBFDataPath.empty())
		return;
	if (m_SBFPlantName.empty())
		return;

	int iInvOff = 1;
	char szLogFile[256];
	std::string tmpPath = m_SBFDataPath;
	stdreplace(tmpPath, "%Y", std::to_string(Year));
	sprintf(szLogFile, "%s%s-%04d%02d.csv", tmpPath.c_str(), m_SBFPlantName.c_str(),Year, Month);

	std::ifstream infile;
	infile.open(szLogFile);
	if (!infile.is_open())
	{
		return;
	}

	std::string tmpString;
	std::string szSeperator;
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
		if (!sLine.empty())
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
				stdreplace(szKwhCounter, ",", ".");
				double kWhCounter = atof(szKwhCounter.c_str()) * 1000;
				unsigned long long ulCounter = (unsigned long long)kWhCounter;

				//check if this day record does not exists in the database, and insert it
				std::vector<std::vector<std::string> > result;

				char szDate[40];
				sprintf(szDate, "%04d-%02d-%02d", year, month, day);

				result = m_sql.safe_query("SELECT Value FROM Meter_Calendar WHERE (DeviceRowID==%" PRIu64 ") AND (Date=='%q')", DevID, szDate);
				if (result.empty())
				{
					//Insert value into our database
					m_sql.safe_query("INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) VALUES ('%" PRIu64 "', '%llu', '%q')", DevID, ulCounter, szDate);
					_log.Log(LOG_STATUS, "SBFSpot Import Old Month Data: Inserting %s",szDate);
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
				if (!tmpString.empty())
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
						if (results[iInvOff] == "kWh")
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

						std::string szKwhCounter = results[iInvOff + 1];
						stdreplace(szKwhCounter, ",", ".");
						double kWhCounter = atof(szKwhCounter.c_str()) * 1000;
						unsigned long long ulCounter = (unsigned long long)kWhCounter;

						//check if this day record does not exists in the database, and insert it
						std::vector<std::vector<std::string> > result;

						char szDate[40];
						sprintf(szDate, "%04d-%02d-%02d", year, month, day);

						result = m_sql.safe_query("SELECT Value FROM Meter_Calendar WHERE (DeviceRowID==%" PRIu64 ") AND (Date=='%q')",
							DevID, szDate);
						if (result.empty())
						{
							//Insert value into our database
							m_sql.safe_query("INSERT INTO Meter_Calendar (DeviceRowID, Value, Date) VALUES ('%" PRIu64 "', '%llu', '%q')",
								DevID, ulCounter, szDate);
							_log.Log(LOG_STATUS, "SBFSpot Import Old Month Data: Inserting %s", szDate);
						}
					}
				}
			}
			else if ((!szSeperator.empty()) && (!m_SBFInverter.empty()))
			{
				StringSplit(sLine, szSeperator, results);
				for (size_t l = 0; l < results.size(); l++)
				{
					if (results[l] == m_SBFInverter)
						iInvOff = l;
				}
			}
		}
	}
	infile.close();
}

int CSBFSpot::getSunRiseSunSetMinutes(const bool bGetSunRise)
{
	std::vector<std::string> strarray;
	std::vector<std::string> sunRisearray;
	std::vector<std::string> sunSetarray;

	if (!m_mainworker.m_LastSunriseSet.empty())
	{
		StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
		StringSplit(strarray[0], ":", sunRisearray);
		StringSplit(strarray[1], ":", sunSetarray);

		int sunRiseInMinutes = (atoi(sunRisearray[0].c_str()) * 60) + atoi(sunRisearray[1].c_str());
		int sunSetInMinutes = (atoi(sunSetarray[0].c_str()) * 60) + atoi(sunSetarray[1].c_str());

		if (bGetSunRise) {
			return sunRiseInMinutes;
		}
		return sunSetInMinutes;
	}
	return 0;
}

void CSBFSpot::GetMeterDetails()
{
	if (m_SBFDataPath.empty())
	{
		_log.Log(LOG_ERROR, "SBFSpot: Data path empty!");
		return;
	}
	if (m_SBFPlantName.empty())
	{
		_log.Log(LOG_ERROR, "SBFSpot: Plant name empty!");
		return;
	}

	time_t atime = time(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int ActHourMin = (ltime.tm_hour * 60) + ltime.tm_min;

	int sunRise = getSunRiseSunSetMinutes(true);
	int sunSet = getSunRiseSunSetMinutes(false);

	//We only poll one hour before sunrise till one hour after sunset
	if (ActHourMin + 120 < sunRise)
		return;
	if (ActHourMin - 120 > sunSet)
		return;

	char szLogFile[400];
	char szDateStr[50];
	strcpy(szDateStr, strftime_t("%Y%m%d", atime));
	sprintf(szLogFile, "%s%s-Spot-%s.csv", strftime_t(m_SBFDataPath.c_str(), atime), m_SBFPlantName.c_str(), szDateStr);

	std::string szSeperator = ";";
	bool bHaveVersion = false;
	std::string tmpString;
	std::ifstream infile;
	std::string szLastDate;
	std::vector<std::string> szLastLines;
	std::vector<std::string> results;
	std::string sLine;
	infile.open(szLogFile);
	if (!infile.is_open())
	{
		if ((ActHourMin > sunRise) && (ActHourMin < sunSet))
		{
			_log.Log(LOG_ERROR, "SBFSpot: Could not open spot file: %s", szLogFile);
		}
		return;
	}
	while (!infile.eof())
	{
		getline(infile, sLine);
		sLine.erase(std::remove(sLine.begin(), sLine.end(), '\r'), sLine.end());
		if (!sLine.empty())
		{
			if (sLine.find("sep=") == 0)
			{
				tmpString = sLine.substr(strlen("sep="));
				if (!tmpString.empty())
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
				if (
					(sLine.find(";DeviceName") == std::string::npos) &&
					(sLine.find(";Watt") == std::string::npos)
					)
				{
					StringSplit(sLine, szSeperator, results);
					if (results.size() >= 30)
					{
						if (m_SBFInverter.empty() || (m_SBFInverter == results[3]))
						{
							if (szLastDate.empty() || (szLastDate != results[0]))
							{
								szLastDate = results[0];
								szLastLines.clear();
							}
							if (szLastDate == results[0])
							{
								szLastLines.push_back(sLine);
							}
						}
					}
				}
			}
		}
	}
	infile.close();

	if (szLastLines.empty())
	{
		_log.Log(LOG_ERROR, "SBFSpot: No data record found in spot file!");
		return;
	}

	if (szLastDate == m_LastDateTime)
	{
		return;
	}
	m_LastDateTime = szLastDate;

	double kWhCounter = 0;
	double Pac = 0;
	int InvIdx = 0;

	for (const auto &line : szLastLines)
	{
		StringSplit(line, szSeperator, results);

		if (results[1].empty())
		{
			_log.Log(LOG_ERROR, "SBFSpot: No data record found in spot file!");
			return;
		}
		if ((results[28] != "OK") && (results[28] != "Ok"))
		{
			_log.Log(LOG_ERROR, "SBFSpot: Invalid field [28] should be OK!");
			return;
		}

		std::string szKwhCounter = results[23];
		stdreplace(szKwhCounter, ",", ".");
		kWhCounter += atof(szKwhCounter.c_str());
		std::string szPacActual = results[20];
		stdreplace(szPacActual, ",", ".");
		Pac += atof(szPacActual.c_str());

		float voltage;
		tmpString = results[16];
		stdreplace(tmpString, ",", ".");
		voltage = static_cast<float>(atof(tmpString.c_str()));
		SendVoltageSensor(0, (InvIdx * 10) + 1, 255, voltage, "Volt uac1");
		tmpString = results[17];
		stdreplace(tmpString, ",", ".");
		voltage = static_cast<float>(atof(tmpString.c_str()));
		if (voltage != 0) {
			SendVoltageSensor(0, (InvIdx * 10) + 2, 255, voltage, "Volt uac2");
		}
		tmpString = results[18];
		stdreplace(tmpString, ",", ".");
		voltage = static_cast<float>(atof(tmpString.c_str()));
		if (voltage != 0) {
			SendVoltageSensor(0, (InvIdx * 10) + 3, 255, voltage, "Volt uac3");
		}

		float percentage;
		tmpString = results[21];
		stdreplace(tmpString, ",", ".");
		percentage = static_cast<float>(atof(tmpString.c_str()));
		SendPercentageSensor((InvIdx * 10) + 1, 0, 255, percentage, "Efficiency");
		tmpString = results[24];
		stdreplace(tmpString, ",", ".");
		percentage = static_cast<float>(atof(tmpString.c_str()));
		SendPercentageSensor((InvIdx * 10) + 2, 0, 255, percentage, "Hz");
		tmpString = results[27];
		stdreplace(tmpString, ",", ".");
		percentage = static_cast<float>(atof(tmpString.c_str()));
		SendPercentageSensor((InvIdx * 10) + 3, 0, 255, percentage, "BT_Signal");

		if (results.size() >= 31)
		{
			tmpString = results[30];
			stdreplace(tmpString, ",", ".");
			float temperature = static_cast<float>(atof(tmpString.c_str()));
			SendTempSensor((InvIdx * 10) + 1, 255, temperature, "Temperature");
		}
		InvIdx++;
	}
	//Send combined counter/pac
	if (kWhCounter != 0)
	{
		double LastUsage = 0;
		double LastTotal = 0;
		if (GetMeter(0, 1, LastUsage, LastTotal))
		{
			if (kWhCounter < (int)(LastTotal * 100) / 100)
			{
				_log.Log(LOG_ERROR, "SBFSpot: Actual KwH counter (%f) less then last Counter (%f)!", kWhCounter, LastTotal);
				return;
			}
		}
		SendMeter(0, 1, Pac / 1000.0, kWhCounter, "SolarMain");
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::SBFSpotImportOldData(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				return;
			}
			int hardwareID = atoi(idx.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hardwareID);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType == HTYPE_SBFSpot)
				{
					CSBFSpot *pSBFSpot = reinterpret_cast<CSBFSpot *>(pHardware);
					pSBFSpot->ImportOldMonthData();
				}
			}
		}
	} // namespace server
} // namespace http
