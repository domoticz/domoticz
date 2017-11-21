#pragma once
//#define TEST_DOMOTICZ 1 //without domoticz
//

// implememtation for eHouse Home Automation System : (eHouse LAN, WiFi, PRO + indirectly RS-485, CAN, RF, AURA, RFID)
//DIY EN: http://smart.ehouse.pro/
//DIY PL: http://idom.ehouse.pro/
//Shop : http://eHouse.Biz/
// by Robert Jarzabek, iSys - Intelligent systems
#include "eHouse/globals.h"

#include <map>
#include "DomoticzHardware.h"
#include "hardwaretypes.h"
extern unsigned char eHEnableAutoDiscovery;
extern unsigned char eHEnableProDiscovery;
extern unsigned char eHEnableAlarmInputs;
extern unsigned int  eHOptA;
extern unsigned int  eHOptB;

class eHouseTCP :  public  CDomoticzHardwareBase
            
{
public:
    eHouseTCP(const int ID, const std::string &IPAddress, const unsigned short IPPort, const std::string& userCode, const int pollInterval,const int AutoDiscovery,const int EnableAlarms, const int EnablePro,const int opta, const int optb);
 	//virtual 
		~eHouseTCP();
		bool WriteToHardware(const char *pdata, const unsigned char length);
		int ConnectTCP();
		void AddTextEvents(unsigned char *ev, int size);
		signed int AddToLocalEvent(unsigned char *Even, unsigned char offset);       
private:
	int m_modelIndex;
	bool m_data32;
	sockaddr_in m_addr;
	int m_socket;
	const         unsigned short m_IPPort;  // 9876;    default port
	const          std::string m_IPAddress; // "192.168.0.200"; - default eHouse PRO srv address
	int m_pollInterval;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	unsigned char m_newData[7];

	// password to eHouse 6 ascii chars
	unsigned char m_userCode[8];

	boost::mutex m_mutex;
	bool m_alarmLast;
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
        
	bool CheckAddress();
	// Closes socket
	void DestroySocket();
	// Connects socket
	bool ConnectToPro();
	// new data is collected in Integra for selected command
	bool ReadNewData();
	// Gets info from hardware about PCB, ETHM1 etc
	bool GetInfo();
	// convert string from cp1250 to utf8
	std::string ISO2UTF8(const std::string &name);
	//void * EhouseSubmitData(void *ptr);
	std::pair<unsigned char*, unsigned int> getFullFrame(const unsigned char* pCmd, const unsigned int cmdLength);
	int SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer, const unsigned int expectedLength1, const unsigned int expectedLength2 = -1);
	void eCMaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHPROaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eAURAaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHEaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHaloc(int eHEIndex, int devaddrh, int devaddrl);
	void eHWIFIaloc(int eHEIndex, int devaddrh, int devaddrl);
	unsigned char IsCM(unsigned char addrh, unsigned char addrl);
	signed int IndexOfeHouseRS485(unsigned char devh, unsigned char devl);
	void UpdateAuraToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdateCMToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdateLanToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdatePROToSQL(unsigned char AddrH, unsigned char AddrL);
	void UpdateWiFiToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void UpdateRS485ToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	void TerminateUDP(void);
	void IntToHex(unsigned char *buf, const unsigned char *inbuf, int received);
	float getAdcVolt2(int index);
	void CalculateAdc2(char index);
	void CalculateAdcWiFi(char index);
	void CalculateAdcEH1(char index);
	void deb(char *prefix, unsigned char *dta, int size);
	void GetStr(unsigned char *GetNamesDta);
	void GetUDPNamesRS485(unsigned char *data, int nbytes);
	int gettype(int adrh, int adrl);
	void GetUDPNamesLAN(unsigned char *data, int nbytes);
	void GetUDPNamesCM(unsigned char *data, int nbytes);
	void GetUDPNamesPRO(unsigned char *data, int nbytes);
	void GetUDPNamesWiFi(unsigned char *data, int nbytes);
//	void UpdateCMToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	//void UpdateAuraToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	//void UpdateWiFiToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	//void UpdateLanToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	//void UpdateRS485ToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index);
	//void UpdatePROToSQL(unsigned char AddrH, unsigned char AddrL);
//	void GetUDPNamesLAN(unsigned char *data, int nbytes);
//	void GetUDPNamesWiFi(unsigned char *data, int nbytes);
//	void GetUDPNamesPRO(unsigned char *data, int nbytes);
//	void GetUDPNamesCM(unsigned char *data, int nbytes);
//	void GetUDPNamesRS485(unsigned char *data, int nbytes);
	int UpdateSQLState(int devh, int devl, int devtype, int type, int subtype, int swtype, int code, int nr, char signal, int nValue, const char  *sValue, const char * Name, const char * SignalName, bool on_off, int battery);
	void UpdateSQLStatus(int devh, int devl, int devtype, int code, int nr, char signal, int nValue, const char  *sValue, int battery);
	int UpdateSQLPlan(int devh, int devl, int devtype, const char * Name);
	void UpdatePGM(int adrh, int adrl, int devtype, const char *names, int idx);
	signed int IndexOfEthDev(unsigned char AddrH, unsigned char AddrL);
	signed int GetIndexOfWiFiDev(unsigned char AddrH, unsigned char AddrL);
	void EhouseInitTcpClient(void);
	char SendTCPEvent(const unsigned char *Events, unsigned char EventCount, unsigned char AddrH, unsigned char AddrL, const unsigned char *EventsToRun);
	void performTCPClientThreads();
	int  getrealERMpgm(int32_t ID, int level); 
	int  getrealRMpgm(int32_t ID, int level);
	signed int GetIndexOfEvent(unsigned char *TempEvent);
	void ExecQueuedEvents(void);
	void ExecEvent(unsigned int i);
	signed int hex2bin(const unsigned char *st, int offset);
	
	int eHouseUDPSocket;		//UDP socket handler
	int UDP_PORT;			//Default UDP PORT
	unsigned char nr_of_ch;	
	char DEBUG_AURA;		//Debug Aura
	char CHANGED_DEBUG;
	unsigned char *dta;
	unsigned disablers485;
	unsigned char StatusDebug,	//Log status reception
		IRPerform,				//Perform InfraRed signals
		ViaCM ;					//eHouse RS-485 via CommManager

	volatile unsigned char eHStatusReceived;			//eHouse1 status received flag
	int CloudStatusChanged;							//data changed => must be updated
	unsigned char   COMMANAGER_IP_HIGH,				//initial addresses of different controller types
	COMMANAGER_IP_LOW,
	INITIAL_ADDRESS_LAN,
	INITIAL_ADDRESS_WIFI;
	volatile unsigned char UDP_terminate_listener;    //terminate udp listener service
	volatile unsigned char eHEStatusReceived;         //Ethernet eHouse status received flag (count of status from reset this flag)
	volatile unsigned char eHWiFiStatusReceived;      //eHouse WiFi status received flag (count of status from reset this flag)



};
