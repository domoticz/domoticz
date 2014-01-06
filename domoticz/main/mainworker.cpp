#include "stdafx.h"
#include "mainworker.h"
#include "Helper.h"
#include "SunRiseSet.h"
#include "localtime_r.h"
#include "Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../webserver/Base64.h"

//Hardware Devices
#include "../hardware/hardwaretypes.h"
#include "../hardware/RFXComSerial.h"
#include "../hardware/RFXComTCP.h"
#include "../hardware/DomoticzTCP.h"
#include "../hardware/P1MeterBase.h"
#include "../hardware/P1MeterSerial.h"
#include "../hardware/P1MeterTCP.h"
#include "../hardware/YouLess.h"
#ifdef WITH_LIBUSB
	#include "../hardware/TE923.h"
#include "../hardware/VolcraftCO20.h"
#endif
#include "../hardware/Rego6XXSerial.h"
#include "../hardware/Razberry.h"
#ifdef WITH_OPENZWAVE
	#include "../hardware/OpenZWave.h"
#endif
#include "../hardware/DavisLoggerSerial.h"
#include "../hardware/1Wire.h"
#include "../hardware/BMP085.h"
#include "../hardware/Wunderground.h"
#include "../hardware/Dummy.h"
#include "../hardware/PiFace.h"
#include "../hardware/S0MeterSerial.h"
#include "../hardware/OTGWSerial.h"
#include "../hardware/OTGWTCP.h"
//#include "../hardware/S0MeterTCP.h"
#include "../hardware/Teleinfo.h"
#include "../hardware/Limitless.h"
#include "../hardware/EnOcean.h"
#ifdef WIN32
#include "../hardware/OpenHardwareMonitor.h"
#endif

#ifdef WIN32
    #include "dirent_windows.h"
#else
    #include <dirent.h>
#endif

#include "mainstructs.h"

#ifdef _DEBUG
	//#define DEBUG_RECEIVE
#endif

//#define PARSE_RFXCOM_DEVICE_LOG

#ifdef PARSE_RFXCOM_DEVICE_LOG
	#include <iostream>
	#include <fstream>
#endif

#define round(a) ( int ) ( a + .5 )

extern std::string szStartupFolder;
extern std::string szWWWFolder;
extern bool bIsRaspberryPi;

MainWorker::MainWorker()
{
	m_SecCountdown=-1;
	m_stoprequested=false;
	m_verboselevel=EVBL_None;
	m_bStartHardware=false;
	m_hardwareStartCounter=0;
	m_webserverport="8080";
	m_bIgnoreUsernamePassword=false;

	time_t atime=mytime(NULL);
	struct tm ltime;
	localtime_r(&atime,&ltime);
	m_ScheduleLastMinute=ltime.tm_min;
	m_ScheduleLastHour=ltime.tm_hour;
	m_bHaveDownloadedDomoticzUpdate=false;
	m_bHaveDownloadedDomoticzUpdateSuccessFull=false;
	m_bDoDownloadDomoticzUpdate=false;
	m_sql.SetMainWorker(this);
}

MainWorker::~MainWorker()
{
	Stop();
	ClearDomoticzHardware();
}

void MainWorker::StartDomoticzHardware()
{
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt=m_hardwaredevices.begin(); itt!=m_hardwaredevices.end(); ++itt)
	{
		if (!(*itt)->IsStarted())
		{
			(*itt)->Start();
		}
	}
}

void MainWorker::StopDomoticzHardware()
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt=m_hardwaredevices.begin(); itt!=m_hardwaredevices.end(); ++itt)
	{
		(*itt)->Stop();
	}
}

void MainWorker::SendResetCommand(CDomoticzHardwareBase *pHardware)
{
	pHardware->m_bEnableReceive=false;

	if (
		(pHardware->HwdType==HTYPE_RFXtrx315)||
		(pHardware->HwdType==HTYPE_RFXtrx433)||
		(pHardware->HwdType==HTYPE_RFXLAN)
		)
	{
		pHardware->m_rxbufferpos=0;
		//Send Reset
		SendCommand(pHardware->m_HwdID,cmdRESET,"Reset");

		//wait at least 50ms
		boost::this_thread::sleep(boost::posix_time::millisec(500));
		pHardware->m_rxbufferpos=0;
	}
	//clear buffer, and enable receive
	pHardware->m_rxbufferpos=0;
	pHardware->m_bEnableReceive=true;

	if (
		(pHardware->HwdType==HTYPE_RFXtrx315)||
		(pHardware->HwdType==HTYPE_RFXtrx433)||
		(pHardware->HwdType==HTYPE_RFXLAN)
		)
	{
		SendCommand(pHardware->m_HwdID,cmdSTATUS,"Status");
	}
}

void MainWorker::AddDomoticzHardware(CDomoticzHardwareBase *pHardware)
{
	int devidx=FindDomoticzHardware(pHardware->m_HwdID);
	if (devidx!=-1) //it is already there!, remove it
	{
		RemoveDomoticzHardware(m_hardwaredevices[devidx]);
	}
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	pHardware->sDecodeRXMessage.connect( boost::bind( &MainWorker::DecodeRXMessage, this, _1, _2 ) );
	pHardware->sOnConnected.connect( boost::bind( &MainWorker::OnHardwareConnected, this, _1 ) );
	m_hardwaredevices.push_back(pHardware);
}

void MainWorker::RemoveDomoticzHardware(CDomoticzHardwareBase *pHardware)
{
	boost::lock_guard<boost::mutex> l1(decodeRXMessageMutex);
	boost::lock_guard<boost::mutex> l2(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt=m_hardwaredevices.begin(); itt!=m_hardwaredevices.end(); ++itt)
	{
		CDomoticzHardwareBase *pOrgDevice=*itt;
		if (pOrgDevice==pHardware) {
			try
			{
				m_hardwaredevices.erase(itt);
				pOrgDevice->Stop();
				delete pOrgDevice;
			}
			catch (...)
			{
			}
			return;
		}
	}
}

void MainWorker::RemoveDomoticzHardware(int HwdId)
{
	int dpos=FindDomoticzHardware(HwdId);
	if (dpos==-1)
		return;
	RemoveDomoticzHardware(m_hardwaredevices[dpos]);
}

int MainWorker::FindDomoticzHardware(int HwdId)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt=m_hardwaredevices.begin(); itt!=m_hardwaredevices.end(); ++itt)
	{
		if ((*itt)->m_HwdID==HwdId)
		{
			return (itt-m_hardwaredevices.begin());
		}
	}
	return -1;
}

CDomoticzHardwareBase* MainWorker::GetHardware(int HwdId)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt=m_hardwaredevices.begin(); itt!=m_hardwaredevices.end(); ++itt)
	{
		if ((*itt)->m_HwdID==HwdId)
		{
			return (*itt);
		}
	}
	return NULL;
}

void MainWorker::ClearDomoticzHardware()
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt=m_hardwaredevices.begin(); itt!=m_hardwaredevices.end(); ++itt)
	{
		CDomoticzHardwareBase* pOrgDevice=(*itt);
//		delete pOrgDevice;
	}
	m_hardwaredevices.clear();
}

// sunset/sunrise
// http://www.earthtools.org/sun/<latitude>/<longitude>/<day>/<month>/<timezone>/<dst>
// example:
// http://www.earthtools.org/sun/52.214268/5.171002/11/11/99/1

bool MainWorker::GetSunSettings()
{
	int nValue;
	std::string sValue;
	if (!m_sql.GetPreferencesVar("Location",nValue,sValue))
		return false;
	std::vector<std::string> strarray;
	StringSplit(sValue, ";", strarray);

	if (strarray.size()!=2)
		return false;

	std::string Latitude=strarray[0];
	std::string Longitude=strarray[1];

	unsigned char *pData=NULL;
	unsigned long ulLength=0;
	time_t atime=mytime(NULL);
	struct tm ltime;
	localtime_r(&atime,&ltime);

	int year=ltime.tm_year+1900;
	int month=ltime.tm_mon+1;
	int day=ltime.tm_mday;

	double dLatitude=atof(Latitude.c_str());
	double dLongitude=atof(Longitude.c_str());

	SunRiseSet::_tSubRiseSetResults sresult;
	SunRiseSet::GetSunRiseSet(dLatitude,dLongitude,year,month,day,sresult);

	std::string sunrise;
	std::string sunset;

	char szRiseSet[30];
	sprintf(szRiseSet,"%02d:%02d:00",sresult.SunRiseHour,sresult.SunRiseMin);
	sunrise=szRiseSet;
	sprintf(szRiseSet,"%02d:%02d:00",sresult.SunSetHour,sresult.SunSetMin);
	sunset=szRiseSet;
	std::string riseset=sunrise+";"+sunset;
	m_sql.UpdateTempVar("SunRiseSet",riseset.c_str());
	m_scheduler.SetSunRiseSetTimers(sunrise,sunset);
	_log.Log(LOG_NORM,"Sunrise: %s SunSet:%s", sunrise.c_str(), sunset.c_str());
	return true;
}

void MainWorker::SetVerboseLevel(eVerboseLevel Level)
{
	m_verboselevel=Level;
}

void MainWorker::SetWebserverPort(const std::string &Port)
{
	m_webserverport=Port;
}

std::string MainWorker::GetWebserverPort()
{
	return m_webserverport;
}

bool MainWorker::RestartHardware(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware WHERE (ID==" << idx << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false;
	std::vector<std::string> sd=result[0];
	std::string Name=sd[0];
	std::string senabled=(sd[1]=="1")?"true":"false";
	_eHardwareTypes htype=(_eHardwareTypes)atoi(sd[2].c_str());
	std::string address=sd[3];
	int port=atoi(sd[4].c_str());
	std::string username=sd[5];
	std::string password=sd[6];
	int Mode1=atoi(sd[7].c_str());
	int Mode2=atoi(sd[8].c_str());
	int Mode3=atoi(sd[9].c_str());
	int Mode4=atoi(sd[10].c_str());
	int Mode5=atoi(sd[11].c_str());

	return AddHardwareFromParams(atoi(idx.c_str()),Name,(senabled=="true")?true:false,htype,address,port,username,password,Mode1,Mode2,Mode3,Mode4,0);
}

bool MainWorker::AddHardwareFromParams(
	int ID,
	std::string Name,
	bool Enabled,
	_eHardwareTypes Type,
	std::string Address, unsigned short Port, 
	std::string Username, std::string Password, 
	int Mode1,
	int Mode2, 
	int Mode3,
	int Mode4,
	int Mode5)
{
	RemoveDomoticzHardware(ID);

	if (!Enabled)
		return true;

	char szSerialPort[100];
	CDomoticzHardwareBase *pHardware=NULL;

	switch (Type)
	{
	case HTYPE_RFXtrx315:
	case HTYPE_RFXtrx433:
	case HTYPE_P1SmartMeter:
	case HTYPE_Rego6XX:
	case HTYPE_DavisVantage:
	case HTYPE_S0SmartMeter:
	case HTYPE_OpenThermGateway:
	case HTYPE_TeleinfoMeter:
	case HTYPE_OpenZWave:
	case HTYPE_EnOcean:
		{
			//USB/Serial
#if defined WIN32
			sprintf(szSerialPort,"COM%d",Port);
#else
			bool bUseDirectPath=false;
			std::vector<std::string> serialports=GetSerialPorts(bUseDirectPath);
			if (bUseDirectPath)
			{
				if (Port>=serialports.size())
				{
					_log.Log(LOG_ERROR,"Serial Port out of range!...");
					return false;
				}
				strcpy(szSerialPort,serialports[Port].c_str());
			}
			else
			{
				sprintf(szSerialPort,"/dev/ttyUSB%d",Port);
			}
#endif
			if (
				(Type==HTYPE_RFXtrx315)||
				(Type==HTYPE_RFXtrx433)
				)
			{
				pHardware = new RFXComSerial(ID,szSerialPort,38400);
			}
			else if (Type==HTYPE_P1SmartMeter)
			{
				pHardware = new P1MeterSerial(ID,szSerialPort,9600);
			}
			else if (Type==HTYPE_Rego6XX)
			{
				pHardware = new CRego6XXSerial(ID,szSerialPort, Mode1);
			}
			else if (Type==HTYPE_DavisVantage)
			{
				pHardware = new CDavisLoggerSerial(ID,szSerialPort, 19200);
			}
			else if (Type==HTYPE_S0SmartMeter)
			{
				pHardware = new S0MeterSerial(ID,szSerialPort, 9600, Mode1, Mode2, Mode3, Mode4);
			}
			else if (Type==HTYPE_OpenThermGateway)
			{
				pHardware = new OTGWSerial(ID,szSerialPort, 9600);
				//pHardware = new S0MeterTCP(ID,"127.0.0.1",2001, Mode1, Mode2, Mode3, Mode4);
			}
			else if (Type==HTYPE_TeleinfoMeter)
			{
				pHardware = new Teleinfo(ID, szSerialPort);
			}
			else if (Type==HTYPE_OpenZWave)
			{
#ifdef WITH_OPENZWAVE
				pHardware = new COpenZWave(ID, szSerialPort);
#endif
			}
			else if (Type==HTYPE_EnOcean)
			{
				pHardware = new CEnOcean(ID,szSerialPort, Mode1);
			}
		}
		break;
	case HTYPE_RFXLAN:
		//LAN
		pHardware = new RFXComTCP(ID, Address, Port);
		break;
	case HTYPE_Domoticz:
		//LAN
		pHardware = new DomoticzTCP(ID, Address, Port, Username, Password);
		break;
	case HTYPE_RazberryZWave:
		//HTTP
		pHardware = new CRazberry(ID, Address, Port, Username, Password);
		break;
	case HTYPE_P1SmartMeterLAN:
		//LAN
		pHardware = new P1MeterTCP(ID, Address, Port);
		break;
	case HTYPE_OpenThermGatewayTCP:
		//LAN
		pHardware = new OTGWTCP(ID, Address, Port);
		break;
	case HTYPE_LimitlessLights:
		//LAN
		{
			if (Mode1==0)
				Mode1=1;
			pHardware = new CLimitLess(ID, Mode1,Address, Port);
		}
		break;
	case HTYPE_YouLess:
		//LAN
		pHardware = new CYouLess(ID, Address, Port, Password);
		break;
	case HTYPE_1WIRE:
		//1-Wire file system
		pHardware = new C1Wire(ID);
		break;
#ifndef WIN32
	case HTYPE_TE923:
		//TE923 compatible weather station
#ifdef WITH_LIBUSB
		pHardware = new CTE923(ID);
#endif
		break;
	case HTYPE_VOLCRAFTCO20:
		//Voltcrafr CO-20 Air Quality
#ifdef WITH_LIBUSB
		pHardware = new CVolcraftCO20(ID);
#endif
		break;
#endif
	case HTYPE_RaspberryBMP085:
		pHardware = new CBMP085(ID);
		break;
	case HTYPE_Wunderground:
		pHardware = new CWunderground(ID,Username,Password);
		break;
	case HTYPE_Dummy:
		pHardware = new CDummy(ID);
		break;
	case HTYPE_PiFace:
		pHardware = new CPiFace(ID);
		break;
	}
	if (pHardware)
	{
		pHardware->HwdType=Type;
		pHardware->Name=Name;
		pHardware->m_pMainWorker=this;
		AddDomoticzHardware(pHardware);
		m_hardwareStartCounter=0;
		m_bStartHardware=true;
		return true;
	}
	return false;
}

bool MainWorker::Start()
{
	Stop();

	if (!m_sql.OpenDatabase())
	{
		return false;
	}
	//Add Hardware devices
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT ID, Name, Enabled, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware ORDER BY ID ASC";
	result=m_sql.query(szQuery.str());
	//std::string revfile;
	//HTTPClient::GET("http://www.domoticz.com/pwiki/piwik.php?idsite=1&amp;rec=1&amp;action_name=Started&amp;idgoal=3",revfile);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			int ID=atoi(sd[0].c_str());
			std::string Name=sd[1];
			std::string sEnabled=sd[2];
			bool Enabled=(sEnabled=="1")?true:false;
			_eHardwareTypes Type=(_eHardwareTypes)atoi(sd[3].c_str());
			std::string Address=sd[4];
			unsigned short Port=atoi(sd[5].c_str());
			std::string Username=sd[6];
			std::string Password=sd[7];
			int mode1=atoi(sd[8].c_str());
			int mode2=atoi(sd[9].c_str());
			int mode3=atoi(sd[10].c_str());
			int mode4=atoi(sd[11].c_str());
			int mode5=atoi(sd[12].c_str());
			AddHardwareFromParams(ID,Name,Enabled,Type,Address,Port,Username,Password,mode1,mode2,mode3,mode4,mode5);
		}
	}
	if (!StartThread())
		return false;
	GetSunSettings();
	return true;
}


bool MainWorker::Stop()
{
	m_scheduler.StopScheduler();
	m_eventsystem.StopEventSystem();
#ifdef WIN32
	m_openhardwaremonitor.StopOpenHardwareMonitor();
#endif
//    m_cameras.StopCameraGrabber();
	StopDomoticzHardware();
	m_webserver.StopServer();

	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	return true;
}

bool MainWorker::StartThread()
{
	//Start WebServer
	if (!m_webserver.StartServer(this, "0.0.0.0",m_webserverport,szWWWFolder,m_bIgnoreUsernamePassword))
	{
        return false;
	}
	_log.Log(LOG_NORM,"Webserver started on port: %s", m_webserverport.c_str());
	int nValue=0;
	if (m_sql.GetPreferencesVar("AuthenticationMethod", nValue))
	{
		m_webserver.SetAuthenticationMethod(nValue);
	}


	m_cameras.SetMainWorker(this);

	//Start Scheduler
	m_scheduler.StartScheduler(this);
#ifdef WIN32
	m_openhardwaremonitor.StartOpenHardwareMonitor(this);
#endif
	m_eventsystem.StartEventSystem(this);
	//m_sql.DeleteHardware("999");

#ifdef PARSE_RFXCOM_DEVICE_LOG
	std::vector<std::string> _lines;
	std::ifstream myfile ("C:\\RFXtrxLog.txt");
	if (myfile.is_open())
	{
		while ( myfile.good() )
		{
			std::string _line;
			getline (myfile,_line);
			_lines.push_back(_line);
		}
		myfile.close();
	}
	DomoticzTCP tempHardware(999,"0.0.0.0",1234,"","");

	std::vector<std::string>::iterator itt;
	unsigned char rxbuffer[100];
	static const char* const lut = "0123456789ABCDEF";
	for (itt=_lines.begin(); itt!=_lines.end(); ++itt)
	{
		std::string hexstring=*itt;
		if (hexstring.size()%2!=0)
			continue;//illegal
		int totbytes=hexstring.size()/2;
		int ii=0;
		for (ii=0; ii<totbytes; ii++)
		{
			std::string hbyte=hexstring.substr((ii*2),2);

			char a = hbyte[0];
			const char* p = std::lower_bound(lut, lut + 16, a);
			if (*p != a) throw std::invalid_argument("not a hex digit");

			char b = hbyte[1];
			const char* q = std::lower_bound(lut, lut + 16, b);
			if (*q != b) throw std::invalid_argument("not a hex digit");

			unsigned char uchar=((p - lut) << 4) | (q - lut);
			rxbuffer[ii]=uchar;
		}
		if (ii==0)
			continue;
		DecodeRXMessage(&tempHardware, (const unsigned char *)&rxbuffer);
	}
	return false;
#endif

	int rnvalue=0;
	m_sql.GetPreferencesVar("RemoteSharedPort", rnvalue);
	if (rnvalue!=0)
	{
		char szPort[100];
		sprintf(szPort,"%d",rnvalue);
		m_sharedserver.sDecodeRXMessage.connect( boost::bind( &MainWorker::DecodeRXMessage, this, _1, _2 ) );
		m_sharedserver.StartServer("0.0.0.0",szPort);

		LoadSharedUsers();
	}

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MainWorker::Do_Work, this)));

	return (m_thread!=NULL);
}

#define HEX( x ) \
	std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)( x )

void MainWorker::GetDomoticzUpdate(const std::string &UpdateURL)
{
	m_szDomoticzUpdateURL=UpdateURL;
	m_bHaveDownloadedDomoticzUpdate=false;
	m_bHaveDownloadedDomoticzUpdateSuccessFull=false;
	m_bDoDownloadDomoticzUpdate=true;
}

void MainWorker::GetRaspberryPiTemperature()
{
	std::string sLine = "";
	std::ifstream infile;

	infile.open("/sys/class/thermal/thermal_zone0/temp");
	if (infile.is_open())
	{
		if (!infile.eof())
		{
			getline(infile, sLine);
			float temperature=(float)atof(sLine.c_str())/1000.0f;
			if ((temperature != 85) && (temperature > -273))
			{
				//Temp
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
				tsen.TEMP.packettype=pTypeTEMP;
				tsen.TEMP.subtype=sTypeTEMP_SYSTEM;
				tsen.TEMP.battery_level=9;
				tsen.TEMP.rssi=6;
				tsen.TEMP.id1=0;
				tsen.TEMP.id2=1;

				tsen.TEMP.tempsign=(temperature>=0)?0:1;
				int at10=round(abs(temperature*10.0f));
				tsen.TEMP.temperatureh=(BYTE)(at10/256);
				at10-=(tsen.TEMP.temperatureh*256);
				tsen.TEMP.temperaturel=(BYTE)(at10);

				// convert now to string form
				time_t now = time(0);
				char *szDate = asctime(localtime(&now));
				szDate[strlen(szDate)-1]=0;

				WriteMessageStart();

				std::stringstream sTmp;
				sTmp << szDate << " (System) ";
				WriteMessage(sTmp.str().c_str(),false);
				WriteMessage("Temperature",false);
				unsigned long long DeviceRowIdx=decode_Temp(1000, (const tRBUF*)&tsen.TEMP);
				WriteMessageEnd();

				m_sharedserver.SendToAll(DeviceRowIdx,(const char*)&tsen,tsen.TEMP.packetlength+1,NULL);
			}
		}
		infile.close();
	}
}

void MainWorker::HandleAutomaticBackups()
{
	int nValue=0;
	if (!m_sql.GetPreferencesVar("UseAutoBackup",nValue))
		return;
	if (nValue==1)
	{
		std::stringstream backup_DirH;
		std::stringstream backup_DirD;
		std::stringstream backup_DirM;

		#ifdef WIN32
			backup_DirH << szStartupFolder << "backups\\hourly\\";
			backup_DirD << szStartupFolder << "backups\\daily\\";
			backup_DirM << szStartupFolder << "backups\\monthly\\";
		#else
			backup_DirH << szStartupFolder << "backups/hourly/";
			backup_DirD << szStartupFolder << "backups/daily/";
			backup_DirM << szStartupFolder << "backups/monthly/";
		#endif
    
		std::string sbackup_DirH = backup_DirH.str();
		std::string sbackup_DirD = backup_DirD.str();
		std::string sbackup_DirM = backup_DirM.str();

		//create folders if they not exists
		mkdir_deep(sbackup_DirH.c_str(),777);
		mkdir_deep(sbackup_DirD.c_str(),777);
		mkdir_deep(sbackup_DirM.c_str(),777);
    	
		time_t now = mytime(NULL);
		struct tm tm1;
		localtime_r(&now,&tm1);
		int hour = tm1.tm_hour;
		int day = tm1.tm_mday;
		int month = tm1.tm_mon;

		int lastHourBackup = -1;
		int lastDayBackup = -1;
		int lastMonthBackup = -1;

		m_sql.GetLastBackupNo("Hour", lastHourBackup);
		m_sql.GetLastBackupNo("Day", lastDayBackup);
		m_sql.GetLastBackupNo("Month", lastMonthBackup);

		DIR *lDir;
		//struct dirent *ent;
		if ((lastHourBackup == -1)||(lastHourBackup !=hour)) {
		
			if ((lDir = opendir(sbackup_DirH.c_str())) != NULL)
			{
				std::stringstream sTmp;
				sTmp << "backup-hour-" << hour << ".db";

				std::string OutputFileName=sbackup_DirH + sTmp.str();
				if (m_sql.BackupDatabase(OutputFileName)) {
					m_sql.SetLastBackupNo("Hour", hour);
				}
				else {
					_log.Log(LOG_ERROR,"Error writing automatic hourly backup file");				
				}
				closedir(lDir);
			}
			else {
				_log.Log(LOG_ERROR,"Error accessing automatic backup directories");
			}
		}
		if ((lastDayBackup == -1)||(lastDayBackup !=day)) {
	
			if ((lDir = opendir(sbackup_DirD.c_str())) != NULL)
			{
				std::stringstream sTmp;
				sTmp << "backup-day-" << day << ".db";

				std::string OutputFileName=sbackup_DirD + sTmp.str();
				if (m_sql.BackupDatabase(OutputFileName)) {
					m_sql.SetLastBackupNo("Day", day);
				}
				else {
					_log.Log(LOG_ERROR,"Error writing automatic daily backup file");
				}
				closedir(lDir);
			}
			else {
				_log.Log(LOG_ERROR,"Error accessing automatic backup directories");
			}
		}
		if ((lastMonthBackup == -1)||(lastMonthBackup !=month)) {
			if ((lDir = opendir(sbackup_DirM.c_str())) != NULL)
			{
				std::stringstream sTmp;
				sTmp << "backup-month-" << month+1 << ".db";

				std::string OutputFileName=sbackup_DirM + sTmp.str();
				if (m_sql.BackupDatabase(OutputFileName)) {
					m_sql.SetLastBackupNo("Month", month);
				}
				else {
					_log.Log(LOG_ERROR,"Error writing automatic monthly backup file");
				}
				closedir(lDir);
			}
			else {
				_log.Log(LOG_ERROR,"Error accessing automatic backup directories");
			}
		}
	}	
}

