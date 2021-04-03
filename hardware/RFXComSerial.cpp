#include "stdafx.h"
#include "RFXComSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>

#include <string>
#include <algorithm>
#include <iostream>
#include <ctime>

#ifndef WIN32
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#define RETRY_DELAY 30

#define RFX_WRITE_DELAY 300

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

#define PKT_pmrangelow	0x001800
#define PKT_pmrangehigh	0x00A7FF

#define PKT_pmrangelow868	0x001000
#define PKT_pmrangehigh868 0x0147FF

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
RFXComSerial::RFXComSerial(const int ID, const std::string& devname, unsigned int baud_rate, const _eRFXAsyncType AsyncType) :
	m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_iBaudRate = baud_rate;

	m_AsyncType = AsyncType;

	m_bReceiverStarted = false;
	m_bInBootloaderMode = false;
	m_bStartFirmwareUpload = false;
	m_FirmwareUploadPercentage = 0;
	m_bHaveRX = false;
	m_rx_tot_bytes = 0;
	m_retrycntr = RETRY_DELAY;

	m_serial.setPort(m_szSerialPort);
	m_serial.setBaudrate(m_iBaudRate);
	m_serial.setBytesize(serial::eightbits);
	m_serial.setParity(serial::parity_none);
	m_serial.setStopbits(serial::stopbits_one);
	m_serial.setFlowcontrol(serial::flowcontrol_none);

	serial::Timeout stimeout = serial::Timeout::simpleTimeout(200);
	m_serial.setTimeout(stimeout);
}

bool RFXComSerial::StartHardware()
{
	RequestStart();

	//return OpenSerialDevice();
	//somehow retry does not seem to work?!
	m_bReceiverStarted = false;

	m_retrycntr = RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);

}

bool RFXComSerial::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void RFXComSerial::Do_Work()
{
	int sec_counter = 0;

	Log(LOG_STATUS, "Worker started...");

	while (IsStopRequested(1000) == false)
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (m_bStartFirmwareUpload)
		{
			m_bStartFirmwareUpload = false;
			terminate();
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
			if (m_retrycntr == 0)
			{
				Log(LOG_STATUS, "retrying in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				OpenSerialDevice();
			}
		}
	}
	terminate(); //Close serial port (if open)

	Log(LOG_STATUS, "Worker stopped...");
}


