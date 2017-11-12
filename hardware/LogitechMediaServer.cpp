#include "stdafx.h"
#include "LogitechMediaServer.h"
#include <boost/lexical_cast.hpp>
#include "../hardware/hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"

CLogitechMediaServer::CLogitechMediaServer(const int ID, const std::string &IPAddress, const int Port, const std::string &User, const std::string &Pwd, const int PollIntervalsec, const int PingTimeoutms) : 
m_IP(IPAddress),
m_User(User),
m_Pwd(Pwd),
m_stoprequested(false),
m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_Port = Port;
	m_bShowedStartupMessage = false;
	m_iMissedQueries = 0;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CLogitechMediaServer::CLogitechMediaServer(const int ID) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_IP = "";
	m_Port = 0;
	m_User = "";
	m_Pwd = "";
	m_bShowedStartupMessage = false;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Address, Port, Username, Password FROM Hardware WHERE ID==%d", m_HwdID);

	if (result.size() > 0)
	{
		m_IP = result[0][0];
		m_Port = atoi(result[0][1].c_str());
		m_User = result[0][2];
		m_Pwd = result[0][3];
	}

	SetSettings(10, 3000);
}

CLogitechMediaServer::~CLogitechMediaServer(void)
{
	m_bIsStarted = false;
}

Json::Value CLogitechMediaServer::Query(const std::string &sIP, const int iPort, const std::string &sPostdata)
{
	Json::Value root;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sURL;
	std::stringstream sPostData;

	if ((m_User != "") && (m_Pwd != ""))
		sURL << "http://" << m_User << ":" << m_Pwd << "@" << sIP << ":" << iPort << "/jsonrpc.js";
	else
		sURL << "http://" << sIP << ":" << iPort << "/jsonrpc.js";

	sPostData << sPostdata;
	
	HTTPClient::SetTimeout(m_iPingTimeoutms / 1000);
	bool bRetVal = HTTPClient::POST(sURL.str(), sPostData.str(), ExtraHeaders, sResult);

	if (!bRetVal)
	{
		return root;
	}
	Json::Reader jReader;
	bRetVal = jReader.parse(sResult, root);
	if ((!bRetVal) || (!root.isObject()))
	{
		size_t aFind = sResult.find("401 Authorization Required");
		if ((aFind > 0) && (aFind != std::string::npos))
			_log.Log(LOG_ERROR, "Logitech Media Server: Username and/or password are incorrect. Check Logitech Media Server settings.");
		else
			_log.Log(LOG_ERROR, "Logitech Media Server: PARSE ERROR: %s", sResult.c_str());
		return root;
	}
	if (root["method"].empty())
	{
		_log.Log(LOG_ERROR, "Logitech Media Server: '%s' request '%s'", sURL.str().c_str(), sPostData.str().c_str());
		return root;
	}
	return root["result"];
}

_eNotificationTypes	CLogitechMediaServer::NotificationType(_eMediaStatus nStatus)
{
	switch (nStatus)
	{
	case MSTAT_OFF:		return NTYPE_SWITCH_OFF;
	case MSTAT_ON:		return NTYPE_SWITCH_ON;
	case MSTAT_PAUSED:	return NTYPE_PAUSED;
	case MSTAT_STOPPED:	return NTYPE_STOPPED;
	case MSTAT_PLAYING:	return NTYPE_PLAYING;
	default:			return NTYPE_SWITCH_OFF;
	}
}

bool CLogitechMediaServer::StartHardware()
{
	StopHardware();
	m_bIsStarted = true;
	sOnConnected(this);
	m_iThreadsRunning = 0;
	m_bShowedStartupMessage = false;

	StartHeartbeatThread();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLogitechMediaServer::Do_Work, this)));

	return (m_thread != NULL);
}

bool CLogitechMediaServer::StopHardware()
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

