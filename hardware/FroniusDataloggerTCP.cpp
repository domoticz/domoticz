#include "stdafx.h"

#include <iostream>
#include <sstream>

#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#include "../../httpclient/HTTPClient.h"
#include "../json/json.h"

#include "../hardware/hardwaretypes.h"
#include "../hardware/FroniusDataloggerTCP.h"

using namespace std;

#define RETRY_DELAY		300
#define POLL_INTERVAL	30

#define SM_DEST_ADDRESS 0x01

FroniusDataLoggerTCP::FroniusDataLoggerTCP(const int ID, const std::string & IPAddress, const unsigned short usIPPort) :
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

FroniusDataLoggerTCP::~FroniusDataLoggerTCP(void)
{
}


bool FroniusDataLoggerTCP::SendHwData(Json::Value data) {


	if (!data["PAC"]["Values"]["1"].empty() && !data["TOTAL_ENERGY"]["Values"]["1"].empty())
	{
//		_log.Log(LOG_ERROR, "eccomi con i dati %d", data["PAC"]["Values"]["1"].asInt());
		int ActUsage = data["PAC"]["Values"]["1"].asInt();

		int kwhCounter = data["TOTAL_ENERGY"]["Values"]["1"].asInt();
		SendKwhMeter(1, 1, 255, ActUsage, kwhCounter / 1000.0f, "kWh Meter");
		return true;
	}
	return false;
}

stringstream FroniusDataLoggerTCP::MakeRequestString()
{

	stringstream sstr2;
	
																		     
	sstr2 << "http://" << this->m_szIPAddress << ":" << this->m_usIPPort << "/solar_api/v1/GetInverterRealtimeData.cgi?Scope=System&DataCollection=CommonInverterData";

	return sstr2;

}

bool FroniusDataLoggerTCP::StartHardware()
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

	StartHeartbeatThread();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&FroniusDataLoggerTCP::Do_Work, this)));

	return (m_thread != NULL);
}

bool FroniusDataLoggerTCP::StopHardware()
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

bool FroniusDataLoggerTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		_log.Log(LOG_ERROR, "Fronius: TCP could not create a TCP/IP socket!");
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
		_log.Log(LOG_ERROR, "Fronius: TCP could not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
		return false;
	}

	_log.Log(LOG_STATUS, "Fronius: TCP connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	sOnConnected(this);
	return true;
}

void FroniusDataLoggerTCP::disconnect()
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

void FroniusDataLoggerTCP::Do_Work()
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
					_log.Log(LOG_STATUS, "Fronius: retrying in %d seconds...", RETRY_DELAY);
				}
			}
		}
		else
		{
			if ((sec_counter % POLL_INTERVAL == 0) || (bFirstTime))
			{
				vector<string> ExtraHeaders;
				string sResult;
				stringstream sPostData;

				bFirstTime = false;

				//Request data from inverter
				stringstream reqString = MakeRequestString();

				if (!HTTPClient::GET(reqString.str(), ExtraHeaders, sResult))
				{
					_log.Log(LOG_ERROR, "Fronius: Cannot get data");// this also happends when plant is off or no sun. We just keep polling. Silently
					continue;
				}

				if (m_stoprequested)
					break;
				try
				{

					Json::Value root, data;
					Json::Reader jReader;
					bool ret = jReader.parse(sResult, root);
					
					if (!ret || root.size() < 1)
					{
						_log.Log(LOG_ERROR, "Fronius: Invalid data received!");
						continue;
					}

					if (!root["Body"]["Data"].empty())
						SendHwData(root["Body"]["Data"]);
					else
						_log.Log(LOG_ERROR, "Fronius: no data");

				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "Fronis Error: Error parsing JSon data!");
				}
			}
		}

	}
	_log.Log(LOG_STATUS, "Fronius: TCP/IP Worker stopped...");
}

void FroniusDataLoggerTCP::write(const char *data, size_t size)
{
	if (m_socket == INVALID_SOCKET)
		return; //not connected!
	send(m_socket, data, size, 0);
}


bool FroniusDataLoggerTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}


#if 0
	if (sLabel == "KT0")
		{
			//Energy total
			kwhCounter = FroniusGetHexStringValue(sVal);// / 10.0f;
		}
		else if (sLabel == "KDY")
		{
			//Energy Today
		}
		else if (sLabel == "PAC")
		{
			//AC power
			ActUsage = FroniusGetHexStringValue(sVal)/2.0f;
		}
		else if (sLabel == "UDC")
		{
			//DC voltage [mV]
			float voltage = float(FroniusGetHexStringValue(sVal)) / 10.0f;
			SendVoltageSensor(1, 2, 255, voltage, "DC voltage");
		}
		else if (sLabel == "UL1")
		{
			//AC voltage [mV]
			float voltage = float(FroniusGetHexStringValue(sVal)) / 10.0f;
			SendVoltageSensor(1, 3, 255, voltage, "AC voltage");
		}
		else if (sLabel == "IDC")
		{
			//DC current [mA]
			float amps = float(FroniusGetHexStringValue(sVal)) / 100.0f;
			SendCurrentSensor(4, 255, amps, 0, 0, "DC current");
		}
		else if (sLabel == "IL1")
		{
			//AC current [mA]
			float amps = float(FroniusGetHexStringValue(sVal)) / 100.0f;
			SendCurrentSensor(5, 255, amps, 0, 0, "AC current");
		}
		else if (sLabel == "PIN")
		{
			//Power installed [mW] (PIN)
			//float power_installed = (float)FroniusGetHexStringValue(sVal);
		}
		else if (sLabel == "PRL")
		{
			//AC power [%]
			float percentage = (float)FroniusGetHexStringValue(sVal);
			SendPercentageSensor(6, 6, 255, percentage, "AC power Percentage");
		}
		else if (sLabel == "TNF")
		{
			//AC Frequency (Hz)
			float freq = (float)FroniusGetHexStringValue(sVal)/100;
			SendPercentageSensor(7, 7, 255, freq, "Hz");
		}
		else if (sLabel == "TKK")
		{
			//Temperature Heat Sink
			float temp = (float)FroniusGetHexStringValue(sVal);// / 10.0f;
			SendTempSensor(8, 255, temp,"Temperature Heat Sink");
		}
	}
	if (kwhCounter != 0)
	{
		SendKwhMeterOldWay(1, 1, 255, ActUsage/1000.0f, kwhCounter, "kWh Meter");
	}

#endif
