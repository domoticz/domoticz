#include "stdafx.h"
#include "SolarMaxTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include "../main/SQLHelper.h"
#include "../hardware/hardwaretypes.h"

#include <sstream>

#define RETRY_DELAY 30
#define POLL_INTERVAL 30

#define SM_DEST_ADDRESS 0x01

SolarMaxTCP::SolarMaxTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_socket = INVALID_SOCKET;
	m_bufferpos = 0;
	m_retrycntr = RETRY_DELAY;
}

static int SolarMaxCalcChecksum(const unsigned char *pData, const int Len)
{
	int chksum = 0;
	for (int ii = 0; ii < Len; ii++)
	{
		chksum += pData[ii];
		chksum %= 0xFFFF;
	}
	return chksum;
}

std::string SolarMaxTCP::MakeRequestString()
{
	int SourceAddress = 0xFB;
	int DestAddress = SM_DEST_ADDRESS;
	std::string RequestString = "64:KDY;KT0;PAC;UDC;UL1;IDC;IL1;PIN;PRL;TNF;TKK";
	char szSendTemp[100];
	char szSendRequest[100];
	sprintf(szSendTemp, "%02X;%02X;%02X|%s|", SourceAddress, DestAddress, (unsigned int)(RequestString.size() + 16), RequestString.c_str());
	int Chksum = SolarMaxCalcChecksum((const unsigned char*)&szSendTemp, (int)strlen(szSendTemp));
	sprintf(szSendRequest, "{%s%04X}", szSendTemp, Chksum);
	return std::string(szSendRequest);
}

SolarMaxTCP::~SolarMaxTCP(void)
{
}

bool SolarMaxTCP::StartHardware()
{
	m_bIsStarted = true;

	m_stoprequested = false;

	memset(&m_addr, 0, sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_usIPPort);

	unsigned long ip;
	ip = inet_addr(m_szIPAddress.c_str());

	// if we have a error in the ip, it means we have entered a string
	if (ip != INADDR_NONE)
	{
		m_addr.sin_addr.s_addr = ip;
	}
	else
	{
		// change Hostname in serveraddr
		hostent *he = gethostbyname(m_szIPAddress.c_str());
		if (he == NULL)
		{
			return false;
		}
		else
		{
			memcpy(&(m_addr.sin_addr), he->h_addr_list[0], 4);
		}
	}

	char szIP[20];
	unsigned char *pAddress = (unsigned char *)&m_addr.sin_addr;
	sprintf(szIP, "%d.%d.%d.%d", (int)pAddress[0], (int)pAddress[1], (int)pAddress[2], (int)pAddress[3]);
	m_endpoint = szIP;

	m_retrycntr = RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&SolarMaxTCP::Do_Work, this)));

	return (m_thread != NULL);
}

bool SolarMaxTCP::StopHardware()
{
	if (isConnected())
	{
		try {
			disconnect();
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}
	else {
		try {
			if (m_thread)
			{
				m_stoprequested = true;
				m_thread->join();
			}
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}
	m_bIsStarted = false;
	return true;
}

bool SolarMaxTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		_log.Log(LOG_ERROR, "SolarMax: TCP could not create a TCP/IP socket!");
		return false;
	}
	/*
	//Set socket timeout to 2 minutes
	#if !defined WIN32
	struct timeval tv;
	tv.tv_sec = 120;
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
	#else
	unsigned long nTimeout = 120*1000;
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(DWORD));
	#endif
	*/
	// connect to the server
	int nRet;
	nRet = connect(m_socket, (const sockaddr*)&m_addr, sizeof(m_addr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		_log.Log(LOG_ERROR, "SolarMax: TCP could not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
		return false;
	}

	_log.Log(LOG_STATUS, "SolarMax: TCP connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	sOnConnected(this);
	return true;
}

void SolarMaxTCP::disconnect()
{
	m_stoprequested = true;
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);	//will terminate the thread
		m_socket = INVALID_SOCKET;
		sleep_seconds(1);
	}
	//m_thread-> join();
}

void SolarMaxTCP::Do_Work()
{
	char buf[1024];
	bool bFirstTime = true;
	int sec_counter = POLL_INTERVAL-5;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (
			(m_socket == INVALID_SOCKET) &&
			(!m_stoprequested)
			)
		{
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				if (!ConnectInternal())
				{
					_log.Log(LOG_STATUS, "SolarMax: retrying in %d seconds...", RETRY_DELAY);
				}
			}
		}
		else
		{
			if ((sec_counter % POLL_INTERVAL == 0) || (bFirstTime))
			{
				bFirstTime = false;

				//Request data from inverter
				std::string reqString = MakeRequestString();
				write(reqString.c_str(), reqString.size());

				//this could take a long time... maybe there will be no data received at all,
				//so it's no good to-do the heartbeat timing here

				int bread = recv(m_socket, (char*)&buf, sizeof(buf), 0);
				if (m_stoprequested)
					break;
				if (bread <= 0) {
					_log.Log(LOG_ERROR, "SolarMax: TCP/IP connection closed! %s", m_szIPAddress.c_str());
					closesocket(m_socket);
					m_socket = INVALID_SOCKET;
					if (!m_stoprequested)
					{
						_log.Log(LOG_STATUS, "SolarMax: retrying in %d seconds...", RETRY_DELAY);
						m_retrycntr = 0;
						continue;
					}
				}
				else
				{
					boost::lock_guard<boost::mutex> l(readQueueMutex);
					ParseData((const unsigned char *)&buf, bread);
				}
			}
		}

	}
	_log.Log(LOG_STATUS, "SolarMax: TCP/IP Worker stopped...");
}

