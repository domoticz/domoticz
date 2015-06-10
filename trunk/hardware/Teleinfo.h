/*
Domoticz Software : http://domoticz.com/
File : Teleinfo.h
Author : Nicolas HILAIRE
Version : 1.3
Description : This class manage the Teleinfo Signal


History :
- 2013-11-01 : Creation
- 2014-10-29 : Add 'EJP' contract (Laurent MEY)
- 2014-12-13 : Add 'Tempo' contract (Kevin NICOLAS)
- 2015-06-10 : Fix bug power divided by 2 (Christophe DELPECH)
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
unsigned long powerusage1;   // normal tariff at power Usage in Wh
unsigned long powerusage2;   // low tariff at power Usage in Wh
unsigned long currentPowerSubscribe; //in A
char currentTariffPeriod[4];
unsigned long instantCurrentPowerUsage; //in A
unsigned long maximalCurrentPowerUsage; //in A
unsigned long apparentPower;   //in VA
} TeleinfoData;
*/

#define TELEINFO_BAUD_RATE         1200
#define TELEINFO_PARITY            boost::asio::serial_port_base::parity::even
#define TELEINFO_CARACTER_SIZE      7
#define TELEINFO_FLOW_CONTROL      boost::asio::serial_port_base::flow_control::none
#define TELEINFO_STOP_BITS         boost::asio::serial_port_base::stop_bits::one

class Teleinfo : public CDomoticzHardwareBase, AsyncSerial
{
	typedef enum {
		ID = 0,
		STD,
	} MatchType;

	//typedef enum {
	//   TELEINFO_OPTION_BASE,   //single tariff
	//   TELEINFO_OPTION_HC,      //double tariff (low and normal)
	//   TELEINFO_OPTION_EJP,      //EJP Option,  (test)
	//   TELEINFO_OPTION_TEMPO,   //Tempo option,  (test)
	//} Option;

	typedef enum {
		TELEINFO_TYPE_ADCO,
		TELEINFO_TYPE_OPTARIF,
		TELEINFO_TYPE_ISOUSC,
		TELEINFO_TYPE_BASE,
		TELEINFO_TYPE_HCHC,
		TELEINFO_TYPE_HCHP,
		TELEINFO_TYPE_EJPHN,
		TELEINFO_TYPE_EJPHPM,
		TELEINFO_TYPE_BBRHCJB,
		TELEINFO_TYPE_BBRHPJB,
		TELEINFO_TYPE_BBRHCJW,
		TELEINFO_TYPE_BBRHPJW,
		TELEINFO_TYPE_BBRHCJR,
		TELEINFO_TYPE_BBRHPJR,
		TELEINFO_TYPE_PTEC,
		TELEINFO_TYPE_IINST,
		TELEINFO_TYPE_IMAX,
		TELEINFO_TYPE_PAPP,
		TELEINFO_TYPE_MOTDETAT
	} Type;

	typedef struct _tMatch {
		MatchType matchtype;
		Type type;
		const char* key;
		int width;
	} Match;

public:

	Teleinfo(const int ID, const std::string& devname, unsigned int baud_rate = TELEINFO_BAUD_RATE);
	~Teleinfo();
	std::string m_szSerialPort;

	P1Power   m_p1power;
	bool WriteToHardware(const char *pdata, const unsigned char length);
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
	bool m_bLabel_PAPP_Exist;
	static const int readBufferSize = 1028;
	static const int NumberOfFrameToSendOne = 8;

	void Init();
	void MatchLine();
	void ParseData(const unsigned char *pData, int Len);
	bool isCheckSumOk();

	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;
	static Teleinfo::Match m_matchlist[19];
};

