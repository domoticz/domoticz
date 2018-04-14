/*
Eco- Devices is a utility consumption monitoring device dedicated to the French market.
It provides 4 inputs, two using the "Teleinfo" protocol found on all recent French electricity meters
and two multi-purpose pulse counters to be used for water, gaz or fuel consumption monitoring

http://gce-electronics.com/en/nos-produits/409-module-teleinfo-eco-devices.html

Detailed information on the API can be found at
http://www.touteladomotique.com/index.php?option=com_content&id=985:premiers-pas-avec-leco-devices-sur-la-route-de-la-maitrise-de-lenergie&Itemid=89#.WKcK0zi3ik5

Version 3.2
Author Blaise Thauvin

Version history

3.2   12-04-2017 Added support for authentication when connecting to EcoDevices
3.1   01-04-2017 Added basic support for recently launched EcoDevices RT2
3.0   15-03-2017 Merge Teleinfo protocol processing with other hardware using common class CTeleinfoBase
2.2   05-03-2017 Move from JSON to XML API on EcoDevices in order to retreive more Teleinfo variables (current, alerts...)
2.1   27-02-2017 Switch from sDecodeRX to standard helpers (Sendxxxxx) for updating devices.
				 Supports 4 main subscription plans (Basic, Tempo, EJP, "Heures Creuses")
				 Text device for reporting the current rate for multiple rates plans.
2.0   21-02-2017 Support for any or all 4 counters and several electricity subscription plans
1.0              Anonymous contributor initiated the EcoDevices hardware support for one counter and one subscription plan

*/

#include "stdafx.h"
#include "EcoDevices.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include <../tinyxpath/xpath_static.h>
#include "../webserver/Base64.h"
#include "../json/json.h"
#include <sstream>

#ifdef _DEBUG
#define DEBUG_EcoDevices
#endif

// Minimum EcoDevises firmware required
#define MAJOR 1
#define MINOR 5
#define RELEASE 12

//Minimum EcoDevices RT2 firmware required
#define MAJOR_RT2 2
#define MINOR_RT2 0
#define RELEASE_RT2 29

CEcoDevices::CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, 
	const std::string &username, const std::string &password, const int datatimeout, const int model, const int ratelimit):
m_szIPAddress(IPAddress),
m_username(username),
m_password(password)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_stoprequested = false;
	m_iModel = model;
        
        // Updates must be at least every 5mn in order to keep consistent historical data
	m_iDataTimeout = (datatimeout >= 300 || datatimeout == 0) ? 300 : datatimeout;
	
 	// system seems unstable if going too fast
	m_iRateLimit = (ratelimit < 2) ? 2 : ratelimit;

        // RateLimit > DataTimeout is an inconsistent setting. In that case, decrease RateLimit (which increases update rate) 
	// down to Timeout in order to avoir watchdog errors due to this user configuration mistake.
        if (m_iRateLimit > m_iDataTimeout)   m_iRateLimit = m_iDataTimeout;

	Init();
}


CEcoDevices::~CEcoDevices(void)
{
}


void CEcoDevices::Init()
{
	m_stoprequested = false;
	m_bFirstRun = true;

	// Is the device we poll password protected?
	m_ssURL.str("");
	if ((m_username.size() > 0) && (m_password.size() > 0))
        	m_ssURL << "http://" << m_username << ":" << m_password << "@";
        else
                m_ssURL <<"http://";

        m_ssURL << m_szIPAddress << ":" << m_usIPPort;

}

bool CEcoDevices::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEcoDevices::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

bool CEcoDevices::StopHardware()
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

void CEcoDevices::Do_Work()
{
	int sec_counter = m_iRateLimit - 2; // Make sure we update once soon after restart
	_log.Log(LOG_STATUS, "(%s): Worker started...", Name.c_str());
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		mytime(&m_LastHeartbeat);
		if (sec_counter >= m_iRateLimit)
		{
			sec_counter = 0;
			if (m_iModel == 0)
				GetMeterDetails();
			else
				GetMeterRT2Details();
		}
	}
	_log.Log(LOG_STATUS, "(%s): Worker stoped...", Name.c_str());
}

