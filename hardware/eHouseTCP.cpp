/***************************************************************************************************************/
/*	Recent update: 2018-08-16
SHOP: http://eHouse.Biz/
DIY:  http://Smart.eHouse.Pro/

Author: Robert Jarzabek
eHouse Home Automation

eHouse BMS & Home Automation system support for Domoticz.
eHouse contains of various communication interfaces and controller sizes to realize any size and complex instalation
- LAN (Ethernet):
	- EthernetRoomManager: 12-20 binary inputs, 32 binary outputs, 3 dimmers, InfraRed RX+TX, 2-15 ADC measurement inputs (~59 intelligent points)
	- EthernetPoolManager: Dedicated swimming pool controller
	- CommManager/Level Manager: Integrated security system, 48 binary inputs, 77 binary outputs 15 ADC measurement inputs (~120 intelligent points)
- RS-485 (full duplex):
	- RoomManager: 12 binary inputs, 24 binary outputs, 3 dimmers, InfraRed RX+TX, 8 ADC measurement inputs (~49 intelligent points)
	- HeatManager: dedicated BoilerRoom+Ventilation controller 21 binary outputs, 3 dimmers, 16 ADC measurement inputs
	- CommManager/Level Manager: Integrated security system, 48 binary inputs, 77 binary outputs 15 ADC measurement inputs
- WiFi - all in one contoller (2-4 binary inputs, 1 temperature inputs, 0-4 relay outputs, 0-3 PWM Dimmers	(>10 intelligent points)
- PRO centralized instalation based on Linux Microcomputer + 128 Inputs / 128 Output modules, SMS Notification (~256 intelligent points)
- Aura - RF863 thermostats
- RFID - access control with card readers
- Cloud - access indirectly via eHouse Cloud or private clouds

Structure UDP status reception from Domoticz
							Domoticz (eHouse UDP Listener)
							|         |       	|
eHouse 	(directly )	    	WiFi     PRO		LAN
									  |
									  |
						 -------------------------------   (indirectly - via eHouse PRO Server)
						|	   |     |     |      |     |
eHouse				RS-485	 RFID  AURA  CLOUD  CAN/RF  (Future NON IP Solutions)

Events/Commands send via TCP/IP socket with simple authorization (dynamic code/chalange-response based on XOR Password)

Domoticz ID construction
MSB...............LSB
AddrH AddrL IOCde IONr
Eg output nr 2 at eHouse LAN (192.168.0.201)
h l O nr
00C92101
*/
////////////////////////////////////////////////////////////////////////////////

//Giz: To the author of this class!, Please, name start all class members with 'm_' and make all function parameters 'const' where possible
//booleans are booleans, not unsigned char

//#define DEBUG_eHouse 1
#include "stdafx.h"
#include "eHouse/globals.h"
#include "eHouse/status.h"
#include "eHouseTCP.h"
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"
//#include <vector>
//#include <thread>
#define OPTA_CLR_DB					0x1		//Clear Domoticz DBs on start (for test of device discovery - other devices will also be deleted)
#define OPTA_FORCE_TCP				0x2		//Force TCP/IP instead of UDP for LAN connection
#define OPTA_DEBUG					0x4		//Debug Info
#define OPTA_DETECT_TCP_PACKETS		0x8		//Perform Multiple TCP Packages

#define EHOUSE_TEMP_POLL_INTERVAL_MS 120*1000 	// 120 sec
#define HEARTBEAT_INTERVAL_MS 12*1000 			// 12 sec