void CLogitechMediaServer::UpdateNodeStatus(const LogitechMediaServerNode &Node, const _eMediaStatus nStatus, const std::string &sStatus, bool bPingOK)
{
	//Find out node, and update it's status
	std::vector<LogitechMediaServerNode>::iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->ID == Node.ID)
		{
			//Found it
			//Retrieve devicename instead of playername in case it was renamed...
			std::string sDevName = itt->Name;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE DeviceID=='%q'", itt->szDevID);
			if (result.size() == 1)	{
				std::vector<std::string> sd = result[0];
				sDevName = sd[0];
			}
			bool	bUseOnOff = false;
			if (((nStatus == MSTAT_OFF) && bPingOK) || ((nStatus != MSTAT_OFF) && !bPingOK)) bUseOnOff = true;
			time_t atime = mytime(NULL);
			itt->LastOK = atime;
			if ((itt->nStatus != nStatus) || (itt->sStatus != sStatus))
			{
				// 1:	Update the DeviceStatus
				if ((nStatus == MSTAT_PLAYING) || (nStatus == MSTAT_PAUSED) || (nStatus == MSTAT_STOPPED))
					_log.Log(LOG_NORM, "Logitech Media Server: (%s) %s - '%s'", Node.Name.c_str(), Media_Player_States(nStatus), sStatus.c_str());
				else
					_log.Log(LOG_NORM, "Logitech Media Server: (%s) %s", Node.Name.c_str(), Media_Player_States(nStatus));
				struct tm ltime;
				localtime_r(&atime, &ltime);
				char szLastUpdate[40];
				sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
					int(nStatus), sStatus.c_str(), szLastUpdate, m_HwdID, itt->szDevID, STYPE_Media);

				// 2:	Log the event if the actual status has changed
				std::string sShortStatus = sStatus;
				if ((itt->nStatus != nStatus) || (itt->sShortStatus != sShortStatus))
				{
					std::string sLongStatus = Media_Player_States(nStatus);
					if ((nStatus == MSTAT_PLAYING) || (nStatus == MSTAT_PAUSED) || (nStatus == MSTAT_STOPPED))
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

				// 4:   Trigger Notifications & events on status change
				if (itt->nStatus != nStatus)
				{
					m_notifications.CheckAndHandleNotification(itt->ID, sDevName, NotificationType(nStatus), sStatus.c_str());
					m_mainworker.m_eventsystem.ProcessDevice(m_HwdID, itt->ID, 1, int(pTypeLighting2), int(sTypeAC), 12, 100, int(nStatus), sStatus.c_str(), sDevName, 0);
				}

				itt->nStatus = nStatus;
				itt->sStatus = sStatus;
				itt->sShortStatus = sShortStatus;
			}
			break;
		}
	}
}