bool CEcoDevices::WriteToHardware(const char *pdata, const unsigned char length)
{
	return true;
}

void CEcoDevices::DecodeXML2Teleinfo(const std::string &sResult, Teleinfo &teleinfo)
{
	TiXmlDocument XMLdoc("Teleinfo.xml");

	XMLdoc.Parse(sResult.c_str(), 0, TIXML_ENCODING_UTF8);
	if (XMLdoc.Error())
	{
		_log.Log(LOG_ERROR, "Error parsing XML for /protect/settings/teleinfo?.xml: %s", XMLdoc.ErrorDesc());
		return;
	}

	using namespace TinyXPath;

	teleinfo.OPTARIF = S_xpath_string(XMLdoc.RootElement(), "/response/OPTARIF/text()").c_str();
	teleinfo.PTEC = S_xpath_string(XMLdoc.RootElement(), "/response/PTEC/text()").c_str();
	teleinfo.DEMAIN = S_xpath_string(XMLdoc.RootElement(), "/response/DEMAIN/text()").c_str();
	teleinfo.ISOUSC = i_xpath_int(XMLdoc.RootElement(), "/response/ISOUSC/text()");
	teleinfo.PAPP = i_xpath_int(XMLdoc.RootElement(), "/response/PAPP/text()");
	teleinfo.BASE = i_xpath_int(XMLdoc.RootElement(), "/response/BASE/text()");
	teleinfo.HCHC = i_xpath_int(XMLdoc.RootElement(), "/response/HCHC/text()");
	teleinfo.HCHP = i_xpath_int(XMLdoc.RootElement(), "/response/HCHP/text()");
	teleinfo.EJPHN = i_xpath_int(XMLdoc.RootElement(), "/response/EJPHN/text()");
	teleinfo.EJPHPM = i_xpath_int(XMLdoc.RootElement(), "/response/EJPHPM/text()");
	teleinfo.BBRHCJB = i_xpath_int(XMLdoc.RootElement(), "/response/BBRHCJB/text()");
	teleinfo.BBRHPJB = i_xpath_int(XMLdoc.RootElement(), "/response/BBRHPJB/text()");
	teleinfo.BBRHCJW = i_xpath_int(XMLdoc.RootElement(), "/response/BBRHCJW/text()");
	teleinfo.BBRHPJW = i_xpath_int(XMLdoc.RootElement(), "/response/BBRHPJB/text()");
	teleinfo.BBRHCJR = i_xpath_int(XMLdoc.RootElement(), "/response/BBRHCJR/text()");
	teleinfo.BBRHPJR = i_xpath_int(XMLdoc.RootElement(), "/response/BBRHPJR/text()");
	teleinfo.PEJP = i_xpath_int(XMLdoc.RootElement(), "/response/PEJP/text()");
	teleinfo.IINST = i_xpath_int(XMLdoc.RootElement(), "/response/IINST/text()");
	teleinfo.IINST1 = i_xpath_int(XMLdoc.RootElement(), "/response/IINST1/text()");
	teleinfo.IINST2 = i_xpath_int(XMLdoc.RootElement(), "/response/IINST2/text()");
	teleinfo.IINST3 = i_xpath_int(XMLdoc.RootElement(), "/response/IINST3/text()");
	teleinfo.PPOT = i_xpath_int(XMLdoc.RootElement(), "/response/PPOT/text()");
	teleinfo.ADPS = i_xpath_int(XMLdoc.RootElement(), "/response/ADPS/text()");

#ifdef DEBUG_EcoDevices
	_log.Log(LOG_NORM, "DEBUG: OPTARIF: '%s'", teleinfo.OPTARIF.c_str());
	_log.Log(LOG_NORM, "DEBUG: PTEC:    '%s'", teleinfo.PTEC.c_str());
	_log.Log(LOG_NORM, "DEBUG: DEMAIN:  '%s'", teleinfo.DEMAIN.c_str());
#endif
}