#define round(a) ( int ) ( a + .5 )
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Init Structures on start
void eHouseTCP::InitStructs()
{
	int i;
	int to = EHOUSE_WIFI_MAX + 1;
	for (i = 0; i < to; i++)
	{
		(m_eHWIFIs[i]) = nullptr;    // full wifi status
		(m_eHWIFIPrev[i]) = nullptr; // full wifi status previous for detecting changes
		(m_eHWIFIn[i]) = nullptr;    // names of i/o for WiFi controllers
		(m_eHWiFi[i]) = nullptr;
	}
	m_ECMn = nullptr;
	m_ECM = nullptr;
	m_ECMPrv = nullptr; // Previous statuses for Update MSQL optimalization  (change data only updated)
	m_eHouseProN = nullptr;
	m_eHouseProStatus = nullptr;
	m_eHouseProStatusPrv = nullptr;

	to = ETHERNET_EHOUSE_RM_MAX + 1;
	for (i = 0; i < to; i++)
	{
		(m_eHEn[i]) = nullptr;	    // names of i/o for Ethernet controllers
		(m_eHERMs[i]) = nullptr;    // full ERM status decoded
		(m_eHERMPrev[i]) = nullptr; // full ERM status decoded previous for detecting changes
	}
	to = EHOUSE1_RM_MAX + 1;
	for (i = 0; i < to; i++)
	{
		(m_eHRMs[i]) = nullptr;	   // full RM status decoded
		(m_eHRMPrev[i]) = nullptr; // full RM status decoded previous for detecting changes
		(m_eHn[i]) = nullptr;
	}
	to = EVENT_QUEUE_MAX;
	for (i = 0; i < to; i++)
		(m_EvQ[i]) = nullptr; // eHouse event queue for submit to the controllers (directly LAN, WiFi, PRO / indirectly via PRO
				      // other variants) - multiple events can be executed at once
	to = MAX_AURA_DEVS;
	for (i = 0; i < to; i++)
	{
		(m_AuraDev[i]) = nullptr;    // Aura status thermostat
		(m_AuraDevPrv[i]) = nullptr; // previous for detecting changes
		(m_AuraN[i]) = nullptr;
	}
	//future initialzation
#ifndef REMOVEUNUSED
	CANStatus 				eHCAN[EHOUSE_RF_MAX];
	CANStatus 				eHCANRF[EHOUSE_RF_MAX];
	CANStatus 				eHCANPrv[EHOUSE_RF_MAX];
	CANStatus 				eHCANRFPrv[EHOUSE_RF_MAX];

	eHouse1Status			eHPrv[EHOUSE1_RM_MAX];
	CMStatus                eHEPrv[ETHERNET_EHOUSE_RM_MAX + 1];
	WiFiStatus              eHWiFiPrv[EHOUSE_WIFI_MAX + 1];
#endif


#ifndef REMOVEUNUSED
	WIFIFullStat            eHCANPrev[EHOUSE_CAN_MAX];
	WIFIFullStat            eHRFPrev[EHOUSE_RF_MAX];
	WIFIFullStat            eHCANs[EHOUSE_CAN_MAX];
	WIFIFullStat            eHRFs[EHOUSE_RF_MAX];
#endif


#ifndef REMOVEUNUSED
	eHouseCANNames              eHCANn[EHOUSE_RF_MAX + 1];
	eHouseCANNames              eHCANRFn[EHOUSE_RF_MAX + 1];
	SatelNames                  SatelN[MAX_SATEL];
	SatelStatus                 SatelStat[MAX_SATEL];
#endif




}
///////////////////////////////////////////////////////////////////////////////////////////
// Get controller type from address
int eHouseTCP::gettype(int adrh, int adrl)
{
	if ((adrh == 1) && (adrl == 1))
	{
		m_Dtype = EH_RS485;
		m_Dsubtype = 1;
		return m_Dtype;
	}
	if ((adrh == 2) && (adrl == 1))
	{
		m_Dtype = EH_RS485;
		m_Dsubtype = 2;
		return m_Dtype;
	}
	if (adrh == 55)
	{
		m_Dtype = EH_RS485;
		m_Dsubtype = 3;
		return m_Dtype;
	}
	if ((adrh == 0x79) || (adrh == 0x80))
	{
		m_Dtype = EH_CANRF;
		m_Dsubtype = adrl;
		return m_Dtype;
	}
	if (adrh == 0x81)
	{
		m_Dtype = EH_AURA;
		m_Dsubtype = adrl;
		return m_Dtype;
	}
	if (adrh == 0x82)
	{
		m_Dtype = EH_LORA;
		m_Dsubtype = adrl;
		return m_Dtype;
	}
	if (((adrl > 190) && (adrl < 201)) || (adrl == m_SrvAddrL))
	{
		m_Dtype = EH_PRO;
		m_Dsubtype = adrl;
		return m_Dtype;
	}
	if ((adrl > 200) && (adrl < 255))
	{
		m_Dtype = EH_LAN;
		m_Dsubtype = adrl;
		return m_Dtype;
	}
	if ((adrl > 99) && (adrl < 190))
	{
		m_Dtype = EH_WIFI;
		m_Dsubtype = adrl;
		return m_Dtype;
	}
	m_Dtype = EH_PRO;
	return m_Dtype;
}
//////////////////////////////////////////////////////////////////////////////////
// Get eHouse Controller Type
void eHouseTCP::eHType(int devtype, char *dta)
{
	switch (devtype)
	{
	case EH_RS485:
		strcpy(dta, "eHRS");
		break;
	case EH_LAN:
		strcpy(dta, "eHEt");
		break;
	case EH_PRO:
		strcpy(dta, "eHPR");
		break;
	case EH_WIFI:
		strcpy(dta, "eHWi");
		break;
	case EH_CANRF:
		strcpy(dta, "eHCR");
		break;
	case EH_AURA:
		strcpy(dta, "eHAu");
		break;
	case EH_LORA:
		strcpy(dta, "eHLo");
		break;
	}
}
//////////////////////////////////////////////////////////////////////////////////////
//Device Discovery - Update database fields (DeviceStatus) + Plans for each controller
///////////////////////////////////////////////////////////////////////////////////////
int eHouseTCP::UpdateSQLState(int devh, const uint8_t devl, int devtype, const uint8_t type, const uint8_t subtype, int swtype, int code,
	int nr, const uint8_t signal, int nValue, const char  *sValue, const char * Name, const char * SignalName,
	bool /*on_off*/, const uint8_t battery,int m_PlanID)
{
	char IDX[20];
	char state[5] = "";
	uint64_t i;
	sprintf(IDX, "%02X%02X%02X%02X", devh, devl, code, nr);  //index calculated adrh,adrl,signalcode,i/o nr
	if ((type == pTypeLighting2)) // || (type==pTypeTEMP))
		sprintf(IDX, "%X%02X%02X%02X", devh, devl, code, nr);    //exception bug in Domoticz??
	std::string devname;
	std::vector<std::vector<std::string> > result;
	//if name contains '@' - ignore i/o - do not add to DB (unused)
	if ((strstr(Name, "@") == nullptr) && (strlen(Name) > 0))
	{
		devname.append(Name, strlen(Name));
		devname.append(" - ");
	}
	if (swtype != STYPE_Selector)
		if ((strstr(SignalName, "@") != nullptr) || (strlen(SignalName) < 1))
		{
			return -1;
		}
	devname.append(SignalName, strlen(SignalName));
	if ((devtype != EH_PRO) && (devtype != EH_CANRF))
		devname = ISO2UTF8(devname);
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
		m_HwdID, IDX, devl);

	if (result.empty())
	{
///GIZ		!!! Your solution NOT WORKING eg, not setting used=1 and is not sufficient We update devicestatus, and add RoomPlan for each controller
//i = m_sql.InsertDevice(m_HwdID, IDX, devl, type, subtype, swtype, nValue, sValue, devname, signal, battery, 1);
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, nValue, sValue, Name, Used, SwitchType ) VALUES ('%d', '%q', '%d','%d', '%d', '%d', '%d', '%d', '%q', '%q', 1, %d)",
			m_HwdID, IDX, devl, type, subtype, signal, battery, nValue, sValue, devname.c_str(), swtype);

	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",m_HwdID,                IDX,           devl );
																		   
