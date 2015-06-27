#include "stdafx.h"
#include "Kodi.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"
#include "../json/json.h"
#include "../httpclient/HTTPClient.h"
#include "../main/localtime_r.h"

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "pinger/icmp_header.h"
#include "pinger/ipv4_header.h"

#include <iostream>

// http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetActivePlayers%22,%22id%22:1}
//		{"id":1,"jsonrpc":"2.0","result":[{"playerid":1,"type":"video"}]}

// http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetItem%22,%22id%22:1,%22params%22:{%22playerid%22:0,%22properties%22:[%22artist%22,%22year%22,%22channel%22,%22season%22,%22episode%22]}}
//		{"id":1,"jsonrpc":"2.0","result":{"item":{"artist":["Coldplay"],"id":25,"label":"The Scientist","type":"song","year":2002}}}

// http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetProperties%22,%22id%22:1,%22params%22:{%22playerid%22:1,%22properties%22:[%22totaltime%22,%22percentage%22,%22time%22]}}
//		{"id":1,"jsonrpc":"2.0","result":{"percentage":22.207427978515625,"time":{"hours":0,"milliseconds":948,"minutes":15,"seconds":31},"totaltime":{"hours":1,"milliseconds":560,"minutes":9,"seconds":56}}}

CKodi::CKodi(const int ID, const int PollIntervalsec, const int PingTimeoutms) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CKodi::~CKodi(void)
{
	m_bIsStarted = false;
}

bool CKodi::StartHardware()
{
	StopHardware();
	m_bIsStarted = true;
	sOnConnected(this);
	m_iThreadsRunning = 0;

	StartHeartbeatThread();

	ReloadNodes();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CKodi::Do_Work, this)));
	_log.Log(LOG_STATUS, "Kodi: Started");

	return true;
}

bool CKodi::StopHardware()
{
	StopHeartbeatThread();

	try {
		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			m_thread.reset();

			//Make sure all our background workers are stopped
			int iRetryCounter = 0;
			while ((m_iThreadsRunning > 0) && (iRetryCounter<15))
			{
				sleep_milliseconds(500);
				iRetryCounter++;
			}
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	m_bIsStarted = false;
	return true;
}

void CKodi::UpdateNodeStatus(const KodiNode &Node, const _eMediaStatus nStatus, const std::string sStatus, bool bPingOK)
{
	_log.Log(LOG_STATUS, "Kodi: %s = %s, Status = %s", Node.Name.c_str(), (bPingOK == true) ? "OK" : "Error", sStatus.c_str());

	//Find out node, and update it's status
	std::vector<KodiNode>::iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->ID == Node.ID)
		{
			//Found it
			time_t atime = mytime(NULL);
			itt->LastOK = atime;
//			SendSwitch(Node.ID, 1, 255, bPingOK, 0, Node.Name);
			if ((itt->nStatus != nStatus) || (itt->sStatus != sStatus))
			{
				struct tm ltime;
				localtime_r(&atime, &ltime);
				char szID[40];
				sprintf(szID, "%X%02X%02X%02X", 0, 0, (itt->ID & 0xFF00) >> 8, itt->ID & 0xFF);
				char szLastUpdate[40];
				sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "UPDATE DeviceStatus SET nValue=" << nStatus << ", sValue='" << sStatus << "', LastUpdate='" << szLastUpdate << "' WHERE(HardwareID == " << m_HwdID << ") AND(DeviceID == '" << szID << "')";
				result = m_sql.query(szQuery.str());
				itt->nStatus = nStatus;
				itt->sStatus = sStatus;
			}
			break;
		}
	}
}

