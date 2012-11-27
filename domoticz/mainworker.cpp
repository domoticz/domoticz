#include "stdafx.h"
#include "mainworker.h"
#include "RFXNames.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "mynetwork.h"
#include "Helper.h"

#include "RFXComSerial.h"
#include "RFXComTCP.h"

//#define PARSE_RFXCOM_DEVICE_LOG

#ifdef PARSE_RFXCOM_DEVICE_LOG
	#include <iostream>
	#include <fstream>
#endif

MainWorker::MainWorker()
{
	m_stoprequested=false;
	m_verboselevel=EVBL_None;
	m_bStartHardware=false;
	m_hardwareStartCounter=0;
	m_bStartup=false;
	m_webserverport="8080";
}

MainWorker::~MainWorker()
{
	Stop();
	ClearDomoticzDevices();
}

void MainWorker::StartDomoticzDevices()
{
	std::vector<CDomoticzDeviceBase*>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		if (!(*itt)->IsStarted())
		{
			(*itt)->Start();
		}
	}
}

void MainWorker::StopDomoticzDevices()
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzDeviceBase*>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		(*itt)->Stop();
	}
}

void MainWorker::SendResetCommand(CDomoticzDeviceBase *pDevice)
{
	pDevice->m_bEnableReceive=false;

	//Send Reset
	SendCommand(pDevice->HwdID,cmdRESET,"Reset");

	//wait at least 50ms
	boost::this_thread::sleep(boost::posix_time::millisec(500));

	//clear buffer, and enable receive
	pDevice->m_rxbufferpos=0;
	pDevice->m_bEnableReceive=true;

	SendCommand(pDevice->HwdID,cmdSTATUS,"Status");
}

void MainWorker::AddDomoticzDevice(CDomoticzDeviceBase *pDevice)
{
	int devidx=FindDomoticzDevice(pDevice->HwdID);
	if (devidx!=-1) //it is already there!, remove it
	{
		RemoveDomoticzDevice(m_devices[devidx]);
	}
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	pDevice->sDecodeRXMessage.connect( boost::bind( &MainWorker::DecodeRXMessage, this, _1, _2 ) );
	pDevice->sOnConnected.connect( boost::bind( &MainWorker::OnDeviceConnected, this, _1 ) );
	m_devices.push_back(pDevice);
}

void MainWorker::RemoveDomoticzDevice(CDomoticzDeviceBase *pDevice)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzDeviceBase*>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		CDomoticzDeviceBase *pOrgDevice=*itt;
		if (pOrgDevice==pDevice) {
			m_devices.erase(itt);
			pOrgDevice->Stop();
			delete pOrgDevice;
			return;
		}
	}
}

void MainWorker::RemoveDomoticzDevice(int HwdId)
{
	int dpos=FindDomoticzDevice(HwdId);
	if (dpos==-1)
		return;
	RemoveDomoticzDevice(m_devices[dpos]);
}

int MainWorker::FindDomoticzDevice(int HwdId)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzDeviceBase*>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		if ((*itt)->HwdID==HwdId)
		{
			return (itt-m_devices.begin());
		}
	}
	return -1;
}

void MainWorker::ClearDomoticzDevices()
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzDeviceBase*>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		CDomoticzDeviceBase* pOrgDevice=(*itt);
//		delete pOrgDevice;
	}
	m_devices.clear();
}

/* sunset/sunrise
http://www.earthtools.org/sun/<latitude>/<longitude>/<day>/<month>/<timezone>/<dst>
example:
http://www.earthtools.org/sun/52.214268/5.171002/11/11/99/1

get lat/long from:
http://www.latlong.net/
*/

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
	time_t atime=time(NULL);
	struct tm *ltime=localtime(&atime);

	char szURL[200];
	sprintf(szURL,"http://www.earthtools.org/sun/%s/%s/%d/%d/99/0",
		Latitude.c_str(),Longitude.c_str(),
		ltime->tm_mday,ltime->tm_mon+1);
	I_HTTPRequest * r = NewHTTPRequest( szURL );
	if (r!=NULL)
	{
		if (r->readDataInVecBuffer())
		{
			r->getBuffer(pData, ulLength);
		}
		r->dispose();
	}
	if (pData==NULL)
		return false;
	
	try
	{
		using boost::property_tree::ptree;
		ptree pt;
		std::istringstream input( (char*)pData );
		read_xml(input, pt);

		std::string sunrise = pt.get_child("sun").get_child("morning").get<std::string>("sunrise");
		std::string sunset = pt.get_child("sun").get_child("evening").get<std::string>("sunset");
		std::string riseset=sunrise+";"+sunset;
		m_sql.UpdateTempVar("SunRiseSet",riseset.c_str());
		m_scheduler.SetSunRiseSetTimers(sunrise,sunset);
		std::cout << "Sunrise: " << sunrise << " SunSet: " << sunset << std::endl;
	}
	catch (...)
	{
	}
	delete pData;

	return true;
}

void MainWorker::SetVerboseLevel(eVerboseLevel Level)
{
	m_verboselevel=Level;
}

