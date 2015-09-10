#include "stdafx.h"
#include "Kodi.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "pinger/icmp_header.h"
#include "pinger/ipv4_header.h"

#include <iostream>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()
#define round(a) ( int ) ( a + .5 )
#define MAX_TITLE_LEN 40

CKodi::CKodi(const int ID, const int PollIntervalsec, const int PingTimeoutms) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CKodi::CKodi(const int ID) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	SetSettings(10, 3000);
}

CKodi::~CKodi(void)
{
	m_bIsStarted = false;
}

Json::Value CKodi::Query(std::string sIP, int iPort, std::string sQuery)
{
	Json::Value root;
	std::string sResult;
	std::stringstream	sURL;
	sURL << "http://" << sIP << ":" << iPort << sQuery;
	HTTPClient::SetTimeout(m_iPingTimeoutms/1000);
	bool	bRetVal = HTTPClient::GET(sURL.str(), sResult);
	if (!bRetVal)
	{
//		_log.Log(LOG_ERROR, "Kodi: GET ERROR: %s", sURL.str().c_str());		// Normally timeout so don't log
		return root;
	}
	Json::Reader jReader;
	bRetVal = jReader.parse(sResult, root);
	if (!bRetVal)
	{
		_log.Log(LOG_ERROR, "Kodi: PARSE ERROR: %s", sResult.c_str());
		return root;
	}
	if (root["error"].empty() != true)
	{
		/* e.g {"error":{"code":-32100,"message":"Failed to execute method."},"id":1,"jsonrpc":"2.0"}	*/
		_log.Log(LOG_ERROR, "Kodi: %d - '%s' request '%s'", root["error"]["code"].asInt(), root["error"]["message"].asCString(), sQuery.c_str());
		return root;
	}
	return root;
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
	//Find out node, and update it's status
	std::vector<KodiNode>::iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->ID == Node.ID)
		{
			//Found it
			bool	bUseOnOff = false;
			if (((nStatus == MSTAT_OFF) && bPingOK) || ((nStatus != MSTAT_OFF) && !bPingOK)) bUseOnOff = true;
			time_t atime = mytime(NULL);
			itt->LastOK = atime;
			if ((itt->nStatus != nStatus) || (itt->sStatus != sStatus))
			{
				/*
					Media device appears too different to integrate into main code :(
				*/

				// 1:	Update the DeviceStatus
				_log.Log(LOG_STATUS, "Kodi: (%s) %s - '%s'", Node.Name.c_str(), Media_Player_States(nStatus), sStatus.c_str());
				struct tm ltime;
				localtime_r(&atime, &ltime);
				char szLastUpdate[40];
				sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
					int(nStatus), sStatus.c_str(), szLastUpdate, m_HwdID, itt->szDevID, STYPE_Media);

				// 2:	Log the event if the actual status has changed but remove ", xx%" to reduce extraneous logging
				std::string sShortStatus = sStatus;
				if (sShortStatus.find_last_of("%") == sShortStatus.length() - 1)
				{
					sShortStatus = sShortStatus.substr(0, sShortStatus.find_last_of(","));
				}
				if ((itt->nStatus != nStatus) || (itt->sShortStatus != sShortStatus))
				{
					std::string sLongStatus = Media_Player_States(nStatus);
					if (sShortStatus.length()) sLongStatus += " - " + sShortStatus;
					result = m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue) VALUES (%d, %d, '%q')", itt->ID, int(nStatus), sLongStatus.c_str());
				}

				// 3:	Trigger On/Off actions
				if (bUseOnOff)
				{
					result = m_sql.safe_query("SELECT StrParam1,StrParam2 FROM DeviceStatus WHERE (HardwareID==%d) AND (ID = '%q') AND (Unit == 1)", m_HwdID, itt->szDevID);
					if (result.size() > 0)
					{
						m_sql.HandleOnOffAction(bPingOK, result[0][0], result[0][1]);
					}
				}

				// 4:	Trigger Notifications (TBD)

				itt->nStatus = nStatus;
				itt->sStatus = sStatus;
				itt->sShortStatus = sShortStatus;
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

	// http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetActivePlayers%22,%22id%22:1}
	//		{"id":1,"jsonrpc":"2.0","result":[{"playerid":1,"type":"video"}]}

	// http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetItem%22,%22id%22:1,%22params%22:{%22playerid%22:0,%22properties%22:[%22artist%22,%22year%22,%22channel%22,%22season%22,%22episode%22]}}
	//		{"id":1,"jsonrpc":"2.0","result":{"item":{"artist":["Coldplay"],"id":25,"label":"The Scientist","type":"song","year":2002}}}

	// http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetProperties%22,%22id%22:1,%22params%22:{%22playerid%22:1,%22properties%22:[%22totaltime%22,%22percentage%22,%22time%22]}}
	//		{"id":1,"jsonrpc":"2.0","result":{"percentage":22.207427978515625,"time":{"hours":0,"milliseconds":948,"minutes":15,"seconds":31},"totaltime":{"hours":1,"milliseconds":560,"minutes":9,"seconds":56}}}

	try
	{
		Json::Value root = Query(Node.IP, Node.Port, "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetActivePlayers%22,%22id%22:1}");
		if (!root.size())
			nStatus = MSTAT_OFF;
		else
		{
			bPingOK = true;
			std::string	sPlayerId;	
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
				std::string	sMedia = root["result"][0]["type"].asCString();
				if (root["result"][0]["playerid"].empty() == true)
				{
					_log.Log(LOG_ERROR, "Kodi: No PlayerID returned when player is not idle!");
					return;
				}
				if (sMedia == "video") nStatus = MSTAT_VIDEO;
				if (sMedia == "audio") nStatus = MSTAT_AUDIO;
				if (sMedia == "picture") nStatus = MSTAT_PHOTO;
				sPlayerId = SSTR((int)root["result"][0]["playerid"].asInt());
			}

			// If player is active then pick up additional details
			if (sPlayerId != "")
			{
				std::string	sTitle;
				std::string	sPercent;
				std::string	sYear;

				root = Query(Node.IP, Node.Port, "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetItem%22,%22id%22:1,%22params%22:{%22playerid%22:" + sPlayerId + ",%22properties%22:[%22artist%22,%22year%22,%22channel%22,%22showtitle%22,%22season%22,%22episode%22,%22title%22]}}");
				if (root.size())
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
							sTitle += SSTR((int)root["result"]["item"]["season"].asInt());
						}
						if (root["result"]["item"]["episode"].empty() != true)
						{
							sTitle += "E";
							sTitle += SSTR((int)root["result"]["item"]["episode"].asInt());
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

					if (root["result"]["item"]["year"].empty() != true)
					{
						sYear = SSTR((int)root["result"]["item"]["year"].asInt());
						if (sYear.length() > 2) sYear = " (" + sYear + ")";
						else sYear = "";
					}

					if (root["result"]["item"]["title"].empty() != true)
					{
						std::string	sLabel = sTitle + root["result"]["item"]["title"].asCString();
						if ((!sLabel.length()) && (root["result"]["item"]["label"].empty() != true))
						{
							sLabel = root["result"]["item"]["label"].asCString();
						}
						// if title is too long shorten it by removing things in brackets, followed by things after a ", "
						boost::algorithm::trim(sLabel);
						if (sLabel.length() > MAX_TITLE_LEN)
						{
							boost::algorithm::replace_all(sLabel, " - ", ", ");
						}
						while (sLabel.length() > MAX_TITLE_LEN)
						{
							int begin = sLabel.find_last_of("(");
							int end = sLabel.find_last_of(")");
							if ((std::string::npos == begin) || (std::string::npos == end) || (begin >= end)) break;
							sLabel.erase(begin, end - begin + 1);
						}
						while (sLabel.length() > MAX_TITLE_LEN)
						{
							int end = sLabel.find_last_of(",");
							if (std::string::npos == end) break;
							sLabel = sLabel.substr(0, end);
						}
						boost::algorithm::trim(sLabel);
						stdreplace(sLabel, " ,", ",");
						sLabel = sLabel.substr(0, MAX_TITLE_LEN);
						sTitle = sLabel;
					}
				}

				root = Query(Node.IP, Node.Port, "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22Player.GetProperties%22,%22id%22:1,%22params%22:{%22playerid%22:" + sPlayerId + ",%22properties%22:[%22live%22,%22percentage%22,%22speed%22]}}");
				if (root.size())
				{
					bool	bLive = root["result"]["live"].asBool();
					if (bLive) sYear = " (Live)";
					int		iSpeed = root["result"]["speed"].asInt();
					if (iSpeed == 0) nStatus = MSTAT_PAUSED;
					float	fPercent = root["result"]["percentage"].asFloat();
					if (fPercent > 1.0) sPercent = SSTR((int)round(fPercent)) + "%";
				}

				// Assemble final status
				sStatus = sTitle;
				if (sStatus.length() < (MAX_TITLE_LEN-7)) sStatus += sYear;
				if (sPercent.length() != 0) sStatus += ", " + sPercent;
			}
		}
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
	//	http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22System.GetProperties%22,%22params%22:{%22properties%22:[%22canreboot%22,%22canhibernate%22,%22cansuspend%22,%22canshutdown%22]},%22id%22:1}
	//		{"id":1,"jsonrpc":"2.0","result":{"canhibernate":false,"canreboot":false,"canshutdown":false,"cansuspend":false}}

	tRBUF *pSen = (tRBUF*)pdata;

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	if (pSen->LIGHTING2.cmnd != light2_sOff)
	{
		return true;
	}
	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;
	std::vector<KodiNode>::const_iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