bool RFXComSerial::OpenSerialDevice(const bool bIsFirmwareUpgrade)
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, m_iBaudRate);
		Log(LOG_STATUS, "Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		Log(LOG_ERROR, "Error opening serial port!");
#ifdef _DEBUG
		Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error opening serial port!!!");
		return false;
	}
	m_bIsStarted = true;
	m_rxbufferpos = 0;
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
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
	int AddressLow = PKT_pmrangelow;
	int AddressHigh = PKT_pmrangehigh;
	if (HwdType == HTYPE_RFXtrx868)
	{
		AddressLow = PKT_pmrangelow868;
		AddressHigh = PKT_pmrangehigh868;
	}
	m_FirmwareUploadPercentage = 0;
	m_bStartFirmwareUpload = false;
	std::map<unsigned long, std::string> firmwareBuffer;
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
		m_szUploadMessage = "Error opening serial port!!!";
		Log(LOG_ERROR, m_szUploadMessage);
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
	//Start bootloader mode
	m_szUploadMessage = "Start bootloader process...";
	Log(LOG_STATUS, m_szUploadMessage);
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);
	m_szUploadMessage = "Get bootloader version...";
	Log(LOG_STATUS, m_szUploadMessage);
	//read bootloader version
	if (!Write_TX_PKT(PKT_VERSION, sizeof(PKT_VERSION)))
	{
		m_szUploadMessage = "Error getting bootloader version!!!";
		Log(LOG_ERROR, m_szUploadMessage);
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
	Log(LOG_STATUS, "bootloader version v%d.%d", m_rx_input_buffer[3], m_rx_input_buffer[2]);

	if (!EraseMemory(AddressLow, AddressHigh))
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
		m_szUploadMessage = "Bootloader, unable to flush serial device!!!";
		Log(LOG_ERROR, m_szUploadMessage);
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
		m_szUploadMessage = "Error opening serial port!!!";
		Log(LOG_ERROR, m_szUploadMessage);
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}

	m_szUploadMessage = "Start bootloader version...";
	Log(LOG_STATUS, m_szUploadMessage);
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);
	Write_TX_PKT(PKT_STARTBOOT, sizeof(PKT_STARTBOOT), 1);

	m_szUploadMessage = "Bootloader, Start programming...";
	Log(LOG_STATUS, m_szUploadMessage);
	for (const auto &firmware : firmwareBuffer)
	{
		icntr++;
		if (icntr % 5 == 0)
		{
			m_LastHeartbeat = mytime(nullptr);
		}
		unsigned long Address = firmware.first;
		m_FirmwareUploadPercentage = (100.0F / float(firmwareBuffer.size())) * icntr;
		if (m_FirmwareUploadPercentage > 100)
			m_FirmwareUploadPercentage = 100;

		//if ((Address >= AddressLow) && (Address <= AddressHigh))
		{
			std::stringstream saddress;
			saddress << "Programming Address: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << Address;

			std::stringstream spercentage;
			spercentage.precision(2);
			spercentage << std::setprecision(2) << std::fixed << m_FirmwareUploadPercentage;
			m_szUploadMessage = saddress.str() + ", " + spercentage.str() + " %";
			_log.Log(LOG_STATUS, m_szUploadMessage);

			unsigned char bcmd[PKT_writeblock + 10];
			bcmd[0] = COMMAND_WRITEPM;
			bcmd[1] = 1;
			bcmd[2] = Address & 0xFF;
			bcmd[3] = (Address & 0xFF00) >> 8;
			bcmd[4] = (unsigned char)((Address & 0xFF0000) >> 16);
			memcpy(bcmd + 5, firmware.second.c_str(), firmware.second.size());
			bool ret = Write_TX_PKT(bcmd, 5 + firmware.second.size(), 20);
			if (!ret)
			{
				m_szUploadMessage = "Bootloader, unable to program firmware memory, please try again!!!";
				Log(LOG_ERROR, m_szUploadMessage);
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
		m_szUploadMessage = "Bootloader, unable to flush serial device!!!";
		Log(LOG_ERROR, m_szUploadMessage);
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
#endif
	//Verify
	m_szUploadMessage = "Start bootloader verify...";
	Log(LOG_STATUS, m_szUploadMessage);
	if (!Write_TX_PKT(PKT_VERIFY_OK, sizeof(PKT_VERIFY_OK)))
	{
		m_szUploadMessage = "Bootloader,  program firmware memory not succeeded, please try again!!!";
		Log(LOG_ERROR, m_szUploadMessage);
		m_FirmwareUploadPercentage = -1;
		goto exitfirmwareupload;
	}
	if (m_rx_input_buffer[0] != PKT_VERIFY_OK[0])
	{
		m_FirmwareUploadPercentage = -1;
		m_szUploadMessage = "Bootloader,  program firmware memory not succeeded, please try again!!!";
		Log(LOG_ERROR, m_szUploadMessage);
	}
	else
	{
		m_szUploadMessage = "Bootloader, Programming completed successfully...";
		Log(LOG_STATUS, m_szUploadMessage);
	}
exitfirmwareupload:
	m_szUploadMessage = "bootloader reset...";
	Log(LOG_STATUS, m_szUploadMessage);
	Write_TX_PKT(PKT_RESET, sizeof(PKT_RESET), 1);
#ifndef WIN32
	try
	{
		m_serial.flush();
	}
	catch (...)
	{
		m_szUploadMessage = "Bootloader, unable to flush serial device!!!";
		Log(LOG_ERROR, m_szUploadMessage);
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
	m_FirmwareUploadPercentage = 100;
	RequestStart();
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
	if (stat(szFilename, &info) == 0)
	{
		struct passwd *pw = getpwuid(info.st_uid);
		int ret = chown(szFilename, pw->pw_uid, pw->pw_gid);
		if (ret != 0)
		{
			m_szUploadMessage = "Error setting firmware ownership (chown returned an error!)";
			Log(LOG_ERROR, m_szUploadMessage);
			return false;
		}
	}
#endif

	std::ifstream infile;
	std::string sLine;
	infile.open(szFilename);
	if (!infile.is_open())
	{
		m_szUploadMessage = "bootloader, unable to open file: " + std::string(szFilename);
		Log(LOG_ERROR, m_szUploadMessage);
		return false;
	}

	m_szUploadMessage = "start reading Firmware...";
	Log(LOG_STATUS, m_szUploadMessage);

	unsigned char rawLineBuf[PKT_writeblock];
	int raw_length = 0;
	unsigned long dest_address = 0;
	int line = 0;
	int addrh = 0;

	fileBuffer.clear();
	std::string dstring;
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
			m_szUploadMessage = "bootloader, firmware does not start with ':'";
			Log(LOG_ERROR, m_szUploadMessage);
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
			m_szUploadMessage = "bootloader, firmware line not equals 2 digests";
			Log(LOG_ERROR, m_szUploadMessage);
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
				m_szUploadMessage = "bootloader, incorrect length";
				Log(LOG_ERROR, m_szUploadMessage);
				return false;
			}

		}
		//
		chksum = ~chksum + 1;
		if ((chksum != rawLineBuf[raw_length - 1]) || (raw_length < 4))
		{
			infile.close();
			m_szUploadMessage = "bootloader, checksum mismatch!";
			Log(LOG_ERROR, m_szUploadMessage);
			return false;
		}
		int byte_count = rawLineBuf[0];
		int faddress = (rawLineBuf[1] << 8) | rawLineBuf[2];
		int rtype = rawLineBuf[3];

		switch (rtype)
		{
		case 0:
			//Data record
			dstring += std::string((const char*)&rawLineBuf + 4, (const char*)rawLineBuf + 4 + byte_count);
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
				m_szUploadMessage = "Bootloader invalid size!";
				Log(LOG_ERROR, m_szUploadMessage);
			}
			break;
		case 2:
			//Extended Segment Address Record
			m_szUploadMessage = "Bootloader type 2 not supported!";
			Log(LOG_ERROR, m_szUploadMessage);
			infile.close();
			return false;
		case 3:
			//Start Segment Address Record
			m_szUploadMessage = "Bootloader type 3 not supported!";
			Log(LOG_ERROR, m_szUploadMessage);
			infile.close();
			return false;
		case 4:
			//Extended Linear Address Record
			if (raw_length < 7)
			{
				m_szUploadMessage = "Invalid line length!!";
				Log(LOG_ERROR, m_szUploadMessage);
				infile.close();
				return false;
			}
			addrh = (rawLineBuf[4] << 8) | rawLineBuf[5];
			break;
		case 5:
			//Start Linear Address Record
			m_szUploadMessage = "Bootloader type 5 not supported!";
			Log(LOG_ERROR, m_szUploadMessage);
			infile.close();
			return false;
		}
		line++;
	}
	infile.close();
	if (!bHaveEOF)
	{
		m_szUploadMessage = "No end-of-line found!!";
		Log(LOG_ERROR, m_szUploadMessage);
		return false;
	}
	m_szUploadMessage = "Firmware read correctly...";
	Log(LOG_STATUS, m_szUploadMessage);
	return true;
}

