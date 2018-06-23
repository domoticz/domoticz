/*
Domoticz Software : http://domoticz.com/
File : TeleinfoSerial.cpp
Author : Nicolas HILAIRE, Blaise Thauvin
Version : 2.3
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
- 2017-03-27 : Greatly simplified code
- 2017-04-01 : 2.3 Added RateLimit, flag to ignore CRC checks, and new CRC computation algorithm available on newer meters
*/

#pragma once

#include "ASyncSerial.h"
#include "TeleinfoBase.h"
#define TELEINFO_PARITY            boost::asio::serial_port_base::parity::even
#define TELEINFO_CARACTER_SIZE      7
#define TELEINFO_FLOW_CONTROL      boost::asio::serial_port_base::flow_control::none
#define TELEINFO_STOP_BITS         boost::asio::serial_port_base::stop_bits::one

class CTeleinfoSerial : public CTeleinfoBase, AsyncSerial
{
	public:
		CTeleinfoSerial(const int ID, const std::string& devname, const int datatimeout, unsigned int baud_rate,
			const bool disable_crc, const int ratelimit);
		~CTeleinfoSerial();
		bool WriteToHardware(const char *pdata, const unsigned char length) override;
	private:
		bool StartHardware() override;
		bool StopHardware() override;
		void Init();
		void MatchLine();
		void ParseData(const char *pData, int Len);
		bool isCheckSumOk(int &isMode1);
		void readCallback(const char *data, size_t len);
private:
		std::string m_szSerialPort;
		Teleinfo teleinfo;

		unsigned int m_iBaudRate;
		bool m_bDisableCRC;
		boost::asio::serial_port_base::parity m_iOptParity;
		boost::asio::serial_port_base::character_size m_iOptCsize;
		boost::asio::serial_port_base::flow_control m_iOptFlow;
		boost::asio::serial_port_base::stop_bits m_iOptStop;

		unsigned int m_counter;

		char m_buffer[1024];
		int m_bufferpos;
};