//		_log.Log(LOG_NORM, "Kodi: (%s) Switch Off: Checking %d == %d.", itt->Name.c_str(), itt->DevID, DevID);
		if (itt->DevID == DevID)
		{
			std::string	sAction = "Nothing";
			bool		bCanReboot = false;
			bool		bCanShutdown = false;
			bool		bCanHibernate = false;
			bool		bCanSuspend = false;
			std::string	sPlayerId;
			Json::Value root = Query(itt->IP, itt->Port, "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22System.GetProperties%22,%22params%22:{%22properties%22:[%22canreboot%22,%22canhibernate%22,%22cansuspend%22,%22canshutdown%22]},%22id%22:1}");
			if (root.size())
			{
				if (root["result"]["canhibernate"].empty() != true)
				{
					bCanHibernate = root["result"]["canhibernate"].asBool();
					if (bCanHibernate) sAction = "Hibernate";
				}
				if (root["result"]["cansuspend"].empty() != true)
				{
					bCanSuspend = root["result"]["cansuspend"].asBool();
					if (bCanSuspend) sAction = "Suspend";
				}
				if (root["result"]["canreboot"].empty() != true)
				{
					bCanReboot = root["result"]["canreboot"].asBool();
					if (bCanReboot) sAction = "Reboot";
				}
				if (root["result"]["canshutdown"].empty() != true)
				{
					bCanShutdown = root["result"]["canshutdown"].asBool();
					if (bCanShutdown) sAction = "Shutdown";
				}
				_log.Log(LOG_NORM, "Kodi: (%s) Switch Off: CanReboot:%s, CanShutdown:%s, CanHibernate:%s, CanSuspend:%s. %s requested.", itt->Name.c_str(),
					bCanReboot ? "true" : "false", bCanShutdown ? "true" : "false", bCanHibernate ? "true" : "false", bCanSuspend ? "true" : "false", sAction.c_str());

				if (sAction != "Nothing")
				{
					root = Query(itt->IP, itt->Port, "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22System." + sAction + "%22,%22id%22:1}");
					return (root.size() > 0);
				}
			}
			else
			{
				_log.Log(LOG_NORM, "Kodi: (%s) Switch Off: No response from device, request ignored.", itt->Name.c_str());
			}
		}
	}

	return false;
}

