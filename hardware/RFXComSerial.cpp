#include "stdafx.h"
#include "RFXComSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

#ifndef WIN32
	#include <sys/stat.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
#endif

#define RETRY_DELAY 30

extern std::string szStartupFolder;

#define round(a) ( int ) ( a + .5 )

const unsigned char PKT_STX = 0x55;
const unsigned char PKT_ETX = 0x04;
const unsigned char PKT_DLE = 0x05;

#define PKT_writeblock 256
#define PKT_readblock 4
#define PKT_eraseblock 2048
#define PKT_maxpacket 261
#define PKT_bytesperaddr 2
//#define PKT_pmrangelow	0x001A00
#define PKT_pmrangelow	0x001800
#define PKT_pmrangehigh	0x00A7FF
#define PKT_userresetvector 0x100
#define PKT_bootdelay 0x102

#define COMMAND_WRITEPM 2
#define COMMAND_ERASEPM 3

const unsigned char PKT_STARTBOOT[5] = { 0x01, 0x01, 0x00, 0x00, 0xFF };
const unsigned char PKT_RESET[2] = { 0x00, 0x00 };
const unsigned char PKT_VERSION[2] = { 0x00, 0x02 };
const unsigned char PKT_VERIFY_OK[5] = { 0x08, 0x01, 0x00, 0x00, 0x00 };

//
//Class RFXComSerial
//
RFXComSerial::RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	m_stoprequested=false;
	m_bReceiverStarted = false;
	m_bInBootloaderMode = false;
	m_bStartFirmwareUpload = false;
	m_szUploadMessage = "";
	m_serial.setPort(m_szSerialPort);
	m_serial.setBaudrate(m_iBaudRate);
	m_serial.setBytesize(serial::eightbits);
	m_serial.setParity(serial::parity_none);
	m_serial.setStopbits(serial::stopbits_one);
	m_serial.setFlowcontrol(serial::flowcontrol_none);

	serial::Timeout stimeout = serial::Timeout::simpleTimeout(100);
	m_serial.setTimeout(stimeout);
}

RFXComSerial::~RFXComSerial()
{
	clearReadCallback();
}

bool RFXComSerial::StartHardware()
{
	//return OpenSerialDevice();
	//somehow retry does not seem to work?!
	m_bReceiverStarted = false;

	m_retrycntr=RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RFXComSerial::Do_Work, this)));

	return (m_thread!=NULL);

}

bool RFXComSerial::StopHardware()
{
	m_stoprequested=true;
	if (m_thread!=NULL)
		m_thread->join();
    // Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
    sleep_milliseconds(10);
	if (m_serial.isOpen())
		m_serial.close();
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	m_bIsStarted=false;
	return true;
}

void RFXComSerial::Do_Work()
{
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (m_stoprequested)
			break;

		if (m_bStartFirmwareUpload)
		{
			m_bStartFirmwareUpload = false;
			if (isOpen())
			{
				try {
					clearReadCallback();
					close();
					doClose();
					setErrorStatus(true);
				}
				catch (...)
				{
					//Don't throw from a Stop command
				}
			}
			try {
				sleep_seconds(1);
				UpgradeFirmware();
			}
			catch (...)
			{
			}
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"RFXCOM: retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
			}
		}

	}
	_log.Log(LOG_STATUS,"RFXCOM: Serial Worker stopped...");
} 


bool RFXComSerial::OpenSerialDevice(const bool bIsFirmwareUpgrade)
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort,m_iBaudRate);
		_log.Log(LOG_STATUS,"RFXCOM: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"RFXCOM: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"RFXCOM: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_rxbufferpos=0;
	setReadCallback(boost::bind(&RFXComSerial::readCallback, this, _1, _2));
	if (!bIsFirmwareUpgrade)
		sOnConnected(this);
	return true;
}

