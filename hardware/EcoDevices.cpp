#include "stdafx.h"
#include "EcoDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

// http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html
// Eco- Devices is a utility consumption monitoring device dedicated to the French market.
// It provides 4 inputs, two using the "Teleinfo" protocol found on all recent French electricity meters
// and two multi-purpose impulse counters to be used for water, gaz or fuel consumption monitoring

// http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html

// Detailed information on the API can be found at
// http://www.touteladomotique.com/index.php?option=com_content&id=985:premiers-pas-avec-leco-devices-sur-la-route-de-la-maitrise-de-lenergie&Itemid=89#.WKcK0zi3ik5


// Version 2.0  2017-02-21
// Author: Blaise Thauvin, heavily based on version 1 from an anonymous contributor

// Version 2 adds support for any or all 4 counters and several electricity subscription plans whereas version 1 only 
// supported the first counter for basic subscription plan
// TODO: add support for EJP and Tempo subscription plans (I would need data samples) and move out of P1SmartMeter
// dependancy in order to benefit from additional data available (Tri phases subscriptions, alerts when intensity comes 
// close to subscribed limit, tarif switches to higher prices (EJP...)

 
#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
//#define DEBUG_EcoDevices
#endif

CEcoDevices::CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_szIPAddress = IPAddress;
	m_usIPPort = usIPPort;
	m_stoprequested = false;

	memset(&m_p1power1, 0, sizeof(m_p1power1));
	m_p1power1.len = sizeof(P1Power)-1;
	m_p1power1.type = pTypeP1Power;
	m_p1power1.subtype = sTypeP1Power;
	m_p1power1.ID = 1;

	memset(&m_p1power2, 0, sizeof(m_p1power2));
	m_p1power2.len = sizeof(P1Power)-1;
	m_p1power2.type = pTypeP1Power;
	m_p1power2.subtype = sTypeP1Power;
	m_p1power2.ID = 2;

	memset(&m_p1water, 0, sizeof(m_p1water));
	m_p1water.len = sizeof(P1Gas)-1;
	m_p1water.type = pTypeP1Gas;
	m_p1water.subtype = sTypeP1Gas;
	m_p1water.ID = 3;

	memset(&m_p1gas, 0, sizeof(m_p1gas));
	m_p1gas.len = sizeof(P1Gas)-1;
	m_p1gas.type = pTypeP1Gas;
	m_p1gas.subtype = sTypeP1Gas;
	m_p1gas.ID = 4;

	Init();
}


CEcoDevices::~CEcoDevices(void)
{
}


void CEcoDevices::Init()
{
	m_stoprequested = false;
	m_lastSendData = 0;
	m_lastusage1 = 0;
	m_lastusage2 = 0;
	m_lastwaterusage = 0;
	m_lastgasusage = 0;
}


bool CEcoDevices::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEcoDevices::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}


bool CEcoDevices::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted=false;
	return true;
}


#define ECODEVICES_POLL_INTERVAL 30

void CEcoDevices::Do_Work()
{
	int sec_counter = 0;
	_log.Log(LOG_STATUS,"EcoDevices: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			mytime(&m_LastHeartbeat);
		}
		if (sec_counter%ECODEVICES_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"EcoDevices: Worker stopped...");
}


bool CEcoDevices::WriteToHardware(const char *pdata, const unsigned char length)
{
	return true;
}


void CEcoDevices::GetMeterDetails()
{
	if (m_szIPAddress.size()==0)
		return;

	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	std::stringstream sstr2;
	sstr2 << "http://" << m_szIPAddress
		<< ":" << m_usIPPort
		<< "/api/xdevices.json?cmd=10";
	//Get Data
	std::string sURL = sstr2.str();
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "EcoDevices: Error getting current state!");
		return;
	}

	time_t atime = mytime(NULL);

	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "EcoDevices: Invalid data received!");
		return;
	}
	if (root["product"].empty() == true)
	{
		_log.Log(LOG_ERROR, "EcoDevices: Invalid data received..!");
		return;
	}

	std::string tPTEC1 = root["T1_PTEC"].asString();
	if (tPTEC1 !="----")		 // Check if a data source is connected to input 1
	{
		if (tPTEC1 == "TH..")
		{
			m_p1power1.powerusage1 = (unsigned long)(root["T1_BASE"].asFloat());
			m_p1power1.powerusage2 = 0;
			m_p1power1.usagecurrent = (unsigned long)(root["T1_PAPP"].asFloat());
		}
		if ((tPTEC1 == "HC..") || (tPTEC1 == "HP.."))
		{
			m_p1power1.powerusage1 = (unsigned long)(root["T1_HCHP"].asFloat());
			m_p1power1.powerusage2 = (unsigned long)(root["T1_HCHC"].asFloat());
			m_p1power1.usagecurrent = (unsigned long)(root["T1_PAPP"].asFloat());
		}
	}

	std::string tPTEC2 = root["T2_PTEC"].asString();
	if (tPTEC2 !="----")		 //  Check if a data source is connected to input 2
	{
		if (tPTEC2 == "TH..")
		{
			m_p1power2.powerusage1 = (unsigned long)(root["T2_BASE"].asFloat());
			m_p1power2.powerusage2 = 0;
			m_p1power2.usagecurrent = (unsigned long)(root["T2_PAPP"].asFloat());
		}
		if ((tPTEC2 == "HC..") || (tPTEC2 == "HP.."))
		{
			m_p1power2.powerusage1 = (unsigned long)(root["T2_HCHP"].asFloat());
			m_p1power2.powerusage2 = (unsigned long)(root["T2_HCHC"].asFloat());
			m_p1power2.usagecurrent = (unsigned long)(root["T2_PAPP"].asFloat());
		}
	}

	if (root["INDEX_C1"].empty() == false)
		m_p1water.gasusage = (unsigned long)(root["INDEX_C1"].asFloat());
	if (root["INDEX_C2"].empty() == false)
		m_p1gas.gasusage = (unsigned long)(root["INDEX_C2"].asFloat());

	//Send data if value changed, or at least every 5 minutes
	if (
		(m_p1power1.usagecurrent != m_lastusage1) ||
		(m_p1power2.usagecurrent != m_lastusage2) ||
		(m_p1water.gasusage != m_lastwaterusage) ||
		(m_p1gas.gasusage != m_lastgasusage) ||
		(difftime(atime,m_lastSendData) >= 300)
		)
	{
		m_lastSendData = atime;
		m_lastusage1 = m_p1power1.usagecurrent;
		m_lastusage2 = m_p1power2.usagecurrent;
		m_lastwaterusage = m_p1water.gasusage;
		m_lastgasusage = m_p1gas.gasusage;

		// Send data only for counters effectively in use. Avoids creating useless devices in Domoticz
		if (m_p1power1.powerusage1 != 0)
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power1, "Power", 255);
		if (m_p1power1.powerusage2 != 0)
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power2, "Power", 255);
		if (m_p1water.gasusage !=0)
			sDecodeRXMessage(this, (const unsigned char *)&m_p1water,  "Gas", 255);
		if (m_p1gas.gasusage != 0)
			sDecodeRXMessage(this, (const unsigned char *)&m_p1gas,    "Gas", 255);
	}
}