void CKodi::AddNode(const std::string &Name, const std::string &IPAddress, const int Port)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.size()>0)
		return; //Already exists
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (%d, '%q', '%q', %d)", m_HwdID, Name.c_str(), IPAddress.c_str(), Port);

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.size()<1)
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also add a light (push) device
	m_sql.safe_query(
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (%d, '%q', 1, %d, %d, %d, 1, 12, 255, '%q', 0, 'Unavailable')",
		m_HwdID, szID, int(pTypeLighting2), int(sTypeAC), int(STYPE_Media), Name.c_str());

	ReloadNodes();
}

bool CKodi::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, ID);
	if (result.size()<1)
		return false; //Not Found!?

	m_sql.safe_query("UPDATE WOLNodes SET Name='%q', MacAddress='%q', Timeout=%d WHERE (HardwareID==%d) AND (ID==%d)", Name.c_str(), IPAddress.c_str(), Port, m_HwdID, ID);

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also update Light/Switch
	m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')", Name.c_str(), m_HwdID, szID);
	ReloadNodes();
	return true;
}

void CKodi::RemoveNode(const int ID)
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, ID);

	//Also delete the switch
	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szID);
	ReloadNodes();
}

void CKodi::RemoveAllNodes()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);

	//Also delete the all switches
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	ReloadNodes();
}

void CKodi::ReloadNodes()
{
	m_nodes.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			KodiNode pnode;
			pnode.DevID = atoi(sd[0].c_str());
			sprintf(pnode.szDevID, "%X%02X%02X%02X", 0, 0, (pnode.DevID & 0xFF00) >> 8, pnode.DevID & 0xFF);
			pnode.Name = sd[1];
			pnode.IP = sd[2];
			int Port = atoi(sd[3].c_str());
			pnode.Port = (Port > 0) ? Port : 8080;
			pnode.nStatus = MSTAT_UNKNOWN;
			pnode.sStatus = "";
			pnode.LastOK = mytime(NULL);

			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == 1)", m_HwdID, pnode.szDevID);
			if (result2.size()==1)
			{
				pnode.ID = atoi(result2[0][0].c_str());
				pnode.nStatus = (_eMediaStatus)atoi(result2[0][1].c_str());
				pnode.sStatus = result2[0][2];
			}

			m_nodes.push_back(pnode);
		}
	}
}