void CKodi::Do_Node_Work(const KodiNode &Node)
{
	bool			bPingOK = false;
	_eMediaStatus	nStatus = MSTAT_UNKNOWN;
	std::string		sStatus = "";

	try
	{
		std::string sResult;
		std::stringstream sURL;
		sURL << "http://" << Node.IP << ":" << Node.Port << "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetActivePlayers%22,%22id%22:1}";
		bool bRetVal = HTTPClient::GET(sURL.str(), sResult);
		if (!bRetVal)
			nStatus = MSTAT_OFF;
		else
		{
			bPingOK = true;
			Json::Value root;
			Json::Reader jReader;
			std::string	sPlayerId;
			std::string	sMedia;
			std::string	sTitle;
			std::string	sPercent;

			bRetVal = jReader.parse(sResult, root);
			if (!bRetVal)
			{
				_log.Log(LOG_ERROR, "Kodi: Invalid data received!");
				return;
			}
			if (root["result"].empty() == true)
			{
				nStatus = MSTAT_IDLE;
			}
			else if (root["result"][0]["type"].empty() == true)
			{
				nStatus = MSTAT_UNKNOWN;
			}
			else
			{
				sMedia = root["result"][0]["type"].asCString();
				if (root["result"][0]["playerid"].empty() == true)
				{
					_log.Log(LOG_ERROR, "Kodi: No PlayerID returned when player is not idle!");
					return;
				}
				if (sMedia == "video") nStatus = MSTAT_VIDEO;
				if (sMedia == "audio") nStatus = MSTAT_AUDIO;
				if (sMedia == "picture") nStatus = MSTAT_PHOTO;
				sPlayerId = std::to_string((int)root["result"][0]["playerid"].asInt());
			}

			// If player is active then pick up additional details
			if (sPlayerId != "")
			{
				sStatus = sMedia;
				sStatus[0] = toupper(sStatus[0]);
				sURL.str(std::string());
				sURL << "http://" << Node.IP << ":" << Node.Port << "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetItem%22,%22id%22:1,%22params%22:{%22playerid%22:" + sPlayerId + ",%22properties%22:[%22artist%22,%22year%22,%22channel%22,%22showtitle%22,%22season%22,%22episode%22,%22title%22]}}";
				if (HTTPClient::GET(sURL.str(), sResult))
				{
					if (jReader.parse(sResult, root))
					{
						std::string	sType;
						if (root["result"]["item"]["type"].empty() != true)
						{
							sType = root["result"]["item"]["type"].asCString();
						}

						if (sType == "song")
						{
							if (root["result"]["item"]["artist"][0].empty() != true)
							{
								sTitle = root["result"]["item"]["artist"][0].asCString();
								sTitle += " - ";
							}
						}

						if (sType == "episode")
						{
							if (root["result"]["item"]["showtitle"].empty() != true)
							{
								sTitle = root["result"]["item"]["showtitle"].asCString();
							}
							if (root["result"]["item"]["season"].empty() != true)
							{
								sTitle += " [S";
								sTitle += std::to_string((int)root["result"]["item"]["season"].asInt());
							}
							if (root["result"]["item"]["episode"].empty() != true)
							{
								sTitle += "E";
								sTitle += std::to_string((int)root["result"]["item"]["episode"].asInt());
								sTitle += "], ";
							}
						}

						if (sType == "channel")
						{
							if (root["result"]["item"]["channel"].empty() != true)
							{
								sTitle = root["result"]["item"]["channel"].asCString();
								sTitle += " - ";
							}
						}

						if (root["result"]["item"]["title"].empty() != true)
						{
							std::string	sLabel = root["result"]["item"]["title"].asCString();
							sLabel = sLabel.substr(0, 25);
							if (sLabel.find_first_of('(') != std::string::npos)
								sLabel = sLabel.substr(0, sLabel.find_first_of('('));
							sTitle += sLabel;
						}
						if (root["result"]["item"]["year"].empty() != true)
						{
							std::string	sYear = std::to_string((int)root["result"]["item"]["year"].asInt());
							if (sYear.length() > 2) sTitle += " (" + sYear + ")";
						}
					}
				}
				sURL.str(std::string());
				sURL << "http://" << Node.IP << ":" << Node.Port << "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetProperties%22,%22id%22:1,%22params%22:{%22playerid%22:" + sPlayerId + ",%22properties%22:[%22live%22,%22percentage%22,%22speed%22]}}";
				if (HTTPClient::GET(sURL.str(), sResult))
				{
					if (jReader.parse(sResult, root))
					{
						bool	bLive = root["result"]["live"].asBool();
						if (bLive) sTitle += " (Live)";
						int		iSpeed = root["result"]["speed"].asInt();
						if (iSpeed == 0) nStatus = MSTAT_PAUSED;
						float	fPercent = root["result"]["percentage"].asFloat();
						if (fPercent > 1.0) sPercent = std::to_string((int)round(fPercent)) + "%";
					}
				}

				// Assemble final status
				sStatus = sTitle;
				if (sPercent.length() != 0) sStatus += ", " + sPercent;
			}
		}
	}
	catch (std::exception& e)
	{
		bPingOK = false;
	}
	catch (...)
	{
		bPingOK = false;
	}
	UpdateNodeStatus(Node, nStatus, sStatus, bPingOK);
	if (m_iThreadsRunning > 0) m_iThreadsRunning--;
}