//add Plan for each controllers
	if (!result.empty())
		{
		i = atoi(result[0][0].c_str());
			result = m_sql.safe_query("SELECT ID FROM DeviceToPlansMap WHERE (DeviceRowID==%d)", i);
			if (result.empty())
			{
				//Why Can't we set PlanID Here - it was working on 2017 !!!!!!!!!!!!!!!!
				result=m_sql.safe_query("INSERT INTO DeviceToPlansMap (DeviceRowID, DevSceneType, PlanID) VALUES ('%d', '%d', '%d')",
					i, 0, m_PlanID);
				/// We must set PlanID it via Update command
				result = m_sql.safe_query("UPDATE DeviceToPlansMap SET PlanID='%d' WHERE (DeviceRowID==%d)", m_PlanID,i);
				// For test
				//result = m_sql.safe_query("SELECT PlanID FROM DeviceToPlansMap WHERE (DeviceRowID==%d)", i);
				//_log.Log(LOG_ERROR,"PlanID %d - %s\r\n", m_PlanID, result[0][0].c_str());

				return		0;
			}
		}
	}
	else
	{
		return atoi(result[0][0].c_str());
	}
	return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////
//Programs for controllers - add Option field (combobox) - upto 10
//!!!!!!!!!!! Coded string for selector is not decoded on WWW display it was working on 2017
void eHouseTCP::UpdatePGM(int /*adrh*/, int /*adrl*/, int /*devtype*/, const char *names, int idx)
{
	if (idx < 0) return;
	std::string Names = ISO2UTF8
	(std::string(names));
	//_log.Log(LOG_ERROR, "PGM: %s", Names.c_str());
	m_sql.SetDeviceOptions(idx, m_sql.BuildDeviceOptions(Names, false));
}
///////////////////////////////////////////////////////////////////////////////////////////
//Add Controllers To 'Plans' DB
int eHouseTCP::UpdateSQLPlan(int /*devh*/, int /*devl*/, int /*devtype*/, const char * Name)
{
	int i = 0;
	std::string devname;
	std::vector<std::vector<std::string> > result;
	devname.append(Name, strlen(Name));
	devname = ISO2UTF8(devname);

	result = m_sql.safe_query("SELECT ID FROM Plans WHERE (Name=='%q') ", devname.c_str());

	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO Plans (Name) VALUES ('%q')", devname.c_str());
	}
	else
	{
		i = atoi(result[0][0].c_str());
		return i;

	}
	return -1;
}
///////////////////////////////////////////////////////////////////////////////////////////
// Update eHouse Controllers status in DeviceStatus Database
void eHouseTCP::UpdateSQLStatus(int devh, int devl, int /*devtype*/, int code, int nr, char /*signal*/, int nValue, const char  *sValue, int /*battery*/)
{
	char IDX[20];
	char state[5] = "";
	char szLastUpdate[40];
	time_t now = time(nullptr);
	struct tm ltime;
	localtime_r(&now, &ltime);
	sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	sprintf(IDX, "%02X%02X%02X%02X", devh, devl, code, nr);
	int lastlevel = 0;
	int _state;
	std::vector<std::vector<std::string> > result;

	switch (code)
	{
	case VISUAL_BLINDS:
		break;
	case VISUAL_DIMMER_OUT:
		_state = 0;
		sprintf(IDX, "%X%02X%02X%02X", devh, devl, code, nr);    //exception for dimmers Domoticz BUG?
		if (nValue == 0)
		{
			lastlevel = 0;
			_state = 0;
			strcpy(state, "0");
		}
		else
			if (nValue == 100)
			{
				lastlevel = 100;
				strcpy(state, "0");
				_state = 1;
			}
			else
			{
				lastlevel = nValue;
				_state = 2;
				sprintf(state, "%d", static_cast<uint8_t>((nValue * 15) / 100));
			}
		result = m_sql.safe_query("UPDATE DeviceStatus  SET nValue=%d, sValue='%q', LastLevel=%d, LastUpdate='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')",
			_state, state, lastlevel, szLastUpdate, m_HwdID, IDX);
		break;
	default:
		result = m_sql.safe_query("UPDATE DeviceStatus  SET nValue=%d, sValue='%s', LastUpdate='%q'  WHERE (HardwareID==%d) AND (DeviceID=='%q')",
			nValue, sValue, szLastUpdate, m_HwdID, IDX);
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////

eHouseTCP::eHouseTCP(const int ID, const std::string &IPAddress, const unsigned short IPPort, const std::string& userCode, const int pollInterval, const unsigned char AutoDiscovery, const unsigned char EnableAlarms, const unsigned char EnablePro, const int opta, const int optb) :
	m_modelIndex(-1),
	m_data32(false),
	m_socket(INVALID_SOCKET),
	m_IPPort(IPPort),
	m_IPAddress(IPAddress),
	m_pollInterval(pollInterval)
{
	m_eHouseUDPSocket = -1;			//UDP socket handler
	m_UDP_PORT = 6789;			//Default UDP PORT
	m_nr_of_ch = 0;
	m_DEBUG_AURA = 0;				//Debug Aura
	m_CHANGED_DEBUG = 0;
	m_disablers485 = 0;
	m_ProSize = 0;
	m_StatusDebug = 0;			//Log status reception
	m_IRPerform = 0;				//Perform InfraRed signals
	m_ViaCM = 0;					//eHouse RS-485 via CommManager
	memset(m_VendorCode, 0, sizeof(m_VendorCode));
	m_eHStatusReceived = 0;		//eHouse1 status received flag
	m_CloudStatusChanged = 0;							//data changed => must be updated
	m_COMMANAGER_IP_HIGH = 0;		//initial addresses of different controller types
	m_COMMANAGER_IP_LOW = 254;
	m_INITIAL_ADDRESS_LAN = 200;
	m_INITIAL_ADDRESS_WIFI = 100;
	m_UDP_terminate_listener = 0; //terminate udp listener service
	m_eHEStatusReceived = 0;      //Ethernet eHouse status received flag (count of status from reset this flag)
	m_eHWiFiStatusReceived = 0;   //eHouse WiFi status received flag (count of status from reset this flag)
	m_VccRef = 0;
	m_AdcRefMax = 0;
	m_CalcCalibration = 0;
	m_DEBUG_TCPCLIENT = 0;
	m_NoDetectTCPPack = 0;
	m_StatusDebug = 0;
	m_eHEnableAutoDiscovery = AutoDiscovery;
	m_eHEnableAlarmInputs = EnableAlarms;
	m_eHEnableProDiscovery = EnablePro;
	m_eHOptA = opta;
	m_eHOptB = optb;
	EhouseInitTcpClient();					//init multithreaded event sender
	if (IPPort > 0) {
		m_EHOUSE_TCP_PORT = IPPort;
	}
	m_ViaTCP = 0;
	if ((m_eHOptA & OPTA_CLR_DB))
	{
		//For Test of Auto Discovery Clean DeviceStatus & DeviceToPlansMap
		//Clear altered database
		//VERY DANGEROUS !
		_log.Debug(DEBUG_HARDWARE, "Clearing Device Databases");
		m_sql.safe_query("DELETE FROM \"DeviceToPlansMap\" WHERE 1");
		m_sql.safe_query("DELETE FROM \"DeviceStatus\" WHERE 1");
		m_sql.safe_query("DELETE FROM \"Plans\" WHERE (ID>1)");
	}
	if (m_eHOptA & OPTA_FORCE_TCP)
	{
		m_ViaTCP = 1;
	}
	if (m_eHOptA & OPTA_DEBUG)
	{
		m_StatusDebug = 1;
		m_DEBUG_AURA = 1;
		m_CHANGED_DEBUG = 1;
		m_DEBUG_TCPCLIENT = 1;
	}
	if (m_eHOptA & OPTA_DETECT_TCP_PACKETS)
	{
		m_NoDetectTCPPack = -1;
	}
	if (m_eHEnableAutoDiscovery)
	{
		_log.Debug(DEBUG_HARDWARE, "[eHouse] Auto Discovery %d\r\n", m_eHEnableAutoDiscovery);
	}
	if (m_eHEnableAlarmInputs)
	{
		_log.Debug(DEBUG_HARDWARE, "[eHouse] Enable Alarm Inputs %d\r\n", m_eHEnableAlarmInputs);
	}
	if (m_eHEnableProDiscovery)
	{
		_log.Debug(DEBUG_HARDWARE, "[eHouse] Enable PRO Discovery %d\r\n", m_eHEnableProDiscovery);
	}

	_log.Debug(DEBUG_HARDWARE, "[eHouse] Opts: %x,%x\r\n", m_eHOptA, m_eHOptB);
	int len = userCode.length();
	if (len > 6) len = 6;
	userCode.copy(m_PassWord, len);
	m_SrvAddrH = 0;
	m_SrvAddrL = 200;
	m_SrvAddrU = 192;
	m_SrvAddrM = 168;
	InitStructs();
	if (!CheckAddress())
	{
		//return false;
	}

	_log.Debug(DEBUG_HARDWARE, "eHouse UDP/TCP: Create instance");
	m_EventsCountInQueue = 0;
	m_HwdID = ID;
	m_HwID = m_HwdID;
	memset(m_newData, 0, sizeof(m_newData));
	m_AddrL = m_SrvAddrL;
	m_AddrH = m_SrvAddrH;
	for (auto &e : m_EvQ)
	{
		e = (struct EventQueueT *)calloc(1, sizeof(struct EventQueueT));
		if (e == nullptr)
		{
			LOG(LOG_ERROR, "Can't Alloc Events Queue Memory");
			return;
		}
	}

	eHPROaloc(0, m_AddrH, m_AddrL);
	unsigned char ev[10] = "";
	if ((m_SrvAddrU == 192) && (m_SrvAddrM == 168))	//local network LAN IP 192.168.x.y
	{
		ev[0] = m_AddrH;
		ev[1] = m_AddrL;
	}
	else										//Via Internet, Intranet, ETC
	{
		ev[0] = 0;
		ev[1] = 0;
	}
	ev[2] = 254;
	ev[3] = 0x33;
	int nr = -1;
	if (m_eHEnableAutoDiscovery) nr = AddToLocalEvent(ev, 0);  //Init UDP broadcast of Device Names for auto Discovery
	if (nr >= 0)
		m_EvQ[nr]->LocalEventsTimeOuts = 200U;
	m_alarmLast = false;
}
//////////////////////////////////////////////////////////////////////
eHouseTCP::~eHouseTCP()
{
	_log.Debug(DEBUG_HARDWARE, "eHouse: Destroy instance");
}
/////////////////////////////////////////////////////////////////////////////
bool eHouseTCP::StartHardware()
{
	RequestStart();

#ifdef UDP_USE_THREAD
	ThEhouseUDPdta.No = 1;
	ThEhouseUDPdta.IntParam = UDP_PORT;		//udp thread setup
#endif

#ifdef DEBUG_eHouse
	_log.Debug(DEBUG_HARDWARE, "eHouse: Start Hardware");
#endif

#ifdef UDP_USE_THREAD
	//please use normal std::thread!
	thread_create(
		&ThEhouseUDP, nullptr, UDPListener,
		(void *)&ThEhouseUDPdta); // for eHouse4ethernet or eHouse1 under CommManager supervision, eHouse Pro server in main thread
#else

#endif
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	StartHeartbeatThread();
	return (m_thread != nullptr);
}
////////////////////////////////////////////////////////////////////////////////////
bool eHouseTCP::StopHardware()
{

	//#ifdef DEBUG_eHouse
	LOG(LOG_STATUS, "eHouse: Stop hardware");
	//#endif
	TerminateUDP();
	sleep_seconds(1);

	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}

	//        DestroySocket();
	m_bIsStarted = false;
	StopHeartbeatThread();
	return true;
}
///////////////////////////////////////////////////////////////////////////////////
int eHouseTCP::ConnectTCP(unsigned int IP)
{
	unsigned char challange[30];
#ifndef WIN32
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;	//100ms for delay
	if (m_eHEnableProDiscovery) timeout.tv_sec = 10;
	if (m_eHEnableAutoDiscovery) timeout.tv_sec = 30;
#else
	unsigned int	timeout = 200;	//ms for TCPIP
	if (m_eHEnableProDiscovery)	timeout = 10000;
	if (m_eHEnableAutoDiscovery)	timeout = 20000;
#endif

	struct sockaddr_in server;
	struct sockaddr_in saddr;
	int TCPSocket = -1;
	saddr.sin_family = AF_INET;							//initialization of protocol & socket
	if (IP > 0)
		saddr.sin_addr.s_addr = IP;
	else
		saddr.sin_addr.s_addr = m_addr.sin_addr.s_addr;
	saddr.sin_port = htons((uint16_t)m_EHOUSE_TCP_PORT);
	memset(&server, 0, sizeof(server));               //clear server structure
	memset(&challange, 0, sizeof(challange));         //clear buffer
	char line[20];
	int status;
	sprintf(line, "%d.%d.%d.%d", saddr.sin_addr.s_addr & 0xff, (saddr.sin_addr.s_addr >> 8) & 0xff, (saddr.sin_addr.s_addr >> 16) & 0xff, (saddr.sin_addr.s_addr >> 24) & 0xff);
	TCPSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (TCPSocket < 0)       //Check if socket was created
	{
		closesocket(TCPSocket);
		_log.Log(LOG_ERROR, "[TCP Client Status] Could not create socket");
		return -1;                              //!!!! Counldn't Create Socket
	}
	server.sin_addr.s_addr = m_addr.sin_addr.s_addr;
	server.sin_family = AF_INET;                    //tcp v4
	server.sin_port = htons((uint16_t)m_EHOUSE_TCP_PORT);       //assign eHouse Port
	_log.Log(LOG_STATUS, "[TCP Cli Status] Trying Connecting to: %s", line);
	if (connect(TCPSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		closesocket(TCPSocket);
		_log.Log(LOG_ERROR, "[TCP Cli Status] error connecting: %s", line);
		return -1;                              //!!!! Counldn't Create Socket
	}
	_log.Log(LOG_STATUS, "[TCP Cli Status] Authorizing");
	int iter = 5;
	while ((status = recv(TCPSocket, (char *)&challange, 6, 0)) < 6)       //receive challenge code
	{
		if ((status < 0) || (!(iter--)))
		{
			_log.Log(LOG_ERROR, "[TCP Cli Status] error connecting: %s", line);
			closesocket(TCPSocket);
			return -1;                              //!!!! Counldn't Create Socket
		}
	}
	if (status == 6)	//challenge received from Ethernet eHouse controllers
	{				// only Hashed password with VendorCode available for OpenSource
		challange[6] = (challange[0] ^ m_PassWord[0] ^ m_VendorCode[0]);
		challange[7] = (challange[1] ^ m_PassWord[1] ^ m_VendorCode[1]);
		challange[8] = (challange[2] ^ m_PassWord[2] ^ m_VendorCode[2]);
		challange[9] = (challange[3] ^ m_PassWord[3] ^ m_VendorCode[3]);
		challange[10] = (challange[4] ^ m_PassWord[4] ^ m_VendorCode[4]);
		challange[11] = (challange[5] ^ m_PassWord[5] ^ m_VendorCode[5]);
		challange[12] = 13;
		challange[13] = 0;
	}
	else
	{
		closesocket(TCPSocket);
		TerminateUDP();
		return -1;
	}
	status = 0;
	iter = 5;
	while ((status = send(TCPSocket, (char *)&challange, 13, 0)) != 13)
	{
		if ((!(iter--)) || (status < 0))
		{
			_log.Log(LOG_ERROR, "[TCP Cli Status] error sending chalange to: %s", line);
			closesocket(TCPSocket);
			return -1;                              //!!!! Counldn't Create Socket
		}
	}

	//Send Challange + response + Events    - Only Xor password
	status = 0;
	iter = 5;
	while ((status = recv(TCPSocket, (char *)&challange, 1, 0)) < 1)       //receive challenge code
	{
		if ((status < 0) || (!(iter--)))
		{
			_log.Log(LOG_STATUS, "[TCP Cli Status] Receiving confirmation: %s", line);
			closesocket(TCPSocket);
			return -1;
		}
	}
	//_log.Log(LOG_STATUS,"Confirmation: %c", challange[0]);
	challange[0] = 1;
	while ((status = send(TCPSocket, (char *)&challange, 1, 0)) != 1)
	{
		if ((!(iter--)) || (status < 0))
		{
			_log.Log(LOG_ERROR, "[TCP Cli Status] error query: %s", line);
			closesocket(TCPSocket);
			return -1;
		}
	}
	_log.Log(LOG_STATUS, "Connected OK");
	/*
	iMode = 1;
	status = ioctlsocket(TCPSocket, FIONBIO, &iMode);
	if (status == SOCKET_ERROR)
		{
		_log.Log(LOG_STATUS,"ioctlsocket failed with error: %d\n", WSAGetLastError());
		closesocket(TCPSocket);
		//WSACleanup();
		return -1;
		}*/
		//after connection change timeout
		//after connection change

	if (setsockopt(TCPSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)   //Set socket Read operation Timeout
	{
		LOG(LOG_ERROR, "[TCP Client Status] Set Read Timeout failed");
		perror("[TCP Client Status] Set Read Timeout failed\n");
	}

	if (setsockopt(TCPSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)   //Set Socket Write operation Timeout
	{
		LOG(LOG_ERROR, "[TCP Client Status] Set Write Timeout failed");
		perror("[TCP Client Status] Set Write Timeout failed\n");
	}

	//TCP_NODELAYACK
	/*opt = 1;

	if (setsockopt(TCPSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
		{
		LOG(LOG_ERROR, "[TCP Cli Status] Set TCP No Delay failed");
		perror("[TCP Cli Status] Set TCP No Delay failed\n");
		}

		*/
	return TCPSocket;
}





//////////////////////////////////////////////////////////////////////////////////
bool eHouseTCP::CheckAddress()
{
	if (m_IPAddress.empty() || m_IPPort < 1 || m_IPPort > 65535)
	{
		LOG(LOG_ERROR, "eHouse: Empty IP Address or bad Port");
		return false;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_IPPort);
	unsigned long ip;
	ip = inet_addr(m_IPAddress.c_str());
	if (ip != INADDR_NONE)
	{
		m_addr.sin_addr.s_addr = ip;
		m_SrvAddrU = ip & 0xff;
		m_SrvAddrM = (ip >> 8) & 0xff;
		m_SrvAddrL = ip >> 24;
		m_SrvAddrH = (ip >> 16) & 0xff;
		LOG(LOG_STATUS, "[eHouse PRO] IP Address: %d.%d.%d.%d\r\n", m_SrvAddrU, m_SrvAddrM, m_SrvAddrH, m_SrvAddrL);
		if ((m_SrvAddrU != 192) || (m_SrvAddrM != 168))
			m_ViaTCP = 1;
	}
	else
	{
		hostent *he = gethostbyname(m_IPAddress.c_str());
		if (he == nullptr)
		{
			LOG(LOG_ERROR, "eHouse: cannot resolve host name");
			return false;
		}
		memcpy(&(m_addr.sin_addr), he->h_addr_list[0], 4);
		m_SrvAddrU = ip & 0xff;
		m_SrvAddrM = (ip >> 8) & 0xff;
		m_SrvAddrL = ip >> 24;
		m_SrvAddrH = (ip >> 16) & 0xff;
		LOG(LOG_STATUS, "[eHouse PRO] %s =>IP Address: %d.%d.%d.%d\r\n", m_IPAddress.c_str(), m_SrvAddrU, m_SrvAddrM, m_SrvAddrH, m_SrvAddrL);
		if ((m_SrvAddrU != 192) || (m_SrvAddrM != 168))
			m_ViaTCP = 1;
	}
	if (m_ViaTCP)
		m_TCPSocket = ConnectTCP(m_addr.sin_addr.s_addr);
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::DestroySocket()
{
	if (m_socket != INVALID_SOCKET)
	{
#ifdef DEBUG_eHouse
		_log.Debug(DEBUG_HARDWARE, "eHouse: destroy socket");
#endif
		try
		{
			closesocket(m_socket);
		}
		catch (...)
		{
		}

		m_socket = INVALID_SOCKET;
	}
}
///////////////////////////////////////////////////////////////////////////////////////
// Get ERM Programs Scenes, Measurement-regulation
int  eHouseTCP::getrealERMpgm(int32_t ID, int level)
{
	uint8_t devh = ID >> 24;
	uint8_t devl = (ID >> 16) & 0xff;
	uint8_t code = (ID >> 8) & 0xff;
	int i;
	int lv = level / 10;
	lv += 1;
	int Lev = 0;
	unsigned char ev[10];
	memset(ev, 0, sizeof(ev));
	ev[0] = devh;
	ev[1] = devl;
	gettype(devh, devl);
	if (m_Dtype != EH_LAN) return -1;
	LOG(LOG_STATUS, "LAN PGM");
	int index = devl - m_INITIAL_ADDRESS_LAN;
	if ((m_Dsubtype < 249))  //ERMs Only =>No PoolManager/CommManager/LevelManager
	{
		switch (code)
		{
		case VISUAL_PGM:
			i = 0;
			for (const auto &program : m_eHEn[index]->Programs)
			{
				if ((strlen(program) > 0) && (strstr(program, "@") == nullptr))
				{
					Lev++;
				}
				if (Lev == lv)
				{
					_log.Debug(DEBUG_HARDWARE, "[EX] Execute pgm %d", i);
					ev[2] = 2;//exec program/scene
					ev[3] = (unsigned char)i;
					AddToLocalEvent(ev, 0);
					return i;
				}
				++i;
			}
			break;
		case VISUAL_APGM:
			i = 0;
			for (const auto &adc : m_eHEn[index]->ADCPrograms)
			{
				if ((strlen(adc) > 0) && (strstr(adc, "@") == nullptr))
				{
					Lev++;
				}
				if (Lev == lv)
				{
					_log.Debug(DEBUG_HARDWARE, "[EX] Execute ADC pgm %d", i);
					ev[2] = 97;//exec ADC program/scene
					ev[3] = (unsigned char)i;
					AddToLocalEvent(ev, 0);
					return i;
				}
				++i;
			}

			break;

		}
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
//Get RoomManager Programs (Scenes)
int  eHouseTCP::getrealRMpgm(int32_t ID, int level)
{
	uint8_t devh = ID >> 24;
	uint8_t devl = (ID >> 16) & 0xff;
	uint8_t code = (ID >> 8) & 0xff;
	int i = 0;
	int lv = level / 10;
	lv += 1;
	int Lev = 0;
	unsigned char ev[10];
	memset(ev, 0, sizeof(ev));
	ev[0] = devh;
	ev[1] = devl;
	gettype(devh, devl);
	if (m_Dtype != EH_RS485) return -1;
	int index = devl;
	if (devh == 1) index = 0;
	if (devh == 2) index = STATUS_ARRAYS_SIZE;
	{
		switch (code)
		{
		case VISUAL_PGM:
			for (const auto &eHn : m_eHn[index]->Programs)
			{
				if ((strlen(eHn) > 0) && (strstr(eHn, "@") == nullptr))
				{
					Lev++;
				}
				if (Lev == lv)
				{
					ev[2] = 2;//exec program/scene
					if (index == 0) ev[2] = 0xaa;
					if (index == STATUS_ARRAYS_SIZE)  ev[2] = 2;

					ev[3] = (unsigned char)i;
					AddToLocalEvent(ev, 0);
					return i;
				}
				i++;
			}
			break;
			/*case VISUAL_APGM:
				for (i=0;i<(sizeof(eHEn[index].ADCPrograms)/sizeof(eHEn[index].ADCPrograms[0]));i++)
					{
					if ((strlen(eHEn[index].ADCPrograms[i])>0) && (strstr(eHEn[index].ADCPrograms[i],"@")==nullptr))
						{
						Lev++;
						}
					if (Lev==lv)
						{
						ev[2]=97;//exec ADC program/scene
						ev[3]=(unsigned char) i;
						AddToLocalEvent(ev,0);
	  //                  printf("PRG: lev=%d, lv=%d , %d\r\n",Lev,lv,i+1);
						return i;
						}
					}

					break;
*/
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Create eHouse events
bool eHouseTCP::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{

	const tRBUF *output = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char ev[10], proev[10];
	char IDX[20];
	char tmp[20];
	uint8_t  cmd;
	uint32_t id;
	uint8_t AddrH;
	uint8_t AddrL;
	uint8_t eHCMD;
	uint8_t nr;

	memset(ev, 0, sizeof(ev));
	memset(proev, 0, sizeof(proev));
	if ((output->ICMND.packettype == pTypeGeneralSwitch) && (output->ICMND.subtype == sSwitchTypeSelector))
	{
		//_log.Debug(DEBUG_HARDWARE, "SW - Type Selector\r\n");
		_tGeneralSwitch *xcmd = (_tGeneralSwitch*)pdata;
		int32_t ID = xcmd->id;
		int level = xcmd->level;
		// For both controller types ERM, RM
		id = getrealERMpgm(ID, level);
		id = getrealRMpgm(ID, level);
	}

	if ((output->ICMND.packettype == pTypeThermostat)
		&& (output->ICMND.subtype == sTypeThermSetpoint))
	{
		const _tThermostat *therm = reinterpret_cast<const _tThermostat*>(pdata);
		AddrH = therm->id1;
		AddrL = therm->id2;
		cmd = therm->id3;
		nr = therm->id4;

		float temp = therm->temp;
		int ttemp = (int)(temp * 10);

		ev[0] = AddrH;
		ev[1] = AddrL;
		ev[2] = 245U;		// MODADC
		ev[3] = nr;		//starting channel arg1
		ev[4] = 3;		//arg2

		gettype(AddrH, AddrL);
		sprintf(IDX, "%02X%02X%02X%02X", therm->id1, therm->id2, therm->id3, therm->id4);
		if ((m_Dtype == EH_LAN) || (m_Dtype == EH_WIFI))
		{
			unsigned int adcvalue = (int)(1023.0 * ((temp + 50.0) / 330.0));  //mcp9700 10mv/c offset -50
			adcvalue -= 2;
			ev[5] =(uint8_t)( adcvalue >> 8);      //arg3
			ev[6] = adcvalue & 0xff;    //arg4
			adcvalue += 4;
			ev[7] = (uint8_t)(adcvalue >> 8);      //arg5
			ev[8] = adcvalue & 0xff;    //arg6
			AddToLocalEvent(ev, 0);
			sprintf(tmp, "%.1f", temp);
			UpdateSQLStatus(AddrH, AddrL, m_Dtype, VISUAL_MCP9700_PRESET, nr, 100, ttemp, tmp, 100);
		}

		if (m_Dtype == EH_AURA)
		{
			unsigned int adcvalue = (int)round(temp);
			ev[3] = 0;	//nr ==0
			ev[4] = 3;	//set value
			ev[5] = (uint8_t)(adcvalue / 10);
			ev[6] = adcvalue % 10;
			adcvalue += 5;
			ev[7] = (uint8_t)(adcvalue / 10);
			ev[8] = adcvalue % 10;
			m_AuraDev[AddrL - 1]->ServerTempSet = temp;
			AddToLocalEvent(ev, 0);
			sprintf(tmp, "%.1f", temp);
			UpdateSQLStatus(AddrH, AddrL, EH_AURA, VISUAL_AURA_PRESET, 1, 100, ttemp, tmp, 100);
		}
	}



	if ((output->ICMND.packettype == pTypeColorSwitch) && (output->LIGHTING2.subtype == sTypeColor_RGB_W))
	{
		const _tColorSwitch *pLed = reinterpret_cast<const _tColorSwitch *>(pdata);
		id = pLed->id;
		cmd = pLed->command;
		if (cmd == Color_SetColor)
		{
			if (pLed->color.mode == ColorModeRGB)
			{
				AddrH = id >> 24;              //address high
				AddrL = (id >> 16) & 0xff;     //address low
				gettype(AddrH, AddrL);
				eHCMD = (id >> 8) & 0xff;     //ehouse visual code
				nr = id & 0xff;              // i/o  nr
				//Compose eHouse Event
				ev[0] = AddrH;
				ev[1] = AddrL;
				ev[2] = 4;					//SET DIMMER
				ev[3] = nr;					//starting channel

/////////////!!!!!!! check 255
//Scale domoticz 0..100 scale for eHouse 0-255
				ev[4] = (uint8_t)(pLed->color.r * pLed->value * 255 / 100);
				ev[5] = (uint8_t)(pLed->color.g * pLed->value * 255 / 100);
				ev[6] = (uint8_t)(pLed->color.b * pLed->value * 255 / 100);
				AddToLocalEvent(ev, 0);
			}
			else
			{
				_log.Log(LOG_STATUS, "eHouse TCP: SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
				return false;
			}
		}
		return true;
	}
	if ((output->ICMND.packettype == pTypeLighting2) && (output->LIGHTING2.subtype == sTypeAC))
	{
		AddrH = output->LIGHTING2.id1;
		AddrL = output->LIGHTING2.id2;
		cmd = output->LIGHTING2.id3;
		nr = output->LIGHTING2.id4;
		gettype(AddrH, AddrL);
		proev[0] = ev[0] = AddrH;
		proev[1] = ev[1] = AddrL;
		proev[5] = 0;
		proev[8] = 2;
		proev[9] = 2;
		gettype(AddrH, AddrL);
		if (cmd == VISUAL_BLINDS)
		{
			proev[2] = ev[2] = VISUAL_BLINDS;
			proev[3] = ev[3] = nr;
			if (output->LIGHTING2.cmnd == 0)		//open
			{
				proev[4] = ev[4] = 1;
				proev[6] = ev[5] = 90;
			}
			if (output->LIGHTING2.cmnd == 1)		//close
			{
				proev[4] = ev[4] = 2;
				proev[6] = ev[5] = 90;
			}
			else
				if (output->LIGHTING2.cmnd == 2)
				{
					sprintf(IDX, "%X%02X%02X%02X", output->LIGHTING2.id1, output->LIGHTING2.id2, output->LIGHTING2.id3, output->LIGHTING2.id4);
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT nValue, sValue, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
						m_HwdID, IDX, AddrL);
					if (!result.empty())
					{
						int nvalue = atoi(result[0][0].c_str());
						int svalue = atoi(result[0][1].c_str());
						//int lastLevel = atoi(result[0][2].c_str());
						if (nvalue == 0)
						{
							proev[4] = ev[4] = 2;
							proev[6] = ev[5] = output->LIGHTING2.level * 2;
						}
						else
						{
							if (nvalue == 1)
							{
								proev[4] = ev[4] = 1;
								proev[6] = ev[5] = 30 - output->LIGHTING2.level * 2;
							}
							else
							{
								if (svalue < output->LIGHTING2.level)
								{
									proev[4] = ev[4] = 2;
									ev[5] = (uint8_t)(output->LIGHTING2.level - svalue);
									ev[5] *= 2;
									proev[6] = ev[5];
								}
								else
								{
									proev[4] = ev[4] = 1;
									ev[5] = (uint8_t)(svalue - output->LIGHTING2.level);
									ev[5] *= 2;
									proev[6] = ev[5];
								}
							}
						}
					}


				}
			//                  if (ev[4]==1) printf("\r\nopening %d\r\n",ev[5]);
			//                  if (ev[4]==2) printf("\r\nclosing %d\r\n",ev[5]);
			if (m_Dtype != EH_PRO)
			{
				//deb((char *)&"EV: ",ev,sizeof(ev));
				AddToLocalEvent(ev, 0);
			}
			else
			{
				//deb((char *)&"PRO: ",proev,sizeof(proev));
				AddToLocalEvent(proev, 0);
			}
			return true;
		}

		if (cmd == 0x7e)       //dimmer event
		{
			ev[2] = 105;  //Modify Dimmer
			ev[3] = nr + 1; //dimmer number

			if (output->LIGHTING2.cmnd == light2_sOn)   //increment to max
			{
				ev[4] = 1;
				ev[5] = 10;
			}
			else
				if (output->LIGHTING2.cmnd == light2_sOff)  //decrement to min
				{
					ev[4] = 0;
					ev[5] = 10;
				}
				else                                        //set value
				{
					ev[4] = 3;
					ev[5] = (output->LIGHTING2.level * 255) / 15;
				}
			AddToLocalEvent(ev, 0);
			return true;
		}
		return true;

	}
	////////////////////////////////////////////////////////////////////////////////
	if ((output->ICMND.packettype == pTypeBlinds))
	{
		AddrH = output->BLINDS1.id1;
		AddrL = output->BLINDS1.id2;
		cmd = output->BLINDS1.id3;
		nr = output->BLINDS1.id4;
		memset(proev, 0, sizeof(proev));
		proev[0] = ev[0] = AddrH;
		proev[1] = ev[1] = AddrL;
		proev[8] = 2;
		proev[9] = 2;
		//printf("ROL: %d, %d\r\n",output->BLINDS1.unitcode,cmd);
		if (cmd == VISUAL_BLINDS)
		{
			proev[2] = ev[2] = VISUAL_BLINDS;
			proev[3] = ev[3] = nr + 1;
			proev[4] = ev[4] = output->BLINDS1.unitcode;
			//printf("ROL: %d, %d\r\n",output->LIGHTING2.level,cmd);
			if (m_Dtype != EH_PRO)
			{
				AddToLocalEvent(ev, 0);
				//deb((char *)&"EV: ",ev,sizeof(ev));
			}
			else
			{
				//deb((char *)&"PRO: ",proev,sizeof(proev));
				AddToLocalEvent(proev, 0);
			}
		}
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////

	if ((output->ICMND.packettype == pTypeGeneralSwitch) && (output->ICMND.subtype == sSwitchTypeAC))
	{
		const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		//uint32_t evt=output->_tGeneralSwitch.id;
		id = general->id;
		cmd = general->cmnd;
		AddrH = id >> 24;              //address high
		AddrL = (id >> 16) & 0xff;     //address low
		eHCMD = (id >> 8) & 0xff;     //ehouse visual code
		nr = id & 0xff;              // i/o  nr
		ev[0] = AddrH;
		ev[1] = AddrL;
		gettype(AddrH, AddrL);

		switch (eHCMD)
		{
		case 0x21:
			ev[2] = 0x21;
			if (m_Dtype == EH_RS485) ev[2] = 0x01;
			ev[3] = nr;
			if (cmd == gswitch_sOn)
			{
				ev[4] = 1;
			}
			else
			{
				ev[4] = 0;
			}
			//(cmd=gwswitch_sOff);
			AddToLocalEvent(ev, 0);
			break;


		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////
// Windows CP1250 => utf8
std::string eHouseTCP::ISO2UTF8(const std::string &name)
{
	char cp1250[] = "\xB9\xE6\xEA\xB3\xF1\xF3\x9C\x9F\xBF\xA5\xC6\xCA\xA3\xD1\xD3\x8C\x8F\xAF";
	char utf8[] = "\xC4\x85\xC4\x87\xC4\x99\xC5\x82\xC5\x84\xC3\xB3\xC5\x9B\xC5\xBA\xC5\xBC\xC4\x84\xC4\x86\xC4\x98\xC5\x81\xC5\x83\xC3\x93\xC5\x9A\xC5\xB9\xC5\xBB";

	std::string UTF8Name;
	for (char i : name)
	{
		bool changed = false;
		for (int j = 0; j < sizeof(cp1250); ++j)
		{
			if (i == cp1250[j])
			{
				UTF8Name += utf8[j * 2];
				UTF8Name += utf8[j * 2 + 1];
				changed = true;
				break;
			}
		}
		if (!changed)
		{
			UTF8Name += i;
		}
	}
	return UTF8Name;
}