void SolarMaxTCP::write(const char *data, size_t size)
{
	if (m_socket == INVALID_SOCKET)
		return; //not connected!
	send(m_socket, data, size, 0);
}

bool SolarMaxTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isConnected())
	{
		write(pdata, length);
		return true;
	}
	return false;
}

void SolarMaxTCP::ParseData(const unsigned char *pData, int Len)
{
	int ii = 0;
	while (ii < Len)
	{
		const unsigned char c = pData[ii];
		if (c == 0x0d)
		{
			ii++;
			continue;
		}

		if (c == '}' || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if (m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

static unsigned long SolarMaxGetHexStringValue(const std::string &svalue)
{
	unsigned long ret = -1;
	std::stringstream ss;
	ss << std::hex << svalue;
	ss >> ret;
	return ret;
}


void SolarMaxTCP::ParseLine()
{
	std::string InputStr = std::string((const char*)&m_buffer);
	size_t npos = InputStr.find("|");
	if (npos == std::string::npos)
	{
		_log.Log(LOG_ERROR, "SolarMax: Invalid data received!");
		return;
	}
	InputStr = InputStr.substr(npos + 4);
	npos = InputStr.find("|");
	if (npos == std::string::npos)
	{
		_log.Log(LOG_ERROR, "SolarMax: Invalid data received!");
		return;
	}
	InputStr = InputStr.substr(0,npos);

	std::vector<std::string> results;
	StringSplit(InputStr, ";", results);
	if (results.size() < 2)
		return; //invalid data

	std::vector<std::string>::const_iterator itt;

	double kwhCounter = 0;
	double ActUsage = 0;

	for (itt = results.begin(); itt != results.end(); ++itt)
	{
		std::vector<std::string> varresults;
		StringSplit(*itt, "=", varresults);
		if (varresults.size() !=2)
			continue;

		std::string sLabel = varresults[0];
		std::string sVal = varresults[1];

		if (sLabel == "KT0")
		{
			//Energy total
			kwhCounter = SolarMaxGetHexStringValue(sVal);// / 10.0f;
		}
		else if (sLabel == "KDY")
		{
			//Energy Today
		}
		else if (sLabel == "PAC")
		{
			//AC power
			ActUsage = SolarMaxGetHexStringValue(sVal)/2.0f;
		}
		else if (sLabel == "UDC")
		{
			//DC voltage [mV]
			float voltage = float(SolarMaxGetHexStringValue(sVal)) / 10.0f;
			SendVoltageSensor(1, 2, 255, voltage, "DC voltage");
		}
		else if (sLabel == "UL1")
		{
			//AC voltage [mV]
			float voltage = float(SolarMaxGetHexStringValue(sVal)) / 10.0f;
			SendVoltageSensor(1, 3, 255, voltage, "AC voltage");
		}
		else if (sLabel == "IDC")
		{
			//DC current [mA]
			float amps = float(SolarMaxGetHexStringValue(sVal)) / 100.0f;
			SendCurrentSensor(4, 255, amps, 0, 0, "DC current");
		}
		else if (sLabel == "IL1")
		{
			//AC current [mA]
			float amps = float(SolarMaxGetHexStringValue(sVal)) / 100.0f;
			SendCurrentSensor(5, 255, amps, 0, 0, "AC current");
		}
		else if (sLabel == "PIN")
		{
			//Power installed [mW] (PIN)
			//float power_installed = (float)SolarMaxGetHexStringValue(sVal);
		}
		else if (sLabel == "PRL")
		{
			//AC power [%]
			float percentage = (float)SolarMaxGetHexStringValue(sVal);
			SendPercentageSensor(6, 6, 255, percentage, "AC power Percentage");
		}
		else if (sLabel == "TNF")
		{
			//AC Frequency (Hz)
			float freq = (float)SolarMaxGetHexStringValue(sVal)/100;
			SendPercentageSensor(7, 7, 255, freq, "Hz");
		}
		else if (sLabel == "TKK")
		{
			//Temperature Heat Sink
			float temp = (float)SolarMaxGetHexStringValue(sVal);// / 10.0f;
			SendTempSensor(8, 255, temp,"Temperature Heat Sink");
		}
	}
	if (kwhCounter != 0)
	{
		SendKwhMeterOldWay(1, 1, 255, ActUsage/1000.0f, kwhCounter, "kWh Meter");
	}

}