void MainWorker::Do_Work()
{
	int second_counter=0;
	while (!m_stoprequested)
	{
		//sleep 500 milliseconds
		sleep_milliseconds(500);

		if (m_bDoDownloadDomoticzUpdate)
		{
			m_bDoDownloadDomoticzUpdate=false;
			std::string outfile = szStartupFolder+"update.tgz";
			m_bHaveDownloadedDomoticzUpdateSuccessFull=HTTPClient::GETBinaryToFile(m_szDomoticzUpdateURL.c_str(),outfile.c_str());
			m_bHaveDownloadedDomoticzUpdate=true;
		}

		if (m_scenes_to_start.size()>0)
		{
			boost::lock_guard<boost::mutex> l(m_startscene_mutex);
			std::vector<_tStartScene>::iterator itt=m_scenes_to_start.begin();
			while (itt!=m_scenes_to_start.end())
			{
				SwitchScene(itt->SceneRowID,itt->switchcmd);
				++itt;
			}
			m_scenes_to_start.clear();
		}

		second_counter++;
		if (second_counter<2)
			continue;
		second_counter=0;

		if (m_bStartHardware)
		{
			m_hardwareStartCounter++;
			if (m_hardwareStartCounter>=2)
			{
				m_bStartHardware=false;
				StartDomoticzHardware();
			}
		}

		if (m_SecCountdown>0)
		{
			m_SecCountdown--;
			if (m_SecCountdown==0)
			{
				SetInternalSecStatus();
			}
		}

		time_t atime=mytime(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);

		if (ltime.tm_min!=m_ScheduleLastMinute)
		{
			m_ScheduleLastMinute=ltime.tm_min;
			//check for 5 minute schedule
			if (ltime.tm_min%5==0)
			{
				m_sql.Schedule5Minute();
			}
			std::string szPwdResetFile=szStartupFolder+"resetpwd";
			if (file_exist(szPwdResetFile.c_str()))
			{
				m_webserver.ClearUserPasswords();
				m_sql.UpdatePreferencesVar("WebUserName","");
				m_sql.UpdatePreferencesVar("WebPassword","");
				std::remove(szPwdResetFile.c_str());
			}
		}
		if (ltime.tm_hour!=m_ScheduleLastHour)
		{
			m_ScheduleLastHour=ltime.tm_hour;
			GetSunSettings();
			m_sql.CheckDeviceTimeout();

			//check for daily schedule
			if (ltime.tm_hour==0)
			{
				m_sql.ScheduleDay();
			}
			HandleAutomaticBackups();
		}
		if ((bIsRaspberryPi)&&(ltime.tm_sec%30==0))
		{
			GetRaspberryPiTemperature();
		}
	}
	_log.Log(LOG_NORM, "Mainworker Stopped...");
}

void MainWorker::SendCommand(const int HwdID, unsigned char Cmd, const char *szMessage)
{
	int hindex=FindDomoticzHardware(HwdID);
	if (hindex==-1)
		return;
#ifdef _DEBUG
	if (szMessage!=NULL)
		_log.Log(LOG_NORM,"Cmd: %s", szMessage);
#endif

	tRBUF cmd;
	cmd.ICMND.packetlength = 13;
	cmd.ICMND.packettype = 0;
	cmd.ICMND.subtype = 0;
	cmd.ICMND.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
	cmd.ICMND.cmnd = Cmd;
	cmd.ICMND.msg1 = 0;
	cmd.ICMND.msg2 = 0;
	cmd.ICMND.msg3 = 0;
	cmd.ICMND.msg4 = 0;
	cmd.ICMND.msg5 = 0;
	cmd.ICMND.msg6 = 0;
	cmd.ICMND.msg7 = 0;
	cmd.ICMND.msg8 = 0;
	cmd.ICMND.msg9 = 0;
	WriteToHardware(HwdID, (const char*)&cmd,sizeof(cmd.ICMND));
}

void MainWorker::WriteToHardware(const int HwdID, const char *pdata, const unsigned char length)
{
	int hindex=FindDomoticzHardware(HwdID);
	if (hindex==-1)
		return;
	m_hardwaredevices[hindex]->WriteToHardware(pdata,length);
}

void MainWorker::WriteMessageStart()
{
	_log.LogSequenceStart();
}

void MainWorker::WriteMessageEnd()
{
	_log.LogSequenceEnd(LOG_NORM);
}

void MainWorker::WriteMessage(const char *szMessage)
{
	_log.LogSequenceAdd(szMessage);
}

void MainWorker::WriteMessage(const char *szMessage, bool linefeed)
{
	if (linefeed)
		_log.LogSequenceAdd(szMessage);
	else
		_log.LogSequenceAddNoLF(szMessage);
}

void MainWorker::OnHardwareConnected(CDomoticzHardwareBase *pHardware)
{
	SendResetCommand(pHardware);
}