bool RFXComSerial::EraseMemory(const int StartAddress, const int StopAddress)
{
	m_szUploadMessage = "Erasing memory....";
	Log(LOG_STATUS, m_szUploadMessage);
	int BootAddr = StartAddress;

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
			m_szUploadMessage = "Error erasing memory!";
			Log(LOG_ERROR, m_szUploadMessage);
			return false;
		}
		BootAddr += (PKT_eraseblock * nBlocks);
	}
	m_szUploadMessage = "Erasing memory completed....";
	Log(LOG_STATUS, m_szUploadMessage);
	return true;
}

bool RFXComSerial::Write_TX_PKT(const unsigned char *pdata, size_t length, int max_retry)
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
	while (max_retry > 0)
	{
		try
		{
			size_t twrite = m_serial.write((const uint8_t *)&output_buffer, tot_bytes);
			sleep_milliseconds(RFX_WRITE_DELAY);
			if (twrite == tot_bytes)
			{
				int rcount = 0;
				while (rcount < 2)
				{
					if (Read_TX_PKT())
						return true;
					sleep_milliseconds(500);
					rcount++;
				}
			}
			max_retry--;
		}
		catch (...)
		{
			return false;
		}
	}
	return m_bHaveRX;
}

//Reads data from serial port between STX/ETX
bool RFXComSerial::Read_TX_PKT()
{
	uint8_t sbuffer[512];
	size_t buffer_offset = 0;
	bool bSTXFound1 = false;
	bool bSTXFound2 = false;
	bool bETXFound = false;
	bool bHadDLE = false;
	unsigned char chksum = 0;
	m_rx_tot_bytes = 0;
	while (m_rx_tot_bytes < sizeof(m_rx_input_buffer))
	{
		size_t tot_read = m_serial.read((uint8_t*)&sbuffer, sizeof(sbuffer));
		if (tot_read <= 0)
			return false;
		int ii = 0;
		while (tot_read > 0)
		{
			uint8_t tByte = sbuffer[ii++];
			if (!bSTXFound1)
			{
				if (tByte != PKT_STX)
					return false;
				bSTXFound1 = true;
			}
			else if (!bSTXFound2)
			{
				if (tByte != PKT_STX)
					return false;
				bSTXFound2 = true;
				chksum = 0;
				bHadDLE = false;
				m_rx_tot_bytes = 0;
			}
			else
			{
				//Read data until ETX is found
				if ((tByte == PKT_ETX) && (!bHadDLE))
				{
					chksum = ~chksum + 1; //test checksum
					if (chksum != 0)
					{
						Log(LOG_ERROR, "bootloader, received response with invalid checksum!");
						return false;
					}
					m_bHaveRX = true;
					return true;
				}
				if (tByte == PKT_DLE)
				{
					bHadDLE = true;
				}
				else
				{
					bHadDLE = false;
					chksum += tByte;
					m_rx_input_buffer[m_rx_tot_bytes++] = tByte;
				}
			}
			tot_read--;
		}
	}
	return ((buffer_offset > 0) && (bETXFound));
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
	while ((ii < length) && (m_rx_tot_bytes < sizeof(m_rx_input_buffer) - 1))
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
				Log(LOG_ERROR, "bootloader, received response with invalid checksum!");
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
			dbyte = pdata[ii + 1];
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
	std::lock_guard<std::mutex> l(readQueueMutex);
	try
	{
		if (!m_bInBootloaderMode)
		{
			bool bRet = onInternalMessage((const unsigned char *)data, len);
			if (bRet == false)
			{
				//close serial connection, and restart
				terminate();

			}
		}
	}
	catch (...)
	{

	}
}

