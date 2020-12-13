#pragma once

// implememtation for eHouse Home Automation System : (eHouse LAN, WiFi, PRO + indirectly RS-485, CAN, RF, AURA, RFID)
// DIY EN: http://smart.ehouse.pro/
// DIY PL: http://idom.ehouse.pro/
// Shop : http://eHouse.Biz/
// by Robert Jarzabek, iSys - Intelligent systems
// Recent update : 2018 - 08 - 16
#include "eHouse/globals.h"

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class eHouseTCP : public CDomoticzHardwareBase

{
      public:
	eHouseTCP(int ID, const std::string &IPAddress, unsigned short IPPort, const std::string &userCode, int pollInterval, unsigned char AutoDiscovery, unsigned char EnableAlarms,
		  unsigned char EnablePro, int opta, int optb);
	~eHouseTCP() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	int ConnectTCP(unsigned int ip);
	void AddTextEvents(unsigned char *ev, int size);		       // Add hex coded string with eHouse events/codes
	signed int AddToLocalEvent(unsigned char *Even, unsigned char offset); // Add binary coded event from buffer
	struct CtrlADCT *(m_adcs[MAX_AURA_DEVS]);
	signed int IndexOfeHouseRS485(unsigned char devh, unsigned char devl);
	void CalculateAdcWiFi(unsigned char index);
	char eH1(unsigned char addrh, unsigned char addrl);
	void InitStructs();

	union ERMFullStatT *(m_eHERMs[ETHERNET_EHOUSE_RM_MAX + 1]);    // full ERM status decoded
	union ERMFullStatT *(m_eHERMPrev[ETHERNET_EHOUSE_RM_MAX + 1]); // full ERM status decoded previous for detecting changes

	union ERMFullStatT *(m_eHRMs[EHOUSE1_RM_MAX + 1]);    // full RM status decoded
	union ERMFullStatT *(m_eHRMPrev[EHOUSE1_RM_MAX + 1]); // full RM status decoded previous for detecting changes

	struct EventQueueT *(
		m_EvQ[EVENT_QUEUE_MAX]); // eHouse event queue for submit to the controllers (directly LAN, WiFi, PRO / indirectly via PRO other variants) - multiple events can be executed at once
	struct AURAT *(m_AuraDev[MAX_AURA_DEVS]);    // Aura status thermostat
	struct AURAT *(m_AuraDevPrv[MAX_AURA_DEVS]); // previous for detecting changes
	struct AuraNamesT *(m_AuraN[MAX_AURA_DEVS]);

	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool CheckAddress();
	// Closes socket
	void DestroySocket();

	std::string ISO2UTF8(const std::string &name);
	// dynamically allocate memories for controllers structures
	void eCMaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHPROaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eAURAaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHEaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHWIFIaloc(int eHEIndex, int devaddrh, int devaddrl);
	unsigned char IsCM(unsigned char addrh, unsigned char addrl);
	// signed int IndexOfeHouseRS485(unsigned char devh, unsigned char devl);
	void UpdateAuraToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdateCMToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdateLanToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdatePROToSQL(unsigned char AddrH, unsigned char AddrL);
	void UpdateWiFiToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdateRS485ToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void TerminateUDP();
	void IntToHex(unsigned char *buf, const unsigned char *inbuf, int received);
	float getAdcVolt2(int index);
	void CalculateAdc2(unsigned char index);
	void CalculateAdcEH1(unsigned char index);
	void deb(char *prefix, unsigned char *dta, int size);
	void GetStr(unsigned char *GetNamesDta);
	void GetUDPNamesRS485(unsigned char *data, int nbytes);
	int gettype(int adrh, int adrl);
	void GetUDPNamesLAN(unsigned char *data, int nbytes);
	void GetUDPNamesCM(unsigned char *data, int nbytes);
	void GetUDPNamesPRO(unsigned char *data, int nbytes);
	void GetUDPNamesWiFi(unsigned char *data, int nbytes);
	int UpdateSQLState(int devh, uint8_t devl, int devtype, uint8_t type, uint8_t subtype, int swtype, int code, int nr, uint8_t signal, int nValue, const char *sValue, const char *Name,
			   const char *SignalName, bool on_off, uint8_t battery, int m_PlanID);
	void UpdateSQLStatus(int devh, int devl, int devtype, int code, int nr, char signal, int nValue, const char *sValue, int battery);
	int UpdateSQLPlan(int devh, int devl, int devtype, const char *Name);
	void UpdatePGM(int adrh, int adrl, int devtype, const char *names, int idx);
	signed int IndexOfEthDev(unsigned char AddrH, unsigned char AddrL);
	//	signed int GetIndexOfWiFiDev(unsigned char AddrH, unsigned char AddrL);
	void EhouseInitTcpClient();
	char SendTCPEvent(const unsigned char *Events, unsigned char EventCount, unsigned char AddrH, unsigned char AddrL, const unsigned char *EventsToRun);
	void performTCPClientThreads();
	int getrealERMpgm(int32_t ID, int level);
	int getrealRMpgm(int32_t ID, int level);
	void ExecEvent(unsigned int i);
	signed int GetIndexOfEvent(unsigned char *TempEvent);
	void ExecQueuedEvents();
	signed int hex2bin(const unsigned char *st, int offset);
	char SubmitEvent(const unsigned char *Events, unsigned char EventCount);
	void EhouseSubmitData(int SocketIndex);
	void eHType(int devtype, char *dta);

      private:
	unsigned char m_eHEnableAutoDiscovery; // enable eHouse Controllers Auto Discovery
	unsigned char m_eHEnableProDiscovery;  // enable eHouse PRO Discovery
	unsigned char m_eHEnableAlarmInputs;   // Future - Alarm inputs
	char m_NoDetectTCPPack;
	unsigned int m_eHOptA; // Admin options
	unsigned int m_eHOptB; // Admin options

	// Variables stored dynamically added during status reception (should be added sequentially)
	union WiFiStatusT *(m_eHWiFi[EHOUSE_WIFI_MAX + 1]);
	struct CommManagerNamesT *m_ECMn;
	union CMStatusT *m_ECM;
	union CMStatusT *m_ECMPrv; // Previous statuses for Update MSQL optimalization  (change data only updated)
	struct eHouseProNamesT *m_eHouseProN;
	union eHouseProStatusUT *m_eHouseProStatus;
	union eHouseProStatusUT *m_eHouseProStatusPrv;

#ifndef REMOVEUNUSED
	CANStatus m_eHCAN[EHOUSE_RF_MAX];
	CANStatus m_eHCANRF[EHOUSE_RF_MAX];
	CANStatus m_eHCANPrv[EHOUSE_RF_MAX];
	CANStatus m_eHCANRFPrv[EHOUSE_RF_MAX];

	eHouse1Status m_eHPrv[EHOUSE1_RM_MAX];
	CMStatus m_eHEPrv[ETHERNET_EHOUSE_RM_MAX + 1];
	WiFiStatus m_eHWiFiPrv[EHOUSE_WIFI_MAX + 1];
#endif

	union WIFIFullStatT *(m_eHWIFIs[EHOUSE_WIFI_MAX + 1]);	  // full wifi status
	union WIFIFullStatT *(m_eHWIFIPrev[EHOUSE_WIFI_MAX + 1]); // full wifi status previous for detecting changes

#ifndef REMOVEUNUSED
	WIFIFullStat m_eHCANPrev[EHOUSE_CAN_MAX];
	WIFIFullStat m_eHRFPrev[EHOUSE_RF_MAX];
	WIFIFullStat m_eHCANs[EHOUSE_CAN_MAX];
	WIFIFullStat m_eHRFs[EHOUSE_RF_MAX];
#endif
	struct eHouse1NamesT *(m_eHn[EHOUSE1_RM_MAX + 1]);		   // names of i/o for rs-485 controllers
	struct EtherneteHouseNamesT *(m_eHEn[ETHERNET_EHOUSE_RM_MAX + 1]); // names of i/o for Ethernet controllers
	struct WiFieHouseNamesT *(m_eHWIFIn[EHOUSE_WIFI_MAX + 1]);	   // names of i/o for WiFi controllers

#ifndef REMOVEUNUSED
	eHouseCANNames m_eHCANn[EHOUSE_RF_MAX + 1];
	eHouseCANNames m_eHCANRFn[EHOUSE_RF_MAX + 1];
	SatelNames m_SatelN[MAX_SATEL];
	SatelStatus m_SatelStat[MAX_SATEL];
#endif

	unsigned char m_COMMANAGER_IP_HIGH; // default CommManager Ip addr h
	unsigned char m_COMMANAGER_IP_LOW;  // default CommManager Ip addr l
	unsigned char m_EHOUSE_PRO_HIGH;    // default eHouse Pro Server IP addr h
	unsigned char m_EHOUSE_PRO_LOW;	    // default eHouse Pro Server IP addr l
	char m_VendorCode[6];
	int m_TCPSocket;
	unsigned char m_DEBUG_TCPCLIENT;
	unsigned char m_EHOUSE_TCP_CLIENT_TIMEOUT;   // Tcp Client operation timeout Connect/send/receive
	unsigned int m_EHOUSE_TCP_CLIENT_TIMEOUT_US; // Tcp Client operation timeout Connect/send/receive
	int m_EHOUSE_TCP_PORT;

	float m_VccRef;
	int m_AdcRefMax;
	float m_CalcCalibration;
	char m_GetLine[SIZEOFTEXT]; // global variable for decoding names discovery
	unsigned int m_GetIndex, m_GetSize;
	int m_HeartBeat;
	// unsigned int GetIndex, GetSize;
	// int HeartBeat;

	uint8_t m_AddrH, m_AddrL; // address high & low for controller type detection & construct idx
	int m_modelIndex;
	bool m_data32;
	sockaddr_in m_addr;
	int m_socket;
	const unsigned short m_IPPort; // 9876;    default port
	const std::string m_IPAddress; // "192.168.0.200"; - default eHouse PRO srv address
	int m_pollInterval;
	std::shared_ptr<std::thread> m_thread;
	std::shared_ptr<std::thread> m_EhouseTcpClientThread[MAX_CLIENT_SOCKETS];
	unsigned char m_newData[7];
	unsigned char m_DisablePerformEvent;

	unsigned char m_userCode[8]; // password to eHouse 6 ascii chars

	std::mutex m_mutex;
	bool m_alarmLast;
	char m_ViaTCP;	       // Statuses via TCP/IP connection
	int m_HwID;	       // Domoticz Hardware ID
	int m_eHouseUDPSocket; // UDP socket handler
	int m_UDP_PORT;	       // Default UDP PORT
	unsigned char m_nr_of_ch;
	char m_DEBUG_AURA;		   // Debug Aura
	char m_CHANGED_DEBUG;		   // Display changes signals (devices) on
	unsigned int m_EventsCountInQueue; // Events In queue count to bypass processing EventQueue when it is empty
	char m_PassWord[6];		   // Password for XOR Password
	unsigned char m_ipaddrh;
	unsigned char m_ipaddrl;

	//	int HeartBeat;
	unsigned char m_ViaCM;					      // eHouse RS-485 Via CommManager
	unsigned char m_eHouse1FrameEmpty;			      // eHouse1 bus free after reception of all status for Safer Event submissions
	unsigned char m_SrvAddrH, m_SrvAddrL, m_SrvAddrU, m_SrvAddrM; // eHouse Pro server IP address splited

	unsigned char *m_dta;
	unsigned char m_disablers485;
	unsigned char m_StatusDebug, // Log status reception
		m_IRPerform;	     // Perform InfraRed signals
	int m_ProSize;

	unsigned char m_eHStatusReceived; // eHouse1 status received flag
	int m_CloudStatusChanged;	  // data changed => must be updated
	unsigned char m_INITIAL_ADDRESS_LAN;
	unsigned char m_INITIAL_ADDRESS_WIFI;
	unsigned char m_UDP_terminate_listener; // terminate udp listener service
	unsigned char m_eHEStatusReceived;	// Ethernet eHouse status received flag (count of status from reset this flag)
	unsigned char m_eHWiFiStatusReceived;	// eHouse WiFi status received flag (count of status from reset this flag)
	typedef struct TcpClientConT
	{
		int Socket;		    // TCP Client Sockets for paralel operations
		unsigned char Events[255U]; // Event buffer for current socket
		// unsigned char TimeOut;                //TimeOut for current client connection in 0.1s require external thread or non blocking socket
		// Active connections to Ehouse Controllers to avoid multiple connection to the same device
		signed int ActiveConnections;		       // index of status matrix eHE[] or ETHERNET_EHOUSE_RM_MAX for CM
		unsigned char AddrH;			       // destination IP byte 3
		unsigned char AddrL;			       // destination IP byte 4
		unsigned char EventSize;		       // size of event to submit
		unsigned char EventsToRun[MAX_EVENTS_IN_PACK]; // Events to send in One Pack
		unsigned char OK;
		unsigned char NotFinished;
		//        unsigned char Stat;                   //Status of client
	} TcpClientCon;
	TcpClientCon m_TC[MAX_CLIENT_SOCKETS]; // TCP Client Instances in case of multi-threading
	/*typedef struct tModel {
		unsigned int type;          //controller type / interface
		unsigned int  id;           //id for controller type detection
		unsigned int  addrh;        //address high (for LAN wariant 192.168.addrh.addrl
		unsigned int  addrlfrom;    //minimal address low value
		unsigned int addrlto;       //maximal address low value
		const char*   name;         //Controller Name
									// count of signals for controller
		unsigned int  inputs;       //inputs on/off (binary)
		unsigned int  outputs;      //outputs on/off (binary)
		unsigned int  adcs;         //adc measurement
		unsigned int  dimmers;      //dimmers - PWM
		unsigned char drives;       //2 outputs drive control
		unsigned int  zones;        //security zones count
		unsigned int  programs;     //outputs+dimmers programs
		unsigned char adcprograms;  //adc measurement+regulation programs
		unsigned char secuprograms; //drives count
	} Model;

	#define TOT_MODELS 13
	Model models[TOT_MODELS] =
	{
		//Non LAN/IP variants
		//type  id    h   lmin  lmax   name                         inp  out  adc  dim drv zon pgm  apg  spg
		{ EH_RS485,    0x1,  1,    1,   1,    "HeatManager - RS-485"      , 0,  21,  16,  3,  0,  0,  24,    0,  0 },
		{ EH_RS485,    0x2,  2,    1,   1,    "ExternalManager - RS-485"  , 12, 2,    8,  3, 14,  0,   0,    0,  24 },
		{ EH_RS485,    0x3,  55,   1, 254,   "RoomManager - RS-485"       , 12, 32,   8,  3,  0,  0,  24,    0,  0 },
		{ EH_CANRF,    0x79, 0x79, 1, 128,   "CAN/RF - RF (863MHz)"       , 4 , 4,    4,  4,  2,  0,   0,    0,  0 },
		{ EH_CANRF,    0x80, 0x80, 1, 128,   "CAN/RF - CAN"               , 4 , 4,    4,  4,  2,  0,   0,    0,  0 },
		{ EH_AURA,    0x81, 0x81, 1, 128,   "Aura RF 863MHz sensors"     , 0 , 0,    1,  0,  0,  0,   0,    0,  0 },
		{ EH_LORA,    0x82, 0x82, 1, 128,   "LORA RF devs"               , 4 , 4,    4,  4,  2,  0,   0,    0,  0 },
		//LAN/IP variants other address H than above
		{ EH_PRO,    0xfe, 256, 190, 200, "eHouse PRO Server / Hybrid"   ,256,256,   0,  0,128,100, 100,  100, 100 },
		{ EH_LAN,    201,  256, 201, 248, "EthernetRoomManger -ERM LAN"  ,22 , 32,  15,  3, 16,  0,  24,   12,   0 },
		{ EH_LAN,    249,  256, 249, 249, "EthernetPoolManger - LAN"     ,5 ,   8,  15,  3,  6,  0,  24,   12,   0 }, //dedicated firmware for swimming pools
		{ EH_LAN,    250,  256, 250, 254, "CommManger - LAN"             ,48,   5,  15,  0, 36, 24,   0,   12,  24 }, //dedicated firmware for roler controller + security system
		{ EH_LAN,    251,  256, 250, 254, "LevelManager - LAN"           ,48,  77,  15,  0, 36, 24,  24,   12,   0 }, //dedicated firmware for floor controller
		{ EH_WIFI,    100,  256, 201, 248, "WiFi Controllers"             ,22 , 32,  15,  3, 16,  0,  24,   12,   0 },
	};
	*/
	int m_Dtype, m_Dsubtype;
};
