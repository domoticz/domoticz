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

KMTronicTCP::KMTronicTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
m_szIPAddress(IPAddress),
m_Username(CURLEncode::URLEncode(username)),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
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
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&KMTronicTCP::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "KMTronic: Started");
	return (m_thread != NULL);
}

bool KMTronicTCP::StopHardware()
{
	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
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

		if (sec_counter % KMTRONIC_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS, "KMTronic: TCP/IP Worker stopped...");
} 

bool KMTronicTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
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

void KMTronicTCP::GetMeterDetails()
{
	std::string sResult;
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/relays.cgi";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "KMTronic: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, "\r\n", results);
	if (results.size()<8)
	{
		_log.Log(LOG_ERROR, "KMTronic: Invalid data received");
		return;
	}
	size_t ii,jj;
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
