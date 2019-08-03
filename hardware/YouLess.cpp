#include "stdafx.h"
#include "YouLess.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../json/json.h"

#define YOULESS_POLL_INTERVAL 10

CYouLess::CYouLess(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password) :
m_szIPAddress(IPAddress),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	Init();
}

CYouLess::~CYouLess(void)
{
}

void CYouLess::Init()
{
	m_meter.len=sizeof(YouLessMeter)-1;
	m_meter.type=pTypeYouLess;
	m_meter.subtype=sTypeYouLess;
	m_meter.powerusage=0;
	m_meter.usagecurrent=0;
	m_meter.ID1=m_usIPPort;

	memset(&m_p1power, 0, sizeof(m_p1power));
	memset(&m_p1gas, 0, sizeof(m_p1gas));

	m_p1power.len = sizeof(P1Power) - 1;
	m_p1power.type = pTypeP1Power;
	m_p1power.subtype = sTypeP1Power;
	m_p1power.ID = 1;

	m_p1gas.len = sizeof(P1Gas) - 1;
	m_p1gas.type = pTypeP1Gas;
	m_p1gas.subtype = sTypeP1Gas;
	m_p1gas.ID = 1;

	m_bHaveP1 = false;
	m_bCheckP1 = true;
	m_lastgasusage = 0;
	m_lastSharedSendGas = mytime(NULL);
}

bool CYouLess::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CYouLess::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CYouLess::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
    return true;
}

void CYouLess::Do_Work()
{
	int sec_counter = YOULESS_POLL_INTERVAL - 2;

	_log.Log(LOG_STATUS, "YouLess: Worker started...");
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (sec_counter % YOULESS_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"YouLess: Worker stopped...");
}

bool CYouLess::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

bool CYouLess::GetP1Details()
{
	m_bCheckP1 = false;

	std::string sResult;
	std::stringstream szURL;

	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/e";
	if (!m_Password.empty())
		szURL << "&w=" << m_Password;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "YouLess: Error getting meter details from %s !", m_szIPAddress.c_str() );
		return false;
	}
	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (root.empty()))
	{
		return false;
	}
	if (root.size() < 1)
		return false;
	root = root[0];


	if (!root["p1"].empty())
	{
		int Pwr = root["pwr"].asInt();

		m_p1power.powerusage1 = (unsigned long)(root["p1"].asDouble() * 1000);
		m_p1power.powerusage2 = (unsigned long)(root["p2"].asDouble() * 1000);
		m_p1power.powerdeliv1 = (unsigned long)(root["n1"].asDouble() * 1000);
		m_p1power.powerdeliv2 = (unsigned long)(root["n2"].asDouble() * 1000);

		if (Pwr >= 0)
		{
			m_p1power.usagecurrent = Pwr;
			m_p1power.delivcurrent = 0;
		}
		else
		{
			m_p1power.delivcurrent = -Pwr;
			m_p1power.usagecurrent = 0;
		}
		sDecodeRXMessage(this, (const unsigned char *)&m_p1power, "Power", 255);

		m_p1gas.gasusage = (unsigned long)(root["gas"].asDouble() * 1000);
		time_t atime = mytime(NULL);
		if (
			(m_p1gas.gasusage != m_lastgasusage) ||
			(difftime(atime, m_lastSharedSendGas) >= 300)
			)
		{
			m_lastgasusage = m_p1gas.gasusage;
			m_lastSharedSendGas = atime;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1gas, "Gas", 255);
		}
		m_bHaveP1 = true;
	}

	if (!root["cs0"].empty())
	{
		//S0 Meter
		double musage = root["ps0"].asDouble();
		SendKwhMeter(m_HwdID, 1, 255, musage, root["cs0"].asDouble(), "S0");
	}
	return true;
}

void CYouLess::GetMeterDetails()
{
	if (m_bCheckP1 || m_bHaveP1)
	{
		if (GetP1Details())
			return;
		if (m_bHaveP1)
			return;
	}

	std::string sResult;
	std::stringstream szURL;

	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/a";
	if (!m_Password.empty())
		szURL << "&w=" << m_Password;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR,"YouLess: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}

	std::vector<std::string> results;
	StringSplit(sResult, "\n", results);
	if (results.size()<2)
	{
		_log.Log(LOG_ERROR,"YouLess: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	int fpos;
	std::string pusage=stdstring_trim(results[0]);
	fpos=pusage.find_first_of(" ");
	if (fpos!=std::string::npos)
		pusage=pusage.substr(0,fpos);
	stdreplace(pusage,",","");

	std::string pcurrent=stdstring_trim(results[1]);
	fpos=pcurrent.find_first_of(" ");
	if (fpos!=std::string::npos)
		pcurrent=pcurrent.substr(0,fpos);
	stdreplace(pcurrent,",","");

	m_meter.powerusage=atol(pusage.c_str());
	m_meter.usagecurrent=atol(pcurrent.c_str());
	sDecodeRXMessage(this, (const unsigned char *)&m_meter, NULL, 255);
}
