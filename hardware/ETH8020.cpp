#include "stdafx.h"
#include "ETH8020.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

#define ETH8020_POLL_INTERVAL 10

CETH8020::CETH8020(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
m_szIPAddress(IPAddress),
m_Username(CURLEncode::URLEncode(username)),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_bOutputLog = false;
	Init();
}

void CETH8020::Init()
{
}

bool CETH8020::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "ETH8020: Started");
	return (m_thread != nullptr);
}

bool CETH8020::StopHardware()
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

void CETH8020::Do_Work()
{
	int sec_counter = ETH8020_POLL_INTERVAL - 2;

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % ETH8020_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"ETH8020: Worker stopped...");
}

bool CETH8020::WriteToHardware(const char *pdata, const unsigned char /*length*/)
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

		if (m_Password.empty())
		{
			szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
		}
		else
		{
			szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
		}

		if (pSen->LIGHTING2.cmnd == light2_sOff)
		{
			szURL << "/io.cgi?DOI" << Relay << "=0";
		}
		else
		{
			szURL << "/io.cgi?DOA" << Relay << "=0";
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			_log.Log(LOG_ERROR, "ETH8020: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
/*
		if (sResult.find("Success") == std::string::npos)
		{
			_log.Log(LOG_ERROR, "ETH8020: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
*/
		return true;
	}
	return false;
}

void CETH8020::UpdateSwitch(const unsigned char Idx, const uint8_t SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	double rlevel = (15.0 / 100)*Level;
	uint8_t level = (uint8_t)(rlevel);

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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}

void CETH8020::GetMeterDetails()
{
	std::string sResult;
	std::stringstream szURL;

	if(m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/status.xml";

	if (!HTTPClient::GET(szURL.str(),sResult))
	{
		_log.Log(LOG_ERROR,"ETH8020: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, "\r\n", results);
	if (results.size()<8)
	{
		_log.Log(LOG_ERROR,"ETH8020: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	if (results[0] != "<response>")
	{
		_log.Log(LOG_ERROR, "ETH8020: Error getting status");
		return;
	}
	size_t ii;
	std::string tmpstr;
	size_t pos1;
	uint8_t Idx = 0;
	for (ii = 1; ii < results.size(); ii++)
	{
		tmpstr = results[ii];
		if (tmpstr.find("<relay") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("<relay"));
			pos1 = tmpstr.find('>');
			if (pos1 != std::string::npos)
			{
				Idx = (uint8_t)atoi(tmpstr.substr(0, pos1).c_str());
				tmpstr = tmpstr.substr(pos1+1);
				pos1 = tmpstr.find('<');
				if (pos1 != std::string::npos)
				{
					int lValue = atoi(tmpstr.substr(0, pos1).c_str());
					std::stringstream sstr;
					sstr << "Relay " << Idx;
					UpdateSwitch(1, Idx, (lValue == 1) ? true : false, 100, sstr.str());
				}
			}
		}
		else if (tmpstr.find("<adc") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("<adc"));
			pos1 = tmpstr.find('>');
			if (pos1 != std::string::npos)
			{
				Idx = (uint8_t)atoi(tmpstr.substr(0, pos1).c_str());
				tmpstr = tmpstr.substr(pos1 + 1);
				pos1 = tmpstr.find('<');
				if (pos1 != std::string::npos)
				{
					int lValue = atoi(tmpstr.substr(0, pos1).c_str());
					float voltage = (float)(5.0F / 1023.0F) * lValue;
					if (voltage > 5.0F)
						voltage = 5.0F;
					std::stringstream sstr;
					sstr << "Voltage " << Idx;
					SendVoltageSensor(0, (uint8_t)Idx, 255, voltage, sstr.str());
				}
			}
		}
	}
}