void CKodi::SendCommand(const int ID, const std::string &command)
{
	std::vector<std::vector<std::string> > result;

	// Get device details
	result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE (ID==%d)", ID);
	if (result.size() == 1)
	{
		// Get connection details
		long	DeviceID = strtol(result[0][0].c_str(), NULL, 16); 
		result = m_sql.safe_query("SELECT Name, MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, DeviceID);
	}

	if (result.size() == 1)
	{
		std::string	sKodiCall;
		std::string	sKodiParam = "";
		if (command == "Home")
		{
			sKodiCall = "Input.Home";
		}
		else if (command == "Up")
		{
			sKodiCall = "Input.Up";
		}
		else if (command == "Down")
		{
			sKodiCall = "Input.Down";
		}
		else if (command == "Left")
		{
			sKodiCall = "Input.Left";
		}
		else if (command == "Right")
		{
			sKodiCall = "Input.Right";
		}
		else  // Assume generic ExecuteAction  for any unrecognised strings
		{
			sKodiCall = "Input.ExecuteAction";
			std::string	sLower = command;
			std::transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower);
			sKodiParam = sLower;
		}

		if (sKodiCall.length())
		{
			//		http://kodi.wiki/view/JSON-RPC_API/v6#Input.Action
			//		{ "jsonrpc": "2.0", "method": "Input.ExecuteAction", "params": { "action": "stop" }, "id": 1 }
			std::stringstream	ssRequest;
			ssRequest << "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22" << sKodiCall << "%22,%22params%22:{";
			if (sKodiParam.length()) ssRequest << "%22action%22:%22" << sKodiParam << "%22";
			ssRequest << "},%22id%22:2}";
			Json::Value root = Query(result[0][1].c_str(), atoi(result[0][2].c_str()), ssRequest.str());
			if (root.size())
			{
				// keep going
				if (root["result"].empty() != true)
				{
					_log.Log(LOG_NORM, "Kodi: (%s) Command: '%s', Result '%s'.", result[0][0].c_str(), command.c_str(), root["result"].asCString());
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Kodi: (%s) Command: '%s'. Unknown command.", result[0][0].c_str(), command.c_str());
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Kodi: (%d) Command: '%s'. Device not found.", ID, command.c_str());
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_KodiGetNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_Kodi)
				return;

			root["status"] = "OK";
			root["title"] = "KodiGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", iHardwareID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = sd[2];
					root["result"][ii]["Port"] = atoi(sd[3].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_KodiSetMode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}
			std::string hwid = m_pWebEm->FindValue("idx");
			std::string mode1 = m_pWebEm->FindValue("mode1");
			std::string mode2 = m_pWebEm->FindValue("mode2");
			if (
				(hwid == "") ||
				(mode1 == "") ||
				(mode2 == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "KodiSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}

		void CWebServer::Cmd_KodiAddNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string name = m_pWebEm->FindValue("name");
			std::string ip = m_pWebEm->FindValue("ip");
			int Port = atoi(m_pWebEm->FindValue("port").c_str());
			if (
				(hwid == "") ||
				(name == "") ||
				(ip == "") ||
				(Port == 0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "KodiAddNode";
			pHardware->AddNode(name, ip, Port);
		}

		void CWebServer::Cmd_KodiUpdateNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			std::string name = m_pWebEm->FindValue("name");
			std::string ip = m_pWebEm->FindValue("ip");
			int Port = atoi(m_pWebEm->FindValue("port").c_str());
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "") ||
				(ip == "") ||
				(Port == 0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "KodiUpdateNode";
			pHardware->UpdateNode(NodeID, name, ip, Port);
		}

		void CWebServer::Cmd_KodiRemoveNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "KodiRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_KodiClearNodes(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "KodiClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_KodiMediaCommand(Json::Value &root)
		{
			std::string sIdx = m_pWebEm->FindValue("idx");
			std::string sAction = m_pWebEm->FindValue("action");
			if (sIdx.empty())
				return;
			int idx = atoi(sIdx.c_str());
			root["status"] = "OK";
			root["title"] = "KodiMediaCommand";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT DS.SwitchType, H.Type, H.ID FROM DeviceStatus DS, Hardware H WHERE (DS.ID=='%q') AND (DS.HardwareID == H.ID)", sIdx.c_str());
			if (result.size() == 1)
			{
				_eSwitchType	sType = (_eSwitchType)atoi(result[0][0].c_str());
				_eHardwareTypes	hType = (_eHardwareTypes)atoi(result[0][1].c_str());
				int HwID = atoi(result[0][2].c_str());
				// Is the device a media Player?
				if (sType == STYPE_Media)
				{
					switch (hType) {
					case HTYPE_Kodi:
						CKodi	Kodi(HwID);
						Kodi.SendCommand(idx, sAction);
						break;
						// put other players here ...
					}
				}
			}
		}

	}
}