bool RFXComSerial::UploadFirmware(const std::string &szFilename)
{
	m_szFirmwareFile = szFilename;
	m_FirmwareUploadPercentage = 0;
	m_bStartFirmwareUpload = true;
	try {
		clearReadCallback();
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	return true;
}

bool RFXComSerial::UpgradeFirmware()
{
	m_FirmwareUploadPercentage = 0;
	m_bStartFirmwareUpload = false;
	std::map<unsigned long, std::string> firmwareBuffer;
	std::map<unsigned long, std::string>::const_iterator itt;
	int icntr = 0;
	if (!Read_Firmware_File(m_szFirmwareFile.c_str(), firmwareBuffer))
	{
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}

	try
	{
		m_serial.open();
	}
	catch (...)
	{
		m_szUploadMessage = "RFXCOM: Error opening serial port!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
	//Start bootloader mode
	m_szUploadMessage = "RFXCOM: Start bootloader process...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);
	m_szUploadMessage = "RFXCOM: Get bootloader version...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	//read bootloader version
	if (!Write_TX_PKT(PKT_VERSION, sizeof(PKT_VERSION)))
	{
		m_szUploadMessage = "RFXCOM: Error getting bootloader version!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
	_log.Log(LOG_STATUS, "RFXCOM: bootloader version v%d.%d", m_rx_input_buffer[3], m_rx_input_buffer[2]);

	if (!EraseMemory(PKT_pmrangelow, PKT_pmrangehigh))
	{
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}

#ifndef WIN32
	try
	{
		m_serial.flush();
	}
	catch (...)
	{
		m_szUploadMessage = "RFXCOM: Bootloader, unable to flush serial device!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
#endif
	try
	{
		m_serial.close();
	}
	catch (...)
	{
	}
	sleep_seconds(1);

	try
	{
		m_serial.open();
	}
	catch (...)
	{
		m_szUploadMessage = "RFXCOM: Error opening serial port!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}

	m_szUploadMessage = "RFXCOM: Start bootloader version...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);

	m_szUploadMessage = "RFXCOM: Bootloader, Start programming...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	for (itt = firmwareBuffer.begin(); itt != firmwareBuffer.end(); ++itt)
	{
		icntr++;
		if (icntr % 5 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}
		unsigned long Address = itt->first;
		m_FirmwareUploadPercentage = (100.0f / float(firmwareBuffer.size()))*icntr;
		if (m_FirmwareUploadPercentage > 100)
			m_FirmwareUploadPercentage = 100;

		//if ((Address >= PKT_pmrangelow) && (Address <= PKT_pmrangehigh))
		{
			std::stringstream saddress;
			saddress << "Programming Address: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << Address;

			std::stringstream spercentage;
			spercentage.precision(2);
			spercentage << std::setprecision(2)  << std::fixed << m_FirmwareUploadPercentage;
			m_szUploadMessage = saddress.str() + ", " + spercentage.str() + " %";
			_log.Log(LOG_STATUS, "%s", m_szUploadMessage.c_str());

			unsigned char bcmd[PKT_writeblock + 10];
			bcmd[0] = COMMAND_WRITEPM;
			bcmd[1] = 1;
			bcmd[2] = Address & 0xFF;
			bcmd[3] = (Address & 0xFF00) >> 8;
			bcmd[4] = (unsigned char)((Address & 0xFF0000) >> 16);
			memcpy(bcmd + 5, itt->second.c_str(), itt->second.size());
			bool ret = Write_TX_PKT(bcmd, 5 + itt->second.size(), 20);
			if (!ret)
			{
				m_szUploadMessage = "RFXCOM: Bootloader, unable to program firmware memory, please try again!!!";
				_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
				m_FirmwareUploadPercentage = -1;
				goto exitfirmwareupload;
			}
		}
	}
	firmwareBuffer.clear();
#ifndef WIN32
	try
	{
		m_serial.flush();
	}
	catch (...)
	{
		m_szUploadMessage = "RFXCOM: Bootloader, unable to flush serial device!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
#endif
	//Verify
	m_szUploadMessage = "RFXCOM: Start bootloader verify...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	if (!Write_TX_PKT(PKT_VERIFY_OK, sizeof(PKT_VERIFY_OK)))
	{
		m_szUploadMessage = "RFXCOM: Bootloader,  program firmware memory not succeeded, please try again!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
	if (m_rx_input_buffer[0] != PKT_VERIFY_OK[0])
	{
		m_FirmwareUploadPercentage = -1;
		m_szUploadMessage = "RFXCOM: Bootloader,  program firmware memory not succeeded, please try again!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
	}
	else
	{
		m_szUploadMessage = "RFXCOM: Bootloader, Programming completed successfully...";
		_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	}
exitfirmwareupload:
	m_szUploadMessage = "RFXCOM: bootloader reset...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	Write_TX_PKT(PKT_RESET, sizeof(PKT_RESET), 1);
#ifndef WIN32
	try
	{
		m_serial.flush();
	}
	catch (...)
	{
		m_szUploadMessage = "RFXCOM: Bootloader, unable to flush serial device!!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
#endif
	sleep_seconds(1);
	try
	{
		if (m_serial.isOpen())
		{
			m_serial.close();
		}
	}
	catch (...)
	{
	}

	m_rxbufferpos = 0;
	m_bInBootloaderMode = false;
	OpenSerialDevice();
	return true;
}

//returns -1 when failed
float RFXComSerial::GetUploadPercentage()
{
	return m_FirmwareUploadPercentage;
}

std::string RFXComSerial::GetUploadMessage()
{
	return m_szUploadMessage;
}

bool RFXComSerial::Read_Firmware_File(const char *szFilename, std::map<unsigned long, std::string>& fileBuffer)
{
#ifndef WIN32
	struct stat info;
	if (stat(szFilename,&info)==0)
	{
		struct passwd *pw = getpwuid(info.st_uid);
		int ret=chown(szFilename,pw->pw_uid,pw->pw_gid);
		if (ret!=0)
		{
			m_szUploadMessage = "Error setting firmware ownership (chown returned an error!)";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			return false;
		}
	}
#endif

	std::ifstream infile;
	std::string sLine;
	infile.open(szFilename);
	if (!infile.is_open())
	{
		m_szUploadMessage = "RFXCOM: bootloader, unable to open file: " + std::string(szFilename);
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		return false;
	}

	m_szUploadMessage = "RFXCOM: start reading Firmware...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());

	unsigned char rawLineBuf[PKT_writeblock];
	int raw_length = 0;
	unsigned long dest_address = 0;
	int line = 0;
	int addrh = 0;

	fileBuffer.clear();
	std::string dstring="";
	bool bHaveEOF = false;

	while (!infile.eof())
	{
		getline(infile, sLine);
		if (sLine.empty())
			continue;
		//Every line should start with ':'
		if (sLine[0] != ':')
		{
			infile.close();
			m_szUploadMessage = "RFXCOM: bootloader, firmware does not start with ':'";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			return false;
		}
		sLine = sLine.substr(1);
		if (sLine.size() > 1)
		{
			if (sLine[sLine.size() - 1] == '\n')
			{
				sLine = sLine.substr(0, sLine.size() - 1);
			}
			if (sLine[sLine.size() - 1] == '\r')
			{
				sLine = sLine.substr(0, sLine.size() - 1);
			}
		}
		if (sLine.size() % 2 != 0)
		{
			infile.close();
			m_szUploadMessage = "RFXCOM: bootloader, firmware line not equals 2 digests";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			return false;
		}
		raw_length = 0;
		unsigned char chksum = 0;
		while (!sLine.empty())
		{
			std::string szHex = sLine.substr(0, 2); sLine = sLine.substr(2);
			std::stringstream sstr;
			int iByte = 0;
			sstr << std::hex << szHex;
			sstr >> iByte;
			rawLineBuf[raw_length++] = (unsigned char)iByte;
			if (!sLine.empty())
				chksum += iByte;
			if (raw_length > sizeof(rawLineBuf) - 1)
			{
				infile.close();
				m_szUploadMessage = "RFXCOM: bootloader, incorrect length";
				_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
				return false;
			}
			
		}
		//
		chksum = ~chksum + 1;
		if ((chksum != rawLineBuf[raw_length - 1]) || (raw_length<4))
		{
			infile.close();
			m_szUploadMessage = "RFXCOM: bootloader, checksum mismatch!";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			return false;
		}
		int byte_count = rawLineBuf[0];
		int faddress = (rawLineBuf[1] << 8) | rawLineBuf[2];
		int rtype = rawLineBuf[3];

		switch (rtype)
		{
		case 0:
			//Data record
			dstring+= std::string((const char*)&rawLineBuf + 4, (const char*)rawLineBuf + 4 + byte_count);
			if (dstring.size() == PKT_writeblock)
			{
				dest_address = (((((addrh << 16) | (faddress + byte_count)) - PKT_writeblock)) / PKT_bytesperaddr);
				fileBuffer[dest_address] = dstring;
				dstring.clear();
			}
			break;
		case 1:
			//EOF Record
			bHaveEOF = dstring.empty();
			if (!bHaveEOF)
			{
				m_szUploadMessage = "RFXCOM: Bootloader invalid size!";
				_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			}
			break;
		case 2:
			//Extended Segment Address Record 
			m_szUploadMessage = "RFXCOM: Bootloader type 2 not supported!";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			infile.close();
			return false;
		case 3:
			//Start Segment Address Record 
			m_szUploadMessage = "RFXCOM: Bootloader type 3 not supported!";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			infile.close();
			return false;
		case 4:
			//Extended Linear Address Record
			if (raw_length < 7)
			{
				m_szUploadMessage = "RFXCOM: Invalid line length!!";
				_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
				infile.close();
				return false;
			}
			addrh = (rawLineBuf[4] << 8) | rawLineBuf[5]; 
			break;
		case 5:
			//Start Linear Address Record
			m_szUploadMessage = "RFXCOM: Bootloader type 5 not supported!";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			infile.close();
			return false;
		}
		line++;
	}
	infile.close();
	if (!bHaveEOF)
	{
		m_szUploadMessage = "RFXCOM: No end-of-line found!!";
		_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
		return false;
	}
	m_szUploadMessage = "RFXCOM: Firmware read correctly...";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	return true;
}

bool RFXComSerial::EraseMemory(const int StartAddress, const int StopAddress)
{
	m_szUploadMessage = "RFXCOM: Erasing memory....";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	int BootAddr = StartAddress;

	int blockcnt = 1;
	while (BootAddr < StopAddress)
	{
		int nBlocks = ((StopAddress - StartAddress + 1) * PKT_bytesperaddr) / PKT_eraseblock;
		nBlocks = (StopAddress - StartAddress + 1) / (PKT_eraseblock / PKT_bytesperaddr);
		if (nBlocks > 255)
			nBlocks = 255;

		unsigned char bcmd[5];
		bcmd[0] = COMMAND_ERASEPM;
		bcmd[1] = nBlocks;
		bcmd[2] = BootAddr & 0xFF;
		bcmd[3] = (BootAddr & 0xFF00) >> 8;
		bcmd[4] = (BootAddr & 0xFF0000) >> 16;

		if (!Write_TX_PKT(bcmd, sizeof(bcmd), 5))
		{
			m_szUploadMessage = "RFXCOM: Error erasing memory!";
			_log.Log(LOG_ERROR, m_szUploadMessage.c_str());
			return false;
		}
		BootAddr+= (PKT_eraseblock * nBlocks);
	}
	m_szUploadMessage = "RFXCOM: Erasing memory completed....";
	_log.Log(LOG_STATUS, m_szUploadMessage.c_str());
	return true;
}

bool RFXComSerial::Write_TX_PKT(const unsigned char *pdata, size_t length, const int max_retry)
{
	if (!m_serial.isOpen())
		return false;

	unsigned char output_buffer[512];
	int tot_bytes = 0;

	output_buffer[tot_bytes++] = PKT_STX;
	output_buffer[tot_bytes++] = PKT_STX;

	// Generate the checksum
	unsigned char chksum = 0;
	for (size_t ii = 0; ii < length; ii++)
	{
		unsigned char dbyte = pdata[ii];
		chksum += dbyte;

		//if control character, stuff DLE
		if (dbyte == PKT_STX || dbyte == PKT_ETX || dbyte == PKT_DLE)
		{
			output_buffer[tot_bytes++] = PKT_DLE;
		}
		output_buffer[tot_bytes++] = dbyte;
	}
	chksum = ~chksum + 1;
	if (chksum == PKT_STX || chksum == PKT_ETX || chksum == PKT_DLE)
	{
		output_buffer[tot_bytes++] = PKT_DLE;
	}
	output_buffer[tot_bytes++] = chksum;
	output_buffer[tot_bytes++] = PKT_ETX;

	m_bInBootloaderMode = true;
	/*
	if (!WaitForResponse)
	{
		write((const char*)&output_buffer, tot_bytes);
		sleep_milliseconds(500);
		return true;
	}
	*/
	int nretry = 0;
	unsigned char input_buffer[512];
	int tot_read;

	while (nretry < max_retry)
	{
		try
		{
			m_serial.write((const uint8_t *)&output_buffer, tot_bytes);
			int rcount = 0;
			while (rcount < 2)
			{
				sleep_milliseconds(500);
				tot_read = m_serial.read((uint8_t *)&input_buffer, sizeof(input_buffer));
				if (tot_read)
				{
					bool bret=Handle_RX_PKT(input_buffer, tot_read);
					if (bret)
						return true;
				}
				rcount++;
			}
			nretry++;
		}
		catch (...)
		{
			return false;
		}
	}
	return m_bHaveRX;
}

bool RFXComSerial::Handle_RX_PKT(const unsigned char *pdata, size_t length)
{
	if (length < 2)
		return false;
	if ((pdata[0] != PKT_STX) || (pdata[1] != PKT_STX))
		return false;

	unsigned char chksum = 0;
	m_rx_tot_bytes = 0;
	size_t ii = 1;
//	std::string szRespone = "Received: ";
//	int jj;
	while ((ii<length) && (m_rx_tot_bytes<sizeof(m_rx_input_buffer) - 1))
	{
		unsigned char dbyte = pdata[ii];
		switch (dbyte)
		{
		case PKT_STX:
			m_rx_tot_bytes = 0;
			chksum = 0;
			break;
		case PKT_ETX:
			chksum = ~chksum + 1; //test checksum
			if (chksum != 0)
			{
				_log.Log(LOG_ERROR, "RFXCOM: bootloader, received response with invalid checksum!");
				return false;
			}
			//Message OK
			/*
			for (jj = 0; jj < m_rx_tot_bytes; jj++)
			{
				std::stringstream sstr;
				sstr << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(m_rx_input_buffer[jj]);
				if (jj != m_rx_tot_bytes - 1)
					sstr << ", ";
				szRespone+=sstr.str();
			}
			_log.Log(LOG_STATUS, "%s", szRespone.c_str());
			*/
			m_bHaveRX = true;
			return true;
			break;
		case PKT_DLE:
			dbyte = pdata[ii+1];
			ii++;
			if (ii >= length)
				return false;
		default:
			chksum += dbyte;
			m_rx_input_buffer[m_rx_tot_bytes++] = dbyte;
			break;
		}
		ii++;
	}
	return false;
}

void RFXComSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		if (!m_bInBootloaderMode)
		{
			bool bRet = onInternalMessage((const unsigned char *)data, len);
			if (bRet == false)
			{
				//close serial connection, and restart
				if (isOpen())
				{
					try {
						clearReadCallback();
						close();
						doClose();
						setErrorStatus(true);
					}
					catch (...)
					{
						//Don't throw from a Stop command
					}
				}

			}
		}
	}
	catch (...)
	{

	}
}

bool RFXComSerial::onInternalMessage(const unsigned char *pBuffer, const size_t Len)
{
	if (!m_bEnableReceive)
		return true; //receiving not enabled

	size_t ii = 0;
	while (ii < Len)
	{
		if (m_rxbufferpos == 0)	//1st char of a packet received
		{
			if (pBuffer[ii] == 0) //ignore first char if 00
				return true;
		}
		m_rxbuffer[m_rxbufferpos] = pBuffer[ii];
		m_rxbufferpos++;
		if (m_rxbufferpos >= sizeof(m_rxbuffer)-1)
		{
			//something is out of sync here!!
			//restart
			_log.Log(LOG_ERROR, "RFXCOM: input buffer out of sync, going to restart!....");
			m_rxbufferpos = 0;
			return false;
		}
		if (m_rxbufferpos > m_rxbuffer[0])
		{
			if (!m_bReceiverStarted)
			{
				if (m_rxbuffer[1] == pTypeInterfaceMessage)
				{
					const tRBUF *pResponse = (tRBUF *)&m_rxbuffer;
					if (pResponse->IRESPONSE.subtype == cmdStartRec)
					{
						m_bReceiverStarted = strstr((char*)&pResponse->IRESPONSE.msg1, "Copyright RFXCOM") != NULL;
					}
					else
					{
						_log.Log(LOG_STATUS, "RFXCOM: Please upgrade your RFXTrx Firmware!...");
						m_bReceiverStarted = true;
					}
				}
			}
			else
				sDecodeRXMessage(this, (const unsigned char *)&m_rxbuffer);//decode message
			m_rxbufferpos = 0;    //set to zero to receive next message
		}
		ii++;
	}
	return true;
}

bool RFXComSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return false;
	if (m_bInBootloaderMode)
		return false;
	write(pdata,length);
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		char * CWebServer::RFXComUpgradeFirmware()
		{
			m_retstr = "/index.html";
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string hardwareid = m_pWebEm->FindValue("hardwareid");
			std::string firmwarefile = m_pWebEm->FindValue("firmwarefile");

			if (firmwarefile.empty())
			{
				return (char*)m_retstr.c_str();
			}

			CDomoticzHardwareBase *pHardware = NULL;
			if ((!hardwareid.empty()) && (hardwareid!="undefined"))
			{
				pHardware = m_mainworker.GetHardware(atoi(hardwareid.c_str()));
			}
			if (pHardware==NULL)
			{
				//Try to find the RFXCom hardware
				pHardware = m_mainworker.GetHardwareByType(HTYPE_RFXtrx433);
				if (pHardware==NULL)
				{
					return (char*)m_retstr.c_str();
				}
			}

			std::string outputfile = szStartupFolder + "rfx_firmware.hex";
			std::ofstream outfile;
			outfile.open(outputfile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
			if (!outfile.is_open())
				return (char*)m_retstr.c_str();
			outfile << firmwarefile;
			outfile.flush();
			outfile.close();

			if (
				(pHardware->HwdType == HTYPE_RFXtrx315)||
				(pHardware->HwdType == HTYPE_RFXtrx433)
				)
			{
				RFXComSerial *pRFXComSerial = (RFXComSerial *)pHardware;
				pRFXComSerial->UploadFirmware(outputfile);
			}
			return (char*)m_retstr.c_str();
		}
		char * CWebServer::SetRFXCOMMode()
		{
			m_retstr = "/index.html";

			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return (char*)m_retstr.c_str();
			}

			std::string idx = m_pWebEm->FindValue("idx");
			if (idx == "") {
				return (char*)m_retstr.c_str();
			}
			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID='%q')",
				idx.c_str());
			if (result.size() < 1)
				return (char*)m_retstr.c_str();

			unsigned char Mode1 = atoi(result[0][0].c_str());
			unsigned char Mode2 = atoi(result[0][1].c_str());
			unsigned char Mode3 = atoi(result[0][2].c_str());
			unsigned char Mode4 = atoi(result[0][3].c_str());
			unsigned char Mode5 = atoi(result[0][4].c_str());
			unsigned char Mode6 = atoi(result[0][5].c_str());

			tRBUF Response;
			Response.ICMND.freqsel = Mode1;
			Response.ICMND.xmitpwr = Mode2;
			Response.ICMND.msg3 = Mode3;
			Response.ICMND.msg4 = Mode4;
			Response.ICMND.msg5 = Mode5;
			Response.ICMND.msg6 = Mode6;

			Response.IRESPONSE.UNDECODEDenabled = (m_pWebEm->FindValue("undecon") == "on") ? 1 : 0;
			Response.IRESPONSE.X10enabled = (m_pWebEm->FindValue("X10") == "on") ? 1 : 0;
			Response.IRESPONSE.ARCenabled = (m_pWebEm->FindValue("ARC") == "on") ? 1 : 0;
			Response.IRESPONSE.ACenabled = (m_pWebEm->FindValue("AC") == "on") ? 1 : 0;
			Response.IRESPONSE.HEEUenabled = (m_pWebEm->FindValue("HomeEasyEU") == "on") ? 1 : 0;
			Response.IRESPONSE.MEIANTECHenabled = (m_pWebEm->FindValue("Meiantech") == "on") ? 1 : 0;
			Response.IRESPONSE.OREGONenabled = (m_pWebEm->FindValue("OregonScientific") == "on") ? 1 : 0;
			Response.IRESPONSE.ATIenabled = (m_pWebEm->FindValue("ATIremote") == "on") ? 1 : 0;
			Response.IRESPONSE.VISONICenabled = (m_pWebEm->FindValue("Visonic") == "on") ? 1 : 0;
			Response.IRESPONSE.MERTIKenabled = (m_pWebEm->FindValue("Mertik") == "on") ? 1 : 0;
			Response.IRESPONSE.LWRFenabled = (m_pWebEm->FindValue("ADLightwaveRF") == "on") ? 1 : 0;
			Response.IRESPONSE.HIDEKIenabled = (m_pWebEm->FindValue("HidekiUPM") == "on") ? 1 : 0;
			Response.IRESPONSE.LACROSSEenabled = (m_pWebEm->FindValue("LaCrosse") == "on") ? 1 : 0;
			Response.IRESPONSE.FS20enabled = (m_pWebEm->FindValue("FS20") == "on") ? 1 : 0;
			Response.IRESPONSE.PROGUARDenabled = (m_pWebEm->FindValue("ProGuard") == "on") ? 1 : 0;
			Response.IRESPONSE.BLINDST0enabled = (m_pWebEm->FindValue("BlindT0") == "on") ? 1 : 0;
			Response.IRESPONSE.BLINDST1enabled = (m_pWebEm->FindValue("BlindT1T2T3T4") == "on") ? 1 : 0;
			Response.IRESPONSE.AEenabled = (m_pWebEm->FindValue("AEBlyss") == "on") ? 1 : 0;
			Response.IRESPONSE.RUBICSONenabled = (m_pWebEm->FindValue("Rubicson") == "on") ? 1 : 0;
			Response.IRESPONSE.FINEOFFSETenabled = (m_pWebEm->FindValue("FineOffsetViking") == "on") ? 1 : 0;
			Response.IRESPONSE.LIGHTING4enabled = (m_pWebEm->FindValue("Lighting4") == "on") ? 1 : 0;
			Response.IRESPONSE.RSLenabled = (m_pWebEm->FindValue("RSL") == "on") ? 1 : 0;
			Response.IRESPONSE.SXenabled = (m_pWebEm->FindValue("ByronSX") == "on") ? 1 : 0;
			Response.IRESPONSE.IMAGINTRONIXenabled = (m_pWebEm->FindValue("ImaginTronix") == "on") ? 1 : 0;
			Response.IRESPONSE.KEELOQenabled = (m_pWebEm->FindValue("Keeloq") == "on") ? 1 : 0;

			m_mainworker.SetRFXCOMHardwaremodes(atoi(idx.c_str()), Response.ICMND.freqsel, Response.ICMND.xmitpwr, Response.ICMND.msg3, Response.ICMND.msg4, Response.ICMND.msg5, Response.ICMND.msg6);

			return (char*)m_retstr.c_str();
		}
		void CWebServer::Cmd_RFXComGetFirmwarePercentage(Json::Value &root)
		{
			root["status"] = "ERR";
			root["title"] = "GetFirmwareUpgradePercentage";
			std::string hardwareid = m_pWebEm->FindValue("hardwareid");

			CDomoticzHardwareBase *pHardware = NULL;
			if ((!hardwareid.empty()) && (hardwareid != "undefined"))
			{
				pHardware = m_mainworker.GetHardware(atoi(hardwareid.c_str()));
			}
			if (pHardware == NULL)
			{
				//Try to find the RFXCom hardware
				pHardware = m_mainworker.GetHardwareByType(HTYPE_RFXtrx433);
			}
			if (pHardware != NULL)
			{
				if (
					(pHardware->HwdType == HTYPE_RFXtrx315) ||
					(pHardware->HwdType == HTYPE_RFXtrx433)
					)
				{
					RFXComSerial *pRFXComSerial = (RFXComSerial *)pHardware;
					root["status"] = "OK";
					root["percentage"] = pRFXComSerial->GetUploadPercentage();
					root["message"] = pRFXComSerial->GetUploadMessage();
				}
			}
			else
				root["message"] = "Hardware not found, or not enabled!";
		}
	}
}
