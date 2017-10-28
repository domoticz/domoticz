#include "stdafx.h"
#include "DenkoviSmartdenLan.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

#define DenkoviSmartdenLan_POLL_INTERVAL 60

CDenkoviSmartdenLan::CDenkoviSmartdenLan(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password) :
m_szIPAddress(IPAddress),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	m_bOutputLog = false;
	Init();
}

CDenkoviSmartdenLan::~CDenkoviSmartdenLan(void)
{
}

void CDenkoviSmartdenLan::Init()
{
}

bool CDenkoviSmartdenLan::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDenkoviSmartdenLan::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "Denkovi: Started");
	return (m_thread!=NULL);
}

bool CDenkoviSmartdenLan::StopHardware()
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

void CDenkoviSmartdenLan::Do_Work()
{
	int sec_counter = DenkoviSmartdenLan_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (sec_counter % DenkoviSmartdenLan_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"Denkovi: Worker stopped...");
}

bool CDenkoviSmartdenLan::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if (packettype == pTypeLighting2)
	{
		//light command

		int Relay = pSen->LIGHTING2.unitcode;
		if (Relay > 20)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
		{
			szURL << "?pw=" << m_Password << "&";
		}
		else
			szURL << "?";
		szURL << "Relay" << Relay << "=";
		if (pSen->LIGHTING2.cmnd == light2_sOff)
		{
			szURL << "0";
		}
		else
		{
			szURL << "1";
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			_log.Log(LOG_ERROR, "Denkovi: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos)
		{
			_log.Log(LOG_ERROR, "Denkovi: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	return false;
}

void CDenkoviSmartdenLan::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, SubUnit);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
		{
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel == level)
				return;
		}
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = 0;
	lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = Idx;
	lcmd.LIGHTING2.unitcode = SubUnit;
	if (!bOn)
	{
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

void CDenkoviSmartdenLan::GetMeterDetails()
{
	std::string sResult;
	std::stringstream szURL;

	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
	if (!m_Password.empty())
	{
		szURL << "?pw=" << m_Password;
	}

	if (!HTTPClient::GET(szURL.str(),sResult))
	{
		_log.Log(LOG_ERROR,"Denkovi: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, "\r\n", results);
	if (results.size()<8)
	{
		_log.Log(LOG_ERROR,"Denkovi: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	if (results[0] != "<CurrentState>")
	{
		_log.Log(LOG_ERROR, "Denkovi: Error getting status");
		return;
	}
	size_t ii;
	std::string tmpstr;
	int Idx = -1;

	for (ii = 1; ii < results.size(); ii++)
	{
		tmpstr = results[ii];
		size_t pos1 = tmpstr.find("<Relay");
		if (pos1 != std::string::npos)
		{
			tmpstr = tmpstr.substr(pos1 + strlen("<Relay"));
			pos1 = tmpstr.find(">");
			if (pos1 != std::string::npos)
			{
				Idx = atoi(tmpstr.substr(0, pos1).c_str());
				continue;
			}
		}
		pos1 = tmpstr.find("<State>");
		if (pos1 != std::string::npos)
		{
			if (Idx==-1)
				continue;
			tmpstr = tmpstr.substr(pos1 + strlen("<State>"));
			pos1 = tmpstr.find("</State>");
			if (pos1 != std::string::npos)
			{
				int lValue = atoi(tmpstr.substr(0, pos1).c_str());
				std::stringstream sstr;
				sstr << "Relay " << Idx;
				UpdateSwitch(1, Idx, (lValue == 1) ? true : false, 100, sstr.str());
				Idx = -1;
			}
		}
	}
}