void CLogitechMediaServer::Do_Node_Work(const LogitechMediaServerNode &Node)
{
	bool bPingOK = false;
	_eMediaStatus nStatus = MSTAT_UNKNOWN;
	_eMediaStatus nOldStatus = Node.nStatus;
	std::string	sPlayerId = Node.IP;
	std::string	sStatus = "";

	try
	{
		std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + sPlayerId + "\",[\"status\",\"-\",1,\"tags:Aadly\"]]}";
		Json::Value root = Query(m_IP, m_Port, sPostdata);

		if (root.isNull())
			nStatus = MSTAT_DISCONNECTED;
		else
		{
			bPingOK = true;

			if (root["player_connected"].asString() == "1")
			{
				if (root["power"].asString() == "0")
					nStatus = MSTAT_OFF;
				else {
					std::string sMode = root["mode"].asString();
					if (sMode == "play") 
						if ((nOldStatus == MSTAT_OFF) || (nOldStatus == MSTAT_DISCONNECTED))
							nStatus = MSTAT_ON;
						else
							nStatus = MSTAT_PLAYING;
					else if (sMode == "pause")
						if ((nOldStatus == MSTAT_OFF) || (nOldStatus == MSTAT_DISCONNECTED))
							nStatus = MSTAT_ON;
						else
							nStatus = MSTAT_PAUSED;
					else if (sMode == "stop")
						if ((nOldStatus == MSTAT_OFF) || (nOldStatus == MSTAT_DISCONNECTED))
							nStatus = MSTAT_ON;
						else
							nStatus = MSTAT_STOPPED;
					else
						nStatus = MSTAT_ON;
					std::string	sTitle = "";
					std::string	sAlbum = "";
					std::string	sArtist = "";
					std::string	sAlbumArtist = "";
					std::string	sTrackArtist = "";
					std::string	sYear = "";
					std::string	sDuration = "";
					std::string sLabel = "";

					if (root["playlist_loop"].size()) {
						sTitle = root["playlist_loop"][0]["title"].asString();
						sAlbum = root["playlist_loop"][0]["album"].asString();
						sArtist = root["playlist_loop"][0]["artist"].asString();
						sAlbumArtist = root["playlist_loop"][0]["albumartist"].asString();
						sTrackArtist = root["playlist_loop"][0]["trackartist"].asString();
						sYear = root["playlist_loop"][0]["year"].asString();
						sDuration = root["playlist_loop"][0]["duration"].asString();

						if (sTrackArtist != "")
							sArtist = sTrackArtist;
						else
							if (sAlbumArtist != "")
								sArtist = sAlbumArtist;
						if (sYear == "0") sYear = "";
						if (sYear != "")
							sYear = " (" + sYear + ")";

						sLabel = sArtist + " - " + sTitle + sYear;
					}
					else
						sLabel = "(empty playlist)";

					sStatus = sLabel;
				}
			}
			else
				nStatus = MSTAT_DISCONNECTED;
		}
	}
	catch (...)
	{
		bPingOK = false;
	}
	UpdateNodeStatus(Node, nStatus, sStatus, bPingOK);
	if (m_iThreadsRunning > 0) m_iThreadsRunning--;
}

void CLogitechMediaServer::Do_Work()
{
	int mcounter = 0;
	int scounter = 0;
	bool bFirstTime = true;

	_log.Log(LOG_STATUS, "Logitech Media Server: Worker started...");

	ReloadNodes();
	ReloadPlaylists();

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

				GetPlayerInfo();

				std::vector<LogitechMediaServerNode>::const_iterator itt;
				for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
				{
					if (m_stoprequested)
						return;
					if (m_iThreadsRunning < 1000)
					{
						m_iThreadsRunning++;
						boost::thread t(boost::bind(&CLogitechMediaServer::Do_Node_Work, this, *itt));
						t.join();
					}
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "Logitech Media Server: Worker stopped...");
}

