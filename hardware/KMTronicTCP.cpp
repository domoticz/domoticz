#include "stdafx.h"
#include "KMTronicTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"

#define KMTRONIC_POLL_INTERVAL 10


#ifdef _DEBUG
	//#define DEBUG_KMTrinicR
#endif

#ifdef DEBUG_KMTrinicR
std::string ReadFile(std::string filename)
{
	std::string sResult = "";
	std::ifstream is;
	is.open(filename, std::ios::in | std::ios::binary);
	if (is.is_open())
	{
		sResult.append((std::istreambuf_iterator<char>(is)),
			(std::istreambuf_iterator<char>()));
		is.close();
	}
	return sResult;
}
#endif


KMTronicTCP::KMTronicTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
m_szIPAddress(IPAddress),
m_Username(CURLEncode::URLEncode(username)),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_bCheckedForTempDevice = false;
	m_bIsTempDevice = false;
}

KMTronicTCP::~KMTronicTCP(void)
{
}

void KMTronicTCP::Init()
{
}

bool KMTronicTCP::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&KMTronicTCP::Do_Work, this);
	m_bIsStarted = true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "KMTronic: Started");
	return (m_thread != nullptr);
}

bool KMTronicTCP::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void KMTronicTCP::Do_Work()
{
	int sec_counter = KMTRONIC_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		int iPollInterval = KMTRONIC_POLL_INTERVAL;

		if (m_bIsTempDevice)
			iPollInterval = 30;

		if (sec_counter % iPollInterval == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS, "KMTronic: TCP/IP Worker stopped...");
}

bool KMTronicTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (m_bIsTempDevice)
		return false;
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if (packettype == pTypeLighting2)
	{
		//light command

		int Relay = pSen->LIGHTING2.id4;
		if (Relay > Max_KMTronic_Relais)
			return false;

		std::stringstream szURL;

		if (m_Password.empty())
		{
			szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
		}
		else
		{
			szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
		}

		//toggle=			//szURL << "/relays.cgi?relay=" << Relay;

		if (pSen->LIGHTING2.cmnd == light2_sOff)
		{
			szURL << "/FF0" << Relay << "00";
		}
		else
		{
			szURL << "/FF0" << Relay << "01";
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult,true))
		{
			_log.Log(LOG_ERROR, "KMTronic: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	return false;
}

bool KMTronicTCP::WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn)
{
	return true;
}

std::string KMTronicTCP::GenerateURL(const bool bIsTempDevice)
{
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	if (!bIsTempDevice)
		szURL << "/relays.cgi";
	else
		szURL << "/status.xml";

	return szURL.str();
}

void KMTronicTCP::GetMeterDetails()
{
	std::string sResult;
#ifndef DEBUG_KMTrinicR
	std::string szURL = GenerateURL(m_bIsTempDevice);

	if (!HTTPClient::GET(szURL, sResult))
	{
		if (!m_bCheckedForTempDevice)
		{
			m_bCheckedForTempDevice = true;

			szURL = GenerateURL(true);
			if (HTTPClient::GET(szURL, sResult))
			{
				m_bIsTempDevice = true;
			}
		}
		if (sResult.empty())
		{
			_log.Log(LOG_ERROR, "KMTronic: Error connecting to: %s", m_szIPAddress.c_str());
			return;
		}
	}
#else
	sResult = ReadFile("E:\\kmtronic_temp_status.xml");
	m_bIsTempDevice = (sResult.find("</temp>") != std::string::npos);
#endif
	if (!m_bIsTempDevice)
		ParseRelays(sResult);
	else
		ParseTemps(sResult);
}

void KMTronicTCP::ParseRelays(const std::string &sResult)
{
	std::vector<std::string> results;
	StringSplit(sResult, "\r\n", results);
	if (results.size() < 8)
	{
		_log.Log(LOG_ERROR, "KMTronic: Invalid data received");
		return;
	}
	size_t ii, jj;
	std::string tmpstr;
	for (ii = 1; ii < results.size(); ii++)
	{
		tmpstr = results[ii];
		if (tmpstr.find("Status:") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("Status:"));
			stdreplace(tmpstr, " ", "");
			for (jj = 0; jj < tmpstr.size(); jj++)
			{
				bool bIsOn = (tmpstr[jj] != '0');
				std::stringstream sstr;
				int iRelay = (jj + 1);
				sstr << "Relay " << iRelay;
				SendSwitch(iRelay, 1, 255, bIsOn, 0, sstr.str());
				if (iRelay > m_TotRelais)
					m_TotRelais = iRelay;
			}
			return;
		}
	}
	_log.Log(LOG_ERROR, "KMTronic: Invalid data received");
}

void KMTronicTCP::ParseTemps(const std::string &sResult)
{
	std::vector<std::string> results;
	StringSplit(sResult, "\r\n", results);
	if (results.size() < 8)
	{
		_log.Log(LOG_ERROR, "KMTronic: Invalid data received");
		return;
	}
	size_t ii;
	std::string tmpstr;
	std::string name;
	int Idx = 1;

	for (ii = 1; ii < results.size(); ii++)
	{
		tmpstr = stdstring_trim(results[ii]);

		int pos1;
		pos1 = tmpstr.find("<name>");
		if (pos1 != std::string::npos)
		{
			tmpstr = tmpstr.substr(pos1 + strlen("<name>"));
			pos1 = tmpstr.find("</name>");
			if (pos1 != std::string::npos)
			{
				name = tmpstr.substr(0, pos1);
				continue;
			}
		}
		pos1 = tmpstr.find("<temp>");
		if (pos1 != std::string::npos)
		{
			tmpstr = tmpstr.substr(pos1 + strlen("<temp>"));
			pos1 = tmpstr.find("</temp>");
			if (pos1 != std::string::npos)
			{
				float temp = (float)atof(tmpstr.substr(0, pos1).c_str());
				SendTempSensor(Idx++, 255, temp, name);
				continue;
			}
		}
	}
}

