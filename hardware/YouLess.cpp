#include "stdafx.h"
#include "YouLess.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define YOULESS_POLL_INTERVAL 10

CYouLess::CYouLess(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password) :
m_szIPAddress(IPAddress),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
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
}

bool CYouLess::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CYouLess::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CYouLess::StopHardware()
{
	/*
    m_stoprequested=true;
	if (m_thread)
		m_thread->join();
	return true;
    */
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CYouLess::Do_Work()
{
	int sec_counter = YOULESS_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
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

void CYouLess::GetMeterDetails()
{
	std::string sResult;

	char szURL[200];

	if(m_Password.size() == 0)
	{
		sprintf(szURL,"http://%s:%d/a",m_szIPAddress.c_str(), m_usIPPort);
	}
	else
	{
		sprintf(szURL,"http://%s:%d/a&w=%s",m_szIPAddress.c_str(), m_usIPPort, m_Password.c_str());
	}

	if (!HTTPClient::GET(szURL,sResult))
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