void CLogitechMediaServer::GetPlayerInfo()
{
	try
	{
		std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"\",[\"serverstatus\",0,999]]}";
		Json::Value root = Query(m_IP, m_Port, sPostdata);

		if (root.isNull()) {
			m_iMissedQueries++;
			if (m_iMissedQueries % 3 == 0) {
				_log.Log(LOG_ERROR, "Logitech Media Server: No response from server %s:%i", m_IP.c_str(), m_Port);
			}
		}
		else {
			SetHeartbeatReceived();
			m_iMissedQueries = 0;

			int totPlayers = root["player count"].asInt();
			if (totPlayers > 0) {
				if (!m_bShowedStartupMessage)
					_log.Log(LOG_STATUS, "Logitech Media Server: %i connected player(s) found.", totPlayers);
				for (int ii = 0; ii < totPlayers; ii++)
				{
					if (root["players_loop"][ii]["name"].empty())
						continue;

					//int isplayer = root["players_loop"][ii]["isplayer"].asInt();
					std::string name = root["players_loop"][ii]["name"].asString();
					std::string model = root["players_loop"][ii]["model"].asString();
					std::string ipport = root["players_loop"][ii]["ip"].asString();
					std::string macaddress = root["players_loop"][ii]["playerid"].asString();
					std::vector<std::string> IPPort;
					StringSplit(ipport, ":", IPPort);
					if (IPPort.size() < 2)
						continue; //invalid ip:port
					std::string ip = IPPort[0];
					//int port = atoi(IPPort[1].c_str());

					if (
						//(model == "slimp3") ||			//SliMP3
						(model == "Squeezebox") ||			//Squeezebox 1
						(model == "squeezebox2") ||			//Squeezebox 2
						(model == "squeezebox3") ||			//Squeezebox 3
						(model == "transporter") ||			//Transporter
						(model == "receiver") ||			//Squeezebox Receiver
						(model == "boom") ||				//Squeezebox Boom
						//(model == "softsqueeze") ||		//Softsqueeze
						(model == "controller") ||			//Squeezebox Controller
						(model == "squeezeplay") ||			//SqueezePlay
						(model == "squeezeplayer") ||		//SqueezePlay
						(model == "baby") ||				//Squeezebox Radio
						(model == "fab4") ||				//Squeezebox Touch
						(model == "iPengiPod") ||			//iPeng iPhone App
						(model == "iPengiPad") ||			//iPeng iPad App
						(model == "squeezelite")			//Max2Play SqueezePlug
						)
					{
						InsertUpdatePlayer(name, ip, macaddress);
					}
					else {
						if (!m_bShowedStartupMessage)
							_log.Log(LOG_ERROR, "Logitech Media Server: model '%s' not supported.", model.c_str());
					}
				}
			}
			else {
				if (!m_bShowedStartupMessage)
					_log.Log(LOG_ERROR, "Logitech Media Server: No connected players found.");
			}
			m_bShowedStartupMessage = true;
		}
	}
	catch (...)
	{
	}
}

void CLogitechMediaServer::InsertUpdatePlayer(const std::string &Name, const std::string &IPAddress, const std::string &MacAddress)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists...
	//1) Check on MacAddress (default); Update Name in case it has been changed
	result = m_sql.safe_query("SELECT Name FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, MacAddress.c_str());
	if (result.size() > 0) {
		if (result[0][0].c_str() != Name) {
			m_sql.safe_query("UPDATE WOLNodes SET Name='%q' WHERE (HardwareID==%d) AND (MacAddress=='%q')", Name.c_str(), m_HwdID, MacAddress.c_str());
			_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' renamed to '%s'", result[0][0].c_str(), Name.c_str());
		}
		return;
	}

	//2) Check on Name+IP (old format); Convert IP -> MacAddress
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.size()>0) {
		m_sql.safe_query("UPDATE WOLNodes SET MacAddress='%q', Timeout=0 WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", MacAddress.c_str(), m_HwdID, Name.c_str(), IPAddress.c_str());
		_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' IP changed to MacAddress", Name.c_str());
		return;
	}

	//3) Check on Name (old format), IP changed?; Convert IP -> MacAddress
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q')", m_HwdID, Name.c_str());
	if (result.size()>0) {
		m_sql.safe_query("UPDATE WOLNodes SET MacAddress='%q', Timeout=0 WHERE (HardwareID==%d) AND (Name=='%q')", MacAddress.c_str(), m_HwdID, Name.c_str());
		_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' IP changed to MacAddress", Name.c_str());
		return;
	}

	//4) Check on IP (old format), Name changed?; Convert IP -> MacAddress
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, IPAddress.c_str());
	if (result.size()>0) {
		m_sql.safe_query("UPDATE WOLNodes SET Name='%q', MacAddress='%q', Timeout=0 WHERE (HardwareID==%d) AND (MacAddress=='%q')", Name.c_str(), MacAddress.c_str(), m_HwdID, IPAddress.c_str());
		_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' IP changed to MacAddress", Name.c_str());
		return;
	}

	//Player not found, add it...
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (%d, '%q', '%q', 0)", m_HwdID, Name.c_str(), MacAddress.c_str());
	_log.Log(LOG_STATUS, "Logitech Media Server: New Player '%s' added", Name.c_str());

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress='%q')", m_HwdID, MacAddress.c_str());
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

