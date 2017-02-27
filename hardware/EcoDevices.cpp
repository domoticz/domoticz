#include "stdafx.h"
#include "EcoDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

/*
Eco- Devices is a utility consumption monitoring device dedicated to the French market.
It provides 4 inputs, two using the "Teleinfo" protocol found on all recent French electricity meters
and two multi-purpose pulse counters to be used for water, gaz or fuel consumption monitoring

http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html

Detailed information on the API can be found at
http://www.touteladomotique.com/index.php?option=com_content&id=985:premiers-pas-avec-leco-devices-sur-la-route-de-la-maitrise-de-lenergie&Itemid=89#.WKcK0zi3ik5

Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
http://www.enedis.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf

Version 2.2
Author  Blaise Thauvin

Version history

2.2   28-02-2017 Move from JSON to XML API on EcoDevices in order to retreive more Teleinfo variables (current, alerts...)
2.1   27-02-2017 Switch from sDecodeRX to standard helpers (Sendxxxxx) for updating devices.
				 Supports 4 main subscription plans (Basic, Tempo, EJP, "Heures Creuses")
				 Text device for reporting the current rate for multiple rates plans.
2.0   21-02-2017 Support for any or all 4 counters and several electricity subscription plans
1.0              Anonymous contributor initiated the EcoDevices hardware support for one counter and one subscription plan

*/

#ifdef _DEBUG
//#define DEBUG_EcoDevices
#endif

CEcoDevices::CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_szIPAddress = IPAddress;
	m_usIPPort = usIPPort;
	m_stoprequested = false;

	Init();
}


CEcoDevices::~CEcoDevices(void)
{
}


void CEcoDevices::Init()
{
	m_stoprequested = false;
	m_lastSendDataT1=0;
	m_lastSendDataT2=0;
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
	// From http://xx.xx.xx.xx/status.xml  we get the pulse counters indexes and current flow
	// From http://xx.xx.xx.xx/protect/settings/teleinfoX.xml we get a complete feed of Teleinfo data

	std::vector<std::string> ExtraHeaders;
	std::string       sResult;
	std::stringstream sstr2;

	time_t atime = mytime(NULL);

	// Process pulse counters
	sstr2 << "http://" << m_szIPAddress
		<< ":" << m_usIPPort
		<< "/status.xml";
	std::string sURL = sstr2.str();
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "EcoDevices: Error getting status.xml from EcoDevices!");
		return;
	}
    // populate tree structure pt
    using boost::property_tree::ptree;
    ptree pt;
    read_xml(sResult, pt);

	if ((m_status.count0 >0) && ((m_status.count0 != m_status.pcount0) || (difftime(atime,m_status.lastsend0) >= 300)))
	{
		m_status.pcount0 = m_status.count0;
                m_status.pmeter2 = m_status.meter2;
		m_status.lastsend0 = atime;
		SendMeterSensor(m_HwdID, 1, 255, m_status.count0/1000.0, m_status.hostname + " Counter 1");
	}

}