bool RFXComSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return false;
	if (m_bInBootloaderMode)
		return false;
	write(pdata, length);
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RFXComUpgradeFirmware(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hardwareid = request::findValue(&req, "hardwareid");
			std::string firmwarefile = request::findValue(&req, "firmwarefile");

			if (firmwarefile.empty())
			{
				return;
			}

			CDomoticzHardwareBase *pHardware = nullptr;
			if ((!hardwareid.empty()) && (hardwareid != "undefined"))
			{
				pHardware = m_mainworker.GetHardware(atoi(hardwareid.c_str()));
			}
			if (pHardware == nullptr)
			{
				//Direct Entry, try to find the RFXCom hardware
				pHardware = m_mainworker.GetHardwareByType(HTYPE_RFXtrx433);
				if (pHardware == nullptr)
				{
					pHardware = m_mainworker.GetHardwareByType(HTYPE_RFXtrx868);
					if (pHardware == nullptr)
					{
						return;
					}
				}
			}
#ifdef WIN32
			std::string outputfile = szStartupFolder + "rfx_firmware.hex";
#else
			std::string outputfile = "/tmp/rfx_firmware.hex";
#endif
			std::ofstream outfile;
			outfile.open(outputfile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
			if (!outfile.is_open())
				return;
			outfile << firmwarefile;
			outfile.flush();
			outfile.close();

			if (
				(pHardware->HwdType == HTYPE_RFXtrx315) ||
				(pHardware->HwdType == HTYPE_RFXtrx433) ||
				(pHardware->HwdType == HTYPE_RFXtrx868)
				)
			{
				RFXComSerial *pRFXComSerial = reinterpret_cast<RFXComSerial *>(pHardware);
				pRFXComSerial->UploadFirmware(outputfile);
			}
		}

		void CWebServer::SetRFXCOMMode(WebEmSession & session, const request& req, std::string & redirect_uri)
		{
			redirect_uri = "/index.html";

			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
			{
				return;
			}
			std::vector<std::vector<std::string> > result;

			result = m_sql.safe_query("SELECT Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, [Type] FROM Hardware WHERE (ID='%q')", idx.c_str());
			if (result.empty())
				return;

			unsigned char Mode1 = atoi(result[0][0].c_str());
			unsigned char Mode2 = atoi(result[0][1].c_str());
			unsigned char Mode3 = atoi(result[0][2].c_str());
			unsigned char Mode4 = atoi(result[0][3].c_str());
			unsigned char Mode5 = atoi(result[0][4].c_str());
			unsigned char Mode6 = atoi(result[0][5].c_str());

			_eHardwareTypes HWType = (_eHardwareTypes)atoi(result[0][6].c_str());

			tRBUF Response;
			Response.ICMND.freqsel = Mode1;
			Response.ICMND.xmitpwr = Mode2;
			Response.ICMND.msg3 = Mode3;
			Response.ICMND.msg4 = Mode4;
			Response.ICMND.msg5 = Mode5;
			Response.ICMND.msg6 = Mode6;

			if (HWType != HTYPE_RFXtrx868)
			{
				Response.IRESPONSE.UNDECODEDenabled = (request::findValue(&req, "undecon") == "on") ? 1 : 0;
				Response.IRESPONSE.X10enabled = (request::findValue(&req, "X10") == "on") ? 1 : 0;
				Response.IRESPONSE.ARCenabled = (request::findValue(&req, "ARC") == "on") ? 1 : 0;
				Response.IRESPONSE.ACenabled = (request::findValue(&req, "AC") == "on") ? 1 : 0;
				Response.IRESPONSE.HEEUenabled = (request::findValue(&req, "HomeEasyEU") == "on") ? 1 : 0;
				Response.IRESPONSE.MEIANTECHenabled = (request::findValue(&req, "Meiantech") == "on") ? 1 : 0;
				Response.IRESPONSE.OREGONenabled = (request::findValue(&req, "OregonScientific") == "on") ? 1 : 0;
				Response.IRESPONSE.ATIenabled = (request::findValue(&req, "ATIremote") == "on") ? 1 : 0;
				Response.IRESPONSE.VISONICenabled = (request::findValue(&req, "Visonic") == "on") ? 1 : 0;
				Response.IRESPONSE.MERTIKenabled = (request::findValue(&req, "Mertik") == "on") ? 1 : 0;
				Response.IRESPONSE.LWRFenabled = (request::findValue(&req, "ADLightwaveRF") == "on") ? 1 : 0;
				Response.IRESPONSE.HIDEKIenabled = (request::findValue(&req, "HidekiUPM") == "on") ? 1 : 0;
				Response.IRESPONSE.LACROSSEenabled = (request::findValue(&req, "LaCrosse") == "on") ? 1 : 0;
				Response.IRESPONSE.LEGRANDenabled = (request::findValue(&req, "Legrand") == "on") ? 1 : 0;
				Response.IRESPONSE.MSG4Reserved5 = (request::findValue(&req, "ProGuard") == "on") ? 1 : 0;
				Response.IRESPONSE.BLINDST0enabled = (request::findValue(&req, "BlindT0") == "on") ? 1 : 0;
				Response.IRESPONSE.BLINDST1enabled = (request::findValue(&req, "BlindT1T2T3T4") == "on") ? 1 : 0;
				Response.IRESPONSE.AEenabled = (request::findValue(&req, "AEBlyss") == "on") ? 1 : 0;
				Response.IRESPONSE.RUBICSONenabled = (request::findValue(&req, "Rubicson") == "on") ? 1 : 0;
				Response.IRESPONSE.FINEOFFSETenabled = (request::findValue(&req, "FineOffsetViking") == "on") ? 1 : 0;
				Response.IRESPONSE.LIGHTING4enabled = (request::findValue(&req, "Lighting4") == "on") ? 1 : 0;
				Response.IRESPONSE.RSLenabled = (request::findValue(&req, "RSL") == "on") ? 1 : 0;
				Response.IRESPONSE.SXenabled = (request::findValue(&req, "ByronSX") == "on") ? 1 : 0;
				Response.IRESPONSE.IMAGINTRONIXenabled = (request::findValue(&req, "ImaginTronix") == "on") ? 1 : 0;
				Response.IRESPONSE.KEELOQenabled = (request::findValue(&req, "Keeloq") == "on") ? 1 : 0;
				Response.IRESPONSE.HCEnabled = (request::findValue(&req, "HC") == "on") ? 1 : 0;

				CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
				if (pHardware)
				{
					CRFXBase *pBase = reinterpret_cast<CRFXBase *>(pHardware);
					pBase->SetRFXCOMHardwaremodes(Response.ICMND.freqsel, Response.ICMND.xmitpwr, Response.ICMND.msg3, Response.ICMND.msg4, Response.ICMND.msg5, Response.ICMND.msg6);

					if (pBase->m_Version.find("Pro XL") != std::string::npos)
					{
						std::string AsyncMode = request::findValue(&req, "combo_rfx_xl_async_type");
						if (AsyncMode.empty())
							AsyncMode = "0";
						result = m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID='%q')", AsyncMode.c_str(), idx.c_str());
						pBase->SetAsyncType((CRFXBase::_eRFXAsyncType)atoi(AsyncMode.c_str()));
					}
				}
			}
			else
			{
				//For now disable setting the protocols on a 868MHz device
			}
		}

		void CWebServer::Cmd_RFXComGetFirmwarePercentage(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["status"] = "ERR";
			root["title"] = "GetFirmwareUpgradePercentage";
			std::string hardwareid = request::findValue(&req, "hardwareid");

			CDomoticzHardwareBase *pHardware = nullptr;
			if ((!hardwareid.empty()) && (hardwareid != "undefined"))
			{
				pHardware = m_mainworker.GetHardware(atoi(hardwareid.c_str()));
			}
			if (pHardware == nullptr)
			{
				//Direct Entry, try to find the RFXCom hardware
				pHardware = m_mainworker.GetHardwareByType(HTYPE_RFXtrx433);
				if (pHardware == nullptr)
				{
					pHardware = m_mainworker.GetHardwareByType(HTYPE_RFXtrx868);
				}
			}
			if (pHardware != nullptr)
			{
				if (
					(pHardware->HwdType == HTYPE_RFXtrx315) ||
					(pHardware->HwdType == HTYPE_RFXtrx433) ||
					(pHardware->HwdType == HTYPE_RFXtrx868)
					)
				{
					RFXComSerial *pRFXComSerial = reinterpret_cast<RFXComSerial *>(pHardware);
					root["status"] = "OK";
					root["percentage"] = pRFXComSerial->GetUploadPercentage();
					root["message"] = pRFXComSerial->GetUploadMessage();
				}
			}
			else
				root["message"] = "Hardware not found, or not enabled!";
		}
	} // namespace server
} // namespace http
