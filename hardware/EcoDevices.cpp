#include "EcoDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../tinyxpath/xpath_processor.h"

// http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html
// Eco- Devices is a utility consumption monitoring device dedicated to the French market.
// It provides 4 inputs, two using the "Teleinfo" protocol found on all recent French electricity meters
// and two multi-purpose impulse counters to be used for water, gaz or fuel consumption monitoring

// http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html

// Detailed information on the API can be found at
// http://www.touteladomotique.com/index.php?option=com_content&id=985:premiers-pas-avec-leco-devices-sur-la-route-de-la-maitrise-de-lenergie&Itemid=89#.WKcK0zi3ik5

// Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
// http://www.enedis.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf

// Version 2.0  21-02-2017
// Author: Blaise Thauvin, heavily based on version 1 from an anonymous contributor

// Version 2 adds support for any or all 4 counters and several electricity subscription plans whereas version 1 only
// supported the first counter for basic subscription plan
// TODO: add support for EJP and Tempo subscription plans (I would need data samples) and move out of P1SmartMeter
// dependancy in order to benefit from additional data available (Tri phases subscriptions, alerts when intensity comes
// close to subscribed limit, tarif switches to higher prices (EJP...)

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
	m_indexC1 = 0;
	m_indexC2 = 0;
	m_previousC1 = 0;
	m_previousC2 = 0;
	m_lastSendDataT1=0;
	m_lastSendDataT2=0;
	m_lastSendDataC1=0;
	m_lastSendDataC2=0;
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
	uint32_t checksum = 0, m_pappHC, m_pappHP, m_pappHCJB, m_pappHPJB, m_pappHCJW, m_pappHPJW, m_pappHCJR, m_pappHPJR;

	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "EcoDevices: Invalid data received!");
		return;
	}
	if (root["product"].empty() == true || root["product"].asString() != "Eco-devices")
	{
		_log.Log(LOG_ERROR, "EcoDevices: Invalid data received..!");
		return;
	}

	// Process Teleinfo 1 data
	m_teleinfo1.PTEC = root["T1_PTEC"].asString();
	// Check if a data source is connected to input 1
	if (m_teleinfo1.PTEC !="----")
	{
		// PAPP only exist on some meter versions. If not present, we could approximate it as (current x 230V)
		if (root["T1_PAPP"].empty())
			m_teleinfo1.PAPP = 0;
		else
			m_teleinfo1.PAPP = (unsigned long)(root["T1_PAPP"].asFloat());
		if (m_teleinfo1.PTEC.substr(0,2) == "TH")
		{
			m_teleinfo1.rate="Tarif de base";
		}
		else if (m_teleinfo1.PTEC.substr(0,2) == "HC")
		{
			m_teleinfo1.rate="Heures Creuses";
			m_pappHC=m_teleinfo1.PAPP;
			m_pappHP=0;
		}
		else if (m_teleinfo1.PTEC.substr(0,2) == "HP")
		{
			m_teleinfo1.rate="Heures Pleines";
			m_pappHC=0;
			m_pappHP=m_teleinfo1.PAPP;
		}
		else if (m_teleinfo1.PTEC.substr(0,2) == "HN")
		{
			m_teleinfo1.rate="Heures Normales";
			m_pappHC=m_teleinfo1.PAPP;
			m_pappHP=0;
		}
		else if (m_teleinfo1.PTEC.substr(0,2) == "PM")
		{
			m_teleinfo1.rate="Pointe Mobile";
			m_pappHC=0;
			m_pappHP=m_teleinfo1.PAPP;
		}
		else
		{
			m_teleinfo1.rate="Unknown";
			m_teleinfo1.tariff="Undefined";
		}
		m_teleinfo1.color="";

		//Basic subscription scheme
		if (m_teleinfo1.PTEC == "TH..")
		{
			m_teleinfo1.tariff="Tarif de Base";
			m_teleinfo1.BASE = (unsigned long)(root["T1_BASE"].asFloat());
		}

		// Peak/Off-Peak subscription scheme
		if ((m_teleinfo1.PTEC == "HC..") || (m_teleinfo1.PTEC == "HP.."))
		{
			m_teleinfo1.tariff="Tarif option Heures Creuses";
			m_teleinfo1.HCHP = (unsigned long)(root["T1_HCHP"].asFloat());
			m_teleinfo1.HCHC = (unsigned long)(root["T1_HCHC"].asFloat());
		}

		// "EJP" subscription scheme
		if ((m_teleinfo1.PTEC == "HN..") || (m_teleinfo1.PTEC == "PM.."))
		{
			m_teleinfo1.tariff="Tarif option EJP";
			m_teleinfo1.EJPHN = (unsigned long)(root["T1_EJPHN"].asFloat());
			m_teleinfo1.EJPHPM = (unsigned long)(root["T1_EJPHPM"].asFloat());
		}

		// "TEMPO" subscription scheme
		if (m_teleinfo1.PTEC.substr(2,1) == "J")
		{
			m_teleinfo1.tariff="Tarif option TEMPO";
			m_pappHCJB=0;
			m_pappHPJB=0;
			m_pappHCJW=0;
			m_pappHPJW=0;
			m_pappHCJR=0;
			m_pappHPJR=0;
			if (m_teleinfo1.PTEC.substr(3,1) == "B")
			{
				m_teleinfo1.color="Bleu";
				if (m_teleinfo1.rate == "Heures Creuses")
					m_pappHCJB=m_teleinfo1.PAPP;
				else
					m_pappHPJB=m_teleinfo1.PAPP;
			}
			else if (m_teleinfo1.PTEC.substr(3,1) == "W")
			{
				m_teleinfo1.color="Blanc";
				if (m_teleinfo1.rate == "Heures Creuses")
					m_pappHCJW=m_teleinfo1.PAPP;
				else
					m_pappHPJW=m_teleinfo1.PAPP;
			}
			else if (m_teleinfo1.PTEC.substr(3,1) == "R")
			{
				m_teleinfo1.color="Rouge";
				if (m_teleinfo1.rate == "Heures Creuses")
					m_pappHCJR=m_teleinfo1.PAPP;
				else
					m_pappHPJR=m_teleinfo1.PAPP;
			}
			else
			{
				m_teleinfo1.color="Unknown";
			}
			m_teleinfo1.BBRHCJB = (unsigned long)(root["T1_BBRHCJB"].asFloat());
			m_teleinfo1.BBRHPJB = (unsigned long)(root["T1_BBRHOJB"].asFloat());
			m_teleinfo1.BBRHCJW = (unsigned long)(root["T1_BBRHCJW"].asFloat());
			m_teleinfo1.BBRHPJW = (unsigned long)(root["T1_BBRHOJW"].asFloat());
			m_teleinfo1.BBRHCJR = (unsigned long)(root["T1_BBRHCJR"].asFloat());
			m_teleinfo1.BBRHPJR = (unsigned long)(root["T1_BBRHOJR"].asFloat());
		}
	}
	// Checksum to detect changes
	checksum=m_teleinfo1.BASE + m_teleinfo1.HCHC + m_teleinfo1.HCHP + m_teleinfo1.EJPHN + m_teleinfo1.EJPHPM + m_teleinfo1.BBRHCJB + m_teleinfo1.BBRHPJB + m_teleinfo1.BBRHCJW + m_teleinfo1.BBRHPJB + m_teleinfo1.BBRHCJR + m_teleinfo1.BBRHPJR + m_teleinfo1.PAPP;
	//Send data if value changed or at least every 5 minutes and only if non zero
	if ((checksum > 0) && ((m_teleinfo1.previous != checksum) || (difftime(atime,m_lastSendDataT1) >= 300)))
	{
		m_teleinfo1.previous = checksum;
		m_lastSendDataT1 = atime;
		if (m_teleinfo1.tariff == "Tarif de Base")
		{
			SendKwhMeter(m_HwdID, 1, 255, m_teleinfo1.PAPP, m_teleinfo1.BASE/1000.0, "Teleinfo 1 Index");
		}
		else if (m_teleinfo1.tariff == "Tarif option Heures Creuses")
		{
			SendKwhMeter(m_HwdID, 2, 255, m_pappHC, m_teleinfo1.HCHC/1000.0, "Teleinfo 1 Heures Creuses");
			SendKwhMeter(m_HwdID, 3, 255, m_pappHP, m_teleinfo1.HCHP/1000.0, "Teleinfo 1 Heures Pleines");
		}
		else if (m_teleinfo1.tariff == "Tarif option EJP")
		{
			SendKwhMeter(m_HwdID, 1, 255, m_pappHC, m_teleinfo1.EJPHN/1000.0, "Teleinfo 1 Heures Normales");
			SendKwhMeter(m_HwdID, 1, 255, m_pappHP, m_teleinfo1.EJPHPM/1000.0, "Teleinfo 1 Heures Pointe Mobile");
		}
		else if (m_teleinfo1.tariff == "Tarif option TEMPO")
		{
			SendKwhMeter(m_HwdID, 1, 255, m_pappHCJB, m_teleinfo1.BBRHCJB/1000.0, "Teleinfo 1 Jour Bleu, Creux");
			SendKwhMeter(m_HwdID, 1, 255, m_pappHPJB, m_teleinfo1.BBRHPJB/1000.0, "Teleinfo 1 Jour Bleu, Plein");
			SendKwhMeter(m_HwdID, 1, 255, m_pappHCJW, m_teleinfo1.BBRHCJW/1000.0, "Teleinfo 1 Jour Blanc, Creux");
			SendKwhMeter(m_HwdID, 1, 255, m_pappHPJW, m_teleinfo1.BBRHPJW/1000.0, "Teleinfo 1 Jour Blanc, Plein");
			SendKwhMeter(m_HwdID, 1, 255, m_pappHCJR, m_teleinfo1.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Creux");
			SendKwhMeter(m_HwdID, 1, 255, m_pappHPJR, m_teleinfo1.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Plein");
		}

	}

	// Process pulse counters
	m_indexC1 = (unsigned long)(root["INDEX_C1"].asFloat());
	if ((m_indexC1 >0) && ((m_indexC1 != m_previousC1) || (difftime(atime,m_lastSendDataC1) >= 300)))
	{
		m_previousC1 = m_indexC1;
		m_lastSendDataC1 = atime;
		SendMeterSensor(m_HwdID, 3, 255, m_indexC1/1000.0, "EcoDevices Counter 1");
	}

        m_indexC2 = (unsigned long)(root["INDEX_C2"].asFloat());
        if ((m_indexC2 >0) && ((m_indexC2 != m_previousC2) || (difftime(atime,m_lastSendDataC2) >= 300)))
        {
                m_previousC2 = m_indexC2;
                m_lastSendDataC2 = atime;
                SendMeterSensor(m_HwdID, 4, 255, m_indexC2/1000.0, "EcoDevices Counter 2");
        }

	//   To be used to display variable "DEMAIN" for "TEMPO" subscriptions
	//   Need to check if the information is available in the standard JSON outpout
	//       SendTextSensor(m_HwdID, 5, 255, "Message Texte", "Message par dfault");

	//   To be used with EJP subscriptions to fire an alert when the 30mn advance warning is triggered
	//   Need to check if the information is available in the standard JSON outpout
	//       SendAlertSensor(m_HwdID, 255, 2, "Alerte");
}
