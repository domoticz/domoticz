/*
Domoticz Software : http://domoticz.com/
File : TeleinfoSerial.cpp
Author : Nicolas HILAIRE
Version : 2.0
Description : This class decodes the Teleinfo signal from serial/USB devices before processing them

History :
- 2013-11-01 : Creation
- 2014-10-29 : Add 'EJP' contract (Laurent MEY)
- 2014-12-13 : Add 'Tempo' contract (Kevin NICOLAS)
- 2015-06-10 : Fix bug power divided by 2 (Christophe DELPECH)
- 2016-02-05 : Fix bug power display with 'Tempo' contract (Anthony LAGUERRE)
- 2016-02-11 : Fix power display when PAPP is missing (Anthony LAGUERRE)
- 2016-02-17 : Fix bug power usage (Anthony LAGUERRE). Thanks to Multinet
- 2017-01-28 : Add 'Heures Creuses' Switch (A.L)
- 2017-03-15 : Renamed from Teleinfo.cpp to TeleinfoSerial.cpp in order to create
						   a shared class to process Teleinfo protocol (Blaise Thauvin)
*/

#pragma once

#include "ASyncSerial.h"
#include "TeleinfoBase.h"
#define TELEINFO_BAUD_RATE         1200
#define TELEINFO_PARITY            boost::asio::serial_port_base::parity::even
#define TELEINFO_CARACTER_SIZE      7
#define TELEINFO_FLOW_CONTROL      boost::asio::serial_port_base::flow_control::none
#define TELEINFO_STOP_BITS         boost::asio::serial_port_base::stop_bits::one

class CTeleinfoSerial : public CTeleinfoBase, AsyncSerial
{
	typedef enum
	{
		ID = 0,
		STD,
	} MatchType;

	typedef enum
	{
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
		TELEINFO_TYPE_IINST1,
		TELEINFO_TYPE_IINST2,
		TELEINFO_TYPE_IINST3,
		TELEINFO_TYPE_PPOT,
		TELEINFO_TYPE_DEMAIN,
		TELEINFO_TYPE_PEJP,
		TELEINFO_TYPE_PAPP,
		TELEINFO_TYPE_MOTDETAT
	} Type;

	typedef struct _tMatch
	{
		MatchType matchtype;
		Type type;
		const char* key;
		int width;
	} Match;

	public:

		CTeleinfoSerial(const int ID, const std::string& devname, unsigned int baud_rate = TELEINFO_BAUD_RATE);
		~CTeleinfoSerial();
		std::string m_szSerialPort;

		bool WriteToHardware(const char *pdata, const unsigned char length);
	
	private:
		Teleinfo teleinfo;
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
		unsigned long m_Power_USAGE_IINST;
		unsigned long m_Power_USAGE_IINST_JW;
		unsigned long m_Power_USAGE_IINST_JR;
		bool m_bLabel_PAPP_Exist;
		bool m_bLabel_PTEC_JB;
		bool m_bLabel_PTEC_JW;
		bool m_bLabel_PTEC_JR;
		bool m_bLabel_Tempo;

		void Init();
		void MatchLine();
		void ParseData(const unsigned char *pData, int Len);
		bool isCheckSumOk();

		unsigned char m_buffer[1028];
		int m_bufferpos;
		static CTeleinfoSerial::Match m_matchlist[27];
};