unsigned long long MainWorker::PerformRealActionFromDomoticzClient(const unsigned char *pRXCommand)
{
	unsigned char devType=pRXCommand[1];
	unsigned char subType=pRXCommand[2];
	std::string ID="";
	unsigned char Unit=0;
	const tRBUF *pResponse=(const tRBUF *)pRXCommand;
	char szTmp[300];
	std::vector<std::vector<std::string> > result;

	if (devType==pTypeLighting1)
	{
		sprintf(szTmp,"%d", pResponse->LIGHTING1.housecode);
		ID = szTmp;
		Unit=pResponse->LIGHTING1.unitcode;
	}
	else if (devType==pTypeLighting2)
	{
		sprintf(szTmp,"%X%02X%02X%02X", pResponse->LIGHTING2.id1, pResponse->LIGHTING2.id2, pResponse->LIGHTING2.id3, pResponse->LIGHTING2.id4);
		ID = szTmp;
		Unit=pResponse->LIGHTING2.unitcode;
	}
	else if (devType==pTypeLighting5)
	{
		if (subType != 	sTypeEMW100)
			sprintf(szTmp,"%02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
		else
			sprintf(szTmp,"%02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
		ID = szTmp;
		Unit=pResponse->LIGHTING5.unitcode;
	}
	else if (devType==pTypeLighting6)
	{
		sprintf(szTmp,"%02X%02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2,pResponse->LIGHTING6.groupcode);
		ID = szTmp;
		Unit=pResponse->LIGHTING6.unitcode;
	}
	else if (devType==pTypeLimitlessLights)
	{
		_tLimitlessLights *pLed=(_tLimitlessLights *)pResponse;
		ID = "1";
		Unit=pLed->dunit;
	}
	else if (devType==pTypeBlinds)
	{
		sprintf(szTmp,"%02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2,pResponse->BLINDS1.id3);
		ID = szTmp;
		Unit=pResponse->BLINDS1.unitcode;
	}
	else if (devType==pTypeSecurity1)
	{
		sprintf(szTmp, "%02X%02X%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
		ID=szTmp;
		Unit=0;
	}
	else if (devType==pTypeChime)
	{
		sprintf(szTmp,"%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
		ID = szTmp;
		Unit=pResponse->CHIME.sound;
	}
	else
		return -1;

	if (ID!="")
	{
		// find our original hardware
		// if it is not a domoticz type, perform the actual command

		sprintf(szTmp,"SELECT HardwareID,ID,Name FROM DeviceStatus WHERE (DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",ID.c_str(), Unit, devType, subType);
		result=m_sql.query(szTmp);
		if (result.size()==1)
		{
			std::vector<std::string> sd=result[0];

			CDomoticzHardwareBase *pHardware=GetHardware(atoi(sd[0].c_str()));
			if (pHardware!=NULL)
			{
				if (pHardware->HwdType != HTYPE_Domoticz)
				{
					pHardware->WriteToHardware((const char*)pRXCommand,pRXCommand[0]+1);
					std::stringstream s_strid;
					s_strid << std::dec << sd[1];
					unsigned long long ullID;
					s_strid >> ullID;
					return ullID;
				}
			}
		}
	}
	return -1;
}

void MainWorker::DecodeRXMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand)
{
	boost::lock_guard<boost::mutex> l(decodeRXMessageMutex);

	// current date/time based on current system
	time_t now = time(0);

	size_t Len=pRXCommand[0]+1;

	// convert now to string form
	char *szDate = asctime(localtime(&now));

	int HwdID = pHardware->m_HwdID;
#ifdef DEBUG_RECEIVE
	std::stringstream sstream;
	sstream << szDate << "HwdID: " << HwdID << " (" << pHardware->Name << ")" << " RX: Len: " << std::dec << Len << " ";

	for (size_t ii=0; ii<Len; ii++)
	{
		if (ii!=0)
			sstream << ":";
		sstream << HEX((unsigned char)pRXCommand[ii]);
	}
	_log.Log(LOG_NORM,"%s",sstream.str().c_str());
#else
	szDate[strlen(szDate)-1]=0;

	WriteMessageStart();

	std::stringstream sTmp;
	sTmp << szDate << " (" << pHardware->Name << ") ";
	WriteMessage(sTmp.str().c_str(),false);
#endif

	unsigned long long DeviceRowIdx=-1;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		switch (pRXCommand[1])
		{
		case pTypeLighting1:
		case pTypeLighting2:
		case pTypeLighting3:
		case pTypeLighting4:
		case pTypeLighting5:
		case pTypeLighting6:
		case pTypeLimitlessLights:
		case pTypeBlinds:
		case pTypeSecurity1:
		case pTypeChime:
			//we received a control message from a domoticz client,
			//and should actually perform this command ourself switch
			DeviceRowIdx=PerformRealActionFromDomoticzClient(pRXCommand);
			if (DeviceRowIdx!=-1)
			{
				WriteMessage("Control Command");
			}
			break;
		}
	}

	if (DeviceRowIdx==-1)
	{
		switch (pRXCommand[1])
		{
		case pTypeInterfaceMessage:
			WriteMessage("Interface Message");
			DeviceRowIdx=decode_InterfaceMessage(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRecXmitMessage:
			WriteMessage("Receiver/Transmitter Message");
			DeviceRowIdx=decode_RecXmitMessage(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeUndecoded:
			WriteMessage("Undecoded RF Message");
			DeviceRowIdx=decode_UNDECODED(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting1:
			WriteMessage("Lighting 1",false);
			DeviceRowIdx=decode_Lighting1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting2:
			WriteMessage("Lighting 2",false);
			DeviceRowIdx=decode_Lighting2(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting3:
			WriteMessage("Lighting 3",false);
			DeviceRowIdx=decode_Lighting3(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting4:
			WriteMessage("Lighting 4",false);
			DeviceRowIdx=decode_Lighting4(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting5:
			WriteMessage("Lighting 5",false);
			DeviceRowIdx=decode_Lighting5(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting6:
			WriteMessage("Lighting 6",false);
			DeviceRowIdx=decode_Lighting6(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCurtain:
			WriteMessage("Curtain");	//only transmit
			break;
		case pTypeBlinds:
			WriteMessage("Blinds",false);
			DeviceRowIdx=decode_BLINDS1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeSecurity1:
			WriteMessage("Security 1",false);
			DeviceRowIdx=decode_Security1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCamera:
			WriteMessage("Camera",false);
			DeviceRowIdx=decode_Camera1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRemote:
			WriteMessage("Remote control & IR",false);
			DeviceRowIdx=decode_Remote(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat: //own type
			WriteMessage("Thermostat",false);
			DeviceRowIdx=decode_Thermostat(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat1:
			WriteMessage("Thermostat 1",false);
			DeviceRowIdx=decode_Thermostat1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat2:
			WriteMessage("Thermostat 2",false);
			DeviceRowIdx=decode_Thermostat2(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat3:
			WriteMessage("Thermostat 3",false);
			DeviceRowIdx=decode_Thermostat3(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP:
			WriteMessage("Temperature",false);
			DeviceRowIdx=decode_Temp(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeHUM:
			WriteMessage("Humidity",false);
			DeviceRowIdx=decode_Hum(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP_HUM:
			WriteMessage("Temperature + Humidity",false);
			DeviceRowIdx=decode_TempHum(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP_RAIN:
			WriteMessage("Temperature + Rain",false);
			DeviceRowIdx=decode_TempRain(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeBARO:
			WriteMessage("Barometric",false);
			DeviceRowIdx=decode_Baro(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP_HUM_BARO:
			WriteMessage("Temperature + Humidity + Barometric",false);
			DeviceRowIdx=decode_TempHumBaro(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP_BARO:
			WriteMessage("Temperature + Barometric",false);
			DeviceRowIdx=decode_TempBaro(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRAIN:
			WriteMessage("Rain Meter",false);
			DeviceRowIdx=decode_Rain(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeWIND:
			WriteMessage("Wind Meter",false);
			DeviceRowIdx=decode_Wind(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeUV:
			WriteMessage("UV Meter",false);
			DeviceRowIdx=decode_UV(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeDT:
			WriteMessage("Date & Time",false);
			DeviceRowIdx=decode_DateTime(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCURRENT:
			WriteMessage("Current Meter",false);
			DeviceRowIdx=decode_Current(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeENERGY:
			WriteMessage("Energy Meter",false);
			DeviceRowIdx=decode_Energy(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCURRENTENERGY:
			WriteMessage("Current/Energy Meter",false);
			DeviceRowIdx=decode_Current_Energy(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeGAS:
			WriteMessage("Gas Meter",false);
			DeviceRowIdx=decode_Gas(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeWATER:
			WriteMessage("Water Meter",false);
			DeviceRowIdx=decode_Water(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeWEIGHT:
			WriteMessage("Weight Scales",false);
			DeviceRowIdx=decode_Weight(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRFXSensor:
			WriteMessage("RFXSensor",false);
			DeviceRowIdx=decode_RFXSensor(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRFXMeter:
			WriteMessage("RFXMeter",false);
			DeviceRowIdx=decode_RFXMeter(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeP1Power:
			WriteMessage("P1 Smart Meter Power",false);
			DeviceRowIdx=decode_P1MeterPower(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeP1Gas:
			WriteMessage("P1 Smart Meter Gas",false);
			DeviceRowIdx=decode_P1MeterGas(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeUsage:
			WriteMessage("Usage Meter",false);
			DeviceRowIdx=decode_Usage(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeYouLess:
			WriteMessage("YouLess Meter",false);
			DeviceRowIdx=decode_YouLessMeter(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeAirQuality:
			WriteMessage("AirQuality Meter",false);
			DeviceRowIdx=decode_AirQuality(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRego6XXTemp:
			WriteMessage("Rego6XX temp",false);
			DeviceRowIdx=decode_Rego6XXTemp(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRego6XXValue:
			WriteMessage("Rego6XX value",false);
			DeviceRowIdx=decode_Rego6XXValue(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeFS20:
			WriteMessage("FS20");
			DeviceRowIdx=decode_FS20(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLux:
			WriteMessage("Lux Meter",false);
			DeviceRowIdx=decode_Lux(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeGeneral:
			WriteMessage("General",false);
			DeviceRowIdx=decode_General(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeChime:
			WriteMessage("Chime",false);
			DeviceRowIdx=decode_Chime(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeBBQ:
			WriteMessage("BBQ",false);
			DeviceRowIdx=decode_BBQ(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypePOWER:
			WriteMessage("Power Current/Energy Meter",false);
			DeviceRowIdx=decode_Power(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLimitlessLights:
			WriteMessage("Limitless Lights",false);
			DeviceRowIdx=decode_LimitlessLights(HwdID, (tRBUF *)pRXCommand);
			break;
		default:
			_log.Log(LOG_ERROR,"UNHANDLED PACKET TYPE:      FS20 %02X", pRXCommand[1]);
			break;
		}
	}
	WriteMessageEnd();
	if (DeviceRowIdx!=-1)
	{
		//Send to connected Sharing Users
		void *pClient2Ignore=NULL;
		if (pHardware->HwdType==HTYPE_Domoticz)
		{
			if (pHardware->m_HwdID!=8765)
			{
				pClient2Ignore=(void*)pHardware;
			}
		}
		m_sharedserver.SendToAll(DeviceRowIdx,(const char*)pRXCommand,pRXCommand[0]+1,pClient2Ignore);
	}
}

unsigned long long MainWorker::decode_InterfaceMessage(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeInterfaceMessage;

	char szTmp[100];

	switch (pResponse->IRESPONSE.subtype)
	{
	case sTypeInterfaceCommand:
		{
			WriteMessage("subtype           = Interface Response");
			sprintf(szTmp,"Sequence nbr      = %d",pResponse->IRESPONSE.seqnbr);
			WriteMessage(szTmp);
			switch (pResponse->IRESPONSE.cmnd)
			{
			case cmdSTATUS:
			case cmdSETMODE:
			case cmd310:
			case cmd315:
			case cmd800:
			case cmd800F:
			case cmd830:
			case cmd830F:
			case cmd835:
			case cmd835F:
			case cmd895:
				{
					WriteMessage("response on cmnd  = ", false);
					switch (pResponse->IRESPONSE.cmnd)
					{
					case cmdSTATUS:
						WriteMessage("Get Status");
						break;
					case cmdSETMODE:
						WriteMessage("Set Mode");
						break;
					case cmd310:
						WriteMessage("Select 310MHz");
						break;
					case cmd315:
						WriteMessage("Select 315MHz");
						break;
					case cmd800:
						WriteMessage("Select 868.00MHz");
						break;
					case cmd800F:
						WriteMessage("Select 868.00MHz FSK");
						break;
					case cmd830:
						WriteMessage("Select 868.30MHz");
						break;
					case cmd830F:
						WriteMessage("Select 868.30MHz FSK");
						break;
					case cmd835:
						WriteMessage("Select 868.35MHz");
						break;
					case cmd835F:
						WriteMessage("Select 868.35MHz FSK");
						break;
					case cmd895:
						WriteMessage("Select 868.95MHz");
						break;
					default:
						WriteMessage("Error: unknown response");
						break;
					}

					m_sql.UpdateRFXCOMHardwareDetails(HwdID,pResponse->IRESPONSE.msg1,pResponse->IRESPONSE.msg2,pResponse->ICMND.msg3,pResponse->ICMND.msg4,pResponse->ICMND.msg5);

					switch (pResponse->IRESPONSE.msg1)
					{
					case recType310:
						WriteMessage("Transceiver type  = 310MHz");
						break;
					case recType315:
						WriteMessage("Receiver type     = 315MHz");
						break;
					case recType43392:
						WriteMessage("Receiver type     = 433.92MHz (receive only)");
						break;
					case trxType43392:
						WriteMessage("Transceiver type  = 433.92MHz");
						break;
					case recType86800:
						WriteMessage("Receiver type     = 868.00MHz");
						break;
					case recType86800FSK:
						WriteMessage("Receiver type     = 868.00MHz FSK");
						break;
					case recType86830:
						WriteMessage("Receiver type     = 868.30MHz");
						break;
					case recType86830FSK:
						WriteMessage("Receiver type     = 868.30MHz FSK");
						break;
					case recType86835:
						WriteMessage("Receiver type     = 868.35MHz");
						break;
					case recType86835FSK:
						WriteMessage("Receiver type     = 868.35MHz FSK");
						break;
					case recType86895:
						WriteMessage("Receiver type     = 868.95MHz");
						break;
					default:
						WriteMessage("Receiver type     = unknown");
						break;
					}
					sprintf(szTmp,"Firmware version  = %d", pResponse->IRESPONSE.msg2);
					WriteMessage(szTmp);
					sprintf(szTmp,"Hardware version  = %d.%d",pResponse->IRESPONSE.msg6,pResponse->IRESPONSE.msg7);
					WriteMessage(szTmp);

					if (pResponse->IRESPONSE.UNDECODEDenabled)
						WriteMessage("Undec             on");
					else
						WriteMessage("Undec             off");

					if (pResponse->IRESPONSE.X10enabled)
						WriteMessage("X10               enabled");
					else
						WriteMessage("X10               disabled");

					if (pResponse->IRESPONSE.ARCenabled)
						WriteMessage("ARC               enabled");
					else
						WriteMessage("ARC               disabled");

					if (pResponse->IRESPONSE.ACenabled)
						WriteMessage("AC                enabled");
					else
						WriteMessage("AC                disabled");

					if (pResponse->IRESPONSE.HEEUenabled)
						WriteMessage("HomeEasy EU       enabled");
					else
						WriteMessage("HomeEasy EU       disabled");

					if (pResponse->IRESPONSE.MEIANTECHenabled)
						WriteMessage("Meiantech/Atlantic enabled");
					else
						WriteMessage("Meiantech/Atlantic disabled");

					if (pResponse->IRESPONSE.OREGONenabled)
						WriteMessage("Oregon Scientific enabled");
					else
						WriteMessage("Oregon Scientific disabled");

					if (pResponse->IRESPONSE.ATIenabled)
						WriteMessage("ATI               enabled");
					else
						WriteMessage("ATI               disabled");

					if (pResponse->IRESPONSE.VISONICenabled)
						WriteMessage("Visonic           enabled");
					else
						WriteMessage("Visonic           disabled");

					if (pResponse->IRESPONSE.MERTIKenabled)
						WriteMessage("Mertik            enabled");
					else
						WriteMessage("Mertik            disabled");

					if (pResponse->IRESPONSE.LWRFenabled)
						WriteMessage("AD                enabled");
					else
						WriteMessage("AD                disabled");

					if (pResponse->IRESPONSE.HIDEKIenabled)
						WriteMessage("Hideki            enabled");
					else
						WriteMessage("Hideki            disabled");

					if (pResponse->IRESPONSE.LACROSSEenabled)
						WriteMessage("La Crosse         enabled");
					else
						WriteMessage("La Crosse         disabled");

					if (pResponse->IRESPONSE.FS20enabled)
						WriteMessage("FS20              enabled");
					else
						WriteMessage("FS20              disabled");

					if (pResponse->IRESPONSE.PROGUARDenabled)
						WriteMessage("ProGuard          enabled");
					else
						WriteMessage("ProGuard          disabled");

					if (pResponse->IRESPONSE.BLINDST0enabled)
						WriteMessage("BlindsT0          enabled");
					else
						WriteMessage("BlindsT0          disabled");

					if (pResponse->IRESPONSE.BLINDST1enabled)
						WriteMessage("BlindsT1          enabled");
					else
						WriteMessage("BlindsT1          disabled");

					if (pResponse->IRESPONSE.AEenabled)
						WriteMessage("AE                enabled");
					else
						WriteMessage("AE                disabled");

					if (pResponse->IRESPONSE.RUBICSONenabled)
						WriteMessage("RUBiCSON          enabled");
					else
						WriteMessage("RUBiCSON          disabled");

					if (pResponse->IRESPONSE.FINEOFFSETenabled)
						WriteMessage("FineOffset        enabled");
					else
						WriteMessage("FineOffset        disabled");

					if (pResponse->IRESPONSE.LIGHTING4enabled)
						WriteMessage("Lighting4         enabled");
					else
						WriteMessage("Lighting4         disabled");

					if (pResponse->IRESPONSE.RSLenabled)
						WriteMessage("Conrad RSL        enabled");
					else
						WriteMessage("Conrad RSL        disabled");

					if (pResponse->IRESPONSE.SXenabled)
						WriteMessage("ByronSX           enabled");
					else
						WriteMessage("ByronSX           disabled");

					if (pResponse->IRESPONSE.RFU6enabled)
						WriteMessage("RFU protocol 6    enabled");
					else
						WriteMessage("RFU protocol 6    disabled");
				}
				break;
			case cmdSAVE:
				WriteMessage("response on cmnd  = Save");
				break;
			}
			break;
		}
		break;
	case sTypeInterfaceWrongCommand:
		WriteMessage("subtype           = Wrong command received from application");
		sprintf(szTmp,"Sequence nbr      = %d", pResponse->IRESPONSE.seqnbr);
		WriteMessage(szTmp);
		break;
	}
	return -1;
}

void MainWorker::decode_BateryLevel(bool bIsInPercentage, unsigned char level)
{
	if (bIsInPercentage)
	{
		switch (level)
		{
		case 0:
			WriteMessage("Battery       = 10%");
			break;
		case 1:
			WriteMessage("Battery       = 20%");
			break;
		case 2:
			WriteMessage("Battery       = 30%");
			break;
		case 3:
			WriteMessage("Battery       = 40%");
			break;
		case 4:
			WriteMessage("Battery       = 50%");
			break;
		case 5:
			WriteMessage("Battery       = 60%");
			break;
		case 6:
			WriteMessage("Battery       = 70%");
			break;
		case 7:
			WriteMessage("Battery       = 80%");
			break;
		case 8:
			WriteMessage("Battery       = 90%");
			break;
		case 9:
			WriteMessage("Battery       = 100%");
			break;
		}
	}
	else
	{
		if (level == 0 )
		{
			WriteMessage("Battery       = Low");
		}
		else
		{
			WriteMessage("Battery       = OK");
		}
	}
}

unsigned char MainWorker::get_BateryLevel(bool bIsInPercentage, unsigned char level)
{
	unsigned char ret=0;
	if (bIsInPercentage)
	{
		switch (level)
		{
		case 0:
			ret=10;
			break;
		case 1:
			ret=20;
			break;
		case 2:
			ret=30;
			break;
		case 3:
			ret=40;
			break;
		case 4:
			ret=50;
			break;
		case 5:
			ret=60;
			break;
		case 6:
			ret=70;
			break;
		case 7:
			ret=80;
			break;
		case 8:
			ret=90;
			break;
		case 9:
			ret=100;
			break;
		}
	}
	else
	{
		if (level == 0 )
		{
			ret=0;
		}
		else
		{
			ret=100;
		}
	}
	return ret;
}

unsigned long long MainWorker::decode_Rain(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeRAIN;
	unsigned char subType=pResponse->RAIN.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->RAIN.id1 * 256) + pResponse->RAIN.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->RAIN.rssi;
	unsigned char BatteryLevel = get_BateryLevel(pResponse->RAIN.subtype==sTypeRAIN1, pResponse->RAIN.battery_level & 0x0F);
	int Rainrate=(pResponse->RAIN.rainrateh * 256) + pResponse->RAIN.rainratel;
	float TotalRain=float((pResponse->RAIN.raintotal1 * 65535) + (pResponse->RAIN.raintotal2 * 256) + pResponse->RAIN.raintotal3) / 10.0f;
	sprintf(szTmp,"%d;%.1f",Rainrate,TotalRain);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleRainNotification(HwdID, ID, Unit, devType, subType, NTYPE_RAIN, TotalRain);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (subType)
		{
		case sTypeRAIN1:
			WriteMessage("subtype       = RAIN1 - RGR126/682/918/928");
			break;
		case sTypeRAIN2:
			WriteMessage("subtype       = RAIN2 - PCR800");
			break;
		case sTypeRAIN3:
			WriteMessage("subtype       = RAIN3 - TFA");
			break;
		case sTypeRAIN4:
			WriteMessage("subtype       = RAIN4 - UPM RG700");
			break;
		case sTypeRAIN5:
			WriteMessage("subtype       = RAIN5 - LaCrosse WS2300");
			break;
		case sTypeRAIN6:
			WriteMessage("subtype       = RAIN6 - LaCrosse TX5");
			break;
		case sTypeRAINWU:
			WriteMessage("subtype       = Weather Underground (Total Rain)");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X : %02X", pResponse->RAIN.packettype, pResponse->RAIN.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->RAIN.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp,"ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		if (pResponse->RAIN.subtype == sTypeRAIN1)
		{
			sprintf(szTmp,"Rain rate     = %d mm/h", Rainrate);
			WriteMessage(szTmp);
		}
		else if (pResponse->RAIN.subtype == sTypeRAIN2)
		{
			sprintf(szTmp,"Rain rate     = %d mm/h", Rainrate);
			WriteMessage(szTmp);
		}

		sprintf(szTmp,"Total rain    = %.1f mm", TotalRain);
		WriteMessage(szTmp);
		sprintf(szTmp,"Signal level  = %d", pResponse->RAIN.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->RAIN.subtype==sTypeRAIN1, pResponse->RAIN.battery_level & 0x0F);
	}
	return DevRowIdx;
}

#define round(a) ( int ) ( a + .5 )

unsigned long long MainWorker::decode_Wind(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[300];
	std::string devname;

	unsigned char devType=pTypeWIND;
	unsigned char subType=pResponse->WIND.subtype;
	unsigned short windID=(pResponse->WIND.id1*256) + pResponse->WIND.id2;
	sprintf(szTmp,"%d", windID);
	std::string ID = szTmp;
	unsigned char Unit=0;

	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->WIND.rssi;
	unsigned char BatteryLevel=get_BateryLevel(pResponse->WIND.subtype==sTypeWIND3, pResponse->WIND.battery_level & 0x0F);

	double dDirection;
	dDirection = (double)(pResponse->WIND.directionh * 256) + pResponse->WIND.directionl;
	dDirection=m_wind_calculator[windID].AddValueAndReturnAvarage(dDirection);

	std::string strDirection;
	if (dDirection > 348.75 || dDirection < 11.26)
		strDirection = "N";
	else if (dDirection < 33.76)
		strDirection = "NNE";
	else if (dDirection < 56.26)
		strDirection = "NE";
	else if (dDirection < 78.76)
		strDirection = "ENE";
	else if (dDirection < 101.26)
		strDirection = "E";
	else if (dDirection < 123.76)
		strDirection = "ESE";
	else if (dDirection < 146.26)
		strDirection = "SE";
	else if (dDirection < 168.76)
		strDirection = "SSE";
	else if (dDirection < 191.26)
		strDirection = "S";
	else if (dDirection < 213.76)
		strDirection = "SSW";
	else if (dDirection < 236.26)
		strDirection = "SW";
	else if (dDirection < 258.76)
		strDirection = "WSW";
	else if (dDirection < 281.26)
		strDirection = "W";
	else if (dDirection < 303.76)
		strDirection = "WNW";
	else if (dDirection < 326.26)
		strDirection = "NW";
	else if (dDirection < 348.76)
		strDirection = "NNW";
	else
		strDirection = "---";

	dDirection=round(dDirection);

	int intSpeed = (pResponse->WIND.av_speedh * 256) + pResponse->WIND.av_speedl;
	int intGust = (pResponse->WIND.gusth * 256) + pResponse->WIND.gustl;

	float temp=0,chill=0;
	if (pResponse->WIND.subtype == sTypeWIND4)
	{
		if (!pResponse->WIND.tempsign)
		{
			temp=float((pResponse->WIND.temperatureh * 256) + pResponse->WIND.temperaturel) / 10.0f;
		}
		else
		{
			temp=-(float(((pResponse->WIND.temperatureh & 0x7F) * 256) + pResponse->WIND.temperaturel) / 10.0f);
		}
		if ((temp<-60)||(temp>260))
		{
			WriteMessage(" Invalid Temperature");
			return -1;
		}

		float AddjValue=0.0f;
		float AddjMulti=1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
		temp+=AddjValue;

		if (!pResponse->WIND.chillsign)
		{
			chill=float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0f;
		}
		else
		{
			chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0f);
		}
		chill+=AddjValue;

		float wspeedms=float(intSpeed)/10.0f;
		if ((temp<10.0)&&(wspeedms>=1.4))
		{
			float chillJatTI = 13.12f+0.6215f*temp -11.37f*pow(wspeedms*3.6f,0.16f) + 0.3965f*temp*pow(wspeedms*3.6f,0.16f);
			chill=chillJatTI;
		}
	}
	else if (pResponse->WIND.subtype==sTypeWINDNoTemp)
	{
		if (!pResponse->WIND.tempsign)
		{
			temp=float((pResponse->WIND.temperatureh * 256) + pResponse->WIND.temperaturel) / 10.0f;
		}
		else
		{
			temp=-(float(((pResponse->WIND.temperatureh & 0x7F) * 256) + pResponse->WIND.temperaturel) / 10.0f);
		}
		if ((temp<-60)||(temp>260))
		{
			WriteMessage(" Invalid Temperature");
			return -1;
		}

		float AddjValue=0.0f;
		float AddjMulti=1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
		temp+=AddjValue;

		if (!pResponse->WIND.chillsign)
		{
			chill=float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0f;
		}
		else
		{
			chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0f);
		}
		chill+=AddjValue;
	}

	sprintf(szTmp,"%.2f;%s;%d;%d;%.1f;%.1f",dDirection,strDirection.c_str(),intSpeed,intGust,temp,chill);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	float wspeedms=float(intSpeed)/10.0f;
	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_WIND, wspeedms);

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, 0, true, false);


	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->WIND.subtype)
		{
		case sTypeWIND1:
			WriteMessage("subtype       = WIND1 - WTGR800");
			break;
		case sTypeWIND2:
			WriteMessage("subtype       = WIND2 - WGR800");
			break;
		case sTypeWIND3:
			WriteMessage("subtype       = WIND3 - STR918/928, WGR918");
			break;
		case sTypeWIND4:
			WriteMessage("subtype       = WIND4 - TFA");
			break;
		case sTypeWIND5:
			WriteMessage("subtype       = WIND5 - UPM WDS500");
			break;
		case sTypeWIND6:
			WriteMessage("subtype       = WIND6 - LaCrosse WS2300");
			break;
		case sTypeWINDNoTemp:
			WriteMessage("subtype       = Weather Station");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->WIND.packettype, pResponse->WIND.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->WIND.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		sprintf(szTmp,"Direction     = %d degrees %s", int(dDirection), strDirection.c_str());
		WriteMessage(szTmp);

		if (pResponse->WIND.subtype != sTypeWIND5)
		{
			sprintf(szTmp,"Average speed = %.1f mtr/sec, %.2f km/hr, %.2f mph",float(intSpeed) / 10.0f, (float(intSpeed) * 0.36f), (float(intSpeed) * 0.223693629f));
			WriteMessage(szTmp);
		}

		sprintf(szTmp,"Wind gust     = %.1f mtr/sec, %.2f km/hr, %.2f mph", float(intGust) / 10.0f,  (float(intGust )* 0.36f),  (float(intGust) * 0.223693629f));
		WriteMessage(szTmp);

		if (pResponse->WIND.subtype == sTypeWIND4)
		{
			sprintf(szTmp,"Temperature   = %.1f C", temp);
			WriteMessage(szTmp);

			sprintf(szTmp,"Chill         = %.1f C", chill);
		}
		if (pResponse->WIND.subtype == sTypeWINDNoTemp)
		{
			sprintf(szTmp,"Temperature   = %.1f C", temp);
			WriteMessage(szTmp);

			sprintf(szTmp,"Chill         = %.1f C", chill);
		}

		sprintf(szTmp,"Signal level  = %d", pResponse->WIND.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->WIND.subtype==sTypeWIND3, pResponse->WIND.battery_level & 0x0F);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Temp(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeTEMP;
	unsigned char subType=pResponse->TEMP.subtype;
	sprintf(szTmp,"%d",(pResponse->TEMP.id1 * 256) + pResponse->TEMP.id2);
	std::string ID=szTmp;
	unsigned char Unit=pResponse->TEMP.id2;

	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->TEMP.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->TEMP.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	float temp;
	if (!pResponse->TEMP.tempsign)
	{
		temp=float((pResponse->TEMP.temperatureh * 256) + pResponse->TEMP.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP.temperatureh & 0x7F) * 256) + pResponse->TEMP.temperaturel) / 10.0f);
	}
	if ((temp<-60)||(temp>260))
	{
		WriteMessage(" Invalid Temperature");
		return -1;
	}

	float AddjValue=0.0f;
	float AddjMulti=1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	temp+=AddjValue;

	sprintf(szTmp,"%.1f",temp);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);

	unsigned char humidity=0;
	if (pResponse->TEMP.subtype==sTypeTEMP5)
	{
		//check if we already had a humidity for this device, if so, keep it!
		char szTmp[300];
		std::vector<std::vector<std::string> > result;

		sprintf(szTmp,"SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HwdID, ID.c_str(),1,pTypeHUM,sTypeHUM1);
		result=m_sql.query(szTmp);
		if (result.size()==1)
		{
			m_sql.GetAddjustment(HwdID, ID.c_str(),2,pTypeTEMP_HUM,sTypeTH_LC_TC,AddjValue,AddjMulti);
			temp+=AddjValue;
			humidity=atoi(result[0][0].c_str());
			unsigned char humidity_status=atoi(result[0][1].c_str());
			sprintf(szTmp,"%.1f;%d;%d",temp,humidity,humidity_status);
			DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),2,pTypeTEMP_HUM,sTypeTH_LC_TC,SignalLevel,BatteryLevel,0,szTmp,devname);
			m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, 2, pTypeTEMP_HUM, sTypeTH_LC_TC, temp, humidity, true, true);
			float dewpoint=(float)CalculateDewPoint(temp,humidity);
			m_sql.CheckAndHandleDewPointNotification(HwdID, ID, 2, pTypeTEMP_HUM, sTypeTH_LC_TC,temp,dewpoint);
		}
	}
	PrintDeviceName(devname);

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, humidity, true, false);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->TEMP.subtype)
		{
		case sTypeTEMP1:
			WriteMessage("subtype       = TEMP1 - THR128/138, THC138");
			sprintf(szTmp,"                channel %d" , pResponse->TEMP.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTEMP2:
			WriteMessage("subtype       = TEMP2 - THC238/268,THN132,THWR288,THRN122,THN122,AW129/131");
			sprintf(szTmp,"                channel %d" , pResponse->TEMP.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTEMP3:
			WriteMessage("subtype       = TEMP3 - THWR800");
			break;
		case sTypeTEMP4:
			WriteMessage("subtype       = TEMP4 - RTHN318");
			sprintf(szTmp,"                channel %d" , pResponse->TEMP.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTEMP5:
			WriteMessage("subtype       = TEMP5 - LaCrosse TX2, TX3, TX4, TX17");
			break;
		case sTypeTEMP6:
			WriteMessage("subtype       = TEMP6 - TS15C");
			break;
		case sTypeTEMP7:
			WriteMessage("subtype       = TEMP7 - Viking 02811");
			break;
		case sTypeTEMP8:
			WriteMessage("subtype       = TEMP8 - LaCrosse WS2300");
			break;
		case sTypeTEMP9:
			if (pResponse->TEMP.id2 & 0xFF)
				WriteMessage("subtype       = TEMP9 - RUBiCSON 48659 stektermometer");
			else
				WriteMessage("subtype       = TEMP9 - RUBiCSON 48695");
			break;
		case sTypeTEMP10:
			WriteMessage("subtype       = TEMP10 - TFA 30.3133");
			break;
		case sTypeTEMP_SYSTEM:
			WriteMessage("subtype       = System");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP.packettype, pResponse->TEMP.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->TEMP.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->TEMP.id1 * 256) + pResponse->TEMP.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Temperature   = %.1f C", temp);
		WriteMessage(szTmp);

		sprintf(szTmp,"Signal level  = %d", pResponse->TEMP.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP.battery_level &0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Hum(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeHUM;
	unsigned char subType=pResponse->HUM.subtype;
	sprintf(szTmp,"%d",(pResponse->HUM.id1 * 256) + pResponse->HUM.id2);
	std::string ID=szTmp;
	unsigned char Unit=1;

	unsigned char SignalLevel=pResponse->HUM.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->HUM.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	unsigned char humidity=pResponse->HUM.humidity;
	if (humidity>100)
	{
		WriteMessage(" Invalid Humidity");
		return -1;
	}

	sprintf(szTmp,"%d",pResponse->HUM.humidity_status);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,humidity,szTmp,devname);

	float temp=0;
	if (pResponse->HUM.subtype==sTypeHUM1)
	{
		//check if we already had a humidity for this device, if so, keep it!
		char szTmp[300];
		std::vector<std::vector<std::string> > result;

		sprintf(szTmp,"SELECT sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HwdID, ID.c_str(),0,pTypeTEMP,sTypeTEMP5);
		result=m_sql.query(szTmp);
		if (result.size()==1)
		{
			temp=(float)atof(result[0][0].c_str());
			float AddjValue=0.0f;
			float AddjMulti=1.0f;
			m_sql.GetAddjustment(HwdID, ID.c_str(),2,pTypeTEMP_HUM,sTypeTH_LC_TC,AddjValue,AddjMulti);
			temp+=AddjValue;
			sprintf(szTmp,"%.1f;%d;%d",temp,humidity,pResponse->HUM.humidity_status);
			DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),2,pTypeTEMP_HUM,sTypeTH_LC_TC,SignalLevel,BatteryLevel,0,szTmp,devname);
			m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, 2, pTypeTEMP_HUM, sTypeTH_LC_TC, temp, humidity, true, true);
			float dewpoint=(float)CalculateDewPoint(temp,humidity);
			m_sql.CheckAndHandleDewPointNotification(HwdID, ID, 2, pTypeTEMP_HUM, sTypeTH_LC_TC,temp,dewpoint);
		}
	}
	PrintDeviceName(devname);

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, humidity, false, true);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->HUM.subtype)
		{
		case sTypeHUM1:
			WriteMessage("subtype       = HUM1 - LaCrosse TX3");
			break;
		case sTypeHUM2:
			WriteMessage("subtype       = HUM2 - LaCrosse WS2300");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->HUM.packettype, pResponse->HUM.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d" ,pResponse->HUM.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->HUM.id1 * 256) + pResponse->HUM.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Humidity      = %d %%" ,pResponse->HUM.humidity);
		WriteMessage(szTmp);

		switch(pResponse->HUM.humidity_status)
		{
		case humstat_normal:
			WriteMessage("Status        = Normal");
			break;
		case humstat_comfort:
			WriteMessage("Status        = Comfortable");
			break;
		case humstat_dry:
			WriteMessage("Status        = Dry");
			break;
		case humstat_wet:
			WriteMessage("Status        = Wet");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->HUM.rssi);
		WriteMessage(szTmp);

		if ((pResponse->HUM.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_TempHum(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeTEMP_HUM;
	unsigned char subType=pResponse->TEMP_HUM.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->TEMP_HUM.id1 * 256) + pResponse->TEMP_HUM.id2);
	ID=szTmp;
	unsigned char Unit=0;

	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->TEMP_HUM.rssi;
	unsigned char BatteryLevel = get_BateryLevel(pResponse->TEMP_HUM.subtype == sTypeTH8,pResponse->TEMP_HUM.battery_level);

	//Get Channel(Unit)
	switch (pResponse->TEMP_HUM.subtype)
	{
	case sTypeTH1:
	case sTypeTH2:
	case sTypeTH3:
	case sTypeTH4:
	case sTypeTH6:
	case sTypeTH8:
	case sTypeTH10:
	case sTypeTH11:
		Unit = pResponse->TEMP_HUM.id2;
		break;
	case sTypeTH5:
	case sTypeTH9:
		//no channel
		break;
	case sTypeTH7:
		if (pResponse->TEMP_HUM.id1 < 0x40)
			Unit = 1;
		else if (pResponse->TEMP_HUM.id1 < 0x60)
			Unit = 2;
		else if (pResponse->TEMP_HUM.id1 < 0x80)
			Unit = 3;
		else if ((pResponse->TEMP_HUM.id1 > 0x9F) && (pResponse->TEMP_HUM.id1 < 0xC0))
			Unit = 4;
		else if (pResponse->TEMP_HUM.id1 < 0xE0)
			Unit = 5;
		break;
	}


	float temp;
	if (!pResponse->TEMP_HUM.tempsign)
	{
		temp=float((pResponse->TEMP_HUM.temperatureh * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP_HUM.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0f);
	}
	if ((temp<-60)||(temp>260))
	{
		WriteMessage(" Invalid Temperature");
		return -1;
	}

	float AddjValue=0.0f;
	float AddjMulti=1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	temp+=AddjValue;

	int Humidity = (int)pResponse->TEMP_HUM.humidity;
	unsigned char HumidityStatus = pResponse->TEMP_HUM.humidity_status;

	if (Humidity>100)
	{
		WriteMessage(" Invalid Humidity");
		return -1;
	}
/*
	AddjValue=0.0f;
	AddjMulti=1.0f;
	m_sql.GetAddjustment2(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	Humidity+=int(AddjValue);
	if (Humidity>100)
		Humidity=100;
	if (Humidity<0)
		Humidity=0;
*/
	sprintf(szTmp,"%.1f;%d;%d",temp,Humidity,HumidityStatus);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, Humidity, true, true);

	float dewpoint=(float)CalculateDewPoint(temp,Humidity);
	m_sql.CheckAndHandleDewPointNotification(HwdID, ID, Unit, devType, subType,temp,dewpoint);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->TEMP_HUM.subtype)
		{
		case sTypeTH1:
			WriteMessage("subtype       = TH1 - THGN122/123/132,THGR122/228/238/268");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;;
		case sTypeTH2:
			WriteMessage("subtype       = TH2 - THGR810,THGN800");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH3:
			WriteMessage("subtype       = TH3 - RTGR328");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH4:
			WriteMessage("subtype       = TH4 - THGR328");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH5:
			WriteMessage("subtype       = TH5 - WTGR800");
			break;
		case sTypeTH6:
			WriteMessage("subtype       = TH6 - THGR918/928,THGRN228,THGN500");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH7:
			WriteMessage("subtype       = TH7 - Cresta, TFA TS34C");
			if (pResponse->TEMP_HUM.id1 < 0x40)
				WriteMessage("                channel 1");
			else if (pResponse->TEMP_HUM.id1 < 0x60)
				WriteMessage("                channel 2");
			else if (pResponse->TEMP_HUM.id1 < 0x80)
				WriteMessage("                channel 3");
			else if ((pResponse->TEMP_HUM.id1 > 0x9F) && (pResponse->TEMP_HUM.id1 < 0xC0))
				WriteMessage("                channel 4");
			else if (pResponse->TEMP_HUM.id1 < 0xE0)
				WriteMessage("                channel 5");
			else
				WriteMessage("                channel ??");
			break;
		case sTypeTH8:
			WriteMessage("subtype       = TH8 - WT260,WT260H,WT440H,WT450,WT450H");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH9:
			WriteMessage("subtype       = TH9 - Viking 02038, 02035 (02035 has no humidity)");
			break;
		case sTypeTH10:
			WriteMessage("subtype       = TH10 - Rubicson/IW008T/TX95");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH11:
			WriteMessage("subtype       = TH11 - Oregon EW109");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP_HUM.packettype, pResponse->TEMP_HUM.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->TEMP_HUM.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		sprintf(szTmp,"Temperature   = %.1f C", temp);
		WriteMessage(szTmp);
		sprintf(szTmp,"Humidity      = %d %%" ,Humidity);
		WriteMessage(szTmp);

		switch(pResponse->TEMP_HUM.humidity_status)
		{
		case humstat_normal:
			WriteMessage("Status        = Normal");
			break;
		case humstat_comfort:
			WriteMessage("Status        = Comfortable");
			break;
		case humstat_dry:
			WriteMessage("Status        = Dry");
			break;
		case humstat_wet:
			WriteMessage("Status        = Wet");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->TEMP_HUM.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->TEMP_HUM.subtype == sTypeTH8,pResponse->TEMP_HUM.battery_level);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_TempHumBaro(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeTEMP_HUM_BARO;
	unsigned char subType=pResponse->TEMP_HUM_BARO.subtype;
	sprintf(szTmp,"%d",(pResponse->TEMP_HUM_BARO.id1 * 256) + pResponse->TEMP_HUM_BARO.id2);
	std::string ID=szTmp;
	unsigned char Unit=pResponse->TEMP_HUM_BARO.id2;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->TEMP_HUM_BARO.rssi;
	unsigned char BatteryLevel;
	if ((pResponse->TEMP_HUM_BARO.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	float temp;
	if (!pResponse->TEMP_HUM_BARO.tempsign)
	{
		temp=float((pResponse->TEMP_HUM_BARO.temperatureh * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP_HUM_BARO.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0f);
	}
	if ((temp<-60)||(temp>260))
	{
		WriteMessage(" Invalid Temperature");
		return -1;
	}

	float AddjValue=0.0f;
	float AddjMulti=1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	temp+=AddjValue;

	unsigned char Humidity = pResponse->TEMP_HUM_BARO.humidity;
	unsigned char HumidityStatus = pResponse->TEMP_HUM_BARO.humidity_status;

	if (Humidity>100)
	{
		WriteMessage(" Invalid Humidity");
		return -1;
	}

	int barometer = (pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol;

	int forcast = pResponse->TEMP_HUM_BARO.forecast;
	float fbarometer=(float)barometer;

	m_sql.GetAddjustment2(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	barometer+=int(AddjValue);

	if (pResponse->TEMP_HUM_BARO.subtype==sTypeTHBFloat)
	{
		if ((barometer<8000)||(barometer>12000))
		{
			WriteMessage(" Invalid Barometer");
			return -1;
		}
		fbarometer=float((pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol)/10.0f;
		fbarometer+=AddjValue;
		sprintf(szTmp,"%.1f;%d;%d;%.1f;%d",temp,Humidity,HumidityStatus, fbarometer,forcast);
	}
	else
	{
		if ((barometer<800)||(barometer>1200))
		{
			WriteMessage(" Invalid Barometer");
			return -1;
		}
		sprintf(szTmp,"%.1f;%d;%d;%d;%d",temp,Humidity,HumidityStatus, barometer,forcast);
	}
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	//calculate Altitude
	//float seaLevelPressure=101325.0f;
	//float altitude = 44330.0f * (1.0f - pow(fbarometer / seaLevelPressure, 0.1903f));

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, Humidity, true, true);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_BARO, fbarometer);

	float dewpoint=(float)CalculateDewPoint(temp,Humidity);
	m_sql.CheckAndHandleDewPointNotification(HwdID, ID, Unit, devType, subType,temp,dewpoint);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->TEMP_HUM_BARO.subtype)
		{
		case sTypeTHB1:
			WriteMessage("subtype       = THB1 - BTHR918");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM_BARO.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTHB2:
			WriteMessage("subtype       = THB2 - BTHR918N, BTHR968");
			sprintf(szTmp,"                channel %d", pResponse->TEMP_HUM_BARO.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTHBFloat:
			WriteMessage("subtype       = Weather Station");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP_HUM_BARO.packettype, pResponse->TEMP_HUM_BARO.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->TEMP_HUM_BARO.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = %d", (pResponse->TEMP_HUM_BARO.id1 * 256) + pResponse->TEMP_HUM_BARO.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Temperature   = %.1f C", temp);
		WriteMessage(szTmp);

		sprintf(szTmp,"Humidity      = %d %%" ,pResponse->TEMP_HUM_BARO.humidity);
		WriteMessage(szTmp);

		switch(pResponse->TEMP_HUM_BARO.humidity_status)
		{
		case humstat_normal:
			WriteMessage("Status        = Normal");
			break;
		case humstat_comfort:
			WriteMessage("Status        = Comfortable");
			break;
		case humstat_dry:
			WriteMessage("Status        = Dry");
			break;
		case humstat_wet:
			WriteMessage("Status        = Wet");
			break;
		}

		if (pResponse->TEMP_HUM_BARO.subtype==sTypeTHBFloat)
			sprintf(szTmp,"Barometer     = %.1f hPa", float((pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol)/10.0f);
		else
			sprintf(szTmp,"Barometer     = %d hPa", (pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol);
		WriteMessage(szTmp);

		switch (pResponse->TEMP_HUM_BARO.forecast)
		{
		case baroForecastNoInfo:
			WriteMessage("Forecast      = No information available");
			break;
		case baroForecastSunny:
			WriteMessage("Forecast      = Sunny");
			break;
		case baroForecastPartlyCloudy:
			WriteMessage("Forecast      = Partly Cloudy");
			break;
		case baroForecastCloudy:
			WriteMessage("Forecast      = Cloudy");
			break;
		case baroForecastRain:
			WriteMessage("Forecast      = Rain");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->TEMP_HUM_BARO.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP_HUM_BARO.battery_level &0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_TempBaro(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeTEMP_BARO;
	unsigned char subType=sTypeBMP085;
	_tTempBaro *pTempBaro=(_tTempBaro*)pResponse;

	sprintf(szTmp,"%d",pTempBaro->id1);
	std::string ID=szTmp;
	unsigned char Unit=1;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel;
	BatteryLevel=100;

	float temp=pTempBaro->temp;
	if ((temp<-60)||(temp>260))
	{
		WriteMessage(" Invalid Temperature");
		return -1;
	}

	float AddjValue=0.0f;
	float AddjMulti=1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	temp+=AddjValue;

	float fbarometer = pTempBaro->baro;
	int forcast = pTempBaro->forecast;
	m_sql.GetAddjustment2(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	fbarometer+=AddjValue;

	sprintf(szTmp,"%.1f;%.1f;%d;%.2f",temp,fbarometer,forcast,pTempBaro->altitude);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, 0, true, false);
	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_BARO, fbarometer);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->TEMP_HUM_BARO.subtype)
		{
		case sTypeBMP085:
			WriteMessage("subtype       = BMP085 I2C");
			sprintf(szTmp,"                channel %d", pTempBaro->id1);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", devType, subType);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Temperature   = %.1f C", temp);
		WriteMessage(szTmp);

		sprintf(szTmp,"Barometer     = %.1f hPa", fbarometer);
		WriteMessage(szTmp);

		switch (pTempBaro->forecast)
		{
		case baroForecastNoInfo:
			WriteMessage("Forecast      = No information available");
			break;
		case baroForecastSunny:
			WriteMessage("Forecast      = Sunny");
			break;
		case baroForecastPartlyCloudy:
			WriteMessage("Forecast      = Partly Cloudy");
			break;
		case baroForecastCloudy:
			WriteMessage("Forecast      = Cloudy");
			break;
		case baroForecastRain:
			WriteMessage("Forecast      = Rain");
			break;
		}

		if (pResponse->TEMP_HUM_BARO.subtype==sTypeBMP085)
		{
			sprintf(szTmp,"Altitude   = %.2f meter", pTempBaro->altitude);
			WriteMessage(szTmp);
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_TempRain(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;


	//We are (also) going to split this device into two separate sensors (temp + rain)

	unsigned char devType=pTypeTEMP_RAIN;
	unsigned char subType=pResponse->TEMP_RAIN.subtype;

	sprintf(szTmp,"%d",(pResponse->TEMP_RAIN.id1 * 256) + pResponse->TEMP_RAIN.id2);
	std::string ID=szTmp;
	int Unit=pResponse->TEMP_RAIN.id2;
	int cmnd=0;

	unsigned char SignalLevel=pResponse->TEMP_RAIN.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->TEMP_RAIN.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	float temp;
	if (!pResponse->TEMP_RAIN.tempsign)
	{
		temp=float((pResponse->TEMP_RAIN.temperatureh * 256) + pResponse->TEMP_RAIN.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP_RAIN.temperatureh & 0x7F) * 256) + pResponse->TEMP_RAIN.temperaturel) / 10.0f);
	}

	float AddjValue=0.0f;
	float AddjMulti=1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,pTypeTEMP,sTypeTEMP3,AddjValue,AddjMulti);
	temp+=AddjValue;

	if ((temp<-60)||(temp>260))
	{
		WriteMessage(" Invalid Temperature");
		return -1;
	}
	float TotalRain=float((pResponse->TEMP_RAIN.raintotal1 * 256) + pResponse->TEMP_RAIN.raintotal2) / 10.0f;

	sprintf(szTmp,"%.1f;%.1f",temp,TotalRain);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	sprintf(szTmp,"%.1f",temp);
	unsigned long long DevRowIdxTemp=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,pTypeTEMP,sTypeTEMP3,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, pTypeTEMP, sTypeTEMP3, temp, 0, true, false);

	sprintf(szTmp,"%d;%.1f",0,TotalRain);
	unsigned long long DevRowIdxRain=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,pTypeRAIN,sTypeRAIN3,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	m_sql.CheckAndHandleRainNotification(HwdID, ID, Unit, pTypeRAIN, sTypeRAIN3, NTYPE_RAIN, TotalRain);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->TEMP_RAIN.subtype)
		{
		case sTypeTR1:
			WriteMessage("Subtype       = Alecto WS1200");
			break;
		}
		sprintf(szTmp,"Sequence nbr  = %d", pResponse->TEMP_RAIN.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->TEMP_RAIN.id1 * 256) + pResponse->TEMP_RAIN.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Temperature   = %.1f C", temp);
		WriteMessage(szTmp);
		sprintf(szTmp,"Total Rain    = %.1f mm", TotalRain);
		WriteMessage(szTmp);

		sprintf(szTmp,"Signal level  = %d", pResponse->TEMP_RAIN.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP_RAIN.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}

	return DevRowIdx;
}

unsigned long long MainWorker::decode_UV(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;
	unsigned char devType=pTypeUV;
	unsigned char subType=pResponse->UV.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->UV.id1 * 256) + pResponse->UV.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->UV.rssi;
	unsigned char BatteryLevel;
	if ((pResponse->UV.battery_level & 0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;
	float Level=float(pResponse->UV.uv) / 10.0f;
	if (Level>30)
	{
		WriteMessage(" Invalid UV");
		return -1;
	}
	float temp=0;
	if (pResponse->UV.subtype == sTypeUV3)
	{
		if (!pResponse->UV.tempsign)
		{
			temp = float((pResponse->UV.temperatureh * 256) + pResponse->UV.temperaturel) / 10.0f;
		}
		else
		{
			temp = -(float(((pResponse->UV.temperatureh & 0x7F) * 256) + pResponse->UV.temperaturel) / 10.0f);
		}
		if ((temp<-60)||(temp>260))
		{
			WriteMessage(" Invalid Temperature");
			return -1;
		}

		float AddjValue=0.0f;
		float AddjMulti=1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
		temp+=AddjValue;
	}

	sprintf(szTmp,"%.1f;%.1f",Level,temp);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, 0, true, false);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_UV, Level);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->UV.subtype)
		{
		case sTypeUV1:
			WriteMessage("Subtype       = UV1 - UVN128, UV138");
			break;
		case sTypeUV2:
			WriteMessage("Subtype       = UV2 - UVN800");
			break;
		case sTypeUV3:
			WriteMessage("Subtype       = UV3 - TFA");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->UV.packettype, pResponse->UV.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->UV.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->UV.id1 * 256) + pResponse->UV.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Level         = %.1f UVI", Level);
		WriteMessage(szTmp);

		if (pResponse->UV.subtype == sTypeUV3)
		{
			sprintf(szTmp,"Temperature   = %.1f C", temp);
			WriteMessage(szTmp);
		}

		if (pResponse->UV.uv < 3)
			WriteMessage("Description = Low");
		else if (pResponse->UV.uv < 6)
			WriteMessage("Description = Medium");
		else if (pResponse->UV.uv < 8)
			WriteMessage("Description = High");
		else if (pResponse->UV.uv < 11)
			WriteMessage("Description = Very high");
		else
			WriteMessage("Description = Dangerous");

		sprintf(szTmp,"Signal level  = %d", pResponse->UV.rssi);
		WriteMessage(szTmp);

		if ((pResponse->UV.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Lighting1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeLighting1;
	unsigned char subType=pResponse->LIGHTING1.subtype;
	sprintf(szTmp,"%d", pResponse->LIGHTING1.housecode);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING1.unitcode;
	unsigned char cmnd=pResponse->LIGHTING1.cmnd;
	unsigned char SignalLevel=pResponse->LIGHTING1.rssi;
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,"");

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,devname);
	PrintDeviceName(devname);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->LIGHTING1.subtype)
		{
		case sTypeX10:
			WriteMessage("subtype       = X10");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp,"unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			case light1_sDim:
				WriteMessage("Dim");
				break;
			case light1_sBright:
				WriteMessage("Bright");
				break;
			case light1_sAllOn:
				WriteMessage("All On");
				break;
			case light1_sAllOff:
				WriteMessage("All Off");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeARC:
			WriteMessage("subtype       = ARC");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp,"unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			case light1_sAllOn:
				WriteMessage("All On");
				break;
			case light1_sAllOff:
				WriteMessage("All Off");
				break;
			case light1_sChime:
				WriteMessage("Chime");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeAB400D:
		case sTypeWaveman:
		case sTypeEMW200:
		case sTypeIMPULS:
		case sTypeRisingSun:
		case sTypeEnergenie:
			//decoding of these types is only implemented for use by simulate and verbose
			//these types are not received by the RFXtrx433
			switch (pResponse->LIGHTING1.subtype)
			{
			case sTypeAB400D:
				WriteMessage("subtype       = ELRO AB400");
				break;
			case sTypeWaveman:
				WriteMessage("subtype       = Waveman");
				break;
			case sTypeEMW200:
				WriteMessage("subtype       = EMW200");
				break;
			case sTypeIMPULS:
				WriteMessage("subtype       = IMPULS");
				break;
			case sTypeRisingSun:
				WriteMessage("subtype       = RisingSun");
				break;
			case sTypeEnergenie:
				WriteMessage("subtype       = Energenie");
				break;
			}
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp,"unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);

			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypePhilips:
			//decoding of this type is only implemented for use by simulate and verbose
			//this type is not received by the RFXtrx433
			WriteMessage("subtype       = Philips SBC");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp,"unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);

			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			case light1_sAllOff:
				WriteMessage("All Off");
				break;
			case light1_sAllOn:
				WriteMessage("All On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING1.packettype, pResponse->LIGHTING1.subtype);
			WriteMessage(szTmp);
			break;
		}
		printf(szTmp,"Signal level  = %d", pResponse->LIGHTING1.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Lighting2(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeLighting2;
	unsigned char subType=pResponse->LIGHTING2.subtype;
	sprintf(szTmp,"%X%02X%02X%02X", pResponse->LIGHTING2.id1, pResponse->LIGHTING2.id2, pResponse->LIGHTING2.id3, pResponse->LIGHTING2.id4);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING2.unitcode;
	unsigned char cmnd=pResponse->LIGHTING2.cmnd;
	unsigned char level=pResponse->LIGHTING2.level;
	unsigned char SignalLevel=pResponse->LIGHTING2.rssi;

	sprintf(szTmp,"%d",level);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,szTmp,devname);
	PrintDeviceName(devname);
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,szTmp);

	if ((cmnd==light2_sGroupOff)||(cmnd==light2_sGroupOn))
	{
		//set the status of all lights with the same code to on/off
		m_sql.Lighting2GroupCmd(ID,subType,(cmnd==light2_sGroupOff)?light2_sOff:light2_sOn);
	}

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->LIGHTING2.subtype)
		{
		case sTypeAC:
		case sTypeHEU:
		case sTypeANSLUT:
			switch (pResponse->LIGHTING2.subtype)
			{
			case sTypeAC:
				WriteMessage("subtype       = AC");
				break;
			case sTypeHEU:
				WriteMessage("subtype       = HomeEasy EU");
				break;
			case sTypeANSLUT:
				WriteMessage("subtype       = ANSLUT");
				break;
			}
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING2.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %s", ID.c_str());
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", Unit);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING2.cmnd)
			{
			case light2_sOff:
				WriteMessage("Off");
				break;
			case light2_sOn:
				WriteMessage("On");
				break;
			case light2_sSetLevel:
				sprintf(szTmp,"Set Level: %d", level);
				WriteMessage(szTmp);
				break;
			case light2_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light2_sGroupOn:
				WriteMessage("Group On");
				break;
			case light2_sSetGroupLevel:
				sprintf(szTmp,"Set Group Level: %d", level);
				WriteMessage(szTmp);
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING2.packettype, pResponse->LIGHTING2.subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Lighting3(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	char szTmp[100];

	switch (pResponse->LIGHTING3.subtype)
	{
	case sTypeKoppla:
		WriteMessage("subtype       = Ikea Koppla");
		sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING3.seqnbr);
		WriteMessage(szTmp);
		WriteMessage("Command       = ", false);
		switch (pResponse->LIGHTING3.cmnd)
		{
		case 0x0:
			WriteMessage("Off");
			break;
		case 0x1:
			WriteMessage("On");
			break;
		case 0x20:
			sprintf(szTmp,"Set Level: %d", pResponse->LIGHTING3.channel10_9);
			WriteMessage(szTmp);
			break;
		case 0x21:
			WriteMessage("Program");
			break;
		default:
			if ((pResponse->LIGHTING3.cmnd >= 0x10) && (pResponse->LIGHTING3.cmnd <0x18))
				WriteMessage("Dim");
			else if ((pResponse->LIGHTING3.cmnd >= 0x18) && (pResponse->LIGHTING3.cmnd < 0x20))
				WriteMessage("Bright");
			else
				WriteMessage("UNKNOWN");
			break;
		}
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING3.packettype, pResponse->LIGHTING3.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp,"Signal level  = %d", pResponse->LIGHTING3.rssi);
	WriteMessage(szTmp);
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Lighting4(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeLighting4;

	//tRBUF *pResponse=(tRBUF*)m_rxbuffer;
	char szTmp[100];

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->LIGHTING4.subtype)
		{
		case sTypePT2262:
			WriteMessage("subtype       = PT2262");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING4.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"Code          = %02X%02X%02X", pResponse->LIGHTING4.cmd1,pResponse->LIGHTING4.cmd2,pResponse->LIGHTING4.cmd3);
			WriteMessage(szTmp);

			WriteMessage("S1- S24  = ", false);
			if ((pResponse->LIGHTING4.cmd1 & 0x80)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd1 & 0x40)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd1 & 0x20)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd1 & 0x10)==0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);
			

			if ((pResponse->LIGHTING4.cmd1 & 0x08)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd1 & 0x04)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd1 & 0x02)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd1 & 0x01)==0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);
			

			if ((pResponse->LIGHTING4.cmd2 & 0x80)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd2 & 0x40)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd2 & 0x20)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd2 & 0x10)==0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);
			

			if ((pResponse->LIGHTING4.cmd2 & 0x08)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd2 & 0x04)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd2 & 0x02)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd2 & 0x01)==0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);
			

			if ((pResponse->LIGHTING4.cmd3 & 0x80)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd3 & 0x40)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd3 & 0x20)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd3 & 0x10)==0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);
			

			if ((pResponse->LIGHTING4.cmd3 & 0x08)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd3 & 0x04)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd3 & 0x02)==0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);
			
			if ((pResponse->LIGHTING4.cmd3 & 0x01)==0)
				WriteMessage("0");
			else
				WriteMessage("1");
				
			sprintf(szTmp,"Pulse         = %d usec", (pResponse->LIGHTING4.pulseHigh * 256) + pResponse->LIGHTING4.pulseLow);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING4.packettype, pResponse->LIGHTING4.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->LIGHTING4.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Lighting5(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeLighting5;
	unsigned char subType=pResponse->LIGHTING5.subtype;
	if ((subType != sTypeEMW100)&&(subType != sTypeLivolo))
		sprintf(szTmp,"%02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	else
		sprintf(szTmp,"%02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING5.unitcode;
	unsigned char cmnd=pResponse->LIGHTING5.cmnd;
	float flevel;
	if (subType==sTypeLivolo)
		flevel=(100.0f/7.0f)*float(pResponse->LIGHTING5.level);
	else
		flevel=(100.0f/31.0f)*float(pResponse->LIGHTING5.level);
	unsigned char SignalLevel=pResponse->LIGHTING5.rssi;

	bool bDoUpdate=true;
	if (subType==sTypeTRC02)
	{
		if (
			(pResponse->LIGHTING5.cmnd!=light5_sOff)&&
			(pResponse->LIGHTING5.cmnd!=light5_sOn)
			)
		{
			bDoUpdate=false;
		}
	}
	unsigned long long DevRowIdx=-1;
	if (bDoUpdate)
	{
		sprintf(szTmp,"%d",pResponse->LIGHTING5.level);
		DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,szTmp,devname);
		PrintDeviceName(devname);
		CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,szTmp);
	}

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->LIGHTING5.subtype)
		{
		case sTypeLightwaveRF:
			WriteMessage("subtype       = LightwaveRF");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sMood1:
				WriteMessage("Group Mood 1");
				break;
			case light5_sMood2:
				WriteMessage("Group Mood 2");
				break;
			case light5_sMood3:
				WriteMessage("Group Mood 3");
				break;
			case light5_sMood4:
				WriteMessage("Group Mood 4");
				break;
			case light5_sMood5:
				WriteMessage("Group Mood 5");
				break;
			case light5_sUnlock:
				WriteMessage("Unlock");
				break;
			case light5_sLock:
				WriteMessage("Lock");
				break;
			case light5_sAllLock:
				WriteMessage("All lock");
				break;
			case light5_sClose:
				WriteMessage("Close inline relay");
				break;
			case light5_sStop:
				WriteMessage("Stop inline relay");
				break;
			case light5_sOpen:
				WriteMessage("Open inline relay");
				break;
			case light5_sSetLevel:
				sprintf(szTmp,"Set dim level to: %.2f %%" , flevel);
				WriteMessage(szTmp);
				break;
			case light5_sColourPalette:
				if (pResponse->LIGHTING5.level==0)
					WriteMessage("Color Palette (Even command)");
				else
					WriteMessage("Color Palette (Odd command)");
				break;
			case light5_sColourTone:
				if (pResponse->LIGHTING5.level==0)
					WriteMessage("Color Tone (Even command)");
				else
					WriteMessage("Color Tone (Odd command)");
				break;
			case light5_sColourCycle:
				if (pResponse->LIGHTING5.level==0)
					WriteMessage("Color Cycle (Even command)");
				else
					WriteMessage("Color Cycle (Odd command)");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeEMW100:
			WriteMessage("subtype       = EMW100");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sLearn:
				WriteMessage("Learn");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeBBSB:
			WriteMessage("subtype       = BBSB new");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeRSL:
			WriteMessage("subtype       = Conrad RSL");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeLivolo:
			WriteMessage("subtype       = Livolo");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeTRC02:
			WriteMessage("subtype       = TRC02 (RGB)");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp,"Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sRGBbright:
				WriteMessage("Bright+");
				break;
			case light5_sRGBdim:
				WriteMessage("Bright-");
				break;
			case light5_sRGBcolorplus:
				WriteMessage("Color+");
				break;
			case light5_sRGBcolormin:
				WriteMessage("Color-");
				break;
			default:
				sprintf(szTmp,"Color = %d",pResponse->LIGHTING5.cmnd);
				WriteMessage(szTmp);
				break;
			}
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING5.packettype, pResponse->LIGHTING5.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->LIGHTING5.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Lighting6(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeLighting6;
	unsigned char subType=pResponse->LIGHTING6.subtype;
	sprintf(szTmp,"%02X%02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2,pResponse->LIGHTING6.groupcode);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING6.unitcode;
	unsigned char cmnd=pResponse->LIGHTING6.cmnd;
	unsigned char rfu=pResponse->LIGHTING6.seqnbr2;
	unsigned char SignalLevel=pResponse->LIGHTING6.rssi;

	sprintf(szTmp,"%d",rfu);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,szTmp,devname);
	PrintDeviceName(devname);
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,szTmp);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->LIGHTING6.subtype)
		{
		case sTypeBlyss:
			WriteMessage("subtype       = Blyss");
			sprintf(szTmp,"Sequence nbr  = %d",  pResponse->LIGHTING6.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2);
			WriteMessage(szTmp);
			sprintf(szTmp,"groupcode     = %d", pResponse->LIGHTING6.groupcode);
			WriteMessage(szTmp);
			sprintf(szTmp,"unitcode      = %d", pResponse->LIGHTING6.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING6.cmnd)
			{
			case light6_sOff:
				WriteMessage("Off");
				break;
			case light6_sOn:
				WriteMessage("On");
				break;
			case light6_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light6_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			sprintf(szTmp,"Command seqnbr= %d", pResponse->LIGHTING6.cmndseqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"seqnbr2       = %d", pResponse->LIGHTING6.seqnbr2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING6.packettype, pResponse->LIGHTING6.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->LIGHTING6.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_LimitlessLights(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	_tLimitlessLights *pLed=(_tLimitlessLights*)pResponse;

	unsigned char devType=pTypeLimitlessLights;
	unsigned char subType=pLed->subtype;
	sprintf(szTmp,"%d", 1);
	std::string ID = szTmp;
	unsigned char Unit=pLed->dunit;
	unsigned char cmnd=pLed->command;
	unsigned char value=pLed->value;

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,12,-1,cmnd,devname);
	PrintDeviceName(devname);
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,szTmp);
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Chime(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeChime;
	unsigned char subType=pResponse->CHIME.subtype;
	sprintf(szTmp,"%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->CHIME.sound;
	unsigned char cmnd=pResponse->CHIME.sound;
	unsigned char SignalLevel=pResponse->CHIME.rssi;

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,devname);
	PrintDeviceName(devname);
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,"");

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->CHIME.subtype)
		{
		case sTypeByronSX:
			WriteMessage("subtype       = Byron SX");
			sprintf(szTmp,"Sequence nbr  = %d",  pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			WriteMessage("Sound          = ", false);
			switch (pResponse->CHIME.sound)
			{
			case chime_sound0:
			case chime_sound4:
				WriteMessage("Tubular 3 notes");
				break;
			case chime_sound1:
			case chime_sound5:
				WriteMessage("Big Ben");
				break;
			case chime_sound2:
			case chime_sound6:
				WriteMessage("Tubular 3 notes");
				break;
			case chime_sound3:
			case chime_sound7:
				WriteMessage("Solo");
				break;
			default:
				WriteMessage("UNKNOWN?");
				break;
			}
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CHIME.packettype, pResponse->CHIME.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->CHIME.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}


unsigned long long MainWorker::decode_UNDECODED(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeUndecoded;

	char szTmp[100];

	WriteMessage("UNDECODED ", false);

	switch (pResponse->UNDECODED.subtype)
	{
	case sTypeUac:
		WriteMessage("AC:", false);
		break;
	case sTypeUarc:
		WriteMessage("ARC:", false);
		break;
	case sTypeUati:
		WriteMessage("ATI:", false);
		break;
	case sTypeUhideki:
		WriteMessage("HIDEKI:", false);
		break;
	case sTypeUlacrosse:
		WriteMessage("LACROSSE:", false);
		break;
	case sTypeUad:
		WriteMessage("AD:", false);
		break;
	case sTypeUmertik:
		WriteMessage("MERTIK:", false);
		break;
	case sTypeUoregon1:
		WriteMessage("OREGON1:", false);
		break;
	case sTypeUoregon2:
		WriteMessage("OREGON2:", false);
		break;
	case sTypeUoregon3:
		WriteMessage("OREGON3:", false);
		break;
	case sTypeUproguard:
		WriteMessage("PROGUARD:", false);
		break;
	case sTypeUvisonic:
		WriteMessage("VISONIC:", false);
		break;
	case sTypeUnec:
		WriteMessage("NEC:", false);
		break;
	case sTypeUfs20:
		WriteMessage("FS20:", false);
		break;
	case sTypeUblinds:
		WriteMessage("Blinds:", false);
		break;
	case sTypeUrubicson:
		WriteMessage("RUBICSON:", false);
		break;
	case sTypeUae:
		WriteMessage("AE:", false);
		break;
	case sTypeUfineoffset:
		WriteMessage("FineOffset:", false);
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->UNDECODED.packettype, pResponse->UNDECODED.subtype);
		WriteMessage(szTmp);
		break;
	}
	std::stringstream sHexDump;
	unsigned char *pRXBytes=(unsigned char*)&pResponse->UNDECODED.msg1;
	for (int i = 0; i< pResponse->UNDECODED.packetlength - 3; i++)
	{
		sHexDump << HEX(pRXBytes[i]);
	}
	WriteMessage(sHexDump.str().c_str());
	return -1;
}


unsigned long long MainWorker::decode_RecXmitMessage(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeRecXmitMessage;
	char szTmp[100];

	switch (pResponse->RXRESPONSE.subtype)
	{
	case sTypeReceiverLockError:
		WriteMessage("subtype           = Receiver lock error");
		sprintf(szTmp,"Sequence nbr      = %d", pResponse->RXRESPONSE.seqnbr);
		WriteMessage(szTmp);
		break;
	case sTypeTransmitterResponse:
		WriteMessage("subtype           = Transmitter Response");
		sprintf(szTmp,"Sequence nbr      = %d", pResponse->RXRESPONSE.seqnbr);
		WriteMessage(szTmp);

		switch (pResponse->RXRESPONSE.msg)
		{
		case 0x0:
			WriteMessage("response          = ACK, data correct transmitted");
			break;
		case 0x1:
			WriteMessage("response          = ACK, but transmit started after 6 seconds delay anyway with RF receive data detected");
			break;
		case 0x2:
			WriteMessage("response          = NAK, transmitter did not lock on the requested transmit frequency");
			break;
		case 0x3:
			WriteMessage("response          = NAK, AC address zero in id1-id4 not allowed");
			break;
		default:
			sprintf(szTmp,"ERROR: Unexpected message type= %02X", pResponse->RXRESPONSE.msg);
			WriteMessage(szTmp);
			break;
		}
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RXRESPONSE.packettype, pResponse->RXRESPONSE.subtype);
		WriteMessage(szTmp);
		break;
	}
	return -1;
}

//not in dbase yet
unsigned long long MainWorker::decode_BLINDS1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeBlinds;
	unsigned char subType=pResponse->BLINDS1.subtype;
	sprintf(szTmp,"%02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2,pResponse->BLINDS1.id3);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->BLINDS1.unitcode;
	unsigned char cmnd=pResponse->BLINDS1.cmnd;
	unsigned char SignalLevel=pResponse->BLINDS1.rssi;

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,devname);
	PrintDeviceName(devname);
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,szTmp);

	if (m_verboselevel == EVBL_ALL)
	{
		char szTmp[100];

		switch (pResponse->BLINDS1.subtype)
		{
		case sTypeBlindsT0:
			WriteMessage("subtype       = Safy / RollerTrol / Hasta new");
			break;
		case sTypeBlindsT1:
			WriteMessage("subtype       = Hasta old");
			break;
		case sTypeBlindsT2:
			WriteMessage("subtype       = A-OK RF01");
			break;
		case sTypeBlindsT3:
			WriteMessage("subtype       = A-OK AC114");
			break;
		case sTypeBlindsT4:
			WriteMessage("subtype       = RAEX");
			break;
		case sTypeBlindsT5:
			WriteMessage("subtype       = Media Mount");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X:", pResponse->BLINDS1.packettype, pResponse->BLINDS1.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Sequence nbr  = %d", pResponse->BLINDS1.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp,"id1-3         = %02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2, pResponse->BLINDS1.id3);
		WriteMessage(szTmp);

		if (pResponse->BLINDS1.unitcode == 15)
			WriteMessage("Unit          = All");
		else
		{
			sprintf(szTmp,"Unit          = %d", pResponse->BLINDS1.unitcode);
			WriteMessage(szTmp);
		}

		WriteMessage("Command       = ", false);

		switch (pResponse->BLINDS1.cmnd)
		{
		case blinds_sOpen:
			WriteMessage("Open");
			break;
		case blinds_sStop:
			WriteMessage("Stop");
			break;
		case blinds_sClose:
			WriteMessage("Close");
			break;
		case blinds_sConfirm:
			WriteMessage("Confirm");
			break;
		case blinds_sLimit:
			WriteMessage("Set Limit");
			if (pResponse->BLINDS1.subtype == sTypeBlindsT4)
				WriteMessage("Set Upper Limit");
			else
				WriteMessage("Set Limit");
			break;
		case blinds_slowerLimit:
			WriteMessage("Set Lower Limit");
			break;
		case blinds_sDeleteLimits:
			WriteMessage("Delete Limits");
			break;
		case blinds_sChangeDirection:
			WriteMessage("Change Direction");
			break;
		case blinds_sLeft:
			WriteMessage("Left");
			break;
		case blinds_sRight:
			WriteMessage("Right");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->BLINDS1.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Security1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeSecurity1;
	unsigned char subType=pResponse->SECURITY1.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=pResponse->SECURITY1.status;
	unsigned char SignalLevel=pResponse->SECURITY1.rssi;
	unsigned char BatteryLevel = get_BateryLevel(false, pResponse->SECURITY1.battery_level & 0x0F);
	if (
		(pResponse->SECURITY1.subtype == sTypeKD101)||
		(pResponse->SECURITY1.subtype == sTypeSA30)
		)
	{
		//KD101 & SA30 do not support battery low indication
		BatteryLevel=255;
	}

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,devname);
	PrintDeviceName(devname);
	CheckSceneCode(HwdID, ID.c_str(),Unit,devType,subType,cmnd,"");

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->SECURITY1.subtype)
		{
		case sTypeSecX10:
			WriteMessage("subtype       = X10 security");
			break;
		case sTypeSecX10M:
			WriteMessage("subtype       = X10 security motion");
			break;
		case sTypeSecX10R:
			WriteMessage("subtype       = X10 security remote");
			break;
		case sTypeKD101:
			WriteMessage("subtype       = KD101 smoke detector");
			break;
		case sTypePowercodeSensor:
			WriteMessage("subtype       = Visonic PowerCode sensor - primary contact");
			break;
		case sTypePowercodeMotion:
			WriteMessage("subtype       = Visonic PowerCode motion");
			break;
		case sTypeCodesecure:
			WriteMessage("subtype       = Visonic CodeSecure");
			break;
		case sTypePowercodeAux:
			WriteMessage("subtype       = Visonic PowerCode sensor - auxiliary contact");
			break;
		case sTypeMeiantech:
			WriteMessage("subtype       = Meiantech/Atlantic/Aidebao");
			break;
		case sTypeSA30:
			WriteMessage("subtype       = Alecto SA30 smoke detector");
			break;
		case sTypeDomoticzSecurity:
			WriteMessage("subtype       = Security Panel");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->SECURITY1.packettype, pResponse->SECURITY1.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->SECURITY1.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "id1-3         = %02X:%02X:%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
		WriteMessage(szTmp);

		WriteMessage("status        = ", false);

		switch (pResponse->SECURITY1.status)
		{
		case sStatusNormal:
			WriteMessage("Normal");
			break;
		case sStatusNormalDelayed:
			WriteMessage("Normal Delayed");
			break;
		case sStatusAlarm:
			WriteMessage("Alarm");
			break;
		case sStatusAlarmDelayed:
			WriteMessage("Alarm Delayed");
			break;
		case sStatusMotion:
			WriteMessage("Motion");
			break;
		case sStatusNoMotion:
			WriteMessage("No Motion");
			break;
		case sStatusPanic:
			WriteMessage("Panic");
			break;
		case sStatusPanicOff:
			WriteMessage("Panic End");
			break;
		case sStatusArmAway:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group2 or ", false);
			WriteMessage("Arm Away");
			break;
		case sStatusArmAwayDelayed:
			WriteMessage("Arm Away Delayed");
			break;
		case sStatusArmHome:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group3 or ", false);
			WriteMessage("Arm Home");
			break;
		case sStatusArmHomeDelayed:
			WriteMessage("Arm Home Delayed");
			break;
		case sStatusDisarm:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group1 or ", false);
			WriteMessage("Disarm");
			break;
		case sStatusLightOff:
			WriteMessage("Light Off");
			break;
		case sStatusLightOn:
			WriteMessage("Light On");
			break;
		case sStatusLight2Off:
			WriteMessage("Light 2 Off");
			break;
		case sStatusLight2On:
			WriteMessage("Light 2 On");
			break;
		case sStatusDark:
			WriteMessage("Dark detected");
			break;
		case sStatusLight:
			WriteMessage("Light Detected");
			break;
		case sStatusBatLow:
			WriteMessage("Battery low MS10 or XX18 sensor");
			break;
		case sStatusPairKD101:
			WriteMessage("Pair KD101");
			break;
		case sStatusNormalTamper:
			WriteMessage("Normal + Tamper");
			break;
		case sStatusNormalDelayedTamper:
			WriteMessage("Normal Delayed + Tamper");
			break;
		case sStatusAlarmTamper:
			WriteMessage("Alarm + Tamper");
			break;
		case sStatusAlarmDelayedTamper:
			WriteMessage("Alarm Delayed + Tamper");
			break;
		case sStatusMotionTamper:
			WriteMessage("Motion + Tamper");
			break;
		case sStatusNoMotionTamper:
			WriteMessage("No Motion + Tamper");
			break;
		}

		if (
			(pResponse->SECURITY1.subtype != sTypeKD101)&&		//KD101 & SA30 does not support battery low indication
			(pResponse->SECURITY1.subtype != sTypeSA30)
			)
		{
			if ((pResponse->SECURITY1.battery_level &0xF) == 0)
				WriteMessage("battery level = Low");
			else
				WriteMessage("battery level = OK");
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->SECURITY1.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Camera1(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeCamera;

	char szTmp[100];

	switch (pResponse->CAMERA1.subtype)
	{
	case sTypeNinja:
		WriteMessage("subtype       = X10 Ninja/Robocam");
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->CAMERA1.seqnbr);
		WriteMessage(szTmp);

		WriteMessage("Command       = ", false);

		switch (pResponse->CAMERA1.cmnd)
		{
		case camera_sLeft:
			WriteMessage("Left");
			break;
		case camera_sRight:
			WriteMessage("Right");
			break;
		case camera_sUp:
			WriteMessage("Up");
			break;
		case camera_sDown:
			WriteMessage("Down");
			break;
		case camera_sPosition1:
			WriteMessage("Position 1");
			break;
		case camera_sProgramPosition1:
			WriteMessage("Position 1 program");
			break;
		case camera_sPosition2:
			WriteMessage("Position 2");
			break;
		case camera_sProgramPosition2:
			WriteMessage("Position 2 program");
			break;
		case camera_sPosition3:
			WriteMessage("Position 3");
			break;
		case camera_sProgramPosition3:
			WriteMessage("Position 3 program");
			break;
		case camera_sPosition4:
			WriteMessage("Position 4");
			break;
		case camera_sProgramPosition4:
			WriteMessage("Position 4 program");
			break;
		case camera_sCenter:
			WriteMessage("Center");
			break;
		case camera_sProgramCenterPosition:
			WriteMessage("Center program");
			break;
		case camera_sSweep:
			WriteMessage("Sweep");
			break;
		case camera_sProgramSweep:
			WriteMessage("Sweep program");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		sprintf(szTmp, "Housecode     = %d", pResponse->CAMERA1.housecode);
		WriteMessage(szTmp);
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CAMERA1.packettype, pResponse->CAMERA1.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp, "Signal level  = %d", pResponse->CAMERA1.rssi);
	WriteMessage(szTmp);
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Remote(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeRemote;

	char szTmp[100];

	switch (pResponse->REMOTE.subtype)
	{
		case sTypeATI:
			WriteMessage("subtype       = ATI Remote Wonder");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("Command       = A");
				break;
			case 0x1:
				WriteMessage("Command       = B");
				break;
			case 0x2:
				WriteMessage("Command       = power");
				break;
			case 0x3:
				WriteMessage("Command       = TV");
				break;
			case 0x4:
				WriteMessage("Command       = DVD");
				break;
			case 0x5:
				WriteMessage("Command       = ?");
				break;
			case 0x6:
				WriteMessage("Command       = Guide");
				break;
			case 0x7:
				WriteMessage("Command       = Drag");
				break;
			case 0x8:
				WriteMessage("Command       = VOL+");
				break;
			case 0x9:
				WriteMessage("Command       = VOL-");
				break;
			case 0xA:
				WriteMessage("Command       = MUTE");
				break;
			case 0xB:
				WriteMessage("Command       = CHAN+");
				break;
			case 0xC:
				WriteMessage("Command       = CHAN-");
				break;
			case 0xD:
				WriteMessage("Command       = 1");
				break;
			case 0xE:
				WriteMessage("Command       = 2");
				break;
			case 0xF:
				WriteMessage("Command       = 3");
				break;
			case 0x10:
				WriteMessage("Command       = 4");
				break;
			case 0x11:
				WriteMessage("Command       = 5");
				break;
			case 0x12:
				WriteMessage("Command       = 6");
				break;
			case 0x13:
				WriteMessage("Command       = 7");
				break;
			case 0x14:
				WriteMessage("Command       = 8");
				break;
			case 0x15:
				WriteMessage("Command       = 9");
				break;
			case 0x16:
				WriteMessage("Command       = txt");
				break;
			case 0x17:
				WriteMessage("Command       = 0");
				break;
			case 0x18:
				WriteMessage("Command       = snapshot ESC");
				break;
			case 0x19:
				WriteMessage("Command       = C");
				break;
			case 0x1A:
				WriteMessage("Command       = ^");
				break;
			case 0x1B:
				WriteMessage("Command       = D");
				break;
			case 0x1C:
				WriteMessage("Command       = TV/RADIO");
				break;
			case 0x1D:
				WriteMessage("Command       = <");
				break;
			case 0x1E:
				WriteMessage("Command       = OK");
				break;
			case 0x1F:
				WriteMessage("Command       = >");
				break;
			case 0x20:
				WriteMessage("Command       = <-");
				break;
			case 0x21:
				WriteMessage("Command       = E");
				break;
			case 0x22:
				WriteMessage("Command       = v");
				break;
			case 0x23:
				WriteMessage("Command       = F");
				break;
			case 0x24:
				WriteMessage("Command       = Rewind");
				break;
			case 0x25:
				WriteMessage("Command       = Play");
				break;
			case 0x26:
				WriteMessage("Command       = Fast forward");
				break;
			case 0x27:
				WriteMessage("Command       = Record");
				break;
			case 0x28:
				WriteMessage("Command       = Stop");
				break;
			case 0x29:
				WriteMessage("Command       = Pause");
				break;
			case 0x2C:
				WriteMessage("Command       = TV");
				break;
			case 0x2D:
				WriteMessage("Command       = VCR");
				break;
			case 0x2E:
				WriteMessage("Command       = RADIO");
				break;
			case 0x2F:
				WriteMessage("Command       = TV Preview");
				break;
			case 0x30:
				WriteMessage("Command       = Channel list");
				break;
			case 0x31:
				WriteMessage("Command       = Video Desktop");
				break;
			case 0x32:
				WriteMessage("Command       = red");
				break;
			case 0x33:
				WriteMessage("Command       = green");
				break;
			case 0x34:
				WriteMessage("Command       = yellow");
				break;
			case 0x35:
				WriteMessage("Command       = blue");
				break;
			case 0x36:
				WriteMessage("Command       = rename TAB");
				break;
			case 0x37:
				WriteMessage("Command       = Acquire image");
				break;
			case 0x38:
				WriteMessage("Command       = edit image");
				break;
			case 0x39:
				WriteMessage("Command       = Full screen");
				break;
			case 0x3A:
				WriteMessage("Command       = DVD Audio");
				break;
			case 0x70:
				WriteMessage("Command       = Cursor-left");
				break;
			case 0x71:
				WriteMessage("Command       = Cursor-right");
				break;
			case 0x72:
				WriteMessage("Command       = Cursor-up");
				break;
			case 0x73:
				WriteMessage("Command       = Cursor-down");
				break;
			case 0x74:
				WriteMessage("Command       = Cursor-up-left");
				break;
			case 0x75:
				WriteMessage("Command       = Cursor-up-right");
				break;
			case 0x76:
				WriteMessage("Command       = Cursor-down-right");
				break;
			case 0x77:
				WriteMessage("Command       = Cursor-down-left");
				break;
			case 0x78:
				WriteMessage("Command       = V");
				break;
			case 0x79:
				WriteMessage("Command       = V-End");
				break;
			case 0x7C:
				WriteMessage("Command       = X");
				break;
			case 0x7D:
				WriteMessage("Command       = X-End");
				break;
			default:
				WriteMessage("Command       = unknown");
				break;
			}
			break;
		case sTypeATIplus:
			WriteMessage("subtype       = ATI Remote Wonder Plus");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);

			WriteMessage("Command       = ", false);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("A", false);
				break;
			case 0x1:
				WriteMessage("B", false);
				break;
			case 0x2:
				WriteMessage("power", false);
				break;
			case 0x3:
				WriteMessage("TV", false);
				break;
			case 0x4:
				WriteMessage("DVD", false);
				break;
			case 0x5:
				WriteMessage("?", false);
				break;
			case 0x6:
				WriteMessage("Guide", false);
				break;
			case 0x7:
				WriteMessage("Drag", false);
				break;
			case 0x8:
				WriteMessage("VOL+", false);
				break;
			case 0x9:
				WriteMessage("VOL-", false);
				break;
			case 0xA:
				WriteMessage("MUTE", false);
				break;
			case 0xB:
				WriteMessage("CHAN+", false);
				break;
			case 0xC:
				WriteMessage("CHAN-", false);
				break;
			case 0xD:
				WriteMessage("1", false);
				break;
			case 0xE:
				WriteMessage("2", false);
				break;
			case 0xF:
				WriteMessage("3", false);
				break;
			case 0x10:
				WriteMessage("4", false);
				break;
			case 0x11:
				WriteMessage("5", false);
				break;
			case 0x12:
				WriteMessage("6", false);
				break;
			case 0x13:
				WriteMessage("7", false);
				break;
			case 0x14:
				WriteMessage("8", false);
				break;
			case 0x15:
				WriteMessage("9", false);
				break;
			case 0x16:
				WriteMessage("txt", false);
				break;
			case 0x17:
				WriteMessage("0", false);
				break;
			case 0x18:
				WriteMessage("Open Setup Menu", false);
				break;
			case 0x19:
				WriteMessage("C", false);
				break;
			case 0x1A:
				WriteMessage("^", false);
				break;
			case 0x1B:
				WriteMessage("D", false);
				break;
			case 0x1C:
				WriteMessage("FM", false);
				break;
			case 0x1D:
				WriteMessage("<", false);
				break;
			case 0x1E:
				WriteMessage("OK", false);
				break;
			case 0x1F:
				WriteMessage(">", false);
				break;
			case 0x20:
				WriteMessage("Max/Restore window", false);
				break;
			case 0x21:
				WriteMessage("E", false);
				break;
			case 0x22:
				WriteMessage("v", false);
				break;
			case 0x23:
				WriteMessage("F", false);
				break;
			case 0x24:
				WriteMessage("Rewind", false);
				break;
			case 0x25:
				WriteMessage("Play", false);
				break;
			case 0x26:
				WriteMessage("Fast forward", false);
				break;
			case 0x27:
				WriteMessage("Record", false);
				break;
			case 0x28:
				WriteMessage("Stop", false);
				break;
			case 0x29:
				WriteMessage("Pause", false);
				break;
			case 0x2A:
				WriteMessage("TV2", false);
				break;
			case 0x2B:
				WriteMessage("Clock", false);
				break;
			case 0x2C:
				WriteMessage("i", false);
				break;
			case 0x2D:
				WriteMessage("ATI", false);
				break;
			case 0x2E:
				WriteMessage("RADIO", false);
				break;
			case 0x2F:
				WriteMessage("TV Preview", false);
				break;
			case 0x30:
				WriteMessage("Channel list", false);
				break;
			case 0x31:
				WriteMessage("Video Desktop", false);
				break;
			case 0x32:
				WriteMessage("red", false);
				break;
			case 0x33:
				WriteMessage("green", false);
				break;
			case 0x34:
				WriteMessage("yellow", false);
				break;
			case 0x35:
				WriteMessage("blue", false);
				break;
			case 0x36:
				WriteMessage("rename TAB", false);
				break;
			case 0x37:
				WriteMessage("Acquire image", false);
				break;
			case 0x38:
				WriteMessage("edit image", false);
				break;
			case 0x39:
				WriteMessage("Full screen", false);
				break;
			case 0x3A:
				WriteMessage("DVD Audio", false);
				break;
			case 0x70:
				WriteMessage("Cursor-left", false);
				break;
			case 0x71:
				WriteMessage("Cursor-right", false);
				break;
			case 0x72:
				WriteMessage("Cursor-up", false);
				break;
			case 0x73:
				WriteMessage("Cursor-down", false);
				break;
			case 0x74:
				WriteMessage("Cursor-up-left", false);
				break;
			case 0x75:
				WriteMessage("Cursor-up-right", false);
				break;
			case 0x76:
				WriteMessage("Cursor-down-right", false);
				break;
			case 0x77:
				WriteMessage("Cursor-down-left", false);
				break;
			case 0x78:
				WriteMessage("Left Mouse Button", false);
				break;
			case 0x79:
				WriteMessage("V-End", false);
				break;
			case 0x7C:
				WriteMessage("Right Mouse Button", false);
				break;
			case 0x7D:
				WriteMessage("X-End", false);
				break;
			default:
				WriteMessage("unknown", false);
				break;
			}
			if ((pResponse->REMOTE.toggle & 1) == 1)
				WriteMessage("  (button press = odd)");
			else
				WriteMessage("  (button press = even)");
			break;
		case sTypeATIrw2:
			WriteMessage("subtype       = ATI Remote Wonder II");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);
			WriteMessage("Command type  = ", false);

			switch (pResponse->REMOTE.cmndtype & 0x0E)
			{
			case 0x0:
				WriteMessage("PC");
				break;
			case 0x2:
				WriteMessage("AUX1");
				break;
			case 0x4:
				WriteMessage("AUX2");
				break;
			case 0x6:
				WriteMessage("AUX3");
				break;
			case 0x8:
				WriteMessage("AUX4");
				break;
			default:
				WriteMessage("unknown");
				break;
			}
			WriteMessage("Command       = ", false);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("A", false);
				break;
			case 0x1:
				WriteMessage("B", false);
				break;
			case 0x2:
				WriteMessage("power", false);
				break;
			case 0x3:
				WriteMessage("TV", false);
				break;
			case 0x4:
				WriteMessage("DVD", false);
				break;
			case 0x5:
				WriteMessage("?", false);
				break;
			case 0x7:
				WriteMessage("Drag", false);
				break;
			case 0x8:
				WriteMessage("VOL+", false);
				break;
			case 0x9:
				WriteMessage("VOL-", false);
				break;
			case 0xA:
				WriteMessage("MUTE", false);
				break;
			case 0xB:
				WriteMessage("CHAN+", false);
				break;
			case 0xC:
				WriteMessage("CHAN-", false);
				break;
			case 0xD:
				WriteMessage("1", false);
				break;
			case 0xE:
				WriteMessage("2", false);
				break;
			case 0xF:
				WriteMessage("3", false);
				break;
			case 0x10:
				WriteMessage("4", false);
				break;
			case 0x11:
				WriteMessage("5", false);
				break;
			case 0x12:
				WriteMessage("6", false);
				break;
			case 0x13:
				WriteMessage("7", false);
				break;
			case 0x14:
				WriteMessage("8", false);
				break;
			case 0x15:
				WriteMessage("9", false);
				break;
			case 0x16:
				WriteMessage("txt", false);
				break;
			case 0x17:
				WriteMessage("0", false);
				break;
			case 0x18:
				WriteMessage("Open Setup Menu", false);
				break;
			case 0x19:
				WriteMessage("C", false);
				break;
			case 0x1A:
				WriteMessage("^", false);
				break;
			case 0x1B:
				WriteMessage("D", false);
				break;
			case 0x1C:
				WriteMessage("TV/RADIO", false);
				break;
			case 0x1D:
				WriteMessage("<", false);
				break;
			case 0x1E:
				WriteMessage("OK", false);
				break;
			case 0x1F:
				WriteMessage(">", false);
				break;
			case 0x20:
				WriteMessage("Max/Restore window", false);
				break;
			case 0x21:
				WriteMessage("E", false);
				break;
			case 0x22:
				WriteMessage("v", false);
				break;
			case 0x23:
				WriteMessage("F", false);
				break;
			case 0x24:
				WriteMessage("Rewind", false);
				break;
			case 0x25:
				WriteMessage("Play", false);
				break;
			case 0x26:
				WriteMessage("Fast forward", false);
				break;
			case 0x27:
				WriteMessage("Record", false);
				break;
			case 0x28:
				WriteMessage("Stop", false);
				break;
			case 0x29:
				WriteMessage("Pause", false);
				break;
			case 0x2C:
				WriteMessage("i", false);
				break;
			case 0x2D:
				WriteMessage("ATI", false);
				break;
			case 0x3B:
				WriteMessage("PC", false);
				break;
			case 0x3C:
				WriteMessage("AUX1", false);
				break;
			case 0x3D:
				WriteMessage("AUX2", false);
				break;
			case 0x3E:
				WriteMessage("AUX3", false);
				break;
			case 0x3F:
				WriteMessage("AUX4", false);
				break;
			case 0x70:
				WriteMessage("Cursor-left", false);
				break;
			case 0x71:
				WriteMessage("Cursor-right", false);
				break;
			case 0x72:
				WriteMessage("Cursor-up", false);
				break;
			case 0x73:
				WriteMessage("Cursor-down", false);
				break;
			case 0x74:
				WriteMessage("Cursor-up-left", false);
				break;
			case 0x75:
				WriteMessage("Cursor-up-right", false);
				break;
			case 0x76:
				WriteMessage("Cursor-down-right", false);
				break;
			case 0x77:
				WriteMessage("Cursor-down-left", false);
				break;
			case 0x78:
				WriteMessage("Left Mouse Button", false);
				break;
			case 0x7C:
				WriteMessage("Right Mouse Button", false);
				break;
			default:
				WriteMessage("unknown", false);
				break;
			}
			if ((pResponse->REMOTE.toggle & 1) == 1)
				WriteMessage("  (button press = odd)");
			else
				WriteMessage("  (button press = even)");
			break;
		case sTypeMedion:
			WriteMessage("subtype       = Medion Remote");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);

			WriteMessage("Command       = ", false);

			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("Mute");
				break;
			case 0x1:
				WriteMessage("B");
				break;
			case 0x2:
				WriteMessage("power");
				break;
			case 0x3:
				WriteMessage("TV");
				break;
			case 0x4:
				WriteMessage("DVD");
				break;
			case 0x5:
				WriteMessage("Photo");
				break;
			case 0x6:
				WriteMessage("Music");
				break;
			case 0x7:
				WriteMessage("Drag");
				break;
			case 0x8:
				WriteMessage("VOL-");
				break;
			case 0x9:
				WriteMessage("VOL+");
				break;
			case 0xA:
				WriteMessage("MUTE");
				break;
			case 0xB:
				WriteMessage("CHAN+");
				break;
			case 0xC:
				WriteMessage("CHAN-");
				break;
			case 0xD:
				WriteMessage("1");
				break;
			case 0xE:
				WriteMessage("2");
				break;
			case 0xF:
				WriteMessage("3");
				break;
			case 0x10:
				WriteMessage("4");
				break;
			case 0x11:
				WriteMessage("5");
				break;
			case 0x12:
				WriteMessage("6");
				break;
			case 0x13:
				WriteMessage("7");
				break;
			case 0x14:
				WriteMessage("8");
				break;
			case 0x15:
				WriteMessage("9");
				break;
			case 0x16:
				WriteMessage("txt");
				break;
			case 0x17:
				WriteMessage("0");
				break;
			case 0x18:
				WriteMessage("snapshot ESC");
				break;
			case 0x19:
				WriteMessage("DVD MENU");
				break;
			case 0x1A:
				WriteMessage("^");
				break;
			case 0x1B:
				WriteMessage("Setup");
				break;
			case 0x1C:
				WriteMessage("TV/RADIO");
				break;
			case 0x1D:
				WriteMessage("<");
				break;
			case 0x1E:
				WriteMessage("OK");
				break;
			case 0x1F:
				WriteMessage(">");
				break;
			case 0x20:
				WriteMessage("<-");
				break;
			case 0x21:
				WriteMessage("E");
				break;
			case 0x22:
				WriteMessage("v");
				break;
			case 0x23:
				WriteMessage("F");
				break;
			case 0x24:
				WriteMessage("Rewind");
				break;
			case 0x25:
				WriteMessage("Play");
				break;
			case 0x26:
				WriteMessage("Fast forward");
				break;
			case 0x27:
				WriteMessage("Record");
				break;
			case 0x28:
				WriteMessage("Stop");
				break;
			case 0x29:
				WriteMessage("Pause");
				break;
			case 0x2C:
				WriteMessage("TV");
				break;
			case 0x2D:
				WriteMessage("VCR");
				break;
			case 0x2E:
				WriteMessage("RADIO");
				break;
			case 0x2F:
				WriteMessage("TV Preview");
				break;
			case 0x30:
				WriteMessage("Channel list");
				break;
			case 0x31:
				WriteMessage("Video Desktop");
				break;
			case 0x32:
				WriteMessage("red");
				break;
			case 0x33:
				WriteMessage("green");
				break;
			case 0x34:
				WriteMessage("yellow");
				break;
			case 0x35:
				WriteMessage("blue");
				break;
			case 0x36:
				WriteMessage("rename TAB");
				break;
			case 0x37:
				WriteMessage("Acquire image");
				break;
			case 0x38:
				WriteMessage("edit image");
				break;
			case 0x39:
				WriteMessage("Full screen");
				break;
			case 0x3A:
				WriteMessage("DVD Audio");
				break;
			case 0x70:
				WriteMessage("Cursor-left");
				break;
			case 0x71:
				WriteMessage("Cursor-right");
				break;
			case 0x72:
				WriteMessage("Cursor-up");
				break;
			case 0x73:
				WriteMessage("Cursor-down");
				break;
			case 0x74:
				WriteMessage("Cursor-up-left");
				break;
			case 0x75:
				WriteMessage("Cursor-up-right");
				break;
			case 0x76:
				WriteMessage("Cursor-down-right");
				break;
			case 0x77:
				WriteMessage("Cursor-down-left");
				break;
			case 0x78:
				WriteMessage("V");
				break;
			case 0x79:
				WriteMessage("V-End");
				break;
			case 0x7C:
				WriteMessage("X");
				break;
			case 0x7D:
				WriteMessage("X-End");
				break;
			default:
				WriteMessage("unknown");
				break;
			}
			break;
		case sTypePCremote:
			WriteMessage("subtype       = PC Remote");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x2:
				WriteMessage("0");
				break;
			case 0x82:
				WriteMessage("1");
				break;
			case 0xD1:
				WriteMessage("MP3");
				break;
			case 0x42:
				WriteMessage("2");
				break;
			case 0xD2:
				WriteMessage("DVD");
				break;
			case 0xC2:
				WriteMessage("3");
				break;
			case 0xD3:
				WriteMessage("CD");
				break;
			case 0x22:
				WriteMessage("4");
				break;
			case 0xD4:
				WriteMessage("PC or SHIFT-4");
				break;
			case 0xA2:
				WriteMessage("5");
				break;
			case 0xD5:
				WriteMessage("SHIFT-5");
				break;
			case 0x62:
				WriteMessage("6");
				break;
			case 0xE2:
				WriteMessage("7");
				break;
			case 0x12:
				WriteMessage("8");
				break;
			case 0x92:
				WriteMessage("9");
				break;
			case 0xC0:
				WriteMessage("CH-");
				break;
			case 0x40:
				WriteMessage("CH+");
				break;
			case 0xE0:
				WriteMessage("VOL-");
				break;
			case 0x60:
				WriteMessage("VOL+");
				break;
			case 0xA0:
				WriteMessage("MUTE");
				break;
			case 0x3A:
				WriteMessage("INFO");
				break;
			case 0x38:
				WriteMessage("REW");
				break;
			case 0xB8:
				WriteMessage("FF");
				break;
			case 0xB0:
				WriteMessage("PLAY");
				break;
			case 0x64:
				WriteMessage("PAUSE");
				break;
			case 0x63:
				WriteMessage("STOP");
				break;
			case 0xB6:
				WriteMessage("MENU");
				break;
			case 0xFF:
				WriteMessage("REC");
				break;
			case 0xC9:
				WriteMessage("EXIT");
				break;
			case 0xD8:
				WriteMessage("TEXT");
				break;
			case 0xD9:
				WriteMessage("SHIFT-TEXT");
				break;
			case 0xF2:
				WriteMessage("TELETEXT");
				break;
			case 0xD7:
				WriteMessage("SHIFT-TELETEXT");
				break;
			case 0xBA:
				WriteMessage("A+B");
				break;
			case 0x52:
				WriteMessage("ENT");
				break;
			case 0xD6:
				WriteMessage("SHIFT-ENT");
				break;
			case 0x70:
				WriteMessage("Cursor-left");
				break;
			case 0x71:
				WriteMessage("Cursor-right");
				break;
			case 0x72:
				WriteMessage("Cursor-up");
				break;
			case 0x73:
				WriteMessage("Cursor-down");
				break;
			case 0x74:
				WriteMessage("Cursor-up-left");
				break;
			case 0x75:
				WriteMessage("Cursor-up-right");
				break;
			case 0x76:
				WriteMessage("Cursor-down-right");
				break;
			case 0x77:
				WriteMessage("Cursor-down-left");
				break;
			case 0x78:
				WriteMessage("Left mouse");
				break;
			case 0x79:
				WriteMessage("Left mouse-End");
				break;
			case 0x7B:
				WriteMessage("Drag");
				break;
			case 0x7C:
				WriteMessage("Right mouse");
				break;
			case 0x7D:
				WriteMessage("Right mouse-End");
				break;
			default:
				WriteMessage("unknown");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->REMOTE.packettype, pResponse->REMOTE.subtype);
			WriteMessage(szTmp);
			break;
	}
	sprintf(szTmp, "Signal level  = %d", pResponse->REMOTE.rssi);
	WriteMessage(szTmp);
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Thermostat1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeThermostat1;
	unsigned char subType=pResponse->THERMOSTAT1.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->THERMOSTAT1.id1 * 256) + pResponse->THERMOSTAT1.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->THERMOSTAT1.rssi;
	unsigned char BatteryLevel = 255;

	unsigned char temp=pResponse->THERMOSTAT1.temperature;
	unsigned char set_point=pResponse->THERMOSTAT1.set_point;
	unsigned char mode=(pResponse->THERMOSTAT1.mode & 0x80);
	unsigned char status=(pResponse->THERMOSTAT1.status & 0x03);

	sprintf(szTmp,"%d;%d;%d;%d",temp,set_point,mode,status);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->THERMOSTAT1.subtype)
		{
		case sTypeDigimax:
			WriteMessage("subtype       = Digimax");
			break;
		case sTypeDigimaxShort:
			WriteMessage("subtype       = Digimax with short format");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->THERMOSTAT1.packettype, pResponse->THERMOSTAT1.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->THERMOSTAT1.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->THERMOSTAT1.id1 * 256) + pResponse->THERMOSTAT1.id2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Temperature   = %d C", pResponse->THERMOSTAT1.temperature);
		WriteMessage(szTmp);

		if (pResponse->THERMOSTAT1.subtype == sTypeDigimax)
		{
			sprintf(szTmp,"Set           = %d C", pResponse->THERMOSTAT1.set_point);
			WriteMessage(szTmp);

			if ((pResponse->THERMOSTAT1.mode & 0x80) == 0)
				WriteMessage("Mode          = heating");
			else
				WriteMessage("Mode          = Cooling");

			switch (pResponse->THERMOSTAT1.status & 0x03)
			{
			case 0:
				WriteMessage("Status        = no status available");
				break;
			case 1:
				WriteMessage("Status        = demand");
				break;
			case 2:
				WriteMessage("Status        = no demand");
				break;
			case 3:
				WriteMessage("Status        = initializing");
				break;
			}
		}

		sprintf(szTmp,"Signal level  = %d", pResponse->THERMOSTAT1.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Thermostat2(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	//decoding of this type is only implemented for use by simulate and verbose
	//HE105 receive is not implemented in the RFXtrx433 firmware
	//and RTS10 commands are received as Thermostat1 commands

	WriteMessage("");
/*
	unsigned char devType=pTypeThermostat2;

		Select Case recbuf(THERMOSTAT2.subtype)
		Case THERMOSTAT2.sTypeHE105
		WriteMessage("subtype       = HE105")
		Case THERMOSTAT2.sTypeRTS10
		WriteMessage("subtype       = RTS10/RFS10/TLX1206")
		Case Else
		WriteMessage("ERROR: Unknown Sub type for Packet type=" & Hex(recbuf(THERMOSTAT2.packettype)) & ": " & Hex(recbuf(THERMOSTAT2.subtype)))
		End Select
		WriteMessage("Sequence nbr  = " & recbuf(THERMOSTAT2.seqnbr).ToString)
		WriteMessage("unitcode      = " & recbuf(THERMOSTAT2.unitcode).ToString)
		If recbuf(THERMOSTAT2.subtype) = THERMOSTAT2.sTypeHE105 Then
		WriteMessage("switches 1- 5 = ", False)
		If (recbuf(THERMOSTAT2.unitcode) And &H10) = 0 Then
		WriteMessage("OFF ", False)
		Else
		WriteMessage("ON  ", False)
		End If
		If (recbuf(THERMOSTAT2.unitcode) And &H8) = 0 Then
		WriteMessage("OFF ", False)
		Else
		WriteMessage("ON  ", False)
		End If
		If (recbuf(THERMOSTAT2.unitcode) And &H4) = 0 Then
		WriteMessage("OFF ", False)
		Else
		WriteMessage("ON  ", False)
		End If
		If (recbuf(THERMOSTAT2.unitcode) And &H2) = 0 Then
		WriteMessage("OFF ", False)
		Else
		WriteMessage("ON  ", False)
		End If
		If (recbuf(THERMOSTAT2.unitcode) And &H1) = 0 Then
		WriteMessage("OFF ")
		Else
		WriteMessage("ON  ")
		End If
		End If
		WriteMessage("Command       = ", False)
		Select Case recbuf(THERMOSTAT2.cmnd)
		Case THERMOSTAT2.sOff
		WriteMessage("Off")
		Case THERMOSTAT2.sOn
		WriteMessage("On")
		Case THERMOSTAT2.sProgram
		WriteMessage("Program RTS10")
		Case Else
		WriteMessage("UNKNOWN")
		End Select
		WriteMessage("Signal level  = " & (recbuf(THERMOSTAT2.rssi) >> 4).ToString)
*/
	WriteMessage("Not implemented");
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Thermostat3(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeThermostat3;

	char szTmp[100];

	switch (pResponse->THERMOSTAT3.subtype)
	{
	case sTypeMertikG6RH4T1:
		WriteMessage("subtype       = Mertik G6R-H4T1");
		break;
	case sTypeMertikG6RH4TB:
		WriteMessage("subtype       = Mertik G6R-H4TB");
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->THERMOSTAT3.packettype, pResponse->THERMOSTAT3.subtype);
		WriteMessage(szTmp);
		break;
	}

	sprintf(szTmp,"Sequence nbr  = %d", pResponse->THERMOSTAT3.seqnbr);
	WriteMessage(szTmp);

	sprintf(szTmp, "ID            = 0x%02X%02X%02X", pResponse->THERMOSTAT3.unitcode1,pResponse->THERMOSTAT3.unitcode2,pResponse->THERMOSTAT3.unitcode3);
	WriteMessage(szTmp);

	switch (pResponse->THERMOSTAT3.cmnd)
	{
	case 0:
		WriteMessage("Command       = Off");
		break;
	case 1:
		WriteMessage("Command       = On");
		break;
	case 2:
		WriteMessage("Command       = Up");
		break;
	case 3:
		WriteMessage("Command       = Down");
		break;
	case 4:
		if (pResponse->THERMOSTAT3.subtype == sTypeMertikG6RH4T1)
			WriteMessage("Command       = Run Up");
		else
			WriteMessage("Command       = 2nd Off");
		break;
	case 5:
		if (pResponse->THERMOSTAT3.subtype == sTypeMertikG6RH4T1)
			WriteMessage("Command       = Run Down");
		else
			WriteMessage("Command       = 2nd On");
		break;
	case 6:
		if (pResponse->THERMOSTAT3.subtype == sTypeMertikG6RH4T1)
			WriteMessage("Command       = Stop");
		else
			WriteMessage("Command       = unknown");
	default:
		WriteMessage("Command       = unknown");
		break;
	}

	sprintf(szTmp,"Signal level  = %d", pResponse->THERMOSTAT3.rssi);
	WriteMessage(szTmp);
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Baro(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeBARO;

	WriteMessage("Not implemented");
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_DateTime(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeDT;

	char szTmp[100];

	switch (pResponse->DT.subtype)
	{
	case sTypeDT1:
		WriteMessage("Subtype       = DT1 - RTGR328N");
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->DT.packettype, pResponse->DT.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp,"Sequence nbr  = %d", pResponse->DT.seqnbr);
	WriteMessage(szTmp);
	sprintf(szTmp,"ID            = %d", (pResponse->DT.id1 * 256) + pResponse->DT.id2);
	WriteMessage(szTmp);

	WriteMessage("Day of week   = ", false);

	switch (pResponse->DT.dow)
	{
	case 1:
		WriteMessage(" Sunday");
		break;
	case 2:
		WriteMessage(" Monday");
		break;
	case 3:
		WriteMessage(" Tuesday");
		break;
	case 4:
		WriteMessage(" Wednesday");
		break;
	case 5:
		WriteMessage(" Thursday");
		break;
	case 6:
		WriteMessage(" Friday");
		break;
	case 7:
		WriteMessage(" Saturday");
		break;
	}
	sprintf(szTmp,"Date yy/mm/dd = %02d/%02d/%02d", pResponse->DT.yy, pResponse->DT.mm, pResponse->DT.dd);
	WriteMessage(szTmp);
	sprintf(szTmp,"Time          = %02d:%02d:%02d", pResponse->DT.hr, pResponse->DT.min, pResponse->DT.sec);
	WriteMessage(szTmp);
	sprintf(szTmp,"Signal level  = %d", pResponse->DT.rssi);
	WriteMessage(szTmp);
	if ((pResponse->DT.battery_level & 0x0F)==0)
		WriteMessage("Battery       = Low");
	else
		WriteMessage("Battery       = OK");
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Current(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeCURRENT;
	unsigned char subType=pResponse->CURRENT.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->CURRENT.id1 * 256) + pResponse->CURRENT.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->CURRENT.rssi;
	unsigned char BatteryLevel = get_BateryLevel(false, pResponse->CURRENT.battery_level & 0x0F);

	float CurrentChannel1= float((pResponse->CURRENT.ch1h * 256) + pResponse->CURRENT.ch1l) / 10.0f;
	float CurrentChannel2= float((pResponse->CURRENT.ch2h * 256) + pResponse->CURRENT.ch2l) / 10.0f;
	float CurrentChannel3= float((pResponse->CURRENT.ch3h * 256) + pResponse->CURRENT.ch3l) / 10.0f;
	sprintf(szTmp,"%.1f;%.1f;%.1f",CurrentChannel1,CurrentChannel2,CurrentChannel3);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleAmpere123Notification(HwdID, ID, Unit, devType, subType, CurrentChannel1, CurrentChannel2, CurrentChannel3);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->CURRENT.subtype)
		{
		case sTypeELEC1:
			WriteMessage("subtype       = ELEC1 - OWL CM113, Electrisave, cent-a-meter");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CURRENT.packettype, pResponse->CURRENT.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->CURRENT.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->CURRENT.id1 * 256) + pResponse->CURRENT.id2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Count         = %d", pResponse->CURRENT.id2);//m_rxbuffer[5]);
		WriteMessage(szTmp);
		sprintf(szTmp,"Channel 1     = %.1f ampere", CurrentChannel1);
		WriteMessage(szTmp);
		sprintf(szTmp,"Channel 2     = %.1f ampere", CurrentChannel2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Channel 3     = %.1f ampere", CurrentChannel3);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->CURRENT.rssi);
		WriteMessage(szTmp);

		if ((pResponse->CURRENT.battery_level & 0xF) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Energy(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeENERGY;
	unsigned char subType=pResponse->ENERGY.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->ENERGY.id1 * 256) + pResponse->ENERGY.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->ENERGY.rssi;
	unsigned char BatteryLevel = get_BateryLevel(false, pResponse->ENERGY.battery_level & 0x0F);

	long instant;
	double usage;

	instant = (pResponse->ENERGY.instant1 * 0x1000000) + (pResponse->ENERGY.instant2 * 0x10000) + (pResponse->ENERGY.instant3 * 0x100) + pResponse->ENERGY.instant4;

	usage = (
				double(pResponse->ENERGY.total1) * 0x10000000000ULL + 
				double(pResponse->ENERGY.total2) * 0x100000000ULL + 
				double(pResponse->ENERGY.total3) * 0x1000000 +
				double(pResponse->ENERGY.total4) * 0x10000 + 
				double(pResponse->ENERGY.total5) * 0x100 + 
				double(pResponse->ENERGY.total6)
			) / 223.666;

	sprintf(szTmp,"%ld;%.2f",instant,usage);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (const float)instant);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->ENERGY.subtype)
		{
		case sTypeELEC2:
			WriteMessage("subtype       = ELEC2 - OWL CM119, CM160");
			break;
		case sTypeELEC3:
			WriteMessage("subtype       = ELEC2 - OWL CM180");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->ENERGY.packettype, pResponse->ENERGY.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->ENERGY.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->ENERGY.id1 * 256) + pResponse->ENERGY.id2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Count         = %d", pResponse->ENERGY.count);
		WriteMessage(szTmp);
		sprintf(szTmp,"Instant usage = %ld Watt", instant);
		WriteMessage(szTmp);
		sprintf(szTmp,"total usage   = %.2f Wh", usage);
		WriteMessage(szTmp);

		sprintf(szTmp,"Signal level  = %d", pResponse->ENERGY.rssi);
		WriteMessage(szTmp);

		if ((pResponse->ENERGY.battery_level & 0xF) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Power(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypePOWER;
	unsigned char subType=pResponse->POWER.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->POWER.id1 * 256) + pResponse->POWER.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->POWER.rssi;
	unsigned char BatteryLevel = 255;

	int Voltage=pResponse->POWER.voltage;
	double current= ((pResponse->POWER.currentH * 256) + pResponse->POWER.currentL) / 100.0;
	double instant= ((pResponse->POWER.powerH * 256) + pResponse->POWER.powerL) / 10.0;// Watt
	double usage= ((pResponse->POWER.energyH * 256) + pResponse->POWER.energyL) / 100.0; //kWh
	double powerfactor = pResponse->POWER.pf / 100.0;
	int frequency = pResponse->POWER.freq; //Hz

	sprintf(szTmp,"%ld;%.2f",long(round(instant)),usage*1000.0);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (const float)instant);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->POWER.subtype)
		{
		case sTypeELEC5:
			WriteMessage("subtype       = ELEC5 - Revolt");
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->POWER.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->POWER.id1 * 256) + pResponse->POWER.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Voltage       = %d Volt", Voltage);
		WriteMessage(szTmp);
		sprintf(szTmp,"Current       = %.2f Ampere",current);
		WriteMessage(szTmp);

		sprintf(szTmp,"Instant usage = %.2f Watt", instant);
		WriteMessage(szTmp);
		sprintf(szTmp,"total usage   = %.2f kWh", usage);
		WriteMessage(szTmp);

		sprintf(szTmp,"Power factor  = %.2f", powerfactor);
		WriteMessage(szTmp);
		sprintf(szTmp,"Frequency     = %d Hz", frequency);
		WriteMessage(szTmp);

		sprintf(szTmp,"Signal level  = %d", pResponse->POWER.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Current_Energy(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeCURRENTENERGY;
	unsigned char subType=pResponse->CURRENT_ENERGY.subtype;
	std::string ID;
	sprintf(szTmp,"%d",(pResponse->CURRENT_ENERGY.id1 * 256) + pResponse->CURRENT_ENERGY.id2);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->CURRENT_ENERGY.rssi;
	unsigned char BatteryLevel = get_BateryLevel(false, pResponse->CURRENT_ENERGY.battery_level & 0x0F);

	float CurrentChannel1= float((pResponse->CURRENT_ENERGY.ch1h * 256) + pResponse->CURRENT_ENERGY.ch1l) / 10.0f;
	float CurrentChannel2= float((pResponse->CURRENT_ENERGY.ch2h * 256) + pResponse->CURRENT_ENERGY.ch2l) / 10.0f;
	float CurrentChannel3= float((pResponse->CURRENT_ENERGY.ch3h * 256) + pResponse->CURRENT_ENERGY.ch3l) / 10.0f;

	double usage=0;

	if (pResponse->CURRENT_ENERGY.count!=0)
	{
		//no usage provided, get the last usage
		std::vector<std::vector<std::string> > result2;
		std::stringstream szQuery2;
		szQuery2 << "SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=" << HwdID << " AND DeviceID='" << ID << "' AND Unit=" << int(Unit) << " AND Type=" << int(devType) << " AND SubType=" << int(subType) << ")";
		result2=m_sql.query(szQuery2.str());
		if (result2.size()>0)
		{
			std::vector<std::string> strarray;
			StringSplit(result2[0][1], ";", strarray);
			if (strarray.size()==4)
			{
				usage = atof(strarray[3].c_str());
			}
		}
	}
	else
	{
		usage = (
			double(pResponse->CURRENT_ENERGY.total1) * 0x10000000000ULL +
			double(pResponse->CURRENT_ENERGY.total2) * 0x100000000ULL +
			double(pResponse->CURRENT_ENERGY.total3) * 0x1000000 +
			double(pResponse->CURRENT_ENERGY.total4) * 0x10000 +
			double(pResponse->CURRENT_ENERGY.total5) * 0x100 +
			pResponse->CURRENT_ENERGY.total6
			) / 223.666;

		if (usage==0)
		{
			//That should not be, let's get the previous value
			//no usage provided, get the last usage
			std::vector<std::vector<std::string> > result2;
			std::stringstream szQuery2;
			szQuery2 << "SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=" << HwdID << " AND DeviceID='" << ID << "' AND Unit=" << int(Unit) << " AND Type=" << int(devType) << " AND SubType=" << int(subType) << ")";
			result2=m_sql.query(szQuery2.str());
			if (result2.size()>0)
			{
				std::vector<std::string> strarray;
				StringSplit(result2[0][1], ";", strarray);
				if (strarray.size()==4)
				{
					usage = atof(strarray[3].c_str());
				}
			}
		}

		int voltage=230;
		m_sql.GetPreferencesVar("ElectricVoltage", voltage);

		sprintf(szTmp,"%ld;%.2f",(long)round((CurrentChannel1+CurrentChannel2+CurrentChannel3)*voltage),usage);
		m_sql.UpdateValue(HwdID, ID.c_str(),Unit,pTypeENERGY,sTypeELEC3,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	}
	sprintf(szTmp,"%.1f;%.1f;%.1f;%.3f",CurrentChannel1,CurrentChannel2,CurrentChannel3,usage);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleAmpere123Notification(HwdID, ID, Unit, devType, subType, CurrentChannel1, CurrentChannel2, CurrentChannel3);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->CURRENT_ENERGY.subtype)
		{
		case sTypeELEC4:
			WriteMessage("subtype       = ELEC4 - OWL CM180i");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CURRENT_ENERGY.packettype, pResponse->CURRENT_ENERGY.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->CURRENT_ENERGY.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->CURRENT_ENERGY.id1 * 256) + pResponse->CURRENT_ENERGY.id2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Count         = %d", pResponse->CURRENT_ENERGY.count);
		WriteMessage(szTmp);
		float ampereChannel1,ampereChannel2,ampereChannel3;
		ampereChannel1=float((pResponse->CURRENT_ENERGY.ch1h * 256) + pResponse->CURRENT_ENERGY.ch1l) / 10.0f;
		ampereChannel2=float((pResponse->CURRENT_ENERGY.ch2h * 256) + pResponse->CURRENT_ENERGY.ch2l) / 10.0f;
		ampereChannel3=float((pResponse->CURRENT_ENERGY.ch3h * 256) + pResponse->CURRENT_ENERGY.ch3l) / 10.0f;
		sprintf(szTmp,"Channel 1     = %.1f ampere", ampereChannel1);
		WriteMessage(szTmp);
		sprintf(szTmp,"Channel 2     = %.1f ampere", ampereChannel2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Channel 3     = %.1f ampere", ampereChannel3);
		WriteMessage(szTmp);

		if (pResponse->CURRENT_ENERGY.count == 0)
		{
			sprintf(szTmp,"total usage   = %.3f Wh", usage);
			WriteMessage(szTmp);
		}

		sprintf(szTmp,"Signal level  = %d", pResponse->CURRENT_ENERGY.rssi);
		WriteMessage(szTmp);

		if ((pResponse->CURRENT_ENERGY.battery_level & 0xF) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Gas(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeGAS;

	WriteMessage("Not implemented");
	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_Water(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	WriteMessage("");

	unsigned char devType=pTypeWATER;

	WriteMessage("Not implemented");
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Weight(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeWEIGHT;
	unsigned char subType=pResponse->WEIGHT.subtype;
	unsigned short weightID=(pResponse->WEIGHT.id1*256) + pResponse->WEIGHT.id2;
	std::string ID;
	sprintf(szTmp, "%d", weightID);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->WEIGHT.rssi;
	unsigned char BatteryLevel = 255;
	float weight=(float(pResponse->WEIGHT.weighthigh) * 25.6f) + (float(pResponse->WEIGHT.weightlow) / 10.0f);

	float AddjValue=0.0f;
	float AddjMulti=1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	weight+=AddjValue;

	sprintf(szTmp,"%.1f",weight);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->WEIGHT.subtype)
		{
		case sTypeWEIGHT1:
			WriteMessage("subtype       = BWR102");
			break;
		case sTypeWEIGHT2:
			WriteMessage("subtype       = GR101");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type = %02X:%02X", pResponse->WEIGHT.packettype, pResponse->WEIGHT.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->WEIGHT.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->WEIGHT.id1 * 256) + pResponse->WEIGHT.id2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Weight        = %.1f kg", (float(pResponse->WEIGHT.weighthigh) * 25.6f) + (float(pResponse->WEIGHT.weightlow) / 10));
		WriteMessage(szTmp);
		sprintf(szTmp, "Signal level  = %d", pResponse->WEIGHT.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_RFXSensor(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeRFXSensor;
	unsigned char subType=pResponse->RFXSENSOR.subtype;
	std::string ID;
	sprintf(szTmp, "%d", pResponse->RFXSENSOR.id);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->RFXSENSOR.rssi;
	unsigned char BatteryLevel = 255;

	switch (pResponse->RFXSENSOR.subtype)
	{
	case sTypeRFXSensorTemp:
		{
			float temp;
			if ((pResponse->RFXSENSOR.msg1 & 0x80) == 0) //positive temperature?
				temp = float( (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2) / 100.0f;
			else
				temp=-(float( ((pResponse->RFXSENSOR.msg1 & 0x7F) * 256) + pResponse->RFXSENSOR.msg2) / 100.0f);
			float AddjValue=0.0f;
			float AddjMulti=1.0f;
			m_sql.GetAddjustment(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
			temp+=AddjValue;

			sprintf(szTmp,"%.1f",temp);
			m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, temp, 0, true, false);
		}
		break;
	case sTypeRFXSensorAD:
	case sTypeRFXSensorVolt:
		{
			int volt=(pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2;
			sprintf(szTmp,"%d",volt);
			m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, float(volt));

		}
		break;
	}
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->RFXSENSOR.subtype)
		{
		case sTypeRFXSensorTemp:
			WriteMessage("subtype       = Temperature");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);

			if ((pResponse->RFXSENSOR.msg1 & 0x80) == 0) //positive temperature?
			{
				sprintf(szTmp,"Temperature   = %.2f C", float( (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2) / 100.0f);
				WriteMessage(szTmp);
			}
			else
			{
				sprintf(szTmp,"Temperature   = -%.2f C", float( ((pResponse->RFXSENSOR.msg1 & 0x7F) * 256) + pResponse->RFXSENSOR.msg2) / 100.0f);
				WriteMessage(szTmp);
			}
			break;
		case sTypeRFXSensorAD:
			WriteMessage("subtype       = A/D");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);
			sprintf(szTmp,"volt          = %d mV", (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXSensorVolt:
			WriteMessage("subtype       = Voltage");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);
			sprintf(szTmp,"volt          = %d mV", (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXSensorMessage:
			WriteMessage("subtype       = Message");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);
			switch (pResponse->RFXSENSOR.msg2)
			{
			case 0x1:
				WriteMessage("msg           = sensor addresses incremented");
				break;
			case 0x2:
				WriteMessage("msg           = battery low detected");
				break;
			case 0x81:
				WriteMessage("msg           = no 1-wire device connected");
				break;
			case 0x82:
				WriteMessage("msg           = 1-Wire ROM CRC error");
				break;
			case 0x83:
				WriteMessage("msg           = 1-Wire device connected is not a DS18B20 or DS2438");
				break;
			case 0x84:
				WriteMessage("msg           = no end of read signal received from 1-Wire device");
				break;
			case 0x85:
				WriteMessage("msg           = 1-Wire scratch pad CRC error");
				break;
			default:
				WriteMessage("ERROR: unknown message");
				break;
			}
			sprintf(szTmp,"msg           = %d", (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RFXSENSOR.packettype, pResponse->RFXSENSOR.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->RFXSENSOR.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_RFXMeter(const int HwdID, const tRBUF *pResponse)
{
	unsigned long long DevRowIdx=-1;
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeRFXMeter;
	unsigned char subType=pResponse->RFXMETER.subtype;
	if (subType==sTypeRFXMeterCount)
	{
		std::string ID;
		sprintf(szTmp,"%d",(pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
		ID=szTmp;
		unsigned char Unit=0;
		unsigned char cmnd=0;
		unsigned char SignalLevel=pResponse->RFXMETER.rssi;
		unsigned char BatteryLevel = 255;

		unsigned long counter = (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
		//float RFXPwr = float(counter) / 1000.0f;

		sprintf(szTmp,"%lu",counter);
		DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
		PrintDeviceName(devname);
	}

	if (m_verboselevel == EVBL_ALL)
	{
		unsigned long counter;

		switch (pResponse->RFXMETER.subtype)
		{
		case sTypeRFXMeterCount:
			WriteMessage("subtype       = RFXMeter counter");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			counter = (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
			sprintf(szTmp,"Counter       = %lu", counter);
			WriteMessage(szTmp);
			sprintf(szTmp,"if RFXPwr     = %.3f kWh", float(counter) / 1000.0f);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterInterval:
			WriteMessage("subtype       = RFXMeter new interval time set");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			WriteMessage("Interval time = ", false);

			switch(pResponse->RFXMETER.count3)
			{
			case 0x1:
				WriteMessage("30 seconds");
				break;
			case 0x2:
				WriteMessage("1 minute");
				break;
			case 0x4:
				WriteMessage("6 minutes");
				break;
			case 0x8:
				WriteMessage("12 minutes");
				break;
			case 0x10:
				WriteMessage("15 minutes");
				break;
			case 0x20:
				WriteMessage("30 minutes");
				break;
			case 0x40:
				WriteMessage("45 minutes");
				break;
			case 0x80:
				WriteMessage("1 hour");
				break;
			}
			break;
		case sTypeRFXMeterCalib:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Calibrate mode for channel 1");
				break;
			case 0x40:
				WriteMessage("subtype       = Calibrate mode for channel 2");
				break;
			case 0x80:
				WriteMessage("subtype       = Calibrate mode for channel 3");
				break;
			}
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			counter = ( ((pResponse->RFXMETER.count2 & 0x3F) << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4 ) / 1000;
			sprintf(szTmp,"Calibrate cnt = %lu msec", counter);
			WriteMessage(szTmp);

			sprintf(szTmp,"RFXPwr        = %.3f kW", 1.0f / ( float(16 * counter) / (3600000.0f / 62.5f)) );
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterAddr:
			WriteMessage("subtype       = New address set, push button for next address");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterCounterReset:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else RESET COUNTER channel 1 will be executed");
				break;
			case 0x40:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else RESET COUNTER channel 2 will be executed");
				break;
			case 0x80:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else RESET COUNTER channel 3 will be executed");
				break;
			}
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterCounterSet:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Counter channel 1 is reset to zero");
				break;
			case 0x40:
				WriteMessage("subtype       = Counter channel 2 is reset to zero");
				break;
			case 0x80:
				WriteMessage("subtype       = Counter channel 3 is reset to zero");
				break;
			}
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			counter =  (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
			sprintf(szTmp,"Counter       = %lu", counter);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterSetInterval:
			WriteMessage("subtype       = Push the button for next mode within 5 seconds or else SET INTERVAL MODE will be entered");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterSetCalib:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else CALIBRATION mode for channel 1 will be executed");
				break;
			case 0x40:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else CALIBRATION mode for channel 2 will be executed");
				break;
			case 0x80:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else CALIBRATION mode for channel 3 will be executed");
				break;
			}
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterSetAddr:
			WriteMessage("subtype       = Push the button for next mode within 5 seconds or else SET ADDRESS MODE will be entered");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterIdent:
			WriteMessage("subtype       = RFXMeter identification");
			sprintf(szTmp,"Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp,"ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			sprintf(szTmp,"FW version    = %02X", pResponse->RFXMETER.count3);
			WriteMessage(szTmp);
			WriteMessage("Interval time = ", false);

			switch (pResponse->RFXMETER.count4)
			{
			case 0x1:
				WriteMessage("30 seconds");
				break;
			case 0x2:
				WriteMessage("1 minute");
				break;
			case 0x4:
				WriteMessage("6 minutes");
				break;
			case 0x8:
				WriteMessage("12 minutes");
				break;
			case 0x10:
				WriteMessage("15 minutes");
				break;
			case 0x20:
				WriteMessage("30 minutes");
				break;
			case 0x40:
				WriteMessage("45 minutes");
				break;
			case 0x80:
				WriteMessage("1 hour");
				break;
			}
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RFXMETER.packettype, pResponse->RFXMETER.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->RFXMETER.rssi);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_P1MeterPower(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tP1Power *p1Power=(const _tP1Power*)pResponse;
	unsigned char devType=p1Power->type;
	unsigned char subType=p1Power->subtype;
	std::string ID="1";
	unsigned char Unit=subType;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp,"%lu;%lu;%lu;%lu;%lu;%lu",
		p1Power->powerusage1,
		p1Power->powerusage2,
		p1Power->powerdeliv1,
		p1Power->powerdeliv2,
		p1Power->usagecurrent,
		p1Power->delivcurrent
		);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (const float)p1Power->usagecurrent);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (p1Power->subtype)
		{
		case sTypeP1Power:
			WriteMessage("subtype       = P1 Smart Meter Power");

			sprintf(szTmp,"powerusage1 = %.3f kWh", float(p1Power->powerusage1) / 1000.0f);
			WriteMessage(szTmp);
			sprintf(szTmp,"powerusage2 = %.3f kWh", float(p1Power->powerusage2) / 1000.0f);
			WriteMessage(szTmp);

			sprintf(szTmp,"powerdeliv1 = %.3f kWh", float(p1Power->powerdeliv1) / 1000.0f);
			WriteMessage(szTmp);
			sprintf(szTmp,"powerdeliv2 = %.3f kWh", float(p1Power->powerdeliv2) / 1000.0f);
			WriteMessage(szTmp);

			sprintf(szTmp,"current usage = %03lu Watt", p1Power->usagecurrent);
			WriteMessage(szTmp);
			sprintf(szTmp,"current deliv = %03lu Watt", p1Power->delivcurrent);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", p1Power->type, p1Power->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_P1MeterGas(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tP1Gas *p1Gas=(const _tP1Gas*)pResponse;
	unsigned char devType=p1Gas->type;
	unsigned char subType=p1Gas->subtype;
	std::string ID="1";
	unsigned char Unit=subType;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp,"%lu",p1Gas->gasusage);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (p1Gas->subtype)
		{
		case sTypeP1Gas:
			WriteMessage("subtype       = P1 Smart Meter Gas");

			sprintf(szTmp,"gasusage = %.3f m3", float(p1Gas->gasusage) / 1000.0f);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", p1Gas->type, p1Gas->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_YouLessMeter(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tYouLessMeter *pMeter=(const _tYouLessMeter*)pResponse;
	unsigned char devType=pMeter->type;
	unsigned char subType=pMeter->subtype;
	sprintf(szTmp,"%d",pMeter->ID1);
	std::string ID=szTmp;
	unsigned char Unit=subType;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp,"%lu;%lu",
		pMeter->powerusage,
		pMeter->usagecurrent
		);
	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (const float)pMeter->usagecurrent);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pMeter->subtype)
		{
		case sTypeYouLess:
			WriteMessage("subtype       = YouLess Meter");

			sprintf(szTmp,"powerusage = %.3f kWh", float(pMeter->powerusage) / 1000.0f);
			WriteMessage(szTmp);
			sprintf(szTmp,"current usage = %03lu kWh", pMeter->usagecurrent);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Rego6XXTemp(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;
	const _tRego6XXTemp *pRego=(const _tRego6XXTemp*)pResponse;
	unsigned char devType=pRego->type;
	unsigned char subType=pRego->subtype;
	std::string ID=pRego->ID;
	unsigned char Unit=subType;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

    sprintf(szTmp,"%.1f",
		pRego->temperature
	);
    unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp, devname);
	PrintDeviceName(devname);
    m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_TEMPERATURE, pRego->temperature);
 
	if (m_verboselevel == EVBL_ALL)
	{
		WriteMessage("subtype       = Rego6XX Temp");

		sprintf(szTmp,"Temp = %.1f", pRego->temperature);
		WriteMessage(szTmp);
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Rego6XXValue(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;
	const _tRego6XXStatus *pRego=(const _tRego6XXStatus*)pResponse;
	unsigned char devType=pRego->type;
	unsigned char subType=pRego->subtype;
	std::string ID=pRego->ID;
	unsigned char Unit = subType;
	int numValue = pRego->value;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

    sprintf(szTmp,"%d",
		pRego->value
	);

    unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,numValue,szTmp, devname);
	PrintDeviceName(devname);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pRego->subtype)
		{
		case sTypeRego6XXStatus:
    		WriteMessage("subtype       = Rego6XX Status");
            sprintf(szTmp,"Status = %d", pRego->value);
		    WriteMessage(szTmp);
            break;
		case sTypeRego6XXCounter:
    		WriteMessage("subtype       = Rego6XX Counter");
            sprintf(szTmp,"Counter = %d", pRego->value);
		    WriteMessage(szTmp);
            break;
        }
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_AirQuality(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tAirQualityMeter *pMeter=(const _tAirQualityMeter*)pResponse;
	unsigned char devType=pMeter->type;
	unsigned char subType=pMeter->subtype;
	sprintf(szTmp,"%d",pMeter->id1);
	std::string ID=szTmp;
	unsigned char Unit=subType;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,pMeter->airquality,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (const float)pMeter->airquality);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pMeter->subtype)
		{
		case sTypeVoltcraft:
			WriteMessage("subtype       = Voltcraft CO-20");

			sprintf(szTmp,"CO2 = %d ppm", pMeter->airquality);
			WriteMessage(szTmp);
			if (pMeter->airquality<700)
				strcpy(szTmp,"Quality = Excellent");
			else if (pMeter->airquality<900)
				strcpy(szTmp,"Quality = Good");
			else if (pMeter->airquality<1100)
				strcpy(szTmp,"Quality = Fair");
			else if (pMeter->airquality<1600)
				strcpy(szTmp,"Quality = Mediocre");
			else
				strcpy(szTmp,"Quality = Bad");
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Usage(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tUsageMeter *pMeter=(const _tUsageMeter*)pResponse;
	unsigned char devType=pMeter->type;
	unsigned char subType=pMeter->subtype;
	sprintf(szTmp,"%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID=szTmp;
	unsigned char Unit=pMeter->dunit;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp,"%.1f",pMeter->fusage);

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, pMeter->fusage);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pMeter->subtype)
		{
		case sTypeElectric:
			WriteMessage("subtype       = Electric");

			sprintf(szTmp,"Usage = %.1f W", pMeter->fusage);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Lux(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tLightMeter *pMeter=(const _tLightMeter*)pResponse;
	unsigned char devType=pMeter->type;
	unsigned char subType=pMeter->subtype;
	sprintf(szTmp,"%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID=szTmp;
	unsigned char Unit=pMeter->dunit;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = pMeter->battery_level;

	sprintf(szTmp,"%.0f",pMeter->fLux);

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, pMeter->fLux);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pMeter->subtype)
		{
		case sTypeLux:
			WriteMessage("subtype       = Lux");

			sprintf(szTmp,"Lux = %.1f W", pMeter->fLux);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_Thermostat(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tThermostat *pMeter=(const _tThermostat*)pResponse;
	unsigned char devType=pMeter->type;
	unsigned char subType=pMeter->subtype;
	sprintf(szTmp,"%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID=szTmp;
	unsigned char Unit=pMeter->dunit;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = pMeter->battery_level;

	switch (pMeter->subtype)
	{
	case sTypeThermSetpoint:
		sprintf(szTmp,"%.1f",pMeter->temp);
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
		WriteMessage(szTmp);
		return -1;
	}

	unsigned long long DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);

	if (pMeter->subtype==sTypeThermSetpoint)
	{
		m_sql.CheckAndHandleTempHumidityNotification(HwdID, ID, Unit, devType, subType, pMeter->temp, 0, true, false);
	}

	//m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, pMeter->fLux);

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pMeter->subtype)
		{
		case sTypeThermSetpoint:
			WriteMessage("subtype       = SetPoint");

			sprintf(szTmp,"Temp = %.1f", pMeter->temp);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

unsigned long long MainWorker::decode_General(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[200];
	std::string devname;

	const _tGeneralDevice *pMeter=(const _tGeneralDevice*)pResponse;
	unsigned char devType=pMeter->type;
	unsigned char subType=pMeter->subtype;
	sprintf(szTmp,"%d", pMeter->id);
	std::string ID=szTmp;
	unsigned char Unit=1;
	unsigned char cmnd=0;
	unsigned char SignalLevel=12;
	unsigned char BatteryLevel = 255;

	unsigned long long DevRowIdx=0;

	if (subType==sTypeVisibility)
	{
		sprintf(szTmp,"%.1f",pMeter->floatval1);
		DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
		PrintDeviceName(devname);
		int meterType=0;
		m_sql.GetMeterType(HwdID, ID.c_str(),Unit,devType,subType,meterType);
		float fValue=pMeter->floatval1;
		if (meterType==1)
		{
			//miles
			fValue*=0.6214f;//convert to miles
		}
		m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, fValue);
	}
	else if (subType==sTypeSolarRadiation)
	{
		sprintf(szTmp,"%.1f",pMeter->floatval1);
		DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
		PrintDeviceName(devname);
		m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType==sTypeSoilMoisture)
	{
		DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,pMeter->intval1,devname);
		PrintDeviceName(devname);
		m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (float)pMeter->intval1);
	}
	else if (subType==sTypeLeafWetness)
	{
		DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,pMeter->intval1,devname);
		PrintDeviceName(devname);
		m_sql.CheckAndHandleNotification(HwdID, ID, Unit, devType, subType, NTYPE_USAGE, (float)pMeter->intval1);
	}

	if (m_verboselevel == EVBL_ALL)
	{
		switch (pMeter->subtype)
		{
		case sTypeVisibility:
			WriteMessage("subtype       = Visibility");
			sprintf(szTmp,"Visibility = %.1f km", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeSolarRadiation:
			WriteMessage("subtype       = Solar Radiation");
			sprintf(szTmp,"Radiation = %.1f Watt/m2", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeSoilMoisture:
			WriteMessage("subtype       = Soil Moisture");
			sprintf(szTmp,"Moisture = %d cb", pMeter->intval1);
			WriteMessage(szTmp);
			break;
		case sTypeLeafWetness:
			WriteMessage("subtype       = Leaf Wetness");
			sprintf(szTmp,"Wetness = %d", pMeter->intval1);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
	}
	return DevRowIdx;
}

//BBQ sensor has two temperature sensors, add them as two temperature devices
unsigned long long MainWorker::decode_BBQ(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
	std::string devname;

	unsigned char devType=pTypeBBQ;
	unsigned char subType=pResponse->BBQ.subtype;

	sprintf(szTmp,"%d",1);//(pResponse->BBQ.id1 * 256) + pResponse->BBQ.id2); //this because every time you turn the device on, you get a new ID
	std::string ID=szTmp;

	unsigned char Unit=pResponse->BBQ.id2;

	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->BBQ.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->BBQ.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	unsigned long long DevRowIdx=0;

	float temp1,temp2;
	temp1=float((pResponse->BBQ.sensor1h * 256) + pResponse->BBQ.sensor1l);// / 10.0f;
	temp2=float((pResponse->BBQ.sensor2h * 256) + pResponse->BBQ.sensor2l);// / 10.0f;

	sprintf(szTmp,"%.0f;%.0f",temp1,temp2);
	DevRowIdx=m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp,devname);
	PrintDeviceName(devname);
	if (m_verboselevel == EVBL_ALL)
	{
		switch (pResponse->BBQ.subtype)
		{
		case sTypeBBQ1:
			WriteMessage("subtype       = Maverick ET-732");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->BBQ.packettype, pResponse->BBQ.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->BBQ.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"ID            = %d", (pResponse->BBQ.id1 * 256) + pResponse->BBQ.id2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Sensor1 Temp  = %.1f C", temp1);
		WriteMessage(szTmp);
		sprintf(szTmp,"Sensor2 Temp  = %.1f C", temp2);
		WriteMessage(szTmp);

		sprintf(szTmp,"Signal level  = %d", pResponse->BBQ.rssi);
		WriteMessage(szTmp);

		if ((pResponse->BBQ.battery_level &0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}

	//Send the two sensors

	//Temp
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP6;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=SignalLevel;
	tsen.TEMP.id1=0;
	tsen.TEMP.id2=1;

	tsen.TEMP.tempsign=(temp1>=0)?0:1;
	int at10=round(abs(temp1*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);
	decode_Temp(HwdID, (const tRBUF*)&tsen.TEMP);

	tsen.TEMP.id1=0;
	tsen.TEMP.id2=2;

	tsen.TEMP.tempsign=(temp2>=0)?0:1;
	at10=round(abs(temp2*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);
	decode_Temp(HwdID, (const tRBUF*)&tsen.TEMP);

	return DevRowIdx;
}

//not in dbase yet
unsigned long long MainWorker::decode_FS20(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeFS20;

	char szTmp[100];

	switch (pResponse->FS20.subtype)
	{
	case sTypeFS20:
		WriteMessage("subtype       = FS20");
		sprintf(szTmp,"Sequence nbr  = %d", pResponse->FS20.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"House code    = %02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Address       = %02X", pResponse->FS20.addr);
		WriteMessage(szTmp);

		WriteMessage("Cmd1          = ", false);

		switch (pResponse->FS20.cmd1 & 0x1F)
		{
		case 0x0:
			WriteMessage("Off");
			break;
		case 0x1:
			WriteMessage("dim level 1 = 6.25%");
			break;
		case 0x2:
			WriteMessage("dim level 2 = 12.5%");
			break;
		case 0x3:
			WriteMessage("dim level 3 = 18.75%");
			break;
		case 0x4:
			WriteMessage("dim level 4 = 25%");
			break;
		case 0x5:
			WriteMessage("dim level 5 = 31.25%");
			break;
		case 0x6:
			WriteMessage("dim level 6 = 37.5%");
			break;
		case 0x7:
			WriteMessage("dim level 7 = 43.75%");
			break;
		case 0x8:
			WriteMessage("dim level 8 = 50%");
			break;
		case 0x9:
			WriteMessage("dim level 9 = 56.25%");
			break;
		case 0xA:
			WriteMessage("dim level 10 = 62.5%");
			break;
		case 0xB:
			WriteMessage("dim level 11 = 68.75%");
			break;
		case 0xC:
			WriteMessage("dim level 12 = 75%");
			break;
		case 0xD:
			WriteMessage("dim level 13 = 81.25%");
			break;
		case 0xE:
			WriteMessage("dim level 14 = 87.5%");
			break;
		case 0xF:
			WriteMessage("dim level 15 = 93.75%");
			break;
		case 0x10:
			WriteMessage("On (100%)");
			break;
		case 0x11:
			WriteMessage("On ( at last dim level set)");
			break;
		case 0x12:
			WriteMessage("Toggle between Off and On (last dim level set)");
			break;
		case 0x13:
			WriteMessage("Bright one step");
			break;
		case 0x14:
			WriteMessage("Dim one step");
			break;
		case 0x15:
			WriteMessage("Start dim cycle");
			break;
		case 0x16:
			WriteMessage("Program(Timer)");
			break;
		case 0x17:
			WriteMessage("Request status from a bidirectional device");
			break;
		case 0x18:
			WriteMessage("Off for timer period");
			break;
		case 0x19:
			WriteMessage("On (100%) for timer period");
			break;
		case 0x1A:
			WriteMessage("On ( at last dim level set) for timer period");
			break;
		case 0x1B:
			WriteMessage("Reset");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown command = %02X", pResponse->FS20.cmd1);
			WriteMessage(szTmp);
			break;
		}

		if ((pResponse->FS20.cmd1 & 0x80) == 0)
			WriteMessage("                command to receiver");
		else
			WriteMessage("                response from receiver");

		if ((pResponse->FS20.cmd1 & 0x40) == 0)
			WriteMessage("                unidirectional command");
		else
			WriteMessage("                bidirectional command");

		if ((pResponse->FS20.cmd1 & 0x20) == 0)
			WriteMessage("                additional cmd2 byte not present");
		else
			WriteMessage("                additional cmd2 byte present");

		if ((pResponse->FS20.cmd1 & 0x20) !=0)
		{
			sprintf(szTmp,"Cmd2          = %02X", pResponse->FS20.cmd2);
			WriteMessage(szTmp);
		}
		break;
	case sTypeFHT8V:
		WriteMessage("subtype       = FHT 8V valve");

		sprintf(szTmp,"Sequence nbr  = %d", pResponse->FS20.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"House code    = %02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Address       = %02X", pResponse->FS20.addr);
		WriteMessage(szTmp);

		WriteMessage("Cmd1          = ", false);

		if ((pResponse->FS20.cmd1 &0x80) == 0)
			WriteMessage("new command");
		else
			WriteMessage("repeated command");

		if ((pResponse->FS20.cmd1 & 0x40) == 0)
			WriteMessage("                unidirectional command");
		else
			WriteMessage("                bidirectional command");

		if ((pResponse->FS20.cmd1 & 0x20) == 0)
			WriteMessage("                additional cmd2 byte not present");
		else
			WriteMessage("                additional cmd2 byte present");

		if ((pResponse->FS20.cmd1 & 0x10) == 0)
			WriteMessage("                battery empty beep not enabled");
		else
			WriteMessage("                enable battery empty beep");

		switch (pResponse->FS20.cmd1 & 0xF)
		{
		case 0x0:
			WriteMessage("                Synchronize now");
			sprintf(szTmp,"Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55f);
			WriteMessage(szTmp);
			break;
		case 0x1:
			WriteMessage("                open valve");
			break;
		case 0x2:
			WriteMessage("                close valve");
			break;
		case 0x6:
			WriteMessage("                open valve at percentage level");
			sprintf(szTmp,"Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55f);
			WriteMessage(szTmp);
			break;
		case 0x8:
			WriteMessage("                relative offset (cmd2 bit 7=direction, bit 5-0 offset value)");
			break;
		case 0xA:
			WriteMessage("                decalcification cycle");
			sprintf(szTmp,"Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55f);
			WriteMessage(szTmp);
			break;
		case 0xC:
			WriteMessage("                synchronization active");
			sprintf(szTmp,"Cmd2          = count down is %d seconds", pResponse->FS20.cmd2 >> 1);
			WriteMessage(szTmp);
			break;
		case 0xE:
			WriteMessage("                test, drive valve and produce an audible signal");
			break;
		case 0xF:
			WriteMessage("                pair valve (cmd2 bit 7-1 is count down in seconds, bit 0=1)");
			sprintf(szTmp,"Cmd2          = count down is %d seconds", pResponse->FS20.cmd2 >> 1);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown command = %02X", pResponse->FS20.cmd1);
			WriteMessage(szTmp);
			break;
		}
		break;
	case sTypeFHT80:
		WriteMessage("subtype       = FHT80 door/window sensor");
		sprintf(szTmp,"Sequence nbr  = %d", pResponse->FS20.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp,"House code    = %02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
		WriteMessage(szTmp);
		sprintf(szTmp,"Address       = %02X", pResponse->FS20.addr);
		WriteMessage(szTmp);

		WriteMessage("Cmd1          = ", false);

		switch (pResponse->FS20.cmd1 & 0xF)
		{
		case 0x1:
			WriteMessage("sensor opened");
			break;
		case 0x2:
			WriteMessage("sensor closed");
			break;
		case 0xC:
			WriteMessage("synchronization active");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown command = %02X", pResponse->FS20.cmd1);
			WriteMessage(szTmp);
			break;
		}

		if ((pResponse->FS20.cmd1 &0x80) == 0)
			WriteMessage("                new command");
		else
			WriteMessage("                repeated command");
		break;
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->FS20.packettype, pResponse->FS20.subtype);
		WriteMessage(szTmp);
		break;
	}

	sprintf(szTmp,"Signal level  = %d", pResponse->FS20.rssi);
	WriteMessage(szTmp);
	return -1;
}

bool MainWorker::SetRFXCOMHardwaremodes(const int HardwareID, const unsigned char Mode1,const unsigned char Mode2,const unsigned char Mode3,const unsigned char Mode4,const unsigned char Mode5)
{
	int hindex=FindDomoticzHardware(HardwareID);
	if (hindex==-1)
		return false;
	m_hardwaredevices[hindex]->m_rxbufferpos=0;
	tRBUF Response;
	Response.ICMND.packetlength = sizeof(Response.ICMND)-1;
	Response.ICMND.packettype = pTypeInterfaceControl;
	Response.ICMND.subtype = sTypeInterfaceCommand;
	Response.ICMND.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
	Response.ICMND.cmnd = cmdSETMODE;
	Response.ICMND.msg1=Mode1;
	Response.ICMND.msg2=Mode2;
	Response.ICMND.msg3=Mode3;
	Response.ICMND.msg4=Mode4;
	Response.ICMND.msg5=Mode5;
	WriteToHardware(HardwareID,(const char*)&Response,sizeof(Response.ICMND));
	DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&Response);
	//Save it also
	SendCommand(HardwareID,cmdSAVE,"Save Settings");

	m_hardwaredevices[hindex]->m_rxbufferpos=0;

	return true;
}

bool MainWorker::HandleSwitchAction(const bool bIsOn, const std::string &OnAction, const std::string &OffAction)
{
	if (bIsOn)
	{
		if (OnAction.find("http://")!=std::string::npos)
		{
			_tTaskItem tItem;
			tItem=_tTaskItem::GetHTTPPage(1,OnAction,"SwitchActionOn");
			m_sql.AddTaskItem(tItem);
		}
		else if (OnAction.find("script://")!=std::string::npos)
		{
			//Execute possible script
			std::string scriptname=OnAction.substr(9);
			if (file_exist(scriptname.c_str()))
			{
				//Add parameters
				std::string scriptparams="";
				m_sql.AddTaskItem(_tTaskItem::ExecuteScript(1,scriptname,scriptparams));
			}
		}
	}
	else
	{
		if (OffAction.find("http://")!=std::string::npos)
		{
			_tTaskItem tItem;
			tItem=_tTaskItem::GetHTTPPage(1,OffAction,"SwitchActionOff");
			m_sql.AddTaskItem(tItem);
		}
		else if (OffAction.find("script://")!=std::string::npos)
		{
			//Execute possible script
			std::string scriptname=OffAction.substr(9);
			if (file_exist(scriptname.c_str()))
			{
				//Add parameters
				std::string scriptparams="";
				m_sql.AddTaskItem(_tTaskItem::ExecuteScript(1,scriptname,scriptparams));
			}
		}
	}
	return true;
}

bool MainWorker::SwitchLightInt(const std::vector<std::string> &sd, std::string switchcmd, int level, int hue, const bool IsTesting)
{
	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	unsigned char ID1=(unsigned char)((ID&0xFF000000)>>24);
	unsigned char ID2=(unsigned char)((ID&0x00FF0000)>>16);
	unsigned char ID3=(unsigned char)((ID&0x0000FF00)>>8);
	unsigned char ID4=(unsigned char)((ID&0x000000FF));

	std::string OnAction=sd[6];
	std::string OffAction=sd[7];

	int HardwareID = atoi(sd[0].c_str());

	if (HardwareID==1000)
	{
		//Special cases
		if (ID==0x00148702)
		{
			int iSecStatus=2;
			if (switchcmd=="Disarm")
				iSecStatus=0;
			else if (switchcmd=="Arm Home")
				iSecStatus=1;
			else if (switchcmd=="Arm Away")
				iSecStatus=2;
			else
				return false;
			UpdateDomoticzSecurityStatus(iSecStatus);
			return HandleSwitchAction((switchcmd!="Disarm"),OnAction,OffAction);
		}
	}

	int hindex=FindDomoticzHardware(HardwareID);
	if (hindex==-1)
		return false;


	unsigned char Unit=atoi(sd[2].c_str());
	unsigned char dType=atoi(sd[3].c_str());
	unsigned char dSubType=atoi(sd[4].c_str());
	_eSwitchType switchtype=(_eSwitchType)atoi(sd[5].c_str());

	//when level = 0, set switch command to Off
	if (switchcmd=="Set Level")
	{
		if (level > 0)
			level-=1;
		if (level==0)
			switchcmd="Off";
	}

	bool bIsLightSwitchOn=IsLightSwitchOn(switchcmd);

	switch (dType)
	{
	case pTypeLighting1:
		{
			tRBUF lcmd;
			lcmd.LIGHTING1.packetlength=sizeof(lcmd.LIGHTING1)-1;
			lcmd.LIGHTING1.packettype=dType;
			lcmd.LIGHTING1.subtype=dSubType;
			lcmd.LIGHTING1.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.LIGHTING1.housecode=atoi(sd[1].c_str());
			lcmd.LIGHTING1.unitcode=Unit;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING1.cmnd))
				return false;
			if (switchtype==STYPE_Doorbell)
			{
				int rnvalue=0;
				m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
				if (rnvalue==0)
					lcmd.LIGHTING1.cmnd=light1_sChime;
				else
					lcmd.LIGHTING1.cmnd=light1_sOn;
			}
			lcmd.LIGHTING1.filler=0;
			lcmd.LIGHTING1.rssi=7;
			WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING1));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeLighting2:
		{
			tRBUF lcmd;
			lcmd.LIGHTING2.packetlength=sizeof(lcmd.LIGHTING2)-1;
			lcmd.LIGHTING2.packettype=dType;
			lcmd.LIGHTING2.subtype=dSubType;
			lcmd.LIGHTING2.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.LIGHTING2.id1=ID1;
			lcmd.LIGHTING2.id2=ID2;
			lcmd.LIGHTING2.id3=ID3;
			lcmd.LIGHTING2.id4=ID4;
			lcmd.LIGHTING2.unitcode=Unit;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING2.cmnd))
				return false;
			if (switchtype==STYPE_Doorbell) {
				int rnvalue=0;
				m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
				if (rnvalue==0)
					lcmd.LIGHTING2.cmnd=light2_sGroupOn;
				else
					lcmd.LIGHTING2.cmnd=light2_sOn;
				level=15;
			}
			else if (switchtype==STYPE_X10Siren) {
				level=15;
			}
			if (level>15)
				level=15;
			lcmd.LIGHTING2.level=(unsigned char)level;
			lcmd.LIGHTING2.filler=0;
			lcmd.LIGHTING2.rssi=7;
			WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING2));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeLighting3:
		if (level>9)
			level=9;
		break;
	case pTypeLighting4:
		break;
	case pTypeLighting5:
		{
			int oldlevel=level;
			tRBUF lcmd;
			lcmd.LIGHTING5.packetlength=sizeof(lcmd.LIGHTING5)-1;
			lcmd.LIGHTING5.packettype=dType;
			lcmd.LIGHTING5.subtype=dSubType;
			lcmd.LIGHTING5.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.LIGHTING5.id1=ID2;
			lcmd.LIGHTING5.id2=ID3;
			lcmd.LIGHTING5.id3=ID4;
			lcmd.LIGHTING5.unitcode=Unit;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING5.cmnd))
				return false;
			if (switchtype==STYPE_Doorbell)
			{
				int rnvalue=0;
				m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
				if (rnvalue==0)
					lcmd.LIGHTING5.cmnd=light5_sGroupOn;
				else
					lcmd.LIGHTING5.cmnd=light5_sOn;
				level=31;
			}
			else if (switchtype==STYPE_X10Siren)
			{
				level=31;
			}
			if (level>31)
				level=31;
			lcmd.LIGHTING5.level=(unsigned char)level;
			lcmd.LIGHTING5.filler=0;
			lcmd.LIGHTING5.rssi=7;
			if (dSubType==sTypeLivolo)
			{
				if ((switchcmd=="Set Level")&&(level==0))
				{
					switchcmd="Off";
					GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING5.cmnd);
				}
				if (switchcmd!="Off")
				{
					//Special Case, turn off first
					unsigned char oldCmd=lcmd.LIGHTING5.cmnd;
					lcmd.LIGHTING5.cmnd=light5_sLivoloAllOff;
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
					lcmd.LIGHTING5.cmnd=oldCmd;
				}
				if (switchcmd=="Set Level")
				{
					//dim value we have to send multiple times
					for (int iDim=0; iDim<level; iDim++)
						WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
				}
				else
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
			}
			else if (dSubType==sTypeTRC02)
			{
				if ((hue!=-1)&&(hue!=1000))
				{
					double dval;
					dval=(255.0/360.0)*float(hue);
					oldlevel=round(dval);
					switchcmd="Set Color";
				}
				if (((switchcmd=="Off")||(switchcmd=="On"))&&(switchcmd!="Set Color"))
				{
					//Special Case, turn off first
					unsigned char oldCmd=lcmd.LIGHTING5.cmnd;
					lcmd.LIGHTING5.cmnd=light5_sRGBoff;
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
					lcmd.LIGHTING5.cmnd=oldCmd;
					sleep_milliseconds(100);
				}
				if ((switchcmd=="On")||(switchcmd=="Set Color"))
				{
					//turn on
					lcmd.LIGHTING5.cmnd=light5_sRGBon;
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
					sleep_milliseconds(100);

					if (switchcmd=="Set Color")
					{
						if ((oldlevel!=-1)&&(oldlevel!=1000))
						{
							double dval;
							dval=(78.0/255.0)*float(oldlevel);
							lcmd.LIGHTING5.cmnd=light5_sRGBcolormin+1+round(dval);
							WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
						}
					}
				}
			}
			else
				WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING5));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeLighting6:
		{
			tRBUF lcmd;
			lcmd.LIGHTING6.packetlength=sizeof(lcmd.LIGHTING6)-1;
			lcmd.LIGHTING6.packettype=dType;
			lcmd.LIGHTING6.subtype=dSubType;
			lcmd.LIGHTING6.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.LIGHTING6.seqnbr2=0;
			lcmd.LIGHTING6.id1=ID2;
			lcmd.LIGHTING6.id2=ID3;
			lcmd.LIGHTING6.groupcode=ID4;
			lcmd.LIGHTING6.unitcode=Unit;
			lcmd.LIGHTING6.cmndseqnbr=m_hardwaredevices[hindex]->m_SeqNr%4;

			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING6.cmnd))
				return false;
			lcmd.LIGHTING6.filler=0;
			lcmd.LIGHTING6.rssi=7;
			WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING6));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeLimitlessLights:
		{
			_tLimitlessLights lcmd;
			lcmd.len=sizeof(_tLimitlessLights)-1;
			lcmd.type=dType;
			lcmd.subtype=dSubType;
			lcmd.dunit=Unit;

			if ((switchcmd=="On")||(switchcmd=="Set Level"))
			{
				if (hue!=-1)
				{
					_tLimitlessLights lcmd;
					lcmd.len=sizeof(_tLimitlessLights)-1;
					lcmd.type=dType;
					lcmd.subtype=dSubType;
					lcmd.dunit=Unit;
					if (hue!=1000)
					{
						double dval;
						dval=(255.0/360.0)*float(hue);
						int ival;
						ival=round(dval);
						lcmd.value=ival;
						lcmd.command=Limitless_SetRGBColour;
					}
					else
					{
						lcmd.command=Limitless_SetColorToWhite;
					}
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(_tLimitlessLights));
					sleep_milliseconds(100);
				}
			}

			lcmd.value=level;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.command))
				return false;
			WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(_tLimitlessLights));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeSecurity1:
		{
			tRBUF lcmd;
			lcmd.SECURITY1.packetlength=sizeof(lcmd.SECURITY1)-1;
			lcmd.SECURITY1.packettype=dType;
			lcmd.SECURITY1.subtype=dSubType;
			lcmd.SECURITY1.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.SECURITY1.id1=ID2;
			lcmd.SECURITY1.id2=ID3;
			lcmd.SECURITY1.id3=ID4;
			switch (dSubType)
			{
			case sTypeKD101:
			case sTypeSA30:
				{
					if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.SECURITY1.status))
						return false;
					//send it twice
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.SECURITY1));
					sleep_milliseconds(500);
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.SECURITY1));
					if (!IsTesting) {
						//send to internal for now (later we use the ACK)
						DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
					}
				}
				break;
			case sTypeSecX10M:
				{
					if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.SECURITY1.status))
						return false;
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.SECURITY1));
					if (!IsTesting) {
						//send to internal for now (later we use the ACK)
						DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
					}
				}
				break;
			case sTypeSecX10R:
				{
					if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.SECURITY1.status))
						return false;
					WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.SECURITY1));
					if (!IsTesting) {
						//send to internal for now (later we use the ACK)
						DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
					}
				}
				break;
			}
			HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeBlinds:
		{
			tRBUF lcmd;
			lcmd.BLINDS1.packetlength=sizeof(lcmd.BLINDS1)-1;
			lcmd.BLINDS1.packettype=dType;
			lcmd.BLINDS1.subtype=dSubType;
			lcmd.BLINDS1.id1=ID2;
			lcmd.BLINDS1.id2=ID3;
			lcmd.BLINDS1.id3=ID4;
			lcmd.BLINDS1.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.BLINDS1.unitcode=Unit;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.BLINDS1.cmnd))
				return false;
			level=15;
			lcmd.BLINDS1.filler=0;
			lcmd.BLINDS1.rssi=7;
			WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.BLINDS1));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	case pTypeChime:
		{
			tRBUF lcmd;
			lcmd.CHIME.packetlength=sizeof(lcmd.CHIME)-1;
			lcmd.CHIME.packettype=dType;
			lcmd.CHIME.subtype=dSubType;
			lcmd.CHIME.seqnbr=m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.CHIME.id1=ID3;
			lcmd.CHIME.id2=ID4;
			level=15;
			lcmd.CHIME.sound=Unit;
			lcmd.CHIME.filler=0;
			lcmd.CHIME.rssi=7;
			WriteToHardware(HardwareID,(const char*)&lcmd,sizeof(lcmd.CHIME));
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				DecodeRXMessage(m_hardwaredevices[hindex],(const unsigned char *)&lcmd);
			}
			return HandleSwitchAction(bIsLightSwitchOn,OnAction,OffAction);
		}
		break;
	}
	return false;
}

bool MainWorker::SwitchLight(unsigned long long idx, const std::string &switchcmd, int level, int hue)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType, StrParam1, StrParam2 FROM DeviceStatus WHERE (ID == " << idx << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false;

	std::vector<std::string> sd=result[0];
	return SwitchLightInt(sd,switchcmd,level,hue,false);
}

bool MainWorker::SwitchLight(const std::string &idx, const std::string &switchcmd,const std::string &level,const std::string &hue)
{
	unsigned long long ID;
	std::stringstream s_str( idx );
	s_str >> ID;

	return SwitchLight(ID,switchcmd,atoi(level.c_str()),atoi(hue.c_str()));
}

bool MainWorker::SetSetPointInt(const std::vector<std::string> &sd, const float TempValue)
{
	int HardwareID = atoi(sd[0].c_str());
	int hindex=FindDomoticzHardware(HardwareID);
	if (hindex==-1)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	unsigned char ID1=(unsigned char)((ID&0xFF000000)>>24);
	unsigned char ID2=(unsigned char)((ID&0x00FF0000)>>16);
	unsigned char ID3=(unsigned char)((ID&0x0000FF00)>>8);
	unsigned char ID4=(unsigned char)((ID&0x000000FF));

	unsigned char Unit=atoi(sd[2].c_str());
	unsigned char dType=atoi(sd[3].c_str());
	unsigned char dSubType=atoi(sd[4].c_str());
	_eSwitchType switchtype=(_eSwitchType)atoi(sd[5].c_str());

	_tThermostat tmeter;
	tmeter.subtype=sTypeThermSetpoint;
	tmeter.id1=ID1;
	tmeter.id2=ID2;
	tmeter.id3=ID3;
	tmeter.id4=ID4;
	tmeter.dunit=1;
	tmeter.temp=TempValue;
	WriteToHardware(HardwareID,(const char*)&tmeter,sizeof(_tThermostat));
	return true;
}

bool MainWorker::SetSetPoint(const std::string &idx, const float TempValue)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == " << idx << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false;

	std::vector<std::string> sd=result[0];
	return SetSetPointInt(sd,TempValue);
}

bool MainWorker::SwitchScene(const std::string &idx, const std::string &switchcmd)
{
	unsigned long long ID;
	std::stringstream s_str( idx );
	s_str >> ID;

	return SwitchScene(ID,switchcmd);
}

bool MainWorker::SwitchScene(const unsigned long long idx, const std::string &switchcmd)
{
	//Get Scene details
	std::vector<std::vector<std::string> > result;
	int nValue=(switchcmd=="On")?1:0;

	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now,&ltime);

	//first set actual scene status
	char szTmp[300];
	std::string Name="Unknown?";
	int scenetype=0;

	//Get Scene Name
	sprintf(szTmp, "SELECT Name, SceneType FROM Scenes WHERE (ID == %llu)",
		idx);
	result=m_sql.query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::string> sds=result[0];
		Name=sds[0];
		scenetype=atoi(sds[1].c_str());
	}

	sprintf(szTmp, "UPDATE Scenes SET nValue=%d, LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (ID == %llu)",
		nValue,
		ltime.tm_year+1900,ltime.tm_mon+1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
		idx);
	m_sql.query(szTmp);

	//Check if we need to email a snapshot of a Camera
	std::string emailserver;
	int n2Value;
	if (m_sql.GetPreferencesVar("EmailServer",n2Value,emailserver))
	{
		if (emailserver!="")
		{
			sprintf(szTmp,
				"SELECT CameraRowID, DevSceneDelay FROM CamerasActiveDevices WHERE (DevSceneType==1) AND (DevSceneRowID==%llu) AND (DevSceneWhen==%d)",
				idx,
				!nValue
				);
			result=m_sql.query(szTmp);
			if (result.size()>0)
			{
				std::vector<std::vector<std::string> >::const_iterator ittCam;
				for (ittCam=result.begin(); ittCam!=result.end(); ++ittCam)
				{
					std::vector<std::string> sd=*ittCam;
					std::string camidx=sd[0];
					int delay=atoi(sd[1].c_str());
					std::string subject;
					if (scenetype==0)
						subject=Name + " Activated";
					else
						subject=Name + " Status: " + switchcmd;
					m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(delay+1,camidx,subject));
				}
			}
		}
	}


	//now switch all attached devices
	std::stringstream szQuery;
	szQuery << "SELECT DeviceRowID, Cmd, Level, Hue FROM SceneDevices WHERE (SceneRowID == " << idx << ")  ORDER BY [Order] ASC";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false;
	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		int cmd=atoi(sd[1].c_str());
		int level=atoi(sd[2].c_str());
		int hue=atoi(sd[3].c_str());
		std::vector<std::vector<std::string> > result2;
		std::stringstream szQuery2;
		szQuery2 << "SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType, nValue, sValue, StrParam1, StrParam2 FROM DeviceStatus WHERE (ID == " << sd[0] << ")";
		result2=m_sql.query(szQuery2.str());
		if (result2.size()>0)
		{
			std::vector<std::string> sd2=result2[0];
			unsigned char rnValue=atoi(sd2[6].c_str());
			std::string sValue=sd2[7];
			unsigned char Unit=atoi(sd2[2].c_str());
			unsigned char dType=atoi(sd2[3].c_str());
			unsigned char dSubType=atoi(sd2[4].c_str());
			_eSwitchType switchtype=(_eSwitchType)atoi(sd2[5].c_str());

			std::string intswitchcmd=switchcmd;

			std::string lstatus="";
			int llevel=0;
			bool bHaveDimmer=false;
			bool bHaveGroupCmd=false;
			int maxDimLevel=0;
			if (scenetype==0)
			{
				GetLightStatus(dType,dSubType,cmd,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
				if (cmd==0)
					intswitchcmd="Off";
				else
					intswitchcmd="On";
			}
			else
				GetLightStatus(dType,dSubType,rnValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
			int ilevel=maxDimLevel-1;

			if ((switchtype == STYPE_Dimmer)&&(maxDimLevel!=0))
			{
				if (intswitchcmd == "On")
				{
					intswitchcmd="Set Level";
					float fLevel=(maxDimLevel/100.0f)*level;
					if (fLevel>100)
						fLevel=100;
					ilevel=round(fLevel)+1;
				}
			}

			if (switchtype != STYPE_PushOn)
			{
				SwitchLightInt(sd2,intswitchcmd,ilevel,hue,false);
			}
			else
			{
				SwitchLightInt(sd2,"On",ilevel,hue,false);
			}
			sleep_milliseconds(50);

		}
	}

	return true;
}

void MainWorker::CheckSceneCode(const int HardwareID, const char* ID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const int nValue, const char* sValue)
{
	//check for scene code
	char szTmp[200];
	std::vector<std::vector<std::string> > result;

	sprintf(szTmp,"SELECT ID, SceneType, ListenCmd FROM Scenes WHERE (HardwareID=%d AND DeviceID='%s' AND Unit=%d AND Type=%d AND SubType=%d)",HardwareID, ID, unit, devType, subType);
	result = m_sql.query(szTmp);
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			unsigned long long ID;
			std::stringstream s_str( sd[0] );
			s_str >> ID;
			int scenetype=atoi(sd[1].c_str());
			int listencmd=atoi(sd[2].c_str());

			if (scenetype==0)
			{
				//it is a 'Scene' match the nValue/Command
				if (nValue!=listencmd)
					continue;
			}

			std::string lstatus="";
			int llevel=0;
			bool bHaveDimmer=false;
			bool bHaveGroupCmd=false;
			int maxDimLevel=0;

			GetLightStatus(devType,subType,nValue,sValue,lstatus,llevel,bHaveDimmer,maxDimLevel,bHaveGroupCmd);
			std::string switchcmd=(IsLightSwitchOn(lstatus)==true)?"On":"Off";

			_tStartScene sscene;
			sscene.SceneRowID=ID;
			sscene.switchcmd=switchcmd;

			//we have to start it outside this function/loop else we have a deadlock because of the mutex in ::DecodeRXMessage
			boost::lock_guard<boost::mutex> l(m_startscene_mutex);
			m_scenes_to_start.push_back(sscene);
		}
	}
}

void MainWorker::PrintDeviceName(const std::string &devname)
{
	if (devname!="Unknown")
	{
		std::stringstream s_str;
		s_str << " (" << devname << ")";
		WriteMessage(s_str.str().c_str());
		return;
	}
	WriteMessage("");
}

void MainWorker::LoadSharedUsers()
{
	std::vector<tcp::server::CTCPServerInt::_tRemoteShareUser> users;

	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> > result2;
	std::vector<std::vector<std::string> >::const_iterator itt;
	std::vector<std::vector<std::string> >::const_iterator itt2;
	std::stringstream szQuery;

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID, Username, Password FROM USERS WHERE ((RemoteSharing==1) AND (Active==1))";
	result=m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			tcp::server::CTCPServerInt::_tRemoteShareUser suser;
			suser.Username=base64_decode(sd[1]);
			suser.Password=base64_decode(sd[2]);

			//Get User Devices
			szQuery.clear();
			szQuery.str("");
			szQuery << "SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == " << sd[0] << ")";
			result2=m_sql.query(szQuery.str());
			if (result2.size()>0)
			{
				for (itt2=result2.begin(); itt2!=result2.end(); ++itt2)
				{
					std::vector<std::string> sd2=*itt2;
					unsigned long long ID;
					std::stringstream s_str( sd2[0] );
					s_str >> ID;
					suser.Devices.push_back(ID);
				}
			}
			users.push_back(suser);
		}
	}
	m_sharedserver.SetRemoteUsers(users);
	m_sharedserver.stopAllClients();
}

