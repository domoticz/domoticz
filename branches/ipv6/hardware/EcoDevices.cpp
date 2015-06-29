#include "stdafx.h"
#include "EcoDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"
#include "../json/json.h"

//http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html
//Eco- Devices, the first IP module dedicated to monitoring your consumption ...
//French

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
	//#define DEBUG_EcoDevices
#endif

CEcoDevices::CEcoDevices(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_szIPAddress = IPAddress;
	m_usIPPort = usIPPort;
	m_stoprequested = false;

	memset(&m_p1power, 0, sizeof(m_p1power));

	m_p1power.len = sizeof(P1Power)-1;
	m_p1power.type = pTypeP1Power;
	m_p1power.subtype = sTypeP1Power;
	m_p1power.ID = 1;
	Init();
}

CEcoDevices::~CEcoDevices(void)
{
}

void CEcoDevices::Init()
{
	m_stoprequested = false;
	m_lastSharedSendElectra = 0;
	m_lastelectrausage = 0;
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
		if (sec_counter % 12 == 0) {
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
	if (!jReader.parse(sResult, root))
	{
		_log.Log(LOG_ERROR, "EcoDevices: Invalid data received!");
		return;
	}
	if (root["product"].empty() == true)
	{
		_log.Log(LOG_ERROR, "EcoDevices: Invalid data received..!");
		return;
	}

	if (root["T1_PAPP"].empty() == false)
	{
		std::string tPAPP = root["T1_PAPP"].asString();
		if (tPAPP != "----")
		{
			m_p1power.powerusage1 = (unsigned long)(root["T1_HCHP"].asFloat());
			m_p1power.powerusage2 = (unsigned long)(root["T1_HCHC"].asFloat());
			m_p1power.usagecurrent = (unsigned long)(root["T1_PAPP"].asFloat());	//Watt
		}
	}

	//Send Electra if value changed, or at least every 5 minutes
	if (
		(m_p1power.usagecurrent != m_lastelectrausage) ||
		(atime - m_lastSharedSendElectra >= 300)
		)
	{
		if ((m_p1power.powerusage1 != 0) || (m_p1power.powerusage2 != 0) || (m_p1power.powerdeliv1 != 0) || (m_p1power.powerdeliv2 != 0))
		{
			m_lastSharedSendElectra = atime;
			m_lastelectrausage = m_p1power.usagecurrent;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power);
		}
	}
}