void CKodi::Do_Work()
{
	int mcounter = 0;
	int scounter = 0;
	bool bFirstTime = true;
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);
		mcounter++;
		if (mcounter == 2)
		{
			mcounter = 0;
			scounter++;
			if ((scounter >= m_iPollInterval) || (bFirstTime))
			{
				boost::lock_guard<boost::mutex> l(m_mutex);

				scounter = 0;
				bFirstTime = false;
				std::vector<KodiNode>::const_iterator itt;
				for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
				{
					if (m_stoprequested)
						return;
					if (m_iThreadsRunning < 1000)
					{
						m_iThreadsRunning++;
						boost::thread t(boost::bind(&CKodi::Do_Node_Work, this, *itt));
						t.join();
					}
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "Kodi: Worker stopped...");
}

void CKodi::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

void CKodi::Restart()
{
	StopHardware();
	StartHardware();
}

bool CKodi::WriteToHardware(const char *pdata, const unsigned char length)
{
	_log.Log(LOG_ERROR, "Kodi: This is a read-only sensor!");
	return true;
}

void CKodi::AddNode(const std::string &Name, const std::string &IPAddress, const int Port)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Check if exists
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (Name=='" << Name << "') AND (MacAddress=='" << IPAddress << "')";
	result = m_sql.query(szQuery.str());
	if (result.size()>0)
		return; //Already exists
	szQuery.clear();
	szQuery.str("");
	szQuery << "INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (" << m_HwdID << ",'" << Name << "','" << IPAddress << "'," << Port << ")";
	m_sql.query(szQuery.str());

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (Name=='" << Name << "') AND (MacAddress=='" << IPAddress << "')";
	result = m_sql.query(szQuery.str());
	if (result.size()<1)
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also add a light (push) device
	szQuery.clear();
	szQuery.str("");
	szQuery <<
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (" << m_HwdID << ",'" << szID << "'," << int(1) << "," << pTypeLighting2 << "," << sTypeAC << "," << int(STYPE_Media) << ",1, 12,255,'" << Name << "',0,'Unavailable')";
	m_sql.query(szQuery.str());

	ReloadNodes();
}

bool CKodi::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Check if exists
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	result = m_sql.query(szQuery.str());
	if (result.size()<1)
		return false; //Not Found!?

	szQuery.clear();
	szQuery.str("");

	szQuery << "UPDATE WOLNodes SET Name='" << Name << "', MacAddress='" << IPAddress << "', Timeout=" << Port << " WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	m_sql.query(szQuery.str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also update Light/Switch
	szQuery.clear();
	szQuery.str("");
	szQuery <<
		"UPDATE DeviceStatus SET Name='" << Name << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	m_sql.query(szQuery.str());
	ReloadNodes();
	return true;
}

void CKodi::RemoveNode(const int ID)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	szQuery << "DELETE FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	m_sql.query(szQuery.str());

	//Also delete the switch
	szQuery.clear();
	szQuery.str("");

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	szQuery << "DELETE FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	m_sql.query(szQuery.str());
	ReloadNodes();
}

void CKodi::RemoveAllNodes()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::stringstream szQuery;
	szQuery << "DELETE FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ")";
	m_sql.query(szQuery.str());

	//Also delete the all switches
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ")";
	m_sql.query(szQuery.str());
	ReloadNodes();
}

void CKodi::ReloadNodes()
{
	m_nodes.clear();
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ")";
	result = m_sql.query(szQuery.str());
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			KodiNode pnode;
			pnode.ID = atoi(sd[0].c_str());
			pnode.Name = sd[1];
			pnode.IP = sd[2];
			pnode.LastOK = mytime(NULL);
			pnode.nStatus = MSTAT_UNKNOWN;
			pnode.sStatus = "";

			int Port = atoi(sd[3].c_str());
			pnode.Port = (Port > 0) ? Port : 8080;
			m_nodes.push_back(pnode);
		}
	}
}


