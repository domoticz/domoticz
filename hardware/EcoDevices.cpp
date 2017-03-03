#include "stdafx.h"
#include "EcoDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>

/*
Eco- Devices is a utility consumption monitoring device dedicated to the French market.
It provides 4 inputs, two using the "Teleinfo" protocol found on all recent French electricity meters
and two multi-purpose pulse counters to be used for water, gaz or fuel consumption monitoring

http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html

Detailed information on the API can be found at
http://www.touteladomotique.com/index.php?option=com_content&id=985:premiers-pas-avec-leco-devices-sur-la-route-de-la-maitrise-de-lenergie&Itemid=89#.WKcK0zi3ik5

Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
http://www.enedis.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf

Version 3.0
Author  Blaise Thauvin

Version history

3.0   28-02-2017 Move from JSON to XML API on EcoDevices in order to retreive more Teleinfo variables (current, alerts...)
2.1   27-02-2017 Switch from sDecodeRX to standard helpers (Sendxxxxx) for updating devices.
				 Supports 4 main subscription plans (Basic, Tempo, EJP, "Heures Creuses")
				 Text device for reporting the current rate for multiple rates plans.
2.0   21-02-2017 Support for any or all 4 counters and several electricity subscription plans
1.0              Anonymous contributor initiated the EcoDevices hardware support for one counter and one subscription plan

*/

#ifdef _DEBUG
#define DEBUG_EcoDevices
#endif


// Minimum EcoDevises firmware require
#define MAJOR 1
#define MINOR 5
#define RELEASE 1