void MainWorker::SetInternalSecStatus()
{
	m_eventsystem.WWWUpdateSecurityState(m_SecStatus);

	//Update Domoticz Security Device
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.SECURITY1.packetlength=sizeof(tsen.TEMP)-1;
	tsen.SECURITY1.packettype=pTypeSecurity1;
	tsen.SECURITY1.subtype=sTypeDomoticzSecurity;
	tsen.SECURITY1.battery_level=9;
	tsen.SECURITY1.rssi=6;
	tsen.SECURITY1.id1=0x14;
	tsen.SECURITY1.id2=0x87;
	tsen.SECURITY1.id3=0x02;
	tsen.SECURITY1.seqnbr=1;
	if (m_SecStatus==SECSTATUS_DISARMED)
		tsen.SECURITY1.status=sStatusNormal;
	else if (m_SecStatus==SECSTATUS_ARMEDHOME)
		tsen.SECURITY1.status=sStatusArmHome;
	else
		tsen.SECURITY1.status=sStatusArmAway;

	// convert now to string form
	time_t now = time(0);
	char *szDate = asctime(localtime(&now));
	szDate[strlen(szDate)-1]=0;

	WriteMessageStart();

	std::stringstream sTmp;
	sTmp << szDate << " (System) ";
	WriteMessage(sTmp.str().c_str(),false);
	WriteMessage("Domoticz Security Status",false);
	unsigned long long DeviceRowIdx=decode_Security1(1000, (const tRBUF*)&tsen.SECURITY1);
	WriteMessageEnd();
	m_sharedserver.SendToAll(DeviceRowIdx,(const char*)&tsen,tsen.TEMP.packetlength+1,NULL);
}

void MainWorker::UpdateDomoticzSecurityStatus(const int iSecStatus)
{
	m_SecCountdown=-1; //cancel possible previous delay
	m_SecStatus=iSecStatus;

	m_sql.UpdatePreferencesVar("SecStatus", iSecStatus);

	int nValue=0;
	m_sql.GetPreferencesVar("SecOnDelay", nValue);

	if ((nValue==0)||(iSecStatus==SECSTATUS_DISARMED))
	{
		//Do it Directly
		SetInternalSecStatus();
	}
	else
	{
		//Schedule It
		m_SecCountdown=nValue;
	}
}