void MainWorker::SetWebserverPort(std::string Port)
{
	m_webserverport=Port;
}

std::string MainWorker::GetWebserverPort()
{
	return m_webserverport;
}

bool MainWorker::AddDeviceFromParams(
	int ID,
	std::string Name,
	_eHardwareTypes Type,
	std::string Address, unsigned short Port, 
	std::string Username, std::string Password, 
	unsigned char Mode1,
	unsigned char Mode2, 
	unsigned char Mode3,
	unsigned char Mode4,
	unsigned char Mode5)
{
	char szSerialPort[100];
	CDomoticzDeviceBase *pDevice=NULL;

	switch (Type)
	{
	case HTYPE_RFXtrx315:
	case HTYPE_RFXtrx433:
		//USB
#if defined WIN32
		sprintf(szSerialPort,"COM%d",Port);
#else
		sprintf(szSerialPort,"/dev/ttyUSB%d",Port);
#endif
		pDevice = new RFXComSerial(ID,szSerialPort,38400);
		break;
	case HTYPE_RFXLAN:
		//LAN
		pDevice = new RFXComTCP(ID, Address, Port);
		break;
	}
	if (pDevice)
	{
		pDevice->Name=Name;
		AddDomoticzDevice(pDevice);
		m_hardwareStartCounter=0;
		m_bStartHardware=true;
		return true;
	}
	return false;
}

bool MainWorker::Start()
{
	m_bStartup=true;
	Stop();
	if (!StartThread())
		return false;

	//Add Hardware devices
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT ID, Name, Type, Address, Port, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5 FROM Hardware ORDER BY ID ASC";
	result=m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			int ID=atoi(sd[0].c_str());
			std::string Name=sd[1];
			_eHardwareTypes Type=(_eHardwareTypes)atoi(sd[2].c_str());
			std::string Address=sd[3];
			unsigned short Port=atoi(sd[4].c_str());
			std::string Username=sd[5];
			std::string Password=sd[6];
			unsigned char mode1=(unsigned char)atoi(sd[7].c_str());
			unsigned char mode2=(unsigned char)atoi(sd[8].c_str());
			unsigned char mode3=(unsigned char)atoi(sd[9].c_str());
			unsigned char mode4=(unsigned char)atoi(sd[10].c_str());
			unsigned char mode5=(unsigned char)atoi(sd[11].c_str());
			AddDeviceFromParams(ID,Name,Type,Address,Port,Username,Password,mode1,mode2,mode3,mode4,mode5);
		}
	}
	return true;
}


bool MainWorker::Stop()
{
	m_scheduler.StopScheduler();
	m_webserver.StopServer();
	StopDomoticzDevices();

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
	m_webserver.StartServer(this, "0.0.0.0",m_webserverport,"www");

	//Start Scheduler
	m_scheduler.StartScheduler(this);

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
		DecodeRXMessage(999, (const unsigned char *)&rxbuffer);
	}
	return false;
#endif

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MainWorker::Do_Work, this)));

	return (m_thread!=NULL);
}

#define HEX( x ) \
	std::setw(2) << std::setfill('0') << std::hex << (int)( x )


void MainWorker::Do_Work()
{
	while (!m_stoprequested)
	{
		//sleep 1 second
		boost::this_thread::sleep(boost::posix_time::seconds(1));

		time_t atime=time(NULL);
		tm *ltime;
		ltime=localtime(&atime);
		if (!ltime)
			continue;

		if (m_bStartHardware)
		{
			m_hardwareStartCounter++;
			if (m_hardwareStartCounter>=2)
			{
				m_bStartHardware=false;
				StartDomoticzDevices();
			}
		}

		//check for daily schedule
		if (
			(ltime->tm_sec==0)&&
			(ltime->tm_min==0)&&
			(ltime->tm_hour==0)
			)
		{
			m_sql.ScheduleDay();
		}

		//check for hour schedule
		if (
			(ltime->tm_sec==0)&&
			(ltime->tm_min==0)
			)
		{
			//
		}

		//check for 5 minute schedule
		if (
			(ltime->tm_sec==0)&&
			(ltime->tm_min%5==0)
			)
		{
			m_sql.Schedule5Minute();
		}

		//these could take some time (http downloads), thats why they are at the bottom
		//check for hour schedule
		if (
			(
			(ltime->tm_sec==0)&&
			(ltime->tm_min==0)
			) ||
			(m_bStartup)
			)
		{
			m_bStartup=false;
			GetSunSettings();
		}

	}
	std::cout << "Mainworker Stopped..." << std::endl;
}

void MainWorker::SendCommand(const int HwdID, unsigned char Cmd, const char *szMessage)
{
	int hindex=FindDomoticzDevice(HwdID);
	if (hindex==-1)
		return;
#ifdef _DEBUG
	if (szMessage!=NULL)
		std::cout << "Cmd: " << szMessage << std::endl;
#endif

	tRBUF cmd;
	cmd.ICMND.packetlength = 13;
	cmd.ICMND.packettype = 0;
	cmd.ICMND.subtype = 0;
	cmd.ICMND.seqnbr = m_devices[hindex]->m_SeqNr++;
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
	WriteToDevice(HwdID, (const char*)&cmd,sizeof(cmd.ICMND));
}