CEcoDevices::CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort)
{
	m_HwdID = ID;
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


void CEcoDevices::DecodeXML2Teleinfo(const std::string &sResult, Teleinfo &teleinfo)
{
	boost::property_tree::ptree pt;
	boost::iostreams::stream<boost::iostreams::array_source> stream(sResult.c_str(), sResult.size());
	try
	{
		read_xml(stream, pt);
	}
	catch(boost::exception const&  ex)
	{
		_log.Log(LOG_ERROR, "XML Parsing error. %s", diagnostic_information(ex).c_str());
		return;
	}
	teleinfo.OPTARIF = pt.get<std::string>("response.OPTARIF", "----");
	teleinfo.PTEC = pt.get<std::string>("response.PTEC", "----");
	teleinfo.DEMAIN = pt.get<std::string>("response.DEMAIN", "");
	teleinfo.ISOUSC = pt.get<unsigned>("response.ISOUSC", 0);
	teleinfo.PAPP = pt.get<unsigned>("response.PAPP", 0);
	teleinfo.BASE = pt.get<unsigned>("response.BASE", 0);
	teleinfo.HCHC = pt.get<unsigned>("response.HCHC", 0);
	teleinfo.HCHP = pt.get<unsigned>("response.HCHP", 0);
	teleinfo.EJPHN = pt.get<unsigned>("response.EJPHN", 0);
	teleinfo.EJPHPM = pt.get<unsigned>("response.EJPHPM", 0);
	teleinfo.BBRHCJB = pt.get<unsigned>("response.BBRHCJB", 0);
	teleinfo.BBRHPJB = pt.get<unsigned>("response.BBRHPJB", 0);
	teleinfo.BBRHCJW = pt.get<unsigned>("response.BBRHCJW", 0);
	teleinfo.BBRHPJW = pt.get<unsigned>("response.BBRHPJB", 0);
	teleinfo.BBRHCJR = pt.get<unsigned>("response.BBRHCJR", 0);
	teleinfo.BBRHPJR = pt.get<unsigned>("response.BBRHPJR", 0);
	teleinfo.PEJP = pt.get<unsigned>("response.PEJP", 0);
	teleinfo.IINST = pt.get<unsigned>("response.IINST", 0);
	teleinfo.IINST1 = pt.get<unsigned>("response.IINST1", 0);
	teleinfo.IINST2 = pt.get<unsigned>("response.IINST2", 0);
	teleinfo.IINST3 = pt.get<unsigned>("response.IINST3", 0);
        teleinfo.IMAX = pt.get<unsigned>("response.IMAX", 0);
        teleinfo.IMAX1 = pt.get<unsigned>("response.IMAX1", 0);
        teleinfo.IMAX2 = pt.get<unsigned>("response.IMAX2", 0);
        teleinfo.IMAX3 = pt.get<unsigned>("response.IMAX3", 0);
	teleinfo.ADPS = pt.get<unsigned>("response.ADPS", 0);
}


void CEcoDevices::ProcessTeleinfo(const std::string &name, int HwdID, int rank, Teleinfo &teleinfo)
{
	uint32_t m_pappHC, m_pappHP, m_pappHCJB, m_pappHPJB, m_pappHCJW, m_pappHPJW, m_pappHCJR, m_pappHPJR, checksum, level;
        double flevel;
	time_t atime = mytime(NULL);

	// PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
	if ((teleinfo.PAPP == 0) && (teleinfo.IINST > 0))
		teleinfo.PAPP = (teleinfo.IINST * 230);

	if (teleinfo.PTEC.substr(0,2) == "TH")
		teleinfo.rate="Toutes Heures";
	else if (teleinfo.PTEC.substr(0,2) == "HC")
	{
		teleinfo.rate = "Heures Creuses";
		m_pappHC = m_teleinfo1.PAPP;
		m_pappHP = 0;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HP")
	{
		teleinfo.rate = "Heures Pleines";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HN")
	{
		teleinfo.rate = "Heures Normales";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
	}
	else if (teleinfo.PTEC.substr(0,2) == "PM")
	{
		teleinfo.rate = "Pointe Mobile";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
	}
	else
	{
		teleinfo.rate = "Unknown";
		teleinfo.tariff = "Undefined";
	}

	// Checksum to detect changes between measures
	checksum=teleinfo.BASE + teleinfo.HCHC + teleinfo.HCHP + teleinfo.EJPHN + teleinfo.EJPHPM + teleinfo.BBRHCJB \
		+ teleinfo.BBRHPJB + teleinfo.BBRHCJW + teleinfo.BBRHPJB + teleinfo.BBRHCJR + teleinfo.BBRHPJR + teleinfo.PAPP;

	//Send data if value changed or at least every 5 minutes
	if  ((teleinfo.previous != checksum) || (difftime(atime, teleinfo.last) >= 300))
	{
		teleinfo.previous = checksum;
		teleinfo.last = atime;
		if (teleinfo.OPTARIF == "BASE")
		{
			teleinfo.tariff="Tarif de Base";
			SendKwhMeter(HwdID, 10, 255, teleinfo.PAPP, teleinfo.BASE/1000.0, name + " Index");
		}
		else if  (teleinfo.OPTARIF == "HC..")
		{
			teleinfo.tariff="Tarif option Heures Creuses";
			SendKwhMeter(HwdID, 12, 255, m_pappHC, teleinfo.HCHC/1000.0, name + " Heures Creuses");
			SendKwhMeter(HwdID, 13, 255, m_pappHP, teleinfo.HCHP/1000.0, name + " Heures Pleines");
			SendKwhMeter(HwdID, 14, 255, teleinfo.PAPP, (teleinfo.HCHP + teleinfo.HCHC)/1000.0, name + " Total");
			SendTextSensor(HwdID, 11, 255, teleinfo.rate, name + " Tarif en cours");
		}
		else if (teleinfo.OPTARIF == "EJP.")
		{
			teleinfo.tariff="Tarif option EJP";
			SendKwhMeter(HwdID, 5, 255, m_pappHC, teleinfo.EJPHN/1000.0, name + " Heures Normales");
			SendKwhMeter(HwdID, 16, 255, m_pappHP, teleinfo.EJPHPM/1000.0, name + " Heures Pointe Mobile");
			SendKwhMeter(HwdID, 17, 255, teleinfo.PAPP, (teleinfo.EJPHN + teleinfo.EJPHPM)/1000.0, name + " Total");
			SendTextSensor(HwdID, 11, 255, teleinfo.rate, name + " Tarif en cours");
			SendAlertSensor((HwdID*256) + rank, 255, teleinfo.PEJP/6,  (name + " Alerte Pointe Mobile").c_str());
		}
		else if (teleinfo.OPTARIF.substr(0,3) == "BBR")
		{
			teleinfo.tariff="Tarif option TEMPO";
			m_pappHCJB=0;
			m_pappHPJB=0;
			m_pappHCJW=0;
			m_pappHPJW=0;
			m_pappHCJR=0;
			m_pappHPJR=0;
			if (teleinfo.PTEC.substr(3,1) == "B")
			{
				teleinfo.color="Bleu";
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJB=teleinfo.PAPP;
				else
					m_pappHPJB=teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3,1) == "W")
			{
				teleinfo.color="Blanc";
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJW=teleinfo.PAPP;
				else
					m_pappHPJW=teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3,1) == "R")
			{
				teleinfo.color="Rouge";
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJR=teleinfo.PAPP;
				else
					m_pappHPJR=teleinfo.PAPP;
			}
			else
			{
				teleinfo.color="Unknown";
			}
			SendKwhMeter(HwdID, 18, 255, m_pappHCJB, teleinfo.BBRHCJB/1000.0, "Teleinfo 1 Jour Bleu, Creux");
			SendKwhMeter(HwdID, 19, 255, m_pappHPJB, teleinfo.BBRHPJB/1000.0, "Teleinfo 1 Jour Bleu, Plein");
			SendKwhMeter(HwdID, 20, 255, m_pappHCJW, teleinfo.BBRHCJW/1000.0, "Teleinfo 1 Jour Blanc, Creux");
			SendKwhMeter(HwdID, 21, 255, m_pappHPJW, teleinfo.BBRHPJW/1000.0, "Teleinfo 1 Jour Blanc, Plein");
			SendKwhMeter(HwdID, 22, 255, m_pappHCJR, teleinfo.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Creux");
			SendKwhMeter(HwdID, 23, 255, m_pappHPJR, teleinfo.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Plein");
			SendKwhMeter(HwdID, 24, 255, teleinfo.PAPP, (teleinfo.BBRHCJB + teleinfo.BBRHPJB + teleinfo.BBRHCJW \
				+ teleinfo.BBRHPJW + teleinfo.BBRHCJR + teleinfo.BBRHPJR)/1000.0,  name + " Total");
			SendTextSensor(HwdID, 11, 255, "Jour " +  teleinfo.color + ", " + teleinfo.rate,  name + " Tarif en cours ");
			SendTextSensor(HwdID, 25, 255, "Demain, jour " + teleinfo.DEMAIN ,  name + " couleur demain");
		}
		// Common sensors for all rates
		SendCurrentSensor(HwdID, 255, teleinfo.IINST1, teleinfo.IINST2, teleinfo.IINST3, name + " Courant");
                
                // Alert level is 0 below 100% usage, linear, 1 for 100% load and 4 for maximum load (IMAX)
                if ((teleinfo.IMAX - teleinfo.IINST ) <=0) 
                     level = 4;
                else 
                     flevel = (3.0 * teleinfo.IINST + teleinfo.IMAX - 4.0 * teleinfo.ISOUSC) / (teleinfo.IMAX - teleinfo.ISOUSC);
                     if (flevel > 4) flevel = 4;
                     if (flevel < 1) flevel = 1;
                     level = round(flevel + 0.49);
                SendAlertSensor((HwdID*256), 255, level,  (name + " Dépassement intensité maximale").c_str()); 
	}
}


void CEcoDevices::GetMeterDetails()
{
	if (m_szIPAddress.size()==0)
		return;
	// From http://xx.xx.xx.xx/status.xml  we get the pulse counters indexes and current flow
	// From http://xx.xx.xx.xx/protect/settings/teleinfoX.xml we get a complete feed of Teleinfo data

	std::vector<std::string> ExtraHeaders;
	std::string       sResult, sub, message;
	std::string::size_type len, pos;
	std::stringstream sstr;
	time_t atime = mytime(NULL);
        int   major, minor, release;
        int min_major = MAJOR, min_minor = MINOR, min_release = RELEASE;
	
        // Check EcoDevices firmware version and process pulse counters
	sstr << "http://" << m_szIPAddress << ":" << m_usIPPort << "/status.xml";
	if (m_status.hostname.empty()) m_status.hostname = m_szIPAddress;
	if (HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_NORM, "Ecodevices: Fetching counters and status data from %s", m_status.hostname.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR, "EcoDevices: Error getting status.xml from EcoDevices%s!", m_status.hostname.c_str());
		return;
	}
	// populate tree structure pt
	using boost::property_tree::ptree;
	ptree pt;
	boost::iostreams::stream<boost::iostreams::array_source> stream(sResult.c_str(), sResult.size());
	try
	{
		read_xml(stream, pt);
	}
	catch(boost::exception const&  ex)
	{
		_log.Log(LOG_ERROR, "XML Parsing error. %s", diagnostic_information(ex).c_str());
		return;
	}
	// XML format changes dramatically between firmware versions. This code was developped for version 1.05.12
	m_status.version  = pt.get<std::string>("response.version");
        major = boost::lexical_cast<int>(m_status.version.substr(0,m_status.version.find(".")).c_str());
        m_status.version.erase(0,m_status.version.find(".")+1);
        minor = boost::lexical_cast<int>(m_status.version.substr(0,m_status.version.find(".")).c_str());
        m_status.version.erase(0,m_status.version.find(".")+1);
        release = boost::lexical_cast<int>(m_status.version.c_str());
        m_status.version = pt.get<std::string>("response.version");

	if ((major>min_major) || ((major==min_major) && (minor>min_minor)) || ((major==min_major) && (minor==min_minor) && (release>=min_release)))
	{
		m_status.hostname = pt.get<std::string>("response.config_hostname");
		m_status.flow1    = pt.get<unsigned>("response.meter2");
		m_status.flow2    = pt.get<unsigned>("response.meter3");
		m_status.index1   = pt.get<unsigned>("response.count0");
		m_status.index2   = pt.get<unsigned>("response.count1");
		m_status.t1_ptec  = pt.get<std::string>("response.T1_PTEC");
		m_status.t2_ptec  = pt.get<std::string>("response.T2_PTEC");

		// Process Counter 1
		if ((m_status.index1 >0) && ((m_status.index1 != m_status.pindex1)  || (m_status.flow1 != m_status.pflow1) \
			|| (difftime(atime,m_status.time1) >= 300)))
		{
			m_status.pindex1 = m_status.index1;
			m_status.pflow1 = m_status.flow1;
			m_status.time1 = atime;
			SendMeterSensor(m_HwdID, 1, 255, m_status.index1/1000.0, m_status.hostname + " Counter 1");
			SendWaterflowSensor(m_HwdID, 2, 255, m_status.flow1, m_status.hostname + " Flow counter 1");
		}

		// Process Counter 2
		if ((m_status.index2 >0) && ((m_status.index2 != m_status.pindex2)  || (m_status.flow2 != m_status.pflow2) \
			|| (difftime(atime,m_status.time2) >= 300)))
		{
			m_status.pindex2 = m_status.index2;
			m_status.pflow2 = m_status.flow2;
			m_status.time2 = atime;
			SendMeterSensor(m_HwdID, 3, 255, m_status.index2/1000.0, m_status.hostname + " Counter 2");
			SendWaterflowSensor(m_HwdID, 4, 255, m_status.flow2, m_status.hostname + " Flow counter 2");
		}
	}
	else
	{
                message = "EcoDevices firmware needs to be at least version MAJOR.MINOR.RELEASE, current vesion is " +  m_status.version;
		_log.Log(LOG_ERROR, message.c_str());
		return;
	}

	// Query Teleinfo counters only if an active subscrition is detected (PTEC != "----")

	//  Get Teleinfo 1
	if (m_status.t1_ptec != "----")
	{
		sstr.str("");
		sstr << "http://" << m_szIPAddress << ":" << m_usIPPort << "/protect/settings/teleinfo1.xml";
		_log.Log(LOG_NORM, "Ecodevices: Fetching Teleinfo 1 data from %s", m_status.hostname.c_str());
		if (!HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "EcoDevices: Error getting teleinfo1.xml from EcoDevices %s!", m_status.hostname.c_str());
			return;
		}
		#ifdef DEBUG_EcoDevices
		_log.Log(LOG_NORM, "Data: %s", sResult.c_str());
		#endif

		// Remove all "T1_"s from output as it prevents writing generic code for both counters
		sub = "T1_";
		len = sub.length();
		for (pos = sResult.find(sub);pos != std::string::npos;pos = sResult.find(sub))
			sResult.erase(pos,len);

		DecodeXML2Teleinfo(sResult, m_teleinfo1);
		ProcessTeleinfo("Teleinfo 1", m_HwdID, 1, m_teleinfo1);
	}
	// Get Teleinfo 2
	if (m_status.t2_ptec != "----")
	{
		sstr.str("");
		sstr << "http://" << m_szIPAddress << ":" << m_usIPPort << "/protect/settings/teleinfo2.xml";
		_log.Log(LOG_NORM, "Ecodevices: Fetching Teleinfo 2 data from %s", m_status.hostname.c_str());
		if (!HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "EcoDevices: Error getting teleinfo2.xml from EcoDevices %s!", m_status.hostname.c_str());
			return;
		}
		#ifdef DEBUG_EcoDevices
		_log.Log(LOG_NORM, "Data: %s", sResult.c_str());
		#endif

		// Remove all "T2_"s from output as it prevents writing generic code for both counters
		sub = "T2_";
		len = sub.length();
		for (pos = sResult.find(sub);pos != std::string::npos;pos = sResult.find(sub))
			sResult.erase(pos,len);

		DecodeXML2Teleinfo(sResult, m_teleinfo2);
		ProcessTeleinfo("Teleinfo 2", m_HwdID, 2, m_teleinfo2);
	}

}
