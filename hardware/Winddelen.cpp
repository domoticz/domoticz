#include "stdafx.h"
#include "Winddelen.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define WINDDELEN_POLL_INTERVAL 10

CWinddelen::CWinddelen(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const unsigned short usMillID) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_usMillID=usMillID;
	m_stoprequested=false;
	Init();
}

CWinddelen::~CWinddelen(void)
{
}

void CWinddelen::Init()
{
}

bool CWinddelen::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CWinddelen::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CWinddelen::StopHardware()
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

void CWinddelen::Do_Work()
{
	int sec_counter = WINDDELEN_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (sec_counter % WINDDELEN_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"Winddelen: Worker stopped...");
}

bool CWinddelen::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void CWinddelen::GetMeterDetails()
{
	std::string sResult;

	char szURL[200];

	sprintf(szURL, "http://backend.windcentrale.nl/windcentrale/productie?id=%d", m_usMillID);

	if (!HTTPClient::GETSingleLine(szURL,sResult))
	{
		_log.Log(LOG_ERROR,"Winddelen: Error connecting to: %s", szURL);
		return;
	}

	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size()<7)
	{
		_log.Log(LOG_ERROR,"Winddelen: Invalid response for '%s'", m_szIPAddress.c_str());
		return;
	}

	int fpos;
	std::string pusage=stdstring_trim(results[7]);
	fpos=pusage.find_first_of(" ");
	if (fpos!=std::string::npos)
		pusage=pusage.substr(0,fpos);
	stdreplace(pusage,",","");

	std::string pcurrent=stdstring_trim(results[2]);
	fpos=pcurrent.find_first_of(" ");
	if (fpos!=std::string::npos)
		pcurrent=pcurrent.substr(0,fpos);
	stdreplace(pcurrent,",","");
	
	std::map<int,float> winddelen_per_mill;
  	winddelen_per_mill[1]=9910.0;
  	winddelen_per_mill[2]=10154.0;
 	winddelen_per_mill[31]=6648.0;
  	winddelen_per_mill[41]=6164.0;
    winddelen_per_mill[51]=5721.0;
	winddelen_per_mill[111]=5579.0;
  	winddelen_per_mill[121]=5602.0;
  	winddelen_per_mill[131]=5534.0;
  	winddelen_per_mill[141]=5512.0;
  	winddelen_per_mill[191]=3000.0;

	double powerusage = atol(pusage.c_str()) * m_usIPPort / winddelen_per_mill[m_usMillID];
	double usagecurrent = atof(pcurrent.c_str()) * m_usIPPort / 1000.0;
#ifdef _DEBUG
	_log.Log(LOG_STATUS,"%d winddelen in '%s' currently produces: %.0f Watt, total production is: %.03f kWh", m_usIPPort , m_szIPAddress.c_str(), usagecurrent * 1000, powerusage);
#endif
	SendKwhMeterOldWay(m_usMillID, 1, 255, usagecurrent, powerusage, "Wind Power");
}
