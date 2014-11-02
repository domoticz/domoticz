/* 
	Domoticz Software : http://domoticz.com/
	File : Teleinfo.h
	Author : Nicolas HILAIRE
	Version : 1.0
	Description : This class manage the Teleinfo Signal
	
	
	History : 
	- 2013-11-01 : Creation
*/

 #pragma once

#include "DomoticzHardware.h"
#include "P1MeterBase.h"
#include "ASyncSerial.h"

/*
typedef struct _teleinfoData {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	char optTariff[4];
	unsigned long powerusage1;	// normal tariff at power Usage in Wh
	unsigned long powerusage2;	// low tariff at power Usage in Wh
	unsigned long currentPowerSubscribe; //in A
	char currentTariffPeriod[4];
	unsigned long instantCurrentPowerUsage; //in A
	unsigned long maximalCurrentPowerUsage; //in A
	unsigned long apparentPower;	//in VA
} TeleinfoData;
*/

#define TELEINFO_BAUD_RATE			1200
#define TELEINFO_PARITY				boost::asio::serial_port_base::parity::even
#define TELEINFO_CARACTER_SIZE		7
#define TELEINFO_FLOW_CONTROL		boost::asio::serial_port_base::flow_control::none
#define TELEINFO_STOP_BITS			boost::asio::serial_port_base::stop_bits::one

class Teleinfo : public CDomoticzHardwareBase, AsyncSerial
{
	typedef enum { 
	ID=0, 
	STD, 
	} MatchType;

	typedef enum {
		TELEINFO_OPTION_BASE,	//single tariff
		TELEINFO_OPTION_HC,		//double tariff (low and normal)
		//TELEINFO_OPTION_EJP,		//EJP Option,  (not developed for the moment)
		//TELEINFO_OPTION_TEMPO,	//Tempo option,  (not developed for the moment)
	} Option;

	typedef enum {
		TELEINFO_TYPE_ADCO,
		TELEINFO_TYPE_OPTARIF ,
		TELEINFO_TYPE_ISOUSC,
		TELEINFO_TYPE_BASE,
		TELEINFO_TYPE_HCHC,
		TELEINFO_TYPE_HCHP,
		TELEINFO_TYPE_PTEC,
		TELEINFO_TYPE_IINST,
		TELEINFO_TYPE_IMAX,
		TELEINFO_TYPE_PAPP,
	} Type;

	typedef struct _tMatch {
		MatchType matchtype;
		Type type;
		const char* key;
		int width;
	} Match;

public:

	Teleinfo(const int ID, const std::string& devname, unsigned int baud_rate = TELEINFO_BAUD_RATE);
	
    /**
    * Opens a serial device.
    * \param devname serial device name, example "/dev/ttyAMA0" or "COM1"
    * \param baud_rate serial baud rate, default 1200
    * \param opt_parity serial parity, default even
    * \param opt_csize serial character size, default 7bit
    * \param opt_flow serial flow control, default none
    * \param opt_stop serial stop bits, default 1
    * \throws boost::system::system_error if cannot open the
    * serial device
    */
	Teleinfo(const std::string& devname, unsigned int baud_rate = TELEINFO_BAUD_RATE,
        boost::asio::serial_port_base::parity opt_parity = boost::asio::serial_port_base::parity(TELEINFO_PARITY),
        boost::asio::serial_port_base::character_size opt_csize = boost::asio::serial_port_base::character_size(TELEINFO_CARACTER_SIZE),
        boost::asio::serial_port_base::flow_control opt_flow = boost::asio::serial_port_base::flow_control(TELEINFO_FLOW_CONTROL),
        boost::asio::serial_port_base::stop_bits opt_stop = boost::asio::serial_port_base::stop_bits(TELEINFO_STOP_BITS));
	
    ~Teleinfo();
	std::string m_szSerialPort;

	P1Power	m_p1power;
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	/**
		* Read callback, stores data in the buffer
		*/
    
	void readCallback(const char *data, size_t len);
	
	unsigned int m_iBaudRate;
	boost::asio::serial_port_base::parity m_iOptParity;
	boost::asio::serial_port_base::character_size m_iOptCsize;
	boost::asio::serial_port_base::flow_control m_iOptFlow;
	boost::asio::serial_port_base::stop_bits m_iOptStop;
	
	int m_counter;
	static const int readBufferSize=1028;
	static const int NumberOfFrameToSendOne=8;
	
	void Init();
	void MatchLine();
	void ParseData(const unsigned char *pData, int Len);
	bool isCheckSumOk();

	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;	
	static Teleinfo::Match m_matchlist[10];
};