void CEcoDevices::GetMeterDetails()
{
	if (m_szIPAddress.size() == 0)
		return;
	// From http://xx.xx.xx.xx/status.xml we get the pulse counters indexes and current flow
	// From http://xx.xx.xx.xx/protect/settings/teleinfoX.xml we get a complete feed of Teleinfo data

	std::vector<std::string> ExtraHeaders;
	std::string       sResult, sub, message;
	std::string::size_type len, pos;
	std::stringstream sstr;
	TiXmlDocument XMLdoc("Teleinfo.xml");
	time_t atime = mytime(NULL);
	int   major, minor, release;
	int min_major = MAJOR, min_minor = MINOR, min_release = RELEASE;

	// Check EcoDevices firmware version and process pulse counters
	sstr << m_ssURL.str() << "/status.xml";
	
	if (m_status.hostname.empty()) m_status.hostname = m_szIPAddress;
	if (HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_NORM, "(%s) Fetching counters and status data", Name.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR, "(%s) Error getting data from: %s", Name.c_str(), sstr.str().c_str());
		return;
	}

	// Process XML result
	XMLdoc.Parse(sResult.c_str(), 0, TIXML_ENCODING_UTF8);
	if (XMLdoc.Error())
	{
		_log.Log(LOG_ERROR, "(%s) Error parsing XML at /status.xml: %s", Name.c_str(), XMLdoc.ErrorDesc());
		return;
	}

	// XML format changes dramatically between firmware versions. This code was developped for version 1.05.12
	using namespace TinyXPath;
	m_status.version = S_xpath_string(XMLdoc.RootElement(), "/response/version/text()").c_str();

#ifdef DEBUG_EcoDevices
	_log.Log(LOG_NORM, "DEBUG: XML output for /status.xml\n%s", MakeHtml(sResult).c_str());
#endif

	m_status.version = m_status.version + "..";
	major = atoi(m_status.version.substr(0, m_status.version.find(".")).c_str());
	m_status.version.erase(0, m_status.version.find(".") + 1);
	minor = atoi(m_status.version.substr(0, m_status.version.find(".")).c_str());
	m_status.version.erase(0, m_status.version.find(".") + 1);
	release = atoi(m_status.version.substr(0, m_status.version.find(".")).c_str());
	m_status.version = S_xpath_string(XMLdoc.RootElement(), "/response/version/text()").c_str();

	if ((major > min_major) || ((major == min_major) && (minor > min_minor)) || ((major == min_major) && (minor == min_minor) && (release >= min_release)))
	{
		m_status.hostname = S_xpath_string(XMLdoc.RootElement(), "/response/config_hostname/text()").c_str();
		m_status.flow1 = i_xpath_int(XMLdoc.RootElement(), "/response/meter2/text()");
		m_status.flow2 = i_xpath_int(XMLdoc.RootElement(), "/response/meter3/text()");
		m_status.index1 = i_xpath_int(XMLdoc.RootElement(), "/response/count0/text()");
		m_status.index2 = i_xpath_int(XMLdoc.RootElement(), "/response/count1/text()");
		m_status.t1_ptec = S_xpath_string(XMLdoc.RootElement(), "/response/T1_PTEC/text()").c_str();
		m_status.t2_ptec = S_xpath_string(XMLdoc.RootElement(), "/response/T2_PTEC/text()").c_str();

		// Update hardware name if this is first run
		if (m_bFirstRun)
		{
			m_sql.safe_query("UPDATE Hardware SET Name = '%s' WHERE ID = %i", m_status.hostname.c_str(), m_HwdID);
			m_bFirstRun = false;
		}
		// Process Counter 1
		if ((m_status.index1 > 0) && ((m_status.index1 != m_status.pindex1) || (m_status.flow1 != m_status.pflow1) \
			|| (difftime(atime, m_status.time1) >= m_iDataTimeout - 10)))
		{
			m_status.pindex1 = m_status.index1;
			m_status.pflow1 = m_status.flow1;
			m_status.time1 = atime;
			SendMeterSensor(m_HwdID, 1, 255, m_status.index1 / 1000.0f, m_status.hostname + " Counter 1");
			SendWaterflowSensor(m_HwdID, 2, 255, (float)m_status.flow1, m_status.hostname + " Flow counter 1");
		}

		// Process Counter 2
		if ((m_status.index2 > 0) && ((m_status.index2 != m_status.pindex2) || (m_status.flow2 != m_status.pflow2) \
			|| (difftime(atime, m_status.time2) >= m_iDataTimeout - 10)))
		{
			m_status.pindex2 = m_status.index2;
			m_status.pflow2 = m_status.flow2;
			m_status.time2 = atime;
			SendMeterSensor(m_HwdID, 3, 255, m_status.index2 / 1000.0f, m_status.hostname + " Counter 2");
			SendWaterflowSensor(m_HwdID, 4, 255, (float)m_status.flow2, m_status.hostname + " Flow counter 2");
		}
	}
	else
	{
		message = "EcoDevices firmware needs to be at least version ";
		message = message + static_cast<std::ostringstream*>(&(std::ostringstream() << min_major << "." << min_minor << "." << min_release))->str();
		message = message + ", current version is " + m_status.version;
		_log.Log(LOG_ERROR, "(%s) %s", Name.c_str(), message.c_str());
		return;
	}

	// Query Teleinfo counters only if an active subscrition is detected (PTEC != "----")

	// Get Teleinfo 1
	if (strcmp(m_status.t1_ptec.c_str(), "----") != 0)
	{
		sstr.str("");
		sstr << m_ssURL.str() << "/protect/settings/teleinfo1.xml";

		_log.Log(LOG_NORM, "(%s) Fetching Teleinfo 1 data", Name.c_str());
		if (!HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "(%s) Error getting teleinfo1.xml from EcoDevices!", Name.c_str());
			return;
		}
