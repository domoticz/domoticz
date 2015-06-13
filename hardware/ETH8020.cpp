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

extern CSQLHelper m_sql;

#define ETH8020_POLL_INTERVAL 10

CETH8020::CETH8020(const int ID, const std::string IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password)
{
	m_HwdID=ID;
	m_szIPAddress=IPAddress;
	m_Username = username;
	m_Password = password;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	m_bOutputLog = false;
	Init();
}

CETH8020::~CETH8020(void)
{
}

void CETH8020::Init()
{
}

bool CETH8020::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CETH8020::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "ETH8020: Started");
	return (m_thread!=NULL);
}

bool CETH8020::StopHardware()
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

void CETH8020::Do_Work()
{
	int sec_counter = ETH8020_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (sec_counter % ETH8020_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"ETH8020: Worker stopped...");
}

bool CETH8020::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pSen = (tRBUF*)pdata;

	unsigned char packettype = pSen->ICMND.packettype;
	unsigned char subtype = pSen->ICMND.subtype;

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
		if (sResult.find("Success") == std::string::npos)
		{
			_log.Log(LOG_ERROR, "ETH8020: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	return false;
}

void CETH8020::UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname)
{
	bool bDeviceExits = true;
	double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, Idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << SubUnit << ")";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	else
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx << "') AND (Unit == " << SubUnit << ")";
		m_sql.query(szQuery.str());
	}
}

void CETH8020::SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	char szTmp[30];
	sprintf(szTmp, "%08X", (unsigned int)Idx);

	szQuery << "SELECT Name FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}

	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVoltage;
	gDevice.id = 1;
	gDevice.floatval1 = Volt;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char *)&gDevice);

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szTmp << "') AND (Type==" << int(pTypeGeneral) << ") AND (Subtype==" << int(sTypeVoltage) << ")";
		result = m_sql.query(szQuery.str());
	}
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
	int pos1;
	int Idx = 0;
	for (ii = 1; ii < results.size(); ii++)
	{
		tmpstr = results[ii];
		if (tmpstr.find("<relay") != std::string::npos)
		{
			tmpstr = tmpstr.substr(strlen("<relay"));
			pos1 = tmpstr.find(">");
			if (pos1 != std::string::npos)
			{
				Idx = atoi(tmpstr.substr(0, pos1).c_str());
				tmpstr = tmpstr.substr(pos1+1);
				pos1 = tmpstr.find("<");
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
			pos1 = tmpstr.find(">");
			if (pos1 != std::string::npos)
			{
				Idx = atoi(tmpstr.substr(0, pos1).c_str());
				tmpstr = tmpstr.substr(pos1 + 1);
				pos1 = tmpstr.find("<");
				if (pos1 != std::string::npos)
				{
					int lValue = atoi(tmpstr.substr(0, pos1).c_str());
					float voltage = (float)(5.0f / 1023.0f)*lValue;
					if (voltage > 5.0f)
						voltage = 5.0f;
					std::stringstream sstr;
					sstr << "Voltage " << Idx;
					SendVoltage(Idx, voltage, sstr.str());
				}
			}
		}
	}
}