void MainWorker::WriteToDevice(const int HwdID, const char *pdata, const unsigned char length)
{
	int hindex=FindDomoticzDevice(HwdID);
	if (hindex==-1)
		return;
	m_devices[hindex]->WriteToDevice(pdata,length);
}

void MainWorker::WriteMessage(const char *szMessage)
{
	std::cout << szMessage << std::endl;
}

void MainWorker::WriteMessage(const char *szMessage, bool linefeed)
{
	std::cout << szMessage;
	if (linefeed)
		std::cout << std::endl;
}

void MainWorker::OnDeviceConnected(CDomoticzDeviceBase *pDevice)
{
	SendResetCommand(pDevice);
}

void MainWorker::DecodeRXMessage(const int HwdID, const unsigned char *pRXCommand)
{
	// current date/time based on current system
	time_t now = time(0);

	size_t Len=pRXCommand[0]+1;

	// convert now to string form
	char *szDate = asctime(localtime(&now));
	std::cout << szDate << "HwdID: " << HwdID << " RX: Len: " << std::dec << Len << " ";

	for (size_t ii=0; ii<Len; ii++)
	{
		if (ii!=0)
			std::cout << ":";
		std::cout << HEX((unsigned char)pRXCommand[ii]);
	}
	std::cout << std::endl;
	switch (pRXCommand[1])
	{
		case pTypeInterfaceMessage:
			WriteMessage("Packettype:      Interface Message");
			decode_InterfaceMessage(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRecXmitMessage:
			WriteMessage("Packettype:      Receiver/Transmitter Message");
			decode_RecXmitMessage(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeUndecoded:
			WriteMessage("Packettype:      Undecoded RF Message");
			decode_UNDECODED(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting1:
			WriteMessage("Packettype:      Lighting 1");
			decode_Lighting1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting2:
			WriteMessage("Packettype:      Lighting 2");
			decode_Lighting2(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting3:
			WriteMessage("Packettype:      Lighting 3");
			decode_Lighting3(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting4:
			WriteMessage("Packettype:      Lighting 4");
			decode_Lighting4(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting5:
			WriteMessage("Packettype:      Lighting 5");
			decode_Lighting5(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeLighting6:
			WriteMessage("Packettype:      Lighting 6");
			decode_Lighting6(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCurtain:
			WriteMessage("Packettype:      Curtain");	//only transmit
			break;
		case pTypeBlinds:
			WriteMessage("Packettype:      Blinds");
			decode_BLINDS1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeSecurity1:
			WriteMessage("Packettype:      Security 1");
			decode_Security1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCamera:
			WriteMessage("Packettype:      Camera");
			decode_Camera1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRemote:
			WriteMessage("Packettype:      Remote control & IR");
			decode_Remote(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat1:
			WriteMessage("Packettype:      Thermostat 1");
			decode_Thermostat1(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat2:
			WriteMessage("Packettype:      Thermostat 2");
			decode_Thermostat2(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeThermostat3:
			WriteMessage("Packettype:      Thermostat 3");
			decode_Thermostat3(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP:
			WriteMessage("Packettype:      Temperature");
			decode_Temp(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeHUM:
			WriteMessage("Packettype:      Humidity");
			decode_Hum(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP_HUM:
			WriteMessage("Packettype:      Temperature + Humidity");
			decode_TempHum(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeBARO:
			WriteMessage("Packettype:      Barometric");
			decode_Baro(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeTEMP_HUM_BARO:
			WriteMessage("Packettype:      Temperature + Humidity + Barometric");
			decode_TempHumBaro(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRAIN:
			WriteMessage("Packettype:      Rain Meter");
			decode_Rain(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeWIND:
			WriteMessage("Packettype:      Wind Meter");
			decode_Wind(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeUV:
			WriteMessage("Packettype:      UV Meter");
			decode_UV(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeDT:
			WriteMessage("Packettype:      Date & Time");
			decode_DateTime(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCURRENT:
			WriteMessage("Packettype:      Current Meter");
			decode_Current(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeENERGY:
			WriteMessage("Packettype:      Energy Meter");
			decode_Energy(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeCURRENTENERGY:
			WriteMessage("Packettype:      Current/Energy Meter");
			decode_Current_Energy(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeGAS:
			WriteMessage("Packettype:      Gas Meter");
			decode_Gas(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeWATER:
			WriteMessage("Packettype:      Water Meter");
			decode_Water(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeWEIGHT:
			WriteMessage("Packettype:      Weight Scales");
			decode_Weight(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRFXSensor:
			WriteMessage("Packettype:      RFXSensor");
			decode_RFXSensor(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeRFXMeter:
			WriteMessage("Packettype:      RFXMeter");
			decode_RFXMeter(HwdID, (tRBUF *)pRXCommand);
			break;
		case pTypeFS20:
			WriteMessage("Packettype:      FS20");
			decode_FS20(HwdID, (tRBUF *)pRXCommand);
			break;
		default:
			std::cout << "UNHANDLED PACKET TYPE:      FS20 " << HEX((unsigned char)pRXCommand[1]) << std::endl;
			break;
	}
}

//not in dbase yet
void MainWorker::decode_InterfaceMessage(const int HwdID, const tRBUF *pResponse)
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

					if (pResponse->IRESPONSE.msg3)
						WriteMessage("Undec             on");
					else
						WriteMessage("Undec             off");

					if (pResponse->IRESPONSE.msg5 & msg5_X10)
						WriteMessage("X10               enabled");
					else
						WriteMessage("X10               disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_ARC)
						WriteMessage("ARC               enabled");
					else
						WriteMessage("ARC               disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_AC)
						WriteMessage("AC                enabled");
					else
						WriteMessage("AC                disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_HEU)
						WriteMessage("HomeEasy EU       enabled");
					else
						WriteMessage("HomeEasy EU       disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_MEI)
						WriteMessage("Meiantech         enabled");
					else
						WriteMessage("Meiantech         disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_OREGON)
						WriteMessage("Oregon Scientific enabled");
					else
						WriteMessage("Oregon Scientific disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_ATI)
						WriteMessage("ATI               enabled");
					else
						WriteMessage("ATI               disabled");

					if (pResponse->IRESPONSE.msg5 & msg5_VISONIC)
						WriteMessage("Visonic           enabled");
					else
						WriteMessage("Visonic           disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_MERTIK)
						WriteMessage("Mertik            enabled");
					else
						WriteMessage("Mertik            disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_AD)
						WriteMessage("AD                enabled");
					else
						WriteMessage("AD                disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_HID)
						WriteMessage("Hideki            enabled");
					else
						WriteMessage("Hideki            disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_LCROS)
						WriteMessage("La Crosse         enabled");
					else
						WriteMessage("La Crosse         disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_FS20)
						WriteMessage("FS20              enabled");
					else
						WriteMessage("FS20              disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_PROGUARD)
						WriteMessage("ProGuard          enabled");
					else
						WriteMessage("ProGuard          disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_BLINDST0)
						WriteMessage("BlindsT0          enabled");
					else
						WriteMessage("BlindsT0          disabled");

					if (pResponse->IRESPONSE.msg4 & msg4_BLINDST1)
						WriteMessage("BlindsT1          enabled");
					else
						WriteMessage("BlindsT1          disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_AE)
						WriteMessage("AE                enabled");
					else
						WriteMessage("AE                disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_RUBICSON)
						WriteMessage("RUBiCSON          enabled");
					else
						WriteMessage("RUBiCSON          disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_FINEOFFSET)
						WriteMessage("FineOffset        enabled");
					else
						WriteMessage("FineOffset        disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_RFU3)
						WriteMessage("RFU protocol 3    enabled");
					else
						WriteMessage("RFU protocol 3    disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_RFU4)
						WriteMessage("RFU protocol 4    enabled");
					else
						WriteMessage("RFU protocol 4    disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_RFU5)
						WriteMessage("RFU protocol 5    enabled");
					else
						WriteMessage("RFU protocol 5    disabled");

					if (pResponse->IRESPONSE.msg3 & msg3_RFU6)
						WriteMessage("RFU protocol 6    enabled");
					else
						WriteMessage("RFU protocol 6    disabled");
				}
				break;
			case cmdSAVE:
				WriteMessage("response on cmnd  = Save");
				break;
			}
		}
	}
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

void MainWorker::decode_Rain(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

void MainWorker::decode_Wind(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[300];

	unsigned char devType=pTypeWIND;
	unsigned char subType=pResponse->WIND.subtype;
	sprintf(szTmp,"%d", (pResponse->WIND.id1*256) + pResponse->WIND.id2);
	std::string ID = szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->WIND.rssi;
	unsigned char BatteryLevel=get_BateryLevel(pResponse->WIND.subtype==sTypeWIND3, pResponse->WIND.battery_level & 0x0F);

	double dDirection;
	std::string strDirection;
	dDirection = (pResponse->WIND.directionh * 256) + pResponse->WIND.directionl;

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

	int intSpeed = (pResponse->WIND.av_speedh * 256) + pResponse->WIND.av_speedl;
	int intGust = (pResponse->WIND.gusth * 256) + pResponse->WIND.gustl;

	float temp=0,chill=0;
	if (pResponse->WIND.subtype == sTypeWIND4)
	{
		if ((pResponse->WIND.tempsign & 0x80) == 0)
		{
			temp=float((pResponse->WIND.temperatureh * 256) + pResponse->WIND.temperaturel) / 10.0f;
		}
		else
		{
			temp=-(float(((pResponse->WIND.temperatureh & 0x7F) * 256) + pResponse->WIND.temperaturel) / 10.0f);
		}

		if ((pResponse->WIND.chillsign & 0x80) == 0 )
		{
			chill=float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0f;
		}
		else
		{
			chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0f);
		}
		float wspeedms=float(intSpeed)/10.0f;
		if ((temp<10.0)&&(wspeedms>=1.4))
		{
			float chillJatTI = 13.12f+0.6215f*temp -11.37f*pow(wspeedms*3.6f,0.16f) + 0.3965f*temp*pow(wspeedms*3.6f,0.16f);
			chill=chillJatTI;
			//std::cout << "Org Chill: " << chill << " New Chill: " << chillJatTI << std::endl;
		}
	}

	sprintf(szTmp,"%.2f;%s;%d;%d;%.1f;%.1f",dDirection,strDirection.c_str(),intSpeed,intGust,temp,chill);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
			sprintf(szTmp,"Average speed = %.1f mtr/sec, %.2f km/hr, %.2f mph",float(intSpeed) / 10.0f, (float(intSpeed) * 0.36f), ((float(intSpeed) * 0.223693629f) / 10.0f));
			WriteMessage(szTmp);
		}

		sprintf(szTmp,"Wind gust     = %.1f mtr/sec, %.2f km/hr, %.2f mph", float(intGust) / 10.0f,  (float(intGust )* 0.36f),  (float(intGust) * 0.223693629f) / 10.0f);
		WriteMessage(szTmp);

		if (pResponse->WIND.subtype == sTypeWIND4)
		{
			sprintf(szTmp,"Temperature   = %.1f C", temp);
			WriteMessage(szTmp);

			sprintf(szTmp,"Chill         = %.1f C", chill);
		}

		sprintf(szTmp,"Signal level  = %d", pResponse->WIND.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->WIND.subtype==sTypeWIND3, pResponse->WIND.battery_level & 0x0F);
	}
}

void MainWorker::decode_Temp(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeTEMP;
	unsigned char subType=pResponse->TEMP.subtype;
	std::string ID="1";
	unsigned char Unit=pResponse->TEMP.id2;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->TEMP.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->TEMP.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	float temp;
	if ((pResponse->TEMP.tempsign &0x80) == 0)
	{
		temp=float((pResponse->TEMP.temperatureh * 256) + pResponse->TEMP.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP.temperatureh & 0x7F) * 256) + pResponse->TEMP.temperaturel) / 10.0f);
	}
	sprintf(szTmp,"%.1f",temp);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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

		sprintf(szTmp,"Signal level  = %d", pResponse->TEMP.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP.battery_level &0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
	}
}

void MainWorker::decode_Hum(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeHUM;
	unsigned char subType=pResponse->HUM.subtype;
	std::string ID="1";
	unsigned char Unit=1;
	unsigned char SignalLevel=pResponse->HUM.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->HUM.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	unsigned char humidity=pResponse->HUM.humidity;

	sprintf(szTmp,"%d",pResponse->HUM.humidity_status);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,humidity,szTmp);

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
}

void MainWorker::decode_TempHum(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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
	if ((pResponse->TEMP_HUM.tempsign &0x80) == 0)
	{
		temp=float((pResponse->TEMP_HUM.temperatureh * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP_HUM.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0f);
	}
	unsigned char Humidity = pResponse->TEMP_HUM.humidity;
	unsigned char HumidityStatus = pResponse->TEMP_HUM.humidity_status;


	sprintf(szTmp,"%.1f;%d;%d",temp,Humidity,HumidityStatus);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

void MainWorker::decode_TempHumBaro(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeTEMP_HUM_BARO;
	unsigned char subType=pResponse->TEMP_HUM_BARO.subtype;
	std::string ID="1";
	unsigned char Unit=pResponse->TEMP_HUM_BARO.id2;
	unsigned char cmnd=0;
	unsigned char SignalLevel=pResponse->TEMP_HUM_BARO.rssi;
	unsigned char BatteryLevel;
	if ((pResponse->TEMP_HUM_BARO.battery_level &0x0F) == 0)
		BatteryLevel=0;
	else
		BatteryLevel=100;

	float temp;
	if ((pResponse->TEMP_HUM_BARO.tempsign &0x80) == 0)
	{
		temp=float((pResponse->TEMP_HUM_BARO.temperatureh * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0f;
	}
	else
	{
		temp=-(float(((pResponse->TEMP_HUM_BARO.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0f);
	}
	unsigned char Humidity = pResponse->TEMP_HUM_BARO.humidity;
	unsigned char HumidityStatus = pResponse->TEMP_HUM_BARO.humidity_status;
	int barometer = (pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol;
	int forcast = pResponse->TEMP_HUM_BARO.forecast;

	sprintf(szTmp,"%.1f;%d;%d;%d;%d",temp,Humidity,HumidityStatus, barometer,forcast);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP_HUM_BARO.packettype, pResponse->TEMP_HUM_BARO.subtype);
			WriteMessage(szTmp);
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
}

void MainWorker::decode_UV(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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
	float temp=0;
	if (pResponse->UV.subtype == sTypeUV3)
	{
		if ((pResponse->UV.tempsign &0x80) == 0)
		{
			temp = float((pResponse->UV.temperatureh * 256) + pResponse->UV.temperaturel) / 10.0f;
		}
		else
		{
			temp = -(float(((pResponse->UV.temperatureh & 0x7F) * 256) + pResponse->UV.temperaturel) / 10.0f);
		}
	}

	sprintf(szTmp,"%.1f;%.1f",Level,temp);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

void MainWorker::decode_Lighting1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeLighting1;
	unsigned char subType=pResponse->LIGHTING1.subtype;
	sprintf(szTmp,"%d", pResponse->LIGHTING1.housecode);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING1.unitcode;
	unsigned char cmnd=pResponse->LIGHTING1.cmnd;
	unsigned char SignalLevel=pResponse->LIGHTING1.rssi;
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd);

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
			WriteMessage("subtype       = ELRO AB400");
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
		default:
			printf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING1.packettype, pResponse->LIGHTING1.subtype);
			WriteMessage(szTmp);
			break;
		}
		printf(szTmp,"Signal level  = %d", pResponse->LIGHTING1.rssi);
		WriteMessage(szTmp);
	}
}

void MainWorker::decode_Lighting2(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeLighting2;
	unsigned char subType=pResponse->LIGHTING2.subtype;
	sprintf(szTmp,"%X%02X%02X%02X", pResponse->LIGHTING2.id1, pResponse->LIGHTING2.id2, pResponse->LIGHTING2.id3, pResponse->LIGHTING2.id4);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING2.unitcode;
	unsigned char cmnd=pResponse->LIGHTING2.cmnd;
	unsigned char level=pResponse->LIGHTING2.level;
	unsigned char SignalLevel=pResponse->LIGHTING2.rssi;
	sprintf(szTmp,"%d",level);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,szTmp);

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
}

//not in dbase yet
void MainWorker::decode_Lighting3(const int HwdID, const tRBUF *pResponse)
{
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
}

//not in dbase yet
void MainWorker::decode_Lighting4(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeLighting4;

	//tRBUF *pResponse=(tRBUF*)m_rxbuffer;
	//char szTmp[100];
	WriteMessage("Not implemented");
}

void MainWorker::decode_Lighting5(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeLighting5;
	unsigned char subType=pResponse->LIGHTING5.subtype;
	if (subType != 	sTypeEMW100)
		sprintf(szTmp,"%02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	else
		sprintf(szTmp,"%02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING5.unitcode;
	unsigned char cmnd=pResponse->LIGHTING5.cmnd;
	float level=float(pResponse->LIGHTING5.level) * 3.2f;
	unsigned char SignalLevel=pResponse->LIGHTING5.rssi;
	sprintf(szTmp,"%.1f",level);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,szTmp);

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
			sprintf(szTmp,"Set dim level to: %.2f %%" , level);
			WriteMessage(szTmp);
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
	default:
		sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING5.packettype, pResponse->LIGHTING5.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp,"Signal level  = %d", pResponse->LIGHTING5.rssi);
	WriteMessage(szTmp);
}

void MainWorker::decode_Lighting6(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeLighting6;
	unsigned char subType=pResponse->LIGHTING6.subtype;
	sprintf(szTmp,"%02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2);
	std::string ID = szTmp;
	unsigned char Unit=pResponse->LIGHTING6.unitcode;
	unsigned char cmnd=pResponse->LIGHTING6.cmnd;
	unsigned char rfu=pResponse->LIGHTING6.rfu;
	unsigned char SignalLevel=pResponse->LIGHTING6.rssi;
	sprintf(szTmp,"%d",rfu);
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,-1,cmnd,szTmp);

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
			sprintf(szTmp,"rfu           = %d", pResponse->LIGHTING6.rfu);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING6.packettype, pResponse->LIGHTING6.subtype);
			WriteMessage(szTmp);
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->LIGHTING6.rssi);
		WriteMessage(szTmp);
	}
}

//not in dbase yet
void MainWorker::decode_UNDECODED(const int HwdID, const tRBUF *pResponse)
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
	for (int i = 0; i< (pResponse->UNDECODED.packetlength - pResponse->UNDECODED.msg1); i++)
	{
		std::cout << HEX((unsigned char)(pResponse->UNDECODED.msg1 + i));
	}
	WriteMessage(" ");
}


void MainWorker::decode_RecXmitMessage(const int HwdID, const tRBUF *pResponse)
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
}

//not in dbase yet
void MainWorker::decode_BLINDS1(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeBlinds;

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
		break;
	default:
		WriteMessage("UNKNOWN");
		break;
	}
	sprintf(szTmp,"Signal level  = %d", pResponse->BLINDS1.rssi);
	WriteMessage(szTmp);
}

void MainWorker::decode_Security1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

	unsigned char devType=pTypeSecurity1;
	unsigned char subType=pResponse->SECURITY1.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
	ID=szTmp;
	unsigned char Unit=0;
	unsigned char cmnd=pResponse->SECURITY1.status;
	unsigned char SignalLevel=pResponse->SECURITY1.rssi;
	unsigned char BatteryLevel = get_BateryLevel(false, pResponse->SECURITY1.battery_level & 0x0F);

	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd);

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
			WriteMessage("subtype       = Meiantech");
			break;
		default:
			sprintf(szTmp,"ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->SECURITY1.packettype, pResponse->SECURITY1.subtype);
			WriteMessage(szTmp);
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
		case sStatusArmHomeDelayed:
			WriteMessage("Arm Home Delayed");
			break;
		case sStatusDisarm:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group1 or ", false);
			WriteMessage("Disarm");
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

		if (pResponse->SECURITY1.subtype != sTypeKD101)		//KD101 does not support battery low indication
		{
			if ((pResponse->SECURITY1.battery_level &0xF) == 0)
				WriteMessage("battery level = Low");
			else
				WriteMessage("battery level = OK");
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->SECURITY1.rssi);
		WriteMessage(szTmp);
	}
}

//not in dbase yet
void MainWorker::decode_Camera1(const int HwdID, const tRBUF *pResponse)
{
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
	}
	sprintf(szTmp, "Signal level  = %d", pResponse->CAMERA1.rssi);
	WriteMessage(szTmp);
}

//not in dbase yet
void MainWorker::decode_Remote(const int HwdID, const tRBUF *pResponse)
{
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
}

//not in dbase yet
void MainWorker::decode_Thermostat1(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];
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
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

//not in dbase yet
void MainWorker::decode_Thermostat2(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeThermostat2;

	WriteMessage("Not implemented");
}

//not in dbase yet
void MainWorker::decode_Thermostat3(const int HwdID, const tRBUF *pResponse)
{
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
}

//not in dbase yet
void MainWorker::decode_Baro(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeBARO;

	WriteMessage("Not implemented");
}

//not in dbase yet
void MainWorker::decode_DateTime(const int HwdID, const tRBUF *pResponse)
{
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
	case 0:
		WriteMessage(" Sunday");
		break;
	case 1:
		WriteMessage(" Monday");
		break;
	case 2:
		WriteMessage(" Tuesday");
		break;
	case 3:
		WriteMessage(" Wednesday");
		break;
	case 4:
		WriteMessage(" Thursday");
		break;
	case 5:
		WriteMessage(" Friday");
		break;
	case 6:
		WriteMessage(" Saterday");
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
}

//not in dbase yet
void MainWorker::decode_Current(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

//not in dbase yet
void MainWorker::decode_Energy(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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
	//usage = double( (pResponse->ENERGY.total1 * 0x10000000000) + (pResponse->ENERGY.total2 * 0x100000000) + (pResponse->ENERGY.total3 * 0x1000000)
	//+ (pResponse->ENERGY.total4 * 0x10000) + (pResponse->ENERGY.total5 * 0x100) + pResponse->ENERGY.total6) / 223.666;
	usage = double( (pResponse->ENERGY.total3 * 0x1000000)
		+ (pResponse->ENERGY.total4 * 0x10000) + (pResponse->ENERGY.total5 * 0x100) + pResponse->ENERGY.total6) / 223.666;

	sprintf(szTmp,"%ld;%.2f",instant,usage);

	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

//not in dbase yet
void MainWorker::decode_Current_Energy(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeCURRENTENERGY;

	char szTmp[100];

	double usage;

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
		//usage = (CDbl(pResponse->CURRENT_ENERGY.total1)) * &H10000000000 + CDbl(pResponse->CURRENT_ENERGY.total2)) * &H100000000 + CDbl(pResponse->CURRENT_ENERGY.total3)) * &H1000000 _
			//+ pResponse->CURRENT_ENERGY.total4) * &H10000 + pResponse->CURRENT_ENERGY.total5) * &H100 + pResponse->CURRENT_ENERGY.total6)) / 223.666
		//usage = double( (pResponse->CURRENT_ENERGY.total1 * 0x10000000000) + (pResponse->CURRENT_ENERGY.total2 * 0x100000000) + (pResponse->CURRENT_ENERGY.total3 * 0x1000000)
		//+ (pResponse->CURRENT_ENERGY.total4 * 0x10000) + (pResponse->CURRENT_ENERGY.total5 * 0x100) + pResponse->CURRENT_ENERGY.total6) / 223.666;
		usage = double( (pResponse->CURRENT_ENERGY.total3 * 0x1000000)
			+ (pResponse->CURRENT_ENERGY.total4 * 0x10000) + (pResponse->CURRENT_ENERGY.total5 * 0x100) + pResponse->CURRENT_ENERGY.total6) / 223.666;

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

//not in dbase yet
void MainWorker::decode_Gas(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeGAS;

	WriteMessage("Not implemented");
}

//not in dbase yet
void MainWorker::decode_Water(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeWATER;

	WriteMessage("Not implemented");
}

//not in dbase yet
void MainWorker::decode_Weight(const int HwdID, const tRBUF *pResponse)
{
	unsigned char devType=pTypeWEIGHT;

	char szTmp[100];

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

//not in dbase yet
void MainWorker::decode_RFXSensor(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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
			sprintf(szTmp,"%.1f",temp);
		}
		break;
	case sTypeRFXSensorAD:
	case sTypeRFXSensorVolt:
		{
			int volt=(pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2;
			sprintf(szTmp,"%d",volt);
		}
		break;
	}
	m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);

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
}

//not in dbase yet
void MainWorker::decode_RFXMeter(const int HwdID, const tRBUF *pResponse)
{
	char szTmp[100];

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

		sprintf(szTmp,"%ld",counter);
		m_sql.UpdateValue(HwdID, ID.c_str(),Unit,devType,subType,SignalLevel,BatteryLevel,cmnd,szTmp);
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
			sprintf(szTmp,"Counter       = %ld", counter);
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
			sprintf(szTmp,"Calibrate cnt = %ld msec", counter);
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
			sprintf(szTmp,"Counter       = %ld", counter);
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
		}
		sprintf(szTmp,"Signal level  = %d", pResponse->RFXMETER.rssi);
		WriteMessage(szTmp);
	}
}

//not in dbase yet
void MainWorker::decode_FS20(const int HwdID, const tRBUF *pResponse)
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
}

bool MainWorker::SwitchLight(unsigned long long idx, std::string switchcmd, unsigned char level)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	std::stringstream szQuery;
	szQuery << "SELECT HardwareID, DeviceID,Unit,Type,SubType,nValue,SwitchType FROM DeviceStatus WHERE (ID == " << idx << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false;
	std::vector<std::string> sd=result[0];

	int HardwareID = atoi(sd[0].c_str());
	int hindex=FindDomoticzDevice(HardwareID);
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
	unsigned char nValue=atoi(sd[5].c_str());
	unsigned char switchtype=atoi(sd[6].c_str());

	switch (dType)
	{
	case pTypeLighting1:
		{
			tRBUF lcmd;
			lcmd.LIGHTING1.packetlength=sizeof(lcmd.LIGHTING1)-1;
			lcmd.LIGHTING1.packettype=dType;
			lcmd.LIGHTING1.subtype=dSubType;
			lcmd.LIGHTING1.seqnbr=m_devices[hindex]->m_SeqNr++;
			lcmd.LIGHTING1.housecode=atoi(sd[1].c_str());
			lcmd.LIGHTING1.unitcode=Unit;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING1.cmnd))
				return false;
			if (switchtype==switchTypeDoorbell)
				level=15;
			lcmd.LIGHTING1.filler=0;
			lcmd.LIGHTING1.rssi=7;
/*
			unsigned char *pData=(unsigned char *)&lcmd;
			for (size_t ii=0; ii<sizeof(lcmd.LIGHTING2); ii++)
			{
				if (ii!=0)
					std::cout << ":";
				std::cout << HEX((unsigned char)pData[ii]);
			}
*/
			WriteToDevice(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING2));
			//send to internal for now (later we use the ACK)
			DecodeRXMessage(HardwareID,(const unsigned char *)&lcmd);
			return true;
		}
		break;
	case pTypeLighting2:
		{
			tRBUF lcmd;
			lcmd.LIGHTING2.packetlength=sizeof(lcmd.LIGHTING2)-1;
			lcmd.LIGHTING2.packettype=dType;
			lcmd.LIGHTING2.subtype=dSubType;
			lcmd.LIGHTING2.seqnbr=m_devices[hindex]->m_SeqNr++;
			lcmd.LIGHTING2.id1=ID1;
			lcmd.LIGHTING2.id2=ID2;
			lcmd.LIGHTING2.id3=ID3;
			lcmd.LIGHTING2.id4=ID4;
			lcmd.LIGHTING2.unitcode=Unit;
			if (!GetLightCommand(dType,dSubType,switchtype,switchcmd,lcmd.LIGHTING2.cmnd))
				return false;
			if (switchtype==switchTypeDoorbell)
				level=15;
			lcmd.LIGHTING2.level=level;
			lcmd.LIGHTING2.filler=0;
			lcmd.LIGHTING2.rssi=7;
/*
			unsigned char *pData=(unsigned char *)&lcmd;
			for (size_t ii=0; ii<sizeof(lcmd.LIGHTING2); ii++)
			{
				if (ii!=0)
					std::cout << ":";
				std::cout << HEX((unsigned char)pData[ii]);
			}
*/
			WriteToDevice(HardwareID,(const char*)&lcmd,sizeof(lcmd.LIGHTING2));
			//send to internal for now (later we use the ACK)
			DecodeRXMessage(HardwareID,(const unsigned char *)&lcmd);
			return true;
		}
		break;
	case pTypeLighting3:
		break;
	case pTypeLighting4:
		break;
	case pTypeLighting5:
		break;
	case pTypeLighting6:
		break;
	}
	return false;
}

bool MainWorker::SwitchLight(std::string idx, std::string switchcmd,std::string level)
{
	unsigned long long ID;
	std::stringstream s_str( idx );
	s_str >> ID;

	return SwitchLight(ID,switchcmd,atoi(level.c_str()));
}