void CLogitechMediaServer::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

void CLogitechMediaServer::Restart()
{
	StopHardware();
	StartHardware();
}

bool CLogitechMediaServer::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;
	std::vector<LogitechMediaServerNode>::const_iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->DevID == DevID)
		{
			int iParam = pSen->LIGHTING2.level;
			std::string sParam;
			switch (pSen->LIGHTING2.cmnd)
			{
			case light2_sOn:
			case light2_sGroupOn:
				return SendCommand(itt->ID, "PowerOn");
			case light2_sOff:
			case light2_sGroupOff:
				return SendCommand(itt->ID, "PowerOff");
			case gswitch_sPlay:
				SendCommand(itt->ID, "NowPlaying");
				return SendCommand(itt->ID, "Play");
			case gswitch_sPlayPlaylist:
				sParam = GetPlaylistByRefID(iParam);
				return SendCommand(itt->ID, "PlayPlaylist", sParam);
			case gswitch_sPlayFavorites:
				return SendCommand(itt->ID, "PlayFavorites");
			case gswitch_sStop:
				return SendCommand(itt->ID, "Stop");
			case gswitch_sPause:
				return SendCommand(itt->ID, "Pause");
			case gswitch_sSetVolume:
				sParam = boost::lexical_cast<std::string>(iParam);
				return SendCommand(itt->ID, "SetVolume", sParam);
			default:
				return true;
			}
		}
	}

	return false;
}

void CLogitechMediaServer::ReloadNodes()
{
	m_nodes.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);
	if (result.size() > 0)
	{
		_log.Log(LOG_STATUS, "Logitech Media Server: %i player-switch(es) found.", result.size());
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			LogitechMediaServerNode pnode;
			pnode.DevID = atoi(sd[0].c_str());
			sprintf(pnode.szDevID, "%X%02X%02X%02X", 0, 0, (pnode.DevID & 0xFF00) >> 8, pnode.DevID & 0xFF);
			pnode.Name = sd[1];
			pnode.IP = sd[2];
			pnode.nStatus = MSTAT_UNKNOWN;
			pnode.sStatus = "";
			pnode.LastOK = mytime(NULL);

			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == 1)", m_HwdID, pnode.szDevID);
			if (result2.size() == 1)
			{
				pnode.ID = atoi(result2[0][0].c_str());
				pnode.nStatus = (_eMediaStatus)atoi(result2[0][1].c_str());
				pnode.sStatus = result2[0][2];
			}

			m_nodes.push_back(pnode);
		}
	}
	else
		_log.Log(LOG_ERROR, "Logitech Media Server: No player-switches found.");
}

void CLogitechMediaServer::ReloadPlaylists()
{
	m_playlists.clear();

	std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"\",[\"playlists\",0,999]]}";
	Json::Value root = Query(m_IP, m_Port, sPostdata);

	if (root.isNull()) {
		_log.Log(LOG_ERROR, "Logitech Media Server: No response from server %s:%i", m_IP.c_str(), m_Port);
	}
	else {
		int totPlaylists = root["count"].asInt();
		if (totPlaylists > 0) {
			_log.Log(LOG_STATUS, "Logitech Media Server: %i playlist(s) found.", totPlaylists);
			for (int ii = 0; ii < totPlaylists; ii++)
			{
				LMSPlaylistNode pnode;

				pnode.ID = root["playlists_loop"][ii]["id"].asInt();
				pnode.Name = root["playlists_loop"][ii]["playlist"].asString();
				pnode.refID = pnode.ID % 256;

				m_playlists.push_back(pnode);
			}
		}
		else
			_log.Log(LOG_STATUS, "Logitech Media Server: No playlists found.");
	}
}