#ifdef DEBUG_EcoDevices
		_log.Log(LOG_NORM, "DEBUG: XML output for Teleinfo1:\n%s", MakeHtml(sResult).c_str());
#endif

		// Remove all "T1_"s from output as it prevents writing generic code for both counters
		sub = "T1_";
		len = sub.length();
		for (pos = sResult.find(sub); pos != std::string::npos; pos = sResult.find(sub))
			sResult.erase(pos, len);

		DecodeXML2Teleinfo(sResult, m_teleinfo1);
		ProcessTeleinfo("Teleinfo 1", 1, m_teleinfo1);
	}
	// Get Teleinfo 2
	if (strcmp(m_status.t2_ptec.c_str(), "----") != 0)
	{
		sstr.str("");
		sstr << m_ssURL.str() << "/protect/settings/teleinfo2.xml";
		
		_log.Log(LOG_NORM, "(%s) Fetching Teleinfo 2 data", Name.c_str());
		if (!HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
		{
			_log.Log(LOG_ERROR, "(%s) Error getting teleinfo2.xml!", Name.c_str());
			return;
		}
#ifdef DEBUG_EcoDevices
		_log.Log(LOG_NORM, "DEBUG: XML output for Teleinfo2:\n%s", MakeHtml(sResult).c_str());
#endif

		// Remove all "T2_"s from output as it prevents writing generic code for both counters
		sub = "T2_";
		len = sub.length();
		for (pos = sResult.find(sub); pos != std::string::npos; pos = sResult.find(sub))
			sResult.erase(pos, len);

		DecodeXML2Teleinfo(sResult, m_teleinfo2);
		ProcessTeleinfo("Teleinfo 2", 2, m_teleinfo2);
	}

}