std::string CLogitechMediaServer::GetPlaylistByRefID(const int ID)
{
	std::vector<CLogitechMediaServer::LMSPlaylistNode>::const_iterator itt;

	for (itt = m_playlists.begin(); itt != m_playlists.end(); ++itt) {
		if (itt->refID == ID) return itt->Name;
	}

	_log.Log(LOG_ERROR, "Logitech Media Server: Playlist ID %d not found.", ID);
	return "";
}

bool CLogitechMediaServer::SendCommand(const int ID, const std::string &command, const std::string &param)
{
	std::vector<std::vector<std::string> > result;
	std::string sPlayerId = "";
	std::string sLMSCmnd = "";

	// Get device details
	result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE (ID==%d)", ID);
	if (result.size() == 1)
	{
		// Get connection details
		long	DeviceID = strtol(result[0][0].c_str(), NULL, 16);
		result = m_sql.safe_query("SELECT Name, MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, DeviceID);
		sPlayerId = result[0][1];
	}

	if (result.size() == 1)
	{
		//std::string	sLMSCall;
		if (command == "Left") {
			sLMSCmnd = "\"button\", \"arrow_left\"";
		}
		else if (command == "Right") {
			sLMSCmnd = "\"button\", \"arrow_right\"";
		}
		else if (command == "Up") {
			sLMSCmnd = "\"button\", \"arrow_up\"";
		}
		else if (command == "Down") {
			sLMSCmnd = "\"button\", \"arrow_down\"";
		}
		else if (command == "Favorites") {
			sLMSCmnd = "\"button\", \"favorites\"";
		}
		else if (command == "Browse") {
			sLMSCmnd = "\"button\", \"browse\"";
		}
		else if (command == "NowPlaying") {
			sLMSCmnd = "\"button\", \"playdisp_toggle\"";
		}
		else if (command == "Shuffle") {
			sLMSCmnd = "\"button\", \"shuffle_toggle\"";
		}
		else if (command == "Repeat") {
			sLMSCmnd = "\"button\", \"repeat_toggle\"";
		}
		else if (command == "Stop") {
			sLMSCmnd = "\"button\", \"stop\"";
		}
		else if (command == "VolumeUp") {
			sLMSCmnd = "\"mixer\", \"volume\", \"+2\"";
		}
		else if (command == "Mute") {
			sLMSCmnd = "\"mixer\", \"muting\", \"toggle\"";
		}
		else if (command == "VolumeDown") {
			sLMSCmnd = "\"mixer\", \"volume\", \"-2\"";
		}
		else if (command == "Rewind") {
			sLMSCmnd = "\"button\", \"rew.single\"";
		}
		else if (command == "Play") {
			sLMSCmnd = "\"button\", \"play.single\"";
		}
		else if (command == "PlayPlaylist") {
			if (param == "") return false;
			sLMSCmnd = "\"playlist\", \"play\", \"" + param + "\"";
		}
		else if (command == "PlayFavorites") {
			sLMSCmnd = "\"favorites\", \"playlist\", \"play\"";
		}
		else if (command == "Pause") {
			sLMSCmnd = "\"button\", \"pause.single\"";
		}
		else if (command == "Forward") {
			sLMSCmnd = "\"button\", \"fwd.single\"";
		}
		else if (command == "PowerOn") {
			sLMSCmnd = "\"power\", \"1\"";
		}
		else if (command == "PowerOff") {
			sLMSCmnd = "\"power\", \"0\"";
		}
		else if (command == "SetVolume") {
			if (param == "") return false;
			sLMSCmnd = "\"mixer\", \"volume\", \"" + param + "\"";
		}

		if (sLMSCmnd != "")
		{
			std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + sPlayerId + "\",[" + sLMSCmnd + "]]}";
			Json::Value root = Query(m_IP, m_Port, sPostdata);

			sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + sPlayerId + "\",[\"status\",\"-\",1,\"tags:uB\"]]}";
			root = Query(m_IP, m_Port, sPostdata);

			if (root["player_connected"].asString() == "1")
			{
				std::string sPower = root["power"].asString();
				std::string sMode = root["mode"].asString();

				if (command == "Stop")
					return sMode == "stop";
				else if (command == "Pause")
					return sMode == "pause";
				else if (command == "Play")
					return sMode == "play";
				else if (command == "PlayPlaylist")
					return sMode == "play";
				else if (command == "PlayFavorites")
					return sMode == "play";
				else if (command == "PowerOn")
					return sPower == "1";
				else if (command == "PowerOff")
					return sPower == "0";
				else
					return false;
			}
			else
				return false;
		}
		else
		{
			_log.Log(LOG_ERROR, "Logitech Media Server: (%s) Command: '%s'. Unknown command.", result[0][0].c_str(), command.c_str());
			return false;
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Logitech Media Server: (%d) Command: '%s'. Device not found.", ID, command.c_str());
		return false;
	}
}

void CLogitechMediaServer::SendText(const std::string &playerIP, const std::string &subject, const std::string &text, const int duration)
{
	if ((playerIP != "") && (text != "") && (duration > 0))
	{
		std::string sLine1 = subject;
		std::string sLine2 = text;
		std::string sFont = ""; //"huge";
		std::string sBrightness = "4";
		std::string sDuration = boost::lexical_cast<std::string>(duration);
		std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + playerIP + "\",[\"show\",\"line1:" + sLine1 + "\",\"line2:" + sLine2 + "\",\"duration:" + sDuration + "\",\"brightness:" + sBrightness + "\",\"font:" + sFont + "\"]]}";
		Json::Value root = Query(m_IP, m_Port, sPostdata);
	}
}

std::vector<CLogitechMediaServer::LMSPlaylistNode> CLogitechMediaServer::GetPlaylists()
{
	return m_playlists;
}

int CLogitechMediaServer::GetPlaylistRefID(const std::string &name)
{
	std::vector<CLogitechMediaServer::LMSPlaylistNode>::const_iterator itt;

	for (itt = m_playlists.begin(); itt != m_playlists.end(); ++itt) {
		if (itt->Name == name) return itt->refID;
	}

	_log.Log(LOG_ERROR, "Logitech Media Server: Playlist '%s' not found.", name.c_str());
	return 0;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_LMSSetMode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			std::string mode1 = request::findValue(&req, "mode1");
			std::string mode2 = request::findValue(&req, "mode2");
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
			if (pBaseHardware->HwdType != HTYPE_LogitechMediaServer)
				return;
			CLogitechMediaServer *pHardware = reinterpret_cast<CLogitechMediaServer*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "LMSSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}

		void CWebServer::Cmd_LMSGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_LogitechMediaServer)
				return;

			root["status"] = "OK";
			root["title"] = "LMSGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress FROM WOLNodes WHERE (HardwareID==%d)", iHardwareID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Mac"] = sd[2];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_LMSGetPlaylists(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_LogitechMediaServer)
				return;
			CLogitechMediaServer *pHardware = reinterpret_cast<CLogitechMediaServer*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "Cmd_LMSGetPlaylists";

			std::vector<CLogitechMediaServer::LMSPlaylistNode> m_nodes = pHardware->GetPlaylists();
			std::vector<CLogitechMediaServer::LMSPlaylistNode>::const_iterator itt;

			int ii = 0;
			for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt) {
				root["result"][ii]["id"] = itt->ID;
				root["result"][ii]["refid"] = itt->refID;
				root["result"][ii]["Name"] = itt->Name;
				ii++;
			}
		}

		void CWebServer::Cmd_LMSMediaCommand(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
			if (sIdx.empty())
				return;
			int idx = atoi(sIdx.c_str());
			root["status"] = "OK";
			root["title"] = "LMSMediaCommand";

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
					case HTYPE_LogitechMediaServer:
						CLogitechMediaServer LMS(HwID);
						LMS.SendCommand(idx, sAction);
						break;
						// put other players here ...
					}
				}
			}
		}

	}
}