void CEcoDevices::GetMeterRT2Details()
{
	if (m_szIPAddress.size() == 0)
		return;

	// From http://xx.xx.xx.xx/admin/status.xml we get the Teleinfo data

	std::vector<std::string> ExtraHeaders, splitresults;
	char XMLLabel[200];
	std::string  sResult, message, label, value;
	float fvalue1, fvalue2;
	std::stringstream sstr;
	std::map<std::string, std::string> XMLmap;
	TiXmlDocument XMLdoc("Teleinfo.xml");
	time_t atime = mytime(NULL);
	int   i, major, minor, release;
	int min_major = MAJOR_RT2, min_minor = MINOR_RT2, min_release = RELEASE_RT2;

	// Check EcoDevices firmware version and hostname from JSON API
	sstr << m_ssURL.str() << "/admin/system.json";

	//Get Data
	std::string sURL = sstr.str();
	if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "(%s) Error getting system information from: %s", Name.c_str(), sstr.str().c_str());
		return;
	}

	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "(%s) Invalid JSON data received from /admin/system.json", Name.c_str());
		return;
	}
	if (root["confighostname"].empty() == true)
	{
		_log.Log(LOG_ERROR, "(%s) Invalid JSON data received from /admin/system.json, hostname missing", Name.c_str());
	}
	else
	{
		m_status.hostname = root["confighostname"].asString();
		stdstring_rtrim(m_status.hostname);
		// Update hardware name if this is first run
		if (m_bFirstRun)
		{
			m_sql.safe_query("UPDATE Hardware SET Name = '%s' WHERE ID = %i", m_status.hostname.c_str(), m_HwdID);
			m_bFirstRun = false;
		}
	}

	// Get Teleinfo meter data and process pulse counters
	sstr.str("");
	sstr << m_ssURL.str() << "/admin/status.xml";

	if (HTTPClient::GET(sstr.str(), ExtraHeaders, sResult))
	{
		_log.Log(LOG_NORM, "(%s) Fetching data from /admin/status.xml", Name.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR, "(%s) Error getting status.xml!", Name.c_str());
		return;
	}

	// Process XML result
	XMLdoc.Parse(sResult.c_str(), 0, TIXML_ENCODING_UTF8);
	if (XMLdoc.Error())
	{
		_log.Log(LOG_ERROR, "(%s) Error parsing XML at /admin/status.xml: %s", Name.c_str(), XMLdoc.ErrorDesc());
		return;
	}

	// XML format changes dramatically between firmware versions. This code was developped for version 2.0.29
	using namespace TinyXPath;
	std::string product = S_xpath_string(XMLdoc.RootElement(), "/response/product/text()").c_str();
	if (product != "ECODEVICES RT 2")
	{
		_log.Log(LOG_ERROR, "(%s) Product information found in XML file is not 'ECODEVICES RT 2' as expected, but '%s'", Name.c_str(), product.c_str());
		return;
	}
	m_status.version = S_xpath_string(XMLdoc.RootElement(), "/response/infofirm/text()").c_str();

#ifdef DEBUG_EcoDevices
	_log.Log(LOG_NORM, "DEBUG: XML output for /admin/status.xml\n%s", MakeHtml(sResult).c_str());
#endif

	m_status.version = m_status.version + "..";
	major = atoi(m_status.version.substr(0, m_status.version.find(".")).c_str());
	m_status.version.erase(0, m_status.version.find(".") + 1);
	minor = atoi(m_status.version.substr(0, m_status.version.find(".")).c_str());
	m_status.version.erase(0, m_status.version.find(".") + 1);
	release = atoi(m_status.version.substr(0, m_status.version.find(".")).c_str());
	m_status.version = S_xpath_string(XMLdoc.RootElement(), "/response/version/text()").c_str();

	if (!((major > min_major) || ((major == min_major) && (minor > min_minor)) || ((major == min_major) && (minor == min_minor) && (release >= min_release))))
	{
		message = "EcoDevices RT2 firmware needs to be at least version ";
		message = message + static_cast<std::ostringstream*>(&(std::ostringstream() << min_major << "." << min_minor << "." << min_release))->str();
		message = message + ", current version is " + m_status.version;
		_log.Log(LOG_ERROR, "(%s) %s", Name.c_str(), message.c_str());
		return;
	}

	//Measured voltage on power supply 
	m_status.voltage = i_xpath_int(XMLdoc.RootElement(), "/response/vmesure/text()");
	SendVoltageSensor(m_HwdID, 1, 255, (float)m_status.voltage, "EcoDevice RT2");

	// Teleinfo data processing
	for (i = 0; i < 32; i++)
	{
		sprintf(XMLLabel, "/response/etiquette%i/text()", i);
		label = S_xpath_string(XMLdoc.RootElement(), XMLLabel).c_str();
		sprintf(XMLLabel, "/response/etiquetteEC%i/text()", i);
		value = S_xpath_string(XMLdoc.RootElement(), XMLLabel).c_str();
		if (label == "") break;
		XMLmap[label] = value;
	}
	m_teleinfo1.OPTARIF = XMLmap["OPTARIF"];
	m_teleinfo1.PTEC = XMLmap["PTEC"];
	m_teleinfo1.DEMAIN = XMLmap["DEMAIN"];
	m_teleinfo1.ISOUSC = atoi(XMLmap["ISOUSC"].c_str());
	m_teleinfo1.PAPP = atoi(XMLmap["PAPP"].c_str());
	m_teleinfo1.BASE = atoi(XMLmap["BASE"].c_str());
	m_teleinfo1.HCHC = atoi(XMLmap["HCHC"].c_str());
	m_teleinfo1.HCHP = atoi(XMLmap["HCHP"].c_str());
	m_teleinfo1.EJPHN = atoi(XMLmap["EJPHN"].c_str());
	m_teleinfo1.EJPHPM = atoi(XMLmap["EJPHPM"].c_str());
	m_teleinfo1.BBRHCJB = atoi(XMLmap["BBRHCJB"].c_str());
	m_teleinfo1.BBRHPJB = atoi(XMLmap["BBRHPJB"].c_str());
	m_teleinfo1.BBRHCJW = atoi(XMLmap["BBRHCJW"].c_str());
	m_teleinfo1.BBRHPJW = atoi(XMLmap["BBRHPJW"].c_str());
	m_teleinfo1.BBRHCJR = atoi(XMLmap["BBRHCJR"].c_str());
	m_teleinfo1.BBRHPJR = atoi(XMLmap["BBRPJR"].c_str());
	m_teleinfo1.PEJP = atoi(XMLmap["PEJP"].c_str());
	m_teleinfo1.IINST = atoi(XMLmap["IINST"].c_str());
	m_teleinfo1.IINST1 = atoi(XMLmap["IINST1"].c_str());
	m_teleinfo1.IINST2 = atoi(XMLmap["IINST2"].c_str());
	m_teleinfo1.IINST3 = atoi(XMLmap["IISNT3"].c_str());
	m_teleinfo1.PPOT = atoi(XMLmap["PPOT"].c_str());
	m_teleinfo1.ADPS = atoi(XMLmap["ADPS"].c_str());

#ifdef DEBUG_EcoDevices
	_log.Log(LOG_NORM, "DEBUG: OPTARIF: '%s'", m_teleinfo1.OPTARIF.c_str());
	_log.Log(LOG_NORM, "DEBUG: PTEC:    '%s'", m_teleinfo1.PTEC.c_str());
	_log.Log(LOG_NORM, "DEBUG: DEMAIN:  '%s'", m_teleinfo1.DEMAIN.c_str());
#endif
	ProcessTeleinfo(m_status.hostname.c_str(), 1, m_teleinfo1);

	// 8 internal counters (postes) processing
	for (i = 0; i < 8; i++)
	{
		sprintf(XMLLabel, "/response/poste%i/text()", i);
		label = S_xpath_string(XMLdoc.RootElement(), XMLLabel).c_str();
		sprintf(XMLLabel, "/response/info%i/text()", i);
		value = S_xpath_string(XMLdoc.RootElement(), XMLLabel).c_str();
		StringSplit(value, ",", splitresults);
		if (splitresults[0] == "")
			break;
		else
		{
			fvalue1 = (float)atof(splitresults[0].c_str());
			if (fvalue1 > 0) SendMeterSensor(m_HwdID, i, 255, fvalue1 / 1000, m_status.hostname + " " + label);
			fvalue2 = (float)atof(splitresults[1].c_str());
			if (fvalue2 > 0) SendWaterflowSensor(m_HwdID, i, 255, fvalue2, m_status.hostname + " " + label);
		}
	}
}
